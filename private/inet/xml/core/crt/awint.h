/***
*awint.h - internal definitions for A&W Win32 wrapper routines.
*
* Copyright (c) 1994 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       Contains internal definitions/declarations for A&W wrapper functions.
*       Not included in internal.h since windows.h is required for these.
*
*       [Internal]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifdef _WIN32

#ifndef _INC_AWINC
#define _INC_AWINC

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <windows.h>

/* internal A&W routines */

int __cdecl __crtCompareStringW(LCID, DWORD, LPCWSTR, int, LPCWSTR, int, int);
int __cdecl __crtCompareStringA(LCID, DWORD, LPCSTR, int, LPCSTR, int, int);

int __cdecl __crtGetLocaleInfoW(LCID, LCTYPE, LPWSTR, int, int);
int __cdecl __crtGetLocaleInfoA(LCID, LCTYPE, LPSTR, int, int);

int __cdecl __crtLCMapStringW(LCID, DWORD, LPCWSTR, int, LPWSTR, int, int);
int __cdecl __crtLCMapStringA(LCID, DWORD, LPCSTR, int, LPSTR, int, int);

BOOL __cdecl __crtGetStringTypeW(DWORD, LPCWSTR, int, LPWORD, int, int);
BOOL __cdecl __crtGetStringTypeA(DWORD, LPCSTR, int, LPWORD, int, int);

LPVOID __cdecl __crtGetEnvironmentStringsW(VOID);
LPVOID __cdecl __crtGetEnvironmentStringsA(VOID);

LPWSTR __cdecl __crtGetCommandLineW(VOID);

int __cdecl __crtMessageBoxA(LPCSTR, LPCSTR, UINT);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _INC_AWINC */

#endif  /* _WIN32 */
