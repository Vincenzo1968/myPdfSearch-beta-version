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

#ifndef	MYCONTENTQUEUELIST_INCLUDED
#define	MYCONTENTQUEUELIST_INCLUDED

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

#include <stdint.h>

#include "mystringqueuelist.h"
#include "mydictionaryqueuelist.h"

//The following example shows a stream, containing the marking instructions for a page, that was
//compressed using the LZW compression method and then encoded in ASCII base-85 representation.

// 1 0 obj
// << /Length 534
// /Filter [ /ASCII85Decode /LZWDecode ]
// >>

// Se ci sono più filtri e uno qualsiasi dei filtri ha parametri impostati su valori non predefiniti,
// DecodeParms deve essere un array con una voce per ciascun filtro: il dizionario dei parametri per quel filtro
// o l'oggetto null se quel filtro non ha parametri (o se tutti i suoi parametri hanno i loro valori di default).

typedef struct _myContent
{
	int bExternalFile;

    // Identificativo dell'oggetto PDF che contiene lo stream.
    uint32_t ObjectNumber;
    uint32_t GenerationNumber;

	unsigned long int LengthFromPdf;
	//unsigned long int Length;
	unsigned long int Offset;
	MyStringQueueList_t queueFilters;
	MyDictionaryQueueList_t decodeParms; // Vedi "Stream extent" a pag. 20 di PDF3000_2008.pdf
} MyContent_t;

typedef struct _MyContentQueueItem
{
	MyContent_t myContent;
	struct _MyContentQueueItem* next;
} MyContentQueueItem_t;

typedef struct _MyContentQueueList
{
	unsigned int count;
	MyContentQueueItem_t* head;
	MyContentQueueItem_t* tail;
} MyContentQueueList_t;


BEGIN_C_DECLS

int  mycontentqueuelist_Init(MyContentQueueList_t* p);
void mycontentqueuelist_Free(MyContentQueueList_t* p);
int  mycontentqueuelist_IsEmpty(MyContentQueueList_t* p);
int  mycontentqueuelist_Enqueue(MyContentQueueList_t* p, MyContent_t* pContent);
int  mycontentqueuelist_Dequeue(MyContentQueueList_t* p, MyContent_t* pContent);
int  mycontentqueuelist_Peek(MyContentQueueList_t* p, MyContent_t* pContent);

END_C_DECLS

#endif // MYCONTENTQUEUELIST_INCLUDED 
