/* DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
 * Copyright 2004 Robert Reif
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/winbase16.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"
#include "dsconf.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

static ULONG WINAPI IDirectSound_IUnknown_AddRef(LPUNKNOWN iface);
static ULONG WINAPI IDirectSound_IDirectSound_AddRef(LPDIRECTSOUND iface);
static ULONG WINAPI IDirectSound8_IUnknown_AddRef(LPUNKNOWN iface);
static ULONG WINAPI IDirectSound8_IDirectSound_AddRef(LPDIRECTSOUND iface);
static ULONG WINAPI IDirectSound8_IDirectSound8_AddRef(LPDIRECTSOUND8 iface);

static const char * dumpCooperativeLevel(DWORD level)
{
    static char unknown[32];
#define LE(x) case x: return #x
    switch (level) {
        LE(DSSCL_NORMAL);
        LE(DSSCL_PRIORITY);
        LE(DSSCL_EXCLUSIVE);
        LE(DSSCL_WRITEPRIMARY);
    }
#undef LE
    sprintf(unknown, "Unknown(%08lx)", level);
    return unknown;
}

static void _dump_DSCAPS(DWORD xmask) {
    struct {
        DWORD   mask;
        char    *name;
    } flags[] = {
#define FE(x) { x, #x },
        FE(DSCAPS_PRIMARYMONO)
        FE(DSCAPS_PRIMARYSTEREO)
        FE(DSCAPS_PRIMARY8BIT)
        FE(DSCAPS_PRIMARY16BIT)
        FE(DSCAPS_CONTINUOUSRATE)
        FE(DSCAPS_EMULDRIVER)
        FE(DSCAPS_CERTIFIED)
        FE(DSCAPS_SECONDARYMONO)
        FE(DSCAPS_SECONDARYSTEREO)
        FE(DSCAPS_SECONDARY8BIT)
        FE(DSCAPS_SECONDARY16BIT)
#undef FE
    };
    unsigned int     i;

    for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
        if ((flags[i].mask & xmask) == flags[i].mask)
            DPRINTF("%s ",flags[i].name);
}

static void _dump_DSBCAPS(DWORD xmask) {
    struct {
        DWORD   mask;
        char    *name;
    } flags[] = {
#define FE(x) { x, #x },
        FE(DSBCAPS_PRIMARYBUFFER)
        FE(DSBCAPS_STATIC)
        FE(DSBCAPS_LOCHARDWARE)
        FE(DSBCAPS_LOCSOFTWARE)
        FE(DSBCAPS_CTRL3D)
        FE(DSBCAPS_CTRLFREQUENCY)
        FE(DSBCAPS_CTRLPAN)
        FE(DSBCAPS_CTRLVOLUME)
        FE(DSBCAPS_CTRLPOSITIONNOTIFY)
        FE(DSBCAPS_STICKYFOCUS)
        FE(DSBCAPS_GLOBALFOCUS)
        FE(DSBCAPS_GETCURRENTPOSITION2)
        FE(DSBCAPS_MUTE3DATMAXDISTANCE)
#undef FE
    };
    unsigned int     i;

    for (i=0;i<sizeof(flags)/sizeof(flags[0]);i++)
        if ((flags[i].mask & xmask) == flags[i].mask)
            DPRINTF("%s ",flags[i].name);
}

/*******************************************************************************
 *		IDirectSoundImpl_DirectSound
 */
static HRESULT WINAPI IDirectSoundImpl_QueryInterface(
    LPDIRECTSOUND8 iface,
    REFIID riid,
    LPVOID * ppobj)
{
    TRACE("(%p,%s,%p)\n",iface,debugstr_guid(riid),ppobj);
    FIXME("shouldn't be called directly\n");
    return E_NOINTERFACE;
}

static HRESULT WINAPI DSOUND_QueryInterface(
    LPDIRECTSOUND8 iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if (ppobj == NULL) {
        WARN("invalid parameter\n");
        return E_INVALIDARG;
    }

    if (IsEqualIID(riid, &IID_IUnknown)) {
        if (!This->pUnknown) {
            IDirectSound_IUnknown_Create(iface, &This->pUnknown);
            if (!This->pUnknown) {
                WARN("IDirectSound_IUnknown_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSound_IUnknown_AddRef(This->pUnknown);
        *ppobj = This->pUnknown;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSound)) {
        if (!This->pDS) {
            IDirectSound_IDirectSound_Create(iface, &This->pDS);
            if (!This->pDS) {
                WARN("IDirectSound_IDirectSound_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSound_IDirectSound_AddRef(This->pDS);
        *ppobj = This->pDS;
        return S_OK;
    }

    *ppobj = NULL;
    WARN("Unknown IID %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI DSOUND_QueryInterface8(
    LPDIRECTSOUND8 iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);

    if (ppobj == NULL) {
        WARN("invalid parameter\n");
        return E_INVALIDARG;
    }

    if (IsEqualIID(riid, &IID_IUnknown)) {
        if (!This->pUnknown) {
            IDirectSound8_IUnknown_Create(iface, &This->pUnknown);
            if (!This->pUnknown) {
                WARN("IDirectSound8_IUnknown_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSound8_IUnknown_AddRef(This->pUnknown);
        *ppobj = This->pUnknown;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSound)) {
        if (!This->pDS) {
            IDirectSound8_IDirectSound_Create(iface, &This->pDS);
            if (!This->pDS) {
                WARN("IDirectSound8_IDirectSound_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSound8_IDirectSound_AddRef(This->pDS);
        *ppobj = This->pDS;
        return S_OK;
    } else if (IsEqualIID(riid, &IID_IDirectSound8)) {
        if (!This->pDS8) {
            IDirectSound8_IDirectSound8_Create(iface, &This->pDS8);
            if (!This->pDS8) {
                WARN("IDirectSound8_IDirectSound8_Create() failed\n");
                *ppobj = NULL;
                return E_NOINTERFACE;
            }
        }
        IDirectSound8_IDirectSound8_AddRef(This->pDS8);
        *ppobj = This->pDS8;
        return S_OK;
    }

    *ppobj = NULL;
    WARN("Unknown IID %s\n",debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirectSoundImpl_AddRef(
    LPDIRECTSOUND8 iface)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());

    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IDirectSoundImpl_Release(
    LPDIRECTSOUND8 iface)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    ULONG ref;
    TRACE("(%p) ref was %ld, thread is %04lx\n",
          This, This->ref, GetCurrentThreadId());

    ref = InterlockedDecrement(&(This->ref));
    if (ref == 0) {
        HRESULT hres;
        INT i;

        timeKillEvent(This->timerID);
        timeEndPeriod(DS_TIME_RES);
        /* wait for timer to expire */
        Sleep(DS_TIME_RES+1);

        /* The sleep above should have allowed the timer process to expire
         * but try to grab the lock just in case. Can't hold lock because
         * IDirectSoundBufferImpl_Destroy also grabs the lock */
        RtlAcquireResourceShared(&(This->buffer_list_lock), TRUE);
        RtlReleaseResource(&(This->buffer_list_lock));

        /* It is allowed to release this object even when buffers are playing */
        if (This->buffers) {
            WARN("%d secondary buffers not released\n", This->nrofbuffers);
            for( i=0;i<This->nrofbuffers;i++)
                IDirectSoundBufferImpl_Destroy(This->buffers[i]);
        }

        if (This->primary) {
            WARN("primary buffer not released\n");
            IDirectSoundBuffer8_Release((LPDIRECTSOUNDBUFFER8)This->primary);
        }

        hres = DSOUND_PrimaryDestroy(This);
        if (hres != DS_OK)
            WARN("DSOUND_PrimaryDestroy failed\n");

        if (This->driver)
            IDsDriver_Close(This->driver);

        if (This->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMOPEN)
            waveOutClose(This->hwo);

        if (This->driver)
            IDsDriver_Release(This->driver);

        RtlDeleteResource(&This->buffer_list_lock);
        This->mixlock.DebugInfo->Spare[1] = 0;
        DeleteCriticalSection(&This->mixlock);
        HeapFree(GetProcessHeap(),0,This);
        dsound = NULL;
        TRACE("(%p) released\n",This);
    }

    return ref;
}

static HRESULT WINAPI IDirectSoundImpl_CreateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);
    FIXME("shouldn't be called directly\n");
    return DSERR_GENERIC;
}

static HRESULT WINAPI DSOUND_CreateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk,
    BOOL from8)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    HRESULT hres = DS_OK;
    TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->initialized == FALSE) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (dsbd == NULL) {
        WARN("invalid parameter: dsbd == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (dsbd->dwSize != sizeof(DSBUFFERDESC) &&
        dsbd->dwSize != sizeof(DSBUFFERDESC1)) {
        WARN("invalid parameter: dsbd\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppdsb == NULL) {
        WARN("invalid parameter: ppdsb == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (TRACE_ON(dsound)) {
        TRACE("(structsize=%ld)\n",dsbd->dwSize);
        TRACE("(flags=0x%08lx:\n",dsbd->dwFlags);
        _dump_DSBCAPS(dsbd->dwFlags);
        DPRINTF(")\n");
        TRACE("(bufferbytes=%ld)\n",dsbd->dwBufferBytes);
        TRACE("(lpwfxFormat=%p)\n",dsbd->lpwfxFormat);
    }

    if (dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER) {
        if (dsbd->lpwfxFormat != NULL) {
            WARN("invalid parameter: dsbd->lpwfxFormat must be NULL for "
                 "primary buffer\n");
            return DSERR_INVALIDPARAM;
        }

        if (This->primary) {
            WARN("Primary Buffer already created\n");
            IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)(This->primary));
            *ppdsb = (LPDIRECTSOUNDBUFFER)(This->primary);
        } else {
           This->dsbd = *dsbd;
           hres = PrimaryBufferImpl_Create(This, (PrimaryBufferImpl**)&(This->primary), &(This->dsbd));
           if (This->primary) {
               IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)(This->primary));
               *ppdsb = (LPDIRECTSOUNDBUFFER)(This->primary);
           } else
               WARN("PrimaryBufferImpl_Create failed\n");
        }
    } else {
        IDirectSoundBufferImpl * dsb;

        if (dsbd->lpwfxFormat == NULL) {
            WARN("invalid parameter: dsbd->lpwfxFormat can't be NULL for "
                 "secondary buffer\n");
            return DSERR_INVALIDPARAM;
        }

        TRACE("(formattag=0x%04x,chans=%d,samplerate=%ld,"
              "bytespersec=%ld,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
              dsbd->lpwfxFormat->wFormatTag, dsbd->lpwfxFormat->nChannels,
              dsbd->lpwfxFormat->nSamplesPerSec,
              dsbd->lpwfxFormat->nAvgBytesPerSec,
              dsbd->lpwfxFormat->nBlockAlign,
              dsbd->lpwfxFormat->wBitsPerSample, dsbd->lpwfxFormat->cbSize);

        if (from8 && (dsbd->dwFlags & DSBCAPS_CTRL3D) && (dsbd->lpwfxFormat->nChannels != 1)) {
            WARN("invalid parameter: 3D buffer format must be mono\n");
            return DSERR_INVALIDPARAM;
        }

        hres = IDirectSoundBufferImpl_Create(This, (IDirectSoundBufferImpl**)&dsb, dsbd);
        if (dsb) {
            hres = SecondaryBufferImpl_Create(dsb, (SecondaryBufferImpl**)ppdsb);
            if (*ppdsb) {
                dsb->dsb = (SecondaryBufferImpl*)*ppdsb;
                IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER)*ppdsb);
            } else
                WARN("SecondaryBufferImpl_Create failed\n");
        } else
           WARN("IDirectSoundBufferImpl_Create failed\n");
   }

   return hres;
}

static HRESULT WINAPI IDirectSoundImpl_GetCaps(
    LPDIRECTSOUND8 iface,
    LPDSCAPS lpDSCaps)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p,%p)\n",This,lpDSCaps);

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->initialized == FALSE) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (lpDSCaps == NULL) {
        WARN("invalid parameter: lpDSCaps = NULL\n");
        return DSERR_INVALIDPARAM;
    }

    /* check if there is enough room */
    if (lpDSCaps->dwSize < sizeof(*lpDSCaps)) {
        WARN("invalid parameter: lpDSCaps->dwSize = %ld < %d\n",
             lpDSCaps->dwSize, sizeof(*lpDSCaps));
        return DSERR_INVALIDPARAM;
    }

    lpDSCaps->dwFlags                           = This->drvcaps.dwFlags;
    if (TRACE_ON(dsound)) {
        TRACE("(flags=0x%08lx:\n",lpDSCaps->dwFlags);
        _dump_DSCAPS(lpDSCaps->dwFlags);
        DPRINTF(")\n");
    }
    lpDSCaps->dwMinSecondarySampleRate          = This->drvcaps.dwMinSecondarySampleRate;
    lpDSCaps->dwMaxSecondarySampleRate          = This->drvcaps.dwMaxSecondarySampleRate;
    lpDSCaps->dwPrimaryBuffers                  = This->drvcaps.dwPrimaryBuffers;
    lpDSCaps->dwMaxHwMixingAllBuffers           = This->drvcaps.dwMaxHwMixingAllBuffers;
    lpDSCaps->dwMaxHwMixingStaticBuffers        = This->drvcaps.dwMaxHwMixingStaticBuffers;
    lpDSCaps->dwMaxHwMixingStreamingBuffers     = This->drvcaps.dwMaxHwMixingStreamingBuffers;
    lpDSCaps->dwFreeHwMixingAllBuffers          = This->drvcaps.dwFreeHwMixingAllBuffers;
    lpDSCaps->dwFreeHwMixingStaticBuffers       = This->drvcaps.dwFreeHwMixingStaticBuffers;
    lpDSCaps->dwFreeHwMixingStreamingBuffers    = This->drvcaps.dwFreeHwMixingStreamingBuffers;
    lpDSCaps->dwMaxHw3DAllBuffers               = This->drvcaps.dwMaxHw3DAllBuffers;
    lpDSCaps->dwMaxHw3DStaticBuffers            = This->drvcaps.dwMaxHw3DStaticBuffers;
    lpDSCaps->dwMaxHw3DStreamingBuffers         = This->drvcaps.dwMaxHw3DStreamingBuffers;
    lpDSCaps->dwFreeHw3DAllBuffers              = This->drvcaps.dwFreeHw3DAllBuffers;
    lpDSCaps->dwFreeHw3DStaticBuffers           = This->drvcaps.dwFreeHw3DStaticBuffers;
    lpDSCaps->dwFreeHw3DStreamingBuffers        = This->drvcaps.dwFreeHw3DStreamingBuffers;
    lpDSCaps->dwTotalHwMemBytes                 = This->drvcaps.dwTotalHwMemBytes;
    lpDSCaps->dwFreeHwMemBytes                  = This->drvcaps.dwFreeHwMemBytes;
    lpDSCaps->dwMaxContigFreeHwMemBytes         = This->drvcaps.dwMaxContigFreeHwMemBytes;

    /* driver doesn't have these */
    lpDSCaps->dwUnlockTransferRateHwBuffers     = 4096; /* But we have none... */
    lpDSCaps->dwPlayCpuOverheadSwBuffers        = 1;    /* 1% */

    return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_DuplicateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb)
{
    IDirectSoundBufferImpl* pdsb;
    IDirectSoundBufferImpl* dsb;
    HRESULT hres = DS_OK;
    int size;
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;

    TRACE("(%p,%p,%p)\n",This,psb,ppdsb);

    if (This == NULL) {
        WARN("invalid parameter: This == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (This->initialized == FALSE) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (psb == NULL) {
        WARN("invalid parameter: psb == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppdsb == NULL) {
        WARN("invalid parameter: ppdsb == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    /* FIXME: hack to make sure we have a secondary buffer */
    if ((DWORD)((SecondaryBufferImpl *)psb)->dsb == (DWORD)This) {
        WARN("trying to duplicate primary buffer\n");
        *ppdsb = NULL;
        return DSERR_INVALIDCALL;
    }

    pdsb = ((SecondaryBufferImpl *)psb)->dsb;

    dsb = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*dsb));

    if (dsb == NULL) {
        WARN("out of memory\n");
        *ppdsb = NULL;
        return DSERR_OUTOFMEMORY;
    }

    memcpy(dsb, pdsb, sizeof(IDirectSoundBufferImpl));

    if (pdsb->hwbuf) {
        TRACE("duplicating hardware buffer\n");

        hres = IDsDriver_DuplicateSoundBuffer(This->driver, pdsb->hwbuf, (LPVOID *)&dsb->hwbuf);
        if (hres != DS_OK) {
            TRACE("IDsDriver_DuplicateSoundBuffer failed, falling back to software buffer\n");
            dsb->hwbuf = NULL;
            /* allocate buffer */
            if (This->drvdesc.dwFlags & DSDDESC_USESYSTEMMEMORY) {
                dsb->buffer = HeapAlloc(GetProcessHeap(),0,sizeof(*(dsb->buffer)));
                if (dsb->buffer == NULL) {
                    WARN("out of memory\n");
                    HeapFree(GetProcessHeap(),0,dsb);
                    *ppdsb = NULL;
                    return DSERR_OUTOFMEMORY;
                }

                dsb->buffer->memory = HeapAlloc(GetProcessHeap(),0,dsb->buflen);
                if (dsb->buffer->memory == NULL) {
                    WARN("out of memory\n");
                    HeapFree(GetProcessHeap(),0,dsb->buffer);
                    HeapFree(GetProcessHeap(),0,dsb);
                    *ppdsb = NULL;
                    return DSERR_OUTOFMEMORY;
                }
                dsb->buffer->ref = 1;

                /* FIXME: copy buffer ? */
            }
        }
    } else {
        dsb->hwbuf = NULL;
        dsb->buffer->ref++;
    }

    dsb->ref = 0;
    dsb->state = STATE_STOPPED;
    dsb->playpos = 0;
    dsb->buf_mixpos = 0;
    dsb->dsound = This;
    dsb->ds3db = NULL;
    dsb->iks = NULL; /* FIXME? */
    dsb->dsb = NULL;

    /* variable sized struct so calculate size based on format */
    size = sizeof(WAVEFORMATEX) + pdsb->pwfx->cbSize;

    dsb->pwfx = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size);
    if (dsb->pwfx == NULL) {
            WARN("out of memory\n");
            HeapFree(GetProcessHeap(),0,dsb->buffer);
            HeapFree(GetProcessHeap(),0,dsb);
            *ppdsb = NULL;
            return DSERR_OUTOFMEMORY;
    }

    memcpy(dsb->pwfx, pdsb->pwfx, size);

    InitializeCriticalSection(&(dsb->lock));
    dsb->lock.DebugInfo->Spare[1] = (DWORD)"DSOUNDBUFFER_lock";

    /* register buffer */
    hres = DSOUND_AddBuffer(This, dsb);
    if (hres != DS_OK) {
        IDirectSoundBuffer8_Release(psb);
        dsb->lock.DebugInfo->Spare[1] = 0;
        DeleteCriticalSection(&(dsb->lock));
        HeapFree(GetProcessHeap(),0,dsb->buffer);
        HeapFree(GetProcessHeap(),0,dsb->pwfx);
        HeapFree(GetProcessHeap(),0,dsb);
        *ppdsb = 0;
    } else {
        hres = SecondaryBufferImpl_Create(dsb, (SecondaryBufferImpl**)ppdsb);
        if (*ppdsb) {
            dsb->dsb = (SecondaryBufferImpl*)*ppdsb;
            IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)*ppdsb);
        } else
            WARN("SecondaryBufferImpl_Create failed\n");
    }

    return hres;
}

static HRESULT WINAPI IDirectSoundImpl_SetCooperativeLevel(
    LPDIRECTSOUND8 iface,
    HWND hwnd,
    DWORD level)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p,%08lx,%s)\n",This,(DWORD)hwnd,dumpCooperativeLevel(level));

    if (level==DSSCL_PRIORITY || level==DSSCL_EXCLUSIVE) {
        FIXME("level=%s not fully supported\n",
              level==DSSCL_PRIORITY ? "DSSCL_PRIORITY" : "DSSCL_EXCLUSIVE");
    }
    This->priolevel = level;
    return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_Compact(
    LPDIRECTSOUND8 iface)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p)\n",This);

    if (This->initialized == FALSE) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (This->priolevel != DSSCL_PRIORITY) {
        WARN("incorrect priority level\n");
        return DSERR_PRIOLEVELNEEDED;
    }

    return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_GetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    LPDWORD lpdwSpeakerConfig)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p, %p)\n",This,lpdwSpeakerConfig);

    if (This->initialized == FALSE) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (lpdwSpeakerConfig == NULL) {
        WARN("invalid parameter: lpdwSpeakerConfig == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    WARN("not fully functional\n");
    *lpdwSpeakerConfig = This->speaker_config;
    return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_SetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    DWORD config)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p,0x%08lx)\n",This,config);

    if (This->initialized == FALSE) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    This->speaker_config = config;
    WARN("not fully functional\n");
    return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_Initialize(
    LPDIRECTSOUND8 iface,
    LPCGUID lpcGuid)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p,%s)\n",This,debugstr_guid(lpcGuid));

    This->initialized = TRUE;

    return DS_OK;
}

static HRESULT WINAPI IDirectSoundImpl_VerifyCertification(
    LPDIRECTSOUND8 iface,
    LPDWORD pdwCertified)
{
    IDirectSoundImpl *This = (IDirectSoundImpl *)iface;
    TRACE("(%p, %p)\n",This,pdwCertified);

    if (This->initialized == FALSE) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (This->drvcaps.dwFlags & DSCAPS_CERTIFIED)
        *pdwCertified = DS_CERTIFIED;
    else
        *pdwCertified = DS_UNCERTIFIED;
    return DS_OK;
}

static IDirectSound8Vtbl IDirectSoundImpl_Vtbl =
{
    IDirectSoundImpl_QueryInterface,
    IDirectSoundImpl_AddRef,
    IDirectSoundImpl_Release,
    IDirectSoundImpl_CreateSoundBuffer,
    IDirectSoundImpl_GetCaps,
    IDirectSoundImpl_DuplicateSoundBuffer,
    IDirectSoundImpl_SetCooperativeLevel,
    IDirectSoundImpl_Compact,
    IDirectSoundImpl_GetSpeakerConfig,
    IDirectSoundImpl_SetSpeakerConfig,
    IDirectSoundImpl_Initialize,
    IDirectSoundImpl_VerifyCertification
};

HRESULT WINAPI IDirectSoundImpl_Create(
    LPCGUID lpcGUID,
    LPDIRECTSOUND8 * ppDS)
{
    HRESULT err;
    PIDSDRIVER drv = NULL;
    IDirectSoundImpl* pDS;
    unsigned wod, wodn;
    BOOLEAN found = FALSE;
    TRACE("(%s,%p)\n",debugstr_guid(lpcGUID),ppDS);

    /* Enumerate WINMM audio devices and find the one we want */
    wodn = waveOutGetNumDevs();
    if (!wodn) {
        WARN("no driver\n");
        *ppDS = NULL;
        return DSERR_NODRIVER;
    }

    TRACE(" expecting GUID %s.\n", debugstr_guid(lpcGUID));

    for (wod=0; wod<wodn; wod++) {
        if (IsEqualGUID( lpcGUID, &renderer_guids[wod])) {
            found = TRUE;
            break;
        }
    }

    if (found == FALSE) {
        WARN("No device found matching given ID!\n");
        *ppDS = NULL;
        return DSERR_NODRIVER;
    }

    /* DRV_QUERYDSOUNDIFACE is a "Wine extension" to get the DSound interface */
    WineWaveOutMessage((HWAVEOUT)wod, DRV_QUERYDSOUNDIFACE, (DWORD)&drv, 0);

    /* Disable the direct sound driver to force emulation if requested. */
    if (ds_hw_accel == DS_HW_ACCEL_EMULATION)
        drv = NULL;

    /* Allocate memory */
    pDS = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(IDirectSoundImpl));
    if (pDS == NULL) {
        WARN("out of memory\n");
        *ppDS = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pDS->lpVtbl         = &IDirectSoundImpl_Vtbl;
    pDS->ref            = 0;

    pDS->driver         = drv;
    pDS->priolevel      = DSSCL_NORMAL;
    pDS->fraglen        = 0;
    pDS->hwbuf          = NULL;
    pDS->buffer         = NULL;
    pDS->buflen         = 0;
    pDS->writelead      = 0;
    pDS->state          = STATE_STOPPED;
    pDS->nrofbuffers    = 0;
    pDS->buffers        = NULL;
    pDS->primary        = NULL;
    pDS->speaker_config = DSSPEAKER_STEREO | (DSSPEAKER_GEOMETRY_NARROW << 16);
    pDS->initialized    = FALSE;

    /* 3D listener initial parameters */
    pDS->listener       = NULL;
    pDS->ds3dl.dwSize   = sizeof(DS3DLISTENER);
    pDS->ds3dl.vPosition.x = 0.0;
    pDS->ds3dl.vPosition.y = 0.0;
    pDS->ds3dl.vPosition.z = 0.0;
    pDS->ds3dl.vVelocity.x = 0.0;
    pDS->ds3dl.vVelocity.y = 0.0;
    pDS->ds3dl.vVelocity.z = 0.0;
    pDS->ds3dl.vOrientFront.x = 0.0;
    pDS->ds3dl.vOrientFront.y = 0.0;
    pDS->ds3dl.vOrientFront.z = 1.0;
    pDS->ds3dl.vOrientTop.x = 0.0;
    pDS->ds3dl.vOrientTop.y = 1.0;
    pDS->ds3dl.vOrientTop.z = 0.0;
    pDS->ds3dl.flDistanceFactor = DS3D_DEFAULTDISTANCEFACTOR;
    pDS->ds3dl.flRolloffFactor = DS3D_DEFAULTROLLOFFFACTOR;
    pDS->ds3dl.flDopplerFactor = DS3D_DEFAULTDOPPLERFACTOR;

    pDS->prebuf         = ds_snd_queue_max;
    pDS->guid           = *lpcGUID;

    /* Get driver description */
    if (drv) {
        err = IDsDriver_GetDriverDesc(drv,&(pDS->drvdesc));
        if (err != DS_OK) {
            WARN("IDsDriver_GetDriverDesc failed\n");
            HeapFree(GetProcessHeap(),0,pDS);
            *ppDS = NULL;
            return err;
        }
    } else {
        /* if no DirectSound interface available, use WINMM API instead */
        pDS->drvdesc.dwFlags = DSDDESC_DOMMSYSTEMOPEN | DSDDESC_DOMMSYSTEMSETFORMAT;
    }

    pDS->drvdesc.dnDevNode = wod;

    /* Set default wave format (may need it for waveOutOpen) */
    pDS->pwfx = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(WAVEFORMATEX));
    if (pDS->pwfx == NULL) {
        WARN("out of memory\n");
        HeapFree(GetProcessHeap(),0,pDS);
        *ppDS = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pDS->pwfx->wFormatTag = WAVE_FORMAT_PCM;
    /* We rely on the sound driver to return the actual sound format of
     * the device if it does not support 22050x8x2 and is given the
     * WAVE_DIRECTSOUND flag.
     */
    pDS->pwfx->nSamplesPerSec = 22050;
    pDS->pwfx->wBitsPerSample = 8;
    pDS->pwfx->nChannels = 2;
    pDS->pwfx->nBlockAlign = pDS->pwfx->wBitsPerSample * pDS->pwfx->nChannels / 8;
    pDS->pwfx->nAvgBytesPerSec = pDS->pwfx->nSamplesPerSec * pDS->pwfx->nBlockAlign;
    pDS->pwfx->cbSize = 0;

    /* If the driver requests being opened through MMSYSTEM
     * (which is recommended by the DDK), it is supposed to happen
     * before the DirectSound interface is opened */
    if (pDS->drvdesc.dwFlags & DSDDESC_DOMMSYSTEMOPEN)
    {
        DWORD flags = CALLBACK_FUNCTION;

        /* disable direct sound if requested */
        if (ds_hw_accel != DS_HW_ACCEL_EMULATION)
            flags |= WAVE_DIRECTSOUND;

        err = mmErr(waveOutOpen(&(pDS->hwo),
                                pDS->drvdesc.dnDevNode, pDS->pwfx,
                                (DWORD)DSOUND_callback, (DWORD)pDS,
                                flags));
        if (err != DS_OK) {
            WARN("waveOutOpen failed\n");
            HeapFree(GetProcessHeap(),0,pDS);
            *ppDS = NULL;
            return err;
        }
    }

    if (drv) {
        err = IDsDriver_Open(drv);
        if (err != DS_OK) {
            WARN("IDsDriver_Open failed\n");
            HeapFree(GetProcessHeap(),0,pDS);
            *ppDS = NULL;
            return err;
        }

        /* the driver is now open, so it's now allowed to call GetCaps */
        err = IDsDriver_GetCaps(drv,&(pDS->drvcaps));
        if (err != DS_OK) {
            WARN("IDsDriver_GetCaps failed\n");
            HeapFree(GetProcessHeap(),0,pDS);
            *ppDS = NULL;
            return err;
        }
    } else {
        WAVEOUTCAPSA woc;
        err = mmErr(waveOutGetDevCapsA(pDS->drvdesc.dnDevNode, &woc, sizeof(woc)));
        if (err != DS_OK) {
            WARN("waveOutGetDevCaps failed\n");
            HeapFree(GetProcessHeap(),0,pDS);
            *ppDS = NULL;
            return err;
        }
        ZeroMemory(&pDS->drvcaps, sizeof(pDS->drvcaps));
        if ((woc.dwFormats & WAVE_FORMAT_1M08) ||
            (woc.dwFormats & WAVE_FORMAT_2M08) ||
            (woc.dwFormats & WAVE_FORMAT_4M08) ||
            (woc.dwFormats & WAVE_FORMAT_48M08) ||
            (woc.dwFormats & WAVE_FORMAT_96M08)) {
            pDS->drvcaps.dwFlags |= DSCAPS_PRIMARY8BIT;
            pDS->drvcaps.dwFlags |= DSCAPS_PRIMARYMONO;
        }
        if ((woc.dwFormats & WAVE_FORMAT_1M16) ||
            (woc.dwFormats & WAVE_FORMAT_2M16) ||
            (woc.dwFormats & WAVE_FORMAT_4M16) ||
            (woc.dwFormats & WAVE_FORMAT_48M16) ||
            (woc.dwFormats & WAVE_FORMAT_96M16)) {
            pDS->drvcaps.dwFlags |= DSCAPS_PRIMARY16BIT;
            pDS->drvcaps.dwFlags |= DSCAPS_PRIMARYMONO;
        }
        if ((woc.dwFormats & WAVE_FORMAT_1S08) ||
            (woc.dwFormats & WAVE_FORMAT_2S08) ||
            (woc.dwFormats & WAVE_FORMAT_4S08) ||
            (woc.dwFormats & WAVE_FORMAT_48S08) ||
            (woc.dwFormats & WAVE_FORMAT_96S08)) {
            pDS->drvcaps.dwFlags |= DSCAPS_PRIMARY8BIT;
            pDS->drvcaps.dwFlags |= DSCAPS_PRIMARYSTEREO;
        }
        if ((woc.dwFormats & WAVE_FORMAT_1S16) ||
            (woc.dwFormats & WAVE_FORMAT_2S16) ||
            (woc.dwFormats & WAVE_FORMAT_4S16) ||
            (woc.dwFormats & WAVE_FORMAT_48S16) ||
            (woc.dwFormats & WAVE_FORMAT_96S16)) {
            pDS->drvcaps.dwFlags |= DSCAPS_PRIMARY16BIT;
            pDS->drvcaps.dwFlags |= DSCAPS_PRIMARYSTEREO;
        }
        if (ds_emuldriver)
            pDS->drvcaps.dwFlags |= DSCAPS_EMULDRIVER;
        pDS->drvcaps.dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
        pDS->drvcaps.dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
        pDS->drvcaps.dwPrimaryBuffers = 1;
    }

    InitializeCriticalSection(&(pDS->mixlock));
    pDS->mixlock.DebugInfo->Spare[1] = (DWORD)"DSOUND_mixlock";

    RtlInitializeResource(&(pDS->buffer_list_lock));

    *ppDS = (LPDIRECTSOUND8)pDS;

    return DS_OK;
}
/*******************************************************************************
 *		IDirectSound_IUnknown
 */
static HRESULT WINAPI IDirectSound_IUnknown_QueryInterface(
    LPUNKNOWN iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return DSOUND_QueryInterface(This->pds, riid, ppobj);
}

static ULONG WINAPI IDirectSound_IUnknown_AddRef(
    LPUNKNOWN iface)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IDirectSound_IUnknown_Release(
    LPUNKNOWN iface)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    ULONG ulReturn;
    TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
    ulReturn = InterlockedDecrement(&(This->ref));
    if (ulReturn == 0) {
        IDirectSoundImpl_Release(This->pds);
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return ulReturn;
}

static IUnknownVtbl DirectSound_Unknown_Vtbl =
{
    IDirectSound_IUnknown_QueryInterface,
    IDirectSound_IUnknown_AddRef,
    IDirectSound_IUnknown_Release
};

HRESULT WINAPI IDirectSound_IUnknown_Create(
    LPDIRECTSOUND8 pds,
    LPUNKNOWN * ppunk)
{
    IDirectSound_IUnknown * pdsunk;
    TRACE("(%p,%p)\n",pds,ppunk);

    if (ppunk == NULL) {
        ERR("invalid parameter: ppunk == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pds == NULL) {
        ERR("invalid parameter: pds == NULL\n");
        *ppunk = NULL;
        return DSERR_INVALIDPARAM;
    }

    pdsunk = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsunk));
    if (pdsunk == NULL) {
        WARN("out of memory\n");
        *ppunk = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsunk->lpVtbl = &DirectSound_Unknown_Vtbl;
    pdsunk->ref = 0;
    pdsunk->pds = pds;

    IDirectSoundImpl_AddRef(pds);
    *ppunk = (LPUNKNOWN)pdsunk;

    return DS_OK;
}

/*******************************************************************************
 *		IDirectSound_IDirectSound
 */
static HRESULT WINAPI IDirectSound_IDirectSound_QueryInterface(
    LPDIRECTSOUND iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return DSOUND_QueryInterface(This->pds, riid, ppobj);
}

static ULONG WINAPI IDirectSound_IDirectSound_AddRef(
    LPDIRECTSOUND iface)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IDirectSound_IDirectSound_Release(
    LPDIRECTSOUND iface)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    ULONG ulReturn;
    TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
    ulReturn = InterlockedDecrement(&This->ref);
    if (ulReturn == 0) {
        IDirectSoundImpl_Release(This->pds);
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return ulReturn;
}

static HRESULT WINAPI IDirectSound_IDirectSound_CreateSoundBuffer(
    LPDIRECTSOUND iface,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);
    return DSOUND_CreateSoundBuffer(This->pds,dsbd,ppdsb,lpunk,FALSE);
}

static HRESULT WINAPI IDirectSound_IDirectSound_GetCaps(
    LPDIRECTSOUND iface,
    LPDSCAPS lpDSCaps)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,%p)\n",This,lpDSCaps);
    return IDirectSoundImpl_GetCaps(This->pds, lpDSCaps);
}

static HRESULT WINAPI IDirectSound_IDirectSound_DuplicateSoundBuffer(
    LPDIRECTSOUND iface,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,%p,%p)\n",This,psb,ppdsb);
    return IDirectSoundImpl_DuplicateSoundBuffer(This->pds,psb,ppdsb);
}

static HRESULT WINAPI IDirectSound_IDirectSound_SetCooperativeLevel(
    LPDIRECTSOUND iface,
    HWND hwnd,
    DWORD level)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,%08lx,%s)\n",This,(DWORD)hwnd,dumpCooperativeLevel(level));
    return IDirectSoundImpl_SetCooperativeLevel(This->pds,hwnd,level);
}

static HRESULT WINAPI IDirectSound_IDirectSound_Compact(
    LPDIRECTSOUND iface)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p)\n", This);
    return IDirectSoundImpl_Compact(This->pds);
}

static HRESULT WINAPI IDirectSound_IDirectSound_GetSpeakerConfig(
    LPDIRECTSOUND iface,
    LPDWORD lpdwSpeakerConfig)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p, %p)\n", This, lpdwSpeakerConfig);
    return IDirectSoundImpl_GetSpeakerConfig(This->pds,lpdwSpeakerConfig);
}

static HRESULT WINAPI IDirectSound_IDirectSound_SetSpeakerConfig(
    LPDIRECTSOUND iface,
    DWORD config)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p,0x%08lx)\n",This,config);
    return IDirectSoundImpl_SetSpeakerConfig(This->pds,config);
}

static HRESULT WINAPI IDirectSound_IDirectSound_Initialize(
    LPDIRECTSOUND iface,
    LPCGUID lpcGuid)
{
    IDirectSound_IDirectSound *This = (IDirectSound_IDirectSound *)iface;
    TRACE("(%p, %s)\n", This, debugstr_guid(lpcGuid));
    return IDirectSoundImpl_Initialize(This->pds,lpcGuid);
}

static IDirectSoundVtbl DirectSound_DirectSound_Vtbl =
{
    IDirectSound_IDirectSound_QueryInterface,
    IDirectSound_IDirectSound_AddRef,
    IDirectSound_IDirectSound_Release,
    IDirectSound_IDirectSound_CreateSoundBuffer,
    IDirectSound_IDirectSound_GetCaps,
    IDirectSound_IDirectSound_DuplicateSoundBuffer,
    IDirectSound_IDirectSound_SetCooperativeLevel,
    IDirectSound_IDirectSound_Compact,
    IDirectSound_IDirectSound_GetSpeakerConfig,
    IDirectSound_IDirectSound_SetSpeakerConfig,
    IDirectSound_IDirectSound_Initialize
};

HRESULT WINAPI IDirectSound_IDirectSound_Create(
    LPDIRECTSOUND8  pds,
    LPDIRECTSOUND * ppds)
{
    IDirectSound_IDirectSound * pdsds;
    TRACE("(%p,%p)\n",pds,ppds);

    if (ppds == NULL) {
        ERR("invalid parameter: ppds == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pds == NULL) {
        ERR("invalid parameter: pds == NULL\n");
        *ppds = NULL;
        return DSERR_INVALIDPARAM;
    }

    pdsds = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsds));
    if (pdsds == NULL) {
        WARN("out of memory\n");
        *ppds = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsds->lpVtbl = &DirectSound_DirectSound_Vtbl;
    pdsds->ref = 0;
    pdsds->pds = pds;

    IDirectSoundImpl_AddRef(pds);
    *ppds = (LPDIRECTSOUND)pdsds;

    return DS_OK;
}

/*******************************************************************************
 *		IDirectSound8_IUnknown
 */
static HRESULT WINAPI IDirectSound8_IUnknown_QueryInterface(
    LPUNKNOWN iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return DSOUND_QueryInterface8(This->pds, riid, ppobj);
}

static ULONG WINAPI IDirectSound8_IUnknown_AddRef(
    LPUNKNOWN iface)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IDirectSound8_IUnknown_Release(
    LPUNKNOWN iface)
{
    IDirectSound_IUnknown *This = (IDirectSound_IUnknown *)iface;
    ULONG ulReturn;
    TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
    ulReturn = InterlockedDecrement(&(This->ref));
    if (ulReturn == 0) {
        IDirectSoundImpl_Release(This->pds);
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return ulReturn;
}

static IUnknownVtbl DirectSound8_Unknown_Vtbl =
{
    IDirectSound8_IUnknown_QueryInterface,
    IDirectSound8_IUnknown_AddRef,
    IDirectSound8_IUnknown_Release
};

HRESULT WINAPI IDirectSound8_IUnknown_Create(
    LPDIRECTSOUND8 pds,
    LPUNKNOWN * ppunk)
{
    IDirectSound8_IUnknown * pdsunk;
    TRACE("(%p,%p)\n",pds,ppunk);

    if (ppunk == NULL) {
        ERR("invalid parameter: ppunk == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pds == NULL) {
        ERR("invalid parameter: pds == NULL\n");
        *ppunk = NULL;
        return DSERR_INVALIDPARAM;
    }

    pdsunk = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsunk));
    if (pdsunk == NULL) {
        WARN("out of memory\n");
        *ppunk = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsunk->lpVtbl = &DirectSound8_Unknown_Vtbl;
    pdsunk->ref = 0;
    pdsunk->pds = pds;

    IDirectSoundImpl_AddRef(pds);
    *ppunk = (LPUNKNOWN)pdsunk;

    return DS_OK;
}

/*******************************************************************************
 *		IDirectSound8_IDirectSound
 */
static HRESULT WINAPI IDirectSound8_IDirectSound_QueryInterface(
    LPDIRECTSOUND iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return DSOUND_QueryInterface8(This->pds, riid, ppobj);
}

static ULONG WINAPI IDirectSound8_IDirectSound_AddRef(
    LPDIRECTSOUND iface)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IDirectSound8_IDirectSound_Release(
    LPDIRECTSOUND iface)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    ULONG ulReturn;
    TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
    ulReturn = InterlockedDecrement(&(This->ref));
    if (ulReturn == 0) {
        IDirectSoundImpl_Release(This->pds);
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return ulReturn;
}

static HRESULT WINAPI IDirectSound8_IDirectSound_CreateSoundBuffer(
    LPDIRECTSOUND iface,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);
    return DSOUND_CreateSoundBuffer(This->pds,dsbd,ppdsb,lpunk,TRUE);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_GetCaps(
    LPDIRECTSOUND iface,
    LPDSCAPS lpDSCaps)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%p)\n",This,lpDSCaps);
    return IDirectSoundImpl_GetCaps(This->pds, lpDSCaps);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_DuplicateSoundBuffer(
    LPDIRECTSOUND iface,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%p,%p)\n",This,psb,ppdsb);
    return IDirectSoundImpl_DuplicateSoundBuffer(This->pds,psb,ppdsb);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_SetCooperativeLevel(
    LPDIRECTSOUND iface,
    HWND hwnd,
    DWORD level)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%08lx,%s)\n",This,(DWORD)hwnd,dumpCooperativeLevel(level));
    return IDirectSoundImpl_SetCooperativeLevel(This->pds,hwnd,level);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_Compact(
    LPDIRECTSOUND iface)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p)\n", This);
    return IDirectSoundImpl_Compact(This->pds);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_GetSpeakerConfig(
    LPDIRECTSOUND iface,
    LPDWORD lpdwSpeakerConfig)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p, %p)\n", This, lpdwSpeakerConfig);
    return IDirectSoundImpl_GetSpeakerConfig(This->pds,lpdwSpeakerConfig);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_SetSpeakerConfig(
    LPDIRECTSOUND iface,
    DWORD config)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,0x%08lx)\n",This,config);
    return IDirectSoundImpl_SetSpeakerConfig(This->pds,config);
}

static HRESULT WINAPI IDirectSound8_IDirectSound_Initialize(
    LPDIRECTSOUND iface,
    LPCGUID lpcGuid)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p, %s)\n", This, debugstr_guid(lpcGuid));
    return IDirectSoundImpl_Initialize(This->pds,lpcGuid);
}

static IDirectSoundVtbl DirectSound8_DirectSound_Vtbl =
{
    IDirectSound8_IDirectSound_QueryInterface,
    IDirectSound8_IDirectSound_AddRef,
    IDirectSound8_IDirectSound_Release,
    IDirectSound8_IDirectSound_CreateSoundBuffer,
    IDirectSound8_IDirectSound_GetCaps,
    IDirectSound8_IDirectSound_DuplicateSoundBuffer,
    IDirectSound8_IDirectSound_SetCooperativeLevel,
    IDirectSound8_IDirectSound_Compact,
    IDirectSound8_IDirectSound_GetSpeakerConfig,
    IDirectSound8_IDirectSound_SetSpeakerConfig,
    IDirectSound8_IDirectSound_Initialize
};

HRESULT WINAPI IDirectSound8_IDirectSound_Create(
    LPDIRECTSOUND8 pds,
    LPDIRECTSOUND * ppds)
{
    IDirectSound8_IDirectSound * pdsds;
    TRACE("(%p,%p)\n",pds,ppds);

    if (ppds == NULL) {
        ERR("invalid parameter: ppds == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pds == NULL) {
        ERR("invalid parameter: pds == NULL\n");
        *ppds = NULL;
        return DSERR_INVALIDPARAM;
    }

    pdsds = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsds));
    if (pdsds == NULL) {
        WARN("out of memory\n");
        *ppds = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsds->lpVtbl = &DirectSound8_DirectSound_Vtbl;
    pdsds->ref = 0;
    pdsds->pds = pds;

    IDirectSoundImpl_AddRef(pds);
    *ppds = (LPDIRECTSOUND)pdsds;

    return DS_OK;
}

/*******************************************************************************
 *		IDirectSound8_IDirectSound8
 */
static HRESULT WINAPI IDirectSound8_IDirectSound8_QueryInterface(
    LPDIRECTSOUND8 iface,
    REFIID riid,
    LPVOID * ppobj)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppobj);
    return DSOUND_QueryInterface8(This->pds, riid, ppobj);
}

static ULONG WINAPI IDirectSound8_IDirectSound8_AddRef(
    LPDIRECTSOUND8 iface)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
    return InterlockedIncrement(&(This->ref));
}

static ULONG WINAPI IDirectSound8_IDirectSound8_Release(
    LPDIRECTSOUND8 iface)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    ULONG ulReturn;
    TRACE("(%p) ref was %ld, thread is %04lx\n", This, This->ref, GetCurrentThreadId());
    ulReturn = InterlockedDecrement(&(This->ref));
    if (ulReturn == 0) {
        IDirectSoundImpl_Release(This->pds);
        HeapFree(GetProcessHeap(),0,This);
        TRACE("(%p) released\n",This);
    }
    return ulReturn;
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_CreateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p,%p,%p,%p)\n",This,dsbd,ppdsb,lpunk);
    return DSOUND_CreateSoundBuffer(This->pds,dsbd,ppdsb,lpunk,TRUE);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_GetCaps(
    LPDIRECTSOUND8 iface,
    LPDSCAPS lpDSCaps)
{
    IDirectSound8_IDirectSound *This = (IDirectSound8_IDirectSound *)iface;
    TRACE("(%p,%p)\n",This,lpDSCaps);
    return IDirectSoundImpl_GetCaps(This->pds, lpDSCaps);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_DuplicateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p,%p,%p)\n",This,psb,ppdsb);
    return IDirectSoundImpl_DuplicateSoundBuffer(This->pds,psb,ppdsb);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_SetCooperativeLevel(
    LPDIRECTSOUND8 iface,
    HWND hwnd,
    DWORD level)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p,%08lx,%s)\n",This,(DWORD)hwnd,dumpCooperativeLevel(level));
    return IDirectSoundImpl_SetCooperativeLevel(This->pds,hwnd,level);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_Compact(
    LPDIRECTSOUND8 iface)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p)\n", This);
    return IDirectSoundImpl_Compact(This->pds);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_GetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    LPDWORD lpdwSpeakerConfig)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p, %p)\n", This, lpdwSpeakerConfig);
    return IDirectSoundImpl_GetSpeakerConfig(This->pds,lpdwSpeakerConfig);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_SetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    DWORD config)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p,0x%08lx)\n",This,config);
    return IDirectSoundImpl_SetSpeakerConfig(This->pds,config);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_Initialize(
    LPDIRECTSOUND8 iface,
    LPCGUID lpcGuid)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p, %s)\n", This, debugstr_guid(lpcGuid));
    return IDirectSoundImpl_Initialize(This->pds,lpcGuid);
}

static HRESULT WINAPI IDirectSound8_IDirectSound8_VerifyCertification(
    LPDIRECTSOUND8 iface,
    LPDWORD pdwCertified)
{
    IDirectSound8_IDirectSound8 *This = (IDirectSound8_IDirectSound8 *)iface;
    TRACE("(%p, %p)\n", This, pdwCertified);
    return IDirectSoundImpl_VerifyCertification(This->pds,pdwCertified);
}

static IDirectSound8Vtbl DirectSound8_DirectSound8_Vtbl =
{
    IDirectSound8_IDirectSound8_QueryInterface,
    IDirectSound8_IDirectSound8_AddRef,
    IDirectSound8_IDirectSound8_Release,
    IDirectSound8_IDirectSound8_CreateSoundBuffer,
    IDirectSound8_IDirectSound8_GetCaps,
    IDirectSound8_IDirectSound8_DuplicateSoundBuffer,
    IDirectSound8_IDirectSound8_SetCooperativeLevel,
    IDirectSound8_IDirectSound8_Compact,
    IDirectSound8_IDirectSound8_GetSpeakerConfig,
    IDirectSound8_IDirectSound8_SetSpeakerConfig,
    IDirectSound8_IDirectSound8_Initialize,
    IDirectSound8_IDirectSound8_VerifyCertification
};

HRESULT WINAPI IDirectSound8_IDirectSound8_Create(
    LPDIRECTSOUND8 pds,
    LPDIRECTSOUND8 * ppds)
{
    IDirectSound8_IDirectSound8 * pdsds;
    TRACE("(%p,%p)\n",pds,ppds);

    if (ppds == NULL) {
        ERR("invalid parameter: ppds == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pds == NULL) {
        ERR("invalid parameter: pds == NULL\n");
        *ppds = NULL;
        return DSERR_INVALIDPARAM;
    }

    pdsds = HeapAlloc(GetProcessHeap(),0,sizeof(*pdsds));
    if (pdsds == NULL) {
        WARN("out of memory\n");
        *ppds = NULL;
        return DSERR_OUTOFMEMORY;
    }

    pdsds->lpVtbl = &DirectSound8_DirectSound8_Vtbl;
    pdsds->ref = 0;
    pdsds->pds = pds;

    IDirectSoundImpl_AddRef(pds);
    *ppds = (LPDIRECTSOUND8)pdsds;

    return DS_OK;
}

HRESULT WINAPI DSOUND_Create(
    LPCGUID lpcGUID,
    LPDIRECTSOUND *ppDS,
    IUnknown *pUnkOuter)
{
    HRESULT hr;
    GUID devGuid;

    TRACE("(%s,%p,%p)\n",debugstr_guid(lpcGUID),ppDS,pUnkOuter);

    if (pUnkOuter != NULL) {
        WARN("invalid parameter: pUnkOuter != NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppDS == NULL) {
        WARN("invalid parameter: ppDS == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    /* Get dsound configuration */
    setup_dsound_options();

    /* Default device? */
    if (!lpcGUID || IsEqualGUID(lpcGUID, &GUID_NULL))
        lpcGUID = &DSDEVID_DefaultPlayback;

    if (GetDeviceID(lpcGUID, &devGuid) != DS_OK) {
        WARN("invalid parameter: lpcGUID\n");
        *ppDS = NULL;
        return DSERR_INVALIDPARAM;
    }

    if (dsound) {
        if (IsEqualGUID(&devGuid, &dsound->guid)) {
            hr = IDirectSound_IDirectSound_Create((LPDIRECTSOUND8)dsound, ppDS);
            if (*ppDS)
                IDirectSound_IDirectSound_AddRef(*ppDS);
            else
                WARN("IDirectSound_IDirectSound_Create failed\n");
        } else {
            ERR("different dsound already opened (only support one sound card at a time now)\n");
            *ppDS = NULL;
            hr = DSERR_ALLOCATED;
        }
    } else {
        LPDIRECTSOUND8 pDS;
        hr = IDirectSoundImpl_Create(&devGuid, &pDS);
        if (hr == DS_OK) {
            hr = DSOUND_PrimaryCreate((IDirectSoundImpl*)pDS);
            if (hr == DS_OK) {
                hr = IDirectSound_IDirectSound_Create(pDS, ppDS);
                if (*ppDS) {
                    IDirectSound_IDirectSound_AddRef(*ppDS);

                    dsound = (IDirectSoundImpl*)pDS;
                    timeBeginPeriod(DS_TIME_RES);
                    dsound->timerID = timeSetEvent(DS_TIME_DEL, DS_TIME_RES, DSOUND_timer,
                                       (DWORD)dsound, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
                } else {
                    WARN("IDirectSound_IDirectSound_Create failed\n");
                    IDirectSound8_Release(pDS);
                }
            } else {
                WARN("DSOUND_PrimaryCreate failed\n");
                IDirectSound8_Release(pDS);
            }
        } else
            WARN("IDirectSoundImpl_Create failed\n");
    }

    return hr;
}

/*******************************************************************************
 *		DirectSoundCreate (DSOUND.1)
 *
 *  Creates and initializes a DirectSound interface.
 *
 *  PARAMS
 *     lpcGUID   [I] Address of the GUID that identifies the sound device.
 *     ppDS      [O] Address of a variable to receive the interface pointer.
 *     pUnkOuter [I] Must be NULL.
 *
 *  RETURNS
 *     Success: DS_OK
 *     Failure: DSERR_ALLOCATED, DSERR_INVALIDPARAM, DSERR_NOAGGREGATION,
 *              DSERR_NODRIVER, DSERR_OUTOFMEMORY
 */
HRESULT WINAPI DirectSoundCreate(
    LPCGUID lpcGUID,
    LPDIRECTSOUND *ppDS,
    IUnknown *pUnkOuter)
{
    HRESULT hr;

    TRACE("(%s,%p,%p)\n",debugstr_guid(lpcGUID),ppDS,pUnkOuter);

    hr = DSOUND_Create(lpcGUID, ppDS, pUnkOuter);
    if (hr == DS_OK)
        IDirectSoundImpl_Initialize((LPDIRECTSOUND8)dsound, lpcGUID);

    return hr;
}

HRESULT WINAPI DSOUND_Create8(
    LPCGUID lpcGUID,
    LPDIRECTSOUND8 *ppDS,
    IUnknown *pUnkOuter)
{
    HRESULT hr;
    GUID devGuid;

    TRACE("(%s,%p,%p)\n",debugstr_guid(lpcGUID),ppDS,pUnkOuter);

    if (pUnkOuter != NULL) {
        WARN("invalid parameter: pUnkOuter != NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (ppDS == NULL) {
        WARN("invalid parameter: ppDS == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    /* Get dsound configuration */
    setup_dsound_options();

    /* Default device? */
    if (!lpcGUID || IsEqualGUID(lpcGUID, &GUID_NULL))
        lpcGUID = &DSDEVID_DefaultPlayback;

    if (GetDeviceID(lpcGUID, &devGuid) != DS_OK) {
        WARN("invalid parameter: lpcGUID\n");
        *ppDS = NULL;
        return DSERR_INVALIDPARAM;
    }

    if (dsound) {
        if (IsEqualGUID(&devGuid, &dsound->guid)) {
            hr = IDirectSound8_IDirectSound8_Create((LPDIRECTSOUND8)dsound, ppDS);
            if (*ppDS)
                IDirectSound8_IDirectSound8_AddRef(*ppDS);
            else
                WARN("IDirectSound8_IDirectSound8_Create failed\n");
        } else {
            ERR("different dsound already opened (only support one sound card at a time now)\n");
            *ppDS = NULL;
            hr = DSERR_ALLOCATED;
        }
    } else {
        LPDIRECTSOUND8 pDS;
        hr = IDirectSoundImpl_Create(&devGuid, &pDS);
        if (hr == DS_OK) {
            hr = DSOUND_PrimaryCreate((IDirectSoundImpl*)pDS);
            if (hr == DS_OK) {
                hr = IDirectSound8_IDirectSound8_Create(pDS, ppDS);
                if (*ppDS) {
                    IDirectSound8_IDirectSound8_AddRef(*ppDS);

                    dsound = (IDirectSoundImpl*)pDS;
                    timeBeginPeriod(DS_TIME_RES);
                    dsound->timerID = timeSetEvent(DS_TIME_DEL, DS_TIME_RES, DSOUND_timer,
                                       (DWORD)dsound, TIME_PERIODIC | TIME_CALLBACK_FUNCTION);
                } else {
                    WARN("IDirectSound8_IDirectSound8_Create failed\n");
                    IDirectSound8_Release(pDS);
                }
            } else {
                WARN("DSOUND_PrimaryCreate failed\n");
                IDirectSound8_Release(pDS);
            }
        } else
            WARN("IDirectSoundImpl_Create failed\n");
    }

    return hr;
}

/*******************************************************************************
 *        DirectSoundCreate8 (DSOUND.11)
 *
 *  Creates and initializes a DirectSound8 interface.
 *
 *  PARAMS
 *     lpcGUID   [I] Address of the GUID that identifies the sound device.
 *     ppDS      [O] Address of a variable to receive the interface pointer.
 *     pUnkOuter [I] Must be NULL.
 *
 *  RETURNS
 *     Success: DS_OK
 *     Failure: DSERR_ALLOCATED, DSERR_INVALIDPARAM, DSERR_NOAGGREGATION,
 *              DSERR_NODRIVER, DSERR_OUTOFMEMORY
 */
HRESULT WINAPI DirectSoundCreate8(
    LPCGUID lpcGUID,
    LPDIRECTSOUND8 *ppDS,
    IUnknown *pUnkOuter)
{
    HRESULT hr;

    TRACE("(%s,%p,%p)\n",debugstr_guid(lpcGUID),ppDS,pUnkOuter);

    hr = DSOUND_Create8(lpcGUID, ppDS, pUnkOuter);
    if (hr == DS_OK)
        IDirectSoundImpl_Initialize((LPDIRECTSOUND8)dsound, lpcGUID);

    return hr;
}

/*
 * Add secondary buffer to buffer list.
 * Gets exclusive access to buffer for writing.
 */
HRESULT DSOUND_AddBuffer(
    IDirectSoundImpl * pDS,
    IDirectSoundBufferImpl * pDSB)
{
    IDirectSoundBufferImpl **newbuffers;
    HRESULT hr = DS_OK;

    TRACE("(%p, %p)\n", pDS, pDSB);

    RtlAcquireResourceExclusive(&(pDS->buffer_list_lock), TRUE);

    if (pDS->buffers)
        newbuffers = HeapReAlloc(GetProcessHeap(),0,pDS->buffers,sizeof(IDirectSoundBufferImpl*)*(pDS->nrofbuffers+1));
    else
        newbuffers = HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSoundBufferImpl*)*(pDS->nrofbuffers+1));

    if (newbuffers) {
        pDS->buffers = newbuffers;
        pDS->buffers[pDS->nrofbuffers] = pDSB;
        pDS->nrofbuffers++;
        TRACE("buffer count is now %d\n", pDS->nrofbuffers);
    } else {
        ERR("out of memory for buffer list! Current buffer count is %d\n", pDS->nrofbuffers);
        hr = DSERR_OUTOFMEMORY;
    }

    RtlReleaseResource(&(pDS->buffer_list_lock));

    return hr;
}

/*
 * Remove secondary buffer from buffer list.
 * Gets exclusive access to buffer for writing.
 */
HRESULT DSOUND_RemoveBuffer(
    IDirectSoundImpl * pDS,
    IDirectSoundBufferImpl * pDSB)
{
    int i;
    HRESULT hr = DS_OK;

    TRACE("(%p, %p)\n", pDS, pDSB);

    RtlAcquireResourceExclusive(&(pDS->buffer_list_lock), TRUE);

    for (i = 0; i < pDS->nrofbuffers; i++)
        if (pDS->buffers[i] == pDSB)
            break;

    if (i < pDS->nrofbuffers) {
        /* Put the last buffer of the list in the (now empty) position */
        pDS->buffers[i] = pDS->buffers[pDS->nrofbuffers - 1];
        pDS->nrofbuffers--;
        pDS->buffers = HeapReAlloc(GetProcessHeap(),0,pDS->buffers,sizeof(LPDIRECTSOUNDBUFFER8)*pDS->nrofbuffers);
        TRACE("buffer count is now %d\n", pDS->nrofbuffers);
    }

    if (pDS->nrofbuffers == 0) {
        HeapFree(GetProcessHeap(),0,pDS->buffers);
        pDS->buffers = NULL;
    }

    RtlReleaseResource(&(pDS->buffer_list_lock));

    return hr;
}
