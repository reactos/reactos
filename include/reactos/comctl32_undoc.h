/*
 * ReactOS undocumented shell interface
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
 * Copyright 2013 Dominik Hornung
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

#ifndef __COMCTL32_UNDOC__H
#define __COMCTL32_UNDOC__H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

typedef struct tagCREATEMRULISTA
{
    DWORD cbSize;
    DWORD nMaxItems;
    DWORD dwFlags;
    HKEY hKey;
    LPCSTR lpszSubKey;
    PROC lpfnCompare;
} CREATEMRULISTA, *LPCREATEMRULISTA;
typedef struct tagCREATEMRULISTW
{
    DWORD cbSize;
    DWORD nMaxItems;
    DWORD dwFlags;
    HKEY hKey;
    LPCWSTR lpszSubKey;
    PROC lpfnCompare;
} CREATEMRULISTW, *LPCREATEMRULISTW;

#define MRU_STRING  0x0
#define MRU_BINARY  0x1
#define MRU_CACHEWRITE  0x2

HANDLE WINAPI CreateMRUListW(LPCREATEMRULISTW);
HANDLE WINAPI CreateMRUListA(LPCREATEMRULISTA);
INT WINAPI AddMRUData(HANDLE,LPCVOID,DWORD);
INT WINAPI FindMRUData(HANDLE,LPCVOID,DWORD,LPINT);
VOID WINAPI FreeMRUList(HANDLE);

INT WINAPI AddMRUStringW(HANDLE hList, LPCWSTR lpszString);
INT WINAPI AddMRUStringA(HANDLE hList, LPCSTR lpszString);
BOOL WINAPI DelMRUString(HANDLE hList, INT nItemPos);
INT WINAPI FindMRUStringW(HANDLE hList, LPCWSTR lpszString, LPINT lpRegNum);
INT WINAPI FindMRUStringA(HANDLE hList, LPCSTR lpszString, LPINT lpRegNum);
HANDLE WINAPI CreateMRUListLazyW(const CREATEMRULISTW *lpcml, DWORD dwParam2,
                                  DWORD dwParam3, DWORD dwParam4);
HANDLE WINAPI CreateMRUListLazyA(const CREATEMRULISTA *lpcml, DWORD dwParam2,
                                  DWORD dwParam3, DWORD dwParam4);
INT WINAPI EnumMRUListW(HANDLE hList, INT nItemPos, LPVOID lpBuffer,
                         DWORD nBufferSize);
INT WINAPI EnumMRUListA(HANDLE hList, INT nItemPos, LPVOID lpBuffer,
                         DWORD nBufferSize);

#ifdef UNICODE
typedef CREATEMRULISTW CREATEMRULIST, *PCREATEMRULIST;
#define CreateMRUList   CreateMRUListW
#else
typedef CREATEMRULISTA CREATEMRULIST, *PCREATEMRULIST;
#define CreateMRUList   CreateMRUListA
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif // __COMCTL32_UNDOC__H
