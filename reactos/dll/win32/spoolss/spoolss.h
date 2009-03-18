/*
 * Copyright 2006-2009 Detlef Riekenberg
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

#include "windef.h"
#include "winbase.h"

/* ################################ */

BOOL backend_load_all(void);
void backend_unload_all(void);

/* ## Memory allocation functions ## */

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc( size_t len )
{
    return HeapAlloc( GetProcessHeap(), 0, len );
}

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc_zero( size_t len )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len );
}

static inline BOOL heap_free( void *mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}
