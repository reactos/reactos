/*
 *  ReactOS winfile
 *
 *  splitpath.c
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

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
    
#include "main.h"


#ifdef UNICODE

void _wsplitpath(const WCHAR* path, WCHAR* drv, WCHAR* dir, WCHAR* name, WCHAR* ext)
{
	const WCHAR* end; // end of processed string
	const WCHAR* p;	  // search pointer
	const WCHAR* s;	  // copy pointer

	 // extract drive name
	if (path[0] && path[1]==':') {
		if (drv) {
			*drv++ = *path++;
			*drv++ = *path++;
			*drv = L'\0';
		}
	} else if (drv)
		*drv = L'\0';

	 // search for end of string or stream separator
	for(end=path; *end && *end!=L':'; )
		end++;

	 // search for begin of file extension
	for(p=end; p>path && *--p!=L'\\' && *p!=L'/'; )
		if (*p == L'.') {
			end = p;
			break;
		}

	if (ext)
		for(s=end; *ext=*s++; )
			ext++;

	 // search for end of directory name
	for(p=end; p>path; )
		if (*--p=='\\' || *p=='/') {
			p++;
			break;
		}

	if (name) {
		for(s=p; s<end; )
			*name++ = *s++;

		*name = L'\0';
	}

	if (dir) {
		for(s=path; s<p; )
			*dir++ = *s++;

		*dir = L'\0';
	}
}

#else

void _splitpath(const CHAR* path, CHAR* drv, CHAR* dir, CHAR* name, CHAR* ext)
{
	const CHAR* end; // end of processed string
	const CHAR* p;	 // search pointer
	const CHAR* s;	 // copy pointer

	 // extract drive name
	if (path[0] && path[1]==':') {
		if (drv) {
			*drv++ = *path++;
			*drv++ = *path++;
			*drv = '\0';
		}
	} else if (drv)
		*drv = '\0';

	 // search for end of string or stream separator
	for(end=path; *end && *end!=':'; )
		end++;

	 // search for begin of file extension
	for(p=end; p>path && *--p!='\\' && *p!='/'; )
		if (*p == '.') {
			end = p;
			break;
		}

	if (ext)
		for(s=end; (*ext=*s++); )
			ext++;

	 // search for end of directory name
	for(p=end; p>path; )
		if (*--p=='\\' || *p=='/') {
			p++;
			break;
		}

	if (name) {
		for(s=p; s<end; )
			*name++ = *s++;

		*name = '\0';
	}

	if (dir) {
		for(s=path; s<p; )
			*dir++ = *s++;

		*dir = '\0';
	}
}

#endif

/*
void main()	// test splipath()
{
	TCHAR drv[_MAX_DRIVE+1], dir[_MAX_DIR], name[_MAX_FNAME], ext[_MAX_EXT];

	_tsplitpath(L"x\\y", drv, dir, name, ext);
	_tsplitpath(L"x\\", drv, dir, name, ext);
	_tsplitpath(L"\\x", drv, dir, name, ext);
	_tsplitpath(L"x", drv, dir, name, ext);
	_tsplitpath(L"", drv, dir, name, ext);
	_tsplitpath(L".x", drv, dir, name, ext);
	_tsplitpath(L":x", drv, dir, name, ext);
	_tsplitpath(L"a:x", drv, dir, name, ext);
	_tsplitpath(L"a.b:x", drv, dir, name, ext);
	_tsplitpath(L"W:\\/\\abc/Z:~", drv, dir, name, ext);
	_tsplitpath(L"abc.EFGH:12345", drv, dir, name, ext);
	_tsplitpath(L"C:/dos/command.com", drv, dir, name, ext);
}
*/
