/*
 *    XMLLite private things
 *
 * Copyright 2012 Nikolay Sivov for CodeWeavers
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

#ifndef __XMLLITE_PRIVATE__
#define __XMLLITE_PRIVATE__

#include <config.h>

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <xmllite.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(xmllite);

/* memory allocation functions */
static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline void *heap_realloc(void *mem, size_t len)
{
    return HeapReAlloc(GetProcessHeap(), 0, mem, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static inline void *m_alloc(IMalloc *imalloc, size_t len)
{
    if (imalloc)
        return IMalloc_Alloc(imalloc, len);
    else
        return heap_alloc(len);
}

static inline void *m_realloc(IMalloc *imalloc, void *mem, size_t len)
{
    if (imalloc)
        return IMalloc_Realloc(imalloc, mem, len);
    else
        return heap_realloc(mem, len);
}

static inline void m_free(IMalloc *imalloc, void *mem)
{
    if (imalloc)
        IMalloc_Free(imalloc, mem);
    else
        heap_free(mem);
}

typedef enum
{
    XmlEncoding_UTF16,
    XmlEncoding_UTF8,
    XmlEncoding_Unknown
} xml_encoding;

xml_encoding parse_encoding_name(const WCHAR*,int) DECLSPEC_HIDDEN;
HRESULT get_code_page(xml_encoding,UINT*) DECLSPEC_HIDDEN;
const WCHAR *get_encoding_name(xml_encoding) DECLSPEC_HIDDEN;
xml_encoding get_encoding_from_codepage(UINT) DECLSPEC_HIDDEN;

#endif /* __XMLLITE_PRIVATE__ */
