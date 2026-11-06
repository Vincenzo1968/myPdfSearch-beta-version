#ifndef	__MY_GENERIC_HASHTABLE_H
#define	__MY_GENERIC_HASHTABLE_H

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else /* !__cplusplus */
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif /* __cplusplus */

#ifdef _MSC_VER
#define INLINE __forceinline /* use __forceinline (VC++ specific) */
#else
#define INLINE inline        /* use standard inline */
#endif

//#define GENHT_SIZE 251
//#define GENHT_SIZE 509
//#define GENHT_SIZE 1021
//#define GENHT_SIZE 2039
//#define GENHT_SIZE 4093
#define GENHT_SIZE 8191
//#define GENHT_SIZE 16381
//#define GENHT_SIZE 32749

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

// ----------------------------------------------- HashTable Functions -------------------------------------------------

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

#endif /* __MY_GENERIC_HASHTABLE_H */
