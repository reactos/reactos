/*
 * IAutomaticUpdates implementation
 *
 * Copyright 2008 Hans Leidekker
 * Copyright 2011 Bernhard Loos
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
#include "winuser.h"
#include "ole2.h"
#include "wuapi.h"
#include "wuapi_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wuapi);

typedef struct _systeminfo
{
    ISystemInformation ISystemInformation_iface;
    LONG refs;
} systeminfo;

static inline systeminfo *impl_from_ISystemInformation(ISystemInformation *iface)
{
    return CONTAINING_RECORD(iface, systeminfo, ISystemInformation_iface);
}

static ULONG WINAPI systeminfo_AddRef(ISystemInformation *iface)
{
    systeminfo *This = impl_from_ISystemInformation(iface);
    return InterlockedIncrement(&This->refs);
}

static ULONG WINAPI systeminfo_Release(ISystemInformation *iface)
{
    systeminfo *This = impl_from_ISystemInformation(iface);
    LONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
    {
        TRACE("destroying %p\n", This);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refs;
}

static HRESULT WINAPI systeminfo_QueryInterface(ISystemInformation *iface,
                    REFIID riid, void **ppvObject)
{
    systeminfo *This = impl_from_ISystemInformation(iface);

    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_ISystemInformation) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IUnknown))
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }
    ISystemInformation_AddRef(iface);
    return S_OK;
}

static HRESULT WINAPI systeminfo_GetTypeInfoCount(ISystemInformation *iface,
                    UINT *pctinfo )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI systeminfo_GetTypeInfo(ISystemInformation *iface,
                    UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI systeminfo_GetIDsOfNames(ISystemInformation *iface,
                    REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid,
                    DISPID *rgDispId)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI systeminfo_Invoke(ISystemInformation *iface,
                    DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags,
                    DISPPARAMS *pDispParams, VARIANT *pVarResult,
                    EXCEPINFO *pExcepInfo, UINT *puArgErr )
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI systeminfo_get_OemHardwareSupportLink(ISystemInformation *iface,
                                BSTR *retval)
{
    FIXME("\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI systeminfo_get_RebootRequired(ISystemInformation *iface,
                                VARIANT_BOOL *retval)
{
    *retval = VARIANT_FALSE;
    return S_OK;
}

static const struct ISystemInformationVtbl systeminfo_vtbl =
{
    systeminfo_QueryInterface,
    systeminfo_AddRef,
    systeminfo_Release,
    systeminfo_GetTypeInfoCount,
    systeminfo_GetTypeInfo,
    systeminfo_GetIDsOfNames,
    systeminfo_Invoke,
    systeminfo_get_OemHardwareSupportLink,
    systeminfo_get_RebootRequired
};

HRESULT SystemInformation_create(LPVOID *ppObj)
{
    systeminfo *info;

    TRACE("(%p)\n", ppObj);

    info = HeapAlloc(GetProcessHeap(), 0, sizeof(*info));
    if (!info)
        return E_OUTOFMEMORY;

    info->ISystemInformation_iface.lpVtbl = &systeminfo_vtbl;
    info->refs = 1;

    *ppObj = &info->ISystemInformation_iface;

    TRACE("returning iface %p\n", *ppObj);
    return S_OK;
}
