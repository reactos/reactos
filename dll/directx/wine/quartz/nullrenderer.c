/*
 * Null Renderer (Promiscuous, not rendering anything at all!)
 *
 * Copyright 2004 Christian Costa
 * Copyright 2008 Maarten Lankhorst
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

#include "config.h"

#include "quartz_private.h"
#include "pin.h"

#include "uuids.h"
#include "vfwmsgs.h"
#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "evcode.h"
#include "strmif.h"
#include "ddraw.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

typedef struct NullRendererImpl
{
    BaseRenderer renderer;
    IUnknown IUnknown_inner;
    IAMFilterMiscFlags IAMFilterMiscFlags_iface;
    IUnknown *outer_unk;
} NullRendererImpl;

static HRESULT WINAPI NullRenderer_DoRenderSample(BaseRenderer *iface, IMediaSample *pMediaSample)
{
    return S_OK;
}

static HRESULT WINAPI NullRenderer_CheckMediaType(BaseRenderer *iface, const AM_MEDIA_TYPE * pmt)
{
    TRACE("Not a stub!\n");
    return S_OK;
}

static const BaseRendererFuncTable RendererFuncTable = {
    NullRenderer_CheckMediaType,
    NullRenderer_DoRenderSample,
    /**/
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    /**/
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

static inline NullRendererImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, NullRendererImpl, IUnknown_inner);
}

static HRESULT WINAPI NullRendererInner_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    NullRendererImpl *This = impl_from_IUnknown(iface);

    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IUnknown_inner;
    else if (IsEqualIID(riid, &IID_IAMFilterMiscFlags))
        *ppv = &This->IAMFilterMiscFlags_iface;
    else
    {
        HRESULT hr;
        hr = BaseRendererImpl_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppv);
        if (SUCCEEDED(hr))
            return hr;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)*ppv);
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin) && !IsEqualIID(riid, &IID_IVideoWindow))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI NullRendererInner_AddRef(IUnknown *iface)
{
    NullRendererImpl *This = impl_from_IUnknown(iface);
    return BaseFilterImpl_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI NullRendererInner_Release(IUnknown *iface)
{
    NullRendererImpl *This = impl_from_IUnknown(iface);
    ULONG refCount = BaseRendererImpl_Release(&This->renderer.filter.IBaseFilter_iface);

    if (!refCount)
    {
        TRACE("Destroying Null Renderer\n");
        CoTaskMemFree(This);
    }

    return refCount;
}

static const IUnknownVtbl IInner_VTable =
{
    NullRendererInner_QueryInterface,
    NullRendererInner_AddRef,
    NullRendererInner_Release
};

static inline NullRendererImpl *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, NullRendererImpl, renderer.filter.IBaseFilter_iface);
}

static HRESULT WINAPI NullRenderer_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    NullRendererImpl *This = impl_from_IBaseFilter(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI NullRenderer_AddRef(IBaseFilter * iface)
{
    NullRendererImpl *This = impl_from_IBaseFilter(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI NullRenderer_Release(IBaseFilter * iface)
{
    NullRendererImpl *This = impl_from_IBaseFilter(iface);
    return IUnknown_Release(This->outer_unk);
}

static const IBaseFilterVtbl NullRenderer_Vtbl =
{
    NullRenderer_QueryInterface,
    NullRenderer_AddRef,
    NullRenderer_Release,
    BaseFilterImpl_GetClassID,
    BaseRendererImpl_Stop,
    BaseRendererImpl_Pause,
    BaseRendererImpl_Run,
    BaseRendererImpl_GetState,
    BaseRendererImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseRendererImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static NullRendererImpl *impl_from_IAMFilterMiscFlags(IAMFilterMiscFlags *iface)
{
    return CONTAINING_RECORD(iface, NullRendererImpl, IAMFilterMiscFlags_iface);
}

static HRESULT WINAPI AMFilterMiscFlags_QueryInterface(IAMFilterMiscFlags *iface, REFIID riid,
        void **ppv)
{
    NullRendererImpl *This = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI AMFilterMiscFlags_AddRef(IAMFilterMiscFlags *iface)
{
    NullRendererImpl *This = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_AddRef(This->outer_unk);
}

static ULONG WINAPI AMFilterMiscFlags_Release(IAMFilterMiscFlags *iface)
{
    NullRendererImpl *This = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_Release(This->outer_unk);
}

static ULONG WINAPI AMFilterMiscFlags_GetMiscFlags(IAMFilterMiscFlags *iface)
{
    return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}

static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl = {
    AMFilterMiscFlags_QueryInterface,
    AMFilterMiscFlags_AddRef,
    AMFilterMiscFlags_Release,
    AMFilterMiscFlags_GetMiscFlags
};

HRESULT NullRenderer_create(IUnknown *pUnkOuter, void **ppv)
{
    HRESULT hr;
    NullRendererImpl *pNullRenderer;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    pNullRenderer = CoTaskMemAlloc(sizeof(NullRendererImpl));
    pNullRenderer->IUnknown_inner.lpVtbl = &IInner_VTable;
    pNullRenderer->IAMFilterMiscFlags_iface.lpVtbl = &IAMFilterMiscFlags_Vtbl;

    if (pUnkOuter)
        pNullRenderer->outer_unk = pUnkOuter;
    else
        pNullRenderer->outer_unk = &pNullRenderer->IUnknown_inner;

    hr = BaseRenderer_Init(&pNullRenderer->renderer, &NullRenderer_Vtbl, pUnkOuter,
            &CLSID_NullRenderer, (DWORD_PTR)(__FILE__ ": NullRendererImpl.csFilter"),
            &RendererFuncTable);

    if (FAILED(hr))
    {
        BaseRendererImpl_Release(&pNullRenderer->renderer.filter.IBaseFilter_iface);
        CoTaskMemFree(pNullRenderer);
    }
    else
        *ppv = &pNullRenderer->IUnknown_inner;

    return S_OK;
}
