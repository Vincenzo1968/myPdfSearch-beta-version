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

#ifndef	__MYSTRINGQUEUELIST_H
#define	__MYSTRINGQUEUELIST_H

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else /* !__cplusplus */
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif /* __cplusplus */

#ifndef MAX_STRLEN
#define MAX_STRLEN 4096
#endif

typedef struct _MyStringQueueItem
{
	char *szItem;
	struct _MyStringQueueItem* next;
} MyStringQueueItem_t;

typedef struct _MyStringQueueList
{
	unsigned int count;
	MyStringQueueItem_t* head;
	MyStringQueueItem_t* tail;
} MyStringQueueList_t;


BEGIN_C_DECLS

int  mystringqueuelist_Init(MyStringQueueList_t* p);
void mystringqueuelist_Free(MyStringQueueList_t* p);
int  mystringqueuelist_IsEmpty(MyStringQueueList_t* p);
int  mystringqueuelist_Enqueue(MyStringQueueList_t* p, const char *s);
int  mystringqueuelist_Dequeue(MyStringQueueList_t* p, char *s);
int  mystringqueuelist_Peek(MyStringQueueList_t* p, char *s);

END_C_DECLS

#endif /* __MYSTRINGQUEUELIST_H */
