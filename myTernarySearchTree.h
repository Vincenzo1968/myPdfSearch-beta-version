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

#ifndef	__MY_TERNARY_SEARCH_TREE_H
#define	__MY_TERNARY_SEARCH_TREE_H

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else /* !__cplusplus */
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif /* __cplusplus */

#define BUFFER_SIZE 4096

#include <stdint.h>

typedef struct _tdata
{
	wchar_t*    key;
	uint8_t* data;
	uint32_t dataSize;
} tdata_t;

typedef struct tnode
{
	wchar_t splitchar;
	//tdata_t *pData;
	struct tnode* lokid;
	struct tnode* eqkid;
	struct tnode* hikid;
} tnode_t;

typedef struct _TernarySearchTree
{
	tnode_t* pRoot;
} TernarySearchTree_t;


BEGIN_C_DECLS


typedef int (*pfnOnTraverseTst)(const wchar_t* key, void* data, uint32_t dataSize);


void tstInit(TernarySearchTree_t* pTree);

tnode_t* tstInsertRecursive(tnode_t* p, const wchar_t *s, const void* pData, uint32_t dataSize, wchar_t* pReserved);

int tstSearchRecursive(tnode_t* p, const wchar_t* s, void* pData, uint32_t* dataSize);

uint32_t tstPartialMatchSearchRecursive(tnode_t* p, const wchar_t* s, pfnOnTraverseTst OnTraverse, uint32_t count);

uint32_t tstNearNeighborSearchRecursive(tnode_t* p, const wchar_t* s, int d, pfnOnTraverseTst OnTraverse, uint32_t count);

void tstFreeRecursive(TernarySearchTree_t* pTree, tnode_t* p);

uint32_t tstTraverseRecursive(tnode_t* p, pfnOnTraverseTst OnTraverse, uint32_t count);

uint32_t tstTraverseDescRecursive(tnode_t* p, pfnOnTraverseTst OnTraverse, uint32_t count);

int tstDeleteKey(TernarySearchTree_t* pTree, const wchar_t* s);

tnode_t* tstPrefixSearchRoot(tnode_t* p, const wchar_t* s);

uint32_t tstPrefixSearch(tnode_t* p, const wchar_t* s, pfnOnTraverseTst OnTraverse);

END_C_DECLS

#endif /* __MY_TERNARY_SEARCH_TREE_H */
