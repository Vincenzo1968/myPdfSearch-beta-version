/*
   Copyright (C) 2026 Vincenzo Lo Cicero

   Author: Vincenzo Lo Cicero.
   e-mail: vincenzolocicero68@gmail.com

   This file is part of mypdfsearch.

   mypdfsearch is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   mypdfsearch is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with mypdfsearch.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MY_RC4_CRYPT_H
#define MY_RC4_CRYPT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Contesto dello stato RC4.
// Ogni operazione indipendente deve utilizzare il proprio contesto.

typedef struct
{
    uint8_t s[256];
    uint8_t i;
    uint8_t j;
} my_rc4_ctx_t;

// Inizializza il contesto RC4.
// Restituisce 1 in caso di successo, 0 in caso di errore.
// Se ctx è valido ma la chiave non lo è, il contesto viene azzerato.

int my_rc4_init(my_rc4_ctx_t *ctx, const uint8_t *key, size_t key_len);

// Cifra o decifra i dati.
// Restituisce 1 in caso di successo, 0 in caso di errore.
// Richiede un contesto inizializzato correttamente.
// Supporta input e output coincidenti.
// Se len è zero, non modifica il contesto e restituisce 1,
// purché ctx non sia NULL.

int my_rc4_crypt(my_rc4_ctx_t *ctx,
                 const uint8_t *in,
                 uint8_t *out,
                 size_t len);

// Azzera il contesto in memoria.
// Se ctx è NULL, non esegue alcuna operazione.

void my_rc4_reset(my_rc4_ctx_t *ctx);

// Inizializza, cifra/decifra e azzera il contesto.
// Restituisce 1 in caso di successo, 0 in caso di errore.

int my_rc4_process(const uint8_t *key,
                   size_t key_len,
                   const uint8_t *in,
                   uint8_t *out,
                   size_t len);

#ifdef __cplusplus
}
#endif

#endif // MY_RC4_CRYPT_H
