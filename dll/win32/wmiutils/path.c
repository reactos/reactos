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

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "wbemcli.h"
#include "wmiutils.h"

#include "wine/debug.h"
#include "wmiutils_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(wmiutils);

struct keylist
{
    IWbemPathKeyList IWbemPathKeyList_iface;
    IWbemPath       *parent;
    LONG             refs;
};

struct key
{
    WCHAR *name;
    int    len_name;
    WCHAR *value;
    int    len_value;
};

struct path
{
    IWbemPath        IWbemPath_iface;
    LONG             refs;
    CRITICAL_SECTION cs;
    WCHAR           *text;
    int              len_text;
    WCHAR           *server;
    int              len_server;
    WCHAR          **namespaces;
    int             *len_namespaces;
    int              num_namespaces;
    WCHAR           *class;
    int              len_class;
    struct key      *keys;
    unsigned int     num_keys;
    ULONGLONG        flags;
};

static inline struct keylist *impl_from_IWbemPathKeyList( IWbemPathKeyList *iface )
{
    return CONTAINING_RECORD(iface, struct keylist, IWbemPathKeyList_iface);
}

static inline struct path *impl_from_IWbemPath( IWbemPath *iface )
{
    return CONTAINING_RECORD(iface, struct path, IWbemPath_iface);
}

static ULONG WINAPI keylist_AddRef(
    IWbemPathKeyList *iface )
{
    struct keylist *keylist = impl_from_IWbemPathKeyList( iface );
    return InterlockedIncrement( &keylist->refs );
}

static ULONG WINAPI keylist_Release(
    IWbemPathKeyList *iface )
{
    struct keylist *keylist = impl_from_IWbemPathKeyList( iface );
    LONG refs = InterlockedDecrement( &keylist->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", keylist);
        IWbemPath_Release( keylist->parent );
        free( keylist );
    }
    return refs;
}

static HRESULT WINAPI keylist_QueryInterface(
    IWbemPathKeyList *iface,
    REFIID riid,
    void **ppvObject )
{
    struct keylist *keylist = impl_from_IWbemPathKeyList( iface );

    TRACE("%p, %s, %p\n", keylist, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID( riid, &IID_IWbemPathKeyList ) ||
        IsEqualGUID( riid, &IID_IUnknown ))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IWbemPathKeyList_AddRef( iface );
    return S_OK;
}

static HRESULT WINAPI keylist_GetCount(
    IWbemPathKeyList *iface,
    ULONG *puKeyCount )
{
    struct keylist *keylist = impl_from_IWbemPathKeyList( iface );
    struct path *parent = impl_from_IWbemPath( keylist->parent );

    TRACE("%p, %p\n", iface, puKeyCount);

    if (!puKeyCount) return WBEM_E_INVALID_PARAMETER;

    EnterCriticalSection( &parent->cs );

    *puKeyCount = parent->num_keys;

    LeaveCriticalSection( &parent->cs );
    return S_OK;
}

static HRESULT WINAPI keylist_SetKey(
    IWbemPathKeyList *iface,
    LPCWSTR wszName,
    ULONG uFlags,
    ULONG uCimType,
    LPVOID pKeyVal )
{
    FIXME("%p, %s, %#lx, %lu, %p\n", iface, debugstr_w(wszName), uFlags, uCimType, pKeyVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI keylist_SetKey2(
    IWbemPathKeyList *iface,
    LPCWSTR wszName,
    ULONG uFlags,
    ULONG uCimType,
    VARIANT *pKeyVal )
{
    FIXME("%p, %s, %#lx, %lu, %p\n", iface, debugstr_w(wszName), uFlags, uCimType, pKeyVal);
    return E_NOTIMPL;
}

static HRESULT WINAPI keylist_GetKey(
    IWbemPathKeyList *iface,
    ULONG uKeyIx,
    ULONG uFlags,
    ULONG *puNameBufSize,
    LPWSTR pszKeyName,
    ULONG *puKeyValBufSize,
    LPVOID pKeyVal,
    ULONG *puApparentCimType )
{
    FIXME("%p, %lu, %#lx, %p, %p, %p, %p, %p\n", iface, uKeyIx, uFlags, puNameBufSize,
          pszKeyName, puKeyValBufSize, pKeyVal, puApparentCimType);
    return E_NOTIMPL;
}

static HRESULT WINAPI keylist_GetKey2(
    IWbemPathKeyList *iface,
    ULONG uKeyIx,
    ULONG uFlags,
    ULONG *puNameBufSize,
    LPWSTR pszKeyName,
    VARIANT *pKeyValue,
    ULONG *puApparentCimType )
{
    FIXME("%p, %lu, %#lx, %p, %p, %p, %p\n", iface, uKeyIx, uFlags, puNameBufSize,
          pszKeyName, pKeyValue, puApparentCimType);
    return E_NOTIMPL;
}

static HRESULT WINAPI keylist_RemoveKey(
    IWbemPathKeyList *iface,
    LPCWSTR wszName,
    ULONG uFlags )
{
    FIXME("%p, %s, %#lx\n", iface, debugstr_w(wszName), uFlags);
    return E_NOTIMPL;
}

static void free_keys( struct key *keys, unsigned int count )
{
    unsigned int i;

    for (i = 0; i < count; i++)
    {
        free( keys[i].name );
        free( keys[i].value );
    }
    free( keys );
}

static HRESULT WINAPI keylist_RemoveAllKeys(
    IWbemPathKeyList *iface,
    ULONG uFlags )
{
    struct keylist *keylist = impl_from_IWbemPathKeyList( iface );
    struct path *parent = impl_from_IWbemPath( keylist->parent );

    TRACE("%p, %#lx\n", iface, uFlags);

    if (uFlags) return WBEM_E_INVALID_PARAMETER;

    EnterCriticalSection( &parent->cs );

    free_keys( parent->keys, parent->num_keys );
    parent->num_keys = 0;
    parent->keys = NULL;

    LeaveCriticalSection( &parent->cs );
    return S_OK;
}

static HRESULT WINAPI keylist_MakeSingleton(
    IWbemPathKeyList *iface,
    boolean bSet )
{
    FIXME("%p, %d\n", iface, bSet);
    return E_NOTIMPL;
}

static HRESULT WINAPI keylist_GetInfo(
    IWbemPathKeyList *iface,
    ULONG uRequestedInfo,
    ULONGLONG *puResponse )
{
    FIXME("%p, %lu, %p\n", iface, uRequestedInfo, puResponse);
    return E_NOTIMPL;
}

static HRESULT WINAPI keylist_GetText(
    IWbemPathKeyList *iface,
    LONG lFlags,
    ULONG *puBuffLength,
    LPWSTR pszText )
{
    FIXME("%p, %#lx, %p, %p\n", iface, lFlags, puBuffLength, pszText);
    return E_NOTIMPL;
}

static const struct IWbemPathKeyListVtbl keylist_vtbl =
{
    keylist_QueryInterface,
    keylist_AddRef,
    keylist_Release,
    keylist_GetCount,
    keylist_SetKey,
    keylist_SetKey2,
    keylist_GetKey,
    keylist_GetKey2,
    keylist_RemoveKey,
    keylist_RemoveAllKeys,
    keylist_MakeSingleton,
    keylist_GetInfo,
    keylist_GetText
};

static HRESULT WbemPathKeyList_create( IWbemPath *parent, LPVOID *ppObj )
{
    struct keylist *keylist;

    TRACE("%p\n", ppObj);

    if (!(keylist = calloc( 1, sizeof(*keylist) ))) return E_OUTOFMEMORY;

    keylist->IWbemPathKeyList_iface.lpVtbl = &keylist_vtbl;
    keylist->refs = 1;
    keylist->parent = parent;
    IWbemPath_AddRef( keylist->parent );

    *ppObj = &keylist->IWbemPathKeyList_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}

static void init_path( struct path *path )
{
    path->text           = NULL;
    path->len_text       = 0;
    path->server         = NULL;
    path->len_server     = 0;
    path->namespaces     = NULL;
    path->len_namespaces = NULL;
    path->num_namespaces = 0;
    path->class          = NULL;
    path->len_class      = 0;
    path->keys           = NULL;
    path->num_keys       = 0;
    path->flags          = 0;
}

static void clear_path( struct path *path )
{
    unsigned int i;

    free( path->text );
    free( path->server );
    for (i = 0; i < path->num_namespaces; i++) free( path->namespaces[i] );
    free( path->namespaces );
    free( path->len_namespaces );
    free( path->class );
    free_keys( path->keys, path->num_keys );
    init_path( path );
}

static ULONG WINAPI path_AddRef(
    IWbemPath *iface )
{
    struct path *path = impl_from_IWbemPath( iface );
    return InterlockedIncrement( &path->refs );
}

static ULONG WINAPI path_Release(
    IWbemPath *iface )
{
    struct path *path = impl_from_IWbemPath( iface );
    LONG refs = InterlockedDecrement( &path->refs );
    if (!refs)
    {
        TRACE("destroying %p\n", path);
        clear_path( path );
        path->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection( &path->cs );
        free( path );
    }
    return refs;
}

static HRESULT WINAPI path_QueryInterface(
    IWbemPath *iface,
    REFIID riid,
    void **ppvObject )
{
    struct path *path = impl_from_IWbemPath( iface );

    TRACE("%p, %s, %p\n", path, debugstr_guid( riid ), ppvObject );

    if ( IsEqualGUID( riid, &IID_IWbemPath ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    IWbemPath_AddRef( iface );
    return S_OK;
}

static HRESULT parse_key( struct key *key, const WCHAR *str, unsigned int *ret_len )
{
    const WCHAR *p, *q;
    unsigned int len;

    p = q = str;
    while (*q && *q != '=')
    {
        if (*q == ',' || iswspace( *q )) return WBEM_E_INVALID_PARAMETER;
        q++;
    }
    len = q - p;
    if (!(key->name = malloc( (len + 1) * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
    memcpy( key->name, p, len * sizeof(WCHAR) );
    key->name[len] = 0;
    key->len_name = len;

    p = ++q;
    if (!*p || *p == ',' || iswspace( *p )) return WBEM_E_INVALID_PARAMETER;

    while (*q && *q != ',') q++;
    len = q - p;
    if (!(key->value = malloc( (len + 1) * sizeof(WCHAR) ))) return E_OUTOFMEMORY;
    memcpy( key->value, p, len * sizeof(WCHAR) );
    key->value[len] = 0;
    key->len_value = len;

    *ret_len = q - str;
    if (*q == ',') (*ret_len)++;
    return S_OK;
}

static HRESULT parse_text( struct path *path, ULONG mode, const WCHAR *text )
{
    HRESULT hr = E_OUTOFMEMORY;
    const WCHAR *p, *q;
    unsigned int i, len;

    p = q = text;
    if ((p[0] == '\\' && p[1] == '\\') || (p[0] == '/' && p[1] == '/'))
    {
        p += 2;
        q = p;
        while (*q && *q != '\\' && *q != '/') q++;
        len = q - p;
        if (!(path->server = malloc( (len + 1) * sizeof(WCHAR) ))) goto done;
        memcpy( path->server, p, len * sizeof(WCHAR) );
        path->server[len] = 0;
        path->len_server = len;
        path->flags |= WBEMPATH_INFO_PATH_HAD_SERVER;
    }
    p = q;
    if (wcschr( p, '\\' ) || wcschr( p, '/' ))
    {
        if (*q != '\\' && *q != '/' && *q != ':')
        {
            path->num_namespaces = 1;
            q++;
        }
        while (*q && *q != ':')
        {
            if (*q == '\\' || *q == '/') path->num_namespaces++;
            q++;
        }
    }
    if (path->num_namespaces)
    {
        if (!(path->namespaces = calloc( path->num_namespaces, sizeof(WCHAR *) ))) goto done;
        if (!(path->len_namespaces = malloc( path->num_namespaces * sizeof(int) ))) goto done;

        i = 0;
        q = p;
        if (*q && *q != '\\' && *q != '/' && *q != ':')
        {
            p = q;
            while (*p && *p != '\\' && *p != '/' && *p != ':') p++;
            len = p - q;
            if (!(path->namespaces[i] = malloc( (len + 1) * sizeof(WCHAR) ))) goto done;
            memcpy( path->namespaces[i], q, len * sizeof(WCHAR) );
            path->namespaces[i][len] = 0;
            path->len_namespaces[i] = len;
            q = p;
            i++;
        }
        while (*q && *q != ':')
        {
            if (*q == '\\' || *q == '/')
            {
                p = q + 1;
                while (*p && *p != '\\' && *p != '/' && *p != ':') p++;
                len = p - q - 1;
                if (!(path->namespaces[i] = malloc( (len + 1) * sizeof(WCHAR) ))) goto done;
                memcpy( path->namespaces[i], q + 1, len * sizeof(WCHAR) );
                path->namespaces[i][len] = 0;
                path->len_namespaces[i] = len;
                i++;
            }
            q++;
        }
    }
    if (*q == ':') q++;
    p = q;
    while (*q && *q != '.' && *q != '=') q++;
    len = q - p;
    if (!(path->class = malloc( (len + 1) * sizeof(WCHAR) ))) goto done;
    memcpy( path->class, p, len * sizeof(WCHAR) );
    path->class[len] = 0;
    path->len_class = len;

    if (*q == '.')
    {
        p = ++q;
        path->num_keys++;
        while (*q)
        {
            if (*q == ',') path->num_keys++;
            q++;
        }
        if (!(path->keys = calloc( path->num_keys, sizeof(struct key) ))) goto done;
        i = 0;
        q = p;
        while (*q)
        {
            if (i >= path->num_keys) break;
            hr = parse_key( &path->keys[i], q, &len );
            if (hr != S_OK) goto done;
            q += len;
            i++;
        }
    }
    else if (*q == '=')
    {
        path->num_keys = 1;
        if (!(path->keys = calloc( path->num_keys, sizeof(struct key) ))) goto done;
        hr = parse_key( &path->keys[0], q, &len );
        if (hr != S_OK) goto done;
    }
    hr = S_OK;

done:
    if (hr != S_OK) clear_path( path );
    else path->flags |= WBEMPATH_INFO_CIM_COMPLIANT | WBEMPATH_INFO_V2_COMPLIANT;
    return hr;
}

static HRESULT WINAPI path_SetText(
    IWbemPath *iface,
    ULONG uMode,
    LPCWSTR pszPath)
{
    struct path *path = impl_from_IWbemPath( iface );
    HRESULT hr = S_OK;
    int len;

    TRACE("%p, %lu, %s\n", iface, uMode, debugstr_w(pszPath));

    if (!uMode || !pszPath) return WBEM_E_INVALID_PARAMETER;

    EnterCriticalSection( &path->cs );

    clear_path( path );
    if (!pszPath[0]) goto done;
    if ((hr = parse_text( path, uMode, pszPath )) != S_OK) goto done;

    len = lstrlenW( pszPath );
    if (!(path->text = malloc( (len + 1) * sizeof(WCHAR) )))
    {
        clear_path( path );
        hr = E_OUTOFMEMORY;
        goto done;
    }
    lstrcpyW( path->text, pszPath );
    path->len_text = len;

done:
    LeaveCriticalSection( &path->cs );
    return hr;
}

static WCHAR *build_namespace( struct path *path, int *len, BOOL leading_slash )
{
    WCHAR *ret, *p;
    int i;

    *len = 0;
    for (i = 0; i < path->num_namespaces; i++)
    {
        if (i > 0 || leading_slash) *len += 1;
        *len += path->len_namespaces[i];
    }
    if (!(p = ret = malloc( (*len + 1) * sizeof(WCHAR) ))) return NULL;
    for (i = 0; i < path->num_namespaces; i++)
    {
        if (i > 0 || leading_slash) *p++ = '\\';
        memcpy( p, path->namespaces[i], path->len_namespaces[i] * sizeof(WCHAR) );
        p += path->len_namespaces[i];
    }
    *p = 0;
    return ret;
}

static WCHAR *build_server( struct path *path, int *len )
{
    WCHAR *ret, *p;

    *len = 0;
    if (path->len_server) *len += 2 + path->len_server;
    else *len += 3;
    if (!(p = ret = malloc( (*len + 1) * sizeof(WCHAR) ))) return NULL;
    if (path->len_server)
    {
        p[0] = p[1] = '\\';
        lstrcpyW( p + 2, path->server );
    }
    else
    {
        p[0] = p[1] = '\\';
        p[2] = '.';
        p[3] = 0;
    }
    return ret;
}

static WCHAR *build_keylist( struct path *path, int *len )
{
    WCHAR *ret, *p;
    unsigned int i;

    *len = 0;
    for (i = 0; i < path->num_keys; i++)
    {
        if (i > 0) *len += 1;
        *len += path->keys[i].len_name + path->keys[i].len_value + 1;
    }
    if (!(p = ret = malloc( (*len + 1) * sizeof(WCHAR) ))) return NULL;
    for (i = 0; i < path->num_keys; i++)
    {
        if (i > 0) *p++ = ',';
        memcpy( p, path->keys[i].name, path->keys[i].len_name * sizeof(WCHAR) );
        p += path->keys[i].len_name;
        *p++ = '=';
        memcpy( p, path->keys[i].value, path->keys[i].len_value * sizeof(WCHAR) );
        p += path->keys[i].len_value;
    }
    *p = 0;
    return ret;
}

static WCHAR *build_path( struct path *path, LONG flags, int *len )
{
    *len = 0;
    switch (flags)
    {
    case 0:
    {
        int len_namespace, len_keylist;
        WCHAR *ret, *namespace = build_namespace( path, &len_namespace, FALSE );
        WCHAR *keylist = build_keylist( path, &len_keylist );

        if (!namespace || !keylist)
        {
            free( namespace );
            free( keylist );
            return NULL;
        }
        *len = len_namespace;
        if (path->len_class)
        {
            *len += path->len_class + 1;
            if (path->num_keys) *len += len_keylist + 1;
        }
        if (!(ret = malloc( (*len + 1) * sizeof(WCHAR) )))
        {
            free( namespace );
            free( keylist );
            return NULL;
        }
        lstrcpyW( ret, namespace );
        if (path->len_class)
        {
            unsigned int offset = len_namespace + path->len_class + 1;

            ret[len_namespace] = ':';
            lstrcpyW( ret + len_namespace + 1, path->class );
            if (path->num_keys)
            {
                if (*keylist != '=') ret[offset++] = '.';
                lstrcpyW( ret + offset, keylist );
            }
        }
        free( namespace );
        free( keylist );
        return ret;

    }
    case WBEMPATH_GET_RELATIVE_ONLY:
    {
        int len_keylist;
        WCHAR *ret, *keylist;

        if (!path->len_class) return NULL;
        if (!(keylist = build_keylist( path, &len_keylist ))) return NULL;

        *len = path->len_class;
        if (path->num_keys) *len += len_keylist + 1;
        if (!(ret = malloc( (*len + 1) * sizeof(WCHAR) )))
        {
            free( keylist );
            return NULL;
        }
        lstrcpyW( ret, path->class );
        if (path->num_keys)
        {
            unsigned int offset = path->len_class;

            if (*keylist != '=') ret[offset++] = '.';
            lstrcpyW( ret + offset, keylist );
        }
        free( keylist );
        return ret;
    }
    case WBEMPATH_GET_SERVER_TOO:
    {
        int len_namespace, len_server, len_keylist;
        WCHAR *p, *ret, *namespace = build_namespace( path, &len_namespace, TRUE );
        WCHAR *server = build_server( path, &len_server );
        WCHAR *keylist = build_keylist( path, &len_keylist );

        if (!namespace || !server || !keylist)
        {
            free( namespace );
            free( server );
            free( keylist );
            return NULL;
        }
        *len = len_namespace + len_server;
        if (path->len_class)
        {
            *len += path->len_class + 1;
            if (path->num_keys) *len += len_keylist + 1;
        }
        if (!(p = ret = malloc( (*len + 1) * sizeof(WCHAR) )))
        {
            free( namespace );
            free( server );
            free( keylist );
            return NULL;
        }
        lstrcpyW( p, server );
        p += len_server;
        lstrcpyW( p, namespace );
        p += len_namespace;
        if (path->len_class)
        {
            *p++ = ':';
            lstrcpyW( p, path->class );
            if (path->num_keys)
            {
                unsigned int offset = path->len_class;

                if (*keylist != '=') p[offset++] = '.';
                lstrcpyW( p + offset, keylist );
            }
        }
        free( namespace );
        free( server );
        free( keylist );
        return ret;
    }
    case WBEMPATH_GET_SERVER_AND_NAMESPACE_ONLY:
    {
        int len_namespace, len_server;
        WCHAR *p, *ret, *namespace = build_namespace( path, &len_namespace, TRUE );
        WCHAR *server = build_server( path, &len_server );

        if (!namespace || !server)
        {
            free( namespace );
            free( server );
            return NULL;
        }
        *len = len_namespace + len_server;
        if (!(p = ret = malloc( (*len + 1) * sizeof(WCHAR) )))
        {
            free( namespace );
            free( server );
            return NULL;
        }
        lstrcpyW( p, server );
        p += len_server;
        lstrcpyW( p, namespace );
        free( namespace );
        free( server );
        return ret;
    }
    case WBEMPATH_GET_NAMESPACE_ONLY:
        return build_namespace( path, len, FALSE );

    case WBEMPATH_GET_ORIGINAL:
        if (!path->len_text) return NULL;
        *len = path->len_text;
        return wcsdup( path->text );

    default:
        ERR( "unhandled flags %#lx\n", flags );
        return NULL;
    }
}

static HRESULT WINAPI path_GetText(
    IWbemPath *iface,
    LONG lFlags,
    ULONG *puBufferLength,
    LPWSTR pszText)
{
    struct path *path = impl_from_IWbemPath( iface );
    HRESULT hr = S_OK;
    WCHAR *str;
    int len;

    TRACE("%p, %#lx, %p, %p\n", iface, lFlags, puBufferLength, pszText);

    if (!puBufferLength) return WBEM_E_INVALID_PARAMETER;

    EnterCriticalSection( &path->cs );

    str = build_path( path, lFlags, &len );
    if (*puBufferLength < len + 1)
    {
        *puBufferLength = len + 1;
        goto done;
    }
    if (!pszText)
    {
        hr = WBEM_E_INVALID_PARAMETER;
        goto done;
    }
    if (str) lstrcpyW( pszText, str );
    else pszText[0] = 0;
    *puBufferLength = len + 1;

    TRACE("returning %s\n", debugstr_w(pszText));

done:
    free( str );
    LeaveCriticalSection( &path->cs );
    return hr;
}

static HRESULT WINAPI path_GetInfo(
    IWbemPath *iface,
    ULONG info,
    ULONGLONG *response)
{
    struct path *path = impl_from_IWbemPath( iface );

    TRACE("%p, %lu, %p\n", iface, info, response);

    if (info || !response) return WBEM_E_INVALID_PARAMETER;

    FIXME("some flags are not implemented\n");

    EnterCriticalSection( &path->cs );

    *response = path->flags;
    if (!path->server || (path->len_server == 1 && path->server[0] == '.'))
        *response |= WBEMPATH_INFO_ANON_LOCAL_MACHINE;
    else
        *response |= WBEMPATH_INFO_HAS_MACHINE_NAME;

    if (!path->class)
        *response |= WBEMPATH_INFO_SERVER_NAMESPACE_ONLY;
    else
    {
        *response |= WBEMPATH_INFO_HAS_SUBSCOPES;
        if (path->num_keys)
        {
            unsigned int i;
            for (i = 0; i < path->num_keys; i++)
                if (!path->keys[i].name[0]) *response |= WBEMPATH_INFO_HAS_IMPLIED_KEY;
            *response |= WBEMPATH_INFO_IS_INST_REF;
        }
        else
            *response |= WBEMPATH_INFO_IS_CLASS_REF;
    }

    LeaveCriticalSection( &path->cs );
    return S_OK;
}

static HRESULT WINAPI path_SetServer(
    IWbemPath *iface,
    LPCWSTR name)
{
    struct path *path = impl_from_IWbemPath( iface );
    static const ULONGLONG flags =
        WBEMPATH_INFO_PATH_HAD_SERVER | WBEMPATH_INFO_V1_COMPLIANT |
        WBEMPATH_INFO_V2_COMPLIANT | WBEMPATH_INFO_CIM_COMPLIANT;
    WCHAR *server;

    TRACE("%p, %s\n", iface, debugstr_w(name));

    EnterCriticalSection( &path->cs );

    if (name)
    {
        if (!(server = wcsdup( name )))
        {
            LeaveCriticalSection( &path->cs );
            return WBEM_E_OUT_OF_MEMORY;
        }
        free( path->server );
        path->server = server;
        path->len_server = lstrlenW( path->server );
        path->flags |= flags;
    }
    else
    {
        free( path->server );
        path->server = NULL;
        path->len_server = 0;
        path->flags &= ~flags;
    }

    LeaveCriticalSection( &path->cs );
    return S_OK;
}

static HRESULT WINAPI path_GetServer(
    IWbemPath *iface,
    ULONG *len,
    LPWSTR name)
{
    struct path *path = impl_from_IWbemPath( iface );

    TRACE("%p, %p, %p\n", iface, len, name);

    if (!len || (*len && !name)) return WBEM_E_INVALID_PARAMETER;

    EnterCriticalSection( &path->cs );

    if (!path->server)
    {
        LeaveCriticalSection( &path->cs );
        return WBEM_E_NOT_AVAILABLE;
    }
    if (*len > path->len_server) lstrcpyW( name, path->server );
    *len = path->len_server + 1;

    LeaveCriticalSection( &path->cs );
    return S_OK;
}

static HRESULT WINAPI path_GetNamespaceCount(
    IWbemPath *iface,
    ULONG *puCount)
{
    struct path *path = impl_from_IWbemPath( iface );

    TRACE("%p, %p\n", iface, puCount);

    if (!puCount) return WBEM_E_INVALID_PARAMETER;

    EnterCriticalSection( &path->cs );
    *puCount = path->num_namespaces;
    LeaveCriticalSection( &path->cs );
    return S_OK;
}

static HRESULT WINAPI path_SetNamespaceAt(
    IWbemPath *iface,
    ULONG idx,
    LPCWSTR name)
{
    struct path *path = impl_from_IWbemPath( iface );
    static const ULONGLONG flags =
        WBEMPATH_INFO_V1_COMPLIANT | WBEMPATH_INFO_V2_COMPLIANT |
        WBEMPATH_INFO_CIM_COMPLIANT;
    int i, *tmp_len;
    WCHAR **tmp, *new;
    DWORD size;

    TRACE( "%p, %lu, %s\n", iface, idx, debugstr_w(name) );

    EnterCriticalSection( &path->cs );

    if (idx > path->num_namespaces || !name)
    {
        LeaveCriticalSection( &path->cs );
        return WBEM_E_INVALID_PARAMETER;
    }
    if (!(new = wcsdup( name )))
    {
        LeaveCriticalSection( &path->cs );
        return WBEM_E_OUT_OF_MEMORY;
    }
    size = (path->num_namespaces + 1) * sizeof(WCHAR *);
    if (path->namespaces) tmp = realloc( path->namespaces, size );
    else tmp = malloc( size );
    if (!tmp)
    {
        free( new );
        LeaveCriticalSection( &path->cs );
        return WBEM_E_OUT_OF_MEMORY;
    }
    path->namespaces = tmp;
    size = (path->num_namespaces + 1) * sizeof(int);
    if (path->len_namespaces) tmp_len = realloc( path->len_namespaces, size );
    else tmp_len = malloc( size );
    if (!tmp_len)
    {
        free( new );
        LeaveCriticalSection( &path->cs );
        return WBEM_E_OUT_OF_MEMORY;
    }
    path->len_namespaces = tmp_len;
    for (i = idx; i < path->num_namespaces; i++)
    {
        path->namespaces[i + 1] = path->namespaces[i];
        path->len_namespaces[i + 1] = path->len_namespaces[i];
    }
    path->namespaces[idx] = new;
    path->len_namespaces[idx] = lstrlenW( new );
    path->num_namespaces++;
    path->flags |= flags;

    LeaveCriticalSection( &path->cs );
    return S_OK;
}

static HRESULT WINAPI path_GetNamespaceAt(
    IWbemPath *iface,
    ULONG idx,
    ULONG *len,
    LPWSTR name)
{
    struct path *path = impl_from_IWbemPath( iface );

    TRACE( "%p, %lu, %p, %p\n", iface, idx, len, name );

    EnterCriticalSection( &path->cs );

    if (!len || (*len && !name) || idx >= path->num_namespaces)
    {
        LeaveCriticalSection( &path->cs );
        return WBEM_E_INVALID_PARAMETER;
    }
    if (*len > path->len_namespaces[idx]) lstrcpyW( name, path->namespaces[idx] );
    *len = path->len_namespaces[idx] + 1;

    LeaveCriticalSection( &path->cs );
    return S_OK;
}

static HRESULT WINAPI path_RemoveNamespaceAt(
    IWbemPath *iface,
    ULONG idx)
{
    struct path *path = impl_from_IWbemPath( iface );

    TRACE( "%p, %lu\n", iface, idx );

    EnterCriticalSection( &path->cs );

    if (idx >= path->num_namespaces)
    {
        LeaveCriticalSection( &path->cs );
        return WBEM_E_INVALID_PARAMETER;
    }
    free( path->namespaces[idx] );
    while (idx < path->num_namespaces - 1)
    {
        path->namespaces[idx] = path->namespaces[idx + 1];
        path->len_namespaces[idx] = path->len_namespaces[idx + 1];
        idx++;
    }
    path->num_namespaces--;

    LeaveCriticalSection( &path->cs );
    return S_OK;
}

static HRESULT WINAPI path_RemoveAllNamespaces(
    IWbemPath *iface)
{
    struct path *path = impl_from_IWbemPath( iface );
    int i;

    TRACE("%p\n", iface);

    EnterCriticalSection( &path->cs );

    for (i = 0; i < path->num_namespaces; i++) free( path->namespaces[i] );
    path->num_namespaces = 0;
    free( path->namespaces );
    path->namespaces = NULL;
    free( path->len_namespaces );
    path->len_namespaces = NULL;

    LeaveCriticalSection( &path->cs );
    return S_OK;
}

static HRESULT WINAPI path_GetScopeCount(
    IWbemPath *iface,
    ULONG *puCount)
{
    FIXME("%p, %p\n", iface, puCount);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_SetScope(
    IWbemPath *iface,
    ULONG uIndex,
    LPWSTR pszClass)
{
    FIXME("%p, %lu, %s\n", iface, uIndex, debugstr_w(pszClass));
    return E_NOTIMPL;
}

static HRESULT WINAPI path_SetScopeFromText(
    IWbemPath *iface,
    ULONG uIndex,
    LPWSTR pszText)
{
    FIXME("%p, %lu, %s\n", iface, uIndex, debugstr_w(pszText));
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetScope(
    IWbemPath *iface,
    ULONG uIndex,
    ULONG *puClassNameBufSize,
    LPWSTR pszClass,
    IWbemPathKeyList **pKeyList)
{
    FIXME("%p, %lu, %p, %p, %p\n", iface, uIndex, puClassNameBufSize, pszClass, pKeyList);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetScopeAsText(
    IWbemPath *iface,
    ULONG uIndex,
    ULONG *puTextBufSize,
    LPWSTR pszText)
{
    FIXME("%p, %lu, %p, %p\n", iface, uIndex, puTextBufSize, pszText);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_RemoveScope(
    IWbemPath *iface,
    ULONG uIndex)
{
    FIXME("%p, %lu\n", iface, uIndex);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_RemoveAllScopes(
    IWbemPath *iface)
{
    FIXME("%p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_SetClassName(
    IWbemPath *iface,
    LPCWSTR name)
{
    struct path *path = impl_from_IWbemPath( iface );
    WCHAR *class;

    TRACE("%p, %s\n", iface, debugstr_w(name));

    if (!name) return WBEM_E_INVALID_PARAMETER;
    if (!(class = wcsdup( name ))) return WBEM_E_OUT_OF_MEMORY;

    EnterCriticalSection( &path->cs );

    free( path->class );
    path->class = class;
    path->len_class = lstrlenW( path->class );
    path->flags |= WBEMPATH_INFO_V2_COMPLIANT | WBEMPATH_INFO_CIM_COMPLIANT;

    LeaveCriticalSection( &path->cs );
    return S_OK;
}

static HRESULT WINAPI path_GetClassName(
    IWbemPath *iface,
    ULONG *len,
    LPWSTR name)
{
    struct path *path = impl_from_IWbemPath( iface );

    TRACE("%p, %p, %p\n", iface, len, name);

    if (!len || (*len && !name)) return WBEM_E_INVALID_PARAMETER;

    EnterCriticalSection( &path->cs );

    if (!path->class)
    {
        LeaveCriticalSection( &path->cs );
        return WBEM_E_INVALID_OBJECT_PATH;
    }
    if (*len > path->len_class) lstrcpyW( name, path->class );
    *len = path->len_class + 1;

    LeaveCriticalSection( &path->cs );
    return S_OK;
}

static HRESULT WINAPI path_GetKeyList(
    IWbemPath *iface,
    IWbemPathKeyList **pOut)
{
    struct path *path = impl_from_IWbemPath( iface );
    HRESULT hr;

    TRACE("%p, %p\n", iface, pOut);

    EnterCriticalSection( &path->cs );

    if (!path->class)
    {
        LeaveCriticalSection( &path->cs );
        return WBEM_E_INVALID_PARAMETER;
    }
    hr = WbemPathKeyList_create( iface, (void **)pOut );

    LeaveCriticalSection( &path->cs );
    return hr;
}

static HRESULT WINAPI path_CreateClassPart(
    IWbemPath *iface,
    LONG lFlags,
    LPCWSTR Name)
{
    FIXME("%p, %#lx, %s\n", iface, lFlags, debugstr_w(Name));
    return E_NOTIMPL;
}

static HRESULT WINAPI path_DeleteClassPart(
    IWbemPath *iface,
    LONG lFlags)
{
    FIXME("%p, %#lx\n", iface, lFlags);
    return E_NOTIMPL;
}

static BOOL WINAPI path_IsRelative(
    IWbemPath *iface,
    LPWSTR wszMachine,
    LPWSTR wszNamespace)
{
    FIXME("%p, %s, %s\n", iface, debugstr_w(wszMachine), debugstr_w(wszNamespace));
    return FALSE;
}

static BOOL WINAPI path_IsRelativeOrChild(
    IWbemPath *iface,
    LPWSTR wszMachine,
    LPWSTR wszNamespace,
    LONG lFlags)
{
    FIXME("%p, %s, %s, %#lx\n", iface, debugstr_w(wszMachine), debugstr_w(wszNamespace), lFlags);
    return FALSE;
}

static BOOL WINAPI path_IsLocal(
    IWbemPath *iface,
    LPCWSTR wszMachine)
{
    FIXME("%p, %s\n", iface, debugstr_w(wszMachine));
    return FALSE;
}

static BOOL WINAPI path_IsSameClassName(
    IWbemPath *iface,
    LPCWSTR wszClass)
{
    FIXME("%p, %s\n", iface, debugstr_w(wszClass));
    return FALSE;
}

static const struct IWbemPathVtbl path_vtbl =
{
    path_QueryInterface,
    path_AddRef,
    path_Release,
    path_SetText,
    path_GetText,
    path_GetInfo,
    path_SetServer,
    path_GetServer,
    path_GetNamespaceCount,
    path_SetNamespaceAt,
    path_GetNamespaceAt,
    path_RemoveNamespaceAt,
    path_RemoveAllNamespaces,
    path_GetScopeCount,
    path_SetScope,
    path_SetScopeFromText,
    path_GetScope,
    path_GetScopeAsText,
    path_RemoveScope,
    path_RemoveAllScopes,
    path_SetClassName,
    path_GetClassName,
    path_GetKeyList,
    path_CreateClassPart,
    path_DeleteClassPart,
    path_IsRelative,
    path_IsRelativeOrChild,
    path_IsLocal,
    path_IsSameClassName
};

HRESULT WbemPath_create( LPVOID *ppObj )
{
    struct path *path;

    TRACE("%p\n", ppObj);

    if (!(path = calloc( 1, sizeof(*path) ))) return E_OUTOFMEMORY;

    path->IWbemPath_iface.lpVtbl = &path_vtbl;
    path->refs = 1;
    InitializeCriticalSectionEx( &path->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO );
    path->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": wmiutils_path.cs");
    init_path( path );

    *ppObj = &path->IWbemPath_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
