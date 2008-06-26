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
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

/*******************************************************************************
 *		IDirectSoundNotify
 */
static HRESULT WINAPI IDirectSoundNotifyImpl_QueryInterface(
	LPDIRECTSOUNDNOTIFY iface,REFIID riid,LPVOID *ppobj
) {
	IDirectSoundNotifyImpl *This = (IDirectSoundNotifyImpl *)iface;
	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

	if (This->dsb == NULL) {
		WARN("invalid parameter\n");
		return E_INVALIDARG;
	}

	return IDirectSoundBuffer_QueryInterface((LPDIRECTSOUNDBUFFER)This->dsb, riid, ppobj);
}

static ULONG WINAPI IDirectSoundNotifyImpl_AddRef(LPDIRECTSOUNDNOTIFY iface)
{
    IDirectSoundNotifyImpl *This = (IDirectSoundNotifyImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSoundNotifyImpl_Release(LPDIRECTSOUNDNOTIFY iface)
{
    IDirectSoundNotifyImpl *This = (IDirectSoundNotifyImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref + 1);

    if (!ref) {
        IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER)This->dsb);
        This->dsb->notify = NULL;
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSoundNotifyImpl_SetNotificationPositions(
	LPDIRECTSOUNDNOTIFY iface,DWORD howmuch,LPCDSBPOSITIONNOTIFY notify
) {
	IDirectSoundNotifyImpl *This = (IDirectSoundNotifyImpl *)iface;
	TRACE("(%p,0x%08lx,%p)\n",This,howmuch,notify);

        if (howmuch > 0 && notify == NULL) {
	    WARN("invalid parameter: notify == NULL\n");
	    return DSERR_INVALIDPARAM;
	}

	if (TRACE_ON(dsound)) {
	    unsigned int	i;
	    for (i=0;i<howmuch;i++)
		TRACE("notify at %ld to %p\n",
		    notify[i].dwOffset,notify[i].hEventNotify);
	}

	if (This->dsb->hwnotify) {
	    HRESULT hres;
	    hres = IDsDriverNotify_SetNotificationPositions(This->dsb->hwnotify, howmuch, notify);
	    if (hres != DS_OK)
		    WARN("IDsDriverNotify_SetNotificationPositions failed\n");
	    return hres;
        } else if (howmuch > 0) {
	    /* Make an internal copy of the caller-supplied array.
	     * Replace the existing copy if one is already present. */
	    if (This->dsb->notifies)
		    This->dsb->notifies = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			This->dsb->notifies, howmuch * sizeof(DSBPOSITIONNOTIFY));
	    else
		    This->dsb->notifies = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			howmuch * sizeof(DSBPOSITIONNOTIFY));

	    if (This->dsb->notifies == NULL) {
		    WARN("out of memory\n");
		    return DSERR_OUTOFMEMORY;
	    }
	    CopyMemory(This->dsb->notifies, notify, howmuch * sizeof(DSBPOSITIONNOTIFY));
	    This->dsb->nrofnotifies = howmuch;
        } else {
           HeapFree(GetProcessHeap(), 0, This->dsb->notifies);
           This->dsb->notifies = NULL;
           This->dsb->nrofnotifies = 0;
        }

	return S_OK;
}

static const IDirectSoundNotifyVtbl dsnvt =
{
    IDirectSoundNotifyImpl_QueryInterface,
    IDirectSoundNotifyImpl_AddRef,
    IDirectSoundNotifyImpl_Release,
    IDirectSoundNotifyImpl_SetNotificationPositions,
};

HRESULT WINAPI IDirectSoundNotifyImpl_Create(
    IDirectSoundBufferImpl * dsb,
    IDirectSoundNotifyImpl **pdsn)
{
    IDirectSoundNotifyImpl * dsn;
    TRACE("(%p,%p)\n",dsb,pdsn);

    dsn = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(dsn));

    if (dsn == NULL) {
        WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    dsn->ref = 0;
    dsn->lpVtbl = &dsnvt;
    dsn->dsb = dsb;
    dsb->notify = dsn;
    IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER)dsb);

    *pdsn = dsn;
    return DS_OK;
}

HRESULT WINAPI IDirectSoundNotifyImpl_Destroy(
    IDirectSoundNotifyImpl *pdsn)
{
    TRACE("(%p)\n",pdsn);

    while (IDirectSoundNotifyImpl_Release((LPDIRECTSOUNDNOTIFY)pdsn) > 0);

    return DS_OK;
}

/*******************************************************************************
 *		IDirectSoundBuffer
 */

static HRESULT WINAPI IDirectSoundBufferImpl_SetFormat(
	LPDIRECTSOUNDBUFFER8 iface,LPCWAVEFORMATEX wfex
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;

	TRACE("(%p,%p)\n",This,wfex);
	/* This method is not available on secondary buffers */
	WARN("invalid call\n");
	return DSERR_INVALIDCALL;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetVolume(
	LPDIRECTSOUNDBUFFER8 iface,LONG vol
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	LONG oldVol;
	HRESULT hres = DS_OK;

	TRACE("(%p,%ld)\n",This,vol);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLVOLUME)) {
		WARN("control unavailable: This->dsbd.dwFlags = 0x%08lx\n", This->dsbd.dwFlags);
		return DSERR_CONTROLUNAVAIL;
	}

	if ((vol > DSBVOLUME_MAX) || (vol < DSBVOLUME_MIN)) {
		WARN("invalid parameter: vol = %ld\n", vol);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&(This->lock));

	if (This->dsbd.dwFlags & DSBCAPS_CTRL3D) {
		oldVol = This->ds3db_lVolume;
		This->ds3db_lVolume = vol;
	} else {
		oldVol = This->volpan.lVolume;
		This->volpan.lVolume = vol;
		if (vol != oldVol)
			DSOUND_RecalcVolPan(&(This->volpan));
	}

	if (vol != oldVol) {
		if (This->hwbuf) {
			hres = IDsDriverBuffer_SetVolumePan(This->hwbuf, &(This->volpan));
	    		if (hres != DS_OK)
		    		WARN("IDsDriverBuffer_SetVolumePan failed\n");
		} else
			DSOUND_ForceRemix(This);
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return hres;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetVolume(
	LPDIRECTSOUNDBUFFER8 iface,LPLONG vol
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	TRACE("(%p,%p)\n",This,vol);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLVOLUME)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (vol == NULL) {
		WARN("invalid parameter: vol == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*vol = This->volpan.lVolume;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetFrequency(
	LPDIRECTSOUNDBUFFER8 iface,DWORD freq
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	DWORD oldFreq;

	TRACE("(%p,%ld)\n",This,freq);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (freq == DSBFREQUENCY_ORIGINAL)
		freq = This->pwfx->nSamplesPerSec;

	if ((freq < DSBFREQUENCY_MIN) || (freq > DSBFREQUENCY_MAX)) {
		WARN("invalid parameter: freq = %ld\n", freq);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	EnterCriticalSection(&(This->lock));

	oldFreq = This->freq;
	This->freq = freq;
	if (freq != oldFreq) {
		This->freqAdjust = (freq << DSOUND_FREQSHIFT) / This->dsound->device->pwfx->nSamplesPerSec;
		This->nAvgBytesPerSec = freq * This->pwfx->nBlockAlign;
		DSOUND_RecalcFormat(This);
		if (!This->hwbuf)
			DSOUND_ForceRemix(This);
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Play(
	LPDIRECTSOUNDBUFFER8 iface,DWORD reserved1,DWORD reserved2,DWORD flags
) {
	HRESULT hres = DS_OK;
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	TRACE("(%p,%08lx,%08lx,%08lx)\n",This,reserved1,reserved2,flags);

	/* **** */
	EnterCriticalSection(&(This->lock));

	This->playflags = flags;
	if (This->state == STATE_STOPPED) {
		This->leadin = TRUE;
		This->startpos = This->buf_mixpos;
		This->state = STATE_STARTING;
	} else if (This->state == STATE_STOPPING)
		This->state = STATE_PLAYING;
	if (This->hwbuf) {
		hres = IDsDriverBuffer_Play(This->hwbuf, 0, 0, This->playflags);
		if (hres != DS_OK)
			WARN("IDsDriverBuffer_Play failed\n");
		else
			This->state = STATE_PLAYING;
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return hres;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Stop(LPDIRECTSOUNDBUFFER8 iface)
{
	HRESULT hres = DS_OK;
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	TRACE("(%p)\n",This);

	/* **** */
	EnterCriticalSection(&(This->lock));

	if (This->state == STATE_PLAYING)
		This->state = STATE_STOPPING;
	else if (This->state == STATE_STARTING)
		This->state = STATE_STOPPED;
	if (This->hwbuf) {
		hres = IDsDriverBuffer_Stop(This->hwbuf);
		if (hres != DS_OK)
			WARN("IDsDriverBuffer_Stop failed\n");
		else
			This->state = STATE_STOPPED;
	}
	DSOUND_CheckEvent(This, 0);

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return hres;
}

static ULONG WINAPI IDirectSoundBufferImpl_AddRef(LPDIRECTSOUNDBUFFER8 iface)
{
    IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSoundBufferImpl_Release(LPDIRECTSOUNDBUFFER8 iface)
{
    IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref + 1);

    if (!ref) {
	DSOUND_RemoveBuffer(This->dsound, This);

	This->lock.DebugInfo->Spare[0] = 0;
	DeleteCriticalSection(&(This->lock));

	if (This->hwbuf) {
		IDsDriverBuffer_Release(This->hwbuf);
		if (This->dsound->device->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY) {
			This->buffer->ref--;
			if (This->buffer->ref==0) {
				HeapFree(GetProcessHeap(),0,This->buffer->memory);
				HeapFree(GetProcessHeap(),0,This->buffer);
			}
		}
	} else {
		This->buffer->ref--;
		if (This->buffer->ref==0) {
			HeapFree(GetProcessHeap(),0,This->buffer->memory);
			HeapFree(GetProcessHeap(),0,This->buffer);
		}
	}

	HeapFree(GetProcessHeap(), 0, This->notifies);
	HeapFree(GetProcessHeap(), 0, This->pwfx);
	HeapFree(GetProcessHeap(), 0, This);

	TRACE("(%p) released\n", This);
    }
    return ref;
}

DWORD DSOUND_CalcPlayPosition(IDirectSoundBufferImpl *This, DWORD pplay, DWORD pwrite)
{
	DWORD bplay = This->buf_mixpos;
	DWORD pmix = This->primary_mixpos;
	TRACE("(%p, pplay=%lu, pwrite=%lu)\n", This, pplay, pwrite);

	/* the actual primary play position (pplay) is always behind last mixed (pmix),
	 * unless the computer is too slow or something */
	/* we need to know how far away we are from there */
	if (pmix < pplay) pmix += This->dsound->device->buflen; /* wraparound */
	pmix -= pplay;
	/* detect buffer underrun */
	if (pwrite < pplay) pwrite += This->dsound->device->buflen; /* wraparound */
	pwrite -= pplay;
	if (pmix > (ds_snd_queue_max * This->dsound->device->fraglen + pwrite + This->dsound->device->writelead)) {
		WARN("detected an underrun: primary queue was %ld\n",pmix);
		pmix = 0;
	}
	/* divide the offset by its sample size */
	pmix /= This->dsound->device->pwfx->nBlockAlign;
	TRACE("primary back-samples=%ld\n",pmix);
	/* adjust for our frequency */
	pmix = (pmix * This->freqAdjust) >> DSOUND_FREQSHIFT;
	/* multiply by our own sample size */
	pmix *= This->pwfx->nBlockAlign;
	TRACE("this back-offset=%ld\n", pmix);
	/* subtract from our last mixed position */
	while (bplay < pmix) bplay += This->buflen; /* wraparound */
	bplay -= pmix;
	if (This->leadin && ((bplay < This->startpos) || (bplay > This->buf_mixpos))) {
		/* seems we haven't started playing yet */
		TRACE("this still in lead-in phase\n");
		bplay = This->startpos;
	}
	/* return the result */
	return bplay;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetCurrentPosition(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD playpos,LPDWORD writepos
) {
	HRESULT	hres;
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	TRACE("(%p,%p,%p)\n",This,playpos,writepos);
	if (This->hwbuf) {
		hres=IDsDriverBuffer_GetPosition(This->hwbuf,playpos,writepos);
		if (hres != DS_OK) {
		    WARN("IDsDriverBuffer_GetPosition failed\n");
		    return hres;
		}
	} else {
		if (playpos && (This->state != STATE_PLAYING)) {
			/* we haven't been merged into the primary buffer (yet) */
			*playpos = This->buf_mixpos;
		} else if (playpos) {
			DWORD pplay, pwrite;
			/* let's get this exact; first, recursively call GetPosition on the primary */
			EnterCriticalSection(&(This->dsound->device->mixlock));
			if (DSOUND_PrimaryGetPosition(This->dsound->device, &pplay, &pwrite) != DS_OK)
				WARN("DSOUND_PrimaryGetPosition failed\n");
			/* detect HEL mode underrun */
			if (!(This->dsound->device->hwbuf || This->dsound->device->pwqueue))
				TRACE("detected an underrun\n");
			if ((This->dsbd.dwFlags & DSBCAPS_GETCURRENTPOSITION2) || This->dsound->device->hwbuf) {
				/* calculate play position using this */
				*playpos = DSOUND_CalcPlayPosition(This, pplay, pwrite);
			} else {
				/* (unless the app isn't using GETCURRENTPOSITION2) */
				/* don't know exactly how this should be handled...
				 * the docs says that play cursor is reported as directly
				 * behind write cursor, hmm... */
				/* let's just do what might work for Half-Life */
				DWORD wp;
				wp = (This->dsound->device->pwplay + ds_hel_margin) * This->dsound->device->fraglen;
				wp %= This->dsound->device->buflen;
				*playpos = DSOUND_CalcPlayPosition(This, wp, pwrite);
			}
			LeaveCriticalSection(&(This->dsound->device->mixlock));
		}
		if (writepos)
                    *writepos = This->buf_mixpos;
	}
	if (writepos) {
		if (This->state != STATE_STOPPED) {
			/* apply the documented 10ms lead to writepos */
			*writepos += This->writelead;
		}
		*writepos %= This->buflen;
	}
	if (playpos)
            This->last_playpos = *playpos;
	TRACE("playpos = %ld, writepos = %ld (%p, time=%ld)\n", playpos?*playpos:0, writepos?*writepos:0, This, GetTickCount());
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetStatus(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD status
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	TRACE("(%p,%p), thread is %04lx\n",This,status,GetCurrentThreadId());

	if (status == NULL) {
		WARN("invalid parameter: status = NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*status = 0;
	if ((This->state == STATE_STARTING) || (This->state == STATE_PLAYING)) {
		*status |= DSBSTATUS_PLAYING;
		if (This->playflags & DSBPLAY_LOOPING)
			*status |= DSBSTATUS_LOOPING;
	}

	TRACE("status=%lx\n", *status);
	return DS_OK;
}


static HRESULT WINAPI IDirectSoundBufferImpl_GetFormat(
    LPDIRECTSOUNDBUFFER8 iface,
    LPWAVEFORMATEX lpwf,
    DWORD wfsize,
    LPDWORD wfwritten)
{
    DWORD size;
    IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
    TRACE("(%p,%p,%ld,%p)\n",This,lpwf,wfsize,wfwritten);

    size = sizeof(WAVEFORMATEX) + This->pwfx->cbSize;

    if (lpwf) { /* NULL is valid */
        if (wfsize >= size) {
            CopyMemory(lpwf,This->pwfx,size);
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
            *wfwritten = sizeof(WAVEFORMATEX) + This->pwfx->cbSize;
        else {
            WARN("invalid parameter: wfwritten == NULL\n");
            return DSERR_INVALIDPARAM;
        }
    }

    return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Lock(
	LPDIRECTSOUNDBUFFER8 iface,DWORD writecursor,DWORD writebytes,LPVOID lplpaudioptr1,LPDWORD audiobytes1,LPVOID lplpaudioptr2,LPDWORD audiobytes2,DWORD flags
) {
	HRESULT hres = DS_OK;
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;

	TRACE("(%p,%ld,%ld,%p,%p,%p,%p,0x%08lx) at %ld\n",
		This,
		writecursor,
		writebytes,
		lplpaudioptr1,
		audiobytes1,
		lplpaudioptr2,
		audiobytes2,
		flags,
		GetTickCount()
	);

	if (flags & DSBLOCK_FROMWRITECURSOR) {
		DWORD writepos;
		/* GetCurrentPosition does too much magic to duplicate here */
		hres = IDirectSoundBufferImpl_GetCurrentPosition(iface, NULL, &writepos);
		if (hres != DS_OK) {
			WARN("IDirectSoundBufferImpl_GetCurrentPosition failed\n");
			return hres;
		}
		writecursor += writepos;
	}
	writecursor %= This->buflen;
	if (flags & DSBLOCK_ENTIREBUFFER)
		writebytes = This->buflen;
	if (writebytes > This->buflen)
		writebytes = This->buflen;

	EnterCriticalSection(&(This->lock));

	if ((writebytes == This->buflen) &&
	    ((This->state == STATE_STARTING) ||
	     (This->state == STATE_PLAYING)))
		/* some games, like Half-Life, try to be clever (not) and
		 * keep one secondary buffer, and mix sounds into it itself,
		 * locking the entire buffer every time... so we can just forget
		 * about tracking the last-written-to-position... */
		This->probably_valid_to = (DWORD)-1;
	else
		This->probably_valid_to = writecursor;

	if (!(This->dsound->device->drvdesc.dwFlags & DSDDESC_DONTNEEDSECONDARYLOCK) && This->hwbuf) {
		hres = IDsDriverBuffer_Lock(This->hwbuf,
				     lplpaudioptr1, audiobytes1,
				     lplpaudioptr2, audiobytes2,
				     writecursor, writebytes,
				     0);
		if (hres != DS_OK) {
			WARN("IDsDriverBuffer_Lock failed\n");
			LeaveCriticalSection(&(This->lock));
			return hres;
		}
	} else {
		BOOL remix = FALSE;
		if (writecursor+writebytes <= This->buflen) {
			*(LPBYTE*)lplpaudioptr1 = This->buffer->memory+writecursor;
			*audiobytes1 = writebytes;
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = NULL;
			if (audiobytes2)
				*audiobytes2 = 0;
			TRACE("->%ld.0\n",writebytes);
		} else {
			*(LPBYTE*)lplpaudioptr1 = This->buffer->memory+writecursor;
			*audiobytes1 = This->buflen-writecursor;
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = This->buffer->memory;
			if (audiobytes2)
				*audiobytes2 = writebytes-(This->buflen-writecursor);
			TRACE("->%ld.%ld\n",*audiobytes1,audiobytes2?*audiobytes2:0);
		}
		if (This->state == STATE_PLAYING) {
			/* if the segment between playpos and buf_mixpos is touched,
			 * we need to cancel some mixing */
			/* we'll assume that the app always calls GetCurrentPosition before
			 * locking a playing buffer, so that last_playpos is up-to-date */
			if (This->buf_mixpos >= This->last_playpos) {
				if (This->buf_mixpos > writecursor &&
				    This->last_playpos < writecursor+writebytes)
					remix = TRUE;
			} else {
				if (This->buf_mixpos > writecursor ||
				    This->last_playpos < writecursor+writebytes)
					remix = TRUE;
			}
			if (remix) {
				TRACE("locking prebuffered region, ouch\n");
				DSOUND_MixCancelAt(This, writecursor);
			}
		}
	}

	LeaveCriticalSection(&(This->lock));
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetCurrentPosition(
	LPDIRECTSOUNDBUFFER8 iface,DWORD newpos
) {
	HRESULT hres = DS_OK;
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	TRACE("(%p,%ld)\n",This,newpos);

	/* **** */
	EnterCriticalSection(&(This->lock));

	newpos %= This->buflen;
	This->buf_mixpos = newpos;
	if (This->hwbuf) {
		hres = IDsDriverBuffer_SetPosition(This->hwbuf, This->buf_mixpos);
		if (hres != DS_OK)
			WARN("IDsDriverBuffer_SetPosition failed\n");
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return hres;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetPan(
	LPDIRECTSOUNDBUFFER8 iface,LONG pan
) {
	HRESULT hres = DS_OK;
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;

	TRACE("(%p,%ld)\n",This,pan);

	if ((pan > DSBPAN_RIGHT) || (pan < DSBPAN_LEFT)) {
		WARN("invalid parameter: pan = %ld\n", pan);
		return DSERR_INVALIDPARAM;
	}

	/* You cannot use both pan and 3D controls */
	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (This->dsbd.dwFlags & DSBCAPS_CTRL3D)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	/* **** */
	EnterCriticalSection(&(This->lock));

	if (This->volpan.lPan != pan) {
		This->volpan.lPan = pan;
		DSOUND_RecalcVolPan(&(This->volpan));

		if (This->hwbuf) {
			hres = IDsDriverBuffer_SetVolumePan(This->hwbuf, &(This->volpan));
			if (hres != DS_OK)
				WARN("IDsDriverBuffer_SetVolumePan failed\n");
		} else
			DSOUND_ForceRemix(This);
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	return hres;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetPan(
	LPDIRECTSOUNDBUFFER8 iface,LPLONG pan
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	TRACE("(%p,%p)\n",This,pan);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLPAN)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (pan == NULL) {
		WARN("invalid parameter: pan = NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*pan = This->volpan.lPan;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Unlock(
	LPDIRECTSOUNDBUFFER8 iface,LPVOID p1,DWORD x1,LPVOID p2,DWORD x2
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	DWORD probably_valid_to;
	HRESULT hres = DS_OK;

	TRACE("(%p,%p,%ld,%p,%ld)\n", This,p1,x1,p2,x2);

	/* **** */
	EnterCriticalSection(&(This->lock));

	if (!(This->dsound->device->drvdesc.dwFlags & DSDDESC_DONTNEEDSECONDARYLOCK) && This->hwbuf) {
		hres = IDsDriverBuffer_Unlock(This->hwbuf, p1, x1, p2, x2);
		if (hres != DS_OK)
			WARN("IDsDriverBuffer_Unlock failed\n");
	}

        if (hres == DS_OK) {
		if (p2) probably_valid_to = (((LPBYTE)p2)-This->buffer->memory) + x2;
		else probably_valid_to = (((LPBYTE)p1)-This->buffer->memory) + x1;
		probably_valid_to %= This->buflen;
		if ((probably_valid_to == 0) && ((x1+x2) == This->buflen) &&
		    ((This->state == STATE_STARTING) ||
		     (This->state == STATE_PLAYING)))
			/* see IDirectSoundBufferImpl_Lock */
			probably_valid_to = (DWORD)-1;
		This->probably_valid_to = probably_valid_to;
	}

	LeaveCriticalSection(&(This->lock));
	/* **** */

	TRACE("probably_valid_to=%ld\n", This->probably_valid_to);
	return hres;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Restore(
	LPDIRECTSOUNDBUFFER8 iface
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	FIXME("(%p):stub\n",This);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetFrequency(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD freq
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	TRACE("(%p,%p)\n",This,freq);

	if (freq == NULL) {
		WARN("invalid parameter: freq = NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*freq = This->freq;
	TRACE("-> %ld\n", *freq);

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetFX(
	LPDIRECTSOUNDBUFFER8 iface,DWORD dwEffectsCount,LPDSEFFECTDESC pDSFXDesc,LPDWORD pdwResultCodes
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	DWORD u;

	FIXME("(%p,%lu,%p,%p): stub\n",This,dwEffectsCount,pDSFXDesc,pdwResultCodes);

	if (pdwResultCodes)
		for (u=0; u<dwEffectsCount; u++) pdwResultCodes[u] = DSFXR_UNKNOWN;

	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI IDirectSoundBufferImpl_AcquireResources(
	LPDIRECTSOUNDBUFFER8 iface,DWORD dwFlags,DWORD dwEffectsCount,LPDWORD pdwResultCodes
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	DWORD u;

	FIXME("(%p,%08lu,%lu,%p): stub\n",This,dwFlags,dwEffectsCount,pdwResultCodes);

	if (pdwResultCodes)
		for (u=0; u<dwEffectsCount; u++) pdwResultCodes[u] = DSFXR_UNKNOWN;

	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetObjectInPath(
	LPDIRECTSOUNDBUFFER8 iface,REFGUID rguidObject,DWORD dwIndex,REFGUID rguidInterface,LPVOID* ppObject
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;

	FIXME("(%p,%s,%lu,%s,%p): stub\n",This,debugstr_guid(rguidObject),dwIndex,debugstr_guid(rguidInterface),ppObject);

	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Initialize(
	LPDIRECTSOUNDBUFFER8 iface,LPDIRECTSOUND dsound,LPCDSBUFFERDESC dbsd
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
	FIXME("(%p,%p,%p):stub\n",This,dsound,dbsd);
	DPRINTF("Re-Init!!!\n");
	WARN("already initialized\n");
	return DSERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetCaps(
	LPDIRECTSOUNDBUFFER8 iface,LPDSBCAPS caps
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
  	TRACE("(%p)->(%p)\n",This,caps);

	if (caps == NULL) {
		WARN("invalid parameter: caps == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (caps->dwSize < sizeof(*caps)) {
		WARN("invalid parameter: caps->dwSize = %ld < %d\n",caps->dwSize, sizeof(*caps));
		return DSERR_INVALIDPARAM;
	}

	caps->dwFlags = This->dsbd.dwFlags;
	if (This->hwbuf) caps->dwFlags |= DSBCAPS_LOCHARDWARE;
	else caps->dwFlags |= DSBCAPS_LOCSOFTWARE;

	caps->dwBufferBytes = This->buflen;

	/* This value represents the speed of the "unlock" command.
	   As unlock is quite fast (it does not do anything), I put
	   4096 ko/s = 4 Mo / s */
	/* FIXME: hwbuf speed */
	caps->dwUnlockTransferRate = 4096;
	caps->dwPlayCpuOverhead = 0;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_QueryInterface(
	LPDIRECTSOUNDBUFFER8 iface,REFIID riid,LPVOID *ppobj
) {
	IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

	if (ppobj == NULL) {
		WARN("invalid parameter\n");
		return E_INVALIDARG;
	}

	*ppobj = NULL;	/* assume failure */

	if ( IsEqualGUID(riid, &IID_IUnknown) ||
	     IsEqualGUID(riid, &IID_IDirectSoundBuffer) ||
	     IsEqualGUID(riid, &IID_IDirectSoundBuffer8) ) {
		if (!This->dsb)
			SecondaryBufferImpl_Create(This, &(This->dsb));
		if (This->dsb) {
			IDirectSoundBuffer8_AddRef((LPDIRECTSOUNDBUFFER8)This->dsb);
			*ppobj = This->dsb;
			return S_OK;
		}
		WARN("IID_IDirectSoundBuffer\n");
		return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IDirectSoundNotify, riid ) ) {
		if (!This->notify)
			IDirectSoundNotifyImpl_Create(This, &(This->notify));
		if (This->notify) {
			IDirectSoundNotify_AddRef((LPDIRECTSOUNDNOTIFY)This->notify);
			*ppobj = This->notify;
			return S_OK;
		}
		WARN("IID_IDirectSoundNotify\n");
		return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IDirectSound3DBuffer, riid ) ) {
		if (!This->ds3db)
			IDirectSound3DBufferImpl_Create(This, &(This->ds3db));
		if (This->ds3db) {
			IDirectSound3DBuffer_AddRef((LPDIRECTSOUND3DBUFFER)This->ds3db);
			*ppobj = This->ds3db;
			return S_OK;
		}
		WARN("IID_IDirectSound3DBuffer\n");
		return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IDirectSound3DListener, riid ) ) {
		ERR("app requested IDirectSound3DListener on secondary buffer\n");
		return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IKsPropertySet, riid ) ) {
		if (!This->iks)
			IKsBufferPropertySetImpl_Create(This, &(This->iks));
		if (This->iks) {
			IKsPropertySet_AddRef((LPKSPROPERTYSET)This->iks);
	    		*ppobj = This->iks;
			return S_OK;
		}
		WARN("IID_IKsPropertySet\n");
		return E_NOINTERFACE;
	}

	FIXME( "Unknown IID %s\n", debugstr_guid( riid ) );

	return E_NOINTERFACE;
}

static const IDirectSoundBuffer8Vtbl dsbvt =
{
	IDirectSoundBufferImpl_QueryInterface,
	IDirectSoundBufferImpl_AddRef,
	IDirectSoundBufferImpl_Release,
	IDirectSoundBufferImpl_GetCaps,
	IDirectSoundBufferImpl_GetCurrentPosition,
	IDirectSoundBufferImpl_GetFormat,
	IDirectSoundBufferImpl_GetVolume,
	IDirectSoundBufferImpl_GetPan,
	IDirectSoundBufferImpl_GetFrequency,
	IDirectSoundBufferImpl_GetStatus,
	IDirectSoundBufferImpl_Initialize,
	IDirectSoundBufferImpl_Lock,
	IDirectSoundBufferImpl_Play,
	IDirectSoundBufferImpl_SetCurrentPosition,
	IDirectSoundBufferImpl_SetFormat,
	IDirectSoundBufferImpl_SetVolume,
	IDirectSoundBufferImpl_SetPan,
	IDirectSoundBufferImpl_SetFrequency,
	IDirectSoundBufferImpl_Stop,
	IDirectSoundBufferImpl_Unlock,
	IDirectSoundBufferImpl_Restore,
	IDirectSoundBufferImpl_SetFX,
	IDirectSoundBufferImpl_AcquireResources,
	IDirectSoundBufferImpl_GetObjectInPath
};

HRESULT WINAPI IDirectSoundBufferImpl_Create(
	IDirectSoundImpl *ds,
	IDirectSoundBufferImpl **pdsb,
	LPCDSBUFFERDESC dsbd)
{
	IDirectSoundBufferImpl *dsb;
	LPWAVEFORMATEX wfex = dsbd->lpwfxFormat;
	HRESULT err = DS_OK;
	DWORD capf = 0;
	int use_hw, alloc_size, cp_size;
	TRACE("(%p,%p,%p)\n",ds,pdsb,dsbd);

	if (dsbd->dwBufferBytes < DSBSIZE_MIN || dsbd->dwBufferBytes > DSBSIZE_MAX) {
		WARN("invalid parameter: dsbd->dwBufferBytes = %ld\n", dsbd->dwBufferBytes);
		*pdsb = NULL;
		return DSERR_INVALIDPARAM; /* FIXME: which error? */
	}

	dsb = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*dsb));

	if (dsb == 0) {
		WARN("out of memory\n");
		*pdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}

	TRACE("Created buffer at %p\n", dsb);

	dsb->ref = 0;
	dsb->dsb = 0;
	dsb->dsound = ds;
	dsb->lpVtbl = &dsbvt;
	dsb->iks = NULL;

	/* size depends on version */
	CopyMemory(&dsb->dsbd, dsbd, dsbd->dwSize);

	/* variable sized struct so calculate size based on format */
	if (wfex->wFormatTag == WAVE_FORMAT_PCM) {
		alloc_size = sizeof(WAVEFORMATEX);
		cp_size = sizeof(PCMWAVEFORMAT);
	} else
		alloc_size = cp_size = sizeof(WAVEFORMATEX) + wfex->cbSize;

	dsb->pwfx = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,alloc_size);
	if (dsb->pwfx == NULL) {
		WARN("out of memory\n");
		HeapFree(GetProcessHeap(),0,dsb);
		*pdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}

	CopyMemory(dsb->pwfx, wfex, cp_size);

	if (dsbd->dwBufferBytes % dsbd->lpwfxFormat->nBlockAlign)
		dsb->buflen = dsbd->dwBufferBytes +
			(dsbd->lpwfxFormat->nBlockAlign -
			(dsbd->dwBufferBytes % dsbd->lpwfxFormat->nBlockAlign));
	else
		dsb->buflen = dsbd->dwBufferBytes;

	dsb->freq = dsbd->lpwfxFormat->nSamplesPerSec;
	dsb->notify = NULL;
	dsb->notifies = NULL;
	dsb->nrofnotifies = 0;
	dsb->hwnotify = 0;

	/* Check necessary hardware mixing capabilities */
	if (wfex->nChannels==2) capf |= DSCAPS_SECONDARYSTEREO;
	else capf |= DSCAPS_SECONDARYMONO;
	if (wfex->wBitsPerSample==16) capf |= DSCAPS_SECONDARY16BIT;
	else capf |= DSCAPS_SECONDARY8BIT;

	use_hw = (ds->device->drvcaps.dwFlags & capf) == capf;
	TRACE("use_hw = 0x%08x, capf = 0x%08lx, ds->drvcaps.dwFlags = 0x%08lx\n", use_hw, capf, ds->device->drvcaps.dwFlags);

	/* FIXME: check hardware sample rate mixing capabilities */
	/* FIXME: check app hints for software/hardware buffer (STATIC, LOCHARDWARE, etc) */
	/* FIXME: check whether any hardware buffers are left */
	/* FIXME: handle DSDHEAP_CREATEHEAP for hardware buffers */

	/* Allocate an empty buffer */
	dsb->buffer = HeapAlloc(GetProcessHeap(),0,sizeof(*(dsb->buffer)));
	if (dsb->buffer == NULL) {
		WARN("out of memory\n");
		HeapFree(GetProcessHeap(),0,dsb->pwfx);
		HeapFree(GetProcessHeap(),0,dsb);
		*pdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}

	/* Allocate system memory for buffer if applicable */
	if ((ds->device->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY) || !use_hw) {
		dsb->buffer->memory = HeapAlloc(GetProcessHeap(),0,dsb->buflen);
		if (dsb->buffer->memory == NULL) {
			WARN("out of memory\n");
			HeapFree(GetProcessHeap(),0,dsb->pwfx);
			HeapFree(GetProcessHeap(),0,dsb->buffer);
			HeapFree(GetProcessHeap(),0,dsb);
			*pdsb = NULL;
			return DSERR_OUTOFMEMORY;
		}
		dsb->buffer->ref = 1;
		FillMemory(dsb->buffer->memory, dsb->buflen, dsbd->lpwfxFormat->wBitsPerSample == 8 ? 128 : 0);
	}

	/* Allocate the hardware buffer */
	if (use_hw) {
		err = IDsDriver_CreateSoundBuffer(ds->device->driver,wfex,dsbd->dwFlags,0,
						  &(dsb->buflen),&(dsb->buffer->memory),
						  (LPVOID*)&(dsb->hwbuf));
                /* fall back to software buffer on failure */
		if (err != DS_OK) {
			TRACE("IDsDriver_CreateSoundBuffer failed, falling back to software buffer\n");
			use_hw = 0;
			if (ds->device->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY) {
				dsb->buffer->memory = HeapAlloc(GetProcessHeap(),0,dsb->buflen);
				if (dsb->buffer->memory == NULL) {
					WARN("out of memory\n");
					HeapFree(GetProcessHeap(),0,dsb->buffer);
					HeapFree(GetProcessHeap(),0,dsb->pwfx);
					HeapFree(GetProcessHeap(),0,dsb);
					*pdsb = NULL;
					return DSERR_OUTOFMEMORY;
				}
				dsb->buffer->ref = 1;
				FillMemory(dsb->buffer->memory, dsb->buflen, dsbd->lpwfxFormat->wBitsPerSample == 8 ? 128 : 0);
			}
			err = DS_OK;
		}
	}

	/* calculate fragment size and write lead */
	DSOUND_RecalcFormat(dsb);

	/* It's not necessary to initialize values to zero since */
	/* we allocated this structure with HEAP_ZERO_MEMORY... */
	dsb->playpos = 0;
	dsb->buf_mixpos = 0;
	dsb->state = STATE_STOPPED;

	dsb->freqAdjust = (dsb->freq << DSOUND_FREQSHIFT) /
		ds->device->pwfx->nSamplesPerSec;
	dsb->nAvgBytesPerSec = dsb->freq *
		dsbd->lpwfxFormat->nBlockAlign;

	if (dsb->dsbd.dwFlags & DSBCAPS_CTRL3D) {
		dsb->ds3db_ds3db.dwSize = sizeof(DS3DBUFFER);
		dsb->ds3db_ds3db.vPosition.x = 0.0;
		dsb->ds3db_ds3db.vPosition.y = 0.0;
		dsb->ds3db_ds3db.vPosition.z = 0.0;
		dsb->ds3db_ds3db.vVelocity.x = 0.0;
		dsb->ds3db_ds3db.vVelocity.y = 0.0;
		dsb->ds3db_ds3db.vVelocity.z = 0.0;
		dsb->ds3db_ds3db.dwInsideConeAngle = DS3D_DEFAULTCONEANGLE;
		dsb->ds3db_ds3db.dwOutsideConeAngle = DS3D_DEFAULTCONEANGLE;
		dsb->ds3db_ds3db.vConeOrientation.x = 0.0;
		dsb->ds3db_ds3db.vConeOrientation.y = 0.0;
		dsb->ds3db_ds3db.vConeOrientation.z = 0.0;
		dsb->ds3db_ds3db.lConeOutsideVolume = DS3D_DEFAULTCONEOUTSIDEVOLUME;
		dsb->ds3db_ds3db.flMinDistance = DS3D_DEFAULTMINDISTANCE;
		dsb->ds3db_ds3db.flMaxDistance = DS3D_DEFAULTMAXDISTANCE;
		dsb->ds3db_ds3db.dwMode = DS3DMODE_NORMAL;

		dsb->ds3db_need_recalc = FALSE;
		DSOUND_Calc3DBuffer(dsb);
	} else
		DSOUND_RecalcVolPan(&(dsb->volpan));

	InitializeCriticalSection(&(dsb->lock));
        dsb->lock.DebugInfo->Spare[0] = (DWORD_PTR)"DSOUNDBUFFER_lock";

	/* register buffer if not primary */
	if (!(dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER)) {
		err = DSOUND_AddBuffer(ds, dsb);
		if (err != DS_OK) {
			HeapFree(GetProcessHeap(),0,dsb->buffer->memory);
			HeapFree(GetProcessHeap(),0,dsb->buffer);
        		dsb->lock.DebugInfo->Spare[0] = 0;
			DeleteCriticalSection(&(dsb->lock));
			HeapFree(GetProcessHeap(),0,dsb->pwfx);
			HeapFree(GetProcessHeap(),0,dsb);
			dsb = NULL;
		}
	}

	*pdsb = dsb;
	return err;
}

HRESULT WINAPI IDirectSoundBufferImpl_Destroy(
    IDirectSoundBufferImpl *pdsb)
{
    TRACE("(%p)\n",pdsb);

    /* This keeps the *_Destroy functions from possibly deleting
     * this object until it is ready to be deleted */
    IDirectSoundBufferImpl_AddRef((LPDIRECTSOUNDBUFFER8)pdsb);

    if (pdsb->iks) {
        WARN("iks not NULL\n");
        IKsBufferPropertySetImpl_Destroy(pdsb->iks);
        pdsb->iks = NULL;
    }

    if (pdsb->ds3db) {
        WARN("ds3db not NULL\n");
        IDirectSound3DBufferImpl_Destroy(pdsb->ds3db);
        pdsb->ds3db = NULL;
    }

    if (pdsb->notify) {
        WARN("notify not NULL\n");
        IDirectSoundNotifyImpl_Destroy(pdsb->notify);
        pdsb->notify = NULL;
    }

    if (pdsb->dsb) {
        WARN("dsb not NULL\n");
        SecondaryBufferImpl_Destroy(pdsb->dsb);
        pdsb->dsb = NULL;
    }

    while (IDirectSoundBuffer8_Release((LPDIRECTSOUNDBUFFER8)pdsb) > 0);

    return S_OK;
}

/*******************************************************************************
 *		SecondaryBuffer
 */

static HRESULT WINAPI SecondaryBufferImpl_QueryInterface(
	LPDIRECTSOUNDBUFFER8 iface,REFIID riid,LPVOID *ppobj)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

	return IDirectSoundBufferImpl_QueryInterface((LPDIRECTSOUNDBUFFER8)This->dsb,riid,ppobj);
}

static ULONG WINAPI SecondaryBufferImpl_AddRef(LPDIRECTSOUNDBUFFER8 iface)
{
    IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI SecondaryBufferImpl_Release(LPDIRECTSOUNDBUFFER8 iface)
{
    IDirectSoundBufferImpl *This = (IDirectSoundBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %ld\n", This, ref + 1);

    if (!ref) {
        This->dsb->dsb = NULL;
        IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER8)This->dsb);
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI SecondaryBufferImpl_GetCaps(
	LPDIRECTSOUNDBUFFER8 iface,LPDSBCAPS caps)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
  	TRACE("(%p)->(%p)\n",This,caps);

	return IDirectSoundBufferImpl_GetCaps((LPDIRECTSOUNDBUFFER8)This->dsb,caps);
}

static HRESULT WINAPI SecondaryBufferImpl_GetCurrentPosition(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD playpos,LPDWORD writepos)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%p,%p)\n",This,playpos,writepos);

	return IDirectSoundBufferImpl_GetCurrentPosition((LPDIRECTSOUNDBUFFER8)This->dsb,playpos,writepos);
}

static HRESULT WINAPI SecondaryBufferImpl_GetFormat(
	LPDIRECTSOUNDBUFFER8 iface,LPWAVEFORMATEX lpwf,DWORD wfsize,LPDWORD wfwritten)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%p,%ld,%p)\n",This,lpwf,wfsize,wfwritten);

	return IDirectSoundBufferImpl_GetFormat((LPDIRECTSOUNDBUFFER8)This->dsb,lpwf,wfsize,wfwritten);
}

static HRESULT WINAPI SecondaryBufferImpl_GetVolume(
	LPDIRECTSOUNDBUFFER8 iface,LPLONG vol)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%p)\n",This,vol);

	return IDirectSoundBufferImpl_GetVolume((LPDIRECTSOUNDBUFFER8)This->dsb,vol);
}

static HRESULT WINAPI SecondaryBufferImpl_GetPan(
	LPDIRECTSOUNDBUFFER8 iface,LPLONG pan)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%p)\n",This,pan);

	return IDirectSoundBufferImpl_GetPan((LPDIRECTSOUNDBUFFER8)This->dsb,pan);
}

static HRESULT WINAPI SecondaryBufferImpl_GetFrequency(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD freq)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%p)\n",This,freq);

	return IDirectSoundBufferImpl_GetFrequency((LPDIRECTSOUNDBUFFER8)This->dsb,freq);
}

static HRESULT WINAPI SecondaryBufferImpl_GetStatus(
	LPDIRECTSOUNDBUFFER8 iface,LPDWORD status)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%p)\n",This,status);

	return IDirectSoundBufferImpl_GetStatus((LPDIRECTSOUNDBUFFER8)This->dsb,status);
}

static HRESULT WINAPI SecondaryBufferImpl_Initialize(
	LPDIRECTSOUNDBUFFER8 iface,LPDIRECTSOUND dsound,LPCDSBUFFERDESC dbsd)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%p,%p)\n",This,dsound,dbsd);

	return IDirectSoundBufferImpl_Initialize((LPDIRECTSOUNDBUFFER8)This->dsb,dsound,dbsd);
}

static HRESULT WINAPI SecondaryBufferImpl_Lock(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD writecursor,
    DWORD writebytes,
    LPVOID lplpaudioptr1,
    LPDWORD audiobytes1,
    LPVOID lplpaudioptr2,
    LPDWORD audiobytes2,
    DWORD dwFlags)
{
    SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
    TRACE("(%p,%ld,%ld,%p,%p,%p,%p,0x%08lx)\n",
        This,writecursor,writebytes,lplpaudioptr1,audiobytes1,lplpaudioptr2,audiobytes2,dwFlags);

    return IDirectSoundBufferImpl_Lock((LPDIRECTSOUNDBUFFER8)This->dsb,
        writecursor,writebytes,lplpaudioptr1,audiobytes1,lplpaudioptr2,audiobytes2,dwFlags);
}

static HRESULT WINAPI SecondaryBufferImpl_Play(
	LPDIRECTSOUNDBUFFER8 iface,DWORD reserved1,DWORD reserved2,DWORD flags)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%08lx,%08lx,%08lx)\n",This,reserved1,reserved2,flags);

	return IDirectSoundBufferImpl_Play((LPDIRECTSOUNDBUFFER8)This->dsb,reserved1,reserved2,flags);
}

static HRESULT WINAPI SecondaryBufferImpl_SetCurrentPosition(
	LPDIRECTSOUNDBUFFER8 iface,DWORD newpos)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%ld)\n",This,newpos);

	return IDirectSoundBufferImpl_SetCurrentPosition((LPDIRECTSOUNDBUFFER8)This->dsb,newpos);
}

static HRESULT WINAPI SecondaryBufferImpl_SetFormat(
	LPDIRECTSOUNDBUFFER8 iface,LPCWAVEFORMATEX wfex)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%p)\n",This,wfex);

	return IDirectSoundBufferImpl_SetFormat((LPDIRECTSOUNDBUFFER8)This->dsb,wfex);
}

static HRESULT WINAPI SecondaryBufferImpl_SetVolume(
	LPDIRECTSOUNDBUFFER8 iface,LONG vol)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%ld)\n",This,vol);

	return IDirectSoundBufferImpl_SetVolume((LPDIRECTSOUNDBUFFER8)This->dsb,vol);
}

static HRESULT WINAPI SecondaryBufferImpl_SetPan(
	LPDIRECTSOUNDBUFFER8 iface,LONG pan)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%ld)\n",This,pan);

	return IDirectSoundBufferImpl_SetPan((LPDIRECTSOUNDBUFFER8)This->dsb,pan);
}

static HRESULT WINAPI SecondaryBufferImpl_SetFrequency(
	LPDIRECTSOUNDBUFFER8 iface,DWORD freq)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%ld)\n",This,freq);

	return IDirectSoundBufferImpl_SetFrequency((LPDIRECTSOUNDBUFFER8)This->dsb,freq);
}

static HRESULT WINAPI SecondaryBufferImpl_Stop(LPDIRECTSOUNDBUFFER8 iface)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p)\n",This);

	return IDirectSoundBufferImpl_Stop((LPDIRECTSOUNDBUFFER8)This->dsb);
}

static HRESULT WINAPI SecondaryBufferImpl_Unlock(
    LPDIRECTSOUNDBUFFER8 iface,
    LPVOID lpvAudioPtr1,
    DWORD dwAudioBytes1,
    LPVOID lpvAudioPtr2,
    DWORD dwAudioBytes2)
{
    SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
    TRACE("(%p,%p,%ld,%p,%ld)\n",
        This, lpvAudioPtr1, dwAudioBytes1, lpvAudioPtr2, dwAudioBytes2);

    return IDirectSoundBufferImpl_Unlock((LPDIRECTSOUNDBUFFER8)This->dsb,
        lpvAudioPtr1,dwAudioBytes1,lpvAudioPtr2,dwAudioBytes2);
}

static HRESULT WINAPI SecondaryBufferImpl_Restore(
	LPDIRECTSOUNDBUFFER8 iface)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p)\n",This);

	return IDirectSoundBufferImpl_Restore((LPDIRECTSOUNDBUFFER8)This->dsb);
}

static HRESULT WINAPI SecondaryBufferImpl_SetFX(
	LPDIRECTSOUNDBUFFER8 iface,DWORD dwEffectsCount,LPDSEFFECTDESC pDSFXDesc,LPDWORD pdwResultCodes)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%lu,%p,%p)\n",This,dwEffectsCount,pDSFXDesc,pdwResultCodes);

	return IDirectSoundBufferImpl_SetFX((LPDIRECTSOUNDBUFFER8)This->dsb,dwEffectsCount,pDSFXDesc,pdwResultCodes);
}

static HRESULT WINAPI SecondaryBufferImpl_AcquireResources(
	LPDIRECTSOUNDBUFFER8 iface,DWORD dwFlags,DWORD dwEffectsCount,LPDWORD pdwResultCodes)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%08lu,%lu,%p)\n",This,dwFlags,dwEffectsCount,pdwResultCodes);

	return IDirectSoundBufferImpl_AcquireResources((LPDIRECTSOUNDBUFFER8)This->dsb,dwFlags,dwEffectsCount,pdwResultCodes);
}

static HRESULT WINAPI SecondaryBufferImpl_GetObjectInPath(
	LPDIRECTSOUNDBUFFER8 iface,REFGUID rguidObject,DWORD dwIndex,REFGUID rguidInterface,LPVOID* ppObject)
{
	SecondaryBufferImpl *This = (SecondaryBufferImpl *)iface;
	TRACE("(%p,%s,%lu,%s,%p)\n",This,debugstr_guid(rguidObject),dwIndex,debugstr_guid(rguidInterface),ppObject);

	return IDirectSoundBufferImpl_GetObjectInPath((LPDIRECTSOUNDBUFFER8)This->dsb,rguidObject,dwIndex,rguidInterface,ppObject);
}

static const IDirectSoundBuffer8Vtbl sbvt =
{
	SecondaryBufferImpl_QueryInterface,
	SecondaryBufferImpl_AddRef,
	SecondaryBufferImpl_Release,
	SecondaryBufferImpl_GetCaps,
	SecondaryBufferImpl_GetCurrentPosition,
	SecondaryBufferImpl_GetFormat,
	SecondaryBufferImpl_GetVolume,
	SecondaryBufferImpl_GetPan,
	SecondaryBufferImpl_GetFrequency,
	SecondaryBufferImpl_GetStatus,
	SecondaryBufferImpl_Initialize,
	SecondaryBufferImpl_Lock,
	SecondaryBufferImpl_Play,
	SecondaryBufferImpl_SetCurrentPosition,
	SecondaryBufferImpl_SetFormat,
	SecondaryBufferImpl_SetVolume,
	SecondaryBufferImpl_SetPan,
	SecondaryBufferImpl_SetFrequency,
	SecondaryBufferImpl_Stop,
	SecondaryBufferImpl_Unlock,
	SecondaryBufferImpl_Restore,
	SecondaryBufferImpl_SetFX,
	SecondaryBufferImpl_AcquireResources,
	SecondaryBufferImpl_GetObjectInPath
};

HRESULT WINAPI SecondaryBufferImpl_Create(
	IDirectSoundBufferImpl *dsb,
	SecondaryBufferImpl **psb)
{
	SecondaryBufferImpl *sb;
	TRACE("(%p,%p)\n",dsb,psb);

	sb = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*sb));

	if (sb == 0) {
		WARN("out of memory\n");
		*psb = NULL;
		return DSERR_OUTOFMEMORY;
	}
	sb->ref = 0;
	sb->dsb = dsb;
	sb->lpVtbl = &sbvt;

	IDirectSoundBuffer8_AddRef((LPDIRECTSOUNDBUFFER8)dsb);
	*psb = sb;
	return S_OK;
}

HRESULT WINAPI SecondaryBufferImpl_Destroy(
    SecondaryBufferImpl *pdsb)
{
    TRACE("(%p)\n",pdsb);

    while (SecondaryBufferImpl_Release((LPDIRECTSOUNDBUFFER8)pdsb) > 0);

    return S_OK;
}
