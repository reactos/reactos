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

#define COBJMACROS
#include "amstream_private.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct enum_pins
{
    IEnumPins IEnumPins_iface;
    LONG refcount;

    IPin **pins;
    unsigned int count, index;
};

static const IEnumPinsVtbl enum_pins_vtbl;

static struct enum_pins *impl_from_IEnumPins(IEnumPins *iface)
{
    return CONTAINING_RECORD(iface, struct enum_pins, IEnumPins_iface);
}

static HRESULT WINAPI enum_pins_QueryInterface(IEnumPins *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IEnumPins))
    {
        IEnumPins_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI enum_pins_AddRef(IEnumPins *iface)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);
    ULONG refcount = InterlockedIncrement(&enum_pins->refcount);
    TRACE("%p increasing refcount to %lu.\n", enum_pins, refcount);
    return refcount;
}

static ULONG WINAPI enum_pins_Release(IEnumPins *iface)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);
    ULONG refcount = InterlockedDecrement(&enum_pins->refcount);
    unsigned int i;

    TRACE("%p decreasing refcount to %lu.\n", enum_pins, refcount);
    if (!refcount)
    {
        for (i = 0; i < enum_pins->count; ++i)
            IPin_Release(enum_pins->pins[i]);
        free(enum_pins->pins);
        free(enum_pins);
    }
    return refcount;
}

static HRESULT WINAPI enum_pins_Next(IEnumPins *iface, ULONG count, IPin **pins, ULONG *ret_count)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);
    unsigned int i;

    TRACE("iface %p, count %lu, pins %p, ret_count %p.\n", iface, count, pins, ret_count);

    if (!pins || (count > 1 && !ret_count))
        return E_POINTER;

    for (i = 0; i < count && enum_pins->index < enum_pins->count; ++i)
    {
        IPin_AddRef(pins[i] = enum_pins->pins[enum_pins->index]);
        enum_pins->index++;
    }

    if (ret_count) *ret_count = i;
    return i == count ? S_OK : S_FALSE;
}

static HRESULT WINAPI enum_pins_Skip(IEnumPins *iface, ULONG count)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);

    TRACE("iface %p, count %lu.\n", iface, count);

    enum_pins->index += count;

    return enum_pins->index >= enum_pins->count ? S_FALSE : S_OK;
}

static HRESULT WINAPI enum_pins_Reset(IEnumPins *iface)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);

    TRACE("iface %p.\n", iface);

    enum_pins->index = 0;
    return S_OK;
}

static HRESULT WINAPI enum_pins_Clone(IEnumPins *iface, IEnumPins **out)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);
    struct enum_pins *object;
    unsigned int i;

    TRACE("iface %p, out %p.\n", iface, out);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumPins_iface.lpVtbl = &enum_pins_vtbl;
    object->refcount = 1;
    object->count = enum_pins->count;
    object->index = enum_pins->index;
    if (!(object->pins = malloc(enum_pins->count * sizeof(*object->pins))))
    {
        free(object);
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < enum_pins->count; ++i)
        IPin_AddRef(object->pins[i] = enum_pins->pins[i]);

    *out = &object->IEnumPins_iface;
    return S_OK;
}

static const IEnumPinsVtbl enum_pins_vtbl =
{
    enum_pins_QueryInterface,
    enum_pins_AddRef,
    enum_pins_Release,
    enum_pins_Next,
    enum_pins_Skip,
    enum_pins_Reset,
    enum_pins_Clone,
};

struct filter
{
    IMediaStreamFilter IMediaStreamFilter_iface;
    IMediaSeeking IMediaSeeking_iface;
    LONG refcount;
    CRITICAL_SECTION cs;

    IReferenceClock *clock;
    WCHAR name[128];
    IFilterGraph *graph;
    ULONG nb_streams;
    IAMMediaStream **streams;
    IAMMediaStream *seekable_stream;
    FILTER_STATE state;
    REFERENCE_TIME start_time;
    struct list free_events;
    struct list used_events;
    LONG eos_count;
};

struct event
{
    struct list entry;
    HANDLE event;
    DWORD_PTR cookie;
    BOOL interrupted;
};

static inline struct filter *impl_from_IMediaStreamFilter(IMediaStreamFilter *iface)
{
    return CONTAINING_RECORD(iface, struct filter, IMediaStreamFilter_iface);
}

static HRESULT WINAPI filter_QueryInterface(IMediaStreamFilter *iface, REFIID iid, void **out)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("filter %p, iid %s, out %p.\n", filter, debugstr_guid(iid), out);

    *out = NULL;

    if (IsEqualGUID(iid, &IID_IUnknown)
            || IsEqualGUID(iid, &IID_IPersist)
            || IsEqualGUID(iid, &IID_IMediaFilter)
            || IsEqualGUID(iid, &IID_IBaseFilter)
            || IsEqualGUID(iid, &IID_IMediaStreamFilter))
        *out = iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking) && filter->seekable_stream)
        *out = &filter->IMediaSeeking_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI filter_AddRef(IMediaStreamFilter *iface)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    ULONG refcount = InterlockedIncrement(&filter->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI filter_Release(IMediaStreamFilter *iface)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    ULONG refcount = InterlockedDecrement(&filter->refcount);
    unsigned int i;

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        struct list *entry;

        while ((entry = list_head(&filter->free_events)))
        {
            struct event *event = LIST_ENTRY(entry, struct event, entry);
            list_remove(entry);
            CloseHandle(event->event);
            free(event);
        }
        for (i = 0; i < filter->nb_streams; ++i)
        {
            IAMMediaStream_JoinFilter(filter->streams[i], NULL);
            IAMMediaStream_Release(filter->streams[i]);
        }
        free(filter->streams);
        if (filter->clock)
            IReferenceClock_Release(filter->clock);
        if (filter->cs.DebugInfo)
            filter->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&filter->cs);
        free(filter);
    }

    return refcount;
}

static HRESULT WINAPI filter_GetClassID(IMediaStreamFilter *iface, CLSID *clsid)
{
    *clsid = CLSID_MediaStreamFilter;
    return S_OK;
}

static void send_ec_complete(struct filter *filter)
{
    IMediaEventSink *event_sink;

    if (!filter->graph)
        return;

    if (FAILED(IFilterGraph_QueryInterface(filter->graph, &IID_IMediaEventSink, (void **)&event_sink)))
        return;

    IMediaEventSink_Notify(event_sink, EC_COMPLETE, S_OK,
            (LONG_PTR)&filter->IMediaStreamFilter_iface);

    IMediaEventSink_Release(event_sink);
}

static void set_state(struct filter *filter, FILTER_STATE state)
{
    if (filter->state != state)
    {
        ULONG i;

        for (i = 0; i < filter->nb_streams; ++i)
            IAMMediaStream_SetState(filter->streams[i], state);
        filter->state = state;
    }
}

static HRESULT WINAPI filter_Stop(IMediaStreamFilter *iface)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    struct event *event;

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&filter->cs);

    if (filter->state != State_Stopped)
        filter->eos_count = 0;

    set_state(filter, State_Stopped);

    LIST_FOR_EACH_ENTRY(event, &filter->used_events, struct event, entry)
    {
        if (!event->interrupted)
        {
            event->interrupted = TRUE;
            IReferenceClock_Unadvise(filter->clock, event->cookie);
            SetEvent(event->event);
        }
    }

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_Pause(IMediaStreamFilter *iface)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&filter->cs);

    set_state(filter, State_Paused);

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_Run(IMediaStreamFilter *iface, REFERENCE_TIME start)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("iface %p, start %s.\n", iface, wine_dbgstr_longlong(start));

    EnterCriticalSection(&filter->cs);

    if (filter->state != State_Running && filter->seekable_stream
            && filter->eos_count == (LONG)filter->nb_streams)
        send_ec_complete(filter);

    filter->start_time = start;
    set_state(filter, State_Running);

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_GetState(IMediaStreamFilter *iface, DWORD timeout, FILTER_STATE *state)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("iface %p, timeout %lu, state %p.\n", iface, timeout, state);

    if (!state)
        return E_POINTER;

    EnterCriticalSection(&filter->cs);

    *state = filter->state;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_SetSyncSource(IMediaStreamFilter *iface, IReferenceClock *clock)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("iface %p, clock %p.\n", iface, clock);

    EnterCriticalSection(&filter->cs);

    if (clock)
        IReferenceClock_AddRef(clock);
    if (filter->clock)
        IReferenceClock_Release(filter->clock);
    filter->clock = clock;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_GetSyncSource(IMediaStreamFilter *iface, IReferenceClock **clock)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("iface %p, clock %p.\n", iface, clock);

    EnterCriticalSection(&filter->cs);

    if (filter->clock)
        IReferenceClock_AddRef(filter->clock);
    *clock = filter->clock;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_EnumPins(IMediaStreamFilter *iface, IEnumPins **enum_pins)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    struct enum_pins *object;
    unsigned int i;

    TRACE("iface %p, enum_pins %p.\n", iface, enum_pins);

    if (!enum_pins)
        return E_POINTER;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    EnterCriticalSection(&filter->cs);

    object->IEnumPins_iface.lpVtbl = &enum_pins_vtbl;
    object->refcount = 1;
    object->count = filter->nb_streams;
    object->index = 0;
    if (!(object->pins = malloc(filter->nb_streams * sizeof(*object->pins))))
    {
        free(object);
        LeaveCriticalSection(&filter->cs);
        return E_OUTOFMEMORY;
    }
    for (i = 0; i < filter->nb_streams; ++i)
    {
        if (FAILED(IAMMediaStream_QueryInterface(filter->streams[i], &IID_IPin, (void **)&object->pins[i])))
            WARN("Stream %p does not support IPin.\n", filter->streams[i]);
    }

    LeaveCriticalSection(&filter->cs);

    *enum_pins = &object->IEnumPins_iface;
    return S_OK;
}

static HRESULT WINAPI filter_FindPin(IMediaStreamFilter *iface, const WCHAR *id, IPin **out)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    unsigned int i;
    WCHAR *ret_id;
    IPin *pin;

    TRACE("iface %p, id %s, out %p.\n", iface, debugstr_w(id), out);

    EnterCriticalSection(&filter->cs);

    for (i = 0; i < filter->nb_streams; ++i)
    {
        if (FAILED(IAMMediaStream_QueryInterface(filter->streams[i], &IID_IPin, (void **)&pin)))
        {
            WARN("Stream %p does not support IPin.\n", filter->streams[i]);
            continue;
        }

        if (SUCCEEDED(IPin_QueryId(pin, &ret_id)))
        {
            if (!wcscmp(id, ret_id))
            {
                CoTaskMemFree(ret_id);
                *out = pin;
                LeaveCriticalSection(&filter->cs);
                return S_OK;
            }
            CoTaskMemFree(ret_id);
        }
        IPin_Release(pin);
    }

    LeaveCriticalSection(&filter->cs);

    return VFW_E_NOT_FOUND;
}

static HRESULT WINAPI filter_QueryFilterInfo(IMediaStreamFilter *iface, FILTER_INFO *info)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("iface %p, info %p.\n", iface, info);

    EnterCriticalSection(&filter->cs);

    wcscpy(info->achName, filter->name);
    if (filter->graph)
        IFilterGraph_AddRef(filter->graph);
    info->pGraph = filter->graph;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_JoinFilterGraph(IMediaStreamFilter *iface,
        IFilterGraph *graph, const WCHAR *name)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("iface %p, graph %p, name.%s.\n", iface, graph, debugstr_w(name));

    EnterCriticalSection(&filter->cs);

    if (name)
        lstrcpynW(filter->name, name, ARRAY_SIZE(filter->name));
    else
        filter->name[0] = 0;
    filter->graph = graph;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_QueryVendorInfo(IMediaStreamFilter *iface, LPWSTR *vendor_info)
{
    WARN("iface %p, vendor_info %p, stub!\n", iface, vendor_info);
    return E_NOTIMPL;
}

/*** IMediaStreamFilter methods ***/

static HRESULT WINAPI filter_AddMediaStream(IMediaStreamFilter *iface, IAMMediaStream *pAMMediaStream)
{
    struct filter *This = impl_from_IMediaStreamFilter(iface);
    IAMMediaStream** streams;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", iface, pAMMediaStream);

    streams = realloc(This->streams, (This->nb_streams + 1) * sizeof(*streams));
    if (!streams)
        return E_OUTOFMEMORY;
    This->streams = streams;

    hr = IAMMediaStream_JoinFilter(pAMMediaStream, iface);
    if (FAILED(hr))
        return hr;

    hr = IAMMediaStream_JoinFilterGraph(pAMMediaStream, This->graph);
    if (FAILED(hr))
        return hr;

    This->streams[This->nb_streams] = pAMMediaStream;
    This->nb_streams++;

    IAMMediaStream_AddRef(pAMMediaStream);

    return S_OK;
}

static HRESULT WINAPI filter_GetMediaStream(IMediaStreamFilter *iface, REFMSPID idPurpose, IMediaStream **ppMediaStream)
{
    struct filter *This = impl_from_IMediaStreamFilter(iface);
    MSPID purpose_id;
    unsigned int i;

    TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid(idPurpose), ppMediaStream);

    if (!ppMediaStream)
        return E_POINTER;

    for (i = 0; i < This->nb_streams; i++)
    {
        IAMMediaStream_GetInformation(This->streams[i], &purpose_id, NULL);
        if (IsEqualIID(&purpose_id, idPurpose))
        {
            *ppMediaStream = (IMediaStream *)This->streams[i];
            IMediaStream_AddRef(*ppMediaStream);
            return S_OK;
        }
    }

    return MS_E_NOSTREAM;
}

static HRESULT WINAPI filter_EnumMediaStreams(IMediaStreamFilter *iface, LONG index, IMediaStream **stream)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("filter %p, index %ld, stream %p.\n", filter, index, stream);

    if (index >= filter->nb_streams)
        return S_FALSE;

    if (!stream)
        return E_POINTER;

    IMediaStream_AddRef(*stream = (IMediaStream *)filter->streams[index]);
    return S_OK;
}

static IMediaSeeking *get_seeking(IAMMediaStream *stream)
{
    IMediaSeeking *seeking;
    IPin *pin, *peer;
    HRESULT hr;

    if (FAILED(IAMMediaStream_QueryInterface(stream, &IID_IPin, (void **)&pin)))
    {
        WARN("Stream %p does not support IPin.\n", stream);
        return NULL;
    }

    hr = IPin_ConnectedTo(pin, &peer);
    IPin_Release(pin);
    if (FAILED(hr))
        return NULL;

    hr = IPin_QueryInterface(peer, &IID_IMediaSeeking, (void **)&seeking);
    IPin_Release(peer);
    if (FAILED(hr))
        return NULL;

    return seeking;
}

static HRESULT WINAPI filter_SupportSeeking(IMediaStreamFilter *iface, BOOL renderer)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    unsigned int i;

    TRACE("filter %p, renderer %d\n", iface, renderer);

    if (!renderer)
        FIXME("Non-renderer filter support is not yet implemented.\n");

    EnterCriticalSection(&filter->cs);

    if (filter->seekable_stream)
    {
        LeaveCriticalSection(&filter->cs);
        return HRESULT_FROM_WIN32(ERROR_ALREADY_INITIALIZED);
    }

    for (i = 0; i < filter->nb_streams; ++i)
    {
        IMediaSeeking *seeking = get_seeking(filter->streams[i]);
        LONGLONG duration;

        if (!seeking)
            continue;

        if (SUCCEEDED(IMediaSeeking_GetDuration(seeking, &duration)))
        {
            filter->seekable_stream = filter->streams[i];
            IMediaSeeking_Release(seeking);
            LeaveCriticalSection(&filter->cs);
            return S_OK;
        }

        IMediaSeeking_Release(seeking);
    }

    LeaveCriticalSection(&filter->cs);
    return E_NOINTERFACE;
}

static HRESULT WINAPI filter_ReferenceTimeToStreamTime(IMediaStreamFilter *iface, REFERENCE_TIME *time)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("filter %p, time %p.\n", filter, time);

    EnterCriticalSection(&filter->cs);

    if (!filter->clock)
    {
        LeaveCriticalSection(&filter->cs);
        return S_FALSE;
    }

    *time -= filter->start_time;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_GetCurrentStreamTime(IMediaStreamFilter *iface, REFERENCE_TIME *time)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("filter %p, time %p.\n", filter, time);

    if (!time)
        return E_POINTER;

    EnterCriticalSection(&filter->cs);

    if (filter->state != State_Running || !filter->clock)
    {
        *time = 0;
        LeaveCriticalSection(&filter->cs);
        return S_FALSE;
    }

    IReferenceClock_GetTime(filter->clock, time);

    *time -= filter->start_time;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_WaitUntil(IMediaStreamFilter *iface, REFERENCE_TIME time)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    struct event *event;
    struct list *entry;
    HRESULT hr;

    TRACE("filter %p, time %s.\n", iface, wine_dbgstr_longlong(time));

    EnterCriticalSection(&filter->cs);

    if (!filter->clock)
    {
        LeaveCriticalSection(&filter->cs);
        return E_FAIL;
    }

    if ((entry = list_head(&filter->free_events)))
    {
        list_remove(entry);
        event = LIST_ENTRY(entry, struct event, entry);
    }
    else
    {
        event = calloc(1, sizeof(struct event));
        event->event = CreateEventW(NULL, FALSE, FALSE, NULL);

        entry = &event->entry;
    }

    hr = IReferenceClock_AdviseTime(filter->clock, time, filter->start_time, (HEVENT)event->event, &event->cookie);
    if (FAILED(hr))
    {
        list_add_tail(&filter->free_events, entry);
        LeaveCriticalSection(&filter->cs);
        return hr;
    }

    event->interrupted = FALSE;
    list_add_tail(&filter->used_events, entry);

    LeaveCriticalSection(&filter->cs);
    WaitForSingleObject(event->event, INFINITE);
    EnterCriticalSection(&filter->cs);

    hr = event->interrupted ? S_FALSE : S_OK;

    list_remove(entry);
    list_add_tail(&filter->free_events, entry);

    LeaveCriticalSection(&filter->cs);

    return hr;
}

static HRESULT WINAPI filter_Flush(IMediaStreamFilter *iface, BOOL cancel_eos)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);
    struct event *event;

    TRACE("filter %p, cancel_eos %d.\n", iface, cancel_eos);

    EnterCriticalSection(&filter->cs);

    LIST_FOR_EACH_ENTRY(event, &filter->used_events, struct event, entry)
    {
        if (!event->interrupted)
        {
            event->interrupted = TRUE;
            IReferenceClock_Unadvise(filter->clock, event->cookie);
            SetEvent(event->event);
        }
    }

    if (cancel_eos)
        --filter->eos_count;

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static HRESULT WINAPI filter_EndOfStream(IMediaStreamFilter *iface)
{
    struct filter *filter = impl_from_IMediaStreamFilter(iface);

    TRACE("filter %p.\n", filter);

    EnterCriticalSection(&filter->cs);

    ++filter->eos_count;
    if (filter->state == State_Running && filter->seekable_stream &&
            filter->eos_count == (LONG)filter->nb_streams)
        send_ec_complete(filter);

    LeaveCriticalSection(&filter->cs);

    return S_OK;
}

static const IMediaStreamFilterVtbl filter_vtbl =
{
    filter_QueryInterface,
    filter_AddRef,
    filter_Release,
    filter_GetClassID,
    filter_Stop,
    filter_Pause,
    filter_Run,
    filter_GetState,
    filter_SetSyncSource,
    filter_GetSyncSource,
    filter_EnumPins,
    filter_FindPin,
    filter_QueryFilterInfo,
    filter_JoinFilterGraph,
    filter_QueryVendorInfo,
    filter_AddMediaStream,
    filter_GetMediaStream,
    filter_EnumMediaStreams,
    filter_SupportSeeking,
    filter_ReferenceTimeToStreamTime,
    filter_GetCurrentStreamTime,
    filter_WaitUntil,
    filter_Flush,
    filter_EndOfStream
};

static inline struct filter *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, struct filter, IMediaSeeking_iface);
}

static HRESULT WINAPI filter_seeking_QueryInterface(IMediaSeeking *iface, REFIID iid, void **out)
{
    struct filter *filter = impl_from_IMediaSeeking(iface);
    return IMediaStreamFilter_QueryInterface(&filter->IMediaStreamFilter_iface, iid, out);
}

static ULONG WINAPI filter_seeking_AddRef(IMediaSeeking *iface)
{
    struct filter *filter = impl_from_IMediaSeeking(iface);
    return IMediaStreamFilter_AddRef(&filter->IMediaStreamFilter_iface);
}

static ULONG WINAPI filter_seeking_Release(IMediaSeeking *iface)
{
    struct filter *filter = impl_from_IMediaSeeking(iface);
    return IMediaStreamFilter_Release(&filter->IMediaStreamFilter_iface);
}

static HRESULT WINAPI filter_seeking_GetCapabilities(IMediaSeeking *iface, DWORD *capabilities)
{
    FIXME("iface %p, capabilities %p, stub!\n", iface, capabilities);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_CheckCapabilities(IMediaSeeking *iface, DWORD *capabilities)
{
    FIXME("iface %p, capabilities %p, stub!\n", iface, capabilities);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_IsFormatSupported(IMediaSeeking *iface, const GUID *format)
{
    struct filter *filter = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seeking;
    HRESULT hr;

    TRACE("filter %p, format %s.\n", filter, debugstr_guid(format));

    EnterCriticalSection(&filter->cs);

    seeking = get_seeking(filter->seekable_stream);

    LeaveCriticalSection(&filter->cs);

    if (!seeking)
        return E_NOTIMPL;

    hr = IMediaSeeking_IsFormatSupported(seeking, format);
    IMediaSeeking_Release(seeking);

    return hr;
}

static HRESULT WINAPI filter_seeking_QueryPreferredFormat(IMediaSeeking *iface, GUID *format)
{
    FIXME("iface %p, format %p, stub!\n", iface, format);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_GetTimeFormat(IMediaSeeking *iface, GUID *format)
{
    FIXME("iface %p, format %p, stub!\n", iface, format);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_IsUsingTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    FIXME("iface %p, format %s, stub!\n", iface, debugstr_guid(format));

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_SetTimeFormat(IMediaSeeking *iface, const GUID *format)
{
    FIXME("iface %p, format %s, stub!\n", iface, debugstr_guid(format));

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_GetDuration(IMediaSeeking *iface, LONGLONG *duration)
{
    struct filter *filter = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seeking;
    HRESULT hr;

    TRACE("filter %p, duration %p.\n", filter, duration);

    EnterCriticalSection(&filter->cs);

    seeking = get_seeking(filter->seekable_stream);

    LeaveCriticalSection(&filter->cs);

    if (!seeking)
        return E_NOTIMPL;

    hr = IMediaSeeking_GetDuration(seeking, duration);
    IMediaSeeking_Release(seeking);

    return hr;
}

static HRESULT WINAPI filter_seeking_GetStopPosition(IMediaSeeking *iface, LONGLONG *stop)
{
    struct filter *filter = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seeking;
    HRESULT hr;

    TRACE("filter %p, stop %p.\n", filter, stop);

    EnterCriticalSection(&filter->cs);

    seeking = get_seeking(filter->seekable_stream);

    LeaveCriticalSection(&filter->cs);

    if (!seeking)
        return E_NOTIMPL;

    hr = IMediaSeeking_GetStopPosition(seeking, stop);
    IMediaSeeking_Release(seeking);

    return hr;
}

static HRESULT WINAPI filter_seeking_GetCurrentPosition(IMediaSeeking *iface, LONGLONG *current)
{
    FIXME("iface %p, current %p, stub!\n", iface, current);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_ConvertTimeFormat(IMediaSeeking *iface, LONGLONG *target,
        const GUID *target_format, LONGLONG source, const GUID *source_format)
{
    FIXME("iface %p, target %p, target_format %s, source 0x%s, source_format %s, stub!\n", iface, target, debugstr_guid(target_format),
            wine_dbgstr_longlong(source), debugstr_guid(source_format));

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_SetPositions(IMediaSeeking *iface, LONGLONG *current_ptr, DWORD current_flags,
        LONGLONG *stop_ptr, DWORD stop_flags)
{
    struct filter *filter = impl_from_IMediaSeeking(iface);
    IMediaSeeking *seeking;
    HRESULT hr;

    TRACE("iface %p, current %s, current_flags %#lx, stop %s, stop_flags %#lx.\n", iface,
            current_ptr ? wine_dbgstr_longlong(*current_ptr) : "<null>", current_flags,
            stop_ptr ? wine_dbgstr_longlong(*stop_ptr): "<null>", stop_flags);

    EnterCriticalSection(&filter->cs);

    seeking = get_seeking(filter->seekable_stream);

    LeaveCriticalSection(&filter->cs);

    if (!seeking)
        return E_NOTIMPL;

    hr = IMediaSeeking_SetPositions(seeking, current_ptr, current_flags, stop_ptr, stop_flags);

    IMediaSeeking_Release(seeking);

    return hr;
}

static HRESULT WINAPI filter_seeking_GetPositions(IMediaSeeking *iface, LONGLONG *current, LONGLONG *stop)
{
    FIXME("iface %p, current %p, stop %p, stub!\n", iface, current, stop);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_GetAvailable(IMediaSeeking *iface, LONGLONG *earliest, LONGLONG *latest)
{
    FIXME("iface %p, earliest %p, latest %p, stub!\n", iface, earliest, latest);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_SetRate(IMediaSeeking *iface, double rate)
{
    FIXME("iface %p, rate %f, stub!\n", iface, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_GetRate(IMediaSeeking *iface, double *rate)
{
    FIXME("iface %p, rate %p, stub!\n", iface, rate);

    return E_NOTIMPL;
}

static HRESULT WINAPI filter_seeking_GetPreroll(IMediaSeeking *iface, LONGLONG *preroll)
{
    FIXME("iface %p, preroll %p, stub!\n", iface, preroll);

    return E_NOTIMPL;
}

static const IMediaSeekingVtbl filter_seeking_vtbl =
{
    filter_seeking_QueryInterface,
    filter_seeking_AddRef,
    filter_seeking_Release,
    filter_seeking_GetCapabilities,
    filter_seeking_CheckCapabilities,
    filter_seeking_IsFormatSupported,
    filter_seeking_QueryPreferredFormat,
    filter_seeking_GetTimeFormat,
    filter_seeking_IsUsingTimeFormat,
    filter_seeking_SetTimeFormat,
    filter_seeking_GetDuration,
    filter_seeking_GetStopPosition,
    filter_seeking_GetCurrentPosition,
    filter_seeking_ConvertTimeFormat,
    filter_seeking_SetPositions,
    filter_seeking_GetPositions,
    filter_seeking_GetAvailable,
    filter_seeking_SetRate,
    filter_seeking_GetRate,
    filter_seeking_GetPreroll,
};

HRESULT filter_create(IUnknown *outer, void **out)
{
    struct filter *object;

    TRACE("outer %p, out %p.\n", outer, out);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMediaStreamFilter_iface.lpVtbl = &filter_vtbl;
    object->IMediaSeeking_iface.lpVtbl = &filter_seeking_vtbl;
    object->refcount = 1;
    list_init(&object->free_events);
    list_init(&object->used_events);
    InitializeCriticalSectionEx(&object->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": MediaStreamFilter.cs");

    TRACE("Created media stream filter %p.\n", object);
    *out = &object->IMediaStreamFilter_iface;
    return S_OK;
}
