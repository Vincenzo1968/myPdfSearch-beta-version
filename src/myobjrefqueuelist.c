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
#include <string.h>
#include <stdlib.h>

#ifndef MAX_STROBJKEYLEN
#define MAX_STROBJKEYLEN 1024
#endif

#include "myobjrefqueuelist.h"

int myobjreflist_Init(MyObjRefList_t* p)
{
	if ( NULL == p )
		return 0;

	p->head = NULL;
	p->tail = NULL;

	p->count = 0;

	return 1;
}

void myobjreflist_Free(MyObjRefList_t* p)
{
	MyObjRefItem_t* n1 = NULL;
	MyObjRefItem_t* n2 = NULL;

	if ( NULL == p )
		return;

	n1 = p->head;

	while ( n1 != NULL )
	{
		n2 = n1->next;
		if ( NULL != n1->szKey )
		{
			//wprintf(L)FREE - > '%s'\n, n1->szKey);
			free(n1->szKey);
		}
		n1->szKey = NULL;
		free(n1);
		n1 = n2;
	}

	myobjreflist_Init(p);
}

int myobjreflist_IsEmpty(MyObjRefList_t* p)
{
	return (NULL != p && NULL == p->head);
}

MyObjRefItem_t* myobjreflist_NewNode(const char *s, int num)
{
	MyObjRefItem_t* n = NULL;
	int len = 0;

	n = (MyObjRefItem_t*)malloc(sizeof(MyObjRefItem_t));
	if( NULL == n )
		return NULL;

	if ( NULL != s )
	{
		len = strnlen(s, MAX_STROBJKEYLEN);

		n->szKey = (char*)malloc(sizeof(char) * len + sizeof(char));
		if ( NULL == n->szKey )
		{
			free(n);
			n = NULL;
			return NULL;
		}

		strncpy(n->szKey, s, len + 1);

		n->numObjRef = num;
	}
	else
	{
		n->szKey = NULL;
		n->numObjRef = 0;
	}

	n->next = NULL;

	return n;
}

int myobjreflist_Enqueue(MyObjRefList_t* p, const char *s, int num)
{
	if ( NULL == p )
	{
		return 0;
	}

	if ( NULL == p->head )
	{
		p->head = myobjreflist_NewNode(s, num);
		if ( NULL == p->head )
			return 0;

		p->tail = p->head;
		p->count++;
		return 1;
	}

	if ( NULL == p->tail )
	{
		return 0;
	}

	p->tail->next = myobjreflist_NewNode(s, num);
	if (NULL == p->tail->next )
	{
		return 0;
	}
	p->tail = p->tail->next;

	p->count++;

	return 1;

}

int myobjreflist_Dequeue(MyObjRefList_t* p, char *s, int *pNum)
{
	int len = 0;

	MyObjRefItem_t* n = NULL;

	if ( NULL != p && NULL == p->head )
		return 0;

	if ( NULL != p->head->szKey )
	{
		len = strnlen(p->head->szKey, MAX_STROBJKEYLEN);
		strncpy(s, p->head->szKey, len + 1);

		*pNum = p->head->numObjRef;
	}
	else
	{
		if ( NULL != s )
			s[0] = '\0';

		*pNum = 0;
	}

	n = p->head;

	p->head = p->head->next;

	if ( NULL != n->szKey )
		free(n->szKey);
	n->szKey = NULL;
	free(n);
	n = NULL;

	p->count--;

	return 1;
}

int myobjreflist_Peek(MyObjRefList_t* p, char *s, int *pNum)
{
	int len = 0;

	if ( NULL != p && NULL == p->head )
		return 0;

	if ( NULL != p->head->szKey )
	{
		len = strnlen(p->head->szKey, MAX_STROBJKEYLEN);
		strncpy(s, p->head->szKey, len + 1);

		*pNum = p->head->numObjRef;
	}
	else
	{
		if ( NULL != s )
			s[0] = '\0';
	}

	return 1;
}
