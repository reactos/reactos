/***
*awint.h - internal definitions for A&W Win32 wrapper routines.
*
*	Copyright (c) 1994-1997, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	Contains internal definitions/declarations for A&W wrapper functions.
*       Not included in internal.h since windows.h is required for these.
*
*	[Internal]
*
*Revision History:
*       03-30-94  CFW   Module created.
*       04-18-94  CFW   Add lcid parameter.
*       02-14-95  CFW   Clean up Mac merge.
*       02-24-95  CFW   Add _crtMessageBox.
*	02-27-95  CFW	Change __crtMessageBoxA params.
*	03-29-95  CFW	Add error message to internal headers.
*	05-26-95  GJF	Changed prototype for __crtGetEnvironmentStringsA.
*       12-14-95  JWM   Add "#pragma once".
*
****/

#if _MSC_VER > 1000 /*IFSTRIP=IGN*/
#pragma once
#endif

#ifdef _WIN32

#ifndef _INC_AWINC
#define _INC_AWINC

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif /* _CRTBLD */

#ifdef __cplusplus
extern "C" {
#endif

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
#endif

#endif	/* _INC_AWINC */

#endif /* _WIN32 */
