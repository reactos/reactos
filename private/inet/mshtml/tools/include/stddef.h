/***
*stddef.h - definitions/declarations for common constants, types, variables
*
*	Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file contains definitions and declarations for some commonly
*	used constants, types, and variables.
*	[ANSI]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_STDDEF
#define _INC_STDDEF

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if	_MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if	_MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef	_NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else	/* ndef _NTSDK */
/* current definition */
#ifdef	_DLL
#define _CRTIMP __declspec(dllimport)
#else	/* ndef _DLL */
#define _CRTIMP
#endif	/* _DLL */
#endif	/* _NTSDK */
#endif	/* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if	( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


/* Define NULL pointer value and the offset() macro */

#ifndef NULL
#ifdef __cplusplus
#define NULL	0
#else
#define NULL	((void *)0)
#endif
#endif


#define offsetof(s,m)	(size_t)&(((s *)0)->m)


/* Declare reference to errno */

#if (defined(_MT) || defined(_DLL)) && (!defined(_M_MPPC) && !defined(_M_M68K))
_CRTIMP extern int * __cdecl _errno(void);
#define errno	(*_errno())
#else	/* ndef _MT && ndef _DLL */
_CRTIMP extern int errno;
#endif	/* _MT || _DLL */


/* define the implementation dependent size types */

#ifndef _PTRDIFF_T_DEFINED
typedef int ptrdiff_t;
#define _PTRDIFF_T_DEFINED
#endif


#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif


#ifdef	_MT
_CRTIMP extern unsigned long  __cdecl __threadid(void);
#define _threadid	(__threadid())
_CRTIMP extern unsigned long  __cdecl __threadhandle(void);
#endif


#ifdef __cplusplus
}
#endif

#endif	/* _INC_STDDEF */
