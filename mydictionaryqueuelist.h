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

#ifndef	MYDICTIONARYQUEUELIST_INCLUDED
#define	MYDICTIONARYQUEUELIST_INCLUDED

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

typedef struct tagToken2
{
	//TokenTypeEnum Type;
	int Type;
	//union tagMyValue
	//{
	char vChar;
	int vInt;
	double vDouble;
	char *vString;
	//} Value;
} Token2;

typedef struct _MyData
{
	int numFilter;
	char *pszKey;
	Token2 tok;
} MyData_t;

typedef struct _MyDictionaryQueueItem
{
	MyData_t myDataItem;
	struct _MyDictionaryQueueItem* next;
} MyDictionaryQueueItem_t;

typedef struct _MyDictionaryQueueList
{
	unsigned int count;
	unsigned int bFreeUserDataOnEnqueue;
	unsigned int bAllocUserDataOnDequeue;
	MyDictionaryQueueItem_t* head;
	MyDictionaryQueueItem_t* tail;
} MyDictionaryQueueList_t;


BEGIN_C_DECLS

int  mydictionaryqueuelist_Init(MyDictionaryQueueList_t* p, int bFreeUserDataOnEnqueue, int bAllocUserDataOnDequeue);
int mydictionaryqueuelist_GetUserDataParams(MyDictionaryQueueList_t* p, int *bFreeUserDataOnEnqueue, int *bAllocUserDataOnDequeue);
int mydictionaryqueuelist_SetUserDataParams(MyDictionaryQueueList_t* p, int bFreeUserDataOnEnqueue, int bAllocUserDataOnDequeue);
void mydictionaryqueuelist_Free(MyDictionaryQueueList_t* p);
int  mydictionaryqueuelist_IsEmpty(MyDictionaryQueueList_t* p);
MyDictionaryQueueItem_t* mydictionaryqueuelist_NewNode(MyData_t *pDataItem);
int  mydictionaryqueuelist_Enqueue(MyDictionaryQueueList_t* p, /*const*/ MyData_t *pDataItem);
int  mydictionaryqueuelist_Dequeue(MyDictionaryQueueList_t* p, MyData_t *pDataItem);
int  mydictionaryqueuelist_Peek(MyDictionaryQueueList_t* p, MyData_t *pDataItem);

END_C_DECLS

#endif // MYDICTIONARYQUEUELIST_INCLUDED
