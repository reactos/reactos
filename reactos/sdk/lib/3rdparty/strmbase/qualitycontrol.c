/*
 * Quality Control Interfaces
 *
 * Copyright 2010 Maarten Lankhorst for CodeWeavers
 *
 * rendering qos functions based on, the original can be found at
 * gstreamer/libs/gst/base/gstbasesink.c which has copyright notice:
 *
 * Copyright (C) 2005-2007 Wim Taymans <wim.taymans@gmail.com>
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

HRESULT QualityControlImpl_Create(IPin *input, IBaseFilter *self, QualityControlImpl **ppv) {
    QualityControlImpl *This;
    *ppv = HeapAlloc(GetProcessHeap(),0,sizeof(QualityControlImpl));
    if (!*ppv)
        return E_OUTOFMEMORY;
    This = *ppv;
    This->input = input;
    This->self = self;
    This->tonotify = NULL;
    This->clock = NULL;
    return S_OK;
}

void QualityControlImpl_Destroy(QualityControlImpl *This)
{
    HeapFree(GetProcessHeap(),0,This);
}

HRESULT WINAPI QualityControlImpl_QueryInterface(IQualityControl *iface, REFIID riid, void **ppv) {
    QualityControlImpl *This = (QualityControlImpl*)iface;
    return IBaseFilter_QueryInterface(This->self, riid, ppv);
}

ULONG WINAPI QualityControlImpl_AddRef(IQualityControl *iface) {
    QualityControlImpl *This = (QualityControlImpl*)iface;
    return IBaseFilter_AddRef(This->self);
}

ULONG WINAPI QualityControlImpl_Release(IQualityControl *iface) {
    QualityControlImpl *This = (QualityControlImpl*)iface;
    return IBaseFilter_Release(This->self);
}

HRESULT WINAPI QualityControlImpl_Notify(IQualityControl *iface, IBaseFilter *sender, Quality qm) {
    HRESULT hr = S_FALSE;
    QualityControlImpl *This = (QualityControlImpl*)iface;
    if (This->tonotify)
        return IQualityControl_Notify(This->tonotify, This->self, qm);
    if (This->input) {
        IPin *to = NULL;
        IPin_ConnectedTo(This->input, &to);
        if (to) {
            IQualityControl *qc = NULL;
            IPin_QueryInterface(to, &IID_IQualityControl, (void**)&qc);
            if (qc) {
                hr = IQualityControl_Notify(qc, This->self, qm);
                IQualityControl_Release(qc);
            }
            IPin_Release(to);
        }
    }
    return hr;
}

HRESULT WINAPI QualityControlImpl_SetSink(IQualityControl *iface, IQualityControl *tonotify) {
    QualityControlImpl *This = (QualityControlImpl*)iface;
    This->tonotify = tonotify;
    return S_OK;
}

/* Macros copied from gstreamer, weighted average between old average and new ones */
#define DO_RUNNING_AVG(avg,val,size) (((val) + ((size)-1) * (avg)) / (size))

/* generic running average, this has a neutral window size */
#define UPDATE_RUNNING_AVG(avg,val)   DO_RUNNING_AVG(avg,val,8)

/* the windows for these running averages are experimentally obtained.
 * possitive values get averaged more while negative values use a small
 * window so we can react faster to badness. */
#define UPDATE_RUNNING_AVG_P(avg,val) DO_RUNNING_AVG(avg,val,16)
#define UPDATE_RUNNING_AVG_N(avg,val) DO_RUNNING_AVG(avg,val,4)

void QualityControlRender_Start(QualityControlImpl *This, REFERENCE_TIME tStart) {
    This->avg_render = This->last_in_time = This->last_left = This->avg_duration = This->avg_pt = -1;
    This->clockstart = tStart;
    This->avg_rate = -1.0;
    This->rendered = This->dropped = 0;
    This->is_dropped = FALSE;
    This->qos_handled = TRUE; /* Lie that will be corrected on first adjustment */
}


void QualityControlRender_SetClock(QualityControlImpl *This, IReferenceClock *clock) {
    This->clock = clock;
}

static BOOL QualityControlRender_IsLate(QualityControlImpl *This, REFERENCE_TIME jitter,
                                        REFERENCE_TIME start, REFERENCE_TIME stop)
{
    REFERENCE_TIME max_lateness = 200000;

    /* we can add a valid stop time */
    if (stop >= start)
        max_lateness += stop;
    else
        max_lateness += start;

    /* if the jitter bigger than duration and lateness we are too late */
    if (start + jitter > max_lateness) {
        WARN("buffer is too late %i > %i\n", (int)((start + jitter)/10000),  (int)(max_lateness/10000));
        /* !!emergency!!, if we did not receive anything valid for more than a
         * second, render it anyway so the user sees something */
        if (This->last_in_time < 0 ||
            start - This->last_in_time < 10000000)
            return TRUE;
        FIXME("A lot of buffers are being dropped.\n");
        FIXME("There may be a timestamping problem, or this computer is too slow.\n");
    }
    This->last_in_time = start;
    return FALSE;
}

HRESULT QualityControlRender_WaitFor(QualityControlImpl *This, IMediaSample *sample, HANDLE ev) {
    REFERENCE_TIME start = -1, stop = -1, jitter = 0;
    This->current_rstart = This->current_rstop = -1;
    This->current_jitter = 0;
    if (!This->clock || FAILED(IMediaSample_GetTime(sample, &start, &stop)))
        return S_OK;

    if (start >= 0) {
        REFERENCE_TIME now;
        IReferenceClock_GetTime(This->clock, &now);
        now -= This->clockstart;

        jitter = now - start;
        if (jitter <= -10000) {
            DWORD_PTR cookie;
            IReferenceClock_AdviseTime(This->clock, This->clockstart, start, (HEVENT)ev, &cookie);
            WaitForSingleObject(ev, INFINITE);
            IReferenceClock_Unadvise(This->clock, cookie);
        }
    }
    else
        start = stop = -1;
    This->current_rstart = start;
    This->current_rstop = stop > start ? stop : start;
    This->current_jitter = jitter;
    This->is_dropped = QualityControlRender_IsLate(This, jitter, start, stop);
    TRACE("Dropped: %i %i %i %i\n", This->is_dropped, (int)(start/10000), (int)(stop/10000), (int)(jitter / 10000));
    if (This->is_dropped) {
        This->dropped++;
        if (!This->qos_handled)
            return S_FALSE;
    } else
        This->rendered++;
    return S_OK;
}

void QualityControlRender_DoQOS(QualityControlImpl *priv)
{
    REFERENCE_TIME start, stop, jitter, pt, entered, left, duration;
    double rate;

    if (!priv->clock || priv->current_rstart < 0)
        return;

    start = priv->current_rstart;
    stop = priv->current_rstop;
    jitter = priv->current_jitter;

    if (jitter < 0) {
        /* this is the time the buffer entered the sink */
        if (start < -jitter)
            entered = 0;
        else
            entered = start + jitter;
        left = start;
    } else {
        /* this is the time the buffer entered the sink */
        entered = start + jitter;
        /* this is the time the buffer left the sink */
        left = start + jitter;
    }

    /* calculate duration of the buffer */
    if (stop >= start)
        duration = stop - start;
    else
        duration = 0;

    /* if we have the time when the last buffer left us, calculate
     * processing time */
    if (priv->last_left >= 0) {
        if (entered > priv->last_left) {
            pt = entered - priv->last_left;
        } else {
            pt = 0;
        }
    } else {
        pt = priv->avg_pt;
    }

#define XTIME(u) (int)(u/10000000), (int)((u / 10000)%1000)
    TRACE("start: %u.%03u, entered %u.%03u, left %u.%03u, pt: %u.%03u, "
          "duration %u.%03u, jitter %u.%03u\n", XTIME(start), XTIME(entered),
          XTIME(left), XTIME(pt), XTIME(duration), XTIME(jitter));

    TRACE("avg_duration: %u.%03u, avg_pt: %u.%03u, avg_rate: %g\n",
      XTIME(priv->avg_duration), XTIME(priv->avg_pt), priv->avg_rate);
#undef XTIME

    /* collect running averages. for first observations, we copy the
    * values */
    if (priv->avg_duration < 0)
        priv->avg_duration = duration;
    else
        priv->avg_duration = UPDATE_RUNNING_AVG (priv->avg_duration, duration);

    if (priv->avg_pt < 0)
        priv->avg_pt = pt;
    else
        priv->avg_pt = UPDATE_RUNNING_AVG (priv->avg_pt, pt);

    if (priv->avg_duration != 0)
        rate =
            (double)priv->avg_pt /
            (double)priv->avg_duration;
    else
        rate = 0.0;

    if (priv->last_left >= 0) {
        if (priv->is_dropped || priv->avg_rate < 0.0) {
            priv->avg_rate = rate;
        } else {
            if (rate > 1.0)
                priv->avg_rate = UPDATE_RUNNING_AVG_N (priv->avg_rate, rate);
            else
                priv->avg_rate = UPDATE_RUNNING_AVG_P (priv->avg_rate, rate);
        }
    }

    if (priv->avg_rate >= 0.0) {
        HRESULT hr;
        Quality q;
        /* if we have a valid rate, start sending QoS messages */
        if (priv->current_jitter < 0) {
            /* make sure we never go below 0 when adding the jitter to the
             * timestamp. */
            if (priv->current_rstart < -priv->current_jitter)
                priv->current_jitter = -priv->current_rstart;
        }
        else
            priv->current_jitter += (priv->current_rstop - priv->current_rstart);
        q.Type = (jitter > 0 ? Famine : Flood);
        q.Proportion = (LONG)(1000. / priv->avg_rate);
        if (q.Proportion < 200)
            q.Proportion = 200;
        else if (q.Proportion > 5000)
            q.Proportion = 5000;
        q.Late = priv->current_jitter;
        q.TimeStamp = priv->current_rstart;
        TRACE("Late: %i from %i, rate: %g\n", (int)(q.Late/10000), (int)(q.TimeStamp/10000), 1./priv->avg_rate);
        hr = IQualityControl_Notify((IQualityControl *)priv, priv->self, q);
        priv->qos_handled = hr == S_OK;
    }

    /* record when this buffer will leave us */
    priv->last_left = left;
}


void QualityControlRender_BeginRender(QualityControlImpl *This) {
    This->start = -1;
    if (!This->clock)
        return;
    IReferenceClock_GetTime(This->clock, &This->start);
}

void QualityControlRender_EndRender(QualityControlImpl *This) {
    REFERENCE_TIME elapsed;
    if (!This->clock || This->start < 0 || FAILED(IReferenceClock_GetTime(This->clock, &This->stop)))
        return;

    elapsed = This->start - This->stop;
    if (elapsed < 0)
        return;
    if (This->avg_render < 0)
        This->avg_render = elapsed;
    else
        This->avg_render = UPDATE_RUNNING_AVG (This->avg_render, elapsed);
}
