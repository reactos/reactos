/*
 * Heap definitions
 *
 * Copyright 2001 Francois Gouget.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __WINE_MALLOC_H
#define __WINE_MALLOC_H

#include <corecrt.h>
#include <corecrt_malloc.h>

/* heap function constants */
#define _HEAPEMPTY    -1
#define _HEAPOK       -2
#define _HEAPBADBEGIN -3
#define _HEAPBADNODE  -4
#define _HEAPEND      -5
#define _HEAPBADPTR   -6

#define _FREEENTRY     0
#define _USEDENTRY     1

#ifndef _HEAPINFO_DEFINED
#define _HEAPINFO_DEFINED
typedef struct _heapinfo
{
  int*           _pentry;
  size_t _size;
  int            _useflag;
} _HEAPINFO;
#endif /* _HEAPINFO_DEFINED */

#ifdef __i386__
_ACRTIMP unsigned int* __cdecl __p__amblksiz(void);
#define _amblksiz (*__p__amblksiz());
#else
extern unsigned int _amblksiz;
#endif

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP int    __cdecl _heapadd(void*,size_t);
_ACRTIMP int    __cdecl _heapchk(void);
_ACRTIMP int    __cdecl _heapmin(void);
_ACRTIMP int    __cdecl _heapset(unsigned int);
_ACRTIMP size_t __cdecl _heapused(size_t*,size_t*);
_ACRTIMP int    __cdecl _heapwalk(_HEAPINFO*);

_ACRTIMP intptr_t __cdecl _get_heap_handle(void);
_ACRTIMP size_t __cdecl _get_sbh_threshold(void);
_ACRTIMP int    __cdecl _set_sbh_threshold(size_t size);

#ifdef _MSC_VER
void *_alloca(size_t size);
#endif

#ifdef __cplusplus
}
#endif

# ifdef __GNUC__
# define _alloca(x) __builtin_alloca((x))
# define alloca(x) __builtin_alloca((x))
# elif defined(_MSC_VER)
# define alloca(x) _alloca((x))
# endif

#endif /* __WINE_MALLOC_H */
