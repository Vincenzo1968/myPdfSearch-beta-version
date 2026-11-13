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

#include "myTree.h"
#include <wchar.h>

Tree * treeNewNode(int32_t numObjNumber, int32_t numObjParent)
{
	Tree *r;

	r = (Tree *) malloc(sizeof(Tree));

	if( !r )
	{
		//printf("Insufficient memory. \n);
		return NULL;
	}

	r->numObjNumber = numObjNumber;
	r->numObjParent = numObjParent;

	r->bCurrentPageHasDirectResources = 0;
	r->nCurrentPageResources = 0;

	r->nCurrentNumPageObjContent = 0;
	r->bCurrentContentIsPresent = 0;

	//r->father = NULL;
	r->firstchild = NULL;
	r->sibling = NULL;

	return r;
}

void treeFree(Tree *head)
{
	Tree *pCurrNode = head;
	Tree *pNodeTemp;

	if ( NULL == head )
		return;

	if ( NULL != head->firstchild )
	{
		pCurrNode = head->firstchild;
		while ( NULL != pCurrNode )
		{
			pNodeTemp = pCurrNode->sibling;
			treeFree(pCurrNode);
			pCurrNode = pNodeTemp;
		}
	}

	if ( NULL != head )
	{
		//wprintf(L"treeFree: FREE NODO -> numObjNumber = %d <> numObjParent = %d;\n", head->numObjNumber, head->numObjParent);
		free(head);
		head = NULL;
	}
}

void treeTraversePreOrder(Tree *head)
{
	Tree *pCurrNode = head;

	// if ( NULL != head  && NULL != head->firstchild ) // Stampiamo solo i nodi foglia
	if ( NULL != head )
		wprintf(L"numObjNumber = %d <> numObjParent = %d; ", head->numObjNumber, head->numObjParent);

	if ( NULL != head->firstchild )
	{
		pCurrNode = head->firstchild;
		while ( NULL != pCurrNode )
		{
			treeTraversePreOrder(pCurrNode);
			pCurrNode = pCurrNode->sibling;
		}
	}
}

void treeTraversePreOrderLeafOnly(Tree *head)
{
	Tree *pCurrNode = head;

	//if ( NULL! = head)
	if ( NULL != head  && NULL == head->firstchild ) // We only print the leaf knots
		wprintf(L"numObjNumber = %d <> numObjParent = %d; ", head->numObjNumber, head->numObjParent);

	if ( NULL != head->firstchild )
	{
		pCurrNode = head->firstchild;
		while ( NULL != pCurrNode )
		{
			treeTraversePreOrderLeafOnly(pCurrNode);
			pCurrNode = pCurrNode->sibling;
		}
	}
}

void treeTraversePostOrder(Tree *head)
{
	Tree *pCurrNode = head;

	//if ( NULL == head )
	// return;

	if ( NULL != head->firstchild )
	{
		pCurrNode = head->firstchild;
		while ( NULL != pCurrNode )
		{
			treeTraversePostOrder(pCurrNode);
			pCurrNode = pCurrNode->sibling;
		}
	}

	if ( NULL != head )
		wprintf(L"numObjNumber = %d <> numObjParent = %d; ", head->numObjNumber, head->numObjParent);
}

void treeTraversePostOrderLeafOnly(Tree *head)
{
	Tree *pCurrNode = head;

	//if ( NULL == head )
	// return;

	if ( NULL != head->firstchild )
	{
		pCurrNode = head->firstchild;
		while ( NULL != pCurrNode )
		{
			treeTraversePostOrderLeafOnly(pCurrNode);
			pCurrNode = pCurrNode->sibling;
		}
	}
	else
	{
		wprintf(L"numObjNumber = %d <> numObjParent = %d; ", head->numObjNumber, head->numObjParent);
	}
}

void treeTraverseInOrder(Tree *head)
{
	Tree *pCurrNode = head;

	if ( NULL == head->firstchild )
	{
		wprintf(L"numObjNumber = %d <> numObjParent = %d; ", head->numObjNumber, head->numObjParent);
	}
	else
	{
		pCurrNode = head->firstchild;
		treeTraverseInOrder(pCurrNode);
		wprintf(L"numObjNumber = %d <> numObjParent = %d; ", head->numObjNumber, head->numObjParent);
		pCurrNode = pCurrNode->sibling;
		while ( NULL != pCurrNode )
		{
			treeTraverseInOrder(pCurrNode);
			pCurrNode = pCurrNode->sibling;
		}
	}
}

void treeTraverseInOrderLeafOnly(Tree *head)
{
	Tree *pCurrNode = head;

	if ( NULL == head->firstchild )
	{
		wprintf(L"numObjNumber = %d <> numObjParent = %d; ", head->numObjNumber, head->numObjParent);
	}
	else
	{
		pCurrNode = head->firstchild;
		treeTraverseInOrderLeafOnly(pCurrNode);
		//wprintf(L"numObjNumber = %d < numObjParent = %d; ", head->numObjNumber, head->numObjParent);
		pCurrNode = pCurrNode->sibling;
		while ( NULL != pCurrNode )
		{
			treeTraverseInOrderLeafOnly(pCurrNode);
			pCurrNode = pCurrNode->sibling;
		}
	}
}

/*
int main()
{
	Tree *pTree = NULL;

	Tree *pNode1 = NULL;
	Tree *pNode2 = NULL;
	Tree *pNode3 = NULL;

	pTree = NewNode(1);
	if ( NULL == pTree )
	{
		return -1;
	}

	pTree->firstchild = NewNode(2);
	pTree->firstchild->sibling = NewNode(3);
	pTree->firstchild->sibling->sibling = NewNode(4);

	pNode1 = pTree->firstchild;
	pNode2 = pNode1->sibling;
	pNode3 = pNode2->sibling;

	pNode1->firstchild = NewNode(5);
	pNode1->firstchild->sibling = NewNode(6);
	pNode1->firstchild->sibling->sibling = NewNode(7);

	pNode2->firstchild = NewNode(8);
	pNode2->firstchild->sibling = NewNode(9);
	pNode2->firstchild->sibling->firstchild = NewNode(21);
	pNode2->firstchild->sibling->sibling = NewNode(10);

	pNode3->firstchild = NewNode(11);
	pNode3->firstchild->sibling = NewNode(12);
	pNode3->firstchild->sibling->sibling = NewNode(13);



	printf("TraverseInOrderLeafOnly; stampiamo solo i nodi foglia:\n");
	TraverseInOrderLeafOnly(pTree);
	printf("\n\n");

	printf("TraversePreOrderLeafOnly; stampiamo solo i nodi foglia:\n");
	TraversePreOrderLeafOnly(pTree);
	printf("\n\n");

	printf("TraversePostOrderLeafOnly; stampiamo solo i nodi foglia:\n");
	TraversePostOrderLeafOnly(pTree);
	printf("\n\n");




	printf("TraversePreOrder\n");
	TraversePreOrder(pTree);
	printf("\n\n");

	printf("TraversePostOrder\n");
	TraversePostOrder(pTree);
	printf("\n\n");

	printf("TraverseInOrder\n");
	TraverseInOrder(pTree);
	printf("\n\n");

	FreeTree(pTree);

	return 0;
}
*/
