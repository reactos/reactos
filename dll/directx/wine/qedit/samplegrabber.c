/*              DirectShow Sample Grabber object (QEDIT.DLL)
 *
 * Copyright 2009 Paul Chitescu
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

#include <assert.h>
#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"

#include "qedit_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct sample_grabber
{
    struct strmbase_filter filter;
    ISampleGrabber ISampleGrabber_iface;

    struct strmbase_source source;
    struct strmbase_passthrough passthrough;

    struct strmbase_sink sink;
    AM_MEDIA_TYPE filter_mt;
    IMemInputPin IMemInputPin_iface;
    IMemAllocator *allocator;

    ISampleGrabberCB *grabberIface;
    LONG grabberMethod;
    LONG oneShot;
    LONG bufferLen;
    void* bufferData;
};

enum {
    OneShot_None,
    OneShot_Wait,
    OneShot_Past,
};

static struct sample_grabber *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, filter);
}

static struct sample_grabber *impl_from_ISampleGrabber(ISampleGrabber *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, ISampleGrabber_iface);
}

static struct sample_grabber *impl_from_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, IMemInputPin_iface);
}


/* Cleanup at end of life */
static void SampleGrabber_cleanup(struct sample_grabber *This)
{
    TRACE("(%p)\n", This);
    if (This->allocator)
        IMemAllocator_Release(This->allocator);
    if (This->grabberIface)
        ISampleGrabberCB_Release(This->grabberIface);
    FreeMediaType(&This->filter_mt);
    CoTaskMemFree(This->bufferData);
}

static struct strmbase_pin *sample_grabber_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct sample_grabber *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    else if (index == 1)
        return &filter->source.pin;
    return NULL;
}

static void sample_grabber_destroy(struct strmbase_filter *iface)
{
    struct sample_grabber *filter = impl_from_strmbase_filter(iface);

    SampleGrabber_cleanup(filter);
    strmbase_sink_cleanup(&filter->sink);
    strmbase_source_cleanup(&filter->source);
    strmbase_passthrough_cleanup(&filter->passthrough);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static HRESULT sample_grabber_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct sample_grabber *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_ISampleGrabber))
        *out = &filter->ISampleGrabber_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = sample_grabber_get_pin,
    .filter_destroy = sample_grabber_destroy,
    .filter_query_interface = sample_grabber_query_interface,
};

/* Helper that buffers data and/or calls installed sample callbacks */
static void SampleGrabber_callback(struct sample_grabber *This, IMediaSample *sample)
{
    double time = 0.0;
    REFERENCE_TIME tStart, tEnd;
    if (This->bufferLen >= 0) {
        BYTE *data = 0;
        LONG size = IMediaSample_GetActualDataLength(sample);
        if (size >= 0 && SUCCEEDED(IMediaSample_GetPointer(sample, &data))) {
            if (!data)
                size = 0;
            EnterCriticalSection(&This->filter.filter_cs);
            if (This->bufferLen != size) {
                CoTaskMemFree(This->bufferData);
                This->bufferData = size ? CoTaskMemAlloc(size) : NULL;
                This->bufferLen = size;
            }
            if (size)
                CopyMemory(This->bufferData, data, size);
            LeaveCriticalSection(&This->filter.filter_cs);
        }
    }
    if (!This->grabberIface)
        return;
    if (SUCCEEDED(IMediaSample_GetTime(sample, &tStart, &tEnd)))
        time = 1e-7 * tStart;
    switch (This->grabberMethod) {
        case 0:
	    {
		ULONG ref = IMediaSample_AddRef(sample);
		ISampleGrabberCB_SampleCB(This->grabberIface, time, sample);
		ref = IMediaSample_Release(sample) + 1 - ref;
		if (ref)
		{
		    ERR("(%p) Callback referenced sample %p by %lu\n", This, sample, ref);
		}
	    }
            break;
        case 1:
            {
                BYTE *data = 0;
                LONG size = IMediaSample_GetActualDataLength(sample);
                if (size && SUCCEEDED(IMediaSample_GetPointer(sample, &data)) && data)
                    ISampleGrabberCB_BufferCB(This->grabberIface, time, data, size);
            }
            break;
        case -1:
            break;
        default:
            FIXME("Unknown method %ld.\n", This->grabberMethod);
            /* do not bother us again */
            This->grabberMethod = -1;
    }
}

/* IUnknown */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_QueryInterface(ISampleGrabber *iface, REFIID riid, void **ppv)
{
    struct sample_grabber *filter = impl_from_ISampleGrabber(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, riid, ppv);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_ISampleGrabber_AddRef(ISampleGrabber *iface)
{
    struct sample_grabber *filter = impl_from_ISampleGrabber(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_ISampleGrabber_Release(ISampleGrabber *iface)
{
    struct sample_grabber *filter = impl_from_ISampleGrabber(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetOneShot(ISampleGrabber *iface, BOOL oneShot)
{
    struct sample_grabber *This = impl_from_ISampleGrabber(iface);
    TRACE("(%p)->(%u)\n", This, oneShot);
    This->oneShot = oneShot ? OneShot_Wait : OneShot_None;
    return S_OK;
}

/* ISampleGrabber */
static HRESULT WINAPI SampleGrabber_ISampleGrabber_SetMediaType(ISampleGrabber *iface, const AM_MEDIA_TYPE *mt)
{
    struct sample_grabber *filter = impl_from_ISampleGrabber(iface);

    TRACE("filter %p, mt %p.\n", filter, mt);
    strmbase_dump_media_type(mt);

    if (!mt)
        return E_POINTER;

    FreeMediaType(&filter->filter_mt);
    CopyMediaType(&filter->filter_mt, mt);
    return S_OK;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_GetConnectedMediaType(ISampleGrabber *iface, AM_MEDIA_TYPE *mt)
{
    struct sample_grabber *filter = impl_from_ISampleGrabber(iface);

    TRACE("filter %p, mt %p.\n", filter, mt);

    if (!mt)
        return E_POINTER;

    if (!filter->sink.pin.peer)
        return VFW_E_NOT_CONNECTED;

    CopyMediaType(mt, &filter->sink.pin.mt);
    return S_OK;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetBufferSamples(ISampleGrabber *iface, BOOL bufferEm)
{
    struct sample_grabber *This = impl_from_ISampleGrabber(iface);
    TRACE("(%p)->(%u)\n", This, bufferEm);
    EnterCriticalSection(&This->filter.filter_cs);
    if (bufferEm) {
        if (This->bufferLen < 0)
            This->bufferLen = 0;
    }
    else
        This->bufferLen = -1;
    LeaveCriticalSection(&This->filter.filter_cs);
    return S_OK;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_GetCurrentBuffer(ISampleGrabber *iface, LONG *bufSize, LONG *buffer)
{
    struct sample_grabber *This = impl_from_ISampleGrabber(iface);
    HRESULT ret = S_OK;
    TRACE("(%p)->(%p, %p)\n", This, bufSize, buffer);
    if (!bufSize)
        return E_POINTER;
    EnterCriticalSection(&This->filter.filter_cs);
    if (!This->sink.pin.peer)
        ret = VFW_E_NOT_CONNECTED;
    else if (This->bufferLen < 0)
        ret = E_INVALIDARG;
    else if (This->bufferLen == 0)
        ret = VFW_E_WRONG_STATE;
    else {
        if (buffer) {
            if (*bufSize >= This->bufferLen)
                CopyMemory(buffer, This->bufferData, This->bufferLen);
            else
                ret = E_OUTOFMEMORY;
        }
        *bufSize = This->bufferLen;
    }
    LeaveCriticalSection(&This->filter.filter_cs);
    return ret;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_GetCurrentSample(ISampleGrabber *iface, IMediaSample **sample)
{
    /* MS doesn't implement it either, no one should call it */
    WARN("(%p): not implemented\n", sample);
    return E_NOTIMPL;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetCallback(ISampleGrabber *iface, ISampleGrabberCB *cb, LONG whichMethod)
{
    struct sample_grabber *This = impl_from_ISampleGrabber(iface);

    TRACE("filter %p, callback %p, method %ld.\n", This, cb, whichMethod);

    if (This->grabberIface)
        ISampleGrabberCB_Release(This->grabberIface);
    This->grabberIface = cb;
    This->grabberMethod = whichMethod;
    if (cb)
        ISampleGrabberCB_AddRef(cb);
    return S_OK;
}

static HRESULT WINAPI SampleGrabber_IMemInputPin_QueryInterface(IMemInputPin *iface, REFIID iid, void **out)
{
    struct sample_grabber *filter = impl_from_IMemInputPin(iface);
    return IPin_QueryInterface(&filter->sink.pin.IPin_iface, iid, out);
}

static ULONG WINAPI SampleGrabber_IMemInputPin_AddRef(IMemInputPin *iface)
{
    struct sample_grabber *filter = impl_from_IMemInputPin(iface);
    return IPin_AddRef(&filter->sink.pin.IPin_iface);
}

static ULONG WINAPI SampleGrabber_IMemInputPin_Release(IMemInputPin *iface)
{
    struct sample_grabber *filter = impl_from_IMemInputPin(iface);
    return IPin_Release(&filter->sink.pin.IPin_iface);
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_GetAllocator(IMemInputPin *iface, IMemAllocator **allocator)
{
    struct sample_grabber *This = impl_from_IMemInputPin(iface);
    TRACE("(%p)->(%p) allocator = %p\n", This, allocator, This->allocator);
    if (!allocator)
        return E_POINTER;
    *allocator = This->allocator;
    if (!*allocator)
        return VFW_E_NO_ALLOCATOR;
    IMemAllocator_AddRef(*allocator);
    return S_OK;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_NotifyAllocator(IMemInputPin *iface, IMemAllocator *allocator, BOOL readOnly)
{
    struct sample_grabber *This = impl_from_IMemInputPin(iface);
    TRACE("(%p)->(%p, %u) allocator = %p\n", This, allocator, readOnly, This->allocator);
    if (This->allocator == allocator)
        return S_OK;
    if (This->allocator)
        IMemAllocator_Release(This->allocator);
    This->allocator = allocator;
    if (allocator)
        IMemAllocator_AddRef(allocator);
    return S_OK;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_GetAllocatorRequirements(IMemInputPin *iface, ALLOCATOR_PROPERTIES *props)
{
    struct sample_grabber *This = impl_from_IMemInputPin(iface);
    FIXME("(%p)->(%p): semi-stub\n", This, props);
    if (!props)
        return E_POINTER;
    return This->source.pMemInputPin ? IMemInputPin_GetAllocatorRequirements(This->source.pMemInputPin, props) : E_NOTIMPL;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_Receive(IMemInputPin *iface, IMediaSample *sample)
{
    struct sample_grabber *This = impl_from_IMemInputPin(iface);
    HRESULT hr;
    TRACE("(%p)->(%p) output = %p, grabber = %p\n", This, sample, This->source.pMemInputPin, This->grabberIface);
    if (!sample)
        return E_POINTER;
    if (This->oneShot == OneShot_Past)
        return S_FALSE;
    SampleGrabber_callback(This, sample);
    hr = This->source.pMemInputPin ? IMemInputPin_Receive(This->source.pMemInputPin, sample) : S_OK;
    if (This->oneShot == OneShot_Wait) {
        This->oneShot = OneShot_Past;
        hr = S_FALSE;
        if (This->source.pin.peer)
            IPin_EndOfStream(This->source.pin.peer);
    }
    return hr;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_ReceiveMultiple(IMemInputPin *iface, IMediaSample **samples, LONG nSamples, LONG *nProcessed)
{
    struct sample_grabber *This = impl_from_IMemInputPin(iface);
    LONG idx;

    TRACE("filter %p, samples %p, count %lu, ret_count %p.\n", This, samples, nSamples, nProcessed);

    if (!samples || !nProcessed)
        return E_POINTER;
    if ((This->filter.state != State_Running) || (This->oneShot == OneShot_Past))
        return S_FALSE;
    for (idx = 0; idx < nSamples; idx++)
        SampleGrabber_callback(This, samples[idx]);
    return This->source.pMemInputPin ? IMemInputPin_ReceiveMultiple(This->source.pMemInputPin, samples, nSamples, nProcessed) : S_OK;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_ReceiveCanBlock(IMemInputPin *iface)
{
    struct sample_grabber *This = impl_from_IMemInputPin(iface);
    TRACE("(%p)\n", This);
    return This->source.pMemInputPin ? IMemInputPin_ReceiveCanBlock(This->source.pMemInputPin) : S_OK;
}

static const ISampleGrabberVtbl ISampleGrabber_VTable =
{
    SampleGrabber_ISampleGrabber_QueryInterface,
    SampleGrabber_ISampleGrabber_AddRef,
    SampleGrabber_ISampleGrabber_Release,
    SampleGrabber_ISampleGrabber_SetOneShot,
    SampleGrabber_ISampleGrabber_SetMediaType,
    SampleGrabber_ISampleGrabber_GetConnectedMediaType,
    SampleGrabber_ISampleGrabber_SetBufferSamples,
    SampleGrabber_ISampleGrabber_GetCurrentBuffer,
    SampleGrabber_ISampleGrabber_GetCurrentSample,
    SampleGrabber_ISampleGrabber_SetCallback,
};

static const IMemInputPinVtbl IMemInputPin_VTable =
{
    SampleGrabber_IMemInputPin_QueryInterface,
    SampleGrabber_IMemInputPin_AddRef,
    SampleGrabber_IMemInputPin_Release,
    SampleGrabber_IMemInputPin_GetAllocator,
    SampleGrabber_IMemInputPin_NotifyAllocator,
    SampleGrabber_IMemInputPin_GetAllocatorRequirements,
    SampleGrabber_IMemInputPin_Receive,
    SampleGrabber_IMemInputPin_ReceiveMultiple,
    SampleGrabber_IMemInputPin_ReceiveCanBlock,
};

static struct sample_grabber *impl_from_sink_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, sink.pin);
}

static HRESULT sample_grabber_sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct sample_grabber *filter = impl_from_sink_pin(iface);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static BOOL check_filter_mt(struct sample_grabber *filter, const AM_MEDIA_TYPE *mt)
{
    if (IsEqualGUID(&filter->filter_mt.majortype, &GUID_NULL))
        return TRUE;
    if (!IsEqualGUID(&filter->filter_mt.majortype, &mt->majortype))
        return FALSE;

    if (IsEqualGUID(&filter->filter_mt.subtype, &GUID_NULL))
        return TRUE;
    if (!IsEqualGUID(&filter->filter_mt.subtype, &mt->subtype))
        return FALSE;

    if (IsEqualGUID(&filter->filter_mt.formattype, &GUID_NULL))
        return TRUE;
    if (!IsEqualGUID(&filter->filter_mt.formattype, &mt->formattype))
        return FALSE;

    return TRUE;
}

static HRESULT sample_grabber_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct sample_grabber *filter = impl_from_sink_pin(iface);

    return check_filter_mt(filter, mt) ? S_OK : S_FALSE;
}

static HRESULT sample_grabber_sink_get_media_type(struct strmbase_pin *iface,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct sample_grabber *filter = impl_from_sink_pin(iface);
    IEnumMediaTypes *enummt;
    AM_MEDIA_TYPE *pmt;
    HRESULT hr;

    if (!filter->source.pin.peer)
        return VFW_E_NOT_CONNECTED;

    if (FAILED(hr = IPin_EnumMediaTypes(filter->source.pin.peer, &enummt)))
        return hr;

    if ((!index || IEnumMediaTypes_Skip(enummt, index) == S_OK)
            && IEnumMediaTypes_Next(enummt, 1, &pmt, NULL) == S_OK)
    {
        CopyMediaType(mt, pmt);
        DeleteMediaType(pmt);
        IEnumMediaTypes_Release(enummt);
        return S_OK;
    }

    IEnumMediaTypes_Release(enummt);
    return VFW_S_NO_MORE_ITEMS;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = sample_grabber_sink_query_interface,
    .base.pin_query_accept = sample_grabber_sink_query_accept,
    .base.pin_get_media_type = sample_grabber_sink_get_media_type,
};

static struct sample_grabber *impl_from_source_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct sample_grabber, source.pin);
}

static HRESULT sample_grabber_source_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct sample_grabber *filter = impl_from_source_pin(iface);

    if (IsEqualGUID(iid, &IID_IMediaPosition))
        *out = &filter->passthrough.IMediaPosition_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &filter->passthrough.IMediaSeeking_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT sample_grabber_source_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE *mt)
{
    struct sample_grabber *filter = impl_from_source_pin(iface);

    if (filter->sink.pin.peer && IPin_QueryAccept(filter->sink.pin.peer, mt) != S_OK)
        return S_FALSE;

    return check_filter_mt(filter, mt) ? S_OK : S_FALSE;
}

static HRESULT sample_grabber_source_get_media_type(struct strmbase_pin *iface,
        unsigned int index, AM_MEDIA_TYPE *mt)
{
    struct sample_grabber *filter = impl_from_source_pin(iface);
    IEnumMediaTypes *enummt;
    AM_MEDIA_TYPE *pmt;
    HRESULT hr;

    if (!filter->sink.pin.peer)
        return VFW_E_NOT_CONNECTED;

    if (FAILED(hr = IPin_EnumMediaTypes(filter->sink.pin.peer, &enummt)))
        return hr;

    if ((!index || IEnumMediaTypes_Skip(enummt, index) == S_OK)
            && IEnumMediaTypes_Next(enummt, 1, &pmt, NULL) == S_OK)
    {
        CopyMediaType(mt, pmt);
        DeleteMediaType(pmt);
        IEnumMediaTypes_Release(enummt);
        return S_OK;
    }

    IEnumMediaTypes_Release(enummt);
    return VFW_S_NO_MORE_ITEMS;
}

static inline BOOL compare_media_types(const AM_MEDIA_TYPE *a, const AM_MEDIA_TYPE *b)
{
    return !memcmp(a, b, offsetof(AM_MEDIA_TYPE, pbFormat))
            && !memcmp(a->pbFormat, b->pbFormat, a->cbFormat);
}

static HRESULT WINAPI sample_grabber_source_DecideAllocator(struct strmbase_source *iface,
        IMemInputPin *peer, IMemAllocator **allocator)
{
    struct sample_grabber *filter = impl_from_source_pin(&iface->pin);
    const AM_MEDIA_TYPE *mt = &iface->pin.mt;

    if (!compare_media_types(mt, &filter->sink.pin.mt))
    {
        IFilterGraph2 *graph;
        HRESULT hr;

        if (FAILED(hr = IFilterGraph_QueryInterface(filter->filter.graph,
                &IID_IFilterGraph2, (void **)&graph)))
        {
            ERR("Failed to get IFilterGraph2 interface, hr %#lx.\n", hr);
            return hr;
        }

        hr = IFilterGraph2_ReconnectEx(graph, &filter->sink.pin.IPin_iface, mt);
        IFilterGraph2_Release(graph);
        return hr;
    }

    return S_OK;
}

static const struct strmbase_source_ops source_ops =
{
    .base.pin_query_interface = sample_grabber_source_query_interface,
    .base.pin_query_accept = sample_grabber_source_query_accept,
    .base.pin_get_media_type = sample_grabber_source_get_media_type,
    .pfnAttemptConnection = BaseOutputPinImpl_AttemptConnection,
    .pfnDecideAllocator = sample_grabber_source_DecideAllocator,
};

HRESULT sample_grabber_create(IUnknown *outer, IUnknown **out)
{
    struct sample_grabber *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_SampleGrabber, &filter_ops);
    object->ISampleGrabber_iface.lpVtbl = &ISampleGrabber_VTable;
    object->IMemInputPin_iface.lpVtbl = &IMemInputPin_VTable;

    strmbase_sink_init(&object->sink, &object->filter, L"In", &sink_ops, NULL);
    wcscpy(object->sink.pin.name, L"Input");

    strmbase_source_init(&object->source, &object->filter, L"Out", &source_ops);
    wcscpy(object->source.pin.name, L"Output");

    strmbase_passthrough_init(&object->passthrough, (IUnknown *)&object->source.pin.IPin_iface);
    ISeekingPassThru_Init(&object->passthrough.ISeekingPassThru_iface, FALSE,
            &object->sink.pin.IPin_iface);

    object->grabberMethod = -1;
    object->oneShot = OneShot_None;
    object->bufferLen = -1;

    TRACE("Created sample grabber %p.\n", object);
    *out = &object->filter.IUnknown_inner;
    return S_OK;
}
