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

#ifndef	__VL_REDBLACKTREE_H
#define	__VL_REDBLACKTREE_H

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


#include <stdint.h>
#include "mypdfsearch.h"

//typedef struct tagGenHashTable* GENHTptr;
//typedef int (*pfnGenHashFunc)(GENHTptr p, const void* pKey, uint32_t keysize);

//typedef struct vlrbtTreeNode* vlrbtTreeNodePtr;

typedef int (*pfnVlRbtOnTraverse)(void* pCurrNode);

typedef int (*pfnVlRbtCompareFunc)(const void* pKey1, uint32_t keysize1, const void* pKey2, uint32_t keysize2);

#define VLRBT_MAX_STACK 1024

typedef struct tag_vlrbtTreeNode
{
	//int data;
	void*    pKey;
	uint32_t keysize;
	void*    pData;
	uint32_t datasize;

	//char color;
	int32_t color;

	struct tag_vlrbtTreeNode *father;
	struct tag_vlrbtTreeNode *left;
	struct tag_vlrbtTreeNode *right;
} vlrbtTreeNode;

typedef struct tag_vlRedBlackTree
{
	vlrbtTreeNode *pRoot;
	uint32_t count;
	pfnVlRbtCompareFunc pCompareFunc;
	pfnVlRbtOnTraverse pOnTraverseFunc;
} vlRedBlackTree;





BEGIN_C_DECLS

int vlrbtInit(vlRedBlackTree* pTree, pfnVlRbtCompareFunc CompareFunc, pfnVlRbtOnTraverse OnTraverseFunc);

vlrbtTreeNode *vlrbtInsertNode(vlRedBlackTree *pTree, const void* pKey, uint32_t keySize, void* pData, uint32_t dataSize);
vlrbtTreeNode *vlrbtDeleteNode(vlRedBlackTree *pTree, vlrbtTreeNode *z);

void vlrbtTraverseReverseInOrder(vlRedBlackTree *pTree);
void vlrbtTraverseInOrder(vlRedBlackTree *pTree);

void vlrbtTreeSuccessor(vlrbtTreeNode *current, vlrbtTreeNode **result);
void vlrbtTreePredecessor(vlrbtTreeNode *current, vlrbtTreeNode **result);

int vlrbtSetCompareFunc(vlRedBlackTree* pTree, pfnVlRbtCompareFunc CompareFunc);
int vlrbtSetOnTraverseFunc(vlRedBlackTree* pTree, pfnVlRbtOnTraverse OnTraverseFunc);

void vlrbtFree(vlRedBlackTree *pTree);
// ------------------------------------------------------------------------------------------------------------------------------------------------

vlrbtTreeNode *vlrbtNewNode(const void* pKey, uint32_t keySize, void* pData, uint32_t dataSize);

void vlrbtInsertFixup(vlrbtTreeNode **head, vlrbtTreeNode **z);
void vlrbtDeleteFixup(vlrbtTreeNode **head, vlrbtTreeNode **x);

vlrbtTreeNode *vlrbtTreeRotateLeft(vlrbtTreeNode *head, vlrbtTreeNode *node);
vlrbtTreeNode *vlrbtTreeRotateRight(vlrbtTreeNode *head, vlrbtTreeNode *node);

END_C_DECLS

#endif // __VL_REDBLACKTREE_H
