/***
*search.h - declarations for searcing/sorting routines
*
*	Copyright (c) 1985-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*	This file contains the declarations for the sorting and
*	searching routines.
*	[System V]
*
*       [Public]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef _INC_SEARCH
#define _INC_SEARCH

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


#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif


/* Function prototypes */

_CRTIMP void * __cdecl bsearch(const void *, const void *, size_t, size_t,
	int (__cdecl *)(const void *, const void *));
_CRTIMP void * __cdecl _lfind(const void *, const void *, unsigned int *, unsigned int,
	int (__cdecl *)(const void *, const void *));
_CRTIMP void * __cdecl _lsearch(const void *, void  *, unsigned int *, unsigned int,
	int (__cdecl *)(const void *, const void *));
_CRTIMP void __cdecl qsort(void *, size_t, size_t, int (__cdecl *)(const void *,
	const void *));


#if	!__STDC__
/* Non-ANSI names for compatibility */
#ifdef	_NTSDK
/* definitions compatible with NT SDK */
#define lfind	_lfind
#define lsearch _lsearch
#else	/* ndef _NTSDK */
/* current declarations */
_CRTIMP void * __cdecl lfind(const void *, const void *, unsigned int *, unsigned int,
	int (__cdecl *)(const void *, const void *));
_CRTIMP void * __cdecl lsearch(const void *, void  *, unsigned int *, unsigned int,
	int (__cdecl *)(const void *, const void *));
#endif	/* _NTSDK */
#endif	/* __STDC__ */


#ifdef __cplusplus
}
#endif

#endif	/* _INC_SEARCH */
