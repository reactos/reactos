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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_MALLOC_H
#define __WINE_MALLOC_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

/* heap function constants */
#define _HEAPEMPTY    -1
#define _HEAPOK       -2
#define _HEAPBADBEGIN -3
#define _HEAPBADNODE  -4
#define _HEAPEND      -5
#define _HEAPBADPTR   -6

#define _FREEENTRY     0
#define _USEDENTRY     1


#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif

#ifndef MSVCRT_HEAPINFO_DEFINED
#define MSVCRT_HEAPINFO_DEFINED
typedef struct _heapinfo
{
  int*           _pentry;
  MSVCRT(size_t) _size;
  int            _useflag;
} _HEAPINFO;
#endif /* MSVCRT_HEAPINFO_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

void*       _expand(void*,MSVCRT(size_t));
int         _heapadd(void*,MSVCRT(size_t));
int         _heapchk(void);
int         _heapmin(void);
int         _heapset(unsigned int);
MSVCRT(size_t) _heapused(MSVCRT(size_t)*,MSVCRT(size_t)*);
int         _heapwalk(_HEAPINFO*);
MSVCRT(size_t) _msize(void*);

void*       MSVCRT(calloc)(MSVCRT(size_t),MSVCRT(size_t));
void        MSVCRT(free)(void*);
void*       MSVCRT(malloc)(MSVCRT(size_t));
void*       MSVCRT(realloc)(void*,MSVCRT(size_t));

#ifdef __cplusplus
}
#endif

#ifndef USE_MSVCRT_PREFIX
# ifdef __GNUC__
# define _alloca(x) __builtin_alloca((x))
# define alloca(x) __builtin_alloca((x))
# endif
#endif /* USE_MSVCRT_PREFIX */

#endif /* __WINE_MALLOC_H */
