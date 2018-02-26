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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "mmsystem.h"
#include "winternl.h"
#include "vfwmsgs.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsound_private.h"
#include "dsconf.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

/*******************************************************************************
 *		IDirectSoundNotify
 */

static inline struct IDirectSoundBufferImpl *impl_from_IDirectSoundNotify(IDirectSoundNotify *iface)
{
    return CONTAINING_RECORD(iface, struct IDirectSoundBufferImpl, IDirectSoundNotify_iface);
}

static HRESULT WINAPI IDirectSoundNotifyImpl_QueryInterface(IDirectSoundNotify *iface, REFIID riid,
        void **ppobj)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundNotify(iface);

    TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppobj);

    return IDirectSoundBuffer8_QueryInterface(&This->IDirectSoundBuffer8_iface, riid, ppobj);
}

static ULONG WINAPI IDirectSoundNotifyImpl_AddRef(IDirectSoundNotify *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundNotify(iface);
    ULONG ref = InterlockedIncrement(&This->refn);

    TRACE("(%p) ref was %d\n", This, ref - 1);

    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI IDirectSoundNotifyImpl_Release(IDirectSoundNotify *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IDirectSoundNotify(iface);
    ULONG ref = InterlockedDecrement(&This->refn);

    TRACE("(%p) ref was %d\n", This, ref + 1);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        secondarybuffer_destroy(This);

    return ref;
}

static int notify_compar(const void *l, const void *r)
{
    const DSBPOSITIONNOTIFY *left = l;
    const DSBPOSITIONNOTIFY *right = r;

    /* place DSBPN_OFFSETSTOP at the start of the sorted array */
    if(left->dwOffset == DSBPN_OFFSETSTOP){
        if(right->dwOffset != DSBPN_OFFSETSTOP)
            return -1;
    }else if(right->dwOffset == DSBPN_OFFSETSTOP)
        return 1;

    if(left->dwOffset == right->dwOffset)
        return 0;

    if(left->dwOffset < right->dwOffset)
        return -1;

    return 1;
}

static HRESULT WINAPI IDirectSoundNotifyImpl_SetNotificationPositions(IDirectSoundNotify *iface,
        DWORD howmuch, const DSBPOSITIONNOTIFY *notify)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundNotify(iface);

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

	if (howmuch > 0) {
	    /* Make an internal copy of the caller-supplied array.
	     * Replace the existing copy if one is already present. */
            HeapFree(GetProcessHeap(), 0, This->notifies);
            This->notifies = HeapAlloc(GetProcessHeap(), 0,
			howmuch * sizeof(DSBPOSITIONNOTIFY));

            if (This->notifies == NULL) {
		    WARN("out of memory\n");
		    return DSERR_OUTOFMEMORY;
	    }
            CopyMemory(This->notifies, notify, howmuch * sizeof(DSBPOSITIONNOTIFY));
            This->nrofnotifies = howmuch;
            qsort(This->notifies, howmuch, sizeof(DSBPOSITIONNOTIFY), notify_compar);
	} else {
           HeapFree(GetProcessHeap(), 0, This->notifies);
           This->notifies = NULL;
           This->nrofnotifies = 0;
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

/*******************************************************************************
 *		IDirectSoundBuffer
 */

static inline IDirectSoundBufferImpl *impl_from_IDirectSoundBuffer8(IDirectSoundBuffer8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectSoundBufferImpl, IDirectSoundBuffer8_iface);
}

static inline BOOL is_primary_buffer(IDirectSoundBufferImpl *This)
{
    return (This->dsbd.dwFlags & DSBCAPS_PRIMARYBUFFER) != 0;
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
		This->freqAdjustNum = This->freq;
		This->freqAdjustDen = This->device->pwfx->nSamplesPerSec;
		This->nAvgBytesPerSec = freq * This->pwfx->nBlockAlign;
		DSOUND_RecalcFormat(This);
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
	int i;

	TRACE("(%p,%08x,%08x,%08x)\n",This,reserved1,reserved2,flags);

	/* **** */
	RtlAcquireResourceExclusive(&This->lock, TRUE);

	This->playflags = flags;
	if (This->state == STATE_STOPPED) {
		This->leadin = TRUE;
		This->state = STATE_STARTING;
	} else if (This->state == STATE_STOPPING)
		This->state = STATE_PLAYING;

	for (i = 0; i < This->num_filters; i++) {
		IMediaObject_Discontinuity(This->filters[i].obj, 0);
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
    ULONG ref;

    if (is_primary_buffer(This)){
        ref = capped_refcount_dec(&This->ref);
        if(!ref)
            capped_refcount_dec(&This->numIfaces);
        TRACE("(%p) ref is now: %d\n", This, ref);
        return ref;
    }

    ref = InterlockedDecrement(&This->ref);
    if (!ref && !InterlockedDecrement(&This->numIfaces))
            secondarybuffer_destroy(This);

    TRACE("(%p) ref is now %d\n", This, ref);

    return ref;
}

static HRESULT WINAPI IDirectSoundBufferImpl_GetCurrentPosition(IDirectSoundBuffer8 *iface,
        DWORD *playpos, DWORD *writepos)
{
        IDirectSoundBufferImpl *This = impl_from_IDirectSoundBuffer8(iface);
	DWORD pos;

	TRACE("(%p,%p,%p)\n",This,playpos,writepos);

	RtlAcquireResourceShared(&This->lock, TRUE);

	pos = This->sec_mixpos;

	/* sanity */
	if (pos >= This->buflen){
		FIXME("Bad play position. playpos: %d, buflen: %d\n", pos, This->buflen);
		pos %= This->buflen;
	}

	if (playpos)
		*playpos = pos;
	if (writepos)
		*writepos = pos;

	if (writepos && This->state != STATE_STOPPED) {
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
		This->buffer->lockedbytes += writebytes;
	} else {
		DWORD remainder = writebytes + writecursor - This->buflen;
		*(LPBYTE*)lplpaudioptr1 = This->buffer->memory+writecursor;
		*audiobytes1 = This->buflen-writecursor;
		This->buffer->lockedbytes += *audiobytes1;
		if (This->sec_mixpos >= writecursor && This->sec_mixpos < writecursor + writebytes && This->state == STATE_PLAYING)
			WARN("Overwriting mixing position, case 2\n");
		if (lplpaudioptr2)
			*(LPBYTE*)lplpaudioptr2 = This->buffer->memory;
		if (audiobytes2) {
			*audiobytes2 = writebytes-(This->buflen-writecursor);
			This->buffer->lockedbytes += *audiobytes2;
		}
		if (audiobytes2 && This->sec_mixpos < remainder && This->state == STATE_PLAYING)
			WARN("Overwriting mixing position, case 3\n");
		TRACE("Locked %p(%i bytes) and %p(%i bytes) writecursor=%d\n", *(LPBYTE*)lplpaudioptr1, *audiobytes1, lplpaudioptr2 ? *(LPBYTE*)lplpaudioptr2 : NULL, audiobytes2 ? *audiobytes2: 0, writecursor);
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

	TRACE("(%p,%d)\n",This,newpos);

	/* **** */
	RtlAcquireResourceExclusive(&This->lock, TRUE);

	/* start mixing from this new location instead */
	newpos %= This->buflen;
	newpos -= newpos%This->pwfx->nBlockAlign;
	This->sec_mixpos = newpos;

	/* at this point, do not attempt to reset buffers, mess with primary mix position,
           or anything like that to reduce latency. The data already prebuffered cannot be changed */

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

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLPAN)) {
		WARN("control unavailable\n");
		return DSERR_CONTROLUNAVAIL;
	}

	/* **** */
	RtlAcquireResourceExclusive(&This->lock, TRUE);

	if (This->volpan.lPan != pan) {
		This->volpan.lPan = pan;
		DSOUND_RecalcVolPan(&(This->volpan));
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

	if (!p2)
		x2 = 0;

	if((p1 && ((BYTE*)p1 < This->buffer->memory || (BYTE*)p1 >= This->buffer->memory + This->buflen)) ||
	   (p2 && ((BYTE*)p2 < This->buffer->memory || (BYTE*)p2 >= This->buffer->memory + This->buflen)))
		return DSERR_INVALIDPARAM;

	if (x1 || x2)
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
			      iter->buffer->lockedbytes -= x1;
                        }

			if (x2)
			{
			    if(x2 + (DWORD_PTR)p2 - (DWORD_PTR)iter->buffer->memory > iter->buflen)
			      hres = DSERR_INVALIDPARAM;
			    else
			      iter->buffer->lockedbytes -= x2;
			}
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
	DSFilter *filters;
	HRESULT hr, hr2;
	DMO_MEDIA_TYPE dmt;
	WAVEFORMATEX wfx;

	TRACE("(%p,%u,%p,%p)\n", This, dwEffectsCount, pDSFXDesc, pdwResultCodes);

	if (pdwResultCodes)
		for (u=0; u<dwEffectsCount; u++) pdwResultCodes[u] = DSFXR_UNKNOWN;

	if ((dwEffectsCount > 0 && !pDSFXDesc) ||
		(dwEffectsCount == 0 && (pDSFXDesc || pdwResultCodes))
	)
		return E_INVALIDARG;

	if (!(This->dsbd.dwFlags & DSBCAPS_CTRLFX)) {
		WARN("attempted to call SetFX on buffer without DSBCAPS_CTRLFX\n");
		return DSERR_CONTROLUNAVAIL;
	}

	if (This->state != STATE_STOPPED)
		return DSERR_INVALIDCALL;

	if (This->buffer->lockedbytes > 0)
		return DSERR_INVALIDCALL;

	if (dwEffectsCount == 0) {
		if (This->num_filters > 0) {
			for (u = 0; u < This->num_filters; u++) {
				IMediaObject_Release(This->filters[u].obj);
			}
			HeapFree(GetProcessHeap(), 0, This->filters);

			This->filters = NULL;
			This->num_filters = 0;
		}

		return DS_OK;
	}

	filters = HeapAlloc(GetProcessHeap(), 0, dwEffectsCount * sizeof(DSFilter));
	if (!filters) {
		WARN("out of memory\n");
		return DSERR_OUTOFMEMORY;
	}

	hr = DS_OK;

	wfx.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
	wfx.nChannels = This->pwfx->nChannels;
	wfx.nSamplesPerSec = This->pwfx->nSamplesPerSec;
	wfx.wBitsPerSample = sizeof(float) * 8;
	wfx.nBlockAlign = (wfx.nChannels * wfx.wBitsPerSample)/8;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.cbSize = sizeof(wfx);

	dmt.majortype = KSDATAFORMAT_TYPE_AUDIO;
	dmt.subtype = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
	dmt.bFixedSizeSamples = TRUE;
	dmt.bTemporalCompression = FALSE;
	dmt.lSampleSize = sizeof(float) * This->pwfx->nChannels / 8;
	dmt.formattype = FORMAT_WaveFormatEx;
	dmt.pUnk = NULL;
	dmt.cbFormat = sizeof(WAVEFORMATEX);
	dmt.pbFormat = (BYTE*)&wfx;

	for (u = 0; u < dwEffectsCount; u++) {
		hr2 = CoCreateInstance(&pDSFXDesc[u].guidDSFXClass, NULL, CLSCTX_INPROC_SERVER, &IID_IMediaObject, (LPVOID*)&filters[u].obj);

		if (SUCCEEDED(hr2)) {
			hr2 = IMediaObject_SetInputType(filters[u].obj, 0, &dmt, 0);
			if (FAILED(hr2))
				WARN("Could not set DMO input type\n");
		}

		if (SUCCEEDED(hr2)) {
			hr2 = IMediaObject_SetOutputType(filters[u].obj, 0, &dmt, 0);
			if (FAILED(hr2))
				WARN("Could not set DMO output type\n");
		}

		if (FAILED(hr2)) {
			if (hr == DS_OK)
				hr = hr2;

			if (pdwResultCodes)
				pdwResultCodes[u] = (hr2 == REGDB_E_CLASSNOTREG) ? DSFXR_UNKNOWN : DSFXR_FAILED;
		} else {
			if (pdwResultCodes)
				pdwResultCodes[u] = DSFXR_LOCSOFTWARE;
		}
	}

	if (FAILED(hr)) {
		for (u = 0; u < dwEffectsCount; u++) {
			if (pdwResultCodes)
				pdwResultCodes[u] = (pdwResultCodes[u] != DSFXR_UNKNOWN) ? DSFXR_PRESENT : DSFXR_UNKNOWN;

			if (filters[u].obj)
				IMediaObject_Release(filters[u].obj);
		}

		HeapFree(GetProcessHeap(), 0, filters);
	} else {
		if (This->num_filters > 0) {
			for (u = 0; u < This->num_filters; u++) {
				IMediaObject_Release(This->filters[u].obj);
				if (This->filters[u].inplace) IMediaObjectInPlace_Release(This->filters[u].inplace);
			}
			HeapFree(GetProcessHeap(), 0, This->filters);
		}

		for (u = 0; u < dwEffectsCount; u++) {
			memcpy(&filters[u].guid, &pDSFXDesc[u].guidDSFXClass, sizeof(GUID));
			if (FAILED(IMediaObject_QueryInterface(filters[u].obj, &IID_IMediaObjectInPlace, (void*)&filters[u].inplace)))
				filters[u].inplace = NULL;
		}

		This->filters = filters;
		This->num_filters = dwEffectsCount;
	}

	return hr;
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

	TRACE("(%p,%s,%u,%s,%p)\n",This,debugstr_guid(rguidObject),dwIndex,debugstr_guid(rguidInterface),ppObject);

	if (dwIndex >= This->num_filters)
		return DSERR_CONTROLUNAVAIL;

	if (!ppObject)
		return E_INVALIDARG;

	if (IsEqualGUID(rguidObject, &This->filters[dwIndex].guid) || IsEqualGUID(rguidObject, &GUID_All_Objects)) {
		if (SUCCEEDED(IMediaObject_QueryInterface(This->filters[dwIndex].obj, rguidInterface, ppObject)))
			return DS_OK;
		else
			return E_NOINTERFACE;
	} else {
		WARN("control unavailable\n");
		return DSERR_OBJECTNOTFOUND;
	}
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
	caps->dwFlags |= DSBCAPS_LOCSOFTWARE;

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
                IDirectSoundNotify_AddRef(&This->IDirectSoundNotify_iface);
                *ppobj = &This->IDirectSoundNotify_iface;
                return S_OK;
	}

	if ( IsEqualGUID( &IID_IDirectSound3DBuffer, riid ) ) {
            if(This->dsbd.dwFlags & DSBCAPS_CTRL3D){
                IDirectSound3DBuffer_AddRef(&This->IDirectSound3DBuffer_iface);
                *ppobj = &This->IDirectSound3DBuffer_iface;
                return S_OK;
            }
            TRACE("app requested IDirectSound3DBuffer on non-3D secondary buffer\n");
            return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IDirectSound3DListener, riid ) ) {
		ERR("app requested IDirectSound3DListener on secondary buffer\n");
		return E_NOINTERFACE;
	}

	if ( IsEqualGUID( &IID_IKsPropertySet, riid ) ) {
                IKsPropertySet_AddRef(&This->IKsPropertySet_iface);
                *ppobj = &This->IKsPropertySet_iface;
                return S_OK;
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

	dsb->ref = 0;
        dsb->refn = 0;
        dsb->ref3D = 0;
        dsb->refiks = 0;
	dsb->numIfaces = 0;
	dsb->device = device;
	dsb->IDirectSoundBuffer8_iface.lpVtbl = &dsbvt;
        dsb->IDirectSoundNotify_iface.lpVtbl = &dsnvt;
        dsb->IDirectSound3DBuffer_iface.lpVtbl = &ds3dbvt;
        dsb->IKsPropertySet_iface.lpVtbl = &iksbvt;

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
	dsb->notifies = NULL;
	dsb->nrofnotifies = 0;

	/* Check necessary hardware mixing capabilities */
	if (wfex->nChannels==2) capf |= DSCAPS_SECONDARYSTEREO;
	else capf |= DSCAPS_SECONDARYMONO;
	if (wfex->wBitsPerSample==16) capf |= DSCAPS_SECONDARY16BIT;
	else capf |= DSCAPS_SECONDARY8BIT;

	TRACE("capf = 0x%08x, device->drvcaps.dwFlags = 0x%08x\n", capf, device->drvcaps.dwFlags);

	/* Allocate an empty buffer */
	dsb->buffer = HeapAlloc(GetProcessHeap(),0,sizeof(*(dsb->buffer)));
	if (dsb->buffer == NULL) {
		WARN("out of memory\n");
		HeapFree(GetProcessHeap(),0,dsb->pwfx);
		HeapFree(GetProcessHeap(),0,dsb);
		*pdsb = NULL;
		return DSERR_OUTOFMEMORY;
	}

	/* Allocate system memory for buffer */
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
	dsb->buffer->lockedbytes = 0;
	list_init(&dsb->buffer->buffers);
	list_add_head(&dsb->buffer->buffers, &dsb->entry);
	FillMemory(dsb->buffer->memory, dsb->buflen, dsbd->lpwfxFormat->wBitsPerSample == 8 ? 128 : 0);

	/* It's not necessary to initialize values to zero since */
	/* we allocated this structure with HEAP_ZERO_MEMORY... */
	dsb->sec_mixpos = 0;
	dsb->state = STATE_STOPPED;

	dsb->freqAdjustNum = dsb->freq;
	dsb->freqAdjustDen = device->pwfx->nSamplesPerSec;
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
        init_eax_buffer(dsb);

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

        IDirectSoundBuffer8_AddRef(&dsb->IDirectSoundBuffer8_iface);
	*pdsb = dsb;
	return err;
}

void secondarybuffer_destroy(IDirectSoundBufferImpl *This)
{
    ULONG ref = InterlockedIncrement(&This->numIfaces);

    if (ref > 1)
        WARN("Destroying buffer with %u in use interfaces\n", ref - 1);

    if (This->dsbd.dwFlags & DSBCAPS_LOCHARDWARE)
        This->device->drvcaps.dwFreeHwMixingAllBuffers++;

    DirectSoundDevice_RemoveBuffer(This->device, This);
    RtlDeleteResource(&This->lock);

    This->buffer->ref--;
    list_remove(&This->entry);
    if (This->buffer->ref == 0) {
        HeapFree(GetProcessHeap(), 0, This->buffer->memory);
        HeapFree(GetProcessHeap(), 0, This->buffer);
    }

    HeapFree(GetProcessHeap(), 0, This->notifies);
    HeapFree(GetProcessHeap(), 0, This->pwfx);

    if (This->filters) {
        int i;
        for (i = 0; i < This->num_filters; i++) {
            IMediaObject_Release(This->filters[i].obj);
            if (This->filters[i].inplace) IMediaObjectInPlace_Release(This->filters[i].inplace);
        }
        HeapFree(GetProcessHeap(), 0, This->filters);
    }

    free_eax_buffer(This);

    HeapFree(GetProcessHeap(), 0, This);

    TRACE("(%p) released\n", This);
}

HRESULT IDirectSoundBufferImpl_Duplicate(
    DirectSoundDevice *device,
    IDirectSoundBufferImpl **ppdsb,
    IDirectSoundBufferImpl *pdsb)
{
    IDirectSoundBufferImpl *dsb;
    HRESULT hres = DS_OK;
    TRACE("(%p,%p,%p)\n", device, ppdsb, pdsb);

    dsb = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*dsb));
    if (dsb == NULL) {
        WARN("out of memory\n");
        *ppdsb = NULL;
        return DSERR_OUTOFMEMORY;
    }

    RtlAcquireResourceShared(&pdsb->lock, TRUE);

    CopyMemory(dsb, pdsb, sizeof(*dsb));

    dsb->pwfx = DSOUND_CopyFormat(pdsb->pwfx);

    RtlReleaseResource(&pdsb->lock);

    if (dsb->pwfx == NULL) {
        HeapFree(GetProcessHeap(),0,dsb);
        *ppdsb = NULL;
        return DSERR_OUTOFMEMORY;
    }

    dsb->buffer->ref++;
    list_add_head(&dsb->buffer->buffers, &dsb->entry);
    dsb->ref = 0;
    dsb->refn = 0;
    dsb->ref3D = 0;
    dsb->refiks = 0;
    dsb->numIfaces = 0;
    dsb->state = STATE_STOPPED;
    dsb->sec_mixpos = 0;
    dsb->notifies = NULL;
    dsb->nrofnotifies = 0;
    dsb->device = device;
    DSOUND_RecalcFormat(dsb);

    RtlInitializeResource(&dsb->lock);

    init_eax_buffer(dsb); /* FIXME: should we duplicate EAX properties? */

    /* register buffer */
    hres = DirectSoundDevice_AddBuffer(device, dsb);
    if (hres != DS_OK) {
        RtlDeleteResource(&dsb->lock);
        list_remove(&dsb->entry);
        dsb->buffer->ref--;
        HeapFree(GetProcessHeap(),0,dsb->pwfx);
        HeapFree(GetProcessHeap(),0,dsb);
        dsb = NULL;
    }else
        IDirectSoundBuffer8_AddRef(&dsb->IDirectSoundBuffer8_iface);

    *ppdsb = dsb;
    return hres;
}

/*******************************************************************************
 *              IKsPropertySet
 */

static inline IDirectSoundBufferImpl *impl_from_IKsPropertySet(IKsPropertySet *iface)
{
    return CONTAINING_RECORD(iface, IDirectSoundBufferImpl, IKsPropertySet_iface);
}

/* IUnknown methods */
static HRESULT WINAPI IKsPropertySetImpl_QueryInterface(IKsPropertySet *iface, REFIID riid,
        void **ppobj)
{
    IDirectSoundBufferImpl *This = impl_from_IKsPropertySet(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    return IDirectSoundBuffer_QueryInterface(&This->IDirectSoundBuffer8_iface, riid, ppobj);
}

static ULONG WINAPI IKsPropertySetImpl_AddRef(IKsPropertySet *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IKsPropertySet(iface);
    ULONG ref = InterlockedIncrement(&This->refiks);

    TRACE("(%p) ref was %d\n", This, ref - 1);

    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI IKsPropertySetImpl_Release(IKsPropertySet *iface)
{
    IDirectSoundBufferImpl *This = impl_from_IKsPropertySet(iface);
    ULONG ref;

    if (is_primary_buffer(This)){
        ref = capped_refcount_dec(&This->refiks);
        if(!ref)
            capped_refcount_dec(&This->numIfaces);
        TRACE("(%p) ref is now: %d\n", This, ref);
        return ref;
    }

    ref = InterlockedDecrement(&This->refiks);
    if (!ref && !InterlockedDecrement(&This->numIfaces))
        secondarybuffer_destroy(This);

    TRACE("(%p) ref is now %d\n", This, ref);

    return ref;
}

static HRESULT WINAPI IKsPropertySetImpl_Get(IKsPropertySet *iface, REFGUID guidPropSet,
        ULONG dwPropID, void *pInstanceData, ULONG cbInstanceData, void *pPropData,
        ULONG cbPropData, ULONG *pcbReturned)
{
    IDirectSoundBufferImpl *This = impl_from_IKsPropertySet(iface);

    TRACE("(iface=%p,guidPropSet=%s,dwPropID=%d,pInstanceData=%p,cbInstanceData=%d,pPropData=%p,cbPropData=%d,pcbReturned=%p)\n",
    This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData,pcbReturned);

    if (IsEqualGUID(&DSPROPSETID_EAX_ReverbProperties, guidPropSet) || IsEqualGUID(&DSPROPSETID_EAXBUFFER_ReverbProperties, guidPropSet))
        return EAX_Get(This, guidPropSet, dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData, pcbReturned);

    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsPropertySetImpl_Set(IKsPropertySet *iface, REFGUID guidPropSet,
        ULONG dwPropID, void *pInstanceData, ULONG cbInstanceData, void *pPropData,
        ULONG cbPropData)
{
    IDirectSoundBufferImpl *This = impl_from_IKsPropertySet(iface);

    TRACE("(%p,%s,%d,%p,%d,%p,%d)\n",This,debugstr_guid(guidPropSet),dwPropID,pInstanceData,cbInstanceData,pPropData,cbPropData);

    if (IsEqualGUID(&DSPROPSETID_EAX_ReverbProperties, guidPropSet) || IsEqualGUID(&DSPROPSETID_EAXBUFFER_ReverbProperties, guidPropSet))
        return EAX_Set(This, guidPropSet, dwPropID, pInstanceData, cbInstanceData, pPropData, cbPropData);

    return E_PROP_ID_UNSUPPORTED;
}

static HRESULT WINAPI IKsPropertySetImpl_QuerySupport(IKsPropertySet *iface, REFGUID guidPropSet,
        ULONG dwPropID, ULONG *pTypeSupport)
{
    IDirectSoundBufferImpl *This = impl_from_IKsPropertySet(iface);

    TRACE("(%p,%s,%d,%p)\n",This,debugstr_guid(guidPropSet),dwPropID,pTypeSupport);

    if (EAX_QuerySupport(guidPropSet, dwPropID, pTypeSupport)) {
        static int once;
        if (!once++)
            FIXME_(winediag)("EAX sound effects are enabled - try to disable it if your app crashes unexpectedly\n");
        return S_OK;
    }

    return E_PROP_ID_UNSUPPORTED;
}

const IKsPropertySetVtbl iksbvt = {
    IKsPropertySetImpl_QueryInterface,
    IKsPropertySetImpl_AddRef,
    IKsPropertySetImpl_Release,
    IKsPropertySetImpl_Get,
    IKsPropertySetImpl_Set,
    IKsPropertySetImpl_QuerySupport
};
