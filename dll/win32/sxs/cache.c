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
#ifdef __REACTOS__
#include <wchar.h>
#endif

#define COBJMACROS
#define INITGUID

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "winsxs.h"
#include "msxml2.h"

#include "wine/debug.h"
#include "wine/list.h"
#include "sxs_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(sxs);

static const WCHAR cache_mutex_nameW[] =
    {'_','_','W','I','N','E','_','S','X','S','_','C','A','C','H','E','_','M','U','T','E','X','_','_',0};

static const WCHAR win32W[] = {'w','i','n','3','2',0};
static const WCHAR win32_policyW[] = {'w','i','n','3','2','-','p','o','l','i','c','y',0};
static const WCHAR backslashW[] = {'\\',0};

struct cache
{
    IAssemblyCache IAssemblyCache_iface;
    LONG refs;
    HANDLE lock;
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
        IAssemblyCache_AddRef( iface );
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
        CloseHandle( cache->lock );
        HeapFree( GetProcessHeap(), 0, cache );
    }
    return refs;
}

static unsigned int build_sxs_path( WCHAR *path )
{
    static const WCHAR winsxsW[] = {'\\','w','i','n','s','x','s','\\',0};
    unsigned int len = GetWindowsDirectoryW( path, MAX_PATH );

    memcpy( path + len, winsxsW, sizeof(winsxsW) );
    return len + ARRAY_SIZE(winsxsW) - 1;
}

static WCHAR *build_assembly_name( const WCHAR *arch, const WCHAR *name, const WCHAR *token,
                                   const WCHAR *version, unsigned int *len )
{
    static const WCHAR fmtW[] =
        {'%','s','_','%','s','_','%','s','_','%','s','_','n','o','n','e','_','d','e','a','d','b','e','e','f',0};
    unsigned int buflen = ARRAY_SIZE(fmtW);
    WCHAR *ret;

    buflen += lstrlenW( arch );
    buflen += lstrlenW( name );
    buflen += lstrlenW( token );
    buflen += lstrlenW( version );
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, buflen * sizeof(WCHAR) ))) return NULL;
    *len = swprintf( ret, fmtW, arch, name, token, version );
    return _wcslwr( ret );
}

static WCHAR *build_manifest_path( const WCHAR *arch, const WCHAR *name, const WCHAR *token,
                                   const WCHAR *version )
{
    static const WCHAR fmtW[] =
        {'%','s','m','a','n','i','f','e','s','t','s','\\','%','s','.','m','a','n','i','f','e','s','t',0};
    WCHAR *path = NULL, *ret, sxsdir[MAX_PATH];
    unsigned int len;

    if (!(path = build_assembly_name( arch, name, token, version, &len ))) return NULL;
    len += ARRAY_SIZE(fmtW);
    len += build_sxs_path( sxsdir );
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        HeapFree( GetProcessHeap(), 0, path );
        return NULL;
    }
    swprintf( ret, fmtW, sxsdir, path );
    HeapFree( GetProcessHeap(), 0, path );
    return ret;
}

static WCHAR *build_policy_name( const WCHAR *arch, const WCHAR *name, const WCHAR *token,
                                 unsigned int *len )
{
    static const WCHAR fmtW[] =
        {'%','s','_','%','s','_','%','s','_','n','o','n','e','_','d','e','a','d','b','e','e','f',0};
    unsigned int buflen = ARRAY_SIZE(fmtW);
    WCHAR *ret;

    buflen += lstrlenW( arch );
    buflen += lstrlenW( name );
    buflen += lstrlenW( token );
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, buflen * sizeof(WCHAR) ))) return NULL;
    *len = swprintf( ret, fmtW, arch, name, token );
    return _wcslwr( ret );
}

static WCHAR *build_policy_path( const WCHAR *arch, const WCHAR *name, const WCHAR *token,
                                 const WCHAR *version )
{
    static const WCHAR fmtW[] =
        {'%','s','p','o','l','i','c','i','e','s','\\','%','s','\\','%','s','.','p','o','l','i','c','y',0};
    WCHAR *path = NULL, *ret, sxsdir[MAX_PATH];
    unsigned int len;

    if (!(path = build_policy_name( arch, name, token, &len ))) return NULL;
    len += ARRAY_SIZE(fmtW);
    len += build_sxs_path( sxsdir );
    len += lstrlenW( version );
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        HeapFree( GetProcessHeap(), 0, path );
        return NULL;
    }
    swprintf( ret, fmtW, sxsdir, path, version );
    HeapFree( GetProcessHeap(), 0, path );
    return ret;
}

static void cache_lock( struct cache *cache )
{
    WaitForSingleObject( cache->lock, INFINITE );
}

static void cache_unlock( struct cache *cache )
{
    ReleaseMutex( cache->lock );
}

#define ASSEMBLYINFO_FLAG_INSTALLED 1

static HRESULT WINAPI cache_QueryAssemblyInfo(
    IAssemblyCache *iface,
    DWORD flags,
    LPCWSTR assembly_name,
    ASSEMBLY_INFO *info )
{
    struct cache *cache = impl_from_IAssemblyCache( iface );
    IAssemblyName *name_obj;
    const WCHAR *arch, *name, *token, *type, *version;
    WCHAR *p, *path = NULL;
    unsigned int len;
    HRESULT hr;

    TRACE("%p, 0x%08x, %s, %p\n", iface, flags, debugstr_w(assembly_name), info);

    if (flags || (info && info->cbAssemblyInfo != sizeof(*info)))
        return E_INVALIDARG;

    hr = CreateAssemblyNameObject( &name_obj, assembly_name, CANOF_PARSE_DISPLAY_NAME, 0 );
    if (FAILED( hr ))
        return hr;

    arch = get_name_attribute( name_obj, NAME_ATTR_ID_ARCH );
    name = get_name_attribute( name_obj, NAME_ATTR_ID_NAME );
    token = get_name_attribute( name_obj, NAME_ATTR_ID_TOKEN );
    type = get_name_attribute( name_obj, NAME_ATTR_ID_TYPE );
    version = get_name_attribute( name_obj, NAME_ATTR_ID_VERSION );
    if (!arch || !name || !token || !type || !version)
    {
        IAssemblyName_Release( name_obj );
        return HRESULT_FROM_WIN32( ERROR_SXS_MISSING_ASSEMBLY_IDENTITY_ATTRIBUTE );
    }
    if (!info)
    {
        IAssemblyName_Release( name_obj );
        return S_OK;
    }
    cache_lock( cache );

    if (!wcscmp( type, win32W )) path = build_manifest_path( arch, name, token, version );
    else if (!wcscmp( type, win32_policyW )) path = build_policy_path( arch, name, token, version );
    else
    {
        hr = HRESULT_FROM_WIN32( ERROR_SXS_INVALID_IDENTITY_ATTRIBUTE_VALUE );
        goto done;
    }
    if (!path)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    hr = S_OK;
    if (GetFileAttributesW( path ) != INVALID_FILE_ATTRIBUTES) /* FIXME: better check */
    {
        info->dwAssemblyFlags = ASSEMBLYINFO_FLAG_INSTALLED;
        TRACE("assembly is installed\n");
    }
    if ((p = wcsrchr( path, '\\' ))) *p = 0;
    len = lstrlenW( path ) + 1;
    if (info->pszCurrentAssemblyPathBuf)
    {
        if (info->cchBuf < len)
        {
            info->cchBuf = len;
            hr = HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER );
        }
        else lstrcpyW( info->pszCurrentAssemblyPathBuf, path );
    }

done:
    HeapFree( GetProcessHeap(), 0, path );
    IAssemblyName_Release( name_obj );
    cache_unlock( cache );
    return hr;
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

    if (!a->type || (wcscmp( a->type, win32W ) && wcscmp( a->type, win32_policyW )) ||
        !a->name || !a->version || !a->arch || !a->token)
    {
        WARN("invalid win32 assembly\n");
        hr = ERROR_SXS_MANIFEST_FORMAT_ERROR;
        goto done;
    }
    if (!wcscmp( a->type, win32W )) hr = parse_files( doc, a );

done:
    if (attrs) IXMLDOMNamedNodeMap_Release( attrs );
    if (node) IXMLDOMNode_Release( node );
    if (list) IXMLDOMNodeList_Release( list );
    if (hr == S_OK) *assembly = a;
    else free_assembly( a );
    return hr;
}

static WCHAR *build_policy_filename( const WCHAR *arch, const WCHAR *name, const WCHAR *token,
                                     const WCHAR *version )
{
    static const WCHAR policiesW[] = {'p','o','l','i','c','i','e','s','\\',0};
    static const WCHAR suffixW[] = {'.','p','o','l','i','c','y',0};
    WCHAR sxsdir[MAX_PATH], *ret, *fullname;
    unsigned int len;

    if (!(fullname = build_policy_name( arch, name, token, &len ))) return NULL;
    len += build_sxs_path( sxsdir );
    len += ARRAY_SIZE(policiesW) - 1;
    len += lstrlenW( version );
    len += ARRAY_SIZE(suffixW) - 1;
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
    {
        HeapFree( GetProcessHeap(), 0, fullname );
        return NULL;
    }
    lstrcpyW( ret, sxsdir );
    lstrcatW( ret, policiesW );
    CreateDirectoryW( ret, NULL );
    lstrcatW( ret, name );
    CreateDirectoryW( ret, NULL );
    lstrcatW( ret, backslashW );
    lstrcatW( ret, version );
    lstrcatW( ret, suffixW );

    HeapFree( GetProcessHeap(), 0, fullname );
    return ret;
}

static HRESULT install_policy( const WCHAR *manifest, struct assembly *assembly )
{
    WCHAR *dst;
    BOOL ret;

    /* FIXME: handle catalog file */

    dst = build_policy_filename( assembly->arch, assembly->name, assembly->token, assembly->version );
    if (!dst) return E_OUTOFMEMORY;

    ret = CopyFileW( manifest, dst, FALSE );
    HeapFree( GetProcessHeap(), 0, dst );
    if (!ret)
    {
        HRESULT hr = HRESULT_FROM_WIN32( GetLastError() );
        WARN("failed to copy policy manifest file 0x%08x\n", hr);
        return hr;
    }
    return S_OK;
}

static WCHAR *build_source_filename( const WCHAR *manifest, struct file *file )
{
    WCHAR *src;
    const WCHAR *p;
    int len;

    p = wcsrchr( manifest, '\\' );
    if (!p) p = wcsrchr( manifest, '/' );
    if (!p) return strdupW( manifest );

    len = p - manifest + 1;
    if (!(src = HeapAlloc( GetProcessHeap(), 0, (len + lstrlenW( file->name ) + 1) * sizeof(WCHAR) )))
        return NULL;

    memcpy( src, manifest, len * sizeof(WCHAR) );
    lstrcpyW( src + len, file->name );
    return src;
}

static WCHAR *build_manifest_filename( const WCHAR *arch, const WCHAR *name, const WCHAR *token,
                                       const WCHAR *version )
{
    static const WCHAR manifestsW[] = {'m','a','n','i','f','e','s','t','s','\\',0};
    static const WCHAR suffixW[] = {'.','m','a','n','i','f','e','s','t',0};
    WCHAR sxsdir[MAX_PATH], *ret, *fullname;
    unsigned int len;

    if (!(fullname = build_assembly_name( arch, name, token, version, &len ))) return NULL;
    len += build_sxs_path( sxsdir );
    len += ARRAY_SIZE(manifestsW) - 1;
    len += ARRAY_SIZE(suffixW) - 1;
    if (!(ret = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) )))
    {
        HeapFree( GetProcessHeap(), 0, fullname );
        return NULL;
    }
    lstrcpyW( ret, sxsdir );
    lstrcatW( ret, manifestsW );
    lstrcatW( ret, fullname );
    lstrcatW( ret, suffixW );

    HeapFree( GetProcessHeap(), 0, fullname );
    return ret;
}

static HRESULT load_manifest( IXMLDOMDocument *doc, const WCHAR *filename )
{
    HRESULT hr;
    VARIANT var;
    VARIANT_BOOL b;
    BSTR str;

    str = SysAllocString( filename );
    VariantInit( &var );
    V_VT( &var ) = VT_BSTR;
    V_BSTR( &var ) = str;
    hr = IXMLDOMDocument_load( doc, var, &b );
    SysFreeString( str );
    if (hr != S_OK) return hr;
    if (!b)
    {
        WARN("failed to load manifest\n");
        return S_FALSE;
    }
    return S_OK;
}

static HRESULT install_assembly( const WCHAR *manifest, struct assembly *assembly )
{
    WCHAR sxsdir[MAX_PATH], *p, *name, *dst, *src;
    unsigned int len, len_name, len_sxsdir = build_sxs_path( sxsdir );
    struct file *file;
    HRESULT hr = E_OUTOFMEMORY;
    BOOL ret;

    dst = build_manifest_filename( assembly->arch, assembly->name, assembly->token, assembly->version );
    if (!dst) return E_OUTOFMEMORY;

    ret = CopyFileW( manifest, dst, FALSE );
    HeapFree( GetProcessHeap(), 0, dst );
    if (!ret)
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        WARN("failed to copy manifest file 0x%08x\n", hr);
        return hr;
    }

    name = build_assembly_name( assembly->arch, assembly->name, assembly->token, assembly->version,
                                &len_name );
    if (!name) return E_OUTOFMEMORY;

    /* FIXME: this should be a transaction */
    LIST_FOR_EACH_ENTRY( file, &assembly->files, struct file, entry )
    {
        if (!(src = build_source_filename( manifest, file ))) goto done;

        len = len_sxsdir + len_name + lstrlenW( file->name );
        if (!(dst = HeapAlloc( GetProcessHeap(), 0, (len + 2) * sizeof(WCHAR) )))
        {
            HeapFree( GetProcessHeap(), 0, src );
            goto done;
        }
        lstrcpyW( dst, sxsdir );
        lstrcatW( dst, name );
        CreateDirectoryW( dst, NULL );

        lstrcatW( dst, backslashW );
        lstrcatW( dst, file->name );
        for (p = dst; *p; p++) *p = towlower( *p );

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
    HeapFree( GetProcessHeap(), 0, name );
    return hr;
}

static HRESULT WINAPI cache_InstallAssembly(
    IAssemblyCache *iface,
    DWORD flags,
    LPCWSTR path,
    LPCFUSION_INSTALL_REFERENCE ref )
{
    struct cache *cache = impl_from_IAssemblyCache( iface );
    HRESULT hr, init;
    IXMLDOMDocument *doc = NULL;
    struct assembly *assembly = NULL;

    TRACE("%p, 0x%08x, %s, %p\n", iface, flags, debugstr_w(path), ref);

    cache_lock( cache );
    init = CoInitialize( NULL );

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void **)&doc );
    if (hr != S_OK)
        goto done;

    if ((hr = load_manifest( doc, path )) != S_OK) goto done;
    if ((hr = parse_assembly( doc, &assembly )) != S_OK) goto done;

    /* FIXME: verify name attributes */

    if (!wcscmp( assembly->type, win32_policyW ))
        hr = install_policy( path, assembly );
    else
        hr = install_assembly( path, assembly );

done:
    free_assembly( assembly );
    if (doc) IXMLDOMDocument_Release( doc );
    if (SUCCEEDED(init)) CoUninitialize();
    cache_unlock( cache );
    return hr;
}

static HRESULT uninstall_assembly( struct assembly *assembly )
{
    WCHAR sxsdir[MAX_PATH], *name, *dirname, *filename;
    unsigned int len, len_name, len_sxsdir = build_sxs_path( sxsdir );
    HRESULT hr = E_OUTOFMEMORY;
    struct file *file;

    name = build_assembly_name( assembly->arch, assembly->name, assembly->token, assembly->version,
                                &len_name );
    if (!name) return E_OUTOFMEMORY;
    if (!(dirname = HeapAlloc( GetProcessHeap(), 0, (len_sxsdir + len_name + 1) * sizeof(WCHAR) )))
        goto done;
    lstrcpyW( dirname, sxsdir );
    lstrcpyW( dirname + len_sxsdir, name );

    LIST_FOR_EACH_ENTRY( file, &assembly->files, struct file, entry )
    {
        len = len_sxsdir + len_name + 1 + lstrlenW( file->name );
        if (!(filename = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) goto done;
        lstrcpyW( filename, dirname );
        lstrcatW( filename, backslashW );
        lstrcatW( filename, file->name );

        if (!DeleteFileW( filename )) WARN( "failed to delete file %u\n", GetLastError() );
        HeapFree( GetProcessHeap(), 0, filename );
    }
    RemoveDirectoryW( dirname );
    hr = S_OK;

done:
    HeapFree( GetProcessHeap(), 0, dirname );
    HeapFree( GetProcessHeap(), 0, name );
    return hr;
}

static HRESULT WINAPI cache_UninstallAssembly(
    IAssemblyCache *iface,
    DWORD flags,
    LPCWSTR assembly_name,
    LPCFUSION_INSTALL_REFERENCE ref,
    ULONG *disp )
{
    struct cache *cache = impl_from_IAssemblyCache( iface );
    HRESULT hr, init;
    IXMLDOMDocument *doc = NULL;
    struct assembly *assembly = NULL;
    IAssemblyName *name_obj = NULL;
    const WCHAR *arch, *name, *token, *type, *version;
    WCHAR *p, *path = NULL;

    TRACE("%p, 0x%08x, %s, %p, %p\n", iface, flags, debugstr_w(assembly_name), ref, disp);

    if (ref)
    {
        FIXME("application reference not supported\n");
        return E_NOTIMPL;
    }
    cache_lock( cache );
    init = CoInitialize( NULL );

    hr = CreateAssemblyNameObject( &name_obj, assembly_name, CANOF_PARSE_DISPLAY_NAME, NULL );
    if (FAILED( hr ))
        goto done;

    arch = get_name_attribute( name_obj, NAME_ATTR_ID_ARCH );
    name = get_name_attribute( name_obj, NAME_ATTR_ID_NAME );
    token = get_name_attribute( name_obj, NAME_ATTR_ID_TOKEN );
    type = get_name_attribute( name_obj, NAME_ATTR_ID_TYPE );
    version = get_name_attribute( name_obj, NAME_ATTR_ID_VERSION );
    if (!arch || !name || !token || !type || !version)
    {
        hr = E_INVALIDARG;
        goto done;
    }
    if (!wcscmp( type, win32W )) path = build_manifest_filename( arch, name, token, version );
    else if (!wcscmp( type, win32_policyW )) path = build_policy_filename( arch, name, token, version );
    else
    {
        hr = E_INVALIDARG;
        goto done;
    }

    hr = CoCreateInstance( &CLSID_DOMDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IXMLDOMDocument, (void **)&doc );
    if (hr != S_OK)
        goto done;

    if ((hr = load_manifest( doc, path )) != S_OK) goto done;
    if ((hr = parse_assembly( doc, &assembly )) != S_OK) goto done;

    if (!DeleteFileW( path )) WARN( "unable to remove manifest file %u\n", GetLastError() );
    else if ((p = wcsrchr( path, '\\' )))
    {
        *p = 0;
        RemoveDirectoryW( path );
    }
    if (!wcscmp( assembly->type, win32W )) hr = uninstall_assembly( assembly );

done:
    if (name_obj) IAssemblyName_Release( name_obj );
    HeapFree( GetProcessHeap(), 0, path );
    free_assembly( assembly );
    if (doc) IXMLDOMDocument_Release( doc );
    if (SUCCEEDED(init)) CoUninitialize();
    cache_unlock( cache );
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
    cache->lock = CreateMutexW( NULL, FALSE, cache_mutex_nameW );
    if (!cache->lock)
    {
        HeapFree( GetProcessHeap(), 0, cache );
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    *obj = &cache->IAssemblyCache_iface;
    return S_OK;
}
