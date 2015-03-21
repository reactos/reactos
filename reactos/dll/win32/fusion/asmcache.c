/*
 * IAssemblyCache implementation
 *
 * Copyright 2008 James Hawkins
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

#include "fusionpriv.h"

typedef struct {
    IAssemblyCache IAssemblyCache_iface;

    LONG ref;
    HANDLE lock;
} IAssemblyCacheImpl;

typedef struct {
    IAssemblyCacheItem IAssemblyCacheItem_iface;

    LONG ref;
} IAssemblyCacheItemImpl;

static const WCHAR cache_mutex_nameW[] =
    {'_','_','W','I','N','E','_','F','U','S','I','O','N','_','C','A','C','H','E','_','M','U','T','E','X','_','_',0};

static BOOL create_full_path(LPCWSTR path)
{
    LPWSTR new_path;
    BOOL ret = TRUE;
    int len;

    new_path = HeapAlloc(GetProcessHeap(), 0, (strlenW(path) + 1) * sizeof(WCHAR));
    if (!new_path)
        return FALSE;

    strcpyW(new_path, path);

    while ((len = strlenW(new_path)) && new_path[len - 1] == '\\')
        new_path[len - 1] = 0;

    while (!CreateDirectoryW(new_path, NULL))
    {
        LPWSTR slash;
        DWORD last_error = GetLastError();

        if(last_error == ERROR_ALREADY_EXISTS)
            break;

        if(last_error != ERROR_PATH_NOT_FOUND)
        {
            ret = FALSE;
            break;
        }

        if(!(slash = strrchrW(new_path, '\\')))
        {
            ret = FALSE;
            break;
        }

        len = slash - new_path;
        new_path[len] = 0;
        if(!create_full_path(new_path))
        {
            ret = FALSE;
            break;
        }

        new_path[len] = '\\';
    }

    HeapFree(GetProcessHeap(), 0, new_path);
    return ret;
}

static BOOL get_assembly_directory(LPWSTR dir, DWORD size, const char *version, PEKIND architecture)
{
    static const WCHAR dotnet[] = {'\\','M','i','c','r','o','s','o','f','t','.','N','E','T','\\',0};
    static const WCHAR gac[] = {'\\','a','s','s','e','m','b','l','y','\\','G','A','C',0};
    static const WCHAR msil[] = {'_','M','S','I','L',0};
    static const WCHAR x86[] = {'_','3','2',0};
    static const WCHAR amd64[] = {'_','6','4',0};
    DWORD len = GetWindowsDirectoryW(dir, size);

    if (!strcmp(version, "v4.0.30319"))
    {
        strcpyW(dir + len, dotnet);
        len += sizeof(dotnet)/sizeof(WCHAR) -1;
        strcpyW(dir + len, gac + 1);
        len += sizeof(gac)/sizeof(WCHAR) - 2;
    }
    else
    {
        strcpyW(dir + len, gac);
        len += sizeof(gac)/sizeof(WCHAR) - 1;
    }
    switch (architecture)
    {
        case peNone:
            break;

        case peMSIL:
            strcpyW(dir + len, msil);
            break;

        case peI386:
            strcpyW(dir + len, x86);
            break;

        case peAMD64:
            strcpyW(dir + len, amd64);
            break;

        default:
            WARN("unhandled architecture %u\n", architecture);
            return FALSE;
    }
    return TRUE;
}

/* IAssemblyCache */

static inline IAssemblyCacheImpl *impl_from_IAssemblyCache(IAssemblyCache *iface)
{
    return CONTAINING_RECORD(iface, IAssemblyCacheImpl, IAssemblyCache_iface);
}

static HRESULT WINAPI IAssemblyCacheImpl_QueryInterface(IAssemblyCache *iface,
                                                        REFIID riid, LPVOID *ppobj)
{
    IAssemblyCacheImpl *This = impl_from_IAssemblyCache(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyCache))
    {
        IAssemblyCache_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyCacheImpl_AddRef(IAssemblyCache *iface)
{
    IAssemblyCacheImpl *This = impl_from_IAssemblyCache(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyCacheImpl_Release(IAssemblyCache *iface)
{
    IAssemblyCacheImpl *cache = impl_from_IAssemblyCache(iface);
    ULONG refCount = InterlockedDecrement( &cache->ref );

    TRACE("(%p)->(ref before = %u)\n", cache, refCount + 1);

    if (!refCount)
    {
        CloseHandle( cache->lock );
        HeapFree( GetProcessHeap(), 0, cache );
    }
    return refCount;
}

static void cache_lock( IAssemblyCacheImpl *cache )
{
    WaitForSingleObject( cache->lock, INFINITE );
}

static void cache_unlock( IAssemblyCacheImpl *cache )
{
    ReleaseMutex( cache->lock );
}

static HRESULT WINAPI IAssemblyCacheImpl_UninstallAssembly(IAssemblyCache *iface,
                                                           DWORD dwFlags,
                                                           LPCWSTR pszAssemblyName,
                                                           LPCFUSION_INSTALL_REFERENCE pRefData,
                                                           ULONG *pulDisposition)
{
    HRESULT hr;
    IAssemblyCacheImpl *cache = impl_from_IAssemblyCache(iface);
    IAssemblyName *asmname, *next = NULL;
    IAssemblyEnum *asmenum = NULL;
    WCHAR *p, *path = NULL;
    ULONG disp;
    DWORD len;

    TRACE("(%p, 0%08x, %s, %p, %p)\n", iface, dwFlags,
          debugstr_w(pszAssemblyName), pRefData, pulDisposition);

    if (pRefData)
    {
        FIXME("application reference not supported\n");
        return E_NOTIMPL;
    }
    hr = CreateAssemblyNameObject( &asmname, pszAssemblyName, CANOF_PARSE_DISPLAY_NAME, NULL );
    if (FAILED( hr ))
        return hr;

    cache_lock( cache );

    hr = CreateAssemblyEnum( &asmenum, NULL, asmname, ASM_CACHE_GAC, NULL );
    if (FAILED( hr ))
        goto done;

    hr = IAssemblyEnum_GetNextAssembly( asmenum, NULL, &next, 0 );
    if (hr == S_FALSE)
    {
        if (pulDisposition)
            *pulDisposition = IASSEMBLYCACHE_UNINSTALL_DISPOSITION_ALREADY_UNINSTALLED;
        goto done;
    }
    hr = IAssemblyName_GetPath( next, NULL, &len );
    if (hr != HRESULT_FROM_WIN32( ERROR_INSUFFICIENT_BUFFER ))
        goto done;

    if (!(path = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    hr = IAssemblyName_GetPath( next, path, &len );
    if (FAILED( hr ))
        goto done;

    if (DeleteFileW( path ))
    {
        if ((p = strrchrW( path, '\\' )))
        {
            *p = 0;
            RemoveDirectoryW( path );
            if ((p = strrchrW( path, '\\' )))
            {
                *p = 0;
                RemoveDirectoryW( path );
            }
        }
        disp = IASSEMBLYCACHE_UNINSTALL_DISPOSITION_UNINSTALLED;
        hr = S_OK;
    }
    else
    {
        disp = IASSEMBLYCACHE_UNINSTALL_DISPOSITION_ALREADY_UNINSTALLED;
        hr = S_FALSE;
    }
    if (pulDisposition) *pulDisposition = disp;

done:
    IAssemblyName_Release( asmname );
    if (next) IAssemblyName_Release( next );
    if (asmenum) IAssemblyEnum_Release( asmenum );
    HeapFree( GetProcessHeap(), 0, path );
    cache_unlock( cache );
    return hr;
}

static HRESULT WINAPI IAssemblyCacheImpl_QueryAssemblyInfo(IAssemblyCache *iface,
                                                           DWORD dwFlags,
                                                           LPCWSTR pszAssemblyName,
                                                           ASSEMBLY_INFO *pAsmInfo)
{
    IAssemblyCacheImpl *cache = impl_from_IAssemblyCache(iface);
    IAssemblyName *asmname, *next = NULL;
    IAssemblyEnum *asmenum = NULL;
    HRESULT hr;

    TRACE("(%p, %d, %s, %p)\n", iface, dwFlags,
          debugstr_w(pszAssemblyName), pAsmInfo);

    if (pAsmInfo)
    {
        if (pAsmInfo->cbAssemblyInfo == 0)
            pAsmInfo->cbAssemblyInfo = sizeof(ASSEMBLY_INFO);
        else if (pAsmInfo->cbAssemblyInfo != sizeof(ASSEMBLY_INFO))
            return E_INVALIDARG;
    }

    hr = CreateAssemblyNameObject(&asmname, pszAssemblyName,
                                  CANOF_PARSE_DISPLAY_NAME, NULL);
    if (FAILED(hr))
        return hr;

    cache_lock( cache );

    hr = CreateAssemblyEnum(&asmenum, NULL, asmname, ASM_CACHE_GAC, NULL);
    if (FAILED(hr))
        goto done;

    for (;;)
    {
        hr = IAssemblyEnum_GetNextAssembly(asmenum, NULL, &next, 0);
        if (hr != S_OK)
        {
            hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            goto done;
        }
        hr = IAssemblyName_IsEqual(asmname, next, ASM_CMPF_IL_ALL);
        if (hr == S_OK) break;
    }

    if (!pAsmInfo)
        goto done;

    hr = IAssemblyName_GetPath(next, pAsmInfo->pszCurrentAssemblyPathBuf, &pAsmInfo->cchBuf);

    pAsmInfo->dwAssemblyFlags = ASSEMBLYINFO_FLAG_INSTALLED;

done:
    IAssemblyName_Release(asmname);
    if (next) IAssemblyName_Release(next);
    if (asmenum) IAssemblyEnum_Release(asmenum);
    cache_unlock( cache );
    return hr;
}

static const IAssemblyCacheItemVtbl AssemblyCacheItemVtbl;

static HRESULT WINAPI IAssemblyCacheImpl_CreateAssemblyCacheItem(IAssemblyCache *iface,
                                                                 DWORD dwFlags,
                                                                 PVOID pvReserved,
                                                                 IAssemblyCacheItem **ppAsmItem,
                                                                 LPCWSTR pszAssemblyName)
{
    IAssemblyCacheItemImpl *item;

    FIXME("(%p, %d, %p, %p, %s) semi-stub!\n", iface, dwFlags, pvReserved,
          ppAsmItem, debugstr_w(pszAssemblyName));

    if (!ppAsmItem)
        return E_INVALIDARG;

    *ppAsmItem = NULL;

    item = HeapAlloc(GetProcessHeap(), 0, sizeof(IAssemblyCacheItemImpl));
    if (!item)
        return E_OUTOFMEMORY;

    item->IAssemblyCacheItem_iface.lpVtbl = &AssemblyCacheItemVtbl;
    item->ref = 1;

    *ppAsmItem = &item->IAssemblyCacheItem_iface;
    return S_OK;
}

static HRESULT WINAPI IAssemblyCacheImpl_CreateAssemblyScavenger(IAssemblyCache *iface,
                                                                 IUnknown **ppUnkReserved)
{
    FIXME("(%p, %p) stub!\n", iface, ppUnkReserved);
    return E_NOTIMPL;
}

static HRESULT copy_file( const WCHAR *src_dir, DWORD src_len, const WCHAR *dst_dir, DWORD dst_len,
                          const WCHAR *filename )
{
    WCHAR *src_file, *dst_file;
    DWORD len = strlenW( filename );
    HRESULT hr = S_OK;

    if (!(src_file = HeapAlloc( GetProcessHeap(), 0, (src_len + len + 1) * sizeof(WCHAR) )))
        return E_OUTOFMEMORY;
    memcpy( src_file, src_dir, src_len * sizeof(WCHAR) );
    strcpyW( src_file + src_len, filename );

    if (!(dst_file = HeapAlloc( GetProcessHeap(), 0, (dst_len + len + 1) * sizeof(WCHAR) )))
    {
        HeapFree( GetProcessHeap(), 0, src_file );
        return E_OUTOFMEMORY;
    }
    memcpy( dst_file, dst_dir, dst_len * sizeof(WCHAR) );
    strcpyW( dst_file + dst_len, filename );

    if (!CopyFileW( src_file, dst_file, FALSE )) hr = HRESULT_FROM_WIN32( GetLastError() );
    HeapFree( GetProcessHeap(), 0, src_file );
    HeapFree( GetProcessHeap(), 0, dst_file );
    return hr;
}

static HRESULT WINAPI IAssemblyCacheImpl_InstallAssembly(IAssemblyCache *iface,
                                                         DWORD dwFlags,
                                                         LPCWSTR pszManifestFilePath,
                                                         LPCFUSION_INSTALL_REFERENCE pRefData)
{
    static const WCHAR format[] =
        {'%','s','\\','%','s','\\','%','s','_','_','%','s','\\',0};
    static const WCHAR format_v40[] =
        {'%','s','\\','%','s','\\','v','4','.','0','_','%','s','_','_','%','s','\\',0};
    static const WCHAR ext_exe[] = {'.','e','x','e',0};
    static const WCHAR ext_dll[] = {'.','d','l','l',0};
    IAssemblyCacheImpl *cache = impl_from_IAssemblyCache(iface);
    ASSEMBLY *assembly;
    const WCHAR *extension, *filename, *src_dir;
    WCHAR *name = NULL, *token = NULL, *version = NULL, *asmpath = NULL;
    WCHAR asmdir[MAX_PATH], *p, **external_files = NULL, *dst_dir = NULL;
    PEKIND architecture;
    char *clr_version;
    DWORD i, count = 0, src_len, dst_len = sizeof(format_v40)/sizeof(format_v40[0]);
    HRESULT hr;

    TRACE("(%p, %d, %s, %p)\n", iface, dwFlags,
          debugstr_w(pszManifestFilePath), pRefData);

    if (!pszManifestFilePath || !*pszManifestFilePath)
        return E_INVALIDARG;

    if (!(extension = strrchrW(pszManifestFilePath, '.')))
        return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);

    if (lstrcmpiW(extension, ext_exe) && lstrcmpiW(extension, ext_dll))
        return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);

    if (GetFileAttributesW(pszManifestFilePath) == INVALID_FILE_ATTRIBUTES)
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    hr = assembly_create(&assembly, pszManifestFilePath);
    if (FAILED(hr))
    {
        hr = COR_E_ASSEMBLYEXPECTED;
        goto done;
    }

    hr = assembly_get_name(assembly, &name);
    if (FAILED(hr))
        goto done;

    hr = assembly_get_pubkey_token(assembly, &token);
    if (FAILED(hr))
        goto done;

    hr = assembly_get_version(assembly, &version);
    if (FAILED(hr))
        goto done;

    hr = assembly_get_runtime_version(assembly, &clr_version);
    if (FAILED(hr))
        goto done;

    hr = assembly_get_external_files(assembly, &external_files, &count);
    if (FAILED(hr))
        goto done;

    cache_lock( cache );

    architecture = assembly_get_architecture(assembly);
    get_assembly_directory(asmdir, MAX_PATH, clr_version, architecture);

    dst_len += strlenW(asmdir) + strlenW(name) + strlenW(version) + strlenW(token);
    if (!(dst_dir = HeapAlloc(GetProcessHeap(), 0, dst_len * sizeof(WCHAR))))
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }
    if (!strcmp(clr_version, "v4.0.30319"))
        dst_len = sprintfW(dst_dir, format_v40, asmdir, name, version, token);
    else
        dst_len = sprintfW(dst_dir, format, asmdir, name, version, token);

    create_full_path(dst_dir);

    hr = assembly_get_path(assembly, &asmpath);
    if (FAILED(hr))
        goto done;

    if ((p = strrchrW(asmpath, '\\')))
    {
        filename = p + 1;
        src_dir  = asmpath;
        src_len  = filename - asmpath;
    }
    else
    {
        filename = asmpath;
        src_dir  = NULL;
        src_len  = 0;
    }
    hr = copy_file(src_dir, src_len, dst_dir, dst_len, filename);
    if (FAILED(hr))
        goto done;

    for (i = 0; i < count; i++)
    {
        hr = copy_file(src_dir, src_len, dst_dir, dst_len, external_files[i]);
        if (FAILED(hr))
            break;
    }

done:
    HeapFree(GetProcessHeap(), 0, name);
    HeapFree(GetProcessHeap(), 0, token);
    HeapFree(GetProcessHeap(), 0, version);
    HeapFree(GetProcessHeap(), 0, asmpath);
    HeapFree(GetProcessHeap(), 0, dst_dir);
    for (i = 0; i < count; i++) HeapFree(GetProcessHeap(), 0, external_files[i]);
    HeapFree(GetProcessHeap(), 0, external_files);
    assembly_release(assembly);
    cache_unlock( cache );
    return hr;
}

static const IAssemblyCacheVtbl AssemblyCacheVtbl = {
    IAssemblyCacheImpl_QueryInterface,
    IAssemblyCacheImpl_AddRef,
    IAssemblyCacheImpl_Release,
    IAssemblyCacheImpl_UninstallAssembly,
    IAssemblyCacheImpl_QueryAssemblyInfo,
    IAssemblyCacheImpl_CreateAssemblyCacheItem,
    IAssemblyCacheImpl_CreateAssemblyScavenger,
    IAssemblyCacheImpl_InstallAssembly
};

/******************************************************************
 *  CreateAssemblyCache   (FUSION.@)
 */
HRESULT WINAPI CreateAssemblyCache(IAssemblyCache **ppAsmCache, DWORD dwReserved)
{
    IAssemblyCacheImpl *cache;

    TRACE("(%p, %d)\n", ppAsmCache, dwReserved);

    if (!ppAsmCache)
        return E_INVALIDARG;

    *ppAsmCache = NULL;

    cache = HeapAlloc(GetProcessHeap(), 0, sizeof(IAssemblyCacheImpl));
    if (!cache)
        return E_OUTOFMEMORY;

    cache->IAssemblyCache_iface.lpVtbl = &AssemblyCacheVtbl;
    cache->ref = 1;
    cache->lock = CreateMutexW( NULL, FALSE, cache_mutex_nameW );
    if (!cache->lock)
    {
        HeapFree( GetProcessHeap(), 0, cache );
        return HRESULT_FROM_WIN32( GetLastError() );
    }
    *ppAsmCache = &cache->IAssemblyCache_iface;
    return S_OK;
}

/* IAssemblyCacheItem */

static inline IAssemblyCacheItemImpl *impl_from_IAssemblyCacheItem(IAssemblyCacheItem *iface)
{
    return CONTAINING_RECORD(iface, IAssemblyCacheItemImpl, IAssemblyCacheItem_iface);
}

static HRESULT WINAPI IAssemblyCacheItemImpl_QueryInterface(IAssemblyCacheItem *iface,
                                                            REFIID riid, LPVOID *ppobj)
{
    IAssemblyCacheItemImpl *This = impl_from_IAssemblyCacheItem(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyCacheItem))
    {
        IAssemblyCacheItem_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyCacheItemImpl_AddRef(IAssemblyCacheItem *iface)
{
    IAssemblyCacheItemImpl *This = impl_from_IAssemblyCacheItem(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyCacheItemImpl_Release(IAssemblyCacheItem *iface)
{
    IAssemblyCacheItemImpl *This = impl_from_IAssemblyCacheItem(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount + 1);

    if (!refCount)
        HeapFree(GetProcessHeap(), 0, This);

    return refCount;
}

static HRESULT WINAPI IAssemblyCacheItemImpl_CreateStream(IAssemblyCacheItem *iface,
                                                        DWORD dwFlags,
                                                        LPCWSTR pszStreamName,
                                                        DWORD dwFormat,
                                                        DWORD dwFormatFlags,
                                                        IStream **ppIStream,
                                                        ULARGE_INTEGER *puliMaxSize)
{
    FIXME("(%p, %d, %s, %d, %d, %p, %p) stub!\n", iface, dwFlags,
          debugstr_w(pszStreamName), dwFormat, dwFormatFlags, ppIStream, puliMaxSize);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheItemImpl_Commit(IAssemblyCacheItem *iface,
                                                  DWORD dwFlags,
                                                  ULONG *pulDisposition)
{
    FIXME("(%p, %d, %p) stub!\n", iface, dwFlags, pulDisposition);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheItemImpl_AbortItem(IAssemblyCacheItem *iface)
{
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static const IAssemblyCacheItemVtbl AssemblyCacheItemVtbl = {
    IAssemblyCacheItemImpl_QueryInterface,
    IAssemblyCacheItemImpl_AddRef,
    IAssemblyCacheItemImpl_Release,
    IAssemblyCacheItemImpl_CreateStream,
    IAssemblyCacheItemImpl_Commit,
    IAssemblyCacheItemImpl_AbortItem
};
