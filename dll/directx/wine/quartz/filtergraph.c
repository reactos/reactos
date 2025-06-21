/*              DirectShow FilterGraph object (QUARTZ.DLL)
 *
 * Copyright 2002 Lionel Ulmer
 * Copyright 2004 Christian Costa
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
#include "winreg.h"
#include "shlwapi.h"
#include "dshow.h"
#include "wine/debug.h"
#include "quartz_private.h"
#include "ole2.h"
#include "olectl.h"
#include "strmif.h"
#include "vfwmsgs.h"
#include "evcode.h"
#include "wine/heap.h"
#include "wine/list.h"


WINE_DEFAULT_DEBUG_CHANNEL(quartz);

DECLARE_CRITICAL_SECTION(message_cs);

struct filter_create_params
{
    HRESULT hr;
    IMoniker *moniker;
    IBaseFilter *filter;
};

static HANDLE message_thread, message_thread_ret;
static DWORD message_thread_id;
static unsigned int message_thread_refcount;

struct media_event
{
    struct list entry;
    LONG code;
    LONG_PTR param1, param2;
};

#define MAX_ITF_CACHE_ENTRIES 3
typedef struct _ITF_CACHE_ENTRY {
   const IID* riid;
   IBaseFilter* filter;
   IUnknown* iface;
} ITF_CACHE_ENTRY;

struct filter
{
    struct list entry;
    IBaseFilter *filter;
    IMediaSeeking *seeking;
    WCHAR *name;
    BOOL sorting;
};

struct filter_graph
{
    IUnknown IUnknown_inner;
    IFilterGraph2 IFilterGraph2_iface;
    IMediaControl IMediaControl_iface;
    IMediaSeeking IMediaSeeking_iface;
    IBasicAudio IBasicAudio_iface;
    IBasicVideo2 IBasicVideo2_iface;
    IVideoWindow IVideoWindow_iface;
    IMediaEventEx IMediaEventEx_iface;
    IMediaFilter IMediaFilter_iface;
    IMediaEventSink IMediaEventSink_iface;
    IGraphConfig IGraphConfig_iface;
    IMediaPosition IMediaPosition_iface;
    IObjectWithSite IObjectWithSite_iface;
    IGraphVersion IGraphVersion_iface;
    /* IAMGraphStreams */
    /* IAMStats */
    /* IFilterChain */
    /* IFilterMapper2 */
    /* IQueueCommand */
    /* IRegisterServiceProvider */
    /* IResourceManager */
    /* IServiceProvider */
    IVideoFrameStep IVideoFrameStep_iface;

    IUnknown *outer_unk;
    LONG ref;
    IUnknown *punkFilterMapper2;

    struct list filters;
    unsigned int name_index;

    FILTER_STATE state;
    TP_WORK *async_run_work;

    IReferenceClock *refClock;
    IBaseFilter *refClockProvider;

    /* We may indirectly wait for streaming threads while holding graph->cs in
     * IMediaFilter::Stop() or IMediaSeeking::SetPositions(). Since streaming
     * threads call IMediaEventSink::Notify() to queue EC_COMPLETE, we must
     * use a separate lock to avoid them deadlocking on graph->cs. */
    CRITICAL_SECTION event_cs;
    struct list media_events;
    HANDLE media_event_handle;
    HWND media_event_window;
    UINT media_event_message;
    LPARAM media_event_lparam;
    HANDLE hEventCompletion;
    int CompletionStatus;
    int nRenderers;
    int EcCompleteCount;
    int HandleEcComplete;
    int HandleEcRepaint;
    int HandleEcClockChanged;
    unsigned int got_ec_complete : 1;
    unsigned int media_events_disabled : 1;

    CRITICAL_SECTION cs;
    ITF_CACHE_ENTRY ItfCacheEntries[MAX_ITF_CACHE_ENTRIES];
    int nItfCacheEntries;
    BOOL defaultclock;
    GUID timeformatseek;
    IUnknown *pSite;
    LONG version;

    /* Respectively: the last timestamp at which we started streaming, and the
     * current offset within the stream. */
    REFERENCE_TIME stream_start, stream_elapsed;
    REFERENCE_TIME stream_stop;
    LONGLONG current_pos;

    unsigned int needs_async_run : 1;
    unsigned int threaded : 1;
};

struct enum_filters
{
    IEnumFilters IEnumFilters_iface;
    LONG ref;
    struct filter_graph *graph;
    LONG version;
    struct list *cursor;
};

static HRESULT create_enum_filters(struct filter_graph *graph, struct list *cursor, IEnumFilters **out);

static inline struct enum_filters *impl_from_IEnumFilters(IEnumFilters *iface)
{
    return CONTAINING_RECORD(iface, struct enum_filters, IEnumFilters_iface);
}

static HRESULT WINAPI EnumFilters_QueryInterface(IEnumFilters *iface, REFIID iid, void **out)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);
    TRACE("enum_filters %p, iid %s, out %p.\n", enum_filters, qzdebugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IEnumFilters))
    {
        IEnumFilters_AddRef(*out = iface);
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", qzdebugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI EnumFilters_AddRef(IEnumFilters *iface)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);
    ULONG ref = InterlockedIncrement(&enum_filters->ref);

    TRACE("%p increasing refcount to %lu.\n", enum_filters, ref);

    return ref;
}

static ULONG WINAPI EnumFilters_Release(IEnumFilters *iface)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);
    ULONG ref = InterlockedDecrement(&enum_filters->ref);

    TRACE("%p decreasing refcount to %lu.\n", enum_filters, ref);

    if (!ref)
    {
        IUnknown_Release(enum_filters->graph->outer_unk);
        heap_free(enum_filters);
    }

    return ref;
}

static HRESULT WINAPI EnumFilters_Next(IEnumFilters *iface, ULONG count,
        IBaseFilter **filters, ULONG *fetched)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);
    unsigned int i = 0;

    TRACE("enum_filters %p, count %lu, filters %p, fetched %p.\n",
            enum_filters, count, filters, fetched);

    if (enum_filters->version != enum_filters->graph->version)
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (!filters)
        return E_POINTER;

    for (i = 0; i < count; ++i)
    {
        struct filter *filter = LIST_ENTRY(enum_filters->cursor, struct filter, entry);

        if (!enum_filters->cursor)
            break;

        IBaseFilter_AddRef(filters[i] = filter->filter);
        enum_filters->cursor = list_next(&enum_filters->graph->filters, enum_filters->cursor);
    }

    if (fetched)
        *fetched = i;

    return (i == count) ? S_OK : S_FALSE;
}

static HRESULT WINAPI EnumFilters_Skip(IEnumFilters *iface, ULONG count)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);

    TRACE("enum_filters %p, count %lu.\n", enum_filters, count);

    if (enum_filters->version != enum_filters->graph->version)
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (!enum_filters->cursor)
        return E_INVALIDARG;

    while (count--)
    {
        if (!(enum_filters->cursor = list_next(&enum_filters->graph->filters, enum_filters->cursor)))
            return count ? S_FALSE : S_OK;
    }

    return S_OK;
}

static HRESULT WINAPI EnumFilters_Reset(IEnumFilters *iface)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);

    TRACE("enum_filters %p.\n", enum_filters);

    enum_filters->cursor = list_head(&enum_filters->graph->filters);
    enum_filters->version = enum_filters->graph->version;
    return S_OK;
}

static HRESULT WINAPI EnumFilters_Clone(IEnumFilters *iface, IEnumFilters **out)
{
    struct enum_filters *enum_filters = impl_from_IEnumFilters(iface);

    TRACE("enum_filters %p, out %p.\n", enum_filters, out);

    return create_enum_filters(enum_filters->graph, enum_filters->cursor, out);
}

static const IEnumFiltersVtbl EnumFilters_vtbl =
{
    EnumFilters_QueryInterface,
    EnumFilters_AddRef,
    EnumFilters_Release,
    EnumFilters_Next,
    EnumFilters_Skip,
    EnumFilters_Reset,
    EnumFilters_Clone,
};

static HRESULT create_enum_filters(struct filter_graph *graph, struct list *cursor, IEnumFilters **out)
{
    struct enum_filters *enum_filters;

    if (!(enum_filters = heap_alloc(sizeof(*enum_filters))))
        return E_OUTOFMEMORY;

    enum_filters->IEnumFilters_iface.lpVtbl = &EnumFilters_vtbl;
    enum_filters->ref = 1;
    enum_filters->cursor = cursor;
    enum_filters->graph = graph;
    IUnknown_AddRef(graph->outer_unk);
    enum_filters->version = graph->version;

    *out = &enum_filters->IEnumFilters_iface;
    return S_OK;
}

static BOOL queue_media_event(struct filter_graph *graph, LONG code,
        LONG_PTR param1, LONG_PTR param2)
{
    struct media_event *event;

    if (!(event = malloc(sizeof(*event))))
        return FALSE;

    event->code = code;
    event->param1 = param1;
    event->param2 = param2;
    list_add_tail(&graph->media_events, &event->entry);

    SetEvent(graph->media_event_handle);
    if (graph->media_event_window)
        PostMessageW(graph->media_event_window, graph->media_event_message, 0, graph->media_event_lparam);

    return TRUE;
}

static void flush_media_events(struct filter_graph *graph)
{
    struct list *cursor;

    while ((cursor = list_head(&graph->media_events)))
    {
        struct media_event *event = LIST_ENTRY(cursor, struct media_event, entry);

        list_remove(&event->entry);
        free(event);
    }
}

static struct filter_graph *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IUnknown_inner);
}

static HRESULT WINAPI FilterGraphInner_QueryInterface(IUnknown *iface, REFIID riid, void **ppvObj)
{
    struct filter_graph *This = impl_from_IUnknown(iface);
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), ppvObj);

    if (IsEqualGUID(&IID_IUnknown, riid)) {
        *ppvObj = &This->IUnknown_inner;
        TRACE("   returning IUnknown interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IFilterGraph, riid) ||
	IsEqualGUID(&IID_IFilterGraph2, riid) ||
	IsEqualGUID(&IID_IGraphBuilder, riid)) {
        *ppvObj = &This->IFilterGraph2_iface;
        TRACE("   returning IGraphBuilder interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaControl, riid)) {
        *ppvObj = &This->IMediaControl_iface;
        TRACE("   returning IMediaControl interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaSeeking, riid)) {
        *ppvObj = &This->IMediaSeeking_iface;
        TRACE("   returning IMediaSeeking interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IBasicAudio, riid)) {
        *ppvObj = &This->IBasicAudio_iface;
        TRACE("   returning IBasicAudio interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IBasicVideo, riid) ||
               IsEqualGUID(&IID_IBasicVideo2, riid)) {
        *ppvObj = &This->IBasicVideo2_iface;
        TRACE("   returning IBasicVideo2 interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IVideoWindow, riid)) {
        *ppvObj = &This->IVideoWindow_iface;
        TRACE("   returning IVideoWindow interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaEvent, riid) ||
	   IsEqualGUID(&IID_IMediaEventEx, riid)) {
        *ppvObj = &This->IMediaEventEx_iface;
        TRACE("   returning IMediaEvent(Ex) interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaFilter, riid) ||
          IsEqualGUID(&IID_IPersist, riid)) {
        *ppvObj = &This->IMediaFilter_iface;
        TRACE("   returning IMediaFilter interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaEventSink, riid)) {
        *ppvObj = &This->IMediaEventSink_iface;
        TRACE("   returning IMediaEventSink interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IGraphConfig, riid)) {
        *ppvObj = &This->IGraphConfig_iface;
        TRACE("   returning IGraphConfig interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IMediaPosition, riid)) {
        *ppvObj = &This->IMediaPosition_iface;
        TRACE("   returning IMediaPosition interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IObjectWithSite, riid)) {
        *ppvObj = &This->IObjectWithSite_iface;
        TRACE("   returning IObjectWithSite interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IFilterMapper, riid)) {
        TRACE("   requesting IFilterMapper interface from aggregated filtermapper (%p)\n", *ppvObj);
        return IUnknown_QueryInterface(This->punkFilterMapper2, riid, ppvObj);
    } else if (IsEqualGUID(&IID_IFilterMapper2, riid)) {
        TRACE("   returning IFilterMapper2 interface from aggregated filtermapper (%p)\n", *ppvObj);
        return IUnknown_QueryInterface(This->punkFilterMapper2, riid, ppvObj);
    } else if (IsEqualGUID(&IID_IFilterMapper3, riid)) {
        TRACE("   returning IFilterMapper3 interface from aggregated filtermapper (%p)\n", *ppvObj);
        return IUnknown_QueryInterface(This->punkFilterMapper2, riid, ppvObj);
    } else if (IsEqualGUID(&IID_IGraphVersion, riid)) {
        *ppvObj = &This->IGraphVersion_iface;
        TRACE("   returning IGraphVersion interface (%p)\n", *ppvObj);
    } else if (IsEqualGUID(&IID_IVideoFrameStep, riid)) {
        *ppvObj = &This->IVideoFrameStep_iface;
        TRACE("   returning IVideoFrameStep interface (%p)\n", *ppvObj);
    } else {
        *ppvObj = NULL;
	FIXME("unknown interface %s\n", debugstr_guid(riid));
	return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppvObj);
    return S_OK;
}

static ULONG WINAPI FilterGraphInner_AddRef(IUnknown *iface)
{
    struct filter_graph *graph = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&graph->ref);

    TRACE("%p increasing refcount to %lu.\n", graph, refcount);

    return refcount;
}

static ULONG WINAPI FilterGraphInner_Release(IUnknown *iface)
{
    struct filter_graph *This = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&This->ref);
    struct list *cursor;

    TRACE("%p decreasing refcount to %lu.\n", This, refcount);

    if (!refcount)
    {
        int i;

        This->ref = 1; /* guard against reentrancy (aggregation). */

        IMediaControl_Stop(&This->IMediaControl_iface);

        while ((cursor = list_head(&This->filters)))
        {
            struct filter *filter = LIST_ENTRY(cursor, struct filter, entry);

            IFilterGraph2_RemoveFilter(&This->IFilterGraph2_iface, filter->filter);
        }

        if (This->refClock)
            IReferenceClock_Release(This->refClock);

        for (i = 0; i < This->nItfCacheEntries; i++)
        {
            if (This->ItfCacheEntries[i].iface)
                IUnknown_Release(This->ItfCacheEntries[i].iface);
        }

        IUnknown_Release(This->punkFilterMapper2);

        if (This->pSite) IUnknown_Release(This->pSite);

        flush_media_events(This);
        CloseHandle(This->media_event_handle);

        EnterCriticalSection(&message_cs);
        if (This->threaded && !--message_thread_refcount)
        {
            PostThreadMessageW(message_thread_id, WM_USER + 1, 0, 0);
            WaitForSingleObject(message_thread, INFINITE);
            CloseHandle(message_thread);
            CloseHandle(message_thread_ret);
        }
        LeaveCriticalSection(&message_cs);

        This->event_cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->event_cs);
        This->cs.DebugInfo->Spare[0] = 0;
	DeleteCriticalSection(&This->cs);
        free(This);
    }
    return refcount;
}

static struct filter_graph *impl_from_IFilterGraph2(IFilterGraph2 *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IFilterGraph2_iface);
}

static HRESULT WINAPI FilterGraph2_QueryInterface(IFilterGraph2 *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);
    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI FilterGraph2_AddRef(IFilterGraph2 *iface)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);
    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI FilterGraph2_Release(IFilterGraph2 *iface)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);
    return IUnknown_Release(graph->outer_unk);
}

static IBaseFilter *find_filter_by_name(struct filter_graph *graph, const WCHAR *name)
{
    struct filter *filter;

    LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
    {
        if (!wcscmp(filter->name, name))
            return filter->filter;
    }

    return NULL;
}

static BOOL has_output_pins(IBaseFilter *filter)
{
    IEnumPins *enumpins;
    PIN_DIRECTION dir;
    IPin *pin;

    if (FAILED(IBaseFilter_EnumPins(filter, &enumpins)))
        return FALSE;

    while (IEnumPins_Next(enumpins, 1, &pin, NULL) == S_OK)
    {
        IPin_QueryDirection(pin, &dir);
        IPin_Release(pin);
        if (dir == PINDIR_OUTPUT)
        {
            IEnumPins_Release(enumpins);
            return TRUE;
        }
    }

    IEnumPins_Release(enumpins);
    return FALSE;
}

static void update_seeking(struct filter *filter)
{
    IMediaSeeking *seeking;

    if (!filter->seeking)
    {
        /* The Legend of Heroes: Trails of Cold Steel II destroys its filter when
         * its IMediaSeeking interface is released, so cache the interface instead
         * of querying for it every time.
         * Some filters (e.g. MediaStreamFilter) can become seekable when they are
         * already in the graph, so always try to query IMediaSeeking if it's not
         * cached yet. */
        if (SUCCEEDED(IBaseFilter_QueryInterface(filter->filter, &IID_IMediaSeeking, (void **)&seeking)))
        {
            if (IMediaSeeking_IsFormatSupported(seeking, &TIME_FORMAT_MEDIA_TIME) == S_OK)
                filter->seeking = seeking;
            else
                IMediaSeeking_Release(seeking);
        }
    }
}

static BOOL is_renderer(struct filter *filter)
{
    IMediaPosition *media_position;
    IAMFilterMiscFlags *flags;
    BOOL ret = FALSE;

    if (SUCCEEDED(IBaseFilter_QueryInterface(filter->filter, &IID_IAMFilterMiscFlags, (void **)&flags)))
    {
        if (IAMFilterMiscFlags_GetMiscFlags(flags) & AM_FILTER_MISC_FLAGS_IS_RENDERER)
            ret = TRUE;
        IAMFilterMiscFlags_Release(flags);
    }
    else if (SUCCEEDED(IBaseFilter_QueryInterface(filter->filter, &IID_IMediaPosition, (void **)&media_position)))
    {
        if (!has_output_pins(filter->filter))
            ret = TRUE;
        IMediaPosition_Release(media_position);
    }
    else
    {
        update_seeking(filter);
        if (filter->seeking && !has_output_pins(filter->filter))
            ret = TRUE;
    }
    return ret;
}

/*** IFilterGraph methods ***/
static HRESULT WINAPI FilterGraph2_AddFilter(IFilterGraph2 *iface,
        IBaseFilter *filter, const WCHAR *name)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);
    BOOL duplicate_name = FALSE;
    struct filter *entry;
    unsigned int i;
    HRESULT hr;

    TRACE("graph %p, filter %p, name %s.\n", graph, filter, debugstr_w(name));

    if (!filter)
        return E_POINTER;

    if (!(entry = heap_alloc(sizeof(*entry))))
        return E_OUTOFMEMORY;

    if (!(entry->name = CoTaskMemAlloc((name ? wcslen(name) + 6 : 5) * sizeof(WCHAR))))
    {
        heap_free(entry);
        return E_OUTOFMEMORY;
    }

    if (name && find_filter_by_name(graph, name))
        duplicate_name = TRUE;

    if (!name || duplicate_name)
    {
        for (i = 0; i < 10000 ; ++i)
        {
            if (name)
                swprintf(entry->name, name ? wcslen(name) + 6 : 5, L"%s %04u", name, graph->name_index);
            else
                swprintf(entry->name, name ? wcslen(name) + 6 : 5, L"%04u", graph->name_index);

            graph->name_index = (graph->name_index + 1) % 10000;

            if (!find_filter_by_name(graph, entry->name))
                break;
        }

        if (i == 10000)
        {
            CoTaskMemFree(entry->name);
            heap_free(entry);
            return VFW_E_DUPLICATE_NAME;
        }
    }
    else
        wcscpy(entry->name, name);

    if (FAILED(hr = IBaseFilter_JoinFilterGraph(filter,
            (IFilterGraph *)&graph->IFilterGraph2_iface, entry->name)))
    {
        CoTaskMemFree(entry->name);
        heap_free(entry);
        return hr;
    }

    IBaseFilter_SetSyncSource(filter, graph->refClock);

    IBaseFilter_AddRef(entry->filter = filter);

    list_add_head(&graph->filters, &entry->entry);
    entry->sorting = FALSE;
    entry->seeking = NULL;
    ++graph->version;

    return duplicate_name ? VFW_S_DUPLICATE_NAME : hr;
}

static HRESULT WINAPI FilterGraph2_RemoveFilter(IFilterGraph2 *iface, IBaseFilter *pFilter)
{
    struct filter_graph *This = impl_from_IFilterGraph2(iface);
    struct filter *entry;
    int i;
    HRESULT hr = E_FAIL;

    TRACE("(%p/%p)->(%p)\n", This, iface, pFilter);

    LIST_FOR_EACH_ENTRY(entry, &This->filters, struct filter, entry)
    {
        if (entry->filter == pFilter)
        {
            IEnumPins *penumpins = NULL;

            if (This->defaultclock && This->refClockProvider == pFilter)
            {
                IMediaFilter_SetSyncSource(&This->IMediaFilter_iface, NULL);
                This->defaultclock = TRUE;
            }

            TRACE("Removing filter %s.\n", debugstr_w(entry->name));

            hr = IBaseFilter_EnumPins(pFilter, &penumpins);
            if (SUCCEEDED(hr)) {
                IPin *ppin;
                while(IEnumPins_Next(penumpins, 1, &ppin, NULL) == S_OK)
                {
                    IPin *peer = NULL;
                    HRESULT hr;

                    IPin_ConnectedTo(ppin, &peer);
                    if (peer)
                    {
                        if (FAILED(hr = IPin_Disconnect(peer)))
                        {
                            WARN("Failed to disconnect peer %p, hr %#lx.\n", peer, hr);
                            IPin_Release(peer);
                            IPin_Release(ppin);
                            IEnumPins_Release(penumpins);
                            return hr;
                        }
                        IPin_Release(peer);

                        if (FAILED(hr = IPin_Disconnect(ppin)))
                        {
                            WARN("Failed to disconnect pin %p, hr %#lx.\n", ppin, hr);
                            IPin_Release(ppin);
                            IEnumPins_Release(penumpins);
                            return hr;
                        }
                    }
                    IPin_Release(ppin);
                }
                IEnumPins_Release(penumpins);
            }

            hr = IBaseFilter_JoinFilterGraph(pFilter, NULL, NULL);
            if (SUCCEEDED(hr))
            {
                IBaseFilter_SetSyncSource(pFilter, NULL);
                IBaseFilter_Release(pFilter);
                if (entry->seeking)
                    IMediaSeeking_Release(entry->seeking);
                list_remove(&entry->entry);
                CoTaskMemFree(entry->name);
                heap_free(entry);
                This->version++;
                /* Invalidate interfaces in the cache */
                for (i = 0; i < This->nItfCacheEntries; i++)
                    if (pFilter == This->ItfCacheEntries[i].filter)
                    {
                        IUnknown_Release(This->ItfCacheEntries[i].iface);
                        This->ItfCacheEntries[i].iface = NULL;
                        This->ItfCacheEntries[i].filter = NULL;
                    }
                return S_OK;
            }
            break;
        }
    }

    return hr; /* FIXME: check this error code */
}

static HRESULT WINAPI FilterGraph2_EnumFilters(IFilterGraph2 *iface, IEnumFilters **out)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);

    TRACE("graph %p, out %p.\n", graph, out);

    return create_enum_filters(graph, list_head(&graph->filters), out);
}

static HRESULT WINAPI FilterGraph2_FindFilterByName(IFilterGraph2 *iface,
        const WCHAR *name, IBaseFilter **filter)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);

    TRACE("graph %p, name %s, filter %p.\n", graph, debugstr_w(name), filter);

    if (!filter)
        return E_POINTER;

    if ((*filter = find_filter_by_name(graph, name)))
    {
        IBaseFilter_AddRef(*filter);
        return S_OK;
    }

    return VFW_E_NOT_FOUND;
}

static HRESULT check_cyclic_connection(IPin *source, IPin *sink)
{
    IPin *upstream_source, *upstream_sink;
    PIN_INFO source_info, sink_info;
    IEnumPins *enumpins;
    HRESULT hr;

    hr = IPin_QueryPinInfo(sink, &sink_info);
    if (FAILED(hr))
    {
        ERR("Failed to query pin, hr %#lx.\n", hr);
        return hr;
    }
    IBaseFilter_Release(sink_info.pFilter);

    hr = IPin_QueryPinInfo(source, &source_info);
    if (FAILED(hr))
    {
        ERR("Failed to query pin, hr %#lx.\n", hr);
        return hr;
    }

    if (sink_info.pFilter == source_info.pFilter)
    {
        WARN("Cyclic connection detected; returning VFW_E_CIRCULAR_GRAPH.\n");
        IBaseFilter_Release(source_info.pFilter);
        return VFW_E_CIRCULAR_GRAPH;
    }

    hr = IBaseFilter_EnumPins(source_info.pFilter, &enumpins);
    if (FAILED(hr))
    {
        ERR("Failed to enumerate pins, hr %#lx.\n", hr);
        IBaseFilter_Release(source_info.pFilter);
        return hr;
    }

    while ((hr = IEnumPins_Next(enumpins, 1, &upstream_sink, NULL)) == S_OK)
    {
        PIN_DIRECTION dir = PINDIR_OUTPUT;

        IPin_QueryDirection(upstream_sink, &dir);
        if (dir == PINDIR_INPUT && IPin_ConnectedTo(upstream_sink, &upstream_source) == S_OK)
        {
            hr = check_cyclic_connection(upstream_source, sink);
            IPin_Release(upstream_source);
            if (FAILED(hr))
            {
                IPin_Release(upstream_sink);
                IEnumPins_Release(enumpins);
                IBaseFilter_Release(source_info.pFilter);
                return hr;
            }
        }
        IPin_Release(upstream_sink);
    }
    IEnumPins_Release(enumpins);

    IBaseFilter_Release(source_info.pFilter);
    return S_OK;
}

static struct filter *find_sorted_filter(struct filter_graph *graph, IBaseFilter *iface)
{
    struct filter *filter;

    LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
    {
        if (filter->filter == iface)
            return filter;
    }

    return NULL;
}

static void sort_filter_recurse(struct filter_graph *graph, struct filter *filter, struct list *sorted)
{
    struct filter *peer_filter;
    IEnumPins *enumpins;
    PIN_DIRECTION dir;
    IPin *pin, *peer;
    PIN_INFO info;

    TRACE("Sorting filter %p.\n", filter->filter);

    /* Cyclic connections should be caught by check_cyclic_connection(). */
    assert(!filter->sorting);

    filter->sorting = TRUE;

    IBaseFilter_EnumPins(filter->filter, &enumpins);
    while (IEnumPins_Next(enumpins, 1, &pin, NULL) == S_OK)
    {
        IPin_QueryDirection(pin, &dir);

        if (dir == PINDIR_INPUT && IPin_ConnectedTo(pin, &peer) == S_OK)
        {
            IPin_QueryPinInfo(peer, &info);
            /* Note that the filter may have already been sorted. */
            if ((peer_filter = find_sorted_filter(graph, info.pFilter)))
                sort_filter_recurse(graph, peer_filter, sorted);
            IBaseFilter_Release(info.pFilter);
            IPin_Release(peer);
        }
        IPin_Release(pin);
    }
    IEnumPins_Release(enumpins);

    filter->sorting = FALSE;

    list_remove(&filter->entry);
    list_add_head(sorted, &filter->entry);
}

static void sort_filters(struct filter_graph *graph)
{
    struct list sorted = LIST_INIT(sorted), *cursor;

    while ((cursor = list_head(&graph->filters)))
    {
        struct filter *filter = LIST_ENTRY(cursor, struct filter, entry);
        sort_filter_recurse(graph, filter, &sorted);
    }

    list_move_tail(&graph->filters, &sorted);
}

/* NOTE: despite the implication, it doesn't matter which
 * way round you put in the input and output pins */
static HRESULT WINAPI FilterGraph2_ConnectDirect(IFilterGraph2 *iface, IPin *ppinIn, IPin *ppinOut,
        const AM_MEDIA_TYPE *pmt)
{
    struct filter_graph *This = impl_from_IFilterGraph2(iface);
    PIN_DIRECTION dir;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p)\n", This, iface, ppinIn, ppinOut, pmt);
    strmbase_dump_media_type(pmt);

    /* FIXME: check pins are in graph */

    if (TRACE_ON(quartz))
    {
        PIN_INFO PinInfo;

        hr = IPin_QueryPinInfo(ppinIn, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning ppinIn(%p) => %p\n", ppinIn, PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);

        hr = IPin_QueryPinInfo(ppinOut, &PinInfo);
        if (FAILED(hr))
            return hr;

        TRACE("Filter owning ppinOut(%p) => %p\n", ppinOut, PinInfo.pFilter);
        IBaseFilter_Release(PinInfo.pFilter);
    }

    hr = IPin_QueryDirection(ppinIn, &dir);
    if (SUCCEEDED(hr))
    {
        if (dir == PINDIR_INPUT)
        {
            hr = check_cyclic_connection(ppinOut, ppinIn);
            if (SUCCEEDED(hr))
                hr = IPin_Connect(ppinOut, ppinIn, pmt);
        }
        else
        {
            hr = check_cyclic_connection(ppinIn, ppinOut);
            if (SUCCEEDED(hr))
                hr = IPin_Connect(ppinIn, ppinOut, pmt);
        }
    }

    return hr;
}

static HRESULT WINAPI FilterGraph2_Reconnect(IFilterGraph2 *iface, IPin *pin)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);

    TRACE("graph %p, pin %p.\n", graph, pin);

    return IFilterGraph2_ReconnectEx(iface, pin, NULL);
}

static HRESULT WINAPI FilterGraph2_Disconnect(IFilterGraph2 *iface, IPin *ppin)
{
    struct filter_graph *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppin);

    if (!ppin)
       return E_POINTER;

    return IPin_Disconnect(ppin);
}

static HRESULT WINAPI FilterGraph2_SetDefaultSyncSource(IFilterGraph2 *iface)
{
    struct filter_graph *This = impl_from_IFilterGraph2(iface);
    IReferenceClock *pClock = NULL;
    struct filter *filter;
    HRESULT hr = S_OK;

    TRACE("(%p/%p)->() live sources not handled properly!\n", This, iface);

    EnterCriticalSection(&This->cs);

    LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
    {
        if (IBaseFilter_QueryInterface(filter->filter, &IID_IReferenceClock, (void **)&pClock) == S_OK)
            break;
    }

    if (!pClock)
    {
        hr = CoCreateInstance(&CLSID_SystemClock, NULL, CLSCTX_INPROC_SERVER, &IID_IReferenceClock, (LPVOID*)&pClock);
        This->refClockProvider = NULL;
    }
    else
    {
        filter = LIST_ENTRY(list_tail(&This->filters), struct filter, entry);
        This->refClockProvider = filter->filter;
    }

    if (SUCCEEDED(hr))
    {
        hr = IMediaFilter_SetSyncSource(&This->IMediaFilter_iface, pClock);
        This->defaultclock = TRUE;
        IReferenceClock_Release(pClock);
    }
    LeaveCriticalSection(&This->cs);

    return hr;
}

static DWORD WINAPI message_thread_run(void *ctx)
{
    MSG msg;

    SetThreadDescription(GetCurrentThread(), L"wine_qz_graph_worker");

    /* Make sure we have a message queue. */
    PeekMessageW(&msg, NULL, 0, 0, PM_NOREMOVE);
    SetEvent(message_thread_ret);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    for (;;)
    {
        GetMessageW(&msg, NULL, 0, 0);

        if (!msg.hwnd && msg.message == WM_USER)
        {
            struct filter_create_params *params = (struct filter_create_params *)msg.wParam;

            params->hr = IMoniker_BindToObject(params->moniker, NULL, NULL,
                    &IID_IBaseFilter, (void **)&params->filter);
            SetEvent(message_thread_ret);
        }
        else if (!msg.hwnd && msg.message == WM_USER + 1)
        {
            break;
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    CoUninitialize();
    return 0;
}

static HRESULT create_filter(struct filter_graph *graph, IMoniker *moniker, IBaseFilter **filter)
{
    if (graph->threaded)
    {
        struct filter_create_params params;

        params.moniker = moniker;

        EnterCriticalSection(&message_cs);
        PostThreadMessageW(message_thread_id, WM_USER, (WPARAM)&params, 0);
        WaitForSingleObject(message_thread_ret, INFINITE);
        LeaveCriticalSection(&message_cs);

        *filter = params.filter;
        return params.hr;
    }
    else
        return IMoniker_BindToObject(moniker, NULL, NULL, &IID_IBaseFilter, (void **)filter);
}

static HRESULT autoplug(struct filter_graph *graph, IPin *source, IPin *sink,
        BOOL render_to_existing, unsigned int recursion_depth);

static HRESULT autoplug_through_sink(struct filter_graph *graph, IPin *source,
        IBaseFilter *filter, IPin *middle_sink, IPin *sink,
        BOOL render_to_existing, unsigned int recursion_depth)
{
    BOOL any = FALSE, all = TRUE;
    IPin *middle_source, *peer;
    IEnumPins *source_enum;
    PIN_DIRECTION dir;
    PIN_INFO info;
    HRESULT hr;

    TRACE("Trying to autoplug %p to %p through %p.\n", source, sink, middle_sink);

    IPin_QueryDirection(middle_sink, &dir);
    if (dir != PINDIR_INPUT)
        return E_FAIL;

    if (IPin_ConnectedTo(middle_sink, &peer) == S_OK)
    {
        IPin_Release(peer);
        return E_FAIL;
    }

    if (FAILED(hr = IFilterGraph2_ConnectDirect(&graph->IFilterGraph2_iface, source, middle_sink, NULL)))
        return E_FAIL;

    if (FAILED(hr = IBaseFilter_EnumPins(filter, &source_enum)))
        goto err;

    while (IEnumPins_Next(source_enum, 1, &middle_source, NULL) == S_OK)
    {
        IPin_QueryPinInfo(middle_source, &info);
        IBaseFilter_Release(info.pFilter);
        if (info.dir != PINDIR_OUTPUT)
        {
            IPin_Release(middle_source);
            continue;
        }
        if (info.achName[0] == '~')
        {
            TRACE("Skipping non-rendered pin %s.\n", debugstr_w(info.achName));
            IPin_Release(middle_source);
            continue;
        }
        if (IPin_ConnectedTo(middle_source, &peer) == S_OK)
        {
            IPin_Release(peer);
            IPin_Release(middle_source);
            continue;
        }

        hr = autoplug(graph, middle_source, sink, render_to_existing, recursion_depth + 1);
        IPin_Release(middle_source);
        if (SUCCEEDED(hr) && sink)
        {
            IEnumPins_Release(source_enum);
            return hr;
        }
        if (SUCCEEDED(hr))
            any = TRUE;
        if (hr != S_OK)
            all = FALSE;
    }
    IEnumPins_Release(source_enum);

    if (!sink)
    {
        if (all)
            return S_OK;
        if (any)
            return VFW_S_PARTIAL_RENDER;
    }

err:
    IFilterGraph2_Disconnect(&graph->IFilterGraph2_iface, source);
    IFilterGraph2_Disconnect(&graph->IFilterGraph2_iface, middle_sink);
    return E_FAIL;
}

static HRESULT autoplug_through_filter(struct filter_graph *graph, IPin *source,
        IBaseFilter *filter, IPin *sink, BOOL render_to_existing,
        unsigned int recursion_depth)
{
    IEnumPins *sink_enum;
    IPin *filter_sink;
    HRESULT hr;

    TRACE("Trying to autoplug %p to %p through %p.\n", source, sink, filter);

    if (FAILED(hr = IBaseFilter_EnumPins(filter, &sink_enum)))
        return hr;

    while (IEnumPins_Next(sink_enum, 1, &filter_sink, NULL) == S_OK)
    {
        hr = autoplug_through_sink(graph, source, filter, filter_sink, sink,
                render_to_existing, recursion_depth);
        IPin_Release(filter_sink);
        if (SUCCEEDED(hr))
        {
            IEnumPins_Release(sink_enum);
            return hr;
        }
    }
    IEnumPins_Release(sink_enum);
    return VFW_E_CANNOT_CONNECT;
}

static HRESULT get_autoplug_types(IPin *source, unsigned int *ret_count, GUID **ret_types)
{
    unsigned int i, mt_count = 0, mt_capacity = 16;
    AM_MEDIA_TYPE **mts = NULL;
    IEnumMediaTypes *enummt;
    GUID *types = NULL;
    HRESULT hr;

    if (FAILED(hr = IPin_EnumMediaTypes(source, &enummt)))
    {
        ERR("Failed to enumerate media types, hr %#lx.\n", hr);
        return hr;
    }

    for (;;)
    {
        ULONG count;

        if (!(mts = realloc(mts, mt_capacity * sizeof(*mts))))
        {
            hr = E_OUTOFMEMORY;
            goto out;
        }

        if (FAILED(hr = IEnumMediaTypes_Next(enummt, mt_capacity - mt_count, mts + mt_count, &count)))
        {
            ERR("Failed to get media types, hr %#lx.\n", hr);
            goto out;
        }

        mt_count += count;
        if (hr == S_FALSE)
            break;

        mt_capacity *= 2;
    }

    if (!(types = malloc(mt_count * 2 * sizeof(*types))))
    {
        hr = E_OUTOFMEMORY;
        goto out;
    }

    for (i = 0; i < mt_count; ++i)
    {
        types[i * 2] = mts[i]->majortype;
        types[i * 2 + 1] = mts[i]->subtype;
        DeleteMediaType(mts[i]);
    }

    *ret_count = mt_count;
    *ret_types = types;

    hr = S_OK;
out:
    free(mts);
    IEnumMediaTypes_Release(enummt);
    return hr;
}

/* Common helper for IGraphBuilder::Connect() and IGraphBuilder::Render(), which
 * share most of the same code. Render() calls this with a NULL sink. */
static HRESULT autoplug(struct filter_graph *graph, IPin *source, IPin *sink,
        BOOL render_to_existing, unsigned int recursion_depth)
{
    IAMGraphBuilderCallback *callback = NULL;
    struct filter *graph_filter;
    IEnumMoniker *enummoniker;
    unsigned int type_count;
    IFilterMapper2 *mapper;
    IBaseFilter *filter;
    GUID *types = NULL;
    IMoniker *moniker;
    HRESULT hr;

    TRACE("Trying to autoplug %p to %p, recursion depth %u.\n", source, sink, recursion_depth);

    if (recursion_depth >= 5)
    {
        WARN("Recursion depth has reached 5; aborting.\n");
        return VFW_E_CANNOT_CONNECT;
    }

    if (sink)
    {
        /* Try to connect directly to this sink. */
        hr = IFilterGraph2_ConnectDirect(&graph->IFilterGraph2_iface, source, sink, NULL);

        /* If direct connection succeeded, we should propagate that return value.
         * If it returned VFW_E_NOT_CONNECTED or VFW_E_NO_AUDIO_HARDWARE, then don't
         * even bother trying intermediate filters, since they won't succeed. */
        if (SUCCEEDED(hr) || hr == VFW_E_NOT_CONNECTED || hr == VFW_E_NO_AUDIO_HARDWARE)
            return hr;
    }

    /* Always prefer filters in the graph. */
    LIST_FOR_EACH_ENTRY(graph_filter, &graph->filters, struct filter, entry)
    {
        if (SUCCEEDED(hr = autoplug_through_filter(graph, source, graph_filter->filter,
                sink, render_to_existing, recursion_depth)))
            return hr;
    }

    IUnknown_QueryInterface(graph->punkFilterMapper2, &IID_IFilterMapper2, (void **)&mapper);

    if (FAILED(hr = get_autoplug_types(source, &type_count, &types)))
    {
        IFilterMapper2_Release(mapper);
        return hr;
    }

    if (graph->pSite)
        IUnknown_QueryInterface(graph->pSite, &IID_IAMGraphBuilderCallback, (void **)&callback);

    if (FAILED(hr = IFilterMapper2_EnumMatchingFilters(mapper, &enummoniker,
            0, FALSE, MERIT_UNLIKELY, TRUE, type_count, types, NULL, NULL, FALSE,
            render_to_existing, 0, NULL, NULL, NULL)))
        goto out;

    while (IEnumMoniker_Next(enummoniker, 1, &moniker, NULL) == S_OK)
    {
        IPropertyBag *bag;
        VARIANT var;

        VariantInit(&var);
        IMoniker_BindToStorage(moniker, NULL, NULL, &IID_IPropertyBag, (void **)&bag);
        hr = IPropertyBag_Read(bag, L"FriendlyName", &var, NULL);
        IPropertyBag_Release(bag);
        if (FAILED(hr))
        {
            IMoniker_Release(moniker);
            continue;
        }

        if (callback && FAILED(hr = IAMGraphBuilderCallback_SelectedFilter(callback, moniker)))
        {
            TRACE("Filter rejected by IAMGraphBuilderCallback::SelectedFilter(), hr %#lx.\n", hr);
            IMoniker_Release(moniker);
            continue;
        }

        hr = create_filter(graph, moniker, &filter);
        IMoniker_Release(moniker);
        if (FAILED(hr))
        {
            ERR("Failed to create filter for %s, hr %#lx.\n", debugstr_w(V_BSTR(&var)), hr);
            VariantClear(&var);
            continue;
        }

        if (callback && FAILED(hr = IAMGraphBuilderCallback_CreatedFilter(callback, filter)))
        {
            TRACE("Filter rejected by IAMGraphBuilderCallback::CreatedFilter(), hr %#lx.\n", hr);
            IBaseFilter_Release(filter);
            continue;
        }

        hr = IFilterGraph2_AddFilter(&graph->IFilterGraph2_iface, filter, V_BSTR(&var));
        VariantClear(&var);
        if (FAILED(hr))
        {
            ERR("Failed to add filter, hr %#lx.\n", hr);
            IBaseFilter_Release(filter);
            continue;
        }

        hr = autoplug_through_filter(graph, source, filter, sink, render_to_existing, recursion_depth);
        if (SUCCEEDED(hr))
        {
            IBaseFilter_Release(filter);
            goto out;
        }

        IFilterGraph2_RemoveFilter(&graph->IFilterGraph2_iface, filter);
        IBaseFilter_Release(filter);
    }
    IEnumMoniker_Release(enummoniker);

    hr = VFW_E_CANNOT_CONNECT;

out:
    free(types);
    if (callback) IAMGraphBuilderCallback_Release(callback);
    IFilterMapper2_Release(mapper);
    return hr;
}

static HRESULT WINAPI FilterGraph2_Connect(IFilterGraph2 *iface, IPin *source, IPin *sink)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);
    PIN_DIRECTION dir;
    HRESULT hr;

    TRACE("graph %p, source %p, sink %p.\n", graph, source, sink);

    if (!source || !sink)
        return E_POINTER;

    if (FAILED(hr = IPin_QueryDirection(source, &dir)))
        return hr;

    if (dir == PINDIR_INPUT)
    {
        IPin *temp;

        TRACE("Directions seem backwards, swapping pins\n");

        temp = sink;
        sink = source;
        source = temp;
    }

    EnterCriticalSection(&graph->cs);

    hr = autoplug(graph, source, sink, TRUE, 0);

    LeaveCriticalSection(&graph->cs);

    TRACE("Returning %#lx.\n", hr);
    return hr;
}

static HRESULT WINAPI FilterGraph2_Render(IFilterGraph2 *iface, IPin *source)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);
    HRESULT hr;

    TRACE("graph %p, source %p.\n", graph, source);

    EnterCriticalSection(&graph->cs);
    hr = autoplug(graph, source, NULL, FALSE, 0);
    LeaveCriticalSection(&graph->cs);
    if (hr == VFW_E_CANNOT_CONNECT)
        hr = VFW_E_CANNOT_RENDER;

    TRACE("Returning %#lx.\n", hr);
    return hr;
}

static HRESULT WINAPI FilterGraph2_RenderFile(IFilterGraph2 *iface, LPCWSTR lpcwstrFile,
        LPCWSTR lpcwstrPlayList)
{
    struct filter_graph *This = impl_from_IFilterGraph2(iface);
    IBaseFilter* preader = NULL;
    IPin* ppinreader = NULL;
    IEnumPins* penumpins = NULL;
    struct filter *filter;
    HRESULT hr;
    BOOL partial = FALSE;
    BOOL any = FALSE;

    TRACE("(%p/%p)->(%s, %s)\n", This, iface, debugstr_w(lpcwstrFile), debugstr_w(lpcwstrPlayList));

    if (lpcwstrPlayList != NULL)
        return E_INVALIDARG;

    hr = IFilterGraph2_AddSourceFilter(iface, lpcwstrFile, L"Reader", &preader);
    if (FAILED(hr))
        return hr;

    hr = IBaseFilter_EnumPins(preader, &penumpins);
    if (SUCCEEDED(hr))
    {
        while (IEnumPins_Next(penumpins, 1, &ppinreader, NULL) == S_OK)
        {
            PIN_DIRECTION dir;

            IPin_QueryDirection(ppinreader, &dir);
            if (dir == PINDIR_OUTPUT)
            {
                hr = IFilterGraph2_Render(iface, ppinreader);

                TRACE("Filters in chain:\n");
                LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
                    TRACE("- %s.\n", debugstr_w(filter->name));

                if (SUCCEEDED(hr))
                    any = TRUE;
                if (hr != S_OK)
                    partial = TRUE;
            }
            IPin_Release(ppinreader);
        }
        IEnumPins_Release(penumpins);

        if (!any)
        {
            if (FAILED(hr = IFilterGraph2_RemoveFilter(iface, preader)))
                ERR("Failed to remove source filter, hr %#lx.\n", hr);
            hr = VFW_E_CANNOT_RENDER;
        }
        else if (partial)
        {
            hr = VFW_S_PARTIAL_RENDER;
        }
        else
        {
            hr = S_OK;
        }
    }
    IBaseFilter_Release(preader);

    TRACE("Returning %#lx.\n", hr);
    return hr;
}

static HRESULT WINAPI FilterGraph2_AddSourceFilter(IFilterGraph2 *iface,
        const WCHAR *filename, const WCHAR *filter_name, IBaseFilter **ret_filter)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);
    IFileSourceFilter *filesource;
    IBaseFilter *filter;
    HRESULT hr;
    GUID clsid;

    TRACE("graph %p, filename %s, filter_name %s, ret_filter %p.\n",
            graph, debugstr_w(filename), debugstr_w(filter_name), ret_filter);

    if (!*filename)
        return VFW_E_NOT_FOUND;

    if (!get_media_type(filename, NULL, NULL, &clsid))
        clsid = CLSID_AsyncReader;
    TRACE("Using source filter %s.\n", debugstr_guid(&clsid));

    if (FAILED(hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER,
            &IID_IBaseFilter, (void **)&filter)))
    {
        WARN("Failed to create filter, hr %#lx.\n", hr);
        return hr;
    }

    if (FAILED(hr = IBaseFilter_QueryInterface(filter, &IID_IFileSourceFilter, (void **)&filesource)))
    {
        WARN("Failed to get IFileSourceFilter, hr %#lx.\n", hr);
        IBaseFilter_Release(filter);
        return hr;
    }

    hr = IFileSourceFilter_Load(filesource, filename, NULL);
    IFileSourceFilter_Release(filesource);
    if (FAILED(hr))
    {
        WARN("Failed to load file, hr %#lx.\n", hr);
        IBaseFilter_Release(filter);
        return hr;
    }

    if (FAILED(hr = IFilterGraph2_AddFilter(iface, filter, filter_name)))
    {
        IBaseFilter_Release(filter);
        return hr;
    }

    if (ret_filter)
        *ret_filter = filter;
    return S_OK;
}

static HRESULT WINAPI FilterGraph2_SetLogFile(IFilterGraph2 *iface, DWORD_PTR file)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);

    TRACE("graph %p, file %#Ix.\n", graph, file);

    return S_OK;
}

static HRESULT WINAPI FilterGraph2_Abort(IFilterGraph2 *iface)
{
    struct filter_graph *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

static HRESULT WINAPI FilterGraph2_ShouldOperationContinue(IFilterGraph2 *iface)
{
    struct filter_graph *This = impl_from_IFilterGraph2(iface);

    TRACE("(%p/%p)->(): stub !!!\n", This, iface);

    return S_OK;
}

/*** IFilterGraph2 methods ***/
static HRESULT WINAPI FilterGraph2_AddSourceFilterForMoniker(IFilterGraph2 *iface,
        IMoniker *pMoniker, IBindCtx *pCtx, LPCWSTR lpcwstrFilterName, IBaseFilter **ppFilter)
{
    struct filter_graph *This = impl_from_IFilterGraph2(iface);
    HRESULT hr;
    IBaseFilter* pfilter;

    TRACE("(%p/%p)->(%p %p %s %p)\n", This, iface, pMoniker, pCtx, debugstr_w(lpcwstrFilterName), ppFilter);

    hr = IMoniker_BindToObject(pMoniker, pCtx, NULL, &IID_IBaseFilter, (void**)&pfilter);
    if(FAILED(hr)) {
        WARN("Failed to bind moniker, hr %#lx.\n", hr);
        return hr;
    }

    hr = IFilterGraph2_AddFilter(iface, pfilter, lpcwstrFilterName);
    if (FAILED(hr)) {
        WARN("Failed to add filter, hr %#lx.\n", hr);
        IBaseFilter_Release(pfilter);
        return hr;
    }

    if(ppFilter)
        *ppFilter = pfilter;
    else IBaseFilter_Release(pfilter);

    return S_OK;
}

static HRESULT WINAPI FilterGraph2_ReconnectEx(IFilterGraph2 *iface, IPin *pin, const AM_MEDIA_TYPE *mt)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);
    PIN_DIRECTION dir;
    HRESULT hr;
    IPin *peer;

    TRACE("graph %p, pin %p, mt %p.\n", graph, pin, mt);

    if (FAILED(hr = IPin_ConnectedTo(pin, &peer)))
        return hr;

    IPin_QueryDirection(pin, &dir);
    IFilterGraph2_Disconnect(iface, peer);
    IFilterGraph2_Disconnect(iface, pin);

    if (dir == PINDIR_INPUT)
        hr = IFilterGraph2_ConnectDirect(iface, peer, pin, mt);
    else
        hr = IFilterGraph2_ConnectDirect(iface, pin, peer, mt);

    IPin_Release(peer);
    return hr;
}

static HRESULT WINAPI FilterGraph2_RenderEx(IFilterGraph2 *iface, IPin *source, DWORD flags, DWORD *context)
{
    struct filter_graph *graph = impl_from_IFilterGraph2(iface);
    HRESULT hr;

    TRACE("graph %p, source %p, flags %#lx, context %p.\n", graph, source, flags, context);

    if (flags & ~AM_RENDEREX_RENDERTOEXISTINGRENDERERS)
        FIXME("Unknown flags %#lx.\n", flags);

    EnterCriticalSection(&graph->cs);
    hr = autoplug(graph, source, NULL, !!(flags & AM_RENDEREX_RENDERTOEXISTINGRENDERERS), 0);
    LeaveCriticalSection(&graph->cs);
    if (hr == VFW_E_CANNOT_CONNECT)
        hr = VFW_E_CANNOT_RENDER;

    TRACE("Returning %#lx.\n", hr);
    return hr;
}


static const IFilterGraph2Vtbl IFilterGraph2_VTable =
{
    FilterGraph2_QueryInterface,
    FilterGraph2_AddRef,
    FilterGraph2_Release,
    FilterGraph2_AddFilter,
    FilterGraph2_RemoveFilter,
    FilterGraph2_EnumFilters,
    FilterGraph2_FindFilterByName,
    FilterGraph2_ConnectDirect,
    FilterGraph2_Reconnect,
    FilterGraph2_Disconnect,
    FilterGraph2_SetDefaultSyncSource,
    FilterGraph2_Connect,
    FilterGraph2_Render,
    FilterGraph2_RenderFile,
    FilterGraph2_AddSourceFilter,
    FilterGraph2_SetLogFile,
    FilterGraph2_Abort,
    FilterGraph2_ShouldOperationContinue,
    FilterGraph2_AddSourceFilterForMoniker,
    FilterGraph2_ReconnectEx,
    FilterGraph2_RenderEx
};

static struct filter_graph *impl_from_IMediaControl(IMediaControl *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IMediaControl_iface);
}

static HRESULT WINAPI MediaControl_QueryInterface(IMediaControl *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IMediaControl(iface);
    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI MediaControl_AddRef(IMediaControl *iface)
{
    struct filter_graph *graph = impl_from_IMediaControl(iface);
    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI MediaControl_Release(IMediaControl *iface)
{
    struct filter_graph *graph = impl_from_IMediaControl(iface);
    return IUnknown_Release(graph->outer_unk);

}

static HRESULT WINAPI MediaControl_GetTypeInfoCount(IMediaControl *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI MediaControl_GetTypeInfo(IMediaControl *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#lx, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IMediaControl_tid, typeinfo);
}

static HRESULT WINAPI MediaControl_GetIDsOfNames(IMediaControl *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#lx, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IMediaControl_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI MediaControl_Invoke(IMediaControl *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %ld, iid %s, lcid %#lx, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IMediaControl_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static void update_render_count(struct filter_graph *graph)
{
    /* Some filters (e.g. MediaStreamFilter) can become renderers when they are
     * already in the graph. */
    struct filter *filter;
    graph->nRenderers = 0;
    LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
    {
        if (is_renderer(filter))
            ++graph->nRenderers;
    }
}

/* Perform the paused -> running transition. The caller must hold graph->cs. */
static HRESULT graph_start(struct filter_graph *graph, REFERENCE_TIME stream_start)
{
    struct media_event *event, *next;
    REFERENCE_TIME stream_stop;
    struct filter *filter;
    HRESULT hr = S_OK;

    EnterCriticalSection(&graph->event_cs);
    graph->EcCompleteCount = 0;
    update_render_count(graph);
    LeaveCriticalSection(&graph->event_cs);

    LIST_FOR_EACH_ENTRY_SAFE(event, next, &graph->media_events, struct media_event, entry)
    {
        if (event->code == EC_COMPLETE)
        {
            list_remove(&event->entry);
            free(event);
        }
    }
    if (list_empty(&graph->media_events))
        ResetEvent(graph->media_event_handle);

    if (graph->defaultclock && !graph->refClock)
        IFilterGraph2_SetDefaultSyncSource(&graph->IFilterGraph2_iface);

    if (!stream_start && graph->refClock)
    {
        IReferenceClock_GetTime(graph->refClock, &graph->stream_start);
        stream_start = graph->stream_start - graph->stream_elapsed;
        /* Delay presentation time by 200 ms, to give filters time to
         * initialize. */
        stream_start += 200 * 10000;
    }

    if (SUCCEEDED(IMediaSeeking_GetStopPosition(&graph->IMediaSeeking_iface, &stream_stop)))
        graph->stream_stop = stream_stop;

    LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
    {
        HRESULT filter_hr = IBaseFilter_Run(filter->filter, stream_start);
        if (hr == S_OK)
            hr = filter_hr;
        TRACE("Filter %p returned %#lx.\n", filter->filter, filter_hr);
    }

    if (FAILED(hr))
        WARN("Failed to start stream, hr %#lx.\n", hr);

    return hr;
}

static void CALLBACK async_run_cb(TP_CALLBACK_INSTANCE *instance, void *context, TP_WORK *work)
{
    struct filter_graph *graph = context;
    struct filter *filter;
    FILTER_STATE state;
    HRESULT hr;

    TRACE("Performing asynchronous state change.\n");

    /* We can't just call GetState(), since that will return State_Running and
     * VFW_S_STATE_INTERMEDIATE regardless of whether we're done pausing yet.
     * Instead replicate it here. */

    for (;;)
    {
        IBaseFilter *async_filter = NULL;

        hr = S_OK;

        EnterCriticalSection(&graph->cs);

        if (!graph->needs_async_run)
            break;

        LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
        {
            hr = IBaseFilter_GetState(filter->filter, 0, &state);

            if (hr == VFW_S_STATE_INTERMEDIATE)
                async_filter = filter->filter;

            if (SUCCEEDED(hr) && state != State_Paused)
                ERR("Filter %p reported incorrect state %u.\n", filter->filter, state);

            if (hr != S_OK)
                break;
        }

        if (hr != VFW_S_STATE_INTERMEDIATE)
            break;

        LeaveCriticalSection(&graph->cs);

        IBaseFilter_GetState(async_filter, 10, &state);
    }

    if (hr == S_OK && graph->needs_async_run)
    {
        sort_filters(graph);
        graph_start(graph, 0);
        graph->needs_async_run = 0;
    }

    LeaveCriticalSection(&graph->cs);
}

static HRESULT WINAPI MediaControl_Run(IMediaControl *iface)
{
    struct filter_graph *graph = impl_from_IMediaControl(iface);
    BOOL need_async_run = TRUE;
    struct filter *filter;
    FILTER_STATE state;
    HRESULT hr = S_OK;

    TRACE("graph %p.\n", graph);

    EnterCriticalSection(&graph->cs);

    if (graph->state == State_Running)
    {
        LeaveCriticalSection(&graph->cs);
        return S_OK;
    }

    sort_filters(graph);

    EnterCriticalSection(&graph->event_cs);
    update_render_count(graph);
    LeaveCriticalSection(&graph->event_cs);

    if (graph->state == State_Stopped)
    {
        if (graph->defaultclock && !graph->refClock)
            IFilterGraph2_SetDefaultSyncSource(&graph->IFilterGraph2_iface);

        LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
        {
            HRESULT filter_hr = IBaseFilter_Pause(filter->filter);
            if (hr == S_OK)
                hr = filter_hr;
            TRACE("Filter %p returned %#lx.\n", filter->filter, filter_hr);

            /* If a filter returns VFW_S_CANT_CUE, we shouldn't wait for a
             * paused state. */
            filter_hr = IBaseFilter_GetState(filter->filter, 0, &state);
            if (filter_hr != S_OK && filter_hr != VFW_S_STATE_INTERMEDIATE)
                need_async_run = FALSE;
        }

        if (FAILED(hr))
        {
            LeaveCriticalSection(&graph->cs);
            WARN("Failed to pause, hr %#lx.\n", hr);
            return hr;
        }
    }

    graph->state = State_Running;

    if (SUCCEEDED(hr))
    {
        if (hr != S_OK && need_async_run)
        {
            if (!graph->async_run_work)
                graph->async_run_work = CreateThreadpoolWork(async_run_cb, graph, NULL);
            graph->needs_async_run = 1;
            SubmitThreadpoolWork(graph->async_run_work);
        }
        else
        {
            hr = graph_start(graph, 0);
        }
    }

    LeaveCriticalSection(&graph->cs);
    return hr;
}

static HRESULT WINAPI MediaControl_Pause(IMediaControl *iface)
{
    struct filter_graph *graph = impl_from_IMediaControl(iface);

    TRACE("graph %p.\n", graph);

    return IMediaFilter_Pause(&graph->IMediaFilter_iface);
}

static HRESULT WINAPI MediaControl_Stop(IMediaControl *iface)
{
    struct filter_graph *graph = impl_from_IMediaControl(iface);

    TRACE("graph %p.\n", graph);

    return IMediaFilter_Stop(&graph->IMediaFilter_iface);
}

static HRESULT WINAPI MediaControl_GetState(IMediaControl *iface, LONG timeout, OAFilterState *state)
{
    struct filter_graph *graph = impl_from_IMediaControl(iface);

    TRACE("graph %p, timeout %ld, state %p.\n", graph, timeout, state);

    if (timeout < 0) timeout = INFINITE;

    return IMediaFilter_GetState(&graph->IMediaFilter_iface, timeout, (FILTER_STATE *)state);
}

static HRESULT WINAPI MediaControl_RenderFile(IMediaControl *iface, BSTR strFilename)
{
    struct filter_graph *This = impl_from_IMediaControl(iface);

    TRACE("(%p/%p)->(%s (%p))\n", This, iface, debugstr_w(strFilename), strFilename);

    return IFilterGraph2_RenderFile(&This->IFilterGraph2_iface, strFilename, NULL);
}

static HRESULT WINAPI MediaControl_AddSourceFilter(IMediaControl *iface, BSTR strFilename,
        IDispatch **ppUnk)
{
    struct filter_graph *This = impl_from_IMediaControl(iface);

    FIXME("(%p/%p)->(%s (%p), %p): stub !!!\n", This, iface, debugstr_w(strFilename), strFilename, ppUnk);

    return S_OK;
}

static HRESULT WINAPI MediaControl_get_FilterCollection(IMediaControl *iface, IDispatch **ppUnk)
{
    struct filter_graph *This = impl_from_IMediaControl(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, ppUnk);

    return S_OK;
}

static HRESULT WINAPI MediaControl_get_RegFilterCollection(IMediaControl *iface, IDispatch **ppUnk)
{
    struct filter_graph *This = impl_from_IMediaControl(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, ppUnk);

    return S_OK;
}

static void CALLBACK wait_pause_cb(TP_CALLBACK_INSTANCE *instance, void *context)
{
    IMediaControl *control = context;
    OAFilterState state;
    HRESULT hr;

    if ((hr = IMediaControl_GetState(control, INFINITE, &state)) != S_OK)
        ERR("Failed to get paused state, hr %#lx.\n", hr);

    if (FAILED(hr = IMediaControl_Stop(control)))
        ERR("Failed to stop, hr %#lx.\n", hr);

    if ((hr = IMediaControl_GetState(control, INFINITE, &state)) != S_OK)
        ERR("Failed to get paused state, hr %#lx.\n", hr);

    IMediaControl_Release(control);
}

static void CALLBACK wait_stop_cb(TP_CALLBACK_INSTANCE *instance, void *context)
{
    IMediaControl *control = context;
    OAFilterState state;
    HRESULT hr;

    if ((hr = IMediaControl_GetState(control, INFINITE, &state)) != S_OK)
        ERR("Failed to get state, hr %#lx.\n", hr);

    IMediaControl_Release(control);
}

static HRESULT WINAPI MediaControl_StopWhenReady(IMediaControl *iface)
{
    struct filter_graph *graph = impl_from_IMediaControl(iface);
    HRESULT hr;

    TRACE("graph %p.\n", graph);

    /* Even if we are already stopped, we still pause. */
    hr = IMediaControl_Pause(iface);
    if (FAILED(hr))
        return hr;
    else if (hr == S_FALSE)
    {
        IMediaControl_AddRef(iface);
        TrySubmitThreadpoolCallback(wait_pause_cb, iface, NULL);
        return S_FALSE;
    }

    hr = IMediaControl_Stop(iface);
    if (FAILED(hr))
        return hr;
    else if (hr == S_FALSE)
    {
        IMediaControl_AddRef(iface);
        TrySubmitThreadpoolCallback(wait_stop_cb, iface, NULL);
        return S_FALSE;
    }

    return S_OK;
}


static const IMediaControlVtbl IMediaControl_VTable =
{
    MediaControl_QueryInterface,
    MediaControl_AddRef,
    MediaControl_Release,
    MediaControl_GetTypeInfoCount,
    MediaControl_GetTypeInfo,
    MediaControl_GetIDsOfNames,
    MediaControl_Invoke,
    MediaControl_Run,
    MediaControl_Pause,
    MediaControl_Stop,
    MediaControl_GetState,
    MediaControl_RenderFile,
    MediaControl_AddSourceFilter,
    MediaControl_get_FilterCollection,
    MediaControl_get_RegFilterCollection,
    MediaControl_StopWhenReady
};

static struct filter_graph *impl_from_IMediaSeeking(IMediaSeeking *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IMediaSeeking_iface);
}

static HRESULT WINAPI MediaSeeking_QueryInterface(IMediaSeeking *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IMediaSeeking(iface);
    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI MediaSeeking_AddRef(IMediaSeeking *iface)
{
    struct filter_graph *graph = impl_from_IMediaSeeking(iface);
    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI MediaSeeking_Release(IMediaSeeking *iface)
{
    struct filter_graph *graph = impl_from_IMediaSeeking(iface);
    return IUnknown_Release(graph->outer_unk);
}

typedef HRESULT (WINAPI *fnFoundSeek)(struct filter_graph *This, IMediaSeeking*, DWORD_PTR arg);

static HRESULT all_renderers_seek(struct filter_graph *This, fnFoundSeek FoundSeek, DWORD_PTR arg) {
    BOOL allnotimpl = TRUE;
    HRESULT hr, hr_return = S_OK;
    struct filter *filter;

    LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
    {
        update_seeking(filter);
        if (!filter->seeking)
            continue;
        hr = FoundSeek(This, filter->seeking, arg);
        if (hr_return != E_NOTIMPL)
            allnotimpl = FALSE;
        if (hr_return == S_OK || (FAILED(hr) && hr != E_NOTIMPL && SUCCEEDED(hr_return)))
            hr_return = hr;
    }

    if (allnotimpl)
        return E_NOTIMPL;
    return hr_return;
}

static HRESULT WINAPI FoundCapabilities(struct filter_graph *This, IMediaSeeking *seek, DWORD_PTR pcaps)
{
    HRESULT hr;
    DWORD caps = 0;

    hr = IMediaSeeking_GetCapabilities(seek, &caps);
    if (FAILED(hr))
        return hr;

    /* Only add common capabilities everything supports */
    *(DWORD*)pcaps &= caps;

    return hr;
}

/*** IMediaSeeking methods ***/
static HRESULT WINAPI MediaSeeking_GetCapabilities(IMediaSeeking *iface, DWORD *pCapabilities)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pCapabilities);

    if (!pCapabilities)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    *pCapabilities = 0xffffffff;

    hr = all_renderers_seek(This, FoundCapabilities, (DWORD_PTR)pCapabilities);
    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MediaSeeking_CheckCapabilities(IMediaSeeking *iface, DWORD *pCapabilities)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);
    DWORD originalcaps;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pCapabilities);

    if (!pCapabilities)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    originalcaps = *pCapabilities;
    hr = all_renderers_seek(This, FoundCapabilities, (DWORD_PTR)pCapabilities);
    LeaveCriticalSection(&This->cs);

    if (FAILED(hr))
        return hr;

    if (!*pCapabilities)
        return E_FAIL;
    if (*pCapabilities != originalcaps)
        return S_FALSE;
    return S_OK;
}

static HRESULT WINAPI MediaSeeking_IsFormatSupported(IMediaSeeking *iface, const GUID *pFormat)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);

    if (!pFormat)
        return E_POINTER;

    TRACE("(%p/%p)->(%s)\n", This, iface, debugstr_guid(pFormat));

    if (!IsEqualGUID(&TIME_FORMAT_MEDIA_TIME, pFormat))
    {
        WARN("Unhandled time format %s\n", debugstr_guid(pFormat));
        return S_FALSE;
    }

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_QueryPreferredFormat(IMediaSeeking *iface, GUID *pFormat)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);

    if (!pFormat)
        return E_POINTER;

    FIXME("(%p/%p)->(%p): semi-stub !!!\n", This, iface, pFormat);
    memcpy(pFormat, &TIME_FORMAT_MEDIA_TIME, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetTimeFormat(IMediaSeeking *iface, GUID *pFormat)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);

    if (!pFormat)
        return E_POINTER;

    TRACE("(%p/%p)->(%p)\n", This, iface, pFormat);
    memcpy(pFormat, &This->timeformatseek, sizeof(GUID));

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_IsUsingTimeFormat(IMediaSeeking *iface, const GUID *pFormat)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, pFormat);
    if (!pFormat)
        return E_POINTER;

    if (memcmp(pFormat, &This->timeformatseek, sizeof(GUID)))
        return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_SetTimeFormat(IMediaSeeking *iface, const GUID *pFormat)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);

    if (!pFormat)
        return E_POINTER;

    TRACE("(%p/%p)->(%s)\n", This, iface, debugstr_guid(pFormat));

    if (!IsEqualGUID(&TIME_FORMAT_MEDIA_TIME, pFormat))
    {
        FIXME("Unhandled time format %s\n", debugstr_guid(pFormat));
        return E_INVALIDARG;
    }

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetDuration(IMediaSeeking *iface, LONGLONG *duration)
{
    struct filter_graph *graph = impl_from_IMediaSeeking(iface);
    HRESULT hr = E_NOTIMPL, filter_hr;
    LONGLONG filter_duration;
    struct filter *filter;

    TRACE("graph %p, duration %p.\n", graph, duration);

    if (!duration)
        return E_POINTER;

    *duration = 0;

    EnterCriticalSection(&graph->cs);

    LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
    {
        update_seeking(filter);
        if (!filter->seeking)
            continue;

        filter_hr = IMediaSeeking_GetDuration(filter->seeking, &filter_duration);
        if (SUCCEEDED(filter_hr))
        {
            hr = S_OK;
            *duration = max(*duration, filter_duration);
        }
        else if (filter_hr != E_NOTIMPL)
        {
            LeaveCriticalSection(&graph->cs);
            return filter_hr;
        }
    }

    LeaveCriticalSection(&graph->cs);

    TRACE("Returning hr %#lx, duration %s (%s seconds).\n", hr,
            wine_dbgstr_longlong(*duration), debugstr_time(*duration));
    return hr;
}

static HRESULT WINAPI MediaSeeking_GetStopPosition(IMediaSeeking *iface, LONGLONG *stop)
{
    struct filter_graph *graph = impl_from_IMediaSeeking(iface);
    HRESULT hr = E_NOTIMPL, filter_hr;
    struct filter *filter;
    LONGLONG filter_stop;

    TRACE("graph %p, stop %p.\n", graph, stop);

    if (!stop)
        return E_POINTER;

    *stop = 0;

    EnterCriticalSection(&graph->cs);

    LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
    {
        update_seeking(filter);
        if (!filter->seeking)
            continue;

        filter_hr = IMediaSeeking_GetStopPosition(filter->seeking, &filter_stop);
        if (SUCCEEDED(filter_hr))
        {
            hr = S_OK;
            *stop = max(*stop, filter_stop);
        }
        else if (filter_hr != E_NOTIMPL)
        {
            LeaveCriticalSection(&graph->cs);
            return filter_hr;
        }
    }

    LeaveCriticalSection(&graph->cs);

    TRACE("Returning %s (%s seconds).\n", wine_dbgstr_longlong(*stop), debugstr_time(*stop));
    return hr;
}

static HRESULT WINAPI MediaSeeking_GetCurrentPosition(IMediaSeeking *iface, LONGLONG *current)
{
    struct filter_graph *graph = impl_from_IMediaSeeking(iface);
    LONGLONG ret = graph->current_pos;

    TRACE("graph %p, current %p.\n", graph, current);

    if (!current)
        return E_POINTER;

    EnterCriticalSection(&graph->cs);

    if (graph->got_ec_complete)
    {
        ret = graph->stream_stop;
    }
    else if (graph->state == State_Running && !graph->needs_async_run && graph->refClock)
    {
        REFERENCE_TIME time;
        IReferenceClock_GetTime(graph->refClock, &time);
        if (time)
            ret += time - graph->stream_start;
    }

    LeaveCriticalSection(&graph->cs);

    TRACE("Returning %s (%s seconds).\n", wine_dbgstr_longlong(ret), debugstr_time(ret));
    *current = ret;

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_ConvertTimeFormat(IMediaSeeking *iface, LONGLONG *pTarget,
        const GUID *pTargetFormat, LONGLONG Source, const GUID *pSourceFormat)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);

    TRACE("(%p/%p)->(%p, %s, 0x%s, %s)\n", This, iface, pTarget,
        debugstr_guid(pTargetFormat), wine_dbgstr_longlong(Source), debugstr_guid(pSourceFormat));

    if (!pSourceFormat)
        pSourceFormat = &This->timeformatseek;

    if (!pTargetFormat)
        pTargetFormat = &This->timeformatseek;

    if (IsEqualGUID(pTargetFormat, pSourceFormat))
        *pTarget = Source;
    else
        FIXME("conversion %s->%s not supported\n", debugstr_guid(pSourceFormat), debugstr_guid(pTargetFormat));

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_SetPositions(IMediaSeeking *iface, LONGLONG *current_ptr,
        DWORD current_flags, LONGLONG *stop_ptr, DWORD stop_flags)
{
    struct filter_graph *graph = impl_from_IMediaSeeking(iface);
    HRESULT hr = E_NOTIMPL, filter_hr;
    struct filter *filter;
    FILTER_STATE state;

    TRACE("graph %p, current %s, current_flags %#lx, stop %s, stop_flags %#lx.\n", graph,
            current_ptr ? wine_dbgstr_longlong(*current_ptr) : "<null>", current_flags,
            stop_ptr ? wine_dbgstr_longlong(*stop_ptr): "<null>", stop_flags);
    if (current_ptr)
        TRACE("Setting current position to %s (%s seconds).\n",
                wine_dbgstr_longlong(*current_ptr), debugstr_time(*current_ptr));
    if (stop_ptr)
        TRACE("Setting stop position to %s (%s seconds).\n",
                wine_dbgstr_longlong(*stop_ptr), debugstr_time(*stop_ptr));

    if ((current_flags & 0x7) != AM_SEEKING_AbsolutePositioning
            && (current_flags & 0x7) != AM_SEEKING_NoPositioning)
        FIXME("Unhandled current_flags %#lx.\n", current_flags & 0x7);

    if ((stop_flags & 0x7) != AM_SEEKING_NoPositioning
            && (stop_flags & 0x7) != AM_SEEKING_AbsolutePositioning)
        FIXME("Unhandled stop_flags %#lx.\n", stop_flags & 0x7);

    EnterCriticalSection(&graph->cs);

    state = graph->state;
    if (state == State_Running && !graph->needs_async_run)
        IMediaControl_Pause(&graph->IMediaControl_iface);

    LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
    {
        LONGLONG current = current_ptr ? *current_ptr : 0, stop = stop_ptr ? *stop_ptr : 0;

        update_seeking(filter);
        if (!filter->seeking)
            continue;

        filter_hr = IMediaSeeking_SetPositions(filter->seeking, &current,
                current_flags | AM_SEEKING_ReturnTime, &stop, stop_flags);
        if (SUCCEEDED(filter_hr))
        {
            hr = S_OK;

            if (current_ptr && (current_flags & AM_SEEKING_ReturnTime))
                *current_ptr = current;
            if (stop_ptr && (stop_flags & AM_SEEKING_ReturnTime))
                *stop_ptr = stop;
            graph->current_pos = current;
        }
        else if (filter_hr != E_NOTIMPL)
        {
            LeaveCriticalSection(&graph->cs);
            return filter_hr;
        }
    }

    if ((current_flags & 0x7) != AM_SEEKING_NoPositioning && graph->refClock)
    {
        IReferenceClock_GetTime(graph->refClock, &graph->stream_start);
        graph->stream_elapsed = 0;
    }

    if (state == State_Running && !graph->needs_async_run)
        IMediaControl_Run(&graph->IMediaControl_iface);

    LeaveCriticalSection(&graph->cs);
    return hr;
}

static HRESULT WINAPI MediaSeeking_GetPositions(IMediaSeeking *iface,
        LONGLONG *current, LONGLONG *stop)
{
    struct filter_graph *graph = impl_from_IMediaSeeking(iface);
    HRESULT hr = S_OK;

    TRACE("graph %p, current %p, stop %p.\n", graph, current, stop);

    if (current)
        hr = IMediaSeeking_GetCurrentPosition(iface, current);
    if (SUCCEEDED(hr) && stop)
        hr = IMediaSeeking_GetStopPosition(iface, stop);

    return hr;
}

static HRESULT WINAPI MediaSeeking_GetAvailable(IMediaSeeking *iface, LONGLONG *pEarliest,
        LONGLONG *pLatest)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);

    FIXME("(%p/%p)->(%p, %p): stub !!!\n", This, iface, pEarliest, pLatest);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_SetRate(IMediaSeeking *iface, double dRate)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);

    FIXME("(%p/%p)->(%f): stub !!!\n", This, iface, dRate);

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetRate(IMediaSeeking *iface, double *pdRate)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pdRate);

    if (!pdRate)
        return E_POINTER;

    *pdRate = 1.0;

    return S_OK;
}

static HRESULT WINAPI MediaSeeking_GetPreroll(IMediaSeeking *iface, LONGLONG *pllPreroll)
{
    struct filter_graph *This = impl_from_IMediaSeeking(iface);

    FIXME("(%p/%p)->(%p): stub !!!\n", This, iface, pllPreroll);

    return S_OK;
}


static const IMediaSeekingVtbl IMediaSeeking_VTable =
{
    MediaSeeking_QueryInterface,
    MediaSeeking_AddRef,
    MediaSeeking_Release,
    MediaSeeking_GetCapabilities,
    MediaSeeking_CheckCapabilities,
    MediaSeeking_IsFormatSupported,
    MediaSeeking_QueryPreferredFormat,
    MediaSeeking_GetTimeFormat,
    MediaSeeking_IsUsingTimeFormat,
    MediaSeeking_SetTimeFormat,
    MediaSeeking_GetDuration,
    MediaSeeking_GetStopPosition,
    MediaSeeking_GetCurrentPosition,
    MediaSeeking_ConvertTimeFormat,
    MediaSeeking_SetPositions,
    MediaSeeking_GetPositions,
    MediaSeeking_GetAvailable,
    MediaSeeking_SetRate,
    MediaSeeking_GetRate,
    MediaSeeking_GetPreroll
};

static struct filter_graph *impl_from_IMediaPosition(IMediaPosition *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IMediaPosition_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI MediaPosition_QueryInterface(IMediaPosition *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IMediaPosition(iface);
    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI MediaPosition_AddRef(IMediaPosition *iface)
{
    struct filter_graph *graph = impl_from_IMediaPosition(iface);
    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI MediaPosition_Release(IMediaPosition *iface)
{
    struct filter_graph *graph = impl_from_IMediaPosition(iface);
    return IUnknown_Release(graph->outer_unk);
}

static HRESULT WINAPI MediaPosition_GetTypeInfoCount(IMediaPosition *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI MediaPosition_GetTypeInfo(IMediaPosition *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#lx, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IMediaPosition_tid, typeinfo);
}

static HRESULT WINAPI MediaPosition_GetIDsOfNames(IMediaPosition *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#lx, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IMediaPosition_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI MediaPosition_Invoke(IMediaPosition *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %ld, iid %s, lcid %#lx, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IMediaPosition_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT ConvertFromREFTIME(IMediaSeeking *seek, REFTIME time_in, LONGLONG *time_out)
{
    GUID time_format;
    HRESULT hr;

    hr = MediaSeeking_GetTimeFormat(seek, &time_format);
    if (FAILED(hr))
        return hr;
    if (!IsEqualGUID(&TIME_FORMAT_MEDIA_TIME, &time_format))
    {
        FIXME("Unsupported time format.\n");
        return E_NOTIMPL;
    }

    *time_out = (LONGLONG) (time_in * 10000000); /* convert from 1 second intervals to 100 ns intervals */
    return S_OK;
}

static HRESULT ConvertToREFTIME(IMediaSeeking *seek, LONGLONG time_in, REFTIME *time_out)
{
    GUID time_format;
    HRESULT hr;

    hr = MediaSeeking_GetTimeFormat(seek, &time_format);
    if (FAILED(hr))
        return hr;
    if (!IsEqualGUID(&TIME_FORMAT_MEDIA_TIME, &time_format))
    {
        FIXME("Unsupported time format.\n");
        return E_NOTIMPL;
    }

    *time_out = (REFTIME)time_in / 10000000; /* convert from 100 ns intervals to 1 second intervals */
    return S_OK;
}

/*** IMediaPosition methods ***/
static HRESULT WINAPI MediaPosition_get_Duration(IMediaPosition * iface, REFTIME *plength)
{
    LONGLONG duration;
    struct filter_graph *This = impl_from_IMediaPosition( iface );
    HRESULT hr = IMediaSeeking_GetDuration(&This->IMediaSeeking_iface, &duration);
    if (FAILED(hr))
        return hr;
    return ConvertToREFTIME(&This->IMediaSeeking_iface, duration, plength);
}

static HRESULT WINAPI MediaPosition_put_CurrentPosition(IMediaPosition * iface, REFTIME llTime)
{
    struct filter_graph *This = impl_from_IMediaPosition( iface );
    LONGLONG reftime;
    HRESULT hr;

    hr = ConvertFromREFTIME(&This->IMediaSeeking_iface, llTime, &reftime);
    if (FAILED(hr))
        return hr;
    return IMediaSeeking_SetPositions(&This->IMediaSeeking_iface, &reftime,
            AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);
}

static HRESULT WINAPI MediaPosition_get_CurrentPosition(IMediaPosition * iface, REFTIME *pllTime)
{
    struct filter_graph *This = impl_from_IMediaPosition( iface );
    LONGLONG pos;
    HRESULT hr;

    hr = IMediaSeeking_GetCurrentPosition(&This->IMediaSeeking_iface, &pos);
    if (FAILED(hr))
        return hr;
    return ConvertToREFTIME(&This->IMediaSeeking_iface, pos, pllTime);
}

static HRESULT WINAPI MediaPosition_get_StopTime(IMediaPosition * iface, REFTIME *pllTime)
{
    struct filter_graph *This = impl_from_IMediaPosition( iface );
    LONGLONG pos;
    HRESULT hr = IMediaSeeking_GetStopPosition(&This->IMediaSeeking_iface, &pos);
    if (FAILED(hr))
        return hr;
    return ConvertToREFTIME(&This->IMediaSeeking_iface, pos, pllTime);
}

static HRESULT WINAPI MediaPosition_put_StopTime(IMediaPosition * iface, REFTIME llTime)
{
    struct filter_graph *This = impl_from_IMediaPosition( iface );
    LONGLONG reftime;
    HRESULT hr;

    hr = ConvertFromREFTIME(&This->IMediaSeeking_iface, llTime, &reftime);
    if (FAILED(hr))
        return hr;
    return IMediaSeeking_SetPositions(&This->IMediaSeeking_iface, NULL, AM_SEEKING_NoPositioning,
            &reftime, AM_SEEKING_AbsolutePositioning);
}

static HRESULT WINAPI MediaPosition_get_PrerollTime(IMediaPosition * iface, REFTIME *pllTime)
{
    FIXME("(%p)->(%p) stub!\n", iface, pllTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_PrerollTime(IMediaPosition * iface, REFTIME llTime)
{
    FIXME("(%p)->(%f) stub!\n", iface, llTime);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_put_Rate(IMediaPosition * iface, double dRate)
{
    struct filter_graph *This = impl_from_IMediaPosition( iface );
    return IMediaSeeking_SetRate(&This->IMediaSeeking_iface, dRate);
}

static HRESULT WINAPI MediaPosition_get_Rate(IMediaPosition * iface, double *pdRate)
{
    struct filter_graph *This = impl_from_IMediaPosition( iface );
    return IMediaSeeking_GetRate(&This->IMediaSeeking_iface, pdRate);
}

static HRESULT WINAPI MediaPosition_CanSeekForward(IMediaPosition * iface, LONG *pCanSeekForward)
{
    FIXME("(%p)->(%p) stub!\n", iface, pCanSeekForward);
    return E_NOTIMPL;
}

static HRESULT WINAPI MediaPosition_CanSeekBackward(IMediaPosition * iface, LONG *pCanSeekBackward)
{
    FIXME("(%p)->(%p) stub!\n", iface, pCanSeekBackward);
    return E_NOTIMPL;
}


static const IMediaPositionVtbl IMediaPosition_VTable =
{
    MediaPosition_QueryInterface,
    MediaPosition_AddRef,
    MediaPosition_Release,
    MediaPosition_GetTypeInfoCount,
    MediaPosition_GetTypeInfo,
    MediaPosition_GetIDsOfNames,
    MediaPosition_Invoke,
    MediaPosition_get_Duration,
    MediaPosition_put_CurrentPosition,
    MediaPosition_get_CurrentPosition,
    MediaPosition_get_StopTime,
    MediaPosition_put_StopTime,
    MediaPosition_get_PrerollTime,
    MediaPosition_put_PrerollTime,
    MediaPosition_put_Rate,
    MediaPosition_get_Rate,
    MediaPosition_CanSeekForward,
    MediaPosition_CanSeekBackward
};

static struct filter_graph *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IObjectWithSite_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI ObjectWithSite_QueryInterface(IObjectWithSite *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IObjectWithSite(iface);
    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI ObjectWithSite_AddRef(IObjectWithSite *iface)
{
    struct filter_graph *graph = impl_from_IObjectWithSite(iface);
    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI ObjectWithSite_Release(IObjectWithSite *iface)
{
    struct filter_graph *graph = impl_from_IObjectWithSite(iface);
    return IUnknown_Release(graph->outer_unk);
}

/*** IObjectWithSite methods ***/

static HRESULT WINAPI ObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *pUnkSite)
{
    struct filter_graph *This = impl_from_IObjectWithSite( iface );

    TRACE("(%p/%p)->()\n", This, iface);
    if (This->pSite) IUnknown_Release(This->pSite);
    This->pSite = pUnkSite;
    IUnknown_AddRef(This->pSite);
    return S_OK;
}

static HRESULT WINAPI ObjectWithSite_GetSite(IObjectWithSite *iface, REFIID riid, PVOID *ppvSite)
{
    struct filter_graph *This = impl_from_IObjectWithSite( iface );

    TRACE("(%p/%p)->(%s)\n", This, iface,debugstr_guid(riid));

    *ppvSite = NULL;
    if (!This->pSite)
        return E_FAIL;
    else
        return IUnknown_QueryInterface(This->pSite, riid, ppvSite);
}

static const IObjectWithSiteVtbl IObjectWithSite_VTable =
{
    ObjectWithSite_QueryInterface,
    ObjectWithSite_AddRef,
    ObjectWithSite_Release,
    ObjectWithSite_SetSite,
    ObjectWithSite_GetSite,
};

static HRESULT GetTargetInterface(struct filter_graph* pGraph, REFIID riid, LPVOID* ppvObj)
{
    struct filter *filter;
    HRESULT hr;
    int entry;

    /* Check if the interface type is already registered */
    for (entry = 0; entry < pGraph->nItfCacheEntries; entry++)
        if (riid == pGraph->ItfCacheEntries[entry].riid)
        {
            if (pGraph->ItfCacheEntries[entry].iface)
            {
                /* Return the interface if available */
                *ppvObj = pGraph->ItfCacheEntries[entry].iface;
                return S_OK;
            }
            break;
        }

    if (entry >= MAX_ITF_CACHE_ENTRIES)
    {
        FIXME("Not enough space to store interface in the cache\n");
        return E_OUTOFMEMORY;
    }

    /* Find a filter supporting the requested interface */
    LIST_FOR_EACH_ENTRY(filter, &pGraph->filters, struct filter, entry)
    {
        hr = IBaseFilter_QueryInterface(filter->filter, riid, ppvObj);
        if (hr == S_OK)
        {
            pGraph->ItfCacheEntries[entry].riid = riid;
            pGraph->ItfCacheEntries[entry].filter = filter->filter;
            pGraph->ItfCacheEntries[entry].iface = *ppvObj;
            if (entry >= pGraph->nItfCacheEntries)
                pGraph->nItfCacheEntries++;
            return S_OK;
        }
        if (hr != E_NOINTERFACE)
            return hr;
    }

    return IsEqualGUID(riid, &IID_IBasicAudio) ? E_NOTIMPL : E_NOINTERFACE;
}

static struct filter_graph *impl_from_IBasicAudio(IBasicAudio *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IBasicAudio_iface);
}

static HRESULT WINAPI BasicAudio_QueryInterface(IBasicAudio *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IBasicAudio(iface);
    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI BasicAudio_AddRef(IBasicAudio *iface)
{
    struct filter_graph *graph = impl_from_IBasicAudio(iface);
    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI BasicAudio_Release(IBasicAudio *iface)
{
    struct filter_graph *graph = impl_from_IBasicAudio(iface);
    return IUnknown_Release(graph->outer_unk);
}

static HRESULT WINAPI BasicAudio_GetTypeInfoCount(IBasicAudio *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI BasicAudio_GetTypeInfo(IBasicAudio *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#lx, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IBasicAudio_tid, typeinfo);
}

static HRESULT WINAPI BasicAudio_GetIDsOfNames(IBasicAudio *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#lx, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicAudio_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI BasicAudio_Invoke(IBasicAudio *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %ld, iid %s, lcid %#lx, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicAudio_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

/*** IBasicAudio methods ***/
static HRESULT WINAPI BasicAudio_put_Volume(IBasicAudio *iface, LONG lVolume)
{
    struct filter_graph *This = impl_from_IBasicAudio(iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("graph %p, volume %ld.\n", This, lVolume);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_put_Volume(pBasicAudio, lVolume);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_get_Volume(IBasicAudio *iface, LONG *plVolume)
{
    struct filter_graph *This = impl_from_IBasicAudio(iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, plVolume);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_get_Volume(pBasicAudio, plVolume);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_put_Balance(IBasicAudio *iface, LONG lBalance)
{
    struct filter_graph *This = impl_from_IBasicAudio(iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("graph %p, balance %ld.\n", This, lBalance);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_put_Balance(pBasicAudio, lBalance);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicAudio_get_Balance(IBasicAudio *iface, LONG *plBalance)
{
    struct filter_graph *This = impl_from_IBasicAudio(iface);
    IBasicAudio* pBasicAudio;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, plBalance);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicAudio, (LPVOID*)&pBasicAudio);

    if (hr == S_OK)
        hr = IBasicAudio_get_Balance(pBasicAudio, plBalance);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static const IBasicAudioVtbl IBasicAudio_VTable =
{
    BasicAudio_QueryInterface,
    BasicAudio_AddRef,
    BasicAudio_Release,
    BasicAudio_GetTypeInfoCount,
    BasicAudio_GetTypeInfo,
    BasicAudio_GetIDsOfNames,
    BasicAudio_Invoke,
    BasicAudio_put_Volume,
    BasicAudio_get_Volume,
    BasicAudio_put_Balance,
    BasicAudio_get_Balance
};

static struct filter_graph *impl_from_IBasicVideo2(IBasicVideo2 *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IBasicVideo2_iface);
}

static HRESULT WINAPI BasicVideo_QueryInterface(IBasicVideo2 *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IBasicVideo2(iface);
    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI BasicVideo_AddRef(IBasicVideo2 *iface)
{
    struct filter_graph *graph = impl_from_IBasicVideo2(iface);
    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI BasicVideo_Release(IBasicVideo2 *iface)
{
    struct filter_graph *graph = impl_from_IBasicVideo2(iface);
    return IUnknown_Release(graph->outer_unk);
}

static HRESULT WINAPI BasicVideo_GetTypeInfoCount(IBasicVideo2 *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI BasicVideo_GetTypeInfo(IBasicVideo2 *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#lx, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IBasicVideo_tid, typeinfo);
}

static HRESULT WINAPI BasicVideo_GetIDsOfNames(IBasicVideo2 *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#lx, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicVideo_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI BasicVideo_Invoke(IBasicVideo2 *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %ld, iid %s, lcid %#lx, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicVideo_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

/*** IBasicVideo methods ***/
static HRESULT WINAPI BasicVideo_get_AvgTimePerFrame(IBasicVideo2 *iface, REFTIME *pAvgTimePerFrame)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pAvgTimePerFrame);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_AvgTimePerFrame(pBasicVideo, pAvgTimePerFrame);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_BitRate(IBasicVideo2 *iface, LONG *pBitRate)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitRate);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_BitRate(pBasicVideo, pBitRate);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_BitErrorRate(IBasicVideo2 *iface, LONG *pBitErrorRate)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBitErrorRate);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_BitErrorRate(pBasicVideo, pBitErrorRate);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_VideoWidth(IBasicVideo2 *iface, LONG *pVideoWidth)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_VideoWidth(pBasicVideo, pVideoWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_VideoHeight(IBasicVideo2 *iface, LONG *pVideoHeight)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVideoHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_VideoHeight(pBasicVideo, pVideoHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceLeft(IBasicVideo2 *iface, LONG SourceLeft)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("graph %p, left %ld.\n", This, SourceLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceLeft(pBasicVideo, SourceLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceLeft(IBasicVideo2 *iface, LONG *pSourceLeft)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceLeft(pBasicVideo, pSourceLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceWidth(IBasicVideo2 *iface, LONG SourceWidth)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("graph %p, width %ld.\n", This, SourceWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceWidth(pBasicVideo, SourceWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceWidth(IBasicVideo2 *iface, LONG *pSourceWidth)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceWidth(pBasicVideo, pSourceWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceTop(IBasicVideo2 *iface, LONG SourceTop)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("graph %p, top %ld.\n", This, SourceTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceTop(pBasicVideo, SourceTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceTop(IBasicVideo2 *iface, LONG *pSourceTop)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceTop(pBasicVideo, pSourceTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_SourceHeight(IBasicVideo2 *iface, LONG SourceHeight)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("graph %p, height %ld.\n", This, SourceHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_SourceHeight(pBasicVideo, SourceHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_SourceHeight(IBasicVideo2 *iface, LONG *pSourceHeight)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pSourceHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_SourceHeight(pBasicVideo, pSourceHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationLeft(IBasicVideo2 *iface, LONG DestinationLeft)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("graph %p, left %ld.\n", This, DestinationLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationLeft(pBasicVideo, DestinationLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationLeft(IBasicVideo2 *iface, LONG *pDestinationLeft)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationLeft(pBasicVideo, pDestinationLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationWidth(IBasicVideo2 *iface, LONG DestinationWidth)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("graph %p, width %ld.\n", This, DestinationWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationWidth(pBasicVideo, DestinationWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationWidth(IBasicVideo2 *iface, LONG *pDestinationWidth)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationWidth(pBasicVideo, pDestinationWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationTop(IBasicVideo2 *iface, LONG DestinationTop)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("graph %p, top %ld.\n", This, DestinationTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationTop(pBasicVideo, DestinationTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationTop(IBasicVideo2 *iface, LONG *pDestinationTop)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationTop(pBasicVideo, pDestinationTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_put_DestinationHeight(IBasicVideo2 *iface, LONG DestinationHeight)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("graph %p, height %ld.\n", This, DestinationHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_put_DestinationHeight(pBasicVideo, DestinationHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_get_DestinationHeight(IBasicVideo2 *iface,
        LONG *pDestinationHeight)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pDestinationHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_get_DestinationHeight(pBasicVideo, pDestinationHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetSourcePosition(IBasicVideo2 *iface, LONG Left, LONG Top,
        LONG Width, LONG Height)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("graph %p, left %ld, top %ld, width %ld, height %ld.\n", This, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetSourcePosition(pBasicVideo, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetSourcePosition(IBasicVideo2 *iface, LONG *pLeft, LONG *pTop,
        LONG *pWidth, LONG *pHeight)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetSourcePosition(pBasicVideo, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDefaultSourcePosition(IBasicVideo2 *iface)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDefaultSourcePosition(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDestinationPosition(IBasicVideo2 *iface, LONG Left, LONG Top,
        LONG Width, LONG Height)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("graph %p, left %ld, top %ld, width %ld, height %ld.\n", This, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDestinationPosition(pBasicVideo, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetDestinationPosition(IBasicVideo2 *iface, LONG *pLeft,
        LONG *pTop, LONG *pWidth, LONG *pHeight)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetDestinationPosition(pBasicVideo, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_SetDefaultDestinationPosition(IBasicVideo2 *iface)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_SetDefaultDestinationPosition(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetVideoSize(IBasicVideo2 *iface, LONG *pWidth, LONG *pHeight)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetVideoSize(pBasicVideo, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetVideoPaletteEntries(IBasicVideo2 *iface, LONG StartIndex,
        LONG Entries, LONG *pRetrieved, LONG *pPalette)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("graph %p, start_index %ld, count %ld, ret_count %p, entries %p.\n",
            This, StartIndex, Entries, pRetrieved, pPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetVideoPaletteEntries(pBasicVideo, StartIndex, Entries, pRetrieved, pPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_GetCurrentImage(IBasicVideo2 *iface, LONG *pBufferSize,
        LONG *pDIBImage)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pBufferSize, pDIBImage);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_GetCurrentImage(pBasicVideo, pBufferSize, pDIBImage);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_IsUsingDefaultSource(IBasicVideo2 *iface)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_IsUsingDefaultSource(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo_IsUsingDefaultDestination(IBasicVideo2 *iface)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo *pBasicVideo;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo, (LPVOID*)&pBasicVideo);

    if (hr == S_OK)
        hr = IBasicVideo_IsUsingDefaultDestination(pBasicVideo);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI BasicVideo2_GetPreferredAspectRatio(IBasicVideo2 *iface, LONG *plAspectX,
        LONG *plAspectY)
{
    struct filter_graph *This = impl_from_IBasicVideo2(iface);
    IBasicVideo2 *pBasicVideo2;
    HRESULT hr;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IBasicVideo2, (LPVOID*)&pBasicVideo2);

    if (hr == S_OK)
        hr = BasicVideo2_GetPreferredAspectRatio(iface, plAspectX, plAspectY);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static const IBasicVideo2Vtbl IBasicVideo_VTable =
{
    BasicVideo_QueryInterface,
    BasicVideo_AddRef,
    BasicVideo_Release,
    BasicVideo_GetTypeInfoCount,
    BasicVideo_GetTypeInfo,
    BasicVideo_GetIDsOfNames,
    BasicVideo_Invoke,
    BasicVideo_get_AvgTimePerFrame,
    BasicVideo_get_BitRate,
    BasicVideo_get_BitErrorRate,
    BasicVideo_get_VideoWidth,
    BasicVideo_get_VideoHeight,
    BasicVideo_put_SourceLeft,
    BasicVideo_get_SourceLeft,
    BasicVideo_put_SourceWidth,
    BasicVideo_get_SourceWidth,
    BasicVideo_put_SourceTop,
    BasicVideo_get_SourceTop,
    BasicVideo_put_SourceHeight,
    BasicVideo_get_SourceHeight,
    BasicVideo_put_DestinationLeft,
    BasicVideo_get_DestinationLeft,
    BasicVideo_put_DestinationWidth,
    BasicVideo_get_DestinationWidth,
    BasicVideo_put_DestinationTop,
    BasicVideo_get_DestinationTop,
    BasicVideo_put_DestinationHeight,
    BasicVideo_get_DestinationHeight,
    BasicVideo_SetSourcePosition,
    BasicVideo_GetSourcePosition,
    BasicVideo_SetDefaultSourcePosition,
    BasicVideo_SetDestinationPosition,
    BasicVideo_GetDestinationPosition,
    BasicVideo_SetDefaultDestinationPosition,
    BasicVideo_GetVideoSize,
    BasicVideo_GetVideoPaletteEntries,
    BasicVideo_GetCurrentImage,
    BasicVideo_IsUsingDefaultSource,
    BasicVideo_IsUsingDefaultDestination,
    BasicVideo2_GetPreferredAspectRatio
};

static struct filter_graph *impl_from_IVideoWindow(IVideoWindow *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IVideoWindow_iface);
}

static HRESULT WINAPI VideoWindow_QueryInterface(IVideoWindow *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IVideoWindow(iface);
    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI VideoWindow_AddRef(IVideoWindow *iface)
{
    struct filter_graph *graph = impl_from_IVideoWindow(iface);
    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI VideoWindow_Release(IVideoWindow *iface)
{
    struct filter_graph *graph = impl_from_IVideoWindow(iface);
    return IUnknown_Release(graph->outer_unk);
}

HRESULT WINAPI VideoWindow_GetTypeInfoCount(IVideoWindow *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

HRESULT WINAPI VideoWindow_GetTypeInfo(IVideoWindow *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#lx, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IVideoWindow_tid, typeinfo);
}

HRESULT WINAPI VideoWindow_GetIDsOfNames(IVideoWindow *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#lx, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IVideoWindow_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI VideoWindow_Invoke(IVideoWindow *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %ld, iid %s, lcid %#lx, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IVideoWindow_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

/*** IVideoWindow methods ***/
static HRESULT WINAPI VideoWindow_put_Caption(IVideoWindow *iface, BSTR strCaption)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%s (%p))\n", This, iface, debugstr_w(strCaption), strCaption);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Caption(pVideoWindow, strCaption);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Caption(IVideoWindow *iface, BSTR *strCaption)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, strCaption);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Caption(pVideoWindow, strCaption);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowStyle(IVideoWindow *iface, LONG WindowStyle)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, style %#lx.\n", This, WindowStyle);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowStyle(pVideoWindow, WindowStyle);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowStyle(IVideoWindow *iface, LONG *WindowStyle)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyle);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowStyle(pVideoWindow, WindowStyle);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowStyleEx(IVideoWindow *iface, LONG WindowStyleEx)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, style %#lx.\n", This, WindowStyleEx);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowStyleEx(pVideoWindow, WindowStyleEx);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowStyleEx(IVideoWindow *iface, LONG *WindowStyleEx)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowStyleEx);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowStyleEx(pVideoWindow, WindowStyleEx);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_AutoShow(IVideoWindow *iface, LONG AutoShow)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, show %#lx.\n", This, AutoShow);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_AutoShow(pVideoWindow, AutoShow);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_AutoShow(IVideoWindow *iface, LONG *AutoShow)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, AutoShow);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_AutoShow(pVideoWindow, AutoShow);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_WindowState(IVideoWindow *iface, LONG WindowState)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, state %ld.\n", This, WindowState);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_WindowState(pVideoWindow, WindowState);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_WindowState(IVideoWindow *iface, LONG *WindowState)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, WindowState);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_WindowState(pVideoWindow, WindowState);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_BackgroundPalette(IVideoWindow *iface, LONG BackgroundPalette)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, palette %ld.\n", This, BackgroundPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_BackgroundPalette(pVideoWindow, BackgroundPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_BackgroundPalette(IVideoWindow *iface,
        LONG *pBackgroundPalette)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pBackgroundPalette);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_BackgroundPalette(pVideoWindow, pBackgroundPalette);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Visible(IVideoWindow *iface, LONG Visible)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, visible %ld.\n", This, Visible);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Visible(pVideoWindow, Visible);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Visible(IVideoWindow *iface, LONG *pVisible)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pVisible);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Visible(pVideoWindow, pVisible);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Left(IVideoWindow *iface, LONG Left)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, left %ld.\n", This, Left);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Left(pVideoWindow, Left);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Left(IVideoWindow *iface, LONG *pLeft)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pLeft);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Left(pVideoWindow, pLeft);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Width(IVideoWindow *iface, LONG Width)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, width %ld.\n", This, Width);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Width(pVideoWindow, Width);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Width(IVideoWindow *iface, LONG *pWidth)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pWidth);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Width(pVideoWindow, pWidth);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Top(IVideoWindow *iface, LONG Top)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, top %ld.\n", This, Top);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Top(pVideoWindow, Top);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Top(IVideoWindow *iface, LONG *pTop)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pTop);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Top(pVideoWindow, pTop);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Height(IVideoWindow *iface, LONG Height)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, height %ld.\n", This, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Height(pVideoWindow, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Height(IVideoWindow *iface, LONG *pHeight)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Height(pVideoWindow, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_Owner(IVideoWindow *iface, OAHWND Owner)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, owner %#Ix.\n", This, Owner);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_Owner(pVideoWindow, Owner);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_Owner(IVideoWindow *iface, OAHWND *Owner)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Owner);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_Owner(pVideoWindow, Owner);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_MessageDrain(IVideoWindow *iface, OAHWND Drain)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, drain %#Ix.\n", This, Drain);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_MessageDrain(pVideoWindow, Drain);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_MessageDrain(IVideoWindow *iface, OAHWND *Drain)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Drain);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_MessageDrain(pVideoWindow, Drain);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_BorderColor(IVideoWindow *iface, LONG *Color)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, Color);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_BorderColor(pVideoWindow, Color);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_BorderColor(IVideoWindow *iface, LONG Color)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, colour %#lx.\n", This, Color);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_BorderColor(pVideoWindow, Color);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_get_FullScreenMode(IVideoWindow *iface, LONG *FullScreenMode)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, FullScreenMode);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_get_FullScreenMode(pVideoWindow, FullScreenMode);
    if (hr == E_NOTIMPL)
    {
        *FullScreenMode = OAFALSE;
        hr = S_OK;
    }

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_put_FullScreenMode(IVideoWindow *iface, LONG FullScreenMode)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, fullscreen %ld.\n", This, FullScreenMode);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_put_FullScreenMode(pVideoWindow, FullScreenMode);
    if (hr == E_NOTIMPL && FullScreenMode == OAFALSE)
        hr = S_FALSE;

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_SetWindowForeground(IVideoWindow *iface, LONG Focus)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, focus %ld.\n", This, Focus);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_SetWindowForeground(pVideoWindow, Focus);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_NotifyOwnerMessage(IVideoWindow *iface, OAHWND hwnd, LONG uMsg,
        LONG_PTR wParam, LONG_PTR lParam)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, hwnd %#Ix, message %#lx, wparam %#Ix, lparam %#Ix.\n", This, hwnd, uMsg, wParam, lParam);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_NotifyOwnerMessage(pVideoWindow, hwnd, uMsg, wParam, lParam);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_SetWindowPosition(IVideoWindow *iface, LONG Left, LONG Top,
        LONG Width, LONG Height)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, left %ld, top %ld, width %ld, height %ld.\n", This, Left, Top, Width, Height);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_SetWindowPosition(pVideoWindow, Left, Top, Width, Height);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetWindowPosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop,
        LONG *pWidth, LONG *pHeight)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetWindowPosition(pVideoWindow, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetMinIdealImageSize(IVideoWindow *iface, LONG *pWidth,
        LONG *pHeight)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetMinIdealImageSize(pVideoWindow, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetMaxIdealImageSize(IVideoWindow *iface, LONG *pWidth,
        LONG *pHeight)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p)\n", This, iface, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetMaxIdealImageSize(pVideoWindow, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_GetRestorePosition(IVideoWindow *iface, LONG *pLeft, LONG *pTop,
        LONG *pWidth, LONG *pHeight)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p, %p, %p, %p)\n", This, iface, pLeft, pTop, pWidth, pHeight);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_GetRestorePosition(pVideoWindow, pLeft, pTop, pWidth, pHeight);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_HideCursor(IVideoWindow *iface, LONG HideCursor)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("graph %p, hide %ld.\n", This, HideCursor);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_HideCursor(pVideoWindow, HideCursor);

    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI VideoWindow_IsCursorHidden(IVideoWindow *iface, LONG *CursorHidden)
{
    struct filter_graph *This = impl_from_IVideoWindow(iface);
    IVideoWindow *pVideoWindow;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, CursorHidden);

    EnterCriticalSection(&This->cs);

    hr = GetTargetInterface(This, &IID_IVideoWindow, (LPVOID*)&pVideoWindow);

    if (hr == S_OK)
        hr = IVideoWindow_IsCursorHidden(pVideoWindow, CursorHidden);

    LeaveCriticalSection(&This->cs);

    return hr;
}


static const IVideoWindowVtbl IVideoWindow_VTable =
{
    VideoWindow_QueryInterface,
    VideoWindow_AddRef,
    VideoWindow_Release,
    VideoWindow_GetTypeInfoCount,
    VideoWindow_GetTypeInfo,
    VideoWindow_GetIDsOfNames,
    VideoWindow_Invoke,
    VideoWindow_put_Caption,
    VideoWindow_get_Caption,
    VideoWindow_put_WindowStyle,
    VideoWindow_get_WindowStyle,
    VideoWindow_put_WindowStyleEx,
    VideoWindow_get_WindowStyleEx,
    VideoWindow_put_AutoShow,
    VideoWindow_get_AutoShow,
    VideoWindow_put_WindowState,
    VideoWindow_get_WindowState,
    VideoWindow_put_BackgroundPalette,
    VideoWindow_get_BackgroundPalette,
    VideoWindow_put_Visible,
    VideoWindow_get_Visible,
    VideoWindow_put_Left,
    VideoWindow_get_Left,
    VideoWindow_put_Width,
    VideoWindow_get_Width,
    VideoWindow_put_Top,
    VideoWindow_get_Top,
    VideoWindow_put_Height,
    VideoWindow_get_Height,
    VideoWindow_put_Owner,
    VideoWindow_get_Owner,
    VideoWindow_put_MessageDrain,
    VideoWindow_get_MessageDrain,
    VideoWindow_get_BorderColor,
    VideoWindow_put_BorderColor,
    VideoWindow_get_FullScreenMode,
    VideoWindow_put_FullScreenMode,
    VideoWindow_SetWindowForeground,
    VideoWindow_NotifyOwnerMessage,
    VideoWindow_SetWindowPosition,
    VideoWindow_GetWindowPosition,
    VideoWindow_GetMinIdealImageSize,
    VideoWindow_GetMaxIdealImageSize,
    VideoWindow_GetRestorePosition,
    VideoWindow_HideCursor,
    VideoWindow_IsCursorHidden
};

static struct filter_graph *impl_from_IMediaEventEx(IMediaEventEx *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IMediaEventEx_iface);
}

static HRESULT WINAPI MediaEvent_QueryInterface(IMediaEventEx *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IMediaEventEx(iface);
    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI MediaEvent_AddRef(IMediaEventEx *iface)
{
    struct filter_graph *graph = impl_from_IMediaEventEx(iface);
    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI MediaEvent_Release(IMediaEventEx *iface)
{
    struct filter_graph *graph = impl_from_IMediaEventEx(iface);
    return IUnknown_Release(graph->outer_unk);
}

static HRESULT WINAPI MediaEvent_GetTypeInfoCount(IMediaEventEx *iface, UINT *count)
{
    TRACE("iface %p, count %p.\n", iface, count);
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetTypeInfo(IMediaEventEx *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    TRACE("iface %p, index %u, lcid %#lx, typeinfo %p.\n", iface, index, lcid, typeinfo);
    return strmbase_get_typeinfo(IMediaEvent_tid, typeinfo);
}

static HRESULT WINAPI MediaEvent_GetIDsOfNames(IMediaEventEx *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, iid %s, names %p, count %u, lcid %#lx, ids %p.\n",
            iface, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IMediaEvent_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI MediaEvent_Invoke(IMediaEventEx *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("iface %p, id %ld, iid %s, lcid %#lx, flags %#x, params %p, result %p, excepinfo %p, error_arg %p.\n",
            iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IMediaEvent_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI MediaEvent_GetEventHandle(IMediaEventEx *iface, OAEVENT *event)
{
    struct filter_graph *graph = impl_from_IMediaEventEx(iface);

    TRACE("graph %p, event %p.\n", graph, event);

    *event = (OAEVENT)graph->media_event_handle;
    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetEvent(IMediaEventEx *iface, LONG *code,
        LONG_PTR *param1, LONG_PTR *param2, LONG timeout)
{
    struct filter_graph *graph = impl_from_IMediaEventEx(iface);
    struct media_event *event;
    struct list *entry;

    TRACE("graph %p, code %p, param1 %p, param2 %p, timeout %ld.\n", graph, code, param1, param2, timeout);

    *code = 0;

    if (WaitForSingleObject(graph->media_event_handle, timeout))
        return E_ABORT;

    EnterCriticalSection(&graph->event_cs);

    if (!(entry = list_head(&graph->media_events)))
    {
        ResetEvent(graph->media_event_handle);
        LeaveCriticalSection(&graph->event_cs);
        return E_ABORT;
    }
    event = LIST_ENTRY(entry, struct media_event, entry);
    list_remove(&event->entry);
    *code = event->code;
    *param1 = event->param1;
    *param2 = event->param2;
    free(event);

    LeaveCriticalSection(&graph->event_cs);
    return S_OK;
}

static HRESULT WINAPI MediaEvent_WaitForCompletion(IMediaEventEx *iface, LONG msTimeout,
        LONG *pEvCode)
{
    struct filter_graph *This = impl_from_IMediaEventEx(iface);

    TRACE("graph %p, timeout %ld, code %p.\n", This, msTimeout, pEvCode);

    if (This->state != State_Running)
        return VFW_E_WRONG_STATE;

    if (WaitForSingleObject(This->hEventCompletion, msTimeout) == WAIT_OBJECT_0)
    {
	*pEvCode = This->CompletionStatus;
	return S_OK;
    }

    *pEvCode = 0;
    return E_ABORT;
}

static HRESULT WINAPI MediaEvent_CancelDefaultHandling(IMediaEventEx *iface, LONG lEvCode)
{
    struct filter_graph *This = impl_from_IMediaEventEx(iface);

    TRACE("graph %p, code %#lx.\n", This, lEvCode);

    if (lEvCode == EC_COMPLETE)
	This->HandleEcComplete = FALSE;
    else if (lEvCode == EC_REPAINT)
	This->HandleEcRepaint = FALSE;
    else if (lEvCode == EC_CLOCK_CHANGED)
        This->HandleEcClockChanged = FALSE;
    else
	return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_RestoreDefaultHandling(IMediaEventEx *iface, LONG lEvCode)
{
    struct filter_graph *This = impl_from_IMediaEventEx(iface);

    TRACE("graph %p, code %#lx.\n", This, lEvCode);

    if (lEvCode == EC_COMPLETE)
	This->HandleEcComplete = TRUE;
    else if (lEvCode == EC_REPAINT)
	This->HandleEcRepaint = TRUE;
    else if (lEvCode == EC_CLOCK_CHANGED)
        This->HandleEcClockChanged = TRUE;
    else
	return S_FALSE;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_FreeEventParams(IMediaEventEx *iface, LONG code,
        LONG_PTR param1, LONG_PTR param2)
{
    struct filter_graph *graph = impl_from_IMediaEventEx(iface);

    WARN("graph %p, code %#lx, param1 %Id, param2 %Id, stub!\n", graph, code, param1, param2);

    return S_OK;
}

/*** IMediaEventEx methods ***/
static HRESULT WINAPI MediaEvent_SetNotifyWindow(IMediaEventEx *iface,
        OAHWND window, LONG message, LONG_PTR lparam)
{
    struct filter_graph *graph = impl_from_IMediaEventEx(iface);

    TRACE("graph %p, window %#Ix, message %#lx, lparam %#Ix.\n", graph, window, message, lparam);

    graph->media_event_window = (HWND)window;
    graph->media_event_message = message;
    graph->media_event_lparam = lparam;

    return S_OK;
}

static HRESULT WINAPI MediaEvent_SetNotifyFlags(IMediaEventEx *iface, LONG flags)
{
    struct filter_graph *graph = impl_from_IMediaEventEx(iface);

    TRACE("graph %p, flags %#lx.\n", graph, flags);

    if (flags & ~AM_MEDIAEVENT_NONOTIFY)
    {
        WARN("Invalid flags %#lx, returning E_INVALIDARG.\n", flags);
        return E_INVALIDARG;
    }

    graph->media_events_disabled = flags;

    if (flags)
    {
        flush_media_events(graph);
        ResetEvent(graph->media_event_handle);
    }

    return S_OK;
}

static HRESULT WINAPI MediaEvent_GetNotifyFlags(IMediaEventEx *iface, LONG *flags)
{
    struct filter_graph *graph = impl_from_IMediaEventEx(iface);

    TRACE("graph %p, flags %p.\n", graph, flags);

    if (!flags)
        return E_POINTER;

    *flags = graph->media_events_disabled;

    return S_OK;
}


static const IMediaEventExVtbl IMediaEventEx_VTable =
{
    MediaEvent_QueryInterface,
    MediaEvent_AddRef,
    MediaEvent_Release,
    MediaEvent_GetTypeInfoCount,
    MediaEvent_GetTypeInfo,
    MediaEvent_GetIDsOfNames,
    MediaEvent_Invoke,
    MediaEvent_GetEventHandle,
    MediaEvent_GetEvent,
    MediaEvent_WaitForCompletion,
    MediaEvent_CancelDefaultHandling,
    MediaEvent_RestoreDefaultHandling,
    MediaEvent_FreeEventParams,
    MediaEvent_SetNotifyWindow,
    MediaEvent_SetNotifyFlags,
    MediaEvent_GetNotifyFlags
};


static struct filter_graph *impl_from_IMediaFilter(IMediaFilter *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IMediaFilter_iface);
}

static HRESULT WINAPI MediaFilter_QueryInterface(IMediaFilter *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IMediaFilter(iface);

    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI MediaFilter_AddRef(IMediaFilter *iface)
{
    struct filter_graph *graph = impl_from_IMediaFilter(iface);

    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI MediaFilter_Release(IMediaFilter *iface)
{
    struct filter_graph *graph = impl_from_IMediaFilter(iface);

    return IUnknown_Release(graph->outer_unk);
}

static HRESULT WINAPI MediaFilter_GetClassID(IMediaFilter *iface, CLSID * pClassID)
{
    FIXME("(%p): stub\n", pClassID);

    return E_NOTIMPL;
}

static HRESULT WINAPI MediaFilter_Stop(IMediaFilter *iface)
{
    struct filter_graph *graph = impl_from_IMediaFilter(iface);
    HRESULT hr = S_OK, filter_hr;
    struct filter *filter;
    TP_WORK *work;

    TRACE("graph %p.\n", graph);

    EnterCriticalSection(&graph->cs);

    if (graph->state == State_Stopped)
    {
        LeaveCriticalSection(&graph->cs);
        return S_OK;
    }

    sort_filters(graph);

    if (graph->state == State_Running)
    {
        LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
        {
            filter_hr = IBaseFilter_Pause(filter->filter);
            if (hr == S_OK)
                hr = filter_hr;
        }
    }

    LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
    {
        filter_hr = IBaseFilter_Stop(filter->filter);
        if (hr == S_OK)
            hr = filter_hr;
    }

    graph->state = State_Stopped;
    graph->needs_async_run = 0;
    work = graph->async_run_work;
    graph->got_ec_complete = 0;

    /* Update the current position, probably to synchronize multiple streams. */
    IMediaSeeking_SetPositions(&graph->IMediaSeeking_iface, &graph->current_pos,
            AM_SEEKING_AbsolutePositioning, NULL, AM_SEEKING_NoPositioning);

    LeaveCriticalSection(&graph->cs);

    if (work)
        WaitForThreadpoolWorkCallbacks(work, TRUE);

    return hr;
}

static HRESULT WINAPI MediaFilter_Pause(IMediaFilter *iface)
{
    struct filter_graph *graph = impl_from_IMediaFilter(iface);
    HRESULT hr = S_OK, filter_hr;
    struct filter *filter;
    TP_WORK *work;

    TRACE("graph %p.\n", graph);

    EnterCriticalSection(&graph->cs);

    if (graph->state == State_Paused)
    {
        LeaveCriticalSection(&graph->cs);
        return S_OK;
    }

    sort_filters(graph);

    EnterCriticalSection(&graph->event_cs);
    update_render_count(graph);
    LeaveCriticalSection(&graph->event_cs);

    if (graph->defaultclock && !graph->refClock)
        IFilterGraph2_SetDefaultSyncSource(&graph->IFilterGraph2_iface);

    if (graph->state == State_Running && !graph->needs_async_run && graph->refClock)
    {
        REFERENCE_TIME time;
        IReferenceClock_GetTime(graph->refClock, &time);
        graph->stream_elapsed += time - graph->stream_start;
        graph->current_pos += graph->stream_elapsed;
    }

    LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
    {
        filter_hr = IBaseFilter_Pause(filter->filter);
        if (hr == S_OK)
            hr = filter_hr;
    }

    graph->state = State_Paused;
    graph->needs_async_run = 0;
    work = graph->async_run_work;

    LeaveCriticalSection(&graph->cs);

    if (work)
        WaitForThreadpoolWorkCallbacks(work, TRUE);

    return hr;
}

static HRESULT WINAPI MediaFilter_Run(IMediaFilter *iface, REFERENCE_TIME start)
{
    struct filter_graph *graph = impl_from_IMediaFilter(iface);
    HRESULT hr;

    TRACE("graph %p, start %s.\n", graph, debugstr_time(start));

    EnterCriticalSection(&graph->cs);

    if (graph->state == State_Running)
    {
        LeaveCriticalSection(&graph->cs);
        return S_OK;
    }

    sort_filters(graph);

    hr = graph_start(graph, start);

    graph->state = State_Running;
    graph->needs_async_run = 0;

    LeaveCriticalSection(&graph->cs);
    return hr;
}

static HRESULT WINAPI MediaFilter_GetState(IMediaFilter *iface, DWORD timeout, FILTER_STATE *state)
{
    struct filter_graph *graph = impl_from_IMediaFilter(iface);
    DWORD end = GetTickCount() + timeout;
    HRESULT hr;

    TRACE("graph %p, timeout %lu, state %p.\n", graph, timeout, state);

    if (!state)
        return E_POINTER;

    /* Thread safety is a little tricky here. GetState() shouldn't block other
     * functions from being called on the filter graph. However, we can't just
     * call IBaseFilter::GetState() in one loop and drop the lock on every
     * iteration, since the filter list might change beneath us. So instead we
     * do what native does, and poll for it every 10 ms. */

    EnterCriticalSection(&graph->cs);
    *state = graph->state;

    for (;;)
    {
        IBaseFilter *async_filter = NULL;
        FILTER_STATE filter_state;
        struct filter *filter;

        hr = S_OK;

        LIST_FOR_EACH_ENTRY(filter, &graph->filters, struct filter, entry)
        {
            HRESULT filter_hr = IBaseFilter_GetState(filter->filter, 0, &filter_state);

            TRACE("Filter %p returned hr %#lx, state %u.\n", filter->filter, filter_hr, filter_state);

            if (filter_hr == VFW_S_STATE_INTERMEDIATE)
                async_filter = filter->filter;

            if (hr == S_OK && filter_hr == VFW_S_STATE_INTERMEDIATE)
                hr = VFW_S_STATE_INTERMEDIATE;
            else if (filter_hr != S_OK && filter_hr != VFW_S_STATE_INTERMEDIATE)
                hr = filter_hr;

            if (hr == S_OK && filter_state == State_Paused && graph->state != State_Paused)
            {
                async_filter = filter->filter;
                hr = VFW_S_STATE_INTERMEDIATE;
            }
            else if (filter_state != graph->state && filter_state != State_Paused)
                hr = E_FAIL;

            if (graph->needs_async_run)
            {
                if (filter_state != State_Paused && filter_state != State_Running)
                    ERR("Filter %p reported incorrect state %u (expected %u or %u).\n",
                            filter->filter, filter_state, State_Paused, State_Running);
            }
            else
            {
                if (filter_state != graph->state)
                    ERR("Filter %p reported incorrect state %u (expected %u).\n",
                            filter->filter, filter_state, graph->state);
            }
        }

        LeaveCriticalSection(&graph->cs);

        if (hr != VFW_S_STATE_INTERMEDIATE || (timeout != INFINITE && GetTickCount() >= end))
            break;

        IBaseFilter_GetState(async_filter, 10, &filter_state);

        EnterCriticalSection(&graph->cs);
    }

    TRACE("Returning %#lx, state %u.\n", hr, *state);
    return hr;
}

static HRESULT WINAPI MediaFilter_SetSyncSource(IMediaFilter *iface, IReferenceClock *pClock)
{
    struct filter_graph *This = impl_from_IMediaFilter(iface);
    struct filter *filter;
    HRESULT hr = S_OK;

    TRACE("(%p/%p)->(%p)\n", This, iface, pClock);

    EnterCriticalSection(&This->cs);
    {
        LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
        {
            hr = IBaseFilter_SetSyncSource(filter->filter, pClock);
            if (FAILED(hr))
                break;
        }

        if (FAILED(hr))
        {
            LIST_FOR_EACH_ENTRY(filter, &This->filters, struct filter, entry)
                IBaseFilter_SetSyncSource(filter->filter, This->refClock);
        }
        else
        {
            if (This->refClock)
                IReferenceClock_Release(This->refClock);
            This->refClock = pClock;
            if (This->refClock)
                IReferenceClock_AddRef(This->refClock);
            This->defaultclock = FALSE;

            if (This->HandleEcClockChanged)
            {
                IMediaEventSink *pEventSink;
                HRESULT eshr;

                eshr = IMediaFilter_QueryInterface(iface, &IID_IMediaEventSink, (void **)&pEventSink);
                if (SUCCEEDED(eshr))
                {
                    IMediaEventSink_Notify(pEventSink, EC_CLOCK_CHANGED, 0, 0);
                    IMediaEventSink_Release(pEventSink);
                }
            }
        }
    }
    LeaveCriticalSection(&This->cs);

    return hr;
}

static HRESULT WINAPI MediaFilter_GetSyncSource(IMediaFilter *iface, IReferenceClock **ppClock)
{
    struct filter_graph *This = impl_from_IMediaFilter(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, ppClock);

    if (!ppClock)
        return E_POINTER;

    EnterCriticalSection(&This->cs);
    {
        *ppClock = This->refClock;
        if (*ppClock)
            IReferenceClock_AddRef(*ppClock);
    }
    LeaveCriticalSection(&This->cs);

    return S_OK;
}

static const IMediaFilterVtbl IMediaFilter_VTable =
{
    MediaFilter_QueryInterface,
    MediaFilter_AddRef,
    MediaFilter_Release,
    MediaFilter_GetClassID,
    MediaFilter_Stop,
    MediaFilter_Pause,
    MediaFilter_Run,
    MediaFilter_GetState,
    MediaFilter_SetSyncSource,
    MediaFilter_GetSyncSource
};

static struct filter_graph *impl_from_IMediaEventSink(IMediaEventSink *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IMediaEventSink_iface);
}

static HRESULT WINAPI MediaEventSink_QueryInterface(IMediaEventSink *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IMediaEventSink(iface);

    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI MediaEventSink_AddRef(IMediaEventSink *iface)
{
    struct filter_graph *graph = impl_from_IMediaEventSink(iface);

    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI MediaEventSink_Release(IMediaEventSink *iface)
{
    struct filter_graph *graph = impl_from_IMediaEventSink(iface);

    return IUnknown_Release(graph->outer_unk);
}

static HRESULT WINAPI MediaEventSink_Notify(IMediaEventSink *iface, LONG code,
        LONG_PTR param1, LONG_PTR param2)
{
    struct filter_graph *graph = impl_from_IMediaEventSink(iface);

    TRACE("graph %p, code %#lx, param1 %#Ix, param2 %#Ix.\n", graph, code, param1, param2);

    EnterCriticalSection(&graph->event_cs);

    if (code == EC_COMPLETE && graph->HandleEcComplete)
    {
        if (++graph->EcCompleteCount == graph->nRenderers)
        {
            if (graph->media_events_disabled)
                SetEvent(graph->media_event_handle);
            else
                queue_media_event(graph, EC_COMPLETE, S_OK, 0);
            graph->CompletionStatus = EC_COMPLETE;
            graph->got_ec_complete = 1;
            SetEvent(graph->hEventCompletion);
        }
    }
    else if ((code == EC_REPAINT) && graph->HandleEcRepaint)
    {
        FIXME("EC_REPAINT is not handled.\n");
    }
    else if (!graph->media_events_disabled)
    {
        queue_media_event(graph, code, param1, param2);
    }

    LeaveCriticalSection(&graph->event_cs);
    return S_OK;
}

static const IMediaEventSinkVtbl IMediaEventSink_VTable =
{
    MediaEventSink_QueryInterface,
    MediaEventSink_AddRef,
    MediaEventSink_Release,
    MediaEventSink_Notify
};

static struct filter_graph *impl_from_IGraphConfig(IGraphConfig *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IGraphConfig_iface);
}

static HRESULT WINAPI GraphConfig_QueryInterface(IGraphConfig *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IGraphConfig(iface);

    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI GraphConfig_AddRef(IGraphConfig *iface)
{
    struct filter_graph *graph = impl_from_IGraphConfig(iface);

    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI GraphConfig_Release(IGraphConfig *iface)
{
    struct filter_graph *graph = impl_from_IGraphConfig(iface);

    return IUnknown_Release(graph->outer_unk);
}

static HRESULT WINAPI GraphConfig_Reconnect(IGraphConfig *iface, IPin *source, IPin *sink,
        const AM_MEDIA_TYPE *mt, IBaseFilter *filter, HANDLE abort_event, DWORD flags)
{
    struct filter_graph *graph = impl_from_IGraphConfig(iface);

    FIXME("graph %p, source %p, sink %p, mt %p, filter %p, abort_event %p, flags %#lx, stub!\n",
            graph, source, sink, mt, filter, abort_event, flags);
    strmbase_dump_media_type(mt);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_Reconfigure(IGraphConfig *iface,
        IGraphConfigCallback *callback, void *context, DWORD flags, HANDLE abort_event)
{
    struct filter_graph *graph = impl_from_IGraphConfig(iface);
    HRESULT hr;

    TRACE("graph %p, callback %p, context %p, flags %#lx, abort_event %p.\n",
            graph, callback, context, flags, abort_event);

    if (abort_event)
        FIXME("The parameter hAbortEvent is not handled!\n");

    EnterCriticalSection(&graph->cs);

    hr = IGraphConfigCallback_Reconfigure(callback, context, flags);

    LeaveCriticalSection(&graph->cs);

    return hr;
}

static HRESULT WINAPI GraphConfig_AddFilterToCache(IGraphConfig *iface, IBaseFilter *pFilter)
{
    struct filter_graph *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p): stub!\n", This, pFilter);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_EnumCacheFilter(IGraphConfig *iface, IEnumFilters **pEnum)
{
    struct filter_graph *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p): stub!\n", This, pEnum);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_RemoveFilterFromCache(IGraphConfig *iface, IBaseFilter *pFilter)
{
    struct filter_graph *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p): stub!\n", This, pFilter);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_GetStartTime(IGraphConfig *iface, REFERENCE_TIME *prtStart)
{
    struct filter_graph *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p): stub!\n", This, prtStart);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_PushThroughData(IGraphConfig *iface, IPin *pOutputPin,
        IPinConnection *pConnection, HANDLE hEventAbort)
{
    struct filter_graph *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p, %p, %p): stub!\n", This, pOutputPin, pConnection, hEventAbort);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_SetFilterFlags(IGraphConfig *iface, IBaseFilter *filter, DWORD flags)
{
    struct filter_graph *graph = impl_from_IGraphConfig(iface);

    FIXME("graph %p, filter %p, flags %#lx, stub!\n", graph, filter, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_GetFilterFlags(IGraphConfig *iface, IBaseFilter *pFilter,
        DWORD *dwFlags)
{
    struct filter_graph *This = impl_from_IGraphConfig(iface);

    FIXME("(%p)->(%p, %p): stub!\n", This, pFilter, dwFlags);

    return E_NOTIMPL;
}

static HRESULT WINAPI GraphConfig_RemoveFilterEx(IGraphConfig *iface, IBaseFilter *filter, DWORD flags)
{
    struct filter_graph *graph = impl_from_IGraphConfig(iface);

    FIXME("graph %p, filter %p, flags %#lx, stub!\n", graph, filter, flags);

    return E_NOTIMPL;
}

static const IGraphConfigVtbl IGraphConfig_VTable =
{
    GraphConfig_QueryInterface,
    GraphConfig_AddRef,
    GraphConfig_Release,
    GraphConfig_Reconnect,
    GraphConfig_Reconfigure,
    GraphConfig_AddFilterToCache,
    GraphConfig_EnumCacheFilter,
    GraphConfig_RemoveFilterFromCache,
    GraphConfig_GetStartTime,
    GraphConfig_PushThroughData,
    GraphConfig_SetFilterFlags,
    GraphConfig_GetFilterFlags,
    GraphConfig_RemoveFilterEx
};

static struct filter_graph *impl_from_IGraphVersion(IGraphVersion *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IGraphVersion_iface);
}

static HRESULT WINAPI GraphVersion_QueryInterface(IGraphVersion *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IGraphVersion(iface);

    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI GraphVersion_AddRef(IGraphVersion *iface)
{
    struct filter_graph *graph = impl_from_IGraphVersion(iface);

    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI GraphVersion_Release(IGraphVersion *iface)
{
    struct filter_graph *graph = impl_from_IGraphVersion(iface);

    return IUnknown_Release(graph->outer_unk);
}

static HRESULT WINAPI GraphVersion_QueryVersion(IGraphVersion *iface, LONG *version)
{
    struct filter_graph *graph = impl_from_IGraphVersion(iface);

    TRACE("graph %p, version %p, returning %ld.\n", graph, version, graph->version);

    if (!version)
        return E_POINTER;

    *version = graph->version;
    return S_OK;
}

static const IGraphVersionVtbl IGraphVersion_VTable =
{
    GraphVersion_QueryInterface,
    GraphVersion_AddRef,
    GraphVersion_Release,
    GraphVersion_QueryVersion,
};

static struct filter_graph *impl_from_IVideoFrameStep(IVideoFrameStep *iface)
{
    return CONTAINING_RECORD(iface, struct filter_graph, IVideoFrameStep_iface);
}

static HRESULT WINAPI VideoFrameStep_QueryInterface(IVideoFrameStep *iface, REFIID iid, void **out)
{
    struct filter_graph *graph = impl_from_IVideoFrameStep(iface);
    return IUnknown_QueryInterface(graph->outer_unk, iid, out);
}

static ULONG WINAPI VideoFrameStep_AddRef(IVideoFrameStep *iface)
{
    struct filter_graph *graph = impl_from_IVideoFrameStep(iface);
    return IUnknown_AddRef(graph->outer_unk);
}

static ULONG WINAPI VideoFrameStep_Release(IVideoFrameStep *iface)
{
    struct filter_graph *graph = impl_from_IVideoFrameStep(iface);
    return IUnknown_Release(graph->outer_unk);
}

static HRESULT WINAPI VideoFrameStep_Step(IVideoFrameStep *iface, DWORD frame_count, IUnknown *filter)
{
    FIXME("iface %p, frame_count %lu, filter %p, stub!\n", iface, frame_count, filter);
    return E_NOTIMPL;
}

static HRESULT WINAPI VideoFrameStep_CanStep(IVideoFrameStep *iface, LONG multiple, IUnknown *filter)
{
    FIXME("iface %p, multiple %ld, filter %p, stub!\n", iface, multiple, filter);
    return E_NOTIMPL;
}

static HRESULT WINAPI VideoFrameStep_CancelStep(IVideoFrameStep *iface)
{
    FIXME("iface %p, stub!\n", iface);
    return E_NOTIMPL;
}

static const IVideoFrameStepVtbl VideoFrameStep_vtbl =
{
    VideoFrameStep_QueryInterface,
    VideoFrameStep_AddRef,
    VideoFrameStep_Release,
    VideoFrameStep_Step,
    VideoFrameStep_CanStep,
    VideoFrameStep_CancelStep
};

static const IUnknownVtbl IInner_VTable =
{
    FilterGraphInner_QueryInterface,
    FilterGraphInner_AddRef,
    FilterGraphInner_Release
};

static HRESULT filter_graph_common_create(IUnknown *outer, IUnknown **out, BOOL threaded)
{
    struct filter_graph *object;
    HRESULT hr;

    *out = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IBasicAudio_iface.lpVtbl = &IBasicAudio_VTable;
    object->IBasicVideo2_iface.lpVtbl = &IBasicVideo_VTable;
    object->IFilterGraph2_iface.lpVtbl = &IFilterGraph2_VTable;
    object->IGraphConfig_iface.lpVtbl = &IGraphConfig_VTable;
    object->IGraphVersion_iface.lpVtbl = &IGraphVersion_VTable;
    object->IMediaControl_iface.lpVtbl = &IMediaControl_VTable;
    object->IMediaEventEx_iface.lpVtbl = &IMediaEventEx_VTable;
    object->IMediaEventSink_iface.lpVtbl = &IMediaEventSink_VTable;
    object->IMediaFilter_iface.lpVtbl = &IMediaFilter_VTable;
    object->IMediaPosition_iface.lpVtbl = &IMediaPosition_VTable;
    object->IMediaSeeking_iface.lpVtbl = &IMediaSeeking_VTable;
    object->IObjectWithSite_iface.lpVtbl = &IObjectWithSite_VTable;
    object->IUnknown_inner.lpVtbl = &IInner_VTable;
    object->IVideoFrameStep_iface.lpVtbl = &VideoFrameStep_vtbl;
    object->IVideoWindow_iface.lpVtbl = &IVideoWindow_VTable;
    object->ref = 1;
    object->outer_unk = outer ? outer : &object->IUnknown_inner;

    if (FAILED(hr = CoCreateInstance(&CLSID_FilterMapper2, object->outer_unk,
            CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&object->punkFilterMapper2)))
    {
        ERR("Failed to create filter mapper, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    InitializeCriticalSectionEx(&object->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": filter_graph.cs");
    InitializeCriticalSectionEx(&object->event_cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->event_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": filter_graph.event_cs");

    object->defaultclock = TRUE;

    object->media_event_handle = CreateEventW(NULL, TRUE, FALSE, NULL);
    list_init(&object->media_events);
    list_init(&object->filters);
    object->HandleEcClockChanged = TRUE;
    object->HandleEcComplete = TRUE;
    object->HandleEcRepaint = TRUE;
    object->hEventCompletion = CreateEventW(0, TRUE, FALSE, 0);

    object->name_index = 1;
    object->timeformatseek = TIME_FORMAT_MEDIA_TIME;

    object->threaded = !!threaded;

    EnterCriticalSection(&message_cs);
    if (threaded && !message_thread_refcount++)
    {
        message_thread_ret = CreateEventW(NULL, FALSE, FALSE, NULL);
        message_thread = CreateThread(NULL, 0, message_thread_run, object, 0, &message_thread_id);
        WaitForSingleObject(message_thread_ret, INFINITE);
    }
    LeaveCriticalSection(&message_cs);

    TRACE("Created %sthreaded filter graph %p.\n", threaded ? "" : "non-", object);
    *out = &object->IUnknown_inner;
    return S_OK;
}

HRESULT filter_graph_create(IUnknown *outer, IUnknown **out)
{
    return filter_graph_common_create(outer, out, TRUE);
}

HRESULT filter_graph_no_thread_create(IUnknown *outer, IUnknown **out)
{
    return filter_graph_common_create(outer, out, FALSE);
}
