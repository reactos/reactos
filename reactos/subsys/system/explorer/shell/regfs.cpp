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
 // ntobjfs.cpp
 //
 // Martin Fuchs, 31.01.2004
 //


#include "../utility/utility.h"
#include "../utility/shellclasses.h"

#include "entries.h"
#include "regfs.h"


void RegDirectory::read_directory(int scan_flags)
{
	CONTEXT("RegDirectory::read_directory()");

	Entry* first_entry = NULL;
	int level = _level + 1;

	TCHAR buffer[MAX_PATH];

	_tcscpy(buffer, (LPCTSTR)_path);
	LPTSTR p = buffer + _tcslen(buffer);

	HKEY hKey;

	if (!RegOpenKeyEx(_hKeyRoot, *buffer=='\\'?buffer+1:buffer, 0, STANDARD_RIGHTS_READ|KEY_QUERY_VALUE|KEY_ENUMERATE_SUB_KEYS, &hKey)) {
		if (p[-1] != '\\')
			*p++ = '\\';

		TCHAR name[MAX_PATH], class_name[MAX_PATH];
		WIN32_FIND_DATA w32fd;
		Entry* last = NULL;
		RegEntry* entry;

		for(int idx=0; ; ++idx) {
			memset(&w32fd, 0, sizeof(WIN32_FIND_DATA));

			DWORD name_len = MAX_PATH;
			DWORD class_len = MAX_PATH;

			if (RegEnumKeyEx(hKey, idx, name, &name_len, 0, class_name, &class_len, &w32fd.ftLastWriteTime))
				break;

			w32fd.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

			_tcscpy(p, name);
			///@todo class_name -> _entry->_class_name

			lstrcpy(w32fd.cFileName, p);

			entry = new RegDirectory(this, buffer, _hKeyRoot);

			memcpy(&entry->_data, &w32fd, sizeof(WIN32_FIND_DATA));

			if (!first_entry)
				first_entry = entry;

			if (last)
				last->_next = entry;

			entry->_down = NULL;
			entry->_expanded = false;
			entry->_scanned = false;
			entry->_level = level;

			last = entry;
		}

		DWORD type;
		for(int idx=0; ; ++idx) {
			DWORD name_len = MAX_PATH;

			if (RegEnumValue(hKey, idx, name, &name_len, 0, &type, NULL, NULL))
				break;

			memset(&w32fd, 0, sizeof(WIN32_FIND_DATA));

			_tcscpy(p, name);
			///@todo type -> _entry->_class_name

			lstrcpy(w32fd.cFileName, p);

			entry = new RegEntry(this);

			memcpy(&entry->_data, &w32fd, sizeof(WIN32_FIND_DATA));

			if (!first_entry)
				first_entry = entry;

			if (last)
				last->_next = entry;

			entry->_down = NULL;
			entry->_expanded = false;
			entry->_scanned = false;
			entry->_level = level;

			last = entry;
		}

		if (last)
			last->_next = NULL;

		RegCloseKey(hKey);
	}

	_down = first_entry;
	_scanned = true;
}


Entry* RegDirectory::find_entry(const void* p)
{
	LPCTSTR name = (LPCTSTR)p;

	for(Entry*entry=_down; entry; entry=entry->_next) {
		LPCTSTR p = name;
		LPCTSTR q = entry->_data.cFileName;

		do {
			if (!*p || *p==TEXT('\\') || *p==TEXT('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));

		p = name;
		q = entry->_data.cAlternateFileName;

		do {
			if (!*p || *p==TEXT('\\') || *p==TEXT('/'))
				return entry;
		} while(tolower(*p++) == tolower(*q++));
	}

	return NULL;
}


 // get full path of specified registry entry
bool RegEntry::get_path(PTSTR path) const
{
	int level = 0;
	int len = 0;
	int l = 0;
	LPCTSTR name = NULL;
	TCHAR buffer[MAX_PATH];

	const Entry* entry;
	for(entry=this; entry; level++) {
		l = 0;

		if (entry->_etype == ET_REGISTRY) {
			name = entry->_data.cFileName;

			for(LPCTSTR s=name; *s && *s!=TEXT('/') && *s!=TEXT('\\'); s++)
				++l;

			if (!entry->_up)
				break;
		} else {
			if (entry->get_path(buffer)) {
				l = _tcslen(buffer);
				name = buffer;

				/* special handling of drive names */
				if (l>0 && buffer[l-1]=='\\' && path[0]=='\\')
					--l;

				memmove(path+l, path, len*sizeof(TCHAR));
				memcpy(path, name, l*sizeof(TCHAR));
				len += l;
			}

			entry = NULL;
			break;
		}

		if (l > 0) {
			memmove(path+l+1, path, len*sizeof(TCHAR));
			memcpy(path+1, name, l*sizeof(TCHAR));
			len += l+1;

			path[0] = TEXT('\\');
		}

		entry = entry->_up;
	}

	if (entry) {
		memmove(path+l, path, len*sizeof(TCHAR));
		memcpy(path, name, l*sizeof(TCHAR));
		len += l;
	}

	if (!level)
		path[len++] = TEXT('\\');

	path[len] = TEXT('\0');

	return true;
}

BOOL RegEntry::launch_entry(HWND hwnd, UINT nCmdShow)
{
	return FALSE;
}


RegDirectory::RegDirectory(Entry* parent, LPCTSTR path, HKEY hKeyRoot)
 :	RegEntry(parent),
	_hKeyRoot(hKeyRoot)
{
	_path = _tcsdup(path);

	memset(&_data, 0, sizeof(WIN32_FIND_DATA));
	_data.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
}


void RegistryRoot::read_directory(int scan_flags)
{
	Entry *entry, *last, *first_entry;
	int level = _level + 1;

	_data.dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;

	entry = new RegDirectory(this, TEXT("\\"), HKEY_CURRENT_USER);
	_tcscpy(entry->_data.cFileName, TEXT("HKEY_CURRENT_USER"));
	entry->_level = level;

	first_entry = entry;
	last = entry;

	entry = new RegDirectory(this, TEXT("\\"), HKEY_LOCAL_MACHINE);
	_tcscpy(entry->_data.cFileName, TEXT("HKEY_LOCAL_MACHINE"));
	entry->_level = level;

	last->_next = entry;
	last = entry;

	entry = new RegDirectory(this, TEXT("\\"), HKEY_CLASSES_ROOT);
	_tcscpy(entry->_data.cFileName, TEXT("HKEY_CLASSES_ROOT"));
	entry->_level = level;

	last->_next = entry;
	last = entry;

	entry = new RegDirectory(this, TEXT("\\"), HKEY_USERS);
	_tcscpy(entry->_data.cFileName, TEXT("HKEY_USERS"));
	entry->_level = level;

	last->_next = entry;
	last = entry;
/*
	entry = new RegDirectory(this, TEXT("\\"), HKEY_PERFORMANCE_DATA);
	_tcscpy(entry->_data.cFileName, TEXT("HKEY_PERFORMANCE_DATA"));
	entry->_level = level;

	last->_next = entry;
	last = entry;
*/
	entry = new RegDirectory(this, TEXT("\\"), HKEY_CURRENT_CONFIG);
	_tcscpy(entry->_data.cFileName, TEXT("HKEY_CURRENT_CONFIG"));
	entry->_level = level;

	last->_next = entry;
	last = entry;
/*
	entry = new RegDirectory(this, TEXT("\\"), HKEY_DYN_DATA);
	_tcscpy(entry->_data.cFileName, TEXT("HKEY_DYN_DATA"));
	entry->_level = level;

	last->_next = entry;
	last = entry;
*/
	last->_next = NULL;

	_down = first_entry;
	_scanned = true;
}
