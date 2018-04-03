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
 */

#include "dsound_private.h"

/*******************************************************************************
 *		IDirectSoundNotify
 */

struct IDirectSoundNotifyImpl
{
    /* IUnknown fields */
    const IDirectSoundNotifyVtbl *lpVtbl;
    LONG                        ref;
    IDirectSoundBufferImpl*     dsb;
};

static HRESULT IDirectSoundNotifyImpl_Create(IDirectSoundBufferImpl *dsb,
                                             IDirectSoundNotifyImpl **pdsn);
static HRESULT IDirectSoundNotifyImpl_Destroy(IDirectSoundNotifyImpl *pdsn);

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
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirectSoundNotifyImpl_Release(LPDIRECTSOUNDNOTIFY iface)
{
    IDirectSoundNotifyImpl *This = (IDirectSoundNotifyImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);

    if (!ref) {
        This->dsb->notify = NULL;
        IDirectSoundBuffer_Release((LPDIRECTSOUNDBUFFER)This->dsb);
        HeapFree(GetProcessHeap(), 0, This);
        TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSoundNotifyImpl_SetNotificationPositions(
	LPDIRECTSOUNDNOTIFY iface,DWORD howmuch,LPCDSBPOSITIONNOTIFY notify
) {
	IDirectSoundNotifyImpl *This = (IDirectSoundNotifyImpl *)iface;
	TRACE("(%p,0x%08x,%p)\n",This,howmuch,notify);

        if (howmuch > 0 && notify == NULL) {
	    WARN("invalid parameter: notify == NULL\n");
	    return DSERR_INVALIDPARAM;
	}

	if (TRACE_ON(dsound)) {
	    unsigned int	i;
	    for (i=0;i<howmuch;i++)
		TRACE("notify at %d to %p\n",
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
	    HeapFree(GetProcessHeap(), 0, This->dsb->notifies);
	    This->dsb->notifies = HeapAlloc(GetProcessHeap(), 0,
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

static HRESULT IDirectSoundNotifyImpl_Create(
    IDirectSoundBufferImpl * dsb,
    IDirectSoundNotifyImpl **pdsn)
{
    IDirectSoundNotifyImpl * dsn;
    TRACE("(%p,%p)\n",dsb,pdsn);

    dsn = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*dsn));

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

static HRESULT IDirectSoundNotifyImpl_Destroy(
    IDirectSoundNotifyImpl *pdsn)
{
    TRACE("(%p)\n",pdsn);

    while (IDirectSoundNotifyImpl_Release((LPDIRECTSOUNDNOTIFY)pdsn) > 0);

    return DS_OK;
}

/*******************************************************************************
 *		IDirectSoundBuffer
 */

static inline IDirectSoundBufferImpl *impl_from_IDirectSoundBuffer8(IDirectSoundBuffer8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectSoundBufferImpl, IDirectSoundBuffer8_iface);
}

static inline BOOL is_primary_buffer(IDirectSoundBufferImpl *This)
{
    return This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER ? TRUE : FALSE;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetFormat(IDirectSoundBuffer8 *iface,
        LPCWAVEFORMATEX wfex)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);

    TRACE("(%p,%p)\n", iface, wfex);

    if (is_primary_buffer(This))
        return primarybuffer_SetFormat(This->device, wfex);
    else {
        WARN("not available for secondary buffers.\n");
        return DSERR_INVALIDCALL;
    }
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetVolume(IDirectSoundBuffer8 *iface, LONG vol)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
	LONG oldVol;

	HRESULT hres = DS_OK;

	TRACE("(%p,%d)\n",This,vol);

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLVOLUME)) {
		WARN("control unavailable: This->dsbd.dwFlags = 0x%08x\n", This->dsbd.dwFlags);
		return DSERR_CONTROLUNAVAIL;
	}

	if ((vol > DSBVOLUME_MAX) || (vol < DSBVOLUME_MIN)) {
		WARN("invalid parameter: vol = %d\n", vol);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	RtlAcquireResourceExclusive(&This->lock, TRUE);

	if (This->dsbd.dwFlags & DSBCAPS_CTRL3D) {
		oldVol = This->ds3db_lVolume;
		This->ds3db_lVolume = vol;
		if (vol != oldVol)
			/* recalc 3d volume, which in turn recalcs the pans */
			DSOUND_Calc3DBuffer(This);
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
		}
	}

	RtlReleaseResource(&This->lock);
	/* **** */

	return hres;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetVolume(IDirectSoundBuffer8 *iface, LONG *vol)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);

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

static HRESULT WINAPI IDirectSoundBufferImpl_SetFrequency(IDirectSoundBuffer8 *iface, DWORD freq)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
	DWORD oldFreq;

	TRACE("(%p,%d)\n",This,freq);

        if (is_primary_buffer(This)) {
                WARN("not available for primary buffers.\n");
                return DSERR_CONTROLUNAVAIL;
        }

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLFREQUENCY)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (freq == DSBFREQUENCY_ORIGINAL)
		freq = This->pwfx->nSamplesPerSec;

	if ((freq < DSBFREQUENCY_MIN) || (freq > DSBFREQUENCY_MAX)) {
		WARN("invalid parameter: freq = %d\n", freq);
		return DSERR_INVALIDPARAM;
	}

	/* **** */
	RtlAcquireResourceExclusive(&This->lock, TRUE);

	oldFreq = This->freq;
	This->freq = freq;
	if (freq != oldFreq) {
		This->freqAdjust = ((DWORD64)This->freq << DSOUND_FREQSHIFT) / This->device->pwfx->nSamplesPerSec;
		This->nAvgBytesPerSec = freq * This->pwfx->nBlockAlign;
		DSOUND_RecalcFormat(This);
		DSOUND_MixToTemporary(This, 0, This->buflen, FALSE);
	}

	RtlReleaseResource(&This->lock);
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Play(IDirectSoundBuffer8 *iface, DWORD reserved1,
        DWORD reserved2, DWORD flags)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
	HRESULT hres = DS_OK;

	TRACE("(%p,%08x,%08x,%08x)\n",This,reserved1,reserved2,flags);

	/* **** */
	RtlAcquireResourceExclusive(&This->lock, TRUE);

	This->playflags = flags;
	if (This->state == STATE_STOPPED && !This->hwbuf) {
		This->leadin = TRUE;
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

	RtlReleaseResource(&This->lock);
	/* **** */

	return hres;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Stop(IDirectSoundBuffer8 *iface)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
	HRESULT hres = DS_OK;

	TRACE("(%p)\n",This);

	/* **** */
	RtlAcquireResourceExclusive(&This->lock, TRUE);

	if (This->state == STATE_PLAYING)
		This->state = STATE_STOPPING;
	else if (This->state == STATE_STARTING)
	{
		This->state = STATE_STOPPED;
		DSOUND_CheckEvent(This, 0, 0);
	}
	if (This->hwbuf) {
		hres = IDsDriverBuffer_Stop(This->hwbuf);
		if (hres != DS_OK)
			WARN("IDsDriverBuffer_Stop failed\n");
		else
			This->state = STATE_STOPPED;
	}

	RtlReleaseResource(&This->lock);
	/* **** */

	return hres;
}

static ULONG WINAPI IDirectSoundBufferImpl_AddRef(IDirectSoundBuffer8 *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, ref - 1);

    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI IDirectSoundBufferImpl_Release(IDirectSoundBuffer8 *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref was %d\n", This, ref + 1);

    if (!ref && !InterlockedDecrement(&This->numIfaces)) {
        if (is_primary_buffer(This))
            primarybuffer_destroy(This);
        else
            secondarybuffer_destroy(This);
    }
    return ref;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetCurrentPosition(IDirectSoundBuffer8 *iface,
        DWORD *playpos, DWORD *writepos)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
	HRESULT	hres;

	TRACE("(%p,%p,%p)\n",This,playpos,writepos);

	RtlAcquireResourceShared(&This->lock, TRUE);
	if (This->hwbuf) {
		hres=IDsDriverBuffer_GetPosition(This->hwbuf,playpos,writepos);
		if (hres != DS_OK) {
		    WARN("IDsDriverBuffer_GetPosition failed\n");
		    return hres;
		}
	} else {
		DWORD pos = This->sec_mixpos;

		/* sanity */
		if (pos >= This->buflen){
			FIXME("Bad play position. playpos: %d, buflen: %d\n", pos, This->buflen);
			pos %= This->buflen;
		}

		if (playpos)
			*playpos = pos;
		if (writepos)
			*writepos = pos;
	}
	if (writepos && This->state != STATE_STOPPED && (!This->hwbuf || !(This->device->drvdesc.dwFlags & DSDDESC_DONTNEEDWRITELEAD))) {
		/* apply the documented 10ms lead to writepos */
		*writepos += This->writelead;
		*writepos %= This->buflen;
	}
	RtlReleaseResource(&This->lock);

	TRACE("playpos = %d, writepos = %d, buflen=%d (%p, time=%d)\n",
		playpos?*playpos:-1, writepos?*writepos:-1, This->buflen, This, GetTickCount());

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetStatus(IDirectSoundBuffer8 *iface, DWORD *status)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);

	TRACE("(%p,%p), thread is %04x\n",This,status,GetCurrentThreadId());

	if (status == NULL) {
		WARN("invalid parameter: status = NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*status = 0;
	RtlAcquireResourceShared(&This->lock, TRUE);
	if ((This->state == STATE_STARTING) || (This->state == STATE_PLAYING)) {
		*status |= DSBSTATUS_PLAYING;
		if (This->playflags & DSBPLAY_LOOPING)
			*status |= DSBSTATUS_LOOPING;
	}
	RtlReleaseResource(&This->lock);

	TRACE("status=%x\n", *status);
	return DS_OK;
}


static HRESULT WINAPI IDirectSoundBufferImpl_GetFormat(IDirectSoundBuffer8 *iface,
        LPWAVEFORMATEX lpwf, DWORD wfsize, DWORD *wfwritten)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
    DWORD size;

    TRACE("(%p,%p,%d,%p)\n",This,lpwf,wfsize,wfwritten);

    size = sizeof(WAVEFORMATEX) + This->pwfx->cbSize;

    if (lpwf) { /* NULL is valid */
        if (wfsize >= size) {
            CopyMemory(lpwf,This->pwfx,size);
            if (wfwritten)
                *wfwritten = size;
        } else {
            WARN("invalid parameter: wfsize too small\n");
            CopyMemory(lpwf,This->pwfx,wfsize);
            if (wfwritten)
                *wfwritten = wfsize;
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

static HRESULT WINAPI IDirectSoundBufferImpl_Lock(IDirectSoundBuffer8 *iface, DWORD writecursor,
        DWORD writebytes, void **lplpaudioptr1, DWORD *audiobytes1, void **lplpaudioptr2,
        DWORD *audiobytes2, DWORD flags)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
	HRESULT hres = DS_OK;

        TRACE("(%p,%d,%d,%p,%p,%p,%p,0x%08x) at %d\n", This, writecursor, writebytes, lplpaudioptr1,
                audiobytes1, lplpaudioptr2, audiobytes2, flags, GetTickCount());

        if (!audiobytes1)
            return DSERR_INVALIDPARAM;

        /* when this flag is set, writecursor is meaningless and must be calculated */
	if (flags & DSBLOCK_FROMWRITECURSOR) {
		/* GetCurrentPosition does too much magic to duplicate here */
		hres = IDirectSoundBufferImpl_GetCurrentPosition(iface, NULL, &writecursor);
		if (hres != DS_OK) {
			WARN("IDirectSoundBufferImpl_GetCurrentPosition failed\n");
			return hres;
		}
	}

        /* when this flag is set, writebytes is meaningless and must be set */
	if (flags & DSBLOCK_ENTIREBUFFER)
		writebytes = This->buflen;

	if (writecursor >= This->buflen) {
		WARN("Invalid parameter, writecursor: %u >= buflen: %u\n",
		     writecursor, This->buflen);
		return DSERR_INVALIDPARAM;
        }

	if (writebytes > This->buflen) {
		WARN("Invalid parameter, writebytes: %u > buflen: %u\n",
		     writebytes, This->buflen);
		return DSERR_INVALIDPARAM;
        }

	/* **** */
	RtlAcquireResourceShared(&This->lock, TRUE);

	if (!(This->device->drvdesc.dwFlags & DSDDESC_DONTNEEDSECONDARYLOCK) && This->hwbuf) {
		hres = IDsDriverBuffer_Lock(This->hwbuf,
				     lplpaudioptr1, audiobytes1,
				     lplpaudioptr2, audiobytes2,
				     writecursor, writebytes,
				     0);
		if (hres != DS_OK) {
			WARN("IDsDriverBuffer_Lock failed\n");
			RtlReleaseResource(&This->lock);
			return hres;
		}
	} else {
		if (writecursor+writebytes <= This->buflen) {
			*(LPBYTE*)lplpaudioptr1 = This->buffer->memory+writecursor;
			if (This->sec_mixpos >= writecursor && This->sec_mixpos < writecursor + writebytes && This->state == STATE_PLAYING)
				WARN("Overwriting mixing position, case 1\n");
			*audiobytes1 = writebytes;
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = NULL;
			if (audiobytes2)
				*audiobytes2 = 0;
			TRACE("Locked %p(%i bytes) and %p(%i bytes) writecursor=%d\n",
			  *(LPBYTE*)lplpaudioptr1, *audiobytes1, lplpaudioptr2 ? *(LPBYTE*)lplpaudioptr2 : NULL, audiobytes2 ? *audiobytes2: 0, writecursor);
			TRACE("->%d.0\n",writebytes);
		} else {
			DWORD remainder = writebytes + writecursor - This->buflen;
			*(LPBYTE*)lplpaudioptr1 = This->buffer->memory+writecursor;
			*audiobytes1 = This->buflen-writecursor;
			if (This->sec_mixpos >= writecursor && This->sec_mixpos < writecursor + writebytes && This->state == STATE_PLAYING)
				WARN("Overwriting mixing position, case 2\n");
			if (lplpaudioptr2)
				*(LPBYTE*)lplpaudioptr2 = This->buffer->memory;
			if (audiobytes2)
				*audiobytes2 = writebytes-(This->buflen-writecursor);
			if (audiobytes2 && This->sec_mixpos < remainder && This->state == STATE_PLAYING)
				WARN("Overwriting mixing position, case 3\n");
			TRACE("Locked %p(%i bytes) and %p(%i bytes) writecursor=%d\n", *(LPBYTE*)lplpaudioptr1, *audiobytes1, lplpaudioptr2 ? *(LPBYTE*)lplpaudioptr2 : NULL, audiobytes2 ? *audiobytes2: 0, writecursor);
		}
	}

	RtlReleaseResource(&This->lock);
	/* **** */

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetCurrentPosition(IDirectSoundBuffer8 *iface,
        DWORD newpos)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
	HRESULT hres = DS_OK;
	DWORD oldpos;

	TRACE("(%p,%d)\n",This,newpos);

	/* **** */
	RtlAcquireResourceExclusive(&This->lock, TRUE);

	oldpos = This->sec_mixpos;

	/* start mixing from this new location instead */
	newpos %= This->buflen;
	newpos -= newpos%This->pwfx->nBlockAlign;
	This->sec_mixpos = newpos;

	/* at this point, do not attempt to reset buffers, mess with primary mix position,
           or anything like that to reduce latency. The data already prebuffered cannot be changed */

	/* position HW buffer if applicable, else just start mixing from new location instead */
	if (This->hwbuf) {
		hres = IDsDriverBuffer_SetPosition(This->hwbuf, This->buf_mixpos);
		if (hres != DS_OK)
			WARN("IDsDriverBuffer_SetPosition failed\n");
	}
	else if (oldpos != newpos)
		/* FIXME: Perhaps add a call to DSOUND_MixToTemporary here? Not sure it's needed */
		This->buf_mixpos = DSOUND_secpos_to_bufpos(This, newpos, 0, NULL);

	RtlReleaseResource(&This->lock);
	/* **** */

	return hres;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetPan(IDirectSoundBuffer8 *iface, LONG pan)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
	HRESULT hres = DS_OK;

	TRACE("(%p,%d)\n",This,pan);

	if ((pan > DSBPAN_RIGHT) || (pan < DSBPAN_LEFT)) {
		WARN("invalid parameter: pan = %d\n", pan);
		return DSERR_INVALIDPARAM;
	}

	/* You cannot use both pan and 3D controls */
	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (This->dsbd.dwFlags & DSBCAPS_CTRL3D)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	/* **** */
	RtlAcquireResourceExclusive(&This->lock, TRUE);

	if (This->volpan.lPan != pan) {
		This->volpan.lPan = pan;
		DSOUND_RecalcVolPan(&(This->volpan));

		if (This->hwbuf) {
			hres = IDsDriverBuffer_SetVolumePan(This->hwbuf, &(This->volpan));
			if (hres != DS_OK)
				WARN("IDsDriverBuffer_SetVolumePan failed\n");
		}
	}

	RtlReleaseResource(&This->lock);
	/* **** */

	return hres;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetPan(IDirectSoundBuffer8 *iface, LONG *pan)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);

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

static HRESULT WINAPI IDirectSoundBufferImpl_Unlock(IDirectSoundBuffer8 *iface, void *p1, DWORD x1,
        void *p2, DWORD x2)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface), *iter;
	HRESULT hres = DS_OK;

	TRACE("(%p,%p,%d,%p,%d)\n", This,p1,x1,p2,x2);

	/* **** */
	RtlAcquireResourceShared(&This->lock, TRUE);

	if (!(This->device->drvdesc.dwFlags & DSDDESC_DONTNEEDSECONDARYLOCK) && This->hwbuf) {
		hres = IDsDriverBuffer_Unlock(This->hwbuf, p1, x1, p2, x2);
		if (hres != DS_OK)
			WARN("IDsDriverBuffer_Unlock failed\n");
	}

	RtlReleaseResource(&This->lock);
	/* **** */

	if (!p2)
		x2 = 0;

	if (!This->hwbuf && (x1 || x2))
	{
		RtlAcquireResourceShared(&This->device->buffer_list_lock, TRUE);
		LIST_FOR_EACH_ENTRY(iter, &This->buffer->buffers, IDirectSoundBufferImpl, entry )
		{
			RtlAcquireResourceShared(&iter->lock, TRUE);
			if (x1)
                        {
			    if(x1 + (DWORD_PTR)p1 - (DWORD_PTR)iter->buffer->memory > iter->buflen)
			      hres = DSERR_INVALIDPARAM;
			    else
			      DSOUND_MixToTemporary(iter, (DWORD_PTR)p1 - (DWORD_PTR)iter->buffer->memory, x1, FALSE);
                        }
			if (x2)
				DSOUND_MixToTemporary(iter, 0, x2, FALSE);
			RtlReleaseResource(&iter->lock);
		}
		RtlReleaseResource(&This->device->buffer_list_lock);
	}

	return hres;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Restore(IDirectSoundBuffer8 *iface)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);

	FIXME("(%p):stub\n",This);
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetFrequency(IDirectSoundBuffer8 *iface, DWORD *freq)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);

	TRACE("(%p,%p)\n",This,freq);

	if (freq == NULL) {
		WARN("invalid parameter: freq = NULL\n");
		return DSERR_INVALIDPARAM;
	}

	*freq = This->freq;
	TRACE("-> %d\n", *freq);

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_SetFX(IDirectSoundBuffer8 *iface, DWORD dwEffectsCount,
        LPDSEFFECTDESC pDSFXDesc, DWORD *pdwResultCodes)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
	DWORD u;

	FIXME("(%p,%u,%p,%p): stub\n",This,dwEffectsCount,pDSFXDesc,pdwResultCodes);

	if (pdwResultCodes)
		for (u=0; u<dwEffectsCount; u++) pdwResultCodes[u] = DSFXR_UNKNOWN;

	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI IDirectSoundBufferImpl_AcquireResources(IDirectSoundBuffer8 *iface,
        DWORD dwFlags, DWORD dwEffectsCount, DWORD *pdwResultCodes)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
	DWORD u;

	FIXME("(%p,%08u,%u,%p): stub, faking success\n",This,dwFlags,dwEffectsCount,pdwResultCodes);

	if (pdwResultCodes)
		for (u=0; u<dwEffectsCount; u++) pdwResultCodes[u] = DSFXR_UNKNOWN;

	WARN("control unavailable\n");
	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetObjectInPath(IDirectSoundBuffer8 *iface,
        REFGUID rguidObject, DWORD dwIndex, REFGUID rguidInterface, void **ppObject)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);

	FIXME("(%p,%s,%u,%s,%p): stub\n",This,debugstr_guid(rguidObject),dwIndex,debugstr_guid(rguidInterface),ppObject);

	WARN("control unavailable\n");
	return DSERR_CONTROLUNAVAIL;
}

static HRESULT WINAPI IDirectSoundBufferImpl_Initialize(IDirectSoundBuffer8 *iface,
        IDirectSound *dsound, LPCDSBUFFERDESC dbsd)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);

	WARN("(%p) already initialized\n", This);
	return DSERR_ALREADYINITIALIZED;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetCaps(IDirectSoundBuffer8 *iface, LPDSBCAPS caps)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);

  	TRACE("(%p)->(%p)\n",This,caps);

	if (caps == NULL) {
		WARN("invalid parameter: caps == NULL\n");
		return DSERR_INVALIDPARAM;
	}

	if (caps->dwSize < sizeof(*caps)) {
		WARN("invalid parameter: caps->dwSize = %d\n",caps->dwSize);
		return DSERR_INVALIDPARAM;
	}

	caps->dwFlags = This->dsbd.dwFlags;
	if (This->hwbuf) caps->dwFlags |= DSBCAPS_LOCHARDWARE;
	else caps->dwFlags |= DSBCAPS_LOCSOFTWARE;

	caps->dwBufferBytes = This->buflen;

	/* According to windows, this is zero*/
	caps->dwUnlockTransferRate = 0;
	caps->dwPlayCpuOverhead = 0;

	return DS_OK;
}

static HRESULT WINAPI IDirectSoundBufferImpl_QueryInterface(IDirectSoundBuffer8 *iface, REFIID riid,
        void **ppobj)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);

	TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

	if (ppobj == NULL) {
		WARN("invalid parameter\n");
		return E_INVALIDARG;
	}

	*ppobj = NULL;	/* assume failure */

	if ( IsEqualGUID(riid, &IID_IUnknown) ||
	     IsEqualGUID(riid, &IID_IDirectSoundBuffer) ||
	     IsEqualGUID(riid, &IID_IDirectSoundBuffer8) ) {
                IDirectSoundBuffer8_AddRef(iface);
                *ppobj = iface;
                return S_OK;
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

HRESULT IDirectSoundBufferImpl_Create(
	DirectSoundDevice * device,
	IDirectSoundBufferImpl **pdsb,
	LPCDSBUFFERDESC dsbd)
{
	IDirectSoundBufferImpl *dsb;
	LPWAVEFORMATEX wfex = dsbd->lpwfxFormat;
	HRESULT err = DS_OK;
	DWORD capf = 0;
	int use_hw;
	TRACE("(%p,%p,%p)\n",device,pdsb,dsbd);

	if (dsbd->dwBufferBytes < DSBSIZE_MIN || dsbd->dwBufferBytes > DSBSIZE_MAX) {
		WARN("invalid parameter: dsbd->dwBufferBytes = %d\n", dsbd->dwBufferBytes);
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

        dsb->ref = 1;
        dsb->numIfaces = 1;
	dsb->device = device;
        dsb->IDirectSoundBuffer8_iface.lpVtbl = &dsbvt;
	dsb->iks = NULL;

	/* size depends on version */
	CopyMemory(&dsb->dsbd, dsbd, dsbd->dwSize);

	dsb->pwfx = DSOUND_CopyFormat(wfex);
	if (dsb->pwfx == NULL) {
		HeapFree(GetProcessHeap(),0,dsb);
		*pdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}

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

	use_hw = !!(dsbd->dwFlags & DSBCAPS_LOCHARDWARE);
	TRACE("use_hw = %d, capf = 0x%08x, device->drvcaps.dwFlags = 0x%08x\n", use_hw, capf, device->drvcaps.dwFlags);
	if (use_hw && ((device->drvcaps.dwFlags & capf) != capf || !device->driver))
	{
		if (device->driver)
			WARN("Format not supported for hardware buffer\n");
		HeapFree(GetProcessHeap(),0,dsb->pwfx);
		HeapFree(GetProcessHeap(),0,dsb);
		*pdsb = NULL;
		if ((device->drvcaps.dwFlags & capf) != capf)
			return DSERR_BADFORMAT;
		return DSERR_GENERIC;
	}

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
	if ((device->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY) || !use_hw) {
		dsb->buffer->memory = HeapAlloc(GetProcessHeap(),0,dsb->buflen);
		if (dsb->buffer->memory == NULL) {
			WARN("out of memory\n");
			HeapFree(GetProcessHeap(),0,dsb->pwfx);
			HeapFree(GetProcessHeap(),0,dsb->buffer);
			HeapFree(GetProcessHeap(),0,dsb);
			*pdsb = NULL;
			return DSERR_OUTOFMEMORY;
		}
	}

	/* Allocate the hardware buffer */
	if (use_hw) {
		err = IDsDriver_CreateSoundBuffer(device->driver,wfex,dsbd->dwFlags,0,
						  &(dsb->buflen),&(dsb->buffer->memory),
						  (LPVOID*)&(dsb->hwbuf));
		if (FAILED(err))
		{
			WARN("Failed to create hardware secondary buffer: %08x\n", err);
			if (device->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY)
				HeapFree(GetProcessHeap(),0,dsb->buffer->memory);
			HeapFree(GetProcessHeap(),0,dsb->buffer);
			HeapFree(GetProcessHeap(),0,dsb->pwfx);
			HeapFree(GetProcessHeap(),0,dsb);
			*pdsb = NULL;
			return DSERR_GENERIC;
		}
	}

	dsb->buffer->ref = 1;
	list_init(&dsb->buffer->buffers);
	list_add_head(&dsb->buffer->buffers, &dsb->entry);
	FillMemory(dsb->buffer->memory, dsb->buflen, dsbd->lpwfxFormat->wBitsPerSample == 8 ? 128 : 0);

	/* It's not necessary to initialize values to zero since */
	/* we allocated this structure with HEAP_ZERO_MEMORY... */
	dsb->buf_mixpos = dsb->sec_mixpos = 0;
	dsb->state = STATE_STOPPED;

	dsb->freqAdjust = ((DWORD64)dsb->freq << DSOUND_FREQSHIFT) / device->pwfx->nSamplesPerSec;
	dsb->nAvgBytesPerSec = dsb->freq *
		dsbd->lpwfxFormat->nBlockAlign;

	/* calculate fragment size and write lead */
	DSOUND_RecalcFormat(dsb);

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

	RtlInitializeResource(&dsb->lock);

	/* register buffer if not primary */
	if (!(dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER)) {
		err = DirectSoundDevice_AddBuffer(device, dsb);
		if (err != DS_OK) {
			HeapFree(GetProcessHeap(),0,dsb->buffer->memory);
			HeapFree(GetProcessHeap(),0,dsb->buffer);
			RtlDeleteResource(&dsb->lock);
			HeapFree(GetProcessHeap(),0,dsb->pwfx);
			HeapFree(GetProcessHeap(),0,dsb);
			dsb = NULL;
		}
	}

	*pdsb = dsb;
	return err;
}

void secondarybuffer_destroy(IDirectSoundBufferImpl *This)
{
    DirectSoundDevice_RemoveBuffer(This->device, This);
    RtlDeleteResource(&This->lock);

    if (This->hwbuf)
        IDsDriverBuffer_Release(This->hwbuf);
    if (!This->hwbuf || (This->device->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY)) {
        This->buffer->ref--;
        list_remove(&This->entry);
        if (This->buffer->ref == 0) {
            HeapFree(GetProcessHeap(), 0, This->buffer->memory);
            HeapFree(GetProcessHeap(), 0, This->buffer);
        }
    }

    HeapFree(GetProcessHeap(), 0, This->tmp_buffer);
    HeapFree(GetProcessHeap(), 0, This->notifies);
    HeapFree(GetProcessHeap(), 0, This->pwfx);
    HeapFree(GetProcessHeap(), 0, This);

    TRACE("(%p) released\n", This);
}

HRESULT IDirectSoundBufferImpl_Destroy(
    IDirectSoundBufferImpl *pdsb)
{
    TRACE("(%p)\n",pdsb);

    /* This keeps the *_Destroy functions from possibly deleting
     * this object until it is ready to be deleted */
    InterlockedIncrement(&pdsb->numIfaces);

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

    secondarybuffer_destroy(pdsb);

    return S_OK;
}

HRESULT IDirectSoundBufferImpl_Duplicate(
    DirectSoundDevice *device,
    IDirectSoundBufferImpl **ppdsb,
    IDirectSoundBufferImpl *pdsb)
{
    IDirectSoundBufferImpl *dsb;
    HRESULT hres = DS_OK;
    TRACE("(%p,%p,%p)\n", device, ppdsb, pdsb);

    dsb = HeapAlloc(GetProcessHeap(),0,sizeof(*dsb));
    if (dsb == NULL) {
        WARN("out of memory\n");
        *ppdsb = NULL;
        return DSERR_OUTOFMEMORY;
    }
    CopyMemory(dsb, pdsb, sizeof(*dsb));

    dsb->pwfx = DSOUND_CopyFormat(pdsb->pwfx);
    if (dsb->pwfx == NULL) {
        HeapFree(GetProcessHeap(),0,dsb);
        *ppdsb = NULL;
        return DSERR_OUTOFMEMORY;
    }

    if (pdsb->hwbuf) {
        TRACE("duplicating hardware buffer\n");

        hres = IDsDriver_DuplicateSoundBuffer(device->driver, pdsb->hwbuf,
                                              (LPVOID *)&dsb->hwbuf);
        if (FAILED(hres)) {
            WARN("IDsDriver_DuplicateSoundBuffer failed (%08x)\n", hres);
            HeapFree(GetProcessHeap(),0,dsb->pwfx);
            HeapFree(GetProcessHeap(),0,dsb);
            *ppdsb = NULL;
            return hres;
        }
    }

    dsb->buffer->ref++;
    list_add_head(&dsb->buffer->buffers, &dsb->entry);
    dsb->ref = 1;
    dsb->numIfaces = 1;
    dsb->state = STATE_STOPPED;
    dsb->buf_mixpos = dsb->sec_mixpos = 0;
    dsb->notify = NULL;
    dsb->notifies = NULL;
    dsb->nrofnotifies = 0;
    dsb->device = device;
    dsb->ds3db = NULL;
    dsb->iks = NULL; /* FIXME? */
    dsb->tmp_buffer = NULL;
    DSOUND_RecalcFormat(dsb);
    DSOUND_MixToTemporary(dsb, 0, dsb->buflen, FALSE);

    RtlInitializeResource(&dsb->lock);

    /* register buffer */
    hres = DirectSoundDevice_AddBuffer(device, dsb);
    if (hres != DS_OK) {
        RtlDeleteResource(&dsb->lock);
        HeapFree(GetProcessHeap(),0,dsb->tmp_buffer);
        list_remove(&dsb->entry);
        dsb->buffer->ref--;
        HeapFree(GetProcessHeap(),0,dsb->pwfx);
        HeapFree(GetProcessHeap(),0,dsb);
        dsb = NULL;
    }

    *ppdsb = dsb;
    return hres;
}

/*******************************************************************************
 *              IKsBufferPropertySet
 */

/* IUnknown methods */
static HRESULT WINAPI IKsBufferPropertySetImpl_QueryInterface(
    LPKSPROPERTYSET iface,
    REFIID riid,
    LPVOID *ppobj )
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    return IDirectSoundBuffer_QueryInterface((LPDIRECTSOUNDBUFFER8)This->dsb, riid, ppobj);
}

static ULONG WINAPI IKsBufferPropertySetImpl_AddRef(LPKSPROPERTYSET iface)
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    ULONG ref = InterlockedIncrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IKsBufferPropertySetImpl_Release(LPKSPROPERTYSET iface)
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    ULONG ref = InterlockedDecrement(&(This->ref));
    TRACE("(%p) ref was %d\n", This, ref + 1);

    if (!ref) {
    This->dsb->iks = 0;
    IDirectSoundBuffer_Release((LPDIRECTSOUND3DBUFFER)This->dsb);
    HeapFree(GetProcessHeap(), 0, This);
    TRACE("(%p) released\n", This);
    }
    return ref;
}

static HRESULT WINAPI IKsBufferPropertySetImpl_Get(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    LPVOID pInstanceData,
    ULONG cbInstanceData,
    LPVOID pPropData,
    ULONG cbPropData,
    PULONG pcbReturned )
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    PIDSDRIVERPROPERTYSET ps;
    TRACE("(iface=%p,guidPropSet=%s,dwPropID=%d,pInstanceData=%p,cbInstanceData=%d,pPropData=%p,cbPropData=%d,pcbReturned=%p)\n",
    This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData,pcbReturned);

    if (This->dsb->hwbuf) {
        IDsDriver_QueryInterface(This->dsb->hwbuf, &IID_IDsDriverPropertySet, (void **)&ps);

        if (ps) {
        DSPROPERTY prop;
        HRESULT hres;

        prop.s.Set = *guidPropSet;
        prop.s.Id = dwPropID;
        prop.s.Flags = 0;  /* unused */
        prop.s.InstanceId = (ULONG_PTR)This->dsb->device;


        hres = IDsDriverPropertySet_Get(ps, &prop, pInstanceData, cbInstanceData, pPropData, cbPropData, pcbReturned);

        IDsDriverPropertySet_Release(ps);

        return hres;
        }
    }

    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsBufferPropertySetImpl_Set(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    LPVOID pInstanceData,
    ULONG cbInstanceData,
    LPVOID pPropData,
    ULONG cbPropData )
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    PIDSDRIVERPROPERTYSET ps;
    TRACE("(%p,%s,%d,%p,%d,%p,%d)\n",This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData);

    if (This->dsb->hwbuf) {
        IDsDriver_QueryInterface(This->dsb->hwbuf, &IID_IDsDriverPropertySet, (void **)&ps);

        if (ps) {
        DSPROPERTY prop;
        HRESULT hres;

        prop.s.Set = *guidPropSet;
        prop.s.Id = dwPropID;
        prop.s.Flags = 0;  /* unused */
        prop.s.InstanceId = (ULONG_PTR)This->dsb->device;
        hres = IDsDriverPropertySet_Set(ps,&prop,pInstanceData,cbInstanceData,pPropData,cbPropData);

        IDsDriverPropertySet_Release(ps);

        return hres;
        }
    }

    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsBufferPropertySetImpl_QuerySupport(
    LPKSPROPERTYSET iface,
    REFGUID guidPropSet,
    ULONG dwPropID,
    PULONG pTypeSupport )
{
    IKsBufferPropertySetImpl *This = (IKsBufferPropertySetImpl *)iface;
    PIDSDRIVERPROPERTYSET ps;
    TRACE("(%p,%s,%d,%p)\n",This,debugstr_guid(guidPropSet),dwPropID,pTypeSupport);

    if (This->dsb->hwbuf) {
        IDsDriver_QueryInterface(This->dsb->hwbuf, &IID_IDsDriverPropertySet, (void **)&ps);

        if (ps) {
            HRESULT hres;

            hres = IDsDriverPropertySet_QuerySupport(ps,guidPropSet, dwPropID,pTypeSupport);

            IDsDriverPropertySet_Release(ps);

            return hres;
        }
    }

    return E_PROP_ID_UNSUPPORTED;
}

static const IKsPropertySetVtbl iksbvt = {
    IKsBufferPropertySetImpl_QueryInterface,
    IKsBufferPropertySetImpl_AddRef,
    IKsBufferPropertySetImpl_Release,
    IKsBufferPropertySetImpl_Get,
    IKsBufferPropertySetImpl_Set,
    IKsBufferPropertySetImpl_QuerySupport
};

HRESULT IKsBufferPropertySetImpl_Create(
    IDirectSoundBufferImpl *dsb,
    IKsBufferPropertySetImpl **piks)
{
    IKsBufferPropertySetImpl *iks;
    TRACE("(%p,%p)\n",dsb,piks);
    *piks = NULL;

    iks = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*iks));
    if (iks == 0) {
        WARN("out of memory\n");
        *piks = NULL;
        return DSERR_OUTOFMEMORY;
    }

    iks->ref = 0;
    iks->dsb = dsb;
    dsb->iks = iks;
    iks->lpVtbl = &iksbvt;

    IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER)dsb);

    *piks = iks;
    return S_OK;
}

HRESULT IKsBufferPropertySetImpl_Destroy(
    IKsBufferPropertySetImpl *piks)
{
    TRACE("(%p)\n",piks);

    while (IKsBufferPropertySetImpl_Release((LPKSPROPERTYSET)piks) > 0);

    return S_OK;
}
