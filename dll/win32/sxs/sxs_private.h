/*
 * Copyright 2012 Hans Leidekker for CodeWeavers
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

#pragma once

enum name_attr_id
{
    NAME_ATTR_ID_NAME,
    NAME_ATTR_ID_ARCH,
    NAME_ATTR_ID_TOKEN,
    NAME_ATTR_ID_TYPE,
    NAME_ATTR_ID_VERSION
};

const WCHAR *get_name_attribute( IAssemblyName *, enum name_attr_id ) DECLSPEC_HIDDEN;

static inline WCHAR *strdupW( const WCHAR *src )
{
    WCHAR *dst;

    if (!src) return NULL;
    dst = HeapAlloc( GetProcessHeap(), 0, (lstrlenW( src ) + 1) * sizeof(WCHAR) );
    if (dst) lstrcpyW( dst, src );
    return dst;
}
