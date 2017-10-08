/*
 * Copyright 2000, 2004 Martin Fuchs
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "winefile.h"


void _wsplitpath(const WCHAR* path, WCHAR* drv, WCHAR* dir, WCHAR* name, WCHAR* ext)
{
        const WCHAR* end; /* end of processed string */
	const WCHAR* p;	  /* search pointer */
	const WCHAR* s;	  /* copy pointer */

	/* extract drive name */
	if (path[0] && path[1]==':') {
		if (drv) {
			*drv++ = *path++;
			*drv++ = *path++;
			*drv = '\0';
		}
	} else if (drv)
		*drv = '\0';

    /* Don't parse colons as stream separators when splitting paths */
    end = path + lstrlenW(path);

	/* search for begin of file extension */
	for(p=end; p>path && *--p!='\\' && *p!='/'; )
		if (*p == '.') {
			end = p;
			break;
		}

	if (ext)
		for(s=end; (*ext=*s++); )
			ext++;

	/* search for end of directory name */
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


/*
void main()	// test splipath()
{
	WCHAR drv[_MAX_DRIVE+1], dir[_MAX_DIR], name[_MAX_FNAME], ext[_MAX_EXT];

	_wsplitpath(L"x\\y", drv, dir, name, ext);
	_wsplitpath(L"x\\", drv, dir, name, ext);
	_wsplitpath(L"\\x", drv, dir, name, ext);
	_wsplitpath(L"x", drv, dir, name, ext);
	_wsplitpath(L"", drv, dir, name, ext);
	_wsplitpath(L".x", drv, dir, name, ext);
	_wsplitpath(L":x", drv, dir, name, ext);
	_wsplitpath(L"a:x", drv, dir, name, ext);
	_wsplitpath(L"a.b:x", drv, dir, name, ext);
	_wsplitpath(L"W:\\/\\abc/Z:~", drv, dir, name, ext);
	_wsplitpath(L"abc.EFGH:12345", drv, dir, name, ext);
	_wsplitpath(L"C:/dos/command.com", drv, dir, name, ext);
}
*/
