/*
 * Generic Implementation of strmbase audio classes
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

static inline BasicAudio *impl_from_IBasicAudio(IBasicAudio *iface)
{
    return CONTAINING_RECORD(iface, BasicAudio, IBasicAudio_iface);
}

HRESULT WINAPI BasicAudio_Init(BasicAudio *pBasicAudio, const IBasicAudioVtbl *lpVtbl)
{
    pBasicAudio->IBasicAudio_iface.lpVtbl = lpVtbl;
    BaseDispatch_Init(&pBasicAudio->baseDispatch, &IID_IBasicAudio);

    return S_OK;
}

HRESULT WINAPI BasicAudio_Destroy(BasicAudio *pBasicAudio)
{
    return BaseDispatch_Destroy(&pBasicAudio->baseDispatch);
}

HRESULT WINAPI BasicAudioImpl_GetTypeInfoCount(IBasicAudio *iface, UINT *pctinfo)
{
    BasicAudio *This = impl_from_IBasicAudio(iface);

    return BaseDispatchImpl_GetTypeInfoCount(&This->baseDispatch, pctinfo);
}

HRESULT WINAPI BasicAudioImpl_GetTypeInfo(IBasicAudio *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    BasicAudio *This = impl_from_IBasicAudio(iface);

    return BaseDispatchImpl_GetTypeInfo(&This->baseDispatch, &IID_NULL, iTInfo, lcid, ppTInfo);
}

HRESULT WINAPI BasicAudioImpl_GetIDsOfNames(IBasicAudio *iface, REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    BasicAudio *This = impl_from_IBasicAudio(iface);

    return BaseDispatchImpl_GetIDsOfNames(&This->baseDispatch, riid, rgszNames, cNames, lcid, rgDispId);
}

HRESULT WINAPI BasicAudioImpl_Invoke(IBasicAudio *iface, DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    BasicAudio *This = impl_from_IBasicAudio(iface);
    ITypeInfo *pTypeInfo;
    HRESULT hr;

    hr = BaseDispatchImpl_GetTypeInfo(&This->baseDispatch, riid, 1, lcid, &pTypeInfo);
    if (SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(pTypeInfo, &This->IBasicAudio_iface, dispIdMember, wFlags, pDispParams, pVarResult, pExepInfo, puArgErr);
        ITypeInfo_Release(pTypeInfo);
    }

    return hr;
}
