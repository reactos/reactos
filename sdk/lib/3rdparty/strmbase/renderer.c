/*
 * Generic Implementation of strmbase Base Renderer classes
 *
 * Copyright 2012 Aric Stewart, CodeWeavers
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

/* The following quality-of-service code is based on GstBaseSink QoS code, which
 * is covered by the following copyright information:
 *
 * GStreamer
 * Copyright (C) 2005-2007 Wim Taymans <wim.taymans@gmail.com>
 *
 * gstbasesink.c: Base class for sink elements
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#define DO_RUNNING_AVG(avg,val,size) (((val) + ((size)-1) * (avg)) / (size))

/* Generic running average; this has a neutral window size. */
#define UPDATE_RUNNING_AVG(avg,val)   DO_RUNNING_AVG(avg,val,8)

/* The windows for these running averages are experimentally obtained.
 * Positive values get averaged more, while negative values use a small
 * window so we can react faster to badness. */
#define UPDATE_RUNNING_AVG_P(avg,val) DO_RUNNING_AVG(avg,val,16)
#define UPDATE_RUNNING_AVG_N(avg,val) DO_RUNNING_AVG(avg,val,4)

static void reset_qos(struct strmbase_renderer *filter)
{
    filter->last_left = filter->avg_duration = filter->avg_pt = -1;
    filter->avg_rate = -1.0;
}

static void perform_qos(struct strmbase_renderer *filter,
        const REFERENCE_TIME start, const REFERENCE_TIME stop, REFERENCE_TIME jitter)
{
    REFERENCE_TIME pt, entered, left, duration;
    double rate;

    if (jitter < 0)
    {
        /* This is the time the buffer entered the sink. */
        if (start < -jitter)
            entered = 0;
        else
            entered = start + jitter;
        left = start;
    }
    else
    {
        /* This is the time the buffer entered the sink. */
        entered = start + jitter;
        /* This is the time the buffer left the sink. */
        left = start + jitter;
    }

    /* Calculate the duration of the buffer. */
    if (stop >= start)
        duration = stop - start;
    else
        duration = 0;

    /* If we have the time when the last buffer left us, calculate processing
     * time. */
    if (filter->last_left >= 0)
    {
        if (entered > filter->last_left)
            pt = entered - filter->last_left;
        else
            pt = 0;
    }
    else
    {
        pt = filter->avg_pt;
    }

    TRACE("start %s, entered %s, left %s, pt %s, duration %s, jitter %s.\n",
            debugstr_time(start), debugstr_time(entered), debugstr_time(left),
            debugstr_time(pt), debugstr_time(duration), debugstr_time(jitter));

    TRACE("average duration %s, average pt %s, average rate %.16e.\n",
            debugstr_time(filter->avg_duration), debugstr_time(filter->avg_pt), filter->avg_rate);

    /* Collect running averages. For first observations, we copy the values. */
    if (filter->avg_duration < 0)
        filter->avg_duration = duration;
    else
        filter->avg_duration = UPDATE_RUNNING_AVG(filter->avg_duration, duration);

    if (filter->avg_pt < 0)
        filter->avg_pt = pt;
    else
        filter->avg_pt = UPDATE_RUNNING_AVG(filter->avg_pt, pt);

    if (filter->avg_duration != 0)
        rate = (double)filter->avg_pt / (double)filter->avg_duration;
    else
        rate = 0.0;

    if (filter->last_left >= 0)
    {
        if (filter->avg_rate < 0.0)
        {
            filter->avg_rate = rate;
        }
        else
        {
            if (rate > 1.0)
                filter->avg_rate = UPDATE_RUNNING_AVG_N(filter->avg_rate, rate);
            else
                filter->avg_rate = UPDATE_RUNNING_AVG_P(filter->avg_rate, rate);
        }
    }

    if (filter->avg_rate >= 0.0)
    {
        Quality q;

        /* If we have a valid rate, start sending QoS messages. */
        if (jitter < 0)
        {
            /* Make sure we never go below 0 when adding the jitter to the
             * timestamp. */
            if (start < -jitter)
                jitter = -start;
        }
        else
        {
            jitter += stop - start;
        }

        q.Type = (jitter > 0 ? Famine : Flood);
        q.Proportion = 1000.0 / filter->avg_rate;
        if (q.Proportion < 200)
            q.Proportion = 200;
        else if (q.Proportion > 5000)
            q.Proportion = 5000;
        q.Late = jitter;
        q.TimeStamp = start;
        IQualityControl_Notify(&filter->IQualityControl_iface, &filter->filter.IBaseFilter_iface, q);
    }

    /* Record when this buffer will leave us. */
    filter->last_left = left;
}

static inline struct strmbase_renderer *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_renderer, filter);
}

static inline struct strmbase_renderer *impl_from_IPin(IPin *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_renderer, sink.pin.IPin_iface);
}

static struct strmbase_pin *renderer_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    return NULL;
}

static void renderer_destroy(struct strmbase_filter *iface)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);
    filter->ops->renderer_destroy(filter);
}

static HRESULT renderer_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);
    HRESULT hr;

    if (filter->ops->renderer_query_interface
            && SUCCEEDED(hr = filter->ops->renderer_query_interface(filter, iid, out)))
    {
        return hr;
    }

    if (IsEqualGUID(iid, &IID_IMediaPosition))
        *out = &filter->passthrough.IMediaPosition_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &filter->passthrough.IMediaSeeking_iface;
    else if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &filter->IQualityControl_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT renderer_init_stream(struct strmbase_filter *iface)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer)
        ResetEvent(filter->state_event);
    filter->eos = FALSE;
    ResetEvent(filter->flush_event);
    if (filter->ops->renderer_init_stream)
        filter->ops->renderer_init_stream(filter);

    return filter->sink.pin.peer ? S_FALSE : S_OK;
}

static HRESULT renderer_start_stream(struct strmbase_filter *iface, REFERENCE_TIME start)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);
    IFilterGraph *graph = filter->filter.graph;
    IMediaEventSink *event_sink;

    filter->stream_start = start;
    SetEvent(filter->state_event);
    SetEvent(filter->run_event);
    reset_qos(filter);
    if (filter->sink.pin.peer && filter->ops->renderer_start_stream)
        filter->ops->renderer_start_stream(filter);

    if ((filter->eos || !filter->sink.pin.peer) && graph
            && SUCCEEDED(IFilterGraph_QueryInterface(graph,
            &IID_IMediaEventSink, (void **)&event_sink)))
    {
        IMediaEventSink_Notify(event_sink, EC_COMPLETE, S_OK,
                (LONG_PTR)&filter->filter.IBaseFilter_iface);
        IMediaEventSink_Release(event_sink);
    }

    return S_OK;
}

static HRESULT renderer_stop_stream(struct strmbase_filter *iface)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    ResetEvent(filter->run_event);

    if (filter->sink.pin.peer && filter->ops->renderer_stop_stream)
        filter->ops->renderer_stop_stream(filter);

    return S_OK;
}

static HRESULT renderer_cleanup_stream(struct strmbase_filter *iface)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    strmbase_passthrough_invalidate_time(&filter->passthrough);
    SetEvent(filter->state_event);
    SetEvent(filter->flush_event);

    return S_OK;
}

static HRESULT renderer_wait_state(struct strmbase_filter *iface, DWORD timeout)
{
    struct strmbase_renderer *filter = impl_from_strmbase_filter(iface);

    if (WaitForSingleObject(filter->state_event, timeout) == WAIT_TIMEOUT)
        return VFW_S_STATE_INTERMEDIATE;
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_get_pin = renderer_get_pin,
    .filter_destroy = renderer_destroy,
    .filter_query_interface = renderer_query_interface,
    .filter_init_stream = renderer_init_stream,
    .filter_start_stream = renderer_start_stream,
    .filter_stop_stream = renderer_stop_stream,
    .filter_cleanup_stream = renderer_cleanup_stream,
    .filter_wait_state = renderer_wait_state,
};

static HRESULT sink_query_accept(struct strmbase_pin *pin, const AM_MEDIA_TYPE *mt)
{
    struct strmbase_renderer *filter = impl_from_IPin(&pin->IPin_iface);
    return filter->ops->renderer_query_accept(filter, mt);
}

static HRESULT sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->IPin_iface);
    HRESULT hr;

    if (filter->ops->renderer_pin_query_interface
            && SUCCEEDED(hr = filter->ops->renderer_pin_query_interface(filter, iid, out)))
        return hr;

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI BaseRenderer_Receive(struct strmbase_sink *pin, IMediaSample *sample)
{
    struct strmbase_renderer *filter = impl_from_IPin(&pin->pin.IPin_iface);
    REFERENCE_TIME start, stop;
    BOOL need_wait = FALSE;
    FILTER_STATE state;
    HRESULT hr = S_OK;
    AM_MEDIA_TYPE *mt;

    if (filter->eos || filter->sink.flushing)
        return S_FALSE;

    state = filter->filter.state;
    if (state == State_Stopped)
        return VFW_E_WRONG_STATE;

    if (IMediaSample_GetMediaType(sample, &mt) == S_OK)
    {
        TRACE("Format change.\n");
        strmbase_dump_media_type(mt);

        if (FAILED(filter->ops->renderer_query_accept(filter, mt)))
            return VFW_E_TYPE_NOT_ACCEPTED;
        DeleteMediaType(mt);
    }

    if (filter->filter.clock && SUCCEEDED(IMediaSample_GetTime(sample, &start, &stop)))
    {
        strmbase_passthrough_update_time(&filter->passthrough, start);
        need_wait = TRUE;
    }

    if (state == State_Paused)
    {
        HANDLE events[2] = {filter->run_event, filter->flush_event};

        filter->current_sample = sample;

        hr = filter->ops->renderer_render(filter, sample);

        SetEvent(filter->state_event);
        LeaveCriticalSection(&filter->filter.stream_cs);
        WaitForMultipleObjects(2, events, FALSE, INFINITE);
        EnterCriticalSection(&filter->filter.stream_cs);

        filter->current_sample = NULL;
    }

    if (need_wait)
    {
        REFERENCE_TIME now, jitter;
        DWORD_PTR cookie;

        IReferenceClock_GetTime(filter->filter.clock, &now);

        /* "jitter" describes how early (negative) or late (positive) this
         * buffer arrived, relative to its PTS. */
        jitter = (now - filter->stream_start) - start;

        if (jitter <= -10000) /* if we are early by at least 1 ms */
        {
            HANDLE handles[2] = {filter->advise_event, filter->flush_event};
            DWORD ret;

            IReferenceClock_AdviseTime(filter->filter.clock, filter->stream_start,
                    start, (HEVENT)filter->advise_event, &cookie);

            ret = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
            IReferenceClock_Unadvise(filter->filter.clock, cookie);

            if (ret == 1)
            {
                TRACE("Flush signaled; discarding current sample.\n");
                return S_OK;
            }
        }

        if (state == State_Running)
            hr = filter->ops->renderer_render(filter, sample);

        perform_qos(filter, start, stop, jitter);
    }
    else
    {
        if (state == State_Running)
            hr = filter->ops->renderer_render(filter, sample);
    }

    return hr;
}

static HRESULT sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);

    if (filter->ops->renderer_connect)
        return filter->ops->renderer_connect(filter, mt);
    return S_OK;
}

static void sink_disconnect(struct strmbase_sink *iface)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);

    if (filter->ops->renderer_disconnect)
        filter->ops->renderer_disconnect(filter);
}

static HRESULT sink_eos(struct strmbase_sink *iface)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);
    IFilterGraph *graph = filter->filter.graph;
    IMediaEventSink *event_sink;

    filter->eos = TRUE;

    if (filter->filter.state == State_Running && graph
            && SUCCEEDED(IFilterGraph_QueryInterface(graph,
            &IID_IMediaEventSink, (void **)&event_sink)))
    {
        IMediaEventSink_Notify(event_sink, EC_COMPLETE, S_OK,
                (LONG_PTR)&filter->filter.IBaseFilter_iface);
        IMediaEventSink_Release(event_sink);
    }
    strmbase_passthrough_eos(&filter->passthrough);
    SetEvent(filter->state_event);

    return S_OK;
}

static HRESULT sink_begin_flush(struct strmbase_sink *iface)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);

    SetEvent(filter->flush_event);

    return S_OK;
}

static HRESULT sink_end_flush(struct strmbase_sink *iface)
{
    struct strmbase_renderer *filter = impl_from_IPin(&iface->pin.IPin_iface);

    EnterCriticalSection(&filter->filter.stream_cs);

    filter->eos = FALSE;
    reset_qos(filter);
    strmbase_passthrough_invalidate_time(&filter->passthrough);
    ResetEvent(filter->flush_event);

    LeaveCriticalSection(&filter->filter.stream_cs);
    return S_OK;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_accept = sink_query_accept,
    .base.pin_query_interface = sink_query_interface,
    .pfnReceive = BaseRenderer_Receive,
    .sink_connect = sink_connect,
    .sink_disconnect = sink_disconnect,
    .sink_eos = sink_eos,
    .sink_begin_flush = sink_begin_flush,
    .sink_end_flush = sink_end_flush,
};

static struct strmbase_renderer *impl_from_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, struct strmbase_renderer, IQualityControl_iface);
}

static HRESULT WINAPI quality_control_QueryInterface(IQualityControl *iface, REFIID iid, void **out)
{
    struct strmbase_renderer *filter = impl_from_IQualityControl(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI quality_control_AddRef(IQualityControl *iface)
{
    struct strmbase_renderer *filter = impl_from_IQualityControl(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI quality_control_Release(IQualityControl *iface)
{
    struct strmbase_renderer *filter = impl_from_IQualityControl(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI quality_control_Notify(IQualityControl *iface, IBaseFilter *sender, Quality q)
{
    struct strmbase_renderer *filter = impl_from_IQualityControl(iface);
    HRESULT hr = S_FALSE;

    TRACE("filter %p, sender %p, type %#x, proportion %ld, late %s, timestamp %s.\n",
            filter, sender, q.Type, q.Proportion, debugstr_time(q.Late), debugstr_time(q.TimeStamp));

    if (filter->qc_sink)
        return IQualityControl_Notify(filter->qc_sink, &filter->filter.IBaseFilter_iface, q);

    if (filter->sink.pin.peer)
    {
        IQualityControl *peer_qc = NULL;
        IPin_QueryInterface(filter->sink.pin.peer, &IID_IQualityControl, (void **)&peer_qc);
        if (peer_qc)
        {
            hr = IQualityControl_Notify(peer_qc, &filter->filter.IBaseFilter_iface, q);
            IQualityControl_Release(peer_qc);
        }
    }

    return hr;
}

static HRESULT WINAPI quality_control_SetSink(IQualityControl *iface, IQualityControl *sink)
{
    struct strmbase_renderer *filter = impl_from_IQualityControl(iface);

    TRACE("filter %p, sink %p.\n", filter, sink);

    filter->qc_sink = sink;
    return S_OK;
}

static const IQualityControlVtbl quality_control_vtbl =
{
    quality_control_QueryInterface,
    quality_control_AddRef,
    quality_control_Release,
    quality_control_Notify,
    quality_control_SetSink,
};

void strmbase_renderer_cleanup(struct strmbase_renderer *filter)
{
    if (filter->sink.pin.peer)
        IPin_Disconnect(filter->sink.pin.peer);
    IPin_Disconnect(&filter->sink.pin.IPin_iface);
    strmbase_sink_cleanup(&filter->sink);

    strmbase_passthrough_cleanup(&filter->passthrough);

    CloseHandle(filter->state_event);
    CloseHandle(filter->advise_event);
    CloseHandle(filter->run_event);
    CloseHandle(filter->flush_event);
    strmbase_filter_cleanup(&filter->filter);
}

void strmbase_renderer_init(struct strmbase_renderer *filter, IUnknown *outer,
        const CLSID *clsid, const WCHAR *sink_name, const struct strmbase_renderer_ops *ops)
{
    memset(filter, 0, sizeof(*filter));
    strmbase_filter_init(&filter->filter, outer, clsid, &filter_ops);
    strmbase_passthrough_init(&filter->passthrough, (IUnknown *)&filter->filter.IBaseFilter_iface);
    ISeekingPassThru_Init(&filter->passthrough.ISeekingPassThru_iface, TRUE, &filter->sink.pin.IPin_iface);
    filter->IQualityControl_iface.lpVtbl = &quality_control_vtbl;

    filter->ops = ops;

    strmbase_sink_init(&filter->sink, &filter->filter, sink_name, &sink_ops, NULL);

    filter->state_event = CreateEventW(NULL, TRUE, TRUE, NULL);
    filter->advise_event = CreateEventW(NULL, FALSE, FALSE, NULL);
    filter->run_event = CreateEventW(NULL, TRUE, FALSE, NULL);
    filter->flush_event = CreateEventW(NULL, TRUE, TRUE, NULL);
}
