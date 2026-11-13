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
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "myTernarySearchTree.h"

void tstInit(TernarySearchTree_t* pTree)
{
	pTree->pRoot = NULL;
}

tnode_t* tstInsertRecursive(tnode_t* p, const wchar_t *s, const void* pData, uint32_t dataSize, wchar_t* pReserved)
{
	if ( NULL == p )
	{
		p = (tnode_t*)malloc(sizeof(tnode_t));
		if ( NULL == p )
		{
			return NULL;
		}
		p->splitchar = *s;
		p->lokid = p->eqkid = p->hikid = NULL;
		//p->pData = NULL;
	}

	if ( *s < p->splitchar )
	{
		p->lokid = tstInsertRecursive(p->lokid, s, pData, dataSize, pReserved);
	}
	else if ( *s == p->splitchar )
	{
		//p->eqkid = tstInsertRecursive(p->eqkid, s, pData, dataSize, pReserved);

		if ( 0 != *s )
			p->eqkid = tstInsertRecursive(p->eqkid, ++s, pData, dataSize, pReserved);
	}
	else
	{
		p->hikid = tstInsertRecursive(p->hikid, s, pData, dataSize, pReserved);
	}

	return p;
}

void tstFreeRecursive(TernarySearchTree_t* pTree, tnode_t* p)
{
	if (p)
	{
		tstFreeRecursive(pTree, p->lokid);

		if (p->splitchar)
		{
			tstFreeRecursive(pTree, p->eqkid);
		}

		tstFreeRecursive(pTree, p->hikid);

		free(p);
    }

	pTree->pRoot = NULL;
}

uint32_t tstTraverseRecursive(tnode_t* p, pfnOnTraverseTst OnTraverse, uint32_t count)
{
	if (!p)
		return count;

	count = tstTraverseRecursive(p->lokid, OnTraverse, count);

	if (p->splitchar)
	{
		count = tstTraverseRecursive(p->eqkid, OnTraverse, count);
	}
	else
	{
		count++;
		if ( NULL != OnTraverse )
		{
			tdata_t* pDataStruct = (tdata_t*)p->eqkid;

			if ( !p->eqkid )
				return count;

			if ( !OnTraverse(pDataStruct->key, pDataStruct->data, pDataStruct->dataSize) )
				return count;
		}
	}

	count = tstTraverseRecursive(p->hikid, OnTraverse, count);

	return count;
}

uint32_t tstTraverseDescRecursive(tnode_t* p, pfnOnTraverseTst OnTraverse, uint32_t count)
{
	if (!p)
		return count;

	count = tstTraverseDescRecursive(p->hikid, OnTraverse, count);

	if (p->splitchar)
	{
		count = tstTraverseDescRecursive(p->eqkid, OnTraverse, count);
	}
	else
	{
		count++;
		if ( NULL != OnTraverse )
		{
			tdata_t* pDataStruct = (tdata_t*)p->eqkid;

			if ( !p->eqkid )
				return count;

			if ( !OnTraverse(pDataStruct->key, pDataStruct->data, pDataStruct->dataSize) )
				return count;
		}
	}

	count = tstTraverseDescRecursive(p->lokid, OnTraverse, count);

	return count;
}

uint32_t tstPartialMatchSearchRecursive(tnode_t* p, const wchar_t* s, pfnOnTraverseTst OnTraverse, uint32_t count)
{
	if (!p)
		return count;

	if (*s == L'.' || *s < p->splitchar)
		count = tstPartialMatchSearchRecursive(p->lokid, s, OnTraverse, count);

	if (*s == L'.' || *s == p->splitchar)
		if (p->splitchar && *s)
			count = tstPartialMatchSearchRecursive(p->eqkid, s + 1, OnTraverse, count);

	if (*s == 0 && p->splitchar == 0)
	{
		tdata_t* pDataStruct = (tdata_t*)p->eqkid;

		count++;

		if ( NULL != OnTraverse )
			if ( !OnTraverse(pDataStruct->key, pDataStruct->data, pDataStruct->dataSize) )
				return count;
	}

	if (*s == L'.' || *s > p->splitchar)
		count = tstPartialMatchSearchRecursive(p->hikid, s, OnTraverse, count);

	return count;
}

uint32_t tstNearNeighborSearchRecursive(tnode_t* p, const wchar_t* s, int d, pfnOnTraverseTst OnTraverse, uint32_t count)
{
	wchar_t c;
	int keylen = 0;

	if ( !p || d < 0 )
		return count;

	if ( d > 0 || *s < p->splitchar )
		count = tstNearNeighborSearchRecursive(p->lokid, s, d, OnTraverse, count);

	if ( 0 == p->splitchar )
	{
		keylen = 0;
		c = s[keylen];
		while ( L'\0' != c )
		{
			keylen++;
			c = s[keylen];
		}

		if ( keylen <= d )
		{
			tdata_t* pDataStruct = (tdata_t*)p->eqkid;

			count++;

			if ( NULL != OnTraverse )
				if ( !OnTraverse(pDataStruct->key, pDataStruct->data, pDataStruct->dataSize) )
					return count;
		}
	}
	else
	{
		count = tstNearNeighborSearchRecursive(p->eqkid, *s ? s + 1 : s, (*s == p->splitchar) ? d : d - 1, OnTraverse, count);
	}

	if (d > 0 || *s > p->splitchar)
		count = tstNearNeighborSearchRecursive(p->hikid, s, d, OnTraverse, count);

	return count;
}

tnode_t* tstPrefixSearchRoot(tnode_t* p, const wchar_t* s)
{
	if (!p)
		return NULL;

    if (*s < p->splitchar)
    {
        return tstPrefixSearchRoot(p->lokid, s);
	}
    else if (*s > p->splitchar)
    {
        return tstPrefixSearchRoot(p->hikid, s);
	}
    else
    {
        if ( 0 == *s )
        {
			return p;
		}
        return tstPrefixSearchRoot(p->eqkid, ++s);
    }
}

uint32_t tstPrefixSearch(tnode_t* p, const wchar_t* s, pfnOnTraverseTst OnTraverse)
{
	tnode_t* pPrefix = NULL;

	pPrefix = tstPrefixSearchRoot(p, s);
	if ( NULL != pPrefix )
	{
		return tstTraverseRecursive(pPrefix, OnTraverse, 0);
	}
	else
	{
		return 0;
	}
}

int tstSearchRecursive(tnode_t* p, const wchar_t* s, void* pData, uint32_t* dataSize)
{
	if (!p)
		return 0;

    if (*s < p->splitchar)
    {
        return tstSearchRecursive(p->lokid, s, pData, dataSize);
	}
    else if (*s > p->splitchar)
    {
        return tstSearchRecursive(p->hikid, s, pData, dataSize);
	}
    else
    {
        if (*s == 0)
        {
			if ( NULL != pData )
			{
				tdata_t* pDataStruct = (tdata_t*)p->eqkid;
				memcpy(pData, pDataStruct->data, pDataStruct->dataSize);
				*dataSize = pDataStruct->dataSize;
			}
			return 1;
		}
        return tstSearchRecursive(p->eqkid, ++s, pData, dataSize);
    }
}

void tst_cleanup(tnode_t* p)
{
	if (p)
	{
		tst_cleanup(p->lokid);

		if (p->splitchar)
			tst_cleanup(p->eqkid);

		tst_cleanup(p->hikid);

		free(p);
	}
}

int tstDeleteKey(TernarySearchTree_t* pTree, const wchar_t* s)
{
	tnode_t* p = pTree->pRoot;
	tnode_t* last_p = p;
	const wchar_t *last_s = s;

	tdata_t* pDataStruct = NULL;

	if (!s)
	{
		return 0;
	}

	while (p)
	{
		if (*s == p->splitchar)
		{
			if (*s++ == 0)
			{
				pDataStruct = (tdata_t*)p->eqkid;
				break;
			}
			p = p->eqkid;
		}
		else
		{
			last_s = s;
			if (*s < p->splitchar)
				p = p->lokid;
			else
				p = p->hikid;
			if (p->lokid || p->hikid)
			{
				last_p = p;
			}
		}
	}

	if (last_p && p) // the node holds common prefix
	{
		if (last_p->splitchar == *last_s)
		{
			if (last_p->eqkid)
			{
				if (last_p->eqkid->hikid)
				{
					tnode_t* tt = last_p->eqkid;
					last_p->eqkid = last_p->eqkid->hikid;
					free(tt);
				}
				else
				{
					tst_cleanup(last_p->eqkid);
					last_p->eqkid = NULL;
				}
			}
		}
		else if (last_p->hikid)
		{
			if (last_p->hikid->splitchar == *last_s)
			{
				tst_cleanup(last_p->hikid);
				last_p->hikid = NULL;
			}
			else if ((last_p->lokid) && (last_p->lokid->splitchar == *last_s))
			{
				tst_cleanup(last_p->lokid);
				last_p->lokid = NULL;
			}
			else if (last_p->eqkid)
			{
				goto eqkid;
			}
		}
		else if (last_p->lokid)
		{
			if (last_p->lokid->splitchar == *last_s)
			{
				tst_cleanup(last_p->lokid);
				last_p->lokid = NULL;
			}
		}
		else if (last_p->eqkid)
		{
			tnode_t* tt;
			eqkid:
			tt = last_p->eqkid;
			if (tt->splitchar == 0)
			{
				if (tt->hikid && tt->hikid->splitchar == *last_s)
				{
					tst_cleanup(tt->hikid);
					tt->hikid = NULL;
				}
			}
		}
	}

	if ( NULL != pDataStruct )
	{
		if ( NULL != pDataStruct->key )
			free(pDataStruct->key);
		pDataStruct->key = NULL;

		if ( NULL != pDataStruct->data )
			free(pDataStruct->data);
		pDataStruct->data = NULL;

		free(pDataStruct);
		pDataStruct = NULL;
	}

	return 1;
}
