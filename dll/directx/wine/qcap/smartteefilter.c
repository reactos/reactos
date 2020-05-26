/*
 * Implementation of the SmartTee filter
 *
 * Copyright 2015 Damjan Jovanovic
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

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wtypes.h"
#include "wingdi.h"
#include "winuser.h"
#include "dshow.h"

#include "qcap_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(qcap);

typedef struct {
    IUnknown IUnknown_iface;
    IUnknown *outerUnknown;
    BaseFilter filter;
    BaseInputPin *input;
    BaseOutputPin *capture;
    BaseOutputPin *preview;
} SmartTeeFilter;

static inline SmartTeeFilter *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, SmartTeeFilter, IUnknown_iface);
}

static inline SmartTeeFilter *impl_from_BaseFilter(BaseFilter *filter)
{
    return CONTAINING_RECORD(filter, SmartTeeFilter, filter);
}

static inline SmartTeeFilter *impl_from_IBaseFilter(IBaseFilter *iface)
{
    BaseFilter *filter = CONTAINING_RECORD(iface, BaseFilter, IBaseFilter_iface);
    return impl_from_BaseFilter(filter);
}

static inline SmartTeeFilter *impl_from_BasePin(BasePin *pin)
{
    return impl_from_IBaseFilter(pin->pinInfo.pFilter);
}

static inline SmartTeeFilter *impl_from_IPin(IPin *iface)
{
    BasePin *bp = CONTAINING_RECORD(iface, BasePin, IPin_iface);
    return impl_from_IBaseFilter(bp->pinInfo.pFilter);
}

static HRESULT WINAPI Unknown_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    SmartTeeFilter *This = impl_from_IUnknown(iface);
    if (IsEqualIID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown, %p)\n", This, ppv);
        *ppv = &This->IUnknown_iface;
    } else if (IsEqualIID(riid, &IID_IPersist)) {
        TRACE("(%p)->(IID_IPersist, %p)\n", This, ppv);
        *ppv = &This->filter.IBaseFilter_iface;
    } else if (IsEqualIID(riid, &IID_IMediaFilter)) {
        TRACE("(%p)->(IID_IMediaFilter, %p)\n", This, ppv);
        *ppv = &This->filter.IBaseFilter_iface;
    } else if (IsEqualIID(riid, &IID_IBaseFilter)) {
        TRACE("(%p)->(IID_IBaseFilter, %p)\n", This, ppv);
        *ppv = &This->filter.IBaseFilter_iface;
    } else {
        FIXME("(%p): no interface for %s\n", This, debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Unknown_AddRef(IUnknown *iface)
{
    SmartTeeFilter *This = impl_from_IUnknown(iface);
    return BaseFilterImpl_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI Unknown_Release(IUnknown *iface)
{
    SmartTeeFilter *This = impl_from_IUnknown(iface);
    ULONG ref = BaseFilterImpl_Release(&This->filter.IBaseFilter_iface);

    TRACE("(%p)->() ref=%d\n", This, ref);

    if (!ref) {
        if(This->input)
            BaseInputPinImpl_Release(&This->input->pin.IPin_iface);
        if(This->capture)
            BaseOutputPinImpl_Release(&This->capture->pin.IPin_iface);
        if(This->preview)
            BaseOutputPinImpl_Release(&This->preview->pin.IPin_iface);
        CoTaskMemFree(This);
    }
    return ref;
}

static const IUnknownVtbl UnknownVtbl = {
    Unknown_QueryInterface,
    Unknown_AddRef,
    Unknown_Release
};

static HRESULT WINAPI SmartTeeFilter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    SmartTeeFilter *This = impl_from_IBaseFilter(iface);
    return IUnknown_QueryInterface(This->outerUnknown, riid, ppv);
}

static ULONG WINAPI SmartTeeFilter_AddRef(IBaseFilter *iface)
{
    SmartTeeFilter *This = impl_from_IBaseFilter(iface);
    return IUnknown_AddRef(This->outerUnknown);
}

static ULONG WINAPI SmartTeeFilter_Release(IBaseFilter *iface)
{
    SmartTeeFilter *This = impl_from_IBaseFilter(iface);
    return IUnknown_Release(This->outerUnknown);
}

static HRESULT WINAPI SmartTeeFilter_Stop(IBaseFilter *iface)
{
    SmartTeeFilter *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)\n", This);
    EnterCriticalSection(&This->filter.csFilter);
    This->filter.state = State_Stopped;
    LeaveCriticalSection(&This->filter.csFilter);
    return S_OK;
}

static HRESULT WINAPI SmartTeeFilter_Pause(IBaseFilter *iface)
{
    SmartTeeFilter *This = impl_from_IBaseFilter(iface);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI SmartTeeFilter_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    SmartTeeFilter *This = impl_from_IBaseFilter(iface);
    HRESULT hr = S_OK;
    TRACE("(%p, %s)\n", This, wine_dbgstr_longlong(tStart));
    EnterCriticalSection(&This->filter.csFilter);
    if(This->filter.state != State_Running) {
        /* We share an allocator among all pins, an allocator can only get committed
         * once, state transitions occur in upstream order, and only output pins
         * commit allocators, so let the filter attached to the input pin worry about it. */
        if (This->input->pin.pConnectedTo)
            This->filter.state = State_Running;
        else
            hr = VFW_E_NOT_CONNECTED;
    }
    LeaveCriticalSection(&This->filter.csFilter);
    return hr;
}

static HRESULT WINAPI SmartTeeFilter_FindPin(IBaseFilter *iface, LPCWSTR Id, IPin **ppPin)
{
    SmartTeeFilter *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_w(Id), ppPin);
    if (lstrcmpW(Id, This->input->pin.pinInfo.achName) == 0) {
        *ppPin = &This->input->pin.IPin_iface;
        IPin_AddRef(*ppPin);
        return S_OK;
    } else if (lstrcmpW(Id, This->capture->pin.pinInfo.achName) == 0) {
        *ppPin = &This->capture->pin.IPin_iface;
        IPin_AddRef(*ppPin);
        return S_OK;
    } else if (lstrcmpW(Id, This->preview->pin.pinInfo.achName) == 0) {
        *ppPin = &This->preview->pin.IPin_iface;
        IPin_AddRef(*ppPin);
        return S_OK;
    }
    return VFW_E_NOT_FOUND;
}

static const IBaseFilterVtbl SmartTeeFilterVtbl = {
    SmartTeeFilter_QueryInterface,
    SmartTeeFilter_AddRef,
    SmartTeeFilter_Release,
    BaseFilterImpl_GetClassID,
    SmartTeeFilter_Stop,
    SmartTeeFilter_Pause,
    SmartTeeFilter_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    SmartTeeFilter_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

static IPin* WINAPI SmartTeeFilter_GetPin(BaseFilter *iface, int pos)
{
    SmartTeeFilter *This = impl_from_BaseFilter(iface);
    IPin *ret;

    TRACE("(%p)->(%d)\n", This, pos);

    switch(pos) {
    case 0:
        ret = &This->input->pin.IPin_iface;
        break;
    case 1:
        ret = &This->capture->pin.IPin_iface;
        break;
    case 2:
        ret = &This->preview->pin.IPin_iface;
        break;
    default:
        TRACE("No pin %d\n", pos);
        return NULL;
    }

    IPin_AddRef(ret);
    return ret;
}

static LONG WINAPI SmartTeeFilter_GetPinCount(BaseFilter *iface)
{
    return 3;
}
static const BaseFilterFuncTable SmartTeeFilterFuncs = {
    SmartTeeFilter_GetPin,
    SmartTeeFilter_GetPinCount
};

static ULONG WINAPI SmartTeeFilterInput_AddRef(IPin *iface)
{
    SmartTeeFilter *This = impl_from_IPin(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI SmartTeeFilterInput_Release(IPin *iface)
{
    SmartTeeFilter *This = impl_from_IPin(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}


static const IPinVtbl SmartTeeFilterInputVtbl = {
    BaseInputPinImpl_QueryInterface,
    SmartTeeFilterInput_AddRef,
    SmartTeeFilterInput_Release,
    BaseInputPinImpl_Connect,
    BaseInputPinImpl_ReceiveConnection,
    BasePinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    BasePinImpl_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseInputPinImpl_EndOfStream,
    BaseInputPinImpl_BeginFlush,
    BaseInputPinImpl_EndFlush,
    BaseInputPinImpl_NewSegment
};

static HRESULT WINAPI SmartTeeFilterInput_CheckMediaType(BasePin *base, const AM_MEDIA_TYPE *pmt)
{
    SmartTeeFilter *This = impl_from_BasePin(base);
    TRACE("(%p, AM_MEDIA_TYPE(%p))\n", This, pmt);
    dump_AM_MEDIA_TYPE(pmt);
    if (!pmt)
        return VFW_E_TYPE_NOT_ACCEPTED;
    /* We'll take any media type, but the output pins will later
     * struggle to connect downstream. */
    return S_OK;
}

static LONG WINAPI SmartTeeFilterInput_GetMediaTypeVersion(BasePin *base)
{
    return 0;
}

static HRESULT WINAPI SmartTeeFilterInput_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    SmartTeeFilter *This = impl_from_BasePin(base);
    HRESULT hr;
    TRACE("(%p)->(%d, %p)\n", This, iPosition, amt);
    if (iPosition)
        return S_FALSE;
    EnterCriticalSection(&This->filter.csFilter);
    if (This->input->pin.pConnectedTo) {
        CopyMediaType(amt, &This->input->pin.mtCurrent);
        hr = S_OK;
    } else
        hr = S_FALSE;
    LeaveCriticalSection(&This->filter.csFilter);
    return hr;
}

static HRESULT copy_sample(IMediaSample *inputSample, IMemAllocator *allocator, IMediaSample **pOutputSample)
{
    REFERENCE_TIME startTime, endTime;
    BOOL haveStartTime = TRUE, haveEndTime = TRUE;
    IMediaSample *outputSample = NULL;
    BYTE *ptrIn, *ptrOut;
    AM_MEDIA_TYPE *mediaType = NULL;
    HRESULT hr;

    hr = IMediaSample_GetTime(inputSample, &startTime, &endTime);
    if (hr == S_OK)
        ;
    else if (hr == VFW_S_NO_STOP_TIME)
        haveEndTime = FALSE;
    else if (hr == VFW_E_SAMPLE_TIME_NOT_SET)
        haveStartTime = haveEndTime = FALSE;
    else
        goto end;

    hr = IMemAllocator_GetBuffer(allocator, &outputSample,
            haveStartTime ? &startTime : NULL, haveEndTime ? &endTime : NULL, 0);
    if (FAILED(hr)) goto end;
    if (IMediaSample_GetSize(outputSample) < IMediaSample_GetActualDataLength(inputSample)) {
        ERR("insufficient space in sample\n");
        hr = VFW_E_BUFFER_OVERFLOW;
        goto end;
    }

    hr = IMediaSample_SetTime(outputSample, haveStartTime ? &startTime : NULL, haveEndTime ? &endTime : NULL);
    if (FAILED(hr)) goto end;

    hr = IMediaSample_GetPointer(inputSample, &ptrIn);
    if (FAILED(hr)) goto end;
    hr = IMediaSample_GetPointer(outputSample, &ptrOut);
    if (FAILED(hr)) goto end;
    memcpy(ptrOut, ptrIn, IMediaSample_GetActualDataLength(inputSample));
    IMediaSample_SetActualDataLength(outputSample, IMediaSample_GetActualDataLength(inputSample));

    hr = IMediaSample_SetDiscontinuity(outputSample, IMediaSample_IsDiscontinuity(inputSample) == S_OK);
    if (FAILED(hr)) goto end;

    haveStartTime = haveEndTime = TRUE;
    hr = IMediaSample_GetMediaTime(inputSample, &startTime, &endTime);
    if (hr == S_OK)
        ;
    else if (hr == VFW_S_NO_STOP_TIME)
        haveEndTime = FALSE;
    else if (hr == VFW_E_MEDIA_TIME_NOT_SET)
        haveStartTime = haveEndTime = FALSE;
    else
        goto end;
    hr = IMediaSample_SetMediaTime(outputSample, haveStartTime ? &startTime : NULL, haveEndTime ? &endTime : NULL);
    if (FAILED(hr)) goto end;

    hr = IMediaSample_GetMediaType(inputSample, &mediaType);
    if (FAILED(hr)) goto end;
    if (hr == S_OK) {
        hr = IMediaSample_SetMediaType(outputSample, mediaType);
        if (FAILED(hr)) goto end;
    }

    hr = IMediaSample_SetPreroll(outputSample, IMediaSample_IsPreroll(inputSample) == S_OK);
    if (FAILED(hr)) goto end;

    hr = IMediaSample_SetSyncPoint(outputSample, IMediaSample_IsSyncPoint(inputSample) == S_OK);
    if (FAILED(hr)) goto end;

end:
    if (mediaType)
        DeleteMediaType(mediaType);
    if (FAILED(hr) && outputSample) {
        IMediaSample_Release(outputSample);
        outputSample = NULL;
    }
    *pOutputSample = outputSample;
    return hr;
}

static HRESULT WINAPI SmartTeeFilterInput_Receive(BaseInputPin *base, IMediaSample *inputSample)
{
    SmartTeeFilter *This = impl_from_BasePin(&base->pin);
    IMediaSample *captureSample = NULL;
    IMediaSample *previewSample = NULL;
    HRESULT hrCapture = VFW_E_NOT_CONNECTED, hrPreview = VFW_E_NOT_CONNECTED;

    TRACE("(%p)->(%p)\n", This, inputSample);

    /* Modifying the image coming out of one pin doesn't modify the image
     * coming out of the other. MSDN claims the filter doesn't copy,
     * but unless it somehow uses copy-on-write, I just don't see how
     * that's possible. */

    /* FIXME: we should ideally do each of these in a separate thread */
    EnterCriticalSection(&This->filter.csFilter);
    if (This->capture->pin.pConnectedTo)
        hrCapture = copy_sample(inputSample, This->capture->pAllocator, &captureSample);
    LeaveCriticalSection(&This->filter.csFilter);
    if (SUCCEEDED(hrCapture))
        hrCapture = BaseOutputPinImpl_Deliver(This->capture, captureSample);
    if (captureSample)
        IMediaSample_Release(captureSample);

    EnterCriticalSection(&This->filter.csFilter);
    if (This->preview->pin.pConnectedTo)
        hrPreview = copy_sample(inputSample, This->preview->pAllocator, &previewSample);
    LeaveCriticalSection(&This->filter.csFilter);
    /* No timestamps on preview stream: */
    if (SUCCEEDED(hrPreview))
        hrPreview = IMediaSample_SetTime(previewSample, NULL, NULL);
    if (SUCCEEDED(hrPreview))
        hrPreview = BaseOutputPinImpl_Deliver(This->preview, previewSample);
    if (previewSample)
        IMediaSample_Release(previewSample);

    /* FIXME: how to merge the HRESULTs from the 2 pins? */
    if (SUCCEEDED(hrCapture))
        return hrCapture;
    else
        return hrPreview;
}

static const BaseInputPinFuncTable SmartTeeFilterInputFuncs = {
    {
        SmartTeeFilterInput_CheckMediaType,
        NULL,
        SmartTeeFilterInput_GetMediaTypeVersion,
        SmartTeeFilterInput_GetMediaType
    },
    SmartTeeFilterInput_Receive
};

static ULONG WINAPI SmartTeeFilterCapture_AddRef(IPin *iface)
{
    SmartTeeFilter *This = impl_from_IPin(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI SmartTeeFilterCapture_Release(IPin *iface)
{
    SmartTeeFilter *This = impl_from_IPin(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI SmartTeeFilterCapture_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    SmartTeeFilter *This = impl_from_IPin(iface);
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, ppEnum);
    EnterCriticalSection(&This->filter.csFilter);
    if (This->input->pin.pConnectedTo) {
        hr = BasePinImpl_EnumMediaTypes(iface, ppEnum);
    } else
        hr = VFW_E_NOT_CONNECTED;
    LeaveCriticalSection(&This->filter.csFilter);
    return hr;
}

static const IPinVtbl SmartTeeFilterCaptureVtbl = {
    BaseOutputPinImpl_QueryInterface,
    SmartTeeFilterCapture_AddRef,
    SmartTeeFilterCapture_Release,
    BaseOutputPinImpl_Connect,
    BaseOutputPinImpl_ReceiveConnection,
    BaseOutputPinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    SmartTeeFilterCapture_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseOutputPinImpl_EndOfStream,
    BaseOutputPinImpl_BeginFlush,
    BaseOutputPinImpl_EndFlush,
    BasePinImpl_NewSegment
};

static LONG WINAPI SmartTeeFilterCapture_GetMediaTypeVersion(BasePin *base)
{
    SmartTeeFilter *This = impl_from_BasePin(base);
    TRACE("(%p)\n", This);
    return 0;
}

static HRESULT WINAPI SmartTeeFilterCapture_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    SmartTeeFilter *This = impl_from_BasePin(base);
    TRACE("(%p, %d, %p)\n", This, iPosition, amt);
    if (iPosition == 0) {
        CopyMediaType(amt, &This->input->pin.mtCurrent);
        return S_OK;
    } else
        return S_FALSE;
}

static HRESULT WINAPI SmartTeeFilterCapture_DecideAllocator(BaseOutputPin *base, IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    SmartTeeFilter *This = impl_from_BasePin(&base->pin);
    TRACE("(%p, %p, %p)\n", This, pPin, pAlloc);
    *pAlloc = This->input->pAllocator;
    IMemAllocator_AddRef(This->input->pAllocator);
    return IMemInputPin_NotifyAllocator(pPin, This->input->pAllocator, TRUE);
}

static HRESULT WINAPI SmartTeeFilterCapture_BreakConnect(BaseOutputPin *base)
{
    SmartTeeFilter *This = impl_from_BasePin(&base->pin);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static const BaseOutputPinFuncTable SmartTeeFilterCaptureFuncs = {
    {
        NULL,
        BaseOutputPinImpl_AttemptConnection,
        SmartTeeFilterCapture_GetMediaTypeVersion,
        SmartTeeFilterCapture_GetMediaType
    },
    NULL,
    SmartTeeFilterCapture_DecideAllocator,
    SmartTeeFilterCapture_BreakConnect
};

static ULONG WINAPI SmartTeeFilterPreview_AddRef(IPin *iface)
{
    SmartTeeFilter *This = impl_from_IPin(iface);
    return IBaseFilter_AddRef(&This->filter.IBaseFilter_iface);
}

static ULONG WINAPI SmartTeeFilterPreview_Release(IPin *iface)
{
    SmartTeeFilter *This = impl_from_IPin(iface);
    return IBaseFilter_Release(&This->filter.IBaseFilter_iface);
}

static HRESULT WINAPI SmartTeeFilterPreview_EnumMediaTypes(IPin *iface, IEnumMediaTypes **ppEnum)
{
    SmartTeeFilter *This = impl_from_IPin(iface);
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, ppEnum);
    EnterCriticalSection(&This->filter.csFilter);
    if (This->input->pin.pConnectedTo) {
        hr = BasePinImpl_EnumMediaTypes(iface, ppEnum);
    } else
        hr = VFW_E_NOT_CONNECTED;
    LeaveCriticalSection(&This->filter.csFilter);
    return hr;
}

static const IPinVtbl SmartTeeFilterPreviewVtbl = {
    BaseOutputPinImpl_QueryInterface,
    SmartTeeFilterPreview_AddRef,
    SmartTeeFilterPreview_Release,
    BaseOutputPinImpl_Connect,
    BaseOutputPinImpl_ReceiveConnection,
    BaseOutputPinImpl_Disconnect,
    BasePinImpl_ConnectedTo,
    BasePinImpl_ConnectionMediaType,
    BasePinImpl_QueryPinInfo,
    BasePinImpl_QueryDirection,
    BasePinImpl_QueryId,
    BasePinImpl_QueryAccept,
    SmartTeeFilterPreview_EnumMediaTypes,
    BasePinImpl_QueryInternalConnections,
    BaseOutputPinImpl_EndOfStream,
    BaseOutputPinImpl_BeginFlush,
    BaseOutputPinImpl_EndFlush,
    BasePinImpl_NewSegment
};

static LONG WINAPI SmartTeeFilterPreview_GetMediaTypeVersion(BasePin *base)
{
    SmartTeeFilter *This = impl_from_BasePin(base);
    TRACE("(%p)\n", This);
    return 0;
}

static HRESULT WINAPI SmartTeeFilterPreview_GetMediaType(BasePin *base, int iPosition, AM_MEDIA_TYPE *amt)
{
    SmartTeeFilter *This = impl_from_BasePin(base);
    TRACE("(%p, %d, %p)\n", This, iPosition, amt);
    if (iPosition == 0) {
        CopyMediaType(amt, &This->input->pin.mtCurrent);
        return S_OK;
    } else
        return S_FALSE;
}

static HRESULT WINAPI SmartTeeFilterPreview_DecideAllocator(BaseOutputPin *base, IMemInputPin *pPin, IMemAllocator **pAlloc)
{
    SmartTeeFilter *This = impl_from_BasePin(&base->pin);
    TRACE("(%p, %p, %p)\n", This, pPin, pAlloc);
    *pAlloc = This->input->pAllocator;
    IMemAllocator_AddRef(This->input->pAllocator);
    return IMemInputPin_NotifyAllocator(pPin, This->input->pAllocator, TRUE);
}

static HRESULT WINAPI SmartTeeFilterPreview_BreakConnect(BaseOutputPin *base)
{
    SmartTeeFilter *This = impl_from_BasePin(&base->pin);
    FIXME("(%p): stub\n", This);
    return E_NOTIMPL;
}

static const BaseOutputPinFuncTable SmartTeeFilterPreviewFuncs = {
    {
        NULL,
        BaseOutputPinImpl_AttemptConnection,
        SmartTeeFilterPreview_GetMediaTypeVersion,
        SmartTeeFilterPreview_GetMediaType
    },
    NULL,
    SmartTeeFilterPreview_DecideAllocator,
    SmartTeeFilterPreview_BreakConnect
};
IUnknown* WINAPI QCAP_createSmartTeeFilter(IUnknown *outer, HRESULT *phr)
{
    PIN_INFO inputPinInfo  = {NULL, PINDIR_INPUT,  {'I','n','p','u','t',0}};
    PIN_INFO capturePinInfo = {NULL, PINDIR_OUTPUT, {'C','a','p','t','u','r','e',0}};
    PIN_INFO previewPinInfo = {NULL, PINDIR_OUTPUT, {'P','r','e','v','i','e','w',0}};
    HRESULT hr;
    SmartTeeFilter *This = NULL;

    TRACE("(%p, %p)\n", outer, phr);

    This = CoTaskMemAlloc(sizeof(*This));
    if (This == NULL) {
        hr = E_OUTOFMEMORY;
        goto end;
    }
    memset(This, 0, sizeof(*This));
    This->IUnknown_iface.lpVtbl = &UnknownVtbl;
    if (outer)
        This->outerUnknown = outer;
    else
        This->outerUnknown = &This->IUnknown_iface;

    BaseFilter_Init(&This->filter, &SmartTeeFilterVtbl, &CLSID_SmartTee,
            (DWORD_PTR)(__FILE__ ": SmartTeeFilter.csFilter"), &SmartTeeFilterFuncs);

    inputPinInfo.pFilter = &This->filter.IBaseFilter_iface;
    hr = BaseInputPin_Construct(&SmartTeeFilterInputVtbl, sizeof(BaseInputPin), &inputPinInfo,
            &SmartTeeFilterInputFuncs, &This->filter.csFilter, NULL, (IPin**)&This->input);
    if (FAILED(hr))
        goto end;
    hr = CoCreateInstance(&CLSID_MemoryAllocator, NULL, CLSCTX_INPROC_SERVER,
            &IID_IMemAllocator, (void**)&This->input->pAllocator);
    if (FAILED(hr))
        goto end;

    capturePinInfo.pFilter = &This->filter.IBaseFilter_iface;
    hr = BaseOutputPin_Construct(&SmartTeeFilterCaptureVtbl, sizeof(BaseOutputPin), &capturePinInfo,
            &SmartTeeFilterCaptureFuncs, &This->filter.csFilter, (IPin**)&This->capture);
    if (FAILED(hr))
        goto end;

    previewPinInfo.pFilter = &This->filter.IBaseFilter_iface;
    hr = BaseOutputPin_Construct(&SmartTeeFilterPreviewVtbl, sizeof(BaseOutputPin), &previewPinInfo,
            &SmartTeeFilterPreviewFuncs, &This->filter.csFilter, (IPin**)&This->preview);

end:
    *phr = hr;
    if (SUCCEEDED(hr)) {
        if (outer)
            return &This->IUnknown_iface;
        else
            return (IUnknown*)&This->filter.IBaseFilter_iface;
    } else {
        if (This)
            IBaseFilter_Release(&This->filter.IBaseFilter_iface);
        return NULL;
    }
}
