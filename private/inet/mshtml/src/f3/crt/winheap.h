/***
*winheap.h - Private include file for winheap directory.
*
*       Copyright (c) 1988-1996, Microsoft Corporation. All rights reserved.
*
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

#ifndef X_WINDOWS_H_
#define X_WINDOWS_H_
#include <windows.h>
#endif

#if defined(_M_ALPHA) || defined(_M_ALPHA64) || defined(UNIX)
#define _PAGESIZE_      0x2000      /* one page */
#else
#define _PAGESIZE_      0x1000      /* one page */
#endif

/*
 * Constants and types for used by the small-block heap
 */
#define _PARASIZE               0x10
#define _PARASHIFT              0x4

#if defined(_M_ALPHA64)

#define _PARAS_PER_PAGE         453
#define _PADDING_PER_PAGE       8
#define _PAGES_PER_REGION       512
#define _PAGES_PER_COMMITMENT   8

#elif defined(_M_IA64)

#define _PARAS_PER_PAGE         239
#define _PADDING_PER_PAGE       12
#define _PAGES_PER_REGION       1024
#define _PAGES_PER_COMMITMENT   16

#elif defined(_M_ALPHA) || defined(UNIX)

#define _PARAS_PER_PAGE         454
#define _PADDING_PER_PAGE       3
#define _PAGES_PER_REGION       512
#define _PAGES_PER_COMMITMENT   8

#else

#define _PARAS_PER_PAGE         240
#define _PADDING_PER_PAGE       3
#define _PAGES_PER_REGION       1024
#define _PAGES_PER_COMMITMENT   16
#define _MARK_SBH

#endif

#define _NO_INDEX               -1

typedef char            __para_t[16];

#if defined(_M_ALPHA) || defined(UNIX)
typedef unsigned short  __map_t;
#else
typedef unsigned char   __map_t;
#endif

#define _FREE_PARA              (__map_t)(0)
#define _UNCOMMITTED_PAGE       (__map_t)(-1)
#define _NO_FAILED_ALLOC        (__map_t)(_PARAS_PER_PAGE + 1);

typedef struct __sbh_region_struct  __sbh_region_t;

/*
 * Small-block heap page. The first four fields of the structure below are
 * descriptor for the page. That is, they hold information about allocations
 * in the page. The last field (typed as an array of paragraphs) is the
 * allocation area.
 */
typedef struct {
        __map_t *   pstarting_alloc_map;
        unsigned long free_paras_at_start;
        __map_t     alloc_map[_PARAS_PER_PAGE];
        __map_t     sentinel;                       /* always set to -1 */
        __map_t     reserved[_PADDING_PER_PAGE];
        __sbh_region_t * preg;
        __para_t    alloc_blocks[_PARAS_PER_PAGE];
}       __sbh_page_t;

// If the compiler barfs on the following line, it means that the data structure for
// __sbh_page_t is not exactly the size of a page.  It needs to be.  Adjust the parameter
// above until this compile-time assert doesn't fire.
typedef int _farf___sbh_page_t[sizeof(__sbh_page_t) == (_PAGESIZE_)];

/*
 * Small-block heap region descriptor. Most often, the small-block heap
 * consists of a single region, described by the statically allocated
 * decriptor __small_block_heap (declared below).
 */
struct __sbh_region_struct {
        struct __sbh_region_struct *    p_next_region;
        struct __sbh_region_struct *    p_prev_region;
        int                             starting_page_index;
        int                             first_uncommitted_index;
        __map_t                         region_map[_PAGES_PER_REGION];
        __map_t                         last_failed_alloc[_PAGES_PER_REGION];
        __sbh_page_t *                  p_pages;
};

typedef struct __sbh_region_struct  __sbh_region_t;

extern  HANDLE _crtheap;

/*
 * Global variable declarations for the small-block heap.
 */
extern __sbh_region_t  __small_block_heap;

// BUGBUG until CTableCell goes below 480 bytes (istvanc)
//efine __sbh_threshold (_PARASIZE * (_PARAS_PER_PAGE / 8))
#define __sbh_threshold (_PARASIZE * 35) 

void * __cdecl _nh_malloc( size_t, int);
void * __cdecl _heap_alloc(size_t);

/*
 * Prototypes for internal functions of the small-block heap.
 */
void *    __cdecl __sbh_alloc_block(size_t);
void *    __cdecl __sbh_alloc_block_from_page(__sbh_page_t *, size_t, size_t);
void      __cdecl __sbh_decommit_pages(int);
__map_t * __cdecl __sbh_find_block(void *, __sbh_region_t **, __sbh_page_t **);
void      __cdecl __sbh_free_block(__sbh_region_t *, __sbh_page_t *, __map_t *);
int       __cdecl __sbh_heap_check(void);
__sbh_region_t * __cdecl __sbh_new_region(void);
void      __cdecl __sbh_release_region(__sbh_region_t *);
int       __cdecl __sbh_resize_block(__sbh_region_t *, __sbh_page_t *,
                                     __map_t *, size_t);


#ifdef __cplusplus
}
#endif  /* __cplusplus */


#endif  /* _INC_WINHEAP */
