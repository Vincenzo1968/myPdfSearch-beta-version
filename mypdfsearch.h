/*
   Copyright (C) 2019 Vincenzo Lo Cicero

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

// https://www.adobe.com/content/dam/acom/en/devnet/pdf/pdfs/PDF32000_2008.pdf

// http://www.eprg.org/pdfcorner/text2pdf/



#ifndef __MYPDFSEARCH__
#define __MYPDFSEARCH__

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <wctype.h>
#include <stdint.h>

//#define HT_SIZE 8191
#define BLOCK_SIZE 4096
//#define BLOCK_SIZE 4096 * 21

#ifndef MAX_LEN_STR
	#define MAX_LEN_STR 1024
#endif

#ifndef MAX_WORDS_TO_SEARCH
	#define MAX_WORDS_TO_SEARCH 100
#endif

#define UNUSED(x) (void)(x)


typedef struct tagFilesList
{
	#if !defined(_WIN64) && !defined(_WIN32)
	char myPathName[PATH_MAX + 1];
	#endif
	char myFileName[PATH_MAX + 1];
	//char relativeName[PATH_MAX + 1];
	struct tagFilesList *next;
} FilesList;

#endif // __MYPDFSEARCH__


/*

MYDEBUG_PRINT_ON_ReadEncrypt_R

MYDEBUG_PRINT_SINGLE_CHAR_ENCODING
MYDEBUG_PRINT_SINGLE_CHAR_COORD

MYDEBUG_PRINT_ON_SHOW_COORDINATE_STRING
MYDEBUG_PRINT_ON_SHOW_STRING_OPERATOR
MYDEBUG_PRINT_ON_POSITIONING_STRING_OPERATOR
MYDEBUG_PRINT_ON_TEXT_STATE_OPERATOR
MYDEBUG_PRINT_ON_CTM_OPERATOR

MYPDFSEARCH_SHOW_TIME
MYDEBUG_PRINT_COUNT_CONTENT_TYPE
MYDEBUG_PRINT_DECODED_CONTENT_TOKENS
MYDEBUG_PRINT_DECODED_CONTENT_NUMSTACK

MYDEBUG_PRINT_ALL
MYDEBUG_PRINT_ON_MATCH
MYDEBUG_PRINT_ON_PARSE_OBJ
MYDEBUG_PRINT_ON_PARSE_XREF_STREAMOBJ
MYDEBUG_PRINT_ON_PARSE_STMOBJ
MYDEBUG_PRINT_ON_PARSE_STREAMOBJ
MYDEBUG_PRINT_ON_PARSE_STREAMXOBJ
MYDEBUG_PRINT_ON_PARSE_TOUNICODE_STREAM
MYDEBUG_PRINT_ON_PARSE_CMAP_STREAM
MYDEBUG_PRINT_ON_PARSE_PAGETREEOBJ
MYDEBUG_PRINT_ON_PARSE_LENGTHOBJ
MYDEBUG_PRINT_ON_PARSE_INTEGEROBJ
MYDEBUG_PRINT_ON_PARSE_RESOURCESDICT
MYDEBUG_PRINT_ON_PARSE_FONTDESCRIPTOROBJ
MYDEBUG_PRINT_ON_PARSE_FONTOBJ
MYDEBUG_PRINT_ON_PARSE_FONTOBJ_WIDTHS
MYDEBUG_PRINT_ON_PARSE_ENCODINGOBJ
MYDEBUG_PRINT_ON_PARSE_COT

MYDEBUG_PRINT_TST

MYDEBUG_PRINT_ON_GetNextToken_FN

MYDEBUG_PRINT_ON_getObjsOffsets_FN

MYDEBUG_PRINT_ON_ReadHeader_FN
MYDEBUG_PRINT_ON_ReadTrailer_FN
MYDEBUG_PRINT_ON_ReadTrailer_OBJ
MYDEBUG_PRINT_ON_ReadLastTrailer_FN
MYDEBUG_PRINT_ON_ReadLastTrailer_FN_BLOCCO
MYDEBUG_PRINT_ON_ReadTrailerBody_FN

MYDEBUG_PRINT_ON_ParseInfoObject_FN

MYDEBUG_PRINT_ON_ReadTrailerEncryptObject
MYDEBUG_PRINT_ON_PARSE_ENCRYPT_OBJ
MYDEBUG_PRINT_ON_Decrypt

MYDEBUG_PRINT_ON_ManageDecodeParams_FN
MYDEBUG_PRINT_ON_ParseTrailerXRefStreamObject_FN

MYDEBUG_PRINT_ON_PARSE_FN
MYDEBUG_PRINT_ON_ParseStmObj_FN
MYDEBUG_PRINT_ON_ManageContent_FN
MYDEBUG_PRINT_ON_ManageContent_FN_ShowResourceSelected
MYDEBUG_PRINT_ON_ManageContent_FN_ShowFontSelected
MYDEBUG_PRINT_ON_ManageContent_FN_ShowGsSelected
MYDEBUG_PRINT_ON_ManageContent_PrintPageNum
MYDEBUG_PRINT_ON_ManageContent_PrintContent
MYDEBUG_PRINT_ON_ManageContent_PrintContent_HEXCODECHAR
MYDEBUG_PRINT_ON_ManageContent_PrintStrings
MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL
MYDEBUG_PRINT_ON_ParseObject_FN
MYDEBUG_PRINT_ON_myTreeTraversePostOrderLeafOnly_FN
MYDEBUG_PRINT_ON_ParseStreamObject_FN
MYDEBUG_PRINT_ON_ParseDictionaryObject_FN
MYDEBUG_PRINT_ON_ParseLengthObject_FN
*/









// ***********************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************************
