/*
 * Copyright (C) 2005 Francois Gouget
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

#ifndef __WINE_ADSHLP_H
#define __WINE_ADSHLP_H

#ifdef __cplusplus
extern "C" {
#endif

BOOL    WINAPI FreeADsMem(void*);
void*   WINAPI AllocADsMem(DWORD) __WINE_ALLOC_SIZE(1) __WINE_DEALLOC(FreeADsMem) __WINE_MALLOC;
void*   WINAPI ReallocADsMem(void*,DWORD,DWORD) __WINE_ALLOC_SIZE(3) __WINE_DEALLOC(FreeADsMem);
BOOL    WINAPI FreeADsStr(WCHAR*);
WCHAR*  WINAPI AllocADsStr(WCHAR*) __WINE_DEALLOC(FreeADsStr) __WINE_MALLOC;

HRESULT WINAPI ADsBuildEnumerator(IADsContainer*,IEnumVARIANT**);
HRESULT WINAPI ADsBuildVarArrayStr(LPWSTR*,DWORD,VARIANT*);
HRESULT WINAPI ADsEnumerateNext(IEnumVARIANT*,ULONG,VARIANT*,ULONG*);
HRESULT WINAPI ADsGetObject(LPCWSTR,REFIID,VOID**);
HRESULT WINAPI ADsOpenObject(LPCWSTR,LPCWSTR,LPCWSTR,DWORD,REFIID,VOID**);

#ifdef __cplusplus
}
#endif

#endif
