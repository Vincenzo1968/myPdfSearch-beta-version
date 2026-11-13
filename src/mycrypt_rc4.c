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

#include "mycrypt_rc4.h"

static inline void rc4_swap(uint8_t *a, uint8_t *b)
{
    uint8_t temp = *a;
    *a = *b;
    *b = temp;
}

int my_rc4_init(my_rc4_ctx_t *ctx,
                const uint8_t *key,
                size_t key_len)
{
    if (ctx == NULL)
        return 0;

    // Eliminate any previous status, even in case of error.
    my_rc4_reset(ctx);

    if (key == NULL || key_len == 0)
        return 0;

    // 1. Initialization S-Box.
    for (size_t idx = 0; idx < 256; idx++)
    {
        ctx->s[idx] = (uint8_t)idx;
    }

    // 2. Key-Scheduling Algorithm (KSA).
    uint8_t j = 0;

    for (size_t idx = 0; idx < 256; idx++)
    {
        j = (uint8_t)(j + ctx->s[idx] + key[idx % key_len]);
        rc4_swap(&ctx->s[idx], &ctx->s[j]);
    }

    // 3. Initialization of PRGA indices.
    ctx->i = 0;
    ctx->j = 0;

    return 1;
}

int my_rc4_crypt(my_rc4_ctx_t *ctx,
                 const uint8_t *in,
                 uint8_t *out,
                 size_t len)
{
    if (ctx == NULL)
        return 0;

    // Processing zero bytes is a valid and no-effect operation.
    if (len == 0)
        return 1;

    if (in == NULL || out == NULL)
        return 0;

    uint8_t i = ctx->i;
    uint8_t j = ctx->j;
    uint8_t *s = ctx->s;

    // Pseudo-Random Generation Algorithm (PRGA).
    for (size_t k = 0; k < len; k++)
    {
        i = (uint8_t)(i + 1);
        j = (uint8_t)(j + s[i]);

        rc4_swap(&s[i], &s[j]);

        uint8_t t = (uint8_t)(s[i] + s[j]);
        out[k] = in[k] ^ s[t];
    }

    ctx->i = i;
    ctx->j = j;

    return 1;
}

void my_rc4_reset(my_rc4_ctx_t *ctx)
{
    if (ctx == NULL)
        return;

    // Use a volatile loop to prevent the compiler
    // eliminate zeroing during optimization.
    volatile uint8_t *p = (volatile uint8_t *)ctx;
    size_t sz = sizeof(*ctx);

    while (sz--)
    {
        *p++ = 0;
    }
}

int my_rc4_process(const uint8_t *key,
                   size_t key_len,
                   const uint8_t *in,
                   uint8_t *out,
                   size_t len)
{
    my_rc4_ctx_t ctx = {0};

    // If initialization fails, do not attempt encryption.
    int result = my_rc4_init(&ctx, key, key_len);

    if (result)
    {
        result = my_rc4_crypt(&ctx, in, out, len);
    }

    // Performed both in case of success and in case of error.
    my_rc4_reset(&ctx);

    return result;
}
