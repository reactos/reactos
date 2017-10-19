/*
 * Copyright (C) 2017 Alexandre Julliard
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

#ifndef _APISETLIBLOADER_
#define _APISETLIBLOADER_

#ifdef __cplusplus
extern "C" {
#endif

typedef void *DLL_DIRECTORY_COOKIE, **PDLL_DIRECTORY_COOKIE;

WINBASEAPI DLL_DIRECTORY_COOKIE WINAPI AddDllDirectory(const WCHAR *);
WINBASEAPI BOOL WINAPI RemoveDllDirectory(DLL_DIRECTORY_COOKIE);
WINBASEAPI BOOL WINAPI SetDefaultDllDirectories(DWORD);

#ifdef __cplusplus
}
#endif

#endif  /* _APISETLIBLOADER_ */
