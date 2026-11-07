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

#ifndef	MYNUMSTACKLIST_INCLUDED
#define	MYNUMSTACKLIST_INCLUDED

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else // !__cplusplus 
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif // __cplusplus

#ifndef MAX_STRLEN
#define MAX_STRLEN 4096
#endif

typedef struct _MyNumStackItem
{
	double num;
	struct _MyNumStackItem* next;
} MyNumStackItem_t;

typedef struct _MyNumStackList
{
	unsigned int   count;
	MyNumStackItem_t* head;
	MyNumStackItem_t* tail;
} MyNumStackList_t;

BEGIN_C_DECLS

int  mynumstacklist_Init(MyNumStackList_t* p);
void mynumstacklist_Free(MyNumStackList_t* p);
int  mynumstacklist_IsEmpty(MyNumStackList_t* p);
int  mynumstacklist_Push(MyNumStackList_t* p, double num);
int  mynumstacklist_Pop(MyNumStackList_t* p, double *num);
int  mynumstacklist_Peek(MyNumStackList_t* p, double *num);

END_C_DECLS

#endif // MYNUMSTACKLIST_INCLUDED
