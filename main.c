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

/*
../configure --prefix=./mybuild

mke

make install

make clean
*/

#define _FILE_OFFSET_BITS 64

#if !defined(_WIN64) && !defined(_WIN32)
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <locale.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#if !defined(_WIN64) && !defined(_WIN32)
#include <termios.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sched.h>
#endif

#include <pthread.h>

#include <wchar.h>

#if defined(_WIN64) || defined(_WIN32)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

#include "mypdfsearch.h"
#include "mytime.h"
#include "parser.h"
#include "myUnicodeNFC.h"

//#if !defined(_WIN64) && !defined(_WIN32)
//#include <sys/sysinfo.h>
//#endif

// Contatore globale per i thread attivi nel pool di elaborazione PDF
int globalActiveWorkerThreads = 0;
// Se usi i thread in parallelo, proteggi l'incremento/decremento con un mutex esistente
// oppure usa le funzioni atomiche integrate di GCC/Clang:
// __sync_fetch_and_add(&globalActiveWorkerThreads, 1);
// __sync_fetch_and_sub(&globalActiveWorkerThreads, 1);

// Specifichiamo che la variabile è definita globalmente
extern int globalActiveWorkerThreads;

// Nota: Ricordarsi di aggiungere __sync_fetch_and_add(&globalActiveWorkerThreads, 1);
// all'inizio della funzione che viene eseguita dai thread pthread,
// e __sync_fetch_and_sub(&globalActiveWorkerThreads, 1);
// alla fine della stessa (subito prima del return o di pthread_exit).

long GetProcessors(void);
void PrintHelpCommandLine(void);
void PrintVersionInfo(void);
void printError(int numError);

FilesList* addFileToFilesList(FilesList *first, FilesList *newFile);
void freeFilesList(FilesList *first);

FilesList* addFileToFilesList(FilesList *first, FilesList *newFile)
{
	FilesList *n;

	n = (FilesList*)malloc(sizeof(FilesList));

	if( n == NULL )
		return NULL;

	#if !defined(_WIN64) && !defined(_WIN32)
	strcpy(n->myPathName, newFile->myPathName);
	#endif
	strcpy(n->myFileName, newFile->myFileName);

	//wprintf(L"ECCO: <%s/%s>\n", n->myPathName, n->myFileName);

	n->next = NULL;

	if ( first != NULL )
		n->next = first;

	return n;
}

void freeFilesList(FilesList *first)
{
	FilesList *n1 = first, *n2;
	while ( n1 != NULL )
	{
		n2 = n1->next;
		free(n1);
		n1 = n2;
	}
}

#if defined(_WIN64) || defined(_WIN32)
void GetErrorString(DWORD err, char *szError, int len)
{
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;

	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(char*)&lpMsgBuf,
		0, NULL);

	strncpy(szError, lpMsgBuf, len);

	LocalFree(lpMsgBuf);
}

FilesList* getFilesRecursive(char *dirName, int lenOrig, int bNoSubDirs, FilesList* myFilesList)
{
	char pathName[MAX_PATH + 1];

	WIN32_FIND_DATAA data;

	char fname[_MAX_DIR];

	FilesList myFile;

	strncpy(fname, dirName, _MAX_DIR - 1);
	strncat(fname, "\\*.*", _MAX_DIR - 1);

	HANDLE h = FindFirstFileA(fname, &data);
	if (h != INVALID_HANDLE_VALUE)
	{
		do {

			strncpy(pathName, dirName, MAX_PATH);
			strncat(pathName, "\\", MAX_PATH);
			strncat(pathName, data.cFileName, MAX_PATH);

			if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				//if (!(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) && !(data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
				if ( !(data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) )
				{
					if (strncmp(data.cFileName, ".", _MAX_DIR) != 0 && strncmp(data.cFileName, "..", _MAX_DIR) != 0)
					{
						if ( !bNoSubDirs )
							myFilesList = getFilesRecursive(pathName, lenOrig, bNoSubDirs, myFilesList);
					}
				}
			}
			else
			{
				int len = strnlen(pathName, MAX_LEN_STR);
				if ( len > 4 ) // .pdf = 4
				{
					if ( tolower(pathName[len - 1]) == 'f' && tolower(pathName[len - 2]) == 'd' && tolower(pathName[len - 3]) == 'p' && pathName[len - 4] == '.' )
					{
						strncpy(myFile.myFileName, pathName, len + 1);
						myFilesList = addFileToFilesList(myFilesList, &myFile);
						if ( !myFilesList )
							goto uscita;
					}
				}

			}
		} while (FindNextFileA(h, &data) != 0);
		FindClose(h);
	}

	uscita:

	return myFilesList;
}
#else // Linux
FilesList* getFilesRecursive(char *dirName, int lenOrig, int bNoSubDirs, FilesList* myFilesList)
{
	FilesList myFile;
	DIR *dir = NULL;
	struct dirent *entry;
	char pathName[PATH_MAX + 1];

	int i;
	int len;
	int len2;
	int index;
	char *szExt;

	dir = opendir(dirName);
	if( dir == NULL )
	{
		wprintf(L"Errore nell'apertura della directory '%s': %s\n", dirName, strerror(errno));
		return myFilesList;
	}

	myFile.myPathName[0] = '\0';
	myFile.myFileName[0] = '\0';

	entry = readdir(dir);
	while( entry != NULL )
	{
		struct stat entryInfo;

		if( ( strncmp(entry->d_name, ".", PATH_MAX) == 0 ) ||
		    ( strncmp(entry->d_name, "..", PATH_MAX) == 0 ) )
		{
			entry = readdir(dir);
		    continue;
		}

		strncpy(pathName, dirName, PATH_MAX);
		strncat(pathName, "/", PATH_MAX);
		strncat(pathName, entry->d_name, PATH_MAX);

		if( lstat(pathName, &entryInfo) == 0 )
		{
			if( S_ISDIR(entryInfo.st_mode) )		// directory
			{
				if ( !bNoSubDirs )
					myFilesList = getFilesRecursive(pathName, lenOrig, bNoSubDirs, myFilesList);
			}
			else if( S_ISREG(entryInfo.st_mode) )	// regular file
			{
				len = strlen(entry->d_name);

				index = 0;
				for ( i = len - 1; i >= 0; i-- )
				{

					if ( entry->d_name[i] == '.' )
					{
						index = i;
						break;
					}
				}

				if ( index > 0 )
				{
					szExt = &entry->d_name[index];
					if ( strcmp(".pdf", szExt) == 0 )
					{
						len2 = strnlen(pathName, PATH_MAX);
						strncpy(myFile.myPathName, pathName, len2 + 1);
						index = strlen(pathName) - len - 1;
						myFile.myPathName[index] = '\0';

						strncpy(myFile.myFileName, entry->d_name, PATH_MAX);

						myFilesList = addFileToFilesList(myFilesList, &myFile);
						if ( !myFilesList )
							return NULL;
					}
				}
			}
		}
		else
		{
			wprintf(L"Errore lstat %s: %s\n", pathName, strerror(errno));
		}

		entry = readdir(dir);
	}

	closedir(dir);

	return myFilesList;
}
#endif

/* -------------------------------------------------------------------------------------------------- */

/*
Per generare le dipendenze per il Makefile, usare -MM:

[vincenzo]$ gcc -MM -D_GNU_SOURCE myoctal.c mydictionaryqueuelist.c mystringqueuelist.c mycontentqueuelist.c mynumstacklist.c myintqueuelist.c mydecode.c scanner.c parser.c main.c -lz

myoctal.o: myoctal.c myoctal.h mypdfsearch.h

mydictionaryqueuelist.o: mydictionaryqueuelist.c scanner.h mypdfsearch.h \
 myobjrefqueuelist.h mynumstacklist.h myintqueuelist.h \
 mystringqueuelist.h mydictionaryqueuelist.h mycontentqueuelist.h \
 myScopeHashTable.h myTernarySearchTree.h mydecode.h

mystringqueuelist.o: mystringqueuelist.c mystringqueuelist.h

mycontentqueuelist.o: mycontentqueuelist.c mycontentqueuelist.h \
 mystringqueuelist.h mydictionaryqueuelist.h

mynumstacklist.o: mynumstacklist.c mynumstacklist.h

myintqueuelist.o: myintqueuelist.c myintqueuelist.h

mydecode.o: mydecode.c mydecode.h myoctal.h mypdfsearch.h
scanner.o: scanner.c myoctal.h mypdfsearch.h scanner.h \
 myobjrefqueuelist.h mynumstacklist.h myintqueuelist.h \
 mystringqueuelist.h mydictionaryqueuelist.h mycontentqueuelist.h \
 myScopeHashTable.h myTernarySearchTree.h mydecode.h

parser.o: parser.c parser.h mypdfsearch.h scanner.h myobjrefqueuelist.h \
 mynumstacklist.h myintqueuelist.h mystringqueuelist.h \
 mydictionaryqueuelist.h mycontentqueuelist.h myScopeHashTable.h \
 myTernarySearchTree.h mydecode.h

main.o: main.c mypdfsearch.h parser.h scanner.h myobjrefqueuelist.h \
 mynumstacklist.h myintqueuelist.h mystringqueuelist.h \
 mydictionaryqueuelist.h mycontentqueuelist.h myScopeHashTable.h \
 myTernarySearchTree.h mydecode.h
*/

/* -------------------------------------------------------------------------------------------------- */

long GetProcessors(void)
{
	//long nProcs = -1;
	long nProcs_max = -1;

#if defined(_WIN64) || defined(_WIN32)
	SYSTEM_INFO info;
	//GetSystemInfo(&info);
	GetNativeSystemInfo(&info);
	nProcs_max = info.dwNumberOfProcessors;
#else
	//nProcs_max = sysconf(_SC_NPROCESSORS_ONLN);
	nProcs_max = sysconf(_SC_NPROCESSORS_CONF);
#endif

	return nProcs_max;
}

wchar_t* myMultibyteToWidechar(const char *psz, size_t mbslen)
{
	wchar_t *wcs = NULL;

	wcs = calloc(mbslen + 1, sizeof(wchar_t));
	if (wcs == NULL)
	{
		//perror("calloc");
		wprintf(L"ERRORE myMultibyteToWidechar calloc: %s \n", strerror(errno));

		return NULL;
	}

	if (mbstowcs(wcs, (char*)psz, mbslen + 1) == (size_t) -1)
	{
		//perror("mbstowcs");
		wprintf(L"ERRORE myMultibyteToWidechar mbstowcs: %s \n", strerror(errno));

		free(wcs);
		wcs = NULL;
		return NULL;
	}

	return wcs;
}

void PrintHelpCommandLine(void)
{
	wprintf(L"\nUsage:\n");
	wprintf(L"./mypdfsearch Options\n");
	wprintf(L"Options:\n");
	wprintf(L"At least one of:\n\n");
	wprintf(L"'-h, --help' (optional)              Print this message and exit\n");
	wprintf(L"\n");
	wprintf(L"'-v, --version' (optional)           Print version info and exit\n");
	wprintf(L"\n");
	wprintf(L"'-e, --extracttextfrom' (optional)   Extract text from specified file and exit\n");
	wprintf(L"   Arguments for option '-e, --extracttextfrom': exactly 1(string)\n");
	wprintf(L"\n");
	wprintf(L"'-f, --frompage' (optional)          Extract text from pagenum specified\n");
	wprintf(L"   Arguments for option '-f, --frompage': exactly 1(integer)\n");
	wprintf(L"\n");
	wprintf(L"'-t, --topage' (optional)            Extract text to pagenum specified\n");
	wprintf(L"   Arguments for option '-t, --topage': exactly 1(integer)\n");
	wprintf(L"\n");
	wprintf(L"'-p, --path' (optional)              Specifies the input file\n");
	wprintf(L"   Arguments for option '-p, --path': exactly 1(string)\n");
	wprintf(L"\n");
	wprintf(L"'-n, --nosubdirs' (optional)         Specifies that the search should be performed only in the specified directory and not in its subdirectories\n");
	wprintf(L"\n");
	wprintf(L"'-s, --sortglyphsbyrow' (optional)   Select the text based on the row order of the glyphs. If this option is not specified, the text is selected based on the order of appearance.\n");
	wprintf(L"\n");
	wprintf(L"'-w, --words' (optional)             Specifies words to search\n");
	wprintf(L"   Arguments for option '-w, --words': exactly 1(string) containing one on more words to search, separated by space character\n");
	wprintf(L"\n");
	wprintf(L"'-o, --outputfile' (optional)        Specifies the output file\n");
	wprintf(L"   Arguments for option '-o, --outputfile': exactly 1(string)\n");
	wprintf(L"\n");
}

void PrintVersionInfo(void)
{
	//wprintf(L"\n   mypdfsearch version 3.0.0\n");
	wprintf(L"\n   mypdfsearch version 0.1.0 beta\n");

	wprintf(L"\n   Copyright (C) 2026 Vincenzo Lo Cicero\n\n");

	wprintf(L"   Author: Vincenzo Lo Cicero.\n");
	wprintf(L"   e-mail: vincenzolocicero68@gmail.com\n\n");

	wprintf(L"   mypdfsearch is free software: you can redistribute it and/or modify\n");
	wprintf(L"   it under the terms of the GNU General Public License as published by\n");
	wprintf(L"   the Free Software Foundation, either version 3 of the License, or\n");
	wprintf(L"   (at your option) any later version.\n\n");

	wprintf(L"   mypdfsearch is distributed in the hope that it will be useful,\n");
	wprintf(L"   but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
	wprintf(L"   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
	wprintf(L"   GNU General Public License for more details.\n\n");

	wprintf(L"   You should have received a copy of the GNU General Public License\n");
	wprintf(L"   along with mypdfsearch.  If not, see <http://www.gnu.org/licenses/>.\n");

	wprintf(L"\n");
}

// Riconosce i separatori di parola per la ricerca.
// Include spazio, punteggiatura latina e punteggiatura Unicode di uso comune
// nelle lingue che gestiamo (danda Devanagari/Bengalese, ideographic full stop,
// virgola araba, ecc.).
static int IsWordSeparator(wchar_t c)
{
    if (c == L' ' || c == L'\t' || c == L'\n' || c == L'\r' || c == L'\f' || c == L'\v') return 1;
    // Punteggiatura latina
    if (c == L'.' || c == L',' || c == L';' || c == L':' || c == L'!' || c == L'?') return 1;
    if (c == L'"' || c == L'\'' || c == L'(' || c == L')' || c == L'[' || c == L']') return 1;
    if (c == L'{' || c == L'}' || c == L'-' || c == L'_' || c == L'/' || c == L'\\') return 1;
    // Punteggiatura Unicode delle lingue gestite
    if (c == 0x0964 || c == 0x0965) return 1;  // Devanagari danda, double danda
    if (c == 0x3002) return 1;                 // Ideographic full stop
    if (c == 0xFF01 || c == 0xFF1F) return 1;  // Fullwidth exclamation / question
    if (c == 0xFF0C) return 1;                 // Fullwidth comma
    if (c == 0x060C) return 1;                 // Arabic comma
    if (c == 0x061B) return 1;                 // Arabic semicolon
    if (c == 0x061F) return 1;                 // Arabic question mark
    if (c == 0x3001) return 1;                 // Ideographic comma
    if (c == 0x00AB || c == 0x00BB) return 1;  // Quotes
    if (c == 0x2018 || c == 0x2019) return 1;  // Single quotes
    if (c == 0x201C || c == 0x201D) return 1;  // Double quotes
    if (c == 0x2026) return 1;                 // Ellipsis
    if (c == 0x2013 || c == 0x2014) return 1;  // Dashes
    if (c == 0xFF1A) return 1;                 // Fullwidth colon
    if (c == 0xFF1B) return 1;                 // Fullwidth semicolon
    if (c == 0x300C || c == 0x300D) return 1;  // CJK brackets
    if (c == 0x300E || c == 0x300F) return 1;  // CJK white brackets
    if (c == 0xFF08 || c == 0xFF09) return 1;  // Fullwidth parentheses
    if (c == 0xFF3B || c == 0xFF3D) return 1;  // Fullwidth square brackets

    return 0;
}

#ifndef IS_ASIAN_SCRIPT
#define IS_ASIAN_SCRIPT(c) ( \
    ((c) >= 0x3040 && (c) <= 0x309F) || /* Hiragana */ \
    ((c) >= 0x30A0 && (c) <= 0x30FF) || /* Katakana */ \
    ((c) >= 0x3400 && (c) <= 0x4DBF) || /* CJK Extension A */ \
    ((c) >= 0x4E00 && (c) <= 0x9FFF) || /* CJK Unified Ideographs */ \
    ((c) >= 0xAC00 && (c) <= 0xD7AF) || /* Hangul Syllables */ \
    ((c) >= 0x20000 && (c) <= 0x2FA1F) || /* CJK Extension B-F */ \
    ((c) >= 0x0E00 && (c) <= 0x0E7F)    /* Thai */ \
)
#endif


int mySplitWords(Params *pParams, wchar_t *pwszOption)
{
    int retValue = 1;

    size_t x = 0;
    size_t y = 0;

    size_t queryLength = 0;
    size_t normalizedLength = 0;

    wchar_t c;
    wchar_t wszTemp[MAX_LEN_STR + 1];
    wchar_t cPrevAsian = L'\0';

    wchar_t *pwszNormalizedOption = NULL;

    MyUnicodeNFCStatus nfcStatus;


    /*
     * Controllo degli argomenti.
     */

    if ( NULL == pParams || NULL == pwszOption )
    {
        fprintf(
            stderr,
            "ERRORE mySplitWords: argomento NULL.\n"
        );

        return 0;
    }


    /*
     * ============================================================
     * PASSAGGIO 1:
     * Normalizzazione NFC della stringa da cercare.
     *
     * pwszOption rimane invariata.
     * ============================================================
     */

    queryLength = wcslen(pwszOption);

    nfcStatus = myUnicodeNFC_Normalize(
        pwszOption,
        queryLength,
        &pwszNormalizedOption,
        &normalizedLength
    );

    if ( MY_UNICODE_NFC_OK != nfcStatus )
    {
        fprintf(
            stderr,
            "ERRORE mySplitWords: normalizzazione NFC fallita: %s.\n",
            myUnicodeNFC_StatusString(nfcStatus)
        );

        retValue = 0;
        goto uscita;
    }


    /*
     * La dimensione di wszTemp è MAX_LEN_STR + 1.
     *
     * NFC può, in alcuni casi particolari, aumentare il numero
     * di code point. Evitiamo quindi di superare la capacità
     * del buffer utilizzato per accumulare le parole.
     */

    if ( normalizedLength > MAX_LEN_STR )
    {
        fprintf(
            stderr,
            "ERRORE mySplitWords: la query normalizzata NFC supera MAX_LEN_STR (%d caratteri).\n",
            MAX_LEN_STR
        );

        retValue = 0;
        goto uscita;
    }


    /*
     * ============================================================
     * PASSAGGIO 2:
     * Suddivisione della query normalizzata in parole e bigrammi.
     *
     * La logica di ricerca rimane quella originale.
     * ============================================================
     */

    while ( 1 )
    {
        c = pwszNormalizedOption[x];

        if ( L'\0' != c && L' ' != c && !IsWordSeparator(c) )
        {
            wchar_t cLower = towlower(c);

            if ( IS_ASIAN_SCRIPT(cLower) )
            {
                /*
                 * Se avevamo accumulato caratteri occidentali
                 * prima, salviamoli subito.
                 */

                if ( y > 0 && cPrevAsian == L'\0' )
                {
                    wszTemp[y] = L'\0';

                    pParams->pWordsToSearchArray[pParams->countWordsToSearch] =
                        (wchar_t*)malloc(
                            sizeof(wchar_t) * y + sizeof(wchar_t)
                        );

                    if (
                        NULL ==
                        pParams->pWordsToSearchArray[pParams->countWordsToSearch]
                    )
                    {
                        retValue = 0;
                        goto uscita;
                    }

                    wcsncpy(
                        pParams->pWordsToSearchArray[pParams->countWordsToSearch],
                        wszTemp,
                        y
                    );

                    pParams->pWordsToSearchArray[pParams->countWordsToSearch][y] =
                        L'\0';

                    pParams->countWordsToSearch++;

                    y = 0;
                }


                if ( cPrevAsian == L'\0' )
                {
                    cPrevAsian = cLower;
                }
                else
                {
                    /*
                     * Creiamo il bigramma da cercare.
                     * Esempio: due caratteri CJK consecutivi.
                     */

                    wszTemp[0] = cPrevAsian;
                    wszTemp[1] = cLower;
                    wszTemp[2] = L'\0';

                    pParams->pWordsToSearchArray[pParams->countWordsToSearch] =
                        (wchar_t*)malloc(
                            sizeof(wchar_t) * 2 + sizeof(wchar_t)
                        );

                    if (
                        NULL ==
                        pParams->pWordsToSearchArray[pParams->countWordsToSearch]
                    )
                    {
                        retValue = 0;
                        goto uscita;
                    }

                    wcsncpy(
                        pParams->pWordsToSearchArray[pParams->countWordsToSearch],
                        wszTemp,
                        2
                    );

                    pParams->pWordsToSearchArray[pParams->countWordsToSearch][2] =
                        L'\0';

                    pParams->countWordsToSearch++;

                    /*
                     * Facciamo scivolare il carattere per
                     * la ricerca continua.
                     */

                    cPrevAsian = cLower;
                }
            }
            else
            {
                /*
                 * Carattere occidentale/standard:
                 * se prima eravamo su uno script asiatico,
                 * resettiamo il flag.
                 */

                if ( cPrevAsian != L'\0' )
                {
                    cPrevAsian = L'\0';
                    y = 0;
                }

                wszTemp[y++] = cLower;
            }
        }
        else
        {
            /*
             * Abbiamo incontrato uno spazio, un separatore
             * oppure la fine della stringa.
             */

            if ( y > 0 && cPrevAsian == L'\0' )
            {
                wszTemp[y] = L'\0';

                pParams->pWordsToSearchArray[pParams->countWordsToSearch] =
                    (wchar_t*)malloc(
                        sizeof(wchar_t) * y + sizeof(wchar_t)
                    );

                if (
                    NULL ==
                    pParams->pWordsToSearchArray[pParams->countWordsToSearch]
                )
                {
                    retValue = 0;
                    goto uscita;
                }

                wcsncpy(
                    pParams->pWordsToSearchArray[pParams->countWordsToSearch],
                    wszTemp,
                    y
                );

                pParams->pWordsToSearchArray[pParams->countWordsToSearch][y] =
                    L'\0';

                pParams->countWordsToSearch++;
            }

            /*
             * Reset dello stato asiatico alla fine del blocco.
             */

            cPrevAsian = L'\0';
            y = 0;

            if ( L'\0' == c )
                goto uscita;
        }

        x++;
    }


uscita:

    /*
     * Rilascio del buffer temporaneo NFC.
     */

    if ( NULL != pwszNormalizedOption )
    {
        free(pwszNormalizedOption);
        pwszNormalizedOption = NULL;
    }

    return retValue;
}

/*
int mySplitWords(Params *pParams, wchar_t *pwszOption)
{
	int retValue = 1;

	unsigned long int x = 0;
	unsigned long int y = 0;
	wchar_t c;
	wchar_t wszTemp[MAX_LEN_STR + 1];
	wchar_t cPrevAsian = L'\0';

	while ( 1 )
	{
		c = pwszOption[x];

		if ( L'\0' != c && L' ' != c && !IsWordSeparator(c) )
		{
			wchar_t cLower = towlower(c);

			if ( IS_ASIAN_SCRIPT(cLower) )
			{
				// Se avevamo accumulato caratteri occidentali prima, salviamoli subito
				if ( y > 0 && cPrevAsian == L'\0' )
				{
					wszTemp[y] = L'\0';
					pParams->pWordsToSearchArray[pParams->countWordsToSearch] = (wchar_t*)malloc(sizeof(wchar_t) * y + sizeof(wchar_t));
					if ( NULL == pParams->pWordsToSearchArray[pParams->countWordsToSearch] ) { retValue = 0; goto uscita; }
					wcsncpy(pParams->pWordsToSearchArray[pParams->countWordsToSearch], wszTemp, y);
					pParams->pWordsToSearchArray[pParams->countWordsToSearch][y] = L'\0';
					pParams->countWordsToSearch++;
					y = 0;
				}

				if ( cPrevAsian == L'\0' )
				{
					cPrevAsian = cLower;
				}
				else
				{
					// Creiamo il bigramma da cercare (es. "ShiJie")
					wszTemp[0] = cPrevAsian;
					wszTemp[1] = cLower;
					wszTemp[2] = L'\0';

					pParams->pWordsToSearchArray[pParams->countWordsToSearch] = (wchar_t*)malloc(sizeof(wchar_t) * 2 + sizeof(wchar_t));
					if ( NULL == pParams->pWordsToSearchArray[pParams->countWordsToSearch] ) { retValue = 0; goto uscita; }
					wcsncpy(pParams->pWordsToSearchArray[pParams->countWordsToSearch], wszTemp, 2);
					pParams->pWordsToSearchArray[pParams->countWordsToSearch][2] = L'\0';
					pParams->countWordsToSearch++;

					// Facciamo scivolare per la ricerca continua
					cPrevAsian = cLower;
				}
			}
			else
			{
				// Carattere occidentale/standard: se prima eravamo su uno script asiatico, resettiamo il flag
				if ( cPrevAsian != L'\0' )
				{
					cPrevAsian = L'\0';
					y = 0;
				}
				wszTemp[y++] = cLower;
			}
		}
		else
		{
			// Abbiamo incontrato uno spazio, un separatore o fine stringa
			if ( y > 0 && cPrevAsian == L'\0' )
			{
				wszTemp[y] = L'\0';
				pParams->pWordsToSearchArray[pParams->countWordsToSearch] = (wchar_t*)malloc(sizeof(wchar_t) * y + sizeof(wchar_t));
				if ( NULL == pParams->pWordsToSearchArray[pParams->countWordsToSearch] ) { retValue = 0; goto uscita; }
				wcsncpy(pParams->pWordsToSearchArray[pParams->countWordsToSearch], wszTemp, y);
				pParams->pWordsToSearchArray[pParams->countWordsToSearch][y] = L'\0';
				pParams->countWordsToSearch++;
			}

			// Reset dello stato asiatico alla fine del blocco di testo
			cPrevAsian = L'\0';
			y = 0;

			if ( L'\0' == c )
				goto uscita;
		}
		x++;
	}

uscita:
	return retValue;
}
*/

int myParseOption(Params *pParams, char *pszOption)
{
	int retValue = 1;

	int bLongName = 0;
	int bEqualCharFound = 0;

	wchar_t c;
	int x;
	int y;

	int n;

	int nLen;

	wchar_t szName[MAX_LEN_STR + 1];

	wchar_t *pwszOption = NULL;

	szName[0] = '\0';

	x = 0;
	y = 0;
	n = 0;

	//wprintf(L"pszOption = <%s>\n", pszOption);

	n = strnlen(pszOption, MAX_LEN_STR);
	if ( n < 2 )
	{
		retValue = 0;
		wprintf(L"\n\nError: unrecognized option '%s'\n\n", pszOption);
		goto uscita;
	}

	//pwszOption = myConvertToUtf8WideCharString(pszOption, n, "UTF-8");
	pwszOption = myMultibyteToWidechar(pszOption, n);
	if ( NULL == pwszOption )
	{
		retValue = 0;
		wprintf(L"\n\nError myParseOption: myConvertToUtf8WideCharString return NULL for '%s'\n\n", pszOption);
		goto uscita;
	}

	n = 0;


	x = 0;
	c = pwszOption[x];
	if ( c != L'-' )
	{
		retValue = 0;
		wprintf(L"\n\nError: invalid option '%s'\n\n", pszOption);
		goto uscita;
	}

	x = 1;
	c = pwszOption[x];
	if ( c == L'-' )
	{
		bLongName = 1;
		x++;
		c = pwszOption[x];
		szName[0] = c;
		szName[1] = L'\0';
		y = 1;
		goto readlongname;
	}
	else if ( c == L'\0' )
	{
		retValue = 0;
		wprintf(L"\n\nError: invalid option '%s'\n\n", pszOption);
		goto uscita;
	}
	else
	{
		szName[0] = c;
		szName[1] = '\0';
		x++;

		if ( c == L'h' )
		{
			pParams->bOptVersionOrHelp = 1;
			PrintHelpCommandLine();
			goto uscita;
		}
		else if ( c == L'v' )
		{
			pParams->bOptVersionOrHelp = 1;
			PrintVersionInfo();
			goto uscita;
		}
		else if ( c == L'n' )
		{
			pParams->bNoSubDirs = 1;
			goto uscita;
		}
		else if ( c == L's' )
		{
			pParams->nSortGlyphs = SORT_GLYPHS_BY_ROW;
			goto uscita;
		}

		goto readvalue;
	}

	readlongname:

	if ( bLongName )
	{
		c = pwszOption[x];

		x++;
		c = pwszOption[x];
		while ( c != L'\0' )
		{
			szName[y] = c;
			y++;

			if ( y > MAX_LEN_STR )
			{
				retValue = 0;
				wprintf(L"Error: option string too long.\n");
				goto uscita;
			}

			if ( c == L'=' )
			{
				y--;
				break;
			}

			x++;
			c = pwszOption[x];
		}

		if ( y < 0 )
		{
			retValue = 0;
			wprintf(L"\n\nError: invalid option '%s'\n\n", pszOption);
			goto uscita;
		}

		szName[y] = L'\0';


		if ( wcsncmp(szName, L"path", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"words", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"nosubdirs", MAX_LEN_STR) == 0 )
		{
			pParams->bNoSubDirs = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"sortglyphsbyrow", MAX_LEN_STR) == 0 )
		{
			pParams->nSortGlyphs = SORT_GLYPHS_BY_ROW;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"extracttextfrom", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"frompage", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"topage", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"outputfile", MAX_LEN_STR) == 0 )
		{
			goto readvalue;
		}
		else if ( wcsncmp(szName, L"nosubdirs", MAX_LEN_STR) == 0 )
		{
			pParams->bNoSubDirs = 1;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"sortglyphsbyrow", MAX_LEN_STR) == 0 )
		{
			pParams->nSortGlyphs = SORT_GLYPHS_BY_ROW;
			goto uscita;
		}
		else if ( wcsncmp(szName, L"version", MAX_LEN_STR) == 0 )
		{
			pParams->bOptVersionOrHelp = 1;
			PrintVersionInfo();
			goto uscita;
		}
		else if ( wcsncmp(szName, L"help", MAX_LEN_STR) == 0 )
		{
			pParams->bOptVersionOrHelp = 1;
			PrintHelpCommandLine();
			goto uscita;
		}
		else
		{
			retValue = 0;
			wprintf(L"\n\nError: unrecognized option '%s'\n\n", pszOption);
			goto uscita;
		}
	}

	readvalue:

	c = szName[0];
	y = 0;
	switch ( c )
	{
		case L'p':   // path
			c = pwszOption[x];
			while ( c != L'\0' )
			{
				nLen = wctomb(&(pParams->szPath[y]), c);
				y += nLen;
				//pParams->szPath[y] = (char)c;
				//y++;
				if ( c == L'=' )
				{
					y = 0;
					pParams->szPath[0] = '\0';
				}
				x++;
				c = pwszOption[x];
			}
			pParams->szPath[y] = '\0';
			break;
		case L'w':   // words
			c = pwszOption[x];
			while ( c != L'\0' )
			{
				pParams->szWordsToSearch[y] = c;
				y++;
				if ( c == L'=' )
				{
					y = 0;
					pParams->szWordsToSearch[0] = L'\0';
				}
				x++;
				c = pwszOption[x];
			}
			pParams->szWordsToSearch[y] = '\0';

			mySplitWords(pParams, pParams->szWordsToSearch);

			break;
		case L'e':   // extracttextfrom
			c = pwszOption[x];
			while ( c != '\0' )
			{
				nLen = wctomb(&(pParams->szFilePdf[y]), c);
				y += nLen;
				//pParams->szFilePdf[y] = (char)c;
				//y++;
				if ( c == L'=' )
				{
					y = 0;
					pParams->szFilePdf[0] = '\0';
				}
				x++;
				c = pwszOption[x];
			}
			pParams->szFilePdf[y] = '\0';
			break;
		case 'f':   // frompage
			n = 0;
			c = pwszOption[x];
			while ( c != L'\0' )
			{
				if ( c >= L'0' && c <= L'9' )
				{
					n = n * 10 + c - L'0';
					x++;
					c = pwszOption[x];
				}
				else if ( c == L'=' )
				{
					bEqualCharFound = 1;
					x++;
					c = pwszOption[x];
				}
				else
				{
					if ( bEqualCharFound )
					{
						retValue = 0;
						wprintf(L"\n\nError: invalid option value -> '%s'. Must be a integer number\n\n", pszOption);
						goto uscita;
					}

					x++;
					c = pwszOption[x];
				}
			}
			pParams->fromPage = n;
			break;
		case L't':   // topage
			n = 0;
			c = pwszOption[x];
			while ( c != L'\0' )
			{
				if ( c >= L'0' && c <= L'9' )
				{
					n = n * 10 + c - L'0';
					x++;
					c = pwszOption[x];
				}
				else if ( c == L'=' )
				{
					bEqualCharFound = 1;
					x++;
					c = pwszOption[x];
				}
				else
				{
					if ( bEqualCharFound )
					{
						retValue = 0;
						wprintf(L"\n\nError: invalid option value -> '%s'. Must be a integer number\n\n", pszOption);
						goto uscita;
					}

					x++;
					c = pszOption[x];
				}
			}
			pParams->toPage = n;
			break;
		case L'o':   // outputfile
			c = pwszOption[x];
			while ( c != L'\0' )
			{
				nLen = wctomb(&(pParams->szOutputFile[y]), c);
				y += nLen;
				//pParams->szOutputFile[y] = (char)c;
				//y++;
				if ( c == L'=' )
				{
					y = 0;
					pParams->szOutputFile[0] = '\0';
				}
				x++;
				c = pwszOption[x];
			}
			pParams->szOutputFile[y] = '\0';
			break;
		default:
			retValue = 0;
			wprintf(L"\n\nError: unrecognized option '%s'\n\n", pszOption);
			goto uscita;
			break;
	}

	uscita:

	if ( NULL != pwszOption )
	{
		free(pwszOption);
		pwszOption = NULL;
	}

	return retValue;
}

int myParseCommandLine(Params *pParams, int argc, char **argv)
{
	int retValue = 1;

	int x;
	int n;

	pParams->bOptVersionOrHelp = 0;
	pParams->szPath[0] = '\0';
	pParams->szFilePdf[0] = '\0';
	pParams->fromPage = 0;
	pParams->toPage = 0;
	pParams->szOutputFile[0] = '\0';
	pParams->szWordsToSearch[0] = '\0';
	pParams->countWordsToSearch = 0;

	pParams->pWordsToSearchArray = NULL;


	pParams->pWordsToSearchArray = (wchar_t**)malloc(sizeof(wchar_t*) * MAX_WORDS_TO_SEARCH);
	if ( NULL == pParams->pWordsToSearchArray )
	{
		retValue = 0;
		wprintf(L"\n\nError myParseOption: memory allocation failed for pWordsToSearchArray\n\n");
		goto uscita;
	}
	for ( n = 0; n < MAX_WORDS_TO_SEARCH; n++ )
	{
		pParams->pWordsToSearchArray[n] = NULL;
	}

	x = 1;
	if ( argc > 1 )
	{
		while ( x < argc )
		{
			retValue = myParseOption(pParams, argv[x]);
			if ( !retValue )
				goto uscita;
			x++;
		}
	}
	else
	{
		retValue = 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->szFilePdf[0] == '\0' && pParams->szPath[0] == '\0') )
	{
		wprintf(L"\n\nErrore: dev'essere specificata una(e solo una, non entrambe) delle due opzioni: '-p (--path)' o '-e(--extracttextfrom)'\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->szFilePdf[0] != '\0' && pParams->szPath[0] != '\0') )
	{
		wprintf(L"\n\nErrore: dev'essere specificata soltanto una delle due opzioni mutualmente esclusive: '-p (--path)' o '-e(--extracttextfrom)'\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->toPage > 0 && pParams->toPage < pParams->fromPage) )
	{
		wprintf(L"\n\nErrore: il valore specificato per l'opzione topage e' minore del valore specificato per l'opzione frompage. Dev'essere maggiore o uguale.\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->szPath[0] != '\0' && pParams->szWordsToSearch[0] == '\0') )
	{
		wprintf(L"\n\nErrore: bisogna specificare le parole da cercare tramite l'opzione '-w (--words)'\n\n");
		return 0;
	}

	if ( !(pParams->bOptVersionOrHelp) && (pParams->szPath[0] == '\0' && pParams->szWordsToSearch[0] != '\0') )
	{
		wprintf(L"\n\nErrore: bisogna specificare la cartella contenente i file pdf tramite l'opzione '-p (--path)'\n\n");
		return 0;
	}

uscita:

	return retValue;
}

int MakeAndOpenOutputFile(Params *pParams)
{
	int retValue = 1;

	#if !defined(_WIN64) && !defined(_WIN32)
	unsigned char szBOM[21];
	// UTF-8 BOM -> EF BB BF
	szBOM[0] = 0xEF;
	szBOM[1] = 0xBB;
	szBOM[2] = 0xBF;
	szBOM[3] = '\0';
	#endif

	pParams->fpOutput = fopen(pParams->szOutputFile, "rb");
	if ( pParams->fpOutput != NULL )
	{
		wprintf(L"\n\nERRORE: il file specificato per l'output esiste già.\nBisogna indicare il nome di un file non esistente.\n\n");
		retValue = 0;
		goto uscita;
	}

	pParams->fpOutput = fopen(pParams->szOutputFile, "wb");
	if ( pParams->fpOutput == NULL )
	{
		wprintf(L"\n\nERRORE: impossibile creara il file specificato per l'output: '%s'.\n\n", pParams->szOutputFile);
		retValue = 0;
		goto uscita;
	}

	#if defined(_WIN64) || defined(_WIN32)
	_setmode(_fileno(pParams->fpOutput), _O_U8TEXT);
	#endif

	#if !defined(_WIN64) && !defined(_WIN32)
	// UTF-8 BOM -> EF BB BF
	fwprintf(pParams->fpOutput, L"%s", szBOM);
	#endif

	uscita:

	return retValue;
}

void checkEndianness(Params *pParams)
{
	//int i = 0xDEED1234;
	//int i = 0xABCDEF12;
	uint32_t i = 0xABCDEF12;
	//int j;
	unsigned char *buff;

	//wprintf(L"\nvalue = %lX\n", i);

	buff = (unsigned char*)&i;

	pParams->nThisMachineEndianness = MACHINE_ENDIANNESS_UNKNOWN;

	if( buff[0] == 0xAB )
	{
		//wprintf(L"This machine is BIG endian - HIGHER order bytes come first");
		pParams->nThisMachineEndianness = MACHINE_ENDIANNESS_BIG_ENDIAN;
	}
	else
	{
		pParams->nThisMachineEndianness = MACHINE_ENDIANNESS_LITTLE_ENDIAN;
		//wprintf(L"This machine is LITTLE endian - LOWER order bytes come first");
	}

	//wprintf(L"\nhere are the 4 bytes\n");

	//for( j = 0; j < 4; j++ )
	//{
	//	wprintf(L"byte [%d] = %X\n", j, buff[j]);
	//}

	//wprintf(L"\n");
}

// Legge la password dal terminale evitando, su Linux e sistemi POSIX,
// di mostrarla sullo schermo durante la digitazione.
static int ReadPasswordFromTerminal(char *pszPassword, size_t capacity, size_t *pPasswordLength)
{
    if (pszPassword == NULL ||
        capacity < 2 ||
        pPasswordLength == NULL)
        return 0;

#if !defined(_WIN64) && !defined(_WIN32)
    {
        struct termios oldSettings;
        struct termios hiddenSettings;
        int bEchoDisabled = 0;

        if (isatty(STDIN_FILENO) &&
            tcgetattr(STDIN_FILENO, &oldSettings) == 0)
        {
            hiddenSettings = oldSettings;
            hiddenSettings.c_lflag &= (tcflag_t)~ECHO;

            if (tcsetattr(
                    STDIN_FILENO,
                    TCSAFLUSH,
                    &hiddenSettings) == 0)
            {
                bEchoDisabled = 1;
            }
        }

        if (fgets(pszPassword, (int)capacity, stdin) == NULL)
        {
            if (bEchoDisabled)
                tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldSettings);

            return 0;
        }

        if (bEchoDisabled)
        {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldSettings);
            wprintf(L"\n");
            fflush(stdout);
        }
    }
#else
    // Su Windows usiamo per ora l'input standard.
    if (fgets(pszPassword, (int)capacity, stdin) == NULL)
        return 0;
#endif

    // Rimuove CR/LF senza usare funzioni stringa per i dati binari.
    *pPasswordLength = strcspn(pszPassword, "\r\n");
    pszPassword[*pPasswordLength] = '\0';

    return 1;
}

// Dichiarazione (se la funzione ReadPasswordFromTerminal è in un altro file,
// basta includere il suo header. Altrimenti incolla qui la sua implementazione).
// Nota: se era static in parser.c, ora deve essere visibile in main.c.

static int MyConsolePasswordCallback(const char *pdf_filename,
                                     const char *error_msg,
                                     char *out_password,
                                     size_t max_len,
                                     size_t *out_len)
{
    // Mostra l'eventuale errore del tentativo precedente
    if (error_msg != NULL)
    {
        fprintf(stderr, "%s\n", error_msg);
    }

    // Mostra il prompt su stderr (non bufferizzato)
    fprintf(stderr, "Inserisci la password utente per '%s' (invio a vuoto per annullare): ", pdf_filename);
    fflush(stderr);

    // Legge la password nascondendo l'input
    if (!ReadPasswordFromTerminal(out_password, max_len, out_len))
    {
        return 0; // Fallimento lettura -> annulla
    }

    // Se l'utente ha premuto solo Invio (stringa vuota), annulla
    if (*out_len == 0)
    {
        return 0;
    }

    return 1; // Successo
}

#if defined(__linux__)
/* Legge una riga da sysfs e rimuove i terminatori. */
static int ReadSysfsText(const char *path, char *buffer, size_t bufferSize)
{
    FILE *fp;
    size_t len;

    if (path == NULL || buffer == NULL || bufferSize == 0)
        return 0;

    buffer[0] = '\0';
    fp = fopen(path, "r");
    if (fp == NULL)
        return 0;

    if (fgets(buffer, (int)bufferSize, fp) == NULL)
    {
        fclose(fp);
        buffer[0] = '\0';
        return 0;
    }

    fclose(fp);

    len = strcspn(buffer, "\r\n");
    buffer[len] = '\0';

    while (len > 0 && (buffer[len - 1] == ' ' || buffer[len - 1] == '\t'))
        buffer[--len] = '\0';

    return len > 0;
}

static const char *LinuxStorageTransport(const char *devicePath)
{
    char resolvedPath[PATH_MAX];

    if (devicePath == NULL || realpath(devicePath, resolvedPath) == NULL)
        return "non rilevato";

    if (strstr(resolvedPath, "/nvme") != NULL)
        return "NVMe";
    if (strstr(resolvedPath, "/ata") != NULL)
        return "SATA/ATA";
    if (strstr(resolvedPath, "/usb") != NULL)
        return "USB";
    if (strstr(resolvedPath, "/mmc") != NULL)
        return "MMC/SD";
    if (strstr(resolvedPath, "/virtio") != NULL)
        return "VirtIO";
    if (strstr(resolvedPath, "/sas") != NULL)
        return "SAS";

    return "non identificato";
}

static int IsVirtualBlockDeviceName(const char *name)
{
    return strncmp(name, "loop", 4) == 0 ||
           strncmp(name, "ram", 3) == 0 ||
           strncmp(name, "dm-", 3) == 0 ||
           strncmp(name, "md", 2) == 0 ||
           strncmp(name, "zram", 4) == 0;
}

static void PrintLinuxStorageInfo(void)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    wprintf(L"\nUnità di memorizzazione (informazioni sysfs):\n");

    dir = opendir("/sys/block");
    if (dir == NULL)
    {
        wprintf(L"  Informazioni sui dispositivi non disponibili.\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char path[PATH_MAX];
        char model[256] = "";
        char vendor[128] = "";
        char sizeText[64] = "";
        char rotationalText[16] = "";
        char devicePath[PATH_MAX];
        char *endPtr;
        unsigned long long sectors = 0;
        int rotational = -1;
        double sizeGiB;

        if (entry->d_name[0] == '.' || IsVirtualBlockDeviceName(entry->d_name))
            continue;

        (void)snprintf(path, sizeof(path), "/sys/block/%s/size", entry->d_name);
        if (!ReadSysfsText(path, sizeText, sizeof(sizeText)))
            continue;

        errno = 0;
        sectors = strtoull(sizeText, &endPtr, 10);
        if (errno != 0 || endPtr == sizeText)
            sectors = 0;

        (void)snprintf(path, sizeof(path), "/sys/block/%s/device/model", entry->d_name);
        (void)ReadSysfsText(path, model, sizeof(model));
        (void)snprintf(path, sizeof(path), "/sys/block/%s/device/vendor", entry->d_name);
        (void)ReadSysfsText(path, vendor, sizeof(vendor));
        (void)snprintf(path, sizeof(path), "/sys/block/%s/queue/rotational", entry->d_name);
        if (ReadSysfsText(path, rotationalText, sizeof(rotationalText)))
            rotational = atoi(rotationalText);

        (void)snprintf(devicePath, sizeof(devicePath), "/sys/block/%s/device", entry->d_name);
        sizeGiB = ((double)sectors * 512.0) / (1024.0 * 1024.0 * 1024.0);

        wprintf(L"  Dispositivo        : /dev/%hs\n", entry->d_name);
        if (vendor[0] != '\0')
            wprintf(L"    Produttore       : %hs\n", vendor);
        if (model[0] != '\0')
            wprintf(L"    Modello          : %hs\n", model);
        wprintf(L"    Interfaccia      : %hs\n", LinuxStorageTransport(devicePath));

        if (sectors == 0)
            wprintf(L"    Capacità         : non disponibile (nessun supporto o capacità zero)\n");
        else
            wprintf(L"    Capacità         : %.2f GiB\n", sizeGiB);

        if (strncmp(entry->d_name, "sr", 2) == 0)
            wprintf(L"    Tipo             : unità ottica\n");
        else if (rotational == 1)
            wprintf(L"    Supporto         : rotazionale (il kernel segnala ROTA=1; tipico HDD)\n");
        else if (rotational == 0)
            wprintf(L"    Supporto         : non rotazionale (SSD/flash o lettore)\n");
        else
            wprintf(L"    Supporto         : non determinato\n");

        count++;
    }

    closedir(dir);

    if (count == 0)
        wprintf(L"  Nessuna unità fisica rilevata.\n");
}

static const char *PciVendorName(unsigned long vendorId)
{
    switch (vendorId)
    {
        case 0x1002: return "AMD/ATI";
        case 0x8086: return "Intel";
        case 0x10DE: return "NVIDIA";
        case 0x1022: return "AMD";
        case 0x1000: return "Broadcom/LSI";
        default: return "produttore non mappato";
    }
}

static void PrintLinuxGraphicsInfo(void)
{
    DIR *dir;
    struct dirent *entry;
    int count = 0;

    wprintf(L"\nGPU e controller grafici (PCI/sysfs):\n");

    dir = opendir("/sys/bus/pci/devices");
    if (dir == NULL)
    {
        wprintf(L"  Informazioni PCI non disponibili.\n");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        char path[PATH_MAX];
        char classText[32] = "";
        char vendorText[32] = "";
        char deviceText[32] = "";
        char subVendorText[32] = "";
        char subDeviceText[32] = "";
        char driverPath[PATH_MAX];
        char driverName[128] = "non rilevato";
        unsigned long classCode;
        unsigned long vendorId;
        unsigned long deviceId;
        unsigned long subVendorId = 0;
        unsigned long subDeviceId = 0;
        ssize_t linkLength;
        char *slash;

        if (entry->d_name[0] == '.')
            continue;

        (void)snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/class", entry->d_name);
        if (!ReadSysfsText(path, classText, sizeof(classText)))
            continue;

        classCode = strtoul(classText, NULL, 0);
        if (((classCode >> 16) & 0xFFUL) != 0x03UL)
            continue;

        (void)snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/vendor", entry->d_name);
        if (!ReadSysfsText(path, vendorText, sizeof(vendorText)))
            continue;
        (void)snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/device", entry->d_name);
        if (!ReadSysfsText(path, deviceText, sizeof(deviceText)))
            continue;
        (void)snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/subsystem_vendor", entry->d_name);
        (void)ReadSysfsText(path, subVendorText, sizeof(subVendorText));
        (void)snprintf(path, sizeof(path), "/sys/bus/pci/devices/%s/subsystem_device", entry->d_name);
        (void)ReadSysfsText(path, subDeviceText, sizeof(subDeviceText));

        vendorId = strtoul(vendorText, NULL, 0);
        deviceId = strtoul(deviceText, NULL, 0);
        if (subVendorText[0] != '\0')
            subVendorId = strtoul(subVendorText, NULL, 0);
        if (subDeviceText[0] != '\0')
            subDeviceId = strtoul(subDeviceText, NULL, 0);

        (void)snprintf(driverPath, sizeof(driverPath), "/sys/bus/pci/devices/%s/driver", entry->d_name);
        linkLength = readlink(driverPath, driverName, sizeof(driverName) - 1);
        if (linkLength >= 0)
        {
            driverName[linkLength] = '\0';
            slash = strrchr(driverName, '/');
            if (slash != NULL)
                memmove(driverName, slash + 1, strlen(slash + 1) + 1);
        }

        wprintf(L"  PCI address       : %hs\n", entry->d_name);
        wprintf(L"    Classe          : ");
        if (((classCode >> 8) & 0xFFUL) == 0x00UL)
            wprintf(L"controller VGA\n");
        else if (((classCode >> 8) & 0xFFUL) == 0x02UL)
            wprintf(L"controller 3D\n");
        else
            wprintf(L"controller display\n");
        wprintf(L"    Produttore      : %hs (PCI %04lx)\n", PciVendorName(vendorId), vendorId);
        wprintf(L"    Device ID       : %04lx\n", deviceId);
        if (vendorId == 0x1002 && deviceId == 0x675D)
            wprintf(L"    Modello noto    : Turks PRO (Radeon HD 7570)\n");
        if (subVendorText[0] != '\0' && subDeviceText[0] != '\0')
            wprintf(L"    Subsystem ID    : %04lx:%04lx\n", subVendorId, subDeviceId);
        wprintf(L"    Driver kernel   : %hs\n", driverName);
        count++;
    }

    closedir(dir);

    if (count == 0)
        wprintf(L"  Nessun controller grafico PCI rilevato.\n");
}
#endif /* __linux__ */

void PrintHardwareInfo(void)
{
    wprintf(L"Informazioni hardware e software:\n");

    /* Informazioni sul compilatore e sulla configurazione di build. */
#if defined(__clang__)
    wprintf(L"  Compilatore        : Clang %hs\n", __VERSION__);
#elif defined(__GNUC__)
    wprintf(L"  Compilatore        : GCC %hs\n", __VERSION__);
#elif defined(_MSC_VER)
    wprintf(L"  Compilatore        : Microsoft Visual C++ (%d)\n", _MSC_VER);
#else
    wprintf(L"  Compilatore        : non identificato\n");
#endif

#if defined(MYPDFSEARCH_BUILD_DEBUG)
    wprintf(L"  Build              : DEBUG (-O0 -g)\n");
#elif defined(MYPDFSEARCH_BUILD_RELEASE)
    wprintf(L"  Build              : RELEASE (-O3)\n");
#elif defined(__OPTIMIZE__)
    wprintf(L"  Ottimizzazione     : attiva (livello %d)\n", __OPTIMIZE__);
#else
    wprintf(L"  Ottimizzazione     : non rilevata/disattivata\n");
#endif

#if defined(__STDC_VERSION__)
#if __STDC_VERSION__ >= 201710L
#ifdef __STRICT_ANSI__
    wprintf(L"  Standard C         : C17\n");
#else
    wprintf(L"  Standard C         : GNU C17\n");
#endif
#elif __STDC_VERSION__ >= 201112L
#ifdef __STRICT_ANSI__
    wprintf(L"  Standard C         : C11\n");
#else
    wprintf(L"  Standard C         : GNU C11\n");
#endif
#elif __STDC_VERSION__ >= 199901L
#ifdef __STRICT_ANSI__
    wprintf(L"  Standard C         : C99\n");
#else
    wprintf(L"  Standard C         : GNU C99\n");
#endif
#else
    wprintf(L"  Standard C         : versione precedente a C99\n");
#endif
#endif

#if defined(_WIN64) || defined(_WIN32)
    SYSTEM_INFO sysInfo;
    MEMORYSTATUSEX memStatus;

    GetNativeSystemInfo(&sysInfo);
    wprintf(L"\nHardware:\n");
    wprintf(L"  CPU logiche online : %u\n", sysInfo.dwNumberOfProcessors);
    wprintf(L"  Architettura OS    : ");
    if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        wprintf(L"x64\n");
    else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        wprintf(L"x86\n");
    else
        wprintf(L"Sconosciuta\n");
    wprintf(L"  Architettura build : %ld-bit\n", (long)(sizeof(void *) * 8));

    /* Il nome CPU viene letto dal Registro di sistema. */
    {
        HKEY hKey;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                          L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                          0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            wchar_t cpuName[256] = {0};
            DWORD bufferSize = sizeof(cpuName);
            if (RegQueryValueExW(hKey, L"ProcessorNameString", NULL, NULL,
                                 (LPBYTE)cpuName, &bufferSize) == ERROR_SUCCESS)
            {
                wprintf(L"  CPU                : %ls\n", cpuName);
            }
            RegCloseKey(hKey);
        }
    }

    memStatus.dwLength = sizeof(memStatus);
    if (GlobalMemoryStatusEx(&memStatus))
    {
        double totalRamGiB = (double)memStatus.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        double availRamGiB = (double)memStatus.ullAvailPhys / (1024.0 * 1024.0 * 1024.0);
        wprintf(L"  RAM totale         : %.2f GiB\n", totalRamGiB);
        wprintf(L"  RAM disponibile    : %.2f GiB (utilizzo sistema: %lu%%)\n",
                availRamGiB, memStatus.dwMemoryLoad);
    }


#else
    /* Informazioni sul sistema operativo e sul kernel. */
    struct utsname osInfo;
    if (uname(&osInfo) == 0)
    {
        wprintf(L"\nSistema:\n");
        wprintf(L"  Sistema operativo : %hs\n", osInfo.sysname);
        wprintf(L"  Kernel             : %hs\n", osInfo.release);
        wprintf(L"  Architettura HW    : %hs\n", osInfo.machine);
    }

    /* CPU logiche visibili al sistema. */
    {
        long numProcessors = sysconf(_SC_NPROCESSORS_ONLN);
        if (numProcessors > 0)
            wprintf(L"\nHardware:\n  CPU logiche online : %ld\n", numProcessors);
        else
            wprintf(L"\nHardware:\n  CPU logiche online : non rilevate\n");
    }

    wprintf(L"  Architettura build : %ld-bit\n", (long)(sizeof(void *) * 8));

    /* CPU logiche consentite al processo secondo la CPU affinity. */
#if defined(__linux__)
    {
        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        if (sched_getaffinity(0, sizeof(cpuSet), &cpuSet) == 0)
        {
            int allowedCpus = CPU_COUNT(&cpuSet);
            if (allowedCpus > 0)
                wprintf(L"  CPU consentite     : %d\n", allowedCpus);
        }
    }
#endif

    /* Modello CPU letto da /proc/cpuinfo. */
    {
        FILE *fpCpu = fopen("/proc/cpuinfo", "r");
        if (fpCpu != NULL)
        {
            char line[512];
            while (fgets(line, sizeof(line), fpCpu) != NULL)
            {
                if (strncmp(line, "model name", 10) == 0)
                {
                    char *colon = strchr(line, ':');
                    if (colon != NULL)
                    {
                        char *cpuName = colon + 1;
                        size_t len;
                        while (*cpuName == ' ' || *cpuName == '\t')
                            cpuName++;
                        len = strlen(cpuName);
                        while (len > 0 && (cpuName[len - 1] == '\n' || cpuName[len - 1] == '\r'))
                            cpuName[--len] = '\0';
                        wprintf(L"  CPU                : %hs\n", cpuName);
                    }
                    break;
                }
            }
            fclose(fpCpu);
        }
    }

    /* Frequenza massima dichiarata dal driver cpufreq (non frequenza istantanea). */
    {
        FILE *fpFreq = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
        if (fpFreq != NULL)
        {
            unsigned long maxFreqKHz;
            if (fscanf(fpFreq, "%lu", &maxFreqKHz) == 1)
                wprintf(L"  Frequenza max CPU  : %.2f GHz (valore dichiarato)\n",
                        (double)maxFreqKHz / 1000000.0);
            fclose(fpFreq);
        }
    }

    /* Memoria complessiva del sistema: fotografia al termine del test. */
    {
        FILE *fpMem = fopen("/proc/meminfo", "r");
        if (fpMem != NULL)
        {
            char line[256];
            unsigned long long memTotal = 0;
            unsigned long long memAvailable = 0;

            while (fgets(line, sizeof(line), fpMem) != NULL)
            {
                if (strncmp(line, "MemTotal:", 9) == 0)
                    (void)sscanf(line, "MemTotal: %llu", &memTotal);
                else if (strncmp(line, "MemAvailable:", 13) == 0)
                    (void)sscanf(line, "MemAvailable: %llu", &memAvailable);

                if (memTotal > 0 && memAvailable > 0)
                    break;
            }
            fclose(fpMem);

            if (memTotal > 0)
            {
                double totalRamGiB = (double)memTotal / (1024.0 * 1024.0);
                double availRamGiB = (double)memAvailable / (1024.0 * 1024.0);
                double usedPercent = 0.0;
                if (memAvailable <= memTotal)
                    usedPercent = ((double)(memTotal - memAvailable) / (double)memTotal) * 100.0;

                wprintf(L"  RAM totale         : %.2f GiB\n", totalRamGiB);
                wprintf(L"  RAM disponibile    : %.2f GiB (utilizzo sistema stimato: %.1f%%)\n",
                        availRamGiB, usedPercent);
            }
        }
    }

    /* Picco di memoria residente (RSS) del solo processo corrente. */
    {
        struct rusage processUsage;
        if (getrusage(RUSAGE_SELF, &processUsage) == 0)
        {
            /* Linux esprime ru_maxrss in KiB; macOS lo esprime in byte. */
#if defined(__APPLE__)
            wprintf(L"  Picco RSS processo : %.2f MiB\n",
                    (double)processUsage.ru_maxrss / (1024.0 * 1024.0));
#else
            wprintf(L"  Picco RSS processo : %.2f MiB\n",
                    (double)processUsage.ru_maxrss / 1024.0);
#endif
        }
    }

#if defined(__linux__)
    PrintLinuxStorageInfo();
    PrintLinuxGraphicsInfo();
#endif
#endif

    /* I dati sui worker saranno aggiunti quando verrà introdotto il thread pool.
       Non stampiamo il numero di thread attivi a fine main(): a quel punto
       eventuali worker sono già stati terminati e uniti. */
}

/*
IMPORTANTE, ATTENZIONE!!! compilare su Windows con MINGW64, richiede di specificare '-liconv' alla fine, dopo '-lz'
                          altrimenti il linker dà errore.
*/




int main(int argc, char **argv)
{
	int retValue = EXIT_SUCCESS;

	Params myParams = {0};

	FilesList* myFilesList = NULL;

	#if !defined(_WIN64) && !defined(_WIN32)
	struct stat entryInfo;
	#endif

	//long numProcessors;

	int len;

	#if defined(MYPDFSEARCH_SHOW_TIME)
	myTimes mt_start;
	myTimes mt_end;
	int bTimeStartValid = 0;

	if ( !getTimes(&mt_start) )
	{
		wprintf(L"\nErrore getTimes(&mt_start) in main.c\n\n");
		retValue = EXIT_FAILURE;
		goto uscita;
	}
	bTimeStartValid = 1;
	#endif

	myParams.countWordsToSearch = 0;
	myParams.pWordsToSearchArray = NULL;

	myParams.bOptVersionOrHelp = 0;
	myParams.szPath[0] = '\0';
	myParams.szFilePdf[0] = '\0';
	myParams.fromPage = 0;
	myParams.toPage = 0;
	myParams.bNoSubDirs = 0;
	myParams.nSortGlyphs = SORT_GLYPHS_BY_APPEARANCE;
	myParams.szOutputFile[0] = '\0';
	//myParams.szWordsToSearch[0] = '\0';
	myParams.countWordsToSearch = 0;

	for ( int k = 0; k < MAX_LEN_STR + 1; k++ )
		myParams.szWordsToSearch[k] = '\0';

	myParams.fpOutput = NULL;

	myParams.fpErrors = NULL;

#if defined(_WIN64) || defined(_WIN32)
	//SetConsoleOutputCP(65001);
	//_setmode(_fileno(stdout), _O_U16TEXT);
	_setmode(_fileno(stdout), _O_U8TEXT);
#else
	//if ( !setlocale(LC_CTYPE, "it_IT.utf8") )
	if ( !setlocale(LC_ALL, "C.UTF-8") )
	{
		wprintf(L"Impossibile selezionare utf8!\n");
		printf("Impossibile selezionare utf8!\n");
		retValue = EXIT_FAILURE;
		goto uscita;
	}
#endif

	if (!PdfCryptoInitialize())
	{
		fprintf(stderr, "ERRORE: inizializzazione globale OpenSSL fallita.\n");

		retValue = EXIT_FAILURE;
		goto uscita;
	}

	checkEndianness(&myParams);

	if ( !myParseCommandLine(&myParams, argc, argv) )
	{
		retValue = EXIT_FAILURE;
		goto uscita;
	}

	if ( myParams.bOptVersionOrHelp )
	{
		retValue = EXIT_SUCCESS;
		goto uscita;
	}

	if ( myParams.szOutputFile[0] != '\0' )
	{
		if ( !MakeAndOpenOutputFile(&myParams) )
		{
			retValue = EXIT_FAILURE;
			goto uscita;
		}
	}

	if ( myParams.szFilePdf[0] != '\0' )
	{
		#if !defined(_WIN64) && !defined(_WIN32)
		char* psz = NULL;
		int x;
		int y;
		#endif

		myFilesList = (FilesList*)malloc(sizeof(FilesList));

		if( myFilesList == NULL )
		{
			wprintf(L"Errore main: impossibile aggiungere il file alla lista.\n");
			retValue = EXIT_FAILURE;
			goto uscita;
		}

		#if !defined(_WIN64) && !defined(_WIN32)
		strcpy(myFilesList->myPathName, myParams.szFilePdf);
		myFilesList->next = NULL;

		x = strlen(myFilesList->myPathName);
		while ( x >= 0 )
		{
			if ( myFilesList->myPathName[x] == '/' )
			{
				myFilesList->myPathName[x] = '\0';
				break;
			}
			x--;
		}

		if ( x > 0 )
			x++;
		psz = &(myParams.szFilePdf[x]);
		y = 0;
		while ( *psz != '\0' )
			myFilesList->myFileName[y++] = *(psz++);
		myFilesList->myFileName[y] = '\0';
		#else
		strcpy(myFilesList->myFileName, myParams.szFilePdf);
		myFilesList->next = NULL;
		#endif
	}
	else
	{
		#if !defined(_WIN64) && !defined(_WIN32)
		if( lstat(myParams.szPath, &entryInfo) == 0 )
		{
			if( S_ISDIR(entryInfo.st_mode) )		// directory
			{
				len = strnlen(myParams.szPath, MAX_LEN_STR);
				myFilesList = getFilesRecursive(myParams.szPath, len, myParams.bNoSubDirs, myFilesList);
				if ( !myFilesList )
				{
					wprintf(L"Errore: impossibile ottenere la lista dei file.\n");
					retValue = EXIT_FAILURE;
					goto uscita;
				}
			}
			else if( S_ISREG(entryInfo.st_mode) )	// regular file
			{
				char* psz = NULL;
				int x;
				int y;

				myFilesList = (FilesList*)malloc(sizeof(FilesList));

				if( myFilesList == NULL )
				{
					wprintf(L"Errore main: impossibile aggiungere il file alla lista.\n");
					retValue = EXIT_FAILURE;
					goto uscita;
				}

				strcpy(myFilesList->myPathName, myParams.szPath);
				myFilesList->next = NULL;

				x = strlen(myFilesList->myPathName);
				while ( x >= 0 )
				{
					if ( myFilesList->myPathName[x] == '/' )
					{
						myFilesList->myPathName[x] = '\0';
						break;
					}
					x--;
				}

				if ( x > 0 )
					x++;
				psz = &(myParams.szPath[x]);
				y = 0;
				while ( *psz != '\0' )
					myFilesList->myFileName[y++] = *(psz++);
				myFilesList->myFileName[y] = '\0';
			}
		}
		#else
		if ( GetFileAttributesA(myParams.szPath) & FILE_ATTRIBUTE_DIRECTORY )
		{
			len = strnlen(myParams.szPath, MAX_LEN_STR);
			myFilesList = getFilesRecursive(myParams.szPath, len, myParams.bNoSubDirs, myFilesList);
			if ( !myFilesList )
			{
				wprintf(L"Errore: impossibile ottenere la lista dei file.\n");
				retValue = EXIT_FAILURE;
				goto uscita;
			}
		}
		else
		{
			myFilesList = (FilesList*)malloc(sizeof(FilesList));
			if( myFilesList == NULL )
			{
				wprintf(L"Errore main: impossibile aggiungere il file alla lista.\n");
				retValue = EXIT_FAILURE;
				goto uscita;
			}
			strcpy(myFilesList->myFileName, myParams.szPath);
			myFilesList->next = NULL;
		}
		#endif
	}

	wprintf(L"\n");

	SetOnUserPasswordCallback(&myParams, MyConsolePasswordCallback);

	//Parse(&myParams, myFilesList, 0);
	Parse(&myParams, myFilesList);

uscita:

	freeFilesList(myFilesList);

	if ( NULL != myParams.pWordsToSearchArray )
	{
		int myIndex;

		for ( myIndex = 0; myIndex < myParams.countWordsToSearch; myIndex++ )
		{
			if ( NULL != myParams.pWordsToSearchArray[myIndex] )
			{
				free(myParams.pWordsToSearchArray[myIndex]);
				myParams.pWordsToSearchArray[myIndex] = NULL;
			}

		}
		free(myParams.pWordsToSearchArray);
		myParams.pWordsToSearchArray = NULL;
	}

	if ( NULL != myParams.fpErrors )
		fclose(myParams.fpErrors);

    // Rilascio del provider globale OpenSSL.
    // A questo punto tutte le elaborazioni crittografiche
    // devono essere terminate.
    if (!PdfCryptoShutdown())
    {
        fprintf(stderr, "ERRORE: rilascio del provider globale OpenSSL fallito.\n");
        retValue = EXIT_FAILURE;
    }

	#if defined(MYPDFSEARCH_SHOW_TIME)
	if ( !(myParams.bOptVersionOrHelp) && bTimeStartValid )
	{
		double elapsedReal;
		double elapsedUser;
		double elapsedSystem;
		double cpuUsagePercent;

		if ( !getTimes(&mt_end) )
		{
			wprintf(L"\nErrore getTimes(&mt_end) in main.c\n\n");
			retValue = EXIT_FAILURE;
		}
		else
		{
			elapsedReal = mt_end.realTime - mt_start.realTime;
			elapsedUser = mt_end.userTime - mt_start.userTime;
			elapsedSystem = mt_end.systemTime - mt_start.systemTime;

			wprintf(L"\n\n-----------------------------------------------------------------------\n");
			wprintf(L"Tempi di esecuzione (secondi):\n");
			wprintf(L"  Real Time  : %5.5f\n  User Time  : %5.5f\n  System Time: %5.5f\n",
				elapsedReal, elapsedUser, elapsedSystem);

			if (elapsedReal > 0.0)
			{
				cpuUsagePercent = ((elapsedUser + elapsedSystem) / elapsedReal) * 100.0;
				wprintf(L"  CPU media  : %.1f%% (100%% equivale a una CPU logica)\n\n",
					cpuUsagePercent);
			}
			else
			{
				wprintf(L"  CPU media  : non calcolabile (tempo reale nullo)\n\n");
			}

			PrintHardwareInfo();
		}
	}
	#else
	wprintf(L"\n\n");
	#endif

	return retValue;
}
