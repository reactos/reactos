/*
 * ReactOS undocumented shell interface
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
 * Copyright 2013 Dominik Hornung
 * Copyright 2020 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

/*
 * See also:
 * https://docs.microsoft.com/en-us/windows/win32/shell/mruinfo
 */

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

// MRUINFOA.lpfnCompare for MRU_STRING
typedef INT (CALLBACK *MRUCMPPROCA)(LPCSTR, LPCSTR);
// MRUINFOW.lpfnCompare for MRU_STRING
typedef INT (CALLBACK *MRUCMPPROCW)(LPCWSTR, LPCWSTR);
// MRUINFO.lpfnCompare for MRU_BINARY
typedef INT (CALLBACK *MRUBINARYCMPPROC)(LPCVOID, LPCVOID, DWORD);

typedef struct _MRUINFOA
{
    DWORD cbSize;
    DWORD uMax;
    DWORD fFlags;
    HKEY hKey;
    LPCSTR lpszSubKey;
    MRUCMPPROCA lpfnCompare;
} MRUINFOA, *LPMRUINFOA;
typedef struct _MRUINFOW
{
    DWORD cbSize;
    DWORD uMax;
    DWORD fFlags;
    HKEY hKey;
    LPCWSTR lpszSubKey;
    MRUCMPPROCW lpfnCompare;
} MRUINFOW, *LPMRUINFOW;

#ifdef UNICODE
    #define MRUINFO MRUINFOW
    #define LPMRUINFO LPMRUINFOW
#else
    #define MRUINFO MRUINFOA
    #define LPMRUINFO LPMRUINFOA
#endif

/* MRUINFO.fFlags */
#define MRU_STRING 0x0
#define MRU_BINARY 0x1
#define MRU_CACHEWRITE 0x2

#ifndef NO_MRU_IMPORTS

HANDLE WINAPI CreateMRUListA(LPMRUINFOA);
HANDLE WINAPI CreateMRUListW(LPMRUINFOW);
HANDLE WINAPI CreateMRUListLazyW(const MRUINFOW *lpcml, DWORD dwParam2,
                                 DWORD dwParam3, DWORD dwParam4);
HANDLE WINAPI CreateMRUListLazyA(const MRUINFOA *lpcml, DWORD dwParam2,
                                 DWORD dwParam3, DWORD dwParam4);
INT WINAPI AddMRUData(HANDLE, LPCVOID, DWORD);
INT WINAPI AddMRUStringA(HANDLE hList, LPCSTR lpszString);
INT WINAPI AddMRUStringW(HANDLE hList, LPCWSTR lpszString);
INT WINAPI EnumMRUListA(HANDLE hList, INT nItemPos, LPVOID lpBuffer,
                        DWORD nBufferSize);
INT WINAPI EnumMRUListW(HANDLE hList, INT nItemPos, LPVOID lpBuffer,
                        DWORD nBufferSize);
INT WINAPI FindMRUData(HANDLE,LPCVOID,DWORD,LPINT);
INT WINAPI FindMRUStringW(HANDLE hList, LPCWSTR lpszString, LPINT lpRegNum);
INT WINAPI FindMRUStringA(HANDLE hList, LPCSTR lpszString, LPINT lpRegNum);
BOOL WINAPI DelMRUString(HANDLE hList, INT nItemPos);
VOID WINAPI FreeMRUList(HANDLE);

#ifdef UNICODE
    #define CreateMRUList CreateMRUListW
    #define CreateMRUListLazy CreateMRUListLazyW
    #define AddMRUString AddMRUStringW
    #define EnumMRUList EnumMRUListW
    #define FindMRUString FindMRUStringW
#else
    #define CreateMRUList CreateMRUListA
    #define CreateMRUListLazy CreateMRUListLazyA
    #define AddMRUString AddMRUStringA
    #define EnumMRUList EnumMRUListA
    #define FindMRUString FindMRUStringA
#endif

#endif  /* ndef NO_MRU_IMPORTS */

typedef HANDLE (WINAPI *FN_CreateMRUListA)(const MRUINFOA *);
typedef HANDLE (WINAPI *FN_CreateMRUListW)(const MRUINFOW *);
typedef HANDLE (WINAPI *FN_CreateMRUListLazyA)(const MRUINFOA *, DWORD, DWORD, DWORD);
typedef HANDLE (WINAPI *FN_CreateMRUListLazyW)(const MRUINFOW *, DWORD, DWORD, DWORD);
typedef INT (WINAPI *FN_AddMRUStringA)(HANDLE, LPCSTR);
typedef INT (WINAPI *FN_AddMRUStringW)(HANDLE, LPCWSTR);
typedef INT (WINAPI *FN_AddMRUData)(HANDLE, LPCVOID, DWORD);
typedef INT (WINAPI *FN_EnumMRUListA)(HANDLE, INT, LPVOID, DWORD);
typedef INT (WINAPI *FN_EnumMRUListW)(HANDLE, INT, LPVOID, DWORD);
typedef INT (WINAPI *FN_FindMRUData)(HANDLE, LPCVOID, DWORD, LPINT);
typedef INT (WINAPI *FN_FindMRUStringA)(HANDLE, LPCSTR, LPINT);
typedef INT (WINAPI *FN_FindMRUStringW)(HANDLE, LPCWSTR, LPINT);
typedef BOOL (WINAPI *FN_DelMRUString)(HANDLE, INT);
typedef INT (WINAPI *FN_FreeMRUList)(HANDLE);

#define I_CreateMRUListA 151
#define I_CreateMRUListW 400
#define I_CreateMRUListLazyA 157
#define I_CreateMRUListLazyW 404
#define I_AddMRUStringA 153
#define I_AddMRUStringW 401
#define I_AddMRUData 167
#define I_EnumMRUListA 154
#define I_EnumMRUListW 403
#define I_FindMRUData 169
#define I_FindMRUStringA 155
#define I_FindMRUStringW 402
#define I_DelMRUString 156
#define I_FreeMRUList 152

// #define GET_PROC(hComCtl32, fn) fn = (FN_##fn)GetProcAddress((hComCtl32), (LPSTR)I_##fn)

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif // __COMCTL32_UNDOC__H
