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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdarg.h>
//#include <stdio.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION
//#include "windef.h"
//#include "winbase.h"
//#include "winuser.h"
#include <winternl.h>
#include "mmddk.h"
//#include "wingdi.h"
//#include "mmreg.h"
//#include "ks.h"
//#include "ksmedia.h"
#include <wine/debug.h>
#include <dsound.h>
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

typedef struct IDirectSoundImpl {
    IUnknown            IUnknown_inner;
    IDirectSound8       IDirectSound8_iface;
    IUnknown           *outer_unk;      /* internal */
    LONG                ref, refds, numIfaces;
    DirectSoundDevice  *device;
    BOOL                has_ds8;
} IDirectSoundImpl;

static const char * dumpCooperativeLevel(DWORD level)
{
#define LE(x) case x: return #x
    switch (level) {
        LE(DSSCL_NORMAL);
        LE(DSSCL_PRIORITY);
        LE(DSSCL_EXCLUSIVE);
        LE(DSSCL_WRITEPRIMARY);
    }
#undef LE
    return wine_dbg_sprintf("Unknown(%08x)", level);
}

static void _dump_DSCAPS(DWORD xmask) {
    struct {
        DWORD   mask;
        const char    *name;
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
            TRACE("%s ",flags[i].name);
}

static void _dump_DSBCAPS(DWORD xmask) {
    struct {
        DWORD   mask;
        const char    *name;
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
            TRACE("%s ",flags[i].name);
}

static void directsound_destroy(IDirectSoundImpl *This)
{
    if (This->device)
        DirectSoundDevice_Release(This->device);
    HeapFree(GetProcessHeap(),0,This);
    TRACE("(%p) released\n", This);
}

/*******************************************************************************
 *      IUnknown Implementation for DirectSound
 */
static inline IDirectSoundImpl *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, IDirectSoundImpl, IUnknown_inner);
}

static HRESULT WINAPI IUnknownImpl_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    IDirectSoundImpl *This = impl_from_IUnknown(iface);

    TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppv);

    if (!ppv) {
        WARN("invalid parameter\n");
        return E_INVALIDARG;
    }
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IUnknown))
        *ppv = &This->IUnknown_inner;
    else if (IsEqualIID(riid, &IID_IDirectSound) ||
            (IsEqualIID(riid, &IID_IDirectSound8) && This->has_ds8))
        *ppv = &This->IDirectSound8_iface;
    else {
        WARN("unknown IID %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI IUnknownImpl_AddRef(IUnknown *iface)
{
    IDirectSoundImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI IUnknownImpl_Release(IUnknown *iface)
{
    IDirectSoundImpl *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        directsound_destroy(This);

    return ref;
}

static const IUnknownVtbl unk_vtbl =
{
    IUnknownImpl_QueryInterface,
    IUnknownImpl_AddRef,
    IUnknownImpl_Release
};

/*******************************************************************************
 *      IDirectSound and IDirectSound8 Implementation
 */
static inline IDirectSoundImpl *impl_from_IDirectSound8(IDirectSound8 *iface)
{
    return CONTAINING_RECORD(iface, IDirectSoundImpl, IDirectSound8_iface);
}

static HRESULT WINAPI IDirectSound8Impl_QueryInterface(IDirectSound8 *iface, REFIID riid,
        void **ppv)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);
    TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppv);
    return IUnknown_QueryInterface(This->outer_unk, riid, ppv);
}

static ULONG WINAPI IDirectSound8Impl_AddRef(IDirectSound8 *iface)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);
    ULONG ref = InterlockedIncrement(&This->refds);

    TRACE("(%p) refds=%d\n", This, ref);

    if(ref == 1)
        InterlockedIncrement(&This->numIfaces);

    return ref;
}

static ULONG WINAPI IDirectSound8Impl_Release(IDirectSound8 *iface)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);
    ULONG ref = InterlockedDecrement(&(This->refds));

    TRACE("(%p) refds=%d\n", This, ref);

    if (!ref && !InterlockedDecrement(&This->numIfaces))
        directsound_destroy(This);

    return ref;
}

static HRESULT WINAPI IDirectSound8Impl_CreateSoundBuffer(IDirectSound8 *iface,
        const DSBUFFERDESC *dsbd, IDirectSoundBuffer **ppdsb, IUnknown *lpunk)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);
    TRACE("(%p,%p,%p,%p)\n", This, dsbd, ppdsb, lpunk);
    return DirectSoundDevice_CreateSoundBuffer(This->device, dsbd, ppdsb, lpunk, This->has_ds8);
}

static HRESULT WINAPI IDirectSound8Impl_GetCaps(IDirectSound8 *iface, DSCAPS *dscaps)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);

    TRACE("(%p, %p)\n", This, dscaps);

    if (!This->device) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }
    if (!dscaps) {
        WARN("invalid parameter: dscaps = NULL\n");
        return DSERR_INVALIDPARAM;
    }
    if (dscaps->dwSize < sizeof(*dscaps)) {
        WARN("invalid parameter: dscaps->dwSize = %d\n", dscaps->dwSize);
        return DSERR_INVALIDPARAM;
    }

    dscaps->dwFlags                        = This->device->drvcaps.dwFlags;
    dscaps->dwMinSecondarySampleRate       = This->device->drvcaps.dwMinSecondarySampleRate;
    dscaps->dwMaxSecondarySampleRate       = This->device->drvcaps.dwMaxSecondarySampleRate;
    dscaps->dwPrimaryBuffers               = This->device->drvcaps.dwPrimaryBuffers;
    dscaps->dwMaxHwMixingAllBuffers        = This->device->drvcaps.dwMaxHwMixingAllBuffers;
    dscaps->dwMaxHwMixingStaticBuffers     = This->device->drvcaps.dwMaxHwMixingStaticBuffers;
    dscaps->dwMaxHwMixingStreamingBuffers  = This->device->drvcaps.dwMaxHwMixingStreamingBuffers;
    dscaps->dwFreeHwMixingAllBuffers       = This->device->drvcaps.dwFreeHwMixingAllBuffers;
    dscaps->dwFreeHwMixingStaticBuffers    = This->device->drvcaps.dwFreeHwMixingStaticBuffers;
    dscaps->dwFreeHwMixingStreamingBuffers = This->device->drvcaps.dwFreeHwMixingStreamingBuffers;
    dscaps->dwMaxHw3DAllBuffers            = This->device->drvcaps.dwMaxHw3DAllBuffers;
    dscaps->dwMaxHw3DStaticBuffers         = This->device->drvcaps.dwMaxHw3DStaticBuffers;
    dscaps->dwMaxHw3DStreamingBuffers      = This->device->drvcaps.dwMaxHw3DStreamingBuffers;
    dscaps->dwFreeHw3DAllBuffers           = This->device->drvcaps.dwFreeHw3DAllBuffers;
    dscaps->dwFreeHw3DStaticBuffers        = This->device->drvcaps.dwFreeHw3DStaticBuffers;
    dscaps->dwFreeHw3DStreamingBuffers     = This->device->drvcaps.dwFreeHw3DStreamingBuffers;
    dscaps->dwTotalHwMemBytes              = This->device->drvcaps.dwTotalHwMemBytes;
    dscaps->dwFreeHwMemBytes               = This->device->drvcaps.dwFreeHwMemBytes;
    dscaps->dwMaxContigFreeHwMemBytes      = This->device->drvcaps.dwMaxContigFreeHwMemBytes;
    dscaps->dwUnlockTransferRateHwBuffers  = This->device->drvcaps.dwUnlockTransferRateHwBuffers;
    dscaps->dwPlayCpuOverheadSwBuffers     = This->device->drvcaps.dwPlayCpuOverheadSwBuffers;

    if (TRACE_ON(dsound)) {
        TRACE("(flags=0x%08x:\n", dscaps->dwFlags);
        _dump_DSCAPS(dscaps->dwFlags);
        TRACE(")\n");
    }

    return DS_OK;
}

static HRESULT WINAPI IDirectSound8Impl_DuplicateSoundBuffer(IDirectSound8 *iface,
        IDirectSoundBuffer *psb, IDirectSoundBuffer **ppdsb)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);
    TRACE("(%p,%p,%p)\n", This, psb, ppdsb);
    return DirectSoundDevice_DuplicateSoundBuffer(This->device, psb, ppdsb);
}

static HRESULT WINAPI IDirectSound8Impl_SetCooperativeLevel(IDirectSound8 *iface, HWND hwnd,
        DWORD level)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);
    DirectSoundDevice *device = This->device;
    DWORD oldlevel;
    HRESULT hr = S_OK;

    TRACE("(%p,%p,%s)\n", This, hwnd, dumpCooperativeLevel(level));

    if (!device) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (level == DSSCL_PRIORITY || level == DSSCL_EXCLUSIVE) {
        WARN("level=%s not fully supported\n",
             level==DSSCL_PRIORITY ? "DSSCL_PRIORITY" : "DSSCL_EXCLUSIVE");
    }

    RtlAcquireResourceExclusive(&device->buffer_list_lock, TRUE);
    EnterCriticalSection(&device->mixlock);
    oldlevel = device->priolevel;
    device->priolevel = level;
    if ((level == DSSCL_WRITEPRIMARY) != (oldlevel == DSSCL_WRITEPRIMARY)) {
        hr = DSOUND_ReopenDevice(device, level == DSSCL_WRITEPRIMARY);
        if (FAILED(hr))
            device->priolevel = oldlevel;
        else
            DSOUND_PrimaryOpen(device);
    }
    LeaveCriticalSection(&device->mixlock);
    RtlReleaseResource(&device->buffer_list_lock);
    return hr;
}

static HRESULT WINAPI IDirectSound8Impl_Compact(IDirectSound8 *iface)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);

    TRACE("(%p)\n", This);

    if (!This->device) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (This->device->priolevel < DSSCL_PRIORITY) {
        WARN("incorrect priority level\n");
        return DSERR_PRIOLEVELNEEDED;
    }
    return DS_OK;
}

static HRESULT WINAPI IDirectSound8Impl_GetSpeakerConfig(IDirectSound8 *iface, DWORD *config)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);

    TRACE("(%p, %p)\n", This, config);

    if (!This->device) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }
    if (!config) {
        WARN("invalid parameter: config == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    WARN("not fully functional\n");
    *config = This->device->speaker_config;
    return DS_OK;
}

static HRESULT WINAPI IDirectSound8Impl_SetSpeakerConfig(IDirectSound8 *iface, DWORD config)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);

    TRACE("(%p,0x%08x)\n", This, config);

    if (!This->device) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    This->device->speaker_config = config;
    WARN("not fully functional\n");
    return DS_OK;
}

static HRESULT WINAPI IDirectSound8Impl_Initialize(IDirectSound8 *iface, const GUID *lpcGuid)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);
    TRACE("(%p, %s)\n", This, debugstr_guid(lpcGuid));
    return DirectSoundDevice_Initialize(&This->device, lpcGuid);
}

static HRESULT WINAPI IDirectSound8Impl_VerifyCertification(IDirectSound8 *iface, DWORD *certified)
{
    IDirectSoundImpl *This = impl_from_IDirectSound8(iface);

    TRACE("(%p, %p)\n", This, certified);

    if (!This->device) {
        WARN("not initialized\n");
        return DSERR_UNINITIALIZED;
    }

    if (This->device->drvcaps.dwFlags & DSCAPS_CERTIFIED)
        *certified = DS_CERTIFIED;
    else
        *certified = DS_UNCERTIFIED;

    return DS_OK;
}

static const IDirectSound8Vtbl ds8_vtbl =
{
    IDirectSound8Impl_QueryInterface,
    IDirectSound8Impl_AddRef,
    IDirectSound8Impl_Release,
    IDirectSound8Impl_CreateSoundBuffer,
    IDirectSound8Impl_GetCaps,
    IDirectSound8Impl_DuplicateSoundBuffer,
    IDirectSound8Impl_SetCooperativeLevel,
    IDirectSound8Impl_Compact,
    IDirectSound8Impl_GetSpeakerConfig,
    IDirectSound8Impl_SetSpeakerConfig,
    IDirectSound8Impl_Initialize,
    IDirectSound8Impl_VerifyCertification
};

HRESULT IDirectSoundImpl_Create(IUnknown *outer_unk, REFIID riid, void **ppv, BOOL has_ds8)
{
    IDirectSoundImpl *obj;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    *ppv = NULL;
    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*obj));
    if (!obj) {
        WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    setup_dsound_options();

    obj->IUnknown_inner.lpVtbl = &unk_vtbl;
    obj->IDirectSound8_iface.lpVtbl = &ds8_vtbl;
    obj->ref = 1;
    obj->refds = 0;
    obj->numIfaces = 1;
    obj->device = NULL;
    obj->has_ds8 = has_ds8;

    /* COM aggregation supported only internally */
    if (outer_unk)
        obj->outer_unk = outer_unk;
    else
        obj->outer_unk = &obj->IUnknown_inner;

    hr = IUnknown_QueryInterface(&obj->IUnknown_inner, riid, ppv);
    IUnknown_Release(&obj->IUnknown_inner);

    return hr;
}

HRESULT DSOUND_Create(REFIID riid, void **ppv)
{
    return IDirectSoundImpl_Create(NULL, riid, ppv, FALSE);
}

HRESULT DSOUND_Create8(REFIID riid, void **ppv)
{
    return IDirectSoundImpl_Create(NULL, riid, ppv, TRUE);
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
    LPDIRECTSOUND pDS;

    TRACE("(%s,%p,%p)\n",debugstr_guid(lpcGUID),ppDS,pUnkOuter);

    if (ppDS == NULL) {
        WARN("invalid parameter: ppDS == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pUnkOuter != NULL) {
        WARN("invalid parameter: pUnkOuter != NULL\n");
        *ppDS = 0;
        return DSERR_INVALIDPARAM;
    }

    hr = DSOUND_Create(&IID_IDirectSound, (void **)&pDS);
    if (hr == DS_OK) {
        hr = IDirectSound_Initialize(pDS, lpcGUID);
        if (hr != DS_OK) {
            if (hr != DSERR_ALREADYINITIALIZED) {
                IDirectSound_Release(pDS);
                pDS = 0;
            } else
                hr = DS_OK;
        }
    }

    *ppDS = pDS;

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
    LPDIRECTSOUND8 pDS;

    TRACE("(%s,%p,%p)\n",debugstr_guid(lpcGUID),ppDS,pUnkOuter);

    if (ppDS == NULL) {
        WARN("invalid parameter: ppDS == NULL\n");
        return DSERR_INVALIDPARAM;
    }

    if (pUnkOuter != NULL) {
        WARN("invalid parameter: pUnkOuter != NULL\n");
        *ppDS = 0;
        return DSERR_INVALIDPARAM;
    }

    hr = DSOUND_Create8(&IID_IDirectSound8, (void **)&pDS);
    if (hr == DS_OK) {
        hr = IDirectSound8_Initialize(pDS, lpcGUID);
        if (hr != DS_OK) {
            if (hr != DSERR_ALREADYINITIALIZED) {
                IDirectSound8_Release(pDS);
                pDS = 0;
            } else
                hr = DS_OK;
        }
    }

    *ppDS = pDS;

    return hr;
}

/*******************************************************************************
 *        DirectSoundDevice
 */
static HRESULT DirectSoundDevice_Create(DirectSoundDevice ** ppDevice)
{
    DirectSoundDevice * device;
    TRACE("(%p)\n", ppDevice);

    /* Allocate memory */
    device = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(DirectSoundDevice));
    if (device == NULL) {
        WARN("out of memory\n");
        return DSERR_OUTOFMEMORY;
    }

    device->ref            = 1;
    device->priolevel      = DSSCL_NORMAL;
    device->state          = STATE_STOPPED;
    device->speaker_config = DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE);

    /* 3D listener initial parameters */
    device->ds3dl.dwSize   = sizeof(DS3DLISTENER);
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

    device->prebuf = ds_snd_queue_max;
    device->guid = GUID_NULL;

    /* Set default wave format (may need it for waveOutOpen) */
    device->pwfx = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(WAVEFORMATEXTENSIBLE));
    device->primary_pwfx = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(WAVEFORMATEXTENSIBLE));
    if (!device->pwfx || !device->primary_pwfx) {
        WARN("out of memory\n");
        HeapFree(GetProcessHeap(),0,device->primary_pwfx);
        HeapFree(GetProcessHeap(),0,device->pwfx);
        HeapFree(GetProcessHeap(),0,device);
        return DSERR_OUTOFMEMORY;
    }

    device->pwfx->wFormatTag = WAVE_FORMAT_PCM;
    device->pwfx->nSamplesPerSec = 22050;
    device->pwfx->wBitsPerSample = 8;
    device->pwfx->nChannels = 2;
    device->pwfx->nBlockAlign = device->pwfx->wBitsPerSample * device->pwfx->nChannels / 8;
    device->pwfx->nAvgBytesPerSec = device->pwfx->nSamplesPerSec * device->pwfx->nBlockAlign;
    device->pwfx->cbSize = 0;
    memcpy(device->primary_pwfx, device->pwfx, sizeof(*device->pwfx));

    InitializeCriticalSection(&(device->mixlock));
    device->mixlock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": DirectSoundDevice.mixlock");

    RtlInitializeResource(&(device->buffer_list_lock));

   *ppDevice = device;

    return DS_OK;
}

static ULONG DirectSoundDevice_AddRef(DirectSoundDevice * device)
{
    ULONG ref = InterlockedIncrement(&(device->ref));
    TRACE("(%p) ref was %d\n", device, ref - 1);
    return ref;
}

ULONG DirectSoundDevice_Release(DirectSoundDevice * device)
{
    HRESULT hr;
    ULONG ref = InterlockedDecrement(&(device->ref));
    TRACE("(%p) ref was %u\n", device, ref + 1);
    if (!ref) {
        int i;

        SetEvent(device->sleepev);
        if (device->thread) {
            WaitForSingleObject(device->thread, INFINITE);
            CloseHandle(device->thread);
        }
        CloseHandle(device->sleepev);

        EnterCriticalSection(&DSOUND_renderers_lock);
        list_remove(&device->entry);
        LeaveCriticalSection(&DSOUND_renderers_lock);

        /* It is allowed to release this object even when buffers are playing */
        if (device->buffers) {
            WARN("%d secondary buffers not released\n", device->nrofbuffers);
            for( i=0;i<device->nrofbuffers;i++)
                secondarybuffer_destroy(device->buffers[i]);
        }

        hr = DSOUND_PrimaryDestroy(device);
        if (hr != DS_OK)
            WARN("DSOUND_PrimaryDestroy failed\n");

        if(device->client)
            IAudioClient_Release(device->client);
        if(device->render)
            IAudioRenderClient_Release(device->render);
        if(device->clock)
            IAudioClock_Release(device->clock);
        if(device->volume)
            IAudioStreamVolume_Release(device->volume);

        HeapFree(GetProcessHeap(), 0, device->tmp_buffer);
        HeapFree(GetProcessHeap(), 0, device->mix_buffer);
        HeapFree(GetProcessHeap(), 0, device->buffer);
        RtlDeleteResource(&device->buffer_list_lock);
        device->mixlock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&device->mixlock);
        HeapFree(GetProcessHeap(),0,device);
        TRACE("(%p) released\n", device);
    }
    return ref;
}

BOOL DSOUND_check_supported(IAudioClient *client, DWORD rate,
        DWORD depth, WORD channels)
{
    WAVEFORMATEX fmt, *junk;
    HRESULT hr;

    fmt.wFormatTag = WAVE_FORMAT_PCM;
    fmt.nChannels = channels;
    fmt.nSamplesPerSec = rate;
    fmt.wBitsPerSample = depth;
    fmt.nBlockAlign = (channels * depth) / 8;
    fmt.nAvgBytesPerSec = rate * fmt.nBlockAlign;
    fmt.cbSize = 0;

    hr = IAudioClient_IsFormatSupported(client, AUDCLNT_SHAREMODE_SHARED, &fmt, &junk);
    if(SUCCEEDED(hr))
        CoTaskMemFree(junk);

    return hr == S_OK;
}

UINT DSOUND_create_timer(LPTIMECALLBACK cb, DWORD_PTR user)
{
    UINT triggertime = DS_TIME_DEL, res = DS_TIME_RES, id;
    TIMECAPS time;

    timeGetDevCaps(&time, sizeof(TIMECAPS));
    TRACE("Minimum timer resolution: %u, max timer: %u\n", time.wPeriodMin, time.wPeriodMax);
    if (triggertime < time.wPeriodMin)
        triggertime = time.wPeriodMin;
    if (res < time.wPeriodMin)
        res = time.wPeriodMin;
    if (timeBeginPeriod(res) == TIMERR_NOCANDO)
        WARN("Could not set minimum resolution, don't expect sound\n");
    id = timeSetEvent(triggertime, res, cb, user, TIME_PERIODIC | TIME_KILL_SYNCHRONOUS);
    if (!id)
    {
        WARN("Timer not created! Retrying without TIME_KILL_SYNCHRONOUS\n");
        id = timeSetEvent(triggertime, res, cb, user, TIME_PERIODIC);
        if (!id)
            ERR("Could not create timer, sound playback will not occur\n");
    }
    return id;
}

HRESULT DirectSoundDevice_Initialize(DirectSoundDevice ** ppDevice, LPCGUID lpcGUID)
{
    HRESULT hr = DS_OK;
    GUID devGUID;
    DirectSoundDevice *device;
    IMMDevice *mmdevice;

    TRACE("(%p,%s)\n",ppDevice,debugstr_guid(lpcGUID));

    if (*ppDevice != NULL) {
        WARN("already initialized\n");
        return DSERR_ALREADYINITIALIZED;
    }

    /* Default device? */
    if (!lpcGUID || IsEqualGUID(lpcGUID, &GUID_NULL))
        lpcGUID = &DSDEVID_DefaultPlayback;

    if(IsEqualGUID(lpcGUID, &DSDEVID_DefaultCapture) ||
            IsEqualGUID(lpcGUID, &DSDEVID_DefaultVoiceCapture))
        return DSERR_NODRIVER;

    if (GetDeviceID(lpcGUID, &devGUID) != DS_OK) {
        WARN("invalid parameter: lpcGUID\n");
        return DSERR_INVALIDPARAM;
    }

    hr = get_mmdevice(eRender, &devGUID, &mmdevice);
    if(FAILED(hr))
        return hr;

    EnterCriticalSection(&DSOUND_renderers_lock);

    LIST_FOR_EACH_ENTRY(device, &DSOUND_renderers, DirectSoundDevice, entry){
        if(IsEqualGUID(&device->guid, &devGUID)){
            IMMDevice_Release(mmdevice);
            DirectSoundDevice_AddRef(device);
            *ppDevice = device;
            LeaveCriticalSection(&DSOUND_renderers_lock);
            return DS_OK;
        }
    }

    hr = DirectSoundDevice_Create(&device);
    if(FAILED(hr)){
        WARN("DirectSoundDevice_Create failed\n");
        IMMDevice_Release(mmdevice);
        LeaveCriticalSection(&DSOUND_renderers_lock);
        return hr;
    }

    device->mmdevice = mmdevice;
    device->guid = devGUID;
    device->sleepev = CreateEventW(0, 0, 0, 0);

    hr = DSOUND_ReopenDevice(device, FALSE);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, device);
        LeaveCriticalSection(&DSOUND_renderers_lock);
        IMMDevice_Release(mmdevice);
        WARN("DSOUND_ReopenDevice failed: %08x\n", hr);
        return hr;
    }

    ZeroMemory(&device->drvcaps, sizeof(device->drvcaps));

    if(DSOUND_check_supported(device->client, 11025, 8, 1) ||
            DSOUND_check_supported(device->client, 22050, 8, 1) ||
            DSOUND_check_supported(device->client, 44100, 8, 1) ||
            DSOUND_check_supported(device->client, 48000, 8, 1) ||
            DSOUND_check_supported(device->client, 96000, 8, 1))
        device->drvcaps.dwFlags |= DSCAPS_PRIMARY8BIT | DSCAPS_PRIMARYMONO;

    if(DSOUND_check_supported(device->client, 11025, 16, 1) ||
            DSOUND_check_supported(device->client, 22050, 16, 1) ||
            DSOUND_check_supported(device->client, 44100, 16, 1) ||
            DSOUND_check_supported(device->client, 48000, 16, 1) ||
            DSOUND_check_supported(device->client, 96000, 16, 1))
        device->drvcaps.dwFlags |= DSCAPS_PRIMARY16BIT | DSCAPS_PRIMARYMONO;

    if(DSOUND_check_supported(device->client, 11025, 8, 2) ||
            DSOUND_check_supported(device->client, 22050, 8, 2) ||
            DSOUND_check_supported(device->client, 44100, 8, 2) ||
            DSOUND_check_supported(device->client, 48000, 8, 2) ||
            DSOUND_check_supported(device->client, 96000, 8, 2))
        device->drvcaps.dwFlags |= DSCAPS_PRIMARY8BIT | DSCAPS_PRIMARYSTEREO;

    if(DSOUND_check_supported(device->client, 11025, 16, 2) ||
            DSOUND_check_supported(device->client, 22050, 16, 2) ||
            DSOUND_check_supported(device->client, 44100, 16, 2) ||
            DSOUND_check_supported(device->client, 48000, 16, 2) ||
            DSOUND_check_supported(device->client, 96000, 16, 2))
        device->drvcaps.dwFlags |= DSCAPS_PRIMARY16BIT | DSCAPS_PRIMARYSTEREO;

    /* the dsound mixer supports all of the following */
    device->drvcaps.dwFlags |= DSCAPS_SECONDARY8BIT | DSCAPS_SECONDARY16BIT;
    device->drvcaps.dwFlags |= DSCAPS_SECONDARYMONO | DSCAPS_SECONDARYSTEREO;
    device->drvcaps.dwFlags |= DSCAPS_CONTINUOUSRATE;

    device->drvcaps.dwPrimaryBuffers = 1;
    device->drvcaps.dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
    device->drvcaps.dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
    device->drvcaps.dwMaxHwMixingAllBuffers = 1;
    device->drvcaps.dwMaxHwMixingStaticBuffers = 1;
    device->drvcaps.dwMaxHwMixingStreamingBuffers = 1;

    ZeroMemory(&device->volpan, sizeof(device->volpan));

    hr = DSOUND_PrimaryCreate(device);
    if (hr == DS_OK) {
        device->thread = CreateThread(0, 0, DSOUND_mixthread, device, 0, 0);
        SetThreadPriority(device->thread, THREAD_PRIORITY_TIME_CRITICAL);
    } else
        WARN("DSOUND_PrimaryCreate failed: %08x\n", hr);

    *ppDevice = device;
    list_add_tail(&DSOUND_renderers, &device->entry);

    LeaveCriticalSection(&DSOUND_renderers_lock);

    return hr;
}

HRESULT DirectSoundDevice_CreateSoundBuffer(
    DirectSoundDevice * device,
    LPCDSBUFFERDESC dsbd,
    LPLPDIRECTSOUNDBUFFER ppdsb,
    LPUNKNOWN lpunk,
    BOOL from8)
{
    HRESULT hres = DS_OK;
    TRACE("(%p,%p,%p,%p)\n",device,dsbd,ppdsb,lpunk);

    if (device == NULL) {
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
    *ppdsb = NULL;

    if (TRACE_ON(dsound)) {
        TRACE("(structsize=%d)\n",dsbd->dwSize);
        TRACE("(flags=0x%08x:\n",dsbd->dwFlags);
        _dump_DSBCAPS(dsbd->dwFlags);
        TRACE(")\n");
        TRACE("(bufferbytes=%d)\n",dsbd->dwBufferBytes);
        TRACE("(lpwfxFormat=%p)\n",dsbd->lpwfxFormat);
    }

    if (dsbd->dwFlags & DSBCAPS_LOCHARDWARE &&
            !(dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER)) {
        TRACE("LOCHARDWARE is not supported, returning E_NOTIMPL\n");
        return E_NOTIMPL;
    }

    if (dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER) {
        if (dsbd->lpwfxFormat != NULL) {
            WARN("invalid parameter: dsbd->lpwfxFormat must be NULL for "
                 "primary buffer\n");
            return DSERR_INVALIDPARAM;
        }

        if (device->primary) {
            WARN("Primary Buffer already created\n");
            IDirectSoundBuffer_AddRef((LPDIRECTSOUNDBUFFER8)(device->primary));
            *ppdsb = (LPDIRECTSOUNDBUFFER)(device->primary);
        } else {
            hres = primarybuffer_create(device, &device->primary, dsbd);
            if (device->primary) {
                *ppdsb = (IDirectSoundBuffer*)&device->primary->IDirectSoundBuffer8_iface;
                device->primary->dsbd.dwFlags &= ~(DSBCAPS_LOCHARDWARE | DSBCAPS_LOCSOFTWARE);
                device->primary->dsbd.dwFlags |= DSBCAPS_LOCSOFTWARE;
            } else
                WARN("primarybuffer_create() failed\n");
        }
    } else {
        IDirectSoundBufferImpl * dsb;
        WAVEFORMATEXTENSIBLE *pwfxe;

        if (dsbd->lpwfxFormat == NULL) {
            WARN("invalid parameter: dsbd->lpwfxFormat can't be NULL for "
                 "secondary buffer\n");
            return DSERR_INVALIDPARAM;
        }
        pwfxe = (WAVEFORMATEXTENSIBLE*)dsbd->lpwfxFormat;

        if (pwfxe->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            /* check if cbSize is at least 22 bytes */
            if (pwfxe->Format.cbSize < (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)))
            {
                WARN("Too small a cbSize %u\n", pwfxe->Format.cbSize);
                return DSERR_INVALIDPARAM;
            }

            /* cbSize should be 22 bytes, with one possible exception */
            if (pwfxe->Format.cbSize > (sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX)) &&
                !((IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM) || IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)) &&
                pwfxe->Format.cbSize == sizeof(WAVEFORMATEXTENSIBLE)))
            {
                WARN("Too big a cbSize %u\n", pwfxe->Format.cbSize);
                return DSERR_CONTROLUNAVAIL;
            }

            if ((!IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM)) && (!IsEqualGUID(&pwfxe->SubFormat, &KSDATAFORMAT_SUBTYPE_IEEE_FLOAT)))
            {
                if (!IsEqualGUID(&pwfxe->SubFormat, &GUID_NULL))
                    FIXME("SubFormat %s not supported right now.\n", debugstr_guid(&pwfxe->SubFormat));
                return DSERR_INVALIDPARAM;
            }
            if (pwfxe->Samples.wValidBitsPerSample > dsbd->lpwfxFormat->wBitsPerSample)
            {
                WARN("Samples.wValidBitsPerSample(%d) > Format.wBitsPerSample (%d)\n", pwfxe->Samples.wValidBitsPerSample, pwfxe->Format.wBitsPerSample);
                return DSERR_INVALIDPARAM;
            }
            if (pwfxe->Samples.wValidBitsPerSample && pwfxe->Samples.wValidBitsPerSample < dsbd->lpwfxFormat->wBitsPerSample)
            {
                FIXME("Non-packed formats not supported right now: %d/%d\n", pwfxe->Samples.wValidBitsPerSample, dsbd->lpwfxFormat->wBitsPerSample);
                return DSERR_CONTROLUNAVAIL;
            }
        }

        TRACE("(formattag=0x%04x,chans=%d,samplerate=%d,"
              "bytespersec=%d,blockalign=%d,bitspersamp=%d,cbSize=%d)\n",
              dsbd->lpwfxFormat->wFormatTag, dsbd->lpwfxFormat->nChannels,
              dsbd->lpwfxFormat->nSamplesPerSec,
              dsbd->lpwfxFormat->nAvgBytesPerSec,
              dsbd->lpwfxFormat->nBlockAlign,
              dsbd->lpwfxFormat->wBitsPerSample, dsbd->lpwfxFormat->cbSize);

        if (from8 && (dsbd->dwFlags & DSBCAPS_CTRL3D) && (dsbd->lpwfxFormat->nChannels != 1)) {
            WARN("invalid parameter: 3D buffer format must be mono\n");
            return DSERR_INVALIDPARAM;
        }

        hres = IDirectSoundBufferImpl_Create(device, &dsb, dsbd);
        if (dsb)
            *ppdsb = (IDirectSoundBuffer*)&dsb->IDirectSoundBuffer8_iface;
        else
            WARN("IDirectSoundBufferImpl_Create failed\n");
   }

   return hres;
}

HRESULT DirectSoundDevice_DuplicateSoundBuffer(
    DirectSoundDevice * device,
    LPDIRECTSOUNDBUFFER psb,
    LPLPDIRECTSOUNDBUFFER ppdsb)
{
    HRESULT hres = DS_OK;
    IDirectSoundBufferImpl* dsb;
    TRACE("(%p,%p,%p)\n",device,psb,ppdsb);

    if (device == NULL) {
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

    /* make sure we have a secondary buffer */
    if (psb == (IDirectSoundBuffer *)&device->primary->IDirectSoundBuffer8_iface) {
        WARN("trying to duplicate primary buffer\n");
        *ppdsb = NULL;
        return DSERR_INVALIDCALL;
    }

    /* duplicate the actual buffer implementation */
    hres = IDirectSoundBufferImpl_Duplicate(device, &dsb, (IDirectSoundBufferImpl*)psb);
    if (hres == DS_OK)
        *ppdsb = (IDirectSoundBuffer*)&dsb->IDirectSoundBuffer8_iface;
    else
        WARN("IDirectSoundBufferImpl_Duplicate failed\n");

    return hres;
}

/*
 * Add secondary buffer to buffer list.
 * Gets exclusive access to buffer for writing.
 */
HRESULT DirectSoundDevice_AddBuffer(
    DirectSoundDevice * device,
    IDirectSoundBufferImpl * pDSB)
{
    IDirectSoundBufferImpl **newbuffers;
    HRESULT hr = DS_OK;

    TRACE("(%p, %p)\n", device, pDSB);

    RtlAcquireResourceExclusive(&(device->buffer_list_lock), TRUE);

    if (device->buffers)
        newbuffers = HeapReAlloc(GetProcessHeap(),0,device->buffers,sizeof(IDirectSoundBufferImpl*)*(device->nrofbuffers+1));
    else
        newbuffers = HeapAlloc(GetProcessHeap(),0,sizeof(IDirectSoundBufferImpl*)*(device->nrofbuffers+1));

    if (newbuffers) {
        device->buffers = newbuffers;
        device->buffers[device->nrofbuffers] = pDSB;
        device->nrofbuffers++;
        TRACE("buffer count is now %d\n", device->nrofbuffers);
    } else {
        ERR("out of memory for buffer list! Current buffer count is %d\n", device->nrofbuffers);
        hr = DSERR_OUTOFMEMORY;
    }

    RtlReleaseResource(&(device->buffer_list_lock));

    return hr;
}

/*
 * Remove secondary buffer from buffer list.
 * Gets exclusive access to buffer for writing.
 */
void DirectSoundDevice_RemoveBuffer(DirectSoundDevice * device, IDirectSoundBufferImpl * pDSB)
{
    int i;

    TRACE("(%p, %p)\n", device, pDSB);

    RtlAcquireResourceExclusive(&(device->buffer_list_lock), TRUE);

    if (device->nrofbuffers == 1) {
        assert(device->buffers[0] == pDSB);
        HeapFree(GetProcessHeap(), 0, device->buffers);
        device->buffers = NULL;
    } else {
        for (i = 0; i < device->nrofbuffers; i++) {
            if (device->buffers[i] == pDSB) {
                /* Put the last buffer of the list in the (now empty) position */
                device->buffers[i] = device->buffers[device->nrofbuffers - 1];
                break;
            }
        }
    }
    device->nrofbuffers--;
    TRACE("buffer count is now %d\n", device->nrofbuffers);

    RtlReleaseResource(&(device->buffer_list_lock));
}
