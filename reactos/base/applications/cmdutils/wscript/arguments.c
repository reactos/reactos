/*
 * Copyright 2011 Michal Zietek
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

#include "wscript.h"

WCHAR **argums;
int numOfArgs;

static HRESULT WINAPI Arguments2_QueryInterface(IArguments2 *iface, REFIID riid, void **ppv)
{
    WINE_TRACE("(%s %p)\n", wine_dbgstr_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)
       || IsEqualGUID(&IID_IDispatch, riid)
       || IsEqualGUID(&IID_IArguments2, riid)) {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI Arguments2_AddRef(IArguments2 *iface)
{
    return 2;
}

static ULONG WINAPI Arguments2_Release(IArguments2 *iface)
{
    return 1;
}

static HRESULT WINAPI Arguments2_GetTypeInfoCount(IArguments2 *iface, UINT *pctinfo)
{
    WINE_TRACE("(%p)\n", pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI Arguments2_GetTypeInfo(IArguments2 *iface, UINT iTInfo, LCID lcid,
        ITypeInfo **ppTInfo)
{
    WINE_TRACE("(%x %x %p\n", iTInfo, lcid, ppTInfo);

    ITypeInfo_AddRef(arguments_ti);
    *ppTInfo = arguments_ti;
    return S_OK;
}

static HRESULT WINAPI Arguments2_GetIDsOfNames(IArguments2 *iface, REFIID riid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    WINE_TRACE("(%s %p %d %x %p)\n", wine_dbgstr_guid(riid), rgszNames,
        cNames, lcid, rgDispId);

    return ITypeInfo_GetIDsOfNames(arguments_ti, rgszNames, cNames, rgDispId);
}

static HRESULT WINAPI Arguments2_Invoke(IArguments2 *iface, DISPID dispIdMember, REFIID riid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    WINE_TRACE("(%d %p %p)\n", dispIdMember, pDispParams, pVarResult);

    return ITypeInfo_Invoke(arguments_ti, iface, dispIdMember, wFlags, pDispParams,
            pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI Arguments2_Item(IArguments2 *iface, LONG index, BSTR *out_Value)
{
    WINE_TRACE("(%d %p)\n", index, out_Value);

    if(index<0 || index >= numOfArgs)
        return E_INVALIDARG;
    if(!(*out_Value = SysAllocString(argums[index])))
        return E_OUTOFMEMORY;

    return S_OK;
}

static HRESULT WINAPI Arguments2_Count(IArguments2 *iface, LONG *out_Count)
{
    WINE_TRACE("(%p)\n", out_Count);

    *out_Count = numOfArgs;
    return S_OK;
}

static HRESULT WINAPI Arguments2_get_length(IArguments2 *iface, LONG *out_Count)
{
    WINE_TRACE("(%p)\n", out_Count);

    *out_Count = numOfArgs;
    return S_OK;
}

static const IArguments2Vtbl Arguments2Vtbl = {
    Arguments2_QueryInterface,
    Arguments2_AddRef,
    Arguments2_Release,
    Arguments2_GetTypeInfoCount,
    Arguments2_GetTypeInfo,
    Arguments2_GetIDsOfNames,
    Arguments2_Invoke,
    Arguments2_Item,
    Arguments2_Count,
    Arguments2_get_length
};

IArguments2 arguments_obj = { &Arguments2Vtbl };
