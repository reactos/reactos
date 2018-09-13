/***
*search.h - declarations for searcing/sorting routines
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This file contains the declarations for the sorting and
*   searching routines.
*   [System V]
*
****/

#ifndef _INC_SEARCH

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif 


/* function prototypes */

void * __cdecl bsearch(const void *, const void *,
    size_t, size_t, int (__cdecl *)(const void *,
    const void *));
void * __cdecl _lfind(const void *, const void *,
    unsigned int *, unsigned int, int (__cdecl *)
    (const void *, const void *));
void * __cdecl _lsearch(const void *, void *,
    unsigned int *, unsigned int, int (__cdecl *)
    (const void *, const void *));
void __cdecl qsort(void *, size_t, size_t, int (__cdecl *)
    (const void *, const void *));

#ifndef __STDC__
/* Non-ANSI names for compatibility */
void * __cdecl lfind(const void *, const void *,
    unsigned int *, unsigned int, int (__cdecl *)
    (const void *, const void *));
void * __cdecl lsearch(const void *, void *,
    unsigned int *, unsigned int, int (__cdecl *)
    (const void *, const void *));
#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_SEARCH
#endif 
