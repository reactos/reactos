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

/* NOTE: buffer can still be filled completely,
 * but we start waiting until only this amount is buffered
 */
static const REFERENCE_TIME DSoundRenderer_Max_Fill = 150 * 10000;

static const IBaseFilterVtbl DSoundRender_Vtbl;
static const IBasicAudioVtbl IBasicAudio_Vtbl;
static const IReferenceClockVtbl IReferenceClock_Vtbl;
static const IAMDirectSoundVtbl IAMDirectSound_Vtbl;
static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl;

typedef struct DSoundRenderImpl
{
    BaseRenderer renderer;
    BasicAudio basicAudio;

    IReferenceClock IReferenceClock_iface;
    IAMDirectSound IAMDirectSound_iface;
    IAMFilterMiscFlags IAMFilterMiscFlags_iface;

    IDirectSound8 *dsound;
    LPDIRECTSOUNDBUFFER dsbuffer;
    DWORD buf_size;
    DWORD in_loop;
    DWORD last_playpos, writepos;

    REFERENCE_TIME play_time;

    HANDLE blocked;

    LONG volume;
    LONG pan;

    DWORD threadid;
    HANDLE advisethread, thread_wait;
} DSoundRenderImpl;

static inline DSoundRenderImpl *impl_from_BaseRenderer(BaseRenderer *iface)
{
    return CONTAINING_RECORD(iface, DSoundRenderImpl, renderer);
}

static inline DSoundRenderImpl *impl_from_IBaseFilter(IBaseFilter *iface)
{
    return CONTAINING_RECORD(iface, DSoundRenderImpl, renderer.filter.IBaseFilter_iface);
}

static inline DSoundRenderImpl *impl_from_IBasicAudio(IBasicAudio *iface)
{
    return CONTAINING_RECORD(iface, DSoundRenderImpl, basicAudio.IBasicAudio_iface);
}

static inline DSoundRenderImpl *impl_from_IReferenceClock(IReferenceClock *iface)
{
    return CONTAINING_RECORD(iface, DSoundRenderImpl, IReferenceClock_iface);
}

static inline DSoundRenderImpl *impl_from_IAMDirectSound(IAMDirectSound *iface)
{
    return CONTAINING_RECORD(iface, DSoundRenderImpl, IAMDirectSound_iface);
}

static inline DSoundRenderImpl *impl_from_IAMFilterMiscFlags(IAMFilterMiscFlags *iface)
{
    return CONTAINING_RECORD(iface, DSoundRenderImpl, IAMFilterMiscFlags_iface);
}

static REFERENCE_TIME time_from_pos(DSoundRenderImpl *This, DWORD pos) {
    WAVEFORMATEX *wfx = (WAVEFORMATEX*)This->renderer.pInputPin->pin.mtCurrent.pbFormat;
    REFERENCE_TIME ret = 10000000;
    ret = ret * pos / wfx->nAvgBytesPerSec;
    return ret;
}

static DWORD pos_from_time(DSoundRenderImpl *This, REFERENCE_TIME time) {
    WAVEFORMATEX *wfx = (WAVEFORMATEX*)This->renderer.pInputPin->pin.mtCurrent.pbFormat;
    REFERENCE_TIME ret = time;
    ret *= wfx->nSamplesPerSec;
    ret /= 10000000;
    ret *= wfx->nBlockAlign;
    return ret;
}

static void DSoundRender_UpdatePositions(DSoundRenderImpl *This, DWORD *seqwritepos, DWORD *minwritepos) {
    WAVEFORMATEX *wfx = (WAVEFORMATEX*)This->renderer.pInputPin->pin.mtCurrent.pbFormat;
    BYTE *buf1, *buf2;
    DWORD size1, size2, playpos, writepos, old_writepos, old_playpos, adv;
    BOOL writepos_set = This->writepos < This->buf_size;

    /* Update position and zero */
    old_writepos = This->writepos;
    old_playpos = This->last_playpos;
    if (old_writepos <= old_playpos)
        old_writepos += This->buf_size;

    IDirectSoundBuffer_GetCurrentPosition(This->dsbuffer, &playpos, &writepos);
    if (old_playpos > playpos) {
        adv = This->buf_size + playpos - old_playpos;
        This->play_time += time_from_pos(This, This->buf_size);
    } else
        adv = playpos - old_playpos;
    This->last_playpos = playpos;
    if (adv) {
        TRACE("Moving from %u to %u: clearing %u bytes\n", old_playpos, playpos, adv);
        IDirectSoundBuffer_Lock(This->dsbuffer, old_playpos, adv, (void**)&buf1, &size1, (void**)&buf2, &size2, 0);
        memset(buf1, wfx->wBitsPerSample == 8 ? 128  : 0, size1);
        memset(buf2, wfx->wBitsPerSample == 8 ? 128  : 0, size2);
        IDirectSoundBuffer_Unlock(This->dsbuffer, buf1, size1, buf2, size2);
    }
    *minwritepos = writepos;
    if (!writepos_set || old_writepos < writepos) {
        if (writepos_set) {
            This->writepos = This->buf_size;
            FIXME("Underrun of data occurred!\n");
        }
        *seqwritepos = writepos;
    } else
        *seqwritepos = This->writepos;
}

static HRESULT DSoundRender_GetWritePos(DSoundRenderImpl *This, DWORD *ret_writepos, REFERENCE_TIME write_at, DWORD *pfree, DWORD *skip)
{
    WAVEFORMATEX *wfx = (WAVEFORMATEX*)This->renderer.pInputPin->pin.mtCurrent.pbFormat;
    DWORD writepos, min_writepos, playpos;
    REFERENCE_TIME max_lag = 50 * 10000;
    REFERENCE_TIME min_lag = 25 * 10000;
    REFERENCE_TIME cur, writepos_t, delta_t;

    DSoundRender_UpdatePositions(This, &writepos, &min_writepos);
    playpos = This->last_playpos;
    if (This->renderer.filter.pClock == &This->IReferenceClock_iface) {
        max_lag = min_lag;
        cur = This->play_time + time_from_pos(This, playpos);
        cur -= This->renderer.filter.rtStreamStart;
    } else if (This->renderer.filter.pClock) {
        IReferenceClock_GetTime(This->renderer.filter.pClock, &cur);
        cur -= This->renderer.filter.rtStreamStart;
    } else
        write_at = -1;

    if (writepos == min_writepos)
        max_lag = 0;

    *skip = 0;
    if (write_at < 0) {
        *ret_writepos = writepos;
        goto end;
    }

    if (writepos >= playpos)
        writepos_t = cur + time_from_pos(This, writepos - playpos);
    else
        writepos_t = cur + time_from_pos(This, This->buf_size + writepos - playpos);

    /* write_at: Starting time of sample */
    /* cur: current time of play position */
    /* writepos_t: current time of our pointer play position */
    delta_t = write_at - writepos_t;
    if (delta_t >= -max_lag && delta_t <= max_lag) {
        TRACE("Continuing from old position\n");
        *ret_writepos = writepos;
    } else if (delta_t < 0) {
        REFERENCE_TIME past, min_writepos_t;
        WARN("Delta too big %i/%i, overwriting old data or even skipping\n", (int)delta_t / 10000, (int)max_lag / 10000);
        if (min_writepos >= playpos)
            min_writepos_t = cur + time_from_pos(This, min_writepos - playpos);
        else
            min_writepos_t = cur + time_from_pos(This, This->buf_size - playpos + min_writepos);
        past = min_writepos_t - write_at;
        if (past >= 0) {
            DWORD skipbytes = pos_from_time(This, past);
            WARN("Skipping %u bytes\n", skipbytes);
            *skip = skipbytes;
            *ret_writepos = min_writepos;
        } else {
            DWORD aheadbytes = pos_from_time(This, -past);
            WARN("Advancing %u bytes\n", aheadbytes);
            *ret_writepos = (min_writepos + aheadbytes) % This->buf_size;
        }
    } else /* delta_t > 0 */ {
        DWORD aheadbytes;
        WARN("Delta too big %i/%i, too far ahead\n", (int)delta_t / 10000, (int)max_lag / 10000);
        aheadbytes = pos_from_time(This, delta_t);
        WARN("Advancing %u bytes\n", aheadbytes);
        if (delta_t >= DSoundRenderer_Max_Fill)
            return S_FALSE;
        *ret_writepos = (min_writepos + aheadbytes) % This->buf_size;
    }
end:
    if (playpos > *ret_writepos)
        *pfree = playpos - *ret_writepos;
    else if (playpos == *ret_writepos)
        *pfree = This->buf_size - wfx->nBlockAlign;
    else
        *pfree = This->buf_size + playpos - *ret_writepos;
    if (time_from_pos(This, This->buf_size - *pfree) >= DSoundRenderer_Max_Fill) {
        TRACE("Blocked: too full %i / %i\n", (int)(time_from_pos(This, This->buf_size - *pfree)/10000), (int)(DSoundRenderer_Max_Fill / 10000));
        return S_FALSE;
    }
    return S_OK;
}

static HRESULT DSoundRender_HandleEndOfStream(DSoundRenderImpl *This)
{
    while (This->renderer.filter.state == State_Running)
    {
        DWORD pos1, pos2;
        DSoundRender_UpdatePositions(This, &pos1, &pos2);
        if (pos1 == pos2)
            break;

        This->in_loop = 1;
        LeaveCriticalSection(&This->renderer.filter.csFilter);
        LeaveCriticalSection(&This->renderer.csRenderLock);
        WaitForSingleObject(This->blocked, 10);
        EnterCriticalSection(&This->renderer.csRenderLock);
        EnterCriticalSection(&This->renderer.filter.csFilter);
        This->in_loop = 0;
    }

    return S_OK;
}

static HRESULT DSoundRender_SendSampleData(DSoundRenderImpl* This, REFERENCE_TIME tStart, REFERENCE_TIME tStop, const BYTE *data, DWORD size)
{
    HRESULT hr;

    while (size && This->renderer.filter.state != State_Stopped) {
        DWORD writepos, skip = 0, free, size1, size2, ret;
        BYTE *buf1, *buf2;

        if (This->renderer.filter.state == State_Running)
            hr = DSoundRender_GetWritePos(This, &writepos, tStart, &free, &skip);
        else
            hr = S_FALSE;

        if (hr != S_OK) {
            This->in_loop = 1;
            LeaveCriticalSection(&This->renderer.csRenderLock);
            ret = WaitForSingleObject(This->blocked, 10);
            EnterCriticalSection(&This->renderer.csRenderLock);
            This->in_loop = 0;
            if (This->renderer.pInputPin->flushing ||
                This->renderer.filter.state == State_Stopped) {
                return This->renderer.filter.state == State_Paused ? S_OK : VFW_E_WRONG_STATE;
            }
            if (ret != WAIT_TIMEOUT)
                ERR("%x\n", ret);
            continue;
        }
        tStart = -1;

        if (skip)
            FIXME("Sample dropped %u of %u bytes\n", skip, size);
        if (skip >= size)
            return S_OK;
        data += skip;
        size -= skip;

        hr = IDirectSoundBuffer_Lock(This->dsbuffer, writepos, min(free, size), (void**)&buf1, &size1, (void**)&buf2, &size2, 0);
        if (hr != DS_OK) {
            ERR("Unable to lock sound buffer! (%x)\n", hr);
            break;
        }
        memcpy(buf1, data, size1);
        if (size2)
            memcpy(buf2, data+size1, size2);
        IDirectSoundBuffer_Unlock(This->dsbuffer, buf1, size1, buf2, size2);
        This->writepos = (writepos + size1 + size2) % This->buf_size;
        TRACE("Wrote %u bytes at %u, next at %u - (%u/%u)\n", size1+size2, writepos, This->writepos, free, size);
        data += size1 + size2;
        size -= size1 + size2;
    }
    return S_OK;
}

static HRESULT WINAPI DSoundRender_ShouldDrawSampleNow(BaseRenderer *This, IMediaSample *pMediaSample, REFERENCE_TIME *pStartTime, REFERENCE_TIME *pEndTime)
{
    /* We time ourselves do not use the base renderers timing */
    return S_OK;
}


static HRESULT WINAPI DSoundRender_PrepareReceive(BaseRenderer *iface, IMediaSample *pSample)
{
    DSoundRenderImpl *This = impl_from_BaseRenderer(iface);
    HRESULT hr;
    AM_MEDIA_TYPE *amt;

    if (IMediaSample_GetMediaType(pSample, &amt) == S_OK)
    {
        AM_MEDIA_TYPE *orig = &This->renderer.pInputPin->pin.mtCurrent;
        WAVEFORMATEX *origfmt = (WAVEFORMATEX *)orig->pbFormat;
        WAVEFORMATEX *newfmt = (WAVEFORMATEX *)amt->pbFormat;

        if (origfmt->wFormatTag == newfmt->wFormatTag &&
            origfmt->nChannels == newfmt->nChannels &&
            origfmt->nBlockAlign == newfmt->nBlockAlign &&
            origfmt->wBitsPerSample == newfmt->wBitsPerSample &&
            origfmt->cbSize ==  newfmt->cbSize)
        {
            if (origfmt->nSamplesPerSec != newfmt->nSamplesPerSec)
            {
                hr = IDirectSoundBuffer_SetFrequency(This->dsbuffer,
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

static HRESULT WINAPI DSoundRender_DoRenderSample(BaseRenderer *iface, IMediaSample * pSample)
{
    DSoundRenderImpl *This = impl_from_BaseRenderer(iface);
    LPBYTE pbSrcStream = NULL;
    LONG cbSrcStream = 0;
    REFERENCE_TIME tStart, tStop;
    HRESULT hr;

    TRACE("%p %p\n", iface, pSample);

    /* Slightly incorrect, Pause completes when a frame is received so we should signal
     * pause completion here, but for sound playing a single frame doesn't make sense
     */

    hr = IMediaSample_GetPointer(pSample, &pbSrcStream);
    if (FAILED(hr))
    {
        ERR("Cannot get pointer to sample data (%x)\n", hr);
        return hr;
    }

    hr = IMediaSample_GetTime(pSample, &tStart, &tStop);
    if (FAILED(hr)) {
        ERR("Cannot get sample time (%x)\n", hr);
        tStart = tStop = -1;
    }

    IMediaSample_IsDiscontinuity(pSample);

    if (IMediaSample_IsPreroll(pSample) == S_OK)
    {
        TRACE("Preroll!\n");
        return S_OK;
    }

    cbSrcStream = IMediaSample_GetActualDataLength(pSample);
    TRACE("Sample data ptr = %p, size = %d\n", pbSrcStream, cbSrcStream);

    hr = DSoundRender_SendSampleData(This, tStart, tStop, pbSrcStream, cbSrcStream);
    if (This->renderer.filter.state == State_Running && This->renderer.filter.pClock && tStart >= 0) {
        REFERENCE_TIME jitter, now = 0;
        Quality q;
        IReferenceClock_GetTime(This->renderer.filter.pClock, &now);
        jitter = now - This->renderer.filter.rtStreamStart - tStart;
        if (jitter <= -DSoundRenderer_Max_Fill)
            jitter += DSoundRenderer_Max_Fill;
        else if (jitter < 0)
            jitter = 0;
        q.Type = (jitter > 0 ? Famine : Flood);
        q.Proportion = 1.;
        q.Late = jitter;
        q.TimeStamp = tStart;
        IQualityControl_Notify((IQualityControl *)This->renderer.qcimpl, (IBaseFilter*)This, q);
    }
    return hr;
}

static HRESULT WINAPI DSoundRender_CheckMediaType(BaseRenderer *iface, const AM_MEDIA_TYPE * pmt)
{
    WAVEFORMATEX* format;

    if (!IsEqualIID(&pmt->majortype, &MEDIATYPE_Audio))
        return S_FALSE;

    format =  (WAVEFORMATEX*)pmt->pbFormat;
    TRACE("Format = %p\n", format);
    TRACE("wFormatTag = %x %x\n", format->wFormatTag, WAVE_FORMAT_PCM);
    TRACE("nChannels = %d\n", format->nChannels);
    TRACE("nSamplesPerSec = %d\n", format->nAvgBytesPerSec);
    TRACE("nAvgBytesPerSec = %d\n", format->nAvgBytesPerSec);
    TRACE("nBlockAlign = %d\n", format->nBlockAlign);
    TRACE("wBitsPerSample = %d\n", format->wBitsPerSample);

    if (!IsEqualIID(&pmt->subtype, &MEDIASUBTYPE_PCM))
        return S_FALSE;

    return S_OK;
}

static VOID WINAPI DSoundRender_OnStopStreaming(BaseRenderer * iface)
{
    DSoundRenderImpl *This = impl_from_BaseRenderer(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    IDirectSoundBuffer_Stop(This->dsbuffer);
    This->writepos = This->buf_size;
    SetEvent(This->blocked);
}

static VOID WINAPI DSoundRender_OnStartStreaming(BaseRenderer * iface)
{
    DSoundRenderImpl *This = impl_from_BaseRenderer(iface);

    TRACE("(%p)\n", This);

    if (This->renderer.pInputPin->pin.pConnectedTo)
    {
        if (This->renderer.filter.state == State_Paused)
        {
            /* Unblock our thread, state changing from paused to running doesn't need a reset for state change */
            SetEvent(This->blocked);
        }
        IDirectSoundBuffer_Play(This->dsbuffer, 0, 0, DSBPLAY_LOOPING);
        ResetEvent(This->blocked);
    }
}

static HRESULT WINAPI DSoundRender_CompleteConnect(BaseRenderer * iface, IPin * pReceivePin)
{
    DSoundRenderImpl *This = impl_from_BaseRenderer(iface);
    const AM_MEDIA_TYPE * pmt = &This->renderer.pInputPin->pin.mtCurrent;
    HRESULT hr = S_OK;
    WAVEFORMATEX *format;
    DSBUFFERDESC buf_desc;

    TRACE("(%p)->(%p)\n", This, pReceivePin);
    dump_AM_MEDIA_TYPE(pmt);

    TRACE("MajorType %s\n", debugstr_guid(&pmt->majortype));
    TRACE("SubType %s\n", debugstr_guid(&pmt->subtype));
    TRACE("Format %s\n", debugstr_guid(&pmt->formattype));
    TRACE("Size %d\n", pmt->cbFormat);

    format = (WAVEFORMATEX*)pmt->pbFormat;

    This->buf_size = format->nAvgBytesPerSec;

    memset(&buf_desc,0,sizeof(DSBUFFERDESC));
    buf_desc.dwSize = sizeof(DSBUFFERDESC);
    buf_desc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN |
                       DSBCAPS_CTRLFREQUENCY | DSBCAPS_GLOBALFOCUS |
                       DSBCAPS_GETCURRENTPOSITION2;
    buf_desc.dwBufferBytes = This->buf_size;
    buf_desc.lpwfxFormat = format;
    hr = IDirectSound_CreateSoundBuffer(This->dsound, &buf_desc, &This->dsbuffer, NULL);
    This->writepos = This->buf_size;
    if (FAILED(hr))
        ERR("Can't create sound buffer (%x)\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IDirectSoundBuffer_SetVolume(This->dsbuffer, This->volume);
        if (FAILED(hr))
            ERR("Can't set volume to %d (%x)\n", This->volume, hr);

        hr = IDirectSoundBuffer_SetPan(This->dsbuffer, This->pan);
        if (FAILED(hr))
            ERR("Can't set pan to %d (%x)\n", This->pan, hr);
        hr = S_OK;
    }

    if (FAILED(hr) && hr != VFW_E_ALREADY_CONNECTED)
    {
        if (This->dsbuffer)
            IDirectSoundBuffer_Release(This->dsbuffer);
        This->dsbuffer = NULL;
    }

    return hr;
}

static HRESULT WINAPI DSoundRender_BreakConnect(BaseRenderer* iface)
{
    DSoundRenderImpl *This = impl_from_BaseRenderer(iface);

    TRACE("(%p)->()\n", iface);

    if (This->threadid) {
        PostThreadMessageW(This->threadid, WM_APP, 0, 0);
        LeaveCriticalSection(This->renderer.pInputPin->pin.pCritSec);
        WaitForSingleObject(This->advisethread, INFINITE);
        EnterCriticalSection(This->renderer.pInputPin->pin.pCritSec);
        CloseHandle(This->advisethread);
    }
    if (This->dsbuffer)
        IDirectSoundBuffer_Release(This->dsbuffer);
    This->dsbuffer = NULL;

    return S_OK;
}

static HRESULT WINAPI DSoundRender_EndOfStream(BaseRenderer* iface)
{
    DSoundRenderImpl *This = impl_from_BaseRenderer(iface);
    HRESULT hr;

    TRACE("(%p)->()\n",iface);

    hr = BaseRendererImpl_EndOfStream(iface);
    if (hr != S_OK)
    {
        ERR("%08x\n", hr);
        return hr;
    }

    hr = DSoundRender_HandleEndOfStream(This);

    return hr;
}

static HRESULT WINAPI DSoundRender_BeginFlush(BaseRenderer* iface)
{
    DSoundRenderImpl *This = impl_from_BaseRenderer(iface);

    TRACE("\n");
    BaseRendererImpl_BeginFlush(iface);
    SetEvent(This->blocked);

    return S_OK;
}

static HRESULT WINAPI DSoundRender_EndFlush(BaseRenderer* iface)
{
    DSoundRenderImpl *This = impl_from_BaseRenderer(iface);

    TRACE("\n");

    BaseRendererImpl_EndFlush(iface);
    if (This->renderer.filter.state != State_Stopped)
        ResetEvent(This->blocked);

    if (This->dsbuffer)
    {
        LPBYTE buffer;
        DWORD size;

        /* Force a reset */
        IDirectSoundBuffer_Lock(This->dsbuffer, 0, 0, (LPVOID *)&buffer, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER);
        memset(buffer, 0, size);
        IDirectSoundBuffer_Unlock(This->dsbuffer, buffer, size, NULL, 0);
        This->writepos = This->buf_size;
    }

    return S_OK;
}

static const BaseRendererFuncTable BaseFuncTable = {
    DSoundRender_CheckMediaType,
    DSoundRender_DoRenderSample,
    /**/
    NULL,
    NULL,
    NULL,
    DSoundRender_OnStartStreaming,
    DSoundRender_OnStopStreaming,
    NULL,
    NULL,
    NULL,
    DSoundRender_ShouldDrawSampleNow,
    DSoundRender_PrepareReceive,
    /**/
    DSoundRender_CompleteConnect,
    DSoundRender_BreakConnect,
    DSoundRender_EndOfStream,
    DSoundRender_BeginFlush,
    DSoundRender_EndFlush,
};

HRESULT DSoundRender_create(IUnknown * pUnkOuter, LPVOID * ppv)
{
    HRESULT hr;
    DSoundRenderImpl * pDSoundRender;

    TRACE("(%p, %p)\n", pUnkOuter, ppv);

    *ppv = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    pDSoundRender = CoTaskMemAlloc(sizeof(DSoundRenderImpl));
    if (!pDSoundRender)
        return E_OUTOFMEMORY;
    ZeroMemory(pDSoundRender, sizeof(DSoundRenderImpl));

    hr = BaseRenderer_Init(&pDSoundRender->renderer, &DSoundRender_Vtbl, (IUnknown*)pDSoundRender, &CLSID_DSoundRender, (DWORD_PTR)(__FILE__ ": DSoundRenderImpl.csFilter"), &BaseFuncTable);

    BasicAudio_Init(&pDSoundRender->basicAudio,&IBasicAudio_Vtbl);
    pDSoundRender->IReferenceClock_iface.lpVtbl = &IReferenceClock_Vtbl;
    pDSoundRender->IAMDirectSound_iface.lpVtbl = &IAMDirectSound_Vtbl;
    pDSoundRender->IAMFilterMiscFlags_iface.lpVtbl = &IAMFilterMiscFlags_Vtbl;

    if (SUCCEEDED(hr))
    {
        hr = DirectSoundCreate8(NULL, &pDSoundRender->dsound, NULL);
        if (FAILED(hr))
            ERR("Cannot create Direct Sound object (%x)\n", hr);
        else
            hr = IDirectSound_SetCooperativeLevel(pDSoundRender->dsound, GetDesktopWindow(), DSSCL_PRIORITY);
        if (SUCCEEDED(hr)) {
            IDirectSoundBuffer *buf;
            DSBUFFERDESC buf_desc;
            memset(&buf_desc,0,sizeof(DSBUFFERDESC));
            buf_desc.dwSize = sizeof(DSBUFFERDESC);
            buf_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
            hr = IDirectSound_CreateSoundBuffer(pDSoundRender->dsound, &buf_desc, &buf, NULL);
            if (SUCCEEDED(hr)) {
                IDirectSoundBuffer_Play(buf, 0, 0, DSBPLAY_LOOPING);
                IDirectSoundBuffer_Release(buf);
            }
            hr = S_OK;
        }
    }

    if (SUCCEEDED(hr))
    {
        pDSoundRender->blocked = CreateEventW(NULL, TRUE, TRUE, NULL);

        if (!pDSoundRender->blocked || FAILED(hr))
        {
            IUnknown_Release((IUnknown *)pDSoundRender);
            return HRESULT_FROM_WIN32(GetLastError());
        }

        *ppv = pDSoundRender;
    }
    else
    {
        BaseRendererImpl_Release(&pDSoundRender->renderer.filter.IBaseFilter_iface);
        CoTaskMemFree(pDSoundRender);
    }

    return hr;
}

static HRESULT WINAPI DSoundRender_QueryInterface(IBaseFilter * iface, REFIID riid, LPVOID * ppv)
{
    DSoundRenderImpl *This = impl_from_IBaseFilter(iface);
    TRACE("(%p, %p)->(%s, %p)\n", This, iface, qzdebugstr_guid(riid), ppv);

    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IBasicAudio))
        *ppv = &This->basicAudio.IBasicAudio_iface;
    else if (IsEqualIID(riid, &IID_IReferenceClock))
        *ppv = &This->IReferenceClock_iface;
    else if (IsEqualIID(riid, &IID_IAMDirectSound))
        *ppv = &This->IAMDirectSound_iface;
    else if (IsEqualIID(riid, &IID_IAMFilterMiscFlags))
        *ppv = &This->IAMFilterMiscFlags_iface;
    else
    {
        HRESULT hr;
        hr = BaseRendererImpl_QueryInterface(iface, riid, ppv);
        if (SUCCEEDED(hr))
            return hr;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown *)(*ppv));
        return S_OK;
    }

    if (!IsEqualIID(riid, &IID_IPin) && !IsEqualIID(riid, &IID_IVideoWindow))
        FIXME("No interface for %s!\n", qzdebugstr_guid(riid));

    return E_NOINTERFACE;
}

static ULONG WINAPI DSoundRender_Release(IBaseFilter * iface)
{
    DSoundRenderImpl *This = impl_from_IBaseFilter(iface);
    ULONG refCount = BaseRendererImpl_Release(iface);

    TRACE("(%p)->() Release from %d\n", This, refCount + 1);

    if (!refCount)
    {
        if (This->threadid) {
            PostThreadMessageW(This->threadid, WM_APP, 0, 0);
            WaitForSingleObject(This->advisethread, INFINITE);
            CloseHandle(This->advisethread);
        }

        if (This->dsbuffer)
            IDirectSoundBuffer_Release(This->dsbuffer);
        This->dsbuffer = NULL;
        if (This->dsound)
            IDirectSound_Release(This->dsound);
        This->dsound = NULL;

        BasicAudio_Destroy(&This->basicAudio);
        CloseHandle(This->blocked);

        TRACE("Destroying Audio Renderer\n");
        CoTaskMemFree(This);

        return 0;
    }
    else
        return refCount;
}

static HRESULT WINAPI DSoundRender_Pause(IBaseFilter * iface)
{
    HRESULT hr = S_OK;
    DSoundRenderImpl *This = (DSoundRenderImpl *)iface;

    TRACE("(%p/%p)->()\n", This, iface);

    EnterCriticalSection(&This->renderer.csRenderLock);
    if (This->renderer.filter.state != State_Paused)
    {
        if (This->renderer.filter.state == State_Stopped)
        {
            if (This->renderer.pInputPin->pin.pConnectedTo)
                ResetEvent(This->renderer.evComplete);
            This->renderer.pInputPin->end_of_stream = 0;
        }

        hr = IDirectSoundBuffer_Stop(This->dsbuffer);
        if (SUCCEEDED(hr))
            This->renderer.filter.state = State_Paused;

        ResetEvent(This->blocked);
        ResetEvent(This->renderer.RenderEvent);
    }
    ResetEvent(This->renderer.ThreadSignal);
    LeaveCriticalSection(&This->renderer.csRenderLock);

    return hr;
}

static const IBaseFilterVtbl DSoundRender_Vtbl =
{
    DSoundRender_QueryInterface,
    BaseFilterImpl_AddRef,
    DSoundRender_Release,
    BaseFilterImpl_GetClassID,
    BaseRendererImpl_Stop,
    DSoundRender_Pause,
    BaseRendererImpl_Run,
    BaseRendererImpl_GetState,
    BaseRendererImpl_SetSyncSource,
    BaseFilterImpl_GetSyncSource,
    BaseFilterImpl_EnumPins,
    BaseRendererImpl_FindPin,
    BaseFilterImpl_QueryFilterInfo,
    BaseFilterImpl_JoinFilterGraph,
    BaseFilterImpl_QueryVendorInfo
};

/*** IUnknown methods ***/
static HRESULT WINAPI Basicaudio_QueryInterface(IBasicAudio *iface,
						REFIID riid,
						LPVOID*ppvObj) {
    DSoundRenderImpl *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return DSoundRender_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppvObj);
}

static ULONG WINAPI Basicaudio_AddRef(IBasicAudio *iface) {
    DSoundRenderImpl *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return BaseFilterImpl_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI Basicaudio_Release(IBasicAudio *iface) {
    DSoundRenderImpl *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return DSoundRender_Release(&This->renderer.filter.IBaseFilter_iface);
}

/*** IBasicAudio methods ***/
static HRESULT WINAPI Basicaudio_put_Volume(IBasicAudio *iface,
                                            LONG lVolume) {
    DSoundRenderImpl *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, lVolume);

    if (lVolume > DSBVOLUME_MAX || lVolume < DSBVOLUME_MIN)
        return E_INVALIDARG;

    if (This->dsbuffer) {
        if (FAILED(IDirectSoundBuffer_SetVolume(This->dsbuffer, lVolume)))
            return E_FAIL;
    }

    This->volume = lVolume;
    return S_OK;
}

static HRESULT WINAPI Basicaudio_get_Volume(IBasicAudio *iface,
                                            LONG *plVolume) {
    DSoundRenderImpl *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, plVolume);

    if (!plVolume)
        return E_POINTER;

    *plVolume = This->volume;
    return S_OK;
}

static HRESULT WINAPI Basicaudio_put_Balance(IBasicAudio *iface,
                                             LONG lBalance) {
    DSoundRenderImpl *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->(%d)\n", This, iface, lBalance);

    if (lBalance < DSBPAN_LEFT || lBalance > DSBPAN_RIGHT)
        return E_INVALIDARG;

    if (This->dsbuffer) {
        if (FAILED(IDirectSoundBuffer_SetPan(This->dsbuffer, lBalance)))
            return E_FAIL;
    }

    This->pan = lBalance;
    return S_OK;
}

static HRESULT WINAPI Basicaudio_get_Balance(IBasicAudio *iface,
                                             LONG *plBalance) {
    DSoundRenderImpl *This = impl_from_IBasicAudio(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, plBalance);

    if (!plBalance)
        return E_POINTER;

    *plBalance = This->pan;
    return S_OK;
}

static const IBasicAudioVtbl IBasicAudio_Vtbl =
{
    Basicaudio_QueryInterface,
    Basicaudio_AddRef,
    Basicaudio_Release,
    BasicAudioImpl_GetTypeInfoCount,
    BasicAudioImpl_GetTypeInfo,
    BasicAudioImpl_GetIDsOfNames,
    BasicAudioImpl_Invoke,
    Basicaudio_put_Volume,
    Basicaudio_get_Volume,
    Basicaudio_put_Balance,
    Basicaudio_get_Balance
};

struct dsoundrender_timer {
    struct dsoundrender_timer *next;
    REFERENCE_TIME start;
    REFERENCE_TIME periodicity;
    HANDLE handle;
    DWORD cookie;
};
static LONG cookie_counter = 1;

static DWORD WINAPI DSoundAdviseThread(LPVOID lpParam) {
    DSoundRenderImpl *This = lpParam;
    struct dsoundrender_timer head = {NULL};
    MSG msg;

    TRACE("(%p): Main Loop\n", This);

    PeekMessageW(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);
    SetEvent(This->thread_wait);

    while (1)
    {
        HRESULT hr;
        REFERENCE_TIME curtime = 0;
        BOOL ret;
        struct dsoundrender_timer *prev = &head, *cur;

        hr = IReferenceClock_GetTime(&This->IReferenceClock_iface, &curtime);
        if (SUCCEEDED(hr)) {
            TRACE("Time: %s\n", wine_dbgstr_longlong(curtime));
            while (prev->next) {
                cur = prev->next;
                if (cur->start > curtime) {
                    TRACE("Skipping %p\n", cur);
                    prev = cur;
                } else if (cur->periodicity) {
                    while (cur->start <= curtime) {
                        cur->start += cur->periodicity;
                        ReleaseSemaphore(cur->handle, 1, NULL);
                    }
                    prev = cur;
                } else {
                    struct dsoundrender_timer *next = cur->next;
                    TRACE("Firing %p %s < %s\n", cur, wine_dbgstr_longlong(cur->start), wine_dbgstr_longlong(curtime));
                    SetEvent(cur->handle);
                    HeapFree(GetProcessHeap(), 0, cur);
                    prev->next = next;
                }
            }
        }
        if (!head.next)
            ret = GetMessageW(&msg, INVALID_HANDLE_VALUE, WM_APP, WM_APP + 4);
        else
            ret = PeekMessageW(&msg, INVALID_HANDLE_VALUE, WM_APP, WM_APP + 4, PM_REMOVE);
        while (ret) {
            switch (LOWORD(msg.message) - WM_APP) {
                case 0: TRACE("Exiting\n"); return 0;
                case 1:
                case 2: {
                    struct dsoundrender_timer *t = (struct dsoundrender_timer *)msg.wParam;
                    if (LOWORD(msg.message) - WM_APP == 1)
                        TRACE("Adding one-shot timer %p\n", t);
                    else
                        TRACE("Adding periodic timer %p\n", t);
                    t->next = head.next;
                    head.next = t;
                    break;
                }
                case 3:
                    prev = &head;
                    while (prev->next) {
                        cur = prev->next;
                        if (cur->cookie == msg.wParam) {
                            struct dsoundrender_timer *next = cur->next;
                            HeapFree(GetProcessHeap(), 0, cur);
                            prev->next = next;
                            break;
                        }
                        prev = cur;
                    }
                    break;
            }
            ret = PeekMessageW(&msg, INVALID_HANDLE_VALUE, WM_APP, WM_APP + 4, PM_REMOVE);
        }
        MsgWaitForMultipleObjects(0, NULL, 5, QS_POSTMESSAGE, 0);
    }
    return 0;
}

/*** IUnknown methods ***/
static HRESULT WINAPI ReferenceClock_QueryInterface(IReferenceClock *iface,
						REFIID riid,
						LPVOID*ppvObj)
{
    DSoundRenderImpl *This = impl_from_IReferenceClock(iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return DSoundRender_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppvObj);
}

static ULONG WINAPI ReferenceClock_AddRef(IReferenceClock *iface)
{
    DSoundRenderImpl *This = impl_from_IReferenceClock(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return BaseFilterImpl_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI ReferenceClock_Release(IReferenceClock *iface)
{
    DSoundRenderImpl *This = impl_from_IReferenceClock(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return DSoundRender_Release(&This->renderer.filter.IBaseFilter_iface);
}

/*** IReferenceClock methods ***/
static HRESULT WINAPI ReferenceClock_GetTime(IReferenceClock *iface,
                                             REFERENCE_TIME *pTime)
{
    DSoundRenderImpl *This = impl_from_IReferenceClock(iface);
    HRESULT hr = E_FAIL;

    TRACE("(%p/%p)->(%p)\n", This, iface, pTime);
    if (!pTime)
        return E_POINTER;

    if (This->dsbuffer) {
        DWORD writepos1, writepos2;
        EnterCriticalSection(&This->renderer.filter.csFilter);
        DSoundRender_UpdatePositions(This, &writepos1, &writepos2);
        if (This->renderer.pInputPin && This->renderer.pInputPin->pin.mtCurrent.pbFormat)
        {
            *pTime = This->play_time + time_from_pos(This, This->last_playpos);
            hr = S_OK;
        }
        else
        {
            ERR("pInputPin Disconncted\n");
            hr = E_FAIL;
        }
        LeaveCriticalSection(&This->renderer.filter.csFilter);
    }
    if (FAILED(hr))
        WARN("Could not get reference time (%x)!\n", hr);

    return hr;
}

static HRESULT WINAPI ReferenceClock_AdviseTime(IReferenceClock *iface,
                                                REFERENCE_TIME rtBaseTime,
                                                REFERENCE_TIME rtStreamTime,
                                                HEVENT hEvent,
                                                DWORD_PTR *pdwAdviseCookie)
{
    DSoundRenderImpl *This = impl_from_IReferenceClock(iface);
    REFERENCE_TIME when = rtBaseTime + rtStreamTime;
    REFERENCE_TIME future;
    TRACE("(%p/%p)->(%s, %s, %p, %p)\n", This, iface, wine_dbgstr_longlong(rtBaseTime), wine_dbgstr_longlong(rtStreamTime), (void*)hEvent, pdwAdviseCookie);

    if (when <= 0)
        return E_INVALIDARG;

    if (!pdwAdviseCookie)
        return E_POINTER;

    EnterCriticalSection(&This->renderer.filter.csFilter);
    future = when - This->play_time;
    if (!This->threadid && This->dsbuffer) {
        This->thread_wait = CreateEventW(0, 0, 0, 0);
        This->advisethread = CreateThread(NULL, 0, DSoundAdviseThread, This, 0, &This->threadid);
        WaitForSingleObject(This->thread_wait, INFINITE);
        CloseHandle(This->thread_wait);
    }
    LeaveCriticalSection(&This->renderer.filter.csFilter);
    /* If it's in the past or the next millisecond, trigger immediately  */
    if (future <= 10000) {
        SetEvent((HANDLE)hEvent);
        *pdwAdviseCookie = 0;
    } else {
        struct dsoundrender_timer *t = HeapAlloc(GetProcessHeap(), 0, sizeof(*t));
        t->next = NULL;
        t->start = when;
        t->periodicity = 0;
        t->handle = (HANDLE)hEvent;
        t->cookie = InterlockedIncrement(&cookie_counter);
        PostThreadMessageW(This->threadid, WM_APP+1, (WPARAM)t, 0);
        *pdwAdviseCookie = t->cookie;
    }

    return S_OK;
}

static HRESULT WINAPI ReferenceClock_AdvisePeriodic(IReferenceClock *iface,
                                                    REFERENCE_TIME rtStartTime,
                                                    REFERENCE_TIME rtPeriodTime,
                                                    HSEMAPHORE hSemaphore,
                                                    DWORD_PTR *pdwAdviseCookie)
{
    DSoundRenderImpl *This = impl_from_IReferenceClock(iface);
    struct dsoundrender_timer *t;

    TRACE("(%p/%p)->(%s, %s, %p, %p)\n", This, iface, wine_dbgstr_longlong(rtStartTime), wine_dbgstr_longlong(rtPeriodTime), (void*)hSemaphore, pdwAdviseCookie);

    if (rtStartTime <= 0 || rtPeriodTime <= 0)
        return E_INVALIDARG;

    if (!pdwAdviseCookie)
        return E_POINTER;

    EnterCriticalSection(&This->renderer.filter.csFilter);
    if (!This->threadid && This->dsbuffer) {
        This->thread_wait = CreateEventW(0, 0, 0, 0);
        This->advisethread = CreateThread(NULL, 0, DSoundAdviseThread, This, 0, &This->threadid);
        WaitForSingleObject(This->thread_wait, INFINITE);
        CloseHandle(This->thread_wait);
    }
    LeaveCriticalSection(&This->renderer.filter.csFilter);

    t = HeapAlloc(GetProcessHeap(), 0, sizeof(*t));
    t->next = NULL;
    t->start = rtStartTime;
    t->periodicity = rtPeriodTime;
    t->handle = (HANDLE)hSemaphore;
    t->cookie = InterlockedIncrement(&cookie_counter);
    PostThreadMessageW(This->threadid, WM_APP+1, (WPARAM)t, 0);
    *pdwAdviseCookie = t->cookie;

    return S_OK;
}

static HRESULT WINAPI ReferenceClock_Unadvise(IReferenceClock *iface,
                                              DWORD_PTR dwAdviseCookie)
{
    DSoundRenderImpl *This = impl_from_IReferenceClock(iface);

    TRACE("(%p/%p)->(%p)\n", This, iface, (void*)dwAdviseCookie);
    if (!This->advisethread || !dwAdviseCookie)
        return S_FALSE;
    PostThreadMessageW(This->threadid, WM_APP+3, dwAdviseCookie, 0);
    return S_OK;
}

static const IReferenceClockVtbl IReferenceClock_Vtbl =
{
    ReferenceClock_QueryInterface,
    ReferenceClock_AddRef,
    ReferenceClock_Release,
    ReferenceClock_GetTime,
    ReferenceClock_AdviseTime,
    ReferenceClock_AdvisePeriodic,
    ReferenceClock_Unadvise
};

/*** IUnknown methods ***/
static HRESULT WINAPI AMDirectSound_QueryInterface(IAMDirectSound *iface,
						REFIID riid,
						LPVOID*ppvObj)
{
    DSoundRenderImpl *This = impl_from_IAMDirectSound(iface);

    TRACE("(%p/%p)->(%s (%p), %p)\n", This, iface, debugstr_guid(riid), riid, ppvObj);

    return DSoundRender_QueryInterface(&This->renderer.filter.IBaseFilter_iface, riid, ppvObj);
}

static ULONG WINAPI AMDirectSound_AddRef(IAMDirectSound *iface)
{
    DSoundRenderImpl *This = impl_from_IAMDirectSound(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return BaseFilterImpl_AddRef(&This->renderer.filter.IBaseFilter_iface);
}

static ULONG WINAPI AMDirectSound_Release(IAMDirectSound *iface)
{
    DSoundRenderImpl *This = impl_from_IAMDirectSound(iface);

    TRACE("(%p/%p)->()\n", This, iface);

    return DSoundRender_Release(&This->renderer.filter.IBaseFilter_iface);
}

/*** IAMDirectSound methods ***/
static HRESULT WINAPI AMDirectSound_GetDirectSoundInterface(IAMDirectSound *iface,  IDirectSound **ds)
{
    DSoundRenderImpl *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, ds);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_GetPrimaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer **buf)
{
    DSoundRenderImpl *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_GetSecondaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer **buf)
{
    DSoundRenderImpl *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_ReleaseDirectSoundInterface(IAMDirectSound *iface, IDirectSound *ds)
{
    DSoundRenderImpl *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, ds);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_ReleasePrimaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer *buf)
{
    DSoundRenderImpl *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_ReleaseSecondaryBufferInterface(IAMDirectSound *iface, IDirectSoundBuffer *buf)
{
    DSoundRenderImpl *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p): stub\n", This, iface, buf);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_SetFocusWindow(IAMDirectSound *iface, HWND hwnd, BOOL bgaudible)
{
    DSoundRenderImpl *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p,%d): stub\n", This, iface, hwnd, bgaudible);

    return E_NOTIMPL;
}

static HRESULT WINAPI AMDirectSound_GetFocusWindow(IAMDirectSound *iface, HWND *hwnd, BOOL *bgaudible)
{
    DSoundRenderImpl *This = impl_from_IAMDirectSound(iface);

    FIXME("(%p/%p)->(%p,%p): stub\n", This, iface, hwnd, bgaudible);

    return E_NOTIMPL;
}

static const IAMDirectSoundVtbl IAMDirectSound_Vtbl =
{
    AMDirectSound_QueryInterface,
    AMDirectSound_AddRef,
    AMDirectSound_Release,
    AMDirectSound_GetDirectSoundInterface,
    AMDirectSound_GetPrimaryBufferInterface,
    AMDirectSound_GetSecondaryBufferInterface,
    AMDirectSound_ReleaseDirectSoundInterface,
    AMDirectSound_ReleasePrimaryBufferInterface,
    AMDirectSound_ReleaseSecondaryBufferInterface,
    AMDirectSound_SetFocusWindow,
    AMDirectSound_GetFocusWindow
};

static HRESULT WINAPI AMFilterMiscFlags_QueryInterface(IAMFilterMiscFlags *iface, REFIID riid, void **ppv) {
    DSoundRenderImpl *This = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_QueryInterface((IUnknown*)This, riid, ppv);
}

static ULONG WINAPI AMFilterMiscFlags_AddRef(IAMFilterMiscFlags *iface) {
    DSoundRenderImpl *This = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_AddRef((IUnknown*)This);
}

static ULONG WINAPI AMFilterMiscFlags_Release(IAMFilterMiscFlags *iface) {
    DSoundRenderImpl *This = impl_from_IAMFilterMiscFlags(iface);
    return IUnknown_Release((IUnknown*)This);
}

static ULONG WINAPI AMFilterMiscFlags_GetMiscFlags(IAMFilterMiscFlags *iface) {
    return AM_FILTER_MISC_FLAGS_IS_RENDERER;
}

static const IAMFilterMiscFlagsVtbl IAMFilterMiscFlags_Vtbl = {
    AMFilterMiscFlags_QueryInterface,
    AMFilterMiscFlags_AddRef,
    AMFilterMiscFlags_Release,
    AMFilterMiscFlags_GetMiscFlags
};
