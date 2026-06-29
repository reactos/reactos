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
#ifndef __WINE_SEARCH_H
#define __WINE_SEARCH_H

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP void* __cdecl _lfind(const void*,const void*,unsigned int*,unsigned int,int (__cdecl *)(const void*,const void*));
_ACRTIMP void* __cdecl _lsearch(const void*,void*,unsigned int*,unsigned int,int (__cdecl *)(const void*,const void*));
_ACRTIMP void* __cdecl bsearch(const void*,const void*,size_t,size_t,int (__cdecl *)(const void*,const void*));
_ACRTIMP void  __cdecl qsort(void*,size_t,size_t,int (__cdecl *)(const void*,const void*));

#ifdef __cplusplus
}
#endif


static inline void* lfind(const void* match, const void* start, unsigned int* array_size, unsigned int elem_size, int (__cdecl *cf)(const void*,const void*))
    { return _lfind(match, start, array_size, elem_size, cf); }
static inline void* lsearch(const void* match, void* start, unsigned int* array_size, unsigned int elem_size, int (__cdecl *cf)(const void*,const void*) )
    { return _lsearch(match, start, array_size, elem_size, cf); }

#endif /* __WINE_SEARCH_H */
