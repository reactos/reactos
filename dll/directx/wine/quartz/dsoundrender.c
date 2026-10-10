/*
 * Direct Sound Audio Renderer
 *
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

#include "quartz_private.h"

#include "uuids.h"
#include "vfwmsgs.h"
#include "windef.h"
#include "winbase.h"
#include "dshow.h"
#include "evcode.h"
#include "strmif.h"
#include "dsound.h"
#include "amaudio.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(quartz);

/* NOTE: buffer can still be filled completely,
 * but we start waiting until only this amount is buffered
 */
static const REFERENCE_TIME DSoundRenderer_Max_Fill = 150 * 10000;

struct dsound_render
{
    struct strmbase_filter filter;
    struct strmbase_passthrough passthrough;
    IAMDirectSound IAMDirectSound_iface;
    IBasicAudio IBasicAudio_iface;
    IQualityControl IQualityControl_iface;
    IUnknown *system_clock;

    struct strmbase_sink sink;

    HANDLE render_thread;
    CRITICAL_SECTION render_cs;
    CONDITION_VARIABLE render_cv;
    IMediaSample **queued_samples;
    size_t queued_sample_count, queued_samples_capacity;
    bool render_thread_shutdown;

    /* Signaled when the filter has completed a state change. The filter waits
     * for this event in IBaseFilter::GetState(). */
    HANDLE state_event;
    /* Signaled when a flush or state change occurs, i.e. anything that needs
     * to immediately unblock the streaming thread. */
    HANDLE flush_event;
    REFERENCE_TIME stream_start;
    BOOL eos;

    IDirectSound8 *dsound;
    LPDIRECTSOUNDBUFFER dsbuffer;
    DWORD buf_size;
    DWORD last_playpos, writepos;

    LONG volume;
    LONG pan;
};

static struct dsound_render *impl_from_strmbase_pin(struct strmbase_pin *iface)
{
    return CONTAINING_RECORD(iface, struct dsound_render, sink.pin);
}

static struct dsound_render *impl_from_strmbase_filter(struct strmbase_filter *iface)
{
    return CONTAINING_RECORD(iface, struct dsound_render, filter);
}

static struct dsound_render *impl_from_IBasicAudio(IBasicAudio *iface)
{
    return CONTAINING_RECORD(iface, struct dsound_render, IBasicAudio_iface);
}

static struct dsound_render *impl_from_IAMDirectSound(IAMDirectSound *iface)
{
    return CONTAINING_RECORD(iface, struct dsound_render, IAMDirectSound_iface);
}

static REFERENCE_TIME time_from_pos(struct dsound_render *filter, DWORD pos)
{
    WAVEFORMATEX *wfx = (WAVEFORMATEX *)filter->sink.pin.mt.pbFormat;
    REFERENCE_TIME ret = 10000000;
    ret = ret * pos / wfx->nAvgBytesPerSec;
    return ret;
}

static DWORD pos_from_time(struct dsound_render *filter, REFERENCE_TIME time)
{
    WAVEFORMATEX *wfx = (WAVEFORMATEX *)filter->sink.pin.mt.pbFormat;
    REFERENCE_TIME ret = time;
    ret *= wfx->nAvgBytesPerSec;
    ret /= 10000000;
    ret -= ret % wfx->nBlockAlign;
    return ret;
}

static void update_positions(struct dsound_render *filter, DWORD *seqwritepos, DWORD *minwritepos)
{
    WAVEFORMATEX *wfx = (WAVEFORMATEX *)filter->sink.pin.mt.pbFormat;
    BYTE *buf1, *buf2;
    DWORD size1, size2, playpos, writepos, old_writepos, old_playpos, adv;
    BOOL writepos_set = filter->writepos < filter->buf_size;

    /* Update position and zero */
    old_writepos = filter->writepos;
    old_playpos = filter->last_playpos;
    if (old_writepos <= old_playpos)
        old_writepos += filter->buf_size;

    IDirectSoundBuffer_GetCurrentPosition(filter->dsbuffer, &playpos, &writepos);
    if (old_playpos > playpos)
        adv = filter->buf_size + playpos - old_playpos;
    else
        adv = playpos - old_playpos;
    filter->last_playpos = playpos;
    if (adv)
    {
        TRACE("Moving from %lu to %lu: clearing %lu bytes.\n", old_playpos, playpos, adv);
        IDirectSoundBuffer_Lock(filter->dsbuffer, old_playpos, adv,
                (void **)&buf1, &size1, (void **)&buf2, &size2, 0);
        memset(buf1, wfx->wBitsPerSample == 8 ? 128  : 0, size1);
        memset(buf2, wfx->wBitsPerSample == 8 ? 128  : 0, size2);
        IDirectSoundBuffer_Unlock(filter->dsbuffer, buf1, size1, buf2, size2);
    }
    *minwritepos = writepos;
    if (!writepos_set || old_writepos < writepos)
    {
        if (writepos_set)
        {
            filter->writepos = filter->buf_size;
            FIXME("Underrun of data occurred!\n");
        }
        *seqwritepos = writepos;
    }
    else
        *seqwritepos = filter->writepos;
}

static HRESULT get_write_pos(struct dsound_render *filter,
        DWORD *ret_writepos, REFERENCE_TIME write_at, DWORD *pfree)
{
    DWORD writepos, min_writepos, playpos;
    REFERENCE_TIME max_lag = 50 * 10000;
    REFERENCE_TIME cur, writepos_t, delta_t;

    update_positions(filter, &writepos, &min_writepos);
    playpos = filter->last_playpos;
    if (filter->filter.clock)
    {
        IReferenceClock_GetTime(filter->filter.clock, &cur);
        cur -= filter->stream_start;
    }
    else
        write_at = -1;

    if (writepos == min_writepos)
        max_lag = 0;

    if (write_at < 0)
    {
        *ret_writepos = writepos;
        goto end;
    }

    if (writepos >= playpos)
        writepos_t = cur + time_from_pos(filter, writepos - playpos);
    else
        writepos_t = cur + time_from_pos(filter, filter->buf_size + writepos - playpos);

    /* write_at: Starting time of sample */
    /* cur: current time of play position */
    /* writepos_t: current time of our pointer play position */
    delta_t = write_at - writepos_t;
    TRACE("Last sample end %s, this sample start %s.\n",
            debugstr_time(writepos_t), debugstr_time(write_at));
    if (delta_t <= max_lag)
    {
        /* If the stream time of a sample is in the past (i.e. the sample is
         * late, in which case write_at < min_writepos), or earlier than the
         * last sample's end time, or there is a gap between the last sample's
         * end time and this sample less than a certain threshold, native
         * simply treats the two as continuous, ignoring this sample's play time
         * and rendering the whole sample even if it's late. */
        TRACE("Difference %s is less than threshold %s; treating sample as continuous.\n",
                debugstr_time(delta_t), debugstr_time(max_lag));
        *ret_writepos = writepos;
    }
    else
    {
        DWORD aheadbytes;
        WARN("Delta too big %s/%s, too far ahead.\n", debugstr_time(delta_t), debugstr_time(max_lag));
        aheadbytes = pos_from_time(filter, delta_t);
        WARN("Advancing %lu bytes.\n", aheadbytes);
        if (delta_t >= DSoundRenderer_Max_Fill)
            return S_FALSE;
        *ret_writepos = (min_writepos + aheadbytes) % filter->buf_size;
    }
end:
    if (playpos >= *ret_writepos)
        *pfree = playpos - *ret_writepos;
    else
        *pfree = filter->buf_size + playpos - *ret_writepos;
    if (time_from_pos(filter, filter->buf_size - *pfree) >= DSoundRenderer_Max_Fill)
    {
        TRACE("Blocked: too full %s / %s\n", debugstr_time(time_from_pos(filter, filter->buf_size - *pfree)),
                debugstr_time(DSoundRenderer_Max_Fill));
        return S_FALSE;
    }
    return S_OK;
}

static HRESULT send_sample_data(struct dsound_render *filter,
        REFERENCE_TIME tStart, const BYTE *data, DWORD size)
{
    HRESULT hr;

    while (size && filter->filter.state != State_Stopped)
    {
        DWORD writepos, free, size1, size2;
        BYTE *buf1, *buf2;

        if (filter->filter.state == State_Running)
            hr = get_write_pos(filter, &writepos, tStart, &free);
        else
            hr = S_FALSE;

        if (hr != S_OK)
        {
            if (!WaitForSingleObject(filter->flush_event, 10))
            {
                TRACE("Flush signaled; discarding sample.\n");
                return VFW_E_WRONG_STATE;
            }
            continue;
        }
        tStart = -1;

        hr = IDirectSoundBuffer_Lock(filter->dsbuffer, writepos, min(free, size),
                (void **)&buf1, &size1, (void **)&buf2, &size2, 0);
        if (hr != DS_OK)
        {
            ERR("Failed to lock sound buffer, hr %#lx.\n", hr);
            break;
        }

        if (data)
        {
            memcpy(buf1, data, size1);
            if (size2)
                memcpy(buf2, data + size1, size2);
            data += size1 + size2;
        }
        else
        {
            const WAVEFORMATEX *wfx = (const WAVEFORMATEX *)filter->sink.pin.mt.pbFormat;
            char silence = (wfx->wBitsPerSample == 8 ? 0x80 : 0);

            memset(buf1, silence, size1);
            if (size2)
                memset(buf2, silence, size2);
        }
        IDirectSoundBuffer_Unlock(filter->dsbuffer, buf1, size1, buf2, size2);
        filter->writepos = (writepos + size1 + size2) % filter->buf_size;
        TRACE("Wrote %lu bytes at %lu, next at %lu - (%lu/%lu)\n",
                size1 + size2, writepos, filter->writepos, free, size);
        size -= size1 + size2;
    }
    return S_OK;
}

static HRESULT configure_buffer(struct dsound_render *filter, IMediaSample *pSample)
{
    HRESULT hr;
    AM_MEDIA_TYPE *amt;

    if (IMediaSample_GetMediaType(pSample, &amt) == S_OK)
    {
        AM_MEDIA_TYPE *orig = &filter->sink.pin.mt;
        WAVEFORMATEX *origfmt = (WAVEFORMATEX *)orig->pbFormat;
        WAVEFORMATEX *newfmt = (WAVEFORMATEX *)amt->pbFormat;

        TRACE("Format change.\n");
        strmbase_dump_media_type(amt);

        if (origfmt->wFormatTag == newfmt->wFormatTag &&
            origfmt->nChannels == newfmt->nChannels &&
            origfmt->nBlockAlign == newfmt->nBlockAlign &&
            origfmt->wBitsPerSample == newfmt->wBitsPerSample &&
            origfmt->cbSize ==  newfmt->cbSize)
        {
            if (origfmt->nSamplesPerSec != newfmt->nSamplesPerSec)
            {
                hr = IDirectSoundBuffer_SetFrequency(filter->dsbuffer,
                                                     newfmt->nSamplesPerSec);
                if (FAILED(hr))
                    return VFW_E_TYPE_NOT_ACCEPTED;
                FreeMediaType(orig);
                CopyMediaType(orig, amt);
                IMediaSample_SetMediaType(pSample, NULL);
            }
        }
        else
            return VFW_E_TYPE_NOT_ACCEPTED;
    }
    return S_OK;
}

static HRESULT render_sample(struct dsound_render *filter, IMediaSample *pSample)
{
    REFERENCE_TIME start = -1, stop = -1;
    LPBYTE pbSrcStream = NULL;
    LONG cbSrcStream = 0;
    HRESULT hr;

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Failed to get buffer pointer, hr %#lx.\n", hr);
        return hr;
    }

    if (IMediaSample_IsDiscontinuity(pSample) == S_OK
            && FAILED(hr = IMediaSample_GetTime(pSample, &start, &stop)))
    {
        ERR("Failed to get sample time, hr %#lx.\n", hr);
        start = stop = -1;
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);
    return send_sample_data(filter, start, pbSrcStream, cbSrcStream);
}

static DWORD WINAPI render_thread_run(void *arg)
{
    struct dsound_render *filter = arg;

    TRACE("Render thread started.\n");

    EnterCriticalSection(&filter->render_cs);

    while (!filter->render_thread_shutdown)
    {
        IMediaSample *sample;

        if (filter->eos)
        {
            LeaveCriticalSection(&filter->render_cs);
            TRACE("Got EOS.\n");
            /* Clear the buffer. */
            send_sample_data(filter, -1, NULL, filter->buf_size);

            TRACE("Render thread exiting.\n");
            return 0;
        }

        if (!filter->queued_sample_count)
        {
            SleepConditionVariableCS(&filter->render_cv, &filter->render_cs, INFINITE);
            continue;
        }

        sample = filter->queued_samples[0];
        if (--filter->queued_sample_count)
            memmove(filter->queued_samples, filter->queued_samples + 1,
                    filter->queued_sample_count * sizeof(*filter->queued_samples));

        LeaveCriticalSection(&filter->render_cs);

        render_sample(filter, sample);
        IMediaSample_Release(sample);

        EnterCriticalSection(&filter->render_cs);
    }

    LeaveCriticalSection(&filter->render_cs);
    TRACE("Render thread exiting.\n");
    return 0;
}

static HRESULT WINAPI dsound_render_sink_Receive(struct strmbase_sink *iface, IMediaSample *sample)
{
    struct dsound_render *filter = impl_from_strmbase_pin(&iface->pin);
    HRESULT hr;

    TRACE("filter %p, sample %p.\n", filter, sample);

    if (filter->eos || filter->sink.flushing)
        return S_FALSE;

    if (filter->filter.state == State_Stopped)
        return VFW_E_WRONG_STATE;

    if (FAILED(hr = configure_buffer(filter, sample)))
        return hr;

    if (filter->filter.state == State_Paused)
        SetEvent(filter->state_event);

    EnterCriticalSection(&filter->render_cs);

    if (!array_reserve((void **)&filter->queued_samples, &filter->queued_samples_capacity,
            filter->queued_sample_count + 1, sizeof(*filter->queued_samples)))
    {
        LeaveCriticalSection(&filter->render_cs);
        return E_OUTOFMEMORY;
    }

    filter->queued_samples[filter->queued_sample_count++] = sample;
    IMediaSample_AddRef(sample);

    LeaveCriticalSection(&filter->render_cs);
    WakeConditionVariable(&filter->render_cv);
    return S_OK;
}

static HRESULT dsound_render_sink_query_interface(struct strmbase_pin *iface, REFIID iid, void **out)
{
    struct dsound_render *filter = impl_from_strmbase_pin(iface);

    if (IsEqualGUID(iid, &IID_IMemInputPin))
        *out = &filter->sink.IMemInputPin_iface;
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT dsound_render_sink_query_accept(struct strmbase_pin *iface, const AM_MEDIA_TYPE * pmt)
{
    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Audio))
        return S_FALSE;

    return S_OK;
}

static HRESULT dsound_render_sink_connect(struct strmbase_sink *iface, IPin *peer, const AM_MEDIA_TYPE *mt)
{
    struct dsound_render *filter = impl_from_strmbase_pin(&iface->pin);
    const WAVEFORMATEX *format = (WAVEFORMATEX *)mt->pbFormat;
    HRESULT hr = S_OK;
    DSBUFFERDESC buf_desc;

    filter->buf_size = format->nAvgBytesPerSec;

    memset(&buf_desc,0,sizeof(DSBUFFERDESC));
    buf_desc.dwSize = sizeof(DSBUFFERDESC);
    buf_desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN |
                       DSBCAPS_CTRLFREQUENCY | DSBCAPS_GLOBALFOCUS |
                       DSBCAPS_GETCURRENTPOSITION2;
    buf_desc.dwBufferBytes = filter->buf_size;
    buf_desc.lpwfxFormat = (WAVEFORMATEX *)format;
    hr = IDirectSound8_CreateSoundBuffer(filter->dsound, &buf_desc, &filter->dsbuffer, NULL);
    filter->writepos = filter->buf_size;
    if (FAILED(hr))
        ERR("Failed to create sound buffer, hr %#lx.\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IDirectSoundBuffer_SetVolume(filter->dsbuffer, filter->volume);
        if (FAILED(hr))
            ERR("Failed to set volume to %ld, hr %#lx.\n", filter->volume, hr);

        hr = IDirectSoundBuffer_SetPan(filter->dsbuffer, filter->pan);
        if (FAILED(hr))
            ERR("Failed to set pan to %ld, hr %#lx.\n", filter->pan, hr);
        hr = S_OK;
    }

    if (FAILED(hr) && hr != VFW_E_ALREADY_CONNECTED)
    {
        if (filter->dsbuffer)
            IDirectSoundBuffer_Release(filter->dsbuffer);
        filter->dsbuffer = NULL;
    }

    return hr;
}

static void dsound_render_sink_disconnect(struct strmbase_sink *iface)
{
    struct dsound_render *filter = impl_from_strmbase_pin(&iface->pin);

    TRACE("filter %p.\n", filter);

    if (filter->dsbuffer)
        IDirectSoundBuffer_Release(filter->dsbuffer);
    filter->dsbuffer = NULL;
}

static HRESULT dsound_render_sink_eos(struct strmbase_sink *iface)
{
    struct dsound_render *filter = impl_from_strmbase_pin(&iface->pin);
    IFilterGraph *graph = filter->filter.graph;
    IMediaEventSink *event_sink;

    EnterCriticalSection(&filter->render_cs);
    filter->eos = TRUE;
    LeaveCriticalSection(&filter->render_cs);
    WakeConditionVariable(&filter->render_cv);

    if (filter->filter.state == State_Running && graph
            && SUCCEEDED(IFilterGraph_QueryInterface(graph,
            &IID_IMediaEventSink, (void **)&event_sink)))
    {
        IMediaEventSink_Notify(event_sink, EC_COMPLETE, S_OK,
                (LONG_PTR)&filter->filter.IBaseFilter_iface);
        IMediaEventSink_Release(event_sink);
    }
    SetEvent(filter->state_event);

    return S_OK;
}

static HRESULT dsound_render_sink_begin_flush(struct strmbase_sink *iface)
{
    struct dsound_render *filter = impl_from_strmbase_pin(&iface->pin);

    SetEvent(filter->flush_event);
    return S_OK;
}

static HRESULT dsound_render_sink_end_flush(struct strmbase_sink *iface)
{
    struct dsound_render *filter = impl_from_strmbase_pin(&iface->pin);

    EnterCriticalSection(&filter->filter.stream_cs);
    if (filter->eos && filter->filter.state != State_Stopped)
    {
        WaitForSingleObject(filter->render_thread, INFINITE);
        CloseHandle(filter->render_thread);

        if (!(filter->render_thread = CreateThread(NULL, 0, render_thread_run, filter, 0, NULL)))
        {
            LeaveCriticalSection(&filter->filter.stream_cs);
            return HRESULT_FROM_WIN32(GetLastError());
        }
        filter->eos = FALSE;
    }

    ResetEvent(filter->flush_event);

    if (filter->dsbuffer)
    {
        void *buffer;
        DWORD size;

        /* Force a reset */
        IDirectSoundBuffer_Lock(filter->dsbuffer, 0, 0, &buffer, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER);
        memset(buffer, 0, size);
        IDirectSoundBuffer_Unlock(filter->dsbuffer, buffer, size, NULL, 0);
        filter->writepos = filter->buf_size;
    }

    LeaveCriticalSection(&filter->filter.stream_cs);
    return S_OK;
}

static const struct strmbase_sink_ops sink_ops =
{
    .base.pin_query_interface = dsound_render_sink_query_interface,
    .base.pin_query_accept = dsound_render_sink_query_accept,
    .pfnReceive = dsound_render_sink_Receive,
    .sink_connect = dsound_render_sink_connect,
    .sink_disconnect = dsound_render_sink_disconnect,
    .sink_eos = dsound_render_sink_eos,
    .sink_begin_flush = dsound_render_sink_begin_flush,
    .sink_end_flush = dsound_render_sink_end_flush,
};

static void dsound_render_destroy(struct strmbase_filter *iface)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    if (filter->dsbuffer)
        IDirectSoundBuffer_Release(filter->dsbuffer);
    filter->dsbuffer = NULL;
    if (filter->dsound)
        IDirectSound8_Release(filter->dsound);
    filter->dsound = NULL;

    IUnknown_Release(filter->system_clock);

    if (filter->sink.pin.peer)
        IPin_Disconnect(filter->sink.pin.peer);
    IPin_Disconnect(&filter->sink.pin.IPin_iface);
    strmbase_sink_cleanup(&filter->sink);

    CloseHandle(filter->state_event);
    CloseHandle(filter->flush_event);

    filter->render_cs.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&filter->render_cs);

    strmbase_passthrough_cleanup(&filter->passthrough);
    strmbase_filter_cleanup(&filter->filter);
    free(filter);
}

static struct strmbase_pin *dsound_render_get_pin(struct strmbase_filter *iface, unsigned int index)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    if (index == 0)
        return &filter->sink.pin;
    return NULL;
}

static HRESULT dsound_render_query_interface(struct strmbase_filter *iface, REFIID iid, void **out)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    if (IsEqualGUID(iid, &IID_IAMDirectSound))
        *out = &filter->IAMDirectSound_iface;
    else if (IsEqualGUID(iid, &IID_IBasicAudio))
        *out = &filter->IBasicAudio_iface;
    else if (IsEqualGUID(iid, &IID_IMediaPosition))
        *out = &filter->passthrough.IMediaPosition_iface;
    else if (IsEqualGUID(iid, &IID_IMediaSeeking))
        *out = &filter->passthrough.IMediaSeeking_iface;
    else if (IsEqualGUID(iid, &IID_IQualityControl))
        *out = &filter->IQualityControl_iface;
    else if (IsEqualGUID(iid, &IID_IReferenceClock))
        return IUnknown_QueryInterface(filter->system_clock, iid, out);
    else
        return E_NOINTERFACE;

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT dsound_render_init_stream(struct strmbase_filter *iface)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    filter->eos = FALSE;
    ResetEvent(filter->flush_event);

    if (!filter->sink.pin.peer)
        return S_OK;

    ResetEvent(filter->state_event);

    filter->render_thread_shutdown = false;
    if (!(filter->render_thread = CreateThread(NULL, 0, render_thread_run, filter, 0, NULL)))
        return HRESULT_FROM_WIN32(GetLastError());

    return S_FALSE;
}

static HRESULT dsound_render_start_stream(struct strmbase_filter *iface, REFERENCE_TIME start)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);
    IFilterGraph *graph = filter->filter.graph;
    IMediaEventSink *event_sink;

    filter->stream_start = start;

    SetEvent(filter->state_event);

    if (filter->sink.pin.peer)
        IDirectSoundBuffer_Play(filter->dsbuffer, 0, 0, DSBPLAY_LOOPING);

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

static HRESULT dsound_render_stop_stream(struct strmbase_filter *iface)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    if (filter->sink.pin.peer)
    {
        IDirectSoundBuffer_Stop(filter->dsbuffer);
        filter->writepos = filter->buf_size;
    }
    return S_OK;
}

static HRESULT dsound_render_cleanup_stream(struct strmbase_filter *iface)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    SetEvent(filter->state_event);
    SetEvent(filter->flush_event);

    if (filter->render_thread)
    {
        EnterCriticalSection(&filter->render_cs);
        filter->render_thread_shutdown = true;
        LeaveCriticalSection(&filter->render_cs);
        WakeConditionVariable(&filter->render_cv);
        WaitForSingleObject(filter->render_thread, INFINITE);
        CloseHandle(filter->render_thread);
        filter->render_thread = NULL;

        for (unsigned int i = 0; i < filter->queued_sample_count; ++i)
            IMediaSample_Release(filter->queued_samples[i]);
        filter->queued_sample_count = 0;
    }

    return S_OK;
}

static HRESULT dsound_render_wait_state(struct strmbase_filter *iface, DWORD timeout)
{
    struct dsound_render *filter = impl_from_strmbase_filter(iface);

    if (WaitForSingleObject(filter->state_event, timeout) == WAIT_TIMEOUT)
        return VFW_S_STATE_INTERMEDIATE;
    return S_OK;
}

static const struct strmbase_filter_ops filter_ops =
{
    .filter_destroy = dsound_render_destroy,
    .filter_get_pin = dsound_render_get_pin,
    .filter_query_interface = dsound_render_query_interface,
    .filter_init_stream = dsound_render_init_stream,
    .filter_start_stream = dsound_render_start_stream,
    .filter_stop_stream = dsound_render_stop_stream,
    .filter_cleanup_stream = dsound_render_cleanup_stream,
    .filter_wait_state = dsound_render_wait_state,
};

static HRESULT WINAPI basic_audio_QueryInterface(IBasicAudio *iface, REFIID riid, void **out)
{
    struct dsound_render *filter = impl_from_IBasicAudio(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, riid, out);
}

static ULONG WINAPI basic_audio_AddRef(IBasicAudio *iface)
{
    struct dsound_render *filter = impl_from_IBasicAudio(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI basic_audio_Release(IBasicAudio *iface)
{
    struct dsound_render *filter = impl_from_IBasicAudio(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI basic_audio_GetTypeInfoCount(IBasicAudio *iface, UINT *count)
{
    struct dsound_render *filter = impl_from_IBasicAudio(iface);

    TRACE("filter %p, count %p.\n", filter, count);

    *count = 1;
    return S_OK;
}

static HRESULT WINAPI basic_audio_GetTypeInfo(IBasicAudio *iface, UINT index,
        LCID lcid, ITypeInfo **typeinfo)
{
    struct dsound_render *filter = impl_from_IBasicAudio(iface);

    TRACE("filter %p, index %u, lcid %lu, typeinfo %p.\n", filter, index, lcid, typeinfo);

    return strmbase_get_typeinfo(IBasicAudio_tid, typeinfo);
}

static HRESULT WINAPI basic_audio_GetIDsOfNames(IBasicAudio *iface, REFIID iid,
        LPOLESTR *names, UINT count, LCID lcid, DISPID *ids)
{
    struct dsound_render *filter = impl_from_IBasicAudio(iface);
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("filter %p, iid %s, names %p, count %u, lcid %lu, ids %p.\n",
            filter, debugstr_guid(iid), names, count, lcid, ids);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicAudio_tid, &typeinfo)))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, names, count, ids);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI basic_audio_Invoke(IBasicAudio *iface, DISPID id, REFIID iid, LCID lcid,
        WORD flags, DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *error_arg)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("filter %p, id %ld, iid %s, lcid %lu, flags %u, params %p, result %p, excepinfo %p, error_arg %p.\n",
          iface, id, debugstr_guid(iid), lcid, flags, params, result, excepinfo, error_arg);

    if (SUCCEEDED(hr = strmbase_get_typeinfo(IBasicAudio_tid, &typeinfo)))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, id, flags, params, result, excepinfo, error_arg);
        ITypeInfo_Release(typeinfo);
    }
    return hr;
}

static HRESULT WINAPI basic_audio_put_Volume(IBasicAudio *iface, LONG volume)
{
    struct dsound_render *filter = impl_from_IBasicAudio(iface);

    TRACE("filter %p, volume %ld.\n", filter, volume);

    if (volume > DSBVOLUME_MAX || volume < DSBVOLUME_MIN)
        return E_INVALIDARG;

    if (filter->dsbuffer && FAILED(IDirectSoundBuffer_SetVolume(filter->dsbuffer, volume)))
        return E_FAIL;

    filter->volume = volume;
    return S_OK;
}

static HRESULT WINAPI basic_audio_get_Volume(IBasicAudio *iface, LONG *volume)
{
    struct dsound_render *filter = impl_from_IBasicAudio(iface);

    TRACE("filter %p, volume %p.\n", filter, volume);

    if (!volume)
        return E_POINTER;

    *volume = filter->volume;
    return S_OK;
}

static HRESULT WINAPI basic_audio_put_Balance(IBasicAudio *iface, LONG balance)
{
    struct dsound_render *filter = impl_from_IBasicAudio(iface);

    TRACE("filter %p, balance %ld.\n", filter, balance);

    if (balance < DSBPAN_LEFT || balance > DSBPAN_RIGHT)
        return E_INVALIDARG;

    if (filter->dsbuffer && FAILED(IDirectSoundBuffer_SetPan(filter->dsbuffer, balance)))
        return E_FAIL;

    filter->pan = balance;
    return S_OK;
}

static HRESULT WINAPI basic_audio_get_Balance(IBasicAudio *iface, LONG *balance)
{
    struct dsound_render *filter = impl_from_IBasicAudio(iface);

    TRACE("filter %p, balance %p.\n", filter, balance);

    if (!balance)
        return E_POINTER;

    *balance = filter->pan;
    return S_OK;
}

static const IBasicAudioVtbl basic_audio_vtbl =
{
    basic_audio_QueryInterface,
    basic_audio_AddRef,
    basic_audio_Release,
    basic_audio_GetTypeInfoCount,
    basic_audio_GetTypeInfo,
    basic_audio_GetIDsOfNames,
    basic_audio_Invoke,
    basic_audio_put_Volume,
    basic_audio_get_Volume,
    basic_audio_put_Balance,
    basic_audio_get_Balance,
};

static HRESULT WINAPI direct_sound_QueryInterface(IAMDirectSound *iface, REFIID riid, void **out)
{
    struct dsound_render *filter = impl_from_IAMDirectSound(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, riid, out);
}

static ULONG WINAPI direct_sound_AddRef(IAMDirectSound *iface)
{
    struct dsound_render *filter = impl_from_IAMDirectSound(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI direct_sound_Release(IAMDirectSound *iface)
{
    struct dsound_render *filter = impl_from_IAMDirectSound(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI direct_sound_GetDirectSoundInterface(IAMDirectSound *iface,  IDirectSound **ds)
{
    struct dsound_render *filter = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", filter, iface, ds);

    return E_NOTIMPL;
}

static HRESULT WINAPI direct_sound_GetPrimaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer **buf)
{
    struct dsound_render *filter = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", filter, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI direct_sound_GetSecondaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer **buf)
{
    struct dsound_render *filter = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", filter, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI direct_sound_ReleaseDirectSoundInterface(IAMDirectSound *iface, IDirectSound *ds)
{
    struct dsound_render *filter = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", filter, iface, ds);

    return E_NOTIMPL;
}

static HRESULT WINAPI direct_sound_ReleasePrimaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer *buf)
{
    struct dsound_render *filter = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", filter, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI direct_sound_ReleaseSecondaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer *buf)
{
    struct dsound_render *filter = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", filter, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI direct_sound_SetFocusWindow(IAMDirectSound *iface, HWND hwnd, BOOL bgaudible)
{
    struct dsound_render *filter = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p,%d): stub\n", filter, iface, hwnd, bgaudible);

    return E_NOTIMPL;
}

static HRESULT WINAPI direct_sound_GetFocusWindow(IAMDirectSound *iface, HWND *hwnd, BOOL *bgaudible)
{
    struct dsound_render *filter = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", filter, iface, hwnd, bgaudible);

    return E_NOTIMPL;
}

static const IAMDirectSoundVtbl direct_sound_vtbl =
{
    direct_sound_QueryInterface,
    direct_sound_AddRef,
    direct_sound_Release,
    direct_sound_GetDirectSoundInterface,
    direct_sound_GetPrimaryBufferInterface,
    direct_sound_GetSecondaryBufferInterface,
    direct_sound_ReleaseDirectSoundInterface,
    direct_sound_ReleasePrimaryBufferInterface,
    direct_sound_ReleaseSecondaryBufferInterface,
    direct_sound_SetFocusWindow,
    direct_sound_GetFocusWindow,
};

static struct dsound_render *impl_from_IQualityControl(IQualityControl *iface)
{
    return CONTAINING_RECORD(iface, struct dsound_render, IQualityControl_iface);
}

static HRESULT WINAPI quality_control_QueryInterface(IQualityControl *iface,
        REFIID iid, void **out)
{
    struct dsound_render *filter = impl_from_IQualityControl(iface);
    return IUnknown_QueryInterface(filter->filter.outer_unk, iid, out);
}

static ULONG WINAPI quality_control_AddRef(IQualityControl *iface)
{
    struct dsound_render *filter = impl_from_IQualityControl(iface);
    return IUnknown_AddRef(filter->filter.outer_unk);
}

static ULONG WINAPI quality_control_Release(IQualityControl *iface)
{
    struct dsound_render *filter = impl_from_IQualityControl(iface);
    return IUnknown_Release(filter->filter.outer_unk);
}

static HRESULT WINAPI quality_control_Notify(IQualityControl *iface,
        IBaseFilter *sender, Quality q)
{
    struct dsound_render *filter = impl_from_IQualityControl(iface);

    FIXME("filter %p, sender %p, type %#x, proportion %ld, late %s, timestamp %s, stub!\n",
            filter, sender, q.Type, q.Proportion, debugstr_time(q.Late), debugstr_time(q.TimeStamp));

    return E_NOTIMPL;
}

static HRESULT WINAPI quality_control_SetSink(IQualityControl *iface, IQualityControl *sink)
{
    struct dsound_render *filter = impl_from_IQualityControl(iface);

    FIXME("filter %p, sink %p, stub!\n", filter, sink);

    return E_NOTIMPL;
}

static const IQualityControlVtbl quality_control_vtbl =
{
    quality_control_QueryInterface,
    quality_control_AddRef,
    quality_control_Release,
    quality_control_Notify,
    quality_control_SetSink,
};

HRESULT dsound_render_create(IUnknown *outer, IUnknown **out)
{
    static const DSBUFFERDESC buffer_desc =
    {
        .dwSize = sizeof(DSBUFFERDESC),
        .dwFlags = DSBCAPS_PRIMARYBUFFER,
    };

    struct dsound_render *object;
    IDirectSoundBuffer *buffer;
    HRESULT hr;

    TRACE("outer %p, out %p.\n", outer, out);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    strmbase_filter_init(&object->filter, outer, &CLSID_DSoundRender, &filter_ops);

    if (FAILED(hr = system_clock_create(&object->filter.IUnknown_inner, &object->system_clock)))
    {
        strmbase_filter_cleanup(&object->filter);
        free(object);
        return hr;
    }

    if (FAILED(hr = DirectSoundCreate8(NULL, &object->dsound, NULL)))
    {
        IUnknown_Release(object->system_clock);
        strmbase_filter_cleanup(&object->filter);
        free(object);
        return hr == DSERR_NODRIVER ? VFW_E_NO_AUDIO_HARDWARE : hr;
    }

    if (FAILED(hr = IDirectSound8_SetCooperativeLevel(object->dsound,
            GetDesktopWindow(), DSSCL_PRIORITY)))
    {
        IDirectSound8_Release(object->dsound);
        IUnknown_Release(object->system_clock);
        strmbase_filter_cleanup(&object->filter);
        free(object);
        return hr;
    }

    if (SUCCEEDED(hr = IDirectSound8_CreateSoundBuffer(object->dsound,
            &buffer_desc, &buffer, NULL)))
    {
        IDirectSoundBuffer_Play(buffer, 0, 0, DSBPLAY_LOOPING);
        IDirectSoundBuffer_Release(buffer);
    }

    strmbase_passthrough_init(&object->passthrough, (IUnknown *)&object->filter.IBaseFilter_iface);
    ISeekingPassThru_Init(&object->passthrough.ISeekingPassThru_iface, TRUE, &object->sink.pin.IPin_iface);

    strmbase_sink_init(&object->sink, &object->filter, L"Audio Input pin (rendered)", &sink_ops, NULL);

    object->state_event = CreateEventW(NULL, TRUE, TRUE, NULL);
    object->flush_event = CreateEventW(NULL, TRUE, TRUE, NULL);

    object->IBasicAudio_iface.lpVtbl = &basic_audio_vtbl;
    object->IAMDirectSound_iface.lpVtbl = &direct_sound_vtbl;
    object->IQualityControl_iface.lpVtbl = &quality_control_vtbl;

    InitializeCriticalSectionEx(&object->render_cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    object->render_cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": dsound_render.render_cs");
    InitializeConditionVariable(&object->render_cv);

    TRACE("Created DirectSound renderer %p.\n", object);
    *out = &object->filter.IUnknown_inner;

    return S_OK;
}
