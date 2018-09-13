/***
*memory.h - declarations for buffer (memory) manipulation routines
*
*   Copyright (c) 1985-1992, Microsoft Corporation.  All rights reserved.
*
*Purpose:
*   This include file contains the function declarations for the
*   buffer (memory) manipulation routines.
*   [System V]
*
****/

#ifndef _INC_MEMORY

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

void * __cdecl _memccpy(void *, const void *,
    int, unsigned int);
void * __cdecl memchr(const void *, int, size_t);
int __cdecl memcmp(const void *, const void *,
    size_t);
void * __cdecl memcpy(void *, const void *,
    size_t);
int __cdecl _memicmp(const void *, const void *,
    unsigned int);
void * __cdecl memset(void *, int, size_t);
void __cdecl _movedata(unsigned int, unsigned int, unsigned int,
    unsigned int, unsigned int);


/* model independent function prototypes */

void __far * __far __cdecl _fmemccpy(void __far *, const void __far *,
    int, unsigned int);
void __far * __far __cdecl _fmemchr(const void __far *, int, size_t);
int __far __cdecl _fmemcmp(const void __far *, const void __far *,
    size_t);
void __far * __far __cdecl _fmemcpy(void __far *, const void __far *,
    size_t);
int __far __cdecl _fmemicmp(const void __far *, const void __far *,
    unsigned int);
void __far * __far __cdecl _fmemset(void __far *, int, size_t);


#ifndef __STDC__
/* Non-ANSI names for compatibility */
void * __cdecl memccpy(void *, const void *,
    int, unsigned int);
int __cdecl memicmp(const void *, const void *,
    unsigned int);
void __cdecl movedata(unsigned int, unsigned int, unsigned int,
    unsigned int, unsigned int);
#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_MEMORY
#endif 
