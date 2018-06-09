/*
 * Implementation of IMediaStream Interfaces
 *
 * Copyright 2005, 2008, 2012 Christian Costa
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

#ifndef __REACTOS__
#define NONAMELESSUNION
#endif
#define COBJMACROS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "dshow.h"

#include "wine/strmbase.h"

#include "amstream_private.h"

#include "ddstream.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(amstream);

#include <initguid.h>
DEFINE_GUID(IID_IDirectDraw7, 0x15e65ec0,0x3b9c,0x11d2,0xb9,0x2f,0x00,0x60,0x97,0x97,0xea,0x5b);

static HRESULT ddrawstreamsample_create(IDirectDrawMediaStream *parent, IDirectDrawSurface *surface,
    const RECT *rect, IDirectDrawStreamSample **ddraw_stream_sample);
static HRESULT audiostreamsample_create(IAudioMediaStream *parent, IAudioData *audio_data, IAudioStreamSample **audio_stream_sample);

struct DirectDrawMediaStreamImpl;

typedef struct {
    BaseInputPin pin;
    struct DirectDrawMediaStreamImpl *parent;
} DirectDrawMediaStreamInputPin;

typedef struct DirectDrawMediaStreamImpl {
    IAMMediaStream IAMMediaStream_iface;
    IDirectDrawMediaStream IDirectDrawMediaStream_iface;
    LONG ref;
    IMultiMediaStream* parent;
    MSPID purpose_id;
    STREAM_TYPE stream_type;
    IDirectDraw7 *ddraw;
    DirectDrawMediaStreamInputPin *input_pin;
    CRITICAL_SECTION critical_section;
} DirectDrawMediaStreamImpl;

static inline DirectDrawMediaStreamImpl *impl_from_DirectDrawMediaStream_IAMMediaStream(IAMMediaStream *iface)
{
    return CONTAINING_RECORD(iface, DirectDrawMediaStreamImpl, IAMMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_QueryInterface(IAMMediaStream *iface,
                                                        REFIID riid, void **ret_iface)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMediaStream) ||
        IsEqualGUID(riid, &IID_IAMMediaStream))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IDirectDrawMediaStream))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = &This->IDirectDrawMediaStream_iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IPin))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = &This->input_pin->pin.pin.IPin_iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IMemInputPin))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = &This->input_pin->pin.IMemInputPin_iface;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_AddRef(IAMMediaStream *iface)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %u\n", iface, This, ref);

    return ref;
}

static ULONG WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_Release(IAMMediaStream *iface)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %u\n", iface, This, ref);

    if (!ref)
    {
        BaseInputPin_Destroy((BaseInputPin *)This->input_pin);
        DeleteCriticalSection(&This->critical_section);
        if (This->ddraw)
            IDirectDraw7_Release(This->ddraw);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IMediaStream methods ***/
static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_GetMultiMediaStream(IAMMediaStream *iface,
        IMultiMediaStream** multi_media_stream)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, multi_media_stream);

    if (!multi_media_stream)
        return E_POINTER;

    IMultiMediaStream_AddRef(This->parent);
    *multi_media_stream = This->parent;

    return S_OK;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_GetInformation(IAMMediaStream *iface,
        MSPID *purpose_id, STREAM_TYPE *type)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, purpose_id, type);

    if (purpose_id)
        *purpose_id = This->purpose_id;
    if (type)
        *type = This->stream_type;

    return S_OK;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_SetSameFormat(IAMMediaStream *iface,
        IMediaStream *pStreamThatHasDesiredFormat, DWORD flags)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x) stub!\n", This, iface, pStreamThatHasDesiredFormat, flags);

    return S_FALSE;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_AllocateSample(IAMMediaStream *iface,
        DWORD flags, IStreamSample **sample)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%x,%p) stub!\n", This, iface, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_CreateSharedSample(IAMMediaStream *iface,
        IStreamSample *existing_sample, DWORD flags, IStreamSample **sample)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p) stub!\n", This, iface, existing_sample, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_SendEndOfStream(IAMMediaStream *iface, DWORD flags)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%x) stub!\n", This, iface, flags);

    return S_FALSE;
}

/*** IAMMediaStream methods ***/
static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_Initialize(IAMMediaStream *iface, IUnknown *source_object, DWORD flags,
                                                    REFMSPID purpose_id, const STREAM_TYPE stream_type)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p,%u) stub!\n", This, iface, source_object, flags, purpose_id, stream_type);

    return S_FALSE;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_SetState(IAMMediaStream *iface, FILTER_STATE state)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%u) stub!\n", This, iface, state);

    return S_FALSE;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_JoinAMMultiMediaStream(IAMMediaStream *iface, IAMMultiMediaStream *am_multi_media_stream)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, am_multi_media_stream);

    return S_FALSE;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_JoinFilter(IAMMediaStream *iface, IMediaStreamFilter *media_stream_filter)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, media_stream_filter);

    This->input_pin->pin.pin.pinInfo.pFilter = (IBaseFilter *)media_stream_filter;

    return S_OK;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IAMMediaStream_JoinFilterGraph(IAMMediaStream *iface, IFilterGraph *filtergraph)
{
    DirectDrawMediaStreamImpl *This = impl_from_DirectDrawMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, filtergraph);

    return S_FALSE;
}

static const struct IAMMediaStreamVtbl DirectDrawMediaStreamImpl_IAMMediaStream_Vtbl =
{
    /*** IUnknown methods ***/
    DirectDrawMediaStreamImpl_IAMMediaStream_QueryInterface,
    DirectDrawMediaStreamImpl_IAMMediaStream_AddRef,
    DirectDrawMediaStreamImpl_IAMMediaStream_Release,
    /*** IMediaStream methods ***/
    DirectDrawMediaStreamImpl_IAMMediaStream_GetMultiMediaStream,
    DirectDrawMediaStreamImpl_IAMMediaStream_GetInformation,
    DirectDrawMediaStreamImpl_IAMMediaStream_SetSameFormat,
    DirectDrawMediaStreamImpl_IAMMediaStream_AllocateSample,
    DirectDrawMediaStreamImpl_IAMMediaStream_CreateSharedSample,
    DirectDrawMediaStreamImpl_IAMMediaStream_SendEndOfStream,
    /*** IAMMediaStream methods ***/
    DirectDrawMediaStreamImpl_IAMMediaStream_Initialize,
    DirectDrawMediaStreamImpl_IAMMediaStream_SetState,
    DirectDrawMediaStreamImpl_IAMMediaStream_JoinAMMultiMediaStream,
    DirectDrawMediaStreamImpl_IAMMediaStream_JoinFilter,
    DirectDrawMediaStreamImpl_IAMMediaStream_JoinFilterGraph
};

static inline DirectDrawMediaStreamImpl *impl_from_IDirectDrawMediaStream(IDirectDrawMediaStream *iface)
{
    return CONTAINING_RECORD(iface, DirectDrawMediaStreamImpl, IDirectDrawMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_QueryInterface(IDirectDrawMediaStream *iface,
        REFIID riid, void **ret_iface)
{
    DirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);
    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ret_iface);
    return IAMMediaStream_QueryInterface(&This->IAMMediaStream_iface, riid, ret_iface);
}

static ULONG WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_AddRef(IDirectDrawMediaStream *iface)
{
    DirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);
    TRACE("(%p/%p)\n", iface, This);
    return IAMMediaStream_AddRef(&This->IAMMediaStream_iface);
}

static ULONG WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_Release(IDirectDrawMediaStream *iface)
{
    DirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);
    TRACE("(%p/%p)\n", iface, This);
    return IAMMediaStream_Release(&This->IAMMediaStream_iface);
}

/*** IMediaStream methods ***/
static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_GetMultiMediaStream(IDirectDrawMediaStream *iface,
        IMultiMediaStream **multi_media_stream)
{
    DirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, multi_media_stream);

    if (!multi_media_stream)
        return E_POINTER;

    IMultiMediaStream_AddRef(This->parent);
    *multi_media_stream = This->parent;

    return S_OK;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_GetInformation(IDirectDrawMediaStream *iface,
        MSPID *purpose_id, STREAM_TYPE *type)
{
    DirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, purpose_id, type);

    if (purpose_id)
        *purpose_id = This->purpose_id;
    if (type)
        *type = This->stream_type;

    return S_OK;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_SetSameFormat(IDirectDrawMediaStream *iface,
        IMediaStream *pStreamThatHasDesiredFormat, DWORD dwFlags)
{
    DirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x) stub!\n", This, iface, pStreamThatHasDesiredFormat, dwFlags);

    return S_FALSE;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_AllocateSample(IDirectDrawMediaStream *iface,
        DWORD dwFlags, IStreamSample **ppSample)
{
    DirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    FIXME("(%p/%p)->(%x,%p) stub!\n", This, iface, dwFlags, ppSample);

    return S_FALSE;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_CreateSharedSample(IDirectDrawMediaStream *iface,
        IStreamSample *pExistingSample, DWORD dwFlags, IStreamSample **ppSample)
{
    DirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p) stub!\n", This, iface, pExistingSample, dwFlags, ppSample);

    return S_FALSE;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_SendEndOfStream(IDirectDrawMediaStream *iface,
        DWORD dwFlags)
{
    DirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    FIXME("(%p/%p)->(%x) stub!\n", This, iface, dwFlags);

    return S_FALSE;
}

/*** IDirectDrawMediaStream methods ***/
static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_GetFormat(IDirectDrawMediaStream *iface,
        DDSURFACEDESC *current_format, IDirectDrawPalette **palette,
        DDSURFACEDESC *desired_format, DWORD *flags)
{
    FIXME("(%p)->(%p,%p,%p,%p) stub!\n", iface, current_format, palette, desired_format,
            flags);

    return MS_E_NOSTREAM;

}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_SetFormat(IDirectDrawMediaStream *iface,
        const DDSURFACEDESC *pDDSurfaceDesc, IDirectDrawPalette *pDirectDrawPalette)
{
    FIXME("(%p)->(%p,%p) stub!\n", iface, pDDSurfaceDesc, pDirectDrawPalette);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_GetDirectDraw(IDirectDrawMediaStream *iface,
        IDirectDraw **ddraw)
{
    DirectDrawMediaStreamImpl *This = impl_from_IDirectDrawMediaStream(iface);

    TRACE("(%p)->(%p)\n", iface, ddraw);

    *ddraw = NULL;
    if (!This->ddraw)
    {
        HRESULT hr = DirectDrawCreateEx(NULL, (void**)&This->ddraw, &IID_IDirectDraw7, NULL);
        if (FAILED(hr))
            return hr;
        IDirectDraw7_SetCooperativeLevel(This->ddraw, NULL, DDSCL_NORMAL);
    }

    return IDirectDraw7_QueryInterface(This->ddraw, &IID_IDirectDraw, (void**)ddraw);
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_SetDirectDraw(IDirectDrawMediaStream *iface,
        IDirectDraw *pDirectDraw)
{
    FIXME("(%p)->(%p) stub!\n", iface, pDirectDraw);

    return E_NOTIMPL;
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_CreateSample(IDirectDrawMediaStream *iface,
        IDirectDrawSurface *surface, const RECT *rect, DWORD dwFlags,
        IDirectDrawStreamSample **ppSample)
{
    TRACE("(%p)->(%p,%s,%x,%p)\n", iface, surface, wine_dbgstr_rect(rect), dwFlags, ppSample);

    return ddrawstreamsample_create(iface, surface, rect, ppSample);
}

static HRESULT WINAPI DirectDrawMediaStreamImpl_IDirectDrawMediaStream_GetTimePerFrame(IDirectDrawMediaStream *iface,
        STREAM_TIME *pFrameTime)
{
    FIXME("(%p)->(%p) stub!\n", iface, pFrameTime);

    return E_NOTIMPL;
}

static const struct IDirectDrawMediaStreamVtbl DirectDrawMediaStreamImpl_IDirectDrawMediaStream_Vtbl =
{
    /*** IUnknown methods ***/
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_QueryInterface,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_AddRef,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_Release,
    /*** IMediaStream methods ***/
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_GetMultiMediaStream,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_GetInformation,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_SetSameFormat,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_AllocateSample,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_CreateSharedSample,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_SendEndOfStream,
    /*** IDirectDrawMediaStream methods ***/
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_GetFormat,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_SetFormat,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_GetDirectDraw,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_SetDirectDraw,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_CreateSample,
    DirectDrawMediaStreamImpl_IDirectDrawMediaStream_GetTimePerFrame
};

static inline DirectDrawMediaStreamInputPin *impl_from_DirectDrawMediaStreamInputPin_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, DirectDrawMediaStreamInputPin, pin.pin.IPin_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI DirectDrawMediaStreamInputPin_IPin_QueryInterface(IPin *iface, REFIID riid, void **ret_iface)
{
    DirectDrawMediaStreamInputPin *This = impl_from_DirectDrawMediaStreamInputPin_IPin(iface);

    return IAMMediaStream_QueryInterface(&This->parent->IAMMediaStream_iface, riid, ret_iface);
}

static ULONG WINAPI DirectDrawMediaStreamInputPin_IPin_AddRef(IPin *iface)
{
    DirectDrawMediaStreamInputPin *This = impl_from_DirectDrawMediaStreamInputPin_IPin(iface);

    return IAMMediaStream_AddRef(&This->parent->IAMMediaStream_iface);
}

static ULONG WINAPI DirectDrawMediaStreamInputPin_IPin_Release(IPin *iface)
{
    DirectDrawMediaStreamInputPin *This = impl_from_DirectDrawMediaStreamInputPin_IPin(iface);

    return IAMMediaStream_Release(&This->parent->IAMMediaStream_iface);
}

static const IPinVtbl DirectDrawMediaStreamInputPin_IPin_Vtbl =
{
    DirectDrawMediaStreamInputPin_IPin_QueryInterface,
    DirectDrawMediaStreamInputPin_IPin_AddRef,
    DirectDrawMediaStreamInputPin_IPin_Release,
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
    BaseInputPinImpl_NewSegment,
};

static HRESULT WINAPI DirectDrawMediaStreamInputPin_CheckMediaType(BasePin *base, const AM_MEDIA_TYPE *media_type)
{
    DirectDrawMediaStreamInputPin *This = impl_from_DirectDrawMediaStreamInputPin_IPin(&base->IPin_iface);

    TRACE("(%p)->(%p)\n", This, media_type);

    if (IsEqualGUID(&media_type->majortype, &MEDIATYPE_Video))
    {
        if (IsEqualGUID(&media_type->subtype, &MEDIASUBTYPE_RGB1) ||
            IsEqualGUID(&media_type->subtype, &MEDIASUBTYPE_RGB4) ||
            IsEqualGUID(&media_type->subtype, &MEDIASUBTYPE_RGB8)  ||
            IsEqualGUID(&media_type->subtype, &MEDIASUBTYPE_RGB565) ||
            IsEqualGUID(&media_type->subtype, &MEDIASUBTYPE_RGB555) ||
            IsEqualGUID(&media_type->subtype, &MEDIASUBTYPE_RGB24) ||
            IsEqualGUID(&media_type->subtype, &MEDIASUBTYPE_RGB32))
        {
            TRACE("Video sub-type %s matches\n", debugstr_guid(&media_type->subtype));
            return S_OK;
        }
    }

    return S_FALSE;
}

static LONG WINAPI DirectDrawMediaStreamInputPin_GetMediaTypeVersion(BasePin *base)
{
    return 0;
}

static HRESULT WINAPI DirectDrawMediaStreamInputPin_GetMediaType(BasePin *base, int index, AM_MEDIA_TYPE *media_type)
{
    DirectDrawMediaStreamInputPin *This = impl_from_DirectDrawMediaStreamInputPin_IPin(&base->IPin_iface);

    TRACE("(%p)->(%d,%p)\n", This, index, media_type);

    /* FIXME: Reset structure as we only fill majortype and minortype for now */
    ZeroMemory(media_type, sizeof(*media_type));

    media_type->majortype = MEDIATYPE_Video;

    switch (index)
    {
        case 0:
            media_type->subtype = MEDIASUBTYPE_RGB1;
            break;
        case 1:
            media_type->subtype = MEDIASUBTYPE_RGB4;
            break;
        case 2:
            media_type->subtype = MEDIASUBTYPE_RGB8;
            break;
        case 3:
            media_type->subtype = MEDIASUBTYPE_RGB565;
            break;
        case 4:
            media_type->subtype = MEDIASUBTYPE_RGB555;
            break;
        case 5:
            media_type->subtype = MEDIASUBTYPE_RGB24;
            break;
        case 6:
            media_type->subtype = MEDIASUBTYPE_RGB32;
            break;
        default:
            return S_FALSE;
    }

    return S_OK;
}

static HRESULT WINAPI DirectDrawMediaStreamInputPin_Receive(BaseInputPin *base, IMediaSample *sample)
{
    DirectDrawMediaStreamInputPin *This = impl_from_DirectDrawMediaStreamInputPin_IPin(&base->pin.IPin_iface);

    FIXME("(%p)->(%p) stub!\n", This, sample);

    return E_NOTIMPL;
}

static const BaseInputPinFuncTable DirectDrawMediaStreamInputPin_FuncTable =
{
    {
        DirectDrawMediaStreamInputPin_CheckMediaType,
        NULL,
        DirectDrawMediaStreamInputPin_GetMediaTypeVersion,
        DirectDrawMediaStreamInputPin_GetMediaType,
    },
    DirectDrawMediaStreamInputPin_Receive,
};

HRESULT ddrawmediastream_create(IMultiMediaStream *parent, const MSPID *purpose_id,
        STREAM_TYPE stream_type, IAMMediaStream **media_stream)
{
    DirectDrawMediaStreamImpl *object;
    PIN_INFO pin_info;
    HRESULT hr;

    TRACE("(%p,%s,%p)\n", parent, debugstr_guid(purpose_id), media_stream);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DirectDrawMediaStreamImpl));
    if (!object)
        return E_OUTOFMEMORY;

    object->IAMMediaStream_iface.lpVtbl = &DirectDrawMediaStreamImpl_IAMMediaStream_Vtbl;
    object->IDirectDrawMediaStream_iface.lpVtbl = &DirectDrawMediaStreamImpl_IDirectDrawMediaStream_Vtbl;
    object->ref = 1;

    InitializeCriticalSection(&object->critical_section);

    pin_info.pFilter = NULL;
    pin_info.dir = PINDIR_INPUT;
    pin_info.achName[0] = 'I';
    StringFromGUID2(purpose_id, pin_info.achName + 1, MAX_PIN_NAME - 1);
    hr = BaseInputPin_Construct(&DirectDrawMediaStreamInputPin_IPin_Vtbl,
        sizeof(DirectDrawMediaStreamInputPin), &pin_info, &DirectDrawMediaStreamInputPin_FuncTable,
        &object->critical_section, NULL, (IPin **)&object->input_pin);
    if (FAILED(hr))
        goto out_object;

    object->input_pin->parent = object;

    object->parent = parent;
    object->purpose_id = *purpose_id;
    object->stream_type = stream_type;

    *media_stream = &object->IAMMediaStream_iface;

    return S_OK;

out_object:
    HeapFree(GetProcessHeap(), 0, object);

    return hr;
}

struct AudioMediaStreamImpl;

typedef struct {
    BaseInputPin pin;
    struct AudioMediaStreamImpl *parent;
} AudioMediaStreamInputPin;

typedef struct AudioMediaStreamImpl {
    IAMMediaStream IAMMediaStream_iface;
    IAudioMediaStream IAudioMediaStream_iface;
    LONG ref;
    IMultiMediaStream* parent;
    MSPID purpose_id;
    STREAM_TYPE stream_type;
    AudioMediaStreamInputPin *input_pin;
    CRITICAL_SECTION critical_section;
} AudioMediaStreamImpl;

static inline AudioMediaStreamImpl *impl_from_AudioMediaStream_IAMMediaStream(IAMMediaStream *iface)
{
    return CONTAINING_RECORD(iface, AudioMediaStreamImpl, IAMMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_QueryInterface(IAMMediaStream *iface,
                                                        REFIID riid, void **ret_iface)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMediaStream) ||
        IsEqualGUID(riid, &IID_IAMMediaStream))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IAudioMediaStream))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = &This->IAudioMediaStream_iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IPin))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = &This->input_pin->pin.pin.IPin_iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IMemInputPin))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = &This->input_pin->pin.IMemInputPin_iface;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI AudioMediaStreamImpl_IAMMediaStream_AddRef(IAMMediaStream *iface)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %u\n", iface, This, ref);

    return ref;
}

static ULONG WINAPI AudioMediaStreamImpl_IAMMediaStream_Release(IAMMediaStream *iface)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %u\n", iface, This, ref);

    if (!ref)
    {
        BaseInputPin_Destroy((BaseInputPin *)This->input_pin);
        DeleteCriticalSection(&This->critical_section);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IMediaStream methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_GetMultiMediaStream(IAMMediaStream *iface,
        IMultiMediaStream** multi_media_stream)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, multi_media_stream);

    if (!multi_media_stream)
        return E_POINTER;

    IMultiMediaStream_AddRef(This->parent);
    *multi_media_stream = This->parent;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_GetInformation(IAMMediaStream *iface,
        MSPID *purpose_id, STREAM_TYPE *type)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, purpose_id, type);

    if (purpose_id)
        *purpose_id = This->purpose_id;
    if (type)
        *type = This->stream_type;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_SetSameFormat(IAMMediaStream *iface,
        IMediaStream *pStreamThatHasDesiredFormat, DWORD flags)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x) stub!\n", This, iface, pStreamThatHasDesiredFormat, flags);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_AllocateSample(IAMMediaStream *iface,
        DWORD flags, IStreamSample **sample)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%x,%p) stub!\n", This, iface, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_CreateSharedSample(IAMMediaStream *iface,
        IStreamSample *existing_sample, DWORD flags, IStreamSample **sample)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p) stub!\n", This, iface, existing_sample, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_SendEndOfStream(IAMMediaStream *iface, DWORD flags)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%x) stub!\n", This, iface, flags);

    return S_FALSE;
}

/*** IAMMediaStream methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_Initialize(IAMMediaStream *iface, IUnknown *source_object, DWORD flags,
                                                    REFMSPID purpose_id, const STREAM_TYPE stream_type)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p,%u) stub!\n", This, iface, source_object, flags, purpose_id, stream_type);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_SetState(IAMMediaStream *iface, FILTER_STATE state)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%u) stub!\n", This, iface, state);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_JoinAMMultiMediaStream(IAMMediaStream *iface, IAMMultiMediaStream *am_multi_media_stream)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, am_multi_media_stream);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_JoinFilter(IAMMediaStream *iface, IMediaStreamFilter *media_stream_filter)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, media_stream_filter);

    This->input_pin->pin.pin.pinInfo.pFilter = (IBaseFilter *)media_stream_filter;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAMMediaStream_JoinFilterGraph(IAMMediaStream *iface, IFilterGraph *filtergraph)
{
    AudioMediaStreamImpl *This = impl_from_AudioMediaStream_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, filtergraph);

    return S_FALSE;
}

static const struct IAMMediaStreamVtbl AudioMediaStreamImpl_IAMMediaStream_Vtbl =
{
    /*** IUnknown methods ***/
    AudioMediaStreamImpl_IAMMediaStream_QueryInterface,
    AudioMediaStreamImpl_IAMMediaStream_AddRef,
    AudioMediaStreamImpl_IAMMediaStream_Release,
    /*** IMediaStream methods ***/
    AudioMediaStreamImpl_IAMMediaStream_GetMultiMediaStream,
    AudioMediaStreamImpl_IAMMediaStream_GetInformation,
    AudioMediaStreamImpl_IAMMediaStream_SetSameFormat,
    AudioMediaStreamImpl_IAMMediaStream_AllocateSample,
    AudioMediaStreamImpl_IAMMediaStream_CreateSharedSample,
    AudioMediaStreamImpl_IAMMediaStream_SendEndOfStream,
    /*** IAMMediaStream methods ***/
    AudioMediaStreamImpl_IAMMediaStream_Initialize,
    AudioMediaStreamImpl_IAMMediaStream_SetState,
    AudioMediaStreamImpl_IAMMediaStream_JoinAMMultiMediaStream,
    AudioMediaStreamImpl_IAMMediaStream_JoinFilter,
    AudioMediaStreamImpl_IAMMediaStream_JoinFilterGraph
};

static inline AudioMediaStreamImpl *impl_from_IAudioMediaStream(IAudioMediaStream *iface)
{
    return CONTAINING_RECORD(iface, AudioMediaStreamImpl, IAudioMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_QueryInterface(IAudioMediaStream *iface,
        REFIID riid, void **ret_iface)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ret_iface);
    return IAMMediaStream_QueryInterface(&This->IAMMediaStream_iface, riid, ret_iface);
}

static ULONG WINAPI AudioMediaStreamImpl_IAudioMediaStream_AddRef(IAudioMediaStream *iface)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)\n", iface, This);
    return IAMMediaStream_AddRef(&This->IAMMediaStream_iface);
}

static ULONG WINAPI AudioMediaStreamImpl_IAudioMediaStream_Release(IAudioMediaStream *iface)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)\n", iface, This);
    return IAMMediaStream_Release(&This->IAMMediaStream_iface);
}

/*** IMediaStream methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_GetMultiMediaStream(IAudioMediaStream *iface,
        IMultiMediaStream **multi_media_stream)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, multi_media_stream);

    if (!multi_media_stream)
        return E_POINTER;

    IMultiMediaStream_AddRef(This->parent);
    *multi_media_stream = This->parent;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_GetInformation(IAudioMediaStream *iface,
        MSPID *purpose_id, STREAM_TYPE *type)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p,%p)\n", iface, This, purpose_id, type);

    if (purpose_id)
        *purpose_id = This->purpose_id;
    if (type)
        *type = This->stream_type;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_SetSameFormat(IAudioMediaStream *iface,
        IMediaStream *stream_format, DWORD flags)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x) stub!\n", iface, This, stream_format, flags);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_AllocateSample(IAudioMediaStream *iface,
        DWORD flags, IStreamSample **sample)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%x,%p) stub!\n", iface, This, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_CreateSharedSample(IAudioMediaStream *iface,
        IStreamSample *existing_sample, DWORD flags, IStreamSample **sample)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p,%x,%p) stub!\n", iface, This, existing_sample, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_SendEndOfStream(IAudioMediaStream *iface,
        DWORD flags)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%x) stub!\n", iface, This, flags);

    return S_FALSE;
}

/*** IAudioMediaStream methods ***/
static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_GetFormat(IAudioMediaStream *iface, WAVEFORMATEX *wave_format_current)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", iface, This, wave_format_current);

    if (!wave_format_current)
        return E_POINTER;

    return MS_E_NOSTREAM;

}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_SetFormat(IAudioMediaStream *iface, const WAVEFORMATEX *wave_format)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", iface, This, wave_format);

    return E_NOTIMPL;
}

static HRESULT WINAPI AudioMediaStreamImpl_IAudioMediaStream_CreateSample(IAudioMediaStream *iface, IAudioData *audio_data,
                                                         DWORD flags, IAudioStreamSample **sample)
{
    AudioMediaStreamImpl *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p,%u,%p)\n", iface, This, audio_data, flags, sample);

    if (!audio_data)
        return E_POINTER;

    return audiostreamsample_create(iface, audio_data, sample);
}

static const struct IAudioMediaStreamVtbl AudioMediaStreamImpl_IAudioMediaStream_Vtbl =
{
    /*** IUnknown methods ***/
    AudioMediaStreamImpl_IAudioMediaStream_QueryInterface,
    AudioMediaStreamImpl_IAudioMediaStream_AddRef,
    AudioMediaStreamImpl_IAudioMediaStream_Release,
    /*** IMediaStream methods ***/
    AudioMediaStreamImpl_IAudioMediaStream_GetMultiMediaStream,
    AudioMediaStreamImpl_IAudioMediaStream_GetInformation,
    AudioMediaStreamImpl_IAudioMediaStream_SetSameFormat,
    AudioMediaStreamImpl_IAudioMediaStream_AllocateSample,
    AudioMediaStreamImpl_IAudioMediaStream_CreateSharedSample,
    AudioMediaStreamImpl_IAudioMediaStream_SendEndOfStream,
    /*** IAudioMediaStream methods ***/
    AudioMediaStreamImpl_IAudioMediaStream_GetFormat,
    AudioMediaStreamImpl_IAudioMediaStream_SetFormat,
    AudioMediaStreamImpl_IAudioMediaStream_CreateSample
};

static inline AudioMediaStreamInputPin *impl_from_AudioMediaStreamInputPin_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, AudioMediaStreamInputPin, pin.pin.IPin_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI AudioMediaStreamInputPin_IPin_QueryInterface(IPin *iface, REFIID riid, void **ret_iface)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(iface);

    return IAMMediaStream_QueryInterface(&This->parent->IAMMediaStream_iface, riid, ret_iface);
}

static ULONG WINAPI AudioMediaStreamInputPin_IPin_AddRef(IPin *iface)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(iface);

    return IAMMediaStream_AddRef(&This->parent->IAMMediaStream_iface);
}

static ULONG WINAPI AudioMediaStreamInputPin_IPin_Release(IPin *iface)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(iface);

    return IAMMediaStream_Release(&This->parent->IAMMediaStream_iface);
}

static const IPinVtbl AudioMediaStreamInputPin_IPin_Vtbl =
{
    AudioMediaStreamInputPin_IPin_QueryInterface,
    AudioMediaStreamInputPin_IPin_AddRef,
    AudioMediaStreamInputPin_IPin_Release,
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
    BaseInputPinImpl_NewSegment,
};

static HRESULT WINAPI AudioMediaStreamInputPin_CheckMediaType(BasePin *base, const AM_MEDIA_TYPE *media_type)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(&base->IPin_iface);

    TRACE("(%p)->(%p)\n", This, media_type);

    if (IsEqualGUID(&media_type->majortype, &MEDIATYPE_Audio))
    {
        if (IsEqualGUID(&media_type->subtype, &MEDIASUBTYPE_PCM))
        {
            TRACE("Audio sub-type %s matches\n", debugstr_guid(&media_type->subtype));
            return S_OK;
        }
    }

    return S_OK;
}

static LONG WINAPI AudioMediaStreamInputPin_GetMediaTypeVersion(BasePin *base)
{
    return 0;
}

static HRESULT WINAPI AudioMediaStreamInputPin_GetMediaType(BasePin *base, int index, AM_MEDIA_TYPE *media_type)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(&base->IPin_iface);

    TRACE("(%p)->(%d,%p)\n", This, index, media_type);

    /* FIXME: Reset structure as we only fill majortype and minortype for now */
    ZeroMemory(media_type, sizeof(*media_type));

    if (index)
        return S_FALSE;

    media_type->majortype = MEDIATYPE_Audio;
    media_type->subtype = MEDIASUBTYPE_PCM;

    return S_OK;
}

static HRESULT WINAPI AudioMediaStreamInputPin_Receive(BaseInputPin *base, IMediaSample *sample)
{
    AudioMediaStreamInputPin *This = impl_from_AudioMediaStreamInputPin_IPin(&base->pin.IPin_iface);

    FIXME("(%p)->(%p) stub!\n", This, sample);

    return E_NOTIMPL;
}

static const BaseInputPinFuncTable AudioMediaStreamInputPin_FuncTable =
{
    {
        AudioMediaStreamInputPin_CheckMediaType,
        NULL,
        AudioMediaStreamInputPin_GetMediaTypeVersion,
        AudioMediaStreamInputPin_GetMediaType,
    },
    AudioMediaStreamInputPin_Receive,
};

HRESULT audiomediastream_create(IMultiMediaStream *parent, const MSPID *purpose_id,
        STREAM_TYPE stream_type, IAMMediaStream **media_stream)
{
    AudioMediaStreamImpl *object;
    PIN_INFO pin_info;
    HRESULT hr;

    TRACE("(%p,%s,%p)\n", parent, debugstr_guid(purpose_id), media_stream);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(AudioMediaStreamImpl));
    if (!object)
        return E_OUTOFMEMORY;

    object->IAMMediaStream_iface.lpVtbl = &AudioMediaStreamImpl_IAMMediaStream_Vtbl;
    object->IAudioMediaStream_iface.lpVtbl = &AudioMediaStreamImpl_IAudioMediaStream_Vtbl;
    object->ref = 1;

    InitializeCriticalSection(&object->critical_section);

    pin_info.pFilter = NULL;
    pin_info.dir = PINDIR_INPUT;
    pin_info.achName[0] = 'I';
    StringFromGUID2(purpose_id, pin_info.achName + 1, MAX_PIN_NAME - 1);
    hr = BaseInputPin_Construct(&AudioMediaStreamInputPin_IPin_Vtbl,
        sizeof(AudioMediaStreamInputPin), &pin_info, &AudioMediaStreamInputPin_FuncTable,
        &object->critical_section, NULL, (IPin **)&object->input_pin);
    if (FAILED(hr))
        goto out_object;

    object->input_pin->parent = object;

    object->parent = parent;
    object->purpose_id = *purpose_id;
    object->stream_type = stream_type;

    *media_stream = &object->IAMMediaStream_iface;

    return S_OK;

out_object:
    HeapFree(GetProcessHeap(), 0, object);

    return hr;
}

typedef struct {
    IDirectDrawStreamSample IDirectDrawStreamSample_iface;
    LONG ref;
    IMediaStream *parent;
    IDirectDrawSurface *surface;
    RECT rect;
} IDirectDrawStreamSampleImpl;

static inline IDirectDrawStreamSampleImpl *impl_from_IDirectDrawStreamSample(IDirectDrawStreamSample *iface)
{
    return CONTAINING_RECORD(iface, IDirectDrawStreamSampleImpl, IDirectDrawStreamSample_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IDirectDrawStreamSampleImpl_QueryInterface(IDirectDrawStreamSample *iface,
        REFIID riid, void **ret_iface)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IStreamSample) ||
        IsEqualGUID(riid, &IID_IDirectDrawStreamSample))
    {
        IDirectDrawStreamSample_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;

    ERR("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectDrawStreamSampleImpl_AddRef(IDirectDrawStreamSample *iface)
{
    IDirectDrawStreamSampleImpl *This = impl_from_IDirectDrawStreamSample(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirectDrawStreamSampleImpl_Release(IDirectDrawStreamSample *iface)
{
    IDirectDrawStreamSampleImpl *This = impl_from_IDirectDrawStreamSample(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
    {
        if (This->surface)
            IDirectDrawSurface_Release(This->surface);
        IMediaStream_Release(This->parent);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IStreamSample methods ***/
static HRESULT WINAPI IDirectDrawStreamSampleImpl_GetMediaStream(IDirectDrawStreamSample *iface, IMediaStream **media_stream)
{
    FIXME("(%p)->(%p): stub\n", iface, media_stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawStreamSampleImpl_GetSampleTimes(IDirectDrawStreamSample *iface, STREAM_TIME *start_time,
                                                                 STREAM_TIME *end_time, STREAM_TIME *current_time)
{
    FIXME("(%p)->(%p,%p,%p): stub\n", iface, start_time, end_time, current_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawStreamSampleImpl_SetSampleTimes(IDirectDrawStreamSample *iface, const STREAM_TIME *start_time,
                                                                 const STREAM_TIME *end_time)
{
    FIXME("(%p)->(%p,%p): stub\n", iface, start_time, end_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI IDirectDrawStreamSampleImpl_Update(IDirectDrawStreamSample *iface, DWORD flags, HANDLE event,
                                                         PAPCFUNC func_APC, DWORD APC_data)
{
    FIXME("(%p)->(%x,%p,%p,%u): stub\n", iface, flags, event, func_APC, APC_data);

    return S_OK;
}

static HRESULT WINAPI IDirectDrawStreamSampleImpl_CompletionStatus(IDirectDrawStreamSample *iface, DWORD flags, DWORD milliseconds)
{
    FIXME("(%p)->(%x,%u): stub\n", iface, flags, milliseconds);

    return E_NOTIMPL;
}

/*** IDirectDrawStreamSample methods ***/
static HRESULT WINAPI IDirectDrawStreamSampleImpl_GetSurface(IDirectDrawStreamSample *iface, IDirectDrawSurface **ddraw_surface,
                                                             RECT *rect)
{
    IDirectDrawStreamSampleImpl *This = impl_from_IDirectDrawStreamSample(iface);

    TRACE("(%p)->(%p,%p)\n", iface, ddraw_surface, rect);

    if (ddraw_surface)
    {
        *ddraw_surface = This->surface;
        if (*ddraw_surface)
            IDirectDrawSurface_AddRef(*ddraw_surface);
    }

    if (rect)
        *rect = This->rect;

    return S_OK;
}

static HRESULT WINAPI IDirectDrawStreamSampleImpl_SetRect(IDirectDrawStreamSample *iface, const RECT *rect)
{
    FIXME("(%p)->(%p): stub\n", iface, rect);

    return E_NOTIMPL;
}

static const struct IDirectDrawStreamSampleVtbl DirectDrawStreamSample_Vtbl =
{
    /*** IUnknown methods ***/
    IDirectDrawStreamSampleImpl_QueryInterface,
    IDirectDrawStreamSampleImpl_AddRef,
    IDirectDrawStreamSampleImpl_Release,
    /*** IStreamSample methods ***/
    IDirectDrawStreamSampleImpl_GetMediaStream,
    IDirectDrawStreamSampleImpl_GetSampleTimes,
    IDirectDrawStreamSampleImpl_SetSampleTimes,
    IDirectDrawStreamSampleImpl_Update,
    IDirectDrawStreamSampleImpl_CompletionStatus,
    /*** IDirectDrawStreamSample methods ***/
    IDirectDrawStreamSampleImpl_GetSurface,
    IDirectDrawStreamSampleImpl_SetRect
};

static HRESULT ddrawstreamsample_create(IDirectDrawMediaStream *parent, IDirectDrawSurface *surface,
    const RECT *rect, IDirectDrawStreamSample **ddraw_stream_sample)
{
    IDirectDrawStreamSampleImpl *object;
    HRESULT hr;

    TRACE("(%p)\n", ddraw_stream_sample);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    object->IDirectDrawStreamSample_iface.lpVtbl = &DirectDrawStreamSample_Vtbl;
    object->ref = 1;
    object->parent = (IMediaStream*)parent;
    IMediaStream_AddRef(object->parent);

    if (surface)
    {
        object->surface = surface;
        IDirectDrawSurface_AddRef(surface);
    }
    else
    {
        DDSURFACEDESC desc;
        IDirectDraw *ddraw;

        hr = IDirectDrawMediaStream_GetDirectDraw(parent, &ddraw);
        if (FAILED(hr))
        {
            IDirectDrawStreamSample_Release(&object->IDirectDrawStreamSample_iface);
            return hr;
        }

        desc.dwSize = sizeof(desc);
        desc.dwFlags = DDSD_CAPS|DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT;
        desc.dwHeight = 100;
        desc.dwWidth = 100;
        desc.ddpfPixelFormat.dwSize = sizeof(desc.ddpfPixelFormat);
        desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
        desc.ddpfPixelFormat.dwRGBBitCount = 32;
        desc.ddpfPixelFormat.dwRBitMask = 0xff0000;
        desc.ddpfPixelFormat.dwGBitMask = 0x00ff00;
        desc.ddpfPixelFormat.dwBBitMask = 0x0000ff;
        desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
        desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY|DDSCAPS_OFFSCREENPLAIN;
        desc.lpSurface = NULL;

        hr = IDirectDraw_CreateSurface(ddraw, &desc, &object->surface, NULL);
        IDirectDraw_Release(ddraw);
        if (FAILED(hr))
        {
            ERR("failed to create surface, 0x%08x\n", hr);
            IDirectDrawStreamSample_Release(&object->IDirectDrawStreamSample_iface);
            return hr;
        }
    }

    if (rect)
        object->rect = *rect;
    else if (object->surface)
    {
        DDSURFACEDESC desc = { sizeof(desc) };
        hr = IDirectDrawSurface_GetSurfaceDesc(object->surface, &desc);
        if (hr == S_OK)
            SetRect(&object->rect, 0, 0, desc.dwWidth, desc.dwHeight);
    }

    *ddraw_stream_sample = &object->IDirectDrawStreamSample_iface;

    return S_OK;
}

typedef struct {
    IAudioStreamSample IAudioStreamSample_iface;
    LONG ref;
    IMediaStream *parent;
    IAudioData *audio_data;
} IAudioStreamSampleImpl;

static inline IAudioStreamSampleImpl *impl_from_IAudioStreamSample(IAudioStreamSample *iface)
{
    return CONTAINING_RECORD(iface, IAudioStreamSampleImpl, IAudioStreamSample_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI IAudioStreamSampleImpl_QueryInterface(IAudioStreamSample *iface,
        REFIID riid, void **ret_iface)
{
    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(riid), ret_iface);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IStreamSample) ||
        IsEqualGUID(riid, &IID_IAudioStreamSample))
    {
        IAudioStreamSample_AddRef(iface);
        *ret_iface = iface;
        return S_OK;
    }

    *ret_iface = NULL;

    ERR("(%p)->(%s,%p),not found\n", iface, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAudioStreamSampleImpl_AddRef(IAudioStreamSample *iface)
{
    IAudioStreamSampleImpl *This = impl_from_IAudioStreamSample(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI IAudioStreamSampleImpl_Release(IAudioStreamSample *iface)
{
    IAudioStreamSampleImpl *This = impl_from_IAudioStreamSample(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IStreamSample methods ***/
static HRESULT WINAPI IAudioStreamSampleImpl_GetMediaStream(IAudioStreamSample *iface, IMediaStream **media_stream)
{
    FIXME("(%p)->(%p): stub\n", iface, media_stream);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_GetSampleTimes(IAudioStreamSample *iface, STREAM_TIME *start_time,
                                                                 STREAM_TIME *end_time, STREAM_TIME *current_time)
{
    FIXME("(%p)->(%p,%p,%p): stub\n", iface, start_time, end_time, current_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_SetSampleTimes(IAudioStreamSample *iface, const STREAM_TIME *start_time,
                                                                 const STREAM_TIME *end_time)
{
    FIXME("(%p)->(%p,%p): stub\n", iface, start_time, end_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_Update(IAudioStreamSample *iface, DWORD flags, HANDLE event,
                                                         PAPCFUNC func_APC, DWORD APC_data)
{
    FIXME("(%p)->(%x,%p,%p,%u): stub\n", iface, flags, event, func_APC, APC_data);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAudioStreamSampleImpl_CompletionStatus(IAudioStreamSample *iface, DWORD flags, DWORD milliseconds)
{
    FIXME("(%p)->(%x,%u): stub\n", iface, flags, milliseconds);

    return E_NOTIMPL;
}

/*** IAudioStreamSample methods ***/
static HRESULT WINAPI IAudioStreamSampleImpl_GetAudioData(IAudioStreamSample *iface, IAudioData **audio_data)
{
    FIXME("(%p)->(%p): stub\n", iface, audio_data);

    return E_NOTIMPL;
}

static const struct IAudioStreamSampleVtbl AudioStreamSample_Vtbl =
{
    /*** IUnknown methods ***/
    IAudioStreamSampleImpl_QueryInterface,
    IAudioStreamSampleImpl_AddRef,
    IAudioStreamSampleImpl_Release,
    /*** IStreamSample methods ***/
    IAudioStreamSampleImpl_GetMediaStream,
    IAudioStreamSampleImpl_GetSampleTimes,
    IAudioStreamSampleImpl_SetSampleTimes,
    IAudioStreamSampleImpl_Update,
    IAudioStreamSampleImpl_CompletionStatus,
    /*** IAudioStreamSample methods ***/
    IAudioStreamSampleImpl_GetAudioData
};

static HRESULT audiostreamsample_create(IAudioMediaStream *parent, IAudioData *audio_data, IAudioStreamSample **audio_stream_sample)
{
    IAudioStreamSampleImpl *object;

    TRACE("(%p)\n", audio_stream_sample);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IAudioStreamSampleImpl));
    if (!object)
        return E_OUTOFMEMORY;

    object->IAudioStreamSample_iface.lpVtbl = &AudioStreamSample_Vtbl;
    object->ref = 1;
    object->parent = (IMediaStream*)parent;
    object->audio_data = audio_data;

    *audio_stream_sample = (IAudioStreamSample*)&object->IAudioStreamSample_iface;

    return S_OK;
}
