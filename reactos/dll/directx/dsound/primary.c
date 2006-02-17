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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winreg.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

void DSOUND_RecalcPrimary(DirectSoundDevice *device)
{
	DWORD nBlockAlign;
	TRACE("(%p)\n", device);

	nBlockAlign = device->pwfx->nBlockAlign;
	if (device->hwbuf) {
		DWORD fraglen;
		/* let fragment size approximate the timer delay */
		fraglen = (device->pwfx->nSamplesPerSec * DS_TIME_DEL / 1000) * nBlockAlign;
		/* reduce fragment size until an integer number of them fits in the buffer */
		/* (FIXME: this may or may not be a good idea) */
		while (device->buflen % fraglen) fraglen -= nBlockAlign;
		device->fraglen = fraglen;
		TRACE("fraglen=%ld\n", device->fraglen);
	}
	/* calculate the 10ms write lead */
	device->writelead = (device->pwfx->nSamplesPerSec / 100) * nBlockAlign;
}

static HRESULT DSOUND_PrimaryOpen(DirectSoundDevice *device)
{
	HRESULT err = DS_OK;
	TRACE("(%p)\n", device);

	/* are we using waveOut stuff? */
	if (!device->driver) {
		LPBYTE newbuf;
		DWORD buflen;
		HRESULT merr = DS_OK;
		/* Start in pause mode, to allow buffers to get filled */
		waveOutPause(device->hwo);
		if (device->state == STATE_PLAYING) device->state = STATE_STARTING;
		else if (device->state == STATE_STOPPING) device->state = STATE_STOPPED;
		/* use fragments of 10ms (1/100s) each (which should get us within
		 * the documented write cursor lead of 10-15ms) */
		buflen = ((device->pwfx->nSamplesPerSec / 100) * device->pwfx->nBlockAlign) * DS_HEL_FRAGS;
		TRACE("desired buflen=%ld, old buffer=%p\n", buflen, device->buffer);
		/* reallocate emulated primary buffer */

		if (device->buffer)
			newbuf = HeapReAlloc(GetProcessHeap(),0,device->buffer,buflen);
		else
			newbuf = HeapAlloc(GetProcessHeap(),0,buflen);

		if (newbuf == NULL) {
			ERR("failed to allocate primary buffer\n");
			merr = DSERR_OUTOFMEMORY;
			/* but the old buffer might still exist and must be re-prepared */
		} else {
			device->buffer = newbuf;
			device->buflen = buflen;
		}
		if (device->buffer) {
			unsigned c;

			device->fraglen = device->buflen / DS_HEL_FRAGS;

			/* prepare fragment headers */
			for (c=0; c<DS_HEL_FRAGS; c++) {
				device->pwave[c]->lpData = (char*)device->buffer + c*device->fraglen;
				device->pwave[c]->dwBufferLength = device->fraglen;
				device->pwave[c]->dwUser = (DWORD)device;
				device->pwave[c]->dwFlags = 0;
				device->pwave[c]->dwLoops = 0;
				err = mmErr(waveOutPrepareHeader(device->hwo,device->pwave[c],sizeof(WAVEHDR)));
				if (err != DS_OK) {
					while (c--)
						waveOutUnprepareHeader(device->hwo,device->pwave[c],sizeof(WAVEHDR));
					break;
				}
			}

			device->pwplay = 0;
			device->pwwrite = 0;
			device->pwqueue = 0;
			device->playpos = 0;
			device->mixpos = 0;
			FillMemory(device->buffer, device->buflen, (device->pwfx->wBitsPerSample == 8) ? 128 : 0);
			TRACE("fraglen=%ld\n", device->fraglen);
			DSOUND_WaveQueue(device, (DWORD)-1);
		}
		if ((err == DS_OK) && (merr != DS_OK))
			err = merr;
	} else if (!device->hwbuf) {
		err = IDsDriver_CreateSoundBuffer(device->driver,device->pwfx,
						  DSBCAPS_PRIMARYBUFFER,0,
						  &(device->buflen),&(device->buffer),
						  (LPVOID*)&(device->hwbuf));
		if (err != DS_OK) {
			WARN("IDsDriver_CreateSoundBuffer failed\n");
			return err;
		}

		if (device->state == STATE_PLAYING) device->state = STATE_STARTING;
		else if (device->state == STATE_STOPPING) device->state = STATE_STOPPED;
		device->playpos = 0;
		device->mixpos = 0;
		FillMemory(device->buffer, device->buflen, (device->pwfx->wBitsPerSample == 8) ? 128 : 0);
	}

	return err;
}


static void DSOUND_PrimaryClose(DirectSoundDevice *device)
{
	TRACE("(%p)\n", device);

	/* are we using waveOut stuff? */
	if (!device->hwbuf) {
		unsigned c;

		device->pwqueue = (DWORD)-1; /* resetting queues */
		waveOutReset(device->hwo);
		for (c=0; c<DS_HEL_FRAGS; c++)
			waveOutUnprepareHeader(device->hwo, device->pwave[c], sizeof(WAVEHDR));
		device->pwqueue = 0;
	} else {
		if (IDsDriverBuffer_Release(device->hwbuf) == 0)
			device->hwbuf = 0;
	}
}

HRESULT DSOUND_PrimaryCreate(DirectSoundDevice *device)
{
	HRESULT err = DS_OK;
	TRACE("(%p)\n", device);

	device->buflen = device->pwfx->nAvgBytesPerSec;

	/* FIXME: verify that hardware capabilities (DSCAPS_PRIMARY flags) match */

	if (device->driver) {
		err = IDsDriver_CreateSoundBuffer(device->driver,device->pwfx,
						  DSBCAPS_PRIMARYBUFFER,0,
						  &(device->buflen),&(device->buffer),
						  (LPVOID*)&(device->hwbuf));
		if (err != DS_OK) {
			WARN("IDsDriver_CreateSoundBuffer failed\n");
			return err;
		}
	}
	if (!device->hwbuf) {
		/* Allocate memory for HEL buffer headers */
		unsigned c;
		for (c=0; c<DS_HEL_FRAGS; c++) {
			device->pwave[c] = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(WAVEHDR));
			if (!device->pwave[c]) {
				/* Argh, out of memory */
				while (c--) {
					HeapFree(GetProcessHeap(),0,device->pwave[c]);
				}
				WARN("out of memory\n");
				return DSERR_OUTOFMEMORY;
			}
		}
	}

	err = DSOUND_PrimaryOpen(device);

	if (err != DS_OK) {
		WARN("DSOUND_PrimaryOpen failed\n");
		return err;
	}

	/* calculate fragment size and write lead */
	DSOUND_RecalcPrimary(device);
	device->state = STATE_STOPPED;
	return DS_OK;
}

HRESULT DSOUND_PrimaryDestroy(DirectSoundDevice *device)
{
	TRACE("(%p)\n", device);

	DSOUND_PrimaryClose(device);
	if (device->driver) {
		if (device->hwbuf) {
			if (IDsDriverBuffer_Release(device->hwbuf) == 0)
				device->hwbuf = 0;
		}
	} else {
		unsigned c;
		for (c=0; c<DS_HEL_FRAGS; c++) {
			HeapFree(GetProcessHeap(),0,device->pwave[c]);
		}
	}
        HeapFree(GetProcessHeap(),0,device->pwfx);
        device->pwfx=NULL;
	return DS_OK;
}

HRESULT DSOUND_PrimaryPlay(DirectSoundDevice *device)
{
	HRESULT err = DS_OK;
	TRACE("(%p)\n", device);

	if (device->hwbuf) {
		err = IDsDriverBuffer_Play(device->hwbuf, 0, 0, DSBPLAY_LOOPING);
		if (err != DS_OK)
			WARN("IDsDriverBuffer_Play failed\n");
	} else {
		err = mmErr(waveOutRestart(device->hwo));
		if (err != DS_OK)
			WARN("waveOutRestart failed\n");
	}

	return err;
}

HRESULT DSOUND_PrimaryStop(DirectSoundDevice *device)
{
	HRESULT err = DS_OK;
	TRACE("(%p)\n", device);

	if (device->hwbuf) {
		err = IDsDriverBuffer_Stop(device->hwbuf);
		if (err == DSERR_BUFFERLOST) {
			DWORD flags = CALLBACK_FUNCTION;
			if (ds_hw_accel != DS_HW_ACCEL_EMULATION)
				flags |= WAVE_DIRECTSOUND;
			/* Wine-only: the driver wants us to reopen the device */
			/* FIXME: check for errors */
			IDsDriverBuffer_Release(device->hwbuf);
			waveOutClose(device->hwo);
			device->hwo = 0;
			err = mmErr(waveOutOpen(&(device->hwo), device->drvdesc.dnDevNode,
						device->pwfx, (DWORD_PTR)DSOUND_callback, (DWORD)device,
						flags));
			if (err == DS_OK) {
				err = IDsDriver_CreateSoundBuffer(device->driver,device->pwfx,
								  DSBCAPS_PRIMARYBUFFER,0,
								  &(device->buflen),&(device->buffer),
								  (LPVOID)&(device->hwbuf));
				if (err != DS_OK)
					WARN("IDsDriver_CreateSoundBuffer failed\n");
			} else {
				WARN("waveOutOpen failed\n");
			}
		} else if (err != DS_OK) {
			WARN("IDsDriverBuffer_Stop failed\n");
		}
	} else {
		err = mmErr(waveOutPause(device->hwo));
		if (err != DS_OK)
			WARN("waveOutPause failed\n");
	}
	return err;
}

HRESULT DSOUND_PrimaryGetPosition(DirectSoundDevice *device, LPDWORD playpos, LPDWORD writepos)
{
	TRACE("(%p,%p,%p)\n", device, playpos, writepos);

	if (device->hwbuf) {
		HRESULT err=IDsDriverBuffer_GetPosition(device->hwbuf,playpos,writepos);
		if (err) {
			WARN("IDsDriverBuffer_GetPosition failed\n");
			return err;
		}
	} else {
		if (playpos) {
			MMTIME mtime;
			mtime.wType = TIME_BYTES;
			waveOutGetPosition(device->hwo, &mtime, sizeof(mtime));
			mtime.u.cb = mtime.u.cb % device->buflen;
			*playpos = mtime.u.cb;
		}
		if (writepos) {
			/* the writepos should only be used by apps with WRITEPRIMARY priority,
			 * in which case our software mixer is disabled anyway */
			*writepos = (device->pwplay + ds_hel_margin) * device->fraglen;
			while (*writepos >= device->buflen)
				*writepos -= device->buflen;
		}
	}
	TRACE("playpos = %ld, writepos = %ld (%p, time=%ld)\n", playpos?*playpos:0, writepos?*writepos:0, device, GetTickCount());
	return DS_OK;
}

/*******************************************************************************
 *		PrimaryBuffer
 */
/* This sets this format for the <em>Primary Buffer Only</em> */
/* See file:///cdrom/sdk52/docs/worddoc/dsound.doc page 120 */
static HRESULT WINAPI PrimaryBufferImpl_SetFormat(
	LPDIRECTSOUNDBUFFER8 iface,LPCWAVEFORMATEX wfex
) {
	DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	HRESULT err = DS_OK;
	int i, alloc_size, cp_size;
	DWORD nSamplesPerSec;
	TRACE("(%p,%p)\n", iface, wfex);

	if (device->priolevel == DSSCL_NORMAL) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

	/* Let's be pedantic! */
	if (wfex == NULL) {
		WARN("invalid parameter: wfex==NULL!\n");
		return DSERR_INVALIDPARAM;
	}
	TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld,"
	      "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
	      wfex->wFormatTag, wfex->nChannels, wfex->nSamplesPerSec,
	      wfex->nAvgBytesPerSec, wfex->nBlockAlign,
	      wfex->wBitsPerSample, wfex->cbSize);

	/* **** */
	RtlAcquireResourceExclusive(&(device->buffer_list_lock), TRUE);
	EnterCriticalSection(&(device->mixlock));

	if (wfex->wFormatTag == WAVE_FORMAT_PCM) {
            alloc_size = sizeof(WAVEFORMATEX);
            cp_size = sizeof(PCMWAVEFORMAT);
        } else
            alloc_size = cp_size = sizeof(WAVEFORMATEX) + wfex->cbSize;

        device->pwfx = HeapReAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,device->pwfx,alloc_size);

	nSamplesPerSec = device->pwfx->nSamplesPerSec;

        CopyMemory(device->pwfx, wfex, cp_size);

	if (device->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMSETFORMAT) {
		DWORD flags = CALLBACK_FUNCTION;
		if (ds_hw_accel != DS_HW_ACCEL_EMULATION)
			flags |= WAVE_DIRECTSOUND;
		/* FIXME: check for errors */
		DSOUND_PrimaryClose(device);
		waveOutClose(device->hwo);
		device->hwo = 0;
                err = mmErr(waveOutOpen(&(device->hwo), device->drvdesc.dnDevNode,
                                        device->pwfx, (DWORD_PTR)DSOUND_callback, (DWORD)device,
                                        flags));
                if (err == DS_OK) {
                    err = DSOUND_PrimaryOpen(device);
		    if (err != DS_OK) {
			WARN("DSOUND_PrimaryOpen failed\n");
			goto done;
		    }
		} else {
			WARN("waveOutOpen failed\n");
			goto done;
		}
	} else if (device->hwbuf) {
		err = IDsDriverBuffer_SetFormat(device->hwbuf, device->pwfx);
		if (err == DSERR_BUFFERLOST) {
			/* Wine-only: the driver wants us to recreate the HW buffer */
			IDsDriverBuffer_Release(device->hwbuf);
			err = IDsDriver_CreateSoundBuffer(device->driver,device->pwfx,
							  DSBCAPS_PRIMARYBUFFER,0,
							  &(device->buflen),&(device->buffer),
							  (LPVOID)&(device->hwbuf));
			if (err != DS_OK) {
				WARN("IDsDriver_CreateSoundBuffer failed\n");
				goto done;
			}
			if (device->state == STATE_PLAYING) device->state = STATE_STARTING;
			else if (device->state == STATE_STOPPING) device->state = STATE_STOPPED;
		} else {
			WARN("IDsDriverBuffer_SetFormat failed\n");
			goto done;
		}
                /* FIXME: should we set err back to DS_OK in all cases ? */
	}
	DSOUND_RecalcPrimary(device);

	if (nSamplesPerSec != device->pwfx->nSamplesPerSec) {
		IDirectSoundBufferImpl** dsb = device->buffers;
		for (i = 0; i < device->nrofbuffers; i++, dsb++) {
			/* **** */
			EnterCriticalSection(&((*dsb)->lock));

			(*dsb)->freqAdjust = ((*dsb)->freq << DSOUND_FREQSHIFT) /
				wfex->nSamplesPerSec;

			LeaveCriticalSection(&((*dsb)->lock));
			/* **** */
		}
	}

done:
	LeaveCriticalSection(&(device->mixlock));
	RtlReleaseResource(&(device->buffer_list_lock));
	/* **** */

	return err;
}

static HRESULT WINAPI PrimaryBufferImpl_SetVolume(
	LPDIRECTSOUNDBUFFER8 iface,LONG vol
) {
	DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	DWORD ampfactors;
	DSVOLUMEPAN volpan;
        HRESULT hres = DS_OK;
	TRACE("(%p,%ld)\n", iface, vol);

	if (!(device->dsbd.dwFlags & DSBCAPS_CTRLVOLUME)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if ((vol > DSBVOLUME_MAX) || (vol < DSBVOLUME_MIN)) {
		WARN("invalid parameter: vol = %ld\n", vol);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&(device->mixlock));

        waveOutGetVolume(device->hwo, &ampfactors);
        volpan.dwTotalLeftAmpFactor=ampfactors & 0xffff;
        volpan.dwTotalRightAmpFactor=ampfactors >> 16;
        DSOUND_AmpFactorToVolPan(&volpan);
        if (vol != volpan.lVolume) {
            volpan.lVolume=vol;
            DSOUND_RecalcVolPan(&volpan);
            if (device->hwbuf) {
                hres = IDsDriverBuffer_SetVolumePan(device->hwbuf, &volpan);
                if (hres != DS_OK)
                    WARN("IDsDriverBuffer_SetVolumePan failed\n");
            } else {
                ampfactors = (volpan.dwTotalLeftAmpFactor & 0xffff) | (volpan.dwTotalRightAmpFactor << 16);
                waveOutSetVolume(device->hwo, ampfactors);
            }
        }

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	return hres;
}

static HRESULT WINAPI PrimaryBufferImpl_GetVolume(
	LPDIRECTSOUNDBUFFER8 iface,LPLONG vol
) {
	DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	DWORD ampfactors;
	DSVOLUMEPAN volpan;
	TRACE("(%p,%p)\n", iface, vol);

	if (!(device->dsbd.dwFlags & DSBCAPS_CTRLVOLUME)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (vol == NULL) {
		WARN("invalid parameter: vol = NULL\n");
		return DSERR_INVALIDPARAM;
	}

	waveOutGetVolume(device->hwo, &ampfactors);
	volpan.dwTotalLeftAmpFactor=ampfactors & 0xffff;
	volpan.dwTotalRightAmpFactor=ampfactors >> 16;
	DSOUND_AmpFactorToVolPan(&volpan);
	*vol = volpan.lVolume;
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_SetFrequency(
	LPDIRECTSOUNDBUFFER8 iface,DWORD freq
) {
	PrimaryBufferImpl *This = (PrimaryBufferImpl *)iface;
	TRACE("(%p,%ld)\n",This,freq);

	/* You cannot set the frequency of the primary buffer */
	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI PrimaryBufferImpl_Play(
	LPDIRECTSOUNDBUFFER8 iface,DWORD reserved1,DWORD reserved2,DWORD flags
) {
	DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	TRACE("(%p,%08lx,%08lx,%08lx)\n", iface, reserved1, reserved2, flags);

	if (!(flags & DSBPLAY_LOOPING)) {
		WARN("invalid parameter: flags = %08lx\n", flags);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&(device->mixlock));

	if (device->state == STATE_STOPPED)
		device->state = STATE_STARTING;
	else if (device->state == STATE_STOPPING)
		device->state = STATE_PLAYING;

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Stop(LPDIRECTSOUNDBUFFER8 iface)
{
	DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	TRACE("(%p)\n", iface);

	/* **** */
	EnterCriticalSection(&(device->mixlock));

	if (device->state == STATE_PLAYING)
		device->state = STATE_STOPPING;
	else if (device->state == STATE_STARTING)
		device->state = STATE_STOPPED;

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	return DS_OK;
}

static ULONG WINAPI PrimaryBufferImpl_AddRef(LPDIRECTSOUNDBUFFER8 iface)
{
    PrimaryBufferImpl *This = (PrimaryBufferImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI PrimaryBufferImpl_Release(LPDIRECTSOUNDBUFFER8 iface)
{
    PrimaryBufferImpl *This = (PrimaryBufferImpl *)iface;
    DWORD ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref + 1);

    if (!ref) {
        This->dsound->device->primary = NULL;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI PrimaryBufferImpl_GetCurrentPosition(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD playpos,LPDWORD writepos
) {
	HRESULT	hres;
        DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	TRACE("(%p,%p,%p)\n", iface, playpos, writepos);

	hres = DSOUND_PrimaryGetPosition(device, playpos, writepos);
	if (hres != DS_OK) {
		WARN("DSOUND_PrimaryGetPosition failed\n");
		return hres;
	}
	if (writepos) {
		if (device->state != STATE_STOPPED)
			/* apply the documented 10ms lead to writepos */
			*writepos += device->writelead;
		while (*writepos >= device->buflen) *writepos -= device->buflen;
	}
	TRACE("playpos = %ld, writepos = %ld (%p, time=%ld)\n", playpos?*playpos:0, writepos?*writepos:0, device, GetTickCount());
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetStatus(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD status
) {
        DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	TRACE("(%p,%p)\n", iface, status);

	if (status == NULL) {
		WARN("invalid parameter: status == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*status = 0;
	if ((device->state == STATE_STARTING) ||
	    (device->state == STATE_PLAYING))
		*status |= DSBSTATUS_PLAYING | DSBSTATUS_LOOPING;

	TRACE("status=%lx\n", *status);
	return DS_OK;
}


static HRESULT WINAPI PrimaryBufferImpl_GetFormat(
    LPDIRECTSOUNDBUFFER8 iface,
    LPWAVEFORMATEX lpwf,
    DWORD wfsize,
    LPDWORD wfwritten)
{
    DWORD size;
    DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
    TRACE("(%p,%p,%ld,%p)\n", iface, lpwf, wfsize, wfwritten);

    size = sizeof(WAVEFORMATEX) + device->pwfx->cbSize;

    if (lpwf) {	/* NULL is valid */
        if (wfsize >= size) {
            CopyMemory(lpwf,device->pwfx,size);
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
            *wfwritten = sizeof(WAVEFORMATEX) + device->pwfx->cbSize;
        else {
            WARN("invalid parameter: wfwritten == NULL\n");
            return DSERR_INVALIDPARAM;
        }
    }

    return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Lock(
	LPDIRECTSOUNDBUFFER8 iface,DWORD writecursor,DWORD writebytes,LPVOID lplpaudioptr1,LPDWORD audiobytes1,LPVOID lplpaudioptr2,LPDWORD audiobytes2,DWORD flags
) {
        DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	TRACE("(%p,%ld,%ld,%p,%p,%p,%p,0x%08lx) at %ld\n",
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

	if (device->priolevel != DSSCL_WRITEPRIMARY) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

	if (flags & DSBLOCK_FROMWRITECURSOR) {
		DWORD writepos;
		HRESULT hres;
		/* GetCurrentPosition does too much magic to duplicate here */
		hres = IDirectSoundBuffer_GetCurrentPosition(iface, NULL, &writepos);
		if (hres != DS_OK) {
			WARN("IDirectSoundBuffer_GetCurrentPosition failed\n");
			return hres;
		}
		writecursor += writepos;
	}
	while (writecursor >= device->buflen)
		writecursor -= device->buflen;
	if (flags & DSBLOCK_ENTIREBUFFER)
		writebytes = device->buflen;
	if (writebytes > device->buflen)
		writebytes = device->buflen;

	if (!(device->drvdesc.dwFlags & DSDDESC_DONTNEEDPRIMARYLOCK) && device->hwbuf) {
		HRESULT hres;
		hres = IDsDriverBuffer_Lock(device->hwbuf,
					    lplpaudioptr1, audiobytes1,
					    lplpaudioptr2, audiobytes2,
					    writecursor, writebytes,
					    0);
		if (hres != DS_OK) {
			WARN("IDsDriverBuffer_Lock failed\n");
			return hres;
		}
	} else {
		if (writecursor+writebytes <= device->buflen) {
			*(LPBYTE*)lplpaudioptr1 = device->buffer+writecursor;
			*audiobytes1 = writebytes;
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = NULL;
			if (audiobytes2)
				*audiobytes2 = 0;
			TRACE("->%ld.0\n",writebytes);
		} else {
			*(LPBYTE*)lplpaudioptr1 = device->buffer+writecursor;
			*audiobytes1 = device->buflen-writecursor;
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = device->buffer;
			if (audiobytes2)
				*audiobytes2 = writebytes-(device->buflen-writecursor);
			TRACE("->%ld.%ld\n",*audiobytes1,audiobytes2?*audiobytes2:0);
		}
	}
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_SetCurrentPosition(
	LPDIRECTSOUNDBUFFER8 iface,DWORD newpos
) {
	PrimaryBufferImpl *This = (PrimaryBufferImpl *)iface;
	TRACE("(%p,%ld)\n",This,newpos);

	/* You cannot set the position of the primary buffer */
	WARN("invalid call\n");
	return DSERR_INVALIDCALL;
}

static HRESULT WINAPI PrimaryBufferImpl_SetPan(
	LPDIRECTSOUNDBUFFER8 iface,LONG pan
) {
        DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	DWORD ampfactors;
	DSVOLUMEPAN volpan;
        HRESULT hres = DS_OK;
	TRACE("(%p,%ld)\n", iface, pan);

	if (!(device->dsbd.dwFlags & DSBCAPS_CTRLPAN)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if ((pan > DSBPAN_RIGHT) || (pan < DSBPAN_LEFT)) {
		WARN("invalid parameter: pan = %ld\n", pan);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&(device->mixlock));

        waveOutGetVolume(device->hwo, &ampfactors);
        volpan.dwTotalLeftAmpFactor=ampfactors & 0xffff;
        volpan.dwTotalRightAmpFactor=ampfactors >> 16;
        DSOUND_AmpFactorToVolPan(&volpan);
        if (pan != volpan.lPan) {
            volpan.lPan=pan;
            DSOUND_RecalcVolPan(&volpan);
            if (device->hwbuf) {
                hres = IDsDriverBuffer_SetVolumePan(device->hwbuf, &volpan);
                if (hres != DS_OK)
                    WARN("IDsDriverBuffer_SetVolumePan failed\n");
            } else {
                ampfactors = (volpan.dwTotalLeftAmpFactor & 0xffff) | (volpan.dwTotalRightAmpFactor << 16);
                waveOutSetVolume(device->hwo, ampfactors);
            }
        }

	LeaveCriticalSection(&(device->mixlock));
	/* **** */

	return hres;
}

static HRESULT WINAPI PrimaryBufferImpl_GetPan(
	LPDIRECTSOUNDBUFFER8 iface,LPLONG pan
) {
        DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	DWORD ampfactors;
	DSVOLUMEPAN volpan;
	TRACE("(%p,%p)\n", iface, pan);

	if (!(device->dsbd.dwFlags & DSBCAPS_CTRLPAN)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (pan == NULL) {
		WARN("invalid parameter: pan == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	waveOutGetVolume(device->hwo, &ampfactors);
	volpan.dwTotalLeftAmpFactor=ampfactors & 0xffff;
	volpan.dwTotalRightAmpFactor=ampfactors >> 16;
	DSOUND_AmpFactorToVolPan(&volpan);
	*pan = volpan.lPan;
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Unlock(
	LPDIRECTSOUNDBUFFER8 iface,LPVOID p1,DWORD x1,LPVOID p2,DWORD x2
) {
        DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	TRACE("(%p,%p,%ld,%p,%ld)\n", iface, p1, x1, p2, x2);

	if (device->priolevel != DSSCL_WRITEPRIMARY) {
		WARN("failed priority check!\n");
		return DSERR_PRIOLEVELNEEDED;
	}

	if (!(device->drvdesc.dwFlags & DSDDESC_DONTNEEDPRIMARYLOCK) && device->hwbuf) {
		HRESULT	hres;
		
		hres = IDsDriverBuffer_Unlock(device->hwbuf, p1, x1, p2, x2);
		if (hres != DS_OK) {
			WARN("IDsDriverBuffer_Unlock failed\n");
			return hres;
		}
	}

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_Restore(
	LPDIRECTSOUNDBUFFER8 iface
) {
	PrimaryBufferImpl *This = (PrimaryBufferImpl *)iface;
	FIXME("(%p):stub\n",This);
	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_GetFrequency(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD freq
) {
        DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
	TRACE("(%p,%p)\n", iface, freq);

	if (freq == NULL) {
		WARN("invalid parameter: freq == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (!(device->dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	*freq = device->pwfx->nSamplesPerSec;
	TRACE("-> %ld\n", *freq);

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_SetFX(
	LPDIRECTSOUNDBUFFER8 iface,DWORD dwEffectsCount,LPDSEFFECTDESC pDSFXDesc,LPDWORD pdwResultCodes
) {
	PrimaryBufferImpl *This = (PrimaryBufferImpl *)iface;
	DWORD u;
	FIXME("(%p,%lu,%p,%p): stub\n",This,dwEffectsCount,pDSFXDesc,pdwResultCodes);

	if (pdwResultCodes)
		for (u=0; u<dwEffectsCount; u++) pdwResultCodes[u] = DSFXR_UNKNOWN;

	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI PrimaryBufferImpl_AcquireResources(
	LPDIRECTSOUNDBUFFER8 iface,DWORD dwFlags,DWORD dwEffectsCount,LPDWORD pdwResultCodes
) {
	PrimaryBufferImpl *This = (PrimaryBufferImpl *)iface;
	DWORD u;
	FIXME("(%p,%08lu,%lu,%p): stub\n",This,dwFlags,dwEffectsCount,pdwResultCodes);

	if (pdwResultCodes)
		for (u=0; u<dwEffectsCount; u++) pdwResultCodes[u] = DSFXR_UNKNOWN;

	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI PrimaryBufferImpl_GetObjectInPath(
	LPDIRECTSOUNDBUFFER8 iface,REFGUID rguidObject,DWORD dwIndex,REFGUID rguidInterface,LPVOID* ppObject
) {
	PrimaryBufferImpl *This = (PrimaryBufferImpl *)iface;
	FIXME("(%p,%s,%lu,%s,%p): stub\n",This,debugstr_guid(rguidObject),dwIndex,debugstr_guid(rguidInterface),ppObject);

	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI PrimaryBufferImpl_Initialize(
	LPDIRECTSOUNDBUFFER8 iface,LPDIRECTSOUND dsound,LPCDSBUFFERDESC dbsd
) {
	PrimaryBufferImpl *This = (PrimaryBufferImpl *)iface;
	FIXME("(%p,%p,%p):stub\n",This,dsound,dbsd);
	DPRINTF("Re-Init!!!\n");
	WARN("already initialized\n");
	return DSERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI PrimaryBufferImpl_GetCaps(
	LPDIRECTSOUNDBUFFER8 iface,LPDSBCAPS caps
) {
        DirectSoundDevice *device = ((PrimaryBufferImpl *)iface)->dsound->device;
  	TRACE("(%p,%p)\n", iface, caps);

	if (caps == NULL) {
		WARN("invalid parameter: caps == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (caps->dwSize < sizeof(*caps)) {
		WARN("invalid parameter: caps->dwSize = %ld: < %d\n", caps->dwSize, sizeof(*caps));
		return DSERR_INVALIDPARAM;
	}

	caps->dwFlags = device->dsbd.dwFlags;
	if (device->hwbuf) caps->dwFlags |= DSBCAPS_LOCHARDWARE;
	else caps->dwFlags |= DSBCAPS_LOCSOFTWARE;

	caps->dwBufferBytes = device->buflen;

	/* This value represents the speed of the "unlock" command.
	   As unlock is quite fast (it does not do anything), I put
	   4096 ko/s = 4 Mo / s */
	/* FIXME: hwbuf speed */
	caps->dwUnlockTransferRate = 4096;
	caps->dwPlayCpuOverhead = 0;

	return DS_OK;
}

static HRESULT WINAPI PrimaryBufferImpl_QueryInterface(
	LPDIRECTSOUNDBUFFER8 iface,REFIID riid,LPVOID *ppobj
) {
        PrimaryBufferImpl *This = (PrimaryBufferImpl *)iface;
        DirectSoundDevice *device = This->dsound->device;
	TRACE("(%p,%s,%p)\n", iface, debugstr_guid(riid), ppobj);

	if (ppobj == NULL) {
		WARN("invalid parameter\n");
		return E_INVALIDARG;
	}

	*ppobj = NULL;	/* assume failure */

	if ( IsEqualGUID(riid, &IID_IUnknown) ||
	     IsEqualGUID(riid, &IID_IDirectSoundBuffer) ) {
		IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER)This);
		*ppobj = This;
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
		if (!device->listener)
			IDirectSound3DListenerImpl_Create(This, &device->listener);
		if (device->listener) {
			*ppobj = device->listener;
			IDirectSound3DListener_AddRef((LPDIRECTSOUND3DLISTENER)*ppobj);
			return S_OK;
		}

		WARN("IID_IDirectSound3DListener failed\n");
		return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IKsPropertySet, riid ) ) {
		FIXME("app requested IKsPropertySet on primary buffer\n");
		return E_NOINTERFACE;
	}

	FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );
	return E_NOINTERFACE;
}

static const IDirectSoundBuffer8Vtbl dspbvt =
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
	PrimaryBufferImpl_Restore,
	PrimaryBufferImpl_SetFX,
	PrimaryBufferImpl_AcquireResources,
	PrimaryBufferImpl_GetObjectInPath
};

HRESULT WINAPI PrimaryBufferImpl_Create(
	IDirectSoundImpl *ds,
	PrimaryBufferImpl **pdsb,
	LPCDSBUFFERDESC dsbd)
{
	PrimaryBufferImpl *dsb;
	TRACE("%p,%p,%p)\n",ds,pdsb,dsbd);

	if (dsbd->lpwfxFormat) {
		WARN("invalid parameter: dsbd->lpwfxFormat != NULL\n");
		*pdsb = NULL;
		return DSERR_INVALIDPARAM;
	}

	dsb = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*dsb));

	if (dsb == NULL) {
		WARN("out of memory\n");
		*pdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}

	dsb->ref = 0;
	dsb->dsound = ds;
	dsb->lpVtbl = &dspbvt;

	CopyMemory(&ds->device->dsbd, dsbd, sizeof(*dsbd));

	TRACE("Created primary buffer at %p\n", dsb);
	TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld,"
		"bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
		ds->device->pwfx->wFormatTag, ds->device->pwfx->nChannels, ds->device->pwfx->nSamplesPerSec,
		ds->device->pwfx->nAvgBytesPerSec, ds->device->pwfx->nBlockAlign,
		ds->device->pwfx->wBitsPerSample, ds->device->pwfx->cbSize);

	*pdsb = dsb;
	return S_OK;
}
