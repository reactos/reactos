/***
*stddef.h - definitions/declarations for common constants, types, variables
*
* Copyright (c) 1985 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       This file contains definitions and declarations for some commonly
*       used constants, types, and variables.
*       [ANSI]
*
*       [Public]
*
****/
#ifdef UNIX
#ifndef __STDDEF_WRAPPER_H__
#define __STDDEF_WRAPPER_H__

#include "mwconfig.h"
#include MW_INCLUDE_ANSI_HEADER(stddef.h)

#ifdef offsetof
#undef offsetof
#endif /* offsetof */

#define offsetof(s, m)  (size_t)(((long)&(((s *)8)->m)) - 8)

#endif /* __STDDEF_WRAPPER_H__ */


#else

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifndef _INC_STDDEF
#define _INC_STDDEF

#if !defined (_WIN32) && !defined (_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif  /* !defined (_WIN32) && !defined (_MAC) */

#ifndef _CRTBLD
/* This version of the header files is NOT for user programs.
 * It is intended for use when building the C runtimes ONLY.
 * The version intended for public use will not have this message.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef _INTERNAL_IFSTRIP_
#include <cruntime.h>
#endif  /* _INTERNAL_IFSTRIP_ */


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef CRTDLL
#define _CRTIMP __declspec(dllexport)
#else  /* CRTDLL */
#ifdef _DLL
#define _CRTIMP __declspec(dllimport)
#else  /* _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* CRTDLL */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if (!defined (_MSC_VER) && !defined (__cdecl))
#define __cdecl
#endif  /* (!defined (_MSC_VER) && !defined (__cdecl)) */


/* Define NULL pointer value and the offset() macro */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else  /* __cplusplus */
#define NULL    ((void *)0)
#endif  /* __cplusplus */
#endif  /* NULL */


#define offsetof(s,m)   (size_t)&(((s *)0)->m)


/* Declare reference to errno */

#if (defined (_MT) || defined (_DLL)) && !defined (_MAC)
_CRTIMP extern int * __cdecl _errno(void);
#define errno   (*_errno())
#else  /* (defined (_MT) || defined (_DLL)) && !defined (_MAC) */
_CRTIMP extern int errno;
#endif  /* (defined (_MT) || defined (_DLL)) && !defined (_MAC) */


/* define the implementation dependent size types */

#ifndef _PTRDIFF_T_DEFINED
typedef int ptrdiff_t;
#define _PTRDIFF_T_DEFINED
#endif  /* _PTRDIFF_T_DEFINED */


#ifndef _SIZE_T_DEFINED
#ifdef  _WIN64
typedef unsigned __int64 size_t;
#else
typedef unsigned int     size_t;
#endif
#define _SIZE_T_DEFINED
#endif  /* _SIZE_T_DEFINED */


#ifndef _WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#define _WCHAR_T_DEFINED
#endif  /* _WCHAR_T_DEFINED */


#ifdef _MT
_CRTIMP extern unsigned long  __cdecl __threadid(void);
#define _threadid       (__threadid())
_CRTIMP extern unsigned long  __cdecl __threadhandle(void);
#endif  /* _MT */


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _INC_STDDEF */
#endif // UNIX
