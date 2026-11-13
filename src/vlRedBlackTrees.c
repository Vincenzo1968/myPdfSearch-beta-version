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

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <wchar.h>
#include "vlRedBlackTrees.h"

int vlrbtInit(vlRedBlackTree* pTree, pfnVlRbtCompareFunc CompareFunc, pfnVlRbtOnTraverse OnTraverseFunc)
{
	if ( NULL == pTree )
		return 0;

	pTree->pRoot = NULL;

	pTree->count = 0;

	pTree->pCompareFunc = CompareFunc;
	pTree->pOnTraverseFunc = OnTraverseFunc;

	return 1;
}

int vlrbtSetCompareFunc(vlRedBlackTree* pTree, pfnVlRbtCompareFunc CompareFunc)
{
	pTree->pCompareFunc = CompareFunc;

	return 1;
}

int vlrbtSetOnTraverseFunc(vlRedBlackTree* pTree, pfnVlRbtOnTraverse OnTraverseFunc)
{
	pTree->pOnTraverseFunc = OnTraverseFunc;

	return 1;
}

vlrbtTreeNode *vlrbtNewNode(const void* pKey, uint32_t keySize, void* pData, uint32_t dataSize)
{
	vlrbtTreeNode *r;

	r = (vlrbtTreeNode *) malloc(sizeof(vlrbtTreeNode));

	if( !r )
	{
		//printf("Insufficient memory. \n);
		return NULL;
	}

	if ( NULL == pKey )
		return NULL;

	//r->data = info;
	r->pKey = (void*)malloc(keySize);
	if ( NULL == r->pKey )
	{
		return NULL;
	}
	memcpy(r->pKey, pKey, keySize);
	r->keysize = keySize;

	if ( NULL != pData )
	{
		r->pData = (void*)malloc(dataSize);
		if ( NULL == r->pData )
		{
			free(r->pKey);
			r->pKey = NULL;
			return NULL;
		}
		memcpy(r->pData, pData, dataSize);
		r->datasize = dataSize;
	}
	else
	{
		r->pData = NULL;
		r->datasize = 0;
	}

	r->color = 'b'; // 'b' = Black; 'r' = Red 

	r->father = NULL;
	r->left = NULL;
	r->right = NULL;

	return r;
}

vlrbtTreeNode *vlrbtInsertNode(vlRedBlackTree *pTree, const void* pKey, uint32_t keySize, void* pData, uint32_t dataSize)
{
	vlrbtTreeNode *node = pTree->pRoot;
	vlrbtTreeNode *z = NULL;
	vlrbtTreeNode *y = NULL;
	vlrbtTreeNode *pRadice = NULL;

	z = vlrbtNewNode(pKey, keySize, pData, dataSize);
	if ( z == NULL )
		return NULL;

	pRadice = node;

	while ( pRadice != NULL )
	{
		y = pRadice;
		//if
		if ( pTree->pCompareFunc(z->pKey, z->keysize, pRadice->pKey, pRadice->keysize) < 0 )
			pRadice = pRadice->left;
		else
			pRadice = pRadice->right;
	}

	z->father = y;
	if ( y == NULL )
	{
		//printf("I insert the root -> %dX1X", key);
		//wprintf(L"I insert the root -> '%lc'\n", ((vlrbtData t*)(pData))->c);

		node = z;
	}
	else
	{
		//if
		if ( pTree->pCompareFunc(z->pKey, z->keysize, y->pKey, y->keysize) < 0 )
		{
			//printf("I insert %d to the left of %d\n", z->data, y->data);
			//wprintf(L"I insert '%lc' to the left of '%lc'\n", ((vlrbtData t*)(pData)))->c, (vlrbtData t*)(y->pData))->c);

			y->left = z;
		}
		else
		{
			//printf("I insert %d to the right of %d\n", z->data, y->data);
			//wprintf(L"I insert '%lc' to the right of '%lc'\n", ((vlrbtData t*)(pData)))->c, (vlrbtData t*)(y->pData))->c);

			y->right = z;
		}
	}

	z->left = NULL;
	z->right = NULL;
	z->color = 'r'; /* Red */

	vlrbtInsertFixup(&node, &z);

	pTree->pRoot = node;

	pTree->count++;

	return node;
}

void vlrbtInsertFixup(vlrbtTreeNode **head, vlrbtTreeNode **z)
{
	vlrbtTreeNode *y = NULL;

	while ( (*z != NULL ) && ((*z)->father != NULL) && ((*z)->father->color == 'r') )
	{
		if ( (*z)->father == (*z)->father->father->left )
		{
			y = (*z)->father->father->right;
			if ( y != NULL && y->color == 'r' )
			{
				(*z)->father->color = 'b';
				y->color = 'b';
				(*z)->father->father->color = 'r';
				*z = (*z)->father->father;
			}
			else
			{
				if ( *z == (*z)->father->right )
				{
					*z = (*z)->father;
					*head = vlrbtTreeRotateLeft(*head, *z);
				}
				(*z)->father->color = 'b';
				(*z)->father->father->color = 'r';
				*head = vlrbtTreeRotateRight(*head, (*z)->father->father);
			}
		}
		else
		{
			y = (*z)->father->father->left;
			if ( y != NULL && y->color == 'r' )
			{
				(*z)->father->color = 'b';
				y->color = 'b';
				(*z)->father->father->color = 'r';
				*z = (*z)->father->father;
			}
			else
			{
				if ( *z == (*z)->father->left )
				{
					*z = (*z)->father;
					*head = vlrbtTreeRotateRight(*head, *z);
				}
				(*z)->father->color = 'b';
				(*z)->father->father->color = 'r';
				*head = vlrbtTreeRotateLeft(*head, (*z)->father->father);
			}
		}
	}

	(*head)->color = 'b';
}

vlrbtTreeNode *vlrbtDeleteNode(vlRedBlackTree *pTree, vlrbtTreeNode *z)
{
	vlrbtTreeNode *root = pTree->pRoot;
	vlrbtTreeNode *y = NULL;
	vlrbtTreeNode *x = NULL;

	if ( root == NULL || z == NULL )
		return root;

	if ( z->left == NULL || z->right == NULL )
		y = z;
	else
		vlrbtTreeSuccessor(z, &y);

	if ( y->left != NULL )
		x = y->left;
	else
		x = y->right;

	if ( NULL != x )
		x->father = y->father;

	if ( y->father == NULL )
	{
		root = x;
	}
	else
	{
		if ( y == y->father->left )
			y->father->left = x;
		else
			y->father->right = x;
	}

	if ( y != z )
	{
		//z->data = y->data;
		if ( z->keysize < y->keysize )
		{
			free(z->pKey);
			z->pKey = NULL;
			z->pKey = (void*)malloc(y->keysize);
			if ( NULL == z->pKey )
				return NULL;
		}
		memcpy(z->pKey, y->pKey, y->keysize);
		z->keysize = y->keysize;

		if ( NULL != y->pData )
		{
			if ( z->datasize < y->datasize )
			{
				free(z->pData);
				z->pData = NULL;
				z->pData = (void*)malloc(y->datasize);
				if ( NULL == z->pData )
				{
					free(z->pKey);
					z->pKey = NULL;
					return NULL;
				}
			}
			memcpy(z->pData, y->pData, y->datasize);
			z->datasize = y->datasize;
		}
		else
		{
			if ( NULL != z->pData )
			{
				free(z->pData);
				z->pData = NULL;
				z->datasize = 0;
			}
		}
	}

	if ( y->color == 'b' )
		vlrbtDeleteFixup(&root, &x);

	free(y);

	pTree->count--;

	return root;
}

void vlrbtDeleteFixup(vlrbtTreeNode **head, vlrbtTreeNode **x)
{
	vlrbtTreeNode *w = NULL;

	while ( (*x != NULL) && (*x != *head && (*x)->color == 'b') )
	{
		if ( *x == (*x)->father->left )
		{
			w = (*x)->father->right;
			if ( w->color == 'r' )
			{
				w->color = 'b';
				(*x)->father->color = 'r';
				*head = vlrbtTreeRotateLeft(*head, (*x)->father);
				w = (*x)->father->right;
			}

			if ( w->left->color == 'b' && w->right->color == 'b' )
			{
				w->color = 'r';
				(*x) = (*x)->father;
			}
			else
			{
				if ( w->right->color == 'b' )
				{
					w->left->color = 'b';
					w->color = 'r';
					*head = vlrbtTreeRotateRight(*head, w);
					w = (*x)->father->right;
				}

				w->color = (*x)->father->color;
				(*x)->father->color = 'b';
				w->right->color = 'b';
				*head = vlrbtTreeRotateLeft(*head, (*x)->father);
				*x = *head;
			}
		}
		else
		{
			w = (*x)->father->left;
			if ( w->color == 'r' )
			{
				w->color = 'b';
				(*x)->father->color = 'r';
				*head = vlrbtTreeRotateRight(*head, (*x)->father);
				w = (*x)->father->left;
			}

			if ( w->right->color == 'b' && w->left->color == 'b' )
			{
				w->color = 'r';
				(*x) = (*x)->father;
			}
			else
			{
				if ( w->left->color == 'b' )
				{
					w->right->color = 'b';
					w->color = 'r';
					*head = vlrbtTreeRotateLeft(*head, w);
					w = (*x)->father->left;
				}

				w->color = (*x)->father->color;
				(*x)->father->color = 'b';
				w->left->color = 'b';
				*head = vlrbtTreeRotateRight(*head, (*x)->father);
				*x = *head;
			}
		}
	}

	if ( (*x) != NULL )
		(*x)->color = 'b';
}

void vlrbtFree(vlRedBlackTree *pTree)
{
	vlrbtTreeNode *head = pTree->pRoot;

	vlrbtTreeNode *temp1, *temp2;

	vlrbtTreeNode *stack[VLRBT_MAX_STACK];
	int top;

	top = 0;

	if ( head == NULL )
		return;

	temp1 = temp2 = head;

	while ( temp1 != NULL )
	{
		for(; temp1->left != NULL; temp1 = temp1->left)
			stack[top++] = temp1;

		while ( (temp1 != NULL) && (temp1->right == NULL || temp1->right == temp2) )
		{
			temp2 = temp1;

			if ( NULL != temp2-> pKey )
			{
				free(temp2->pKey);
				temp2->pKey = NULL;
			}
			if ( NULL != temp2-> pData )
			{
				free(temp2->pData);
				temp2->pData = NULL;
			}
			free(temp2);
			//temp2 = NULL; // DO NOT DECOMMEND TO OTHER ENTER IN A FINITE LOOP.

			if ( top == 0 )
				return;
			temp1 = stack[--top];
		}

		stack[top++] = temp1;
		temp1 = temp1->right;
	}

	pTree->count = 0;

	pTree->pRoot = NULL;
}

vlrbtTreeNode *vlrbtTreeRotateLeft(vlrbtTreeNode *head, vlrbtTreeNode *node)
{
	vlrbtTreeNode *y;

	if ( head == NULL || node == NULL )
		return head;

	if ( node->right == NULL )
		return head;

	y = node->right;
	node->right = y->left;

	if ( y->left != NULL )
	{
		y->left->father = node;
	}

	y->father = node->father;

	if ( node->father == NULL )
	{
		head = y;
	}
	else
	{
		if ( node == node->father->left )
		{
			node->father->left = y;
		}
		else
		{
			node->father->right = y;
		}
	}

	y->left = node;
	node->father = y;

	return head;
}

vlrbtTreeNode *vlrbtTreeRotateRight(vlrbtTreeNode *head, vlrbtTreeNode *node)
{
	vlrbtTreeNode *y;

	if ( head == NULL || node == NULL )
		return head;

	if ( node->left == NULL )
		return head;

	y = node->left;
	node->left = y->right;

	if ( y->right != NULL )
	{
		y->right->father = node;
	}

	y->father = node->father;

	if ( node->father == NULL )
	{
		head = y;
	}
	else
	{
		if ( node == node->father->right )
		{
			node->father->right = y;
		}
		else
		{
			node->father->left = y;
		}
	}

	y->right = node;
	node->father = y;

	return head;
}

void vlrbtTreeSuccessor(vlrbtTreeNode *current, vlrbtTreeNode **result)
{
	vlrbtTreeNode *nodo = current;

	*result = NULL;

	if ( nodo == NULL )
		return;

	if ( nodo->right != NULL )
	{
		nodo = nodo->right;
		while ( nodo != NULL )
		{
			*result = nodo;
			nodo = nodo->left;
		}
	}
	else
	{
		*result = nodo->father;
		while ( *result != NULL && nodo == (*result)->right )
		{
			nodo = *result;
			*result = (*result)->father;
		}
	}
}

void vlrbtTreePredecessor(vlrbtTreeNode *current, vlrbtTreeNode **result)
{
	vlrbtTreeNode *nodo = current;

	*result = NULL;

	if ( nodo == NULL )
		return;

	if ( nodo->left != NULL )
	{
		nodo = nodo->left;
		while ( nodo != NULL )
		{
			*result = nodo;
			nodo = nodo->right;
		}
	}
	else
	{
		*result = nodo->father;
		while ( *result != NULL && nodo == (*result)->left )
		{
			nodo = *result;
			*result = (*result)->father;
		}
	}
}

void vlrbtTraverseReverseInOrder(vlRedBlackTree *pTree)
{
	vlrbtTreeNode *head = pTree->pRoot;
	vlrbtTreeNode *temp;

	vlrbtTreeNode *stack[VLRBT_MAX_STACK];
	int top;
	top = -1;

	if ( head == NULL )
		return;

	temp = head;

	while ( 1 )
	{
		if ( temp != NULL )
		{
			if ( top < VLRBT_MAX_STACK )
			{
				stack[++top] = temp; // Push 
				temp = temp->right;
			}
			else
			{
				printf("Errore: lo stack e' pieno.\n");
				return;
			}
		}
		else
		{
			if ( top >= 0 )
			{
				temp = stack[top--]; // Pop 
				//printf("%d\n", temp->data);
				pTree->pOnTraverseFunc((void*)temp);
				temp = temp->left;
			}
			else
			{
				break;
			}
		}
	}
}

void vlrbtTraverseInOrder(vlRedBlackTree *pTree)
{
	vlrbtTreeNode *head = pTree->pRoot;
	vlrbtTreeNode *temp;

	vlrbtTreeNode *stack[VLRBT_MAX_STACK];
	int top;
	top = -1;

	if ( head == NULL )
		return;

	temp = head;

	while ( 1 )
	{
		if ( temp != NULL )
		{
			if ( top < VLRBT_MAX_STACK )
			{
				stack[++top] = temp; // Push 
				temp = temp->left;
			}
			else
			{
				printf("Errore: lo stack e' pieno.\n");
				return;
			}
		}
		else
		{
			if ( top >= 0 )
			{
				temp = stack[top--]; // Pop 
				//printf("%d\n", temp->data);
				pTree->pOnTraverseFunc((void*)temp);
				temp = temp->right;
			}
			else
			{
				break;
			}
		}
	}
}

/*
int VlRbtCompareFunc(const void* pKey1, uint32_t keysize1, const void* pKey2, uint32_t keysize2)
{
	UNUSED(keysize1);
	UNUSED(keysize2);

	myKey *pMyKey1 = (myKey*)pKey1;
	myKey *pMyKey2 = (myKey*)pKey2;

	//if ( (pMyKey1->row == pMyKey2->row) && (pMyKey1->col == pMyKey2->col) && (pMyKey1->ord == pMyKey2->ord) )
	//	return 0;

	if ( pMyKey1->row > pMyKey2->row )
	{
		//return 1;
		return -1;   // Perché le righe vanno dal basso in alto
	}
	else if ( pMyKey1->row < pMyKey2->row )
	{
		//return -1;
		return 1;    // Perché le righe vanno dal basso in alto
	}
	else // pMyKey1->row == pMyKey2->row; CONFRONTIAMO LE COLONNE.
	{
		if ( pMyKey1->col < pMyKey2->col )
		{
			return -1;
		}
		else if ( pMyKey1->col > pMyKey2->col )
		{
			return 1;
		}
		else // pMyKey1->col == pMyKey2->col; CONFRONTIAMO L'ordine di lettura.
		{
			if ( pMyKey1->ord > pMyKey2->ord )
			{
				return 1;
			}
			else if ( pMyKey1->ord < pMyKey2->ord )
			{
				return -1;
			}
			else
			{
				return 0;
			}
		}
	}
}

int VlRbtOnTraverseFunc(const void* pKey, uint32_t keysize, void* pData, uint32_t datasize)
{
	UNUSED(keysize);
	UNUSED(datasize);

	myKey *pMyKey = (myKey*)pKey;
	myData *pMyData = (myData*)pData;

	printf("(row = %f; col = %f; ord = %d) -> c = '%c'; width = %f; currFontSpaceWidth = %f\n", pMyKey->row, pMyKey->col, pMyKey->ord, pMyData->c, pMyData->width, pMyData->currFontSpaceWidth);

	return 1;
}
*/

/* gcc -Wall -Wextra -pedantic -O0 -g vlRedBlackTrees.c -o rbtdebug
valgrind --leak-check=full --show-reachable=yes --track-origins=yes --log-file=outValgrind.txt ./rbtdebug

-Wall -Wall -pedantic -O3 vlRedBlackTrees.c -o rbt */

/*
int main()
{
	//int k;
	//int SearchKey;

	myKey key;
	myData data;

	vlRedBlackTree myTree;

	//vlrbtTreeNode *pSearch = NULL;
	//vlrbtTreeNode *pSuccessor = NULL;
	//vlrbtTreeNode *pPredecessor = NULL;
	//vlrbtTreeNode *pTree = NULL;

	vlrbtInit(&myTree, VlRbtCompareFunc, VlRbtOnTraverseFunc);

	key.row = 900.0;
	key.col = 515.0;
	key.ord = 1;
	data.c = 'e';
	data.width = 3.0;
	data.currFontSpaceWidth = 10.0;
	vlrbtInsertNode(&myTree, (void*)&key, sizeof(key), (void*)&data, sizeof(data));

	key.row = 1000.0;
	key.col = 515.0;
	key.ord++;
	data.c = 'b';
	data.width = 3.0;
	data.currFontSpaceWidth = 10.0;
	vlrbtInsertNode(&myTree, (void*)&key, sizeof(key), (void*)&data, sizeof(data));

	key.row = 800.0;
	key.col = 521.0;
	key.ord++;
	data.c = 'i';
	data.width = 3.0;
	data.currFontSpaceWidth = 10.0;
	vlrbtInsertNode(&myTree, (void*)&key, sizeof(key), (void*)&data, sizeof(data));

	key.row = 1000.0;
	key.col = 518.0;
	key.ord++;
	data.c = 'c';
	data.width = 3.0;
	data.currFontSpaceWidth = 10.0;
	vlrbtInsertNode(&myTree, (void*)&key, sizeof(key), (void*)&data, sizeof(data));

	key.row = 900.0;
	key.col = 512.0;
	key.ord++;
	data.c = 'd';
	data.width = 3.0;
	data.currFontSpaceWidth = 10.0;
	vlrbtInsertNode(&myTree, (void*)&key, sizeof(key), (void*)&data, sizeof(data));

	key.row = 800.0;
	key.col = 512.0;
	key.ord++;
	data.c = 'f';
	data.width = 3.0;
	data.currFontSpaceWidth = 10.0;
	vlrbtInsertNode(&myTree, (void*)&key, sizeof(key), (void*)&data, sizeof(data));

	key.row = 800.0;
	key.col = 515.0;
	key.ord++;
	data.c = 'g';
	data.width = 3.0;
	data.currFontSpaceWidth = 10.0;
	vlrbtInsertNode(&myTree, (void*)&key, sizeof(key), (void*)&data, sizeof(data));

	key.row = 1000.0;
	key.col = 512.0;
	key.ord++;
	data.c = 'a';
	data.width = 3.0;
	data.currFontSpaceWidth = 10.0;
	vlrbtInsertNode(&myTree, (void*)&key, sizeof(key), (void*)&data, sizeof(data));

	key.row = 800.0;
	key.col = 518.0;
	key.ord++;
	data.c = 'h';
	data.width = 3.0;
	data.currFontSpaceWidth = 10.0;
	vlrbtInsertNode(&myTree, (void*)&key, sizeof(key), (void*)&data, sizeof(data));



	printf("\nstampa in ordine crescente(inorder):\n");
	vlrbtTraverseInOrder(&myTree);

	printf("\nstampa in PreOrder:\n");
	vlrbtTraversePreOrder(&myTree);

	printf("\nstampa in PostOrder:\n");
	vlrbtTraversePostOrder(&myTree);

	printf("\nstampa in ordine decrescente:\n");
	vlrbtTraverseReverseInOrder(&myTree);

	vlrbtFree(&myTree);

	return 0;
}
*/
