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

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winver.h"
#include "wincrypt.h"
#include "winreg.h"
#include "shlwapi.h"
#include "dbghelp.h"
#include "ole2.h"
#include "fusion.h"
#include "corerror.h"

#include "fusionpriv.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(fusion);

static BOOL create_full_path(LPCSTR path)
{
    LPSTR new_path;
    BOOL ret = TRUE;
    int len;

    new_path = HeapAlloc(GetProcessHeap(), 0, lstrlenA(path) + 1);
    if (!new_path)
        return FALSE;

    lstrcpyA(new_path, path);

    while ((len = lstrlenA(new_path)) && new_path[len - 1] == '\\')
        new_path[len - 1] = 0;

    while (!CreateDirectoryA(new_path, NULL))
    {
        LPSTR slash;
        DWORD last_error = GetLastError();

        if(last_error == ERROR_ALREADY_EXISTS)
            break;

        if(last_error != ERROR_PATH_NOT_FOUND)
        {
            ret = FALSE;
            break;
        }

        if(!(slash = strrchr(new_path, '\\')))
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

/* IAssemblyCache */

typedef struct {
    const IAssemblyCacheVtbl *lpIAssemblyCacheVtbl;

    LONG ref;
} IAssemblyCacheImpl;

static HRESULT WINAPI IAssemblyCacheImpl_QueryInterface(IAssemblyCache *iface,
                                                        REFIID riid, LPVOID *ppobj)
{
    IAssemblyCacheImpl *This = (IAssemblyCacheImpl *)iface;

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyCache))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyCacheImpl_AddRef(IAssemblyCache *iface)
{
    IAssemblyCacheImpl *This = (IAssemblyCacheImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyCacheImpl_Release(IAssemblyCache *iface)
{
    IAssemblyCacheImpl *This = (IAssemblyCacheImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount + 1);

    if (!refCount)
        HeapFree(GetProcessHeap(), 0, This);

    return refCount;
}

static HRESULT WINAPI IAssemblyCacheImpl_UninstallAssembly(IAssemblyCache *iface,
                                                           DWORD dwFlags,
                                                           LPCWSTR pszAssemblyName,
                                                           LPCFUSION_INSTALL_REFERENCE pRefData,
                                                           ULONG *pulDisposition)
{
    FIXME("(%p, %d, %s, %p, %p) stub!\n", iface, dwFlags,
          debugstr_w(pszAssemblyName), pRefData, pulDisposition);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheImpl_QueryAssemblyInfo(IAssemblyCache *iface,
                                                           DWORD dwFlags,
                                                           LPCWSTR pszAssemblyName,
                                                           ASSEMBLY_INFO *pAsmInfo)
{
    FIXME("(%p, %d, %s, %p) stub!\n", iface, dwFlags,
          debugstr_w(pszAssemblyName), pAsmInfo);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheImpl_CreateAssemblyCacheItem(IAssemblyCache *iface,
                                                                 DWORD dwFlags,
                                                                 PVOID pvReserved,
                                                                 IAssemblyCacheItem **ppAsmItem,
                                                                 LPCWSTR pszAssemblyName)
{
    FIXME("(%p, %d, %p, %p, %s) stub!\n", iface, dwFlags, pvReserved,
          ppAsmItem, debugstr_w(pszAssemblyName));

    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheImpl_CreateAssemblyScavenger(IAssemblyCache *iface,
                                                                 IUnknown **ppUnkReserved)
{
    FIXME("(%p, %p) stub!\n", iface, ppUnkReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyCacheImpl_InstallAssembly(IAssemblyCache *iface,
                                                         DWORD dwFlags,
                                                         LPCWSTR pszManifestFilePath,
                                                         LPCFUSION_INSTALL_REFERENCE pRefData)
{
    ASSEMBLY *assembly;
    LPSTR filename;
    LPSTR name = NULL;
    LPSTR token = NULL;
    LPSTR version = NULL;
    LPSTR asmpath = NULL;
    CHAR path[MAX_PATH];
    CHAR windir[MAX_PATH];
    LPWSTR ext;
    HRESULT hr;

    static const WCHAR ext_exe[] = {'.','e','x','e',0};
    static const WCHAR ext_dll[] = {'.','d','l','l',0};

    TRACE("(%p, %d, %s, %p)\n", iface, dwFlags,
          debugstr_w(pszManifestFilePath), pRefData);

    if (!pszManifestFilePath || !*pszManifestFilePath)
        return E_INVALIDARG;

    if (!(ext = strrchrW(pszManifestFilePath, '.')))
        return HRESULT_FROM_WIN32(ERROR_INVALID_NAME);

    if (lstrcmpiW(ext, ext_exe) && lstrcmpiW(ext, ext_dll))
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

    GetWindowsDirectoryA(windir, MAX_PATH);

    FIXME("Ignoring assembly architecture!\n");

    sprintf(path, "%s\\assembly\\GAC_MSIL\\%s\\%s__%s\\", windir, name,
            version, token);

    create_full_path(path);

    hr = assembly_get_path(assembly, &asmpath);
    if (FAILED(hr))
        goto done;

    filename = PathFindFileNameA(asmpath);

    lstrcatA(path, filename);
    if (!CopyFileA(asmpath, path, FALSE))
        hr = HRESULT_FROM_WIN32(GetLastError());

done:
    HeapFree(GetProcessHeap(), 0, name);
    HeapFree(GetProcessHeap(), 0, token);
    HeapFree(GetProcessHeap(), 0, version);
    HeapFree(GetProcessHeap(), 0, asmpath);
    assembly_release(assembly);
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

    cache->lpIAssemblyCacheVtbl = &AssemblyCacheVtbl;
    cache->ref = 1;

    *ppAsmCache = (IAssemblyCache *)cache;

    return S_OK;
}

/* IAssemblyCacheItem */

typedef struct {
    const IAssemblyCacheItemVtbl *lpIAssemblyCacheItemVtbl;

    LONG ref;
} IAssemblyCacheItemImpl;

static HRESULT WINAPI IAssemblyCacheItemImpl_QueryInterface(IAssemblyCacheItem *iface,
                                                            REFIID riid, LPVOID *ppobj)
{
    IAssemblyCacheItemImpl *This = (IAssemblyCacheItemImpl *)iface;

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyCacheItem))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyCacheItemImpl_AddRef(IAssemblyCacheItem *iface)
{
    IAssemblyCacheItemImpl *This = (IAssemblyCacheItemImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyCacheItemImpl_Release(IAssemblyCacheItem *iface)
{
    IAssemblyCacheItemImpl *This = (IAssemblyCacheItemImpl *)iface;
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

/* IAssemblyEnum */

typedef struct {
    const IAssemblyEnumVtbl *lpIAssemblyEnumVtbl;

    LONG ref;
} IAssemblyEnumImpl;

static HRESULT WINAPI IAssemblyEnumImpl_QueryInterface(IAssemblyEnum *iface,
                                                       REFIID riid, LPVOID *ppobj)
{
    IAssemblyEnumImpl *This = (IAssemblyEnumImpl *)iface;

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyEnum))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyEnumImpl_AddRef(IAssemblyEnum *iface)
{
    IAssemblyEnumImpl *This = (IAssemblyEnumImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyEnumImpl_Release(IAssemblyEnum *iface)
{
    IAssemblyEnumImpl *This = (IAssemblyEnumImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount + 1);

    if (!refCount)
        HeapFree(GetProcessHeap(), 0, This);

    return refCount;
}

static HRESULT WINAPI IAssemblyEnumImpl_GetNextAssembly(IAssemblyEnum *iface,
                                                        LPVOID pvReserved,
                                                        IAssemblyName **ppName,
                                                        DWORD dwFlags)
{
    FIXME("(%p, %p, %p, %d) stub!\n", iface, pvReserved, ppName, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyEnumImpl_Reset(IAssemblyEnum *iface)
{
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyEnumImpl_Clone(IAssemblyEnum *iface,
                                               IAssemblyEnum **ppEnum)
{
    FIXME("(%p, %p) stub!\n", iface, ppEnum);
    return E_NOTIMPL;
}

static const IAssemblyEnumVtbl AssemblyEnumVtbl = {
    IAssemblyEnumImpl_QueryInterface,
    IAssemblyEnumImpl_AddRef,
    IAssemblyEnumImpl_Release,
    IAssemblyEnumImpl_GetNextAssembly,
    IAssemblyEnumImpl_Reset,
    IAssemblyEnumImpl_Clone
};
