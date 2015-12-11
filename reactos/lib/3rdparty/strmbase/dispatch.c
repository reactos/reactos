/*
 * Generic Implementation of IDispatch for strmbase classes
 *
 * Copyright 2012 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

#include <oleauto.h>

HRESULT WINAPI BaseDispatch_Init(BaseDispatch *This, REFIID riid)
{
    ITypeLib *pTypeLib;
    HRESULT hr;

    This->pTypeInfo = NULL;
    hr = LoadRegTypeLib(&LIBID_QuartzTypeLib, 1, 0, LOCALE_SYSTEM_DEFAULT, &pTypeLib);
    if (SUCCEEDED(hr))
    {
        hr = ITypeLib_GetTypeInfoOfGuid(pTypeLib, riid, &This->pTypeInfo);
        ITypeLib_Release(pTypeLib);
    }
    return hr;
}

HRESULT WINAPI BaseDispatch_Destroy(BaseDispatch *This)
{
    if (This->pTypeInfo)
        ITypeInfo_Release(This->pTypeInfo);
    return S_OK;
}

HRESULT WINAPI BaseDispatchImpl_GetIDsOfNames(BaseDispatch *This, REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgdispid)
{
    if (This->pTypeInfo)
        return ITypeInfo_GetIDsOfNames(This->pTypeInfo, rgszNames, cNames, rgdispid);
    return E_NOTIMPL;
}

HRESULT WINAPI BaseDispatchImpl_GetTypeInfo(BaseDispatch *This, REFIID riid, UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
{
    if (This->pTypeInfo)
    {
        ITypeInfo_AddRef(This->pTypeInfo);
        *pptinfo = This->pTypeInfo;
        return S_OK;
    }
    return E_NOTIMPL;
}

HRESULT WINAPI BaseDispatchImpl_GetTypeInfoCount(BaseDispatch *This, UINT *pctinfo)
{
    if (This->pTypeInfo)
        *pctinfo = 1;
    else
        *pctinfo = 0;
    return S_OK;
}
