/*
 * Transform Filter (Base for decoders, etc...)
 *
 * Copyright 2005 Christian Costa
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
#include "control_private.h"
#include "pin.h"

#include "amvideo.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "strmif.h"
#include "vfw.h"

#include <assert.h>

#include "wine/unicode.h"
#include "wine/debug.h"

#include "transform.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const WCHAR wcsInputPinName[] = {'i','n','p','u','t',' ','p','i','n',0};
static const WCHAR wcsOutputPinName[] = {'o','u','t','p','u','t',' ','p','i','n',0};

static const IBaseFilterVtbl TransformFilter_Vtbl;
static const IPinVtbl TransformFilter_InputPin_Vtbl;
static const IMemInputPinVtbl MemInputPin_Vtbl; 
static const IPinVtbl TransformFilter_OutputPin_Vtbl;

static HRESULT TransformFilter_Input_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    TransformFilterImpl* This = (TransformFilterImpl *)((IPinImpl *)iface)->pinInfo.pFilter;
    TRACE("%p\n", iface);
    dump_AM_MEDIA_TYPE(pmt);

    if (This->pFuncsTable->pfnQueryConnect)
        return This->pFuncsTable->pfnQueryConnect(This, pmt);
    /* Assume OK if there's no query method (the connection will fail if
       needed) */
    return S_OK;
}


static HRESULT TransformFilter_Output_QueryAccept(LPVOID iface, const AM_MEDIA_TYPE * pmt)
{
    TransformFilterImpl* pTransformFilter = (TransformFilterImpl*)iface;
    AM_MEDIA_TYPE* outpmt = &pTransformFilter->pmt;
    TRACE("%p\n", iface);

    if (IsEqualIID(&pmt->majortype, &outpmt->majortype)
        && (IsEqualIID(&pmt->subtype, &outpmt->subtype) || IsEqualIID(&outpmt->subtype, &GUID_NULL)))
        return S_OK;
    return S_FALSE;
}


static inline TransformFilterImpl *impl_from_IMediaSeeking( IMediaSeeking *iface )
{
    return (TransformFilterImpl *)((char*)iface - FIELD_OFFSET(TransformFilterImpl, mediaSeeking.lpVtbl));
}

static HRESULT WINAPI TransformFilter_Seeking_QueryInterface(IMediaSeeking * iface, REFIID riid, LPVOID * ppv)
{
    TransformFilterImpl *This = impl_from_IMediaSeeking(iface);

    return IUnknown_QueryInterface((IUnknown *)This, riid, ppv);
}

static ULONG WINAPI TransformFilter_Seeking_AddRef(IMediaSeeking * iface)
{
    TransformFilterImpl *This = impl_from_IMediaSeeking(iface);

    return IUnknown_AddRef((IUnknown *)This);
}

static ULONG WINAPI TransformFilter_Seeking_Release(IMediaSeeking * iface)
{
    TransformFilterImpl *This = impl_from_IMediaSeeking(iface);

    return IUnknown_Release((IUnknown *)This);
}

static const IMediaSeekingVtbl TransformFilter_Seeking_Vtbl =
{
    TransformFilter_Seeking_QueryInterface,
    TransformFilter_Seeking_AddRef,
    TransformFilter_Seeking_Release,
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

/* These shouldn't be implemented by default.
 * Usually only source filters should implement these
 * and even it's not needed all of the time
 */
static HRESULT TransformFilter_ChangeCurrent(IBaseFilter *iface)
{
    TRACE("(%p) filter hasn't implemented current position change!\n", iface);
    return S_OK;
}

static HRESULT TransformFilter_ChangeStop(IBaseFilter *iface)
{
    TRACE("(%p) filter hasn't implemented stop position change!\n", iface);
    return S_OK;
}

static HRESULT TransformFilter_ChangeRate(IBaseFilter *iface)
{
    TRACE("(%p) filter hasn't implemented rate change!\n", iface);
    return S_OK;
}

HRESULT TransformFilter_Create(TransformFilterImpl* pTransformFilter, const CLSID* pClsid, const TransformFuncsTable* pFuncsTable, CHANGEPROC stop, CHANGEPROC current, CHANGEPROC rate)
{
    HRESULT hr;
    PIN_INFO piInput;
    PIN_INFO piOutput;

    /* pTransformFilter is already allocated */
    pTransformFilter->clsid = *pClsid;
    pTransformFilter->pFuncsTable = pFuncsTable;

    pTransformFilter->lpVtbl = &TransformFilter_Vtbl;

    pTransformFilter->refCount = 1;
    InitializeCriticalSection(&pTransformFilter->csFilter);
    pTransformFilter->csFilter.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": TransformFilterImpl.csFilter");
    pTransformFilter->state = State_Stopped;
    pTransformFilter->pClock = NULL;
    ZeroMemory(&pTransformFilter->filterInfo, sizeof(FILTER_INFO));
    ZeroMemory(&pTransformFilter->pmt, sizeof(pTransformFilter->pmt));
    pTransformFilter->npins = 2;

    pTransformFilter->ppPins = CoTaskMemAlloc(2 * sizeof(IPin *));

    /* construct input pin */
    piInput.dir = PINDIR_INPUT;
    piInput.pFilter = (IBaseFilter *)pTransformFilter;
    lstrcpynW(piInput.achName, wcsInputPinName, sizeof(piInput.achName) / sizeof(piInput.achName[0]));
    piOutput.dir = PINDIR_OUTPUT;
    piOutput.pFilter = (IBaseFilter *)pTransformFilter;
    lstrcpynW(piOutput.achName, wcsOutputPinName, sizeof(piOutput.achName) / sizeof(piOutput.achName[0]));

    hr = InputPin_Construct(&TransformFilter_InputPin_Vtbl, &piInput, (SAMPLEPROC_PUSH)pFuncsTable->pfnProcessSampleData, NULL, TransformFilter_Input_QueryAccept, NULL, &pTransformFilter->csFilter, NULL, &pTransformFilter->ppPins[0]);

    if (SUCCEEDED(hr))
    {
        ALLOCATOR_PROPERTIES props;
        props.cbAlign = 1;
        props.cbPrefix = 0;
        props.cbBuffer = 0; /* Will be updated at connection time */
        props.cBuffers = 1;

       ((InputPin *)pTransformFilter->ppPins[0])->pin.pUserData = pTransformFilter->ppPins[0];

        hr = OutputPin_Construct(&TransformFilter_OutputPin_Vtbl, sizeof(OutputPin), &piOutput, &props, pTransformFilter, TransformFilter_Output_QueryAccept, &pTransformFilter->csFilter, &pTransformFilter->ppPins[1]);

        if (FAILED(hr))
            ERR("Cannot create output pin (%x)\n", hr);
        else
        {
            if (!stop)
                stop = TransformFilter_ChangeStop;
            if (!current)
                current = TransformFilter_ChangeCurrent;
            if (!rate)
                rate = TransformFilter_ChangeRate;

            MediaSeekingImpl_Init((IBaseFilter*)pTransformFilter, stop, current, rate, &pTransformFilter->mediaSeeking, &pTransformFilter->csFilter);
            pTransformFilter->mediaSeeking.lpVtbl = &TransformFilter_Seeking_Vtbl;
        }
    }
    else
    {
        CoTaskMemFree(pTransformFilter->ppPins);
        pTransformFilter->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&pTransformFilter->csFilter);
        CoTaskMemFree(pTransformFilter);
    }

    return hr;
}

static HRESULT WINAPI TransformFilter_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;
    TRACE("(%p/%p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = (LPVOID)This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = (LPVOID)This;
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

static ULONG WINAPI TransformFilter_AddRef(IBaseFilter * iface)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p/%p)->() AddRef from %d\n", This, iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI TransformFilter_Release(IBaseFilter * iface)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p/%p)->() Release from %d\n", This, iface, refCount + 1);

    if (!refCount)
    {
        ULONG i;

        if (This->pClock)
            IReferenceClock_Release(This->pClock);

        for (i = 0; i < This->npins; i++)
        {
            IPin *pConnectedTo;

            if (SUCCEEDED(IPin_ConnectedTo(This->ppPins[i], &pConnectedTo)))
            {
                IPin_Disconnect(pConnectedTo);
                IPin_Release(pConnectedTo);
            }
            IPin_Disconnect(This->ppPins[i]);

            IPin_Release(This->ppPins[i]);
        }

        CoTaskMemFree(This->ppPins);
        This->lpVtbl = NULL;

        This->csFilter.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->csFilter);

        TRACE("Destroying transform filter\n");
        FreeMediaType(&This->pmt);
        CoTaskMemFree(This);

        return 0;
    }
    else
        return refCount;
}

/** IPersist methods **/

static HRESULT WINAPI TransformFilter_GetClassID(IBaseFilter * iface, CLSID * pClsid)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClsid);

    *pClsid = This->clsid;

    return S_OK;
}

/** IMediaFilter methods **/

static HRESULT WINAPI TransformFilter_Stop(IBaseFilter * iface)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;
    HRESULT hr = S_OK;

    TRACE("(%p/%p)\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    {
        This->state = State_Stopped;
        if (This->pFuncsTable->pfnProcessEnd)
            hr = This->pFuncsTable->pfnProcessEnd(This);
    }
    LeaveCriticalSection(&This->csFilter);

    return hr;
}

static HRESULT WINAPI TransformFilter_Pause(IBaseFilter * iface)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->csFilter);
    {
        if (This->state == State_Stopped)
            hr = IBaseFilter_Run(iface, -1);
        else
            hr = S_OK;

        if (SUCCEEDED(hr))
            This->state = State_Paused;
    }
    LeaveCriticalSection(&This->csFilter);

    return hr;
}

static HRESULT WINAPI TransformFilter_Run(IBaseFilter * iface, REFERENCE_TIME tStart)
{
    HRESULT hr = S_OK;
    TransformFilterImpl *This = (TransformFilterImpl *)iface;

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(tStart));

    EnterCriticalSection(&This->csFilter);
    {
        if (This->state == State_Stopped)
        {
            ((InputPin *)This->ppPins[0])->end_of_stream = 0;
            if (This->pFuncsTable->pfnProcessBegin)
                hr = This->pFuncsTable->pfnProcessBegin(This);
            if (SUCCEEDED(hr))
                hr = OutputPin_CommitAllocator((OutputPin *)This->ppPins[1]);
        }

        if (SUCCEEDED(hr))
        {
            This->rtStreamStart = tStart;
            This->state = State_Running;
        }
    }
    LeaveCriticalSection(&This->csFilter);

    return hr;
}

static HRESULT WINAPI TransformFilter_GetState(IBaseFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;

    TRACE("(%p/%p)->(%d, %p)\n", This, iface, dwMilliSecsTimeout, pState);

    EnterCriticalSection(&This->csFilter);
    {
        *pState = This->state;
    }
    LeaveCriticalSection(&This->csFilter);

    return S_OK;
}

static HRESULT WINAPI TransformFilter_SetSyncSource(IBaseFilter * iface, IReferenceClock *pClock)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;

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

static HRESULT WINAPI TransformFilter_GetSyncSource(IBaseFilter * iface, IReferenceClock **ppClock)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;

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

static HRESULT TransformFilter_GetPin(IBaseFilter *iface, ULONG pos, IPin **pin, DWORD *lastsynctick)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;

    /* Our pins are static, not changing so setting static tick count is ok */
    *lastsynctick = 0;

    if (pos >= This->npins)
        return S_FALSE;

    *pin = This->ppPins[pos];
    IPin_AddRef(*pin);
    return S_OK;
}

static HRESULT WINAPI TransformFilter_EnumPins(IBaseFilter * iface, IEnumPins **ppEnum)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    return IEnumPinsImpl_Construct(ppEnum, TransformFilter_GetPin, iface);
}

static HRESULT WINAPI TransformFilter_FindPin(IBaseFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, debugstr_w(Id), ppPin);

    return E_NOTIMPL;
}

static HRESULT WINAPI TransformFilter_QueryFilterInfo(IBaseFilter * iface, FILTER_INFO *pInfo)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;

    TRACE("(%p/%p)->(%p)\n", This, iface, pInfo);

    strcpyW(pInfo->achName, This->filterInfo.achName);
    pInfo->pGraph = This->filterInfo.pGraph;

    if (pInfo->pGraph)
        IFilterGraph_AddRef(pInfo->pGraph);

    return S_OK;
}

static HRESULT WINAPI TransformFilter_JoinFilterGraph(IBaseFilter * iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    HRESULT hr = S_OK;
    TransformFilterImpl *This = (TransformFilterImpl *)iface;

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

    return hr;
}

static HRESULT WINAPI TransformFilter_QueryVendorInfo(IBaseFilter * iface, LPWSTR *pVendorInfo)
{
    TransformFilterImpl *This = (TransformFilterImpl *)iface;
    TRACE("(%p/%p)->(%p)\n", This, iface, pVendorInfo);
    return E_NOTIMPL;
}

static const IBaseFilterVtbl TransformFilter_Vtbl =
{
    TransformFilter_QueryInterface,
    TransformFilter_AddRef,
    TransformFilter_Release,
    TransformFilter_GetClassID,
    TransformFilter_Stop,
    TransformFilter_Pause,
    TransformFilter_Run,
    TransformFilter_GetState,
    TransformFilter_SetSyncSource,
    TransformFilter_GetSyncSource,
    TransformFilter_EnumPins,
    TransformFilter_FindPin,
    TransformFilter_QueryFilterInfo,
    TransformFilter_JoinFilterGraph,
    TransformFilter_QueryVendorInfo
};

static HRESULT WINAPI TransformFilter_InputPin_EndOfStream(IPin * iface)
{
    InputPin* This = (InputPin*) iface;
    TransformFilterImpl* pTransform;
    IPin* ppin;
    HRESULT hr;
    
    TRACE("(%p)->()\n", iface);

    /* Since we process samples synchronously, just forward notification downstream */
    pTransform = (TransformFilterImpl*)This->pin.pinInfo.pFilter;
    if (!pTransform)
        hr = E_FAIL;
    else
        hr = IPin_ConnectedTo(pTransform->ppPins[1], &ppin);
    if (SUCCEEDED(hr))
    {
        hr = IPin_EndOfStream(ppin);
        IPin_Release(ppin);
    }

    if (FAILED(hr))
        ERR("%x\n", hr);
    return hr;
}

static HRESULT WINAPI TransformFilter_InputPin_ReceiveConnection(IPin * iface, IPin * pReceivePin, const AM_MEDIA_TYPE * pmt)
{
    InputPin* This = (InputPin*) iface;
    TransformFilterImpl* pTransform;
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", iface, pReceivePin, pmt);

    pTransform = (TransformFilterImpl*)This->pin.pinInfo.pFilter;

    hr = pTransform->pFuncsTable->pfnConnectInput(This, pmt);
    if (SUCCEEDED(hr))
    {
        hr = InputPin_ReceiveConnection(iface, pReceivePin, pmt);
        if (FAILED(hr))
            pTransform->pFuncsTable->pfnCleanup(This);
    }

    return hr;
}

static HRESULT WINAPI TransformFilter_InputPin_Disconnect(IPin * iface)
{
    InputPin* This = (InputPin*) iface;
    TransformFilterImpl* pTransform;

    TRACE("(%p)->()\n", iface);

    pTransform = (TransformFilterImpl*)This->pin.pinInfo.pFilter;
    pTransform->pFuncsTable->pfnCleanup(This);

    return IPinImpl_Disconnect(iface);
}

static const IPinVtbl TransformFilter_InputPin_Vtbl = 
{
    InputPin_QueryInterface,
    IPinImpl_AddRef,
    InputPin_Release,
    InputPin_Connect,
    TransformFilter_InputPin_ReceiveConnection,
    TransformFilter_InputPin_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    IPinImpl_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    TransformFilter_InputPin_EndOfStream,
    InputPin_BeginFlush,
    InputPin_EndFlush,
    InputPin_NewSegment
};

static HRESULT WINAPI TransformFilter_Output_EnumMediaTypes(IPin * iface, IEnumMediaTypes ** ppEnum)
{
    IPinImpl *This = (IPinImpl *)iface;
    TransformFilterImpl *pTransform = (TransformFilterImpl *)This->pinInfo.pFilter;
    ENUMMEDIADETAILS emd;

    TRACE("(%p/%p)->(%p)\n", This, iface, ppEnum);

    emd.cMediaTypes = 1;
    emd.pMediaTypes = &pTransform->pmt;

    return IEnumMediaTypesImpl_Construct(&emd, ppEnum);
}

static const IPinVtbl TransformFilter_OutputPin_Vtbl =
{
    OutputPin_QueryInterface,
    IPinImpl_AddRef,
    OutputPin_Release,
    OutputPin_Connect,
    OutputPin_ReceiveConnection,
    OutputPin_Disconnect,
    IPinImpl_ConnectedTo,
    IPinImpl_ConnectionMediaType,
    IPinImpl_QueryPinInfo,
    IPinImpl_QueryDirection,
    IPinImpl_QueryId,
    IPinImpl_QueryAccept,
    TransformFilter_Output_EnumMediaTypes,
    IPinImpl_QueryInternalConnections,
    OutputPin_EndOfStream,
    OutputPin_BeginFlush,
    OutputPin_EndFlush,
    OutputPin_NewSegment
};

static const IMemInputPinVtbl MemInputPin_Vtbl = 
{
    MemInputPin_QueryInterface,
    MemInputPin_AddRef,
    MemInputPin_Release,
    MemInputPin_GetAllocator,
    MemInputPin_NotifyAllocator,
    MemInputPin_GetAllocatorRequirements,
    MemInputPin_Receive,
    MemInputPin_ReceiveMultiple,
    MemInputPin_ReceiveCanBlock
};
