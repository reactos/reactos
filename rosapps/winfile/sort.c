/*
 *  ReactOS winfile
 *
 *  sort.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include <shellapi.h>
//#include <winspool.h>
#include <windowsx.h>
#include <shellapi.h>
#include <ctype.h>
#include <assert.h>
#define ASSERT assert

#include "winfile.h"
#include "sort.h"



 // directories first...
static int compareType(const WIN32_FIND_DATA* fd1, const WIN32_FIND_DATA* fd2)
{
	int dir1 = fd1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
	int dir2 = fd2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

	return dir2==dir1? 0: dir2<dir1? -1: 1;
}


static int compareName(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATA* fd1 = &(*(Entry**)arg1)->data;
	const WIN32_FIND_DATA* fd2 = &(*(Entry**)arg2)->data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	return lstrcmpi(fd1->cFileName, fd2->cFileName);
}

static int compareExt(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATA* fd1 = &(*(Entry**)arg1)->data;
	const WIN32_FIND_DATA* fd2 = &(*(Entry**)arg2)->data;
	const TCHAR *name1, *name2, *ext1, *ext2;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	name1 = fd1->cFileName;
	name2 = fd2->cFileName;

	ext1 = _tcsrchr(name1, _T('.'));
	ext2 = _tcsrchr(name2, _T('.'));

	if (ext1)
		ext1++;
	else
		ext1 = _T("");

	if (ext2)
		ext2++;
	else
		ext2 = _T("");

	cmp = lstrcmpi(ext1, ext2);
	if (cmp)
		return cmp;

	return lstrcmpi(name1, name2);
}

static int compareSize(const void* arg1, const void* arg2)
{
	WIN32_FIND_DATA* fd1 = &(*(Entry**)arg1)->data;
	WIN32_FIND_DATA* fd2 = &(*(Entry**)arg2)->data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	cmp = fd2->nFileSizeHigh - fd1->nFileSizeHigh;

	if (cmp < 0)
		return -1;
	else if (cmp > 0)
		return 1;

	cmp = fd2->nFileSizeLow - fd1->nFileSizeLow;

	return cmp<0? -1: cmp>0? 1: 0;
}

static int compareDate(const void* arg1, const void* arg2)
{
	WIN32_FIND_DATA* fd1 = &(*(Entry**)arg1)->data;
	WIN32_FIND_DATA* fd2 = &(*(Entry**)arg2)->data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	return CompareFileTime(&fd2->ftLastWriteTime, &fd1->ftLastWriteTime);
}


static int (*sortFunctions[])(const void* arg1, const void* arg2) = {
	compareName,	// SORT_NAME
	compareExt,		// SORT_EXT
	compareSize,	// SORT_SIZE
	compareDate		// SORT_DATE
};


void SortDirectory(Entry* parent, SORT_ORDER sortOrder)
{
	Entry* entry = parent->down;
	Entry** array, **p;
	int len;

	len = 0;
	for(entry=parent->down; entry; entry=entry->next)
		len++;

	if (len) {
#ifdef _MSC_VER
		array = (Entry**) alloca(len*sizeof(Entry*));
#else
		array = (Entry**) malloc(len*sizeof(Entry*));
#endif
		p = array;
		for(entry=parent->down; entry; entry=entry->next)
			*p++ = entry;

		 // call qsort with the appropriate compare function
		qsort(array, len, sizeof(array[0]), sortFunctions[sortOrder]);

		parent->down = array[0];

		for(p=array; --len; p++)
			p[0]->next = p[1];

		(*p)->next = 0;
	}
}


