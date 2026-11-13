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

#ifndef MYCRYPT_H_INCLUDED
#define MYCRYPT_H_INCLUDED

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PdfCryptMethod
{
    PDF_CRYPT_AUTO = 0,
    PDF_CRYPT_NONE,
    PDF_CRYPT_RC4,
    PDF_CRYPT_AESV2, // AES-128-CBC, PDF /AESV2
    PDF_CRYPT_AESV3  // AES-256-CBC, PDF /AESV3
} PdfCryptMethod;

typedef enum PdfCryptTarget
{
    PDF_CRYPT_TARGET_STREAM = 0,
    PDF_CRYPT_TARGET_STRING,
    PDF_CRYPT_TARGET_EMBEDDED_FILE
} PdfCryptTarget;

typedef struct tagPdfCryptoParams
{
    // Input bytes. They are borrowed; the caller retains ownership.
    const unsigned char *pInput;
    size_t lenInput;

    // Output bytes. Owned by this module; release with PdfCryptoParams FreeOutput.
    unsigned char *pszStream;
    unsigned int *lenStream; // points to lenStreamValue after initialization/output
    unsigned int lenStreamValue; // backing storage owned by this struct

    // Password candidate. For R<=4, bytes must already be PDFDocEncoding.
    // For R=5/6, supply UTF-8 after the PDF-required password preparation
    // (SASLprep/normalization); this module does not implement Unicode preparation.
    const unsigned char *pPassword;
    size_t lenPassword;
    int bPasswordSet; // single candidate; distinguishes empty from absent

    // Optional extra candidates. Each candidate is tested both as user and
    //   owner password. They are useful if the caller wants to supply them both.
    const unsigned char *pUserPassword;
    size_t lenUserPassword;
    int bUserPasswordSet;
    const unsigned char *pOwnerPassword;
    size_t lenOwnerPassword;
    int bOwnerPasswordSet;

    // Cached/provided file encryption key. Set bUseProvidedFileKey=1 to bypass
    // password authentication (useful for isolated Encrypt/Decrypt tests).
    // R<=4: 5..16 bytes; R=5/6: 32 bytes. A successful Decrypt caches the key here. *
    unsigned char fileKey[32];
    size_t lenFileKey;
    int bUseProvidedFileKey;

    // Parsed Standard Security Handler encryption dictionary values.
    int V;                  // /V
    int R;                  // /R
    int keyLengthBits;      // /Length, in bits; 40/128/256 as applicable
    int32_t P;              // /P
    int encryptMetadata;    // /EncryptMetadata value, if present
    int hasEncryptMetadata; // 0 means absent; PDF default is true

    const unsigned char *pID0; size_t lenID0; // trailer /ID[0], raw bytes
    const unsigned char *pO;   size_t lenO;   // /O, raw string bytes
    const unsigned char *pU;   size_t lenU;   // /U, raw string bytes
    const unsigned char *pOE;  size_t lenOE;  // /OE for R=5/6
    const unsigned char *pUE;  size_t lenUE;  // /EU for R=5/6
    const unsigned char *pPerms; size_t lenPerms; // /Perms for R=5/6

    // Method for this object. If method!
    // Otherwise the selected crypt-filter method is used according to target.
    // The caller resolves /CF entries (/V2, /AESV2, /AESV3, /Identity).
    PdfCryptMethod method;
    PdfCryptMethod strMethod;
    PdfCryptMethod streamMethod;
    PdfCryptMethod embeddedFileMethod;
    PdfCryptTarget target;

    // Indirect object identity; used to drift per-object keys for R<=4.
    uint32_t objectNumber;
    uint32_t generationNumber;

    // Diagnostic text on failure (never contains the supplied password).
    char szError[256];
} PdfCryptoParams;

// Params is defined by scanner.h in the integrating project. Forwarding
// avoids forcing mycrypt.h to include the parser's full dependency tree.
struct tagParams;

// Returns 1 on success, 0 on failure. Both operate on the bytes in pInput and
// return to malloc-allocated byte string in pCryptoParams->pszStream. These are
// PDF object/string/stream transforms, NOT whole-file PDF writers.
int Encrypt(struct tagParams *pParams);
int Decrypt(struct tagParams *pParams);

int PdfCryptoInitialize(void);
int PdfCryptoShutdown(void);

int  PdfCryptoParams_Authenticate(PdfCryptoParams *cp);
void PdfCryptoParams_FreeOutput(PdfCryptoParams *pCryptoParams);

#ifdef __cplusplus
}
#endif
#endif // MYCRYPT_H_INCLUDED 
