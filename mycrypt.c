#include "scanner.h"
#include "mycrypt_rc4.h"
#include "mycrypt.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/provider.h>
#include <openssl/rand.h>

#define PDF_PASSWORD_PADDING_LEN 32U
#define PDF_MAX_PASSWORD_R56 127U
#define PDF_MAX_OBJECT_KEY_LEN 16U

static const unsigned char pdf_password_padding[PDF_PASSWORD_PADDING_LEN] = {
    0x28,0xbf,0x4e,0x5e,0x4e,0x75,0x8a,0x41,
    0x64,0x00,0x4e,0x56,0xff,0xfa,0x01,0x08,
    0x2e,0x2e,0x00,0xb6,0xd0,0x68,0x3e,0x80,
    0x2f,0x0c,0xa9,0xfe,0x64,0x53,0x69,0x7a
};

typedef struct CryptoEnv
{
    OSSL_LIB_CTX *libctx;
    OSSL_PROVIDER *default_provider;
    OSSL_PROVIDER *legacy_provider;
} CryptoEnv;

static void crypto_env_destroy(CryptoEnv *env);

static void set_error(PdfCryptoParams *cp, const char *message)
{
    if (cp == NULL)
		return;

    (void)snprintf(cp->szError, sizeof(cp->szError), "%s", message ? message : "Errore crittografico.");
}

void PdfCryptoParams_FreeOutput(PdfCryptoParams *cp)
{
    if (cp == NULL) return;

    if (cp->pszStream != NULL && cp->lenStreamValue != 0U)
        OPENSSL_cleanse(cp->pszStream, (size_t)cp->lenStreamValue);

    free(cp->pszStream);
    cp->pszStream = NULL;

    cp->lenStreamValue = 0;

    cp->lenStream = &cp->lenStreamValue;
}

/*
 * Inizializzazione del provider default nel contesto globale.
 *
 * Il riferimento viene conservato per la vita del processo:
 * non deve essere liberato dal cleanup delle singole operazioni,
 * perché altri thread potrebbero utilizzare ancora OpenSSL.
 */
static CRYPTO_ONCE crypto_global_once = CRYPTO_ONCE_STATIC_INIT;
static OSSL_PROVIDER *crypto_global_default_provider = NULL;
static int crypto_global_ready = 0;

static void crypto_global_init_once(void)
{
    crypto_global_default_provider =
        OSSL_PROVIDER_load(NULL, "default");

    crypto_global_ready =
        (crypto_global_default_provider != NULL);
}

static int crypto_global_init(void)
{
    if (CRYPTO_THREAD_run_once(
            &crypto_global_once,
            crypto_global_init_once) != 1)
        return 0;

    return crypto_global_ready;
}

int PdfCryptoInitialize(void)
{
    return crypto_global_init();
}

int PdfCryptoShutdown(void)
{
    if (crypto_global_default_provider == NULL)
    {
        crypto_global_ready = 0;
        return 1;
    }

    if (OSSL_PROVIDER_unload(crypto_global_default_provider) != 1)
        return 0;

    crypto_global_default_provider = NULL;
    crypto_global_ready = 0;

    return 1;
}

static int crypto_env_init(CryptoEnv *env)
{
    if (env == NULL)
        return 0;

    memset(env, 0, sizeof(*env));

    /* Completa l'inizializzazione globale prima di creare il contesto locale. */
    if (!crypto_global_init())
        goto errore;

    env->libctx = OSSL_LIB_CTX_new();
    if (env->libctx == NULL)
        goto errore;

    env->default_provider =
        OSSL_PROVIDER_load(env->libctx, "default");

    if (env->default_provider == NULL)
        goto errore;

    // RC4 viene cercato esclusivamente nel provider legacy.
    // AES e gli hash utilizzano il provider default.
    // Se legacy non è disponibile, gli algoritmi del provider
    // default possono comunque essere utilizzati.

    // NON PIÙ NECESSARIO: USIAMO mycrypt_rc4.c e mycrypt_rc4.h
    //env->legacy_provider = OSSL_PROVIDER_load(env->libctx, "legacy");

    return 1;

errore:
    crypto_env_destroy(env);
    return 0;
}

static void crypto_env_destroy(CryptoEnv *env)
{
    if (env == NULL)
		return;

    // NON PIÙ NECESSARIO: USIAMO mycrypt_rc4.c e mycrypt_rc4.h
    //if (env->legacy_provider != NULL) OSSL_PROVIDER_unload(env->legacy_provider);

    if (env->default_provider != NULL)
		OSSL_PROVIDER_unload(env->default_provider);

    if (env->libctx != NULL)
		OSSL_LIB_CTX_free(env->libctx);

    memset(env, 0, sizeof(*env));
}

static int hash_parts(CryptoEnv *env, const char *name, const unsigned char *const parts[], const size_t lengths[], size_t count, unsigned char *out, size_t *out_len)
{
    EVP_MD *md = NULL;
    EVP_MD_CTX *ctx = NULL;
    unsigned int produced = 0;
    size_t i;
    int ok = 0;

    md = EVP_MD_fetch(env->libctx, name, "provider=default");

    ctx = EVP_MD_CTX_new();

    if (md == NULL || ctx == NULL || EVP_DigestInit_ex2(ctx, md, NULL) != 1)
		goto done;

    for (i = 0; i < count; ++i)
    {
        if (lengths[i] != 0 && parts[i] == NULL)
			goto done;

        if (lengths[i] != 0 && EVP_DigestUpdate(ctx, parts[i], lengths[i]) != 1)
			goto done;
    }

    if (EVP_DigestFinal_ex(ctx, out, &produced) != 1)
		goto done;

    if (out_len != NULL)
		*out_len = (size_t)produced;

    ok = 1;

done:

    EVP_MD_CTX_free(ctx);
    EVP_MD_free(md);

    return ok;
}

static int digest_one(CryptoEnv *env, const char *name, const unsigned char *data, size_t data_len, unsigned char *out, size_t *out_len)
{
    const unsigned char *parts[1] = { data };

    const size_t lengths[1] = { data_len };

    return hash_parts(env, name, parts, lengths, 1, out, out_len);
}

/*
static int rc4_crypt(CryptoEnv *env, const unsigned char *key, size_t key_len,
                     const unsigned char *input, size_t input_len, unsigned char *output)
{
    EVP_CIPHER *cipher = NULL;
    EVP_CIPHER_CTX *ctx = NULL;
    int out1 = 0, out2 = 0;
    int ok = 0;

    if (env->legacy_provider == NULL) return 0;
    if (key == NULL || key_len == 0 || key_len > INT_MAX) return 0;
    cipher = EVP_CIPHER_fetch(env->libctx, "RC4", "provider=legacy");
    ctx = EVP_CIPHER_CTX_new();
    if (cipher == NULL || ctx == NULL) goto done;
    if (EVP_CipherInit_ex2(ctx, cipher, NULL, NULL, 1, NULL) != 1) goto done;
    if (EVP_CIPHER_CTX_set_key_length(ctx, (int)key_len) != 1) goto done;
    if (EVP_CipherInit_ex2(ctx, NULL, key, NULL, 1, NULL) != 1) goto done;
    if (input_len > INT_MAX) goto done;
    if (input_len != 0 && EVP_CipherUpdate(ctx, output, &out1, input, (int)input_len) != 1) goto done;
    if (EVP_CipherFinal_ex(ctx, output + out1, &out2) != 1) goto done;
    ok = ((size_t)(out1 + out2) == input_len);
done:
    EVP_CIPHER_CTX_free(ctx);
    EVP_CIPHER_free(cipher);
    return ok;
}
*/

static int rc4_crypt(CryptoEnv *env, const unsigned char *key, size_t key_len, const unsigned char *input, size_t input_len, unsigned char *output)
{
    (void)env; // Non serve più il contesto OpenSSL per RC4

    if (key == NULL || key_len == 0)
        return 0;

    if (input_len == 0)
        return 1;

    if (input == NULL || output == NULL)
        return 0;

    // Invoca l'algoritmo nativo: senza allocazioni heap.
	if (!my_rc4_process(key, key_len, input, output, input_len))
	{
		return 0;
	}

	return 1;
}

static const char *aes_cipher_name(size_t key_len, int ecb)
{
    if (key_len == 16) return ecb ? "AES-128-ECB" : "AES-128-CBC";
    if (key_len == 32) return ecb ? "AES-256-ECB" : "AES-256-CBC";
    return NULL;
}

static int aes_crypt(CryptoEnv *env, int encrypt, const unsigned char *key, size_t key_len, const unsigned char *iv, const unsigned char *input, size_t input_len, unsigned char *output, size_t *output_len, int padding, int ecb)
{
    EVP_CIPHER *cipher = NULL;

    EVP_CIPHER_CTX *ctx = NULL;

    const char *name = aes_cipher_name(key_len, ecb);

    int out1 = 0, out2 = 0;

    int ok = 0;

    if (name == NULL || key == NULL || (input_len != 0 && input == NULL) || output == NULL)
		return 0;

    if (!ecb && iv == NULL)
		return 0;

    if (input_len > INT_MAX)
		return 0;

    cipher = EVP_CIPHER_fetch(env->libctx, name, "provider=default");

    ctx = EVP_CIPHER_CTX_new();

    if (cipher == NULL || ctx == NULL)
		goto done;

    if (EVP_CipherInit_ex2(ctx, cipher, key, ecb ? NULL : iv, encrypt, NULL) != 1)
		goto done;

    if (EVP_CIPHER_CTX_set_padding(ctx, padding ? 1 : 0) != 1)
		goto done;

    if (input_len != 0 && EVP_CipherUpdate(ctx, output, &out1, input, (int)input_len) != 1)
		goto done;

    if (EVP_CipherFinal_ex(ctx, output + out1, &out2) != 1)
		goto done;

    if (output_len != NULL)
		*output_len = (size_t)(out1 + out2);

    ok = 1;

done:

    EVP_CIPHER_CTX_free(ctx);
    EVP_CIPHER_free(cipher);

    return ok;
}

static void pad_legacy_password(const unsigned char *password, size_t password_len, unsigned char out[PDF_PASSWORD_PADDING_LEN])
{
    size_t copy_len = password_len < PDF_PASSWORD_PADDING_LEN ? password_len : PDF_PASSWORD_PADDING_LEN;

    if (copy_len != 0 && password != NULL)
		memcpy(out, password, copy_len);

    if (copy_len < PDF_PASSWORD_PADDING_LEN)
		memcpy(out + copy_len, pdf_password_padding, PDF_PASSWORD_PADDING_LEN - copy_len);
}

static int legacy_key_length(const PdfCryptoParams *cp, size_t *key_len)
{
    int bits = cp->keyLengthBits;

    if (cp->V <= 1 || cp->R == 2)
		bits = 40;

    if (bits == 0)
		bits = 40;

    if (bits < 40 || bits > 128 || (bits % 8) != 0)
		return 0;

    *key_len = (size_t)bits / 8U;

    return 1;
}

static int derive_legacy_file_key(CryptoEnv *env, const PdfCryptoParams *cp, const unsigned char *password, size_t password_len, unsigned char key[16], size_t *key_len)
{
    unsigned char padded[32], p_bytes[4], digest[EVP_MAX_MD_SIZE];

    const unsigned char metadata_no[4] = {0xff,0xff,0xff,0xff};

    const unsigned char *parts[5];

    size_t lengths[5], dlen = 0, wanted;

    int rounds, i;

    uint32_t pbits;

    int ok = 0;

    if (!legacy_key_length(cp, &wanted))
		return 0;

    if (cp->lenO < 32 || cp->pO == NULL)
		return 0;

    pad_legacy_password(password, password_len, padded);

    pbits = (uint32_t)cp->P;

    p_bytes[0] = (unsigned char)(pbits & 0xffU);
    p_bytes[1] = (unsigned char)((pbits >> 8) & 0xffU);
    p_bytes[2] = (unsigned char)((pbits >> 16) & 0xffU);
    p_bytes[3] = (unsigned char)((pbits >> 24) & 0xffU);

    parts[0] = padded; lengths[0] = sizeof(padded);
    parts[1] = cp->pO; lengths[1] = 32U;
    parts[2] = p_bytes; lengths[2] = sizeof(p_bytes);
    parts[3] = cp->pID0; lengths[3] = cp->lenID0;
    parts[4] = metadata_no; lengths[4] = (cp->R >= 4 && cp->hasEncryptMetadata && !cp->encryptMetadata) ? 4U : 0U;

    if (!hash_parts(env, "MD5", parts, lengths, 5, digest, &dlen) || dlen < 16)
		goto done;

    rounds = cp->R >= 3 ? 50 : 0;

    for (i = 0; i < rounds; ++i)
    {
        if (!digest_one(env, "MD5", digest, wanted, digest, &dlen) || dlen < 16)
			goto done;
    }

    memcpy(key, digest, wanted);

    *key_len = wanted;

    ok = 1;

done:

    OPENSSL_cleanse(padded, sizeof(padded));
    OPENSSL_cleanse(digest, sizeof(digest));

    return ok;
}

static int rc4_repeat(CryptoEnv *env, unsigned char *data, size_t data_len, const unsigned char *base_key, size_t key_len, int reverse)
{
    unsigned char *tmp = NULL;
    unsigned char key[16];
    int i;
    int ok = 0;

    tmp = (unsigned char *)malloc(data_len ? data_len : 1U);

    if (tmp == NULL || key_len == 0 || key_len > sizeof(key))
		goto done;

    for (i = 0; i < 20; ++i)
    {
        size_t j;

        int v = reverse ? (19 - i) : i;

        for (j = 0; j < key_len; ++j)
			key[j] = (unsigned char)(base_key[j] ^ (unsigned char)v);

        if (!rc4_crypt(env, key, key_len, data, data_len, tmp))
			goto done;

        if (data_len != 0)
			memcpy(data, tmp, data_len);
    }

    ok = 1;

done:

    OPENSSL_cleanse(key, sizeof(key));

    if (tmp != NULL)
    {
		OPENSSL_cleanse(tmp, data_len); free(tmp);
	}

    return ok;
}

static int check_legacy_user(CryptoEnv *env, const PdfCryptoParams *cp, const unsigned char *password, size_t password_len, unsigned char out_key[32], size_t *out_key_len)
{
    unsigned char key[16], padded[32], ucalc[32], digest[EVP_MAX_MD_SIZE];

    const unsigned char *parts[2];

    size_t lengths[2], key_len = 0, dlen = 0;

    int ok = 0;

    if (cp->lenU < 32 || cp->pU == NULL)
		goto done;

    if (!derive_legacy_file_key(env, cp, password, password_len, key, &key_len))
		goto done;

    if (cp->R == 2)
    {
        memcpy(padded, pdf_password_padding, 32);

        if (!rc4_crypt(env, key, key_len, padded, 32, ucalc))
			goto done;

        if (CRYPTO_memcmp(ucalc, cp->pU, 32) != 0)
			goto done;
    }
    else
    {
        parts[0] = pdf_password_padding; lengths[0] = 32;
        parts[1] = cp->pID0; lengths[1] = cp->lenID0;

        if (!hash_parts(env, "MD5", parts, lengths, 2, digest, &dlen) || dlen < 16)
			goto done;

        memcpy(ucalc, digest, 16);

        if (!rc4_repeat(env, ucalc, 16, key, key_len, 0))
			goto done;

        if (CRYPTO_memcmp(ucalc, cp->pU, 16) != 0)
			goto done;
    }

    memcpy(out_key, key, key_len);
    *out_key_len = key_len;
    ok = 1;

done:

    OPENSSL_cleanse(key, sizeof(key));
    OPENSSL_cleanse(padded, sizeof(padded));
    OPENSSL_cleanse(ucalc, sizeof(ucalc));
    OPENSSL_cleanse(digest, sizeof(digest));

    return ok;
}

static int check_legacy_owner(CryptoEnv *env, const PdfCryptoParams *cp, const unsigned char *password, size_t password_len, unsigned char out_key[32], size_t *out_key_len)
{
    unsigned char padded[32], owner_key[EVP_MAX_MD_SIZE], user_pad[32];

    unsigned char md[EVP_MAX_MD_SIZE];

    size_t owner_key_len = 0, md_len = 0, file_key_len = 0;

    size_t wanted = 0;

    int i, rounds, ok = 0;

    if (cp->lenO < 32 || cp->pO == NULL || cp->lenU < 32 || cp->pU == NULL)
		goto done;

    if (!legacy_key_length(cp, &wanted))
		goto done;

    pad_legacy_password(password, password_len, padded);

    if (!digest_one(env, "MD5", padded, 32, owner_key, &owner_key_len) || owner_key_len < 16)
		goto done;

    rounds = cp->R >= 3 ? 50 : 0;

    for (i = 0; i < rounds; ++i)
    {
        if (!digest_one(env, "MD5", owner_key, wanted, md, &md_len))
			goto done;
        memcpy(owner_key, md, md_len);
        owner_key_len = md_len;
    }

    memcpy(user_pad, cp->pO, 32);

    if (cp->R >= 3)
    {
        if (!rc4_repeat(env, user_pad, 32, owner_key, wanted, 1))
			goto done;
    }
    else
    {
        if (!rc4_crypt(env, owner_key, wanted, user_pad, 32, md))
			goto done;

        memcpy(user_pad, md, 32);
    }

    if (!check_legacy_user(env, cp, user_pad, 32, out_key, &file_key_len))
		goto done;

    *out_key_len = file_key_len;

    ok = 1;

done:

    OPENSSL_cleanse(padded, sizeof(padded));
    OPENSSL_cleanse(owner_key, sizeof(owner_key));
    OPENSSL_cleanse(user_pad, sizeof(user_pad));
    OPENSSL_cleanse(md, sizeof(md));

    return ok;
}

static int hash_r56(CryptoEnv *env, int revision,
                    const unsigned char *password, size_t password_len,
                    const unsigned char salt[8],
                    const unsigned char *user_data, size_t user_data_len,
                    unsigned char out[32])
{
    const unsigned char *parts[3];
    size_t lens[3], k_len = 0;
    unsigned char k[EVP_MAX_MD_SIZE], next[EVP_MAX_MD_SIZE];
    unsigned char *round_input = NULL, *e = NULL;
    size_t round_base_len, max_base_len, e_len = 0, i, j;
    unsigned int round_no = 0;
    int sum_mod3, ok = 0;
    const char *hash_name;

    if (password_len > PDF_MAX_PASSWORD_R56)
		password_len = PDF_MAX_PASSWORD_R56;

    parts[0] = password; lens[0] = password_len;
    parts[1] = salt; lens[1] = 8;
    parts[2] = user_data; lens[2] = user_data_len;

    if (!hash_parts(env, "SHA256", parts, lens, 3, k, &k_len) || k_len != 32)
		goto done;

    if (revision == 5)
    {
		memcpy(out, k, 32);
		ok = 1;
		goto done;
	}

    if (password_len > SIZE_MAX - 64U || password_len + 64U > SIZE_MAX - user_data_len)
		goto done;

    max_base_len = password_len + 64U + user_data_len;

    if (max_base_len == 0 || max_base_len > (SIZE_MAX - 16U) / 64U)
		goto done;

    round_input = (unsigned char *)malloc(max_base_len * 64U);

    e = (unsigned char *)malloc(max_base_len * 64U + 16U);

    if (round_input == NULL || e == NULL)
		goto done;

    for (;;)
    {
        unsigned char *q = round_input;

        if (password_len > SIZE_MAX - k_len || password_len + k_len > SIZE_MAX - user_data_len)
			goto done;

        round_base_len = password_len + k_len + user_data_len;

        for (j = 0; j < 64U; ++j)
        {
            if (password_len)
            {
				memcpy(q, password, password_len);
				q += password_len;
			}

            memcpy(q, k, k_len); q += k_len;

            if (user_data_len)
            {
				memcpy(q, user_data, user_data_len);
				q += user_data_len;
			}
        }

        {
            size_t enc_len = 0;

            if (!aes_crypt(env, 1, k, 16, k + 16, round_input, round_base_len * 64U, e, &enc_len, 0, 0))
				goto done;

            e_len = enc_len;
        }

        sum_mod3 = 0;

        for (i = 0; i < 16U; ++i)
			sum_mod3 += e[i];

        sum_mod3 %= 3;

        hash_name = sum_mod3 == 0 ? "SHA256" : (sum_mod3 == 1 ? "SHA384" : "SHA512");

        if (!digest_one(env, hash_name, e, e_len, next, &k_len))
			goto done;

        memcpy(k, next, k_len);

        ++round_no;

        if (round_no >= 64U && e[e_len - 1U] <= (unsigned char)(round_no - 32U))
			break;

        // The unsigned last byte makes termination certain by round 287.
        if (round_no >= 287U)
			break;
    }

    memcpy(out, k, 32);

    ok = 1;

done:

    OPENSSL_cleanse(k, sizeof(k));
    OPENSSL_cleanse(next, sizeof(next));

    if (round_input != NULL)
    {
		OPENSSL_cleanse(round_input, max_base_len * 64U); free(round_input);
	}

    if (e != NULL)
    {
		OPENSSL_cleanse(e, max_base_len * 64U + 16U); free(e);
	}

    return ok;
}

static int check_perms(CryptoEnv *env, const PdfCryptoParams *cp, const unsigned char file_key[32])
{
    unsigned char clear[32], iv_dummy[16] = {0};
    size_t clear_len = 0;
    uint32_t pbits;
    int ok = 0;
    (void)iv_dummy;

    if (cp->pPerms == NULL || cp->lenPerms < 16) return 1; /* permit caller to omit validation bytes */

    if (!aes_crypt(env, 0, file_key, 32, NULL, cp->pPerms, 16, clear, &clear_len, 0, 1) || clear_len != 16) goto done;

    pbits = (uint32_t)cp->P;

    if (clear[0] != (unsigned char)pbits || clear[1] != (unsigned char)(pbits >> 8)
										 || clear[2] != (unsigned char)(pbits >> 16)
										 || clear[3] != (unsigned char)(pbits >> 24))
		goto done;

    if (clear[9] != 'a' || clear[10] != 'd' || clear[11] != 'b')
		goto done;

    if (clear[8] != ((cp->hasEncryptMetadata && !cp->encryptMetadata) ? 'F' : 'T'))
		goto done;

    ok = 1;

done:

    OPENSSL_cleanse(clear, sizeof(clear));

    return ok;
}

static int authenticate_r56(CryptoEnv *env, const PdfCryptoParams *cp,
                            const unsigned char *password, size_t password_len,
                            unsigned char key[32])
{
    unsigned char hash[32], intermediate[32], zero_iv[16] = {0};
    size_t clear_len = 0;
    int owner_valid = 0, user_valid = 0, ok = 0;

    if (cp->lenO < 48 || cp->pO == NULL || cp->lenU < 48 || cp->pU == NULL ||
        cp->lenOE < 32 || cp->pOE == NULL || cp->lenUE < 32 || cp->pUE == NULL ||
        cp->lenPerms < 16 || cp->pPerms == NULL)
		return 0;

    if (password_len > PDF_MAX_PASSWORD_R56)
		password_len = PDF_MAX_PASSWORD_R56;

    // Owner authentication is checked first, as in established PDF implementations.
    if (hash_r56(env, cp->R, password, password_len, cp->pO + 32, cp->pU, 48, hash) && CRYPTO_memcmp(hash, cp->pO, 32) == 0)
    {
        owner_valid = 1;

        if (!hash_r56(env, cp->R, password, password_len, cp->pO + 40, cp->pU, 48, intermediate))
			goto done;

        if (!aes_crypt(env, 0, intermediate, 32, zero_iv, cp->pOE, 32, key, &clear_len, 0, 0) || clear_len != 32)
			goto done;

    }
    else if (hash_r56(env, cp->R, password, password_len, cp->pU + 32, NULL, 0, hash) && CRYPTO_memcmp(hash, cp->pU, 32) == 0)
    {
        user_valid = 1;

        if (!hash_r56(env, cp->R, password, password_len, cp->pU + 40, NULL, 0, intermediate))
			goto done;

        if (!aes_crypt(env, 0, intermediate, 32, zero_iv, cp->pUE, 32, key, &clear_len, 0, 0) || clear_len != 32)
			goto done;
    }
    else
    {
        goto done;
    }

    (void)owner_valid;
    (void)user_valid;

    if (!check_perms(env, cp, key))
		goto done;

    ok = 1;

done:

    OPENSSL_cleanse(hash, sizeof(hash));
    OPENSSL_cleanse(intermediate, sizeof(intermediate));

    return ok;
}

static int try_password_candidate(CryptoEnv *env, const PdfCryptoParams *cp,
                                  const unsigned char *password, size_t password_len,
                                  unsigned char key[32], size_t *key_len)
{
    if (password_len != 0 && password == NULL)
		return 0;

    if (cp->R >= 2 && cp->R <= 4)
    {
        if (check_legacy_user(env, cp, password, password_len, key, key_len))
			return 1;

        return check_legacy_owner(env, cp, password, password_len, key, key_len);
    }

    if (cp->R == 5 || cp->R == 6)
    {
        if (authenticate_r56(env, cp, password, password_len, key))
        {
            *key_len = 32;
            return 1;
        }

        return 0;
    }

    return -1;
}

static int get_file_key(CryptoEnv *env, PdfCryptoParams *cp, unsigned char key[32], size_t *key_len)
{
    int status = 0;

    //wprintf(L"[DEBUG get_file_key] bUseProvidedFileKey=%d, bPasswordSet=%d, bUserPasswordSet=%d, bOwnerPasswordSet=%d, R=%d, lenFileKey=%zu\n",
    //        cp->bUseProvidedFileKey, cp->bPasswordSet, cp->bUserPasswordSet, cp->bOwnerPasswordSet,
    //        cp->R, cp->lenFileKey);

    if (cp->bUseProvidedFileKey)
    {
        if (cp->lenFileKey == 0 || cp->lenFileKey > sizeof(cp->fileKey))
        {
            set_error(cp, "Lunghezza della file key non valida.");
            return 0;
        }

        memcpy(key, cp->fileKey, cp->lenFileKey);
        *key_len = cp->lenFileKey;

        return 1;
    }

    if (cp->R < 2 || cp->R > 6)
    {
        set_error(cp, "Revisione PDF non supportata: sono gestite R=2..R=6.");
        return 0;
    }

    if (cp->bPasswordSet)
        status = try_password_candidate(env, cp, cp->pPassword, cp->lenPassword, key, key_len);

    if (status == 0 && cp->bUserPasswordSet)
        status = try_password_candidate(env, cp, cp->pUserPassword, cp->lenUserPassword, key, key_len);

    if (status == 0 && cp->bOwnerPasswordSet)
        status = try_password_candidate(env, cp, cp->pOwnerPassword, cp->lenOwnerPassword, key, key_len);

    if (status <= 0)
    {
        set_error(cp, status < 0 ? "Revisione PDF non supportata." : "Password errata, dizionario Encrypt incompleto o algoritmo non supportato.");
        return 0;
    }

    if (*key_len > sizeof(cp->fileKey))
    {
        set_error(cp, "La chiave derivata supera la dimensione supportata.");
        OPENSSL_cleanse(key, 32);

        return 0;
    }

    memcpy(cp->fileKey, key, *key_len);

    cp->lenFileKey = *key_len;

    cp->bUseProvidedFileKey = 1;

    return 1;
}

// Verifica la password candidata e memorizza la file key se valida.
// Non decifra alcuno stream PDF.
int PdfCryptoParams_Authenticate(PdfCryptoParams *cp)
{
    CryptoEnv env;
    unsigned char key[32];
    size_t key_len = 0;
    int retValue = 0;

    memset(&env, 0, sizeof(env));
    memset(key, 0, sizeof(key));

    if (cp == NULL)
        return 0;

    cp->szError[0] = '\0';

    if (!crypto_env_init(&env))
    {
        set_error(cp, "Impossibile inizializzare i provider OpenSSL.");
        goto uscita;
    }

    // Verifica la password e conserva la file key in PdfCryptoParams.
    if (!get_file_key(&env, cp, key, &key_len))
        goto uscita;

    retValue = 1;

uscita:

    OPENSSL_cleanse(key, sizeof(key));
    crypto_env_destroy(&env);

    return retValue;
}

static PdfCryptMethod choose_method(const PdfCryptoParams *cp)
{
    PdfCryptMethod method;

    if (cp->method != PDF_CRYPT_AUTO) return cp->method;

    if (cp->V < 4) return PDF_CRYPT_RC4;

    switch (cp->target)
    {
        case PDF_CRYPT_TARGET_STRING:
            method = cp->strMethod;
            // Fallback: se /StrF non e' stato parsato ma /StmF si',
            // usa lo stesso crypt filter degli stream. Caso comune
            // (pdfTeX, Distiller, ecc.): /StrF == /StmF.
            if (method == PDF_CRYPT_AUTO &&
                cp->streamMethod != PDF_CRYPT_AUTO &&
                cp->streamMethod != PDF_CRYPT_NONE)
            {
                method = cp->streamMethod;
            }
            break;

        case PDF_CRYPT_TARGET_EMBEDDED_FILE:
            method = cp->embeddedFileMethod != PDF_CRYPT_AUTO
                     ? cp->embeddedFileMethod
                     : cp->streamMethod;
            break;

        case PDF_CRYPT_TARGET_STREAM:
        default:
            method = cp->streamMethod;
            break;
    }

    return method;
}

/*
static PdfCryptMethod choose_method(const PdfCryptoParams *cp)
{
    PdfCryptMethod method;

    if (cp->method != PDF_CRYPT_AUTO) return cp->method;

    if (cp->V < 4) return PDF_CRYPT_RC4;

    switch (cp->target)
    {
		case PDF_CRYPT_TARGET_STRING:
			method = cp->strMethod;
			break;

		case PDF_CRYPT_TARGET_EMBEDDED_FILE:
			method = cp->embeddedFileMethod != PDF_CRYPT_AUTO ? cp->embeddedFileMethod : cp->streamMethod;
			break;

		case PDF_CRYPT_TARGET_STREAM:

		default:
			method = cp->streamMethod;
			break;
    }

    return method;
}
*/

static int derive_object_key(CryptoEnv *env, const PdfCryptoParams *cp,
                             const unsigned char *file_key, size_t file_key_len,
                             PdfCryptMethod method, unsigned char object_key[16], size_t *object_key_len)
{
    unsigned char id_bytes[5], digest[EVP_MAX_MD_SIZE];
    const unsigned char salt[] = {'s','A','l','T'};
    const unsigned char *parts[3];
    size_t lens[3], digest_len = 0, n;
    uint32_t obj = cp->objectNumber, gen = cp->generationNumber;
    int ok = 0;

    if (cp->V >= 5)
		return 0; // AESV3 uses the file key directly in transform().

    if (file_key_len == 0 || file_key_len > 16)
		return 0;

    id_bytes[0] = (unsigned char)(obj & 0xffU);
    id_bytes[1] = (unsigned char)((obj >> 8) & 0xffU);
    id_bytes[2] = (unsigned char)((obj >> 16) & 0xffU);
    id_bytes[3] = (unsigned char)(gen & 0xffU);
    id_bytes[4] = (unsigned char)((gen >> 8) & 0xffU);

    parts[0] = file_key; lens[0] = file_key_len;
    parts[1] = id_bytes; lens[1] = sizeof(id_bytes);
    parts[2] = salt; lens[2] = method == PDF_CRYPT_AESV2 ? sizeof(salt) : 0U;

    if (!hash_parts(env, "MD5", parts, lens, 3, digest, &digest_len) || digest_len < 16)
		goto done;

    n = file_key_len + 5U;

    if (n > PDF_MAX_OBJECT_KEY_LEN) n = PDF_MAX_OBJECT_KEY_LEN;

    memcpy(object_key, digest, n);

    *object_key_len = n;

    ok = 1;

done:

    OPENSSL_cleanse(digest, sizeof(digest));
    OPENSSL_cleanse(id_bytes, sizeof(id_bytes));

    return ok;
}

static int transform(PdfCryptoParams *cp, int encrypt)
{
    CryptoEnv env;
    PdfCryptMethod method;
    unsigned char file_key[32], object_key[16], iv[16];
    size_t file_key_len = 0, object_key_len = 0, out_len = 0, out_cap = 0;
    unsigned char *out = NULL;
    const unsigned char *key_for_data = NULL;
    size_t key_for_data_len = 0;
    int ok = 0;

    memset(&env, 0, sizeof(env));
    memset(file_key, 0, sizeof(file_key));
    memset(object_key, 0, sizeof(object_key));
    memset(iv, 0, sizeof(iv));

    if (cp == NULL)
		return 0;

    PdfCryptoParams_FreeOutput(cp);

    cp->szError[0] = '\0';

    if (cp->lenInput != 0 && cp->pInput == NULL)
    {
        set_error(cp, "pInput è NULL ma lenInput è diverso da zero.");
        goto done;
    }

    method = choose_method(cp);

    if (method == PDF_CRYPT_AUTO)
    {
        set_error(cp, "Crypt filter non risolto: impostare method/strMethod/streamMethod in base a /CF.");
        goto done;
    }

    if (method < PDF_CRYPT_NONE || method > PDF_CRYPT_AESV3)
    {
        set_error(cp, "Metodo di cifratura non valido.");
        goto done;
    }

    if (!crypto_env_init(&env))
    {
        set_error(cp, "Impossibile inizializzare i provider OpenSSL.");
        goto done;
    }

    if (method == PDF_CRYPT_NONE)
    {
        out_cap = cp->lenInput ? cp->lenInput : 1U;

        out = (unsigned char *)malloc(out_cap);

        if (out == NULL)
        {
			set_error(cp, "malloc fallita per lo stream di output.");
			goto done;
		}

        if (cp->lenInput)
			memcpy(out, cp->pInput, cp->lenInput);

        out_len = cp->lenInput;

        goto success;
    }

    //if (!get_file_key(&env, cp, file_key, &file_key_len))
	//	goto done;
	//wprintf(L"[DEBUG transform] target=%d, method=%d, bUseProvidedFileKey=%d, lenFileKey=%zu, R=%d\n", cp->target, method, cp->bUseProvidedFileKey, cp->lenFileKey, cp->R);
	if (!get_file_key(&env, cp, file_key, &file_key_len))
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_Decrypt)
		wprintf(L"[DEBUG transform] get_file_key FALLITA: %s\n", cp->szError);
		#endif
		goto done;
	}
	//wprintf(L"[DEBUG transform] get_file_key OK, file_key_len=%zu\n", file_key_len);

    if (method == PDF_CRYPT_RC4 || method == PDF_CRYPT_AESV2)
    {
        if (cp->V >= 5)
        {
            set_error(cp, "RC4/AESV2 non sono validi per il percorso PDF R=5/6.");
            goto done;
        }

        if (!derive_object_key(&env, cp, file_key, file_key_len, method, object_key, &object_key_len))
        {
            set_error(cp, "Impossibile derivare la chiave dell'oggetto PDF.");
            goto done;
        }

        key_for_data = object_key;
        key_for_data_len = object_key_len;

    }
    else if (method == PDF_CRYPT_AESV3)
    {
        if (file_key_len != 32)
        {
            set_error(cp, "AESV3 richiede una file key di 32 byte.");
            goto done;
        }
        key_for_data = file_key;
        key_for_data_len = 32;
    }

    if (method == PDF_CRYPT_RC4)
    {
        out_cap = cp->lenInput ? cp->lenInput : 1U;
        out = (unsigned char *)malloc(out_cap);

        if (out == NULL)
        {
			set_error(cp, "malloc fallita per lo stream RC4.");
			goto done;
		}

        if (!rc4_crypt(&env, key_for_data, key_for_data_len, cp->pInput, cp->lenInput, out))
        {
            set_error(cp, "Decifratura RC4 fallita.");
            goto done;
        }

        out_len = cp->lenInput;

    }
    else if (method == PDF_CRYPT_AESV2 || method == PDF_CRYPT_AESV3)
    {
        if (encrypt)
        {
            size_t cap;

            if (cp->lenInput > SIZE_MAX - 48U)
            {
				set_error(cp, "Input troppo grande.");
				goto done;
			}

            cap = cp->lenInput + 48U;
            out_cap = cap ? cap : 1U;

            out = (unsigned char *)malloc(out_cap);

            if (out == NULL)
            {
				set_error(cp, "malloc fallita per lo stream AES.");
				goto done;
			}

            if (RAND_bytes_ex(env.libctx, iv, sizeof(iv), 0) != 1)
            {
                set_error(cp, "RAND_bytes_ex non è riuscita a generare il vettore IV.");
                goto done;
            }

            memcpy(out, iv, 16);

            if (!aes_crypt(&env, 1, key_for_data, key_for_data_len, iv, cp->pInput, cp->lenInput, out + 16, &out_len, 1, 0))
            {
                set_error(cp, "Cifratura AES-CBC fallita."); goto done;
            }

            out_len += 16U;

        }
        else
        {
            size_t encrypted_len;

            if (cp->lenInput < 32U || ((cp->lenInput - 16U) % 16U) != 0U)
            {
                set_error(cp, "Stream AES malformato: attesi IV di 16 byte e ciphertext a blocchi da 16."); goto done;
            }

            encrypted_len = cp->lenInput - 16U;
            out_cap = encrypted_len ? encrypted_len : 1U;

            out = (unsigned char *)malloc(out_cap);

            if (out == NULL)
            {
				set_error(cp, "malloc fallita per lo stream AES decifrato.");
				goto done;
			}

            if (!aes_crypt(&env, 0, key_for_data, key_for_data_len, cp->pInput, cp->pInput + 16U, encrypted_len, out, &out_len, 1, 0))
            {
                set_error(cp, "Decifratura AES fallita: password errata, ciphertext danneggiato o padding non valido.");
                goto done;
            }
        }
    }
    else
    {
        set_error(cp, "Metodo di cifratura non gestito.");
        goto done;
    }

success:

    if (out_len > UINT_MAX)
    {
        set_error(cp, "L'output supera UINT_MAX: il contratto lenStream usa unsigned int.");
        goto done;
    }

    cp->pszStream = out;
    cp->lenStreamValue = (unsigned int)out_len;
    cp->lenStream = &cp->lenStreamValue;

    out = NULL;

    ok = 1;

done:

    if (out != NULL)
    {
		OPENSSL_cleanse(out, out_cap); free(out);
	}
    OPENSSL_cleanse(file_key, sizeof(file_key));
    OPENSSL_cleanse(object_key, sizeof(object_key));
    OPENSSL_cleanse(iv, sizeof(iv));

    crypto_env_destroy(&env);

    return ok;
}

int Encrypt(struct tagParams *pParams)
{
    if (pParams == NULL || pParams->pCryptoParams == NULL)
		return 0;

    return transform(pParams->pCryptoParams, 1);
}

int Decrypt(struct tagParams *pParams)
{
    if (pParams == NULL || pParams->pCryptoParams == NULL)
		return 0;

    return transform(pParams->pCryptoParams, 0);
}
