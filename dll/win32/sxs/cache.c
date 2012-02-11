/*
 * IAssemblyCache implementation
 *
 * Copyright 2010 Hans Leidekker for CodeWeavers
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

#define COBJMACROS
#define INITGUID

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "winsxs.h"
#include "msxml2.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(sxs);

static inline WCHAR *strdupW( const WCHAR *s )
{
    WCHAR *t;
    if (!s) return NULL;
    if ((t = HeapAlloc( GetProcessHeap(), 0, (strlenW( s ) + 1) * sizeof(WCHAR) ))) strcpyW( t, s );
    return t;
}

struct cache
{
    IAssemblyCache IAssemblyCache_iface;
    LONG refs;
};

static inline struct cache *impl_from_IAssemblyCache(IAssemblyCache *iface)
{
    return CONTAINING_RECORD(iface, struct cache, IAssemblyCache_iface);
}

static HRESULT WINAPI cache_QueryInterface(
    IAssemblyCache *iface,
    REFIID riid,
    void **obj )
{
    struct cache *cache = impl_from_IAssemblyCache(iface);

    TRACE("%p, %s, %p\n", cache, debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyCache))
    {
        IUnknown_AddRef( iface );
        *obj = cache;
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI cache_AddRef( IAssemblyCache *iface )
{
    struct cache *cache = impl_from_IAssemblyCache(iface);
    return InterlockedIncrement( &cache->refs );
}

static ULONG WINAPI cache_Release( IAssemblyCache *iface )
{
    struct cache *cache = impl_from_IAssemblyCache(iface);
    ULONG refs = InterlockedDecrement( &cache->refs );

    if (!refs)
    {
        TRACE("destroying %p\n", cache);
        HeapFree( GetProcessHeap(), 0, cache );
    }
    return refs;
}

static HRESULT WINAPI cache_UninstallAssembly(
    IAssemblyCache *iface,
    DWORD flags,
    LPCWSTR name,
    LPCFUSION_INSTALL_REFERENCE ref,
    ULONG *disp )
{
    FIXME("%p, 0x%08x, %s, %p, %p\n", iface, flags, debugstr_w(name), ref, disp);
    return E_NOTIMPL;
}

static HRESULT WINAPI cache_QueryAssemblyInfo(
    IAssemblyCache *iface,
    DWORD flags,
    LPCWSTR name,
    ASSEMBLY_INFO *info )
{
    FIXME("%p, 0x%08x, %s, %p\n", iface, flags, debugstr_w(name), info);
    return E_NOTIMPL;
}

static HRESULT WINAPI cache_CreateAssemblyCacheItem(
    IAssemblyCache *iface,
    DWORD flags,
    PVOID reserved,
    IAssemblyCacheItem **item,
    LPCWSTR name )
{
    FIXME("%p, 0x%08x, %p, %p, %s\n", iface, flags, reserved, item, debugstr_w(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI cache_Reserved(
    IAssemblyCache *iface,
    IUnknown **reserved)
{
    FIXME("%p\n", reserved);
    return E_NOTIMPL;
}

static BSTR get_attribute_value( IXMLDOMNamedNodeMap *map, const WCHAR *value_name )
{
    HRESULT hr;
    IXMLDOMNode *attr;
    VARIANT var;
    BSTR str;

    str = SysAllocString( value_name );
    hr = IXMLDOMNamedNodeMap_getNamedItem( map, str, &attr );
    SysFreeString( str );
    if (hr != S_OK) return NULL;

    hr = IXMLDOMNode_get_nodeValue( attr, &var );
    IXMLDOMNode_Release( attr );
    if (hr != S_OK) return NULL;
    if (V_VT(&var) != VT_BSTR)
    {
        VariantClear( &var );
        return NULL;
    }
    TRACE("%s=%s\n", debugstr_w(value_name), debugstr_w(V_BSTR( &var )));
    return V_BSTR( &var );
}

struct file
{
    struct list entry;
    BSTR name;
};

struct assembly
{
    BSTR type;
    BSTR name;
    BSTR version;
    BSTR arch;
    BSTR token;
    struct list files;
};

static void free_assembly( struct assembly *assembly )
{
    struct list *item, *cursor;

    if (!assembly) return;
    SysFreeString( assembly->type );
    SysFreeString( assembly->name );
    SysFreeString( assembly->version );
    SysFreeString( assembly->arch );
    SysFreeString( assembly->token );
    LIST_FOR_EACH_SAFE( item, cursor, &assembly->files )
    {
        struct file *file = LIST_ENTRY( item, struct file, entry );
        list_remove( &file->entry );
        SysFreeString( file->name );
        HeapFree( GetProcessHeap(), 0, file );
    }
    HeapFree( GetProcessHeap(), 0, assembly );
}

static HRESULT parse_files( IXMLDOMDocument *doc, struct assembly *assembly )
{
    static const WCHAR fileW[] = {'f','i','l','e',0};
    static const WCHAR nameW[] = {'n','a','m','e',0};
    IXMLDOMNamedNodeMap *attrs;
    IXMLDOMNodeList *list;
    IXMLDOMNode *node;
    struct file *f;
    BSTR str;
    HRESULT hr;
    LONG len;

    str = SysAllocString( fileW );
    hr = IXMLDOMDocument_getElementsByTagName( doc, str, &list );
    SysFreeString( str );
    if (hr != S_OK) return hr;

    hr = IXMLDOMNodeList_get_length( list, &len );
    if (hr != S_OK) goto done;
    TRACE("found %d files\n", len);
    if (!len)
    {
        hr = ERROR_SXS_MANIFEST_FORMAT_ERROR;
        goto done;
    }

    for (;;)
    {
        hr = IXMLDOMNodeList_nextNode( list, &node );
        if (hr != S_OK || !node)
        {
            hr = S_OK;
            break;
        }

        /* FIXME: validate node type */

        hr = IXMLDOMNode_get_attributes( node, &attrs );
        IXMLDOMNode_Release( node );
        if (hr != S_OK)
            goto done;

        if (!(f = HeapAlloc( GetProcessHeap(), 0, sizeof(struct file) )))
        {
            IXMLDOMNamedNodeMap_Release( attrs );
            hr = E_OUTOFMEMORY;
            goto done;
        }

        f->name = get_attribute_value( attrs, nameW );
        IXMLDOMNamedNodeMap_Release( attrs );
        if (!f->name)
        {
            HeapFree( GetProcessHeap(), 0, f );
            hr = ERROR_SXS_MANIFEST_FORMAT_ERROR;
            goto done;
        }
        list_add_tail( &assembly->files, &f->entry );
    }

    if (list_empty( &assembly->files ))
    {
        WARN("no files found\n");
        hr = ERROR_SXS_MANIFEST_FORMAT_ERROR;
    }

done:
    IXMLDOMNodeList_Release( list );
    return hr;
}

static HRESULT parse_assembly( IXMLDOMDocument *doc, struct assembly **assembly )
{
    static const WCHAR identityW[] = {'a','s','s','e','m','b','l','y','I','d','e','n','t','i','t','y',0};
    static const WCHAR typeW[] = {'t','y','p','e',0};
    static const WCHAR nameW[] = {'n','a','m','e',0};
    static const WCHAR versionW[] = {'v','e','r','s','i','o','n',0};
    static const WCHAR architectureW[] = {'p','r','o','c','e','s','s','o','r','A','r','c','h','i','t','e','c','t','u','r','e',0};
    static const WCHAR tokenW[] = {'p','u','b','l','i','c','K','e','y','T','o','k','e','n',0};
    static const WCHAR win32W[] = {'w','i','n','3','2',0};
    static const WCHAR policyW[] = {'w','i','n','3','2','-','p','o','l','i','c','y',0};
    IXMLDOMNodeList *list = NULL;
    IXMLDOMNode *node = NULL;
    IXMLDOMNamedNodeMap *attrs = NULL;
    struct assembly *a = NULL;
    BSTR str;
    HRESULT hr;
    LONG len;

    str = SysAllocString( identityW );
    hr = IXMLDOMDocument_getElementsByTagName( doc, str, &list );
    SysFreeString( str );
    if (hr != S_OK) goto done;

    hr = IXMLDOMNodeList_get_length( list, &len );
    if (hr != S_OK) goto done;
    if (!len)
    {
        hr = ERROR_SXS_MANIFEST_FORMAT_ERROR;
        goto done;
    }
    hr = IXMLDOMNodeList_nextNode( list, &node );
    if (hr != S_OK) goto done;
    if (!node)
    {
        hr = ERROR_SXS_MANIFEST_FORMAT_ERROR;
        goto done;
    }
    if (!(a = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(struct assembly) )))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    list_init( &a->files );

    hr = IXMLDOMNode_get_attributes( node, &attrs );
    if (hr != S_OK) goto done;

    a->type    = get_attribute_value( attrs, typeW );
    a->name    = get_attribute_value( attrs, nameW );
    a->version = get_attribute_value( attrs, versionW );
    a->arch    = get_attribute_value( attrs, architectureW );
    a->token   = get_attribute_value( attrs, tokenW );

    if (!a->type || (strcmpW( a->type, win32W ) && strcmpW( a->type, policyW )) ||
        !a->name || !a->version || !a->arch || !a->token)
    {
        WARN("invalid win32 assembly\n");
        hr = ERROR_SXS_MANIFEST_FORMAT_ERROR;
        goto done;
    }
    if (!strcmpW( a->type, win32W )) hr = parse_files( doc, a );

done:
    if (attrs) IXMLDOMNamedNodeMap_Release( attrs );
    if (node) IXMLDOMNode_Release( node );
    if (list) IXMLDOMNodeList_Release( list );
    if (hr == S_OK) *assembly = a;
    else free_assembly( a );
    return hr;
}

static WCHAR *build_sxs_path( void )
{
    static const WCHAR winsxsW[] = {'\\','w','i','n','s','x','s','\\',0};
    WCHAR sxsdir[MAX_PATH];

    GetWindowsDirectoryW( sxsdir, MAX_PATH );
    strcatW( sxsdir, winsxsW );
    return strdupW( sxsdir );
}

static WCHAR *build_assembly_name( struct assembly *assembly )
{
    static const WCHAR fmtW[] =
        {'%','s','_','%','s','_','%','s','_','%','s','_','n','o','n','e','_','d','e','a','d','b','e','e','f',0};
    WCHAR *ret, *p;
    int len;

    len = strlenW( fmtW );
    len += strlenW( assembly->arch );
    len += strlenW( assembly->name );
    len += strlenW( assembly->token );
    len += strlenW( assembly->version );

    if (!(ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return NULL;
    sprintfW( ret, fmtW, assembly->arch, assembly->name, assembly->token, assembly->version );
    for (p = ret; *p; p++) *p = tolowerW( *p );
    return ret;
}

static WCHAR *build_policy_name( struct assembly *assembly )
{
    static const WCHAR fmtW[] =
        {'%','s','_','%','s','_','%','s','_','n','o','n','e','_','d','e','a','d','b','e','e','f',0};
    WCHAR *ret, *p;
    int len;

    len = strlenW( fmtW );
    len += strlenW( assembly->arch );
    len += strlenW( assembly->name );
    len += strlenW( assembly->token );

    if (!(ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return NULL;
    sprintfW( ret, fmtW, assembly->arch, assembly->name, assembly->token );
    for (p = ret; *p; p++) *p = tolowerW( *p );
    return ret;
}

static HRESULT install_policy( const WCHAR *manifest, struct assembly *assembly )
{
    static const WCHAR policiesW[] = {'p','o','l','i','c','i','e','s','\\',0};
    static const WCHAR suffixW[] = {'.','p','o','l','i','c','y',0};
    static const WCHAR backslashW[] = {'\\',0};
    WCHAR *sxsdir, *name, *dst;
    HRESULT hr = E_OUTOFMEMORY;
    BOOL ret;
    int len;

    /* FIXME: handle catalog file */

    if (!(sxsdir = build_sxs_path())) return E_OUTOFMEMORY;
    if (!(name = build_policy_name( assembly ))) goto done;

    len = strlenW( sxsdir );
    len += strlenW( policiesW );
    len += strlenW( name ) + 1;
    len += strlenW( assembly->version );
    len += strlenW( suffixW );

    if (!(dst = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) goto done;
    strcpyW( dst, sxsdir );
    strcatW( dst, policiesW );
    CreateDirectoryW( dst, NULL );
    strcatW( dst, name );
    CreateDirectoryW( dst, NULL );
    strcatW( dst, backslashW );
    strcatW( dst, assembly->version );
    strcatW( dst, suffixW );

    ret = CopyFileW( manifest, dst, FALSE );
    HeapFree( GetProcessHeap(), 0, dst );
    if (!ret)
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        WARN("failed to copy policy manifest file 0x%08x\n", hr);
    }
    hr = S_OK;

done:
    HeapFree( GetProcessHeap(), 0, sxsdir );
    HeapFree( GetProcessHeap(), 0, name );
    return hr;
}

static WCHAR *build_source_filename( const WCHAR *manifest, struct file *file )
{
    WCHAR *src;
    const WCHAR *p;
    int len;

    p = strrchrW( manifest, '\\' );
    if (!p) p = strrchrW( manifest, '/' );
    if (!p) return strdupW( manifest );

    len = p - manifest + 1;
    if (!(src = HeapAlloc( GetProcessHeap(), 0, (len + strlenW( file->name ) + 1) * sizeof(WCHAR) )))
        return NULL;

    memcpy( src, manifest, len * sizeof(WCHAR) );
    strcpyW( src + len, file->name );
    return src;
}

static HRESULT install_assembly( const WCHAR *manifest, struct assembly *assembly )
{
    static const WCHAR manifestsW[] = {'m','a','n','i','f','e','s','t','s','\\',0};
    static const WCHAR suffixW[] = {'.','m','a','n','i','f','e','s','t',0};
    static const WCHAR backslashW[] = {'\\',0};
    WCHAR *sxsdir, *p, *name, *dst, *src;
    struct file *file;
    HRESULT hr = E_OUTOFMEMORY;
    BOOL ret;
    int len;

    if (!(sxsdir = build_sxs_path())) return E_OUTOFMEMORY;
    if (!(name = build_assembly_name( assembly ))) goto done;

    len = strlenW( sxsdir );
    len += strlenW( manifestsW );
    len += strlenW( name );
    len += strlenW( suffixW );
    if (!(dst = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) goto done;
    strcpyW( dst, sxsdir );
    strcatW( dst, manifestsW );
    strcatW( dst, name );
    strcatW( dst, suffixW );

    ret = CopyFileW( manifest, dst, FALSE );
    HeapFree( GetProcessHeap(), 0, dst );
    if (!ret)
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        WARN("failed to copy manifest file 0x%08x\n", hr);
        goto done;
    }

    /* FIXME: this should be a transaction */
    LIST_FOR_EACH_ENTRY( file, &assembly->files, struct file, entry )
    {
        if (!(src = build_source_filename( manifest, file )))
        {
            hr = E_OUTOFMEMORY;
            goto done;
        }
        len = strlenW( sxsdir ) + strlenW( name ) + strlenW( file->name );
        if (!(dst = HeapAlloc( GetProcessHeap(), 0, (len + 2) * sizeof(WCHAR) )))
        {
            HeapFree( GetProcessHeap(), 0, src );
            hr = E_OUTOFMEMORY;
            goto done;
        }
        strcpyW( dst, sxsdir );
        strcatW( dst, name );
        CreateDirectoryW( dst, NULL );

        strcatW( dst, backslashW );
        strcatW( dst, file->name );
        for (p = dst; *p; p++) *p = tolowerW( *p );

        ret = CopyFileW( src, dst, FALSE );
        HeapFree( GetProcessHeap(), 0, src );
        HeapFree( GetProcessHeap(), 0, dst );
        if (!ret)
        {
            hr = HRESULT_FROM_WIN32( GetLastError() );
            WARN("failed to copy file 0x%08x\n", hr);
            goto done;
        }
    }
    hr = S_OK;

done:
    HeapFree( GetProcessHeap(), 0, sxsdir );
    HeapFree( GetProcessHeap(), 0, name );
    return hr;
}

static HRESULT WINAPI cache_InstallAssembly(
    IAssemblyCache *iface,
    DWORD flags,
    LPCWSTR path,
    LPCFUSION_INSTALL_REFERENCE ref )
{
    static const WCHAR policyW[] = {'w','i','n','3','2','-','p','o','l','i','c','y',0};
    HRESULT hr, init;
    IXMLDOMDocument *doc = NULL;
    struct assembly *assembly = NULL;
    BSTR str;
    VARIANT var;
    VARIANT_BOOL b;

    TRACE("%p, 0x%08x, %s, %p\n", iface, flags, debugstr_w(path), ref);

    init = CoInitialize( NULL );

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void **)&doc );
    if (hr != S_OK)
        goto done;

    str = SysAllocString( path );
    VariantInit( &var );
    V_VT( &var ) = VT_BSTR;
    V_BSTR( &var ) = str;
    hr = IXMLDOMDocument_load( doc, var, &b );
    SysFreeString( str );
    if (hr != S_OK) goto done;
    if (!b)
    {
        WARN("failed to load manifest\n");
        hr = S_FALSE;
        goto done;
    }

    hr = parse_assembly( doc, &assembly );
    if (hr != S_OK)
        goto done;

    /* FIXME: verify name attributes */

    if (!strcmpW( assembly->type, policyW ))
        hr = install_policy( path, assembly );
    else
        hr = install_assembly( path, assembly );

done:
    free_assembly( assembly );
    if (doc) IXMLDOMDocument_Release( doc );

    if (SUCCEEDED(init))
        CoUninitialize();

    return hr;
}

static const IAssemblyCacheVtbl cache_vtbl =
{
    cache_QueryInterface,
    cache_AddRef,
    cache_Release,
    cache_UninstallAssembly,
    cache_QueryAssemblyInfo,
    cache_CreateAssemblyCacheItem,
    cache_Reserved,
    cache_InstallAssembly
};

/******************************************************************
 *  CreateAssemblyCache   (SXS.@)
 */
HRESULT WINAPI CreateAssemblyCache( IAssemblyCache **obj, DWORD reserved )
{
    struct cache *cache;

    TRACE("%p, %u\n", obj, reserved);

    if (!obj)
        return E_INVALIDARG;

    *obj = NULL;

    cache = HeapAlloc( GetProcessHeap(), 0, sizeof(struct cache) );
    if (!cache)
        return E_OUTOFMEMORY;

    cache->IAssemblyCache_iface.lpVtbl = &cache_vtbl;
    cache->refs = 1;

    *obj = &cache->IAssemblyCache_iface;
    return S_OK;
}
