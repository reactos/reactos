/*
 * malloc.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Support for programs which want to use malloc.h to get memory management
 * functions. Unless you absolutely need some of these functions and they are
 * not in the ANSI headers you should use the ANSI standard header files
 * instead.
 *
 */

#ifndef _MALLOC_H_
#define _MALLOC_H_

/* All the headers include this file. */
#include <_mingw.h>

#include <stdlib.h>

#ifndef RC_INVOKED

/*
 * The structure used to walk through the heap with _heapwalk.
 */
typedef	struct _heapinfo
{
	int*	_pentry;
	size_t	_size;
	int	_useflag;
} _HEAPINFO;

/* Values for _heapinfo.useflag */
#define _FREEENTRY 0
#define _USEDENTRY 1

/* Return codes for _heapwalk()  */
#define _HEAPEMPTY	(-1)
#define _HEAPOK		(-2)
#define _HEAPBADBEGIN	(-3)
#define _HEAPBADNODE	(-4)
#define _HEAPEND	(-5)
#define _HEAPBADPTR	(-6)

#ifdef	__cplusplus
extern "C" {
#endif
/*
   The _heap* memory allocation functions are supported on NT
   but not W9x. On latter, they always set errno to ENOSYS.
*/
_CRTIMP int __cdecl __MINGW_NOTHROW _heapwalk (_HEAPINFO*);
#ifdef __GNUC__
#define _alloca(x) __builtin_alloca((x))
#endif

#ifndef	_NO_OLDNAMES
_CRTIMP int __cdecl __MINGW_NOTHROW heapwalk (_HEAPINFO*);
#ifdef __GNUC__
#define alloca(x) __builtin_alloca((x))
#endif
#endif	/* Not _NO_OLDNAMES */

_CRTIMP int __cdecl __MINGW_NOTHROW _heapchk (void);	/* Verify heap integrety. */
_CRTIMP int __cdecl __MINGW_NOTHROW _heapmin (void);	/* Return unused heap to the OS. */
_CRTIMP int __cdecl __MINGW_NOTHROW _heapset (unsigned int);

_CRTIMP size_t __cdecl __MINGW_NOTHROW _msize (void*);
_CRTIMP size_t __cdecl __MINGW_NOTHROW _get_sbh_threshold (void); 
_CRTIMP int __cdecl __MINGW_NOTHROW _set_sbh_threshold (size_t);
_CRTIMP void* __cdecl __MINGW_NOTHROW _expand (void*, size_t); 

/* These require msvcr70.dll or higher. */ 
#if __MSVCRT_VERSION__ >= 0x0700
_CRTIMP void * __cdecl __MINGW_NOTHROW _aligned_offset_malloc(size_t, size_t, size_t);
_CRTIMP void * __cdecl __MINGW_NOTHROW _aligned_offset_realloc(void*, size_t, size_t, size_t);

_CRTIMP void * __cdecl __MINGW_NOTHROW _aligned_malloc (size_t, size_t);
_CRTIMP void * __cdecl __MINGW_NOTHROW _aligned_realloc (void*, size_t, size_t);
_CRTIMP void __cdecl __MINGW_NOTHROW _aligned_free (void*);
#endif /* __MSVCRT_VERSION__ >= 0x0700 */

/* These require libmingwex.a. */ 
void * __cdecl __MINGW_NOTHROW __mingw_aligned_offset_malloc (size_t, size_t, size_t);
void * __cdecl __MINGW_NOTHROW __mingw_aligned_offset_realloc (void*, size_t, size_t, size_t);

void * __cdecl __MINGW_NOTHROW __mingw_aligned_malloc (size_t, size_t);
void * __cdecl __MINGW_NOTHROW __mingw_aligned_realloc (void*, size_t, size_t);
void __cdecl __MINGW_NOTHROW __mingw_aligned_free (void*);

#ifdef __cplusplus
}
#endif

#endif	/* RC_INVOKED */

#endif /* Not _MALLOC_H_ */
