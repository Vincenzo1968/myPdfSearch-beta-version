#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "myGenHashTable.h"

int genhtInit(GenHashTable_t* p, uint32_t genhtSize, pfnGenHashFunc HashFunc, pfnGenCompareFunc CompareFunc)
{
	uint32_t x;

	if ( NULL == p )
	{
		return 0;
	}

	p->pHashTable = NULL;

	p->genhtSize = genhtSize;
	if ( p->genhtSize == 0 )
		p->genhtSize = GENHT_SIZE;

	p->pHashTable = (GenHashTableItem_t**)malloc(sizeof(GenHashTableItem_t*) * p->genhtSize);
	if ( !p->pHashTable )
		return 0;

	for ( x = 0; x < p->genhtSize; x++ )
		p->pHashTable[x] = NULL;

	p->HashFunc = HashFunc;
	p->CompareFunc = CompareFunc;

	return 1;
}

int genhtSetHashFunc(GenHashTable_t* p, pfnGenHashFunc HashFunc)
{
	p->HashFunc = HashFunc;

	return 1;
}

int genhtSetCompareFunc(GenHashTable_t* p, pfnGenCompareFunc CompareFunc)
{
	p->CompareFunc = CompareFunc;

	return 1;
}

GenHashTableItem_t* genhtNewNode(const void* pKey, uint32_t keysize, const void* pData, uint32_t datasize)
{
	GenHashTableItem_t* n = NULL;

	n = (GenHashTableItem_t*)malloc(sizeof(GenHashTableItem_t));

	if ( NULL == n )
		return NULL;

	n->pKey = (uint8_t*)malloc(sizeof(uint8_t) * keysize);
	if ( NULL == n->pKey )
	{
		free(n);
		n = NULL;
		return NULL;
	}
	memcpy(n->pKey, pKey, keysize);
	n->keysize = keysize;

	if ( NULL != pData )
	{
		n->pData = (uint8_t*)malloc(sizeof(uint8_t) * datasize);
		if ( NULL == n->pData )
		{
			free(n->pKey);
			n->pKey = NULL;
			free(n);
			n = NULL;
			return NULL;
		}
		memcpy(n->pData, pData, datasize);
		n->datasize = datasize;
	}
	else
	{
		n->pData = NULL;
		n->datasize = 0;
	}

	n->next = NULL;

	return n;
}

int genhtFind(GenHashTable_t* p, const void* pKey, uint32_t keysize, void** pData, uint32_t* datasize)
{
	int index = 0;
	GenHashTableItem_t* t = NULL;

	if ( NULL == p )
	{
		wprintf(L"\n\n\tERRORE genhtFind: p È NULL!!!\n\n");
		return -2;
	}

	if ( NULL == p->HashFunc )
		wprintf(L"\n\n\tERRORE genhtFind: p->HashFunc È NULL!!! -> KEY = <%s>\n\n", (char*)pKey);

	index = p->HashFunc(p, pKey, keysize);

	t = p->pHashTable[index];
	while ( t != NULL )
	{
		if ( p->CompareFunc(t->pKey, t->keysize, pKey, keysize) == 0 )
		{
			if ( NULL != *pData )
			{
				if ( NULL != t->pData )
				{
					memcpy(*pData, t->pData, t->datasize);
					*datasize = t->datasize;

					return index;
				}
				else
				{
					*pData = NULL;
					*datasize = 0;
				}
			}
			else
			{
				if ( NULL != t->pData && *datasize > 0 )
				{
					*pData = malloc(t->datasize);
					if ( NULL == pData )
						return -1;

					memcpy(*pData, t->pData, t->datasize);
					*datasize = t->datasize;

					return index;
				}
				else
				{
					free(*pData);
					*pData = NULL;
					*datasize = 0;
				}
			}

			return index;
		}

		t = t->next;
	}

	return -1;
}

int genhtFind_OLD(GenHashTable_t* p, const void* pKey, uint32_t keysize, void* pData, uint32_t* datasize)
{
	int index = 0;
	GenHashTableItem_t* t = NULL;

	//wprintf(L"\n\n\tgenhtFind: INIZIO LA RICERCA...\n\n");
	if ( NULL == p )
	{
		wprintf(L"\n\n\tERRORE genhtFind: p È NULL!!!\n\n");
		return -2;
	}
	//wprintf(L"\n\n\tOK! genhtFind: p NON È NULL!!! -> KEY = <%s>\n\n", (char*)pKey);

	if ( NULL == p->HashFunc )
		wprintf(L"\n\n\tERRORE genhtFind: p->HashFunc È NULL!!! -> KEY = <%s>\n\n", (char*)pKey);
	//else
	//	wprintf(L"\n\n\tOK! genhtFind: p->HashFunc NON È NULL!!! -> KEY = <%s>\n\n", (char*)pKey);

	index = p->HashFunc(p, pKey, keysize);

	//wprintf(L"\n\n\tgenhtFind: index = %d\n\n", index);

	t = p->pHashTable[index];
	while ( t != NULL )
	{
		if ( p->CompareFunc(t->pKey, t->keysize, pKey, keysize) == 0 )
		{
			if ( NULL != pData )
			{
				if ( NULL != t->pData )
				{
					memcpy(pData, t->pData, t->datasize);
					*datasize = t->datasize;

					return index;
				}
				else
				{
					pData = NULL;
					*datasize = 0;
				}
			}

			return index;
		}

		t = t->next;
	}

	return -1;
}

int genhtUpdateData(GenHashTable_t* p, const void* pKey, uint32_t keysize, const void* pData, uint32_t datasize)
{
	int index = 0;
	GenHashTableItem_t* t = NULL;

	if ( NULL == p )
		return -2;

	index = p->HashFunc(p, pKey, keysize);

	t = p->pHashTable[index];
	while ( t != NULL )
	{
		if ( p->CompareFunc(t->pKey, t->keysize, pKey, keysize) == 0 )
		{
			if ( NULL != pData )
			{
				if ( NULL != t->pData )
				{
					free(t->pData);
					t->pData = NULL;

					t->pData = (uint8_t*)malloc(sizeof(uint8_t) * datasize);
					if ( NULL == t->pData )
						return -2;

					memcpy(t->pData, pData, t->datasize);
					t->datasize = datasize;

					return index;
				}
			}

			return index;
		}

		t = t->next;
	}

	return -1;
}

int genhtDeleteKey(GenHashTable_t* p, const void* pKey, uint32_t keysize)
{
	int index = 0;
	GenHashTableItem_t* t = NULL;

	GenHashTableItem_t* prev = NULL;

	index = p->HashFunc(p, pKey, keysize);

	t = p->pHashTable[index];
	while ( t != NULL )
	{
		if ( p->CompareFunc(t->pKey, t->keysize, pKey, keysize) == 0 )
		{
			if ( NULL != t->pData )
			{
				free(t->pData);
				t->pData = NULL;
			}

			free(t->pKey);
			t->pKey = NULL;

			if ( NULL == prev )
			{
				p->pHashTable[index] = NULL;
			}
			else
			{
				prev->next = t->next;
			}

			free(t);
			t = NULL;

			return index;
		}

		prev = t;
		t = t->next;
	}

	return -1;
}

int genhtInsert(GenHashTable_t* p, const void* pKey, uint32_t keysize, const void* pData, uint32_t datasize)
{
	int index = 0;
	GenHashTableItem_t* t = NULL;

	index = p->HashFunc(p, pKey, keysize);

	t = p->pHashTable[index];
	if ( t == NULL )
	{
		p->pHashTable[index] = genhtNewNode(pKey, keysize, pData, datasize);
		return index;
	}

	while ( t != NULL )
	{
		if ( p->CompareFunc(t->pKey, t->keysize, pKey, keysize) == 0 )
		{
			return -1;
		}
		if ( t->next == NULL )
		{
			//wprintf(L"\nINDICE USATO PIÙ DI UNA VOLTAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA -> %d\n", index);
			t->next = genhtNewNode(pKey, keysize, pData, datasize);
			return index;
		}
		t = t->next;
	}

	return index;
}

void genhtFreeList(GenHashTableItem_t* first)
{
	GenHashTableItem_t *n1 = first, *n2 = NULL;

	while ( n1 != NULL )
	{
		n2 = n1->next;

		free(n1->pKey);
		n1->pKey = NULL;

		if ( NULL != n1->pData )
		{
			free(n1->pData);
			n1->pData = NULL;
		}

		free(n1);

		n1 = NULL;

		n1 = n2;
	}
}

void genhtFree(GenHashTable_t* p)
{
	uint32_t x;

	for( x = 0; x < p->genhtSize; x++ )
	{
		if ( p->pHashTable[x] )
		{
			genhtFreeList(p->pHashTable[x]);
			p->pHashTable[x] = NULL;
		}
	}

	free(p->pHashTable);

	p->pHashTable = NULL;

	p->genhtSize = 0;
}

void genhtTraverse(GenHashTable_t* p, pfnGenOnTraverse OnTraverse)
{
	uint32_t x;

	for( x = 0; x < p->genhtSize; x++ )
	{
		if ( p->pHashTable[x] )
		{
			GenHashTableItem_t *n1 = p->pHashTable[x];

			while ( n1 != NULL )
			{
				OnTraverse(n1->pKey, n1->keysize, n1->pData, n1->datasize);
				n1 = n1->next;
			}
		}
	}
}

/*
int GenStringHashFunc(GenHashTable_t* p, const void* pKey, uint32_t keysize)
{
	int index = 0;
	int a = 31415;
	const char* s = (const char*)pKey;

	for(; *s; s++)
		index = (a*index + *s) % p->genhtSize;

	return index;
}
*/

/*
int GenStringHashFunc(GenHashTable_t* p, const void* pKey, uint32_t keysize)
{
    const unsigned char *s = pKey;
    unsigned h = 0;
    uint32_t i;

    for (i = 0; i < keysize - 1; i++)
    {
        h ^= (h << 5) + (h >> 2) + s[i];
    }

    return h % p->genhtSize;
}
*/

/*
int GenStringHashFunc(GenHashTable_t* p, const void* pKey, uint32_t keysize)
{
    const unsigned char *s = pKey;
    unsigned h = 0;
    uint32_t i;

    for (i = 0; i < keysize - 1; i++)
    {
        h = 33 * h ^ s[i];
    }

    return h % p->genhtSize;
}
*/

/*
int GenStringHashFunc(GenHashTable_t* p, const void* pKey, uint32_t keysize)
{
    const unsigned char *s = pKey;
    unsigned h = 2166136261;
    uint32_t i;

    for (i = 0; i < keysize - 1; i++)
    {
        h = (h * 16777619) ^ s[i];
    }

    return h % p->genhtSize;
}
*/

/*
int GenStringHashFunc(GenHashTable_t* p, const void* pKey, uint32_t keysize)
{
    const unsigned char *s = pKey;
    unsigned h = 0, g;
    uint32_t i;

    for (i = 0; i < keysize - 1; i++)
    {
        h = (h << 4) + s[i];
        g = h & 0xf0000000L;

        if (g != 0)
        {
            h ^= g >> 24;
        }

        h &= ~g;
    }

    return h % p->genhtSize;
}
*/

int GenStringHashFunc(GenHashTable_t* p, const void* pKey, uint32_t keysize)
{
	unsigned n = 0;
	const char* s = (const char*)pKey;

	UNUSED(keysize);

	for ( ; *s; s++)
		n = 31 * n + *s;

	return n % p->genhtSize;
}

int GenStringCompareFunc(const void* pKey1, uint32_t keysize1, const void* pKey2, uint32_t keysize2)
{
	UNUSED(keysize1);
	UNUSED(keysize2);

	//wprintf(L"GenStringCompareFunc -> KEY1[%s](keysize1 = %lu) <> KEY2[%s](keysize2 = %lu)\n", pKey1, keysize1, pKey2, keysize2);

	return strncmp((char*)pKey1, (char*)pKey2, BLOCK_SIZE - 1);
}

int GenWideStringHashFunc(GenHashTable_t* p, const void* pKey, uint32_t keysize)
{
	unsigned n = 0;
	const wchar_t* s = (const wchar_t*)pKey;

	UNUSED(keysize);

	for ( ; *s; s++)
		n = 31 * n + *s;

	return n % p->genhtSize;
}

int GenWideStringCompareFunc(const void* pKey1, uint32_t keysize1, const void* pKey2, uint32_t keysize2)
{
	UNUSED(keysize1);
	UNUSED(keysize2);

	//wprintf(L"GenWideStringCompareFunc -> KEY1[%ls](keysize1 = %u) <> KEY2[%ls](keysize2 = %u)\n", (wchar_t*)pKey1, keysize1, (wchar_t*)pKey2, keysize2);

	return wcsncmp((wchar_t*)pKey1, (wchar_t*)pKey2, BLOCK_SIZE - 1);
}
