/*
 * Copyright 2022 Robert Wilhelm
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

#define  COBJMACROS

#include "dispex.h"
#include "wshom_private.h"
#include "wshom.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wshom);

static IWshNetwork2 WshNetwork2;

static HRESULT WINAPI WshNetwork2_QueryInterface(IWshNetwork2 *iface, REFIID riid, void **ppv)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppv);

    if(IsEqualGUID(riid, &IID_IUnknown) ||
           IsEqualGUID(riid, &IID_IDispatch) ||
           IsEqualGUID(riid, &IID_IWshNetwork) ||
           IsEqualGUID(riid, &IID_IWshNetwork2)) {
        *ppv = iface;
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }
    else {
        WARN("interface not supported %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI WshNetwork2_AddRef(IWshNetwork2 *iface)
{
    TRACE("()\n");
    return 2;
}

static ULONG WINAPI WshNetwork2_Release(IWshNetwork2 *iface)
{
    TRACE("()\n");
    return 2;
}

static HRESULT WINAPI WshNetwork2_GetTypeInfoCount(IWshNetwork2 *iface, UINT *pctinfo)
{
    TRACE("(%p)\n", pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI WshNetwork2_GetTypeInfo(IWshNetwork2 *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("%p, %u, %lx, %p.\n", iface, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IWshNetwork2_tid, ppTInfo);
}

static HRESULT WINAPI WshNetwork2_GetIDsOfNames(IWshNetwork2 *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %lx, %p.\n", iface, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IWshNetwork2_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshNetwork2_Invoke(IWshNetwork2 *iface, DISPID dispIdMember, REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %ld, %s, %lx, %d, %p, %p, %p, %p.\n", iface, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IWshNetwork2_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, &WshNetwork2, dispIdMember, wFlags,
                pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI WshNetwork2_get_UserDomain(IWshNetwork2 *iface, BSTR *user_domain)
{
    TRACE("%p, %p.\n", iface, user_domain);

    if (!user_domain)
        return E_POINTER;

    return get_env_var(L"USERDOMAIN", user_domain);
}

static HRESULT WINAPI WshNetwork2_get_UserName(IWshNetwork2 *iface, BSTR *user_name)
{
    BOOL ret;
    DWORD len = 0;

    TRACE("%p, %p.\n", iface, user_name);

    if (!user_name)
        return E_POINTER;

    GetUserNameW(NULL, &len);
    *user_name = SysAllocStringLen(NULL, len-1);
    if (!*user_name)
        return E_OUTOFMEMORY;

    ret = GetUserNameW(*user_name, &len);
    if (!ret) {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        SysFreeString(*user_name);
        *user_name = NULL;
        return hr;
    }

    return S_OK;
}

static HRESULT WINAPI WshNetwork2_get_UserProfile(IWshNetwork2 *iface, BSTR *user_profile)
{
    FIXME("%p stub\n", user_profile);

    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_get_ComputerName(IWshNetwork2 *iface, BSTR *name)
{
    HRESULT hr = S_OK;
    DWORD len = 0;
    BOOL ret;

    TRACE("%p, %p.\n", iface, name);

    if (!name)
        return E_POINTER;

    GetComputerNameW(NULL, &len);
    *name = SysAllocStringLen(NULL, len - 1);
    if (!*name)
        return E_OUTOFMEMORY;

    ret = GetComputerNameW(*name, &len);
    if (!ret)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        SysFreeString(*name);
        *name = NULL;
        return hr;
    }

    return S_OK;
}

static HRESULT WINAPI WshNetwork2_get_Organization(IWshNetwork2 *iface, BSTR *name)
{
    FIXME("%p stub\n", name);

    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_get_Site(IWshNetwork2 *iface, BSTR *name)
{
    FIXME("%p stub\n", name);

    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_MapNetworkDrive(IWshNetwork2 *iface, BSTR local_name, BSTR remote_name,
        VARIANT *update_profile, VARIANT *user_name, VARIANT *password)
{
    FIXME("%s, %s, %s, %p, %p stub\n", debugstr_w(local_name), debugstr_w(remote_name), debugstr_variant(update_profile),
            user_name, password);

    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_RemoveNetworkDrive(IWshNetwork2 *iface, BSTR name, VARIANT *force, VARIANT *update_profile)
{
    FIXME("%s, %s, %s stub\n", debugstr_w(name), debugstr_variant(force), debugstr_variant(update_profile));

    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_EnumNetworkDrives(IWshNetwork2 *iface, IWshCollection **ret)
{
    FIXME("%p stub\n", ret);

    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_AddPrinterConnection(IWshNetwork2 *iface, BSTR local_name, BSTR remote_name,
        VARIANT *update_profile, VARIANT *user_name, VARIANT *password)
{
    FIXME("%s, %s, %s, %p, %p stub\n", debugstr_w(local_name), debugstr_w(remote_name), debugstr_variant(update_profile),
            user_name, password);

    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_RemovePrinterConnection(IWshNetwork2 *iface, BSTR name, VARIANT *force, VARIANT *update_profile)
{
    FIXME("%s, %s, %s stub\n", debugstr_w(name), debugstr_variant(force), debugstr_variant(update_profile));

    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_EnumPrinterConnections(IWshNetwork2 *iface, IWshCollection **ret)
{
    FIXME("%p stub\n", ret);

    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_SetDefaultPrinter(IWshNetwork2 *iface, BSTR name)
{
    FIXME("%s stub\n", debugstr_w(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI WshNetwork2_AddWindowsPrinterConnection(IWshNetwork2 *iface, BSTR printer_name,
        BSTR driver_name, BSTR port)
{
    FIXME("%s, %s, %s stub\n", debugstr_w(printer_name), debugstr_w(driver_name), debugstr_w(port));

    return E_NOTIMPL;
}

static const IWshNetwork2Vtbl WshNetwork2Vtbl = {
    WshNetwork2_QueryInterface,
    WshNetwork2_AddRef,
    WshNetwork2_Release,
    WshNetwork2_GetTypeInfoCount,
    WshNetwork2_GetTypeInfo,
    WshNetwork2_GetIDsOfNames,
    WshNetwork2_Invoke,
    WshNetwork2_get_UserDomain,
    WshNetwork2_get_UserName,
    WshNetwork2_get_UserProfile,
    WshNetwork2_get_ComputerName,
    WshNetwork2_get_Organization,
    WshNetwork2_get_Site,
    WshNetwork2_MapNetworkDrive,
    WshNetwork2_RemoveNetworkDrive,
    WshNetwork2_EnumNetworkDrives,
    WshNetwork2_AddPrinterConnection,
    WshNetwork2_RemovePrinterConnection,
    WshNetwork2_EnumPrinterConnections,
    WshNetwork2_SetDefaultPrinter,
    WshNetwork2_AddWindowsPrinterConnection,
};

static IWshNetwork2 WshNetwork2 = { &WshNetwork2Vtbl };

HRESULT WINAPI WshNetworkFactory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    TRACE("%p, %s, %p.\n", outer, debugstr_guid(riid), ppv);

    return IWshNetwork2_QueryInterface(&WshNetwork2, riid, ppv);
}
