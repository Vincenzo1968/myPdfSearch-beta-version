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

#ifndef	MYOBJREFQUEUELIST_INCLUDED
#define	MYOBJREFQUEUELIST_INCLUDED

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

typedef struct _MyObjRefItem
{
	char *szKey;
	int  numObjRef;
	struct _MyObjRefItem* next;
} MyObjRefItem_t;

typedef struct _MyObjRefList
{
	unsigned int count;
	MyObjRefItem_t* head;
	MyObjRefItem_t* tail;
} MyObjRefList_t;


BEGIN_C_DECLS

int  myobjreflist_Init(MyObjRefList_t* p);
void myobjreflist_Free(MyObjRefList_t* p);
int  myobjreflist_IsEmpty(MyObjRefList_t* p);
int  myobjreflist_Enqueue(MyObjRefList_t* p, const char *s, int num);
int  myobjreflist_Dequeue(MyObjRefList_t* p, char *s, int *pNum);
int  myobjreflist_Peek(MyObjRefList_t* p, char *s, int *pNum);

END_C_DECLS

#endif // MYOBJREFQUEUELIST_INCLUDED
