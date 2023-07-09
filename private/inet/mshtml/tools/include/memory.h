/***
*memory.h - declarations for buffer (memory) manipulation routines
*
*       Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       This include file contains the function declarations for the
*       buffer (memory) manipulation routines.
*       [System V]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_MEMORY
#define _INC_MEMORY

#if !defined(_WIN32) && !defined(_MAC)
#error ERROR: Only Mac or Win32 targets supported!
#endif


#ifdef __cplusplus
extern "C" {
#endif


/* Define _CRTAPI1 (for compatibility with the NT SDK) */

#ifndef _CRTAPI1
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI1 __cdecl
#else
#define _CRTAPI1
#endif
#endif


/* Define _CRTAPI2 (for compatibility with the NT SDK) */

#ifndef _CRTAPI2
#if     _MSC_VER >= 800 && _M_IX86 >= 300
#define _CRTAPI2 __cdecl
#else
#define _CRTAPI2
#endif
#endif


/* Define _CRTIMP */

#ifndef _CRTIMP
#ifdef  _NTSDK
/* definition compatible with NT SDK */
#define _CRTIMP
#else   /* ndef _NTSDK */
/* current definition */
#ifdef  _DLL
#define _CRTIMP __declspec(dllimport)
#else   /* ndef _DLL */
#define _CRTIMP
#endif  /* _DLL */
#endif  /* _NTSDK */
#endif  /* _CRTIMP */


/* Define __cdecl for non-Microsoft compilers */

#if     ( !defined(_MSC_VER) && !defined(__cdecl) )
#define __cdecl
#endif


#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif

/* Function prototypes */

_CRTIMP void *  __cdecl _memccpy(void *, const void *, int, unsigned int);
_CRTIMP void *  __cdecl memchr(const void *, int, size_t);
_CRTIMP int     __cdecl _memicmp(const void *, const void *, unsigned int);
#ifdef _M_MRX000
_CRTIMP int     __cdecl memcmp(const void *, const void *, size_t);
_CRTIMP void *  __cdecl memcpy(void *, const void *, size_t);
_CRTIMP void *  __cdecl memset(void *, int, size_t);
#else
        int     __cdecl memcmp(const void *, const void *, size_t);
        void *  __cdecl memcpy(void *, const void *, size_t);
        void *  __cdecl memset(void *, int, size_t);
#endif

#if !__STDC__

/* Non-ANSI names for compatibility */

#if     defined(_NTSDK)
/* definitions compatible with the NT SDK */
#define memccpy  _memccpy
#define memicmp  _memicmp
#else   /* _NTSDK */
/* current declarations */
_CRTIMP void * __cdecl memccpy(void *, const void *, int, unsigned int);
_CRTIMP int __cdecl memicmp(const void *, const void *, unsigned int);
#endif  /* _NTSDK */

#endif  /* __STDC__ */

#ifdef __cplusplus
}
#endif

#endif  /* _INC_MEMORY */
