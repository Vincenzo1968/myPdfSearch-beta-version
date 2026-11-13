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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <locale.h>
#include <wchar.h>

#include "vlEncoding.h"

#define write_bits_msb_first(_Code) \
{ \
	tmp = (tmp << bits) | (_Code); \
	nBits += bits; \
	if ( sizeOutputBlock <= lenOut ) \
	{ \
		uint8_t *pTemp = NULL; \
		int oldSize = sizeOutputBlock; \
		sizeOutputBlock *= 2; \
		pTemp = (uint8_t *)realloc(pOutputStream, sizeOutputBlock * sizeof(uint8_t)); \
		if ( NULL == pTemp ) \
		{ \
			free(pDict); \
			pDict = NULL; \
			free(pOutputStream); \
			pOutputStream = NULL; \
			*pErrorCode = LZW_ERROR_MEMALLOC_FAILED; \
			return NULL; \
		} \
		pOutputStream = pTemp; \
		memset(pOutputStream + oldSize, 0, oldSize * sizeof(uint8_t)); \
	} \
	while ( nBits >= 8 ) \
	{ \
		nBits -= 8; \
		pOutputStream[lenOut++] = tmp >> nBits; \
		tmp &= (1 << nBits) - 1; \
	} \
}

#define write_bits_lsb_first(_Code) \
{ \
	tmp |= (_Code) << nBits; \
	nBits += bits; \
	if ( sizeOutputBlock <= lenOut ) \
	{ \
		uint8_t *pTemp = NULL; \
		int oldSize = sizeOutputBlock; \
		sizeOutputBlock *= 2; \
		pTemp = (uint8_t *)realloc(pOutputStream, sizeOutputBlock * sizeof(uint8_t)); \
		if ( NULL == pTemp ) \
		{ \
			free(pDict); \
			pDict = NULL; \
			free(pOutputStream); \
			pOutputStream = NULL; \
			*pErrorCode = LZW_ERROR_MEMALLOC_FAILED; \
			return NULL; \
		} \
		pOutputStream = pTemp; \
		memset(pOutputStream + oldSize, 0, oldSize * sizeof(uint8_t)); \
	} \
	while ( nBits >= 8 ) \
	{ \
		nBits -= 8; \
		pOutputStream[lenOut++] = tmp & 0xFF; \
		tmp >>= 8; \
	} \
}

#define read_bits_msb_first() \
{ \
	while ( nBits < bits ) \
	{ \
		if ( len > 0 ) \
		{ \
			len--; \
			tmp = (tmp << 8) | *(pInputStream++); \
			nBits += 8; \
		} \
		else \
		{ \
			tmp = tmp << (bits - nBits); \
			nBits = bits; \
		} \
	} \
	nBits -= bits; \
	code = tmp >> nBits; \
	tmp &= (1 << nBits) - 1; \
}

#define read_bits_lsb_first() \
{ \
	while ( nBits < bits ) \
	{ \
		if ( len > 0 ) \
			len--; \
		tmp |= *(pInputStream)++ << nBits; \
		nBits += 8; \
	} \
	code = tmp; \
	tmp >>= bits; \
	nBits -= bits; \
    code &= curmask; \
}

// ****************************************************************** AsciiHex **************************************************************

unsigned char * asciiHexEncode(unsigned char * pInputStream, size_t len, size_t *pLenOut, uint32_t *pErrorCode)
{
	size_t k;
	size_t y;

	unsigned char * pOutputStream = NULL;

	char s[3];

	*pErrorCode = ASCIIHEX_ERROR_NONE;

	*pLenOut = len * 2 * sizeof(unsigned char) + sizeof(unsigned char);
	pOutputStream = (unsigned char *)malloc(*pLenOut);
	if ( NULL == pOutputStream )
	{
		*pErrorCode = ASCIIHEX_ERROR_MEMALLOC_FAILED;
		*pLenOut = 0;
		return NULL;
	}
	for ( k = 0; k < *pLenOut; k++ )
		pOutputStream[k] = '>';

	y = 0;
	for ( k = 0; k < len; k++ )
	{
		sprintf(s, "%X", pInputStream[k]);
		if ( pInputStream[k] > 0xF )
		{
			memcpy(pOutputStream + y, s, 2);
			y += 2;
		}
		else
		{
			memcpy(pOutputStream + y, "0", 1);
			y++;
			memcpy(pOutputStream + y, s, 1);
			y++;
		}
	}

	return pOutputStream;
}

unsigned char * asciiHexDecode(unsigned char * pInputStream, size_t len, size_t *pLenOut, uint32_t *pErrorCode)
{
	unsigned char cHexadecimal;

	size_t lenNew;

	size_t k;
	size_t y;
	size_t x;
	size_t z;

	unsigned char * pOutputStream = NULL;

	*pErrorCode = ASCIIHEX_ERROR_NONE;

	*pLenOut = 0;

	if ( pInputStream[len - 1] != '>' )
	{
		*pErrorCode = ASCIIHEX_ERROR_EOD_NOT_FOUND;
		*pLenOut = 0;
		return NULL;
	}

	lenNew = len - 1;
	if ( 0 != lenNew % 2 )
	{
		//bZeroFollowLastDigit = 1;
		lenNew++;
		//wprintf(L"\n\nEHI!!! I replace '%c' with '0'\n", pInputStream[lenNew]);
		pInputStream[lenNew - 1] = '0';
		//wprintf(L"\tECCO: pInputStream[%d] = '%c'\n\n', lenNew - 1, pInputStream[lenNew - 1]);
	}

	*pLenOut = lenNew / 2;

	pOutputStream = (unsigned char *)malloc(*pLenOut * sizeof(unsigned char));
	if ( NULL == pOutputStream )
	{
		*pErrorCode = ASCIIHEX_ERROR_MEMALLOC_FAILED;
		*pLenOut = 0;
		return NULL;
	}
	for ( k = 0 ; k < lenNew * 4 * sizeof(unsigned char); k++ )
		pOutputStream[k] = '\0';

	//wprintf(L"\n\nasciiHexDecode: len = %d; lenNew = %d; *pLenOut = %d\n\n, lenNew, *pLenOut);

	y = 0;
	z = 0;
	for ( x = 0; x < lenNew/2; x++ )
	{
		switch ( pInputStream[y] )
		{
			case '0':
				cHexadecimal = 0;
				break;
			case '1':
				cHexadecimal = 16; // 1 * 16;
				break;
			case '2':
				cHexadecimal = 32; // 2 * 16;
				break;
			case '3':
				cHexadecimal = 48; // 3 * 16;
				break;
			case '4':
				cHexadecimal = 64; // 4 * 16;
				break;
			case '5':
				cHexadecimal = 80; // 5 * 16;
				break;
			case '6':
				cHexadecimal = 96; // 6 * 16;
				break;
			case '7':
				cHexadecimal = 112; // 7 * 16;
				break;
			case '8':
				cHexadecimal = 128; // 8 * 16;
				break;
			case '9':
				cHexadecimal = 144; // 9 * 16;
				break;
			case 'A':
			case 'a':
				cHexadecimal = 160; // 10 * 16;
				break;
			case 'B':
			case 'b':
				cHexadecimal = 176; // 11 * 16;
				break;
			case 'C':
			case 'c':
				cHexadecimal = 192; // 12 * 16;
				break;
			case 'D':
			case 'd':
				cHexadecimal = 208 ;// 13 * 16;
				break;
			case 'E':
			case 'e':
				cHexadecimal = 224; // 14 * 16;
				break;
			case 'F':
			case 'f':
				cHexadecimal = 240; // 15 * 16;
				break;
			default:
				//wprintf(L"ERRORE asciiHexDecode 1: invalid character: pInputStream[%d] = '%c'; z = %d\n, y, pInputStream[y], z);
				*pErrorCode = ASCIIHEX_ERROR_UNEXPECTED_CHAR;
				free(pOutputStream);
				pOutputStream = NULL;
				*pLenOut = 0;
				return NULL;
				break;
		}

		switch ( pInputStream[y + 1] )
		{
			case '0':
				//cHexadecimal += 0;
				break;
			case '1':
				cHexadecimal += 1; // 1 * 16;
				break;
			case '2':
				cHexadecimal += 2;
				break;
			case '3':
				cHexadecimal += 3;
				break;
			case '4':
				cHexadecimal += 4;
				break;
			case '5':
				cHexadecimal += 5;
				break;
			case '6':
				cHexadecimal += 6;
				break;
			case '7':
				cHexadecimal += 7;
				break;
			case '8':
				cHexadecimal += 8;
				break;
			case '9':
				cHexadecimal += 9;
				break;
			case 'A':
			case 'a':
				cHexadecimal += 10;
				break;
			case 'B':
			case 'b':
				cHexadecimal += 11;
				break;
			case 'C':
			case 'c':
				cHexadecimal += 12;
				break;
			case 'D':
			case 'd':
				cHexadecimal += 13;
				break;
			case 'E':
			case 'e':
				cHexadecimal += 14;
				break;
			case 'F':
			case 'f':
				cHexadecimal += 15;
				break;
			default:
				//wprintf(L"ERRORE asciiHexDecode 2: invalid character: pInputStream[%d] = '%c'; z = %d\n, y + 1, pInputStream[y + 1], z);
				*pErrorCode = ASCIIHEX_ERROR_UNEXPECTED_CHAR;
				free(pOutputStream);
				pOutputStream = NULL;
				*pLenOut = 0;
				return NULL;
				break;
		}

		pOutputStream[z] = cHexadecimal;
		z++;

		y += 2;
	}

	return pOutputStream;
}

// ****************************************************************** Ascii85 **************************************************************

unsigned char * ascii85Encode(unsigned char * pInputStream, size_t len, size_t *pLenOut, uint32_t *pErrorCode)
{
	size_t k;
	size_t y;

	uint32_t countBlocks = 0;

	unsigned char * pStreamNew = NULL;
	size_t lenNew;
	uint32_t numPaddingInput;

	unsigned char * pOutputStream = NULL;

	uint32_t num;
	uint32_t numTemp;

	uint32_t howmanyBlocks;
	uint32_t numPadding;

	unsigned char b1, b2, b3, b4;
	unsigned char c0, c1, c2, c3, c4;

	uint32_t nMod;

	*pErrorCode = ASCII85_ERROR_NONE;

	nMod = len % 4;
	if ( nMod > 0 )
		numPaddingInput = 4 - nMod;
	else
		numPaddingInput = 0;

	lenNew = len + numPaddingInput;

	pStreamNew = (unsigned char*)malloc(lenNew * sizeof(unsigned char));
	if ( NULL == pStreamNew )
	{
		*pErrorCode = ASCII85_ERROR_MEMALLOC_FAILED;
		*pLenOut = 0;
		return NULL;
	}
	for ( k = 0; k < len; k++ )
		pStreamNew[k] = pInputStream[k];
	for ( ; k < lenNew; k++ )
		pStreamNew[k] = '\0';


	//numPadding = len % 5;
	numPadding = lenNew % 5;

	howmanyBlocks = lenNew / 5;
	if ( numPadding != 0 )
		howmanyBlocks++;

	//wprintf(L"\nlen = %d; numPaddingInput = %d; lenNew = %d\n", len, numPaddingInput, lenNew);

	//wprintf(L"LEN INPUT = %d; lenNew = %d; %d/5 = %d; BLOCCHI NECESSARI = %d; numPadding = %d\n", lenNew, lenNew, lenNew, lenNew/5, howmanyBlocks, numPadding);

	*pLenOut = howmanyBlocks * 5 * sizeof(unsigned char);
	pOutputStream = (unsigned char *)malloc(*pLenOut * 2);
	if ( NULL == pOutputStream )
	{
		*pErrorCode = ASCII85_ERROR_MEMALLOC_FAILED;
		free(pStreamNew);
		pStreamNew = NULL;
		*pLenOut = 0;
		return NULL;
	}
	for ( k = 0; k < *pLenOut; k++ )
		pOutputStream[k] = '\0';

	//wprintf(L"ALLOCATI %d BYTES FOR POutputStream\n", *pLenOut * 2);

	num = 0;
	y = 0;
	for ( k = 0; k < lenNew; k += 4 )
	{
		b1 = pStreamNew[k];
		b2 = pStreamNew[k + 1];
		b3 = pStreamNew[k + 2];
		b4 = pStreamNew[k + 3];

		numTemp = b1;
		numTemp <<= 24;
		num |= numTemp;

		numTemp = b2;
		numTemp <<= 16;
		num |= numTemp;

		numTemp = b3;
		numTemp <<= 8;
		num |= numTemp;

		numTemp = b4;
		//numTemp <= 0;
		num |= numTemp;

		//wprintf(L"\nnum = %d\n", num);

		c4 = pOutputStream[y + 4] = (num % 85) + 33;
		num /= 85;

		c3 = pOutputStream[y + 3] = (num % 85) + 33;
		num /= 85;

		c2 = pOutputStream[y + 2] = (num % 85) + 33;
		num /= 85;

		c1 = pOutputStream[y + 1] = (num % 85) + 33;
		num /= 85;

		c0 = pOutputStream[y] = (num % 85) + 33;
		num /= 85;

		//I'm sorry.
		if ( (c0 != 33 || c1 != 33 || c2 != 33 || c3 != 33 || c4 != 33) || ( numPaddingInput > 0 && k > (lenNew - 5)) )
		{
			y += 5;
			countBlocks++;

			//wprintf(L"\tBLOCCO[%u] = <%c%c%c%c%c> y = %d\n", countBlocks, c0, c2, c3, c4, y);
		}
		else
		{
			pOutputStream[y] = 'z';
			y++;
			countBlocks++;

			//wprintf(L"\tBLOCCO[%u] = <%c> y = %d\n", countBlocks, pOutputStream[y - 1], y);
		}


		num = 0;
	}

	//wprintf(L"\n\ncountZ = %d\n\n", countZ);

	//*pLenOut = y - (lenNew - len);
	*pLenOut = y - numPaddingInput;
	//if ( countZ)
	//{
	// *pLenOut -= (countZ * 3);
	// *pLenOut += (countZ) + 1;
	//}

	pOutputStream[*pLenOut] = '~';
	(*pLenOut)++;
	pOutputStream[*pLenOut] = '>';
	(*pLenOut)++;

	//output:

	free(pStreamNew);
	pStreamNew = NULL;

	return pOutputStream;
}

unsigned char * ascii85Decode(unsigned char * pInputStream, size_t len, size_t *pLenOut, uint32_t *pErrorCode)
{
	size_t k;
	size_t y;
	size_t x;
	size_t w;

	unsigned char * pStreamNew = NULL;
	size_t lenNew;
	uint32_t numPaddingInput;

	unsigned char * pOutputStream = NULL;

	uint32_t num;
	//uint32 t numTemp;

	//uint32 t howmanyBlocks;
	//uint32 t numPadding;

	unsigned char cBlock[5];

	unsigned char c1, c2, c3, c4;

	uint32_t nMod;

	uint32_t c1Mask = 4278190080;
	uint32_t c2Mask = 16711680;
	uint32_t c3Mask = 65280;
	uint32_t c4Mask = 255;

	*pLenOut = 0;

	*pErrorCode = ASCII85_ERROR_NONE;

	while ( ' ' == pInputStream[len - 1] || '\n' == pInputStream[len - 1] || 'r' == pInputStream[len - 1] || '\t' == pInputStream[len - 1] || '\f' == pInputStream[len - 1] )
	{
		len--;
	}

	if ( pInputStream[len - 1] == '>' && pInputStream[len - 2] == '~' )
	{
		len -= 2;
	}
	else
	{
		//wprintf(L"ERRORE ascii85Decode: ATTESI '~>' TROVATI '%c%c'\n\n", pInputStream[len - 2], pInputStream[len - 1]);
		*pErrorCode = ASCII85_ERROR_EOD_NOT_FOUND;
		return NULL;
	}


	nMod = len % 5;
	if ( nMod > 0 )
		numPaddingInput = 5 - nMod;
	else
		numPaddingInput = 0;



	lenNew = len + numPaddingInput;
	pStreamNew = (unsigned char*)malloc(lenNew * 5 * sizeof(unsigned char));
	if ( NULL == pStreamNew )
	{
		*pErrorCode = ASCII85_ERROR_MEMALLOC_FAILED;
		*pLenOut = 0;
		return NULL;
	}

	lenNew = len;
	w = 0;
	for ( k = 0; k < len; k++ )
	{
		switch ( pInputStream[k] )
		{
			case 'z':
				if ( w % 5 != 0 )
				{
					//wprintf(L"ERRORE 0 ascii85Decode: k = %d; w = %d; invalid character -> 'z'", k, w);
					*pErrorCode = ASCII85_ERROR_UNEXPECTED_CHAR;
					free(pOutputStream);
					pOutputStream = NULL;
					*pLenOut = 0;
					goto uscita;
				}

				pStreamNew[w] = '!';
				w++;
				pStreamNew[w] = '!';
				w++;
				pStreamNew[w] = '!';
				w++;
				pStreamNew[w] = '!';
				w++;
				pStreamNew[w] = '!';
				w++;

				lenNew += 4;

				break;
			case ' ':
			case '\n':
			case '\r':
			case '\t':
			case '\f':
				lenNew--;
				break;
			default:
				pStreamNew[w] = pInputStream[k];
				w++;
				break;
		}
	}
	len = lenNew;

	nMod = len % 5;
	if ( nMod > 0 )
		numPaddingInput = 5 - nMod;
	else
		numPaddingInput = 0;

	lenNew = len + numPaddingInput;

	for ( ; w < lenNew; w++ )
		pStreamNew[w] = 'u';



	//wprintf(L"\n\nascii85Decode pStreamNew -> ENCODED STREAM(len = %d)\n", lenNew);
	//for (k = 0; k < lenNew; k++ )
	//{
	// if (\0)
	//  wprintf(L"%c", pStreamNew[k]);
	//	else
	//  wprintf(L"\nEHI!!! ->\\0\n");
	//}
	//wprintf(L"\nFINE ascii85Decode pStreamNew ENCODED STREAM. \n\n);

	//wprintf(L"\n\nlen = %d; numPaddingInput = %d; lenNew = %d\n\n", len, numPaddingInput, lenNew);
	//wprintf(L"\n\nlenNew * 4 * sizeof(unsigned char) = %d\n\n", lenNew * 4 * sizeof(unsigned char));

	pOutputStream = (unsigned char *)malloc(lenNew * 4 * sizeof(unsigned char));
	if ( NULL == pOutputStream )
	{
		*pErrorCode = ASCII85_ERROR_MEMALLOC_FAILED;
		*pLenOut = 0;
		return NULL;
	}
	for ( k = 0 ; k < lenNew * 4 * sizeof(unsigned char); k++ )
		pOutputStream[k] = '\0';


	x = 0;
	y = 0;
	for ( k = 0; k <= lenNew - 5; k += 5 )
	{
		if ( pStreamNew[k] < 33 || pStreamNew[k] > 117 )
		{
			//if (pStreamNew[k]!= '\0')
			// wprintf(L"ERRORE 1 ascii85Decode: k = %d; invalid character -> %d -> '%c'", k, pStreamNew[k], pStreamNew[k]);
			//else
			// wprintf(L"ERRORE 1 ascii85Decode: k = %d; invalid character -> 0 -> '\\0'", k);

			*pErrorCode = ASCII85_ERROR_UNEXPECTED_CHAR;
			free(pOutputStream);
			pOutputStream = NULL;
			*pLenOut = 0;
			goto uscita;
		}
		cBlock[y] = pStreamNew[k] - 33;
		y++;

		if ( pStreamNew[k] < 33 || pStreamNew[k] > 117 )
		{
			//if (pStreamNew[k]!= '\0')
			// wprintf(L"ERRORE 2 ascii85Decode: k = %d; invalid character -> %d -> '%c'", k, pStreamNew[k], pStreamNew[k]);
			//else
			// wprintf(L"ERRORE 2 ascii85Decode: k = %d; invalid character -> 0 -> '\\0'", k);

			*pErrorCode = ASCII85_ERROR_UNEXPECTED_CHAR;
			free(pOutputStream);
			pOutputStream = NULL;
			*pLenOut = 0;
			goto uscita;
		}
		cBlock[y] = pStreamNew[k + 1] - 33;
		y++;

		if ( pStreamNew[k] < 33 || pStreamNew[k] > 117 )
		{
			//if (pStreamNew[k]!= '\0')
			// wprintf(L"ERRORE 3 ascii85Decode: k = %d; invalid character -> %d -> '%c'", k, pStreamNew[k], pStreamNew[k]);
			//else
			// wprintf(L"ERRORE 3 ascii85Decode: k = %d; invalid character -> 0 -> '\\0'", k);

			*pErrorCode = ASCII85_ERROR_UNEXPECTED_CHAR;
			free(pOutputStream);
			pOutputStream = NULL;
			*pLenOut = 0;
			goto uscita;
		}
		cBlock[y] = pStreamNew[k + 2] - 33;
		y++;

		if ( pStreamNew[k] < 33 || pStreamNew[k] > 117 )
		{
			//if (pStreamNew[k]!= '\0')
			// wprintf(L"ERRORE 4 ascii85Decode: k = %d; invalid character -> %d -> '%c'", k, pStreamNew[k], pStreamNew[k]);
			//else
			// wprintf(L"ERRORE 4 ascii85Decode: k = %d; invalid character -> 0 -> '\\0'", k);

			*pErrorCode = ASCII85_ERROR_UNEXPECTED_CHAR;
			free(pOutputStream);
			pOutputStream = NULL;
			*pLenOut = 0;
			goto uscita;
		}
		cBlock[y] = pStreamNew[k + 3] - 33;
		y++;

		if ( pStreamNew[k] < 33 || pStreamNew[k] > 117 )
		{
			//if (pStreamNew[k]!= '\0')
			// wprintf(L"ERRORE 5 ascii85Decode: k = %d; invalid character -> %d -> '%c'", k, pStreamNew[k], pStreamNew[k]);
			//else
			// wprintf(L"ERRORE 5 ascii85Decode: k = %d; invalid character -> 0 -> '\\0'", k);

			*pErrorCode = ASCII85_ERROR_UNEXPECTED_CHAR;
			free(pOutputStream);
			pOutputStream = NULL;
			*pLenOut = 0;
			goto uscita;
		}
		cBlock[y] = pStreamNew[k + 4] - 33;
		//y++;

		num = cBlock[0] * 52200625 + cBlock[1] * 614125 + cBlock[2] * 7225 + cBlock[3] * 85 + cBlock[4];

		//cprintf(L"num = %u -> (%c)%u *85^4 + (%c)%u *85^3 + (%c)%u *85^2 + (%c)%u*85 + (%c)%uX11X", num[2] cBlock[0]

		c1 = (unsigned char)((num & c1Mask) >> 24);
		c2 = (unsigned char)((num & c2Mask) >> 16);
		c3 = (unsigned char)((num & c3Mask) >> 8);
		c4 = (unsigned char)((num & c4Mask));

		pOutputStream[x] = c1;
		x++;
		pOutputStream[x] = c2;
		x++;
		pOutputStream[x] = c3;
		x++;
		pOutputStream[x] = c4;
		x++;

		//wprintf(L"\tc1(%c) = %u; c2(%c) = %u; c3(%c) = %u; c4(%c) = %u; x = %d; k = %d\n", c1, c2, c4, c4, c4, c4, c4, c4

		y = 0;

		if ( k >= lenNew - 5 )
			break;
	}

	*pLenOut = x - numPaddingInput;
	//*pLenOut = x;

	uscita:

	if ( NULL != pStreamNew )
	{
		free(pStreamNew);
		pStreamNew = NULL;
	}

	return pOutputStream;
}

// ****************************************************************** LZW **************************************************************

int IsLittleEndian(void)
{
	uint32_t w = 0x00000001;
	uint8_t *b = (uint8_t*)&w;

	return (b[0] ? 1 : 0);
}

int IsBigEndian(void)
{
	uint32_t w = 0x00000001;
	uint8_t *b = (uint8_t*)&w;

	return (b[0] ? 0 : 1);
}

// ----------------------------------------------------------------------------------------------------------------------------------------

unsigned char * lzwEncode(unsigned char * pInputStream, size_t len, uint32_t BitsOrder, uint32_t MaxBits, size_t *pLenOut, uint32_t *pErrorCode)
{
	uint32_t bits = BITS_MIN; // bits = 9
	uint32_t nextShift = 1 << BITS_MIN; // nextShift = 512;

	uint16_t code;
	uint16_t c;
	uint16_t nc;
	uint16_t nextCode = M_NEW;

	lzw_enc_t *pDict = NULL;
	uint8_t *pOutputStream = NULL;
	size_t lenOut = 0;

	size_t nBits = 0;
	uint32_t tmp = 0;

	uint32_t sizeOutputBlock;

	*pErrorCode = LZW_ERROR_NONE;
	*pLenOut = 0;

	pDict = (lzw_enc_t *)calloc(nextShift, sizeof(lzw_enc_t));
	if ( NULL == pDict )
	{
		*pErrorCode = LZW_ERROR_MEMALLOC_FAILED;
		return NULL;
	}

	if ( MaxBits > BITS_MAX )
		MaxBits = BITS_MAX;

	if ( MaxBits < BITS_MIN )
		MaxBits = DEFAULT_MAX_BITS;

	sizeOutputBlock = len * 3;
	pOutputStream = (uint8_t *)calloc(sizeOutputBlock, sizeof(uint8_t));
	if ( NULL == pOutputStream )
	{
		free(pDict);
		pDict = NULL;
		*pErrorCode = LZW_ERROR_MEMALLOC_FAILED;
		return NULL;
	}

	if ( BITS_ORDER_MSB_FIRST == BitsOrder )
	{
		write_bits_msb_first(M_CLR);
	}
	else
	{
		write_bits_lsb_first(M_CLR);
	}

	for ( code = *(pInputStream++); --len; )
	{
		c = *(pInputStream++);

		if ( (nc = pDict[code].next[c]) )
		{
			code = nc;
		}
		else
		{
			if ( BITS_ORDER_MSB_FIRST == BitsOrder )
			{
				write_bits_msb_first(code);
			}
			else
			{
				write_bits_lsb_first(code);
			}

			nc = pDict[code].next[c] = nextCode++;

			code = c;
		}

		if ( nextCode == nextShift )
		{
			bits++;

			if ( bits <= MaxBits )
			{
				// EXTEND TABLE
				lzw_enc_t *pTemp = NULL;
				int oldSize = nextShift;
				nextShift *= 2;
				pTemp = (lzw_enc_t *)realloc(pDict, nextShift * sizeof(lzw_enc_t));
				if ( NULL == pTemp )
				{
					free(pDict);
					pDict = NULL;
					free(pOutputStream);
					pOutputStream = NULL;
					*pErrorCode = LZW_ERROR_MEMALLOC_FAILED;
					return NULL;
				}
				pDict = pTemp;
				memset(pDict + oldSize, 0, oldSize * sizeof(lzw_enc_t));
			}
			else
			{
				// RESET TABLE
				if ( BITS_ORDER_MSB_FIRST == BitsOrder )
				{
					write_bits_msb_first(M_CLR);
				}
				else
				{
					write_bits_lsb_first(M_CLR);
				}

				bits = BITS_MIN;
				nextShift = 1 << BITS_MIN;
				nextCode = M_NEW;

				memset(pDict, 0, nextShift * sizeof(lzw_enc_t));
			}
		}
	}

	if ( BITS_ORDER_MSB_FIRST == BitsOrder )
	{
		write_bits_msb_first(code);
		write_bits_msb_first(M_EOD);
	}
	else
	{
		write_bits_lsb_first(code);
		write_bits_lsb_first(M_EOD);
	}

	if ( tmp )
	{
		if ( BITS_ORDER_MSB_FIRST == BitsOrder )
		{
			write_bits_msb_first(tmp);
		}
		else
		{
			write_bits_lsb_first(tmp);
		}
	}

	*pLenOut = lenOut;

	free(pDict);
	pDict = NULL;

	return pOutputStream;
}

unsigned char * lzwDecode(unsigned char * pInputStream, size_t len, uint32_t BitsOrder, size_t *pLenOut, uint32_t *pErrorCode)
{
	uint8_t *pOutputStream = NULL;
	size_t lenOut = 0;

	lzw_dec_t *pDict = NULL;

	uint32_t bits = BITS_MIN; // BITS = 9
	uint32_t nextShift = 1 << BITS_MIN; // bits = 512
	uint32_t nBits = 0;

	uint16_t code = 0;
	uint16_t c = 0;
	uint16_t t = 0;
	uint16_t nextCode = M_NEW;

	uint32_t tmp = 0;

	uint32_t sizeOutputBlock = 0;
	uint32_t i = 0;

	uint16_t mask[17] =
	{
		0x0000, 0x0001, 0x0003, 0x0007,
		0x000F, 0x001F, 0x003F, 0x007F,
		0x00FF, 0x01FF, 0x03FF, 0x07FF,
		0x0FFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
	};

	uint32_t curmask = mask[bits];

	*pErrorCode = LZW_ERROR_NONE;
	*pLenOut = 0;

	sizeOutputBlock = len * 3;
	pOutputStream = (uint8_t *)calloc(sizeOutputBlock, sizeof(uint8_t));
	if ( NULL == pOutputStream )
	{
		*pErrorCode = LZW_ERROR_MEMALLOC_FAILED;
		return NULL;
	}

	pDict = (lzw_dec_t *)calloc(nextShift, sizeof(lzw_dec_t));
	if ( NULL == pDict )
	{
		free(pOutputStream);
		pOutputStream = NULL;
		*pErrorCode = LZW_ERROR_MEMALLOC_FAILED;
		return NULL;
	}

	// CLEAR TABLE
	nextCode = M_NEW;
	nextShift = 1 << BITS_MIN;
	bits = BITS_MIN;
	memset(pDict, 0, nextShift * sizeof(lzw_dec_t));
	for ( i = 0; i < 256; i++ )
		pDict[i].c = i;

	while ( len > 0 )
	{
		if ( BITS_ORDER_MSB_FIRST == BitsOrder )
		{
			read_bits_msb_first();
		}
		else
		{
			read_bits_lsb_first();
		}

		if ( code == M_EOD )
			break;

		if ( code == M_CLR )
		{
			// CLEAR TABLE
			nextCode = M_NEW;
			nextShift = 1 << BITS_MIN;;
			bits = BITS_MIN;
			memset(pDict, 0, nextShift * sizeof(lzw_dec_t));
			for ( i = 0; i < 256; i++ )
				pDict[i].c = i;

			curmask = mask[bits];

			continue;
		}

		if ( code >= nextCode )
		{
			*pErrorCode = LZW_ERROR_UNEXPECTED_CODE;
			*pLenOut = lenOut;
			goto uscita;
		}

		pDict[nextCode].prev = c = code;

		while ( c > 255 )
		{
			t = pDict[c].prev;
			pDict[t].back = c;
			c = t;
		}

		pDict[nextCode - 1].c = c;

		while ( pDict[c].back )
		{
			while ( lenOut >= sizeOutputBlock )
			{
				uint8_t *pTemp = NULL;
				int oldSize = sizeOutputBlock;
				sizeOutputBlock *= 2;
				pTemp = (uint8_t *)realloc(pOutputStream, sizeOutputBlock * sizeof(uint8_t));
				if ( NULL == pTemp )
				{
					free(pDict);
					pDict = NULL;
					free(pOutputStream);
					pOutputStream = NULL;
					*pErrorCode = LZW_ERROR_MEMALLOC_FAILED;
					return NULL;
				}
				pOutputStream = pTemp;
				memset(pOutputStream + oldSize, 0, oldSize * sizeof(uint8_t));
			}

			pOutputStream[lenOut++] = pDict[c].c;

			t = pDict[c].back;
			pDict[c].back = 0;
			c = t;
		}

		while ( lenOut >= sizeOutputBlock )
		{
			uint8_t *pTemp = NULL;
			int oldSize = sizeOutputBlock;
			sizeOutputBlock *= 2;
			pTemp = (uint8_t *)realloc(pOutputStream, sizeOutputBlock * sizeof(uint8_t));
			if ( NULL == pTemp )
			{
				free(pDict);
				pDict = NULL;
				free(pOutputStream);
				pOutputStream = NULL;
				*pErrorCode = LZW_ERROR_MEMALLOC_FAILED;
				return NULL;
			}
			pOutputStream = pTemp;
			memset(pOutputStream + oldSize, 0, oldSize * sizeof(uint8_t));
		}

		pOutputStream[lenOut++] = pDict[c].c;

		if ( ++nextCode >= nextShift )
		{
			lzw_dec_t *pTemp = NULL;
			int oldSize = nextShift;

			if ( ++bits > (BITS_MAX + 1) )
			{
				*pErrorCode = LZW_ERROR_TOO_MANY_BITS;
				free(pOutputStream);
				pOutputStream = NULL;
				goto uscita;
			}

			nextShift *= 2;
			pTemp = (lzw_dec_t *)realloc(pDict, nextShift * sizeof(lzw_dec_t));
			if ( NULL == pTemp )
			{
				free(pDict);
				pDict = NULL;
				free(pOutputStream);
				pOutputStream = NULL;
				*pErrorCode = LZW_ERROR_MEMALLOC_FAILED;
				return NULL;
			}
			pDict = pTemp;
			memset(pDict + oldSize, 0, oldSize * sizeof(lzw_dec_t));

			curmask = mask[bits];
		}
	}

	if ( code != M_EOD )
	{
		*pErrorCode = LZW_ERROR_EOD_NOT_FOUND;
		free(pOutputStream);
		pOutputStream = NULL;
		goto uscita;
	}

	*pLenOut = lenOut;

uscita:

	if ( NULL != pDict )
	{
		free(pDict);
		pDict = NULL;
	}

	return pOutputStream;
}
