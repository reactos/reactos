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
 // unixfs.cpp
 //
 // Martin Fuchs, 23.07.2003
 //


#ifdef __linux__

#include "../utility/utility.h"
#include "../utility/shellclasses.h"

#include "entries.h"
#include "unixfs.h"

 // for UnixDirectory::read_directory()
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>


void UnixDirectory::read_directory(LPCTSTR path)
{
	Entry* first_entry = NULL;
	Entry* last = NULL;
	Entry* entry;

	int level = _level + 1;

	DIR* pdir = opendir(path);

	if (pdir) {
		struct stat st;
		struct dirent* ent;
		TCHAR buffer[MAX_PATH], *p;

		for(p=buffer; *path; )
			*p++ = *path++;

		if (p==buffer || p[-1]!='/')
			*p++ = '/';

		while((ent=readdir(pdir))) {
			if (w32fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				entry = new UnixDirectory(this, buffer);
			else
				entry = new UnixEntry(this);

			if (!first_entry)
				first_entry = entry;

			if (last)
				last->next = entry;

			lstrcpy(entry->data.cFileName, ent->d_name);
			entry->data.dwFileAttributes = ent->d_name[0]=='.'? FILE_ATTRIBUTE_HIDDEN: 0;

			strcpy(p, ent->d_name);

			if (!stat(buffer, &st)) {
				if (S_ISDIR(st.st_mode))
					entry->data.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

				entry->data.nFileSizeLow = st.st_size & 0xFFFFFFFF;
				entry->data.nFileSizeHigh = st.st_size >> 32;

				memset(&entry->data.ftCreationTime, 0, sizeof(FILETIME));
				time_to_filetime(&st.st_atime, &entry->data.ftLastAccessTime);
				time_to_filetime(&st.st_mtime, &entry->data.ftLastWriteTime);

				entry->bhfi.nFileIndexLow = ent->d_ino;
				entry->bhfi.nFileIndexHigh = 0;

				entry->bhfi.nNumberOfLinks = st.st_nlink;

				entry->bhfi_valid = TRUE;
			} else {
				entry->data.nFileSizeLow = 0;
				entry->data.nFileSizeHigh = 0;
				entry->bhfi_valid = FALSE;
			}

			entry->down = NULL;
			entry->up = dir;
			entry->expanded = FALSE;
			entry->scanned = FALSE;
			entry->level = level;

			last = entry;
		}

		last->next = NULL;

		closedir(pdir);
	}

	_down = first_entry;
	_scanned = true;
}


const void* UnixDirectory::get_next_path_component(const void* p)
{
	LPCTSTR s = (LPCTSTR) p;

	while(*s && *s!=TEXT('/'))
		++s;

	while(*s == TEXT('/'))
		++s;

	if (!*s)
		return NULL;

	return s;
}


Entry* UnixDirectory::find_entry(const void* p)
{
	LPCTSTR name = (LPCTSTR)p;

	for(Entry*entry=_down; entry; entry=entry->next) {
		LPCTSTR p = name;
		LPCTSTR q = entry->data.cFileName;

		do {
			if (!*p || *p==TEXT('/'))
				return entry;
		} while(*p++ == *q++);
	}

	return NULL;
}


 // get full path of specified directory entry
void UnixEntry::get_path(PTSTR path)
{
	int level = 0;
	int len = 0;

	for(Entry* entry=this; entry; level++) {
		LPCTSTR name = entry->_data.cFileName;
		int l = 0;

		for(LPCTSTR s=name; *s && *s!=TEXT('/'); s++)
			++l;

		if (entry->_up) {
			if (l > 0) {
				memmove(path+l+1, path, len*sizeof(TCHAR));
				memcpy(path+1, name, l*sizeof(TCHAR));
				len += l+1;

				path[0] = TEXT('/');
			}

			entry = entry->_up;
		} else {
			memmove(path+l, path, len*sizeof(TCHAR));
			memcpy(path, name, l*sizeof(TCHAR));
			len += l;
			break;
		}
	}

	if (!level)
		path[len++] = TEXT('/');

	path[len] = TEXT('\0');
}

#endif // __linux__
