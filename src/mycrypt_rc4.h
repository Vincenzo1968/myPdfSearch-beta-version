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

#ifndef MY_RC4_CRYPT_H
#define MY_RC4_CRYPT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Context of the RC4 state.
// Each independent operation must use its context.

typedef struct
{
    uint8_t s[256];
    uint8_t i;
    uint8_t j;
} my_rc4_ctx_t;

// It initiates the RC4 context.
// Returns 1 in case of success, 0 in case of error.
// If ctx is valid but the key is not, the context is reset.

int my_rc4_init(my_rc4_ctx_t *ctx, const uint8_t *key, size_t key_len);

// Cipher or decrypt data.
// Returns 1 in case of success, 0 in case of error.
// Requires an initialized context correctly.
// Supports coin input and output.
// If len is zero, it does not change the context and returns 1,
// provided ctx is not NULL.

int my_rc4_crypt(my_rc4_ctx_t *ctx,
                 const uint8_t *in,
                 uint8_t *out,
                 size_t len);

// Clear the context in memory.
// If ctx is NULL, it does not perform any operation.

void my_rc4_reset(my_rc4_ctx_t *ctx);

// Initialize, digit/decipher and reset the context.
// Returns 1 in case of success, 0 in case of error.

int my_rc4_process(const uint8_t *key,
                   size_t key_len,
                   const uint8_t *in,
                   uint8_t *out,
                   size_t len);

#ifdef __cplusplus
}
#endif

#endif // MY_RC4_CRYPT_H
