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

#define MAX_STRLEN 1024
#include "scanner.h"
#include "mydictionaryqueuelist.h"

int mydictionaryqueuelist_GetUserDataParams(MyDictionaryQueueList_t* p, int *bFreeUserDataOnEnqueue, int *bAllocUserDataOnDequeue)
{
	if ( NULL == p )
	{
		*bFreeUserDataOnEnqueue = -1;
		*bAllocUserDataOnDequeue = -1;
		return 0;
	}

	*bFreeUserDataOnEnqueue = p->bFreeUserDataOnEnqueue;
	*bAllocUserDataOnDequeue = p->bAllocUserDataOnDequeue;

	return 1;
}

int mydictionaryqueuelist_SetUserDataParams(MyDictionaryQueueList_t* p, int bFreeUserDataOnEnqueue, int bAllocUserDataOnDequeue)
{
	if ( NULL == p )
		return 0;

	p->bFreeUserDataOnEnqueue = bFreeUserDataOnEnqueue;
	p->bAllocUserDataOnDequeue = bAllocUserDataOnDequeue;

	return 1;
}

int  mydictionaryqueuelist_Init(MyDictionaryQueueList_t* p, int bFreeUserDataOnEnqueue, int bAllocUserDataOnDequeue)
{
	if ( NULL == p )
		return 0;

	p->bFreeUserDataOnEnqueue = bFreeUserDataOnEnqueue;
	p->bAllocUserDataOnDequeue = bAllocUserDataOnDequeue;

	p->head = NULL;
	p->tail = NULL;

	p->count = 0;

	return 1;
}

void mydictionaryqueuelist_Free(MyDictionaryQueueList_t* p)
{
	MyDictionaryQueueItem_t* n1 = NULL;
	MyDictionaryQueueItem_t* n2 = NULL;

	int bFreeUserDataOnEnqueue;
	int bAllocUserDataOnDequeue;

	if ( NULL == p )
		return;

	n1 = p->head;

	while ( n1 != NULL )
	{
		n2 = n1->next;

		if ( NULL != n1->myDataItem.pszKey )
		{
			free(n1->myDataItem.pszKey);
			n1->myDataItem.pszKey = NULL;
		}

		if ( T_NAME == n1->myDataItem.tok.Type || T_STRING_LITERAL == n1->myDataItem.tok.Type || T_STRING_HEXADECIMAL == n1->myDataItem.tok.Type || T_STRING == n1->myDataItem.tok.Type )
		{
			if ( NULL != n1->myDataItem.tok.vString )
			{
				free(n1->myDataItem.tok.vString);
				n1->myDataItem.tok.vString = NULL;
			}
		}

		free(n1);
		n1 = n2;
	}

	mydictionaryqueuelist_GetUserDataParams(p, &bFreeUserDataOnEnqueue, &bAllocUserDataOnDequeue);
	mydictionaryqueuelist_Init(p, bFreeUserDataOnEnqueue, bAllocUserDataOnDequeue);
}

int mydictionaryqueuelist_IsEmpty(MyDictionaryQueueList_t* p)
{
	return (NULL != p && NULL == p->head);
}

MyDictionaryQueueItem_t* mydictionaryqueuelist_NewNode(MyData_t *pDataItem)
{
	MyDictionaryQueueItem_t* n = NULL;
	unsigned int len = 0;

	n = (MyDictionaryQueueItem_t*)malloc(sizeof(MyDictionaryQueueItem_t));
	if ( NULL == n )
		return NULL;

	n->myDataItem.pszKey = NULL;

	n->myDataItem.tok.Type = pDataItem->tok.Type;
	n->myDataItem.numFilter = pDataItem->numFilter;

	if ( NULL != pDataItem->pszKey )
	{
		len = strnlen(pDataItem->pszKey, MAX_STRLEN);

		n->myDataItem.pszKey = (char*)malloc(sizeof(char) * len + sizeof(char));
		if ( NULL == n->myDataItem.pszKey )
		{
			wprintf(L"ERRORE mystringqueuelist_NewNode: impossibile allocare la memoria per n->myDataItem.pszKey\n");
			free(n);
			n = NULL;
			return NULL;
		}

		strncpy(n->myDataItem.pszKey, pDataItem->pszKey, len + 1);
	}
	else
	{
		n->myDataItem.pszKey = NULL;
	}


	if ( T_NAME == pDataItem->tok.Type ||
		T_STRING_LITERAL == pDataItem->tok.Type ||
		T_STRING_HEXADECIMAL == pDataItem->tok.Type ||
		T_STRING == pDataItem->tok.Type )
	{
		if ( NULL != pDataItem->tok.vString )
		{
			len = strnlen(pDataItem->tok.vString, MAX_STRLEN);

			n->myDataItem.tok.vString = (char*)malloc(sizeof(char) * len + sizeof(char));
			if ( NULL == n->myDataItem.tok.vString )
			{
				wprintf(L"ERRORE mystringqueuelist_NewNode: impossibile allocare la memoria per n->myDataItem.tok.vString\n");
				free(n);
				n = NULL;
				return NULL;
			}
			strncpy(n->myDataItem.tok.vString, pDataItem->tok.vString, len + 1);
		}
		else
		{
			n->myDataItem.tok.vString = NULL;
		}
	}
	else
	{
		switch ( pDataItem->tok.Type )
		{
			case T_INT_LITERAL:
				n->myDataItem.tok.vInt = pDataItem->tok.vInt;
				break;
			case T_REAL_LITERAL:
				n->myDataItem.tok.vDouble = pDataItem->tok.vDouble;
				break;
			default:
				break;
		}
	}

	n->next = NULL;

	return n;
}

int mydictionaryqueuelist_Enqueue(MyDictionaryQueueList_t* p, MyData_t *pDataItem)
{
	if ( NULL == p )
	{
		return 0;
	}

	if ( NULL == p->head )
	{
		p->head = mydictionaryqueuelist_NewNode(pDataItem);
		if ( NULL == p->head )
			return 0;

		p->tail = p->head;
		p->count++;

		if ( p->bFreeUserDataOnEnqueue )
		{
			if ( NULL != pDataItem->pszKey )
			{
				free(pDataItem->pszKey);
				pDataItem->pszKey = NULL;
			}

			if ( T_NAME == pDataItem->tok.Type ||
				T_STRING_LITERAL == pDataItem->tok.Type ||
				T_STRING_HEXADECIMAL == pDataItem->tok.Type ||
				T_STRING == pDataItem->tok.Type )
			{
				if ( NULL != pDataItem->tok.vString )
				{
					free(pDataItem->tok.vString);
					pDataItem->tok.vString = NULL;
				}
			}
		}

		return 1;
	}

	if ( NULL == p->tail )
	{
		return 0;
	}

	p->tail->next = mydictionaryqueuelist_NewNode(pDataItem);
	if (NULL == p->tail->next )
	{
		return 0;
	}
	p->tail = p->tail->next;

	p->count++;

	if ( p->bFreeUserDataOnEnqueue )
	{
		if ( NULL != pDataItem->pszKey )
		{
			free(pDataItem->pszKey);
			pDataItem->pszKey = NULL;
		}

		if ( T_NAME == pDataItem->tok.Type ||
			T_STRING_LITERAL == pDataItem->tok.Type ||
			T_STRING_HEXADECIMAL == pDataItem->tok.Type ||
			T_STRING == pDataItem->tok.Type )
		{
			if ( NULL != pDataItem->tok.vString )
			{
				free(pDataItem->tok.vString);
				pDataItem->tok.vString = NULL;
			}
		}
	}

	return 1;

}

int mydictionaryqueuelist_Dequeue(MyDictionaryQueueList_t* p, MyData_t *pDataItem)
{
	unsigned int len = 0;

	MyDictionaryQueueItem_t* n = NULL;

	if ( NULL != p && NULL == p->head )
	{
		pDataItem->numFilter = -1;
		return 0;
	}

	pDataItem->tok.Type = p->head->myDataItem.tok.Type;
	pDataItem->numFilter = p->head->myDataItem.numFilter;

	if ( NULL != p->head->myDataItem.pszKey )
	{
		len = strnlen(p->head->myDataItem.pszKey, MAX_STRLEN);
		if ( p->bAllocUserDataOnDequeue )
		{
			if ( NULL != pDataItem->pszKey )
				free(pDataItem->pszKey);
			pDataItem->pszKey = (char*)malloc(sizeof(char) * len + sizeof(char));
			if ( NULL == pDataItem->pszKey )
			{
				return 0;
			}
		}

		strncpy(pDataItem->pszKey, p->head->myDataItem.pszKey, len + 1);
	}
	else
	{
		pDataItem->pszKey = NULL;
	}

	if ( T_NAME == p->head->myDataItem.tok.Type ||
		T_STRING_LITERAL == p->head->myDataItem.tok.Type ||
		T_STRING_HEXADECIMAL == p->head->myDataItem.tok.Type ||
		T_STRING == p->head->myDataItem.tok.Type )
	{
		if ( NULL != p->head->myDataItem.tok.vString )
		{
			len = strnlen(p->head->myDataItem.tok.vString, MAX_STRLEN);
			if ( p->bAllocUserDataOnDequeue )
			{
				pDataItem->tok.vString = (char*)malloc(sizeof(char) * len + sizeof(char));
				if ( NULL == pDataItem->tok.vString )
				{
					wprintf(L"ERRORE mystringqueuelist_NewNode: impossibile allocare la memoria per pDataItem->tok.vString\n");
					return 0;
				}
			}

			strncpy(pDataItem->tok.vString, p->head->myDataItem.tok.vString, len + 1);
		}
		else
		{
			if ( p->bFreeUserDataOnEnqueue )
			{
				if ( NULL != pDataItem->tok.vString )
				{
					free(pDataItem->tok.vString);
				}
				pDataItem->tok.vString = NULL;
			}
		}
	}
	else
	{
		switch ( p->head->myDataItem.tok.Type )
		{
			case T_INT_LITERAL:
				pDataItem->tok.vInt = p->head->myDataItem.tok.vInt;
				break;
			case T_REAL_LITERAL:
				pDataItem->tok.vDouble = p->head->myDataItem.tok.vDouble;
				break;
			default:
				break;
		}
	}

	n = p->head;

	p->head = p->head->next;

	if ( NULL != n->myDataItem.pszKey )
	{
		free(n->myDataItem.pszKey);
	}
	n->myDataItem.pszKey = NULL;

	if ( T_NAME == n->myDataItem.tok.Type ||
		T_STRING_LITERAL == n->myDataItem.tok.Type ||
		T_STRING_HEXADECIMAL == n->myDataItem.tok.Type ||
		T_STRING == n->myDataItem.tok.Type )
	{
		if ( NULL != n->myDataItem.tok.vString )
		{
			free(n->myDataItem.tok.vString);
		}
		n->myDataItem.tok.vString = NULL;
	}

	free(n);
	n = NULL;

	p->count--;

	return 1;
}

int mydictionaryqueuelist_Peek(MyDictionaryQueueList_t* p, MyData_t *pDataItem)
{
	unsigned int len = 0;

	if ( NULL != p && NULL == p->head )
	{
		pDataItem->numFilter = -1;
		return 0;
	}

	pDataItem->tok.Type = p->head->myDataItem.tok.Type;
	pDataItem->numFilter = p->head->myDataItem.numFilter;

	if ( NULL != p->head->myDataItem.pszKey )
	{
		len = strnlen(p->head->myDataItem.pszKey, MAX_STRLEN);
		if ( p->bAllocUserDataOnDequeue )
		{
			if ( NULL != pDataItem->pszKey )
				free(pDataItem->pszKey);
			pDataItem->pszKey = (char*)malloc(sizeof(char) * len + sizeof(char));
			if ( NULL == pDataItem->pszKey )
			{
				wprintf(L"ERRORE mydictionaryqueuelist_Peek -> impossibile allocare la memoria per pDataItem->pszKey\n");
				return 0;
			}
		}

		strncpy(pDataItem->pszKey, p->head->myDataItem.pszKey, len + 1);
	}
	else
	{
		pDataItem->pszKey = NULL;
	}

	if ( T_NAME == p->head->myDataItem.tok.Type ||
		T_STRING_LITERAL == p->head->myDataItem.tok.Type ||
		T_STRING_HEXADECIMAL == p->head->myDataItem.tok.Type ||
		T_STRING == p->head->myDataItem.tok.Type )
	{

		if ( NULL != p->head->myDataItem.tok.vString )
		{
			len = strnlen(p->head->myDataItem.tok.vString, MAX_STRLEN);
			if ( p->bAllocUserDataOnDequeue )
			{
				pDataItem->tok.vString = (char*)malloc(sizeof(char) * len + sizeof(char));
				if ( NULL == pDataItem->tok.vString )
				{
					wprintf(L"ERRORE mystringqueuelist_NewNode: impossibile allocare la memoria per pDataItem->tok.vString\n");
					return 0;
				}
			}

			strncpy(pDataItem->tok.vString, p->head->myDataItem.tok.vString, len + 1);
		}
		else
		{
			if ( p->bFreeUserDataOnEnqueue )
			{
				if ( NULL != pDataItem->tok.vString )
					free(pDataItem->tok.vString);
				pDataItem->tok.vString = NULL;
			}
		}
	}
	else
	{
		switch ( p->head->myDataItem.tok.Type )
		{
			case T_INT_LITERAL:
				pDataItem->tok.vInt = p->head->myDataItem.tok.vInt;
				break;
			case T_REAL_LITERAL:
				pDataItem->tok.vDouble = p->head->myDataItem.tok.vDouble;
				break;
			default:
				break;
		}
	}

	return 1;
}
