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

WINE_DEFAULT_DEBUG_CHANNEL(qedit);

static WCHAR const vendor_name[] = { 'W', 'i', 'n', 'e', 0 };

/* Sample Grabber filter implementation */
typedef struct _SG_Impl {
    const IBaseFilterVtbl* IBaseFilter_Vtbl;
    const ISampleGrabberVtbl* ISampleGrabber_Vtbl;
    /* TODO: IMediaPosition, IMediaSeeking, IQualityControl */
    LONG refCount;
    FILTER_INFO info;
    FILTER_STATE state;
    IMemAllocator *allocator;
    IReferenceClock *refClock;
} SG_Impl;

/* Get the SampleGrabber implementation This pointer from various interface pointers */
static inline SG_Impl *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return (SG_Impl *)((char*)iface - FIELD_OFFSET(SG_Impl, IBaseFilter_Vtbl));
}

static inline SG_Impl *impl_from_ISampleGrabber(ISampleGrabber *iface)
{
    return (SG_Impl *)((char*)iface - FIELD_OFFSET(SG_Impl, ISampleGrabber_Vtbl));
}


/* Cleanup at end of life */
static void SampleGrabber_cleanup(SG_Impl *This)
{
    TRACE("(%p)\n", This);
    if (This->info.pGraph)
	WARN("(%p) still joined to filter graph %p\n", This, This->info.pGraph);
    if (This->allocator)
        IMemAllocator_Release(This->allocator);
    if (This->refClock)
	IReferenceClock_Release(This->refClock);
}

/* Common helper AddRef called from all interfaces */
static ULONG SampleGrabber_addref(SG_Impl *This)
{
    ULONG refCount = InterlockedIncrement(&This->refCount);
    TRACE("(%p) new ref = %u\n", This, refCount);
    return refCount;
}

/* Common helper Release called from all interfaces */
static ULONG SampleGrabber_release(SG_Impl *This)
{
    ULONG refCount = InterlockedDecrement(&This->refCount);
    TRACE("(%p) new ref = %u\n", This, refCount);
    if (refCount == 0)
    {
        SampleGrabber_cleanup(This);
        CoTaskMemFree(This);
        return 0;
    }
    return refCount;
}

/* Common helper QueryInterface called from all interfaces */
static HRESULT SampleGrabber_query(SG_Impl *This, REFIID riid, void **ppvObject)
{
    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPersist) ||
        IsEqualIID(riid, &IID_IMediaFilter) ||
        IsEqualIID(riid, &IID_IBaseFilter)) {
        SampleGrabber_addref(This);
        *ppvObject = &(This->IBaseFilter_Vtbl);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_ISampleGrabber)) {
        SampleGrabber_addref(This);
        *ppvObject = &(This->ISampleGrabber_Vtbl);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IMemInputPin))
        FIXME("IMemInputPin not implemented\n");
    else if (IsEqualIID(riid, &IID_IMediaPosition))
        FIXME("IMediaPosition not implemented\n");
    else if (IsEqualIID(riid, &IID_IMediaSeeking))
        FIXME("IMediaSeeking not implemented\n");
    else if (IsEqualIID(riid, &IID_IQualityControl))
        FIXME("IQualityControl not implemented\n");
    *ppvObject = NULL;
    WARN("(%p, %s,%p): not found\n", This, debugstr_guid(riid), ppvObject);
    return E_NOINTERFACE;
}


/* SampleGrabber implementation of IBaseFilter interface */

/* IUnknown */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_QueryInterface(IBaseFilter *iface, REFIID riid, void **ppvObject)
{
    return SampleGrabber_query(impl_from_IBaseFilter(iface), riid, ppvObject);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IBaseFilter_AddRef(IBaseFilter *iface)
{
    return SampleGrabber_addref(impl_from_IBaseFilter(iface));
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_IBaseFilter_Release(IBaseFilter *iface)
{
    return SampleGrabber_release(impl_from_IBaseFilter(iface));
}

/* IPersist */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_GetClassID(IBaseFilter *iface, CLSID *pClassID)
{
    TRACE("(%p)\n", pClassID);
    if (!pClassID)
        return E_POINTER;
    *pClassID = CLSID_SampleGrabber;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_Stop(IBaseFilter *iface)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)\n", This);
    This->state = State_Stopped;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_Pause(IBaseFilter *iface)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)\n", This);
    This->state = State_Paused;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_Run(IBaseFilter *iface, REFERENCE_TIME tStart)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)\n", This);
    This->state = State_Running;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_GetState(IBaseFilter *iface, DWORD msTout, FILTER_STATE *state)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%u, %p)\n", This, msTout, state);
    if (!state)
        return E_POINTER;
    *state = This->state;
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_SetSyncSource(IBaseFilter *iface, IReferenceClock *clock)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, clock);
    if (clock != This->refClock)
    {
	if (clock)
	    IReferenceClock_AddRef(clock);
	if (This->refClock)
	    IReferenceClock_Release(This->refClock);
	This->refClock = clock;
    }
    return S_OK;
}

/* IMediaFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_GetSyncSource(IBaseFilter *iface, IReferenceClock **clock)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, clock);
    if (!clock)
        return E_POINTER;
    if (This->refClock)
	IReferenceClock_AddRef(This->refClock);
    *clock = This->refClock;
    return S_OK;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_EnumPins(IBaseFilter *iface, IEnumPins **pins)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%p): stub\n", This, pins);
    if (!pins)
        return E_POINTER;
    return E_OUTOFMEMORY;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_FindPin(IBaseFilter *iface, LPCWSTR id, IPin **pin)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    FIXME("(%p)->(%s, %p): stub\n", This, debugstr_w(id), pin);
    if (!id || !pin)
        return E_POINTER;
    *pin = NULL;
    return VFW_E_NOT_FOUND;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *info)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p)\n", This, info);
    if (!info)
        return E_POINTER;
    if (This->info.pGraph)
	IFilterGraph_AddRef(This->info.pGraph);
    *info = This->info;
    return S_OK;
}

/* IBaseFilter */
static HRESULT WINAPI
SampleGrabber_IBaseFilter_JoinFilterGraph(IBaseFilter *iface, IFilterGraph *graph, LPCWSTR name)
{
    SG_Impl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p)->(%p, %s)\n", This, graph, debugstr_w(name));
    This->info.pGraph = graph;
    if (name)
	lstrcpynW(This->info.achName,name,MAX_FILTER_NAME);
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
SampleGrabber_ISampleGrabber_QueryInterface(ISampleGrabber *iface, REFIID riid, void **ppvObject)
{
    return SampleGrabber_query(impl_from_ISampleGrabber(iface), riid, ppvObject);
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_ISampleGrabber_AddRef(ISampleGrabber *iface)
{
    return SampleGrabber_addref(impl_from_ISampleGrabber(iface));
}

/* IUnknown */
static ULONG WINAPI
SampleGrabber_ISampleGrabber_Release(ISampleGrabber *iface)
{
    return SampleGrabber_release(impl_from_ISampleGrabber(iface));
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetOneShot(ISampleGrabber *iface, BOOL oneShot)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    FIXME("(%p)->(%u): stub\n", This, oneShot);
    return E_NOTIMPL;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetMediaType(ISampleGrabber *iface, const AM_MEDIA_TYPE *type)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    FIXME("(%p)->(%p): stub\n", This, type);
    if (!type)
        return E_POINTER;
    return E_NOTIMPL;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_GetConnectedMediaType(ISampleGrabber *iface, AM_MEDIA_TYPE *type)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    FIXME("(%p)->(%p): stub\n", This, type);
    if (!type)
        return E_POINTER;
    return E_NOTIMPL;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetBufferSamples(ISampleGrabber *iface, BOOL bufferEm)
{
    TRACE("(%u)\n", bufferEm);
    if (bufferEm) {
        FIXME("buffering not implemented\n");
        return E_NOTIMPL;
    }
    return S_OK;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_GetCurrentBuffer(ISampleGrabber *iface, LONG *bufSize, LONG *buffer)
{
    FIXME("(%p, %p): stub\n", bufSize, buffer);
    if (!bufSize)
        return E_POINTER;
    return E_INVALIDARG;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_GetCurrentSample(ISampleGrabber *iface, IMediaSample **sample)
{
    /* MS doesn't implement it either, noone should call it */
    WARN("(%p): not implemented\n", sample);
    return E_NOTIMPL;
}

/* ISampleGrabber */
static HRESULT WINAPI
SampleGrabber_ISampleGrabber_SetCallback(ISampleGrabber *iface, ISampleGrabberCB *cb, LONG whichMethod)
{
    SG_Impl *This = impl_from_ISampleGrabber(iface);
    FIXME("(%p)->(%p, %u): stub\n", This, cb, whichMethod);
    return E_NOTIMPL;
}


/* SampleGrabber vtables and constructor */

static const IBaseFilterVtbl IBaseFilter_VTable =
{
    SampleGrabber_IBaseFilter_QueryInterface,
    SampleGrabber_IBaseFilter_AddRef,
    SampleGrabber_IBaseFilter_Release,
    SampleGrabber_IBaseFilter_GetClassID,
    SampleGrabber_IBaseFilter_Stop,
    SampleGrabber_IBaseFilter_Pause,
    SampleGrabber_IBaseFilter_Run,
    SampleGrabber_IBaseFilter_GetState,
    SampleGrabber_IBaseFilter_SetSyncSource,
    SampleGrabber_IBaseFilter_GetSyncSource,
    SampleGrabber_IBaseFilter_EnumPins,
    SampleGrabber_IBaseFilter_FindPin,
    SampleGrabber_IBaseFilter_QueryFilterInfo,
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

HRESULT SampleGrabber_create(IUnknown *pUnkOuter, LPVOID *ppv)
{
    SG_Impl* obj = NULL;

    TRACE("(%p,%p)\n", ppv, pUnkOuter);

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    obj = CoTaskMemAlloc(sizeof(SG_Impl));
    if (NULL == obj) {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }
    ZeroMemory(obj, sizeof(SG_Impl));

    obj->refCount = 1;
    obj->IBaseFilter_Vtbl = &IBaseFilter_VTable;
    obj->ISampleGrabber_Vtbl = &ISampleGrabber_VTable;
    obj->info.achName[0] = 0;
    obj->info.pGraph = NULL;
    obj->state = State_Stopped;
    obj->allocator = NULL;
    obj->refClock = NULL;
    *ppv = obj;

    return S_OK;
}
