/*
 *  ReactOS shell32 - 
 *
 *  shell32.h
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

#ifndef __SHELL32_H__
#define __SHELL32_H__


#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>

#ifdef _MSC_VER
#define inline
#pragma warning (disable:4273) // : inconsistent dll linkage.  dllexport assumed.
#pragma warning (disable:4018) // : signed/unsigned mismatch
#pragma warning (disable:4141) // : 'dllexport' : used more than once

#undef WINAPI
#define WINAPI  __declspec(dllexport) 
#define STDCALL __stdcall
#define WINBOOL BOOL

#else

//#define WINAPI STDCALL 

typedef struct _SHQUERYRBINFO {
    DWORD cbSize;     
    __int64 i64Size;
    __int64 i64NumItems;
} SHQUERYRBINFO, *LPSHQUERYRBINFO;

#define DWORD_PTR DWORD*

/*
#define STDAPI long __stdcall

typedef struct _SHELLEXECUTEINFO{
    DWORD cbSize; 
    ULONG fMask; 
    HWND hwnd; 
    LPCTSTR lpVerb; 
    LPCTSTR lpFile; 
    LPCTSTR lpParameters; 
    LPCTSTR lpDirectory; 
    int nShow; 
    HINSTANCE hInstApp; 
 
    // Optional members 
    LPVOID lpIDList; 
    LPCSTR lpClass; 
    HKEY hkeyClass; 
    DWORD dwHotKey; 
	union {
		HANDLE hIcon;
		HANDLE hMonitor;
	};
    HANDLE hProcess; 
} SHELLEXECUTEINFO, *LPSHELLEXECUTEINFO; 
 
typedef struct _NOTIFYICONDATA { 
    DWORD cbSize; 
    HWND hWnd; 
    UINT uID; 
    UINT uFlags; 
    UINT uCallbackMessage; 
    HICON hIcon; 
    TCHAR szTip[64];
    DWORD dwState; //Version 5.0
    DWORD dwStateMask; //Version 5.0
    TCHAR szInfo[256]; //Version 5.0
    union {
        UINT  uTimeout; //Version 5.0
        UINT  uVersion; //Version 5.0
    } DUMMYUNIONNAME;
    TCHAR szInfoTitle[64]; //Version 5.0
    DWORD dwInfoFlags; //Version 5.0
} NOTIFYICONDATA, *PNOTIFYICONDATA; 
 
 */
/*
 */
#endif


#endif  /* __SHELL32_H__ */
