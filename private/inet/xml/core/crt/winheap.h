/***
*winheap.h - Private include file for winheap directory.
*
* Copyright (c) 1988 - 1999 Microsoft Corporation. All rights reserved.*
*Purpose:
*       Contains information needed by the C library heap code.
*
*       [Internal]
*
****/

#if _MSC_VER > 1000
#pragma once
#endif  /* _MSC_VER > 1000 */

#ifndef _INC_WINHEAP
#define _INC_WINHEAP

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

#ifdef _M_ALPHA
#define _PAGESIZE_      0x2000      /* one page */
#else  /* _M_ALPHA */
#define _PAGESIZE_      0x1000      /* one page */
#endif  /* _M_ALPHA */

/*
 * Constants and types for used by the small-block heap
 */
#define _PARASIZE               0x10
#define _PARASHIFT              0x4

#ifdef _M_ALPHA
#define _PARAS_PER_PAGE         453 // 454
#define _PADDING_PER_PAGE       10 // 3 // 5
#define _PAGES_PER_REGION       512
#define _PAGES_PER_COMMITMENT   8
#else  /* _M_ALPHA */
#define _PARAS_PER_PAGE         239 // 240
#define _PADDING_PER_PAGE       16 // 3 // 7
#define _PAGES_PER_REGION       1024
#define _PAGES_PER_COMMITMENT   16
#endif  /* _M_ALPHA */

typedef char            __para_t[16];

#ifdef _M_ALPHA
typedef unsigned short  __page_map_t;
#else  /* _M_ALPHA */
typedef unsigned char   __page_map_t;
#endif  /* _M_ALPHA */

#define _FREE_PARA              (__page_map_t)(0)
#define _UNCOMMITTED_PAGE       (-1)
#define _NO_FAILED_ALLOC        (size_t)(_PARAS_PER_PAGE + 1)

struct __sbh_region_struct;

#define _DEFAULT_THRESHOLD  (512 + 16)

struct __sbh_heap_struct;

/*
 * Small-block heap page. The first four fields of the structure below are
 * descriptor for the page. That is, they hold information about allocations
 * in the page. The last field (typed as an array of paragraphs) is the
 * allocation area.
 */
typedef struct __sbh_page_struct {
        struct __sbh_heap_struct *     p_heap;
        ULONG_PTR               _lPageLock;
        __page_map_t *          p_starting_alloc_map;
        size_t                  free_paras_at_start;
        __page_map_t            alloc_map[_PARAS_PER_PAGE + 1];
        __page_map_t            reserved[_PADDING_PER_PAGE];
        __para_t                alloc_blocks[_PARAS_PER_PAGE];
}       __sbh_page_t;

#define _NO_PAGES               (__sbh_page_t *)0xFFFFFFFF

// BUGBUG: for now use casting to long until we fix the SpinLock API to use ULONG_PTR
inline ULONG_PTR __sbh_try_lock_page(__sbh_page_struct * ppage)
{
    return INTERLOCKEDEXCHANGE_PTR(&ppage->_lPageLock, REF_LOCKED);
}

inline ULONG_PTR __sbh_lock_page(__sbh_page_struct * ppage)
{
    return (ULONG_PTR)::SpinLock(&ppage->_lPageLock);
}

inline void __sbh_unlock_page(__sbh_page_struct * ppage, ULONG_PTR ul)
{
    ::SpinUnlock(&ppage->_lPageLock, ul);
}

/*
 * Type used in small block region desciptor type (see below).
 */
typedef struct {
        int     free_paras_in_page;
        size_t  last_failed_alloc;
}       __region_map_t;

/*
 * Small-block heap region descriptor. Most often, the small-block heap
 * consists of a single region, described by the statically allocated
 * decriptor __small_block_heap (declared below).
 */
struct __sbh_region_struct {
        struct __sbh_region_struct *    p_next_region;
        struct __sbh_region_struct *    p_prev_region;
        __region_map_t *                p_starting_region_map;
        __region_map_t *                p_first_uncommitted;
        __sbh_page_t *                  p_pages_begin;
        __sbh_page_t *                  p_pages_end;
        __region_map_t                  region_map[_PAGES_PER_REGION + 1];
};

typedef struct __sbh_region_struct  __sbh_region_t;

struct __sbh_heap_struct {
    __sbh_region_t              __small_block_heap;
    __sbh_region_t *            __sbh_p_starting_region;
    int                         __sbh_decommitable_pages;
    size_t                      __sbh_threshold;
};

typedef struct __sbh_heap_struct  __sbh_heap_t;

/*
 * Global variable declarations for the small-block heap.
 */
extern __sbh_region_t  __small_block_heap;
extern size_t          __sbh_threshold;

void * __cdecl _nh_malloc( size_t, int);
void * __cdecl _heap_alloc(size_t);

/*
 * Prototypes for internal functions of the small-block heap.
 */
__sbh_heap_t * __cdecl __sbh_create_heap();
void      __cdecl __sbh_destroy_heap(__sbh_heap_t * pheap);
void *    __cdecl __sbh_alloc_block(__sbh_heap_t * pheap, size_t);
void *    __cdecl __sbh_alloc_block_from_page(__sbh_page_t *, size_t, size_t);
void      __cdecl __sbh_decommit_pages(__sbh_heap_t * pheap, int count);
__page_map_t * __cdecl __sbh_find_block(__sbh_heap_t * pheap, void *, __sbh_region_t **,
        __sbh_page_t **);
void      __cdecl __sbh_free_block(__sbh_heap_t * pheap, __sbh_region_t *, __sbh_page_t *,
        __page_map_t *);
int       __cdecl __sbh_heap_check(__sbh_heap_t * pheap);
__sbh_region_t * __cdecl __sbh_new_region(__sbh_heap_t * pheap);
void      __cdecl __sbh_release_region(__sbh_heap_t * pheap, __sbh_region_t *);
int       __cdecl __sbh_resize_block(__sbh_region_t *, __sbh_page_t *,
        __page_map_t *, size_t);

/*
 * Prototypes for user functions.
 */
size_t __cdecl _get_sbh_heap_threshold(__sbh_heap_t * pheap);
int    __cdecl _set_sbh_heap_threshold(__sbh_heap_t * pheap, size_t);
void __cdecl __sbh_init_heap(__sbh_heap_t * pheap);
void __cdecl __sbh_finish_heap(__sbh_heap_t * pheap);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* _INC_WINHEAP */
