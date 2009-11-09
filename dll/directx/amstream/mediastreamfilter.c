/*
 * Implementation of MediaStream Filter
 *
 * Copyright 2008 Christian Costa
 *
 * This file contains the (internal) driver registration functions,
 * driver enumeration APIs and DirectDraw creation functions.
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

#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"

#include "amstream_private.h"
#include "amstream.h"

#include "ddstream.h"

WINE_DEFAULT_DEBUG_CHANNEL(amstream);

typedef struct {
    IMediaStreamFilter lpVtbl;
    LONG ref;
    CRITICAL_SECTION csFilter;
    FILTER_STATE state;
    REFERENCE_TIME rtStreamStart;
    IReferenceClock * pClock;
    FILTER_INFO filterInfo;
} IMediaStreamFilterImpl;

static const struct IMediaStreamFilterVtbl MediaStreamFilter_Vtbl;

HRESULT MediaStreamFilter_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    IMediaStreamFilterImpl* object;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IMediaStreamFilterImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->lpVtbl.lpVtbl = &MediaStreamFilter_Vtbl;
    object->ref = 1;

    *ppObj = object;

    return S_OK;
}

/*** IUnknown methods ***/

static HRESULT WINAPI MediaStreamFilterImpl_QueryInterface(IMediaStreamFilter * iface, REFIID riid, LPVOID * ppv)
{
    IMediaStreamFilterImpl *This = (IMediaStreamFilterImpl *)iface;

    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IPersist))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IMediaFilter))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IBaseFilter))
        *ppv = This;
    else if (IsEqualIID(riid, &IID_IMediaStreamFilter))
        *ppv = This;

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI MediaStreamFilterImpl_AddRef(IMediaStreamFilter * iface)
{
    IMediaStreamFilterImpl *This = (IMediaStreamFilterImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() AddRef from %d\n", iface, refCount - 1);

    return refCount;
}

static ULONG WINAPI MediaStreamFilterImpl_Release(IMediaStreamFilter * iface)
{
    IMediaStreamFilterImpl *This = (IMediaStreamFilterImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() Release from %d\n", iface, refCount + 1);

    if (!refCount)
    {
        This->lpVtbl.lpVtbl = NULL;
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refCount;
}

/*** IPersist methods ***/

static HRESULT WINAPI MediaStreamFilterImpl_GetClassID(IMediaStreamFilter * iface, CLSID * pClsid)
{
    TRACE("(%p)->(%p)\n", iface, pClsid);

    *pClsid = CLSID_MediaStreamFilter;

    return S_OK;
}

/*** IMediaFilter methods ***/

static HRESULT WINAPI MediaStreamFilterImpl_Stop(IMediaStreamFilter * iface)
{
    FIXME("(%p)->(): Stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_Pause(IMediaStreamFilter * iface)
{
    FIXME("(%p)->(): Stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_Run(IMediaStreamFilter * iface, REFERENCE_TIME tStart)
{
    FIXME("(%p)->(%s): Stub!\n", iface, wine_dbgstr_longlong(tStart));

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_GetState(IMediaStreamFilter * iface, DWORD dwMilliSecsTimeout, FILTER_STATE *pState)
{
    FIXME("(%p)->(%d,%p): Stub!\n", iface, dwMilliSecsTimeout, pState);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_SetSyncSource(IMediaStreamFilter * iface, IReferenceClock *pClock)
{
    TRACE("(%p)->(%p): Stub!\n", iface, pClock);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_GetSyncSource(IMediaStreamFilter * iface, IReferenceClock **ppClock)
{
    FIXME("(%p)->(%p): Stub!\n", iface, ppClock);

    return E_NOTIMPL;
}

/*** IBaseFilter methods ***/

static HRESULT WINAPI MediaStreamFilterImpl_EnumPins(IMediaStreamFilter * iface, IEnumPins **ppEnum)
{
    FIXME("(%p)->(%p): Stub!\n", iface, ppEnum);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_FindPin(IMediaStreamFilter * iface, LPCWSTR Id, IPin **ppPin)
{
    FIXME("(%p)->(%s,%p): Stub!\n", iface, debugstr_w(Id), ppPin);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_QueryFilterInfo(IMediaStreamFilter * iface, FILTER_INFO *pInfo)
{
    FIXME("(%p)->(%p): Stub!\n", iface, pInfo);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_JoinFilterGraph(IMediaStreamFilter * iface, IFilterGraph *pGraph, LPCWSTR pName)
{
    FIXME("(%p)->(%p, %s): Stub!\n", iface, pGraph, debugstr_w(pName));

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_QueryVendorInfo(IMediaStreamFilter * iface, LPWSTR *pVendorInfo)
{
    FIXME("(%p)->(%p): Stub!\n", iface, pVendorInfo);

    return E_NOTIMPL;
}

/*** IMediaStreamFilter methods ***/

static HRESULT WINAPI MediaStreamFilterImpl_AddMediaStream(IMediaStreamFilter* iface, IAMMediaStream *pAMMediaStream)
{
    FIXME("(%p)->(%p): Stub!\n", iface, pAMMediaStream);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_GetMediaStream(IMediaStreamFilter* iface, REFMSPID idPurpose, IMediaStream **ppMediaStream)
{
    FIXME("(%p)->(%s,%p): Stub!\n", iface, debugstr_guid(idPurpose), ppMediaStream);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_EnumMediaStreams(IMediaStreamFilter* iface, LONG Index, IMediaStream **ppMediaStream)
{
    FIXME("(%p)->(%d,%p): Stub!\n", iface, Index, ppMediaStream);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_SupportSeeking(IMediaStreamFilter* iface, BOOL bRenderer)
{
    FIXME("(%p)->(%d): Stub!\n", iface, bRenderer);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_ReferenceTimeToStreamTime(IMediaStreamFilter* iface, REFERENCE_TIME *pTime)
{
    FIXME("(%p)->(%p): Stub!\n", iface, pTime);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_GetCurrentStreamTime(IMediaStreamFilter* iface, REFERENCE_TIME *pCurrentStreamTime)
{
    FIXME("(%p)->(%p): Stub!\n", iface, pCurrentStreamTime);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_WaitUntil(IMediaStreamFilter* iface, REFERENCE_TIME WaitStreamTime)
{
    FIXME("(%p)->(%s): Stub!\n", iface, wine_dbgstr_longlong(WaitStreamTime));

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_Flush(IMediaStreamFilter* iface, BOOL bCancelEOS)
{
    FIXME("(%p)->(%d): Stub!\n", iface, bCancelEOS);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_EndOfStream(IMediaStreamFilter* iface)
{
    FIXME("(%p)->(): Stub!\n",  iface);

    return E_NOTIMPL;
}

static const IMediaStreamFilterVtbl MediaStreamFilter_Vtbl =
{
    MediaStreamFilterImpl_QueryInterface,
    MediaStreamFilterImpl_AddRef,
    MediaStreamFilterImpl_Release,
    MediaStreamFilterImpl_GetClassID,
    MediaStreamFilterImpl_Stop,
    MediaStreamFilterImpl_Pause,
    MediaStreamFilterImpl_Run,
    MediaStreamFilterImpl_GetState,
    MediaStreamFilterImpl_SetSyncSource,
    MediaStreamFilterImpl_GetSyncSource,
    MediaStreamFilterImpl_EnumPins,
    MediaStreamFilterImpl_FindPin,
    MediaStreamFilterImpl_QueryFilterInfo,
    MediaStreamFilterImpl_JoinFilterGraph,
    MediaStreamFilterImpl_QueryVendorInfo,
    MediaStreamFilterImpl_AddMediaStream,
    MediaStreamFilterImpl_GetMediaStream,
    MediaStreamFilterImpl_EnumMediaStreams,
    MediaStreamFilterImpl_SupportSeeking,
    MediaStreamFilterImpl_ReferenceTimeToStreamTime,
    MediaStreamFilterImpl_GetCurrentStreamTime,
    MediaStreamFilterImpl_WaitUntil,
    MediaStreamFilterImpl_Flush,
    MediaStreamFilterImpl_EndOfStream
};
