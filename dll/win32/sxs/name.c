/*
 * IAssemblyName implementation
 *
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

#include <stdarg.h>
#ifdef __REACTOS__
#include <wchar.h>
#endif

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "winsxs.h"

#include "wine/debug.h"
#include "sxs_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sxs);

struct name
{
    IAssemblyName IAssemblyName_iface;
    LONG   refs;
    WCHAR *name;
    WCHAR *arch;
    WCHAR *token;
    WCHAR *type;
    WCHAR *version;
};

static const WCHAR archW[] = {'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e',0};
static const WCHAR tokenW[] = {'p','u','b','l','i','c','K','e','y','T','o','k','e','n',0};
static const WCHAR typeW[] = {'t','y','p','e',0};
static const WCHAR versionW[] = {'v','e','r','s','i','o','n',0};

static inline struct name *impl_from_IAssemblyName( IAssemblyName *iface )
{
    return CONTAINING_RECORD( iface, struct name, IAssemblyName_iface );
}

static HRESULT WINAPI name_QueryInterface(
    IAssemblyName *iface,
    REFIID riid,
    void **obj )
{
    struct name *name = impl_from_IAssemblyName( iface );

    TRACE("%p, %s, %p\n", name, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID( riid, &IID_IUnknown ) ||
        IsEqualIID( riid, &IID_IAssemblyName ))
    {
        IAssemblyName_AddRef( iface );
        *obj = name;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI name_AddRef(
    IAssemblyName *iface )
{
    struct name *name = impl_from_IAssemblyName( iface );
    return InterlockedIncrement( &name->refs );
}

static ULONG WINAPI name_Release( IAssemblyName *iface )
{
    struct name *name = impl_from_IAssemblyName( iface );
    ULONG refs = InterlockedDecrement( &name->refs );

    if (!refs)
    {
        TRACE("destroying %p\n", name);
        HeapFree( GetProcessHeap(), 0, name->name );
        HeapFree( GetProcessHeap(), 0, name->arch );
        HeapFree( GetProcessHeap(), 0, name->token );
        HeapFree( GetProcessHeap(), 0, name->type );
        HeapFree( GetProcessHeap(), 0, name->version );
        HeapFree( GetProcessHeap(), 0, name );
    }
    return refs;
}

static HRESULT WINAPI name_SetProperty(
    IAssemblyName *iface,
    DWORD id,
    LPVOID property,
    DWORD size )
{
    FIXME("%p, %d, %p, %d\n", iface, id, property, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_GetProperty(
    IAssemblyName *iface,
    DWORD id,
    LPVOID buffer,
    LPDWORD buflen )
{
    FIXME("%p, %d, %p, %p\n", iface, id, buffer, buflen);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_Finalize(
    IAssemblyName *iface )
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_GetDisplayName(
    IAssemblyName *iface,
    LPOLESTR buffer,
    LPDWORD buflen,
    DWORD flags )
{
    static const WCHAR fmtW[] = {',','%','s','=','\"','%','s','\"',0};
    struct name *name = impl_from_IAssemblyName( iface );
    WCHAR version[30];
    unsigned int len;

    TRACE("%p, %p, %p, 0x%08x\n", iface, buffer, buflen, flags);

    if (!buflen || flags) return E_INVALIDARG;

    len = lstrlenW( name->name ) + 1;
    if (name->arch)    len += lstrlenW( archW ) + lstrlenW( name->arch ) + 4;
    if (name->token)   len += lstrlenW( tokenW ) + lstrlenW( name->token ) + 4;
    if (name->type)    len += lstrlenW( typeW ) + lstrlenW( name->type ) + 4;
    if (name->version) len += lstrlenW( versionW ) + lstrlenW( version ) + 4;
    if (len > *buflen)
    {
        *buflen = len;
        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    }
    lstrcpyW( buffer, name->name );
    len = lstrlenW( buffer );
    if (name->arch)    len += swprintf( buffer + len, fmtW, archW, name->arch );
    if (name->token)   len += swprintf( buffer + len, fmtW, tokenW, name->token );
    if (name->type)    len += swprintf( buffer + len, fmtW, typeW, name->type );
    if (name->version) len += swprintf( buffer + len, fmtW, versionW, name->version );
    return S_OK;
}

static HRESULT WINAPI name_Reserved(
    IAssemblyName *iface,
    REFIID riid,
    IUnknown *pUnkReserved1,
    IUnknown *pUnkReserved2,
    LPCOLESTR szReserved,
    LONGLONG llReserved,
    LPVOID pvReserved,
    DWORD cbReserved,
    LPVOID *ppReserved )
{
    FIXME("%p, %s, %p, %p, %s, %s, %p, %d, %p\n", iface,
          debugstr_guid(riid), pUnkReserved1, pUnkReserved2,
          debugstr_w(szReserved), wine_dbgstr_longlong(llReserved),
          pvReserved, cbReserved, ppReserved);
    return E_NOTIMPL;
}

const WCHAR *get_name_attribute( IAssemblyName *iface, enum name_attr_id id )
{
    struct name *name = impl_from_IAssemblyName( iface );

    switch (id)
    {
    case NAME_ATTR_ID_NAME:    return name->name;
    case NAME_ATTR_ID_ARCH:    return name->arch;
    case NAME_ATTR_ID_TOKEN:   return name->token;
    case NAME_ATTR_ID_TYPE:    return name->type;
    case NAME_ATTR_ID_VERSION: return name->version;
    default:
        ERR("unhandled name attribute %u\n", id);
        break;
    }
    return NULL;
}

static HRESULT WINAPI name_GetName(
    IAssemblyName *iface,
    LPDWORD buflen,
    WCHAR *buffer )
{
    const WCHAR *name;
    int len;

    TRACE("%p, %p, %p\n", iface, buflen, buffer);

    if (!buflen || !buffer) return E_INVALIDARG;

    name = get_name_attribute( iface, NAME_ATTR_ID_NAME );
    len = lstrlenW( name ) + 1;
    if (len > *buflen)
    {
        *buflen = len;
        return HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
    }
    lstrcpyW( buffer, name );
    *buflen = len + 3;
    return S_OK;
}

static HRESULT parse_version( WCHAR *version, DWORD *high, DWORD *low )
{
    WORD ver[4];
    WCHAR *p, *q;
    unsigned int i;

    memset( ver, 0, sizeof(ver) );
    for (i = 0, p = version; i < 4; i++)
    {
        if (!*p) break;
        q = wcschr( p, '.' );
        if (q) *q = 0;
        ver[i] = wcstol( p, NULL, 10 );
        if (!q && i < 3) break;
        p = q + 1;
    }
    *high = (ver[0] << 16) + ver[1];
    *low = (ver[2] << 16) + ver[3];
    return S_OK;
}

static HRESULT WINAPI name_GetVersion(
    IAssemblyName *iface,
    LPDWORD high,
    LPDWORD low )
{
    struct name *name = impl_from_IAssemblyName( iface );
    WCHAR *version;
    HRESULT hr;

    TRACE("%p, %p, %p\n", iface, high, low);

    if (!name->version) return HRESULT_FROM_WIN32( ERROR_NOT_FOUND );
    if (!(version = strdupW( name->version ))) return E_OUTOFMEMORY;
    hr = parse_version( version, high, low );
    HeapFree( GetProcessHeap(), 0, version );
    return hr;
}

static HRESULT WINAPI name_IsEqual(
    IAssemblyName *name1,
    IAssemblyName *name2,
    DWORD flags )
{
    FIXME("%p, %p, 0x%08x\n", name1, name2, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI name_Clone(
    IAssemblyName *iface,
    IAssemblyName **name )
{
    FIXME("%p, %p\n", iface, name);
    return E_NOTIMPL;
}

static const IAssemblyNameVtbl name_vtbl =
{
    name_QueryInterface,
    name_AddRef,
    name_Release,
    name_SetProperty,
    name_GetProperty,
    name_Finalize,
    name_GetDisplayName,
    name_Reserved,
    name_GetName,
    name_GetVersion,
    name_IsEqual,
    name_Clone
};

static WCHAR *parse_value( const WCHAR *str, unsigned int *len )
{
    WCHAR *ret;
    const WCHAR *p = str;

    if (*p++ != '\"') return NULL;
    while (*p && *p != '\"') p++;
    if (!*p) return NULL;

    *len = p - str;
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, *len * sizeof(WCHAR) ))) return NULL;
    memcpy( ret, str + 1, (*len - 1) * sizeof(WCHAR) );
    ret[*len - 1] = 0;
    return ret;
}

static HRESULT parse_displayname( struct name *name, const WCHAR *displayname )
{
    const WCHAR *p, *q;
    unsigned int len;

    p = q = displayname;
    while (*q && *q != ',') q++;
    len = q - p;
    if (!(name->name = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
    memcpy( name->name, p, len * sizeof(WCHAR) );
    name->name[len] = 0;
    if (!*q) return S_OK;

    for (;;)
    {
        p = ++q;
        while (*q && *q != '=') q++;
        if (!*q) return E_INVALIDARG;
        len = q - p;
        if (len == ARRAY_SIZE(archW) - 1 && !memcmp( p, archW, len * sizeof(WCHAR) ))
        {
            p = ++q;
            if (!(name->arch = parse_value( p, &len ))) return E_INVALIDARG;
            q += len;
        }
        else if (len == ARRAY_SIZE(tokenW) - 1 && !memcmp( p, tokenW, len * sizeof(WCHAR) ))
        {
            p = ++q;
            if (!(name->token = parse_value( p, &len ))) return E_INVALIDARG;
            q += len;
        }
        else if (len == ARRAY_SIZE(typeW) - 1 && !memcmp( p, typeW, len * sizeof(WCHAR) ))
        {
            p = ++q;
            if (!(name->type = parse_value( p, &len ))) return E_INVALIDARG;
            q += len;
        }
        else if (len == ARRAY_SIZE(versionW) - 1 && !memcmp( p, versionW, len * sizeof(WCHAR) ))
        {
            p = ++q;
            if (!(name->version = parse_value( p, &len ))) return E_INVALIDARG;
            q += len;
        }
        else return HRESULT_FROM_WIN32( ERROR_SXS_INVALID_ASSEMBLY_IDENTITY_ATTRIBUTE_NAME );
        while (*q && *q != ',') q++;
        if (!*q) break;
    }
    return S_OK;
}

/******************************************************************
 *  CreateAssemblyNameObject   (SXS.@)
 */
HRESULT WINAPI CreateAssemblyNameObject(
    LPASSEMBLYNAME *obj,
    LPCWSTR assembly,
    DWORD flags,
    LPVOID reserved )
{
    struct name *name;
    HRESULT hr;

    TRACE("%p, %s, 0x%08x, %p\n", obj, debugstr_w(assembly), flags, reserved);

    if (!obj) return E_INVALIDARG;

    *obj = NULL;
    if (!assembly || !assembly[0] || flags != CANOF_PARSE_DISPLAY_NAME)
        return E_INVALIDARG;

    if (!(name = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*name) )))
        return E_OUTOFMEMORY;

    name->IAssemblyName_iface.lpVtbl = &name_vtbl;
    name->refs = 1;

    hr = parse_displayname( name, assembly );
    if (hr != S_OK)
    {
        HeapFree( GetProcessHeap(), 0, name->name );
        HeapFree( GetProcessHeap(), 0, name->arch );
        HeapFree( GetProcessHeap(), 0, name->token );
        HeapFree( GetProcessHeap(), 0, name->type );
        HeapFree( GetProcessHeap(), 0, name->version );
        HeapFree( GetProcessHeap(), 0, name );
        return hr;
    }
    *obj = &name->IAssemblyName_iface;
    return S_OK;
}
