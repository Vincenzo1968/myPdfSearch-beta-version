/*    Copyright (C) 2026 Vincenzo Lo Cicero

Author: Vincenzo Lo Cicero.
e-mail: vincenzolocicero68@gmail.com

This file is part of mypdfsearch.

mypdfsearch is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any side version.

mypdfsearch is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty
MERCHANTABILITY or FITNESS FOR PARTICULAR PURPOSE. See the
GNU General Public License for details.

You should have received a copy of the GNU General Public License
along with mypdfsearch. If not, see <http://www.gnu.org/licenses/>. */

#ifndef MY_UNICODE_NFC_H
#define MY_UNICODE_NFC_H

// Minimal, standalone Unicode NFC Normalzer.
// The caller owns the returned buffer and must release it with free().
// Input and output are wchar t sequences. This implementation
// wchar t to represent every Unicode scalar value (the usual GNU/Linux ABI).

#include <stddef.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum MyUnicodeNFCStatus {
    MY_UNICODE_NFC_OK = 0,
    MY_UNICODE_NFC_INVALID_ARGUMENT,
    MY_UNICODE_NFC_INVALID_SCALAR,
    MY_UNICODE_NFC_OUT_OF_MEMORY,
    MY_UNICODE_NFC_INVALID_TABLE
} MyUnicodeNFCStatus;

// Normalize input[0..input length) to Unicode NFC.
// - input may be NULL only when input length is zero.
// - output and output length must be non-NULL.
// - on success, *output points to a newly allocated NUL-terminated buffer;
//   *output_length is the number of wchar_t code points, excluding the NUL.
// - on failure, *output is NULL and *output_length is zero.
// - invalid Unicode scalar values (including UTF-16 surrogate code points)
//   are rejected.
// The returned buffer must be released with free().
MyUnicodeNFCStatus myUnicodeNFC_Normalize(
    const wchar_t *input,
    size_t input_length,
    wchar_t **output,
    size_t *output_length
);

const char *myUnicodeNFC_StatusString(MyUnicodeNFCStatus status);
const char *myUnicodeNFC_UnicodeVersion(void);

#ifdef __cplusplus
}
#endif

#endif // MY_UNICODE_NFC_H 
