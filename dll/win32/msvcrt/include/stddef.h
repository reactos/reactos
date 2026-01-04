/*
 * Copyright 2000 Francois Gouget.
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
#ifndef __WINE_STDDEF_H
#define __WINE_STDDEF_H

#include <corecrt.h>

#if defined(__GNUC__) || defined(__clang__)
#define offsetof(s,m)       __builtin_offsetof(s,m)
#elif defined(_WIN64)
#define offsetof(s,m)       (size_t)((ptrdiff_t)&(((s*)NULL)->m))
#else
#define offsetof(s,m)       (size_t)&(((s*)NULL)->m)
#endif

typedef double max_align_t;

#ifdef __cplusplus
extern "C" {
#endif

_ACRTIMP __msvcrt_ulong __cdecl __threadid(void);
_ACRTIMP __msvcrt_ulong __cdecl __threadhandle(void);
#define _threadid    (__threadid())

#ifdef __cplusplus
}
#endif

#endif /* __WINE_STDDEF_H */
