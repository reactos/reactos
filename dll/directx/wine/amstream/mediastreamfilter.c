/*
 * Implementation of MediaStream Filter
 *
 * Copyright 2008, 2012 Christian Costa
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

#include "amstream_private.h"

#include <wine/strmbase.h>

typedef struct MediaStreamFilter_InputPin
{
    BaseInputPin pin;
} MediaStreamFilter_InputPin;

static const IPinVtbl MediaStreamFilter_InputPin_Vtbl =
{
    BaseInputPinImpl_QueryInterface,
    BasePinImpl_AddRef,
    BaseInputPinImpl_Release,
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
    BasePinImpl_NewSegment
};

typedef struct {
    BaseFilter filter;
    ULONG nb_streams;
    IMediaStream** streams;
    IPin** pins;
} IMediaStreamFilterImpl;

static inline IMediaStreamFilterImpl *impl_from_IMediaStreamFilter(IMediaStreamFilter *iface)
{
    return CONTAINING_RECORD(iface, IMediaStreamFilterImpl, filter);
}

static HRESULT WINAPI BasePinImpl_CheckMediaType(BasePin *This, const AM_MEDIA_TYPE *pmt)
{
    IMediaStreamFilterImpl *filter = impl_from_IMediaStreamFilter((IMediaStreamFilter*)This->pinInfo.pFilter);
    MSPID purpose_id;
    ULONG i;

    TRACE("Checking media type %s - %s\n", debugstr_guid(&pmt->majortype), debugstr_guid(&pmt->subtype));

    /* Find which stream is associated with the pin */
    for (i = 0; i < filter->nb_streams; i++)
        if (&This->IPin_iface == filter->pins[i])
            break;

    if (i == filter->nb_streams)
        return S_FALSE;

    if (FAILED(IMediaStream_GetInformation(filter->streams[i], &purpose_id, NULL)))
        return S_FALSE;

    TRACE("Checking stream with purpose id %s\n", debugstr_guid(&purpose_id));

    if (IsEqualGUID(&purpose_id, &MSPID_PrimaryVideo) && IsEqualGUID(&pmt->majortype, &MEDIATYPE_Video))
    {
        if (IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_RGB1) ||
            IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_RGB4) ||
            IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_RGB8)  ||
            IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_RGB565) ||
            IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_RGB555) ||
            IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_RGB24) ||
            IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_RGB32))
        {
            TRACE("Video sub-type %s matches\n", debugstr_guid(&pmt->subtype));
            return S_OK;
        }
    }
    else if (IsEqualGUID(&purpose_id, &MSPID_PrimaryAudio) && IsEqualGUID(&pmt->majortype, &MEDIATYPE_Audio))
    {
        if (IsEqualGUID(&pmt->subtype, &MEDIASUBTYPE_PCM))
        {
            TRACE("Audio sub-type %s matches\n", debugstr_guid(&pmt->subtype));
            return S_OK;
        }
    }

    return S_FALSE;
}

static LONG WINAPI BasePinImp_GetMediaTypeVersion(BasePin *This)
{
    return 0;
}

static HRESULT WINAPI BasePinImp_GetMediaType(BasePin *This, int index, AM_MEDIA_TYPE *amt)
{
    IMediaStreamFilterImpl *filter = (IMediaStreamFilterImpl*)This->pinInfo.pFilter;
    MSPID purpose_id;
    ULONG i;

    /* FIXME: Reset structure as we only fill majortype and minortype for now */
    ZeroMemory(amt, sizeof(*amt));

    /* Find which stream is associated with the pin */
    for (i = 0; i < filter->nb_streams; i++)
        if (&This->IPin_iface == filter->pins[i])
            break;

    if (i == filter->nb_streams)
        return S_FALSE;

    if (FAILED(IMediaStream_GetInformation(filter->streams[i], &purpose_id, NULL)))
        return S_FALSE;

    TRACE("Processing stream with purpose id %s\n", debugstr_guid(&purpose_id));

    if (IsEqualGUID(&purpose_id, &MSPID_PrimaryVideo))
    {
        amt->majortype = MEDIATYPE_Video;

        switch (index)
        {
            case 0:
                amt->subtype = MEDIASUBTYPE_RGB1;
                break;
            case 1:
                amt->subtype = MEDIASUBTYPE_RGB4;
                break;
            case 2:
                amt->subtype = MEDIASUBTYPE_RGB8;
                break;
            case 3:
                amt->subtype = MEDIASUBTYPE_RGB565;
                break;
            case 4:
                amt->subtype = MEDIASUBTYPE_RGB555;
                break;
            case 5:
                amt->subtype = MEDIASUBTYPE_RGB24;
                break;
            case 6:
                amt->subtype = MEDIASUBTYPE_RGB32;
                break;
            default:
                return S_FALSE;
        }
    }
    else if (IsEqualGUID(&purpose_id, &MSPID_PrimaryAudio))
    {
        if (index)
            return S_FALSE;

         amt->majortype = MEDIATYPE_Audio;
         amt->subtype = MEDIASUBTYPE_PCM;
    }

    return S_OK;
}

static const BaseInputPinFuncTable input_BaseInputFuncTable = {
    {
        BasePinImpl_CheckMediaType,
        NULL,
        BasePinImp_GetMediaTypeVersion,
        BasePinImp_GetMediaType
    },
    NULL
};

/*** IUnknown methods ***/

static HRESULT WINAPI MediaStreamFilterImpl_QueryInterface(IMediaStreamFilter *iface, REFIID riid, void **ret_iface)
{
    TRACE("(%p)->(%s, %p)\n", iface, debugstr_guid(riid), ret_iface);

    *ret_iface = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IPersist) ||
        IsEqualIID(riid, &IID_IMediaFilter) ||
        IsEqualIID(riid, &IID_IBaseFilter) ||
        IsEqualIID(riid, &IID_IMediaStreamFilter))
        *ret_iface = iface;

    if (*ret_iface)
    {
        IMediaStreamFilter_AddRef(*ret_iface);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI MediaStreamFilterImpl_AddRef(IMediaStreamFilter *iface)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    ULONG ref = BaseFilterImpl_AddRef(&This->filter.IBaseFilter_iface);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    return ref;
}

static ULONG WINAPI MediaStreamFilterImpl_Release(IMediaStreamFilter *iface)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    ULONG ref = InterlockedDecrement(&This->filter.refCount);

    TRACE("(%p)->(): new ref = %u\n", iface, ref);

    if (!ref)
    {
        ULONG i;
        for (i = 0; i < This->nb_streams; i++)
        {
            IMediaStream_Release(This->streams[i]);
            IPin_Release(This->pins[i]);
        }
        CoTaskMemFree(This->streams);
        CoTaskMemFree(This->pins);
        BaseFilter_Destroy(&This->filter);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IPersist methods ***/

static HRESULT WINAPI MediaStreamFilterImpl_GetClassID(IMediaStreamFilter *iface, CLSID *clsid)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    return BaseFilterImpl_GetClassID(&This->filter.IBaseFilter_iface, clsid);
}

/*** IBaseFilter methods ***/

static HRESULT WINAPI MediaStreamFilterImpl_Stop(IMediaStreamFilter *iface)
{
    FIXME("(%p)->(): Stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_Pause(IMediaStreamFilter *iface)
{
    FIXME("(%p)->(): Stub!\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_Run(IMediaStreamFilter *iface, REFERENCE_TIME start)
{
    FIXME("(%p)->(%s): Stub!\n", iface, wine_dbgstr_longlong(start));

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_GetState(IMediaStreamFilter *iface, DWORD ms_timeout, FILTER_STATE *state)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    return BaseFilterImpl_GetState(&This->filter.IBaseFilter_iface, ms_timeout, state);
}

static HRESULT WINAPI MediaStreamFilterImpl_SetSyncSource(IMediaStreamFilter *iface, IReferenceClock *clock)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    return BaseFilterImpl_SetSyncSource(&This->filter.IBaseFilter_iface, clock);
}

static HRESULT WINAPI MediaStreamFilterImpl_GetSyncSource(IMediaStreamFilter *iface, IReferenceClock **clock)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    return BaseFilterImpl_GetSyncSource(&This->filter.IBaseFilter_iface, clock);
}

static HRESULT WINAPI MediaStreamFilterImpl_EnumPins(IMediaStreamFilter *iface, IEnumPins **enum_pins)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    return BaseFilterImpl_EnumPins(&This->filter.IBaseFilter_iface, enum_pins);
}

static HRESULT WINAPI MediaStreamFilterImpl_FindPin(IMediaStreamFilter *iface, LPCWSTR id, IPin **pin)
{
    FIXME("(%p)->(%s,%p): Stub!\n", iface, debugstr_w(id), pin);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaStreamFilterImpl_QueryFilterInfo(IMediaStreamFilter *iface, FILTER_INFO *info)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    return BaseFilterImpl_QueryFilterInfo(&This->filter.IBaseFilter_iface, info);
}

static HRESULT WINAPI MediaStreamFilterImpl_JoinFilterGraph(IMediaStreamFilter *iface, IFilterGraph *graph, LPCWSTR name)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    return BaseFilterImpl_JoinFilterGraph(&This->filter.IBaseFilter_iface, graph, name);
}

static HRESULT WINAPI MediaStreamFilterImpl_QueryVendorInfo(IMediaStreamFilter *iface, LPWSTR *vendor_info)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    return BaseFilterImpl_QueryVendorInfo(&This->filter.IBaseFilter_iface, vendor_info);
}

/*** IMediaStreamFilter methods ***/

static HRESULT WINAPI MediaStreamFilterImpl_AddMediaStream(IMediaStreamFilter* iface, IAMMediaStream *pAMMediaStream)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    IMediaStream** streams;
    IPin** pins;
    MediaStreamFilter_InputPin* pin;
    HRESULT hr;
    PIN_INFO info;
    MSPID purpose_id;

    TRACE("(%p)->(%p)\n", iface, pAMMediaStream);

    streams = CoTaskMemRealloc(This->streams, (This->nb_streams + 1) * sizeof(IMediaStream*));
    if (!streams)
        return E_OUTOFMEMORY;
    This->streams = streams;
    pins = CoTaskMemRealloc(This->pins, (This->nb_streams + 1) * sizeof(IPin*));
    if (!pins)
        return E_OUTOFMEMORY;
    This->pins = pins;
    info.pFilter = (IBaseFilter*)&This->filter;
    info.dir = PINDIR_INPUT;
    hr = IAMMediaStream_GetInformation(pAMMediaStream, &purpose_id, NULL);
    if (FAILED(hr))
        return hr;
    /* Pin name is "I{guid MSPID_PrimaryVideo or MSPID_PrimaryAudio}" */
    info.achName[0] = 'I';
    StringFromGUID2(&purpose_id, info.achName + 1, 40);
    hr = BaseInputPin_Construct(&MediaStreamFilter_InputPin_Vtbl, sizeof(BaseInputPin), &info,
            &input_BaseInputFuncTable, &This->filter.csFilter, NULL, &This->pins[This->nb_streams]);
    if (FAILED(hr))
        return hr;

    pin = (MediaStreamFilter_InputPin*)This->pins[This->nb_streams];
    pin->pin.pin.pinInfo.pFilter = (LPVOID)This;
    This->streams[This->nb_streams] = (IMediaStream*)pAMMediaStream;
    This->nb_streams++;

    IMediaStream_AddRef((IMediaStream*)pAMMediaStream);

    return S_OK;
}

static HRESULT WINAPI MediaStreamFilterImpl_GetMediaStream(IMediaStreamFilter* iface, REFMSPID idPurpose, IMediaStream **ppMediaStream)
{
    IMediaStreamFilterImpl *This = impl_from_IMediaStreamFilter(iface);
    MSPID purpose_id;
    unsigned int i;

    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(idPurpose), ppMediaStream);

    for (i = 0; i < This->nb_streams; i++)
    {
        IMediaStream_GetInformation(This->streams[i], &purpose_id, NULL);
        if (IsEqualIID(&purpose_id, idPurpose))
        {
            *ppMediaStream = This->streams[i];
            IMediaStream_AddRef(*ppMediaStream);
            return S_OK;
        }
    }

    return MS_E_NOSTREAM;
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

static IPin* WINAPI MediaStreamFilterImpl_GetPin(BaseFilter *iface, int pos)
{
    IMediaStreamFilterImpl* This = (IMediaStreamFilterImpl*)iface;

    if (pos < This->nb_streams)
    {
        IPin_AddRef(This->pins[pos]);
        return This->pins[pos];
    }

    return NULL;
}

static LONG WINAPI MediaStreamFilterImpl_GetPinCount(BaseFilter *iface)
{
    IMediaStreamFilterImpl* This = (IMediaStreamFilterImpl*)iface;

    return This->nb_streams;
}

static const BaseFilterFuncTable BaseFuncTable = {
    MediaStreamFilterImpl_GetPin,
    MediaStreamFilterImpl_GetPinCount
};

HRESULT MediaStreamFilter_create(IUnknown *pUnkOuter, void **ppObj)
{
    IMediaStreamFilterImpl* object;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IMediaStreamFilterImpl));
    if (!object)
        return E_OUTOFMEMORY;

    BaseFilter_Init(&object->filter, (IBaseFilterVtbl*)&MediaStreamFilter_Vtbl, &CLSID_MediaStreamFilter, (DWORD_PTR)(__FILE__ ": MediaStreamFilterImpl.csFilter"), &BaseFuncTable);

    *ppObj = object;

    return S_OK;
}
