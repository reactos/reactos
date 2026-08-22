/*
 * Generic Implementation of IBaseFilter Interface
 *
 * Copyright 2010 Aric Stewart, CodeWeavers
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

#include "strmbase_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

struct enum_pins
{
    IEnumPins IEnumPins_iface;
    LONG refcount;

    unsigned int index, count;
    int version;
    struct strmbase_filter *filter;
};

static const IEnumPinsVtbl enum_pins_vtbl;

static HRESULT enum_pins_create(struct strmbase_filter *filter, IEnumPins **out)
{
    struct enum_pins *object;

    if (!out)
        return E_POINTER;

    if (!(object = heap_alloc_zero(sizeof(*object))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    object->IEnumPins_iface.lpVtbl = &enum_pins_vtbl;
    object->refcount = 1;
    object->filter = filter;
    IBaseFilter_AddRef(&filter->IBaseFilter_iface);
    object->version = filter->pin_version;

    while (filter->ops->filter_get_pin(filter, object->count))
        ++object->count;

    TRACE("Created enumerator %p.\n", object);
    *out = &object->IEnumPins_iface;

    return S_OK;
}

static inline struct enum_pins *impl_from_IEnumPins(IEnumPins *iface)
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

    TRACE("%p decreasing refcount to %lu.\n", enum_pins, refcount);
    if (!refcount)
    {
        IBaseFilter_Release(&enum_pins->filter->IBaseFilter_iface);
        heap_free(enum_pins);
    }
    return refcount;
}

static HRESULT WINAPI enum_pins_Next(IEnumPins *iface, ULONG count, IPin **pins, ULONG *ret_count)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);
    unsigned int i;

    TRACE("iface %p, count %lu, pins %p, ret_count %p.\n", iface, count, pins, ret_count);

    if (!pins)
        return E_POINTER;

    if (count > 1 && !ret_count)
        return E_INVALIDARG;

    if (ret_count)
        *ret_count = 0;

    if (enum_pins->version != enum_pins->filter->pin_version)
        return S_FALSE;

    for (i = 0; i < count; ++i)
    {
        struct strmbase_pin *pin = enum_pins->filter->ops->filter_get_pin(enum_pins->filter, enum_pins->index + i);

        if (!pin)
            break;

        IPin_AddRef(pins[i] = &pin->IPin_iface);
    }

    if (ret_count)
        *ret_count = i;
    enum_pins->index += i;
    return i == count ? S_OK : S_FALSE;
}

static HRESULT WINAPI enum_pins_Skip(IEnumPins *iface, ULONG count)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);

    TRACE("iface %p, count %lu.\n", iface, count);

    if (enum_pins->version != enum_pins->filter->pin_version)
        return VFW_E_ENUM_OUT_OF_SYNC;

    if (enum_pins->index + count > enum_pins->count)
        return S_FALSE;

    enum_pins->index += count;
    return S_OK;
}

static HRESULT WINAPI enum_pins_Reset(IEnumPins *iface)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);

    TRACE("iface %p.\n", iface);

    if (enum_pins->version != enum_pins->filter->pin_version)
    {
        enum_pins->count = 0;
        while (enum_pins->filter->ops->filter_get_pin(enum_pins->filter, enum_pins->count))
            ++enum_pins->count;
    }

    enum_pins->version = enum_pins->filter->pin_version;
    enum_pins->index = 0;

    return S_OK;
}

static HRESULT WINAPI enum_pins_Clone(IEnumPins *iface, IEnumPins **out)
{
    struct enum_pins *enum_pins = impl_from_IEnumPins(iface);
    HRESULT hr;

    TRACE("iface %p, out %p.\n", iface, out);

    if (FAILED(hr = enum_pins_create(enum_pins->filter, out)))
        return hr;
    return IEnumPins_Skip(*out, enum_pins->index);
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

static inline struct strmbase_filter *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_filter, IUnknown_inner);
}

static HRESULT WINAPI filter_inner_QueryInterface(IUnknown *iface, REFIID iid, void **out)
{
    struct strmbase_filter *filter = impl_from_IUnknown(iface);
    HRESULT hr;

    TRACE("filter %p %s, iid %s, out %p.\n", filter, debugstr_w(filter->name), debugstr_guid(iid), out);

    *out = NULL;

    if (filter->ops->filter_query_interface
            && SUCCEEDED(hr = filter->ops->filter_query_interface(filter, iid, out)))
    {
        return hr;
    }

    if (IsEqualIID(iid, &IID_IUnknown))
        *out = iface;
    else if (IsEqualIID(iid, &IID_IPersist)
            || IsEqualIID(iid, &IID_IMediaFilter)
            || IsEqualIID(iid, &IID_IBaseFilter))
    {
        *out = &filter->IBaseFilter_iface;
    }
    else
    {
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static ULONG WINAPI filter_inner_AddRef(IUnknown *iface)
{
    struct strmbase_filter *filter = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&filter->refcount);

    TRACE("%p increasing refcount to %lu.\n", filter, refcount);

    return refcount;
}

static ULONG WINAPI filter_inner_Release(IUnknown *iface)
{
    struct strmbase_filter *filter = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&filter->refcount);

    TRACE("%p decreasing refcount to %lu.\n", filter, refcount);

    if (!refcount)
        filter->ops->filter_destroy(filter);

    return refcount;
}

static const IUnknownVtbl filter_inner_vtbl =
{
    filter_inner_QueryInterface,
    filter_inner_AddRef,
    filter_inner_Release,
};

static inline struct strmbase_filter *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_filter, IBaseFilter_iface);
}

static HRESULT WINAPI filter_QueryInterface(IBaseFilter *iface, REFIID iid, void **out)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);
    return IUnknown_QueryInterface(filter->outer_unk, iid, out);
}

static ULONG WINAPI filter_AddRef(IBaseFilter *iface)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);
    return IUnknown_AddRef(filter->outer_unk);
}

static ULONG WINAPI filter_Release(IBaseFilter *iface)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);
    return IUnknown_Release(filter->outer_unk);
}

static HRESULT WINAPI filter_GetClassID(IBaseFilter *iface, CLSID *clsid)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);

    TRACE("filter %p %s, clsid %p.\n", filter, debugstr_w(filter->name), clsid);

    *clsid = filter->clsid;

    return S_OK;
}

static HRESULT WINAPI filter_Stop(IBaseFilter *iface)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);
    HRESULT hr = S_OK;

    TRACE("filter %p %s.\n", filter, debugstr_w(filter->name));

    EnterCriticalSection(&filter->filter_cs);

    if (filter->state == State_Running && filter->ops->filter_stop_stream)
        hr = filter->ops->filter_stop_stream(filter);
    if (SUCCEEDED(hr) && filter->ops->filter_cleanup_stream)
        hr = filter->ops->filter_cleanup_stream(filter);
    if (SUCCEEDED(hr))
        filter->state = State_Stopped;

    LeaveCriticalSection(&filter->filter_cs);

    return hr;
}

static HRESULT WINAPI filter_Pause(IBaseFilter *iface)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);
    HRESULT hr = S_OK;

    TRACE("filter %p %s.\n", filter, debugstr_w(filter->name));

    EnterCriticalSection(&filter->filter_cs);

    if (filter->state == State_Stopped && filter->ops->filter_init_stream)
        hr = filter->ops->filter_init_stream(filter);
    else if (filter->state == State_Running && filter->ops->filter_stop_stream)
        hr = filter->ops->filter_stop_stream(filter);
    if (SUCCEEDED(hr))
        filter->state = State_Paused;

    LeaveCriticalSection(&filter->filter_cs);

    return hr;
}

static HRESULT WINAPI filter_Run(IBaseFilter *iface, REFERENCE_TIME start)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);
    HRESULT hr = S_OK;

    TRACE("filter %p %s, start %s.\n", filter, debugstr_w(filter->name), debugstr_time(start));

    EnterCriticalSection(&filter->filter_cs);

    if (filter->state == State_Stopped && filter->ops->filter_init_stream)
        hr = filter->ops->filter_init_stream(filter);
    if (SUCCEEDED(hr) && filter->ops->filter_start_stream)
        hr = filter->ops->filter_start_stream(filter, start);
    if (SUCCEEDED(hr))
        filter->state = State_Running;

    LeaveCriticalSection(&filter->filter_cs);

    return hr;
}

static HRESULT WINAPI filter_GetState(IBaseFilter *iface, DWORD timeout, FILTER_STATE *state)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);
    HRESULT hr = S_OK;

    TRACE("filter %p %s, timeout %lu, state %p.\n", filter, debugstr_w(filter->name), timeout, state);

    EnterCriticalSection(&filter->filter_cs);

    if (filter->ops->filter_wait_state)
        hr = filter->ops->filter_wait_state(filter, timeout);
    *state = filter->state;

    LeaveCriticalSection(&filter->filter_cs);

    return hr;
}

static HRESULT WINAPI filter_SetSyncSource(IBaseFilter *iface, IReferenceClock *clock)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);

    TRACE("filter %p %s, clock %p.\n", filter, debugstr_w(filter->name), clock);

    EnterCriticalSection(&filter->filter_cs);

    if (filter->clock)
        IReferenceClock_Release(filter->clock);
    filter->clock = clock;
    if (filter->clock)
        IReferenceClock_AddRef(filter->clock);

    LeaveCriticalSection(&filter->filter_cs);

    return S_OK;
}

static HRESULT WINAPI filter_GetSyncSource(IBaseFilter *iface, IReferenceClock **clock)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);

    TRACE("filter %p %s, clock %p.\n", filter, debugstr_w(filter->name), clock);

    EnterCriticalSection(&filter->filter_cs);

    *clock = filter->clock;
    if (filter->clock)
        IReferenceClock_AddRef(filter->clock);

    LeaveCriticalSection(&filter->filter_cs);

    return S_OK;
}

static HRESULT WINAPI filter_EnumPins(IBaseFilter *iface, IEnumPins **enum_pins)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);

    TRACE("filter %p %s, enum_pins %p.\n", filter, debugstr_w(filter->name), enum_pins);

    return enum_pins_create(filter, enum_pins);
}

static HRESULT WINAPI filter_FindPin(IBaseFilter *iface, const WCHAR *id, IPin **ret)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);
    struct strmbase_pin *pin;
    unsigned int i;

    TRACE("filter %p %s, id %s, ret %p.\n", filter, debugstr_w(filter->name), debugstr_w(id), ret);

    for (i = 0; (pin = filter->ops->filter_get_pin(filter, i)); ++i)
    {
        if (!lstrcmpW(id, pin->id))
        {
            IPin_AddRef(*ret = &pin->IPin_iface);
            return S_OK;
        }
    }

    return VFW_E_NOT_FOUND;
}

static HRESULT WINAPI filter_QueryFilterInfo(IBaseFilter *iface, FILTER_INFO *info)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);

    TRACE("filter %p %s, info %p.\n", filter, debugstr_w(filter->name), info);

    lstrcpyW(info->achName, filter->name);
    info->pGraph = filter->graph;

    if (info->pGraph)
        IFilterGraph_AddRef(info->pGraph);

    return S_OK;
}

static HRESULT WINAPI filter_JoinFilterGraph(IBaseFilter *iface, IFilterGraph *graph, const WCHAR *name)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);

    TRACE("filter %p %s, graph %p, name %s.\n", filter, debugstr_w(filter->name), graph, debugstr_w(name));

    EnterCriticalSection(&filter->filter_cs);

    if (name)
        lstrcpynW(filter->name, name, ARRAY_SIZE(filter->name));
    else
        filter->name[0] = 0;
    /* The graph references us, so we cannot also reference the graph. */
    filter->graph = graph;

    LeaveCriticalSection(&filter->filter_cs);

    return S_OK;
}

static HRESULT WINAPI filter_QueryVendorInfo(IBaseFilter *iface, WCHAR **info)
{
    struct strmbase_filter *filter = impl_from_IBaseFilter(iface);

    TRACE("filter %p %s, info %p.\n", filter, debugstr_w(filter->name), info);

    return E_NOTIMPL;
}

static const IBaseFilterVtbl filter_vtbl =
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
};

VOID WINAPI BaseFilterImpl_IncrementPinVersion(struct strmbase_filter *filter)
{
    InterlockedIncrement(&filter->pin_version);
}

void strmbase_filter_init(struct strmbase_filter *filter, IUnknown *outer,
        const CLSID *clsid, const struct strmbase_filter_ops *ops)
{
    memset(filter, 0, sizeof(*filter));

    filter->IBaseFilter_iface.lpVtbl = &filter_vtbl;
    filter->IUnknown_inner.lpVtbl = &filter_inner_vtbl;
    filter->outer_unk = outer ? outer : &filter->IUnknown_inner;
    filter->refcount = 1;

    if (!InitializeCriticalSectionEx(&filter->filter_cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO))
        InitializeCriticalSection(&filter->filter_cs);
    if (filter->filter_cs.DebugInfo != (RTL_CRITICAL_SECTION_DEBUG *)-1)
        filter->filter_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": strmbase_filter.filter_cs");
    if (!InitializeCriticalSectionEx(&filter->stream_cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO))
        InitializeCriticalSection(&filter->stream_cs);
    if (filter->stream_cs.DebugInfo != (RTL_CRITICAL_SECTION_DEBUG *)-1)
        filter->stream_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": strmbase_filter.stream_cs");
    filter->clsid = *clsid;
    filter->pin_version = 1;
    filter->ops = ops;
}

void strmbase_filter_cleanup(struct strmbase_filter *filter)
{
    if (filter->clock)
        IReferenceClock_Release(filter->clock);

    filter->IBaseFilter_iface.lpVtbl = NULL;
    if (filter->filter_cs.DebugInfo != (RTL_CRITICAL_SECTION_DEBUG *)-1)
        filter->filter_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->filter_cs);
    if (filter->stream_cs.DebugInfo != (RTL_CRITICAL_SECTION_DEBUG *)-1)
        filter->stream_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->stream_cs);
}
