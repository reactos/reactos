/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/stubs.c
 * PURPOSE:         DSound stubs
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

HRESULT
WINAPI
DirectSoundCreate(
    LPCGUID lpcGUID,
    LPDIRECTSOUND *ppDS,
    IUnknown *pUnkOuter)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}


HRESULT
WINAPI
DirectSoundEnumerateA(
    LPDSENUMCALLBACKA lpDSEnumCallback,
    LPVOID lpContext)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
DirectSoundEnumerateW(
    LPDSENUMCALLBACKW lpDSEnumCallback,
    LPVOID lpContext )
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
DllRegisterServer(void)
{
    UNIMPLEMENTED
    return SELFREG_E_CLASS;
}

HRESULT
WINAPI
DllUnregisterServer(void)
{
    UNIMPLEMENTED
    return SELFREG_E_CLASS;
}

HRESULT
WINAPI
DirectSoundCaptureCreate(
    LPCGUID lpcGUID,
    LPDIRECTSOUNDCAPTURE *ppDSC,
    LPUNKNOWN pUnkOuter)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
DirectSoundCaptureCreate8(
    LPCGUID lpcGUID,
    LPDIRECTSOUNDCAPTURE8 *ppDSC8,
    LPUNKNOWN pUnkOuter)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
DirectSoundCreate8(
    LPCGUID lpcGUID,
    LPDIRECTSOUND8 *ppDS,
    IUnknown *pUnkOuter)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
DirectSoundFullDuplexCreate(
    LPCGUID pcGuidCaptureDevice,
    LPCGUID pcGuidRenderDevice,
    LPCDSCBUFFERDESC pcDSCBufferDesc,
    LPCDSBUFFERDESC pcDSBufferDesc,
    HWND hWnd,
    DWORD dwLevel,
    LPDIRECTSOUNDFULLDUPLEX *ppDSFD,
    LPDIRECTSOUNDCAPTUREBUFFER8 *ppDSCBuffer8,
    LPDIRECTSOUNDBUFFER8 *ppDSBuffer8,
    LPUNKNOWN pUnkOuter)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
DirectSoundCaptureEnumerateA(
    LPDSENUMCALLBACKA lpDSEnumCallback,
    LPVOID lpContext)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
DirectSoundCaptureEnumerateW(
    LPDSENUMCALLBACKW lpDSEnumCallback,
    LPVOID lpContext)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
GetDeviceID(LPCGUID pGuidSrc, LPGUID pGuidDest)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}
