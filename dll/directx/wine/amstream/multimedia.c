/*
 * Multimedia stream object
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

#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"

#include "amstream_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct multimedia_stream
{
    IAMMultiMediaStream IAMMultiMediaStream_iface;
    LONG ref;
    IGraphBuilder *graph;
    IMediaSeeking* media_seeking;
    IMediaControl* media_control;
    IMediaStreamFilter *filter;
    IPin* ipin;
    BOOL initialized;
    STREAM_TYPE type;
    OAEVENT event;
    STREAM_STATE state;
};

static inline struct multimedia_stream *impl_from_IAMMultiMediaStream(IAMMultiMediaStream *iface)
{
    return CONTAINING_RECORD(iface, struct multimedia_stream, IAMMultiMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI multimedia_stream_QueryInterface(IAMMultiMediaStream *iface,
        REFIID riid, void **ppvObject)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

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

static ULONG WINAPI multimedia_stream_AddRef(IAMMultiMediaStream *iface)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    TRACE("(%p/%p)\n", iface, This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI multimedia_stream_Release(IAMMultiMediaStream *iface)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)\n", iface, This);

    if (!ref)
    {
        if (This->ipin)
            IPin_Release(This->ipin);
        IMediaStreamFilter_Release(This->filter);
        IMediaStreamFilter_Release(This->filter);
        if (This->media_seeking)
            IMediaSeeking_Release(This->media_seeking);
        if (This->media_control)
            IMediaControl_Release(This->media_control);
        if (This->graph)
            IGraphBuilder_Release(This->graph);
        free(This);
    }

    return ref;
}

/*** IMultiMediaStream methods ***/
static HRESULT WINAPI multimedia_stream_GetInformation(IAMMultiMediaStream *iface,
        DWORD *pdwFlags, STREAM_TYPE *pStreamType)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p,%p) stub!\n", This, iface, pdwFlags, pStreamType);

    return E_NOTIMPL;
}

static HRESULT WINAPI multimedia_stream_GetMediaStream(IAMMultiMediaStream *iface,
        REFMSPID id, IMediaStream **stream)
{
    struct multimedia_stream *mmstream = impl_from_IAMMultiMediaStream(iface);

    TRACE("mmstream %p, id %s, stream %p.\n", mmstream, debugstr_guid(id), stream);

    return IMediaStreamFilter_GetMediaStream(mmstream->filter, id, stream);
}

static HRESULT WINAPI multimedia_stream_EnumMediaStreams(IAMMultiMediaStream *iface,
        LONG index, IMediaStream **stream)
{
    struct multimedia_stream *mmstream = impl_from_IAMMultiMediaStream(iface);

    TRACE("mmstream %p, index %ld, stream %p.\n", mmstream, index, stream);

    return IMediaStreamFilter_EnumMediaStreams(mmstream->filter, index, stream);
}

static HRESULT WINAPI multimedia_stream_GetState(IAMMultiMediaStream *iface, STREAM_STATE *state)
{
    struct multimedia_stream *mmstream = impl_from_IAMMultiMediaStream(iface);

    TRACE("mmstream %p, state %p.\n", mmstream, state);

    *state = mmstream->state;

    return S_OK;
}

static HRESULT WINAPI multimedia_stream_SetState(IAMMultiMediaStream *iface, STREAM_STATE new_state)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p/%p)->(%u)\n", This, iface, new_state);

    if (new_state == STREAMSTATE_RUN)
    {
        hr = IMediaControl_Run(This->media_control);
        if (SUCCEEDED(hr))
        {
            FILTER_STATE state;
            IMediaControl_GetState(This->media_control, INFINITE, (OAFilterState *)&state);
            hr = S_OK;
        }
    }
    else if (new_state == STREAMSTATE_STOP)
        hr = IMediaControl_Stop(This->media_control);

    if (SUCCEEDED(hr))
        This->state = new_state;

    return hr;
}

static HRESULT WINAPI multimedia_stream_GetTime(IAMMultiMediaStream *iface, STREAM_TIME *time)
{
    struct multimedia_stream *stream = impl_from_IAMMultiMediaStream(iface);

    TRACE("stream %p, time %p.\n", stream, time);

    return IMediaStreamFilter_GetCurrentStreamTime(stream->filter, time);
}

static HRESULT WINAPI multimedia_stream_GetDuration(IAMMultiMediaStream *iface, STREAM_TIME *duration)
{
    struct multimedia_stream *mmstream = impl_from_IAMMultiMediaStream(iface);

    TRACE("mmstream %p, duration %p.\n", mmstream, duration);

    if (!mmstream->media_seeking)
        return E_NOINTERFACE;

    if (IMediaSeeking_GetDuration(mmstream->media_seeking, duration) != S_OK)
        return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI multimedia_stream_Seek(IAMMultiMediaStream *iface, STREAM_TIME seek_time)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    TRACE("(%p/%p)->(%s)\n", This, iface, wine_dbgstr_longlong(seek_time));

    return IMediaSeeking_SetPositions(This->media_seeking, &seek_time, AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
}

static HRESULT WINAPI multimedia_stream_GetEndOfStream(IAMMultiMediaStream *iface, HANDLE *eos)
{
    struct multimedia_stream *mmstream = impl_from_IAMMultiMediaStream(iface);

    TRACE("mmstream %p, eos %p.\n", mmstream, eos);

    if (!eos)
        return E_POINTER;

    *eos = (HANDLE)mmstream->event;

    return S_OK;
}

static HRESULT create_graph(struct multimedia_stream *mmstream, IGraphBuilder *graph)
{
    IMediaEventEx *eventsrc;
    HRESULT hr;

    if (graph)
        IGraphBuilder_AddRef(mmstream->graph = graph);
    else if (FAILED(hr = CoCreateInstance(&CLSID_FilterGraph, NULL,
            CLSCTX_INPROC_SERVER, &IID_IGraphBuilder, (void **)&mmstream->graph)))
        return hr;

    hr = IGraphBuilder_QueryInterface(mmstream->graph, &IID_IMediaSeeking, (void **)&mmstream->media_seeking);
    if (SUCCEEDED(hr))
        hr = IGraphBuilder_QueryInterface(mmstream->graph, &IID_IMediaControl, (void **)&mmstream->media_control);
    if (SUCCEEDED(hr))
        hr = IGraphBuilder_AddFilter(mmstream->graph, (IBaseFilter *)mmstream->filter, L"MediaStreamFilter");
    if (SUCCEEDED(hr))
        hr = IGraphBuilder_QueryInterface(mmstream->graph, &IID_IMediaEventEx, (void **)&eventsrc);
    if (SUCCEEDED(hr))
    {
        hr = IMediaEventEx_GetEventHandle(eventsrc, &mmstream->event);
        if (SUCCEEDED(hr))
            hr = IMediaEventEx_SetNotifyFlags(eventsrc, AM_MEDIAEVENT_NONOTIFY);
        IMediaEventEx_Release(eventsrc);
    }

    if (FAILED(hr))
    {
        if (mmstream->media_seeking)
            IMediaSeeking_Release(mmstream->media_seeking);
        mmstream->media_seeking = NULL;
        if (mmstream->media_control)
            IMediaControl_Release(mmstream->media_control);
        mmstream->media_control = NULL;
        if (mmstream->graph)
            IGraphBuilder_Release(mmstream->graph);
        mmstream->graph = NULL;
    }

    return hr;
}

static HRESULT WINAPI multimedia_stream_Initialize(IAMMultiMediaStream *iface,
        STREAM_TYPE type, DWORD flags, IGraphBuilder *graph)
{
    struct multimedia_stream *mmstream = impl_from_IAMMultiMediaStream(iface);
    HRESULT hr;

    TRACE("mmstream %p, type %u, flags %#lx, graph %p.\n", mmstream, type, flags, graph);

    if (graph && mmstream->graph)
    {
        WARN("Graph already initialized, returning E_INVALIDARG.\n");
        return E_INVALIDARG;
    }

    if (mmstream->initialized && type != mmstream->type)
    {
        WARN("Attempt to change type from %u, returning E_INVALIDARG.\n", mmstream->type);
        return E_INVALIDARG;
    }

    if (graph && FAILED(hr = create_graph(mmstream, graph)))
        return hr;

    mmstream->type = type;
    mmstream->initialized = TRUE;

    return S_OK;
}

static HRESULT WINAPI multimedia_stream_GetFilterGraph(IAMMultiMediaStream *iface, IGraphBuilder **graph)
{
    struct multimedia_stream *mmstream = impl_from_IAMMultiMediaStream(iface);

    TRACE("mmstream %p, graph %p.\n", mmstream, graph);

    if (!graph)
        return E_POINTER;

    if (mmstream->graph)
        IGraphBuilder_AddRef(*graph = mmstream->graph);
    else
        *graph = NULL;

    return S_OK;
}

static HRESULT WINAPI multimedia_stream_GetFilter(IAMMultiMediaStream *iface,
        IMediaStreamFilter **filter)
{
    struct multimedia_stream *mmstream = impl_from_IAMMultiMediaStream(iface);

    TRACE("mmstream %p, filter %p.\n", mmstream, filter);

    if (!filter)
        return E_POINTER;

    IMediaStreamFilter_AddRef(*filter = mmstream->filter);

    return S_OK;
}

static void add_stream(struct multimedia_stream *mmstream, IAMMediaStream *stream, IMediaStream **ret_stream)
{
    IMediaStreamFilter_AddMediaStream(mmstream->filter, stream);
    IAMMediaStream_JoinAMMultiMediaStream(stream, &mmstream->IAMMultiMediaStream_iface);
    if (ret_stream)
    {
        *ret_stream = (IMediaStream *)stream;
        IMediaStream_AddRef(*ret_stream);
    }
}

static HRESULT WINAPI multimedia_stream_AddMediaStream(IAMMultiMediaStream *iface,
        IUnknown *stream_object, const MSPID *PurposeId, DWORD dwFlags, IMediaStream **ret_stream)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);
    HRESULT hr;
    IAMMediaStream* pStream;
    IMediaStream *stream;

    TRACE("mmstream %p, stream_object %p, id %s, flags %#lx, ret_stream %p.\n",
            This, stream_object, debugstr_guid(PurposeId), dwFlags, ret_stream);

    if (IMediaStreamFilter_GetMediaStream(This->filter, PurposeId, &stream) == S_OK)
    {
        IMediaStream_Release(stream);
        return MS_E_PURPOSEID;
    }

    if (!This->graph && FAILED(hr = create_graph(This, NULL)))
        return hr;

    if (dwFlags & AMMSF_ADDDEFAULTRENDERER)
    {
        IBaseFilter *dsound_render;

        if (ret_stream)
            return E_INVALIDARG;

        if (!IsEqualGUID(PurposeId, &MSPID_PrimaryAudio))
        {
            WARN("AMMSF_ADDDEFAULTRENDERER requested with id %s, returning MS_E_PURPOSEID.\n", debugstr_guid(PurposeId));
            return MS_E_PURPOSEID;
        }

        if (SUCCEEDED(hr = CoCreateInstance(&CLSID_DSoundRender, NULL,
                CLSCTX_INPROC_SERVER, &IID_IBaseFilter, (void **)&dsound_render)))
        {
            hr = IGraphBuilder_AddFilter(This->graph, dsound_render, NULL);
            IBaseFilter_Release(dsound_render);
        }
        return hr;
    }

    if (stream_object)
    {
        hr = IUnknown_QueryInterface(stream_object, &IID_IAMMediaStream, (void **)&pStream);
        if (SUCCEEDED(hr))
        {
            MSPID stream_id;
            hr = IAMMediaStream_GetInformation(pStream, &stream_id, NULL);
            if (SUCCEEDED(hr))
            {
                if (IsEqualGUID(PurposeId, &stream_id))
                {
                    add_stream(This, pStream, ret_stream);
                    hr = S_OK;
                }
                else
                {
                    hr = MS_E_PURPOSEID;
                }
            }

            IAMMediaStream_Release(pStream);

            return hr;
        }
    }

    if (IsEqualGUID(PurposeId, &MSPID_PrimaryVideo))
        hr = ddraw_stream_create(NULL, (void **)&pStream);
    else if (IsEqualGUID(PurposeId, &MSPID_PrimaryAudio))
        hr = audio_stream_create(NULL, (void **)&pStream);
    else
        return MS_E_PURPOSEID;

    if (FAILED(hr))
        return hr;

    hr = IAMMediaStream_Initialize(pStream, stream_object, dwFlags, PurposeId, This->type);
    if (FAILED(hr))
    {
        IAMMediaStream_Release(pStream);
        return hr;
    }

    add_stream(This, pStream, ret_stream);
    IAMMediaStream_Release(pStream);

    return S_OK;
}

static HRESULT WINAPI multimedia_stream_OpenFile(IAMMultiMediaStream *iface,
        const WCHAR *filename, DWORD flags)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);
    HRESULT ret = S_OK;
    IBaseFilter *BaseFilter = NULL;
    IEnumPins *EnumPins = NULL;
    IPin *ipin;
    PIN_DIRECTION pin_direction;

    TRACE("(%p/%p)->(%s,%lx)\n", This, iface, debugstr_w(filename), flags);

    if (!filename)
        return E_POINTER;

    /* If Initialize was not called before, we do it here */
    if (!This->graph)
    {
        ret = IAMMultiMediaStream_Initialize(iface, STREAMTYPE_READ, 0, NULL);
        if (SUCCEEDED(ret))
            ret = create_graph(This, NULL);
    }

    if (SUCCEEDED(ret))
        ret = IGraphBuilder_AddSourceFilter(This->graph, filename, L"Source", &BaseFilter);

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
    {
        IFilterGraph2 *graph;

        if (SUCCEEDED(ret = IGraphBuilder_QueryInterface(This->graph, &IID_IFilterGraph2, (void **)&graph)))
        {
            DWORD renderflags = (flags & AMMSF_RENDERALLSTREAMS) ? 0 : AM_RENDEREX_RENDERTOEXISTINGRENDERERS;

            ret = IFilterGraph2_RenderEx(graph, This->ipin, renderflags, NULL);
            if (ret == VFW_E_CANNOT_RENDER) ret = VFW_E_CANNOT_CONNECT;
            else if (ret == VFW_S_PARTIAL_RENDER) ret = S_OK;

            IFilterGraph2_Release(graph);
        }
        else
        {
            FIXME("Failed to get IFilterGraph2 interface, hr %#lx.\n", ret);
            ret = IGraphBuilder_Render(This->graph, This->ipin);
        }
    }

    if (SUCCEEDED(ret) && (flags & AMMSF_NOCLOCK))
    {
        IMediaFilter *media_filter;

        if (SUCCEEDED(ret = IGraphBuilder_QueryInterface(This->graph, &IID_IMediaFilter, (void **)&media_filter)))
        {
            ret = IMediaFilter_SetSyncSource(media_filter, NULL);
            IMediaFilter_Release(media_filter);
        }
    }

    IMediaStreamFilter_SupportSeeking(This->filter, This->type == STREAMTYPE_READ);

    if (SUCCEEDED(ret) && (flags & AMMSF_RUN))
        ret = IAMMultiMediaStream_SetState(iface, STREAMSTATE_RUN);

    if (EnumPins)
        IEnumPins_Release(EnumPins);
    if (BaseFilter)
        IBaseFilter_Release(BaseFilter);
    return ret;
}

static HRESULT WINAPI multimedia_stream_OpenMoniker(IAMMultiMediaStream *iface,
        IBindCtx *pCtx, IMoniker *pMoniker, DWORD dwFlags)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%p,%p,%lx) stub!\n", This, iface, pCtx, pMoniker, dwFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI multimedia_stream_Render(IAMMultiMediaStream *iface, DWORD dwFlags)
{
    struct multimedia_stream *This = impl_from_IAMMultiMediaStream(iface);

    FIXME("(%p/%p)->(%lx) partial stub!\n", This, iface, dwFlags);

    if(dwFlags != AMMSF_NOCLOCK)
        return E_INVALIDARG;

    return IGraphBuilder_Render(This->graph, This->ipin);
}

static const IAMMultiMediaStreamVtbl multimedia_stream_vtbl =
{
    multimedia_stream_QueryInterface,
    multimedia_stream_AddRef,
    multimedia_stream_Release,
    multimedia_stream_GetInformation,
    multimedia_stream_GetMediaStream,
    multimedia_stream_EnumMediaStreams,
    multimedia_stream_GetState,
    multimedia_stream_SetState,
    multimedia_stream_GetTime,
    multimedia_stream_GetDuration,
    multimedia_stream_Seek,
    multimedia_stream_GetEndOfStream,
    multimedia_stream_Initialize,
    multimedia_stream_GetFilterGraph,
    multimedia_stream_GetFilter,
    multimedia_stream_AddMediaStream,
    multimedia_stream_OpenFile,
    multimedia_stream_OpenMoniker,
    multimedia_stream_Render
};

HRESULT multimedia_stream_create(IUnknown *outer, void **out)
{
    struct multimedia_stream *object;
    HRESULT hr;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IAMMultiMediaStream_iface.lpVtbl = &multimedia_stream_vtbl;
    object->ref = 1;

    if (FAILED(hr = CoCreateInstance(&CLSID_MediaStreamFilter, NULL,
            CLSCTX_INPROC_SERVER, &IID_IMediaStreamFilter, (void **)&object->filter)))
    {
        ERR("Failed to create stream filter, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    /* The stream takes an additional reference to the filter. */
    IMediaStreamFilter_AddRef(object->filter);

    TRACE("Created multimedia stream %p.\n", object);
    *out = &object->IAMMultiMediaStream_iface;

    return S_OK;
}
