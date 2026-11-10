/*
   Copyright (C) 2026 Vincenzo Lo Cicero

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef MAX_STRLEN
#define MAX_STRLEN 1024
#endif

#include "mystringqueuelist.h"

int mystringqueuelist_Init(MyStringQueueList_t* p)
{
	if ( NULL == p )
		return 0;

	p->head = NULL;
	p->tail = NULL;

	p->count = 0;

	return 1;
}

void mystringqueuelist_Free(MyStringQueueList_t* p)
{
	MyStringQueueItem_t* n1 = NULL;
	MyStringQueueItem_t* n2 = NULL;

	if ( NULL == p )
		return;

	n1 = p->head;

	while ( n1 != NULL )
	{
		n2 = n1->next;
		if ( NULL != n1->szItem )
		{
			free(n1->szItem);
		}
		n1->szItem = NULL;
		free(n1);
		n1 = n2;
	}

	mystringqueuelist_Init(p);

	//p->count = 0;
}

int mystringqueuelist_IsEmpty(MyStringQueueList_t* p)
{
	return (NULL != p && NULL == p->head);
}

MyStringQueueItem_t* mystringqueuelist_NewNode(const char *s)
{
	MyStringQueueItem_t* n = NULL;
	int len = 0;

	n = (MyStringQueueItem_t*)malloc(sizeof(MyStringQueueItem_t));
	if( NULL == n )
		return NULL;

	if ( NULL != s )
	{
		len = strnlen(s, MAX_STRLEN);

		n->szItem = (char*)malloc(sizeof(char) * len + sizeof(char));
		if ( NULL == n->szItem )
		{
			free(n);
			n = NULL;
			return NULL;
		}

		strncpy(n->szItem, s, len + 1);
	}
	else
	{
		n->szItem = NULL;
	}

	n->next = NULL;

	return n;
}

int mystringqueuelist_Enqueue(MyStringQueueList_t* p, const char *s)
{
	if ( NULL == p )
	{
		return 0;
	}

	if ( NULL == p->head )
	{
		p->head = mystringqueuelist_NewNode(s);
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

	p->tail->next = mystringqueuelist_NewNode(s);
	if (NULL == p->tail->next )
	{
		return 0;
	}
	p->tail = p->tail->next;

	p->count++;

	return 1;

}

int mystringqueuelist_Dequeue(MyStringQueueList_t* p, char *s)
{
	int len = 0;

	MyStringQueueItem_t* n = NULL;

	if ( NULL != p && NULL == p->head )
		return 0;

	//wprintf(L"mystringqueuelist_Dequeue -> p->head->szItem -> '%s'\n", p->head->szItem);

	if ( NULL != p->head->szItem )
	{
		len = strnlen(p->head->szItem, MAX_STRLEN);
		strncpy(s, p->head->szItem, len + 1);
	}
	else
	{
		if ( NULL != s )
			s[0] = '\0';
	}

	//wprintf(L"mystringqueuelist_Dequeue -> s -> '%s'\n", s);

	n = p->head;

	p->head = p->head->next;

	if ( NULL != n->szItem )
		free(n->szItem);
	n->szItem = NULL;
	free(n);
	n = NULL;

	p->count--;

	return 1;
}

int mystringqueuelist_Peek(MyStringQueueList_t* p, char *s)
{
	int len = 0;

	if ( NULL != p && NULL == p->head )
		return 0;

	if ( NULL != p->head->szItem )
	{
		len = strnlen(p->head->szItem, MAX_STRLEN);
		strncpy(s, p->head->szItem, len + 1);
	}
	else
	{
		if ( NULL != s )
			s[0] = '\0';
	}

	return 1;
}
