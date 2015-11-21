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

#ifndef _SXS_PRIVATE_H_
#define _SXS_PRIVATE_H_

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <winsxs.h>

#include <wine/debug.h>
#include <wine/unicode.h>

WINE_DEFAULT_DEBUG_CHANNEL(sxs);

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
    dst = HeapAlloc( GetProcessHeap(), 0, (strlenW( src ) + 1) * sizeof(WCHAR) );
    if (dst) strcpyW( dst, src );
    return dst;
}

#endif /* _SXS_PRIVATE_H_ */
