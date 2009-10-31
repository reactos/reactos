/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/capturebuffer.c
 * PURPOSE:         IDirectSoundCaptureBuffer8 implementation
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */


#include "precomp.h"

const GUID KSINTERFACESETID_Standard            = {0x1A8766A0L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSMEDIUMSETID_Standard               = {0x4747B320L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSDATAFORMAT_TYPE_AUDIO              = {0x73647561L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SPECIFIER_WAVEFORMATEX  = {0x05589f81L, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};
const GUID KSPROPSETID_Connection              = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};


typedef struct
{
    IDirectSoundCaptureBuffer8Vtbl *lpVtbl;
    LONG ref;
    LPFILTERINFO Filter;
    HANDLE hPin;
    PUCHAR Buffer;
    DWORD BufferSize;
    LPWAVEFORMATEX Format;
    KSSTATE State;


}CDirectSoundCaptureBufferImpl, *LPCDirectSoundCaptureBufferImpl;

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_QueryInterface(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    IN REFIID riid,
    LPVOID* ppobj)
{
    LPOLESTR pStr;
    LPCDirectSoundCaptureBufferImpl This = (LPCDirectSoundCaptureBufferImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureBufferImpl, lpVtbl);

    /* check if requested interface is supported */
    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectSoundCaptureBuffer) ||
        IsEqualIID(riid, &IID_IDirectSoundCaptureBuffer8))
    {
        *ppobj = (LPVOID)&This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return S_OK;
    }

    /* interface not supported */
    if (SUCCEEDED(StringFromIID(riid, &pStr)))
    {
        DPRINT("No Interface for class %s\n", pStr);
        CoTaskMemFree(pStr);
    }
    return E_NOINTERFACE;
}

ULONG
WINAPI
IDirectSoundCaptureBufferImpl_AddRef(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface)
{
    ULONG ref;
    LPCDirectSoundCaptureBufferImpl This = (LPCDirectSoundCaptureBufferImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureBufferImpl, lpVtbl);

    /* increment reference count */
    ref = InterlockedIncrement(&This->ref);

    return ref;

}

ULONG
WINAPI
IDirectSoundCaptureBufferImpl_Release(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface)
{
    ULONG ref;
    LPCDirectSoundCaptureBufferImpl This = (LPCDirectSoundCaptureBufferImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureBufferImpl, lpVtbl);

    /* release reference count */
    ref = InterlockedDecrement(&(This->ref));

    if (!ref)
    {
        /* free capture buffer */
        HeapFree(GetProcessHeap(), 0, This->Buffer);
        HeapFree(GetProcessHeap(), 0, This->Format);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}


HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_GetCaps(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDSCBCAPS lpDSCBCaps )
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_GetCurrentPosition(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDWORD lpdwCapturePosition,
    LPDWORD lpdwReadPosition)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}


HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_GetFormat(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPWAVEFORMATEX lpwfxFormat,
    DWORD dwSizeAllocated,
    LPDWORD lpdwSizeWritten)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_GetStatus(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDWORD lpdwStatus )
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_Initialize(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDIRECTSOUNDCAPTURE lpDSC,
    LPCDSCBUFFERDESC lpcDSCBDesc)
{
    /* capture buffer is already initialized */
    return DSERR_ALREADYINITIALIZED;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_Lock(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwReadCusor,
    DWORD dwReadBytes,
    LPVOID* lplpvAudioPtr1,
    LPDWORD lpdwAudioBytes1,
    LPVOID* lplpvAudioPtr2,
    LPDWORD lpdwAudioBytes2,
    DWORD dwFlags )
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_Start(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwFlags )
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_Stop( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_Unlock(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPVOID lpvAudioPtr1,
    DWORD dwAudioBytes1,
    LPVOID lpvAudioPtr2,
    DWORD dwAudioBytes2 )
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_GetObjectInPath(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    REFGUID rguidObject,
    DWORD dwIndex,
    REFGUID rguidInterface,
    LPVOID* ppObject )
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_GetFXStatus(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwFXCount,
    LPDWORD pdwFXStatus )
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}


static IDirectSoundCaptureBuffer8Vtbl vt_DirectSoundCaptureBuffer8 =
{
    /* IUnknown methods */
    IDirectSoundCaptureBufferImpl_QueryInterface,
    IDirectSoundCaptureBufferImpl_AddRef,
    IDirectSoundCaptureBufferImpl_Release,

    /* IDirectSoundCaptureBuffer methods */
    IDirectSoundCaptureBufferImpl_GetCaps,
    IDirectSoundCaptureBufferImpl_GetCurrentPosition,
    IDirectSoundCaptureBufferImpl_GetFormat,
    IDirectSoundCaptureBufferImpl_GetStatus,
    IDirectSoundCaptureBufferImpl_Initialize,
    IDirectSoundCaptureBufferImpl_Lock,
    IDirectSoundCaptureBufferImpl_Start,
    IDirectSoundCaptureBufferImpl_Stop,
    IDirectSoundCaptureBufferImpl_Unlock,

    /* IDirectSoundCaptureBuffer methods */
    IDirectSoundCaptureBufferImpl_GetObjectInPath,
    IDirectSoundCaptureBufferImpl_GetFXStatus
};


HRESULT
NewDirectSoundCaptureBuffer(
    LPDIRECTSOUNDCAPTUREBUFFER8 *OutBuffer,
    LPFILTERINFO Filter,
    LPCDSCBUFFERDESC lpcDSBufferDesc)
{
    DWORD FormatSize;
    ULONG DeviceId = 0, PinId;
    DWORD Result = ERROR_SUCCESS;

    LPCDirectSoundCaptureBufferImpl This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CDirectSoundCaptureBufferImpl));

    if (!This)
    {
        /* not enough memory */
        return DSERR_OUTOFMEMORY;
    }

     /* calculate format size */
    FormatSize = sizeof(WAVEFORMATEX) + lpcDSBufferDesc->lpwfxFormat->cbSize;
    /* allocate format struct */
    This->Format = HeapAlloc(GetProcessHeap(), 0, FormatSize);
    if (!This->Format)
    {
        /* not enough memory */
        HeapFree(GetProcessHeap(), 0, This);
        return DSERR_OUTOFMEMORY;
    }

    /* sanity check */
    ASSERT(lpcDSBufferDesc->dwBufferBytes);

    /* allocate capture buffer */
    This->Buffer = HeapAlloc(GetProcessHeap(), 0, lpcDSBufferDesc->dwBufferBytes);
    if (!This->Buffer)
    {
        /* not enough memory */
        HeapFree(GetProcessHeap(), 0, This->Format);
        HeapFree(GetProcessHeap(), 0, This);
        return DSERR_OUTOFMEMORY;
    }

    /* store buffer size */
    This->BufferSize = lpcDSBufferDesc->dwBufferBytes;
    ASSERT(lpcDSBufferDesc->lpwfxFormat->cbSize == 0);

    do
    {
        /* try all available recording pins on that filter */
        PinId = GetPinIdFromFilter(Filter, TRUE, DeviceId);
        DPRINT("PinId %u DeviceId %u\n", PinId, DeviceId);

        if (PinId == ULONG_MAX)
            break;

        Result = OpenPin(Filter->hFilter, PinId, lpcDSBufferDesc->lpwfxFormat, &This->hPin, TRUE);
        if (Result == ERROR_SUCCESS)
            break;

        DeviceId++;
    }while(TRUE);

    if (Result != ERROR_SUCCESS)
    {
        /* failed to instantiate the capture pin */
        HeapFree(GetProcessHeap(), 0, This->Buffer);
        HeapFree(GetProcessHeap(), 0, This->Format);
        HeapFree(GetProcessHeap(), 0, This);
        return DSERR_OUTOFMEMORY;
    }

    /* initialize capture buffer */
    This->ref = 1;
    This->lpVtbl = &vt_DirectSoundCaptureBuffer8;
    This->Filter = Filter;
    This->State = KSSTATE_STOP;

    *OutBuffer = (LPDIRECTSOUNDCAPTUREBUFFER8)&This->lpVtbl;
    return DS_OK;
}
