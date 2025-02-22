/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/capture.c
 * PURPOSE:         Implement IDirectSoundCapture
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

typedef struct
{
    IDirectSoundCaptureVtbl *lpVtbl;
    LONG ref;
    GUID DeviceGUID;
    BOOL bInitialized;
    LPFILTERINFO Filter;
}CDirectSoundCaptureImpl, *LPCDirectSoundCaptureImpl;

HRESULT
WINAPI
CDirectSoundCapture_fnQueryInterface(
    LPDIRECTSOUNDCAPTURE8 iface,
    REFIID riid,
    LPVOID * ppobj)
{
    LPOLESTR pStr;
    LPCDirectSoundCaptureImpl This = (LPCDirectSoundCaptureImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureImpl, lpVtbl);

    /* check if the interface is supported */
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectSoundCapture))
    {
        *ppobj = (LPVOID)&This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return S_OK;
    }

    /* unsupported interface */
    if (SUCCEEDED(StringFromIID(riid, &pStr)))
    {
        DPRINT("No Interface for class %s\n", pStr);
        CoTaskMemFree(pStr);
    }
    return E_NOINTERFACE;
}

ULONG
WINAPI
CDirectSoundCapture_fnAddRef(
    LPDIRECTSOUNDCAPTURE8 iface)
{
    ULONG ref;
    LPCDirectSoundCaptureImpl This = (LPCDirectSoundCaptureImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureImpl, lpVtbl);

    /* increment reference count */
    ref = InterlockedIncrement(&This->ref);

    return ref;
}

ULONG
WINAPI
CDirectSoundCapture_fnRelease(
    LPDIRECTSOUNDCAPTURE8 iface)
{
    ULONG ref;
    LPCDirectSoundCaptureImpl This = (LPCDirectSoundCaptureImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureImpl, lpVtbl);

    /* release reference count */
    ref = InterlockedDecrement(&(This->ref));

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}


HRESULT
WINAPI
CDirectSoundCapture_fnCreateCaptureBuffer(
    LPDIRECTSOUNDCAPTURE8 iface,
    LPCDSCBUFFERDESC lpcDSBufferDesc,
    LPDIRECTSOUNDCAPTUREBUFFER *ppDSCBuffer,
    LPUNKNOWN pUnkOuter)
{
    HRESULT hResult;
    LPCDirectSoundCaptureImpl This = (LPCDirectSoundCaptureImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureImpl, lpVtbl);

    if (!This->bInitialized)
    {
        /* object not yet initialized */
        return DSERR_UNINITIALIZED;
    }

    if (!lpcDSBufferDesc  || !ppDSCBuffer || pUnkOuter != NULL)
    {
        /* invalid param */
        return DSERR_INVALIDPARAM;
    }

    /* check buffer description */
    if ((lpcDSBufferDesc->dwSize != sizeof(DSCBUFFERDESC) && lpcDSBufferDesc->dwSize != sizeof(DSCBUFFERDESC1)) ||
        lpcDSBufferDesc->dwReserved != 0 || lpcDSBufferDesc->dwBufferBytes == 0 || lpcDSBufferDesc->lpwfxFormat == NULL)
    {
        /* invalid buffer description */
        return DSERR_INVALIDPARAM;
    }

    DPRINT("This %p wFormatTag %x nChannels %u nSamplesPerSec %u nAvgBytesPerSec %u NBlockAlign %u wBitsPerSample %u cbSize %u\n",
           This, lpcDSBufferDesc->lpwfxFormat->wFormatTag, lpcDSBufferDesc->lpwfxFormat->nChannels, lpcDSBufferDesc->lpwfxFormat->nSamplesPerSec, lpcDSBufferDesc->lpwfxFormat->nAvgBytesPerSec, lpcDSBufferDesc->lpwfxFormat->nBlockAlign, lpcDSBufferDesc->lpwfxFormat->wBitsPerSample, lpcDSBufferDesc->lpwfxFormat->cbSize);

    hResult = NewDirectSoundCaptureBuffer((LPDIRECTSOUNDCAPTUREBUFFER8*)ppDSCBuffer, This->Filter, lpcDSBufferDesc);
    return hResult;
}


HRESULT
WINAPI
CDirectSoundCapture_fnGetCaps(
    LPDIRECTSOUNDCAPTURE8 iface,
    LPDSCCAPS pDSCCaps)
{
    WAVEINCAPSW Caps;
    MMRESULT Result;
    LPCDirectSoundCaptureImpl This = (LPCDirectSoundCaptureImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureImpl, lpVtbl);

    if (!This->bInitialized)
    {
        /* object not yet initialized */
        return DSERR_UNINITIALIZED;
    }

    if (!pDSCCaps)
    {
        /* invalid param */
        return DSERR_INVALIDPARAM;
    }

    if (pDSCCaps->dwSize != sizeof(DSCCAPS))
    {
        /* invalid param */
        return DSERR_INVALIDPARAM;
    }


    /* We are certified ;) */
    pDSCCaps->dwFlags = DSCCAPS_CERTIFIED;

    ASSERT(This->Filter);

    Result = waveInGetDevCapsW(This->Filter->MappedId[0], &Caps, sizeof(WAVEINCAPSW));
    if (Result != MMSYSERR_NOERROR)
    {
        /* failed */
        DPRINT("waveInGetDevCapsW for device %u failed with %x\n", This->Filter->MappedId[0], Result);
        return DSERR_UNSUPPORTED;
    }

    pDSCCaps->dwFormats = Caps.dwFormats;
    pDSCCaps->dwChannels = Caps.wChannels;

    return DS_OK;
}


HRESULT
WINAPI
CDirectSoundCapture_fnInitialize(
    LPDIRECTSOUNDCAPTURE8 iface,
    LPCGUID pcGuidDevice)
{
    GUID DeviceGuid;
    LPOLESTR pGuidStr;
    LPCDirectSoundCaptureImpl This = (LPCDirectSoundCaptureImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureImpl, lpVtbl);

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
        pcGuidDevice = &DSDEVID_DefaultCapture;
    }

    if (IsEqualIID(pcGuidDevice, &DSDEVID_DefaultVoicePlayback) || IsEqualIID(pcGuidDevice, &DSDEVID_DefaultPlayback))
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

    if (FindDeviceByGuid(&DeviceGuid, &This->Filter))
    {
        This->bInitialized = TRUE;
        return DS_OK;
    }

    DPRINT("Failed to find device\n");
    return DSERR_INVALIDPARAM;
}

static IDirectSoundCaptureVtbl vt_DirectSoundCapture =
{
    /* IUnknown methods */
    CDirectSoundCapture_fnQueryInterface,
    CDirectSoundCapture_fnAddRef,
    CDirectSoundCapture_fnRelease,
    CDirectSoundCapture_fnCreateCaptureBuffer,
    CDirectSoundCapture_fnGetCaps,
    CDirectSoundCapture_fnInitialize
};

HRESULT
InternalDirectSoundCaptureCreate(
    LPCGUID lpcGUID,
    LPDIRECTSOUNDCAPTURE8 *ppDS,
    IUnknown *pUnkOuter)
{
    LPCDirectSoundCaptureImpl This;
    HRESULT hr;

    if (!ppDS || pUnkOuter != NULL)
    {
        /* invalid parameter passed */
        return DSERR_INVALIDPARAM;
    }

    /* allocate CDirectSoundCaptureImpl struct */
    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CDirectSoundCaptureImpl));
    if (!This)
    {
        /* not enough memory */
        return DSERR_OUTOFMEMORY;
    }

    /* initialize IDirectSoundCapture object */
    This->ref = 1;
    This->lpVtbl = &vt_DirectSoundCapture;


    /* initialize direct sound interface */
    hr = IDirectSoundCapture_Initialize((LPDIRECTSOUNDCAPTURE8)&This->lpVtbl, lpcGUID);

    /* check for success */
    if (!SUCCEEDED(hr))
    {
        /* failed */
        DPRINT("Failed to initialize DirectSoundCapture object with %x\n", hr);
        IDirectSoundCapture_Release((LPDIRECTSOUND8)&This->lpVtbl);
        return hr;
    }

    /* store result */
    *ppDS = (LPDIRECTSOUNDCAPTURE8)&This->lpVtbl;
    DPRINT("DirectSoundCapture object %p\n", *ppDS);
    return DS_OK;
}

HRESULT
CALLBACK
NewDirectSoundCapture(
    IUnknown* pUnkOuter,
    REFIID riid,
    LPVOID* ppvObject)
{
    LPOLESTR pStr;
    LPCDirectSoundCaptureImpl This;

    /* check requested interface */
    if (!IsEqualIID(riid, &IID_IUnknown) && !IsEqualIID(riid, &IID_IDirectSoundCapture) && !IsEqualIID(riid, &IID_IDirectSoundCapture8))
    {
        *ppvObject = 0;
        StringFromIID(riid, &pStr);
        DPRINT("NewDirectSoundCapture does not support Interface %ws\n", pStr);
        CoTaskMemFree(pStr);
        return E_NOINTERFACE;
    }

    /* allocate CDirectSoundCaptureImpl struct */
    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CDirectSoundCaptureImpl));
    if (!This)
    {
        /* not enough memory */
        return DSERR_OUTOFMEMORY;
    }

    /* initialize object */
    This->ref = 1;
    This->lpVtbl = &vt_DirectSoundCapture;
    This->bInitialized = FALSE;
    *ppvObject = (LPVOID)&This->lpVtbl;

    return S_OK;
}


HRESULT
WINAPI
DirectSoundCaptureCreate(
    LPCGUID lpcGUID,
    LPDIRECTSOUNDCAPTURE *ppDSC,
    LPUNKNOWN pUnkOuter)
{
    return InternalDirectSoundCaptureCreate(lpcGUID, (LPDIRECTSOUNDCAPTURE8*)ppDSC, pUnkOuter);
}

HRESULT
WINAPI
DirectSoundCaptureCreate8(
    LPCGUID lpcGUID,
    LPDIRECTSOUNDCAPTURE8 *ppDSC8,
    LPUNKNOWN pUnkOuter)
{
    return InternalDirectSoundCaptureCreate(lpcGUID, ppDSC8, pUnkOuter);
}
