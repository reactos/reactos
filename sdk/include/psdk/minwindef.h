/*
 * minwindef.h
 *
 * Basic Win-API definitions
 *
 * This file is part of the ReactOS SDK.
 *
 * Contributors:
 *   Created by Timo Kreuzer <timo.kreuzer@reactos.org>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _MINWINDEF_
#define _MINWINDEF_

#pragma once

#include <specstrings.h>

// FIXME: Ugly hack
#if (defined(_LP64) || defined(__LP64__)) && !defined(_M_AMD64)
#ifndef __ROS_LONG64__
#define __ROS_LONG64__
#endif
#endif

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif

#ifndef WIN32
#define WIN32
#endif

#if defined(_MAC) && !defined(_WIN32)
#define _WIN32
#endif

#ifndef BASETYPES
#define BASETYPES
#ifndef __ROS_LONG64__
typedef unsigned long ULONG;
#else
typedef unsigned int ULONG;
#endif
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef _Null_terminated_ char *PSZ;
#endif  /* BASETYPES */

#undef MAX_PATH // TODO: Remove this
#define MAX_PATH 260

#ifndef NULL
#ifdef __cplusplus
#ifndef _WIN64
#define NULL 0
#else
#define NULL 0LL
#endif  /* W64 */
#else
#define NULL ((void *)0)
#endif
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifndef OPTIONAL
#define OPTIONAL
#endif

#undef far
#undef near
#undef pascal

#define far
#define near
#define pascal __stdcall

#define cdecl
#ifndef CDECL
#define CDECL
#endif

#if !defined(__x86_64__) //defined(_STDCALL_SUPPORTED)
#ifndef CALLBACK
#define CALLBACK __stdcall
#endif
#ifndef WINAPI
#define WINAPI __stdcall
#endif
#define WINAPIV __cdecl
#define APIENTRY WINAPI
#define APIPRIVATE WINAPI
#define PASCAL WINAPI
#else
#define CALLBACK
#define WINAPI
#define WINAPIV
#define APIENTRY WINAPI
#define APIPRIVATE
#define PASCAL pascal
#endif

#undef FAR
#undef NEAR
#define FAR
#define NEAR

#ifndef CONST
#define CONST const
#endif

typedef int BOOL;
typedef BOOL *PBOOL;
typedef BOOL *LPBOOL;
typedef unsigned char BYTE;
typedef unsigned short WORD;
#ifndef __ROS_LONG64__
typedef unsigned long DWORD;
#else
typedef unsigned int DWORD;
#endif
typedef float FLOAT;
typedef FLOAT *PFLOAT;
typedef BYTE *PBYTE;
typedef BYTE *LPBYTE;
typedef int *PINT;
typedef int *LPINT;
typedef WORD *PWORD;
typedef WORD *LPWORD;
#ifndef __ROS_LONG64__
typedef long *LPLONG;
#else
typedef int *LPLONG;
#endif
typedef DWORD *PDWORD;
typedef DWORD *LPDWORD;
typedef void *LPVOID;
#ifndef _LPCVOID_DEFINED
#define _LPCVOID_DEFINED
typedef CONST void *LPCVOID;
#endif
typedef int INT;
typedef unsigned int UINT;
typedef unsigned int *PUINT;

#ifndef NT_INCLUDED
#include <winnt.h>
#endif

typedef UINT_PTR WPARAM;
typedef LONG_PTR LPARAM;
typedef LONG_PTR LRESULT;

#ifndef NOMINMAX
#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif
#endif

#define MAKEWORD(bLow, bHigh)   ((WORD)(((BYTE)((DWORD_PTR)(bLow) & 0xff  )) |  (((WORD)((BYTE)((DWORD_PTR)(bHigh) & 0xff)))   << 8 )))
#define MAKELONG(wLow, wHigh)   ((LONG)(((WORD)((DWORD_PTR)(wLow) & 0xffff)) | (((DWORD)((WORD)((DWORD_PTR)(wHigh) & 0xffff))) << 16)))
#define LOWORD(l)               ((WORD)((DWORD_PTR)(l) & 0xffff))
#define HIWORD(l)               ((WORD)(((DWORD_PTR)(l) >> 16) & 0xffff))
#define LOBYTE(w)               ((BYTE)((DWORD_PTR)(w) & 0xff))
#define HIBYTE(w)               ((BYTE)(((DWORD_PTR)(w) >> 8) & 0xff))

typedef WORD ATOM;

typedef HANDLE *SPHANDLE;
typedef HANDLE *LPHANDLE;
typedef HANDLE HGLOBAL;
typedef HANDLE HLOCAL;
typedef HANDLE GLOBALHANDLE;
typedef HANDLE LOCALHANDLE;

typedef INT_PTR (WINAPI *FARPROC)();
typedef INT_PTR (WINAPI *NEARPROC)();
typedef INT_PTR (WINAPI *PROC)();

DECLARE_HANDLE(HKEY);
typedef HKEY *PHKEY;

DECLARE_HANDLE(HMETAFILE);
DECLARE_HANDLE(HINSTANCE);
typedef HINSTANCE HMODULE;
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HRSRC);
DECLARE_HANDLE(HSTR);
DECLARE_HANDLE(HTASK);
DECLARE_HANDLE(HWINSTA);
DECLARE_HANDLE(HKL);
DECLARE_HANDLE(HSPRITE);
DECLARE_HANDLE(HLSURF);

typedef int HFILE;

typedef struct _FILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;
#define _FILETIME_

#endif // _MINWINDEF_
