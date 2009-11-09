/*
 * Implementation of IMediaStream Interface
 *
 * Copyright 2005, 2008 Christian Costa
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
#include "ddstream.h"
#include "amstream.h"

WINE_DEFAULT_DEBUG_CHANNEL(amstream);

typedef struct {
    IMediaStream lpVtbl;
    LONG ref;
    IMultiMediaStream* Parent;
    MSPID PurposeId;
    STREAM_TYPE StreamType;
} IMediaStreamImpl;

typedef struct {
    IDirectDrawMediaStream lpVtbl;
    LONG ref;
    IMultiMediaStream* Parent;
    MSPID PurposeId;
    STREAM_TYPE StreamType;
} IDirectDrawMediaStreamImpl;

static const struct IMediaStreamVtbl MediaStream_Vtbl;
static const struct IDirectDrawMediaStreamVtbl DirectDrawMediaStream_Vtbl;

HRESULT MediaStream_create(IMultiMediaStream* Parent, const MSPID* pPurposeId, STREAM_TYPE StreamType, IMediaStream** ppMediaStream)
{
    IMediaStreamImpl* object; 

    TRACE("(%p,%p,%p)\n", Parent, pPurposeId, ppMediaStream);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IMediaStreamImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->lpVtbl.lpVtbl = &MediaStream_Vtbl;
    object->ref = 1;

    object->Parent = Parent;
    object->PurposeId = *pPurposeId;
    object->StreamType = StreamType;

    *ppMediaStream = (IMediaStream*)object;

    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IMediaStreamImpl_QueryInterface(IMediaStream* iface, REFIID riid, void** ppvObject)
{
    IMediaStreamImpl *This = (IMediaStreamImpl *)iface;

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMediaStream))
    {
      IClassFactory_AddRef(iface);
      *ppvObject = This;
      return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static ULONG WINAPI IMediaStreamImpl_AddRef(IMediaStream* iface)
{
    IMediaStreamImpl *This = (IMediaStreamImpl *)iface;

    TRACE("(%p/%p)\n", iface, This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IMediaStreamImpl_Release(IMediaStream* iface)
{
    IMediaStreamImpl *This = (IMediaStreamImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)\n", iface, This);

    if (!ref)
      HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IMediaStream methods ***/
static HRESULT WINAPI IMediaStreamImpl_GetMultiMediaStream(IMediaStream* iface, IMultiMediaStream** ppMultiMediaStream)
{
    IMediaStreamImpl *This = (IMediaStreamImpl *)iface;

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, ppMultiMediaStream);

    return S_FALSE;
}


static HRESULT WINAPI IMediaStreamImpl_GetInformation(IMediaStream* iface, MSPID* pPurposeId, STREAM_TYPE* pType)
{
    IMediaStreamImpl *This = (IMediaStreamImpl *)iface;

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, pPurposeId, pType);

    if (pPurposeId)
        *pPurposeId = This->PurposeId;
    if (pType)
        *pType = This->StreamType;

    return S_OK;
}

static HRESULT WINAPI IMediaStreamImpl_SetSameFormat(IMediaStream* iface, IMediaStream* pStreamThatHasDesiredFormat, DWORD dwFlags)
{
    IMediaStreamImpl *This = (IMediaStreamImpl *)iface;

    FIXME("(%p/%p)->(%p,%x) stub!\n", This, iface, pStreamThatHasDesiredFormat, dwFlags);

    return S_FALSE;
}

static HRESULT WINAPI IMediaStreamImpl_AllocateSample(IMediaStream* iface, DWORD dwFlags, IStreamSample** ppSample)
{
    IMediaStreamImpl *This = (IMediaStreamImpl *)iface;

    FIXME("(%p/%p)->(%x,%p) stub!\n", This, iface, dwFlags, ppSample);

    return S_FALSE;
}

static HRESULT WINAPI IMediaStreamImpl_CreateSharedSample(IMediaStream* iface, IStreamSample* pExistingSample, DWORD dwFlags, IStreamSample** ppSample)
{
    IMediaStreamImpl *This = (IMediaStreamImpl *)iface;

    FIXME("(%p/%p)->(%p,%x,%p) stub!\n", This, iface, pExistingSample, dwFlags, ppSample);

    return S_FALSE;
}

static HRESULT WINAPI IMediaStreamImpl_SendEndOfStream(IMediaStream* iface, DWORD dwFlags)
{
    IMediaStreamImpl *This = (IMediaStreamImpl *)iface;

    FIXME("(%p/%p)->(%x) stub!\n", This, iface, dwFlags);

    return S_FALSE;
}

static const struct IMediaStreamVtbl MediaStream_Vtbl =
{
    IMediaStreamImpl_QueryInterface,
    IMediaStreamImpl_AddRef,
    IMediaStreamImpl_Release,
    IMediaStreamImpl_GetMultiMediaStream,
    IMediaStreamImpl_GetInformation,
    IMediaStreamImpl_SetSameFormat,
    IMediaStreamImpl_AllocateSample,
    IMediaStreamImpl_CreateSharedSample,
    IMediaStreamImpl_SendEndOfStream
};

HRESULT DirectDrawMediaStream_create(IMultiMediaStream* Parent, const MSPID* pPurposeId, STREAM_TYPE StreamType, IMediaStream** ppMediaStream)
{
    IDirectDrawMediaStreamImpl* object;

    TRACE("(%p,%p,%p)\n", Parent, pPurposeId, ppMediaStream);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IMediaStreamImpl));
    if (!object)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    object->lpVtbl.lpVtbl = &DirectDrawMediaStream_Vtbl;
    object->ref = 1;

    object->Parent = Parent;
    object->PurposeId = *pPurposeId;
    object->StreamType = StreamType;

    *ppMediaStream = (IMediaStream*)object;

    return S_OK;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_QueryInterface(IDirectDrawMediaStream* iface, REFIID riid, void** ppvObject)
{
    IMediaStreamImpl *This = (IMediaStreamImpl *)iface;

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMediaStream) ||
        IsEqualGUID(riid, &IID_IDirectDrawMediaStream))
    {
      IClassFactory_AddRef(iface);
      *ppvObject = This;
      return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);
    return E_NOINTERFACE;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_GetFormat(IDirectDrawMediaStream* iface, DDSURFACEDESC *pDDSDCurrent,
                                                           IDirectDrawPalette **ppDirectDrawPalette, DDSURFACEDESC *pDDSDDesired, DWORD *pdwFlags)
{
    FIXME("(%p)->(%p,%p,%p,%p) stub!\n", iface, pDDSDCurrent, ppDirectDrawPalette, pDDSDDesired, pdwFlags);

    return E_NOTIMPL;

}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_SetFormat(IDirectDrawMediaStream* iface, const DDSURFACEDESC *pDDSurfaceDesc,
                                                          IDirectDrawPalette *pDirectDrawPalette)
{
    FIXME("(%p)->(%p,%p) stub!\n", iface, pDDSurfaceDesc, pDirectDrawPalette);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_GetDirectDraw(IDirectDrawMediaStream* iface, IDirectDraw **ppDirectDraw)
{
    FIXME("(%p)->(%p) stub!\n", iface, ppDirectDraw);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_SetDirectDraw(IDirectDrawMediaStream* iface, IDirectDraw *pDirectDraw)
{
    FIXME("(%p)->(%p) stub!\n", iface, pDirectDraw);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_CreateSample(IDirectDrawMediaStream* iface, IDirectDrawSurface *pSurface, const RECT *pRect,
                                                             DWORD dwFlags, IDirectDrawStreamSample **ppSample)
{
    FIXME("(%p)->(%p,%p,%x,%p) stub!\n", iface, pSurface, pRect, dwFlags, ppSample);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawMediaStreamImpl_GetTimePerFrame(IDirectDrawMediaStream* iface, STREAM_TIME *pFrameTime)
{
    FIXME("(%p)->(%p) stub!\n", iface, pFrameTime);

    return E_NOTIMPL;
}

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(DirectDrawMediaStream_Vtbl.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const struct IDirectDrawMediaStreamVtbl DirectDrawMediaStream_Vtbl =
{
    IDirectDrawMediaStreamImpl_QueryInterface,
    XCAST(AddRef)IMediaStreamImpl_AddRef,
    XCAST(Release)IMediaStreamImpl_Release,
    XCAST(GetMultiMediaStream)IMediaStreamImpl_GetMultiMediaStream,
    XCAST(GetInformation)IMediaStreamImpl_GetInformation,
    XCAST(SetSameFormat)IMediaStreamImpl_SetSameFormat,
    XCAST(AllocateSample)IMediaStreamImpl_AllocateSample,
    XCAST(CreateSharedSample)IMediaStreamImpl_CreateSharedSample,
    XCAST(SendEndOfStream)IMediaStreamImpl_SendEndOfStream,
    IDirectDrawMediaStreamImpl_GetFormat,
    IDirectDrawMediaStreamImpl_SetFormat,
    IDirectDrawMediaStreamImpl_GetDirectDraw,
    IDirectDrawMediaStreamImpl_SetDirectDraw,
    IDirectDrawMediaStreamImpl_CreateSample,
    IDirectDrawMediaStreamImpl_GetTimePerFrame
};
#undef XCAST
