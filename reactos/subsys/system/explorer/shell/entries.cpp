/*
 * Copyright 2003 Martin Fuchs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


 //
 // Explorer clone
 //
 // entries.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#include "../utility/utility.h"
#include "../utility/shellclasses.h"
#include "../globals.h"	// for _prescan_nodes

#include "entries.h"


 // allocate and initialise a directory entry
Entry::Entry(ENTRY_TYPE etype)
 :	_etype(etype)
{
	_up = NULL;
	_next = NULL;
	_down = NULL;
	_expanded = false;
	_scanned = false;
	_bhfi_valid = false;
	_level = 0;
	_hIcon = 0;
}

Entry::Entry(Entry* parent)
 :	_up(parent),
	_etype(parent->_etype)
{
	_next = NULL;
	_down = NULL;
	_expanded = false;
	_scanned = false;
	_bhfi_valid = false;
	_level = 0;
	_hIcon = 0;
}

 // free a directory entry
Entry::~Entry()
{
	if (_hIcon && _hIcon!=(HICON)-1)
		DestroyIcon(_hIcon);
}


 // read directory tree and expand to the given location
Entry* Entry::read_tree(const void* path, SORT_ORDER sortOrder)
{
	HCURSOR old_cursor = SetCursor(LoadCursor(0, IDC_WAIT));

	Entry* entry = this;
	Entry* next_entry = entry;

	for(const void*p=path; p&&next_entry; p=entry->get_next_path_component(p)) {
		entry = next_entry;

		entry->read_directory(sortOrder);

		if (entry->_down)
			entry->_expanded = true;

		next_entry = entry->find_entry(p);
	}

	SetCursor(old_cursor);

	return entry;
}


void Entry::read_directory(SORT_ORDER sortOrder)
{
	 // call into subclass
	read_directory();

	if (g_Globals._prescan_nodes) {
		for(Entry*entry=_down; entry; entry=entry->_next)
			if (entry->_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				entry->read_directory();
				entry->sort_directory(sortOrder);
			}
	}

	sort_directory(sortOrder);
}


Root::Root()
{
	memset(this, 0, sizeof(Root));
}

Root::~Root()
{
	if (_entry)
		_entry->free_subentries();
}


 // directories first...
static int compareType(const WIN32_FIND_DATA* fd1, const WIN32_FIND_DATA* fd2)
{
	int dir1 = fd1->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
	int dir2 = fd2->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

	return dir2==dir1? 0: dir2<dir1? -1: 1;
}


static int compareName(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATA* fd1 = &(*(Entry**)arg1)->_data;
	const WIN32_FIND_DATA* fd2 = &(*(Entry**)arg2)->_data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	return lstrcmpi(fd1->cFileName, fd2->cFileName);
}

static int compareExt(const void* arg1, const void* arg2)
{
	const WIN32_FIND_DATA* fd1 = &(*(Entry**)arg1)->_data;
	const WIN32_FIND_DATA* fd2 = &(*(Entry**)arg2)->_data;
	const TCHAR *name1, *name2, *ext1, *ext2;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	name1 = fd1->cFileName;
	name2 = fd2->cFileName;

	ext1 = _tcsrchr(name1, TEXT('.'));
	ext2 = _tcsrchr(name2, TEXT('.'));

	if (ext1)
		++ext1;
	else
		ext1 = TEXT("");

	if (ext2)
		++ext2;
	else
		ext2 = TEXT("");

	cmp = lstrcmpi(ext1, ext2);
	if (cmp)
		return cmp;

	return lstrcmpi(name1, name2);
}

static int compareSize(const void* arg1, const void* arg2)
{
	WIN32_FIND_DATA* fd1 = &(*(Entry**)arg1)->_data;
	WIN32_FIND_DATA* fd2 = &(*(Entry**)arg2)->_data;

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
	WIN32_FIND_DATA* fd1 = &(*(Entry**)arg1)->_data;
	WIN32_FIND_DATA* fd2 = &(*(Entry**)arg2)->_data;

	int cmp = compareType(fd1, fd2);
	if (cmp)
		return cmp;

	return CompareFileTime(&fd2->ftLastWriteTime, &fd1->ftLastWriteTime);
}


static int (*sortFunctions[])(const void* arg1, const void* arg2) = {
	compareName,	// SORT_NAME
	compareExt, 	// SORT_EXT
	compareSize,	// SORT_SIZE
	compareDate 	// SORT_DATE
};


void Entry::sort_directory(SORT_ORDER sortOrder)
{
	Entry* entry = _down;
	Entry** array, **p;
	int len;

	len = 0;
	for(entry=_down; entry; entry=entry->_next)
		++len;

	if (len) {
		array = (Entry**) alloca(len*sizeof(Entry*));

		p = array;
		for(entry=_down; entry; entry=entry->_next)
			*p++ = entry;

		 // call qsort with the appropriate compare function
		qsort(array, len, sizeof(array[0]), sortFunctions[sortOrder]);

		_down = array[0];

		for(p=array; --len; p++)
			p[0]->_next = p[1];

		(*p)->_next = 0;
	}
}


void Entry::smart_scan()
{
	if (!_scanned) {
		free_subentries();
		read_directory(SORT_NAME);	// we could use IShellFolder2::GetDefaultColumn to determine sort order
	}
}


BOOL Entry::launch_entry(HWND hwnd, UINT nCmdShow)
{
	TCHAR cmd[MAX_PATH];

	get_path(cmd);

	  // start program, open document...
	return launch_file(hwnd, cmd, nCmdShow);
}


 // recursively free all child entries
void Entry::free_subentries()
{
	Entry *entry, *next=_down;

	if (next) {
		_down = 0;

		do {
			entry = next;
			next = entry->_next;

			entry->free_subentries();
			delete entry;
		} while(next);
	}
}
