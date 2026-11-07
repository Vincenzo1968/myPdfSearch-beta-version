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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>

#if defined(_WIN64) || defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

#include "myoctal.h"
#include "scanner.h"

#define MY_BYTE  uint8_t
#define MY_WORD  uint16_t
#define MY_DWORD uint32_t
#define MY_LONG  uint32_t

#define MY_MAKEWORD(a, b) ((MY_WORD)(((MY_BYTE)((MY_DWORD)(a) & 0xff)) | ((MY_WORD)((MY_BYTE)((MY_DWORD)(b) & 0xff))) << 8))

#define MY_MAKELONG(a, b) ((MY_LONG)(((MY_WORD)((MY_DWORD)(a) & 0xffff)) | ((MY_DWORD)((MY_WORD)((MY_DWORD)(b) & 0xffff))) << 16))

#define MY_LOWORD(l) ((MY_WORD)((MY_DWORD)(l) & 0xFFFF))
#define MY_HIWORD(l) ((MY_WORD)((MY_DWORD)(l) >> 16))
#define MY_LOBYTE(w) ((MY_BYTE)((MY_DWORD)(w) & 0xFF))
#define MY_HIBYTE(w) ((MY_BYTE)((MY_DWORD)(w) >> 8))

// typedef uint32_t* MY_DWORD_PTR;

// #define MY_MAKEWORD(a, b) ((MY_WORD)(((MY_BYTE)((MY_DWORD_PTR)(a) & 0xff)) | ((MY_WORD)((MY_BYTE)((MY_DWORD_PTR)(b) & 0xff))) << 8))

// #define MY_MAKELONG(a, b) ((MY_LONG)(((MY_WORD)((MY_DWORD_PTR)(a) & 0xffff)) | ((MY_DWORD)((MY_WORD)((MY_DWORD_PTR)(b) & 0xffff))) << 16))

// #define MY_LOWORD(l) ((MY_WORD)((MY_DWORD_PTR)(l) & 0xFFFF))
// #define MY_HIWORD(l) ((MY_WORD)((MY_DWORD_PTR)(l) >> 16))
// #define MY_LOBYTE(w) ((MY_BYTE)((MY_DWORD_PTR)(w) & 0xFF))
// #define MY_HIBYTE(w) ((MY_BYTE)((MY_DWORD_PTR)(w) >> 8))

static int read_pdf_literal_string(const unsigned char *input, int *pIndex, unsigned char **outBytes, size_t *outLen)
{
    int cursor;
    int depth = 1;
    size_t cap = 32;
    size_t len = 0;
    unsigned char *buf;

    if (input == NULL || pIndex == NULL ||
        outBytes == NULL || outLen == NULL || *pIndex < 0)
        return 0;

    *outBytes = NULL;
    *outLen = 0;

    buf = (unsigned char *)malloc(cap);
    if (buf == NULL)
        return 0;

    cursor = *pIndex;

    for (;;)
    {
        unsigned char ch = input[cursor];

        if (ch == '\0')
        {
            free(buf);
            return 0;
        }

        if (ch == '\\')
        {
            unsigned char out = 0;
            int bEmit = 1;

            cursor++;
            ch = input[cursor];
            if (ch == '\0') { free(buf); return 0; }

            switch (ch)
            {
                case 'n':  out = '\n'; break;
                case 'r':  out = '\r'; break;
                case 't':  out = '\t'; break;
                case 'b':  out = '\b'; break;
                case 'f':  out = '\f'; break;
                case '(':  out = '(';  break;
                case ')':  out = ')';  break;
                case '\\': out = '\\'; break;

                case '\r':
                    // continuazione di riga: \r[\n] rimosso 
                    cursor++;
                    if (input[cursor] == '\n')
                        cursor++;
                    bEmit = 0;
                    break;

                case '\n':
                    // continuazione di riga: \n rimosso 
                    cursor++;
                    bEmit = 0;
                    break;

                default:
                    if (ch >= '0' && ch <= '7')
                    {
                        int oct = ch - '0';

                        if (input[cursor + 1] >= '0' &&
                            input[cursor + 1] <= '7')
                        {
                            oct = oct * 8 + (input[cursor + 1] - '0');
                            cursor++;

                            if (input[cursor + 1] >= '0' &&
                                input[cursor + 1] <= '7')
                            {
                                int oct3 = oct * 8 +
                                           (input[cursor + 1] - '0');
                                if (oct3 <= 0xFF)
                                {
                                    oct = oct3;
                                    cursor++;
                                }
                            }
                        }
                        out = (unsigned char)(oct & 0xFF);
                    }
                    else
                    {
                        // backslash davanti a carattere sconosciuto:
                        // il backslash si ignora, si tiene il carattere 
                        out = ch;
                    }
                    break;
            }

            if (bEmit)
            {
                if (len + 1 > cap)
                {
                    unsigned char *tmp;
                    cap *= 2;
                    tmp = (unsigned char *)realloc(buf, cap);
                    if (tmp == NULL) { free(buf); return 0; }
                    buf = tmp;
                }
                buf[len++] = out;
            }

            cursor++;
            continue;
        }

        if (ch == '(')
        {
            depth++;
            if (len + 1 > cap)
            {
                unsigned char *tmp;
                cap *= 2;
                tmp = (unsigned char *)realloc(buf, cap);
                if (tmp == NULL) { free(buf); return 0; }
                buf = tmp;
            }
            buf[len++] = '(';
            cursor++;
            continue;
        }

        if (ch == ')')
        {
            depth--;
            if (depth == 0)
            {
                cursor++;
                break;
            }
            if (len + 1 > cap)
            {
                unsigned char *tmp;
                cap *= 2;
                tmp = (unsigned char *)realloc(buf, cap);
                if (tmp == NULL) { free(buf); return 0; }
                buf = tmp;
            }
            buf[len++] = ')';
            cursor++;
            continue;
        }

        if (len + 1 > cap)
        {
            unsigned char *tmp;
            cap *= 2;
            tmp = (unsigned char *)realloc(buf, cap);
            if (tmp == NULL) { free(buf); return 0; }
            buf = tmp;
        }
        buf[len++] = ch;
        cursor++;
    }

    *pIndex  = cursor;
    *outBytes = buf;
    *outLen  = len;
    return 1;
}

int MakeAndOpenErrorsFile(Params *pParams, int bBom)
{
	int retValue = 1;

	#if !defined(_WIN64) && !defined(_WIN32)
	unsigned char szBOM[21];
	#endif

	if ( NULL != pParams->fpErrors )
		goto uscita;

	#if !defined(_WIN64) && !defined(_WIN32)
	// UTF-8 BOM -> EF BB BF
	szBOM[0] = 0xEF;
	szBOM[1] = 0xBB;
	szBOM[2] = 0xBF;
	szBOM[3] = '\0';
	#endif

	//pParams->fpErrors = fopen("AAA_mypdfsearch_parsing_errors.txt", "wb");
	pParams->fpErrors = fopen("AAA_mypdfsearch_parsing_errors.txt", "a");
	if ( pParams->fpErrors == NULL )
	{
		wprintf(L"\n\nERRORE: impossibile creara il file specificato per i messaggi d'errore.\n\n");
		retValue = 0;
		goto uscita;
	}

	if ( !bBom )
		goto uscita;

	#if defined(_WIN64) || defined(_WIN32)
	_setmode(_fileno(pParams->fpErrors), _O_U8TEXT);
	#endif

	#if !defined(_WIN64) && !defined(_WIN32)
	// UTF-8 BOM -> EF BB BF
	fwprintf(pParams->fpErrors, L"%s", szBOM);
	#endif

	uscita:

	return retValue;
}

int myShowErrorMessage(Params *pParams, const char *szMsg, int bPrintToErrorFile)
{
	if ( bPrintToErrorFile )
	{
		if ( !MakeAndOpenErrorsFile(pParams, 1) )
			return 0;

		#if !defined(_WIN64) && !defined(_WIN32)
		fwprintf(pParams->fpErrors, L"\nFile: '%s'\n", pParams->szFileName);
		#else
		int lung = strnlen(pParams->szFileName, MAX_LEN_STR);
		fwprintf(pParams->fpErrors, L"\nFILE = <");
		for ( int idx = 0; idx < lung; idx++ )
		{
			fwprintf(pParams->fpErrors, L"%c", pParams->szFileName[idx]);
		}
		fwprintf(pParams->fpErrors, L">\n");
		#endif

		fwprintf(pParams->fpErrors, L"\t%s\n", szMsg);
	}
	else
	{
		wprintf(L"%s\n", szMsg);
	}

	return 1;
}

int myCodepointToUtf8(uint32_t codepoint, uint32_t *pUtf8)
{
	uint8_t bytes[4];

	int numBytes = 0;

	uint32_t nTemp;
	int j;
	int nShift;

	if (codepoint <= 0x7F)
	{
		bytes[0] = 0x00;
		numBytes = 1;
	}
	else if (codepoint <= 0x7FF)
	{
		bytes[0] = 0xC0;
		numBytes = 2;
	}
	else if (codepoint <= 0xFFFF)
	{
		bytes[0] = 0xE0;
		numBytes = 3;
	}
	else if (codepoint <= 0x10FFFF)
	{
		bytes[0] = 0xF0;
		numBytes = 4;
	}
	else
	{
		// illegal!
		//printf("\nIllegal codepoint\n");
		return 0;
	}

	for(int i = 1; i < numBytes; ++i)
	{
		bytes[numBytes-i] = 0x80 | (uint8_t) (codepoint & 0x3F);
		codepoint >>= 6;
	}

	bytes[0] |= (uint8_t) codepoint;

	*pUtf8 = 0;

	nShift = 0;
	for ( j = numBytes - 1; j >= 0; j-- )
	{
		nTemp = bytes[j];
		nTemp <<= nShift;
		*pUtf8 |= nTemp;
		nShift += 8;
	}

	return 1;
}

void PrintTokenTrailer(Token *pToken, char cCarattereIniziale, char cCarattereFinale, int bPrintACapo)
{
	if ( cCarattereIniziale != '\0' )
		wprintf(L"%c", cCarattereIniziale);

	switch ( pToken->Type )
	{
		case T_NAME:
			wprintf(L"T_NAME = '%s'", pToken->vString);
			break;
		case T_STRING:
			wprintf(L"T_STRING = '%s'", pToken->vString);
			break;
		case T_STRING_LITERAL:
			wprintf(L"T_STRING_LITERAL = '%s'", pToken->vString);
			break;
		case T_STRING_HEXADECIMAL:
			wprintf(L"T_STRING_HEXADECIMAL = '%s'", pToken->vString);
			break;
		case T_INT_LITERAL:
			wprintf(L"T_INT_LITERAL = %d", pToken->vInt);
			break;
		case T_REAL_LITERAL:
			wprintf(L"T_REAL_LITERAL = %f", pToken->vDouble);
			break;
		case T_KW_NULL:
			wprintf(L"T_KW_NULL");
			break;
		case T_KW_TRUE:
			wprintf(L"T_KW_TRUE");
			break;
		case T_KW_FALSE:
			wprintf(L"T_KW_FALSE");
			break;
		case T_ERROR:
			wprintf(L"T_ERROR");
			break;
		case T_UNKNOWN:
			wprintf(L"T_UNKNOWN");
			break;
		case T_EOF:
			wprintf(L"T_EOF");
			break;
		case T_OPAREN:		// '('
			wprintf(L"T_OPAREN = '('");
			break;
		case T_CPAREN:		// ')'
			wprintf(L"T_CPAREN = ')'");
			break;
		case T_QOPAREN:		// '['
			wprintf(L"T_QOPAREN ");
			break;
		case T_QCPAREN:		// ']'
			wprintf(L"T_QCPAREN ");
			break;
		case T_DICT_BEGIN:   // "<<"
			wprintf(L"T_DICT_BEGIN = '<<'");
			break;
		case T_DICT_END:     // ">>"
			wprintf(L"T_DICT_END = '>>'");
			break;
		case T_KW_OBJ:
			wprintf(L"T_KW_OBJ");
			break;
		case T_KW_ENDOBJ:
			wprintf(L"T_KW_ENDOBJ");
			break;
		case T_KW_STREAM:
			wprintf(L"T_KW_STREAM");
			break;
		case T_KW_ENDSTREAM:
			wprintf(L"T_KW_ENDSTREAM");
			break;
		case T_KW_R:
			wprintf(L"T_KW_R");
			break;
		case T_CONTENT_LQUOTE:   // "<"
			wprintf(L"T_CONTENT_LQUOTE = '<'");
			break;
		case T_CONTENT_RQUOTE:   // ">"
			wprintf(L"T_CONTENT_RQUOTE = '>'");
			break;
		case T_CONTENT_OP_Do:
			wprintf(L"T_CONTENT_OP_Do = 'Do'");
			break;
		case T_CONTENT_OP_BT:
			wprintf(L"T_CONTENT_OP_BT = 'BT -> Begin Text'");
			break;
		case T_CONTENT_OP_ET:
			wprintf(L"T_CONTENT_OP_ET = 'ET -> End Text'");
			break;
		case T_CONTENT_OP_TD:
			wprintf(L"T_CONTENT_OP_TD = 'TD'");
			break;
		case T_CONTENT_OP_Td:
			wprintf(L"T_CONTENT_OP_Td = 'Td'");
			break;
		case T_CONTENT_OP_Tm:
			wprintf(L"T_CONTENT_OP_Tm = 'Tm'");
			break;
		case T_CONTENT_OP_TSTAR:
			wprintf(L"T_CONTENT_OP_TSTAR = 'T*'");
			break;
		case T_CONTENT_OP_TJ:
			wprintf(L"T_CONTENT_OP_TJ = 'TJ'");
			break;
		case T_CONTENT_OP_Tj:
			wprintf(L"T_CONTENT_OP_Tj = 'Tj'");
			break;
		case T_CONTENT_OP_SINGLEQUOTE:
			wprintf(L"T_CONTENT_OP_SINGLEQUOTE = '");
			break;
		case T_CONTENT_OP_DOUBLEQUOTE:
			wprintf(L"T_CONTENT_OP_DOUBLEQUOTE = '\"'");
			break;
		case T_LEFT_CURLY_BRACKET:
			wprintf(L"T_LEFT_CURLY_BRACKET = '{'");
			break;
		case T_RIGHT_CURLY_BRACKET:
			wprintf(L"T_RIGHT_CURLY_BRACKET = '}'");
			break;
		case T_VOID_STRING:
			wprintf(L"T_VOID_STRING");
			break;
		default:
			wprintf(L"TOKEN n° -> %d", pToken->Type);
			break;
	}

	if ( cCarattereFinale != '\0' )
		wprintf(L"%c", cCarattereFinale);

	if ( bPrintACapo )
		wprintf(L"\n");
}

int matchTrailer(Params *pParams, TokenTypeEnum ExpectedToken, char *pszFunctionName)
{
	int retValue = 1;

	if ( pParams->myToken.Type == ExpectedToken )
	{
		GetNextToken(pParams);
	}
	else
	{
		retValue = 0;

		if ( NULL != pszFunctionName )
		{
			snprintf(pParams->szError, 1024, "\nFUNZIONE match(richiamata dalla funzione '%s' -> errore di sintassi: Atteso token n° %d, trovato token n° %d\n", pszFunctionName, ExpectedToken, pParams->myToken.Type);
			myShowErrorMessage(pParams, pParams->szError, 1);
			//fwprintf(pParams->fpErrors, L"\nFUNZIONE match(richiamata dalla funzione '%s' -> errore di sintassi: Atteso token n° %d, trovato token n° %d\n", pszFunctionName, ExpectedToken, pParams->myToken.Type);
			//wprintf(L"\nFUNZIONE match(richiamata dalla funzione '%s' -> errore di sintassi: Atteso token n° %d, trovato token n° %d\n", pszFunctionName, ExpectedToken, pParams->myToken.Type);
		}
		else
		{
			snprintf(pParams->szError, 1024, "\nFUNZIONE match -> errore di sintassi: Atteso token n° %d, trovato token n° %d\n", ExpectedToken, pParams->myToken.Type);
			myShowErrorMessage(pParams, pParams->szError, 1);
			//fwprintf(pParams->fpErrors, L"\nFUNZIONE match -> errore di sintassi: Atteso token n° %d, trovato token n° %d\n", ExpectedToken, pParams->myToken.Type);
			//wprintf(L"\nFUNZIONE match -> errore di sintassi: Atteso token n° %d, trovato token n° %d\n", ExpectedToken, pParams->myToken.Type);
		}
	}

#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_MATCH)

	if ( 0 == retValue )
		wprintf(L"Token atteso : ");

	wprintf(L"MATCH ");
	switch ( ExpectedToken )
	{
		case T_ERROR:
			wprintf(L"T_ERROR");
			break;
		case T_UNKNOWN:
			wprintf(L"T_UNKNOWN");
			break;
		case  T_EOF:
			wprintf(L"T_EOF");
			break;
		case T_STRING:
			wprintf(L"T_STRING");
			break;
		case T_STRING_LITERAL:
			wprintf(L"T_STRING_LITERAL");
			break;
		case T_STRING_HEXADECIMAL:
			wprintf(L"T_STRING_HEXADECIMAL");
			break;
		case T_NAME:
			wprintf(L"T_NAME");
			break;
		case T_INT_LITERAL:
			wprintf(L"T_INT_LITERAL");
			break;
		case T_REAL_LITERAL:
			wprintf(L"T_REAL_LITERAL");
			break;
		case T_OPAREN:		// '('
			wprintf(L"T_OPAREN");
			break;
		case T_CPAREN:		// ')'
			wprintf(L"T_CPAREN");
			break;
		case T_QOPAREN:		// '['
			wprintf(L"T_QOPAREN");
			break;
		case T_QCPAREN:		// ']'
			wprintf(L"T_QCPAREN");
			break;
		case T_DICT_BEGIN:   // "<<"
			wprintf(L"T_DICT_BEGIN");
			break;
		case T_DICT_END:     // ">>"
			wprintf(L"T_DICT_END");
			break;
		case T_KW_NULL:
			wprintf(L"T_KW_NULL");
			break;
		case T_KW_OBJ:
			wprintf(L"T_KW_OBJ");
			break;
		case T_KW_ENDOBJ:
			wprintf(L"T_KW_ENDOBJ");
			break;
		case T_KW_STREAM:
			wprintf(L"T_KW_STREAM");
			break;
		case T_KW_ENDSTREAM:
			wprintf(L"T_KW_ENDSTREAM");
			break;
		case T_KW_FALSE:
			wprintf(L"T_KW_FALSE");
			break;
		case T_KW_TRUE:
			wprintf(L"T_KW_TRUE");
			break;
		case T_KW_R:
			wprintf(L"T_KW_R");
			break;
		default:
			break;
	}

	if ( 0 == retValue )
		wprintf(L" <> Token trovato : ");

	PrintTokenTrailer(&(pParams->myToken), '\n', '\n', 1);

#endif

	return retValue;
}

// trailerbody      : T_DICT_BEGIN traileritems T_DICT_END;
int trailerbody(Params *pParams)
{
	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
	PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
	#endif

	if ( !matchTrailer(pParams, T_DICT_BEGIN, "trailerbody") )
	{
		mynumstacklist_Free( &(pParams->myNumStack) );

		wprintf(L"\nBLOCCO ERRATO INIZIO\n");
		for (int k = 0; k < pParams->blockLen; k++ )
		{
			wprintf(L"%c", pParams->myBlock[k]);
		}
		wprintf(L"\nBLOCCO ERRATO FINE\n");

		return 0;
	}

	wprintf(L"\nBLOCCO CORRETTO INIZIO\n");
	for (int k = 0; k < pParams->blockLen; k++ )
	{
		wprintf(L"%c", pParams->myBlock[k]);
	}
	wprintf(L"\nBLOCCO CORRETTO FINE\n");


	if ( !traileritems(pParams) )
	{
		mynumstacklist_Free( &(pParams->myNumStack) );
		return 0;
	}

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
	PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
	#endif

	if ( !matchTrailer(pParams, T_DICT_END, "trailerbody") )
	{
		mynumstacklist_Free( &(pParams->myNumStack) );
		return 0;
	}

	if ( pParams->myPdfTrailer.Size <= 0 )
	{
		snprintf(pParams->szError, 1024, "Error parsing trailerbody: trailer Size value must be > 0 %d\n", pParams->myToken.Type);
		myShowErrorMessage(pParams, pParams->szError, 1);
		//wprintf(L"Errore parsing trailerbody: atteso T_KW_R, trovato %d\n", pParams->myToken.Type);
		//fwprintf(pParams->fpErrors, L"Error parsing trailerbody: trailer Size value must be > 0 %d\n", pParams->myToken.Type);
		return 0;
	}

	if ( pParams->myPdfTrailer.Root.Number <= 0 )
	{
		mynumstacklist_Free( &(pParams->myNumStack) );

		snprintf(pParams->szError, 1024, "Error parsing trailerbody: trailer Root value must be > 0 %d\n", pParams->myToken.Type);
		myShowErrorMessage(pParams, pParams->szError, 1);
		//wprintf(L"Errore parsing trailerbody: atteso T_KW_R, trovato %d\n", pParams->myToken.Type);
		//fwprintf(pParams->fpErrors, L"Error parsing trailerbody: trailer Root value must be > 0 %d\n", pParams->myToken.Type);
		return 0;
	}

	if ( pParams->myPdfTrailer.Encrypt.Number <= 0 )
	{
		mynumstacklist_Free( &(pParams->myNumStack) );

		snprintf(pParams->szError, 1024, "Error parsing trailerbody: trailer Encrypt value must be > 0 %d\n", pParams->myToken.Type);
		myShowErrorMessage(pParams, pParams->szError, 1);
		//wprintf(L"Errore parsing trailerbody: atteso T_KW_R, trovato %d\n", pParams->myToken.Type);
		//fwprintf(pParams->fpErrors, L"Error parsing trailerbody: trailer Encrypt value must be > 0 %d\n", pParams->myToken.Type);
		return 0;
	}

	return 1;
}

// traileritems     : {T_NAME trailerobj};
int traileritems(Params *pParams)
{
	while ( pParams->myToken.Type == T_NAME )
	{
		strncpy(pParams->szCurrKeyName, pParams->myToken.vString, 4096 - 1);

		if ( strncmp(pParams->szCurrKeyName, "Encrypt", 4096) == 0  )
		{
			pParams->isEncrypted = 1;
		}

		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
		PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
		#endif

		GetNextToken(pParams);

		if ( !trailerobj(pParams) )
			return 0;
	}

	return 1;
}

/*
trailerobj : T_NAME
            | T_INT_LITERAL [ T_INT_LITERAL T_KW_R ]
            | T_STRING_LITERAL
            | T_STRING_HEXADECIMAL
            | T_KW_TRUE
            | T_KW_FALSE
            | T_QOPAREN trailerarrayobjs T_QCPAREN
            | T_DICT_BEGIN trailerdictobjs T_DICT_END
            ;
*/
int trailerobj(Params *pParams)
{
	double dNum;
	int iNum = -1;
	int iGen = -1;

	switch ( pParams->myToken.Type )
	{
		case T_NAME:
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
			PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
			#endif

			GetNextToken(pParams);

			break;
		case T_INT_LITERAL:
			mynumstacklist_Push(&(pParams->myNumStack), pParams->myToken.vInt);

			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
			PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
			#endif

			GetNextToken(pParams);

			if ( pParams->myToken.Type == T_INT_LITERAL )
			{
				mynumstacklist_Push(&(pParams->myNumStack), pParams->myToken.vInt);

				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
				PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
				#endif

				GetNextToken(pParams);

				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
				PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
				#endif

				if ( !matchTrailer(pParams, T_KW_R, "trailerobj") )
				{
					snprintf(pParams->szError, 1024, "Errore parsing trailerobj: atteso T_KW_R, trovato %d\n", pParams->myToken.Type);
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore parsing trailerobj: atteso T_KW_R, trovato %d\n", pParams->myToken.Type);
					//fwprintf(pParams->fpErrors, L"Errore parsing trailerobj: atteso T_KW_R, trovato %d\n", pParams->myToken.Type);
					return 0;
				}

				mynumstacklist_Pop(&(pParams->myNumStack), &dNum);
				iGen = (int)dNum;
				mynumstacklist_Pop(&(pParams->myNumStack), &dNum);
				iNum = (int)dNum;

				//wprintf(L"trailerobj OBJECT REFERENCE -> %d %d R\n", iNum, iGen);

				if ( strncmp(pParams->szCurrKeyName, "Root", 4096) == 0  )
				{
					if ( iGen < 0 )
					{
						snprintf(pParams->szError, 1024, "Errore parsing trailerobj: Root value shall be an indirect reference\n");
						myShowErrorMessage(pParams, pParams->szError, 1);
						//wprintf(L"Errore parsing trailerobj: Root value shall not be an indirect reference");
						//fwprintf(pParams->fpErrors, L"Errore parsing trailerobj: Root value shall be an indirect reference\n");
						return 0;
					}

					pParams->myPdfTrailer.Root.Number = iNum;
					pParams->myPdfTrailer.Root.Generation = iGen;
				}
				else if ( strncmp(pParams->szCurrKeyName, "Encrypt", 4096) == 0  )
				{
					if ( iGen < 0 )
					{
						snprintf(pParams->szError, 1024, "Errore parsing trailerobj: Encrypt value shall be an indirect reference\n");
						myShowErrorMessage(pParams, pParams->szError, 1);
						//wprintf(L"Errore parsing trailerobj: Encrypt value shall not be an indirect reference");
						//fwprintf(pParams->fpErrors, L"Errore parsing trailerobj: Encrypt value shall be an indirect reference\n");
						return 0;
					}

					pParams->myPdfTrailer.Encrypt.Number = iNum;
					pParams->myPdfTrailer.Encrypt.Generation = iGen;

					//#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ) || defined(MYDEBUG_PRINT_ON_ReadEncrypt_R)
					//wprintf(L"Encrypt obj:\n");
					//wprintf(L"\tpParams->myPdfTrailer.Encrypt.Number = %u\n", pParams->myPdfTrailer.Encrypt.Number);
					//wprintf(L"\tpParams->myPdfTrailer.Encrypt.Generation = %u\n", pParams->myPdfTrailer.Encrypt.Generation);
					//#endif
				}
				else if ( strncmp(pParams->szCurrKeyName, "Size", 4096) == 0  )
				{
					if ( iGen < 0 )
					{
						snprintf(pParams->szError, 1024, "Errore parsing trailerobj: Size value shall be an indirect reference\n");
						myShowErrorMessage(pParams, pParams->szError, 1);
						//wprintf(L"Errore parsing trailerobj: Size value shall not be an indirect reference");
						//fwprintf(pParams->fpErrors, L"Errore parsing trailerobj: Size value shall be an indirect reference\n");
						return 0;
					}
				}
			}
			else
			{
				mynumstacklist_Pop(&(pParams->myNumStack), &dNum);
				iNum = (int)dNum;

				if ( strncmp(pParams->szCurrKeyName, "Size", 4096) == 0 )
				{
					pParams->myPdfTrailer.Size = iNum;
				}
				else if ( strncmp(pParams->szCurrKeyName, "Prev", 4096) == 0  )
				{
					if ( iGen < 0 )
					{
						snprintf(pParams->szError, 1024, "Errore parsing trailerobj: Prev value shall be an indirect reference\n");
						myShowErrorMessage(pParams, pParams->szError, 1);
						//wprintf(L"Errore parsing trailerobj: Prev value shall not be an indirect reference");
						//fwprintf(pParams->fpErrors, L"Errore parsing trailerobj: Prev value shall be an indirect reference\n");
						return 0;
					}

					pParams->myPdfTrailer.Prev = iNum;
				}
			}
			break;
		case T_STRING_LITERAL:
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
			PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
			#endif
			GetNextToken(pParams);
			break;
		case T_STRING_HEXADECIMAL:
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
			PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
			#endif
			GetNextToken(pParams);
			break;
		case T_KW_TRUE:
		case T_KW_FALSE:
			// IGNORIAMO
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
			PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
			#endif
			GetNextToken(pParams);
			break;
		case T_QOPAREN:
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
			PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
			#endif

			pParams->countArrayScope = 1;

			GetNextToken(pParams);

			if ( !trailerarrayobjs(pParams) )
				return 0;

			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
			PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
			#endif

			if ( !matchTrailer(pParams, T_QCPAREN, "trailerobj") )
				return 0;

			pParams->countArrayScope = 0;
			break;
		case T_DICT_BEGIN:
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
			PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
			#endif

			GetNextToken(pParams);

			if ( !trailerdictobjs(pParams) )
				return 0;

			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
			PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
			#endif

			if ( !matchTrailer(pParams, T_DICT_END, "trailerobj") )
				return 0;
			break;
		default:
			snprintf(pParams->szError, 1024, "Errore parsing trailerobj: token non valido: %d\n", pParams->myToken.Type);
			myShowErrorMessage(pParams, pParams->szError, 1);
			//wprintf(L"Errore parsing trailerobj: token non valido: %d\n", pParams->myToken.Type);
			//fwprintf(pParams->fpErrors, L"Errore parsing trailerobj: token non valido: %d\n", pParams->myToken.Type);
			PrintTokenTrailer(&(pParams->myToken), '\0', ' ', 1);
			return 0;
			break;
	}

	return 1;
}

// trailerarrayobjs : {T_INT_LITERAL T_INT_LITERAL T_KW_R};
int trailerarrayobjs(Params *pParams)
{
	do
	{
		if ( pParams->myToken.Type == T_QCPAREN )
			pParams->countArrayScope--;
		else if ( pParams->myToken.Type == T_QOPAREN )
			pParams->countArrayScope++;

		if ( pParams->myToken.Type == T_ERROR || pParams->myToken.Type == T_EOF )
		{
			snprintf(pParams->szError, 8192, "Errore parsing trailerarrayobjs: token non valido: %d\n", pParams->myToken.Type);
			myShowErrorMessage(pParams, pParams->szError, 1);
			//wprintf(L"Errore parsing trailerarrayobjs: token non valido: %d\n", pParams->myToken.Type);
			//fwprintf(pParams->fpErrors, L"Errore parsing trailerarrayobjs: token non valido: %d\n", pParams->myToken.Type);
			PrintTokenTrailer(&(pParams->myToken), '\0', ' ', 1);
			return 0;
		}

		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
		PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
		#endif

		GetNextToken(pParams);

	} while ( pParams->myToken.Type != T_QCPAREN || pParams->countArrayScope > 1 );

	return 1;
}

// trailerdictobjs  : {T_NAME trailerobj};
int trailerdictobjs(Params *pParams)
{
	while ( T_NAME == pParams->myToken.Type )
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_OBJ)
		PrintTokenTrailer(&(pParams->myToken), ' ', ' ', 1);
		#endif

		GetNextToken(pParams);

		if ( !trailerobj(pParams) )
			return 0;
	}

	return 1;
}

// Restituisce il valore numerico di una cifra esadecimale.
// Restituisce -1 se il carattere non e' valido.
static int hex_digit_value(unsigned char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return -1;
}

// Riconosce i caratteri di spaziatura PDF più comuni.
static int is_pdf_whitespace(unsigned char c)
{
    return c == ' '  ||
           c == '\t' ||
           c == '\r' ||
           c == '\n' ||
           c == '\f';
}

// Decodifica una stringa esadecimale PDF.
// All'ingresso, *pIndex punta al primo carattere
// successivo al delimitatore '<'.
// All'uscita:
//   - *outBytes contiene i byte decodificati;
//   - *outLen contiene la lunghezza in byte;
//   - *pIndex punta al carattere successivo a '>'.
// Restituisce 1 in caso di successo, 0 in caso di errore.
// Non aggiunge un terminatore NUL ai dati binari.

static int read_pdf_hex_string(
    const unsigned char *input,
    int *pIndex,
    unsigned char **outBytes,
    size_t *outLen)
{
    int cursor;
    int highNibble = -1;
    size_t hexDigits = 0;
    size_t byteLen;
    size_t outPos = 0;
    unsigned char *buffer;
    int i;

    if (input == NULL ||
        pIndex == NULL ||
        outBytes == NULL ||
        outLen == NULL ||
        *pIndex < 0)
    {
        return 0;
    }

    *outBytes = NULL;
    *outLen = 0;

    // Prima passata: verifica il contenuto e conta
    // le cifre esadecimali fino al delimitatore '>'.
    cursor = *pIndex;

    while (input[cursor] != '\0' && input[cursor] != '>')
    {
        unsigned char ch = input[cursor++];

        if (is_pdf_whitespace(ch))
            continue;

        if (hex_digit_value(ch) < 0)
            return 0;

        hexDigits++;
    }

    // La stringa deve terminare con '>'.
    if (input[cursor] != '>')
        return 0;

    
    // Due cifre formano un byte.
    // Se le cifre sono dispari, l'ultima cifra viene
    // completata a destra con uno zero, come previsto
    // dalla sintassi delle stringhe esadecimali PDF.
    
    byteLen = hexDigits / 2 + hexDigits % 2;

    // Allochiamo almeno un byte anche per una stringa
    // vuota, così NULL continua a indicare un errore
    // oppure un buffer non ancora inizializzato.
    buffer = (unsigned char *)malloc(byteLen > 0 ? byteLen : 1);

    if (buffer == NULL)
        return 0;

    // Seconda passata: converte le cifre in byte reali.
    for (i = *pIndex; i < cursor; i++)
    {
        unsigned char ch = input[i];
        int value;

        if (is_pdf_whitespace(ch))
            continue;

        value = hex_digit_value(ch);

        if (value < 0)
        {
            free(buffer);
            return 0;
        }

        if (highNibble < 0)
        {
            highNibble = value;
        }
        else
        {
            buffer[outPos++] =
                (unsigned char)((highNibble << 4) | value);

            highNibble = -1;
        }
    }

    // Se rimane una sola cifra, completa il byte
    // con uno zero come nibble meno significativo.
    if (highNibble >= 0)
        buffer[outPos++] = (unsigned char)(highNibble << 4);

    *outBytes = buffer;
    *outLen = outPos;

    // Salta anche il delimitatore finale '>'.
    *pIndex = cursor + 1;

    return 1;
}

int ReadTrailerBody(Params *pParams, unsigned char *szInput, int index)
{
	unsigned char c;
	TrailerStates myState = S_Error;
	int x;
	char szTemp[1024];
	int bSizeYes, bRootYes;

	int bInfoState = 0;
	int bStoreRootOrInfo = 0;

	c = szInput[index++];
	while ( (c == '\n' || c == '\r' || c == '\t' || c == ' ' || c == '\f' || c == '\b') )
	{
		c = szInput[index++];
	}

	bSizeYes = bRootYes = 0;

	pParams->myPdfTrailer.Prev = 0;

	if ( c != '<' )
	{
		snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 1: atteso '<' trovato '%c'\n", c);
		myShowErrorMessage(pParams, pParams->szError, 1);
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		myShowErrorMessage(pParams, pParams->szError, 0);
		#endif
		return 0;
	}

	c = szInput[index++];

	if ( c != '<' )
	{
		snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 2: atteso '<' trovato '%c'\n", c);
		myShowErrorMessage(pParams, pParams->szError, 1);
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		myShowErrorMessage(pParams, pParams->szError, 0);
		#endif
		return 0;
	}

ciclo1:

	c = szInput[index++];
	while ( (c == '\n' || c == '\r' || c == '\t' || c == ' ' || c == '\f' || c == '\b') )
	{
		c = szInput[index++];
	}

ciclo2:
	if ( c == '>' )
		goto ciclo3;

	if ( c != '/' )
	{
		snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 3: atteso '/' trovato '%c'\n", c);
		myShowErrorMessage(pParams, pParams->szError, 1);
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		myShowErrorMessage(pParams, pParams->szError, 0);
		#endif
		return 0;
	}

	c = szInput[index++];
	switch ( c )
	{
		case 'S':
			c = szInput[index++];
			if ( c != 'i' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( c != 'z' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( c != 'e' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( (c != ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			myState = S_Size;
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
			wprintf(L"/Size OK\n");
			#endif

			break;
		case 'P':
			c = szInput[index++];
			if ( c != 'r' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( c != 'e' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( c != 'v' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( (c != ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			myState = S_Prev;
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
			wprintf(L"/Prev OK\n");
			#endif

			break;
		case 'R':
			c = szInput[index++];
			if ( c != 'o' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( c != 'o' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( c != 't' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( (c != ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			myState = S_Root;
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
			wprintf(L"/Root OK\n");
			#endif

			break;
		case 'I':
			c = szInput[index++];
			if ( c == 'D' )
			{
				myState = S_ID;
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
				wprintf(L"/ID OK\n");
				#endif
				c = szInput[index++];
				if ( (c != '[' && c != ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
				{
					while ( c != '/' )
					{
						c = szInput[index++];
						if ( c == '>' )
						{
							c = szInput[index++];
							goto ciclo4;
						}
					}
					goto ciclo2;
				}
			}
			else if ( c == 'n' )
			{
				c = szInput[index++];
				if ( c != 'f' )
				{
					while ( c != '/' )
					{
						c = szInput[index++];
						if ( c == '>' )
						{
							c = szInput[index++];
							goto ciclo4;
						}
					}
					goto ciclo2;
				}

				c = szInput[index++];
				if ( c != 'o' )
				{
					while ( c != '/' )
					{
						c = szInput[index++];
						if ( c == '>' )
						{
							c = szInput[index++];
							goto ciclo4;
						}
					}
					goto ciclo2;
				}

				myState = S_Info;
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
				wprintf(L"/Info OK\n");
				#endif

				c = szInput[index++];
				if ( (c != ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
				{
					while ( c != '/' )
					{
						c = szInput[index++];
						if ( c == '>' )
						{
							c = szInput[index++];
							goto ciclo4;
						}
					}
					goto ciclo2;
				}
			}
			else
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}
			break;
		case 'E':
			c = szInput[index++];
			if ( c != 'n' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( c != 'c' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( c != 'r' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( c != 'y' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( c != 'p' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( c != 't' )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			c = szInput[index++];
			if ( (c != ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
			{
				while ( c != '/' )
				{
					c = szInput[index++];
					if ( c == '>' )
					{
						c = szInput[index++];
						goto ciclo4;
					}
				}
				goto ciclo2;
			}

			pParams->isEncrypted = 1;
			myState = S_Encrypt;
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
			wprintf(L"/Encrypt OK\n");
			#endif

			break;
		default:
			while ( c != '/' )
			{
				c = szInput[index++];
				if ( c == '>' )
				{
					c = szInput[index++];
					goto ciclo4;
				}
			}
			goto ciclo2;
			break;
	}


	c = szInput[index++];
	while ( (c == '\n' || c == '\r' || c == '\t' || c == ' ' || c == '\f' || c == '\b') )
	{
		c = szInput[index++];
	}

	if ( c == '\0' )
	{
		snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 4: atteso '<' trovato carattere NULL -> '\\0'\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
		myShowErrorMessage(pParams, pParams->szError, 0);
		#endif
		return 0;
	}

	switch ( myState )
	{
		case S_Size:
			x = 0;
			while ( c >= '0' && c <= '9' && x < 1024 )
			{
				szTemp[x++] = c;
				c = szInput[index++];
			}

			if ( x <= 0 || x >= 1024 )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 5\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			szTemp[x] = '\0';
			if ( pParams->myPdfTrailer.Size <= 0 )
				pParams->myPdfTrailer.Size = atoi(szTemp);
			bSizeYes = 1;
			break;
		case S_Prev:
			x = 0;
			while ( c >= '0' && c <= '9' && x < 1024 )
			{
				szTemp[x++] = c;
				c = szInput[index++];
			}

			if ( x <= 0 || x >= 1024 )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 6\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			szTemp[x] = '\0';
			pParams->myPdfTrailer.Prev = atoi(szTemp);
			break;

		case S_Info:
		case S_Root:
			bInfoState = (myState == S_Info);

			bStoreRootOrInfo = bInfoState ? (pParams->myPdfTrailer.Info.Number <= 0) : (pParams->myPdfTrailer.Root.Number <= 0);

			x = 0;
			while ( c >= '0' && c <= '9' && x < 1024 )
			{
				szTemp[x++] = c;
				c = szInput[index++];
			}

			if ( x <= 0 || x >= 1024 )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 7\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			szTemp[x] = '\0';

			if ( bStoreRootOrInfo )
			{
				if ( bInfoState )
					pParams->myPdfTrailer.Info.Number = atoi(szTemp);
				else
					pParams->myPdfTrailer.Root.Number = atoi(szTemp);
			}

			if ( (c != ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 8: atteso spazio trovato '%c'\n", c);
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			c = szInput[index++];
			while ( (c == '\n' || c == '\r' || c == '\t' || c == ' ' || c == '\f' || c == '\b') )
			{
				c = szInput[index++];
			}

			if ( c == '\0' )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 9\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			x = 0;
			while ( c >= '0' && c <= '9' && x < 1024 )
			{
				szTemp[x++] = c;
				c = szInput[index++];
			}

			if ( x <= 0 || x >= 1024 )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 10\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			szTemp[x] = '\0';

			if ( bStoreRootOrInfo )
			{
				if ( bInfoState )
					pParams->myPdfTrailer.Info.Generation = atoi(szTemp);
				else
					pParams->myPdfTrailer.Root.Generation = atoi(szTemp);
			}

			if ( (c != ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 11: atteso spazio trovato '%c'\n", c);
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			c = szInput[index++];
			while ( (c == '\n' || c == '\r' || c == '\t' || c == ' ' || c == '\f' || c == '\b') )
			{
				c = szInput[index++];
			}

			if ( c != 'R' )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 12\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			c = szInput[index++];
			if ( (c != '/' && c != ' ' && c != '>' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 13: atteso '/' o spazio, trovato '%c'\n", c);
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			if ( !bInfoState )
				bRootYes = 1;

			break;

		case S_Encrypt:
			x = 0;
			while ( c >= '0' && c <= '9' && x < 1024 )
			{
				szTemp[x++] = c;
				c = szInput[index++];
			}

			if ( x <= 0 || x >= 1024 )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 7 Bis\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			szTemp[x] = '\0';
			if ( pParams->myPdfTrailer.Encrypt.Number <= 0 )
				pParams->myPdfTrailer.Encrypt.Number = atoi(szTemp);

			if ( (c != ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 8 Bis: atteso spazio trovato '%c'\n", c);
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			c = szInput[index++];
			while ( (c == '\n' || c == '\r' || c == '\t' || c == ' ' || c == '\f' || c == '\b') )
			{
				c = szInput[index++];
			}

			if ( c == '\0' )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 9 Bis\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			x = 0;
			while ( c >= '0' && c <= '9' && x < 1024 )
			{
				szTemp[x++] = c;
				c = szInput[index++];
			}

			if ( x <= 0 || x >= 1024 )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 10 Bis\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			szTemp[x] = '\0';
			if ( pParams->myPdfTrailer.Encrypt.Number <= 0 )
				pParams->myPdfTrailer.Encrypt.Generation = atoi(szTemp);

			if ( (c != ' ' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 11 Bis: atteso spazio trovato '%c'\n", c);
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			c = szInput[index++];
			while ( (c == '\n' || c == '\r' || c == '\t' || c == ' ' || c == '\f' || c == '\b') )
			{
				c = szInput[index++];
			}

			if ( c != 'R' )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 12 Bis\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			c = szInput[index++];
			if ( (c != '/' && c != ' ' && c != '>' && c != '\r' && c != '\n' && c != '\t' && c != '\f' && c != '\b') )
			{
				snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 13 Bis: atteso '/' o spazio, trovato '%c'\n", c);
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				return 0;
			}

			pParams->isEncrypted = 1;

			break;

        case S_ID:
        {
            unsigned char *id0 = NULL;
            unsigned char *id1 = NULL;
            size_t lenID0 = 0;
            size_t lenID1 = 0;
            int bID0IsHex = 0;
            int bID1IsHex = 0;

            // Salta whitespace iniziali.
            while (is_pdf_whitespace(c))
                c = szInput[index++];

            if (c == '[')
            {
                // /ID [ ... ]: consuma '[' e passa al primo carattere di ID0. 
                while (szInput[index] != '\0' && is_pdf_whitespace(szInput[index]))
                    index++;

                if (szInput[index] == '<')
                {
                    if (szInput[index + 1] == '<')
                        goto id_error;
                    index++;
                    bID0IsHex = 1;
                }
                else if (szInput[index] == '(')
                {
                    index++;
                    bID0IsHex = 0;
                }
                else
                {
                    goto id_error;
                }
            }
            else if (c == '<')
            {
                // /ID<...: c e' gia' il '<' di ID0. 
                if (szInput[index] == '<')
                    goto id_error;
                bID0IsHex = 1;
            }
            else if (c == '(')
            {
                // /ID(...: c e' gia' il '(' di ID0. 
                bID0IsHex = 0;
            }
            else
            {
                goto id_error;
            }

            // Legge ID0. 
            if (bID0IsHex)
            {
                if (!read_pdf_hex_string(szInput, &index, &id0, &lenID0))
                    goto id_error;
            }
            else
            {
                if (!read_pdf_literal_string(szInput, &index, &id0, &lenID0))
                    goto id_error;
            }

            // Whitespace tra ID[0] e ID[1]. 
            while (szInput[index] != '\0' &&
                   is_pdf_whitespace(szInput[index]))
                index++;

            if (szInput[index] == '<')
            {
                if (szInput[index + 1] == '<')
                    goto id_error;
                index++;
                bID1IsHex = 1;
            }
            else if (szInput[index] == '(')
            {
                index++;
                bID1IsHex = 0;
            }
            else
            {
                goto id_error;
            }

            // Legge ID1. 
            if (bID1IsHex)
            {
                if (!read_pdf_hex_string(szInput, &index, &id1, &lenID1))
                    goto id_error;
            }
            else
            {
                if (!read_pdf_literal_string(szInput, &index, &id1, &lenID1))
                    goto id_error;
            }

            // Whitespace e ']' di chiusura. 
            while (szInput[index] != '\0' &&
                   is_pdf_whitespace(szInput[index]))
                index++;

            if (szInput[index] != ']')
                goto id_error;

            index++;    // consuma ']' 

            // Conserva la prima coppia incontrata. 
            if (pParams->myPdfTrailer.pID0Bytes == NULL &&
                pParams->myPdfTrailer.pID1Bytes == NULL)
            {
                pParams->myPdfTrailer.pID0Bytes = id0;
                pParams->myPdfTrailer.lenID0    = lenID0;
                pParams->myPdfTrailer.pID1Bytes = id1;
                pParams->myPdfTrailer.lenID1    = lenID1;

                id0 = NULL;
                id1 = NULL;
            }

            free(id0);
            free(id1);

            #if defined(MYDEBUG_PRINT_ALL) || \
                defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN) || \
                defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
            {
                size_t i;

                wprintf(L"\npParams->myPdfTrailer.pID0Bytes = <");
                if (pParams->myPdfTrailer.pID0Bytes != NULL)
                {
                    for (i = 0; i < pParams->myPdfTrailer.lenID0; i++)
                        wprintf(L"%02X",
                                (unsigned int)pParams->myPdfTrailer.pID0Bytes[i]);
                }
                else
                {
                    wprintf(L"(NULL)");
                }
                wprintf(L"> lenID0 = %zu\n", pParams->myPdfTrailer.lenID0);

                wprintf(L"pParams->myPdfTrailer.pID1Bytes = <");
                if (pParams->myPdfTrailer.pID1Bytes != NULL)
                {
                    for (i = 0; i < pParams->myPdfTrailer.lenID1; i++)
                        wprintf(L"%02X",
                                (unsigned int)pParams->myPdfTrailer.pID1Bytes[i]);
                }
                else
                {
                    wprintf(L"(NULL)");
                }
                wprintf(L"> lenID1 = %zu\n", pParams->myPdfTrailer.lenID1);
            }
            #endif

            c = szInput[index++];
            break;

        id_error:
            free(id0);
            free(id1);

            // /ID e' indispensabile solo se (a) il file e' cifrato e
            // (b) non abbiamo ancora gli ID dal trailer principale.
            // Un /ID malformato in un trailer di revisione precedente
            // non deve far fallire il parsing.
            if (pParams->isEncrypted &&
                pParams->myPdfTrailer.pID0Bytes == NULL &&
                pParams->myPdfTrailer.pID1Bytes == NULL)
            {
                snprintf(pParams->szError, sizeof(pParams->szError), "Errore ReadTrailerBody: array /ID non valido o incompleto.\n");
                myShowErrorMessage(pParams, pParams->szError, 1);
                return 0;
            }

            // Se arriviamo qui, o non e' cifrato, o abbiamo gia' gli ID:
            //   salta l'array /ID malformato e prosegui. 
            while (szInput[index] != '\0' &&
                   szInput[index] != ']' &&
                   !(szInput[index] == '>' && szInput[index + 1] == '>'))
            {
                index++;
            }

            if (szInput[index] == ']')
            {
                index++;
                c = szInput[index++];
            }
            else if (szInput[index] == '>' && szInput[index + 1] == '>')
            {
                // c = '>' e index punta al secondo '>': il flusso
                //   esterno proseguira' in ciclo3. 
                index++;
                c = '>';
            }
            else
            {
                snprintf(pParams->szError, sizeof(pParams->szError), "Errore ReadTrailerBody: array /ID malformato e fine buffer inattesa.\n");
                myShowErrorMessage(pParams, pParams->szError, 1);
                return 0;
            }

            break;
        }

		default:
			while ( c != '/' )
			{
				c = szInput[index++];
				if ( c == '>' )
				{
					c = szInput[index++];
					goto ciclo4;
				}
			}
			goto ciclo2;
			break;
	}

	if ( c == '/' )
		goto ciclo2;
	else if ( c == '>' )
		goto ciclo3;
	else
		goto ciclo1;

ciclo3:

	c = szInput[index++];
	if ( c != '>' )
	{
		snprintf(pParams->szError, 8192, "Errore ReadTrailerBody 16: atteso '>', trovato '%c'\n", c);
		myShowErrorMessage(pParams, pParams->szError, 1);
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
		myShowErrorMessage(pParams, pParams->szError, 0);
		#endif
		return 0;
	}

ciclo4:

	if ( !bSizeYes )
	{
		snprintf(pParams->szError, 8192, "Errore ReadTrailerBody: manca la voce 'Size' nel trailer\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
		myShowErrorMessage(pParams, pParams->szError, 0);
		#endif
		return 0;
	}

	if ( 0 == pParams->myPdfTrailer.Root.Number && !bRootYes )
	{
		snprintf(pParams->szError, 8192, "Errore ReadTrailerBody: manca la voce 'Root' nel trailer\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailerBody_FN)
		myShowErrorMessage(pParams, pParams->szError, 0);
		#endif
		return 0;
	}

	return 1;
}

int GetLenNextInput(Params *pParams, int startxref, int *len)
{
	int retValue = 1;
	unsigned char *myBlock = NULL;

	unsigned char c;
	int blockLen = 0;
	int index1 = 0;

	int eofOK = 0;

	States state = S0;

	if ( fseek(pParams->fp, startxref, SEEK_SET) )
	{
		snprintf(pParams->szError, 8192, "Errore GetLenNextInput fseek\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		myShowErrorMessage(pParams, pParams->szError, 0);
		//wprintf(L"Errore GetLenNextInput fseek\n");
		//fwprintf(pParams->fpErrors, L"Errore GetLenNextInput fseek\n");
		retValue = 0;
		goto uscita;
	}

	myBlock = (unsigned char *)malloc(sizeof(unsigned char) * BLOCK_SIZE);
	if ( !myBlock )
	{
		snprintf(pParams->szError, 8192, "Errore GetLenNextInput: memoria insufficiente.\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		myShowErrorMessage(pParams, pParams->szError, 0);
		//wprintf(L"Errore GetLenNextInput: memoria insufficiente.\n");
		//fwprintf(pParams->fpErrors, L"Errore GetLenNextInput: memoria insufficiente.\n");
		retValue = 0;
		goto uscita;
	}

	*len = 0;
	while ( (blockLen = fread(myBlock, 1, BLOCK_SIZE, pParams->fp)) )
	{
		index1 = 0;
		while ( index1 < blockLen )
		{
			c = myBlock[index1++];
			(*len)++;

			switch ( state )
			{
				case S0:
					if ( c == '%' )
						state = S1;
					break;
				case S1:
					if ( c == '%' )
						state = S2;
					else
						state = S0;
					break;
				case S2:
					if ( c == 'E' )
						state = S3;
					else
						state = S0;
					break;
				case S3:
					if ( c == 'O' )
						state = S4;
					else
						state = S0;
					break;
				case S4:
					if ( c == 'F' )
						state = S5;
					else
						state = S0;
					break;
				case S5:
					if ( c == '\r' || c == '\n' )
						eofOK = 1;
					else
						state = S0;
					break;
				default:
					state = S0;
					break;
			}

			if ( eofOK )
				break;
		}
	}

	if ( !eofOK )
		retValue = 0;

uscita:

	if ( NULL != myBlock )
		free(myBlock);

	return retValue;
}

int ReadLastTrailer(Params *pParams, unsigned char *szInput)
{
	int index;
	unsigned char c;

	char *pszTemp;

	char szXRefObjNum[8];
	int x;

	index = 0;

	szXRefObjNum[0] = '\0';

	c = szInput[index++];
	if ( c != 'x' )
	{
		if ( c >= '1' && c <= '9' )
		{
			x = 0;
			szXRefObjNum[x] = c;
			for ( x = 1; x < 7; x++ )
			{
				c = szInput[index++];
				if ( ( c < '0' || c > '9' ) )
				{
					szXRefObjNum[x] = '\0';
					break;
				}

				szXRefObjNum[x] = c;
			}

			if ( x >= 7 )
			{
				snprintf(pParams->szError, 8192, "Errore ReadLastTrailer XRefStream: T_INT_LITERAL troppo lungo.\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadLastTrailer_FN)
				myShowErrorMessage(pParams, pParams->szError, 0);
				#endif
				//wprintf(L"Errore ReadLastTrailer XRefStream: T_INT_LITERAL troppo lungo.\n");
				//fwprintf(pParams->fpErrors, L"Errore ReadLastTrailer XRefStream: T_INT_LITERAL troppo lungo.\n");
				return 0;
			}

			pParams->nXRefStreamObjNum = atoi(szXRefObjNum);

			return 2;
		}
		else
		{
			snprintf(pParams->szError, 8192, "Errore ReadLastTrailer: atteso 'x' o T_INT_LITERAL. Trovato invece '%c'\n", c);
			myShowErrorMessage(pParams, pParams->szError, 1);
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadLastTrailer_FN)
			myShowErrorMessage(pParams, pParams->szError, 0);
			#endif
			//wprintf(L"Errore ReadLastTrailer: atteso 'x' trovato '%c'\n", c);
			//fwprintf(pParams->fpErrors, L"Errore ReadLastTrailer: atteso 'x' o T_INT_LITERAL. Trovato invece '%c'\n", c);

			pszTemp = (char*)szInput + index - 1;
			pszTemp[index + 144] = '\0';

			snprintf(pParams->szError, 8192, "szInput = <%s>\n", pszTemp);
			myShowErrorMessage(pParams, pParams->szError, 1);
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadLastTrailer_FN)
			myShowErrorMessage(pParams, pParams->szError, 0);
			#endif
			//wprintf(L"szInput = <%s>\n", pszTemp);
			//fwprintf(pParams->fpErrors, L"szInput = <%s>\n", pszTemp);

			return 0;
		}
	}

	c = szInput[index++];
	if ( c != 'r' )
		return 0;

	c = szInput[index++];
	if ( c != 'e' )
		return 0;

	c = szInput[index++];
	if ( c != 'f' )
		return 0;

	while ( c != '\0' && c != 't' )
	{
		c = szInput[index++];
	}
	if ( c == '\0' )
		return 0;

	c = szInput[index - 2];
	if ( (c != '\n' && c != '\r' && c != '\t' && c != ' ' && c != '\f' && c != '\b') )
		return 0;


	c = szInput[index++];
	if ( c != 'r' )
		return 0;

	c = szInput[index++];
	if ( c != 'a' )
		return 0;

	c = szInput[index++];
	if ( c != 'i' )
		return 0;

	c = szInput[index++];
	if ( c != 'l' )
		return 0;

	c = szInput[index++];
	if ( c != 'e' )
		return 0;

	c = szInput[index++];
	if ( c != 'r' )
		return 0;

	c = szInput[index];
	if ( (c != '\n' && c != '\r' && c != '\t' && c != ' ' && c != '\f' && c != '\b' && c != '<') )
		return 0;

	strncpy((char*)pParams->myBlock, (char*)&(szInput[index]), BLOCK_SIZE);
	pParams->blockCurPos = 0;
	pParams->blockLen = strnlen((char*)pParams->myBlock, 4096);


	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadLastTrailer_FN_BLOCCO)
	wprintf(L"\n\n************************************* BLOCCO INIZIO ***************************************\n");
	for (int k = 0; k < pParams->blockLen; k++ )
	{
		wprintf(L"%c", pParams->myBlock[k]);
	}
	wprintf(L"\n\n************************************* BLOCCO FINE *****************************************\n\n");
	#endif

	GetNextToken(pParams);

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadLastTrailer_FN)
	wprintf(L"\nReadLastTrailer: Fin qui tutto OK!\n\n");
	#endif

	//if ( !trailerbody(pParams) )
	if ( !ReadTrailerBody(pParams, szInput, index) )
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadLastTrailer_FN)
		wprintf(L"\nReadLastTrailer: ReadTrailerBody ERRORE!\n\n");
		#endif
		return 0;
	}

	return 1;
}

int ReadSubSectionBody(Params *pParams, unsigned char *szInput, int fromNum, int numObjs, int *index)
{
	uint32_t x;
	unsigned char c;
	unsigned char c2;
	char szTemp[1024];
	uint32_t numInit;
	uint32_t toNum;

	uint32_t Offset;
	uint32_t GenNum;

	numInit = fromNum;
	toNum = fromNum + numObjs - 1;

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
	wprintf(L"ReadSubSectionBody: fromNum %d toNum %d\n", fromNum, toNum);
	#endif

	if ( NULL == pParams->myObjsTable )
	{
		pParams->myObjsTable = (PdfObjsTableItem **)malloc(sizeof(PdfObjsTableItem*) * pParams->nObjsTableSizeFromPrescanFile);
		if ( !(pParams->myObjsTable) )
		{
			snprintf(pParams->szError, 8192, "ERROR ReadSubSectionBody: memoria insufficiente per la tabella degli oggetti.\n\n");
			myShowErrorMessage(pParams, pParams->szError, 1);
			myShowErrorMessage(pParams, pParams->szError, 0);
			//wprintf(L"ERROR ReadSubSectionBody: memoria insufficiente per la tabella degli oggetti.\n\n");
			//fwprintf(pParams->fpErrors,L"ERROR ReadSubSectionBody: memoria insufficiente per la tabella degli oggetti.\n\n");
			return 0;
		}
		for ( x = 0; x < pParams->nObjsTableSizeFromPrescanFile; x++ )
			pParams->myObjsTable[x] = NULL;
	}

	while ( numInit <= toNum )
	{
		for ( x = 0; x < 10; x++ )
		{
			c = szInput[(*index)++];
			if ( c < '0' || c > '9' )
				return 0;
			szTemp[x] = c;
		}
		szTemp[x] = '\0';

		Offset = atoi(szTemp);

		if ( szInput[(*index)++] != ' ' )
			return 0;

		for ( x = 0; x < 5; x++ )
		{
			c = szInput[(*index)++];
			if ( c < '0' || c > '9' )
				return 0;
			szTemp[x] = c;
		}
		szTemp[x] = '\0';

		GenNum = atoi(szTemp);

		if ( szInput[(*index)++] != ' ' )
			return 0;

		c = szInput[(*index)++];

		if ( c != 'f' && c != 'n' )
			return 0;

		if ( numInit >= 1 && numInit < pParams->nObjsTableSizeFromPrescanFile )
		{
			if ( NULL == pParams->myObjsTable[numInit] )
			{
				pParams->myObjsTable[numInit] = (PdfObjsTableItem *)malloc(sizeof(PdfObjsTableItem));
				if ( NULL == pParams->myObjsTable[numInit] )
				{
					snprintf(pParams->szError, 8192, "ERRORE ReadSubSectionBody 0 bis: malloc failed\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					myShowErrorMessage(pParams, pParams->szError, 0);
					//wprintf(L"ERRORE ReadSubSectionBody 0 bis: malloc failed\n");
					//fwprintf(pParams->fpErrors, L"ERROR ReadSubSectionBody 0 bis: malloc failed\n");
					return 0;
				}
			}

			if ( 'n' == c)
				pParams->myObjsTable[numInit]->Obj.Type = OBJ_TYPE_IN_USE;
			else
				pParams->myObjsTable[numInit]->Obj.Type = OBJ_TYPE_FREE;
			pParams->myObjsTable[numInit]->Obj.Number = numInit;
			pParams->myObjsTable[numInit]->Obj.Generation = GenNum;
			pParams->myObjsTable[numInit]->Obj.Offset = Offset;
			pParams->myObjsTable[numInit]->Obj.numObjParent = -1;
			pParams->myObjsTable[numInit]->Obj.pszDecodedStream = NULL;
			//pParams->myObjsTable[numInit]->Obj.StreamOffset = 0;
			//pParams->myObjsTable[numInit]->Obj.StreamLength = 0;
			pParams->myObjsTable[numInit]->Obj.pTreeNode = NULL;
			myobjreflist_Init(&(pParams->myObjsTable[numInit]->myXObjRefList));
			myobjreflist_Init(&(pParams->myObjsTable[numInit]->myFontsRefList));
			myobjreflist_Init(&(pParams->myObjsTable[numInit]->myGsRefList));
			myintqueuelist_Init(&(pParams->myObjsTable[numInit]->queueContentsObjRefs));
			//pParams->myObjsTable[numInit]->dFontSpaceWidth = -1.0;
			pParams->myObjsTable[numInit]->pGlyphsWidths = NULL;
			pParams->myObjsTable[numInit]->pszDirectFontResourceString = NULL;
			pParams->myObjsTable[numInit]->lenDirectFontResourceString = 0;
			pParams->myObjsTable[numInit]->pszDirectGsResourceString = NULL;
			pParams->myObjsTable[numInit]->lenDirectGsResourceString = 0;
		}
		else
		{
			if ( numInit != 0 )
			{
				snprintf(pParams->szError, 8192, "ERROR ReadSubSectionBody 0: INDEX %d OUT OF RANGE [0-%d]\n", numInit, pParams->nObjsTableSizeFromPrescanFile);
				myShowErrorMessage(pParams, pParams->szError, 1);
				//wprintf(L"ERRORE ReadSubSectionBody 0: INDEX %d OUT OF RANGE [0-%lu]\n", numInit, pParams->nObjsTableSizeFromPrescanFile);
				//fwprintf(pParams->fpErrors, L"ERROR ReadSubSectionBody 0: INDEX %d OUT OF RANGE [0-%lu]\n", numInit, pParams->nObjsTableSizeFromPrescanFile);
				return 0;
			}

			if ( NULL == pParams->myObjsTable[numInit] )
			{
				pParams->myObjsTable[numInit] = (PdfObjsTableItem *)malloc(sizeof(PdfObjsTableItem));
				if ( NULL == pParams->myObjsTable[numInit] )
				{
					snprintf(pParams->szError, 8192, "ERRORE ReadSubSectionBody 0 tris: malloc failed\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					myShowErrorMessage(pParams, pParams->szError, 0);
					//wprintf(L"ERRORE ReadSubSectionBody 0 tris: malloc failed\n");
					//fwprintf(pParams->fpErrors, L"ERROR ReadSubSectionBody 0 tris: malloc failed\n");
					return 0;
				}
			}

			pParams->myObjsTable[numInit]->Obj.Type = OBJ_TYPE_FREE;
			pParams->myObjsTable[numInit]->Obj.Number = 0;
			pParams->myObjsTable[numInit]->Obj.Generation = 0xFFFF;
			pParams->myObjsTable[numInit]->Obj.Offset = Offset;
			pParams->myObjsTable[numInit]->Obj.numObjParent = -1;
			pParams->myObjsTable[numInit]->Obj.pszDecodedStream = NULL;
			pParams->myObjsTable[numInit]->Obj.StreamOffset = 0;
			pParams->myObjsTable[numInit]->Obj.StreamLength = 0;
			pParams->myObjsTable[numInit]->Obj.pTreeNode = NULL;
			myobjreflist_Init(&(pParams->myObjsTable[numInit]->myXObjRefList));
			myobjreflist_Init(&(pParams->myObjsTable[numInit]->myFontsRefList));
			myobjreflist_Init(&(pParams->myObjsTable[numInit]->myGsRefList));
			//pParams->myObjsTable[numInit]->dFontSpaceWidth = -1.0;
			pParams->myObjsTable[numInit]->pGlyphsWidths = NULL;
			pParams->myObjsTable[numInit]->pszDirectFontResourceString = NULL;
			pParams->myObjsTable[numInit]->lenDirectFontResourceString = 0;
			pParams->myObjsTable[numInit]->pszDirectGsResourceString = NULL;
			pParams->myObjsTable[numInit]->lenDirectGsResourceString = 0;
		}

		c = szInput[(*index)++];
		switch( c )
		{
		case ' ':
			c2 = szInput[(*index)++];
			if ( c2 != '\r' && c2 != '\n' )
			{
				snprintf(pParams->szError, 8192, "ERRORE ReadSubSectionBody 1: c = '%c', c2 = '%c'\n", c, c2);
				myShowErrorMessage(pParams, pParams->szError, 1);
				//wprintf(L"ERRORE ReadSubSectionBody 1: c = '%c', c2 = '%c'\n", c, c2);
				//fwprintf(pParams->fpErrors, L"ERRORE ReadSubSectionBody 1: c = '%c', c2 = '%c'\n", c, c2);
				return 0;
			}
			break;
		case '\r':
			c2 = szInput[(*index)++];
			if ( c2 != '\n' )
			{
				snprintf(pParams->szError, 8192, "ERRORE ReadSubSectionBody 2: c = '%c', c2 = '%c'\n", c, c2);
				myShowErrorMessage(pParams, pParams->szError, 1);
				//wprintf(L"ERRORE ReadSubSectionBody 2: c = '%c', c2 = '%c'\n", c, c2);
				//fwprintf(pParams->fpErrors, L"ERRORE ReadSubSectionBody 2: c = '%c', c2 = '%c'\n", c, c2);
				return 0;
			}
			break;
		default:
			snprintf(pParams->szError, 8192, "Errore ReadSubSectionBody: atteso spazio trovato '%c'\n", c);
			myShowErrorMessage(pParams, pParams->szError, 1);
			//wprintf(L"Errore ReadSubSectionBody: atteso spazio trovato '%c'\n", c);
			//fwprintf(pParams->fpErrors, L"Errore ReadSubSectionBody: atteso spazio trovato '%c'\n", c);
			return 0;
			break;
		}

		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		wprintf(L"ReadSubSectionBody: <ObjNum = %d> <Offset = %d> <GenNum = %d>\n", numInit, Offset, GenNum);
		#endif

		numInit++;
	}

	return 1;
}

int ReadObjsTable(Params *pParams, unsigned char *szInput)
{
	int retValue = 1;
	int x;
	int index;
	unsigned char c;
	char szTemp[1024];

	int fromNum;
	int numObjs;

	// saltiamo "xref"
	index = 4;

	c = szInput[index];

	if ( (c != '\n' && c != '\r' && c != '\t' && c != ' ' && c != '\f' && c != '\b') )
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		wprintf(L"ReadObjsTable: ACCIDENTACCIO!!! DOPO 'xref' NON HO TROVATO UN CARATTERE SPAZIO SEPARATORE MA QUESTO: <%c>\n", c);
		#endif

		retValue = 0;
		goto uscita;
	}
	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
	else
	{
		wprintf(L"\nReadObjsTable: BENISSIMO!!! DOPO 'xref' HO TROVATO UN CARATTERE SPAZIO SEPARATORE:\n");
		switch ( c )
		{
			case '\n':
				wprintf(L"\\n");
				break;
			case '\r':
				wprintf(L"\\r");
				break;
			case '\t':
				wprintf(L"\\t");
				break;
			case ' ':
				wprintf(L"SPAZIO");
				break;
			case '\f':
				wprintf(L"\\f");
				break;
			case '\b':
				wprintf(L"\\b");
				break;
			default:
				wprintf(L"CARATTERE SEPARATORE SCONOSCIUTO!");
				break;
		}
		wprintf(L"\n");
	}
	#endif

	while ( 1 )
	{
		c = szInput[index++];

		while ( (c == '\n' || c == '\r' || c == '\t' || c == ' ' || c == '\f' || c == '\b') )
		{
			c = szInput[index++];
		}

		if ( c == 't' || c == '\0' )
		{
			break;
		}

		if ( c < '0' || c > '9' )
		{
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
			wprintf(L"ReadObjsTable: 1 ACCIDENTACCIO!!! DOPO 'xref' NON HO TROVATO IL CARATTERE CHE CERCAVO MA QUESTO: <%c>\n", c);
			#endif

			retValue = 0;
			goto uscita;
		}

		x = 0;
		szTemp[x++] = c;
		while ( c >= '0' && c <= '9' )
		{
			c = szInput[index++];
			szTemp[x++] = c;
		}
		szTemp[x] = '\0';
		x = 0;

		if ( c != ' ' )
		{
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
			wprintf(L"ReadObjsTable: 2 ACCIDENTACCIO!!! DOPO 'xref' NON HO TROVATO IL CARATTERE CHE CERCAVO MA QUESTO: <%c>\n", c);
			#endif

			retValue = 0;
			goto uscita;
		}

		fromNum = atoi(szTemp);

		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		wprintf(L"ReadObjsTable: fromNum = %d\n", fromNum);
		#endif

		c = szInput[index++];

		if ( c < '0' || c > '9' )
		{
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
			wprintf(L"ReadObjsTable: 3 ACCIDENTACCIO!!! DOPO 'xref' NON HO TROVATO IL CARATTERE CHE CERCAVO MA QUESTO: <%c>\n", c);
			#endif

			retValue = 0;
			goto uscita;
		}

		x = 0;
		szTemp[x++] = c;
		while ( c >= '0' && c <= '9' )
		{
			c = szInput[index++];
			szTemp[x++] = c;
		}
		szTemp[x] = '\0';
		x = 0;

		numObjs = atoi(szTemp);

		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		wprintf(L"ReadObjsTable: numObjs = %d\n", numObjs);
		#endif

		while ( ( c == '\t' || c == ' ' || c == '\f' || c == '\b') )
		{
			c = szInput[index++];
		}

		if ( c != '\r' && c != '\n' )
		{
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
			wprintf(L"ReadObjsTable: 4 ACCIDENTACCIO!!! DOPO 'xref' NON HO TROVATO IL CARATTERE CHE CERCAVO MA QUESTO: <%c>\n", c);
			#endif

			retValue = 0;
			goto uscita;
		}

		while ( c == '\r' || c == '\n' )
		{
			c = szInput[index++];
		}

		if ( c >= '0' && c <= '9' )
			index--;

		if ( !ReadSubSectionBody(pParams, szInput, fromNum, numObjs, &index) )
		{
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
			wprintf(L"ReadObjsTable: 5 ACCIDENTACCIO!!! DOPO 'xref' NON HO TROVATO IL CARATTERE CHE CERCAVO MA QUESTO: <%c>\n", c);
			#endif

			retValue = 0;
			goto uscita;
		}
	}

	if ( c != 't' )
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		wprintf(L"ReadObjsTable: 6 ACCIDENTACCIO!!! DOPO 'xref' NON HO TROVATO IL CARATTERE CHE CERCAVO MA QUESTO: <%c>\n", c);
		#endif

		retValue = 0;
		goto uscita;
	}

uscita:

	return retValue;
}

int ReadHeader(Params *pParams)
{
	int bytereads = 0;

	unsigned char c;

	int idx;
	int idxVersion;

	unsigned char szTemp[BLOCK_SIZE];

	pParams->szPdfVersionFromCatalog[0] = '\0';

	if ( fseek(pParams->fp, 0, SEEK_CUR) )
	{
		snprintf(pParams->szError, 8192, "Errore ReadHeader fseek\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		myShowErrorMessage(pParams, pParams->szError, 0);
		//wprintf(L"Errore ReadHeader fseek\n");
		//fwprintf(pParams->fpErrors, L"Errore ReadHeader fseek\n");
		return 0;
	}

	bytereads = fread(szTemp, 1, BLOCK_SIZE, pParams->fp);
	if ( bytereads <= 5 )
	{
		snprintf(pParams->szError, 8192, "Errore ReadHeader fred\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		myShowErrorMessage(pParams, pParams->szError, 0);
		//wprintf(L"Errore ReadHeader fread\n");
		//fwprintf(pParams->fpErrors, L"Errore ReadHeader fread\n");
		return 0;
	}

	idx = idxVersion = 0;
	while ( idx < bytereads )
	{
		c = szTemp[idx];
		if ( '\n' == c || '\r' == c || idx > 127 )
		{
			pParams->szPdfHeader[idx] = '\0';
			pParams->szPdfVersion[idxVersion] = '\0';
			break;
		}
		pParams->szPdfHeader[idx] = c;
		if ( idx > 4 )
			pParams->szPdfVersion[idxVersion++] = c;

		idx++;
	}

	if ( pParams->szPdfHeader[0] != '%' )
	{
		snprintf(pParams->szError, 8192, "Errore ReadHeader formato file non valido\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		//wprintf(L"ERRORE ReadHeader: formaro file non valido\n");
		//fwprintf(pParams->fpErrors, L"ERRORE ReadHeader: formato file non valido\n");
		return 0;
	}

	if ( pParams->szPdfHeader[1] != 'P' )
	{
		snprintf(pParams->szError, 8192, "Errore ReadHeader formato file non valido\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		//wprintf(L"ERRORE ReadHeader: formaro file non valido\n");
		//fwprintf(pParams->fpErrors, L"ERRORE ReadHeader: formaro file non valido\n");
		return 0;
	}

	if ( pParams->szPdfHeader[2] != 'D' )
	{
		snprintf(pParams->szError, 8192, "Errore ReadHeader formato file non valido\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		//wprintf(L"ERRORE ReadHeader: formaro file non valido\n");
		//fwprintf(pParams->fpErrors, L"ERRORE ReadHeader: formaro file non valido\n");
		return 0;
	}

	if ( pParams->szPdfHeader[3] != 'F' )
	{
		snprintf(pParams->szError, 8192, "Errore ReadHeader formato file non valido\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		//wprintf(L"ERRORE ReadHeader: formaro file non valido\n");
		//fwprintf(pParams->fpErrors, L"ERRORE ReadHeader: formaro file non valido\n");
		return 0;
	}

	if ( pParams->szPdfHeader[4] != '-' )
	{
		snprintf(pParams->szError, 8192, "Errore ReadHeader formato file non valido\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		//wprintf(L"ERRORE ReadHeader: formaro file non valido\n");
		//fwprintf(pParams->fpErrors, L"ERRORE ReadHeader: formaro file non valido\n");
		return 0;
	}

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadHeader_FN)
	wprintf(L"\nPDF HEADER  = %s\n", pParams->szPdfHeader);
	wprintf(L"PDF VERSION = %s\n", pParams->szPdfVersion);
	#endif

	return 1;
}

int ReadTrailer(Params *pParams)
{
	//int dimFile = 0;
	int byteOffsetXRef = 0;
	int bytereads = 0;
	unsigned char *szInput = NULL;
	int dimInput = 0;
	unsigned char c;

	int index;
	int index1;
	int index2;
	int x;
	int y;
	unsigned char szTemp[BLOCK_SIZE];
	unsigned char szOffsetXRef[128];

	int retReadLastTrailer;

	pParams->nXRefStreamObjNum = 0;
	pParams->offsetXRefObjStream = 0;

	if ( fseek(pParams->fp, 0, SEEK_END) )
	{
		return 0;
	}

	pParams->dimFile = ftell(pParams->fp);
	if ( pParams->dimFile <= 0 )
	{
		return 0;
	}

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
	wprintf(L"ReadTrailer: dimFile = %d\n", pParams->dimFile);
	#endif

	if ( pParams->dimFile >= BLOCK_SIZE )
	{
		if ( fseek(pParams->fp, -BLOCK_SIZE, SEEK_CUR) )
		{
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
			wprintf(L"Errore ReadTrailer: fseek 1\n");
			#endif

			snprintf(pParams->szError, 8192, "Errore ReadTrailer: fseek 1\n");
			myShowErrorMessage(pParams, pParams->szError, 1);
			//fwprintf(pParams->fpErrors, L"Errore ReadTrailer: fseek 1\n");
			return 0;
		}
	}
	else
	{
		if ( fseek(pParams->fp, 0, SEEK_SET) )
		{
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
			wprintf(L"Errore ReadTrailer: fseek 2\n");
			#endif

			snprintf(pParams->szError, 8192, "Errore ReadTrailer: fseek 2\n");
			myShowErrorMessage(pParams, pParams->szError, 1);
			//fwprintf(pParams->fpErrors, L"Errore ReadTrailer: fseek 2\n");
			return 0;
		}
	}

	bytereads = fread(szTemp, 1, BLOCK_SIZE, pParams->fp);
	if ( bytereads <= 5 )
	{
		snprintf(pParams->szError, 8192, "Errore ReadTrailer: fread 1\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		myShowErrorMessage(pParams, pParams->szError, 0);
		//wprintf(L"Errore ReadTrailer: fread 1\n");
		//fwprintf(pParams->fpErrors, L"Errore ReadTrailer: fread 1\n");
		return 0;
	}

	index = bytereads - 1;

	while ( index && (
					  szTemp[index] == '\n' ||
					  szTemp[index] == '\r' ||
					  szTemp[index] == '\t' ||
					  szTemp[index] == ' ' ||
					  szTemp[index] == '\f' ||
					  szTemp[index] == '\b'
					  )
		  )
	{
		index--;
	}

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
	wprintf(L"ReadTrailer: index = %d\n", index);
	#endif
	if ( index <= 5 )
		return 0;

	c = szTemp[index--];
	if ( c != 'F')
		return 0;

	c = szTemp[index--];
	if ( c != 'O')
		return 0;

	c = szTemp[index--];
	if ( c != 'E')
		return 0;

	c = szTemp[index--];
	if ( c != '%')
		return 0;

	c = szTemp[index--];
	if ( c != '%')
		return 0;

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
	wprintf(L"ReadTrailer: %%PDF OK!\n");
	#endif

	index1 = index2 = 0;

	while ( index )
	{
		if ( szTemp[index] >= '0' && szTemp[index] <= '9' )
		{
			index2 = index;
			index--;
			break;
		}
		index--;
	}

	if ( index < 0 )
		return 0;

	while ( (szTemp[index] >= '0' && szTemp[index] <= '9') && index >= 0 )
	{
		index--;
	}

	if ( index < 0 )
		return 0;

	index1 = index;

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
	wprintf(L"ReadTrailer: index1 = %d\nReadTrailer: index2 = %d\n", index1, index2);
	#endif

	y = 0;
	for(x = index1; x <= index2; x++)
	{
		szOffsetXRef[y++] = szTemp[x];
	}
	if ( y == 0 )
		return 0;
	szOffsetXRef[y] = '\0';
	byteOffsetXRef = atoi((char*)szOffsetXRef);

	dimInput = pParams->dimFile - byteOffsetXRef;

	if ( dimInput <= 0 )
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		wprintf(L"Errore ReadTrailer: manca l'offset per xref.\n");
		#endif

		snprintf(pParams->szError, 8192, "Errore ReadTrailer: manca l'offset per xref.\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		//fwprintf(pParams->fpErrors, L"Errore ReadTrailer: manca l'offset per xref.\n");
		return 0;
	}

	szInput = (unsigned char*)malloc(sizeof(unsigned char)*(dimInput) + 1);
	if ( !szInput )
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		wprintf(L"Errore ReadTrailer: memoria insufficiente.\n");
		#endif

		snprintf(pParams->szError, 8192, "Errore ReadTrailer: memoria insufficiente.\n");
		myShowErrorMessage(pParams, pParams->szError, 1);
		//fwprintf(pParams->fpErrors, L"Errore ReadTrailer: memoria insufficiente.\n");
		return 0;
	}

	fseek(pParams->fp, byteOffsetXRef, SEEK_SET);

	bytereads = fread(szInput, 1, dimInput, pParams->fp);
	*(szInput + bytereads) = '\0';

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
	wprintf(L"bytereads = %d\n\n", bytereads);
	#endif

	//wprintf(L"file: '%s'\n\n%s\n\n", pParams->szFileName, szInput);
	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
	//wprintf(L"byteOffsetXRef = %d\ndimInput = %d\nszInput: <%s>\n\n", byteOffsetXRef, dimInput, szInput);
	if ( NULL != pParams->fpOutput )
	{
		fwprintf(pParams->fpOutput, L"\n\nReadTrailer: bytes read for szInput -> %d\n", bytereads);
		fwprintf(pParams->fpOutput, L"ECCO IL BLOCCO ALL'OFFSET %d -> <", byteOffsetXRef);
	}
	else
	{
		wprintf(L"\n\nReadTrailer: bytes read for szInput -> %d\n", bytereads);
		wprintf(L"ECCO IL BLOCCO ALL'OFFSET %d -> <", byteOffsetXRef);
	}

	for ( int i = 0; i < bytereads; i++ )
	{
		if ( (szInput[i] >= 32 || szInput[i] == '\n' ) && ( szInput[i] < 128 ) && (szInput[i] != 127 && szInput[i] != 129 && szInput[i] != 141 && szInput[i] != 143 && szInput[i] != 144 && szInput[i] != 157) )
		{
			if ( NULL != pParams->fpOutput )
			{
				fwprintf(pParams->fpOutput, L"%c", szInput[i]);
			}
			else
			{
				wprintf(L"%c", szInput[i]);
			}
		}
		else
		{
			if ( NULL != pParams->fpOutput )
			{
				if ( szInput[i] < 32 )
				{
					fwprintf(pParams->fpOutput, L"\n@<%X>@\n", szInput[i]);
				}
				else
				{
					wprintf(L"\n@<%X>@\n", szInput[i]);
				}
			}
		}
	}
	if ( NULL != pParams->fpOutput )
	{
		fwprintf(pParams->fpOutput, L">\n\n");
	}
	else
	{
		wprintf(L">\n\n");
	}
	#endif

	pParams->myPdfTrailer.Size = 0;

	pParams->myPdfTrailer.Root.Number = 0;
	pParams->myPdfTrailer.Root.Generation = 0;

	pParams->myPdfTrailer.Encrypt.Number = 0;
	pParams->myPdfTrailer.Encrypt.Generation = 0;

	pParams->myPdfTrailer.Info.Number = 0;
	pParams->myPdfTrailer.Info.Generation = 0;

	retReadLastTrailer = ReadLastTrailer(pParams, szInput);
	if ( !retReadLastTrailer )
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		wprintf(L"ReadTrailer: ACCIDENTACCIO! ERRORE IN ReadLastTrailer!\n");
		#endif

		free(szInput);
		szInput = NULL;
		return 0;
	}
	else if ( 2 == retReadLastTrailer )
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		wprintf(L"ReadTrailer: EHI! ritorno retReadLastTrailer = 2.\n");
		#endif

		free(szInput);
		szInput = NULL;

		pParams->offsetXRefObjStream = byteOffsetXRef;

		//Il parametro "pParams->nXRefStreamObjNum" viene calcolato nella funzione "ReadLastTrailer".

		return retReadLastTrailer;
	}

	if ( !ReadObjsTable(pParams, szInput) )
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
		wprintf(L"ReadTrailer: ACCIDENTACCIO! ERRORE IN ReadObjsTable!\n");
		#endif

		free(szInput);
		szInput = NULL;
		return 0;
	}

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN)
	wprintf(L"ReadTrailer: ReadObjsTable OK!\n");
	#endif

	//wprintf(L"file: '%s'\n\n%s\n\n", pParams->szFileName, szInput);
	//wprintf(L"file: '%s'\n\n", pParams->szFileName);

	while ( pParams->myPdfTrailer.Prev > 0 )
	{
		free(szInput);
		if ( !GetLenNextInput(pParams, pParams->myPdfTrailer.Prev, &dimInput) )
		{
			snprintf(pParams->szError, 8192, "Errore GetLenNextInput\n");
			myShowErrorMessage(pParams, pParams->szError, 1);
			//wprintf(L"Errore GetLenNextInput\n");
			//fwprintf(pParams->fpErrors, L"Errore GetLenNextInput\n");
			return 0;
		}

		szInput = (unsigned char*)malloc(sizeof(unsigned char)*(dimInput) + 1);
		if ( !szInput )
		{
			snprintf(pParams->szError, 8192, "GetLenNextInput errore ReadTrailer: memoria insufficiente.\n");
			myShowErrorMessage(pParams, pParams->szError, 1);
			//wprintf(L"GetLenNextInput errore ReadTrailer: memoria insufficiente.\n");
			//fwprintf(pParams->fpErrors, L"GetLenNextInput errore ReadTrailer: memoria insufficiente.\n");
			return 0;
		}

		if ( fseek(pParams->fp, pParams->myPdfTrailer.Prev, SEEK_SET) )
		{
			snprintf(pParams->szError, 8192, "GetLenNextInput: Errore fseek 3\n");
			myShowErrorMessage(pParams, pParams->szError, 1);
			//wprintf(L"Errore fseek 3\n");
			//fwprintf(pParams->fpErrors, L"Errore fseek 3\n");
			free(szInput);
			return 0;
		}

		bytereads = fread(szInput, 1, dimInput, pParams->fp);
		*(szInput + bytereads) = '\0';

		//wprintf(L"\nszInput = <%s>\n", szInput);
		//wprintf(L"dimInput = %d\n", dimInput);


		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadEncrypt_R)
		wprintf(L"pParams->myPdfTrailer.Prev = %d Encrypt obj:\n", pParams->myPdfTrailer.Prev);
		wprintf(L"\tPrima della chiamata a ReadLastTrailer:\n");
		wprintf(L"\tpParams->myPdfTrailer.Encrypt.Number = %u\n", pParams->myPdfTrailer.Encrypt.Number);
		wprintf(L"\tpParams->myPdfTrailer.Encrypt.Generation = %u\n", pParams->myPdfTrailer.Encrypt.Generation);
		#endif

		if ( !ReadLastTrailer(pParams, szInput) )
		{
			free(szInput);
			return 0;
		}

		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadEncrypt_R)
		wprintf(L"pParams->myPdfTrailer.Prev = %d Encrypt obj:\n", pParams->myPdfTrailer.Prev);
		wprintf(L"\tPrima della chiamata a ReadObjsTable:\n");
		wprintf(L"\tpParams->myPdfTrailer.Encrypt.Number = %u\n", pParams->myPdfTrailer.Encrypt.Number);
		wprintf(L"\tpParams->myPdfTrailer.Encrypt.Generation = %u\n", pParams->myPdfTrailer.Encrypt.Generation);
		#endif

		if ( !ReadObjsTable(pParams, szInput) )
		{
			free(szInput);
			return 0;
		}

		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadEncrypt_R)
		wprintf(L"pParams->myPdfTrailer.Prev = %d Encrypt obj:\n", pParams->myPdfTrailer.Prev);
		wprintf(L"\tDopo le chiamate a ReadLastTrailer e ReadObjsTable:\n");
		wprintf(L"\tpParams->myPdfTrailer.Encrypt.Number = %u\n", pParams->myPdfTrailer.Encrypt.Number);
		wprintf(L"\tpParams->myPdfTrailer.Encrypt.Generation = %u\n", pParams->myPdfTrailer.Encrypt.Generation);
		#endif
	}

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ReadTrailer_FN) || defined(MYDEBUG_PRINT_ON_ReadEncrypt_R)
	wprintf(L"\nEncrypt obj:\n", pParams->myPdfTrailer.Prev);
	wprintf(L"\tpParams->myPdfTrailer.Encrypt.Number = %u\n", pParams->myPdfTrailer.Encrypt.Number);
	wprintf(L"\tpParams->myPdfTrailer.Encrypt.Generation = %u\n", pParams->myPdfTrailer.Encrypt.Generation);
	#endif

	free(szInput);
	fseek(pParams->fp, 0, SEEK_SET);

	return 1;
}

unsigned char GetHexChar(unsigned char c1, unsigned char c2)
{
	unsigned char c = '\0';

	//wprintf(L"c1 = '%c' <> c2 = '%c'\n", c1, c2);

	switch ( c1 )
	{
		case '0':
			if ( c2 == '0' )
				return '\x00';
			else if ( c2 == '1' )
				return '\x01';
			else if ( c2 == '2' )
				return '\x02';
			else if ( c2 == '3' )
				return '\x03';
			else if ( c2 == '4' )
				return '\x04';
			else if ( c2 == '5' )
				return '\x05';
			else if ( c2 == '6' )
				return '\x06';
			else if ( c2 == '7' )
				return '\x07';
			else if ( c2 == '8' )
				return '\x08';
			else if ( c2 == '9' )
				return '\x09';
			else if ( c2 == 'a' || c == 'A' )
				return '\x0A';
			else if ( c2 == 'b' || c == 'B' )
				return '\x0B';
			else if ( c2 == 'c' || c == 'C' )
				return '\x0C';
			else if ( c2 == 'd' || c == 'D' )
				return '\x0D';
			else if ( c2 == 'e' || c == 'E' )
				return '\x0E';
			else if ( c2 == 'f' || c == 'F' )
				return '\x0F';
			else
				return '\0';
			break;
		case '1':
			if ( c2 == '0' )
				return '\x10';
			else if ( c2 == '1' )
				return '\x11';
			else if ( c2 == '2' )
				return '\x12';
			else if ( c2 == '3' )
				return '\x13';
			else if ( c2 == '4' )
				return '\x14';
			else if ( c2 == '5' )
				return '\x15';
			else if ( c2 == '6' )
				return '\x16';
			else if ( c2 == '7' )
				return '\x17';
			else if ( c2 == '8' )
				return '\x18';
			else if ( c2 == '9' )
				return '\x19';
			else if ( c2 == 'a' || c == 'A' )
				return '\x1A';
			else if ( c2 == 'b' || c == 'B' )
				return '\x1B';
			else if ( c2 == 'c' || c == 'C' )
				return '\x1C';
			else if ( c2 == 'd' || c == 'D' )
				return '\x1D';
			else if ( c2 == 'e' || c == 'E' )
				return '\x1E';
			else if ( c2 == 'f' || c == 'F' )
				return '\x1F';
			else
				return '\0';
			break;
		case '2':
			if ( c2 == '0' )
				return '\x20';
			else if ( c2 == '1' )
				return '\x21';
			else if ( c2 == '2' )
				return '\x22';
			else if ( c2 == '3' )
				return '\x23';
			else if ( c2 == '4' )
				return '\x24';
			else if ( c2 == '5' )
				return '\x25';
			else if ( c2 == '6' )
				return '\x26';
			else if ( c2 == '7' )
				return '\x27';
			else if ( c2 == '8' )
				return '\x28';
			else if ( c2 == '9' )
				return '\x29';
			else if ( c2 == 'a' || c == 'A' )
				return '\x2A';
			else if ( c2 == 'b' || c == 'B' )
				return '\x2B';
			else if ( c2 == 'c' || c == 'C' )
				return '\x2C';
			else if ( c2 == 'd' || c == 'D' )
				return '\x2D';
			else if ( c2 == 'e' || c == 'E' )
				return '\x2E';
			else if ( c2 == 'f' || c == 'F' )
				return '\x2F';
			else
				return '\0';
			break;
		case '3':
			if ( c2 == '0' )
				return '\x30';
			else if ( c2 == '1' )
				return '\x31';
			else if ( c2 == '2' )
				return '\x32';
			else if ( c2 == '3' )
				return '\x33';
			else if ( c2 == '4' )
				return '\x34';
			else if ( c2 == '5' )
				return '\x35';
			else if ( c2 == '6' )
				return '\x36';
			else if ( c2 == '7' )
				return '\x37';
			else if ( c2 == '8' )
				return '\x38';
			else if ( c2 == '9' )
				return '\x39';
			else if ( c2 == 'a' || c == 'A' )
				return '\x3A';
			else if ( c2 == 'b' || c == 'B' )
				return '\x3B';
			else if ( c2 == 'c' || c == 'C' )
				return '\x3C';
			else if ( c2 == 'd' || c == 'D' )
				return '\x3D';
			else if ( c2 == 'e' || c == 'E' )
				return '\x3E';
			else if ( c2 == 'f' || c == 'F' )
				return '\x3F';
			else
				return '\0';
			break;
		case '4':
			if ( c2 == '0' )
				return '\x40';
			else if ( c2 == '1' )
				return '\x41';
			else if ( c2 == '2' )
				return '\x42';
			else if ( c2 == '3' )
				return '\x43';
			else if ( c2 == '4' )
				return '\x44';
			else if ( c2 == '5' )
				return '\x45';
			else if ( c2 == '6' )
				return '\x46';
			else if ( c2 == '7' )
				return '\x47';
			else if ( c2 == '8' )
				return '\x48';
			else if ( c2 == '9' )
				return '\x49';
			else if ( c2 == 'a' || c == 'A' )
				return '\x4A';
			else if ( c2 == 'b' || c == 'B' )
				return '\x4B';
			else if ( c2 == 'c' || c == 'C' )
				return '\x4C';
			else if ( c2 == 'd' || c == 'D' )
				return '\x4D';
			else if ( c2 == 'e' || c == 'E' )
				return '\x4E';
			else if ( c2 == 'f' || c == 'F' )
				return '\x4F';
			else
				return '\0';
			break;
		case '5':
			if ( c2 == '0' )
				return '\x50';
			else if ( c2 == '1' )
				return '\x51';
			else if ( c2 == '2' )
				return '\x52';
			else if ( c2 == '3' )
				return '\x53';
			else if ( c2 == '4' )
				return '\x54';
			else if ( c2 == '5' )
				return '\x55';
			else if ( c2 == '6' )
				return '\x56';
			else if ( c2 == '7' )
				return '\x57';
			else if ( c2 == '8' )
				return '\x58';
			else if ( c2 == '9' )
				return '\x59';
			else if ( c2 == 'a' || c == 'A' )
				return '\x5A';
			else if ( c2 == 'b' || c == 'B' )
				return '\x5B';
			else if ( c2 == 'c' || c == 'C' )
				return '\x5C';
			else if ( c2 == 'd' || c == 'D' )
				return '\x5D';
			else if ( c2 == 'e' || c == 'E' )
				return '\x5E';
			else if ( c2 == 'f' || c == 'F' )
				return '\x5F';
			else
				return '\0';
			break;
		case '6':
			if ( c2 == '0' )
				return '\x60';
			else if ( c2 == '1' )
				return '\x61';
			else if ( c2 == '2' )
				return '\x62';
			else if ( c2 == '3' )
				return '\x63';
			else if ( c2 == '4' )
				return '\x64';
			else if ( c2 == '5' )
				return '\x65';
			else if ( c2 == '6' )
				return '\x66';
			else if ( c2 == '7' )
				return '\x67';
			else if ( c2 == '8' )
				return '\x68';
			else if ( c2 == '9' )
				return '\x69';
			else if ( c2 == 'a' || c == 'A' )
				return '\x6A';
			else if ( c2 == 'b' || c == 'B' )
				return '\x6B';
			else if ( c2 == 'c' || c == 'C' )
				return '\x6C';
			else if ( c2 == 'd' || c == 'D' )
				return '\x6D';
			else if ( c2 == 'e' || c == 'E' )
				return '\x6E';
			else if ( c2 == 'f' || c == 'F' )
				return '\x6F';
			else
				return '\0';
			break;
		case '7':
			if ( c2 == '0' )
				return '\x70';
			else if ( c2 == '1' )
				return '\x71';
			else if ( c2 == '2' )
				return '\x72';
			else if ( c2 == '3' )
				return '\x73';
			else if ( c2 == '4' )
				return '\x74';
			else if ( c2 == '5' )
				return '\x75';
			else if ( c2 == '6' )
				return '\x76';
			else if ( c2 == '7' )
				return '\x77';
			else if ( c2 == '8' )
				return '\x78';
			else if ( c2 == '9' )
				return '\x79';
			else if ( c2 == 'a' || c == 'A' )
				return '\x7A';
			else if ( c2 == 'b' || c == 'B' )
				return '\x7B';
			else if ( c2 == 'c' || c == 'C' )
				return '\x7C';
			else if ( c2 == 'd' || c == 'D' )
				return '\x7D';
			else if ( c2 == 'e' || c == 'E' )
				return '\x7E';
			else if ( c2 == 'f' || c == 'F' )
				return '\x7F';
			else
				return '\0';
			break;
		case '8':
			if ( c2 == '0' )
				return '\x80';
			else if ( c2 == '1' )
				return '\x81';
			else if ( c2 == '2' )
				return '\x82';
			else if ( c2 == '3' )
				return '\x83';
			else if ( c2 == '4' )
				return '\x84';
			else if ( c2 == '5' )
				return '\x85';
			else if ( c2 == '6' )
				return '\x86';
			else if ( c2 == '7' )
				return '\x87';
			else if ( c2 == '8' )
				return '\x88';
			else if ( c2 == '9' )
				return '\x89';
			else if ( c2 == 'a' || c == 'A' )
				return '\x8A';
			else if ( c2 == 'b' || c == 'B' )
				return '\x8B';
			else if ( c2 == 'c' || c == 'C' )
				return '\x8C';
			else if ( c2 == 'd' || c == 'D' )
				return '\x8D';
			else if ( c2 == 'e' || c == 'E' )
				return '\x8E';
			else if ( c2 == 'f' || c == 'F' )
				return '\x8F';
			else
				return '\0';
			break;
		case '9':
			if ( c2 == '0' )
				return '\x90';
			else if ( c2 == '1' )
				return '\x91';
			else if ( c2 == '2' )
				return '\x92';
			else if ( c2 == '3' )
				return '\x93';
			else if ( c2 == '4' )
				return '\x94';
			else if ( c2 == '5' )
				return '\x95';
			else if ( c2 == '6' )
				return '\x96';
			else if ( c2 == '7' )
				return '\x97';
			else if ( c2 == '8' )
				return '\x98';
			else if ( c2 == '9' )
				return '\x99';
			else if ( c2 == 'a' || c == 'A' )
				return '\x9A';
			else if ( c2 == 'b' || c == 'B' )
				return '\x9B';
			else if ( c2 == 'c' || c == 'C' )
				return '\x9C';
			else if ( c2 == 'd' || c == 'D' )
				return '\x9D';
			else if ( c2 == 'e' || c == 'E' )
				return '\x9E';
			else if ( c2 == 'f' || c == 'F' )
				return '\x9F';
			else
				return '\0';
			break;
		case 'a':
		case 'A':
			if ( c2 == '0' )
				return '\xA0';
			else if ( c2 == '1' )
				return '\xA1';
			else if ( c2 == '2' )
				return '\xA2';
			else if ( c2 == '3' )
				return '\xA3';
			else if ( c2 == '4' )
				return '\xA4';
			else if ( c2 == '5' )
				return '\xA5';
			else if ( c2 == '6' )
				return '\xA6';
			else if ( c2 == '7' )
				return '\xA7';
			else if ( c2 == '8' )
				return '\xA8';
			else if ( c2 == '9' )
				return '\xA9';
			else if ( c2 == 'a' || c == 'A' )
				return '\xAA';
			else if ( c2 == 'b' || c == 'B' )
				return '\xAB';
			else if ( c2 == 'c' || c == 'C' )
				return '\xAC';
			else if ( c2 == 'd' || c == 'D' )
				return '\xAD';
			else if ( c2 == 'e' || c == 'E' )
				return '\xAE';
			else if ( c2 == 'f' || c == 'F' )
				return '\xAF';
			else
				return '\0';
			break;
		case 'b':
		case 'B':
			if ( c2 == '0' )
				return '\xB0';
			else if ( c2 == '1' )
				return '\xB1';
			else if ( c2 == '2' )
				return '\xB2';
			else if ( c2 == '3' )
				return '\xB3';
			else if ( c2 == '4' )
				return '\xB4';
			else if ( c2 == '5' )
				return '\xB5';
			else if ( c2 == '6' )
				return '\xB6';
			else if ( c2 == '7' )
				return '\xB7';
			else if ( c2 == '8' )
				return '\xB8';
			else if ( c2 == '9' )
				return '\xB9';
			else if ( c2 == 'a' || c == 'A' )
				return '\xBA';
			else if ( c2 == 'b' || c == 'B' )
				return '\xBB';
			else if ( c2 == 'c' || c == 'C' )
				return '\xBC';
			else if ( c2 == 'd' || c == 'D' )
				return '\xBD';
			else if ( c2 == 'e' || c == 'E' )
				return '\xBE';
			else if ( c2 == 'f' || c == 'F' )
				return '\xBF';
			else
				return '\0';
			break;
		case 'c':
		case 'C':
			if ( c2 == '0' )
				return '\xC0';
			else if ( c2 == '1' )
				return '\xC1';
			else if ( c2 == '2' )
				return '\xC2';
			else if ( c2 == '3' )
				return '\xC3';
			else if ( c2 == '4' )
				return '\xC4';
			else if ( c2 == '5' )
				return '\xC5';
			else if ( c2 == '6' )
				return '\xC6';
			else if ( c2 == '7' )
				return '\xC7';
			else if ( c2 == '8' )
				return '\xC8';
			else if ( c2 == '9' )
				return '\xC9';
			else if ( c2 == 'a' || c == 'A' )
				return '\xCA';
			else if ( c2 == 'b' || c == 'B' )
				return '\xCB';
			else if ( c2 == 'c' || c == 'C' )
				return '\xCC';
			else if ( c2 == 'd' || c == 'D' )
				return '\xCD';
			else if ( c2 == 'e' || c == 'E' )
				return '\xCE';
			else if ( c2 == 'f' || c == 'F' )
				return '\xCF';
			else
				return '\0';
			break;
		case 'd':
		case 'D':
			if ( c2 == '0' )
				return '\xD0';
			else if ( c2 == '1' )
				return '\xD1';
			else if ( c2 == '2' )
				return '\xD2';
			else if ( c2 == '3' )
				return '\xD3';
			else if ( c2 == '4' )
				return '\xD4';
			else if ( c2 == '5' )
				return '\xD5';
			else if ( c2 == '6' )
				return '\xD6';
			else if ( c2 == '7' )
				return '\xD7';
			else if ( c2 == '8' )
				return '\xD8';
			else if ( c2 == '9' )
				return '\xD9';
			else if ( c2 == 'a' || c == 'A' )
				return '\xDA';
			else if ( c2 == 'b' || c == 'B' )
				return '\xDB';
			else if ( c2 == 'c' || c == 'C' )
				return '\xDC';
			else if ( c2 == 'd' || c == 'D' )
				return '\xDD';
			else if ( c2 == 'e' || c == 'E' )
				return '\xDE';
			else if ( c2 == 'f' || c == 'F' )
				return '\xDF';
			else
				return '\0';
			break;
		case 'e':
		case 'E':
			if ( c2 == '0' )
				return '\xE0';
			else if ( c2 == '1' )
				return '\xE1';
			else if ( c2 == '2' )
				return '\xE2';
			else if ( c2 == '3' )
				return '\xE3';
			else if ( c2 == '4' )
				return '\xE4';
			else if ( c2 == '5' )
				return '\xE5';
			else if ( c2 == '6' )
				return '\xE6';
			else if ( c2 == '7' )
				return '\xE7';
			else if ( c2 == '8' )
				return '\xE8';
			else if ( c2 == '9' )
				return '\xE9';
			else if ( c2 == 'a' || c == 'A' )
				return '\xEA';
			else if ( c2 == 'b' || c == 'B' )
				return '\xEB';
			else if ( c2 == 'c' || c == 'C' )
				return '\xEC';
			else if ( c2 == 'd' || c == 'D' )
				return '\xED';
			else if ( c2 == 'e' || c == 'E' )
				return '\xEE';
			else if ( c2 == 'f' || c == 'F' )
				return '\xEF';
			else
				return '\0';
			break;
		case 'f':
		case 'F':
			if ( c2 == '0' )
				return '\xF0';
			else if ( c2 == '1' )
				return '\xF1';
			else if ( c2 == '2' )
				return '\xF2';
			else if ( c2 == '3' )
				return '\xF3';
			else if ( c2 == '4' )
				return '\xF4';
			else if ( c2 == '5' )
				return '\xF5';
			else if ( c2 == '6' )
				return '\xF6';
			else if ( c2 == '7' )
				return '\xF7';
			else if ( c2 == '8' )
				return '\xF8';
			else if ( c2 == '9' )
				return '\xF9';
			else if ( c2 == 'a' || c == 'A' )
				return '\xFA';
			else if ( c2 == 'b' || c == 'B' )
				return '\xFB';
			else if ( c2 == 'c' || c == 'C' )
				return '\xFC';
			else if ( c2 == 'd' || c == 'D' )
				return '\xFD';
			else if ( c2 == 'e' || c == 'E' )
				return '\xFE';
			else if ( c2 == 'f' || c == 'F' )
				return '\xFF';
			else
				return '\0';
			break;

		default:
			return '\0';
	}

	return c;
}

void IgnoreStreamChars(Params *pParams)
{
	if ( pParams->blockCurPos < 0 )
	{
		if ( pParams->dimFile >= BLOCK_SIZE )
		{
			if ( fseek(pParams->fp, -BLOCK_SIZE, SEEK_CUR) )
			{
				snprintf(pParams->szError, 8192, "Errore IgnoreStreamChars fseek 1\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				//wprintf(L"Errore IgnoreStreamChars fseek 1\n");
				//fwprintf(pParams->fpErrors, L"Errore IgnoreStreamChars fseek 1\n");
				return;
			}
		}
		else
		{
			if ( fseek(pParams->fp, 0, SEEK_SET) )
			{
				snprintf(pParams->szError, 8192, "Errore IgnoreStreamChars fseek 2\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				//wprintf(L"Errore IgnoreStreamChars fseek 2\n");
				//fwprintf(pParams->fpErrors, L"Errore IgnoreStreamChars fseek 2\n");
				return;
			}
		}

		pParams->blockCurPos = pParams->blockLen - 1;
	}

	if ( pParams->blockCurPos >= pParams->blockLen )
	{
		pParams->blockLen = fread(pParams->myBlock, 1, BLOCK_SIZE, pParams->fp);
		if ( pParams->blockLen == 0 )
		{
			pParams->myToken.Type = T_EOF;
			return;
		}
		pParams->blockCurPos = 0;
	}
}

int IsDelimiterChar(unsigned char c)
{
    // Mappatura statica pre-calcolata per tutti i 256 possibili byte ASCII.
    // Sostituisce lo switch riducendo la funzione a un unico accesso istantaneo in memoria O(1).
    static const uint8_t DelimiterTable[256] = {
        [' '] = 1, ['\r'] = 1, ['\n'] = 1, ['\f'] = 1, ['\t'] = 1, ['\0'] = 1,
        ['%'] = 2, ['('] = 2, [')'] = 2, ['<'] = 2, ['>'] = 2, 
        ['['] = 2, [']'] = 2, ['{'] = 2, ['}'] = 2, ['/'] = 2
    };

    return DelimiterTable[c];
}

/*
int IsDelimiterChar(unsigned char c)
{
	switch ( c )
	{
		case ' ':
		case '\r':
		case '\n':
		case '\f':
		case '\t':
		case '\0':
			return DELIM_SPACECHAR;
			break;
		case '%':
		case '(':
		case ')':
		case '<':
		case '>':
		case '[':
		case ']':
		case '{':
		case '}':
		case '/':
			return DELIM_SPECIALSYMBOL;
			break;
		default:
			return DELIM_FALSE;
			break;
	}

	return DELIM_FALSE;
}
*/

unsigned char ReadNextChar(Params *pParams)
{
	if ( pParams->blockCurPos < 0 )
	{
		if ( pParams->dimFile >= BLOCK_SIZE )
		{
			if ( fseek(pParams->fp, -BLOCK_SIZE, SEEK_CUR) )
			{
				snprintf(pParams->szError, 8192, "Errore ReadNextChar fseek 1\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				//wprintf(L"Errore ReadNextChar fseek 1\n");
				//fwprintf(pParams->fpErrors, L"Errore ReadNextChar fseek 1\n");
				return 0;
			}
		}
		else
		{
			if ( fseek(pParams->fp, 0, SEEK_SET) )
			{
				snprintf(pParams->szError, 8192, "Errore ReadNextChar fseek 2\n");
				myShowErrorMessage(pParams, pParams->szError, 1);
				//wprintf(L"Errore ReadNextChar fseek 2\n");
				//fwprintf(pParams->fpErrors, L"Errore ReadNextChar fseek 2\n");
				return 0;
			}
		}

		pParams->blockCurPos = pParams->blockLen + pParams->blockCurPos;

		pParams->currentFileOffset -= pParams->blockLen + pParams->blockCurPos;
	}

	if ( pParams->blockCurPos >= pParams->blockLen )
	{
		if ( pParams->bStreamState )
		{
			pParams->myToken.Type = T_EOF;
			//return '\0';
			return ' ';
		}

		pParams->blockLen = fread(pParams->myBlock, 1, BLOCK_SIZE, pParams->fp);
		if ( pParams->blockLen == 0 )
		{
			pParams->myToken.Type = T_EOF;
			return ' ';
			//return ' ';
		}
		pParams->blockCurPos = 0;
	}

	if ( pParams->bUpdateNumBytesReadFromCurrentStream )
		pParams->nNumBytesReadFromCurrentStream++;



	if ( pParams->bReadingStringsFromDecodedStream && pParams->bReadingStringState && pParams->nCurrentFontSubtype == FONT_SUBTYPE_Type0 )
	{
		pParams->currentFileOffset++;

		return pParams->myBlock[pParams->blockCurPos++];
	}

	pParams->currentFileOffset++;

	if ( pParams->bReadingStringState || '\0' != pParams->myBlock[pParams->blockCurPos] )
	{
		return pParams->myBlock[pParams->blockCurPos++];
	}
	else
	{
		pParams->blockCurPos++;
		return ' ';
	}
}

void SetStringToken(Params *pParams, int k)
{
	pParams->myToken.Type = T_STRING;
	pParams->myToken.vString = pParams->lexeme;

	#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
	wprintf(L"GetNextToken: T_STRING -> '%s'\n", pParams->myToken.vString);
	#endif

	pParams->lastTokenOffset = pParams->currentFileOffset - (k + 1);

	pParams->bCurrTokenIsOperator = 0;
}

void GetNextToken(Params *pParams)
{
	States state = S0;
	unsigned char c;
	unsigned char c1, c2;
	unsigned char cOctal;
	unsigned char cHexadecimal;
	int k = 0;
	int bOctalOverflow;
	int nDelimChar;
	unsigned long int w;

	int x;
	int y;
	int z;

	pParams->bCurrTokenIsOperator = 0;

	if ( pParams->blockCurPos > pParams->blockLen )
	{
		pParams->myToken.Type = T_EOF;
		return;
	}

	c = pParams->pReadNextChar(pParams);
	if ( T_EOF == pParams->myToken.Type && pParams->blockCurPos >= pParams->blockLen )
	{
		return;
	}

	while ( c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' ||  c == '\0' )
	{
		c = pParams->pReadNextChar(pParams);

		if ( T_EOF == pParams->myToken.Type && pParams->blockCurPos >= pParams->blockLen )
		{
			return;
		}
	}

	while ( 1 )
	{
		switch ( state )
		{
			inizioswitch:

			case S0:
				k = 0;
				pParams->nStackStringOpenParen = 0;
				pParams->bReadingStringState = 0;

				pParams->lexemeTemp[0] = '\0';

				pParams->bCurrTokenIsOperator = 0;

				if ( c == '%' )
				{
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"\nCOMMENTO : *****");
					#endif

					c = pParams->pReadNextChar(pParams);

					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"%c", c);
					#endif

					while ( c != '\n' && c != '\r' )
					{
						c = pParams->pReadNextChar(pParams);
						if ( pParams->myToken.Type == T_EOF )
							return;

						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"%c", c);
						#endif
					}
					if ( c == '\r' )
					{
						c = pParams->pReadNextChar(pParams);
						if ( pParams->myToken.Type == T_EOF )
							return;

						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"%c", c);
						#endif

						if ( c != '\n' )
						{
							goto inizioswitch;
						}
					}

					//while ( c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' ||  c == '\0' )
					while ( c == ' ' || c == '\t' || c == '\f' ||  c == '\0' )
					{
						c = pParams->pReadNextChar(pParams);

						if ( pParams->myToken.Type == T_EOF )
							return;

						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"%c", c);
						#endif
					}

					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"*****\n");
					#endif
				}
				else if ( c == '-' || c == '+' || ( c >= '0' && c <= '9' ) )
				{
					pParams->lexeme[k++] = c;
					state = S1;
				}
				else if ( c == '.' )
				{
					pParams->lexeme[k++] = c;
					state = S2;
				}
				else if ( c == '/' )
				{
					state = S3;
				}
				else if ( c == '(' )
				{
					pParams->nStackStringOpenParen++; // PUSH
					pParams->bReadingStringState = 1;
					state = S6;
				}
				else if ( c == '<' )
				{
					state = S11;
				}
				else if ( c == '[' )
				{
					pParams->lastTokenOffset = pParams->currentFileOffset - 1;

					pParams->myToken.Type = T_QOPAREN;
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"GetNextToken: T_QOPAREN -> [\n");
					#endif

					return;
				}
				else if ( c == ']' )
				{
					pParams->lastTokenOffset = pParams->currentFileOffset - 1;

					pParams->myToken.Type = T_QCPAREN;
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"GetNextToken: T_QCPAREN -> ]\n");
					#endif

					return;
				}
				else if ( c == '>' )
				{
					c = pParams->pReadNextChar(pParams);
					if ( c == '>' )
					{
						pParams->lastTokenOffset = pParams->currentFileOffset - 2;

						pParams->myToken.Type = T_DICT_END;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"GetNextToken: T_DICT_END -> >>\n");
						#endif
						return;
					}
					else
					{
						if ( !(pParams->bStreamState) )
						{
							pParams->myToken.Type = T_ERROR;
							snprintf(pParams->szError, 8192, "Errore GetNextToken: trovato '>' singolo\n");
							myShowErrorMessage(pParams, pParams->szError, 1);
							//wprintf(L"Errore GetNextToken: trovato '>' singolo\n");
							return;
						}
					}
				}
				else if ( '{' == c ) // IGNORIAMO, NON SERVE A UN C.
				{
					pParams->myToken.Type = T_LEFT_CURLY_BRACKET;
					return;
				}
				else if ( '}' == c ) // IGNORIAMO, NON SERVE A UN C.
				{
					pParams->myToken.Type = T_RIGHT_CURLY_BRACKET;
					return;
				}
				else // T_STRING oppure T_KEYWORD
				{
					nDelimChar = IsDelimiterChar(c);

					if ( nDelimChar )
					{
						if ( !(pParams->bStreamState) )
						{
							// CORREZIONE: Avanza al carattere successivo prima di ritornare allo switch,
							// altrimenti lo scanner leggerà sempre lo stesso delimitatore all'infinito.
							c = pParams->pReadNextChar(pParams);

							goto inizioswitch;
						}
						else
						{
							nDelimChar = DELIM_FALSE;
						}
					}

					while ( !nDelimChar )
					{
						if ( k < MAX_STRING_LENTGTH_IN_CONTENT_STREAM )
						{
							pParams->lexeme[k++] = c;
						}
						else
						{
							pParams->lexeme[k] = '\0';
						}
						c = pParams->pReadNextChar(pParams);

						if ( 6 == k && !(pParams->bStringIsDecoded) )
						{
							if ( 's' == pParams->lexeme[0] )
							{
								if ( 't' == pParams->lexeme[1] )
								{
									if ( 'r' == pParams->lexeme[2] )
									{
										if ( 'e' == pParams->lexeme[3] )
										{
											if ( 'a' == pParams->lexeme[4] )
											{
												if ( 'm' == pParams->lexeme[5] )
												{
													if ( '\r' == c  )
													{
														c = pParams->pReadNextChar(pParams);
														if ( c != '\n' )
														{
															pParams->myToken.Type = T_ERROR;
															snprintf(pParams->szError, 8192, "Errore GetNextToken: la keyword 'stream' dev'essere seguita da '\\r\\n' oppure da '\\n'. Trovato invece '%c'(hex = <%2X>)\n", c, c);
															myShowErrorMessage(pParams, pParams->szError, 1);
															//wprintf(L"Errore GetNextToken: la keyword 'stream' dev'essere seguita da '\\r\\n' oppure da '\\n'. Trovato invece '%c'(hex = <%2X>)\n", c, c);
															//fwprintf(pParams->fpErrors, L"Errore GetNextToken: la keyword 'stream' dev'essere seguita da '\\r\\n' oppure da '\\n'. Trovato invece '%c'(hex = <%2X>)\n", c, c);
															return;
														}

														pParams->blockCurPos--;
														//pParams->nNumBytesReadFromCurrentStream--;   // NON DECOMMENTARE; NON ELIMINARE: A FUTURA MEMORIA.
														pParams->currentFileOffset--;

														pParams->lexeme[6] = '\0';

														pParams->bUpdateNumBytesReadFromCurrentStream = 1;
														pParams->bStreamState = 1;

														pParams->myToken.Type = T_KW_STREAM;
														#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
														wprintf(L"GetNextToken: T_KW_STREAM -> 'stream'\n");
														#endif

														pParams->lastTokenOffset = pParams->currentFileOffset - (k + 1);

														return;
													}
													else if ( '\n' == c )
													{
														pParams->blockCurPos--;
														//pParams->nNumBytesReadFromCurrentStream--;   // NON DECOMMENTARE; NON ELIMINARE: A FUTURA MEMORIA.
														pParams->currentFileOffset--;

														pParams->lexeme[6] = '\0';

														pParams->bUpdateNumBytesReadFromCurrentStream = 1;
														pParams->bStreamState = 1;

														pParams->myToken.Type = T_KW_STREAM;
														#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
														wprintf(L"GetNextToken: T_KW_STREAM -> 'stream'\n");
														#endif

														pParams->lastTokenOffset = pParams->currentFileOffset - (k + 1);

														return;
													}
												}
											}
										}
									}
								}
							}
						}

						nDelimChar = IsDelimiterChar(c);
					}

					pParams->lexeme[k] = '\0';
					if ( DELIM_SPECIALSYMBOL == nDelimChar )
					{
						pParams->blockCurPos--;
						pParams->nNumBytesReadFromCurrentStream--;
						pParams->currentFileOffset--;
					}

					pParams->lastTokenOffset = pParams->currentFileOffset - (k + 1);

					if ( strncmp(pParams->lexeme, "stream", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						if ( c == '\r' )
						{
							c = pParams->pReadNextChar(pParams);
							if ( c != '\n' )
							{
								pParams->myToken.Type = T_ERROR;
								snprintf(pParams->szError, 8192, "Errore GetNextToken: la keyword 'stream' dev'essere seguita da '\\r\\n' oppure da '\\n'. Trovato invece '\\r'\n");
								myShowErrorMessage(pParams, pParams->szError, 1);
								//wprintf(L"Errore GetNextToken: la keyword 'stream' dev'essere seguita da '\\r\\n' oppure da '\\n'. Trovato invece '\\r'\n");
								//fwprintf(pParams->fpErrors, L"Errore GetNextToken: la keyword 'stream' dev'essere seguita da '\\r\\n' oppure da '\\n'. Trovato invece '\\r'\n");
								return;
							}
						}
						else if ( c != '\n' )
						{
							pParams->myToken.Type = T_ERROR;
							snprintf(pParams->szError, 8192, "Errore GetNextToken: la keyword 'stream' dev'essere seguita da '\\r\\n' oppure da '\\n'. Trovato invece '%c'\n", c);
							myShowErrorMessage(pParams, pParams->szError, 1);
							//wprintf(L"Errore GetNextToken: la keyword 'stream' dev'essere seguita da '\\r\\n' oppure da '\\n'. Trovato invece '%c'\n", c);
							//fwprintf(pParams->fpErrors, L"Errore GetNextToken: la keyword 'stream' dev'essere seguita da '\\r\\n' oppure da '\\n'. Trovato invece '%c'\n", c);
							return;
						}

						pParams->bUpdateNumBytesReadFromCurrentStream = 0;
						for ( w = 0; w < pParams->CurrentContent.LengthFromPdf; w++ )
						{
							 pParams->pReadNextChar(pParams);
						}

						pParams->bUpdateNumBytesReadFromCurrentStream = 1;

						pParams->myToken.Type = T_KW_STREAM;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"GetNextToken: T_KW_STREAM -> 'stream'\n");
						#endif
						pParams->bStreamState = 1;
						return;
					}
					else if ( strncmp(pParams->lexeme, "endstream", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_KW_ENDSTREAM;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"GetNextToken: T_KW_ENDSTREAM -> 'endstream'\n");
						#endif
						pParams->bStreamState = 0;

						return;
					}
					else if ( strncmp(pParams->lexeme, "R", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_KW_R;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"GetNextToken: T_KW_R -> 'R'\n");
						#endif

						return;
					}
					else if ( strncmp(pParams->lexeme, "true", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_KW_TRUE;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"GetNextToken: T_KW_TRUE -> 'true'\n");
						#endif

						return;
					}
					else if ( strncmp(pParams->lexeme, "false", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_KW_FALSE;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"GetNextToken: T_KW_FALSE -> 'false'\n");
						#endif

						return;
					}
					else if ( strncmp(pParams->lexeme, "null", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_KW_NULL;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"GetNextTOken: T_KW_NULL -> 'null'\n");
						#endif

						return;
					}
					else if ( strncmp(pParams->lexeme, "obj", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_KW_OBJ;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"GetNextToken: T_KW_OBJ -> 'obj'\n");
						#endif

						return;
					}
					else if ( strncmp(pParams->lexeme, "endobj", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_KW_ENDOBJ;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
						wprintf(L"GetNextTOken: T_KW_ENDOBJ -> 'endobj'\n");
						#endif

						return;
					}
					else if ( (k <= 3) && (pParams->bStringIsDecoded && pParams->bStreamState) )
					{
						switch ( k )
						{
							case 3:
								switch ( pParams->lexeme[0] )
								{
									case 'S':
										if ( 'C' == pParams->lexeme[1] && 'N' == pParams->lexeme[2] )
										{
											pParams->myToken.Type = T_CONTENT_OP_SCN;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 's':
										if ( 'c' == pParams->lexeme[1] && 'n' == pParams->lexeme[2] )
										{
											pParams->myToken.Type = T_CONTENT_OP_scn;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'B':
										if ( 'M' == pParams->lexeme[1] && 'C' == pParams->lexeme[2] )
										{
											pParams->myToken.Type = T_CONTENT_OP_BMC;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'D' == pParams->lexeme[1] && 'C' == pParams->lexeme[2] )
										{
											pParams->myToken.Type = T_CONTENT_OP_BDC;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'E':
										if ( 'M' == pParams->lexeme[1] && 'C' == pParams->lexeme[2] )
										{
											pParams->myToken.Type = T_CONTENT_OP_EMC;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									default:
										SetStringToken(pParams, k);
										return;
										break;
								}
								break;
							case 2:
								switch ( pParams->lexeme[0] )
								{
									case 'B':
										if ( 'T' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_BT;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( '*' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_BSTAR;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'I' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_BI;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'X' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_BX;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'b':
										if ( '*' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_bSTAR;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'C':
										if ( 'S' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_CS;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'c':
										if ( 'm' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_cm;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 's' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_cs;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'D':
										if ( 'o' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_Do;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'P' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_DP;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'd':
										if ( '0' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_d0;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( '1' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_d1;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'E':
										if ( 'I' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_EI;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'T' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_ET;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'X' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_EX;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'f':
										if ( '*' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_fSTAR;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'g':
										if ( 's' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_gs;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'I':
										if ( 'D' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_ID;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'M':
										if ( 'D' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_MP;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'R':
										if ( 'D' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_RG;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'r':
										if ( 'e' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_re;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'g' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_rg;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'i' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_ri;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 'S':
										if ( 'C' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_SC;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									case 's':
										if ( 'c' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_sc;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'h' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_sh;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									// INIZIO T
									case 'T':
										if ( 'f' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_Tf;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'c' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_Tc;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'w' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_Tw;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'z' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_Tz;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'L' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_TL;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'r' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_Tr;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 's' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_Ts;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'D' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_TD;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'd' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_Td;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'm' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_Tm;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( '*' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_TSTAR;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'J' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_TJ;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else if ( 'j' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_Tj;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;
									// FINE T
									case 'W':
										if ( '*' == pParams->lexeme[1] )
										{
											pParams->myToken.Type = T_CONTENT_OP_WSTAR;
											pParams->bCurrTokenIsOperator = 1;
											return;
										}
										else
										{
											SetStringToken(pParams, k);
											return;
										}
										break;

									default:
										SetStringToken(pParams, k);
										return;
										break;
								}
								break;
							case 1:
								switch ( pParams->lexeme[0] )
								{
									case 'B':
										pParams->myToken.Type = T_CONTENT_OP_B;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'b':
										pParams->myToken.Type = T_CONTENT_OP_b;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'c':
										pParams->myToken.Type = T_CONTENT_OP_c;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'd':
										pParams->myToken.Type = T_CONTENT_OP_d;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'F':
										pParams->myToken.Type = T_CONTENT_OP_F;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'f':
										pParams->myToken.Type = T_CONTENT_OP_f;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'G':
										pParams->myToken.Type = T_CONTENT_OP_G;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'g':
										pParams->myToken.Type = T_CONTENT_OP_g;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'h':
										pParams->myToken.Type = T_CONTENT_OP_h;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'i':
										pParams->myToken.Type = T_CONTENT_OP_i;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'J':
										pParams->myToken.Type = T_CONTENT_OP_J;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'j':
										pParams->myToken.Type = T_CONTENT_OP_j;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'K':
										pParams->myToken.Type = T_CONTENT_OP_K;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'k':
										pParams->myToken.Type = T_CONTENT_OP_k;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'l':
										pParams->myToken.Type = T_CONTENT_OP_l;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'M':
										pParams->myToken.Type = T_CONTENT_OP_M;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'm':
										pParams->myToken.Type = T_CONTENT_OP_m;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'n':
										pParams->myToken.Type = T_CONTENT_OP_n;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'Q':
										pParams->myToken.Type = T_CONTENT_OP_Q;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'q':
										pParams->myToken.Type = T_CONTENT_OP_q;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'S':
										pParams->myToken.Type = T_CONTENT_OP_S;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 's':
										pParams->myToken.Type = T_CONTENT_OP_s;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'v':
										pParams->myToken.Type = T_CONTENT_OP_v;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'W':
										pParams->myToken.Type = T_CONTENT_OP_W;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case 'w':
										pParams->myToken.Type = T_CONTENT_OP_w;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case '\'':
										pParams->myToken.Type = T_CONTENT_OP_SINGLEQUOTE;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									case '"':
										pParams->myToken.Type = T_CONTENT_OP_DOUBLEQUOTE;
										pParams->bCurrTokenIsOperator = 1;
										return;
										break;
									default:
										SetStringToken(pParams, k);
										return;
										break;
								}
								break;
							case 0:
								pParams->myToken.Type = T_VOID_STRING;
								#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
								wprintf(L"GetNextToken: T__VOID_STRING\n");
								#endif

								pParams->lastTokenOffset = pParams->currentFileOffset;

								c = pParams->pReadNextChar(pParams);
								if ( T_EOF == pParams->myToken.Type )
									return;
								while ( !IsDelimiterChar(c) )
								{
									c = pParams->pReadNextChar(pParams);
									if ( T_EOF == pParams->myToken.Type )
										return;
								}
								return;
								break;
							default:
								SetStringToken(pParams, k);
								return;
								break;
						}
					}
					else
					{
						if ( k > 0 )
						{
							SetStringToken(pParams, k);
						}
						else
						{
							pParams->myToken.Type = T_VOID_STRING;
							#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
							wprintf(L"GetNextToken: T__VOID_STRING\n");
							#endif

							pParams->lastTokenOffset = pParams->currentFileOffset;

							c = pParams->pReadNextChar(pParams);
							if ( T_EOF == pParams->myToken.Type )
								return;
							while ( !IsDelimiterChar(c) )
							{
								c = pParams->pReadNextChar(pParams);
								if ( T_EOF == pParams->myToken.Type )
									return;
							}
						}

						return;
					}
				}
				break;
			case S1:
				if (  c >= '0' && c <= '9' )
				{
					pParams->lexeme[k++] = c;
				}
				else if ( c == '.' )
				{
					pParams->lexeme[k++] = c;
					state = S2;
				}
				else
				{
					pParams->lexeme[k] = '\0';
					pParams->myToken.Type = T_INT_LITERAL;
					pParams->myToken.vInt = atoi(pParams->lexeme);
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"GetNextToken: T_INT_LITERAL -> %d\n", pParams->myToken.vInt);
					#endif
					nDelimChar = IsDelimiterChar(c);
					if ( DELIM_SPECIALSYMBOL == nDelimChar )
					{
						pParams->blockCurPos--;
						pParams->nNumBytesReadFromCurrentStream--;
						pParams->currentFileOffset--;
					}

					pParams->lastTokenOffset = pParams->currentFileOffset - (k + 1);

					return;
				}
				break;
			case S2:
				if (  c >= '0' && c <= '9' )
				{
					pParams->lexeme[k++] = c;
				}
				else
				{
					pParams->lexeme[k] = '\0';

					pParams->myToken.Type = T_REAL_LITERAL;
					pParams->myToken.vDouble = atof(pParams->lexeme);
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"GetNextTOken: T_REAL_LITERAL -> %lf\n", pParams->myToken.vDouble);
					#endif
					nDelimChar = IsDelimiterChar(c);
					if ( DELIM_SPECIALSYMBOL == nDelimChar )
					{
						pParams->blockCurPos--;
						pParams->nNumBytesReadFromCurrentStream--;
						pParams->currentFileOffset--;
					}

					pParams->lastTokenOffset = pParams->currentFileOffset - (k + 1);

					return;
				}
				break;
			case S3:
				nDelimChar = IsDelimiterChar(c);
				if ( !nDelimChar )
				{
					if ( c != '#' )
					{
						pParams->lexeme[k++] = c;
					}
					else
					{
						state = S4;
					}
				}
				else
				{
					pParams->lexeme[k] = '\0';
					pParams->myToken.Type = T_NAME;
					pParams->myToken.vString = pParams->lexeme;

					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"GetNextToken: T_NAME -> '/%s'\n", pParams->myToken.vString);
					#endif

					if ( DELIM_SPECIALSYMBOL == nDelimChar )
					{
						pParams->blockCurPos--;
						pParams->nNumBytesReadFromCurrentStream--;
						pParams->currentFileOffset--;
					}

					pParams->lastTokenOffset = pParams->currentFileOffset - (k + 1);

					return;
				}
				break;
			case S4:
				if ( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') )
				{
					c1 = c;
					state = S5;
				}
				else
				{
					pParams->myToken.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextToken: numero esadecimale errato.\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextToken: numero esadecimale errato.\n");
					//fwprintf(pParams->fpErrors, L"Errore GetNextToken: numero esadecimale errato.\n");
					return;
				}
				break;
			case S5:
				if ( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'f' && c <= 'f') )
				{
					c2 = c;
					pParams->lexeme[k++] = GetHexChar(c1, c2);
					state = S3;
				}
				else
				{
					pParams->myToken.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextToken: numero esadecimale errato.\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextToken: numero esadecimale errato.\n");
					//fwprintf(pParams->fpErrors, L"Errore GetNextToken: numero esadecimale errato.\n");
					return;
				}
				break;

			case S6:
				if ( c != '\\' )
				{
					// Conserva TUTTI i byte (inclusi 0x00) in lexeme per:
					//   - /Encrypt e /Info (stringhe binarie, che possono
					//     contenere 0x00 e devono essere decifrate integre)
					//   - Font Type0 (CID a 2 byte; la mappatura avviene
					//     dopo, ma lexeme deve contenere i byte grezzi
					//     completi perche' il calcolo dei CID funzioni)
					// Per i font non-Type0 (testo "classico"), scarta i
					// byte 0x00 come nella versione storica.
					int bKeepAllBytes = pParams->bOnParseEncryptObject || pParams->bOnParseInfoObject || (pParams->nCurrentFontSubtype == FONT_SUBTYPE_Type0);

					if ( bKeepAllBytes )
					{
						pParams->lexeme[k++] = c;
					}
					else if ( '\0' != c )
					{
						pParams->pEncodingString[k] = c;
						pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[c];
						pParams->lexeme[k++] = c;
					}

					// Da qui in poi lascia invariato il tuo codice.

					if ( c == ')' )
					{
						pParams->nStackStringOpenParen--; // POP

						if ( pParams->nStackStringOpenParen <= 0 )
						{
							pParams->lexeme[--k] = '\0';

							#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
							wprintf(L"\n\tpParams->lexeme(hexadecimal string) = <");
							for ( int r = 0; r < k; r++ )
							{
								if ( pParams->lexeme[r] < 16 )
									wprintf(L"0");
								wprintf(L"%X", pParams->lexeme[r]);
							}
							wprintf(L">\n");

							wprintf(L"\tpParams->lexeme =                     <");
							for ( int r = 0; r < k; r++ )
							{
								if ( '\0' == pParams->lexeme[r] )
									wprintf(L"\\0");

								if ( pParams->lexeme[r] == '\n' )
									wprintf(L"\\n");
								else if ( pParams->lexeme[r] == '\r' )
									wprintf(L"\\r");
								else if ( pParams->lexeme[r] == '\t' )
									wprintf(L"\\t");
								else if ( pParams->lexeme[r] == '\f' )
									wprintf(L"\\f");
								else
									wprintf(L"%c", pParams->lexeme[r]);
							}
							wprintf(L">\n");
							#endif

							if ( pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0 )
							{
								pParams->pEncodingString[k] = L'\0';
								pParams->pUtf8String[k] = L'\0';
							}
							else
							{
								uint32_t u = 0;
								int z = 0;
								int w = 0;
								unsigned char num1;
								unsigned char num2;

								if ( pParams->bHasCodeSpaceTwoByte && !pParams->bHasCodeSpaceOneByte ) // CODE SPACE RANGE TWO BYTES ONLY.
								{
									#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
									wprintf(L"\nEHI!!! SIAMO NEL CODE SPACE RANGE TWO BYTES ONLY.\n");
									#endif

									w = 0;
									for ( z = 0; z < k - 1; z++ )
									{
										num1 = pParams->lexeme[z];
										z++;
										if ( z < k )
											num2 = pParams->lexeme[z];
										else
											num2 = 0;

										if ( MACHINE_ENDIANNESS_LITTLE_ENDIAN == pParams->nThisMachineEndianness )
											pParams->myCID = MY_MAKEWORD(num2, num1);
										else
											pParams->myCID = MY_MAKEWORD(num1, num2);

										#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
										wprintf(L"\tpParams->myCID TWO BYTES = %u(hex: %X)\n", pParams->myCID, pParams->myCID);
										#endif

										for ( u = 0; u < pParams->nCurrentFontCodeSpacesNum; u++ )
										{
											if ( NULL != pParams->pCodeSpaceRangeArray )
											{
												if ( pParams->myCID >= pParams->pCodeSpaceRangeArray[u].nFrom && pParams->myCID <= pParams->pCodeSpaceRangeArray[u].nTo )
												{
													#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
													wprintf(L"\tECCOLO! -> CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE TWO BYTES, NEL RANGE %lu(hex: %X) - %lu(hex: %X).\n", pParams->myCID, pParams->myCID, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nTo, pParams->pCodeSpaceRangeArray[u].nTo);
													#endif

													break;
												}
											}
											else
											{
												#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
												wprintf(L"\tpParams->pCodeSpaceRangeArray È NULL!!!\n");
												#endif

												pParams->myCID = 0xFFFD;
												u = pParams->nCurrentFontCodeSpacesNum + 1;
												break;
											}
										}

										if ( u >= pParams->nCurrentFontCodeSpacesNum ) // CID NON TROVATO NEL CODE SPACE RANGE
										{
											pParams->myCID = 0xFFFD;
										}

										if ( 0xFFFD != pParams->myCID )
										{
											pParams->pEncodingString[w] = pParams->myCID;
											pParams->pUtf8String[w] = pParams->pCurrentEncodingArray[pParams->myCID];

											#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
											wprintf(L"\tOK, CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE TWO BYTES-> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
											#endif
										}
										else
										{
											pParams->pEncodingString[w] = 0xFFFD;
											pParams->pUtf8String[w] = 0xFFFD;

											#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
											wprintf(L"\tCACCHIO, CID %u(hex: %X) NON TROVATO NEL CODE SPACE RANGE TWO BYTES-> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
											#endif
										}

										w++;
									}
								}
								else if ( pParams->bHasCodeSpaceTwoByte && pParams->bHasCodeSpaceOneByte ) // CODE SPACE RANGE ONE AND TWO BYTES.
								{
									#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
									wprintf(L"\nEHI!!! SIAMO NEL CODE SPACE RANGE ONE AND TWO BYTES.\n");
									#endif

									w = 0;
									for ( z = 0; z < k; z++ )
									{
										num1 = pParams->lexeme[z];
										z++;

										pParams->myCID = num1;

										#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
										wprintf(L"\tpParams->myCID ONE BYTE = %u(hex: %X)\n", pParams->myCID, pParams->myCID);
										#endif

										for ( u = 0; u < pParams->nCurrentFontCodeSpacesNum; u++ )
										{
											if ( NULL != pParams->pCodeSpaceRangeArray )
											{
												if ( 1 == pParams->pCodeSpaceRangeArray[u].nNumBytes && (pParams->myCID >= pParams->pCodeSpaceRangeArray[u].nFrom && pParams->myCID <= pParams->pCodeSpaceRangeArray[u].nTo) )
												{
													#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
													wprintf(L"\tECCOLO! -> CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE ONE BYTES, NEL RANGE %lu(hex: %X) - %lu(hex: %X).\n", pParams->myCID, pParams->myCID, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nTo, pParams->pCodeSpaceRangeArray[u].nTo);
													#endif
													break;
												}
											}
											else
											{
												#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
												wprintf(L"\tpParams->pCodeSpaceRangeArray È NULL!!!\n");
												#endif

												pParams->myCID = 0xFFFD;
												u = pParams->nCurrentFontCodeSpacesNum + 1;
												break;
											}
										}
										if ( u >= pParams->nCurrentFontCodeSpacesNum ) // CID ONE BYTE NON TROVATO NEL CODE SPACE RANGE
										{
											#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
											wprintf(L"\tCID %u(hex: %X) NON TROVATO NEL CODE SPACE RANGE ONE BYTE. CONSUMO UN ALTRO BYTE.\n", pParams->myCID, pParams->myCID);
											#endif
										}
										else // CID ONE BYTE TROVATO NEL CODE SPACE RANGE
										{
											pParams->pEncodingString[w] = pParams->myCID;
											pParams->pUtf8String[w] = pParams->pCurrentEncodingArray[pParams->myCID];

											#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
											wprintf(L"\tOK, CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE ONE BYTE -> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
											#endif

											w++;
											continue;
										}

										if ( z < k )
											num2 = pParams->lexeme[z];
										else
											num2 = 0;

										if ( MACHINE_ENDIANNESS_LITTLE_ENDIAN == pParams->nThisMachineEndianness )
											pParams->myCID = MY_MAKEWORD(num2, num1);
										else
											pParams->myCID = MY_MAKEWORD(num1, num2);

										#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
										wprintf(L"\tpParams->myCID TWO BYTES = %u(hex: %X)\n", pParams->myCID, pParams->myCID);
										#endif

										for ( u = 0; u < pParams->nCurrentFontCodeSpacesNum; u++ )
										{
											if ( NULL != pParams->pCodeSpaceRangeArray )
											{
												if ( 2 == pParams->pCodeSpaceRangeArray[u].nNumBytes && (pParams->myCID >= pParams->pCodeSpaceRangeArray[u].nFrom && pParams->myCID <= pParams->pCodeSpaceRangeArray[u].nTo) )
												{
													#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
													wprintf(L"\tECCOLO! -> CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE TWO BYTES, NEL RANGE %lu(hex: %X) - %lu(hex: %X).\n", pParams->myCID, pParams->myCID, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nTo, pParams->pCodeSpaceRangeArray[u].nTo);
													#endif

													break;
												}
											}
											else
											{
												#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
												wprintf(L"\tpParams->pCodeSpaceRangeArray È NULL!!!\n");
												#endif

												pParams->myCID = 0xFFFD;
												u = pParams->nCurrentFontCodeSpacesNum + 1;
												break;
											}
										}

										if ( u >= pParams->nCurrentFontCodeSpacesNum ) // CID NON TROVATO NEL CODE SPACE RANGE
										{
											pParams->myCID = 0xFFFD;
										}

										if ( 0xFFFD != pParams->myCID )
										{
											pParams->pEncodingString[w] = pParams->myCID;
											pParams->pUtf8String[w] = pParams->pCurrentEncodingArray[pParams->myCID];

											#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
											wprintf(L"\tOK, CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE TWO BYTES-> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
											#endif
										}
										else
										{
											pParams->pEncodingString[w] = 0xFFFD;
											pParams->pUtf8String[w] = 0xFFFD;

											#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
											wprintf(L"\tCACCHIO, CID %u(hex: %X) NON TROVATO NEL CODE SPACE RANGE TWO BYTES-> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
											#endif
										}

										w++;
									}
								}
								else if ( !pParams->bHasCodeSpaceTwoByte && pParams->bHasCodeSpaceOneByte ) // CODE SPACE RANGE ONE BYTE ONLY.
								{
									#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
									wprintf(L"\nEHI!!! SIAMO NEL CODE SPACE RANGE ONE BYTE ONLY.\n");
									#endif

									w = 0;
									for ( z = 0; z < k; z++ )
									{
										pParams->myCID = pParams->lexeme[z];
										z++;

										#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
										wprintf(L"\tpParams->myCID ONE BYTE = %u(hex: %X)\n", pParams->myCID, pParams->myCID);
										#endif

										for ( u = 0; u < pParams->nCurrentFontCodeSpacesNum; u++ )
										{
											if ( NULL != pParams->pCodeSpaceRangeArray )
											{
												if ( pParams->myCID >= pParams->pCodeSpaceRangeArray[u].nFrom && pParams->myCID <= pParams->pCodeSpaceRangeArray[u].nTo )
												{
													#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
													wprintf(L"\tECCOLO! -> CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE ONE BYTE, NEL RANGE %lu(hex: %X) - %lu(hex: %X).\n", pParams->myCID, pParams->myCID, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nTo, pParams->pCodeSpaceRangeArray[u].nTo);
													#endif

													break;
												}
											}
											else
											{
												#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
												wprintf(L"\tpParams->pCodeSpaceRangeArray È NULL!!!\n");
												#endif

												pParams->myCID = 0xFFFD;
												u = pParams->nCurrentFontCodeSpacesNum + 1;
												break;
											}
										}

										if ( u >= pParams->nCurrentFontCodeSpacesNum ) // CID NON TROVATO NEL CODE SPACE RANGE
										{
											pParams->myCID = 0xFFFD;
										}

										if ( 0xFFFD != pParams->myCID )
										{
											pParams->pEncodingString[w] = pParams->myCID;
											pParams->pUtf8String[w] = pParams->pCurrentEncodingArray[pParams->myCID];

											#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
											wprintf(L"\tOK, CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE ONE BYTE ONLY-> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
											#endif
										}
										else
										{
											pParams->pEncodingString[w] = 0xFFFD;
											pParams->pUtf8String[w] = 0xFFFD;

											#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
											wprintf(L"\tCACCHIO, CID %u(hex: %X) NON TROVATO NEL CODE SPACE RANGE ONE BYTE ONLY -> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
											#endif
										}

										w++;
									}
								}

								pParams->pEncodingString[w] = L'\0';
								pParams->pUtf8String[w] = L'\0';
							}

							pParams->myToken.Type = T_STRING_LITERAL;
							pParams->myToken.vString = pParams->lexeme;

							pParams->bReadingStringState = 0;

							#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
							wprintf(L"GetNextToken: T_STRING_LITERAL -> '%s'\n", pParams->myToken.vString);
							#endif

							pParams->lastTokenOffset = pParams->currentFileOffset - (k + 1);

							pParams->myToken.vStringLen = k;

							return;
						}
						else
						{
							pParams->nStackStringOpenParen--; // POP
						}
					}

					if ( c == '(' )
					{
						pParams->nStackStringOpenParen++; // PUSH
					}
				}
				else
				{
					state = S7;
				}
				break;

			case S7:
				if ( c == 'n' )
				{
					if ( pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0 )
					{
						pParams->pEncodingString[k] = L'\n';
						pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\n'];
					}
					pParams->lexeme[k++] = '\n';
					state = S6;
				}
				else if ( c == 'r' )
				{
					if ( pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0 )
					{
						pParams->pEncodingString[k] = L'\r';
						pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\r'];
					}
					pParams->lexeme[k++] = '\r';
					state = S6;
				}
				else if ( c == 't' )
				{
					if ( pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0 )
					{
						pParams->pEncodingString[k] = L'\t';
						pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\t'];
					}
					pParams->lexeme[k++] = '\t';
					state = S6;
				}
				else if ( c == 'b' )
				{
					if ( pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0 )
					{
						pParams->pEncodingString[k] = L'\b';
						pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\b'];
					}
					pParams->lexeme[k++] = '\b';
					state = S6;
				}
				else if ( c == 'f' )
				{
					if ( pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0 )
					{
						pParams->pEncodingString[k] = L'\f';
						pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\f'];
					}
					pParams->lexeme[k++] = '\f';
					state = S6;
				}
				else if ( c == '(' )
				{
					if ( pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0 )
					{
						pParams->pEncodingString[k] = L'(';
						pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'('];
					}
					pParams->lexeme[k++] = '(';
					state = S6;
				}
				else if ( c == ')' )
				{
					if ( pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0 )
					{
						pParams->pEncodingString[k] = L')';
						pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L')'];
					}
					pParams->lexeme[k++] = ')';
					state = S6;
				}
				else if ( c == '\\' )
				{
					if ( pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0 )
					{
						pParams->pEncodingString[k] = L'\\';
						pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\\'];
					}
					pParams->lexeme[k++] = '\\';
					state = S6;
				}
				else if ( c == '\n' )
				{
					state = S6;
				}
				else if ( c == '\r' )
				{
					state = S8;
				}
				else if ( c >= '0' && c <= '7' )
				{
					c1 = c;
					state = S9;
				}
				else
				{
					pParams->lexeme[k++] = c;
					state = S6;
				}
				break;
			case S8:
				if ( c != '\n' )
				{
					if ( pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0 )
					{
						pParams->pEncodingString[k] = c;
						pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[c];
					}
					pParams->lexeme[k++] = c;
				}
				state = S6;
				break;
			case S9:
				if ( c >= '0' && c <= '7' )
				{
					c2 = c;
					state = S10;
				}
				else
				{
					if ( !GetOctalChar(c1, '\0', '\0', &cOctal, &bOctalOverflow) )
					{
						if ( bOctalOverflow )
						{
							state = S6; // ignora
						}
						else
						{
							pParams->myToken.Type = T_ERROR;
							snprintf(pParams->szError, 8192, "Errore GetNextToken: numero ottale errato.\n");
							myShowErrorMessage(pParams, pParams->szError, 1);
							//wprintf(L"Errore GetNextToken: numero ottale errato.\n");
							//fwprintf(pParams->fpErrors, L"Errore GetNextToken: numero ottale errato.\n");
							return;
						}
					}
					else
					{
						if ( 0 != cOctal )
						{
							if (pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0)
							{
								pParams->pEncodingString[k] = cOctal;
								pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[cOctal];
							}

							pParams->lexeme[k++] = cOctal;
						}
						else if ( 0 == cOctal && ( pParams->bOnParseEncryptObject ||	pParams->nCurrentFontSubtype == FONT_SUBTYPE_Type0 ) )
						{
							pParams->lexeme[k++] = cOctal;
						}

						state = S6;
					}
				}
				break;
			case S10:
				if ( c >= '0' && c <= '7' )
				{
					if ( !GetOctalChar(c1, c2, c, &cOctal, &bOctalOverflow) )
					{
						if ( bOctalOverflow )
						{
							state = S6; // ignora
						}
						else
						{
							pParams->myToken.Type = T_ERROR;
							snprintf(pParams->szError, 8192, "Errore GetNextToken: numero ottale errato.\n");
							myShowErrorMessage(pParams, pParams->szError, 1);
							//wprintf(L"Errore GetNextToken: numero ottale errato.\n");
							//fwprintf(pParams->fpErrors, L"Errore GetNextToken: numero ottale errato.\n");
							return;
						}
					}
					else
					{
						if ( 0 != cOctal )
						{
							if (pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0)
							{
								pParams->pEncodingString[k] = cOctal;
								pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[cOctal];
							}

							pParams->lexeme[k++] = cOctal;
						}
						else if ( 0 == cOctal && ( pParams->bOnParseEncryptObject ||	pParams->nCurrentFontSubtype == FONT_SUBTYPE_Type0 ) )
						{
							pParams->lexeme[k++] = cOctal;
						}

						state = S6;
					}
				}
				else
				{
					if ( !GetOctalChar(c1, c2, '\0', &cOctal, &bOctalOverflow) )
					{
						if ( bOctalOverflow )
						{
							state = S6; // ignora
						}
						else
						{
							pParams->myToken.Type = T_ERROR;
							snprintf(pParams->szError, 8192, "Errore GetNextToken: numero ottale errato.\n");
							myShowErrorMessage(pParams, pParams->szError, 1);
							//wprintf(L"Errore GetNextToken: numero ottale errato.\n");
							//fwprintf(pParams->fpErrors, L"Errore GetNextToken: numero ottale errato.\n");
							return;
						}
					}
					else
					{
						if ( 0 != cOctal )
						{
							if (pParams->nCurrentFontSubtype != FONT_SUBTYPE_Type0)
							{
								pParams->pEncodingString[k] = cOctal;
								pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[cOctal];
							}

							pParams->lexeme[k++] = cOctal;
						}
						else if ( 0 == cOctal && ( pParams->bOnParseEncryptObject ||	pParams->nCurrentFontSubtype == FONT_SUBTYPE_Type0 ) )
						{
							pParams->lexeme[k++] = cOctal;
						}

						state = S6;
					}
				}
				break;
			case S11:
				if ( c == '<' )
				{
					pParams->myToken.Type = T_DICT_BEGIN;
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"GetNextTOken: T_DICT_BEGIN -> <<\n");
					#endif

					pParams->lastTokenOffset = pParams->currentFileOffset - 2;

					return;
				}
				else if ( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') )
				{
					pParams->lexeme[k++] = c;
				}
				else if ( c == ' ' || c == '\t' || c == '\f' || c == '\0' || c == '\n' || c == '\r' || c == '\b' )
				{
					; // IGNORIAMO
				}
				else if ( c == '>' )
				{
					if ( k % 2 > 0 )
					{
						pParams->lexeme[k++] = '0';
					}

					y = 0;
					z = 0;
					for ( x = 0; x < k/2; x++ )
					{
						switch ( pParams->lexeme[y] )
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
								cHexadecimal = 0;
								break;
						}

						switch ( pParams->lexeme[y + 1] )
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
								cHexadecimal += 0;
								break;
						}

						if ( pParams->bReadingActualTextValue ||
						     FONT_SUBTYPE_Type0 == pParams->nCurrentFontSubtype )
						{
							pParams->lexemeTemp[z] = cHexadecimal;
							z++;
						}
						else if ( 0 != cHexadecimal )
						{
							if ( FONT_SUBTYPE_Type0 != pParams->nCurrentFontSubtype )
							{
								pParams->pEncodingString[z] = cHexadecimal;
								pParams->pUtf8String[z] = pParams->pCurrentEncodingArray[cHexadecimal];

								#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_SINGLE_CHAR_ENCODING_SCANNER)
								if ( cHexadecimal != ')' )
								{
									wprintf(L"c = '%lc', Value = %d; pParams->pCurrentEncodingArray[c] = '%lc', Value = %d; pParams->pUtf8String[%d] = '%lc'\n",
											cHexadecimal, cHexadecimal, pParams->pCurrentEncodingArray[cHexadecimal], pParams->pCurrentEncodingArray[cHexadecimal], z, pParams->pUtf8String[z]);
									wprintf(L"\tpParams->pEncodingString[%d] = %d\n", z, pParams->pEncodingString[z]);
								}
								#endif
							}
							z++;
						}

						y += 2;
					}

					if ( pParams->bReadingActualTextValue )
					{
						// ActualText: conserva i byte originali senza decodifica CID.
						pParams->lexemeTemp[z] = '\0';
						memcpy(pParams->lexeme, pParams->lexemeTemp, z);
						pParams->lexeme[z] = '\0';
						pParams->pEncodingString[0] = L'\0';
						pParams->pUtf8String[0] = L'\0';
						pParams->myToken.vStringLen = z;
						pParams->bReadingActualTextValue = 0;
					}
					else if ( FONT_SUBTYPE_Type0 == pParams->nCurrentFontSubtype )
					{
						// Type0: decodifica i byte in lexemeTemp e copiali in lexeme.
						// Cosi' myToken.vString contiene i byte REALI (es. <82 E0 82 CC>)
						// e non l'ASCII esadecimale "82E082CC...".
						ManageTypeZeroHexString(pParams, z);

						memcpy(pParams->lexeme, pParams->lexemeTemp, z);
						pParams->lexeme[z] = '\0';
						pParams->myToken.vStringLen = z;   // lunghezza in BYTE reali
					}
					else
					{
						// Non-Type0: lexeme contiene la stringa ASCII hex.
						// Termina tutti i buffer come faceva il codice originale.
						pParams->lexemeTemp[z] = '\0';
						pParams->pEncodingString[z] = L'\0';
						pParams->pUtf8String[z] = L'\0';
						pParams->lexeme[k] = '\0';
						pParams->myToken.vStringLen = k;   // lunghezza in CIFRE hex
					}

					pParams->myToken.Type = T_STRING_HEXADECIMAL;
					pParams->myToken.vString = pParams->lexeme;

					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
					wprintf(L"\nGetNextToken: T_STRING_HEXADECIMAL -> '%s'\n", pParams->myToken.vString);
					#endif

					pParams->lastTokenOffset = pParams->currentFileOffset - (k + 1);

					return;
				}
				else
				{
					pParams->pEncodingString[0] = L'\0';
					pParams->pUtf8String[0] = L'\0';

					pParams->myToken.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextToken: numero esadecimale errato. Carattere non valido -> codice decimale = %u\n", (unsigned int)c);
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextToken: numero esadecimale errato. Carattere non valido -> codice decimale = %u\n", (unsigned int)c);
					//fwprintf(pParams->fpErrors, L"Errore GetNextToken: numero esadecimale errato. Carattere non valido -> codice decimale = %u\n", (unsigned int)c);

					c = pParams->pReadNextChar(pParams);

					return;
				}
				break;
			default:
				break;
		} // FINE -> switch ( state )

		c = pParams->pReadNextChar(pParams);
		if ( T_EOF == pParams->myToken.Type && pParams->blockCurPos >= pParams->blockLen )
		{
			return;
		}

		if ( k > MAX_STRING_LENTGTH_IN_CONTENT_STREAM )
		{
			pParams->myToken.Type = T_ERROR;
			snprintf(pParams->szError, 8192, "Errore GetNextToken: lexeme troppo lungo(> %d byte)\n", MAX_STRING_LENTGTH_IN_CONTENT_STREAM);
			myShowErrorMessage(pParams, pParams->szError, 1);
			//wprintf(L"Errore GetNextToken: lexeme troppo lungo(> %d byte)\n", MAX_STRING_LENTGTH_IN_CONTENT_STREAM);
			//fwprintf(pParams->fpErrors, L"Errore GetNextToken: lexeme troppo lungo(> %d byte)\n", MAX_STRING_LENTGTH_IN_CONTENT_STREAM);
			return;
		}
	}

	return;
}

int ManageTypeZeroHexString(Params *pParams, int lenCurrLexeme)
{
	int retVal = 1;

	int k = lenCurrLexeme;

	uint32_t u = 0;
	int z = 0;
	int w = 0;
	unsigned char num1;
	unsigned char num2;

	if ( pParams->bHasCodeSpaceTwoByte && !pParams->bHasCodeSpaceOneByte ) // CODE SPACE RANGE TWO BYTES ONLY.
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
		wprintf(L"\nEHI!!! SIAMO NEL CODE SPACE RANGE TWO BYTES ONLY.\n");
		#endif

		w = 0;
		for ( z = 0; z < k - 1; z++ )
		{
			num1 = pParams->lexemeTemp[z];
			z++;
			if ( z < k )
				num2 = pParams->lexemeTemp[z];
			else
				num2 = 0;

			if ( MACHINE_ENDIANNESS_LITTLE_ENDIAN == pParams->nThisMachineEndianness )
				pParams->myCID = MY_MAKEWORD(num2, num1);
			else
				pParams->myCID = MY_MAKEWORD(num1, num2);

			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
			wprintf(L"\tpParams->myCID TWO BYTES = %u(hex: %X)\n", pParams->myCID, pParams->myCID);
			#endif


			for ( u = 0; u < pParams->nCurrentFontCodeSpacesNum; u++ )
			{
				if ( NULL != pParams->pCodeSpaceRangeArray )
				{
					if ( pParams->myCID >= pParams->pCodeSpaceRangeArray[u].nFrom && pParams->myCID <= pParams->pCodeSpaceRangeArray[u].nTo )
					{
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
						wprintf(L"\tECCOLO! -> CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE TWO BYTE, NEL RANGE %lu(hex: %X) - %lu(hex: %X).\n", pParams->myCID, pParams->myCID, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nTo, pParams->pCodeSpaceRangeArray[u].nTo);
						#endif

						break;
					}
				}
				else
				{
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
					wprintf(L"\tpParams->pCodeSpaceRangeArray È NULL!!!\n");
					#endif

					pParams->myCID = 0xFFFD;
					u = pParams->nCurrentFontCodeSpacesNum + 1;
					break;
				}
			}

			if ( u >= pParams->nCurrentFontCodeSpacesNum ) // CID NON TROVATO NEL CODE SPACE RANGE
			{
				// Effettuare qui la ricerca nel NOTDEF CODE SPACE RANGE

				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
				wprintf(L"\tCID %u(hex: %X) NON TROVATO NEL CODE SPACE RANGE.\n", pParams->myCID, pParams->myCID);
				#endif

				pParams->myCID = 0xFFFD;

			}
			#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
			else
			{
				wprintf(L"\tCID %u(hex: %X) TROVATO NEL CODE SPACE RANGE. YAHOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO!!!\n", pParams->myCID, pParams->myCID);
			}
			#endif

			if ( 0xFFFD != pParams->myCID )
			{
				pParams->pEncodingString[w] = pParams->myCID;
				pParams->pUtf8String[w] = pParams->pCurrentEncodingArray[pParams->myCID];

				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
				wprintf(L"\tOK, CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE TWO BYTES-> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
				#endif
			}
			else
			{
				pParams->pEncodingString[w] = 0xFFFD;
				pParams->pUtf8String[w] = 0xFFFD;

				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
				wprintf(L"\tATTENZIONE, CID %u(hex: %X) NON TROVATO NEL CODE SPACE RANGE TWO BYTES-> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
				#endif
			}

			w++;
		}
	}
	else if ( pParams->bHasCodeSpaceTwoByte && pParams->bHasCodeSpaceOneByte ) // CODE SPACE RANGE ONE AND TWO BYTES.
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
		wprintf(L"\nEHI!!! SIAMO NEL CODE SPACE RANGE ONE AND TWO BYTES.\n");
		#endif

		w = 0;
		for ( z = 0; z < k; z++ )
		{
			// ALL'INIZIO ESTRAIAMO UN SOLO BYTE E CERCHIAMO NEL ONE BYTE CODE SPACE RANGE:
			num1 = pParams->lexemeTemp[z];
			z++;

			pParams->myCID = num1;

			for ( u = 0; u < pParams->nCurrentFontCodeSpacesNum; u++ )
			{
				if ( NULL != pParams->pCodeSpaceRangeArray )
				{
					if ( 1 == pParams->pCodeSpaceRangeArray[u].nNumBytes && (pParams->myCID >= pParams->pCodeSpaceRangeArray[u].nFrom && pParams->myCID <= pParams->pCodeSpaceRangeArray[u].nTo) )
					{
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
						wprintf(L"\tECCOLO! -> CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE ONE BYTE, NEL RANGE %lu(hex: %X) - %lu(hex: %X).\n", pParams->myCID, pParams->myCID, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nTo, pParams->pCodeSpaceRangeArray[u].nTo);
						#endif

						break;
					}
				}
				else
				{
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
					wprintf(L"\tpParams->pCodeSpaceRangeArray È NULL!!!\n");
					#endif

					pParams->myCID = 0xFFFD;
					u = pParams->nCurrentFontCodeSpacesNum + 1;
					break;
				}
			}
			if ( u >= pParams->nCurrentFontCodeSpacesNum ) // CID ONE BYTE NON TROVATO NEL CODE SPACE RANGE
			{
				// Effettuare qui la ricerca nel NOTDEF CODE SPACE RANGE

				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
				wprintf(L"\tCID %u(hex: %X) NON TROVATO NEL CODE SPACE RANGE ONE BYTE. CONSUMO UN ALTRO BYTE.\n", pParams->myCID, pParams->myCID);
				#endif
			}
			else // CID ONE BYTE TROVATO NEL CODE SPACE RANGE
			{
				pParams->pEncodingString[w] = pParams->myCID;
				pParams->pUtf8String[w] = pParams->pCurrentEncodingArray[pParams->myCID];

				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
				wprintf(L"\tOK, CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE ONE BYTES-> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
				#endif

				w++;
				continue;
			}

			// PRELEVIAMO UN ALTRO BYTE E CERCHIAMO NEL TWO BYTES CODE SPACE RANGE:
			if ( z < k )
				num2 = pParams->lexemeTemp[z];
			else
				num2 = 0;

			if ( MACHINE_ENDIANNESS_LITTLE_ENDIAN == pParams->nThisMachineEndianness )
				pParams->myCID = MY_MAKEWORD(num2, num1);
			else
				pParams->myCID = MY_MAKEWORD(num1, num2);

			for ( u = 0; u < pParams->nCurrentFontCodeSpacesNum; u++ )
			{
				if ( NULL != pParams->pCodeSpaceRangeArray )
				{
					if ( 2 == pParams->pCodeSpaceRangeArray[u].nNumBytes && (pParams->myCID >= pParams->pCodeSpaceRangeArray[u].nFrom && pParams->myCID <= pParams->pCodeSpaceRangeArray[u].nTo) )
					{
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
						wprintf(L"\tECCOLO! -> CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE TWO BYTE, NEL RANGE %lu(hex: %X) - %lu(hex: %X).\n", pParams->myCID, pParams->myCID, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nTo, pParams->pCodeSpaceRangeArray[u].nTo);
						#endif

						break;
					}
				}
				else
				{
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
					wprintf(L"\tpParams->pCodeSpaceRangeArray È NULL!!!\n");
					#endif

					pParams->myCID = 0xFFFD;
					u = pParams->nCurrentFontCodeSpacesNum + 1;
					break;
				}
			}

			if ( u >= pParams->nCurrentFontCodeSpacesNum ) // CID NON TROVATO NEL CODE SPACE RANGE
			{
				// Effettuare qui la ricerca nel NOTDEF CODE SPACE RANGE

				pParams->myCID = 0xFFFD;
			}

			if ( 0xFFFD != pParams->myCID )
			{
				pParams->pEncodingString[w] = pParams->myCID;
				pParams->pUtf8String[w] = pParams->pCurrentEncodingArray[pParams->myCID];

				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
				wprintf(L"\tOK, CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE TWO BYTES-> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
				#endif
			}
			else
			{
				pParams->pEncodingString[w] = 0xFFFD;
				pParams->pUtf8String[w] = 0xFFFD;

				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
				wprintf(L"\tCACCHIO, CID %u(hex: %X) NON TROVATO NEL CODE SPACE RANGE ONE AND TWO BYTES-> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
				#endif
			}

			w++;
		}
	}
	else if ( !pParams->bHasCodeSpaceTwoByte && pParams->bHasCodeSpaceOneByte ) // CODE SPACE RANGE ONE BYTE ONLY.
	{
		#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
		wprintf(L"\nEHI!!! SIAMO NEL CODE SPACE RANGE ONE BYTE ONLY.\n");
		#endif

		w = 0;
		for ( z = 0; z < k; z++ )
		{
			pParams->myCID = pParams->lexemeTemp[z];
			z++;

			for ( u = 0; u < pParams->nCurrentFontCodeSpacesNum; u++ )
			{
				if ( NULL != pParams->pCodeSpaceRangeArray )
				{
					if ( pParams->myCID >= pParams->pCodeSpaceRangeArray[u].nFrom && pParams->myCID <= pParams->pCodeSpaceRangeArray[u].nTo )
					{
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
						wprintf(L"\tECCOLO! -> CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE ONE BYTE, NEL RANGE %lu(hex: %X) - %lu(hex: %X).\n", pParams->myCID, pParams->myCID, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nFrom, pParams->pCodeSpaceRangeArray[u].nTo, pParams->pCodeSpaceRangeArray[u].nTo);
						#endif

						break;
					}
				}
				else
				{
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
					wprintf(L"\tpParams->pCodeSpaceRangeArray È NULL!!!\n");
					#endif

					pParams->myCID = 0xFFFD;
					u = pParams->nCurrentFontCodeSpacesNum + 1;
					break;
				}
			}

			if ( u >= pParams->nCurrentFontCodeSpacesNum ) // CID NON TROVATO NEL CODE SPACE RANGE
			{
				// Effettuare qui la ricerca nel NOTDEF CODE SPACE RANGE

				pParams->myCID = 0xFFFD;
			}

			if ( 0xFFFD != pParams->myCID )
			{
				pParams->pEncodingString[w] = pParams->myCID;
				pParams->pUtf8String[w] = pParams->pCurrentEncodingArray[pParams->myCID];

				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
				wprintf(L"\tOK, CID %u(hex: %X) TROVATO NEL CODE SPACE RANGE ONE BYTE ONLY -> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
				#endif
			}
			else
			{
				pParams->pEncodingString[w] = 0xFFFD;
				pParams->pUtf8String[w] = 0xFFFD;

				#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_ManageContent_PrintStrings_HEXADECIMAL)
				wprintf(L"\tATTENZIONE, CID %u(hex: %X) NON TROVATO NEL CODE SPACE RANGE ONE BYTE ONLY -> pParams->pUtf8String[%d] = '%lc' -> (hex: <%4X>)\n", pParams->myCID, pParams->myCID, w, pParams->pUtf8String[w], pParams->pUtf8String[w]);
				#endif
			}

			w++;
		}
	}

	pParams->pEncodingString[w] = L'\0';
	pParams->pUtf8String[w] = L'\0';

	return retVal;
}

unsigned char ReadNextCharLengthObj(Params *pParams)
{
	if ( pParams->blockCurPosLengthObj >= pParams->blockLenLengthObj )
	{
		pParams->blockLenLengthObj = fread(pParams->myBlockLengthObj, 1, BLOCK_SIZE, pParams->fpLengthObjRef);
		if ( pParams->blockLenLengthObj == 0 )
		{
			pParams->myTokenLengthObj.Type = T_EOF;
			return '\0';
		}
		pParams->blockCurPosLengthObj = 0;
	}

	return pParams->myBlockLengthObj[pParams->blockCurPosLengthObj++];
}

void GetNextTokenLengthObj(Params *pParams)
{
	States state = S0;
	unsigned char c;
	char lexeme[4096];
	int k = 0;
	int nDelimChar;

	c = ReadNextCharLengthObj(pParams);
	if ( pParams->myTokenLengthObj.Type == T_EOF )
		return;

	while ( c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' ||  c == '\0' )
	{
		c = ReadNextCharLengthObj(pParams);
		if ( pParams->myTokenLengthObj.Type == T_EOF )
			return;
	}

	while ( 1 )
	{
		switch ( state )
		{
			inizioswitch:

			case S0:
				k = 0;

				if ( c == '%' )
				{
					c = ReadNextCharLengthObj(pParams);
					while ( c != '\n' && c != '\r' )
					{
						c = ReadNextCharLengthObj(pParams);
						if ( pParams->myTokenLengthObj.Type == T_EOF )
							return;
					}
					if ( c == '\r' )
					{
						c = ReadNextCharLengthObj(pParams);
						if ( pParams->myTokenLengthObj.Type == T_EOF )
							return;
						if ( c != '\n' )
						{
							goto inizioswitch;
						}
					}

					while ( c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' ||  c == '\0' )
					{
						c = ReadNextCharLengthObj(pParams);

						if ( pParams->myTokenLengthObj.Type == T_EOF )
							return;
					}
				}
				else if ( c == '+' || ( c >= '0' && c <= '9' ) )
				{
					lexeme[k++] = c;
					state = S1;
				}
				else if ( c == 'o' )
				{
					state = S2;
				}
				else if ( c == 'e' )
				{
					state = S4;
				}
				break;
			case S1:
				if (  c >= '0' && c <= '9' )
				{
					lexeme[k++] = c;
				}
				else
				{
					nDelimChar = IsDelimiterChar(c);
					if ( DELIM_SPACECHAR != nDelimChar )
					{
						pParams->myTokenLengthObj.Type = T_ERROR;
						snprintf(pParams->szError, 8192, "Errore GetNextTokenLengthObj: atteso T_INT_LITERAL\n");
						myShowErrorMessage(pParams, pParams->szError, 1);
						//wprintf(L"Errore GetNextTokenLengthObj: atteso T_INT_LITERAL\n");
						//fwprintf(pParams->fpErrors, L"Errore GetNextTokenLengthObj: atteso T_INT_LITERAL\n");
						return;
					}

					lexeme[k] = '\0';
					pParams->myTokenLengthObj.Type = T_INT_LITERAL;
					pParams->myTokenLengthObj.vInt = atoi(lexeme);
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"GetNextTOkenLengthObj: T_INT_LITERAL -> %d\n", pParams->myTokenLengthObj.vInt);
					#endif
					return;
				}
				break;
			case S2:
				if ( c == 'b' )
				{
					state = S3;
				}
				else
				{
					pParams->myTokenLengthObj.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextTokenLengthObj: atteso T_KW_OBJ\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextTokenLengthObj: atteso T_KW_OBJ\n");
					//fwprintf(pParams->fpErrors, L"Errore GetNextTokenLengthObj: atteso T_KW_OBJ\n");
					return;
				}
				break;
			case S3:
				if ( c == 'j' )
				{
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"\nGetNextTOkenLengthObj T_KW_OBJ\n");
					#endif
					pParams->myTokenLengthObj.Type = T_KW_OBJ;
					return;
				}
				else
				{
					pParams->myTokenLengthObj.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextTokenLengthObj: atteso T_KW_OBJ\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextTokenLengthObj: atteso T_KW_OBJ\n");
					//fwprintf(pParams->fpErrors, L"Errore GetNextTokenLengthObj: atteso T_KW_OBJ\n");
					return;
				}
				break;
			case S4:
				if ( c == 'n' )
				{
					state = S5;
				}
				else
				{
					pParams->myTokenLengthObj.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					//fwprintf(pParams->fpErrors, L"Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					return;
				}
				break;
			case S5:
				if ( c == 'd' )
				{
					state = S6;
				}
				else
				{
					pParams->myTokenLengthObj.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					//fwprintf(pParams->fpErrors, L"Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					return;
				}
				break;
			case S6:
				if ( c == 'o' )
				{
					state = S7;
				}
				else
				{
					pParams->myTokenLengthObj.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					//fwprintf(pParams->fpErrors, L"Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					return;
				}
				break;
			case S7:
				if ( c == 'b' )
				{
					state = S8;
				}
				else
				{
					pParams->myTokenLengthObj.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextTokenLengthObj: atteso T_KW_OBJ\n");
					//fwprintf(pParams->fpErrors, L"Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					return;
				}
				break;
			case S8:
				if ( c == 'j' )
				{
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextToken_FN)
					wprintf(L"\nGetNextTokenLengthObj T_KW_ENDOBJ\n");
					#endif
					pParams->myTokenLengthObj.Type = T_KW_ENDOBJ;
					return;
				}
				else
				{
					pParams->myTokenLengthObj.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextTokenLengthObj: atteso T_KW_OBJ\n");
					//fwprintf(pParams->fpErrors, L"Errore GetNextTokenLengthObj: atteso T_KW_ENDOBJ\n");
					return;
				}
				break;
			default:
				break;
		} // FINE -> switch ( state )

		c = ReadNextCharLengthObj(pParams);
		if ( pParams->myTokenLengthObj.Type == T_EOF )
			return;

		if ( k >= 4096 )
		{
			pParams->myTokenLengthObj.Type = T_ERROR;
			snprintf(pParams->szError, 8192, "Errore GetNextTokenLength: lexeme troppo lungo(> %d byte)\n", MAX_STRING_LENTGTH_IN_CONTENT_STREAM);
			myShowErrorMessage(pParams, pParams->szError, 1);
			//wprintf(L"Errore GetNextTokenLength: lexeme troppo lungo(> %d byte)\n", MAX_STRING_LENTGTH_IN_CONTENT_STREAM);
			//fwprintf(pParams->fpErrors, L"Errore GetNextTokenLength: lexeme troppo lungo(> %d byte)\n", MAX_STRING_LENTGTH_IN_CONTENT_STREAM);
			return;
		}
	} // FINE -> While ( 1 )

	return;
}

// ***************************************************************************************************************************************************************************

unsigned char ReadNextCharFromStmObjStream(Params *pParams)
{
	if ( pParams->blockCurPos >= pParams->blockLen )
	{
		pParams->myToken.Type = T_EOF;
		//return '\0';
		return ' ';
	}

	return pParams->myBlock[pParams->blockCurPos++];
}

unsigned char ReadNextCharFromToUnicodeStream(Params *pParams)
{
	if ( pParams->blockCurPosToUnicode >= pParams->blockLenToUnicode )
	{
		pParams->myToken.Type = T_EOF;
		//return '\0';
		return ' ';
	}

	return pParams->myBlockToUnicode[pParams->blockCurPosToUnicode++];
}

void GetNextTokenFromToUnicodeStream(Params *pParams)
{
	States state = S0;
	unsigned char c = 0;
	unsigned char c1 = 0, c2 = 0; // warning autoconf
	unsigned char cOctal = 0;
	unsigned char cHexadecimal = 0;
	int k = 0;
	int bOctalOverflow = 0;
	int nDelimChar = 0;

	int x = 0;
	int y = 0;
	int z = 0;

	if ( pParams->blockCurPosToUnicode > pParams->blockLenToUnicode )
	{
		pParams->myToken.Type = T_EOF;
		return;
	}

	c = ReadNextCharFromToUnicodeStream(pParams);
	if ( pParams->myToken.Type == T_EOF )
		return;

	while ( c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' ||  c == '\0' )
	{
		c = ReadNextCharFromToUnicodeStream(pParams);

		if ( pParams->myToken.Type == T_EOF )
			return;
	}

	while ( 1 )
	{
		switch ( state )
		{
			inizioswitch:

			case S0:
				k = 0;
				pParams->nStackStringOpenParen = 0;

				if ( c == '%' )
				{
					c = ReadNextCharFromToUnicodeStream(pParams);
					while ( c != '\n' && c != '\r' )
					{
						c = ReadNextCharFromToUnicodeStream(pParams);
						if ( pParams->myToken.Type == T_EOF )
							return;
					}
					if ( c == '\r' )
					{
						c = ReadNextCharFromToUnicodeStream(pParams);
						if ( pParams->myToken.Type == T_EOF )
							return;
						if ( c != '\n' )
						{
							goto inizioswitch;
						}
					}

					while ( c == ' ' || c == '\r' || c == '\n' || c == '\t' || c == '\f' ||  c == '\0' )
					{
						c = ReadNextCharFromToUnicodeStream(pParams);

						if ( pParams->myToken.Type == T_EOF )
							return;
					}
				}
				else if ( c == '-' || c == '+' || ( c >= '0' && c <= '9' ) )
				{
					pParams->lexeme[k++] = c;
					state = S1;
				}
				else if ( c == '.' )
				{
					pParams->lexeme[k++] = c;
					state = S2;
				}
				else if ( c == '/' )
				{
					state = S3;
				}
				else if ( c == '(' )
				{
					pParams->nStackStringOpenParen++; // PUSH
					state = S6;
				}
				else if ( c == '<' )
				{
					state = S11;
				}
				else if ( c == '[' )
				{
					pParams->myToken.Type = T_QOPAREN;
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
					wprintf(L"GetNextTokenFromToUnicodeStream: T_QOPAREN -> [\n");
					#endif
					return;
				}
				else if ( c == ']' )
				{
					pParams->myToken.Type = T_QCPAREN;
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
					wprintf(L"GetNextTokenFromToUnicodeStream: T_QCPAREN -> ]\n");
					#endif
					return;
				}
				else if ( c == '>' )
				{
					c = ReadNextCharFromToUnicodeStream(pParams);
					if ( c == '>' )
					{
						pParams->myToken.Type = T_DICT_END;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_DICT_END -> >>\n");
						#endif
						return;
					}
				}
				else if ( '{' == c ) // IGNORIAMO, NON SERVE A UN C.
				{
					pParams->myToken.Type = T_LEFT_CURLY_BRACKET;
					return;
				}
				else if ( '}' == c ) // IGNORIAMO, NON SERVE A UN C.
				{
					pParams->myToken.Type = T_RIGHT_CURLY_BRACKET;
					return;
				}
				else // T_STRING oppure T_KEYWORD
				{
					nDelimChar = IsDelimiterChar(c);

					if ( nDelimChar )
						goto inizioswitch;

					while ( !nDelimChar )
					{
						if ( k < MAX_STRING_LENTGTH_IN_CONTENT_STREAM )
						{
							pParams->lexeme[k++] = c;
						}
						else
						{
							pParams->lexeme[k] = '\0';
						}
						c = ReadNextCharFromToUnicodeStream(pParams);
						nDelimChar = IsDelimiterChar(c);
					}
					pParams->lexeme[k] = '\0';
					if ( DELIM_SPECIALSYMBOL == nDelimChar )
					{
						pParams->blockCurPosToUnicode--;
					}

					if ( strncmp(pParams->lexeme, "begincodespacerange", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_begincodespacerange;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_begincodespacerange -> 'begincodespacerange'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "endcodespacerange", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_endcodespacerange;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_endcodespacerange -> 'endcodespacerange'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "beginbfchar", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_beginbfchar;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_beginbfchar -> 'beginbfchar'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "endbfchar", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_endbfchar;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_endbfchar -> 'endbfchar'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "beginbfrange", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_beginbfrange;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_beginbfrange -> 'beginbfrange'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "endbfrange", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_endbfrange;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_endbfrange -> 'endbfrange'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "begincidchar", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_begincidchar;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_begincidchar -> 'begincidchar'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "endcidchar", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_endcidchar;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_endcidchar -> 'endcidchar'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "begincidrange", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_begincidrange;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_begincidrange -> 'begincidrange'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "endcidrange", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_endcidrange;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_endcidrange -> 'endcidrange'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "beginnotdefrange", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_beginnotdefrange;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_beginnotdefrange -> 'beginnotdefrange'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "endnotdefrange", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_endnotdefrange;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_endnotdefrange -> 'endnotdefrange'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "beginnotdefchar", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_beginnotdefchar;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_beginnotdefchar -> 'beginnotdefchar'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "endnotdefchar", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_endnotdefchar;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_endnotdefchar -> 'endnotdefchar'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "usecmap", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_CONTENT_OP_usecmap;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_CONTENT_OP_usecmap -> 'usecmap'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "true", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_KW_TRUE;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_KW_TRUE -> 'true'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "false", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_KW_FALSE;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_KW_FALSE -> 'false'\n");
						#endif
						return;
					}
					else if ( strncmp(pParams->lexeme, "null", MAX_STRING_LENTGTH_IN_CONTENT_STREAM) == 0 )
					{
						pParams->myToken.Type = T_KW_NULL;
						#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
						wprintf(L"GetNextTokenFromToUnicodeStream: T_KW_NULL -> 'null'\n");
						#endif
						return;
					}
					else
					{
						if ( k > 0 )
						{
							pParams->myToken.Type = T_STRING;
							pParams->myToken.vString = pParams->lexeme;

							#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
							wprintf(L"GetNextTokenFromToUnicodeStream: T_STRING -> '%s'\n", pParams->myToken.vString);
							#endif
						}
						else
						{
							pParams->myToken.Type = T_VOID_STRING;
							#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
							wprintf(L"GetNextTokenFromToUnicodeStream: T__VOID_STRING\n");
							#endif

							ReadNextCharFromToUnicodeStream(pParams);
						}

						return;
					}
				}
				break;
			case S1:
				if (  c >= '0' && c <= '9' )
				{
					pParams->lexeme[k++] = c;
				}
				else if ( c == '.' )
				{
					pParams->lexeme[k++] = c;
					state = S2;
				}
				else
				{
					pParams->lexeme[k] = '\0';
					pParams->myToken.Type = T_INT_LITERAL;
					pParams->myToken.vInt = atoi(pParams->lexeme);
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
					wprintf(L"GetNextTokenFromToUnicodeStream: T_INT_LITERAL -> %d\n", pParams->myToken.vInt);
					#endif
					nDelimChar = IsDelimiterChar(c);
					if ( DELIM_SPECIALSYMBOL == nDelimChar )
					{
						pParams->blockCurPosToUnicode--;
						pParams->nNumBytesReadFromCurrentStream--;
					}
					return;
				}
				break;
			case S2:
				if (  c >= '0' && c <= '9' )
				{
					pParams->lexeme[k++] = c;
				}
				else
				{
					pParams->lexeme[k] = '\0';

					pParams->myToken.Type = T_REAL_LITERAL;
					pParams->myToken.vDouble = atof(pParams->lexeme);
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
					wprintf(L"GetNextTOken: T_REAL_LITERAL -> %lf\n", pParams->myToken.vDouble);
					#endif
					nDelimChar = IsDelimiterChar(c);
					if ( DELIM_SPECIALSYMBOL == nDelimChar )
					{
						pParams->blockCurPosToUnicode--;
						pParams->nNumBytesReadFromCurrentStream--;
					}
					return;
				}
				break;
			case S3:
				nDelimChar = IsDelimiterChar(c);
				if ( !nDelimChar )
				{
					if ( c != '#' )
					{
						pParams->lexeme[k++] = c;
					}
					else
					{
						state = S4;
					}
				}
				else
				{
					pParams->lexeme[k] = '\0';
					pParams->myToken.Type = T_NAME;
					pParams->myToken.vString = pParams->lexeme;

					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
					wprintf(L"GetNextTokenFromToUnicodeStream: T_NAME -> '/%s'\n", pParams->myToken.vString);
					#endif

					if ( DELIM_SPECIALSYMBOL == nDelimChar )
					{
						pParams->blockCurPosToUnicode--;
						pParams->nNumBytesReadFromCurrentStream--;
					}

					return;
				}
				break;
			case S4:
				if ( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') )
				{
					c1 = c;
					state = S5;
				}
				else
				{
					pParams->myToken.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextTokenFromToUnicodeStream: numero esadecimale errato.\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextTokenFromToUnicodeStream: numero esadecimale errato.\n");
					//fwprintf(pParams->fpErrors, L"Errore GetNextTokenFromToUnicodeStream: numero esadecimale errato.\n");
					return;
				}
				break;
			case S5:
				if ( (c >= '0' && c >= '9') || (c >= 'A' && c <= 'F') || (c >= 'f' && c <= 'f') )
				{
					c2 = c;
					pParams->lexeme[k++] = GetHexChar(c1, c2);
					state = S3;
				}
				else
				{
					pParams->myToken.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextTokenFromToUnicodeStream: numero esadecimale errato.\n");
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextTokenFromToUnicodeStream: numero esadecimale errato.\n");
					//fwprintf(pParams->fpErrors, L"Errore GetNextTokenFromToUnicodeStream: numero esadecimale errato.\n");
					return;
				}
				break;
			case S6:
				if ( c != '\\' )
				{
					if ( '\0' != c )
					{
						pParams->pEncodingString[k] = c;
						pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[c];
						pParams->lexeme[k++] = c;
					}

					if ( c == ')' )
					{
						pParams->nStackStringOpenParen--; // POP

						if ( pParams->nStackStringOpenParen <= 0 )
						{
							pParams->lexeme[--k] = '\0';
							pParams->pEncodingString[k] = L'\0';
							pParams->pUtf8String[k] = L'\0';
							pParams->myToken.Type = T_STRING_LITERAL;
							pParams->myToken.vString = pParams->lexeme;

							#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
							wprintf(L"GetNextTokenFromToUnicodeStream: T_STRING_LITERAL -> '%s'\n", pParams->myToken.vString);
							#endif
							return;
						}
						else
						{
							pParams->nStackStringOpenParen--; // POP
						}
					}

					if ( c == '(' )
					{
						pParams->nStackStringOpenParen++; // PUSH
					}
				}
				else
				{
					state = S7;
				}
				break;
			case S7:
				if ( c == 'n' )
				{
					pParams->pEncodingString[k] = L'\n';
					pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\n'];
					pParams->lexeme[k++] = '\n';
					state = S6;
				}
				else if ( c == 'r' )
				{
					pParams->pEncodingString[k] = L'\r';
					pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\r'];
					pParams->lexeme[k++] = '\r';
					state = S6;
				}
				else if ( c == 't' )
				{
					pParams->pEncodingString[k] = L'\t';
					pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\t'];
					pParams->lexeme[k++] = '\t';
					state = S6;
				}
				else if ( c == 'b' )
				{
					pParams->pEncodingString[k] = L'\b';
					pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\b'];
					pParams->lexeme[k++] = '\b';
					state = S6;
				}
				else if ( c == 'f' )
				{
					pParams->pEncodingString[k] = L'\f';
					pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\f'];
					pParams->lexeme[k++] = '\f';
					state = S6;
				}
				else if ( c == '(' )
				{
					pParams->pEncodingString[k] = L'(';
					pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'('];
					pParams->lexeme[k++] = '(';
					state = S6;
				}
				else if ( c == ')' )
				{
					pParams->pEncodingString[k] = L')';
					pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L')'];
					pParams->lexeme[k++] = ')';
					state = S6;
				}
				else if ( c == '\\' )
				{
					pParams->pEncodingString[k] = L'\\';
					pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[L'\\'];
					pParams->lexeme[k++] = '\\';
					state = S6;
				}
				else if ( c == '\n' )
				{
					state = S6;
				}
				else if ( c == '\r' )
				{
					state = S8;
				}
				else if ( c >= '0' && c <= '7' )
				{
					c1 = c;
					state = S9;
				}
				else
				{
					pParams->lexeme[k++] = c;
					state = S6;
				}
				break;
			case S8:
				if ( c != '\n' )
				{
					pParams->pEncodingString[k] = c;
					pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[c];
					pParams->lexeme[k++] = c;
				}
				state = S6;
				break;
			case S9:
				if ( c >= '0' && c <= '7' )
				{
					c2 = c;
					state = S10;
				}
				else
				{
					if ( !GetOctalChar(c1, '\0', '\0', &cOctal, &bOctalOverflow) )
					{
						if ( bOctalOverflow )
						{
							state = S6; // ignora
						}
						else
						{
							pParams->myToken.Type = T_ERROR;
							snprintf(pParams->szError, 8192, "Errore GetNextTokenFromToUnicodeStream: numero ottale errato.\n");
							myShowErrorMessage(pParams, pParams->szError, 1);
							//wprintf(L"Errore GetNextTokenFromToUnicodeStream: numero ottale errato.\n");
							//fwprintf(pParams->fpErrors, L"Errore GetNextTokenFromToUnicodeStream: numero ottale errato.\n");
							return;
						}
					}
					else
					{
						if ( 0 != cOctal )
						{
							pParams->pEncodingString[k] = cOctal;
							pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[cOctal];
							pParams->lexeme[k++] = cOctal;
						}
						state = S6;
					}
				}
				break;
			case S10:
				if ( c >= '0' && c <= '7' )
				{
					if ( !GetOctalChar(c1, c2, c, &cOctal, &bOctalOverflow) )
					{
						if ( bOctalOverflow )
						{
							state = S6; // ignora
						}
						else
						{
							pParams->myToken.Type = T_ERROR;
							snprintf(pParams->szError, 8192, "Errore GetNextTokenFromToUnicodeStream: numero ottale errato.\n");
							myShowErrorMessage(pParams, pParams->szError, 1);
							//wprintf(L"Errore GetNextTokenFromToUnicodeStream: numero ottale errato.\n");
							//fwprintf(pParams->fpErrors, L"Errore GetNextTokenFromToUnicodeStream: numero ottale errato.\n");
							return;
						}
					}
					else
					{
						if ( 0 != cOctal )
						{
							pParams->pEncodingString[k] = cOctal;
							pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[cOctal];
							pParams->lexeme[k++] = cOctal;
						}
						state = S6;
					}
				}
				else
				{
					if ( !GetOctalChar(c1, c2, '\0', &cOctal, &bOctalOverflow) )
					{
						if ( bOctalOverflow )
						{
							state = S6; // ignora
						}
						else
						{
							pParams->myToken.Type = T_ERROR;
							snprintf(pParams->szError, 8192, "Errore GetNextTokenFromToUnicodeStream: numero ottale errato.\n");
							myShowErrorMessage(pParams, pParams->szError, 1);
							//wprintf(L"Errore GetNextTokenFromToUnicodeStream: numero ottale errato.\n");
							//fwprintf(pParams->fpErrors, L"Errore GetNextTokenFromToUnicodeStream: numero ottale errato.\n");
							return;
						}
					}
					else
					{
						if ( 0 != cOctal )
						{
							pParams->pEncodingString[k] = cOctal;
							pParams->pUtf8String[k] = pParams->pCurrentEncodingArray[cOctal];
							pParams->lexeme[k++] = cOctal;
						}
						state = S6;
					}
				}
				break;
			case S11:
				if ( c == '<' )
				{
					pParams->myToken.Type = T_DICT_BEGIN;
					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
					wprintf(L"GetNextTokenFromToUnicodeStream: T_DICT_BEGIN -> <<\n");
					#endif
					return;
				}
				else if ( (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') )
				{
					pParams->lexeme[k++] = c;
				}
				else if ( c == ' ' || c == '\t' || c == '\f' || c == '\0' || c == '\n' || c == '\r' || c == '\b' )
				{
					; // IGNORIAMO
				}
				else if ( c == '>' )
				{
					if ( k % 2 > 0 )
					{
						pParams->lexeme[k++] = '0';
					}

					y = 0;
					z = 0;
					for ( x = 0; x < k/2; x++ )
					{
						switch ( pParams->lexeme[y] )
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
								cHexadecimal = 0;
								break;
						}

						switch ( pParams->lexeme[y + 1] )
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
								cHexadecimal += 0;
								break;
						}

						if ( 0 != cHexadecimal )
						{
							pParams->pEncodingString[z] = cHexadecimal;
							pParams->pUtf8String[z] = pParams->pCurrentEncodingArray[cHexadecimal];
							z++;
						}

						y += 2;
					}

					pParams->pEncodingString[z] = L'\0';
					pParams->pUtf8String[z] = L'\0';
					pParams->lexeme[k] = '\0';
					pParams->myToken.Type = T_STRING_HEXADECIMAL;
					pParams->myToken.vString = pParams->lexeme;

					#if defined(MYDEBUG_PRINT_ALL) || defined(MYDEBUG_PRINT_ON_GetNextTokenFromToUnicodeStream_FN)
					wprintf(L"GetNextTokenFromToUnicodeStream: T_STRING_HEXADECIMAL -> '%s'\n", pParams->myToken.vString);
					#endif
					return;
				}
				else
				{
					pParams->pEncodingString[0] = L'\0';
					pParams->pUtf8String[0] = L'\0';

					pParams->myToken.Type = T_ERROR;
					snprintf(pParams->szError, 8192, "Errore GetNextTokenFromToUnicodeStream: numero esadecimale errato. Carattere non valido -> codice decimale = %u\n", (unsigned int)c);
					myShowErrorMessage(pParams, pParams->szError, 1);
					//wprintf(L"Errore GetNextTokenFromToUnicodeStream: numero esadecimale errato. Carattere non valido -> codice decimale = %u\n", (unsigned int)c);
					//fwprintf(pParams->fpErrors, L"Errore GetNextTokenFromToUnicodeStream: numero esadecimale errato. Carattere non valido -> codice decimale = %u\n", (unsigned int)c);

					c = ReadNextCharFromToUnicodeStream(pParams);

					return;
				}
				break;
			default:
				break;
		}

		c = ReadNextCharFromToUnicodeStream(pParams);
		if ( pParams->myToken.Type == T_EOF )
			return;

		if ( k > MAX_STRING_LENTGTH_IN_CONTENT_STREAM )
		{
			pParams->myToken.Type = T_ERROR;
			snprintf(pParams->szError, 8192, "Errore GetNextTokenFromToUnicodeStream: lexeme troppo lungo(> %d byte)\n", MAX_STRING_LENTGTH_IN_CONTENT_STREAM);
			myShowErrorMessage(pParams, pParams->szError, 1);
			//wprintf(L"Errore GetNextTokenFromToUnicodeStream: lexeme troppo lungo(> %d byte)\n", MAX_STRING_LENTGTH_IN_CONTENT_STREAM);
			//fwprintf(pParams->fpErrors, L"Errore GetNextTokenFromToUnicodeStream: lexeme troppo lungo(> %d byte)\n", MAX_STRING_LENTGTH_IN_CONTENT_STREAM);
			return;
		}
	}

	return;
}

void GetNextTokenFromToUnicodeStreamFast(Params *pParams)
{
    unsigned char *buf = pParams->myBlockToUnicode;
    int max_len = pParams->blockLenToUnicode;
    int pos = pParams->blockCurPosToUnicode;
    int k = 0;

    // Salta gli spazi bianchi iniziali direttamente in RAM
    while (pos < max_len && (buf[pos] == ' ' || buf[pos] == '\r' || buf[pos] == '\n' || buf[pos] == '\t' || buf[pos] == '\f' || buf[pos] == '\0')) {
        pos++;
    }

    if (pos >= max_len) {
        pParams->myToken.Type = T_EOF;
        pParams->blockCurPosToUnicode = pos;
        return;
    }

    unsigned char c = buf[pos++];

    // Identifica i numeri letterali istantaneamente
    if ((c >= '0' && c <= '9') || c == '-' || c == '+') {
        pParams->lexeme[k++] = c;
        while (pos < max_len && ((buf[pos] >= '0' && buf[pos] <= '9') || buf[pos] == '.')) {
            pParams->lexeme[k++] = buf[pos++];
        }
        pParams->lexeme[k] = '\0';
        if (strchr(pParams->lexeme, '.')) {
            pParams->myToken.Type = T_REAL_LITERAL;
            pParams->myToken.vDouble = atof(pParams->lexeme);
        } else {
            pParams->myToken.Type = T_INT_LITERAL;
            pParams->myToken.vInt = atoi(pParams->lexeme);
        }
        pParams->blockCurPosToUnicode = pos;
        return;
    }

    // Identifica le stringhe esadecimali dei font (<AAAA>) senza cicli di stato
    if (c == '<') {
        if (pos < max_len && buf[pos] == '<') {
            pParams->myToken.Type = T_DICT_BEGIN;
            pos++;
        } else {
            pParams->myToken.Type = T_STRING_HEXADECIMAL;
            while (pos < max_len && buf[pos] != '>') {
                if (buf[pos] != ' ' && buf[pos] != '\r' && buf[pos] != '\n' && buf[pos] != '\t') {
                    pParams->lexeme[k++] = buf[pos];
                }
                pos++;
            }
            if (pos < max_len && buf[pos] == '>') pos++;
            pParams->lexeme[k] = '\0';
            ManageTypeZeroHexString(pParams, k);
        }
        pParams->blockCurPosToUnicode = pos;
        return;
    }

    // Accumula i nomi delle macro dei font (es. begincodespacerange)
    pParams->lexeme[k++] = c;
    while (pos < max_len && buf[pos] != ' ' && buf[pos] != '\r' && buf[pos] != '\n' && buf[pos] != '<' && buf[pos] != '>' && buf[pos] != '[' && buf[pos] != ']') {
        pParams->lexeme[k++] = buf[pos++];
    }
    pParams->lexeme[k] = '\0';

    if (strcmp(pParams->lexeme, "begincodespacerange") == 0) pParams->myToken.Type = T_CONTENT_OP_begincodespacerange;
    else if (strcmp(pParams->lexeme, "endcodespacerange") == 0) pParams->myToken.Type = T_CONTENT_OP_endcodespacerange;
    else if (strcmp(pParams->lexeme, "beginbfchar") == 0) pParams->myToken.Type = T_CONTENT_OP_beginbfchar;
    else if (strcmp(pParams->lexeme, "endbfchar") == 0) pParams->myToken.Type = T_CONTENT_OP_endbfchar;
    else if (strcmp(pParams->lexeme, "beginbfrange") == 0) pParams->myToken.Type = T_CONTENT_OP_beginbfrange;
    else if (strcmp(pParams->lexeme, "endbfrange") == 0) pParams->myToken.Type = T_CONTENT_OP_endbfrange;
    else if (strcmp(pParams->lexeme, "begincidchar") == 0) pParams->myToken.Type = T_CONTENT_OP_begincidchar;
    else if (strcmp(pParams->lexeme, "endcidchar") == 0) pParams->myToken.Type = T_CONTENT_OP_endcidchar;
    else if (strcmp(pParams->lexeme, "begincidrange") == 0) pParams->myToken.Type = T_CONTENT_OP_begincidrange;
    else if (strcmp(pParams->lexeme, "endcidrange") == 0) pParams->myToken.Type = T_CONTENT_OP_endcidrange;
    else pParams->myToken.Type = T_STRING;

    pParams->blockCurPosToUnicode = pos;
}
