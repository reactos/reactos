/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
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
 *
 * TODO:
 *      When PrimarySetFormat (via ReopenDevice or PrimaryOpen) fails,
 *       it leaves dsound in unusable (not really open) state.
 */

#include <stdarg.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

static DWORD speaker_config_to_channel_mask(DWORD speaker_config)
{
    switch (DSSPEAKER_CONFIG(speaker_config)) {
        case DSSPEAKER_MONO:
            return SPEAKER_FRONT_LEFT;

        case DSSPEAKER_STEREO:
        case DSSPEAKER_HEADPHONE:
            return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;

        case DSSPEAKER_QUAD:
            return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;

        case DSSPEAKER_5POINT1_BACK:
            return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT;
    }

    WARN("unknown speaker_config %u\n", speaker_config);
    return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
}

static DWORD DSOUND_FindSpeakerConfig(IMMDevice *mmdevice, int channels)
{
    IPropertyStore *store;
    HRESULT hr;
    PROPVARIANT pv;
    ULONG phys_speakers;

    const DWORD def = DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE);

    hr = IMMDevice_OpenPropertyStore(mmdevice, STGM_READ, &store);
    if (FAILED(hr)) {
        WARN("IMMDevice_OpenPropertyStore failed: %08x\n", hr);
        return def;
    }

    hr = IPropertyStore_GetValue(store, &PKEY_AudioEndpoint_PhysicalSpeakers, &pv);

    if (FAILED(hr)) {
        WARN("IPropertyStore_GetValue failed: %08x\n", hr);
        IPropertyStore_Release(store);
        return def;
    }

    if (pv.vt != VT_UI4) {
        WARN("PKEY_AudioEndpoint_PhysicalSpeakers is not a ULONG: 0x%x\n", pv.vt);
        PropVariantClear(&pv);
        IPropertyStore_Release(store);
        return def;
    }

    phys_speakers = pv.u.ulVal;

    PropVariantClear(&pv);
    IPropertyStore_Release(store);

    if ((channels >= 6 || channels == 0) && (phys_speakers & KSAUDIO_SPEAKER_5POINT1) == KSAUDIO_SPEAKER_5POINT1)
        return DSSPEAKER_5POINT1_BACK;
    else if ((channels >= 6 || channels == 0) && (phys_speakers & KSAUDIO_SPEAKER_5POINT1_SURROUND) == KSAUDIO_SPEAKER_5POINT1_SURROUND)
        return DSSPEAKER_5POINT1_SURROUND;
    else if ((channels >= 4 || channels == 0) && (phys_speakers & KSAUDIO_SPEAKER_QUAD) == KSAUDIO_SPEAKER_QUAD)
        return DSSPEAKER_QUAD;
    else if ((channels >= 2 || channels == 0) && (phys_speakers & KSAUDIO_SPEAKER_STEREO) == KSAUDIO_SPEAKER_STEREO)
        return DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE);
    else if ((phys_speakers & KSAUDIO_SPEAKER_MONO) == KSAUDIO_SPEAKER_MONO)
        return DSSPEAKER_MONO;

    return def;
}

static HRESULT DSOUND_WaveFormat(DirectSoundDevice *device, IAudioClient *client,
				 BOOL forcewave, WAVEFORMATEX **wfx)
{
    WAVEFORMATEXTENSIBLE *retwfe = NULL;
    WAVEFORMATEX *w;
    HRESULT hr;

    if (!forcewave) {
        WAVEFORMATEXTENSIBLE *mixwfe;
        hr = IAudioClient_GetMixFormat(client, (WAVEFORMATEX**)&mixwfe);

        if (FAILED(hr))
            return hr;

        if (mixwfe->Format.nChannels < device->num_speakers) {
            device->speaker_config = DSOUND_FindSpeakerConfig(device->mmdevice, mixwfe->Format.nChannels);
            DSOUND_ParseSpeakerConfig(device);
        } else if (mixwfe->Format.nChannels > device->num_speakers) {
            mixwfe->Format.nChannels = device->num_speakers;
            mixwfe->Format.nBlockAlign = mixwfe->Format.nChannels * mixwfe->Format.wBitsPerSample / 8;
            mixwfe->Format.nAvgBytesPerSec = mixwfe->Format.nSamplesPerSec * mixwfe->Format.nBlockAlign;
            mixwfe->dwChannelMask = speaker_config_to_channel_mask(device->speaker_config);
        }

        if (!IsEqualGUID(&mixwfe->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) {
            WAVEFORMATEXTENSIBLE testwfe = *mixwfe;

            testwfe.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
            testwfe.Samples.wValidBitsPerSample = testwfe.Format.wBitsPerSample = 32;
            testwfe.Format.nBlockAlign = testwfe.Format.nChannels * testwfe.Format.wBitsPerSample / 8;
            testwfe.Format.nAvgBytesPerSec = testwfe.Format.nSamplesPerSec * testwfe.Format.nBlockAlign;

            if (FAILED(IAudioClient_IsFormatSupported(client, AUDCLNT_SHAREMODE_SHARED, &testwfe.Format, (WAVEFORMATEX**)&retwfe)))
                w = DSOUND_CopyFormat(&mixwfe->Format);
            else if (retwfe)
                w = DSOUND_CopyFormat(&retwfe->Format);
            else
                w = DSOUND_CopyFormat(&testwfe.Format);
            CoTaskMemFree(retwfe);
            retwfe = NULL;
        } else
            w = DSOUND_CopyFormat(&mixwfe->Format);
        CoTaskMemFree(mixwfe);
    } else if (device->primary_pwfx->wFormatTag == WAVE_FORMAT_PCM ||
               device->primary_pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
        WAVEFORMATEX *wi = device->primary_pwfx;
        WAVEFORMATEXTENSIBLE *wfe;

        /* Convert to WAVEFORMATEXTENSIBLE */
        w = HeapAlloc(GetProcessHeap(), 0, sizeof(WAVEFORMATEXTENSIBLE));
        wfe = (WAVEFORMATEXTENSIBLE*)w;
        if (!wfe)
            return DSERR_OUTOFMEMORY;

        wfe->Format = *wi;
        w->wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        w->cbSize = sizeof(*wfe) - sizeof(*w);
        w->nBlockAlign = w->nChannels * w->wBitsPerSample / 8;
        w->nAvgBytesPerSec = w->nSamplesPerSec * w->nBlockAlign;

        wfe->dwChannelMask = 0;
        if (wi->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) {
            w->wBitsPerSample = 32;
            wfe->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        } else
            wfe->SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
        wfe->Samples.wValidBitsPerSample = w->wBitsPerSample;
    } else
        w = DSOUND_CopyFormat(device->primary_pwfx);

    if (!w)
        return DSERR_OUTOFMEMORY;

    hr = IAudioClient_IsFormatSupported(client, AUDCLNT_SHAREMODE_SHARED, w, (WAVEFORMATEX**)&retwfe);
    if (retwfe) {
        memcpy(w, retwfe, sizeof(WAVEFORMATEX) + retwfe->Format.cbSize);
        CoTaskMemFree(retwfe);
    }
    if (FAILED(hr)) {
        WARN("IsFormatSupported failed: %08x\n", hr);
        HeapFree(GetProcessHeap(), 0, w);
        return hr;
    }
    *wfx = w;
    return S_OK;
}

static void DSOUND_ReleaseDevice(DirectSoundDevice *device)
{
    if(device->client){
        IAudioClient_Release(device->client);
        device->client = NULL;
    }
    if(device->render){
        IAudioRenderClient_Release(device->render);
        device->render = NULL;
    }
    if(device->volume){
        IAudioStreamVolume_Release(device->volume);
        device->volume = NULL;
    }

    if (device->pad) {
        device->playpos += device->pad;
        device->playpos %= device->buflen;
        device->pad = 0;
    }
}

static HRESULT DSOUND_PrimaryOpen(DirectSoundDevice *device, WAVEFORMATEX *wfx, DWORD frames, BOOL forcewave)
{
    IDirectSoundBufferImpl** dsb = device->buffers;
    LPBYTE newbuf;
    DWORD new_buflen;
    BOOL mixfloat = FALSE;
    int i;

    TRACE("(%p)\n", device);

    new_buflen = device->buflen;
    new_buflen -= new_buflen % wfx->nBlockAlign;

    if (wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
        (wfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
         IsEqualGUID(&((WAVEFORMATEXTENSIBLE*)wfx)->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)))
        mixfloat = TRUE;

    /* reallocate emulated primary buffer */
    if (forcewave || !mixfloat) {
        if (!forcewave)
            new_buflen = frames * wfx->nChannels * sizeof(float);

        if (device->buffer)
            newbuf = HeapReAlloc(GetProcessHeap(), 0, device->buffer, new_buflen);
        else
            newbuf = HeapAlloc(GetProcessHeap(), 0, new_buflen);

        if (!newbuf) {
            ERR("failed to allocate primary buffer\n");
            return DSERR_OUTOFMEMORY;
        }
        FillMemory(newbuf, new_buflen, (wfx->wBitsPerSample == 8) ? 128 : 0);
    } else {
        HeapFree(GetProcessHeap(), 0, device->buffer);
        newbuf = NULL;
    }

    device->buffer = newbuf;
    device->buflen = new_buflen;
    HeapFree(GetProcessHeap(), 0, device->pwfx);
    device->pwfx = wfx;

    device->writelead = (wfx->nSamplesPerSec / 100) * wfx->nBlockAlign;

    TRACE("buflen: %u, frames %u\n", device->buflen, frames);

    if (!mixfloat)
        device->normfunction = normfunctions[wfx->wBitsPerSample/8 - 1];
    else
        device->normfunction = NULL;

    device->playpos = 0;

    for (i = 0; i < device->nrofbuffers; i++) {
        RtlAcquireResourceExclusive(&dsb[i]->lock, TRUE);
        DSOUND_RecalcFormat(dsb[i]);
        RtlReleaseResource(&dsb[i]->lock);
    }

    return DS_OK;
}

HRESULT DSOUND_ReopenDevice(DirectSoundDevice *device, BOOL forcewave)
{
    HRESULT hres;
    REFERENCE_TIME period;
    UINT32 acbuf_frames, aclen_frames;
    DWORD period_ms;
    IAudioClient *client = NULL;
    IAudioRenderClient *render = NULL;
    IAudioStreamVolume *volume = NULL;
    DWORD frag_frames;
    WAVEFORMATEX *wfx = NULL;
    DWORD oldspeakerconfig = device->speaker_config;

    TRACE("(%p, %d)\n", device, forcewave);

    hres = IMMDevice_Activate(device->mmdevice, &IID_IAudioClient,
            CLSCTX_INPROC_SERVER, NULL, (void **)&client);
    if(FAILED(hres)){
        WARN("Activate failed: %08x\n", hres);
        return hres;
    }

    hres = DSOUND_WaveFormat(device, client, forcewave, &wfx);
    if (FAILED(hres)) {
        IAudioClient_Release(client);
        return hres;
    }

    hres = IAudioClient_Initialize(client,
            AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_NOPERSIST |
            AUDCLNT_STREAMFLAGS_EVENTCALLBACK, 800000, 0, wfx, NULL);
    if(FAILED(hres)){
        IAudioClient_Release(client);
        ERR("Initialize failed: %08x\n", hres);
        return hres;
    }

    IAudioClient_SetEventHandle(client, device->sleepev);

    hres = IAudioClient_GetService(client, &IID_IAudioRenderClient, (void**)&render);
    if(FAILED(hres))
        goto err_service;

    hres = IAudioClient_GetService(client, &IID_IAudioStreamVolume, (void**)&volume);
    if(FAILED(hres))
        goto err_service;

    /* Now kick off the timer so the event fires periodically */
    hres = IAudioClient_Start(client);
    if (FAILED(hres)) {
        WARN("Start failed with %08x\n", hres);
        goto err;
    }
    hres = IAudioClient_GetStreamLatency(client, &period);
    if (FAILED(hres)) {
        WARN("GetStreamLatency failed with %08x\n", hres);
        goto err;
    }
    hres = IAudioClient_GetBufferSize(client, &acbuf_frames);
    if (FAILED(hres)) {
        WARN("GetBufferSize failed with %08x\n", hres);
        goto err;
    }

    period_ms = (period + 9999) / 10000;
    frag_frames = MulDiv(wfx->nSamplesPerSec, period, 10000000);

    aclen_frames = min(acbuf_frames, 3 * frag_frames);

    TRACE("period %u ms frag_frames %u buf_frames %u\n", period_ms, frag_frames, aclen_frames);

    hres = DSOUND_PrimaryOpen(device, wfx, aclen_frames, forcewave);
    if(FAILED(hres))
        goto err;

    DSOUND_ReleaseDevice(device);
    device->client = client;
    device->render = render;
    device->volume = volume;
    device->frag_frames = frag_frames;
    device->ac_frames = aclen_frames;

    if (period_ms < 3)
        device->sleeptime = 5;
    else
        device->sleeptime = period_ms * 5 / 2;

    return S_OK;

err_service:
    WARN("GetService failed: %08x\n", hres);
err:
    device->speaker_config = oldspeakerconfig;
    DSOUND_ParseSpeakerConfig(device);
    if (volume)
        IAudioStreamVolume_Release(volume);
    if (render)
        IAudioRenderClient_Release(render);
    IAudioClient_Release(client);
    HeapFree(GetProcessHeap(), 0, wfx);
    return hres;
}

HRESULT DSOUND_PrimaryDestroy(DirectSoundDevice *device)
{
	TRACE("(%p)\n", device);

	/* **** */
	EnterCriticalSection(&(device->mixlock));

	if(device->primary && (device->primary->ref || device->primary->numIfaces))
		WARN("Destroying primary buffer while references held (%u %u)\n", device->primary->ref, device->primary->numIfaces);

	HeapFree(GetProcessHeap(), 0, device->primary);
	device->primary = NULL;

	HeapFree(GetProcessHeap(),0,device->primary_pwfx);
	HeapFree(GetProcessHeap(),0,device->pwfx);
	device->pwfx=NULL;

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	return DS_OK;
}

WAVEFORMATEX *DSOUND_CopyFormat(const WAVEFORMATEX *wfex)
{
    WAVEFORMATEX *pwfx;
    if(wfex->wFormatTag == WAVE_FORMAT_PCM){
        pwfx = HeapAlloc(GetProcessHeap(), 0, sizeof(WAVEFORMATEX));
        if (!pwfx)
            return NULL;
        CopyMemory(pwfx, wfex, sizeof(PCMWAVEFORMAT));
        pwfx->cbSize = 0;
    }else{
        pwfx = HeapAlloc(GetProcessHeap(), 0, sizeof(WAVEFORMATEX) + wfex->cbSize);
        if (!pwfx)
            return NULL;
        CopyMemory(pwfx, wfex, sizeof(WAVEFORMATEX) + wfex->cbSize);
    }

    if(pwfx->wFormatTag == WAVE_FORMAT_PCM ||
            (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
             IsEqualGUID(&((const WAVEFORMATEXTENSIBLE*)pwfx)->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)))
        pwfx->nBlockAlign = (pwfx->nChannels * pwfx->wBitsPerSample) / 8;

    return pwfx;
}

HRESULT primarybuffer_SetFormat(DirectSoundDevice *device, LPCWAVEFORMATEX passed_fmt)
{
	HRESULT err = S_OK;
	WAVEFORMATEX *old_fmt;
	WAVEFORMATEXTENSIBLE *fmtex, *passed_fmtex = (WAVEFORMATEXTENSIBLE*)passed_fmt;

	TRACE("(%p,%p)\n", device, passed_fmt);

	if (device->priolevel == DSSCL_NORMAL) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

	/* Let's be pedantic! */
	if (passed_fmt == NULL) {
		WARN("invalid parameter: passed_fmt==NULL!\n");
		return DSERR_INVALIDPARAM;
	}
	TRACE("(formattag=0x%04x,chans=%d,samplerate=%d,"
			  "bytespersec=%d,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		  passed_fmt->wFormatTag, passed_fmt->nChannels, passed_fmt->nSamplesPerSec,
		  passed_fmt->nAvgBytesPerSec, passed_fmt->nBlockAlign,
		  passed_fmt->wBitsPerSample, passed_fmt->cbSize);

	if(passed_fmt->wBitsPerSample < 8 || passed_fmt->wBitsPerSample % 8 != 0 ||
			passed_fmt->nChannels == 0 || passed_fmt->nSamplesPerSec == 0 ||
			passed_fmt->nAvgBytesPerSec == 0 ||
			passed_fmt->nBlockAlign != passed_fmt->nChannels * passed_fmt->wBitsPerSample / 8)
		return DSERR_INVALIDPARAM;

	if(passed_fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE){
		if(passed_fmtex->Samples.wValidBitsPerSample > passed_fmtex->Format.wBitsPerSample)
			return DSERR_INVALIDPARAM;
	}

	/* **** */
	RtlAcquireResourceExclusive(&(device->buffer_list_lock), TRUE);
	EnterCriticalSection(&(device->mixlock));

	if (device->priolevel == DSSCL_WRITEPRIMARY) {
		old_fmt = device->primary_pwfx;
		device->primary_pwfx = DSOUND_CopyFormat(passed_fmt);
		fmtex = (WAVEFORMATEXTENSIBLE *)device->primary_pwfx;
		if (device->primary_pwfx == NULL) {
			err = DSERR_OUTOFMEMORY;
			goto out;
		}

		if (fmtex->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
		    fmtex->Samples.wValidBitsPerSample == 0) {
			TRACE("Correcting 0 valid bits per sample\n");
			fmtex->Samples.wValidBitsPerSample = fmtex->Format.wBitsPerSample;
		}

		err = DSOUND_ReopenDevice(device, TRUE);
		if (FAILED(err)) {
			ERR("No formats could be opened\n");
			HeapFree(GetProcessHeap(), 0, device->primary_pwfx);
			device->primary_pwfx = old_fmt;
		} else
			HeapFree(GetProcessHeap(), 0, old_fmt);
	} else {
		WAVEFORMATEX *wfx = DSOUND_CopyFormat(passed_fmt);
		if (wfx) {
			HeapFree(GetProcessHeap(), 0, device->primary_pwfx);
			device->primary_pwfx = wfx;
		} else
			err = DSERR_OUTOFMEMORY;
	}

out:
	LeaveCriticalSection(&(device->mixlock));
	RtlReleaseResource(&(device->buffer_list_lock));
	/* **** */

	return err;
}

/*******************************************************************************
 *		PrimaryBuffer
 */
static inline IDirectSoundBufferImpl *impl_from_IDirectSoundBuffer(IDirectSoundBuffer *iface)
{
    /* IDirectSoundBuffer and IDirectSoundBuffer8 use the same iface. */
    return CONTAINING_RECORD((IDirectSoundBuffer8 *)iface, IDirectSoundBufferImpl, IDirectSoundBuffer8_iface);
}

/* This sets this format for the primary buffer only */
static HRESULT WINAPI PrimaryBufferImpl_SetFormat(IDirectSoundBuffer *iface,
        const WAVEFORMATEX *wfex)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
    TRACE("(%p,%p)\n", iface, wfex);
    return primarybuffer_SetFormat(This->device, wfex);
}

static HRESULT WINAPI PrimaryBufferImpl_SetVolume(IDirectSoundBuffer *iface, LONG vol)
{
	IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	DirectSoundDevice *device = This->device;
	HRESULT hr;
	float fvol;
	int i;

	TRACE("(%p,%d)\n", iface, vol);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLVOLUME)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if ((vol > DSBVOLUME_MAX) || (vol < DSBVOLUME_MIN)) {
		WARN("invalid parameter: vol = %d\n", vol);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&device->mixlock);

	for (i = 0; i < DS_MAX_CHANNELS; i++) {
		if (device->pwfx->nChannels > i){
			hr = IAudioStreamVolume_GetChannelVolume(device->volume, i, &fvol);
			if (FAILED(hr)){
				LeaveCriticalSection(&device->mixlock);
				WARN("GetChannelVolume failed: %08x\n", hr);
				return hr;
			}
		} else
			fvol=1.0f;

		device->volpan.dwTotalAmpFactor[i]=((UINT16)(fvol * (DWORD)0xFFFF));
	}

	DSOUND_AmpFactorToVolPan(&device->volpan);
	if (vol != device->volpan.lVolume) {
		device->volpan.lVolume=vol;
		DSOUND_RecalcVolPan(&device->volpan);

		for (i = 0; i < DS_MAX_CHANNELS; i++) {
			if (device->pwfx->nChannels > i){
				fvol = (float)((DWORD)(device->volpan.dwTotalAmpFactor[i] & 0xFFFF) / (float)0xFFFF);
				hr = IAudioStreamVolume_SetChannelVolume(device->volume, i, fvol);
				if (FAILED(hr)){
					LeaveCriticalSection(&device->mixlock);
					WARN("SetChannelVolume failed: %08x\n", hr);
					return hr;
				}
			}
		}
	}

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetVolume(IDirectSoundBuffer *iface, LONG *vol)
{
	IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	DirectSoundDevice *device = This->device;
	float fvol;
	HRESULT hr;
	int i;

	TRACE("(%p,%p)\n", iface, vol);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLVOLUME)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (vol == NULL) {
		WARN("invalid parameter: vol = NULL\n");
		return DSERR_INVALIDPARAM;
	}

	EnterCriticalSection(&device->mixlock);

	for (i = 0; i < DS_MAX_CHANNELS; i++) {
		if (device->pwfx->nChannels > i){
			hr = IAudioStreamVolume_GetChannelVolume(device->volume, i, &fvol);
			if (FAILED(hr)){
				LeaveCriticalSection(&device->mixlock);
				WARN("GetChannelVolume failed: %08x\n", hr);
				return hr;
			}
		} else
			fvol = 1;

		device->volpan.dwTotalAmpFactor[i] = ((UINT16)(fvol * (DWORD)0xFFFF));
    }

	DSOUND_AmpFactorToVolPan(&device->volpan);
	*vol = device->volpan.lVolume;

	LeaveCriticalSection(&device->mixlock);

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_SetFrequency(IDirectSoundBuffer *iface, DWORD freq)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	TRACE("(%p,%d)\n",This,freq);

	/* You cannot set the frequency of the primary buffer */
	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI PrimaryBufferImpl_Play(IDirectSoundBuffer *iface, DWORD reserved1,
        DWORD reserved2, DWORD flags)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%08x,%08x,%08x)\n", iface, reserved1, reserved2, flags);

	if (!(flags & DSBPLAY_LOOPING)) {
		WARN("invalid parameter: flags = %08x\n", flags);
		return DSERR_INVALIDPARAM;
	}

	device->stopped = 0;

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Stop(IDirectSoundBuffer *iface)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p)\n", iface);

	device->stopped = 1;

	return DS_OK;
}

static ULONG WINAPI PrimaryBufferImpl_AddRef(IDirectSoundBuffer *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);
    return ref;
}

/* Decreases *out by 1 to no less than 0.
 * Returns the new value of *out. */
LONG capped_refcount_dec(LONG *out)
{
    LONG ref, oldref;
    do {
        ref = *out;
        if(!ref)
            return 0;
        oldref = InterlockedCompareExchange(out, ref - 1, ref);
    } while(oldref != ref);
    return ref - 1;
}

static ULONG WINAPI PrimaryBufferImpl_Release(IDirectSoundBuffer *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
    ULONG ref;

    ref = capped_refcount_dec(&This->ref);
    if(!ref)
        capped_refcount_dec(&This->numIfaces);

    TRACE("(%p) primary ref is now %d\n", This, ref);

    return ref;
}

static HRESULT WINAPI PrimaryBufferImpl_GetCurrentPosition(IDirectSoundBuffer *iface,
        DWORD *playpos, DWORD *writepos)
{
	HRESULT	hres = DS_OK;
	UINT32 pad = 0;
	UINT32 mixpos;
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%p,%p)\n", iface, playpos, writepos);

	/* **** */
	EnterCriticalSection(&(device->mixlock));

	if (device->client)
		hres = IAudioClient_GetCurrentPadding(device->client, &pad);
	if (hres != DS_OK) {
		WARN("IAudioClient_GetCurrentPadding failed\n");
		LeaveCriticalSection(&(device->mixlock));
		return hres;
	}
	mixpos = (device->playpos + pad * device->pwfx->nBlockAlign) % device->buflen;
	if (playpos)
		*playpos = mixpos;
	if (writepos) {
		*writepos = mixpos;
		if (!device->stopped) {
			/* apply the documented 10ms lead to writepos */
			*writepos += device->writelead;
			*writepos %= device->buflen;
		}
	}

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	TRACE("playpos = %d, writepos = %d (%p, time=%d)\n", playpos?*playpos:0, writepos?*writepos:0, device, GetTickCount());
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetStatus(IDirectSoundBuffer *iface, DWORD *status)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%p)\n", iface, status);

	if (status == NULL) {
		WARN("invalid parameter: status == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*status = 0;
	if (!device->stopped)
		*status |= DSBSTATUS_PLAYING | DSBSTATUS_LOOPING;

	TRACE("status=%x\n", *status);
	return DS_OK;
}


static HRESULT WINAPI PrimaryBufferImpl_GetFormat(IDirectSoundBuffer *iface, WAVEFORMATEX *lpwf,
        DWORD wfsize, DWORD *wfwritten)
{
    DWORD size;
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
    DirectSoundDevice *device = This->device;
    TRACE("(%p,%p,%d,%p)\n", iface, lpwf, wfsize, wfwritten);

    size = sizeof(WAVEFORMATEX) + device->primary_pwfx->cbSize;

    if (lpwf) {	/* NULL is valid */
        if (wfsize >= size) {
            CopyMemory(lpwf,device->primary_pwfx,size);
            if (wfwritten)
                *wfwritten = size;
        } else {
            WARN("invalid parameter: wfsize too small\n");
            if (wfwritten)
                *wfwritten = 0;
            return DSERR_INVALIDPARAM;
        }
    } else {
        if (wfwritten)
            *wfwritten = sizeof(WAVEFORMATEX) + device->primary_pwfx->cbSize;
        else {
            WARN("invalid parameter: wfwritten == NULL\n");
            return DSERR_INVALIDPARAM;
        }
    }

    return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Lock(IDirectSoundBuffer *iface, DWORD writecursor,
        DWORD writebytes, void **lplpaudioptr1, DWORD *audiobytes1, void **lplpaudioptr2,
        DWORD *audiobytes2, DWORD flags)
{
	HRESULT hres;
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%d,%d,%p,%p,%p,%p,0x%08x) at %d\n",
		iface,
		writecursor,
		writebytes,
		lplpaudioptr1,
		audiobytes1,
		lplpaudioptr2,
		audiobytes2,
		flags,
		GetTickCount()
	);

        if (!audiobytes1)
            return DSERR_INVALIDPARAM;

	if (device->priolevel != DSSCL_WRITEPRIMARY) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

        /* when this flag is set, writecursor is meaningless and must be calculated */
	if (flags & DSBLOCK_FROMWRITECURSOR) {
		/* GetCurrentPosition does too much magic to duplicate here */
		hres = IDirectSoundBuffer_GetCurrentPosition(iface, NULL, &writecursor);
		if (hres != DS_OK) {
			WARN("IDirectSoundBuffer_GetCurrentPosition failed\n");
			return hres;
		}
	}

        /* when this flag is set, writebytes is meaningless and must be set */
	if (flags & DSBLOCK_ENTIREBUFFER)
		writebytes = device->buflen;

        if (writecursor >= device->buflen) {
                WARN("Invalid parameter, writecursor: %u >= buflen: %u\n",
		     writecursor, device->buflen);
                return DSERR_INVALIDPARAM;
        }

        if (writebytes > device->buflen) {
                WARN("Invalid parameter, writebytes: %u > buflen: %u\n",
		     writebytes, device->buflen);
                return DSERR_INVALIDPARAM;
        }

	if (writecursor+writebytes <= device->buflen) {
		*(LPBYTE*)lplpaudioptr1 = device->buffer+writecursor;
		*audiobytes1 = writebytes;
		if (lplpaudioptr2)
			*(LPBYTE*)lplpaudioptr2 = NULL;
		if (audiobytes2)
			*audiobytes2 = 0;
		TRACE("->%d.0\n",writebytes);
	} else {
		*(LPBYTE*)lplpaudioptr1 = device->buffer+writecursor;
		*audiobytes1 = device->buflen-writecursor;
		if (lplpaudioptr2)
			*(LPBYTE*)lplpaudioptr2 = device->buffer;
		if (audiobytes2)
			*audiobytes2 = writebytes-(device->buflen-writecursor);
		TRACE("->%d.%d\n",*audiobytes1,audiobytes2?*audiobytes2:0);
	}
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_SetCurrentPosition(IDirectSoundBuffer *iface, DWORD newpos)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	TRACE("(%p,%d)\n",This,newpos);

	/* You cannot set the position of the primary buffer */
	WARN("invalid call\n");
	return DSERR_INVALIDCALL;
}

static HRESULT WINAPI PrimaryBufferImpl_SetPan(IDirectSoundBuffer *iface, LONG pan)
{
	IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	DirectSoundDevice *device = This->device;
	float fvol;
	HRESULT hr;
	int i;

	TRACE("(%p,%d)\n", iface, pan);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLPAN)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if ((pan > DSBPAN_RIGHT) || (pan < DSBPAN_LEFT)) {
		WARN("invalid parameter: pan = %d\n", pan);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&device->mixlock);

	for (i = 0; i < DS_MAX_CHANNELS; i++) {
		if (device->pwfx->nChannels > i){
			hr = IAudioStreamVolume_GetChannelVolume(device->volume, i, &fvol);
			if (FAILED(hr)){
				LeaveCriticalSection(&device->mixlock);
				WARN("GetChannelVolume failed: %08x\n", hr);
				return hr;
			}
		} else
			fvol = 1;

		device->volpan.dwTotalAmpFactor[i] = ((UINT16)(fvol * (DWORD)0xFFFF));
	}

	DSOUND_AmpFactorToVolPan(&device->volpan);
	if (pan != device->volpan.lPan) {
		device->volpan.lPan=pan;
		DSOUND_RecalcVolPan(&device->volpan);

		for (i = 0; i < DS_MAX_CHANNELS; i++) {
			if (device->pwfx->nChannels > i) {
				fvol = (float)((DWORD)(device->volpan.dwTotalAmpFactor[i] & 0xFFFF) / (float)0xFFFF);
				hr = IAudioStreamVolume_SetChannelVolume(device->volume, i, fvol);
				if (FAILED(hr)){
					LeaveCriticalSection(&device->mixlock);
					WARN("SetChannelVolume failed: %08x\n", hr);
					return hr;
				}
			}
		}
	}

	LeaveCriticalSection(&device->mixlock);
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetPan(IDirectSoundBuffer *iface, LONG *pan)
{
	IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	DirectSoundDevice *device = This->device;
	float fvol;
	HRESULT hr;
	int i;

	TRACE("(%p,%p)\n", iface, pan);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLPAN)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (pan == NULL) {
		WARN("invalid parameter: pan == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	EnterCriticalSection(&device->mixlock);

	for (i = 0; i < DS_MAX_CHANNELS; i++) {
		if (device->pwfx->nChannels > i) {
			hr = IAudioStreamVolume_GetChannelVolume(device->volume, i, &fvol);
			if (FAILED(hr)){
				LeaveCriticalSection(&device->mixlock);
				WARN("GetChannelVolume failed: %08x\n", hr);
				return hr;
			}
		} else
			fvol = 1;

		device->volpan.dwTotalAmpFactor[i] = ((UINT16)(fvol * (DWORD)0xFFFF));
    }

	DSOUND_AmpFactorToVolPan(&device->volpan);
	*pan = device->volpan.lPan;

	LeaveCriticalSection(&device->mixlock);

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Unlock(IDirectSoundBuffer *iface, void *p1, DWORD x1,
        void *p2, DWORD x2)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%p,%d,%p,%d)\n", iface, p1, x1, p2, x2);

	if (device->priolevel != DSSCL_WRITEPRIMARY) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

	if ((p1 && ((BYTE*)p1 < device->buffer || (BYTE*)p1 >= device->buffer + device->buflen)) ||
	    (p2 && ((BYTE*)p2 < device->buffer || (BYTE*)p2 >= device->buffer + device->buflen)))
		return DSERR_INVALIDPARAM;

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Restore(IDirectSoundBuffer *iface)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	FIXME("(%p):stub\n",This);
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetFrequency(IDirectSoundBuffer *iface, DWORD *freq)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
	TRACE("(%p,%p)\n", iface, freq);

	if (freq == NULL) {
		WARN("invalid parameter: freq == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	*freq = device->pwfx->nSamplesPerSec;
	TRACE("-> %d\n", *freq);

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Initialize(IDirectSoundBuffer *iface, IDirectSound *dsound,
        const DSBUFFERDESC *dbsd)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
	WARN("(%p) already initialized\n", This);
	return DSERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI PrimaryBufferImpl_GetCaps(IDirectSoundBuffer *iface, DSBCAPS *caps)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);
        DirectSoundDevice *device = This->device;
  	TRACE("(%p,%p)\n", iface, caps);

	if (caps == NULL) {
		WARN("invalid parameter: caps == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (caps->dwSize < sizeof(*caps)) {
		WARN("invalid parameter: caps->dwSize = %d\n", caps->dwSize);
		return DSERR_INVALIDPARAM;
	}

	caps->dwFlags = This->dsbd.dwFlags;
	caps->dwBufferBytes = device->buflen;

	/* Windows reports these as zero */
	caps->dwUnlockTransferRate = 0;
	caps->dwPlayCpuOverhead = 0;

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_QueryInterface(IDirectSoundBuffer *iface, REFIID riid,
        void **ppobj)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer(iface);

	TRACE("(%p,%s,%p)\n", iface, debugstr_guid(riid), ppobj);

	if (ppobj == NULL) {
		WARN("invalid parameter\n");
		return E_INVALIDARG;
	}

	*ppobj = NULL;	/* assume failure */

	if ( IsEqualGUID(riid, &IID_IUnknown) ||
	     IsEqualGUID(riid, &IID_IDirectSoundBuffer) ) {
		IDirectSoundBuffer_AddRef(iface);
		*ppobj = iface;
		return S_OK;
	}

	/* DirectSoundBuffer and DirectSoundBuffer8 are different and */
	/* a primary buffer can't have a DirectSoundBuffer8 interface */
	if ( IsEqualGUID( &IID_IDirectSoundBuffer8, riid ) ) {
		WARN("app requested DirectSoundBuffer8 on primary buffer\n");
		return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IDirectSoundNotify, riid ) ) {
		ERR("app requested IDirectSoundNotify on primary buffer\n");
		/* FIXME: should we support this? */
		return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IDirectSound3DBuffer, riid ) ) {
		ERR("app requested IDirectSound3DBuffer on primary buffer\n");
		return E_NOINTERFACE;
	}

        if ( IsEqualGUID( &IID_IDirectSound3DListener, riid ) ) {
                *ppobj = &This->IDirectSound3DListener_iface;
                IDirectSound3DListener_AddRef(&This->IDirectSound3DListener_iface);
                return S_OK;
	}

	if ( IsEqualGUID( &IID_IKsPropertySet, riid ) ) {
                *ppobj = &This->IKsPropertySet_iface;
                IKsPropertySet_AddRef(&This->IKsPropertySet_iface);
                return S_OK;
	}

	FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );
	return E_NOINTERFACE;
}

static const IDirectSoundBufferVtbl dspbvt =
{
	PrimaryBufferImpl_QueryInterface,
	PrimaryBufferImpl_AddRef,
	PrimaryBufferImpl_Release,
	PrimaryBufferImpl_GetCaps,
	PrimaryBufferImpl_GetCurrentPosition,
	PrimaryBufferImpl_GetFormat,
	PrimaryBufferImpl_GetVolume,
	PrimaryBufferImpl_GetPan,
        PrimaryBufferImpl_GetFrequency,
	PrimaryBufferImpl_GetStatus,
	PrimaryBufferImpl_Initialize,
	PrimaryBufferImpl_Lock,
	PrimaryBufferImpl_Play,
	PrimaryBufferImpl_SetCurrentPosition,
	PrimaryBufferImpl_SetFormat,
	PrimaryBufferImpl_SetVolume,
	PrimaryBufferImpl_SetPan,
	PrimaryBufferImpl_SetFrequency,
	PrimaryBufferImpl_Stop,
	PrimaryBufferImpl_Unlock,
	PrimaryBufferImpl_Restore
};

HRESULT primarybuffer_create(DirectSoundDevice *device, IDirectSoundBufferImpl **ppdsb,
	const DSBUFFERDESC *dsbd)
{
	IDirectSoundBufferImpl *dsb;
	TRACE("%p,%p,%p)\n",device,ppdsb,dsbd);

	if (dsbd->lpwfxFormat) {
		WARN("invalid parameter: dsbd->lpwfxFormat != NULL\n");
		*ppdsb = NULL;
		return DSERR_INVALIDPARAM;
	}

	dsb = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*dsb));

	if (dsb == NULL) {
		WARN("out of memory\n");
		*ppdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}

        dsb->ref = 0;
        dsb->ref3D = 0;
        dsb->refiks = 0;
        dsb->numIfaces = 0;
	dsb->device = device;
	dsb->IDirectSoundBuffer8_iface.lpVtbl = (IDirectSoundBuffer8Vtbl *)&dspbvt;
        dsb->IDirectSound3DListener_iface.lpVtbl = &ds3dlvt;
        dsb->IKsPropertySet_iface.lpVtbl = &iksbvt;
	dsb->dsbd = *dsbd;

        /* IDirectSound3DListener */
        device->ds3dl.dwSize = sizeof(DS3DLISTENER);
        device->ds3dl.vPosition.x = 0.0;
        device->ds3dl.vPosition.y = 0.0;
        device->ds3dl.vPosition.z = 0.0;
        device->ds3dl.vVelocity.x = 0.0;
        device->ds3dl.vVelocity.y = 0.0;
        device->ds3dl.vVelocity.z = 0.0;
        device->ds3dl.vOrientFront.x = 0.0;
        device->ds3dl.vOrientFront.y = 0.0;
        device->ds3dl.vOrientFront.z = 1.0;
        device->ds3dl.vOrientTop.x = 0.0;
        device->ds3dl.vOrientTop.y = 1.0;
        device->ds3dl.vOrientTop.z = 0.0;
        device->ds3dl.flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
        device->ds3dl.flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;
        device->ds3dl.flDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;
        device->ds3dl_need_recalc = TRUE;

	TRACE("Created primary buffer at %p\n", dsb);
	TRACE("(formattag=0x%04x,chans=%d,samplerate=%d,"
		"bytespersec=%d,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		device->pwfx->wFormatTag, device->pwfx->nChannels,
                device->pwfx->nSamplesPerSec, device->pwfx->nAvgBytesPerSec,
                device->pwfx->nBlockAlign, device->pwfx->wBitsPerSample,
                device->pwfx->cbSize);

        IDirectSoundBuffer_AddRef(&dsb->IDirectSoundBuffer8_iface);
	*ppdsb = dsb;
	return S_OK;
}
