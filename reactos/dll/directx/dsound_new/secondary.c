/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/secondary.c
 * PURPOSE:         Secondary IDirectSoundBuffer8 implementation
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */


#include "precomp.h"

typedef struct
{
    const IDirectSoundBuffer8Vtbl *lpVtbl;
    LONG ref;

    LPFILTERINFO Filter;
    DWORD dwLevel;
    DWORD dwFlags;
    LPWAVEFORMATEX Format;
    PUCHAR Buffer;
    DWORD BufferSize;
    KSSTATE State;
    DWORD Flags;
    DWORD Position;
    DWORD PlayPosition;

    LPDIRECTSOUNDBUFFER8 PrimaryBuffer;


}CDirectSoundBuffer, *LPCDirectSoundBuffer;

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnQueryInterface(
    LPDIRECTSOUNDBUFFER8 iface,
    IN REFIID riid,
    LPVOID* ppobj)
{
    LPOLESTR pStr;
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IDirectSoundBuffer) ||
        IsEqualIID(riid, &IID_IDirectSoundBuffer8))
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
SecondaryDirectSoundBuffer8Impl_fnAddRef(
    LPDIRECTSOUNDBUFFER8 iface)
{
    ULONG ref;
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    ref = InterlockedIncrement(&This->ref);

    return ref;

}

ULONG
WINAPI
SecondaryDirectSoundBuffer8Impl_fnRelease(
    LPDIRECTSOUNDBUFFER8 iface)
{
    ULONG ref;
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    ref = InterlockedDecrement(&(This->ref));

    if (!ref)
    {
        HeapFree(GetProcessHeap(), 0, This->Buffer);
        HeapFree(GetProcessHeap(), 0, This->Format);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnGetCaps(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDSBCAPS pDSBufferCaps)
{
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    if (!pDSBufferCaps)
    {
        /* invalid parameter */
        return DSERR_INVALIDPARAM;
    }

    if (pDSBufferCaps->dwSize < sizeof(DSBCAPS))
    {
        /* invalid buffer size */
        return DSERR_INVALIDPARAM;
    }

    /* get buffer details */
    pDSBufferCaps->dwUnlockTransferRate = 0;
    pDSBufferCaps->dwPlayCpuOverhead = 0;
    pDSBufferCaps->dwSize = This->BufferSize;
    pDSBufferCaps->dwFlags = This->dwFlags;

    return DS_OK;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnGetCurrentPosition(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDWORD pdwCurrentPlayCursor,
    LPDWORD pdwCurrentWriteCursor)
{
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    //DPRINT("SecondaryDirectSoundBuffer8Impl_fnGetCurrentPosition This %p Play %p Write %p\n", This, pdwCurrentPlayCursor, pdwCurrentWriteCursor);

    return PrimaryDirectSoundBuffer_GetPosition(This->PrimaryBuffer, pdwCurrentPlayCursor, pdwCurrentWriteCursor);
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnGetFormat(
    LPDIRECTSOUNDBUFFER8 iface,
    LPWAVEFORMATEX pwfxFormat, 
    DWORD dwSizeAllocated, 
    LPDWORD pdwSizeWritten)
{
    DWORD FormatSize;
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    FormatSize = sizeof(WAVEFORMATEX) + This->Format->cbSize;

    if (!pwfxFormat && !pdwSizeWritten)
    {
        /* invalid parameter */
        return DSERR_INVALIDPARAM;
    }

    if (!pwfxFormat)
    {
        /* return required format size */
        *pdwSizeWritten = FormatSize;
        return DS_OK;
    }
    else
    {
        if (dwSizeAllocated >= FormatSize)
        {
            /* copy format */
            CopyMemory(pwfxFormat, This->Format, FormatSize);

            if (pdwSizeWritten)
                *pdwSizeWritten = FormatSize;

            return DS_OK;
        }
        /* buffer too small */
        if (pdwSizeWritten)
            *pdwSizeWritten = 0;

        return DSERR_INVALIDPARAM;
    }
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnGetVolume(
    LPDIRECTSOUNDBUFFER8 iface,
    LPLONG plVolume)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnGetPan(
    LPDIRECTSOUNDBUFFER8 iface,
    LPLONG plPan)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnGetFrequency(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDWORD pdwFrequency)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnGetStatus(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDWORD pdwStatus)
{
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    if (!pdwStatus)
    {
        /* invalid parameter */
        return DSERR_INVALIDPARAM;
    }

    *pdwStatus = 0;
    if (This->State == KSSTATE_RUN || This->State == KSSTATE_ACQUIRE)
    {
        /* buffer is playing */
        *pdwStatus |= DSBSTATUS_PLAYING;
        if (This->Flags & DSBPLAY_LOOPING)
            *pdwStatus |= DSBSTATUS_LOOPING;
    }

    return DS_OK;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnInitialize(
    LPDIRECTSOUNDBUFFER8 iface,
    LPDIRECTSOUND pDirectSound,
    LPCDSBUFFERDESC pcDSBufferDesc)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnLock(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwOffset,
    DWORD dwBytes,
    LPVOID *ppvAudioPtr1,
    LPDWORD pdwAudioBytes1,
    LPVOID *ppvAudioPtr2, 
    LPDWORD pdwAudioBytes2,
    DWORD dwFlags)
{
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    DPRINT("This %p dwOffset %u dwBytes %u ppvAudioPtr1 %p pdwAudioBytes1 %p ppvAudioPtr2 %p pdwAudioBytes2 %p dwFlags %x This->BufferSize %u\n",
           This, dwOffset, dwBytes, ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2, dwFlags, This->BufferSize);

    if (dwFlags == DSBLOCK_ENTIREBUFFER)
    {
        *ppvAudioPtr1 = (LPVOID)This->Buffer;
        *pdwAudioBytes1 = This->BufferSize;
        if (ppvAudioPtr2)
            *ppvAudioPtr2 = NULL;
        if (pdwAudioBytes2)
            *pdwAudioBytes2 = 0;

        return DS_OK;
    }
    else if (dwFlags == DSBLOCK_FROMWRITECURSOR)
    {
        UNIMPLEMENTED
        return DSERR_UNSUPPORTED;
    }
    else
    {
        ASSERT(dwOffset < This->BufferSize);
        ASSERT(dwBytes <= This->BufferSize);

        dwBytes = min(This->BufferSize - dwOffset, dwBytes);

        *ppvAudioPtr1 = This->Buffer + dwOffset;
        *pdwAudioBytes1 = dwBytes;
        if (ppvAudioPtr2)
            *ppvAudioPtr2 = NULL;
        if (pdwAudioBytes2)
            *pdwAudioBytes2 = 0;

        return DS_OK;
    }
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnPlay(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwReserved1,
    DWORD dwPriority,
    DWORD dwFlags)
{
    HRESULT hResult;
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    if (dwReserved1 != 0)
    {
        /* must be zero */
        return DSERR_INVALIDPARAM;
    }

    /* sanity check */
    ASSERT(dwFlags & DSBPLAY_LOOPING);

    /* set dataformat */
    hResult = PrimaryDirectSoundBuffer_SetFormat(This->PrimaryBuffer, This->Format, TRUE);

    if (!SUCCEEDED(hResult))
    {
        /* failed */
        DPRINT1("Failed to set format Tag %u Samples %u Bytes %u nChannels %u\n", This->Format->wFormatTag, This->Format->nSamplesPerSec, This->Format->wBitsPerSample, This->Format->nChannels);
        return hResult;
    }

    /* start primary buffer */
    PrimaryDirectSoundBuffer_SetState(This->PrimaryBuffer, KSSTATE_RUN);
    /* acquire primary buffer */
    PrimaryDirectSoundBuffer_AcquireLock(This->PrimaryBuffer);
    /* HACK write buffer */
    PrimaryDirectSoundBuffer_Write(This->PrimaryBuffer, This->Buffer, This->BufferSize);
    /* release primary buffer */
    PrimaryDirectSoundBuffer_ReleaseLock(This->PrimaryBuffer);

    DPRINT("SetFormatSuccess PrimaryBuffer %p\n", This->PrimaryBuffer);
    return DS_OK;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnSetCurrentPosition(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwNewPosition)
{
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    DPRINT("Setting position %u\n", dwNewPosition);
    This->Position = dwNewPosition;

    return DS_OK;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnSetFormat(
    LPDIRECTSOUNDBUFFER8 iface,
    LPCWAVEFORMATEX pcfxFormat)
{
    return DSERR_INVALIDCALL;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnSetVolume(
    LPDIRECTSOUNDBUFFER8 iface,
    LONG lVolume)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnSetPan(
    LPDIRECTSOUNDBUFFER8 iface,
    LONG lPan)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnSetFrequency(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwFrequency)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnStop(
    LPDIRECTSOUNDBUFFER8 iface)
{
    LPCDirectSoundBuffer This = (LPCDirectSoundBuffer)CONTAINING_RECORD(iface, CDirectSoundBuffer, lpVtbl);

    PrimaryDirectSoundBuffer_SetState(This->PrimaryBuffer, KSSTATE_PAUSE);
    PrimaryDirectSoundBuffer_SetState(This->PrimaryBuffer, KSSTATE_ACQUIRE);
    PrimaryDirectSoundBuffer_SetState(This->PrimaryBuffer, KSSTATE_STOP);

    return DS_OK;
}


HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnUnlock(
    LPDIRECTSOUNDBUFFER8 iface,
    LPVOID pvAudioPtr1,
    DWORD dwAudioBytes1,
    LPVOID pvAudioPtr2,
    DWORD dwAudioBytes2)
{
    //DPRINT("SecondaryDirectSoundBuffer8Impl_fnUnlock pvAudioPtr1 %p dwAudioBytes1 %u pvAudioPtr2 %p dwAudioBytes2 %u Unimplemented\n");
    return DS_OK;
}




HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnRestore(
    LPDIRECTSOUNDBUFFER8 iface)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}


HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnSetFX(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwEffectsCount, 
    LPDSEFFECTDESC pDSFXDesc,
    LPDWORD pdwResultCodes)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnAcquireResources(
    LPDIRECTSOUNDBUFFER8 iface,
    DWORD dwFlags,
    DWORD dwEffectsCount, 
    LPDWORD pdwResultCodes)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
SecondaryDirectSoundBuffer8Impl_fnGetObjectInPath(
    LPDIRECTSOUNDBUFFER8 iface,
    REFGUID rguidObject,
    DWORD dwIndex,
    REFGUID rguidInterface,
    LPVOID *ppObject)
{
    UNIMPLEMENTED
    return DSERR_INVALIDPARAM;
}

static IDirectSoundBuffer8Vtbl vt_DirectSoundBuffer8 =
{
    /* IUnknown methods */
    SecondaryDirectSoundBuffer8Impl_fnQueryInterface,
    SecondaryDirectSoundBuffer8Impl_fnAddRef,
    SecondaryDirectSoundBuffer8Impl_fnRelease,
    /* IDirectSoundBuffer methods */
    SecondaryDirectSoundBuffer8Impl_fnGetCaps,
    SecondaryDirectSoundBuffer8Impl_fnGetCurrentPosition,
    SecondaryDirectSoundBuffer8Impl_fnGetFormat,
    SecondaryDirectSoundBuffer8Impl_fnGetVolume,
    SecondaryDirectSoundBuffer8Impl_fnGetPan,
    SecondaryDirectSoundBuffer8Impl_fnGetFrequency,
    SecondaryDirectSoundBuffer8Impl_fnGetStatus,
    SecondaryDirectSoundBuffer8Impl_fnInitialize,
    SecondaryDirectSoundBuffer8Impl_fnLock,
    SecondaryDirectSoundBuffer8Impl_fnPlay,
    SecondaryDirectSoundBuffer8Impl_fnSetCurrentPosition,
    SecondaryDirectSoundBuffer8Impl_fnSetFormat,
    SecondaryDirectSoundBuffer8Impl_fnSetVolume,
    SecondaryDirectSoundBuffer8Impl_fnSetPan,
    SecondaryDirectSoundBuffer8Impl_fnSetFrequency,
    SecondaryDirectSoundBuffer8Impl_fnStop,
    SecondaryDirectSoundBuffer8Impl_fnUnlock,
    SecondaryDirectSoundBuffer8Impl_fnRestore,
    /* IDirectSoundBuffer8 methods */
    SecondaryDirectSoundBuffer8Impl_fnSetFX,
    SecondaryDirectSoundBuffer8Impl_fnAcquireResources,
    SecondaryDirectSoundBuffer8Impl_fnGetObjectInPath
};

HRESULT
NewSecondarySoundBuffer(
    LPDIRECTSOUNDBUFFER8 *OutBuffer,
    LPFILTERINFO Filter,
    DWORD dwLevel,
    LPCDSBUFFERDESC lpcDSBufferDesc,
    LPDIRECTSOUNDBUFFER8 PrimaryBuffer)
{
    ULONG FormatSize;
    LPCDirectSoundBuffer This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CDirectSoundBuffer));

    if (!This)
    {
        /* not enough memory */
        return DSERR_OUTOFMEMORY;
    }

    FormatSize = sizeof(WAVEFORMATEX) + lpcDSBufferDesc->lpwfxFormat->cbSize;

    This->Format = HeapAlloc(GetProcessHeap(), 0, FormatSize);
    if (!This->Format)
    {
        /* not enough memory */
        HeapFree(GetProcessHeap(), 0, This);
        return DSERR_OUTOFMEMORY;
    }

    /* sanity check */
    ASSERT(lpcDSBufferDesc->dwBufferBytes);

    /* allocate sound buffer */
    This->Buffer = HeapAlloc(GetProcessHeap(), 0, lpcDSBufferDesc->dwBufferBytes);
    if (!This->Buffer)
    {
        /* not enough memory */
        HeapFree(GetProcessHeap(), 0, This->Format);
        HeapFree(GetProcessHeap(), 0, This);
        return DSERR_OUTOFMEMORY;
    }


    This->ref = 1;
    This->lpVtbl = &vt_DirectSoundBuffer8;
    This->Filter = Filter;
    This->dwLevel = dwLevel;
    This->dwFlags = lpcDSBufferDesc->dwFlags;
    This->State = KSSTATE_STOP;
    This->Flags = 0;
    This->Position = 0;
    This->BufferSize = lpcDSBufferDesc->dwBufferBytes;
    This->PrimaryBuffer = PrimaryBuffer;

    CopyMemory(This->Format, lpcDSBufferDesc->lpwfxFormat, FormatSize);

    *OutBuffer = (LPDIRECTSOUNDBUFFER8)&This->lpVtbl;
    return DS_OK;
}

