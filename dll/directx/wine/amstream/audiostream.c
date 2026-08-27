/*
 * Primary audio stream
 *
 * Copyright 2012 Christian Costa
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

#define COBJMACROS
#include "amstream_private.h"
#include "wine/debug.h"
#include "wine/list.h"
#include "wine/strmbase.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

static const WCHAR sink_id[] = L"I{A35FF56B-9FDA-11D0-8FDF-00C04FD9189D}";

struct queued_receive
{
    struct list entry;
    IMediaSample *sample;
    DWORD length;
    BYTE *pointer;
    DWORD position;
    STREAM_TIME start_time;
};

struct audio_stream
{
    IAMMediaStream IAMMediaStream_iface;
    IAudioMediaStream IAudioMediaStream_iface;
    IMemInputPin IMemInputPin_iface;
    IPin IPin_iface;
    LONG ref;

    IMultiMediaStream* parent;
    MSPID purpose_id;
    STREAM_TYPE stream_type;
    CRITICAL_SECTION cs;
    IMediaStreamFilter *filter;

    IPin *peer;
    IMemAllocator *allocator;
    AM_MEDIA_TYPE mt;
    WAVEFORMATEX format;
    FILTER_STATE state;
    REFERENCE_TIME segment_start;
    BOOL eos;
    BOOL flushing;
    struct list receive_queue;
    struct list update_queue;
};

struct audio_sample
{
    IAudioStreamSample IAudioStreamSample_iface;
    LONG ref;
    struct audio_stream *parent;
    IAudioData *audio_data;
    STREAM_TIME start_time;
    STREAM_TIME end_time;
    HANDLE update_event;

    struct list entry;
    DWORD length;
    BYTE *pointer;
    DWORD position;
    HRESULT update_hr;
};

static void remove_queued_receive(struct queued_receive *receive)
{
    list_remove(&receive->entry);
    IMediaSample_Release(receive->sample);
    free(receive);
}

static void remove_queued_update(struct audio_sample *sample)
{
    HRESULT hr;

    hr = IAudioData_SetActual(sample->audio_data, sample->position);
    if (FAILED(hr))
        sample->update_hr = hr;

    list_remove(&sample->entry);
    SetEvent(sample->update_event);
}

static void flush_receive_queue(struct audio_stream *stream)
{
    struct list *entry;

    while ((entry = list_head(&stream->receive_queue)))
        remove_queued_receive(LIST_ENTRY(entry, struct queued_receive, entry));
}

static STREAM_TIME stream_time_from_position(struct audio_stream *stream, struct queued_receive *receive)
{
    const WAVEFORMATEX *format = (WAVEFORMATEX *)stream->mt.pbFormat;
    return receive->start_time + (receive->position * 10000000 + format->nAvgBytesPerSec / 2) / format->nAvgBytesPerSec;
}

static void process_update(struct audio_sample *sample, struct queued_receive *receive)
{
    DWORD advance;

    advance = min(receive->length - receive->position, sample->length - sample->position);
    memcpy(&sample->pointer[sample->position], &receive->pointer[receive->position], advance);

    if (!sample->position)
        sample->start_time = stream_time_from_position(sample->parent, receive);

    receive->position += advance;
    sample->position += advance;

    sample->end_time = stream_time_from_position(sample->parent, receive);

    sample->update_hr = (sample->position == sample->length) ? S_OK : MS_S_PENDING;
}

static void process_updates(struct audio_stream *stream)
{
    while (!list_empty(&stream->update_queue) && !list_empty(&stream->receive_queue))
    {
        struct audio_sample *sample = LIST_ENTRY(list_head(&stream->update_queue), struct audio_sample, entry);
        struct queued_receive *receive = LIST_ENTRY(list_head(&stream->receive_queue), struct queued_receive, entry);

        process_update(sample, receive);

        if (sample->update_hr != MS_S_PENDING)
            remove_queued_update(sample);
        if (receive->position == receive->length)
            remove_queued_receive(receive);
    }
    if (stream->eos)
    {
        while (!list_empty(&stream->update_queue))
        {
            struct audio_sample *sample = LIST_ENTRY(list_head(&stream->update_queue), struct audio_sample, entry);

            sample->update_hr = sample->position ? S_OK : MS_S_ENDOFSTREAM;
            remove_queued_update(sample);
        }
    }
}

static inline struct audio_sample *impl_from_IAudioStreamSample(IAudioStreamSample *iface)
{
    return CONTAINING_RECORD(iface, struct audio_sample, IAudioStreamSample_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI audio_sample_QueryInterface(IAudioStreamSample *iface,
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

static ULONG WINAPI audio_sample_AddRef(IAudioStreamSample *iface)
{
    struct audio_sample *sample = impl_from_IAudioStreamSample(iface);
    ULONG refcount = InterlockedIncrement(&sample->ref);
    TRACE("%p increasing refcount to %lu.\n", sample, refcount);
    return refcount;
}

static ULONG WINAPI audio_sample_Release(IAudioStreamSample *iface)
{
    struct audio_sample *sample = impl_from_IAudioStreamSample(iface);
    ULONG refcount = InterlockedDecrement(&sample->ref);
    TRACE("%p decreasing refcount to %lu.\n", sample, refcount);
    if (!refcount)
    {
        IAMMediaStream_Release(&sample->parent->IAMMediaStream_iface);
        IAudioData_Release(sample->audio_data);
        CloseHandle(sample->update_event);
        free(sample);
    }
    return refcount;
}

/*** IStreamSample methods ***/
static HRESULT WINAPI audio_sample_GetMediaStream(IAudioStreamSample *iface, IMediaStream **media_stream)
{
    struct audio_sample *sample = impl_from_IAudioStreamSample(iface);

    TRACE("sample %p, media_stream %p.\n", iface, media_stream);

    if (!media_stream)
        return E_POINTER;

    IAMMediaStream_AddRef(&sample->parent->IAMMediaStream_iface);
    *media_stream = (IMediaStream *)&sample->parent->IAMMediaStream_iface;

    return S_OK;
}

static HRESULT WINAPI audio_sample_GetSampleTimes(IAudioStreamSample *iface, STREAM_TIME *start_time,
                                                                 STREAM_TIME *end_time, STREAM_TIME *current_time)
{
    struct audio_sample *sample = impl_from_IAudioStreamSample(iface);

    TRACE("sample %p, start_time %p, end_time %p, current_time %p.\n", sample, start_time, end_time, current_time);

    if (current_time)
        IMediaStreamFilter_GetCurrentStreamTime(sample->parent->filter, current_time);

    if (start_time)
        *start_time = sample->start_time;
    if (end_time)
        *end_time = sample->end_time;

    return S_OK;
}

static HRESULT WINAPI audio_sample_SetSampleTimes(IAudioStreamSample *iface, const STREAM_TIME *start_time,
                                                                 const STREAM_TIME *end_time)
{
    FIXME("(%p)->(%p,%p): stub\n", iface, start_time, end_time);

    return E_NOTIMPL;
}

static HRESULT WINAPI audio_sample_Update(IAudioStreamSample *iface,
        DWORD flags, HANDLE event, PAPCFUNC apc_func, DWORD apc_data)
{
    struct audio_sample *sample = impl_from_IAudioStreamSample(iface);
    BYTE *pointer;
    DWORD length;
    HRESULT hr;

    TRACE("sample %p, flags %#lx, event %p, apc_func %p, apc_data %#lx.\n",
            sample, flags, event, apc_func, apc_data);

    hr = IAudioData_GetInfo(sample->audio_data, &length, &pointer, NULL);
    if (FAILED(hr))
        return hr;

    if (event && apc_func)
        return E_INVALIDARG;

    if (apc_func)
    {
        FIXME("APC support is not implemented!\n");
        return E_NOTIMPL;
    }

    if (event)
    {
        FIXME("Event parameter support is not implemented!\n");
        return E_NOTIMPL;
    }

    if (flags & ~SSUPDATE_ASYNC)
    {
        FIXME("Unsupported flags %#lx.\n", flags);
        return E_NOTIMPL;
    }

    EnterCriticalSection(&sample->parent->cs);

    if (sample->parent->state != State_Running)
    {
        LeaveCriticalSection(&sample->parent->cs);
        return MS_E_NOTRUNNING;
    }
    if (!sample->parent->peer)
    {
        LeaveCriticalSection(&sample->parent->cs);
        return MS_S_ENDOFSTREAM;
    }
    if (MS_S_PENDING == sample->update_hr)
    {
        LeaveCriticalSection(&sample->parent->cs);
        return MS_E_BUSY;
    }

    sample->length = length;
    sample->pointer = pointer;
    sample->position = 0;
    sample->update_hr = MS_S_PENDING;
    ResetEvent(sample->update_event);
    list_add_tail(&sample->parent->update_queue, &sample->entry);

    process_updates(sample->parent);
    hr = sample->update_hr;

    LeaveCriticalSection(&sample->parent->cs);

    if (hr != MS_S_PENDING || (flags & SSUPDATE_ASYNC))
        return hr;

    WaitForSingleObject(sample->update_event, INFINITE);

    return sample->update_hr;
}

static HRESULT WINAPI audio_sample_CompletionStatus(IAudioStreamSample *iface, DWORD flags, DWORD milliseconds)
{
    struct audio_sample *sample = impl_from_IAudioStreamSample(iface);
    HRESULT hr;

    TRACE("sample %p, flags %#lx, milliseconds %lu.\n", sample, flags, milliseconds);

    if (flags)
    {
        FIXME("Unhandled flags %#lx.\n", flags);
        return E_NOTIMPL;
    }

    EnterCriticalSection(&sample->parent->cs);

    hr = sample->update_hr;

    LeaveCriticalSection(&sample->parent->cs);

    return hr;
}

/*** IAudioStreamSample methods ***/
static HRESULT WINAPI audio_sample_GetAudioData(IAudioStreamSample *iface, IAudioData **audio_data)
{
    struct audio_sample *sample = impl_from_IAudioStreamSample(iface);

    TRACE("sample %p, audio_data %p.\n", sample, audio_data);

    if (!audio_data)
        return E_POINTER;

    IAudioData_AddRef(sample->audio_data);
    *audio_data = sample->audio_data;

    return S_OK;
}

static const struct IAudioStreamSampleVtbl AudioStreamSample_Vtbl =
{
    /*** IUnknown methods ***/
    audio_sample_QueryInterface,
    audio_sample_AddRef,
    audio_sample_Release,
    /*** IStreamSample methods ***/
    audio_sample_GetMediaStream,
    audio_sample_GetSampleTimes,
    audio_sample_SetSampleTimes,
    audio_sample_Update,
    audio_sample_CompletionStatus,
    /*** IAudioStreamSample methods ***/
    audio_sample_GetAudioData
};

static HRESULT audiostreamsample_create(struct audio_stream *parent, IAudioData *audio_data, IAudioStreamSample **audio_stream_sample)
{
    struct audio_sample *object;

    TRACE("(%p)\n", audio_stream_sample);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IAudioStreamSample_iface.lpVtbl = &AudioStreamSample_Vtbl;
    object->ref = 1;
    object->parent = parent;
    IAMMediaStream_AddRef(&parent->IAMMediaStream_iface);
    object->audio_data = audio_data;
    IAudioData_AddRef(audio_data);
    object->update_event = CreateEventW(NULL, FALSE, FALSE, NULL);

    *audio_stream_sample = &object->IAudioStreamSample_iface;

    return S_OK;
}

static inline struct audio_stream *impl_from_IAMMediaStream(IAMMediaStream *iface)
{
    return CONTAINING_RECORD(iface, struct audio_stream, IAMMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI audio_IAMMediaStream_QueryInterface(IAMMediaStream *iface,
        REFIID riid, void **ret_iface)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

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
        *ret_iface = &This->IPin_iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_IMemInputPin))
    {
        IAMMediaStream_AddRef(iface);
        *ret_iface = &This->IMemInputPin_iface;
        return S_OK;
    }

    ERR("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ret_iface);
    return E_NOINTERFACE;
}

static ULONG WINAPI audio_IAMMediaStream_AddRef(IAMMediaStream *iface)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->(): new ref = %lu\n", iface, This, ref);

    return ref;
}

static ULONG WINAPI audio_IAMMediaStream_Release(IAMMediaStream *iface)
{
    struct audio_stream *stream = impl_from_IAMMediaStream(iface);
    ULONG ref = InterlockedDecrement(&stream->ref);

    TRACE("%p decreasing refcount to %lu.\n", stream, ref);

    if (!ref)
    {
        DeleteCriticalSection(&stream->cs);
        free(stream);
    }

    return ref;
}

/*** IMediaStream methods ***/
static HRESULT WINAPI audio_IAMMediaStream_GetMultiMediaStream(IAMMediaStream *iface,
        IMultiMediaStream **mmstream)
{
    struct audio_stream *stream = impl_from_IAMMediaStream(iface);

    TRACE("stream %p, mmstream %p.\n", stream, mmstream);

    if (!mmstream)
        return E_POINTER;

    if (stream->parent)
        IMultiMediaStream_AddRef(stream->parent);
    *mmstream = stream->parent;
    return S_OK;
}

static HRESULT WINAPI audio_IAMMediaStream_GetInformation(IAMMediaStream *iface,
        MSPID *purpose_id, STREAM_TYPE *type)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    TRACE("(%p/%p)->(%p,%p)\n", This, iface, purpose_id, type);

    if (purpose_id)
        *purpose_id = This->purpose_id;
    if (type)
        *type = This->stream_type;

    return S_OK;
}

static HRESULT WINAPI audio_IAMMediaStream_SetSameFormat(IAMMediaStream *iface,
        IMediaStream *pStreamThatHasDesiredFormat, DWORD flags)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%lx) stub!\n", This, iface, pStreamThatHasDesiredFormat, flags);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAMMediaStream_AllocateSample(IAMMediaStream *iface,
        DWORD flags, IStreamSample **sample)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%lx,%p) stub!\n", This, iface, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAMMediaStream_CreateSharedSample(IAMMediaStream *iface,
        IStreamSample *existing_sample, DWORD flags, IStreamSample **sample)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%p,%lx,%p) stub!\n", This, iface, existing_sample, flags, sample);

    return S_FALSE;
}

static HRESULT WINAPI audio_IAMMediaStream_SendEndOfStream(IAMMediaStream *iface, DWORD flags)
{
    struct audio_stream *This = impl_from_IAMMediaStream(iface);

    FIXME("(%p/%p)->(%lx) stub!\n", This, iface, flags);

    return S_FALSE;
}

/*** IAMMediaStream methods ***/
static HRESULT WINAPI audio_IAMMediaStream_Initialize(IAMMediaStream *iface, IUnknown *source_object, DWORD flags,
                                                    REFMSPID purpose_id, const STREAM_TYPE stream_type)
{
    struct audio_stream *stream = impl_from_IAMMediaStream(iface);

    TRACE("stream %p, source_object %p, flags %lx, purpose_id %s, stream_type %u.\n", stream, source_object, flags,
            debugstr_guid(purpose_id), stream_type);

    if (!purpose_id)
        return E_POINTER;

    if (source_object)
        FIXME("Specifying a stream object is not yet supported.\n");

    if (flags & AMMSF_CREATEPEER)
        FIXME("AMMSF_CREATEPEER is not yet supported.\n");

    stream->purpose_id = *purpose_id;
    stream->stream_type = stream_type;

    return S_OK;
}

static HRESULT WINAPI audio_IAMMediaStream_SetState(IAMMediaStream *iface, FILTER_STATE state)
{
    struct audio_stream *stream = impl_from_IAMMediaStream(iface);

    TRACE("stream %p, state %u.\n", stream, state);

    EnterCriticalSection(&stream->cs);

    if (state == State_Stopped)
        flush_receive_queue(stream);
    if (stream->state == State_Stopped)
        stream->eos = FALSE;

    stream->state = state;

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static HRESULT WINAPI audio_IAMMediaStream_JoinAMMultiMediaStream(IAMMediaStream *iface,
        IAMMultiMediaStream *mmstream)
{
    struct audio_stream *stream = impl_from_IAMMediaStream(iface);

    TRACE("stream %p, mmstream %p.\n", stream, mmstream);

    stream->parent = (IMultiMediaStream *)mmstream;

    return S_OK;
}

static HRESULT WINAPI audio_IAMMediaStream_JoinFilter(IAMMediaStream *iface, IMediaStreamFilter *filter)
{
    struct audio_stream *stream = impl_from_IAMMediaStream(iface);

    TRACE("stream %p, filter %p.\n", stream, filter);

    stream->filter = filter;

    return S_OK;
}

static HRESULT WINAPI audio_IAMMediaStream_JoinFilterGraph(IAMMediaStream *iface, IFilterGraph *filtergraph)
{
    struct audio_stream *stream = impl_from_IAMMediaStream(iface);

    TRACE("stream %p, filtergraph %p.\n", stream, filtergraph);

    return S_OK;
}

static const struct IAMMediaStreamVtbl audio_IAMMediaStream_vtbl =
{
    audio_IAMMediaStream_QueryInterface,
    audio_IAMMediaStream_AddRef,
    audio_IAMMediaStream_Release,
    audio_IAMMediaStream_GetMultiMediaStream,
    audio_IAMMediaStream_GetInformation,
    audio_IAMMediaStream_SetSameFormat,
    audio_IAMMediaStream_AllocateSample,
    audio_IAMMediaStream_CreateSharedSample,
    audio_IAMMediaStream_SendEndOfStream,
    audio_IAMMediaStream_Initialize,
    audio_IAMMediaStream_SetState,
    audio_IAMMediaStream_JoinAMMultiMediaStream,
    audio_IAMMediaStream_JoinFilter,
    audio_IAMMediaStream_JoinFilterGraph,
};

static inline struct audio_stream *impl_from_IAudioMediaStream(IAudioMediaStream *iface)
{
    return CONTAINING_RECORD(iface, struct audio_stream, IAudioMediaStream_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI audio_IAudioMediaStream_QueryInterface(IAudioMediaStream *iface,
        REFIID riid, void **ret_iface)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ret_iface);
    return IAMMediaStream_QueryInterface(&This->IAMMediaStream_iface, riid, ret_iface);
}

static ULONG WINAPI audio_IAudioMediaStream_AddRef(IAudioMediaStream *iface)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)\n", iface, This);
    return IAMMediaStream_AddRef(&This->IAMMediaStream_iface);
}

static ULONG WINAPI audio_IAudioMediaStream_Release(IAudioMediaStream *iface)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);
    TRACE("(%p/%p)\n", iface, This);
    return IAMMediaStream_Release(&This->IAMMediaStream_iface);
}

static HRESULT WINAPI audio_IAudioMediaStream_GetMultiMediaStream(IAudioMediaStream *iface,
        IMultiMediaStream **mmstream)
{
    struct audio_stream *stream = impl_from_IAudioMediaStream(iface);
    return IAMMediaStream_GetMultiMediaStream(&stream->IAMMediaStream_iface, mmstream);
}

static HRESULT WINAPI audio_IAudioMediaStream_GetInformation(IAudioMediaStream *iface,
        MSPID *purpose_id, STREAM_TYPE *type)
{
    struct audio_stream *stream = impl_from_IAudioMediaStream(iface);
    return IAMMediaStream_GetInformation(&stream->IAMMediaStream_iface, purpose_id, type);
}

static HRESULT WINAPI audio_IAudioMediaStream_SetSameFormat(IAudioMediaStream *iface,
        IMediaStream *other, DWORD flags)
{
    struct audio_stream *stream = impl_from_IAudioMediaStream(iface);
    return IAMMediaStream_SetSameFormat(&stream->IAMMediaStream_iface, other, flags);
}

static HRESULT WINAPI audio_IAudioMediaStream_AllocateSample(IAudioMediaStream *iface,
        DWORD flags, IStreamSample **sample)
{
    struct audio_stream *stream = impl_from_IAudioMediaStream(iface);
    return IAMMediaStream_AllocateSample(&stream->IAMMediaStream_iface, flags, sample);
}

static HRESULT WINAPI audio_IAudioMediaStream_CreateSharedSample(IAudioMediaStream *iface,
        IStreamSample *existing_sample, DWORD flags, IStreamSample **sample)
{
    struct audio_stream *stream = impl_from_IAudioMediaStream(iface);
    return IAMMediaStream_CreateSharedSample(&stream->IAMMediaStream_iface, existing_sample, flags, sample);
}

static HRESULT WINAPI audio_IAudioMediaStream_SendEndOfStream(IAudioMediaStream *iface, DWORD flags)
{
    struct audio_stream *stream = impl_from_IAudioMediaStream(iface);
    return IAMMediaStream_SendEndOfStream(&stream->IAMMediaStream_iface, flags);
}

/*** IAudioMediaStream methods ***/
static HRESULT WINAPI audio_IAudioMediaStream_GetFormat(IAudioMediaStream *iface, WAVEFORMATEX *format)
{
    struct audio_stream *stream = impl_from_IAudioMediaStream(iface);

    TRACE("stream %p, format %p.\n", stream, format);

    if (!format)
        return E_POINTER;

    EnterCriticalSection(&stream->cs);

    if (!stream->peer)
    {
        LeaveCriticalSection(&stream->cs);
        return MS_E_NOSTREAM;
    }

    *format = *(WAVEFORMATEX *)stream->mt.pbFormat;

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static HRESULT WINAPI audio_IAudioMediaStream_SetFormat(IAudioMediaStream *iface, const WAVEFORMATEX *format)
{
    struct audio_stream *stream = impl_from_IAudioMediaStream(iface);

    TRACE("stream %p, format %p.\n", stream, format);

    if (!format)
        return E_POINTER;

    if (format->wFormatTag != WAVE_FORMAT_PCM)
        return E_INVALIDARG;

    EnterCriticalSection(&stream->cs);

    if ((stream->peer && memcmp(format, stream->mt.pbFormat, sizeof(WAVEFORMATEX)))
            || (stream->format.wFormatTag && memcmp(format, &stream->format, sizeof(WAVEFORMATEX))))
    {
        LeaveCriticalSection(&stream->cs);
        return E_INVALIDARG;
    }

    stream->format = *format;

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static HRESULT WINAPI audio_IAudioMediaStream_CreateSample(IAudioMediaStream *iface, IAudioData *audio_data,
                                                         DWORD flags, IAudioStreamSample **sample)
{
    struct audio_stream *This = impl_from_IAudioMediaStream(iface);

    TRACE("(%p/%p)->(%p,%lu,%p)\n", iface, This, audio_data, flags, sample);

    if (!audio_data)
        return E_POINTER;

    return audiostreamsample_create(This, audio_data, sample);
}

static const struct IAudioMediaStreamVtbl audio_IAudioMediaStream_vtbl =
{
    audio_IAudioMediaStream_QueryInterface,
    audio_IAudioMediaStream_AddRef,
    audio_IAudioMediaStream_Release,
    audio_IAudioMediaStream_GetMultiMediaStream,
    audio_IAudioMediaStream_GetInformation,
    audio_IAudioMediaStream_SetSameFormat,
    audio_IAudioMediaStream_AllocateSample,
    audio_IAudioMediaStream_CreateSharedSample,
    audio_IAudioMediaStream_SendEndOfStream,
    audio_IAudioMediaStream_GetFormat,
    audio_IAudioMediaStream_SetFormat,
    audio_IAudioMediaStream_CreateSample,
};

struct enum_media_types
{
    IEnumMediaTypes IEnumMediaTypes_iface;
    LONG refcount;
    unsigned int index;
};

static const IEnumMediaTypesVtbl enum_media_types_vtbl;

static struct enum_media_types *impl_from_IEnumMediaTypes(IEnumMediaTypes *iface)
{
    return CONTAINING_RECORD(iface, struct enum_media_types, IEnumMediaTypes_iface);
}

static HRESULT WINAPI enum_media_types_QueryInterface(IEnumMediaTypes *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IEnumMediaTypes))
    {
        IEnumMediaTypes_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI enum_media_types_AddRef(IEnumMediaTypes *iface)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);
    ULONG refcount = InterlockedIncrement(&enum_media_types->refcount);
    TRACE("%p increasing refcount to %lu.\n", enum_media_types, refcount);
    return refcount;
}

static ULONG WINAPI enum_media_types_Release(IEnumMediaTypes *iface)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);
    ULONG refcount = InterlockedDecrement(&enum_media_types->refcount);
    TRACE("%p decreasing refcount to %lu.\n", enum_media_types, refcount);
    if (!refcount)
        free(enum_media_types);
    return refcount;
}

static HRESULT WINAPI enum_media_types_Next(IEnumMediaTypes *iface, ULONG count, AM_MEDIA_TYPE **mts, ULONG *ret_count)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);

    static const WAVEFORMATEX wfx =
    {
        .wFormatTag = WAVE_FORMAT_PCM,
        .nChannels = 1,
        .nSamplesPerSec = 11025,
        .nAvgBytesPerSec = 11025 * 2,
        .nBlockAlign = 2,
        .wBitsPerSample = 16,
        .cbSize = 0,
    };

    TRACE("iface %p, count %lu, mts %p, ret_count %p.\n", iface, count, mts, ret_count);

    if (!ret_count)
        return E_POINTER;

    if (count && !enum_media_types->index)
    {
        mts[0] = CoTaskMemAlloc(sizeof(AM_MEDIA_TYPE));
        memset(mts[0], 0, sizeof(AM_MEDIA_TYPE));
        mts[0]->majortype = MEDIATYPE_Audio;
        mts[0]->subtype = GUID_NULL;
        mts[0]->bFixedSizeSamples = TRUE;
        mts[0]->bTemporalCompression = FALSE;
        mts[0]->lSampleSize = 2;
        mts[0]->formattype = FORMAT_WaveFormatEx;
        mts[0]->cbFormat = sizeof(WAVEFORMATEX);
        mts[0]->pbFormat = CoTaskMemAlloc(sizeof(WAVEFORMATEX));
        memcpy(mts[0]->pbFormat, &wfx, sizeof(WAVEFORMATEX));

        ++enum_media_types->index;
        *ret_count = 1;
        return count == 1 ? S_OK : S_FALSE;
    }

    *ret_count = 0;
    return count ? S_FALSE : S_OK;
}

static HRESULT WINAPI enum_media_types_Skip(IEnumMediaTypes *iface, ULONG count)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);

    TRACE("iface %p, count %lu.\n", iface, count);

    enum_media_types->index += count;
    return S_OK;
}

static HRESULT WINAPI enum_media_types_Reset(IEnumMediaTypes *iface)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);

    TRACE("iface %p.\n", iface);

    enum_media_types->index = 0;
    return S_OK;
}

static HRESULT WINAPI enum_media_types_Clone(IEnumMediaTypes *iface, IEnumMediaTypes **out)
{
    struct enum_media_types *enum_media_types = impl_from_IEnumMediaTypes(iface);
    struct enum_media_types *object;

    TRACE("iface %p, out %p.\n", iface, out);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumMediaTypes_iface.lpVtbl = &enum_media_types_vtbl;
    object->refcount = 1;
    object->index = enum_media_types->index;

    *out = &object->IEnumMediaTypes_iface;
    return S_OK;
}

static const IEnumMediaTypesVtbl enum_media_types_vtbl =
{
    enum_media_types_QueryInterface,
    enum_media_types_AddRef,
    enum_media_types_Release,
    enum_media_types_Next,
    enum_media_types_Skip,
    enum_media_types_Reset,
    enum_media_types_Clone,
};

static inline struct audio_stream *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct audio_stream, IPin_iface);
}

static HRESULT WINAPI audio_sink_QueryInterface(IPin *iface, REFIID iid, void **out)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    return IAMMediaStream_QueryInterface(&stream->IAMMediaStream_iface, iid, out);
}

static ULONG WINAPI audio_sink_AddRef(IPin *iface)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    return IAMMediaStream_AddRef(&stream->IAMMediaStream_iface);
}

static ULONG WINAPI audio_sink_Release(IPin *iface)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    return IAMMediaStream_Release(&stream->IAMMediaStream_iface);
}

static HRESULT WINAPI audio_sink_Connect(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    WARN("iface %p, peer %p, mt %p, unexpected call!\n", iface, peer, mt);
    return E_UNEXPECTED;
}

static HRESULT WINAPI audio_sink_ReceiveConnection(IPin *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    PIN_DIRECTION dir;

    TRACE("stream %p, peer %p, mt %p.\n", stream, peer, mt);

    if (!IsEqualGUID(&mt->majortype, &MEDIATYPE_Audio)
            || !IsEqualGUID(&mt->formattype, &FORMAT_WaveFormatEx)
            || mt->cbFormat < sizeof(WAVEFORMATEX))
        return VFW_E_TYPE_NOT_ACCEPTED;

    if (((const WAVEFORMATEX *)mt->pbFormat)->wFormatTag != WAVE_FORMAT_PCM)
        return E_INVALIDARG;

    EnterCriticalSection(&stream->cs);

    if (stream->peer)
    {
        LeaveCriticalSection(&stream->cs);
        return VFW_E_ALREADY_CONNECTED;
    }

    IPin_QueryDirection(peer, &dir);
    if (dir != PINDIR_OUTPUT)
    {
        WARN("Rejecting connection from input pin.\n");
        LeaveCriticalSection(&stream->cs);
        return VFW_E_INVALID_DIRECTION;
    }

    if (stream->format.wFormatTag && memcmp(mt->pbFormat, &stream->format, sizeof(WAVEFORMATEX)))
    {
        LeaveCriticalSection(&stream->cs);
        return E_INVALIDARG;
    }

    CopyMediaType(&stream->mt, mt);
    IPin_AddRef(stream->peer = peer);

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static HRESULT WINAPI audio_sink_Disconnect(IPin *iface)
{
    struct audio_stream *stream = impl_from_IPin(iface);

    TRACE("stream %p.\n", stream);

    EnterCriticalSection(&stream->cs);

    if (!stream->peer)
    {
        LeaveCriticalSection(&stream->cs);
        return S_FALSE;
    }

    IPin_Release(stream->peer);
    stream->peer = NULL;
    FreeMediaType(&stream->mt);
    memset(&stream->mt, 0, sizeof(AM_MEDIA_TYPE));

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static HRESULT WINAPI audio_sink_ConnectedTo(IPin *iface, IPin **peer)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    HRESULT hr;

    TRACE("stream %p, peer %p.\n", stream, peer);

    EnterCriticalSection(&stream->cs);

    if (stream->peer)
    {
        IPin_AddRef(*peer = stream->peer);
        hr = S_OK;
    }
    else
    {
        *peer = NULL;
        hr = VFW_E_NOT_CONNECTED;
    }

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI audio_sink_ConnectionMediaType(IPin *iface, AM_MEDIA_TYPE *mt)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    HRESULT hr;

    TRACE("stream %p, mt %p.\n", stream, mt);

    EnterCriticalSection(&stream->cs);

    if (stream->peer)
    {
        CopyMediaType(mt, &stream->mt);
        hr = S_OK;
    }
    else
    {
        memset(mt, 0, sizeof(AM_MEDIA_TYPE));
        hr = VFW_E_NOT_CONNECTED;
    }

    LeaveCriticalSection(&stream->cs);

    return hr;
}

static HRESULT WINAPI audio_sink_QueryPinInfo(IPin *iface, PIN_INFO *info)
{
    struct audio_stream *stream = impl_from_IPin(iface);

    TRACE("stream %p, info %p.\n", stream, info);

    IBaseFilter_AddRef(info->pFilter = (IBaseFilter *)stream->filter);
    info->dir = PINDIR_INPUT;
    wcscpy(info->achName, sink_id);

    return S_OK;
}

static HRESULT WINAPI audio_sink_QueryDirection(IPin *iface, PIN_DIRECTION *dir)
{
    TRACE("iface %p, dir %p.\n", iface, dir);
    *dir = PINDIR_INPUT;
    return S_OK;
}

static HRESULT WINAPI audio_sink_QueryId(IPin *iface, WCHAR **id)
{
    TRACE("iface %p, id %p.\n", iface, id);

    if (!(*id = CoTaskMemAlloc(sizeof(sink_id))))
        return E_OUTOFMEMORY;

    wcscpy(*id, sink_id);

    return S_OK;
}

static HRESULT WINAPI audio_sink_QueryAccept(IPin *iface, const AM_MEDIA_TYPE *mt)
{
    TRACE("iface %p, mt %p.\n", iface, mt);
    return E_NOTIMPL;
}

static HRESULT WINAPI audio_sink_EnumMediaTypes(IPin *iface, IEnumMediaTypes **enum_media_types)
{
    struct enum_media_types *object;

    TRACE("iface %p, enum_media_types %p.\n", iface, enum_media_types);

    if (!enum_media_types)
        return E_POINTER;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumMediaTypes_iface.lpVtbl = &enum_media_types_vtbl;
    object->refcount = 1;
    object->index = 0;

    *enum_media_types = &object->IEnumMediaTypes_iface;
    return S_OK;
}

static HRESULT WINAPI audio_sink_QueryInternalConnections(IPin *iface, IPin **pins, ULONG *count)
{
    TRACE("iface %p, pins %p, count %p.\n", iface, pins, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI audio_sink_EndOfStream(IPin *iface)
{
    struct audio_stream *stream = impl_from_IPin(iface);

    TRACE("stream %p.\n", stream);

    EnterCriticalSection(&stream->cs);

    if (stream->eos || stream->flushing)
    {
        LeaveCriticalSection(&stream->cs);
        return E_FAIL;
    }

    stream->eos = TRUE;

    process_updates(stream);

    LeaveCriticalSection(&stream->cs);

    /* Calling IMediaStreamFilter::EndOfStream() inside the critical section
     * would invert the locking order, so we must leave it first to avoid
     * the streaming thread deadlocking on the filter's critical section. */
    IMediaStreamFilter_EndOfStream(stream->filter);

    return S_OK;
}

static HRESULT WINAPI audio_sink_BeginFlush(IPin *iface)
{
    struct audio_stream *stream = impl_from_IPin(iface);
    BOOL cancel_eos;

    TRACE("stream %p.\n", stream);

    EnterCriticalSection(&stream->cs);

    cancel_eos = stream->eos;

    stream->flushing = TRUE;
    stream->eos = FALSE;
    flush_receive_queue(stream);

    LeaveCriticalSection(&stream->cs);

    /* Calling IMediaStreamFilter::Flush() inside the critical section would
     * invert the locking order, so we must leave it first to avoid the
     * application thread deadlocking on the filter's critical section. */
    IMediaStreamFilter_Flush(stream->filter, cancel_eos);

    return S_OK;
}

static HRESULT WINAPI audio_sink_EndFlush(IPin *iface)
{
    struct audio_stream *stream = impl_from_IPin(iface);

    TRACE("stream %p.\n", stream);

    EnterCriticalSection(&stream->cs);

    stream->flushing = FALSE;

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static HRESULT WINAPI audio_sink_NewSegment(IPin *iface, REFERENCE_TIME start, REFERENCE_TIME stop, double rate)
{
    struct audio_stream *stream = impl_from_IPin(iface);

    TRACE("stream %p, start %s, stop %s, rate %0.16e\n",
            stream, wine_dbgstr_longlong(start), wine_dbgstr_longlong(stop), rate);

    EnterCriticalSection(&stream->cs);

    stream->segment_start = start;

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static const IPinVtbl audio_sink_vtbl =
{
    audio_sink_QueryInterface,
    audio_sink_AddRef,
    audio_sink_Release,
    audio_sink_Connect,
    audio_sink_ReceiveConnection,
    audio_sink_Disconnect,
    audio_sink_ConnectedTo,
    audio_sink_ConnectionMediaType,
    audio_sink_QueryPinInfo,
    audio_sink_QueryDirection,
    audio_sink_QueryId,
    audio_sink_QueryAccept,
    audio_sink_EnumMediaTypes,
    audio_sink_QueryInternalConnections,
    audio_sink_EndOfStream,
    audio_sink_BeginFlush,
    audio_sink_EndFlush,
    audio_sink_NewSegment,
};

static inline struct audio_stream *impl_from_IMemInputPin(IMemInputPin *iface)
{
    return CONTAINING_RECORD(iface, struct audio_stream, IMemInputPin_iface);
}

static HRESULT WINAPI audio_meminput_QueryInterface(IMemInputPin *iface, REFIID iid, void **out)
{
    struct audio_stream *stream = impl_from_IMemInputPin(iface);
    return IAMMediaStream_QueryInterface(&stream->IAMMediaStream_iface, iid, out);
}

static ULONG WINAPI audio_meminput_AddRef(IMemInputPin *iface)
{
    struct audio_stream *stream = impl_from_IMemInputPin(iface);
    return IAMMediaStream_AddRef(&stream->IAMMediaStream_iface);
}

static ULONG WINAPI audio_meminput_Release(IMemInputPin *iface)
{
    struct audio_stream *stream = impl_from_IMemInputPin(iface);
    return IAMMediaStream_Release(&stream->IAMMediaStream_iface);
}

static HRESULT WINAPI audio_meminput_GetAllocator(IMemInputPin *iface, IMemAllocator **allocator)
{
    struct audio_stream *stream = impl_from_IMemInputPin(iface);

    TRACE("stream %p, allocator %p.\n", stream, allocator);

    if (stream->allocator)
    {
        IMemAllocator_AddRef(*allocator = stream->allocator);
        return S_OK;
    }

    *allocator = NULL;
    return VFW_E_NO_ALLOCATOR;
}

static HRESULT WINAPI audio_meminput_NotifyAllocator(IMemInputPin *iface, IMemAllocator *allocator, BOOL readonly)
{
    struct audio_stream *stream = impl_from_IMemInputPin(iface);

    TRACE("stream %p, allocator %p, readonly %d.\n", stream, allocator, readonly);

    if (!allocator)
        return E_POINTER;

    if (allocator)
        IMemAllocator_AddRef(allocator);
    if (stream->allocator)
        IMemAllocator_Release(stream->allocator);
    stream->allocator = allocator;

    return S_OK;
}

static HRESULT WINAPI audio_meminput_GetAllocatorRequirements(IMemInputPin *iface, ALLOCATOR_PROPERTIES *props)
{
    TRACE("iface %p, props %p.\n", iface, props);
    return E_NOTIMPL;
}

static HRESULT WINAPI audio_meminput_Receive(IMemInputPin *iface, IMediaSample *sample)
{
    struct audio_stream *stream = impl_from_IMemInputPin(iface);
    struct queued_receive *receive;
    REFERENCE_TIME start_time = 0;
    REFERENCE_TIME end_time = 0;
    BYTE *pointer;
    HRESULT hr;

    TRACE("stream %p, sample %p.\n", stream, sample);

    EnterCriticalSection(&stream->cs);

    if (stream->state == State_Stopped)
    {
        LeaveCriticalSection(&stream->cs);
        return VFW_E_WRONG_STATE;
    }
    if (stream->flushing)
    {
        LeaveCriticalSection(&stream->cs);
        return S_FALSE;
    }

    hr = IMediaSample_GetPointer(sample, &pointer);
    if (FAILED(hr))
    {
        LeaveCriticalSection(&stream->cs);
        return hr;
    }

    IMediaSample_GetTime(sample, &start_time, &end_time);

    receive = calloc(1, sizeof(*receive));
    if (!receive)
    {
        LeaveCriticalSection(&stream->cs);
        return E_OUTOFMEMORY;
    }

    receive->length = IMediaSample_GetActualDataLength(sample);
    receive->pointer = pointer;
    receive->sample = sample;
    receive->start_time = start_time + stream->segment_start;
    IMediaSample_AddRef(receive->sample);
    list_add_tail(&stream->receive_queue, &receive->entry);

    process_updates(stream);

    LeaveCriticalSection(&stream->cs);

    return S_OK;
}

static HRESULT WINAPI audio_meminput_ReceiveMultiple(IMemInputPin *iface,
        IMediaSample **samples, LONG count, LONG *processed)
{
    FIXME("iface %p, samples %p, count %lu, processed %p, stub!\n", iface, samples, count, processed);
    return E_NOTIMPL;
}

static HRESULT WINAPI audio_meminput_ReceiveCanBlock(IMemInputPin *iface)
{
    TRACE("iface %p.\n", iface);
    return S_OK;
}

static const IMemInputPinVtbl audio_meminput_vtbl =
{
    audio_meminput_QueryInterface,
    audio_meminput_AddRef,
    audio_meminput_Release,
    audio_meminput_GetAllocator,
    audio_meminput_NotifyAllocator,
    audio_meminput_GetAllocatorRequirements,
    audio_meminput_Receive,
    audio_meminput_ReceiveMultiple,
    audio_meminput_ReceiveCanBlock,
};

HRESULT audio_stream_create(IUnknown *outer, void **out)
{
    struct audio_stream *object;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IAMMediaStream_iface.lpVtbl = &audio_IAMMediaStream_vtbl;
    object->IAudioMediaStream_iface.lpVtbl = &audio_IAudioMediaStream_vtbl;
    object->IMemInputPin_iface.lpVtbl = &audio_meminput_vtbl;
    object->IPin_iface.lpVtbl = &audio_sink_vtbl;
    object->ref = 1;

    InitializeCriticalSection(&object->cs);
    list_init(&object->receive_queue);
    list_init(&object->update_queue);

    TRACE("Created audio stream %p.\n", object);

    *out = &object->IAMMediaStream_iface;

    return S_OK;
}
