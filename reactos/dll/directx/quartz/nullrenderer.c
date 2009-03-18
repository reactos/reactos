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

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "quartz_private.h"
#include "control_private.h"
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

static const WCHAR wcsInputPinName[] = {'i','n','p','u','t',' ','p','i','n',0};

static const IBaseFilterVtbl NullRenderer_Vtbl;
static const IUnknownVtbl IInner_VTable;
static const IPinVtbl NullRenderer_InputPin_Vtbl;

typedef struct NullRendererImpl
{
    const IBaseFilterVtbl * lpVtbl;
    const IUnknownVtbl * IInner_vtbl;

    LONG refCount;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    REFERENCE_TIME rtStreamStart;
    IReferenceClock * pClock;
    FILTER_INFO filterInfo;

    InputPin *pInputPin;
    IUnknown * pUnkOuter;
    BOOL bUnkOuterValid;
    BOOL bAggregatable;
    MediaSeekingImpl mediaSeeking;
} NullRendererImpl;

static HRESULT NullRenderer_Sample(LPVOID iface, IMediaSample * pSample)
{
    NullRendererImpl *This = iface;
    HRESULT hr = S_OK;

    TRACE("%p %p\n", iface, pSample);

    EnterCriticalSection(&This->csFilter);
    if (This->pInputPin->flushing || This->pInputPin->end_of_stream)
        hr = S_FALSE;
    LeaveCriticalSection(&This->csFilter);

    return hr;
}

static HRESULT NullRenderer_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    TRACE("Not a stub!\n");
    return S_OK;
}

static inline NullRendererImpl *impl_from_IMediaSeeking( IMediaSeeking *iface )
{
    return (NullRendererImpl *)((char*)iface - FIELD_OFFSET(NullRendererImpl, mediaSeeking.lpVtbl));
}

static HRESULT WINAPI NullRendererImpl_Seeking_QueryInterface(IMediaSeeking * iface, REFIID riid, LPVOID * ppv)
{
    NullRendererImpl *This = impl_from_IMediaSeeking(iface);

    return IUnknown_QueryInterface((IUnknown *)This, riid, ppv);
}

static ULONG WINAPI NullRendererImpl_Seeking_AddRef(IMediaSeeking * iface)
{
    NullRendererImpl *This = impl_from_IMediaSeeking(iface);

    return IUnknown_AddRef((IUnknown *)This);
}

static ULONG WINAPI NullRendererImpl_Seeking_Release(IMediaSeeking * iface)
{
    NullRendererImpl *This = impl_from_IMediaSeeking(iface);

    return IUnknown_Release((IUnknown *)This);
}

static const IMediaSeekingVtbl TransformFilter_Seeking_Vtbl =
{
    NullRendererImpl_Seeking_QueryInterface,
    NullRendererImpl_Seeking_AddRef,
    NullRendererImpl_Seeking_Release,
    MediaSeekingImpl_GetCapabilities,
    MediaSeekingImpl_CheckCapabilities,
    MediaSeekingImpl_IsFormatSupported,
    MediaSeekingImpl_QueryPreferredFormat,
    MediaSeekingImpl_GetTimeFormat,
    MediaSeekingImpl_IsUsingTimeFormat,
    MediaSeekingImpl_SetTimeFormat,
    MediaSeekingImpl_GetDuration,
    MediaSeekingImpl_GetStopPosition,
    MediaSeekingImpl_GetCurrentPosition,
    MediaSeekingImpl_ConvertTimeFormat,
    MediaSeekingImpl_SetPositions,
    MediaSeekingImpl_GetPositions,
    MediaSeekingImpl_GetAvailable,
    MediaSeekingImpl_SetRate,
    MediaSeekingImpl_GetRate,
    MediaSeekingImpl_GetPreroll
};

static HRESULT NullRendererImpl_Change(IBaseFilter *iface)
{
    TRACE("(%p)\n", iface);
    return S_OK;
}

HRESULT NullRenderer_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    PIN_INFO piInput;
    NullRendererImpl * pNullRenderer;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    pNullRenderer = CoTaskMemAlloc(sizeof(NullRendererImpl));
    pNullRenderer->pUnkOuter = pUnkOuter;
    pNullRenderer->bUnkOuterValid = FALSE;
    pNullRenderer->bAggregatable = FALSE;
    pNullRenderer->IInner_vtbl = &IInner_VTable;

    pNullRenderer->lpVtbl = &NullRenderer_Vtbl;
    pNullRenderer->refCount = 1;
    InitializeCriticalSection(&pNullRenderer->csFilter);
    pNullRenderer->csFilter.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": NullRendererImpl.csFilter");
    pNullRenderer->state = State_Stopped;
    pNullRenderer->pClock = NULL;
    ZeroMemory(&pNullRenderer->filterInfo, sizeof(FILTER_INFO));

    /* construct input pin */
    piInput.dir = PINDIR_INPUT;
    piInput.pFilter = (IBaseFilter *)pNullRenderer;
    lstrcpynW(piInput.achName, wcsInputPinName, sizeof(piInput.achName) / sizeof(piInput.achName[0]));

    hr = InputPin_Construct(&NullRenderer_InputPin_Vtbl, &piInput, NullRenderer_Sample, (LPVOID)pNullRenderer, NullRenderer_QueryAccept, NULL, &pNullRenderer->csFilter, NULL, (IPin **)&pNullRenderer->pInputPin);

    if (SUCCEEDED(hr))
    {
        MediaSeekingImpl_Init((IBaseFilter*)pNullRenderer, NullRendererImpl_Change, NullRendererImpl_Change, NullRendererImpl_Change, &pNullRenderer->mediaSeeking, &pNullRenderer->csFilter);
        pNullRenderer->mediaSeeking.lpVtbl = &TransformFilter_Seeking_Vtbl;

        *ppv = pNullRenderer;
    }
    else
    {
        pNullRenderer->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&pNullRenderer->csFilter);
        CoTaskMemFree(pNullRenderer);
    }

    return hr;
}

static HRESULT WINAPI NullRendererInner_QueryInterface(IUnknown * iface, REFIID riid, LPVOID * ppv)
{
    ICOM_THIS_MULTI(NullRendererImpl, IInner_vtbl, iface);
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IInner_vtbl;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        *ppv = &This->mediaSeeking;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin) && !IsEqualIID(riid, &IID_IVideoWindow))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI NullRendererInner_AddRef(IUnknown * iface)
{
    ICOM_THIS_MULTI(NullRendererImpl, IInner_vtbl, iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p/%p)->() AddRef from %d\n", This, iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI NullRendererInner_Release(IUnknown * iface)
{
    ICOM_THIS_MULTI(NullRendererImpl, IInner_vtbl, iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p/%p)->() Release from %d\n", This, iface, refCount + 1);

    if (!refCount)
    {
        IPin *pConnectedTo;

        if (This->pClock)
            IReferenceClock_Release(This->pClock);

        if (SUCCEEDED(IPin_ConnectedTo((IPin *)This->pInputPin, &pConnectedTo)))
        {
            IPin_Disconnect(pConnectedTo);
            IPin_Release(pConnectedTo);
        }
        IPin_Disconnect((IPin *)This->pInputPin);
        IPin_Release((IPin *)This->pInputPin);

        This->lpVtbl = NULL;

        This->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csFilter);

        TRACE("Destroying Null Renderer\n");
        CoTaskMemFree(This);
        return 0;
    }
    else
        return refCount;
}

static const IUnknownVtbl IInner_VTable =
{
    NullRendererInner_QueryInterface,
    NullRendererInner_AddRef,
    NullRendererInner_Release
};

static HRESULT WINAPI NullRenderer_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    if (This->bAggregatable)
        This->bUnkOuterValid = TRUE;

    if (This->pUnkOuter)
    {
        if (This->bAggregatable)
            return IUnknown_QueryInterface(This->pUnkOuter, riid, ppv);

        if (IsEqualIID(riid, &IID_IUnknown))
        {
            HRESULT hr;

            IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
            hr = IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
            IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
            This->bAggregatable = TRUE;
            return hr;
        }

        *ppv = NULL;
        return E_NOINTERFACE;
    }

    return IUnknown_QueryInterface((IUnknown *)&(This->IInner_vtbl), riid, ppv);
}

static ULONG WINAPI NullRenderer_AddRef(IBaseFilter * iface)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_AddRef(This->pUnkOuter);
    return IUnknown_AddRef((IUnknown *)&(This->IInner_vtbl));
}

static ULONG WINAPI NullRenderer_Release(IBaseFilter * iface)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    if (This->pUnkOuter && This->bUnkOuterValid)
        return IUnknown_Release(This->pUnkOuter);
    return IUnknown_Release((IUnknown *)&(This->IInner_vtbl));
}

/** IPersist methods **/

static HRESULT WINAPI NullRenderer_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClsid);

    *pClsid = CLSID_NullRenderer;

    return S_OK;
}

/** IMediaFilter methods **/

static HRESULT WINAPI NullRenderer_Stop(IBaseFilter * iface)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    {
        This->state = State_Stopped;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_Pause(IBaseFilter * iface)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    {
        if (This->state == State_Stopped)
            This->pInputPin->end_of_stream = 0;
        This->state = State_Paused;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&This->csFilter);
    {
        This->rtStreamStart = tStart;
        This->state = State_Running;
        This->pInputPin->end_of_stream = 0;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%d, %p)\n", This, iface, dwMilliSecsTimeout, pState);

    EnterCriticalSection(&This->csFilter);
    {
        *pState = This->state;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClock);

    EnterCriticalSection(&This->csFilter);
    {
        if (This->pClock)
            IReferenceClock_Release(This->pClock);
        This->pClock = pClock;
        if (This->pClock)
            IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppClock);

    EnterCriticalSection(&This->csFilter);
    {
        *ppClock = This->pClock;
        if (This->pClock)
            IReferenceClock_AddRef(This->pClock);
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

/** IBaseFilter implementation **/

static HRESULT NullRenderer_GetPin(IBaseFilter *iface, ULONG pos, IPin **pin, DWORD *lastsynctick)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    /* Our pins are static, not changing so setting static tick count is ok */
    *lastsynctick = 0;

    if (pos >= 1)
        return S_FALSE;

    *pin = (IPin *)This->pInputPin;
    IPin_AddRef(*pin);
    return S_OK;
}

static HRESULT WINAPI NullRenderer_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    return IEnumPinsImpl_Construct(ppEnum, NullRenderer_GetPin, iface);
}

static HRESULT WINAPI NullRenderer_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, debugstr_w(Id), ppPin);

    FIXME("NullRenderer::FindPin(...)\n");

    /* FIXME: critical section */

    return E_NOTIMPL;
}

static HRESULT WINAPI NullRenderer_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pInfo);

    strcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;

    TRACE("(%p/%p)->(%p, %s)\n", This, iface, pGraph, debugstr_w(pName));

    EnterCriticalSection(&This->csFilter);
    {
        if (pName)
            strcpyW(This->filterInfo.achName, pName);
        else
            *This->filterInfo.achName = '\0';
        This->filterInfo.pGraph = pGraph; /* NOTE: do NOT increase ref. count */
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI NullRenderer_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    NullRendererImpl *This = (NullRendererImpl *)iface;
    TRACE("(%p/%p)->(%p)\n", This, iface, pVendorInfo);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl NullRenderer_Vtbl =
{
    NullRenderer_QueryInterface,
    NullRenderer_AddRef,
    NullRenderer_Release,
    NullRenderer_GetClassID,
    NullRenderer_Stop,
    NullRenderer_Pause,
    NullRenderer_Run,
    NullRenderer_GetState,
    NullRenderer_SetSyncSource,
    NullRenderer_GetSyncSource,
    NullRenderer_EnumPins,
    NullRenderer_FindPin,
    NullRenderer_QueryFilterInfo,
    NullRenderer_JoinFilterGraph,
    NullRenderer_QueryVendorInfo
};

static HRESULT WINAPI NullRenderer_InputPin_EndOfStream(IPin * iface)
{
    InputPin* This = (InputPin*)iface;
    IMediaEventSink* pEventSink;
    IFilterGraph *graph;
    HRESULT hr = S_OK;

    TRACE("(%p/%p)->()\n", This, iface);

    InputPin_EndOfStream(iface);
    graph = ((NullRendererImpl*)This->pin.pinInfo.pFilter)->filterInfo.pGraph;
    if (graph)
    {
        hr = IFilterGraph_QueryInterface(((NullRendererImpl*)This->pin.pinInfo.pFilter)->filterInfo.pGraph, &IID_IMediaEventSink, (LPVOID*)&pEventSink);
        if (SUCCEEDED(hr))
        {
            hr = IMediaEventSink_Notify(pEventSink, EC_COMPLETE, S_OK, 0);
            IMediaEventSink_Release(pEventSink);
        }
    }

    return hr;
}

static const IPinVtbl NullRenderer_InputPin_Vtbl =
{
    InputPin_QueryInterface,
    IPinImpl_AddRef,
    InputPin_Release,
    InputPin_Connect,
    InputPin_ReceiveConnection,
    IPinImpl_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    IPinImpl_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    NullRenderer_InputPin_EndOfStream,
    InputPin_BeginFlush,
    InputPin_EndFlush,
    InputPin_NewSegment
};
