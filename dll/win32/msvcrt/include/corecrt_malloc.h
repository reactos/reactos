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
#ifndef __WINE_CORECRT_MALLOC_H
#define __WINE_CORECRT_MALLOC_H

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP void*  __cdecl calloc(size_t,size_t);
_ACRTIMP void   __cdecl free(void*);
_ACRTIMP void*  __cdecl malloc(size_t);
_ACRTIMP void*  __cdecl realloc(void*,size_t);
_ACRTIMP void*  __cdecl _recalloc(void*,size_t,size_t) __WINE_ALLOC_SIZE(2,3) __WINE_DEALLOC(free);

_ACRTIMP void*  __cdecl _expand(void*,size_t);
_ACRTIMP size_t __cdecl _msize(void*);

_ACRTIMP void   __cdecl _aligned_free(void*);
_ACRTIMP void*  __cdecl _aligned_malloc(size_t,size_t) __WINE_ALLOC_SIZE(1) __WINE_DEALLOC(_aligned_free) __WINE_MALLOC;
_ACRTIMP void*  __cdecl _aligned_offset_malloc(size_t,size_t,size_t) __WINE_ALLOC_SIZE(1) __WINE_DEALLOC(_aligned_free) __WINE_MALLOC;
_ACRTIMP void*  __cdecl _aligned_realloc(void*,size_t,size_t) __WINE_ALLOC_SIZE(2) __WINE_DEALLOC(_aligned_free);
_ACRTIMP void*  __cdecl _aligned_offset_realloc(void*,size_t,size_t,size_t) __WINE_ALLOC_SIZE(2) __WINE_DEALLOC(_aligned_free);

#ifdef __cplusplus
}
#endif

#endif /* __WINE_CORECRT_MALLOC_H */
