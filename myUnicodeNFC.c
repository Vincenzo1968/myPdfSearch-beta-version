#include "myUnicodeNFC.h"
#include "unicode_nfc_data.h"

#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#if WCHAR_MAX < 0x10FFFF
#error "myUnicodeNFC requires a wchar_t type capable of representing Unicode scalar values"
#endif

#define MY_NFC_MAX_DECOMPOSITION_DEPTH 64u
#define MY_NFC_NO_STARTER ((size_t)-1)

typedef struct MyNfcBuffer {
    uint32_t *data;
    size_t length;
    size_t capacity;
} MyNfcBuffer;

static int myNfcIsScalar(uint32_t cp)
{
    return cp <= 0x10FFFFu && !(cp >= 0xD800u && cp <= 0xDFFFu);
}

static int myNfcReserve(MyNfcBuffer *buffer, size_t additional)
{
    size_t needed;
    size_t new_capacity;
    uint32_t *new_data;

    if (additional > SIZE_MAX - buffer->length)
        return 0;

    needed = buffer->length + additional;
    if (needed <= buffer->capacity)
        return 1;

    new_capacity = (buffer->capacity != 0) ? buffer->capacity : 32u;
    while (new_capacity < needed) {
        if (new_capacity > SIZE_MAX / 2u) {
            new_capacity = needed;
            break;
        }
        new_capacity *= 2u;
    }

    if (new_capacity > SIZE_MAX / sizeof(*buffer->data))
        return 0;

    new_data = (uint32_t *)realloc(buffer->data,
                                   new_capacity * sizeof(*buffer->data));
    if (new_data == NULL)
        return 0;

    buffer->data = new_data;
    buffer->capacity = new_capacity;
    return 1;
}

static int myNfcAppend(MyNfcBuffer *buffer, uint32_t cp)
{
    if (!myNfcIsScalar(cp))
        return -1;
    if (!myNfcReserve(buffer, 1u))
        return 0;
    buffer->data[buffer->length++] = cp;
    return 1;
}

static const MyNfcDecompEntry *myNfcFindDecomposition(uint32_t cp)
{
    size_t low = 0;
    size_t high = (size_t)MY_NFC_DECOMP_COUNT;

    while (low < high) {
        size_t mid = low + (high - low) / 2u;
        uint32_t key = my_nfc_decomp[mid].codepoint;

        if (key < cp)
            low = mid + 1u;
        else
            high = mid;
    }

    if (low < (size_t)MY_NFC_DECOMP_COUNT &&
        my_nfc_decomp[low].codepoint == cp)
        return &my_nfc_decomp[low];

    return NULL;
}

static uint8_t myNfcGetCombiningClass(uint32_t cp)
{
    size_t low = 0;
    size_t high = (size_t)MY_NFC_CCC_COUNT;

    while (low < high) {
        size_t mid = low + (high - low) / 2u;
        uint32_t key = my_nfc_ccc[mid].codepoint;

        if (key < cp)
            low = mid + 1u;
        else
            high = mid;
    }

    if (low < (size_t)MY_NFC_CCC_COUNT &&
        my_nfc_ccc[low].codepoint == cp)
        return my_nfc_ccc[low].ccc;

    return 0u;
}

static int myNfcFindCompositionPair(uint32_t first, uint32_t second,
                                    uint32_t *composite)
{
    size_t low = 0;
    size_t high = (size_t)MY_NFC_COMPOSE_COUNT;

    while (low < high) {
        size_t mid = low + (high - low) / 2u;
        const MyNfcComposeEntry *entry = &my_nfc_compose[mid];

        if (entry->first < first ||
            (entry->first == first && entry->second < second)) {
            low = mid + 1u;
        } else {
            high = mid;
        }
    }

    if (low < (size_t)MY_NFC_COMPOSE_COUNT &&
        my_nfc_compose[low].first == first &&
        my_nfc_compose[low].second == second) {
        *composite = my_nfc_compose[low].composite;
        return 1;
    }

    return 0;
}

/* Unicode Standard, section 3.12: algorithmic Hangul decomposition. */
static int myNfcDecomposeHangul(uint32_t cp, MyNfcBuffer *buffer)
{
    const uint32_t S_BASE = 0xAC00u;
    const uint32_t L_BASE = 0x1100u;
    const uint32_t V_BASE = 0x1161u;
    const uint32_t T_BASE = 0x11A7u;
    const uint32_t L_COUNT = 19u;
    const uint32_t V_COUNT = 21u;
    const uint32_t T_COUNT = 28u;
    const uint32_t N_COUNT = V_COUNT * T_COUNT;
    const uint32_t S_COUNT = L_COUNT * N_COUNT;
    uint32_t s_index;
    uint32_t l_part;
    uint32_t v_part;
    uint32_t t_index;
    int rc;

    if (cp < S_BASE || cp >= S_BASE + S_COUNT)
        return 0;

    s_index = cp - S_BASE;
    l_part = L_BASE + s_index / N_COUNT;
    v_part = V_BASE + (s_index % N_COUNT) / T_COUNT;
    t_index = s_index % T_COUNT;

    rc = myNfcAppend(buffer, l_part);
    if (rc <= 0)
        return rc;
    rc = myNfcAppend(buffer, v_part);
    if (rc <= 0)
        return rc;
    if (t_index != 0u) {
        rc = myNfcAppend(buffer, T_BASE + t_index);
        if (rc <= 0)
            return rc;
    }

    return 1;
}

/* Recursive canonical decomposition, including algorithmic Hangul. */
static MyUnicodeNFCStatus myNfcDecompose(uint32_t cp,
                                          MyNfcBuffer *buffer,
                                          unsigned int depth)
{
    const MyNfcDecompEntry *entry;
    uint32_t i;
    int hangul_rc;

    if (!myNfcIsScalar(cp))
        return MY_UNICODE_NFC_INVALID_SCALAR;

    if (depth > MY_NFC_MAX_DECOMPOSITION_DEPTH)
        return MY_UNICODE_NFC_INVALID_TABLE;

    hangul_rc = myNfcDecomposeHangul(cp, buffer);
    if (hangul_rc > 0)
        return MY_UNICODE_NFC_OK;
    if (hangul_rc < 0)
        return MY_UNICODE_NFC_OUT_OF_MEMORY;

    entry = myNfcFindDecomposition(cp);
    if (entry == NULL) {
        int append_rc = myNfcAppend(buffer, cp);
        if (append_rc > 0)
            return MY_UNICODE_NFC_OK;
        if (append_rc < 0)
            return MY_UNICODE_NFC_INVALID_SCALAR;
        return MY_UNICODE_NFC_OUT_OF_MEMORY;
    }

    if (entry->length == 0u ||
        (size_t)entry->offset > (size_t)MY_NFC_DECOMP_DATA_COUNT ||
        (size_t)entry->length >
            (size_t)MY_NFC_DECOMP_DATA_COUNT - (size_t)entry->offset)
        return MY_UNICODE_NFC_INVALID_TABLE;

    for (i = 0u; i < (uint32_t)entry->length; ++i) {
        uint32_t mapped =
            my_nfc_decomp_data[(size_t)entry->offset + (size_t)i];
        {
            MyUnicodeNFCStatus status =
                myNfcDecompose(mapped, buffer, depth + 1u);
            if (status != MY_UNICODE_NFC_OK)
                return status;
        }
    }

    return MY_UNICODE_NFC_OK;
}

/*
 * Canonical ordering: stable counting sort of each consecutive run of
 * non-starters. Canonical Combining Class is an 8-bit value, so this is
 * linear in the input length and avoids quadratic behavior on long mark runs.
 */
static int myNfcCanonicalOrder(MyNfcBuffer *buffer)
{
    uint32_t *scratch;
    size_t i = 0u;

    if (buffer->length < 2u)
        return 1;
    if (buffer->length > SIZE_MAX / sizeof(*scratch))
        return 0;

    scratch = (uint32_t *)malloc(buffer->length * sizeof(*scratch));
    if (scratch == NULL)
        return 0;

    while (i < buffer->length) {
        size_t start;
        size_t end;
        size_t counts[256] = { 0u };
        size_t positions[256] = { 0u };
        size_t j;
        size_t offset = 0u;

        /* A starter (CCC=0) is already correctly positioned. */
        if (myNfcGetCombiningClass(buffer->data[i]) == 0u) {
            ++i;
            continue;
        }

        start = i;
        while (i < buffer->length &&
               myNfcGetCombiningClass(buffer->data[i]) != 0u) {
            ++i;
        }
        end = i;

        if (end - start < 2u)
            continue;

        for (j = start; j < end; ++j) {
            uint8_t ccc = myNfcGetCombiningClass(buffer->data[j]);
            ++counts[ccc];
        }

        /* Prefix positions for CCC values 1..255; position 0 is unused. */
        for (j = 1u; j < 256u; ++j) {
            positions[j] = offset;
            offset += counts[j];
        }

        /* Stable distribution preserves the order of equal CCC values. */
        for (j = start; j < end; ++j) {
            uint8_t ccc = myNfcGetCombiningClass(buffer->data[j]);
            scratch[start + positions[ccc]++] = buffer->data[j];
        }

        for (j = start; j < end; ++j)
            buffer->data[j] = scratch[j];
    }

    free(scratch);
    return 1;
}

/* Algorithmic Hangul composition, followed by the generated pair table. */
static int myNfcComposePair(uint32_t first, uint32_t second,
                            uint32_t *composite)
{
    const uint32_t S_BASE = 0xAC00u;
    const uint32_t L_BASE = 0x1100u;
    const uint32_t V_BASE = 0x1161u;
    const uint32_t T_BASE = 0x11A7u;
    const uint32_t L_COUNT = 19u;
    const uint32_t V_COUNT = 21u;
    const uint32_t T_COUNT = 28u;
    const uint32_t N_COUNT = V_COUNT * T_COUNT;
    const uint32_t S_COUNT = L_COUNT * N_COUNT;

    /* L Jamo + V Jamo -> LV Hangul syllable. */
    if (first >= L_BASE && first < L_BASE + L_COUNT &&
        second >= V_BASE && second < V_BASE + V_COUNT) {
        uint32_t l_index = first - L_BASE;
        uint32_t v_index = second - V_BASE;
        *composite = S_BASE + (l_index * V_COUNT + v_index) * T_COUNT;
        return 1;
    }

    /* LV Hangul syllable without a final consonant + T Jamo -> LVT. */
    if (first >= S_BASE && first < S_BASE + S_COUNT &&
        ((first - S_BASE) % T_COUNT) == 0u &&
        second > T_BASE && second < T_BASE + T_COUNT) {
        *composite = first + (second - T_BASE);
        return 1;
    }

    return myNfcFindCompositionPair(first, second, composite);
}

/* Canonical composition, performed in place after decomposition and ordering. */
static void myNfcCanonicalCompose(MyNfcBuffer *buffer)
{
    size_t read_pos;
    size_t write_pos = 0u;
    size_t starter_pos = MY_NFC_NO_STARTER;
    uint32_t starter = 0u;
    uint8_t last_ccc = 0u;

    for (read_pos = 0u; read_pos < buffer->length; ++read_pos) {
        uint32_t current = buffer->data[read_pos];
        uint8_t current_ccc = myNfcGetCombiningClass(current);
        uint32_t composite;

        if (starter_pos != MY_NFC_NO_STARTER &&
            (last_ccc == 0u || last_ccc < current_ccc) &&
            myNfcComposePair(starter, current, &composite)) {
            buffer->data[starter_pos] = composite;
            starter = composite;
            /* A successful composition does not change last_ccc. */
            continue;
        }

        if (current_ccc == 0u) {
            starter_pos = write_pos;
            starter = current;
        }

        last_ccc = current_ccc;
        buffer->data[write_pos++] = current;
    }

    buffer->length = write_pos;
}

MyUnicodeNFCStatus myUnicodeNFC_Normalize(
    const wchar_t *input,
    size_t input_length,
    wchar_t **output,
    size_t *output_length)
{
    MyNfcBuffer buffer = { NULL, 0u, 0u };
    wchar_t *result = NULL;
    size_t i;
    MyUnicodeNFCStatus status = MY_UNICODE_NFC_OK;

    if (output != NULL)
        *output = NULL;
    if (output_length != NULL)
        *output_length = 0u;

    if (output == NULL || output_length == NULL ||
        (input == NULL && input_length != 0u))
        return MY_UNICODE_NFC_INVALID_ARGUMENT;

    for (i = 0u; i < input_length; ++i) {
        uintmax_t raw = (uintmax_t)input[i];
        uint32_t cp;

        if (raw > 0x10FFFFu) {
            status = MY_UNICODE_NFC_INVALID_SCALAR;
            goto cleanup;
        }
        cp = (uint32_t)raw;
        if (!myNfcIsScalar(cp)) {
            status = MY_UNICODE_NFC_INVALID_SCALAR;
            goto cleanup;
        }

        status = myNfcDecompose(cp, &buffer, 0u);
        if (status != MY_UNICODE_NFC_OK)
            goto cleanup;
    }

    if (!myNfcCanonicalOrder(&buffer)) {
        status = MY_UNICODE_NFC_OUT_OF_MEMORY;
        goto cleanup;
    }
    myNfcCanonicalCompose(&buffer);

    if (buffer.length == SIZE_MAX ||
        buffer.length + 1u > SIZE_MAX / sizeof(*result)) {
        status = MY_UNICODE_NFC_OUT_OF_MEMORY;
        goto cleanup;
    }

    result = (wchar_t *)malloc((buffer.length + 1u) * sizeof(*result));
    if (result == NULL) {
        status = MY_UNICODE_NFC_OUT_OF_MEMORY;
        goto cleanup;
    }

    for (i = 0u; i < buffer.length; ++i)
        result[i] = (wchar_t)buffer.data[i];
    result[buffer.length] = L'\0';

    *output = result;
    *output_length = buffer.length;
    result = NULL;

cleanup:
    free(result);
    free(buffer.data);
    return status;
}

const char *myUnicodeNFC_UnicodeVersion(void)
{
    return MY_UNICODE_NFC_UNICODE_VERSION;
}

const char *myUnicodeNFC_StatusString(MyUnicodeNFCStatus status)
{
    switch (status) {
        case MY_UNICODE_NFC_OK:
            return "success";
        case MY_UNICODE_NFC_INVALID_ARGUMENT:
            return "invalid argument";
        case MY_UNICODE_NFC_INVALID_SCALAR:
            return "invalid Unicode scalar value";
        case MY_UNICODE_NFC_OUT_OF_MEMORY:
            return "out of memory";
        case MY_UNICODE_NFC_INVALID_TABLE:
            return "invalid or inconsistent NFC data table";
        default:
            return "unknown NFC error";
    }
}
