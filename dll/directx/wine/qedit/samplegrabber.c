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

#include "qedit_private.h"

#include <wine/strmbase.h>

static const WCHAR vendor_name[] = { 'W', 'i', 'n', 'e', 0 };
static const WCHAR pin_in_name[] = { 'I', 'n', 0 };
static const WCHAR pin_out_name[] = { 'O', 'u', 't', 0 };

static IEnumMediaTypes *mediaenum_create(const AM_MEDIA_TYPE *mtype, BOOL past);

/* Single media type enumerator */
typedef struct _ME_Impl {
    IEnumMediaTypes IEnumMediaTypes_iface;
    LONG refCount;
    BOOL past;
    AM_MEDIA_TYPE mtype;
} ME_Impl;


/* IEnumMediaTypes interface implementation */

static inline ME_Impl *impl_from_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, ME_Impl, IEnumMediaTypes_iface);
}

static HRESULT WINAPI Single_IEnumMediaTypes_QueryInterface(IEnumMediaTypes *iface, REFIID riid,
        void **ret_iface)
{
    ME_Impl *This = impl_from_IEnumMediaTypes(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ret_iface);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IEnumMediaTypes)) {
        *ret_iface = iface;
        IEnumMediaTypes_AddRef(iface);
        return S_OK;
    }
    *ret_iface = NULL;
    WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI Single_IEnumMediaTypes_AddRef(IEnumMediaTypes *iface)
{
    ME_Impl *This = impl_from_IEnumMediaTypes(iface);
    ULONG refCount = InterlockedIncrement(&This->refCount);

    TRACE("(%p) new ref = %u\n", This, refCount);
    return refCount;
}

static ULONG WINAPI Single_IEnumMediaTypes_Release(IEnumMediaTypes *iface)
{
    ME_Impl *This = impl_from_IEnumMediaTypes(iface);
    ULONG refCount = InterlockedDecrement(&This->refCount);

    TRACE("(%p) new ref = %u\n", This, refCount);
    if (refCount == 0)
    {
        if (This->mtype.pbFormat)
            CoTaskMemFree(This->mtype.pbFormat);
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

/* IEnumMediaTypes */
static HRESULT WINAPI Single_IEnumMediaTypes_Next(IEnumMediaTypes *iface, ULONG nTypes,
        AM_MEDIA_TYPE **types, ULONG *fetched)
{
    ME_Impl *This = impl_from_IEnumMediaTypes(iface);
    ULONG count = 0;

    TRACE("(%p)->(%u, %p, %p)\n", This, nTypes, types, fetched);
    if (!nTypes)
        return E_INVALIDARG;
    if (!types || ((nTypes != 1) && !fetched))
        return E_POINTER;
    if (!This->past && !IsEqualGUID(&This->mtype.majortype,&GUID_NULL)) {
        AM_MEDIA_TYPE *mtype = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        *mtype = This->mtype;
        if (mtype->cbFormat) {
            mtype->pbFormat = CoTaskMemAlloc(mtype->cbFormat);
            CopyMemory(mtype->pbFormat, This->mtype.pbFormat, mtype->cbFormat);
        }
        *types = mtype;
        This->past = TRUE;
        count = 1;
    }
    if (fetched)
        *fetched = count;
    return (count == nTypes) ? S_OK : S_FALSE;
}

static HRESULT WINAPI Single_IEnumMediaTypes_Skip(IEnumMediaTypes *iface, ULONG nTypes)
{
    ME_Impl *This = impl_from_IEnumMediaTypes(iface);

    TRACE("(%p)->(%u)\n", This, nTypes);
    if (nTypes)
        This->past = TRUE;
    return This->past ? S_FALSE : S_OK;
}

static HRESULT WINAPI Single_IEnumMediaTypes_Reset(IEnumMediaTypes *iface)
{
    ME_Impl *This = impl_from_IEnumMediaTypes(iface);

    TRACE("(%p)->()\n", This);
    This->past = FALSE;
    return S_OK;
}

static HRESULT WINAPI Single_IEnumMediaTypes_Clone(IEnumMediaTypes *iface, IEnumMediaTypes **me)
{
    ME_Impl *This = impl_from_IEnumMediaTypes(iface);

    TRACE("(%p)->(%p)\n", This, me);
    if (!me)
        return E_POINTER;
    *me = mediaenum_create(&This->mtype, This->past);
    if (!*me)
        return E_OUTOFMEMORY;
    return S_OK;
}


/* Virtual tables and constructor */

static const IEnumMediaTypesVtbl IEnumMediaTypes_VTable =
{
    Single_IEnumMediaTypes_QueryInterface,
    Single_IEnumMediaTypes_AddRef,
    Single_IEnumMediaTypes_Release,
    Single_IEnumMediaTypes_Next,
    Single_IEnumMediaTypes_Skip,
    Single_IEnumMediaTypes_Reset,
    Single_IEnumMediaTypes_Clone,
};

static IEnumMediaTypes *mediaenum_create(const AM_MEDIA_TYPE *mtype, BOOL past)
{
    ME_Impl *obj = CoTaskMemAlloc(sizeof(ME_Impl));

    if (!obj)
        return NULL;
    ZeroMemory(obj, sizeof(*obj));
    obj->IEnumMediaTypes_iface.lpVtbl = &IEnumMediaTypes_VTable;
    obj->refCount = 1;
    obj->past = past;
    if (mtype) {
        obj->mtype = *mtype;
        obj->mtype.pUnk = NULL;
        if (mtype->cbFormat) {
            obj->mtype.pbFormat = CoTaskMemAlloc(mtype->cbFormat);
            CopyMemory(obj->mtype.pbFormat, mtype->pbFormat, mtype->cbFormat);
        }
        else
            obj->mtype.pbFormat = NULL;
    }
    else
        obj->mtype.majortype = GUID_NULL;

    return &obj->IEnumMediaTypes_iface;
}


/* Sample Grabber pin implementation */
typedef struct _SG_Pin {
    IPin IPin_iface;
    PIN_DIRECTION dir;
    WCHAR const *name;
    struct _SG_Impl *sg;
    IPin *pair;
} SG_Pin;

static inline SG_Pin *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, SG_Pin, IPin_iface);
}

/* Sample Grabber filter implementation */
typedef struct _SG_Impl {
    IUnknown IUnknown_inner;
    BaseFilter filter;
    ISampleGrabber ISampleGrabber_iface;
    /* IMediaSeeking and IMediaPosition are implemented by ISeekingPassThru */
    IUnknown* seekthru_unk;
    IUnknown *outer_unk;
    AM_MEDIA_TYPE mtype;
    SG_Pin pin_in;
    SG_Pin pin_out;
    IMemInputPin IMemInputPin_iface;
    IMemAllocator *allocator;
    IMemInputPin *memOutput;
    ISampleGrabberCB *grabberIface;
    LONG grabberMethod;
    LONG oneShot;
    LONG bufferLen;
    void* bufferData;
} SG_Impl;

enum {
    OneShot_None,
    OneShot_Wait,
    OneShot_Past,
};

static inline SG_Impl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, SG_Impl, IUnknown_inner);
}

static inline SG_Impl *impl_from_BaseFilter(BaseFilter *iface)
{
    return CONTAINING_RECORD(iface, SG_Impl, filter);
}

static inline SG_Impl *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, SG_Impl, filter.IBaseFilter_iface);
}

static inline SG_Impl *impl_from_ISampleGrabber(ISampleGrabber *iface)
{
    return CONTAINING_RECORD(iface, SG_Impl, ISampleGrabber_iface);
}

static inline SG_Impl *impl_from_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, SG_Impl, IMemInputPin_iface);
}


/* Cleanup at end of life */
static void SampleGrabber_cleanup(SG_Impl *This)
{
    TRACE("(%p)\n", This);
    if (This->filter.filterInfo.pGraph)
        WARN("(%p) still joined to filter graph %p\n", This, This->filter.filterInfo.pGraph);
    if (This->allocator)
        IMemAllocator_Release(This->allocator);
    if (This->memOutput)
        IMemInputPin_Release(This->memOutput);
    if (This->grabberIface)
        ISampleGrabberCB_Release(This->grabberIface);
    if (This->mtype.pbFormat)
        CoTaskMemFree(This->mtype.pbFormat);
    if (This->bufferData)
        CoTaskMemFree(This->bufferData);
    if(This->seekthru_unk)
        IUnknown_Release(This->seekthru_unk);
}

/* SampleGrabber inner IUnknown */
static HRESULT WINAPI SampleGrabber_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    SG_Impl *This = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IUnknown_inner;
    else if (IsEqualIID(riid, &IID_IPersist) || IsEqualIID(riid, &IID_IMediaFilter) ||
        IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = &This->filter.IBaseFilter_iface;
    else if (IsEqualIID(riid, &IID_ISampleGrabber))
        *ppv = &This->ISampleGrabber_iface;
    else if (IsEqualIID(riid, &IID_IMediaPosition))
        return IUnknown_QueryInterface(This->seekthru_unk, riid, ppv);
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        return IUnknown_QueryInterface(This->seekthru_unk, riid, ppv);
    else
        WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppv);

    if (!*ppv)
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SampleGrabber_AddRef(IUnknown *iface)
{
    SG_Impl *This = impl_from_IUnknown(iface);
    ULONG ref = BaseFilterImpl_AddRef(&This->filter.IBaseFilter_iface);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI SampleGrabber_Release(IUnknown *iface)
{
    SG_Impl *This = impl_from_IUnknown(iface);
    ULONG ref = BaseFilterImpl_Release(&This->filter.IBaseFilter_iface);

    TRACE("(%p) ref=%d\n", This, ref);

    if (ref == 0)
    {
        SampleGrabber_cleanup(This);
        CoTaskMemFree(This);
        return 0;
    }
    return ref;
}

static const IUnknownVtbl samplegrabber_vtbl =
{
    SampleGrabber_QueryInterface,
    SampleGrabber_AddRef,
    SampleGrabber_Release,
};

static IPin *WINAPI SampleGrabber_GetPin(BaseFilter *iface, int pos)
{
    SG_Impl *This = impl_from_BaseFilter(iface);
    IPin *pin;

    if (pos == 0)
        pin = &This->pin_in.IPin_iface;
    else if (pos == 1)
        pin = &This->pin_out.IPin_iface;
    else
        return NULL;

    IPin_AddRef(pin);
    return pin;
}

static LONG WINAPI SampleGrabber_GetPinCount(BaseFilter *iface)
{
    return 2;
}

static const BaseFilterFuncTable basefunc_vtbl = {
    SampleGrabber_GetPin,
    SampleGrabber_GetPinCount
};

/* Helper that buffers data and/or calls installed sample callbacks */
static void SampleGrabber_callback(SG_Impl *This, IMediaSample *sample)
{
    double time = 0.0;
    REFERENCE_TIME tStart, tEnd;
    if (This->bufferLen >= 0) {
        BYTE *data = 0;
        LONG size = IMediaSample_GetActualDataLength(sample);
        if (size >= 0 && SUCCEEDED(IMediaSample_GetPointer(sample, &data))) {
            if (!data)
                size = 0;
            EnterCriticalSection(&This->filter.csFilter);
            if (This->bufferLen != size) {
                if (This->bufferData)
                    CoTaskMemFree(This->bufferData);
                This->bufferData = size ? CoTaskMemAlloc(size) : NULL;
                This->bufferLen = size;
            }
            if (size)
                CopyMemory(This->bufferData, data, size);
            LeaveCriticalSection(&This->filter.csFilter);
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
		    ERR("(%p) Callback referenced sample %p by %u\n", This, sample, ref);
		    /* ugly as hell but some apps are sooo buggy */
		    while (ref--)
			IMediaSample_Release(sample);
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
            FIXME("unsupported method %d\n", This->grabberMethod);
            /* do not bother us again */
            This->grabberMethod = -1;
    }
}


/* SampleGrabber implementation of IBaseFilter interface */

/* IUnknown */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppv)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IBaseFilter_AddRef(IBaseFilter *iface)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    return IUnknown_AddRef(This->outer_unk);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IBaseFilter_Release(IBaseFilter *iface)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    return IUnknown_Release(This->outer_unk);
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_Stop(IBaseFilter *iface)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)\n", This);
    This->filter.state = State_Stopped;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_Pause(IBaseFilter *iface)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)\n", This);
    This->filter.state = State_Paused;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)\n", This);
    This->filter.state = State_Running;
    return S_OK;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_FindPin(IBaseFilter *iface, LPCWSTR id, IPin **pin)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_w(id), pin);
    if (!id || !pin)
        return E_POINTER;
    if (!lstrcmpiW(id,pin_in_name))
    {
        *pin = &This->pin_in.IPin_iface;
        IPin_AddRef(*pin);
        return S_OK;
    }
    else if (!lstrcmpiW(id,pin_out_name))
    {
        *pin = &This->pin_out.IPin_iface;
        IPin_AddRef(*pin);
        return S_OK;
    }
    *pin = NULL;
    return VFW_E_NOT_FOUND;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_JoinFilterGraph(IBaseFilter *iface, IFilterGraph *graph, LPCWSTR name)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);

    TRACE("(%p)->(%p, %s)\n", This, graph, debugstr_w(name));

    BaseFilterImpl_JoinFilterGraph(iface, graph, name);
    This->oneShot = OneShot_None;

    return S_OK;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_QueryVendorInfo(IBaseFilter *iface, LPWSTR *vendor)
{
    TRACE("(%p)\n", vendor);
    if (!vendor)
        return E_POINTER;
    *vendor = CoTaskMemAlloc(sizeof(vendor_name));
    CopyMemory(*vendor, vendor_name, sizeof(vendor_name));
    return S_OK;
}


/* SampleGrabber implementation of ISampleGrabber interface */

/* IUnknown */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_QueryInterface(ISampleGrabber *iface, REFIID riid, void **ppv)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_ISampleGrabber_AddRef(ISampleGrabber *iface)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    return IUnknown_AddRef(This->outer_unk);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_ISampleGrabber_Release(ISampleGrabber *iface)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    return IUnknown_Release(This->outer_unk);
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetOneShot(ISampleGrabber *iface, BOOL oneShot)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    TRACE("(%p)->(%u)\n", This, oneShot);
    This->oneShot = oneShot ? OneShot_Wait : OneShot_None;
    return S_OK;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetMediaType(ISampleGrabber *iface, const AM_MEDIA_TYPE *type)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    TRACE("(%p)->(%p)\n", This, type);
    if (!type)
        return E_POINTER;
    TRACE("Media type: %s/%s ssize: %u format: %s (%u bytes)\n",
	debugstr_guid(&type->majortype), debugstr_guid(&type->subtype),
	type->lSampleSize,
	debugstr_guid(&type->formattype), type->cbFormat);
    if (This->mtype.pbFormat)
        CoTaskMemFree(This->mtype.pbFormat);
    This->mtype = *type;
    This->mtype.pUnk = NULL;
    if (type->cbFormat) {
        This->mtype.pbFormat = CoTaskMemAlloc(type->cbFormat);
        CopyMemory(This->mtype.pbFormat, type->pbFormat, type->cbFormat);
    }
    else
        This->mtype.pbFormat = NULL;
    return S_OK;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_GetConnectedMediaType(ISampleGrabber *iface, AM_MEDIA_TYPE *type)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    TRACE("(%p)->(%p)\n", This, type);
    if (!type)
        return E_POINTER;
    if (!This->pin_in.pair)
        return VFW_E_NOT_CONNECTED;
    *type = This->mtype;
    if (type->cbFormat) {
        type->pbFormat = CoTaskMemAlloc(type->cbFormat);
        CopyMemory(type->pbFormat, This->mtype.pbFormat, type->cbFormat);
    }
    return S_OK;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetBufferSamples(ISampleGrabber *iface, BOOL bufferEm)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    TRACE("(%p)->(%u)\n", This, bufferEm);
    EnterCriticalSection(&This->filter.csFilter);
    if (bufferEm) {
        if (This->bufferLen < 0)
            This->bufferLen = 0;
    }
    else
        This->bufferLen = -1;
    LeaveCriticalSection(&This->filter.csFilter);
    return S_OK;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_GetCurrentBuffer(ISampleGrabber *iface, LONG *bufSize, LONG *buffer)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    HRESULT ret = S_OK;
    TRACE("(%p)->(%p, %p)\n", This, bufSize, buffer);
    if (!bufSize)
        return E_POINTER;
    EnterCriticalSection(&This->filter.csFilter);
    if (!This->pin_in.pair)
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
    LeaveCriticalSection(&This->filter.csFilter);
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
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    TRACE("(%p)->(%p, %u)\n", This, cb, whichMethod);
    if (This->grabberIface)
        ISampleGrabberCB_Release(This->grabberIface);
    This->grabberIface = cb;
    This->grabberMethod = whichMethod;
    if (cb)
        ISampleGrabberCB_AddRef(cb);
    return S_OK;
}


/* SampleGrabber implementation of IMemInputPin interface */

/* IUnknown */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_QueryInterface(IMemInputPin *iface, REFIID riid, void **ppv)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IMemInputPin_AddRef(IMemInputPin *iface)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    return IUnknown_AddRef(This->outer_unk);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IMemInputPin_Release(IMemInputPin *iface)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    return IUnknown_Release(This->outer_unk);
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_GetAllocator(IMemInputPin *iface, IMemAllocator **allocator)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
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
    SG_Impl *This = impl_from_IMemInputPin(iface);
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
    SG_Impl *This = impl_from_IMemInputPin(iface);
    FIXME("(%p)->(%p): semi-stub\n", This, props);
    if (!props)
        return E_POINTER;
    return This->memOutput ? IMemInputPin_GetAllocatorRequirements(This->memOutput, props) : E_NOTIMPL;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_Receive(IMemInputPin *iface, IMediaSample *sample)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    HRESULT hr;
    TRACE("(%p)->(%p) output = %p, grabber = %p\n", This, sample, This->memOutput, This->grabberIface);
    if (!sample)
        return E_POINTER;
    if ((This->filter.state != State_Running) || (This->oneShot == OneShot_Past))
        return S_FALSE;
    SampleGrabber_callback(This, sample);
    hr = This->memOutput ? IMemInputPin_Receive(This->memOutput, sample) : S_OK;
    if (This->oneShot == OneShot_Wait) {
        This->oneShot = OneShot_Past;
        hr = S_FALSE;
        if (This->pin_out.pair)
            IPin_EndOfStream(This->pin_out.pair);
    }
    return hr;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_ReceiveMultiple(IMemInputPin *iface, IMediaSample **samples, LONG nSamples, LONG *nProcessed)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    LONG idx;
    TRACE("(%p)->(%p, %u, %p) output = %p, grabber = %p\n", This, samples, nSamples, nProcessed, This->memOutput, This->grabberIface);
    if (!samples || !nProcessed)
        return E_POINTER;
    if ((This->filter.state != State_Running) || (This->oneShot == OneShot_Past))
        return S_FALSE;
    for (idx = 0; idx < nSamples; idx++)
        SampleGrabber_callback(This, samples[idx]);
    return This->memOutput ? IMemInputPin_ReceiveMultiple(This->memOutput, samples, nSamples, nProcessed) : S_OK;
}

/* IMemInputPin */
static HRESULT WINAPI
SampleGrabber_IMemInputPin_ReceiveCanBlock(IMemInputPin *iface)
{
    SG_Impl *This = impl_from_IMemInputPin(iface);
    TRACE("(%p)\n", This);
    return This->memOutput ? IMemInputPin_ReceiveCanBlock(This->memOutput) : S_OK;
}


/* SampleGrabber member pin implementation */

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IPin_AddRef(IPin *iface)
{
    SG_Pin *This = impl_from_IPin(iface);
    return ISampleGrabber_AddRef(&This->sg->ISampleGrabber_iface);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IPin_Release(IPin *iface)
{
    SG_Pin *This = impl_from_IPin(iface);
    return ISampleGrabber_Release(&This->sg->ISampleGrabber_iface);
}

/* IUnknown */
static HRESULT WINAPI
SampleGrabber_IPin_QueryInterface(IPin *iface, REFIID riid, void **ppv)
{
    SG_Pin *This = impl_from_IPin(iface);
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IPin))
        *ppv = iface;
    else if (IsEqualIID(riid, &IID_IMemInputPin))
        *ppv = &This->sg->IMemInputPin_iface;
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        return IUnknown_QueryInterface(&This->sg->IUnknown_inner, riid, ppv);
    else if (IsEqualIID(riid, &IID_IMediaPosition))
        return IUnknown_QueryInterface(&This->sg->IUnknown_inner, riid, ppv);
    else {
        WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppv);
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

/* IPin - input pin */
static HRESULT WINAPI
SampleGrabber_In_IPin_Connect(IPin *iface, IPin *receiver, const AM_MEDIA_TYPE *mtype)
{
    WARN("(%p, %p): unexpected\n", receiver, mtype);
    return E_UNEXPECTED;
}

/* IPin - output pin */
static HRESULT WINAPI
SampleGrabber_Out_IPin_Connect(IPin *iface, IPin *receiver, const AM_MEDIA_TYPE *type)
{
    SG_Pin *This = impl_from_IPin(iface);
    HRESULT hr;

    TRACE("(%p)->(%p, %p)\n", This, receiver, type);
    if (!receiver)
        return E_POINTER;
    if (This->pair)
        return VFW_E_ALREADY_CONNECTED;
    if (This->sg->filter.state != State_Stopped)
        return VFW_E_NOT_STOPPED;
    if (type) {
	TRACE("Media type: %s/%s ssize: %u format: %s (%u bytes)\n",
	    debugstr_guid(&type->majortype), debugstr_guid(&type->subtype),
	    type->lSampleSize,
	    debugstr_guid(&type->formattype), type->cbFormat);
	if (!IsEqualGUID(&This->sg->mtype.majortype,&GUID_NULL) &&
	    !IsEqualGUID(&This->sg->mtype.majortype,&type->majortype))
	    return VFW_E_TYPE_NOT_ACCEPTED;
	if (!IsEqualGUID(&This->sg->mtype.subtype,&MEDIASUBTYPE_None) &&
	    !IsEqualGUID(&This->sg->mtype.subtype,&type->subtype))
	    return VFW_E_TYPE_NOT_ACCEPTED;
	if (!IsEqualGUID(&This->sg->mtype.formattype,&GUID_NULL) &&
	    !IsEqualGUID(&This->sg->mtype.formattype,&FORMAT_None) &&
	    !IsEqualGUID(&This->sg->mtype.formattype,&type->formattype))
	    return VFW_E_TYPE_NOT_ACCEPTED;
    }
    else
	type = &This->sg->mtype;
    if (!IsEqualGUID(&type->formattype, &FORMAT_None) &&
	!IsEqualGUID(&type->formattype, &GUID_NULL) &&
	!type->pbFormat)
	return VFW_E_TYPE_NOT_ACCEPTED;
    hr = IPin_ReceiveConnection(receiver, &This->IPin_iface, type);
    if (FAILED(hr))
	return hr;
    This->pair = receiver;
    if (This->sg->memOutput) {
        IMemInputPin_Release(This->sg->memOutput);
        This->sg->memOutput = NULL;
    }
    IPin_QueryInterface(receiver,&IID_IMemInputPin,(void **)&(This->sg->memOutput));
    TRACE("(%p) Accepted IPin %p, IMemInputPin %p\n", This, receiver, This->sg->memOutput);
    return S_OK;
}

/* IPin - input pin */
static HRESULT WINAPI
SampleGrabber_In_IPin_ReceiveConnection(IPin *iface, IPin *connector, const AM_MEDIA_TYPE *type)
{
    SG_Pin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p, %p)\n", This, connector, type);
    if (!connector)
        return E_POINTER;
    if (This->pair)
        return VFW_E_ALREADY_CONNECTED;
    if (This->sg->filter.state != State_Stopped)
        return VFW_E_NOT_STOPPED;
    if (type) {
	TRACE("Media type: %s/%s ssize: %u format: %s (%u bytes)\n",
	    debugstr_guid(&type->majortype), debugstr_guid(&type->subtype),
	    type->lSampleSize,
	    debugstr_guid(&type->formattype), type->cbFormat);
	if (!IsEqualGUID(&type->formattype, &FORMAT_None) &&
	    !IsEqualGUID(&type->formattype, &GUID_NULL) &&
	    !type->pbFormat)
	    return VFW_E_INVALIDMEDIATYPE;
	if (!IsEqualGUID(&This->sg->mtype.majortype,&GUID_NULL) &&
	    !IsEqualGUID(&This->sg->mtype.majortype,&type->majortype))
	    return VFW_E_TYPE_NOT_ACCEPTED;
	if (!IsEqualGUID(&This->sg->mtype.subtype,&MEDIASUBTYPE_None) &&
	    !IsEqualGUID(&This->sg->mtype.subtype,&type->subtype))
	    return VFW_E_TYPE_NOT_ACCEPTED;
	if (!IsEqualGUID(&This->sg->mtype.formattype,&GUID_NULL) &&
	    !IsEqualGUID(&This->sg->mtype.formattype,&FORMAT_None) &&
	    !IsEqualGUID(&This->sg->mtype.formattype,&type->formattype))
	    return VFW_E_TYPE_NOT_ACCEPTED;
        if (This->sg->mtype.pbFormat)
            CoTaskMemFree(This->sg->mtype.pbFormat);
        This->sg->mtype = *type;
        This->sg->mtype.pUnk = NULL;
        if (type->cbFormat) {
            This->sg->mtype.pbFormat = CoTaskMemAlloc(type->cbFormat);
            CopyMemory(This->sg->mtype.pbFormat, type->pbFormat, type->cbFormat);
        }
        else
            This->sg->mtype.pbFormat = NULL;
    }
    This->pair = connector;
    TRACE("(%p) Accepted IPin %p\n", This, connector);
    return S_OK;
}

/* IPin - output pin */
static HRESULT WINAPI
SampleGrabber_Out_IPin_ReceiveConnection(IPin *iface, IPin *connector, const AM_MEDIA_TYPE *mtype)
{
    WARN("(%p, %p): unexpected\n", connector, mtype);
    return E_UNEXPECTED;
}

/* IPin - input pin */
static HRESULT WINAPI
SampleGrabber_In_IPin_Disconnect(IPin *iface)
{
    SG_Pin *This = impl_from_IPin(iface);

    TRACE("(%p)->() pair = %p\n", This, This->pair);
    if (This->sg->filter.state != State_Stopped)
        return VFW_E_NOT_STOPPED;
    if (This->pair) {
        This->pair = NULL;
        return S_OK;
    }
    return S_FALSE;
}

/* IPin - output pin */
static HRESULT WINAPI
SampleGrabber_Out_IPin_Disconnect(IPin *iface)
{
    SG_Pin *This = impl_from_IPin(iface);

    TRACE("(%p)->() pair = %p\n", This, This->pair);
    if (This->sg->filter.state != State_Stopped)
        return VFW_E_NOT_STOPPED;
    if (This->pair) {
        This->pair = NULL;
        if (This->sg->memOutput) {
            IMemInputPin_Release(This->sg->memOutput);
            This->sg->memOutput = NULL;
        }
        return S_OK;
    }
    return S_FALSE;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_ConnectedTo(IPin *iface, IPin **pin)
{
    SG_Pin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p) pair = %p\n", This, pin, This->pair);
    if (!pin)
        return E_POINTER;
    *pin = This->pair;
    if (*pin) {
        IPin_AddRef(*pin);
        return S_OK;
    }
    return VFW_E_NOT_CONNECTED;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *mtype)
{
    SG_Pin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p)\n", This, mtype);
    if (!mtype)
        return E_POINTER;
    if (!This->pair)
        return VFW_E_NOT_CONNECTED;
    *mtype = This->sg->mtype;
    if (mtype->cbFormat) {
        mtype->pbFormat = CoTaskMemAlloc(mtype->cbFormat);
        CopyMemory(mtype->pbFormat, This->sg->mtype.pbFormat, mtype->cbFormat);
    }
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_QueryPinInfo(IPin *iface, PIN_INFO *info)
{
    SG_Pin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p)\n", This, info);
    if (!info)
        return E_POINTER;
    info->pFilter = &This->sg->filter.IBaseFilter_iface;
    IBaseFilter_AddRef(info->pFilter);
    info->dir = This->dir;
    lstrcpynW(info->achName,This->name,MAX_PIN_NAME);
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_QueryDirection(IPin *iface, PIN_DIRECTION *dir)
{
    SG_Pin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p)\n", This, dir);
    if (!dir)
        return E_POINTER;
    *dir = This->dir;
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_QueryId(IPin *iface, LPWSTR *id)
{
    SG_Pin *This = impl_from_IPin(iface);

    int len;
    TRACE("(%p)->(%p)\n", This, id);
    if (!id)
        return E_POINTER;
    len = sizeof(WCHAR)*(1+lstrlenW(This->name));
    *id = CoTaskMemAlloc(len);
    CopyMemory(*id, This->name, len);
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *mtype)
{
    TRACE("(%p)\n", mtype);
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_EnumMediaTypes(IPin *iface, IEnumMediaTypes **mtypes)
{
    SG_Pin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p)\n", This, mtypes);
    if (!mtypes)
        return E_POINTER;
    *mtypes = mediaenum_create(This->sg->pin_in.pair ? &This->sg->mtype : NULL, FALSE);
    return *mtypes ? S_OK : E_OUTOFMEMORY;
}

/* IPin - input pin */
static HRESULT WINAPI
SampleGrabber_In_IPin_QueryInternalConnections(IPin *iface, IPin **pins, ULONG *nPins)
{
    SG_Pin *This = impl_from_IPin(iface);

    TRACE("(%p)->(%p, %p) size = %u\n", This, pins, nPins, (nPins ? *nPins : 0));
    if (!nPins)
        return E_POINTER;
    if (*nPins) {
	if (!pins)
	    return E_POINTER;
        IPin_AddRef(&This->sg->pin_out.IPin_iface);
        *pins = &This->sg->pin_out.IPin_iface;
	*nPins = 1;
	return S_OK;
    }
    *nPins = 1;
    return S_FALSE;
}

/* IPin - output pin */
static HRESULT WINAPI
SampleGrabber_Out_IPin_QueryInternalConnections(IPin *iface, IPin **pins, ULONG *nPins)
{
    WARN("(%p, %p): unexpected\n", pins, nPins);
    if (nPins)
        *nPins = 0;
    return E_NOTIMPL;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_EndOfStream(IPin *iface)
{
    FIXME(": stub\n");
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_BeginFlush(IPin *iface)
{
    FIXME(": stub\n");
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_EndFlush(IPin *iface)
{
    FIXME(": stub\n");
    return S_OK;
}

/* IPin */
static HRESULT WINAPI
SampleGrabber_IPin_NewSegment(IPin *iface, REFERENCE_TIME tStart, REFERENCE_TIME tStop, double rate)
{
    FIXME(": stub\n");
    return S_OK;
}


/* SampleGrabber vtables and constructor */

static const IBaseFilterVtbl IBaseFilter_VTable =
{
    SampleGrabber_IBaseFilter_QueryInterface,
    SampleGrabber_IBaseFilter_AddRef,
    SampleGrabber_IBaseFilter_Release,
    BaseFilterImpl_GetClassID,
    SampleGrabber_IBaseFilter_Stop,
    SampleGrabber_IBaseFilter_Pause,
    SampleGrabber_IBaseFilter_Run,
    BaseFilterImpl_GetState,
    BaseFilterImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    SampleGrabber_IBaseFilter_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    SampleGrabber_IBaseFilter_JoinFilterGraph,
    SampleGrabber_IBaseFilter_QueryVendorInfo,
};

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

static const IPinVtbl IPin_In_VTable =
{
    SampleGrabber_IPin_QueryInterface,
    SampleGrabber_IPin_AddRef,
    SampleGrabber_IPin_Release,
    SampleGrabber_In_IPin_Connect,
    SampleGrabber_In_IPin_ReceiveConnection,
    SampleGrabber_In_IPin_Disconnect,
    SampleGrabber_IPin_ConnectedTo,
    SampleGrabber_IPin_ConnectionMediaType,
    SampleGrabber_IPin_QueryPinInfo,
    SampleGrabber_IPin_QueryDirection,
    SampleGrabber_IPin_QueryId,
    SampleGrabber_IPin_QueryAccept,
    SampleGrabber_IPin_EnumMediaTypes,
    SampleGrabber_In_IPin_QueryInternalConnections,
    SampleGrabber_IPin_EndOfStream,
    SampleGrabber_IPin_BeginFlush,
    SampleGrabber_IPin_EndFlush,
    SampleGrabber_IPin_NewSegment,
};

static const IPinVtbl IPin_Out_VTable =
{
    SampleGrabber_IPin_QueryInterface,
    SampleGrabber_IPin_AddRef,
    SampleGrabber_IPin_Release,
    SampleGrabber_Out_IPin_Connect,
    SampleGrabber_Out_IPin_ReceiveConnection,
    SampleGrabber_Out_IPin_Disconnect,
    SampleGrabber_IPin_ConnectedTo,
    SampleGrabber_IPin_ConnectionMediaType,
    SampleGrabber_IPin_QueryPinInfo,
    SampleGrabber_IPin_QueryDirection,
    SampleGrabber_IPin_QueryId,
    SampleGrabber_IPin_QueryAccept,
    SampleGrabber_IPin_EnumMediaTypes,
    SampleGrabber_Out_IPin_QueryInternalConnections,
    SampleGrabber_IPin_EndOfStream,
    SampleGrabber_IPin_BeginFlush,
    SampleGrabber_IPin_EndFlush,
    SampleGrabber_IPin_NewSegment,
};

HRESULT SampleGrabber_create(IUnknown *pUnkOuter, LPVOID *ppv)
{
    SG_Impl* obj = NULL;
    ISeekingPassThru *passthru;
    HRESULT hr;

    TRACE("(%p,%p)\n", ppv, pUnkOuter);

    obj = CoTaskMemAlloc(sizeof(SG_Impl));
    if (NULL == obj) {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }
    ZeroMemory(obj, sizeof(SG_Impl));

    BaseFilter_Init(&obj->filter, &IBaseFilter_VTable, &CLSID_SampleGrabber,
            (DWORD_PTR)(__FILE__ ": SG_Impl.csFilter"), &basefunc_vtbl);
    obj->IUnknown_inner.lpVtbl = &samplegrabber_vtbl;
    obj->ISampleGrabber_iface.lpVtbl = &ISampleGrabber_VTable;
    obj->IMemInputPin_iface.lpVtbl = &IMemInputPin_VTable;
    obj->pin_in.IPin_iface.lpVtbl = &IPin_In_VTable;
    obj->pin_in.dir = PINDIR_INPUT;
    obj->pin_in.name = pin_in_name;
    obj->pin_in.sg = obj;
    obj->pin_in.pair = NULL;
    obj->pin_out.IPin_iface.lpVtbl = &IPin_Out_VTable;
    obj->pin_out.dir = PINDIR_OUTPUT;
    obj->pin_out.name = pin_out_name;
    obj->pin_out.sg = obj;
    obj->pin_out.pair = NULL;
    obj->mtype.majortype = GUID_NULL;
    obj->mtype.subtype = MEDIASUBTYPE_None;
    obj->mtype.formattype = FORMAT_None;
    obj->allocator = NULL;
    obj->memOutput = NULL;
    obj->grabberIface = NULL;
    obj->grabberMethod = -1;
    obj->oneShot = OneShot_None;
    obj->bufferLen = -1;
    obj->bufferData = NULL;

    if (pUnkOuter)
        obj->outer_unk = pUnkOuter;
    else
        obj->outer_unk = &obj->IUnknown_inner;

    hr = CoCreateInstance(&CLSID_SeekingPassThru, (IUnknown*)obj, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&obj->seekthru_unk);
    if(hr)
        return hr;
    IUnknown_QueryInterface(obj->seekthru_unk, &IID_ISeekingPassThru, (void**)&passthru);
    ISeekingPassThru_Init(passthru, FALSE, &obj->pin_in.IPin_iface);
    ISeekingPassThru_Release(passthru);

    *ppv = &obj->IUnknown_inner;
    return S_OK;
}
