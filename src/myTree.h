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

#ifndef	MYTREE_INCLUDED
#define	MYTREE_INCLUDED

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else // ! 
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif //  

#include "mypdfsearch.h"

typedef struct tagTree
{
	int32_t numObjNumber;
	int32_t numObjParent;

	int bCurrentPageHasDirectResources;
	int nCurrentPageResources;   // 0 if the page has no references to Resources;
	                             // -1 if the page inherits Resources from one of its parents;
	                             // otherwise a whole greater than zero, which indicates
	                             // reference to the number of Resources object.

	int nCurrentNumPageObjContent;
	int bCurrentContentIsPresent;

	//struct tagTree *father;
	struct tagTree *firstchild;
	struct tagTree *sibling;
} Tree;

BEGIN_C_DECLS

Tree * treeNewNode(int32_t numObjNumber, int32_t numObjParent);

void treeFree(Tree *head);

void treeTraversePreOrder(Tree *head);
void treeTraversePostOrder(Tree *head);
void treeTraverseInOrder(Tree *head);

void treeTraversePreOrderLeafOnly(Tree *head);
void treeTraversePostOrderLeafOnly(Tree *head);
void treeTraverseInOrderLeafOnly(Tree *head);

END_C_DECLS

#endif // MYTREE_INCLUDED
