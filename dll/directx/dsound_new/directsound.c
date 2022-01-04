/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/directsound.c
 * PURPOSE:         Handles IDirectSound interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

typedef struct
{
    IDirectSound8Vtbl *lpVtbl;
    LONG ref;
    GUID DeviceGUID;
    BOOL bInitialized;
    BOOL bDirectSound8;
    DWORD dwLevel;
    LPFILTERINFO Filter;
    LPDIRECTSOUNDBUFFER8 PrimaryBuffer;


}CDirectSoundImpl, *LPCDirectSoundImpl;

HRESULT
WINAPI
IDirectSound8_fnQueryInterface(
    LPDIRECTSOUND8 iface,
    REFIID riid,
    LPVOID * ppobj)
{
    LPOLESTR pStr;
    LPCDirectSoundImpl This = (LPCDirectSoundImpl)CONTAINING_RECORD(iface, CDirectSoundImpl, lpVtbl);

    if ((IsEqualIID(riid, &IID_IDirectSound) && This->bDirectSound8 == FALSE) ||
        (IsEqualIID(riid, &IID_IDirectSound8) && This->bDirectSound8 != FALSE) ||
        (IsEqualIID(riid, &IID_IUnknown)))
    {
        *ppobj = (LPVOID)&This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return S_OK;
    }

    if (SUCCEEDED(StringFromIID(riid, &pStr)))
    {
        DPRINT("No Interface for class %s\n", pStr);
        CoTaskMemFree(pStr);
    }
    return E_NOINTERFACE;
}

ULONG
WINAPI
IDirectSound8_fnAddRef(
    LPDIRECTSOUND8 iface)
{
    ULONG ref;
    LPCDirectSoundImpl This = (LPCDirectSoundImpl)CONTAINING_RECORD(iface, CDirectSoundImpl, lpVtbl);

    ref = InterlockedIncrement(&This->ref);

    return ref;
}

ULONG
WINAPI
IDirectSound8_fnRelease(
    LPDIRECTSOUND8 iface)
{
    ULONG ref;
    LPCDirectSoundImpl This = (LPCDirectSoundImpl)CONTAINING_RECORD(iface, CDirectSoundImpl, lpVtbl);

    ref = InterlockedDecrement(&(This->ref));

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

HRESULT
WINAPI
IDirectSound8_fnCreateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPCDSBUFFERDESC lpcDSBufferDesc,
    LPLPDIRECTSOUNDBUFFER lplpDirectSoundBuffer,
    IUnknown FAR* pUnkOuter)
{
    HRESULT hResult;
    LPCDirectSoundImpl This = (LPCDirectSoundImpl)CONTAINING_RECORD(iface, CDirectSoundImpl, lpVtbl);

    if (!This->bInitialized)
    {
        /* object not yet initialized */
        return DSERR_UNINITIALIZED;
    }

    if (!lpcDSBufferDesc  || !lplpDirectSoundBuffer || pUnkOuter != NULL)
    {
        DPRINT("Invalid parameter %p %p %p\n", lpcDSBufferDesc, lplpDirectSoundBuffer, pUnkOuter);
        return DSERR_INVALIDPARAM;
    }

    /* check buffer description */
    if ((lpcDSBufferDesc->dwSize != sizeof(DSBUFFERDESC) && lpcDSBufferDesc->dwSize != sizeof(DSBUFFERDESC1)) || lpcDSBufferDesc->dwReserved != 0)
    {
        DPRINT("Invalid buffer description size %u expected %u dwReserved %u\n", lpcDSBufferDesc->dwSize, sizeof(DSBUFFERDESC1), lpcDSBufferDesc->dwReserved);
        return DSERR_INVALIDPARAM;
    }

    DPRINT("This %p dwFlags %x dwBufferBytes %u lpwfxFormat %p dwSize %u\n", This, lpcDSBufferDesc->dwFlags, lpcDSBufferDesc->dwBufferBytes, lpcDSBufferDesc->lpwfxFormat, lpcDSBufferDesc->dwSize);

    if (lpcDSBufferDesc->dwFlags & DSBCAPS_PRIMARYBUFFER)
    {
        if (lpcDSBufferDesc->lpwfxFormat != NULL)
        {
            /* format must be null for primary sound buffer */
            return DSERR_INVALIDPARAM;
        }

        if (lpcDSBufferDesc->dwBufferBytes != 0)
        {
            /* buffer size must be zero for primary sound buffer */
            return DSERR_INVALIDPARAM;
        }

        if (This->PrimaryBuffer)
        {
            /* primary buffer already exists */
            IDirectSoundBuffer8_AddRef(This->PrimaryBuffer);
            *lplpDirectSoundBuffer = (LPDIRECTSOUNDBUFFER)This->PrimaryBuffer;
            return S_OK;
        }

        hResult = NewPrimarySoundBuffer((LPLPDIRECTSOUNDBUFFER8)lplpDirectSoundBuffer, This->Filter, This->dwLevel, lpcDSBufferDesc->dwFlags);
        if (SUCCEEDED(hResult))
        {
            /* store primary buffer */
            This->PrimaryBuffer = (LPDIRECTSOUNDBUFFER8)*lplpDirectSoundBuffer;
        }
        return hResult;
    }
    else
    {
        if (lpcDSBufferDesc->lpwfxFormat == NULL)
        {
            /* format must not be null */
            return DSERR_INVALIDPARAM;
        }

        if (lpcDSBufferDesc->dwBufferBytes < DSBSIZE_MIN || lpcDSBufferDesc->dwBufferBytes > DSBSIZE_MAX)
        {
            /* buffer size must be within bounds for secondary sound buffer*/
            return DSERR_INVALIDPARAM;
        }

        if (!This->PrimaryBuffer)
        {
            hResult = NewPrimarySoundBuffer((LPLPDIRECTSOUNDBUFFER8)lplpDirectSoundBuffer, This->Filter, This->dwLevel, lpcDSBufferDesc->dwFlags);
            if (SUCCEEDED(hResult))
            {
                /* store primary buffer */
                This->PrimaryBuffer = (LPDIRECTSOUNDBUFFER8)*lplpDirectSoundBuffer;
            }
            else
            {
                DPRINT("Failed to create primary buffer with %x\n", hResult);
                return hResult;
            }

        }

        ASSERT(This->PrimaryBuffer);

        DPRINT("This %p wFormatTag %x nChannels %u nSamplesPerSec %u nAvgBytesPerSec %u NBlockAlign %u wBitsPerSample %u cbSize %u\n",
               This, lpcDSBufferDesc->lpwfxFormat->wFormatTag, lpcDSBufferDesc->lpwfxFormat->nChannels, lpcDSBufferDesc->lpwfxFormat->nSamplesPerSec, lpcDSBufferDesc->lpwfxFormat->nAvgBytesPerSec, lpcDSBufferDesc->lpwfxFormat->nBlockAlign, lpcDSBufferDesc->lpwfxFormat->wBitsPerSample, lpcDSBufferDesc->lpwfxFormat->cbSize);

        hResult = NewSecondarySoundBuffer((LPLPDIRECTSOUNDBUFFER8)lplpDirectSoundBuffer, This->Filter, This->dwLevel, lpcDSBufferDesc, This->PrimaryBuffer);
        return hResult;
    }
}

HRESULT
WINAPI
IDirectSound8_fnGetCaps(
    LPDIRECTSOUND8 iface,
    LPDSCAPS lpDSCaps)
{
    LPCDirectSoundImpl This = (LPCDirectSoundImpl)CONTAINING_RECORD(iface, CDirectSoundImpl, lpVtbl);

    if (!This->bInitialized)
    {
        /* object not yet initialized */
        return DSERR_UNINITIALIZED;
    }

    if (!lpDSCaps)
    {
        /* object not yet initialized */
        return DSERR_INVALIDPARAM;
    }

    if (lpDSCaps->dwSize != sizeof(DSCAPS))
    {
        /* object not yet initialized */
        return DSERR_INVALIDPARAM;
    }

    UNIMPLEMENTED;
    return DSERR_GENERIC;
}

HRESULT
WINAPI
IDirectSound8_fnDuplicateSoundBuffer(
    LPDIRECTSOUND8 iface,
    LPDIRECTSOUNDBUFFER lpDsbOriginal,
    LPLPDIRECTSOUNDBUFFER lplpDsbDuplicate)
{
    UNIMPLEMENTED;
    return DSERR_OUTOFMEMORY;
}

HRESULT
WINAPI
IDirectSound8_fnSetCooperativeLevel(
    LPDIRECTSOUND8 iface,
    HWND hwnd,
    DWORD dwLevel)
{
    LPCDirectSoundImpl This = (LPCDirectSoundImpl)CONTAINING_RECORD(iface, CDirectSoundImpl, lpVtbl);

    if (!This->bInitialized)
    {
        /* object not yet initialized */
        return DSERR_UNINITIALIZED;
    }

    /* store cooperation level */
    This->dwLevel = dwLevel;
    return DS_OK;
}

HRESULT
WINAPI
IDirectSound8_fnCompact(
    LPDIRECTSOUND8 iface)
{
    LPCDirectSoundImpl This = (LPCDirectSoundImpl)CONTAINING_RECORD(iface, CDirectSoundImpl, lpVtbl);

    if (!This->bInitialized)
    {
        /* object not yet initialized */
        return DSERR_UNINITIALIZED;
    }

    if (This->dwLevel != DSSCL_PRIORITY)
    {
        /* needs priority level */
        return DSERR_PRIOLEVELNEEDED;
    }

    /* done */
    return DS_OK;
}

HRESULT
WINAPI
IDirectSound8_fnGetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    LPDWORD pdwSpeakerConfig)
{
    LPCDirectSoundImpl This = (LPCDirectSoundImpl)CONTAINING_RECORD(iface, CDirectSoundImpl, lpVtbl);

    if (!This->bInitialized)
    {
        /* object not yet initialized */
        return DSERR_UNINITIALIZED;
    }


    UNIMPLEMENTED;
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
IDirectSound8_fnSetSpeakerConfig(
    LPDIRECTSOUND8 iface,
    DWORD dwSpeakerConfig)
{
    UNIMPLEMENTED;
    return DSERR_INVALIDPARAM;
}


HRESULT
WINAPI
IDirectSound8_fnInitialize(
    LPDIRECTSOUND8 iface,
    LPCGUID pcGuidDevice)
{
    GUID DeviceGuid;
    LPOLESTR pGuidStr;
    HRESULT hr;
    LPCDirectSoundImpl This = (LPCDirectSoundImpl)CONTAINING_RECORD(iface, CDirectSoundImpl, lpVtbl);

    if (!RootInfo)
    {
        EnumAudioDeviceInterfaces(&RootInfo);
    }

    /* sanity check */
    ASSERT(RootInfo);

    if (This->bInitialized)
    {
        /* object has already been initialized */
        return DSERR_ALREADYINITIALIZED;
    }

    /* fixme mutual exclusion */

    if (pcGuidDevice == NULL || IsEqualGUID(pcGuidDevice, &GUID_NULL))
    {
        /* use default playback device id */
        pcGuidDevice = &DSDEVID_DefaultPlayback;
    }

    if (IsEqualIID(pcGuidDevice, &DSDEVID_DefaultCapture) || IsEqualIID(pcGuidDevice, &DSDEVID_DefaultVoiceCapture))
    {
        /* this has to be a winetest */
        return DSERR_NODRIVER;
    }

    /* now verify the guid */
    if (GetDeviceID(pcGuidDevice, &DeviceGuid) != DS_OK)
    {
        if (SUCCEEDED(StringFromIID(pcGuidDevice, &pGuidStr)))
        {
            DPRINT("IDirectSound8_fnInitialize: Unknown GUID %ws\n", pGuidStr);
            CoTaskMemFree(pGuidStr);
        }
        return DSERR_INVALIDPARAM;
    }

    hr = FindDeviceByGuid(&DeviceGuid, &This->Filter);

    if (SUCCEEDED(hr))
    {
        This->bInitialized = TRUE;
        return DS_OK;
    }

    DPRINT("Failed to find device\n");
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
IDirectSound8_fnVerifyCertification(
    LPDIRECTSOUND8 iface,
    LPDWORD pdwCertified)
{
    LPCDirectSoundImpl This = (LPCDirectSoundImpl)CONTAINING_RECORD(iface, CDirectSoundImpl, lpVtbl);

    if (!This->bInitialized)
    {
        /* object not yet initialized */
        return DSERR_UNINITIALIZED;
    }

    UNIMPLEMENTED;
    return DS_CERTIFIED;
}

static IDirectSound8Vtbl vt_DirectSound8 =
{
    /* IUnknown methods */
    IDirectSound8_fnQueryInterface,
    IDirectSound8_fnAddRef,
    IDirectSound8_fnRelease,
    /* IDirectSound methods */
    IDirectSound8_fnCreateSoundBuffer,
    IDirectSound8_fnGetCaps,
    IDirectSound8_fnDuplicateSoundBuffer,
    IDirectSound8_fnSetCooperativeLevel,
    IDirectSound8_fnCompact,
    IDirectSound8_fnGetSpeakerConfig,
    IDirectSound8_fnSetSpeakerConfig,
    IDirectSound8_fnInitialize,
    /* IDirectSound8 methods */
    IDirectSound8_fnVerifyCertification
};

HRESULT
InternalDirectSoundCreate(
    LPCGUID lpcGUID,
    LPDIRECTSOUND8 *ppDS,
    IUnknown *pUnkOuter,
    BOOL bDirectSound8)
{
    LPCDirectSoundImpl This;
    HRESULT hr;

    if (!ppDS || pUnkOuter != NULL)
    {
        /* invalid parameter passed */
        return DSERR_INVALIDPARAM;
    }

    /* allocate CDirectSoundImpl struct */
    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CDirectSoundImpl));
    if (!This)
    {
        /* not enough memory */
        return DSERR_OUTOFMEMORY;
    }

    /* initialize IDirectSound object */
    This->ref = 1;
    This->bDirectSound8 = bDirectSound8;
    This->lpVtbl = &vt_DirectSound8;


    /* initialize direct sound interface */
    hr = IDirectSound8_Initialize((LPDIRECTSOUND8)&This->lpVtbl, lpcGUID);

    /* check for success */
    if (!SUCCEEDED(hr))
    {
        /* failed */
        DPRINT("Failed to initialize DirectSound object with %x\n", hr);
        IDirectSound8_Release((LPDIRECTSOUND8)&This->lpVtbl);
        return hr;
    }

    /* store result */
    *ppDS = (LPDIRECTSOUND8)&This->lpVtbl;
    DPRINT("DirectSound object %p\n", *ppDS);
    return DS_OK;
}

HRESULT
CALLBACK
NewDirectSound(
    IUnknown* pUnkOuter,
    REFIID riid,
    LPVOID* ppvObject)
{
    LPOLESTR pStr;
    LPCDirectSoundImpl This;

    /* check param */
    if (!ppvObject)
    {
        /* invalid param */
        return E_INVALIDARG;
    }

    /* check requested interface */
    if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IDirectSound) && !IsEqualIID(riid, &IID_IDirectSound8))
    {
        *ppvObject = 0;
        StringFromIID(riid, &pStr);
        DPRINT("KsPropertySet does not support Interface %ws\n", pStr);
        CoTaskMemFree(pStr);
        return E_NOINTERFACE;
    }

    /* allocate CDirectSoundCaptureImpl struct */
    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CDirectSoundImpl));
    if (!This)
    {
        /* not enough memory */
        return DSERR_OUTOFMEMORY;
    }

    /* initialize object */
    This->ref = 1;
    This->lpVtbl = &vt_DirectSound8;
    This->bInitialized = FALSE;
    *ppvObject = (LPVOID)&This->lpVtbl;

    return S_OK;
}


HRESULT
WINAPI
DirectSoundCreate(
    LPCGUID lpcGUID,
    LPDIRECTSOUND *ppDS,
    IUnknown *pUnkOuter)
{
    return InternalDirectSoundCreate(lpcGUID, (LPDIRECTSOUND8*)ppDS, pUnkOuter, FALSE);
}

HRESULT
WINAPI
DirectSoundCreate8(
    LPCGUID lpcGUID,
    LPDIRECTSOUND8 *ppDS,
    IUnknown *pUnkOuter)
{
    return InternalDirectSoundCreate(lpcGUID, ppDS, pUnkOuter, TRUE);
}
