/*
 * PROJECT:     ReactOS Sound System
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wdmaud.drv/mmixer.c
 *
 * PURPOSE:     WDM Audio Mixer API (User-mode part)
 * PROGRAMMERS: Johannes Anderwald
 */

#include "wdmaud.h"

#include <winreg.h>
#include <setupapi.h>
#include <mmixer.h>
#define NTOS_MODE_USER
#include <ndk/rtlfuncs.h>
#include <ndk/iofuncs.h>

#define NDEBUG
#include <debug.h>
#include <mmebuddy_debug.h>


BOOL MMixerLibraryInitialized = FALSE;



PVOID Alloc(ULONG NumBytes);
MIXER_STATUS Close(HANDLE hDevice);
VOID Free(PVOID Block);
VOID Copy(PVOID Src, PVOID Dst, ULONG NumBytes);
MIXER_STATUS Open(IN LPWSTR DevicePath, OUT PHANDLE hDevice);
MIXER_STATUS Control(IN HANDLE hMixer, IN ULONG dwIoControlCode, IN PVOID lpInBuffer, IN ULONG nInBufferSize, OUT PVOID lpOutBuffer, ULONG nOutBufferSize, PULONG lpBytesReturned);
MIXER_STATUS Enum(IN  PVOID EnumContext, IN  ULONG DeviceIndex, OUT LPWSTR * DeviceName, OUT PHANDLE OutHandle, OUT PHANDLE OutKey);
MIXER_STATUS OpenKey(IN HANDLE hKey, IN LPWSTR SubKey, IN ULONG DesiredAccess, OUT PHANDLE OutKey);
MIXER_STATUS CloseKey(IN HANDLE hKey);
MIXER_STATUS QueryKeyValue(IN HANDLE hKey, IN LPWSTR KeyName, OUT PVOID * ResultBuffer, OUT PULONG ResultLength, OUT PULONG KeyType);
PVOID AllocEventData(IN ULONG ExtraSize);
VOID FreeEventData(IN PVOID EventData);

MIXER_CONTEXT MixerContext =
{
    sizeof(MIXER_CONTEXT),
    NULL,
    Alloc,
    Control,
    Free,
    Open,
    Close,
    Copy,
    OpenKey,
    QueryKeyValue,
    CloseKey,
    AllocEventData,
    FreeEventData
};

GUID CategoryGuid = {STATIC_KSCATEGORY_AUDIO};

MIXER_STATUS
QueryKeyValue(
    IN HANDLE hKey,
    IN LPWSTR KeyName,
    OUT PVOID * ResultBuffer,
    OUT PULONG ResultLength,
    OUT PULONG KeyType)
{
    if (RegQueryValueExW((HKEY)hKey, KeyName, NULL, KeyType, NULL, ResultLength) == ERROR_FILE_NOT_FOUND)
        return MM_STATUS_UNSUCCESSFUL;

    *ResultBuffer = HeapAlloc(GetProcessHeap(), 0, *ResultLength);
    if (*ResultBuffer == NULL)
        return MM_STATUS_NO_MEMORY;

    if (RegQueryValueExW((HKEY)hKey, KeyName, NULL, KeyType, *ResultBuffer, ResultLength) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, *ResultBuffer);
        return MM_STATUS_UNSUCCESSFUL;
    }
    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
OpenKey(
    IN HANDLE hKey,
    IN LPWSTR SubKey,
    IN ULONG DesiredAccess,
    OUT PHANDLE OutKey)
{
    if (RegOpenKeyExW((HKEY)hKey, SubKey, 0, DesiredAccess, (PHKEY)OutKey) == ERROR_SUCCESS)
        return MM_STATUS_SUCCESS;

    return MM_STATUS_UNSUCCESSFUL;
}

MIXER_STATUS
CloseKey(
    IN HANDLE hKey)
{
    RegCloseKey((HKEY)hKey);
    return MM_STATUS_SUCCESS;
}


PVOID Alloc(ULONG NumBytes)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NumBytes);
}

MIXER_STATUS
Close(HANDLE hDevice)
{
    if (CloseHandle(hDevice))
        return MM_STATUS_SUCCESS;
    else
        return MM_STATUS_UNSUCCESSFUL;
}

VOID
Free(PVOID Block)
{
    HeapFree(GetProcessHeap(), 0, Block);
}

VOID
Copy(PVOID Src, PVOID Dst, ULONG NumBytes)
{
    RtlMoveMemory(Src, Dst, NumBytes);
}

MIXER_STATUS
Open(
    IN LPWSTR DevicePath,
    OUT PHANDLE hDevice)
{
     DevicePath[1] = L'\\';
    *hDevice = CreateFileW(DevicePath,
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_FLAG_OVERLAPPED,
                           NULL);
    if (*hDevice == INVALID_HANDLE_VALUE)
    {
        return MM_STATUS_UNSUCCESSFUL;
    }

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
Control(
    IN HANDLE hMixer,
    IN ULONG dwIoControlCode,
    IN PVOID lpInBuffer,
    IN ULONG nInBufferSize,
    OUT PVOID lpOutBuffer,
    ULONG nOutBufferSize,
    PULONG lpBytesReturned)
{
    OVERLAPPED Overlapped;
    BOOLEAN IoResult;
    DWORD Transferred = 0;

    /* Overlapped I/O is done here - this is used for waiting for completion */
    ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! Overlapped.hEvent )
        return MM_STATUS_NO_MEMORY;

    /* Talk to the device */
    IoResult = DeviceIoControl(hMixer,
                               dwIoControlCode,
                               lpInBuffer,
                               nInBufferSize,
                               lpOutBuffer,
                               nOutBufferSize,
                               &Transferred,
                               &Overlapped);

    /* If failure occurs, make sure it's not just due to the overlapped I/O */
    if ( ! IoResult )
    {
        if ( GetLastError() != ERROR_IO_PENDING )
        {
            CloseHandle(Overlapped.hEvent);

            if (GetLastError() == ERROR_MORE_DATA || GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                if ( lpBytesReturned )
                    *lpBytesReturned = Transferred;
                return MM_STATUS_MORE_ENTRIES;
            }

            return MM_STATUS_UNSUCCESSFUL;
        }
    }

    /* Wait for the I/O to complete */
    IoResult = GetOverlappedResult(hMixer,
                                   &Overlapped,
                                   &Transferred,
                                   TRUE);

    /* Don't need this any more */
    CloseHandle(Overlapped.hEvent);

    if ( ! IoResult )
        return MM_STATUS_UNSUCCESSFUL;

    if ( lpBytesReturned )
        *lpBytesReturned = Transferred;

    return MM_STATUS_SUCCESS;
}

MIXER_STATUS
Enum(
    IN  PVOID EnumContext,
    IN  ULONG DeviceIndex,
    OUT LPWSTR * DeviceName,
    OUT PHANDLE OutHandle,
    OUT PHANDLE OutKey)
{
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    SP_DEVINFO_DATA DeviceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DetailData;
    BOOL Result;
    DWORD Length;
    MIXER_STATUS Status;

    //printf("Enum EnumContext %p DeviceIndex %lu OutHandle %p\n", EnumContext, DeviceIndex, OutHandle);

    InterfaceData.cbSize = sizeof(InterfaceData);
    InterfaceData.Reserved = 0;

    Result = SetupDiEnumDeviceInterfaces(EnumContext,
                                NULL,
                                &CategoryGuid,
                                DeviceIndex,
                                &InterfaceData);

    if (!Result)
    {
        if (GetLastError() == ERROR_NO_MORE_ITEMS)
        {
            return MM_STATUS_NO_MORE_DEVICES;
        }
        return MM_STATUS_UNSUCCESSFUL;
    }

    Length = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof(WCHAR);
    DetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)HeapAlloc(GetProcessHeap(),
                                                             0,
                                                             Length);
    DetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    DeviceData.cbSize = sizeof(DeviceData);
    DeviceData.Reserved = 0;

    Result = SetupDiGetDeviceInterfaceDetailW(EnumContext,
                                    &InterfaceData,
                                    DetailData,
                                    Length,
                                    NULL,
                                    &DeviceData);

    if (!Result)
    {
        DPRINT("SetupDiGetDeviceInterfaceDetailW failed with %lu\n", GetLastError());
        return MM_STATUS_UNSUCCESSFUL;
    }


    *OutKey = SetupDiOpenDeviceInterfaceRegKey(EnumContext, &InterfaceData, 0, KEY_READ);
     if ((HKEY)*OutKey == INVALID_HANDLE_VALUE)
     {
        HeapFree(GetProcessHeap(), 0, DetailData);
        return MM_STATUS_UNSUCCESSFUL;
    }

    Status = Open(DetailData->DevicePath, OutHandle);

    if (Status != MM_STATUS_SUCCESS)
    {
        RegCloseKey((HKEY)*OutKey);
        HeapFree(GetProcessHeap(), 0, DetailData);
        return Status;
    }

    *DeviceName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (wcslen(DetailData->DevicePath)+1) * sizeof(WCHAR));
    if (*DeviceName == NULL)
    {
        CloseHandle(*OutHandle);
        RegCloseKey((HKEY)*OutKey);
        HeapFree(GetProcessHeap(), 0, DetailData);
        return MM_STATUS_NO_MEMORY;
    }
    wcscpy(*DeviceName, DetailData->DevicePath);
    HeapFree(GetProcessHeap(), 0, DetailData);

    return Status;
}

PVOID
AllocEventData(
    IN ULONG ExtraSize)
{
    PKSEVENTDATA Data = (PKSEVENTDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(KSEVENTDATA) + ExtraSize);
    if (!Data)
        return NULL;

    Data->EventHandle.Event = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!Data->EventHandle.Event)
    {
        HeapFree(GetProcessHeap(), 0, Data);
        return NULL;
    }

    Data->NotificationType = KSEVENTF_EVENT_HANDLE;
    return Data;
}

VOID
FreeEventData(IN PVOID EventData)
{
    PKSEVENTDATA Data = (PKSEVENTDATA)EventData;

    CloseHandle(Data->EventHandle.Event);
    HeapFree(GetProcessHeap(), 0, Data);
}


BOOL
WdmAudInitUserModeMixer()
{
    HDEVINFO DeviceHandle;
    MIXER_STATUS Status;

    if (MMixerLibraryInitialized)
    {
        /* library is already initialized */
        return TRUE;
    }


    /* create a device list */
    DeviceHandle = SetupDiGetClassDevs(&CategoryGuid,
                                       NULL,
                                       NULL,
                                       DIGCF_DEVICEINTERFACE/* FIXME |DIGCF_PRESENT*/);

    if (DeviceHandle == INVALID_HANDLE_VALUE)
    {
        /* failed to create a device list */
        return FALSE;
    }


    /* initialize the mixer library */
    Status = MMixerInitialize(&MixerContext, Enum, (PVOID)DeviceHandle);

    /* free device list */
    SetupDiDestroyDeviceInfoList(DeviceHandle);

    if (Status != MM_STATUS_SUCCESS)
    {
        /* failed to initialize mixer library */
        DPRINT1("Failed to initialize mixer library with %x\n", Status);
        return FALSE;
    }

    /* library is now initialized */
    MMixerLibraryInitialized = TRUE;

    /* completed successfully */
    return TRUE;
}

MMRESULT
WdmAudCleanupByMMixer()
{
    /* TODO */
    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudGetMixerCapabilities(
    IN ULONG DeviceId,
    LPMIXERCAPSW Capabilities)
{
    if (MMixerGetCapabilities(&MixerContext, DeviceId, Capabilities) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_BADDEVICEID;
}

MMRESULT
WdmAudGetLineInfo(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERLINEW MixLine,
    IN ULONG Flags)
{
    if (MMixerGetLineInfo(&MixerContext, hMixer, MixerId, Flags, MixLine)  == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudGetLineControls(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERLINECONTROLSW MixControls,
    IN ULONG Flags)
{
    if (MMixerGetLineControls(&MixerContext, hMixer, MixerId, Flags, MixControls) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudSetControlDetails(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERCONTROLDETAILS MixDetails,
    IN ULONG Flags)
{
    if (MMixerSetControlDetails(&MixerContext, hMixer, MixerId, Flags, MixDetails) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;

}

MMRESULT
WdmAudGetControlDetails(
    IN HANDLE hMixer,
    IN DWORD MixerId,
    IN LPMIXERCONTROLDETAILS MixDetails,
    IN ULONG Flags)
{
    if (MMixerGetControlDetails(&MixerContext, hMixer, MixerId, Flags, MixDetails) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudGetWaveOutCapabilities(
    IN ULONG DeviceId,
    LPWAVEOUTCAPSW Capabilities)
{
    if (MMixerWaveOutCapabilities(&MixerContext, DeviceId, Capabilities) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;

}

MMRESULT
WdmAudGetWaveInCapabilities(
    IN ULONG DeviceId,
    LPWAVEINCAPSW Capabilities)
{
    if (MMixerWaveInCapabilities(&MixerContext, DeviceId, Capabilities) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudSetWaveDeviceFormatByMMixer(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD DeviceId,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    MMDEVICE_TYPE DeviceType;
    PSOUND_DEVICE SoundDevice;
    MMRESULT Result;
    BOOL bWaveIn;

    Result = GetSoundDeviceFromInstance(Instance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    bWaveIn = (DeviceType == WAVE_IN_DEVICE_TYPE ? TRUE : FALSE);

    if (MMixerOpenWave(&MixerContext, DeviceId, bWaveIn, WaveFormat, NULL, NULL, &Instance->Handle) == MM_STATUS_SUCCESS)
    {
        if (DeviceType == WAVE_OUT_DEVICE_TYPE)
        {
            MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_ACQUIRE);
            MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_PAUSE);
            MMixerSetWaveStatus(&MixerContext, Instance->Handle, KSSTATE_RUN);
        }
        return MMSYSERR_NOERROR;
    }
    return MMSYSERR_ERROR;
}


MMRESULT
WdmAudGetCapabilitiesByMMixer(
    IN  PSOUND_DEVICE SoundDevice,
    IN  DWORD DeviceId,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize)
{
    MMDEVICE_TYPE DeviceType;
    MMRESULT Result;

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if (DeviceType == MIXER_DEVICE_TYPE)
    {
        return WdmAudGetMixerCapabilities(DeviceId, (LPMIXERCAPSW)Capabilities);
    }
    else if (DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        return WdmAudGetWaveOutCapabilities(DeviceId, (LPWAVEOUTCAPSW)Capabilities);
    }
    else if (DeviceType == WAVE_IN_DEVICE_TYPE)
    {
        return WdmAudGetWaveInCapabilities(DeviceId, (LPWAVEINCAPSW)Capabilities);
    }
    else
    {
        // not supported
        return MMSYSERR_ERROR;
    }
}

MMRESULT
WdmAudOpenSoundDeviceByMMixer(
    IN  struct _SOUND_DEVICE* SoundDevice,
    OUT PVOID* Handle)
{
    if (WdmAudInitUserModeMixer())
        return MMSYSERR_NOERROR;
    else
        return MMSYSERR_ERROR;
}

MMRESULT
WdmAudCloseSoundDeviceByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  PVOID Handle)
{
    MMDEVICE_TYPE DeviceType;
    PSOUND_DEVICE SoundDevice;
    MMRESULT Result;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if (DeviceType == MIXER_DEVICE_TYPE)
    {
        /* no op */
        return MMSYSERR_NOERROR;
    }
    else if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        /* make sure the pin is stopped */
        MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_PAUSE);
        MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_ACQUIRE);
        MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_STOP);

        CloseHandle(Handle);
        return MMSYSERR_NOERROR;
    }

    /* midi is not supported */
    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudGetNumWdmDevsByMMixer(
    IN  MMDEVICE_TYPE DeviceType,
    OUT DWORD* DeviceCount)
{
    switch(DeviceType)
    {
        case MIXER_DEVICE_TYPE:
            *DeviceCount = MMixerGetCount(&MixerContext);
            break;
        case WAVE_OUT_DEVICE_TYPE:
            *DeviceCount = MMixerGetWaveOutCount(&MixerContext);
            break;
        case WAVE_IN_DEVICE_TYPE:
            *DeviceCount = MMixerGetWaveInCount(&MixerContext);
            break;
        default:
            *DeviceCount = 0;
    }
    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudQueryMixerInfoByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN DWORD MixerId,
    IN UINT uMsg,
    IN LPVOID Parameter,
    IN DWORD Flags)
{
    LPMIXERLINEW MixLine;
    LPMIXERLINECONTROLSW MixControls;
    LPMIXERCONTROLDETAILS MixDetails;
    HANDLE hMixer = NULL;

    MixLine = (LPMIXERLINEW)Parameter;
    MixControls = (LPMIXERLINECONTROLSW)Parameter;
    MixDetails = (LPMIXERCONTROLDETAILS)Parameter;

    /* FIXME param checks */

    if (SoundDeviceInstance)
    {
        hMixer = SoundDeviceInstance->Handle;
    }

    switch(uMsg)
    {
        case MXDM_GETLINEINFO:
            return WdmAudGetLineInfo(hMixer, MixerId, MixLine, Flags);
        case MXDM_GETLINECONTROLS:
            return WdmAudGetLineControls(hMixer, MixerId, MixControls, Flags);
       case MXDM_SETCONTROLDETAILS:
            return WdmAudSetControlDetails(hMixer, MixerId, MixDetails, Flags);
       case MXDM_GETCONTROLDETAILS:
            return WdmAudGetControlDetails(hMixer, MixerId, MixDetails, Flags);
       default:
           DPRINT1("MixerId %lu, uMsg %lu, Parameter %p, Flags %lu\n", MixerId, uMsg, Parameter, Flags);
           SND_ASSERT(0);
           return MMSYSERR_NOTSUPPORTED;
    }
}

MMRESULT
WdmAudGetDeviceInterfaceStringByMMixer(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  LPWSTR Interface,
    IN  DWORD  InterfaceLength,
    OUT  DWORD * InterfaceSize)
{
    /* FIXME */
    return MMSYSERR_NOTSUPPORTED;
}

VOID
CALLBACK
MixerEventCallback(
    IN PVOID MixerEventContext,
    IN HANDLE hMixer,
    IN ULONG NotificationType,
    IN ULONG Value)
{
    PSOUND_DEVICE_INSTANCE Instance = (PSOUND_DEVICE_INSTANCE)MixerEventContext;

    DriverCallback(Instance->WinMM.ClientCallback,
                   HIWORD(Instance->WinMM.Flags),
                   Instance->WinMM.Handle,
                   NotificationType,
                   Instance->WinMM.ClientCallbackInstanceData,
                   (DWORD_PTR)Value,
                   0);
}

MMRESULT
WdmAudSetMixerDeviceFormatByMMixer(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD DeviceId,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    if (MMixerOpen(&MixerContext, DeviceId, (PVOID)Instance, MixerEventCallback, &Instance->Handle) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_BADDEVICEID;
}

MMRESULT
WdmAudSetWaveStateByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN BOOL bStart)
{
    MMDEVICE_TYPE DeviceType;
    PSOUND_DEVICE SoundDevice;
    MMRESULT Result;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);
    SND_ASSERT( Result == MMSYSERR_NOERROR );


    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        if (bStart)
        {
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_ACQUIRE);
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_PAUSE);
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_RUN);
        }
        else
        {
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_PAUSE);
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_ACQUIRE);
            MMixerSetWaveStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_STOP);
        }
    }
    else if (DeviceType == MIDI_IN_DEVICE_TYPE || DeviceType == MIDI_OUT_DEVICE_TYPE)
    {
        if (bStart)
        {
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_ACQUIRE);
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_PAUSE);
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_RUN);
        }
        else
        {
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_PAUSE);
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_ACQUIRE);
            MMixerSetMidiStatus(&MixerContext, SoundDeviceInstance->Handle, KSSTATE_STOP);
        }
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudResetStreamByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  MMDEVICE_TYPE DeviceType,
    IN  BOOLEAN bStartReset)
{
    MIXER_STATUS Status;

    if (DeviceType == WAVE_IN_DEVICE_TYPE || DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        Status = MMixerSetWaveResetState(&MixerContext, SoundDeviceInstance->Handle, bStartReset);
        if (Status == MM_STATUS_SUCCESS)
        {
            /* completed successfully */
            return MMSYSERR_NOERROR;
        }
    }


    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT
WdmAudGetWavePositionByMMixer(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  MMTIME* Time)
{
    /* FIXME */
    return MMSYSERR_NOTSUPPORTED;
}

static
VOID WINAPI
CommitWaveBufferApc(PVOID ApcContext,
           PIO_STATUS_BLOCK IoStatusBlock,
           ULONG Reserved)
{
    DWORD dwErrorCode;
    PSOUND_OVERLAPPED Overlap;
    KSSTREAM_HEADER* lpHeader;

    dwErrorCode = RtlNtStatusToDosError(IoStatusBlock->Status);
    Overlap = (PSOUND_OVERLAPPED)IoStatusBlock;
    lpHeader = Overlap->CompletionContext;

    /* Call mmebuddy overlap routine */
    Overlap->OriginalCompletionRoutine(dwErrorCode,
        lpHeader->DataUsed, &Overlap->Standard);

    HeapFree(GetProcessHeap(), 0, lpHeader);
}

MMRESULT
WdmAudCommitWaveBufferByMMixer(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID OffsetPtr,
    IN  DWORD Length,
    IN  PSOUND_OVERLAPPED Overlap,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine)
{
    PSOUND_DEVICE SoundDevice;
    MMDEVICE_TYPE DeviceType;
    MMRESULT Result;
    ULONG IoCtl;
    KSSTREAM_HEADER* lpHeader;
    NTSTATUS Status;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    lpHeader = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(KSSTREAM_HEADER));
    if ( ! lpHeader )
    {
        /* no memory */
        return MMSYSERR_NOMEM;
    }

    /* setup stream packet */
    lpHeader->Size = sizeof(KSSTREAM_HEADER);
    lpHeader->PresentationTime.Numerator = 1;
    lpHeader->PresentationTime.Denominator = 1;
    lpHeader->Data = OffsetPtr;
    lpHeader->FrameExtent = Length;
    Overlap->CompletionContext = lpHeader;
    Overlap->OriginalCompletionRoutine = CompletionRoutine;
    IoCtl = (DeviceType == WAVE_OUT_DEVICE_TYPE ? IOCTL_KS_WRITE_STREAM : IOCTL_KS_READ_STREAM);

    if (DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        lpHeader->DataUsed = Length;
    }

    Status = NtDeviceIoControlFile(SoundDeviceInstance->Handle,
                                   NULL,
                                   CommitWaveBufferApc,
                                   NULL,
                                   (PIO_STATUS_BLOCK)Overlap,
                                   IoCtl,
                                   NULL,
                                   0,
                                   lpHeader,
                                   sizeof(KSSTREAM_HEADER));

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDeviceIoControlFile() failed with status %08lx\n", Status);
        return MMSYSERR_ERROR;
    }

    return MMSYSERR_NOERROR;
}
