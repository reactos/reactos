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
#include <stdio.h>
#include <math.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winternl.h"
#include "mmddk.h"
#include "wingdi.h"
#include "mmreg.h"
#include "ks.h"
#include "ksmedia.h"
#include "wine/debug.h"
#include "dsound.h"
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
    device->stopped        = 1;
    device->speaker_config = DSSPEAKER_COMBINED(DSSPEAKER_STEREO, DSSPEAKER_GEOMETRY_WIDE);

    DSOUND_ParseSpeakerConfig(device);

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

    device->guid = GUID_NULL;

    /* Set default wave format (may need it for waveOutOpen) */
    device->primary_pwfx = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(WAVEFORMATEXTENSIBLE));
    if (!device->primary_pwfx) {
        WARN("out of memory\n");
        HeapFree(GetProcessHeap(),0,device);
        return DSERR_OUTOFMEMORY;
    }

    device->primary_pwfx->wFormatTag = WAVE_FORMAT_PCM;
    device->primary_pwfx->nSamplesPerSec = 22050;
    device->primary_pwfx->wBitsPerSample = 8;
    device->primary_pwfx->nChannels = 2;
    device->primary_pwfx->nBlockAlign = device->primary_pwfx->wBitsPerSample * device->primary_pwfx->nChannels / 8;
    device->primary_pwfx->nAvgBytesPerSec = device->primary_pwfx->nSamplesPerSec * device->primary_pwfx->nBlockAlign;
    device->primary_pwfx->cbSize = 0;

    InitializeCriticalSection(&(device->mixlock));
    device->mixlock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": DirectSoundDevice.mixlock");

    RtlInitializeResource(&(device->buffer_list_lock));

    init_eax_device(device);

   *ppDevice = device;

    return DS_OK;
}

static ULONG DirectSoundDevice_AddRef(DirectSoundDevice * device)
{
    ULONG ref = InterlockedIncrement(&(device->ref));
    TRACE("(%p) ref was %d\n", device, ref - 1);
    return ref;
}

static ULONG DirectSoundDevice_Release(DirectSoundDevice * device)
{
    HRESULT hr;
    ULONG ref = InterlockedDecrement(&(device->ref));
    TRACE("(%p) ref was %u\n", device, ref + 1);
    if (!ref) {
        int i;

        SetEvent(device->sleepev);
        if (device->thread) {
            WaitForSingleObject(device->thread_finished, INFINITE);
            CloseHandle(device->thread);
            CloseHandle(device->thread_finished);
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

        if(device->client) {
            IAudioClient_Stop(device->client);
            IAudioClient_Release(device->client);
        }
        if(device->render)
            IAudioRenderClient_Release(device->render);
        if(device->volume)
            IAudioStreamVolume_Release(device->volume);
        if(device->mmdevice)
            IMMDevice_Release(device->mmdevice);

        HeapFree(GetProcessHeap(), 0, device->dsp_buffer);
        HeapFree(GetProcessHeap(), 0, device->tmp_buffer);
        HeapFree(GetProcessHeap(), 0, device->cp_buffer);
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

static HRESULT DirectSoundDevice_Initialize(DirectSoundDevice ** ppDevice, LPCGUID lpcGUID)
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
    device->buflen = ds_hel_buflen;

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

    /* pretend that the driver is certified */
    device->drvcaps.dwFlags |= DSCAPS_CERTIFIED;

    device->drvcaps.dwPrimaryBuffers = 1;
    device->drvcaps.dwMinSecondarySampleRate = DSBFREQUENCY_MIN;
    device->drvcaps.dwMaxSecondarySampleRate = DSBFREQUENCY_MAX;
    device->drvcaps.dwMaxHwMixingAllBuffers = 16;
    device->drvcaps.dwMaxHwMixingStaticBuffers = device->drvcaps.dwMaxHwMixingAllBuffers;
    device->drvcaps.dwMaxHwMixingStreamingBuffers = device->drvcaps.dwMaxHwMixingAllBuffers;
    device->drvcaps.dwFreeHwMixingAllBuffers = device->drvcaps.dwMaxHwMixingAllBuffers;
    device->drvcaps.dwFreeHwMixingStaticBuffers = device->drvcaps.dwMaxHwMixingStaticBuffers;
    device->drvcaps.dwFreeHwMixingStreamingBuffers = device->drvcaps.dwMaxHwMixingStreamingBuffers;

    ZeroMemory(&device->volpan, sizeof(device->volpan));

    device->thread_finished = CreateEventW(0, 0, 0, 0);
    device->thread = CreateThread(0, 0, DSOUND_mixthread, device, 0, 0);
    SetThreadPriority(device->thread, THREAD_PRIORITY_TIME_CRITICAL);

    *ppDevice = device;
    list_add_tail(&DSOUND_renderers, &device->entry);

    LeaveCriticalSection(&DSOUND_renderers_lock);

    return hr;
}

static HRESULT DirectSoundDevice_CreateSoundBuffer(
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

    if (!(dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER) &&
        dsbd->dwFlags & DSBCAPS_LOCHARDWARE &&
        device->drvcaps.dwFreeHwMixingAllBuffers == 0)
    {
        WARN("ran out of emulated hardware buffers\n");
        return DSERR_ALLOCATED;
    }

    if (dsbd->dwFlags & DSBCAPS_PRIMARYBUFFER) {
        if (dsbd->lpwfxFormat != NULL) {
            WARN("invalid parameter: dsbd->lpwfxFormat must be NULL for "
                 "primary buffer\n");
            return DSERR_INVALIDPARAM;
        }

        if (device->primary) {
            WARN("Primary Buffer already created\n");
            IDirectSoundBuffer8_AddRef(&device->primary->IDirectSoundBuffer8_iface);
            *ppdsb = (IDirectSoundBuffer *)&device->primary->IDirectSoundBuffer8_iface;
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

        if (dsbd->lpwfxFormat == NULL) {
            WARN("invalid parameter: dsbd->lpwfxFormat can't be NULL for "
                 "secondary buffer\n");
            return DSERR_INVALIDPARAM;
        }

        if(dsbd->lpwfxFormat->wFormatTag != WAVE_FORMAT_PCM &&
                dsbd->lpwfxFormat->wFormatTag != WAVE_FORMAT_IEEE_FLOAT &&
                dsbd->lpwfxFormat->wFormatTag != WAVE_FORMAT_EXTENSIBLE) {
            WARN("We can't mix this format: 0x%x\n", dsbd->lpwfxFormat->wFormatTag);
            return E_NOTIMPL;
        }

        if(dsbd->lpwfxFormat->wBitsPerSample < 8 || dsbd->lpwfxFormat->wBitsPerSample % 8 != 0 ||
                dsbd->lpwfxFormat->nChannels == 0 || dsbd->lpwfxFormat->nSamplesPerSec == 0 ||
                dsbd->lpwfxFormat->nAvgBytesPerSec == 0 ||
                dsbd->lpwfxFormat->nBlockAlign != dsbd->lpwfxFormat->nChannels * dsbd->lpwfxFormat->wBitsPerSample / 8) {
            WARN("Format inconsistency\n");
            return DSERR_INVALIDPARAM;
        }

        if (dsbd->lpwfxFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
        {
            WAVEFORMATEXTENSIBLE *pwfxe = (WAVEFORMATEXTENSIBLE*)dsbd->lpwfxFormat;

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
                WARN("Non-packed formats may not function : %d/%d\n", pwfxe->Samples.wValidBitsPerSample, dsbd->lpwfxFormat->wBitsPerSample);
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

        if (from8 && (dsbd->dwFlags & DSBCAPS_CTRL3D) && (dsbd->dwFlags & DSBCAPS_CTRLPAN)) {
            WARN("invalid parameter: DSBCAPS_CTRL3D and DSBCAPS_CTRLPAN cannot be used together\n");
            return DSERR_INVALIDPARAM;
        }

        hres = IDirectSoundBufferImpl_Create(device, &dsb, dsbd);
        if (dsb) {
            *ppdsb = (IDirectSoundBuffer*)&dsb->IDirectSoundBuffer8_iface;
            if (dsbd->dwFlags & DSBCAPS_LOCHARDWARE)
                device->drvcaps.dwFreeHwMixingAllBuffers--;
        } else
            WARN("IDirectSoundBufferImpl_Create failed\n");
   }

   return hres;
}

static HRESULT DirectSoundDevice_DuplicateSoundBuffer(
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

/*******************************************************************************
 *      IUnknown Implementation for DirectSound
 */

static void directsound_destroy(IDirectSoundImpl *This)
{
    if (This->device)
        DirectSoundDevice_Release(This->device);
    HeapFree(GetProcessHeap(),0,This);
    TRACE("(%p) released\n", This);
}

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
    dscaps->dwFreeHwMixingStaticBuffers    = This->device->drvcaps.dwFreeHwMixingAllBuffers;
    dscaps->dwFreeHwMixingStreamingBuffers = This->device->drvcaps.dwFreeHwMixingAllBuffers;

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
    if ((level == DSSCL_WRITEPRIMARY) != (device->priolevel == DSSCL_WRITEPRIMARY))
        hr = DSOUND_ReopenDevice(device, level == DSSCL_WRITEPRIMARY);
    if (SUCCEEDED(hr))
        device->priolevel = level;
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

    /* NOP on Vista and above */

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

void DSOUND_ParseSpeakerConfig(DirectSoundDevice *device)
{
    switch (DSSPEAKER_CONFIG(device->speaker_config)) {
        case DSSPEAKER_MONO:
            device->speaker_angles[0] = M_PI/180.0f * 0.0f;
            device->speaker_num[0] = 0;
            device->num_speakers = 1;
            device->lfe_channel = -1;
        break;

        case DSSPEAKER_STEREO:
        case DSSPEAKER_HEADPHONE:
            device->speaker_angles[0] = M_PI/180.0f * -90.0f;
            device->speaker_angles[1] = M_PI/180.0f *  90.0f;
            device->speaker_num[0] = 0; /* Left */
            device->speaker_num[1] = 1; /* Right */
            device->num_speakers = 2;
            device->lfe_channel = -1;
        break;

        case DSSPEAKER_QUAD:
            device->speaker_angles[0] = M_PI/180.0f * -135.0f;
            device->speaker_angles[1] = M_PI/180.0f *  -45.0f;
            device->speaker_angles[2] = M_PI/180.0f *   45.0f;
            device->speaker_angles[3] = M_PI/180.0f *  135.0f;
            device->speaker_num[0] = 2; /* Rear left */
            device->speaker_num[1] = 0; /* Front left */
            device->speaker_num[2] = 1; /* Front right */
            device->speaker_num[3] = 3; /* Rear right */
            device->num_speakers = 4;
            device->lfe_channel = -1;
        break;

        case DSSPEAKER_5POINT1_BACK:
            device->speaker_angles[0] = M_PI/180.0f * -135.0f;
            device->speaker_angles[1] = M_PI/180.0f *  -45.0f;
            device->speaker_angles[2] = M_PI/180.0f *    0.0f;
            device->speaker_angles[3] = M_PI/180.0f *   45.0f;
            device->speaker_angles[4] = M_PI/180.0f *  135.0f;
            device->speaker_angles[5] = 9999.0f;
            device->speaker_num[0] = 4; /* Rear left */
            device->speaker_num[1] = 0; /* Front left */
            device->speaker_num[2] = 2; /* Front centre */
            device->speaker_num[3] = 1; /* Front right */
            device->speaker_num[4] = 5; /* Rear right */
            device->speaker_num[5] = 3; /* LFE */
            device->num_speakers = 6;
            device->lfe_channel = 3;
        break;

        case DSSPEAKER_5POINT1_SURROUND:
            device->speaker_angles[0] = M_PI/180.0f *  -90.0f;
            device->speaker_angles[1] = M_PI/180.0f *  -30.0f;
            device->speaker_angles[2] = M_PI/180.0f *    0.0f;
            device->speaker_angles[3] = M_PI/180.0f *   30.0f;
            device->speaker_angles[4] = M_PI/180.0f *   90.0f;
            device->speaker_angles[5] = 9999.0f;
            device->speaker_num[0] = 4; /* Rear left */
            device->speaker_num[1] = 0; /* Front left */
            device->speaker_num[2] = 2; /* Front centre */
            device->speaker_num[3] = 1; /* Front right */
            device->speaker_num[4] = 5; /* Rear right */
            device->speaker_num[5] = 3; /* LFE */
            device->num_speakers = 6;
            device->lfe_channel = 3;
        break;

        default:
            WARN("unknown speaker_config %u\n", device->speaker_config);
    }
}
