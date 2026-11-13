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

#include "mycontentqueuelist.h"

// --------------------------------------------------------------------------------------------------------------------------------

int mycontentqueuelist_Init(MyContentQueueList_t* p)
{
	if ( NULL == p )
		return 0;

	p->head = NULL;
	p->tail = NULL;

	p->count = 0;

	return 1;
}

void mycontentqueuelist_Free(MyContentQueueList_t* p)
{
	MyContentQueueItem_t* n1 = NULL;
	MyContentQueueItem_t* n2 = NULL;

	if ( NULL == p )
	{
		return;
	}

	n1 = p->head;

	while ( n1 != NULL )
	{
		n2 = n1->next;

		mystringqueuelist_Free(&(n1->myContent.queueFilters));

		mydictionaryqueuelist_Free(&(n1->myContent.decodeParms));

		free(n1);
		n1 = n2;
	}

	mycontentqueuelist_Init(p);
}

int mycontentqueuelist_IsEmpty(MyContentQueueList_t* p)
{
	return (NULL != p && NULL == p->head);
}

MyContentQueueItem_t* mycontentqueuelist_NewNode(MyContent_t* pContent)
{
	MyContentQueueItem_t* n = NULL;

	MyStringQueueItem_t* pQueueFilters = NULL;

	MyDictionaryQueueItem_t* pDecodeParms = NULL;

	int len = 0;

	char szTemp[128];
	szTemp[0] = '\0';

	if ( NULL == pContent )
		return NULL;

	n = (MyContentQueueItem_t*)malloc(sizeof(MyContentQueueItem_t));
	if ( NULL == n )
		return NULL;
	n->next = NULL;

	mystringqueuelist_Init(&(n->myContent.queueFilters));

	n->myContent.bExternalFile = pContent->bExternalFile;
	n->myContent.LengthFromPdf = pContent->LengthFromPdf;
	//n->myContent.Length = pContent->Length;
	n->myContent.Offset = pContent->Offset;

	// Keep the PDF object identifier.
	n->myContent.ObjectNumber = pContent->ObjectNumber;
	n->myContent.GenerationNumber = pContent->GenerationNumber;

	pQueueFilters = pContent->queueFilters.head;
	while( pQueueFilters != NULL )
	{
		if ( NULL != pQueueFilters->szItem )
		{
			len = strnlen(pQueueFilters->szItem, 128);
			strncpy(szTemp, pQueueFilters->szItem, len + 1);
			mystringqueuelist_Enqueue(&(n->myContent.queueFilters), szTemp);
		}

		pQueueFilters = pQueueFilters->next;
	}


	mydictionaryqueuelist_Init(&(n->myContent.decodeParms), 1, 1);

	pDecodeParms = pContent->decodeParms.head;
	while ( pDecodeParms != NULL )
	{
		mydictionaryqueuelist_Enqueue(&(n->myContent.decodeParms), &(pDecodeParms->myDataItem));

		pDecodeParms = pDecodeParms->next;
	}

	n->next = NULL;

	return n;
}

int mycontentqueuelist_Enqueue(MyContentQueueList_t* p, MyContent_t* pContent)
{
	if ( NULL == p )
	{
		return 0;
	}

	if ( NULL == p->head )
	{
		p->head = mycontentqueuelist_NewNode(pContent);
		if ( NULL == p->head )
			return 0;

		p->tail = p->head;
		p->count++;
		return 1;
	}

	p->tail->next = mycontentqueuelist_NewNode(pContent);
	if ( NULL == p->tail->next )
	{
		return 0;
	}
	p->tail = p->tail->next;

	p->count++;

	return 1;

}

int mycontentqueuelist_Dequeue(MyContentQueueList_t* p, MyContent_t* pContent)
{
	MyContentQueueItem_t* n = NULL;

	MyData_t myDataTemp;

	char szTemp[128];

	szTemp[0] = '\0';

	if (NULL != p && NULL == p->head )
		return 0;

	pContent->bExternalFile = p->head->myContent.bExternalFile;
	pContent->LengthFromPdf = p->head->myContent.LengthFromPdf;
	//pContent->Length = p->head->myContent.Length;
	pContent->Offset = p->head->myContent.Offset;

	// Recovers the PDF object identifier.
	pContent->ObjectNumber = p->head->myContent.ObjectNumber;
	pContent->GenerationNumber = p->head->myContent.GenerationNumber;

	if ( pContent->queueFilters.count > 0 )
	{
		mystringqueuelist_Free(&(pContent->queueFilters));
	}

	while ( mystringqueuelist_Dequeue(&(p->head->myContent.queueFilters), szTemp) )
	{
		mystringqueuelist_Enqueue(&(pContent->queueFilters), szTemp);
	}

	if ( pContent->decodeParms.count > 0 )
	{
		mydictionaryqueuelist_Free(&(pContent->decodeParms));
	}

	myDataTemp.pszKey = NULL;
	myDataTemp.tok.vString = NULL;
	while ( mydictionaryqueuelist_Dequeue(&(p->head->myContent.decodeParms), &myDataTemp) )
	{
		mydictionaryqueuelist_Enqueue(&(pContent->decodeParms), &myDataTemp);
	}

	n = p->head;

	p->head = p->head->next;

	mystringqueuelist_Free(&(n->myContent.queueFilters));

	mydictionaryqueuelist_Free(&(n->myContent.decodeParms));

	free(n);
	n = NULL;

	p->count--;

	return 1;
}

int mycontentqueuelist_Peek(MyContentQueueList_t* p, MyContent_t* pContent)
{
	MyStringQueueItem_t* pQueueFilters;

	MyDictionaryQueueItem_t* pDecodeParmsItem = NULL;

	char szTemp[128];
	int len = 0;

	szTemp[0] = '\0';

	if (NULL != p && NULL == p->head )
		return 0;

	pContent->bExternalFile = p->head->myContent.bExternalFile;
	pContent->LengthFromPdf = p->head->myContent.LengthFromPdf;
	//pContent->Length = p->head->myContent.Length;
	pContent->Offset = p->head->myContent.Offset;

	if ( pContent->queueFilters.count > 0 )
	{
		mystringqueuelist_Free(&(pContent->queueFilters));
	}

	pQueueFilters = p->head->myContent.queueFilters.head;
	while( pQueueFilters != NULL )
	{
		len = strnlen(pQueueFilters->szItem, 128);
		strncpy(szTemp, pQueueFilters->szItem, len + 1);

		mystringqueuelist_Enqueue(&(pContent->queueFilters), szTemp);

		pQueueFilters = pQueueFilters->next;
	}


	pDecodeParmsItem = p->head->myContent.decodeParms.head;
	while ( NULL != pDecodeParmsItem )
	{
		mydictionaryqueuelist_Enqueue(&(pContent->decodeParms), &(pDecodeParmsItem->myDataItem));
		pDecodeParmsItem = pDecodeParmsItem->next;
	}

	return 1;
}
