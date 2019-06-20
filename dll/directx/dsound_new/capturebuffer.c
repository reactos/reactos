/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/capturebuffer.c
 * PURPOSE:         IDirectSoundCaptureBuffer8 implementation
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */


#include "precomp.h"

const GUID KSINTERFACESETID_Standard            = {0x1A8766A0L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSMEDIUMSETID_Standard               = {0x4747B320L, 0x62CE, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSDATAFORMAT_TYPE_AUDIO              = {0x73647561L, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
const GUID KSDATAFORMAT_SPECIFIER_WAVEFORMATEX  = {0x05589f81L, 0xc356, 0x11ce, {0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a}};
const GUID KSPROPSETID_Connection              = {0x1D58C920L, 0xAC9B, 0x11CF, {0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, 0x00, 0x00}};
const GUID KSEVENTSETID_LoopedStreaming        = {0x4682B940L, 0xC6EF, 0x11D0, {0x96, 0xD8, 0x00, 0xAA, 0x00, 0x51, 0xE5, 0x1D}};



typedef struct
{
    IDirectSoundCaptureBuffer8Vtbl *lpVtbl;

    LONG ref;
    LPFILTERINFO Filter;
    HANDLE hPin;
    PUCHAR Buffer;
    DWORD BufferSize;
    LPWAVEFORMATEX Format;
    WAVEFORMATEX MixFormat;
    BOOL bMix;
    BOOL bLoop;
    KSSTATE State;
    PUCHAR MixBuffer;
    ULONG MixBufferSize;
    HANDLE hStopEvent;
    volatile LONG StopMixerThread;
    volatile LONG CurrentMixPosition;

    LPDIRECTSOUNDNOTIFY Notify;

}CDirectSoundCaptureBufferImpl, *LPCDirectSoundCaptureBufferImpl;

DWORD
WINAPI
MixerThreadRoutine(
    LPVOID lpParameter)
{
    KSPROPERTY Request;
    KSAUDIO_POSITION Position;
    DWORD Result, MixPosition, BufferPosition, BytesWritten, BytesRead, MixLength, BufferLength;
    LPCDirectSoundCaptureBufferImpl This = (LPCDirectSoundCaptureBufferImpl)lpParameter;

    /* setup audio position property request */
    Request.Id = KSPROPERTY_AUDIO_POSITION;
    Request.Set = KSPROPSETID_Audio;
    Request.Flags = KSPROPERTY_TYPE_GET;

    MixPosition = 0;
    BufferPosition = 0;
    do
    {
        /* query current position */
        Result = SyncOverlappedDeviceIoControl(This->hPin, IOCTL_KS_PROPERTY, (PVOID)&Request, sizeof(KSPROPERTY), (PVOID)&Position, sizeof(KSAUDIO_POSITION), NULL);

        /* sanity check */
        ASSERT(Result == ERROR_SUCCESS);

        /* FIXME implement samplerate conversion */
        ASSERT(This->MixFormat.nSamplesPerSec == This->Format->nSamplesPerSec);

        /* FIXME implement bitrate conversion */
        ASSERT(This->MixFormat.wBitsPerSample == This->Format->wBitsPerSample);

        /* sanity check */
        ASSERT(BufferPosition <= This->BufferSize);
        ASSERT(MixPosition  <= This->MixBufferSize);

        if (BufferPosition == This->BufferSize)
        {
            /* restart from front */
            BufferPosition = 0;
        }

        if (MixPosition == This->MixBufferSize)
        {
            /* restart from front */
            MixPosition = 0;
        }

        if (This->MixFormat.nChannels != This->Format->nChannels)
        {
            if ((DWORD)Position.PlayOffset >= MixPosition)
            {
                /* calculate buffer position difference */
                MixLength = Position.PlayOffset - MixPosition;
            }
            else
            {
                /* buffer overlap */
                MixLength = This->MixBufferSize - MixPosition;
            }

            BufferLength = This->BufferSize - BufferPosition;

            /* convert the format */
            PerformChannelConversion(&This->MixBuffer[MixPosition], MixLength, &BytesRead, This->MixFormat.nChannels, This->Format->nChannels, This->Format->wBitsPerSample, &This->Buffer[BufferPosition], BufferLength, &BytesWritten);

            /* update buffer offsets */
            MixPosition += BytesRead;
            BufferPosition += BytesWritten;
            DPRINT("MixPosition %u BufferPosition %u BytesRead %u BytesWritten %u MixLength %u BufferLength %u\n", MixPosition, BufferPosition, BytesRead, BytesWritten, MixLength, BufferLength);
        }

        /* Notify Events */
        if (This->Notify)
        {
            DoNotifyPositionEvents(This->Notify, This->CurrentMixPosition, BufferPosition);
        }

        /* update offset */
        InterlockedExchange(&This->CurrentMixPosition, (LONG)BufferPosition);

        /* FIXME use timer */
        Sleep(10);

    }while(InterlockedCompareExchange(&This->StopMixerThread, 0, 0) == 0);


    /* signal stop event */
    SetEvent(This->hStopEvent);

    /* done */
    return 0;
}



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

    /* check if the interface is supported */
    if (IsEqualIID(riid, &IID_IDirectSoundNotify))
    {
        if (!This->Notify)
        {
            HRESULT hr = NewDirectSoundNotify(&This->Notify, This->bLoop, This->bMix, This->hPin, This->BufferSize);
            if (FAILED(hr))
                return hr;

            *ppobj = (LPVOID)This->Notify;
            return S_OK;
        }

        /* increment reference count on existing notify object */
        IDirectSoundNotify_AddRef(This->Notify);
        *ppobj = (LPVOID)This->Notify;
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
        if (This->hPin)
        {
            /* close pin handle */
            CloseHandle(This->hPin);
        }

        if (This->hStopEvent)
        {
            /* close stop event handle */
            CloseHandle(This->hStopEvent);
        }

        if (This->MixBuffer)
        {
            /* free mix buffer */
            HeapFree(GetProcessHeap(), 0, This->MixBuffer);
        }

        /* free capture buffer */
        HeapFree(GetProcessHeap(), 0, This->Buffer);
        /* free wave format */
        HeapFree(GetProcessHeap(), 0, This->Format);
        /* free capture buffer */
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
    LPCDirectSoundCaptureBufferImpl This = (LPCDirectSoundCaptureBufferImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureBufferImpl, lpVtbl);

    if (!lpDSCBCaps)
    {
        /* invalid parameter */
        return DSERR_INVALIDPARAM;
    }

    if (lpDSCBCaps->dwSize != sizeof(DSCBCAPS))
    {
        /* invalid parameter */
        return DSERR_INVALIDPARAM;
    }

    lpDSCBCaps->dwBufferBytes = This->BufferSize;
    lpDSCBCaps->dwReserved = 0;
    //lpDSCBCaps->dwFlags =  DSCBCAPS_WAVEMAPPED;

    return DS_OK;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_GetCurrentPosition(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDWORD lpdwCapturePosition,
    LPDWORD lpdwReadPosition)
{
    KSAUDIO_POSITION Position;
    KSPROPERTY Request;
    DWORD Result;
    DWORD Value;

    LPCDirectSoundCaptureBufferImpl This = (LPCDirectSoundCaptureBufferImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureBufferImpl, lpVtbl);

    if (!This->hPin)
    {
        if (lpdwCapturePosition)
            *lpdwCapturePosition = 0;

        if (lpdwReadPosition)
            *lpdwReadPosition = 0;

        DPRINT("No Audio Pin\n");
        return DS_OK;
    }

    if (This->bMix)
    {
        /* read current position */
        Value = InterlockedCompareExchange(&This->CurrentMixPosition, 0, 0);

        if (lpdwCapturePosition)
            *lpdwCapturePosition = (DWORD)Value;

        if (lpdwReadPosition)
            *lpdwReadPosition = (DWORD)Value;

        return DS_OK;
    }

    /* setup audio position property request */
    Request.Id = KSPROPERTY_AUDIO_POSITION;
    Request.Set = KSPROPSETID_Audio;
    Request.Flags = KSPROPERTY_TYPE_GET;


    Result = SyncOverlappedDeviceIoControl(This->hPin, IOCTL_KS_PROPERTY, (PVOID)&Request, sizeof(KSPROPERTY), (PVOID)&Position, sizeof(KSAUDIO_POSITION), NULL);

    if (Result != ERROR_SUCCESS)
    {
        DPRINT("GetPosition failed with %x\n", Result);
        return DSERR_UNSUPPORTED;
    }

    //DPRINT("Play %I64u Write %I64u \n", Position.PlayOffset, Position.WriteOffset);

    if (lpdwCapturePosition)
        *lpdwCapturePosition = (DWORD)Position.PlayOffset;

    if (lpdwReadPosition)
        *lpdwReadPosition = (DWORD)Position.WriteOffset;

    return DS_OK;
}


HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_GetFormat(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPWAVEFORMATEX lpwfxFormat,
    DWORD dwSizeAllocated,
    LPDWORD lpdwSizeWritten)
{
    DWORD FormatSize;
    LPCDirectSoundCaptureBufferImpl This = (LPCDirectSoundCaptureBufferImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureBufferImpl, lpVtbl);

    FormatSize = sizeof(WAVEFORMATEX) + This->Format->cbSize;

    if (!lpwfxFormat && !lpdwSizeWritten)
    {
        /* invalid parameter */
        return DSERR_INVALIDPARAM;
    }

    if (!lpwfxFormat)
    {
        /* return required format size */
        *lpdwSizeWritten = FormatSize;
        return DS_OK;
    }
    else
    {
        if (dwSizeAllocated >= FormatSize)
        {
            /* copy format */
            CopyMemory(lpwfxFormat, This->Format, FormatSize);

            if (lpdwSizeWritten)
                *lpdwSizeWritten = FormatSize;

            return DS_OK;
        }
        /* buffer too small */
        if (lpdwSizeWritten)
            *lpdwSizeWritten = 0;
        return DSERR_INVALIDPARAM;
    }
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_GetStatus(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    LPDWORD lpdwStatus )
{
    LPCDirectSoundCaptureBufferImpl This = (LPCDirectSoundCaptureBufferImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureBufferImpl, lpVtbl);

    if (!lpdwStatus)
    {
        /* invalid parameter */
        return DSERR_INVALIDPARAM;
    }

    /* reset flags */
    *lpdwStatus = 0;

    /* check if pin is running */
    if (This->State == KSSTATE_RUN)
        *lpdwStatus |= DSCBSTATUS_CAPTURING;

    /* check if a looped buffer is used */
    if (This->bLoop)
        *lpdwStatus |= DSCBSTATUS_LOOPING;

    /* done */
    return DS_OK;
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
    DWORD dwOffset,
    DWORD dwBytes,
    LPVOID* ppvAudioPtr1,
    LPDWORD pdwAudioBytes1,
    LPVOID* ppvAudioPtr2,
    LPDWORD pdwAudioBytes2,
    DWORD dwFlags )
{
    LPCDirectSoundCaptureBufferImpl This = (LPCDirectSoundCaptureBufferImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureBufferImpl, lpVtbl);

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
    else
    {
        ASSERT(dwOffset < This->BufferSize);
        ASSERT(dwBytes < This->BufferSize);
        ASSERT(dwBytes + dwOffset <= This->BufferSize);

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
IDirectSoundCaptureBufferImpl_Start(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwFlags )
{
    KSPROPERTY Property;
    KSSTREAM_HEADER Header;
    DWORD Result, BytesTransferred;
    OVERLAPPED Overlapped;
    KSSTATE State;
    HANDLE hThread;

    LPCDirectSoundCaptureBufferImpl This = (LPCDirectSoundCaptureBufferImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureBufferImpl, lpVtbl);

    DPRINT("IDirectSoundCaptureBufferImpl_Start Flags %x\n", dwFlags);
    ASSERT(dwFlags == DSCBSTART_LOOPING);

    /* check if pin is already running */
    if (This->State == KSSTATE_RUN)
        return DS_OK;


    /* check if there is a pin instance */
    if (!This->hPin)
        return DSERR_GENERIC;

    /* setup request */
    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;
    State = KSSTATE_RUN;

    /* set pin to run */
    Result = SyncOverlappedDeviceIoControl(This->hPin, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&State, sizeof(KSSTATE), &BytesTransferred);

    ASSERT(Result == ERROR_SUCCESS);

    if (Result == ERROR_SUCCESS)
    {
        /* store result */
        This->State = State;
    }

    /* initialize overlapped struct */
    ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    /* clear stream header */
    ZeroMemory(&Header, sizeof(KSSTREAM_HEADER));

    /* initialize stream header */
    Header.FrameExtent = This->BufferSize;
    Header.DataUsed = 0;
    Header.Data = (This->bMix ? This->MixBuffer : This->Buffer);
    Header.Size = sizeof(KSSTREAM_HEADER);
    Header.PresentationTime.Numerator = 1;
    Header.PresentationTime.Denominator = 1;

    Result = DeviceIoControl(This->hPin, IOCTL_KS_WRITE_STREAM, NULL, 0, &Header, sizeof(KSSTREAM_HEADER), &BytesTransferred, &Overlapped);

    if (Result != ERROR_SUCCESS)
    {
        DPRINT("Failed submit buffer with %lx\n", Result);
        return DSERR_GENERIC;
    }

    if (This->bMix)
    {
        if (!This->hStopEvent)
        {
            /* create stop event */
            This->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            if (!This->hStopEvent)
            {
                DPRINT1("Failed to create event object with %x\n", GetLastError());
                return DSERR_GENERIC;
            }
        }

        /* set state to stop false */
        This->StopMixerThread = FALSE;

        hThread = CreateThread(NULL, 0, MixerThreadRoutine, (PVOID)This, 0, NULL);
        if (!hThread)
        {
            DPRINT1("Failed to create thread with %x\n", GetLastError());
            return DSERR_GENERIC;
        }

        /* close thread handle */
        CloseHandle(hThread);
    }


    return DS_OK;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_Stop( LPDIRECTSOUNDCAPTUREBUFFER8 iface )
{
    KSPROPERTY Property;
    DWORD Result;
    KSSTATE State;

    LPCDirectSoundCaptureBufferImpl This = (LPCDirectSoundCaptureBufferImpl)CONTAINING_RECORD(iface, CDirectSoundCaptureBufferImpl, lpVtbl);

    if (This->State == KSSTATE_STOP)
    {
        /* stream has already been stopped */
        return DS_OK;
    }

    if (!This->hPin)
        return DSERR_GENERIC;

    /* setup request */
    Property.Set = KSPROPSETID_Connection;
    Property.Id = KSPROPERTY_CONNECTION_STATE;
    Property.Flags = KSPROPERTY_TYPE_SET;
    State = KSSTATE_STOP;


    /* set pin to stop */
    Result = SyncOverlappedDeviceIoControl(This->hPin, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)&State, sizeof(KSSTATE), NULL);

    ASSERT(Result == ERROR_SUCCESS);


    if (This->bMix)
    {
        /* sanity check */
        ASSERT(This->hStopEvent);
        /* reset event */
        ResetEvent(This->hStopEvent);
        /* signal event to stop */
        This->StopMixerThread = TRUE;
        /* Wait for the event to stop */
        WaitForSingleObject(This->hStopEvent, INFINITE);
    }


    if (Result == ERROR_SUCCESS)
    {
        /* store result */
        This->State = State;
        return DS_OK;
    }

    DPRINT("Failed to stop pin\n");
    return DSERR_GENERIC;
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
    return DS_OK;
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
    UNIMPLEMENTED;
    return DSERR_INVALIDPARAM;
}

HRESULT
WINAPI
IDirectSoundCaptureBufferImpl_GetFXStatus(
    LPDIRECTSOUNDCAPTUREBUFFER8 iface,
    DWORD dwFXCount,
    LPDWORD pdwFXStatus )
{
    UNIMPLEMENTED;
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
    DWORD FormatSize, MixBufferSize;
    ULONG DeviceId = 0, PinId;
    DWORD Result = ERROR_SUCCESS;
    WAVEFORMATEX MixFormat;

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

        if (PinId == ULONG_MAX)
            break;

        Result = OpenPin(Filter->hFilter, PinId, lpcDSBufferDesc->lpwfxFormat, &This->hPin, TRUE);
        if (Result == ERROR_SUCCESS)
            break;

        DeviceId++;
    }while(TRUE);

    if (Result != ERROR_SUCCESS)
    {
        /* failed to instantiate the capture pin with the native format
         * try to compute a compatible format and use that
         * we could use the mixer api for this purpose but... the kmixer isnt working very good atm
         */

       DeviceId = 0;
       do
       {
           /* try all available recording pins on that filter */
            PinId = GetPinIdFromFilter(Filter, TRUE, DeviceId);
            DPRINT("PinId %u DeviceId %u\n", PinId, DeviceId);

            if (PinId == ULONG_MAX)
                break;

            if (CreateCompatiblePin(Filter->hFilter, PinId, TRUE, lpcDSBufferDesc->lpwfxFormat, &MixFormat, &This->hPin))
            {
                This->bMix = TRUE;
                CopyMemory(&This->MixFormat, &MixFormat, sizeof(WAVEFORMATEX));
                break;
            }

            DeviceId++;
        }while(TRUE);


        if (!This->bMix)
        {
            /* FIXME should not happen */
            DPRINT("failed to compute a compatible format\n");
            HeapFree(GetProcessHeap(), 0, This->MixBuffer);
            HeapFree(GetProcessHeap(), 0, This->Buffer);
            HeapFree(GetProcessHeap(), 0, This->Format);
            HeapFree(GetProcessHeap(), 0, This);
            return DSERR_GENERIC;
        }

        MixBufferSize = lpcDSBufferDesc->dwBufferBytes;
        MixBufferSize /= lpcDSBufferDesc->lpwfxFormat->nChannels;
        MixBufferSize /= (lpcDSBufferDesc->lpwfxFormat->wBitsPerSample/8);

        MixBufferSize *= This->MixFormat.nChannels;
        MixBufferSize *= (This->MixFormat.wBitsPerSample/8);

        /* allocate buffer for mixing */
        This->MixBuffer = HeapAlloc(GetProcessHeap(), 0, MixBufferSize);
        if (!This->Buffer)
        {
            /* not enough memory */
            CloseHandle(This->hPin);
            HeapFree(GetProcessHeap(), 0, This->Buffer);
            HeapFree(GetProcessHeap(), 0, This->Format);
            HeapFree(GetProcessHeap(), 0, This);
            return DSERR_OUTOFMEMORY;
        }
        This->MixBufferSize = MixBufferSize;
        DPRINT1("MixBufferSize %u BufferSize %u\n", MixBufferSize, This->BufferSize);
    }

    /* initialize capture buffer */
    This->ref = 1;
    This->lpVtbl = &vt_DirectSoundCaptureBuffer8;
    This->Filter = Filter;
    This->State = KSSTATE_STOP;
    This->bLoop = TRUE;

    RtlMoveMemory(This->Format, lpcDSBufferDesc->lpwfxFormat, FormatSize);

    *OutBuffer = (LPDIRECTSOUNDCAPTUREBUFFER8)&This->lpVtbl;
    return DS_OK;
}
