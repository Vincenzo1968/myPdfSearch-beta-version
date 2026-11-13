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

#ifndef	MY_GENERIC_HASHTABLE_INCLUDED
#define	MY_GENERIC_HASHTABLE_INCLUDED

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else /* ! */
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif /*  */

#ifdef _MSC_VER
#define INLINE __forceinline /* use   forceinline (VC++ specific) */
#else
#define INLINE inline        /* use standard inline */
#endif

//#define GENHT SIZE 251
//#define GENHT SIZE 509
//#define GENHT SIZE 1021
//#define GENHT SIZE 2039
//#define GENHT SIZE 4093
#define GENHT_SIZE 8191
//#define GENHT SIZE 16381
//#define GENHT SIZE 32749

#include <stdint.h>
#include "mypdfsearch.h"

typedef struct tagGenHashTable* GENHTptr;

typedef int (*pfnGenHashFunc)(GENHTptr p, const void* pKey, uint32_t keysize);

typedef int (*pfnGenCompareFunc)(const void* pKey1, uint32_t keysize1, const void* pKey2, uint32_t keysize2);

typedef int (*pfnGenOnTraverse)(const void* pKey, uint32_t keysize, void* pData, uint32_t dataSize);

typedef struct tagGenHashTableItem
{
	void*    pKey;
	uint32_t keysize;
	void*    pData;
	uint32_t datasize;
	struct tagGenHashTableItem *next;
} GenHashTableItem_t;

typedef struct tagGenHashTable
{
	GenHashTableItem_t**   pHashTable;
	uint32_t               genhtSize;
	pfnGenHashFunc         HashFunc;
	pfnGenCompareFunc      CompareFunc;
} GenHashTable_t;

BEGIN_C_DECLS

// ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

int  genhtInit(GenHashTable_t* p, uint32_t genhtSize, pfnGenHashFunc HashFunc, pfnGenCompareFunc CompareFunc);
int  genhtSetHashFunc(GenHashTable_t* p, pfnGenHashFunc HashFunc);
int  genhtSetCompareFunc(GenHashTable_t* p, pfnGenCompareFunc CompareFunc);
int  genhtFind(GenHashTable_t* p, const void* pKey, uint32_t keysize, void** pData, uint32_t* datasize);
int  genhtUpdateData(GenHashTable_t* p, const void* pKey, uint32_t keysize, const void* pData, uint32_t datasize);
int  genhtDeleteKey(GenHashTable_t* p, const void* pKey, uint32_t keysize);
int  genhtInsert(GenHashTable_t* p, const void* pKey, uint32_t keysize, const void* pData, uint32_t datasize);
void genhtFree(GenHashTable_t* p);
void genhtTraverse(GenHashTable_t* p, pfnGenOnTraverse OnTraverse);

GenHashTableItem_t* genhtNewNode(const void* pKey, uint32_t keysize, const void* pData, uint32_t datasize);
void genhtFreeList(GenHashTableItem_t* first);

int GenStringHashFunc(GenHashTable_t* p, const void* pKey, uint32_t keysize);
int GenStringCompareFunc(const void* pKey1, uint32_t keysize1, const void* pKey2, uint32_t keysize2);

int GenWideStringHashFunc(GenHashTable_t* p, const void* pKey, uint32_t keysize);
int GenWideStringCompareFunc(const void* pKey1, uint32_t keysize1, const void* pKey2, uint32_t keysize2);

END_C_DECLS

#endif // MY_GENERIC_HASHTABLE_INCLUDED
