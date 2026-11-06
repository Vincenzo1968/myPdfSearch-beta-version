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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_STRLEN 1024
#include "myintqueuelist.h"

int myintqueuelist_Init(MyIntQueueList_t* p)
{
	p->head = NULL;
	p->tail = NULL;

	p->count = 0;

	return 1;
}

void myintqueuelist_Free(MyIntQueueList_t* p)
{
	MyIntQueueItem_t* n1;
	MyIntQueueItem_t* n2;

	if ( NULL == p )
		return;

	n1 = p->head;

	while ( n1 != NULL )
	{
		n2 = n1->next;
		free(n1);
		n1 = n2;
	}

	p->count = 0;
}

int myintqueuelist_IsEmpty(MyIntQueueList_t* p)
{
	return (NULL == p->head);
}

MyIntQueueItem_t* myintqueuelist_NewNode(int num)
{
	MyIntQueueItem_t* n;

	n = (MyIntQueueItem_t*)malloc(sizeof(MyIntQueueItem_t));
	if( NULL == n )
		return NULL;

	n->nItem = num;

	n->next = NULL;

	return n;
}

int myintqueuelist_Enqueue(MyIntQueueList_t* p, int num)
{
	if ( NULL == p->head )
	{
		p->head = myintqueuelist_NewNode(num);
		if ( NULL == p->head )
			return 0;

		p->tail = p->head;
		p->count++;
		return 1;
	}

	p->tail->next = myintqueuelist_NewNode(num);
	if ( NULL == p->tail->next )
	{
		return 0;
	}
	p->tail = p->tail->next;

	p->count++;

	return 1;

}

int myintqueuelist_Dequeue(MyIntQueueList_t* p, int* pNum)
{
	MyIntQueueItem_t* n;

	if ( NULL == p->head )
		return 0;

	*pNum = p->head->nItem;

	n = p->head;

	p->head = p->head->next;

	free(n);
	n = NULL;

	p->count--;

	return 1;
}

int myintqueuelist_Peek(MyIntQueueList_t* p, int* pNum)
{
	if ( NULL == p->head )
		return 0;

	*pNum = p->head->nItem;

	return 1;
}
