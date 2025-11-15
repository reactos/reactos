/*
 * IMetaDataDispenserEx - dynamic creation/editing of assemblies
 *
 * Copyright 2010 Vincent Povirk for CodeWeavers
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

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "ole2.h"
#include "cor.h"
#include "mscoree.h"
#include "corhdr.h"
#include "cordebug.h"
#include "metahost.h"
#include "wine/list.h"
#include "mscoree_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

typedef struct MetaDataDispenser
{
    IMetaDataDispenserEx IMetaDataDispenserEx_iface;
    LONG ref;
} MetaDataDispenser;

static inline MetaDataDispenser *impl_from_IMetaDataDispenserEx(IMetaDataDispenserEx *iface)
{
    return CONTAINING_RECORD(iface, MetaDataDispenser, IMetaDataDispenserEx_iface);
}

static HRESULT WINAPI MetaDataDispenser_QueryInterface(IMetaDataDispenserEx* iface,
    REFIID riid, void **ppvObject)
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IMetaDataDispenserEx) ||
        IsEqualGUID(riid, &IID_IMetaDataDispenser) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IMetaDataDispenserEx_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI MetaDataDispenser_AddRef(IMetaDataDispenserEx* iface)
{
    MetaDataDispenser *This = impl_from_IMetaDataDispenserEx(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI MetaDataDispenser_Release(IMetaDataDispenserEx* iface)
{
    MetaDataDispenser *This = impl_from_IMetaDataDispenserEx(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref=%lu\n", This, ref);

    if (ref == 0)
    {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI MetaDataDispenser_DefineScope(IMetaDataDispenserEx* iface,
    REFCLSID rclsid, DWORD dwCreateFlags, REFIID riid, IUnknown **ppIUnk)
{
    FIXME("%p %s %lx %s %p\n", iface, debugstr_guid(rclsid), dwCreateFlags,
        debugstr_guid(riid), ppIUnk);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_OpenScope(IMetaDataDispenserEx* iface,
    LPCWSTR szScope, DWORD dwOpenFlags, REFIID riid, IUnknown **ppIUnk)
{
    FIXME("%p %s %lx %s %p\n", iface, debugstr_w(szScope), dwOpenFlags,
        debugstr_guid(riid), ppIUnk);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_OpenScopeOnMemory(IMetaDataDispenserEx* iface,
    const void *pData, ULONG cbData, DWORD dwOpenFlags, REFIID riid, IUnknown **ppIUnk)
{
    FIXME("%p %p %lu %lx %s %p\n", iface, pData, cbData, dwOpenFlags,
        debugstr_guid(riid), ppIUnk);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_SetOption(IMetaDataDispenserEx* iface,
    REFGUID optionid, const VARIANT *value)
{
    FIXME("%p %s\n", iface, debugstr_guid(optionid));
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_GetOption(IMetaDataDispenserEx* iface,
    REFGUID optionid, VARIANT *pvalue)
{
    FIXME("%p %s\n", iface, debugstr_guid(optionid));
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_OpenScopeOnITypeInfo(IMetaDataDispenserEx* iface,
    ITypeInfo *pITI, DWORD dwOpenFlags, REFIID riid, IUnknown **ppIUnk)
{
    FIXME("%p %p %lu %s %p\n", iface, pITI, dwOpenFlags, debugstr_guid(riid), ppIUnk);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_GetCORSystemDirectory(IMetaDataDispenserEx* iface,
    LPWSTR szBuffer, DWORD cchBuffer, DWORD *pchBuffer)
{
    FIXME("%p %p %lu %p\n", iface, szBuffer, cchBuffer, pchBuffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_FindAssembly(IMetaDataDispenserEx* iface,
    LPCWSTR szAppBase, LPCWSTR szPrivateBin, LPCWSTR szGlobalBin, LPCWSTR szAssemblyName,
    LPWSTR szName, ULONG cchName, ULONG *pcName)
{
    FIXME("%p %s %s %s %s %p %lu %p\n", iface, debugstr_w(szAppBase),
        debugstr_w(szPrivateBin), debugstr_w(szGlobalBin),
        debugstr_w(szAssemblyName), szName, cchName, pcName);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetaDataDispenser_FindAssemblyModule(IMetaDataDispenserEx* iface,
    LPCWSTR szAppBase, LPCWSTR szPrivateBin, LPCWSTR szGlobalBin, LPCWSTR szAssemblyName,
    LPCWSTR szModuleName, LPWSTR szName, ULONG cchName, ULONG *pcName)
{
    FIXME("%p %s %s %s %s %s %p %lu %p\n", iface, debugstr_w(szAppBase),
        debugstr_w(szPrivateBin), debugstr_w(szGlobalBin), debugstr_w(szAssemblyName),
        debugstr_w(szModuleName), szName, cchName, pcName);
    return E_NOTIMPL;
}

static const struct IMetaDataDispenserExVtbl MetaDataDispenserVtbl =
{
    MetaDataDispenser_QueryInterface,
    MetaDataDispenser_AddRef,
    MetaDataDispenser_Release,
    MetaDataDispenser_DefineScope,
    MetaDataDispenser_OpenScope,
    MetaDataDispenser_OpenScopeOnMemory,
    MetaDataDispenser_SetOption,
    MetaDataDispenser_GetOption,
    MetaDataDispenser_OpenScopeOnITypeInfo,
    MetaDataDispenser_GetCORSystemDirectory,
    MetaDataDispenser_FindAssembly,
    MetaDataDispenser_FindAssemblyModule
};

HRESULT MetaDataDispenser_CreateInstance(IUnknown **ppUnk)
{
    MetaDataDispenser *This;

    This = malloc(sizeof(MetaDataDispenser));

    if (!This)
        return E_OUTOFMEMORY;

    This->IMetaDataDispenserEx_iface.lpVtbl = &MetaDataDispenserVtbl;
    This->ref = 1;

    *ppUnk = (IUnknown*)&This->IMetaDataDispenserEx_iface;

    return S_OK;
}
