/*
 * Wine heap memory allocation wrappers
 *
 * Copyright 2006 Jacek Caban for CodeWeavers
 * Copyright 2013, 2018 Michael Stefaniuc
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

#ifndef __WINE_WINE_HEAP_H
#define __WINE_WINE_HEAP_H

#include <winbase.h>

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc_zero(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

static inline void * __WINE_ALLOC_SIZE(2) heap_realloc(void *mem, SIZE_T len)
{
    if (!mem)
        return HeapAlloc(GetProcessHeap(), 0, len);
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline void heap_free(void *mem)
{
    HeapFree(GetProcessHeap(), 0, mem);
}

static inline void *heap_calloc(SIZE_T count, SIZE_T size)
{
    SIZE_T len = count * size;

    if (size && len / size != count)
        return NULL;
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

#endif  /* __WINE_WINE_HEAP_H */
