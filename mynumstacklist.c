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

#ifndef MAX_STRLEN
#define MAX_STRLEN 1024
#endif

#include "mynumstacklist.h"

int mynumstacklist_Init(MyNumStackList_t* p)
{
	p->head = NULL;
	p->tail = NULL;

	p->count = 0;

	return 1;
}

void mynumstacklist_Free(MyNumStackList_t* p)
{
	MyNumStackItem_t* n1;
	MyNumStackItem_t* n2;

	if ( NULL == p )
		return;

	n1 = p->head;

	while ( n1 != NULL )
	{
		n2 = n1->next;
		free(n1);
		n1 = n2;
	}

	mynumstacklist_Init(p);
	//p->count = 0;
}

int mynumstacklist_IsEmpty(MyNumStackList_t* p)
{
	return (NULL == p->head);
}

MyNumStackItem_t* mynumstacklist_NewNode(double num)
{
	MyNumStackItem_t* n;

	n = (MyNumStackItem_t*)malloc(sizeof(MyNumStackItem_t));
	if( NULL == n )
		return NULL;

	n->num = num;

	n->next = NULL;

	return n;
}

int mynumstacklist_Push(MyNumStackList_t* p, double num)
{
	MyNumStackItem_t* n;

	//wprintf(L"PUSH '%f'\n", num);

	if ( NULL == p->head )
	{
		p->head = mynumstacklist_NewNode(num);
		if ( NULL == p->head )
			return 0;

		p->count++;

		return 1;
	}

	n = mynumstacklist_NewNode(num);
	if ( NULL == n )
		return 0;

	n->next = p->head;
	p->head = n;

	p->count++;

	return 1;
}

int mynumstacklist_Pop(MyNumStackList_t* p, double *num)
{
	MyNumStackItem_t* n;

	if ( NULL == p->head )
		return 0;

	*num = p->head->num;

	//wprintf(L"POP  '%f'\n", *num);

	n = p->head;

	p->head = p->head->next;

	free(n);
	n = NULL;

	p->count--;

	return 1;
}

int mynumstacklist_Peek(MyNumStackList_t* p, double *num)
{
	if ( NULL == p->head )
		return 0;

	*num = p->head->num;

	return 1;
}
