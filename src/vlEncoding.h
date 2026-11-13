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

#ifndef	MY_ENCODING_INCLUDED
#define	MY_ENCODING_INCLUDED

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else // ! 
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif //  

#ifdef _MSC_VER
#define INLINE __forceinline // use   forceinline (VC++ specific) 
#else
#define INLINE inline        // use standard inline 
#endif

#include <stdint.h>

//#includes mypdfsearch. h"

#define SwapTwoBytes(data) \
( (((data) >> 8) & 0x00FF) | (((data) << 8) & 0xFF00) )

#define SwapFourBytes(data)   \
( (((data) >> 24) & 0x000000FF) | (((data) >>  8) & 0x0000FF00) | \
  (((data) <<  8) & 0x00FF0000) | (((data) << 24) & 0xFF000000) )

#define SwapEightBytes(data)   \
( (((data) >> 56) & 0x00000000000000FF) | (((data) >> 40) & 0x000000000000FF00) | \
  (((data) >> 24) & 0x0000000000FF0000) | (((data) >>  8) & 0x00000000FF000000) | \
  (((data) <<  8) & 0x000000FF00000000) | (((data) << 24) & 0x0000FF0000000000) | \
  (((data) << 40) & 0x00FF000000000000) | (((data) << 56) & 0xFF00000000000000) )

#define BIT_SET(byte,nbit)   ((byte) |=  (1 << (nbit)))
#define BIT_UNSET(byte,nbit) ((byte) &= ~(1 << (nbit)))
#define BIT_CHECK(byte,nbit) ((byte) &   (1 << (nbit)))
#define BIT_FLIP(byte,nbit)  ((byte) ^=  (1 << (nbit))) // If the new bit(nbit) is zero, it sets it to one; and vice versa.

#define UNUSED(x) (void)(x)

#define MY_BUFFER_SIZE 4096
#define BLOCK_SIZE 4096

#define ASCIIHEX_ERROR_NONE                    0
#define ASCIIHEX_ERROR_EOD_NOT_FOUND           1
#define ASCIIHEX_ERROR_MEMALLOC_FAILED         2
#define ASCIIHEX_ERROR_UNEXPECTED_CHAR         3

#define ASCII85_ERROR_NONE                     0
#define ASCII85_ERROR_EOD_NOT_FOUND            1
#define ASCII85_ERROR_MEMALLOC_FAILED          2
#define ASCII85_ERROR_UNEXPECTED_CHAR          3

#define LZW_ERROR_NONE                         0
#define LZW_ERROR_UNEXPECTED_CODE              1
#define LZW_ERROR_MEMALLOC_FAILED              2
#define LZW_ERROR_TABLE_INIT_FAILED            3
#define LZW_ERROR_TABLE_INSERT_FAILED          4
#define LZW_ERROR_TABLE_FIND_FAILED            5
#define LZW_ERROR_WRITEBITS_FAILED             6
#define LZW_ERROR_READBITS_FAILED              7
#define LZW_ERROR_STRING_NOT_FOUND_IN_TABLE    8
#define LZW_ERROR_EOD_NOT_FOUND                9
#define LZW_ERROR_INIT_ENCODE_PARAMS_FAILED   10
#define LZW_ERROR_INIT_DECODE_PARAMS_FAILED   11
#define LZW_ERROR_TOO_MANY_BITS               12

#define BITS_ORDER_MSB_FIRST 1
#define BITS_ORDER_LSB_FIRST 2

#define BITS_MIN  9
#define BITS_MAX 15

#define DEFAULT_MIN_BITS   BITS_MIN
#define DEFAULT_MAX_BITS   12

#define M_CLR 256 // clear table marker
#define M_EOD 257 // end-of-date marker
#define M_NEW 258 // new code index



// Dictionary structures.
// For encoding, entry at code index is a list of indexes that follow current one.
// For example, if code 99 is 'c', code 481 is 'ci', code 1034 is 'cia', and code 2001 is 'hello',
// then
//   dict[99].next['i'] = 481,
//  dict[481].next['a'] = 1034,
// dict[1034].next['o'] = 2001,
// etc.
typedef struct tag_lzw_enc
{
	uint16_t next[256];
} lzw_enc_t;

// For decoding, dictionary contains index of whatever prefix plus trailing byte. For example, like previous:
// dict[2001] = { c: 'o', prev: 1034 },
// dict[1034] = {c: 'a', prev: 481 },
//  dict[481] = {c: 'i', prev: 99 },
//   dict[99] = {c: 'c', prev: 0 }
// "back" is used for chaining indexes when resolving a code to bytes
typedef struct tag_lzw_dec
{
	uint16_t prev;
	uint16_t back;
	uint8_t  c;
} lzw_dec_t;

BEGIN_C_DECLS

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int IsLittleEndian(void);
int IsBigEndian(void);

unsigned char * asciiHexEncode(unsigned char * pInputStream, size_t len, size_t *pLenOut, uint32_t *pErrorCode);
unsigned char * asciiHexDecode(unsigned char * pInputStream, size_t len, size_t *pLenOut, uint32_t *pErrorCode);

unsigned char * ascii85Encode(unsigned char * pInputStream, size_t len, size_t *pLenOut, uint32_t *pErrorCode);
unsigned char * ascii85Decode(unsigned char * pInputStream, size_t len, size_t *pLenOut, uint32_t *pErrorCode);

unsigned char * lzwEncode(unsigned char * pInputStream, size_t len, uint32_t BitsOrder, uint32_t MaxBits, size_t *pLenOut, uint32_t *pErrorCode);
unsigned char * lzwDecode(unsigned char * pInputStream, size_t len, uint32_t BitsOrder, size_t *pLenOut, uint32_t *pErrorCode);

END_C_DECLS

#endif // MY_ENCODING_INCLUDED
