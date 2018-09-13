/***
*dbgint.h - Supports debugging features of the C runtime library.
*
* Copyright (c) 1985 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       Support CRT debugging features.
*
*       [Internal]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifndef _INC_DBGINT
#define _INC_DBGINT

#ifndef _CRTBLD
/*
 * This is an internal C runtime header file. It is used when building
 * the C runtimes only. It is not to be used as a public header file.
 */
#error ERROR: Use of C runtime library internal header file.
#endif  /* _CRTBLD */

#include <crtdbg.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef _DEBUG

 /****************************************************************************
 *
 * Debug OFF
 * Debug OFF
 * Debug OFF
 *
 ***************************************************************************/

#ifdef __cplusplus

#define _new_crt                        new

#endif  /* __cplusplus */

#define _malloc_crt                     malloc
#define _calloc_crt                     calloc
#define _realloc_crt                    realloc
#define _expand_crt                     _expand
#define _free_crt                       free
#define _msize_crt                      _msize


#define _malloc_base                    malloc
#define _nh_malloc_base                 _nh_malloc
#define _nh_malloc_dbg(s, n, t, f, l)   _nh_malloc(s, n)
#define _heap_alloc_base                _heap_alloc
#define _heap_alloc_dbg(s, t, f, l)     _heap_alloc(s)
#define _calloc_base                    calloc
#define _realloc_base                   realloc
#define _expand_base                    _expand
#define _free_base                      free
#define _msize_base                     _msize

#ifdef _MT

#define _calloc_dbg_lk(c, s, t, f, l)   _calloc_lk(c, s)
#define _realloc_dbg_lk(p, s, t, f, l)  _realloc_lk(p, s)
#define _free_base_lk                   _free_lk
#define _free_dbg_lk(p, t)              _free_lk(p)

#else  /* _MT */

#define _calloc_dbg_lk(c, s, t, f, l)   calloc(c, s)
#define _realloc_dbg_lk(p, s, t, f, l)  realloc(p, s)
#define _free_base_lk                   free
#define _free_dbg_lk(p, t)              free(p)

#endif  /* _MT */


#else  /* _DEBUG */


 /****************************************************************************
 *
 * Debug ON
 * Debug ON
 * Debug ON
 *
 ***************************************************************************/

#define _THISFILE   __FILE__

#ifdef __cplusplus

#define _new_crt        new(_CRT_BLOCK, _THISFILE, __LINE__)

#endif  /* __cplusplus */

#define _malloc_crt(s)      _malloc_dbg(s, _CRT_BLOCK, _THISFILE, __LINE__)
#define _calloc_crt(c, s)   _calloc_dbg(c, s, _CRT_BLOCK, _THISFILE, __LINE__)
#define _realloc_crt(p, s)  _realloc_dbg(p, s, _CRT_BLOCK, _THISFILE, __LINE__)
#define _expand_crt(p, s)   _expand_dbg(p, s, _CRT_BLOCK)
#define _free_crt(p)        _free_dbg(p, _CRT_BLOCK)
#define _msize_crt(p)       _msize_dbg(p, _CRT_BLOCK)

/*
 * Prototypes for malloc, free, realloc, etc are in malloc.h
 */

void * __cdecl _malloc_base(
        size_t
        );

void * __cdecl _nh_malloc_base (
        size_t,
        int
        );

void * __cdecl _nh_malloc_dbg (
        size_t,
        int,
        int,
        const char *,
        int
        );

void * __cdecl _heap_alloc_base(
        size_t
        );

void * __cdecl _heap_alloc_dbg(
        size_t,
        int,
        const char *,
        int
        );

void * __cdecl _calloc_base(
        size_t,
        size_t
        );

void * __cdecl _realloc_base(
        void *,
        size_t
        );

void * __cdecl _expand_base(
        void *,
        size_t
        );

void __cdecl _free_base(
        void *
        );

size_t __cdecl _msize_base (
        void *
        );

#ifdef _MT

/*
 * Prototypes and macros for multi-thread support
 */


void * __cdecl _calloc_dbg_lk(
        size_t,
        size_t,
        int,
        char *,
        int
        );


void * __cdecl _realloc_dbg_lk(
        void *,
        size_t,
        int,
        const char *,
        int
        );


void __cdecl _free_base_lk(
        void *
        );

void __cdecl _free_dbg_lk(
        void *,
        int
        );

#else  /* _MT */

#define _calloc_dbg_lk                  _calloc_dbg
#define _realloc_dbg_lk                 _realloc_dbg
#define _free_base_lk                   _free_base
#define _free_dbg_lk                    _free_dbg

#endif  /* _MT */

/*
 * For diagnostic purpose, blocks are allocated with extra information and
 * stored in a doubly-linked list.  This makes all blocks registered with
 * how big they are, when they were allocated, and what they are used for.
 */

#define nNoMansLandSize 4

typedef struct _CrtMemBlockHeader
{
        struct _CrtMemBlockHeader * pBlockHeaderNext;
        struct _CrtMemBlockHeader * pBlockHeaderPrev;
        char *                      szFileName;
        int                         nLine;
        size_t                      nDataSize;
        int                         nBlockUse;
        long                        lRequest;
        unsigned char               gap[nNoMansLandSize];
        /* followed by:
         *  unsigned char           data[nDataSize];
         *  unsigned char           anotherGap[nNoMansLandSize];
         */
} _CrtMemBlockHeader;

#define pbData(pblock) ((unsigned char *)((_CrtMemBlockHeader *)pblock + 1))
#define pHdr(pbData) (((_CrtMemBlockHeader *)pbData)-1)


_CRTIMP void __cdecl _CrtSetDbgBlockType(
        void *,
        int
        );

#define _BLOCK_TYPE_IS_VALID(use) (_BLOCK_TYPE(use) == _CLIENT_BLOCK || \
                                              (use) == _NORMAL_BLOCK || \
                                   _BLOCK_TYPE(use) == _CRT_BLOCK    || \
                                              (use) == _IGNORE_BLOCK)

#ifndef UNIX
extern _CRT_ALLOC_HOOK _pfnAllocHook; /* defined in dbghook.c */
#endif /* UNIX */

int __cdecl _CrtDefaultAllocHook(
        int,
        void *,
        size_t,
        int,
        long,
        const unsigned char *,
        int
        );

#endif  /* _DEBUG */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _INC_DBGINT */
