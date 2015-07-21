/*
 * Implementation of IAMMultiMediaStream Interface
 *
 * Copyright 2004, 2012 Christian Costa
 * Copyright 2006 Ivan Leo Puoti
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

typedef struct {
    IAMMultiMediaStream IAMMultiMediaStream_iface;
    LONG ref;
    IGraphBuilder* pFilterGraph;
    IMediaSeeking* media_seeking;
    IMediaControl* media_control;
    IMediaStreamFilter *media_stream_filter;
    IPin* ipin;
    ULONG nbStreams;
    IAMMediaStream **pStreams;
    STREAM_TYPE StreamType;
    OAEVENT event;
} IAMMultiMediaStreamImpl;

static inline IAMMultiMediaStreamImpl *impl_from_IAMMultiMediaStream(IAMMultiMediaStream *iface)
{
    return CONTAINING_RECORD(iface, IAMMultiMediaStreamImpl, IAMMultiMediaStream_iface);
}

static const struct IAMMultiMediaStreamVtbl AM_Vtbl;

HRESULT AM_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    IAMMultiMediaStreamImpl* object; 

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IAMMultiMediaStreamImpl));
    if (!object)
        return E_OUTOFMEMORY;

    object->IAMMultiMediaStream_iface.lpVtbl = &AM_Vtbl;
    object->ref = 1;

    *ppObj = object;

    return S_OK;
}

/*** IUnknown methods ***/
static HRESULT WINAPI IAMMultiMediaStreamImpl_QueryInterface(IAMMultiMediaStream* iface, REFIID riid, void** ppvObject)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IMultiMediaStream) ||
        IsEqualGUID(riid, &IID_IAMMultiMediaStream))
    {
        IAMMultiMediaStream_AddRef(iface);
        *ppvObject = iface;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);

    return E_NOINTERFACE;
}

static ULONG WINAPI IAMMultiMediaStreamImpl_AddRef(IAMMultiMediaStream* iface)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    TRACE("(%p/%p)\n", iface, This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IAMMultiMediaStreamImpl_Release(IAMMultiMediaStream* iface)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    ULONG i;

    TRACE("(%p/%p)\n", iface, This);

    if (!ref)
    {
        for(i = 0; i < This->nbStreams; i++)
            IAMMediaStream_Release(This->pStreams[i]);
        CoTaskMemFree(This->pStreams);
        if (This->ipin)
            IPin_Release(This->ipin);
        if (This->media_stream_filter)
            IMediaStreamFilter_Release(This->media_stream_filter);
        if (This->media_seeking)
            IMediaSeeking_Release(This->media_seeking);
        if (This->media_control)
            IMediaControl_Release(This->media_control);
        if (This->pFilterGraph)
            IGraphBuilder_Release(This->pFilterGraph);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/*** IMultiMediaStream methods ***/
static HRESULT WINAPI IAMMultiMediaStreamImpl_GetInformation(IAMMultiMediaStream* iface, DWORD* pdwFlags, STREAM_TYPE* pStreamType)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pdwFlags, pStreamType);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetMediaStream(IAMMultiMediaStream* iface, REFMSPID idPurpose, IMediaStream** ppMediaStream)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);
    MSPID PurposeId;
    unsigned int i;

    TRACE("(%p/%p)->(%s,%p)\n", This, iface, debugstr_guid(idPurpose), ppMediaStream);

    for (i = 0; i < This->nbStreams; i++)
    {
        IAMMediaStream_GetInformation(This->pStreams[i], &PurposeId, NULL);
        if (IsEqualIID(&PurposeId, idPurpose))
        {
            *ppMediaStream = (IMediaStream*)This->pStreams[i];
            IMediaStream_AddRef(*ppMediaStream);
            return S_OK;
        }
    }

    return MS_E_NOSTREAM;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_EnumMediaStreams(IAMMultiMediaStream* iface, LONG Index, IMediaStream** ppMediaStream)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%d,%p) stub!\n", This, iface, Index, ppMediaStream);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetState(IAMMultiMediaStream* iface, STREAM_STATE* pCurrentState)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, pCurrentState);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_SetState(IAMMultiMediaStream* iface, STREAM_STATE new_state)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p/%p)->(%u)\n", This, iface, new_state);

    if (new_state == STREAMSTATE_RUN)
        hr = IMediaControl_Run(This->media_control);
    else if (new_state == STREAMSTATE_STOP)
        hr = IMediaControl_Stop(This->media_control);

    return hr;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetTime(IAMMultiMediaStream* iface, STREAM_TIME* pCurrentTime)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, pCurrentTime);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetDuration(IAMMultiMediaStream* iface, STREAM_TIME* pDuration)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, pDuration);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_Seek(IAMMultiMediaStream* iface, STREAM_TIME seek_time)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(seek_time));

    return IMediaSeeking_SetPositions(This->media_seeking, &seek_time, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetEndOfStream(IAMMultiMediaStream* iface, HANDLE* phEOS)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p) stub!\n", This, iface, phEOS);

    return E_NOTIMPL;
}

/*** IAMMultiMediaStream methods ***/
static HRESULT WINAPI IAMMultiMediaStreamImpl_Initialize(IAMMultiMediaStream* iface, STREAM_TYPE StreamType, DWORD dwFlags, IGraphBuilder* pFilterGraph)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);
    HRESULT hr = S_OK;
    const WCHAR filternameW[] = {'M','e','d','i','a','S','t','r','e','a','m','F','i','l','t','e','r',0};

    TRACE("(%p/%p)->(%x,%x,%p)\n", This, iface, (DWORD)StreamType, dwFlags, pFilterGraph);

    if (pFilterGraph)
    {
        This->pFilterGraph = pFilterGraph;
        IGraphBuilder_AddRef(This->pFilterGraph);
    }
    else
    {
        hr = CoCreateInstance(&CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (LPVOID*)&This->pFilterGraph);
    }

    if (SUCCEEDED(hr))
    {
        This->StreamType = StreamType;
        hr = IGraphBuilder_QueryInterface(This->pFilterGraph, &IID_IMediaSeeking, (void**)&This->media_seeking);
        if (SUCCEEDED(hr))
            hr = IGraphBuilder_QueryInterface(This->pFilterGraph, &IID_IMediaControl, (void**)&This->media_control);
        if (SUCCEEDED(hr))
            hr = CoCreateInstance(&CLSID_MediaStreamFilter, NULL, CLSCTX_INPROC_SERVER, &IID_IMediaStreamFilter, (void**)&This->media_stream_filter);
        if (SUCCEEDED(hr))
            hr = IGraphBuilder_AddFilter(This->pFilterGraph, (IBaseFilter*)This->media_stream_filter, filternameW);
        if (SUCCEEDED(hr))
        {
            IMediaEventEx* media_event = NULL;
            hr = IGraphBuilder_QueryInterface(This->pFilterGraph, &IID_IMediaEventEx, (void**)&media_event);
            if (SUCCEEDED(hr))
                hr = IMediaEventEx_GetEventHandle(media_event, &This->event);
            if (SUCCEEDED(hr))
                hr = IMediaEventEx_SetNotifyFlags(media_event, AM_MEDIAEVENT_NONOTIFY);
            if (media_event)
                IMediaEventEx_Release(media_event);
        }
    }

    if (FAILED(hr))
    {
        if (This->media_stream_filter)
            IMediaStreamFilter_Release(This->media_stream_filter);
        This->media_stream_filter = NULL;
        if (This->media_seeking)
            IMediaSeeking_Release(This->media_seeking);
        This->media_seeking = NULL;
        if (This->media_control)
            IMediaControl_Release(This->media_control);
        This->media_control = NULL;
        if (This->pFilterGraph)
            IGraphBuilder_Release(This->pFilterGraph);
        This->pFilterGraph = NULL;
    }

    return hr;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetFilterGraph(IAMMultiMediaStream* iface, IGraphBuilder** ppGraphBuilder)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppGraphBuilder);

    if (!ppGraphBuilder)
        return E_POINTER;

    if (This->pFilterGraph)
        return IGraphBuilder_QueryInterface(This->pFilterGraph, &IID_IGraphBuilder, (void**)ppGraphBuilder);
    else
        *ppGraphBuilder = NULL;

    return S_OK;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_GetFilter(IAMMultiMediaStream* iface, IMediaStreamFilter** ppFilter)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppFilter);

    if (!ppFilter)
        return E_POINTER;

    *ppFilter = This->media_stream_filter;
    if (*ppFilter)
        IMediaStreamFilter_AddRef(*ppFilter);

    return S_OK;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_AddMediaStream(IAMMultiMediaStream* iface, IUnknown* stream_object, const MSPID* PurposeId,
                                          DWORD dwFlags, IMediaStream** ppNewStream)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);
    HRESULT hr;
    IAMMediaStream* pStream;
    IAMMediaStream** pNewStreams;

    TRACE("(%p/%p)->(%p,%s,%x,%p)\n", This, iface, stream_object, debugstr_guid(PurposeId), dwFlags, ppNewStream);

    if (!IsEqualGUID(PurposeId, &MSPID_PrimaryVideo) && !IsEqualGUID(PurposeId, &MSPID_PrimaryAudio))
        return MS_E_PURPOSEID;

    if (stream_object)
        FIXME("Specifying a stream object in params is not yet supported\n");

    if (dwFlags & AMMSF_ADDDEFAULTRENDERER)
    {
        if (IsEqualGUID(PurposeId, &MSPID_PrimaryVideo))
        {
            /* Default renderer not supported by video stream */
            return MS_E_PURPOSEID;
        }
        else
        {
            IBaseFilter* dsoundrender_filter;

            /* Create the default renderer for audio */
            hr = CoCreateInstance(&CLSID_DSoundRender, NULL, CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (LPVOID*)&dsoundrender_filter);
            if (SUCCEEDED(hr))
            {
                 hr = IGraphBuilder_AddFilter(This->pFilterGraph, dsoundrender_filter, NULL);
                 IBaseFilter_Release(dsoundrender_filter);
            }

            /* No media stream created when the default renderer is used */
            return hr;
        }
    }

    if (IsEqualGUID(PurposeId, &MSPID_PrimaryVideo))
        hr = ddrawmediastream_create((IMultiMediaStream*)iface, PurposeId, This->StreamType, &pStream);
    else
        hr = audiomediastream_create((IMultiMediaStream*)iface, PurposeId, This->StreamType, &pStream);
    if (SUCCEEDED(hr))
    {
        pNewStreams = CoTaskMemRealloc(This->pStreams, (This->nbStreams+1) * sizeof(IAMMediaStream*));
        if (!pNewStreams)
        {
            IAMMediaStream_Release(pStream);
            return E_OUTOFMEMORY;
        }
        This->pStreams = pNewStreams;
        This->pStreams[This->nbStreams] = pStream;
        This->nbStreams++;

        if (ppNewStream)
            *ppNewStream = (IMediaStream*)pStream;
    }

    if (SUCCEEDED(hr))
    {
        /* Add stream to the media stream filter */
        IMediaStreamFilter_AddMediaStream(This->media_stream_filter, pStream);
    }

    return hr;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_OpenFile(IAMMultiMediaStream* iface, LPCWSTR filename, DWORD flags)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);
    HRESULT ret = S_OK;
    IBaseFilter *BaseFilter = NULL;
    IEnumPins *EnumPins = NULL;
    IPin *ipin;
    PIN_DIRECTION pin_direction;
    const WCHAR sourceW[] = {'S','o','u','r','c','e',0};

    TRACE("(%p/%p)->(%s,%x)\n", This, iface, debugstr_w(filename), flags);

    if (!filename)
        return E_POINTER;

    /* If Initialize was not called before, we do it here */
    if (!This->pFilterGraph)
        ret = IAMMultiMediaStream_Initialize(iface, STREAMTYPE_READ, 0, NULL);

    if (SUCCEEDED(ret))
        ret = IGraphBuilder_AddSourceFilter(This->pFilterGraph, filename, sourceW, &BaseFilter);

    if (SUCCEEDED(ret))
        ret = IBaseFilter_EnumPins(BaseFilter, &EnumPins);

    if (SUCCEEDED(ret))
        ret = IEnumPins_Next(EnumPins, 1, &ipin, NULL);

    if (SUCCEEDED(ret))
    {
        ret = IPin_QueryDirection(ipin, &pin_direction);
        if (ret == S_OK && pin_direction == PINDIR_OUTPUT)
            This->ipin = ipin;
    }

    if (SUCCEEDED(ret) && !(flags & AMMSF_NORENDER))
        ret = IGraphBuilder_Render(This->pFilterGraph, This->ipin);

    if (EnumPins)
        IEnumPins_Release(EnumPins);
    if (BaseFilter)
        IBaseFilter_Release(BaseFilter);
    return ret;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_OpenMoniker(IAMMultiMediaStream* iface, IBindCtx* pCtx, IMoniker* pMoniker, DWORD dwFlags)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p,%p,%x) stub!\n", This, iface, pCtx, pMoniker, dwFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAMMultiMediaStreamImpl_Render(IAMMultiMediaStream* iface, DWORD dwFlags)
{
    IAMMultiMediaStreamImpl *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%x) partial stub!\n", This, iface, dwFlags);

    if(dwFlags != AMMSF_NOCLOCK)
        return E_INVALIDARG;

    return IGraphBuilder_Render(This->pFilterGraph, This->ipin);
}

static const IAMMultiMediaStreamVtbl AM_Vtbl =
{
    IAMMultiMediaStreamImpl_QueryInterface,
    IAMMultiMediaStreamImpl_AddRef,
    IAMMultiMediaStreamImpl_Release,
    IAMMultiMediaStreamImpl_GetInformation,
    IAMMultiMediaStreamImpl_GetMediaStream,
    IAMMultiMediaStreamImpl_EnumMediaStreams,
    IAMMultiMediaStreamImpl_GetState,
    IAMMultiMediaStreamImpl_SetState,
    IAMMultiMediaStreamImpl_GetTime,
    IAMMultiMediaStreamImpl_GetDuration,
    IAMMultiMediaStreamImpl_Seek,
    IAMMultiMediaStreamImpl_GetEndOfStream,
    IAMMultiMediaStreamImpl_Initialize,
    IAMMultiMediaStreamImpl_GetFilterGraph,
    IAMMultiMediaStreamImpl_GetFilter,
    IAMMultiMediaStreamImpl_AddMediaStream,
    IAMMultiMediaStreamImpl_OpenFile,
    IAMMultiMediaStreamImpl_OpenMoniker,
    IAMMultiMediaStreamImpl_Render
};
