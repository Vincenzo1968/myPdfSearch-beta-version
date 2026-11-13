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

#ifndef	MYCONTENTQUEUELIST_INCLUDED
#define	MYCONTENTQUEUELIST_INCLUDED

#ifdef __cplusplus
#  define BEGIN_C_DECLS extern "C" {
#  define END_C_DECLS   }
#else // ! 
#  define BEGIN_C_DECLS
#  define END_C_DECLS
#endif //  

#ifndef MAX_STRLEN
#define MAX_STRLEN 4096
#endif

#include <stdint.h>

#include "mystringqueuelist.h"
#include "mydictionaryqueuelist.h"

//The following example shows a stream, containing the marking instructions for a page, that was
//compressed using the LZW compression method and then encoded in ASCII base-85 representation.

// 1 0 obj
// < /Length 534
// /Filter [ /ASCII85Decode /LZWDecode]
// >>

// If there are multiple filters and any of the filters has parameters set on undefined values,
// DecodeParms must be an array with a voice for each filter: the parameter dictionary for that filter
// or the null object if that filter does not have parameters (or if all its parameters have their default values).

typedef struct _myContent
{
	int bExternalFile;

    // Identifying the PDF object containing the stream.
    uint32_t ObjectNumber;
    uint32_t GenerationNumber;

	unsigned long int LengthFromPdf;
	//unsigned long int Length;
	unsigned long int Offset;
	MyStringQueueList_t queueFilters;
	MyDictionaryQueueList_t decodeParms; // See "Stream extension" on page 20 of PDF3000 2008. pdf
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
