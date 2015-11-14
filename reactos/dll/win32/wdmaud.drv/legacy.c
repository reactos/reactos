/*
 * PROJECT:     ReactOS Sound System
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wdmaud.drv/wdmaud.c
 *
 * PURPOSE:     WDM Audio Driver (User-mode part)
 * PROGRAMMERS: Andrew Greenwood (silverblade@reactos.org)
                Johannes Anderwald
 *
 * NOTES:       Looking for wodMessage & co? You won't find them here. Try
 *              the MME Buddy library, which is where these routines are
 *              actually implemented.
 *
 */

#define NDEBUG
#include "wdmaud.h"
#include <debug.h>

#define KERNEL_DEVICE_NAME      L"\\\\.\\wdmaud"

HANDLE KernelHandle = INVALID_HANDLE_VALUE;
DWORD OpenCount = 0;

DWORD
WINAPI
MixerEventThreadRoutine(
    LPVOID Parameter)
{
    HANDLE WaitObjects[2];
    DWORD dwResult;
    MMRESULT Result;
    WDMAUD_DEVICE_INFO DeviceInfo;
    PSOUND_DEVICE_INSTANCE Instance = (PSOUND_DEVICE_INSTANCE)Parameter;

    /* setup wait objects */
    WaitObjects[0] = Instance->hNotifyEvent;
    WaitObjects[1] = Instance->hStopEvent;

    /* zero device info */
    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));

    DeviceInfo.hDevice = Instance->Handle;
    DeviceInfo.DeviceType = MIXER_DEVICE_TYPE;

    do
    {
        dwResult = WaitForMultipleObjects(2, WaitObjects, FALSE, INFINITE);

        if (dwResult == WAIT_OBJECT_0 + 1)
        {
            /* stop event was signalled */
            break;
        }

        do
        {
            Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                                   IOCTL_GET_MIXER_EVENT,
                                                   (LPVOID) &DeviceInfo,
                                                   sizeof(WDMAUD_DEVICE_INFO),
                                                   (LPVOID) &DeviceInfo,
                                                   sizeof(WDMAUD_DEVICE_INFO),
                                                   NULL);

            if (Result == MMSYSERR_NOERROR)
            {
                DriverCallback(Instance->WinMM.ClientCallback,
                               HIWORD(Instance->WinMM.Flags),
                               Instance->WinMM.Handle,
                               DeviceInfo.u.MixerEvent.NotificationType,
                               Instance->WinMM.ClientCallbackInstanceData,
                               (DWORD_PTR)DeviceInfo.u.MixerEvent.Value,
                               0);
            }
        }while(Result == MMSYSERR_NOERROR);
    }while(TRUE);

    /* done */
    return 0;
}

MMRESULT
WdmAudCleanupByLegacy()
{
    if (KernelHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(KernelHandle);
        KernelHandle = INVALID_HANDLE_VALUE;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudGetNumWdmDevsByLegacy(
    IN  MMDEVICE_TYPE DeviceType,
    OUT DWORD* DeviceCount)
{
    MMRESULT Result;
    WDMAUD_DEVICE_INFO DeviceInfo;

    VALIDATE_MMSYS_PARAMETER( KernelHandle != INVALID_HANDLE_VALUE );
    VALIDATE_MMSYS_PARAMETER( IS_VALID_SOUND_DEVICE_TYPE(DeviceType) );
    VALIDATE_MMSYS_PARAMETER( DeviceCount );

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.DeviceType = DeviceType;

    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_GETNUMDEVS_TYPE,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);

    if ( ! MMSUCCESS( Result ) )
    {
        SND_ERR(L"Call to IOCTL_GETNUMDEVS_TYPE failed\n");
        *DeviceCount = 0;
        return TranslateInternalMmResult(Result);
    }

    *DeviceCount = DeviceInfo.DeviceCount;

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudGetCapabilitiesByLegacy(
    IN  PSOUND_DEVICE SoundDevice,
    IN  DWORD DeviceId,
    OUT PVOID Capabilities,
    IN  DWORD CapabilitiesSize)
{
    MMRESULT Result;
    MMDEVICE_TYPE DeviceType;
    WDMAUD_DEVICE_INFO DeviceInfo;

    SND_ASSERT( SoundDevice );
    SND_ASSERT( Capabilities );

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if ( ! MMSUCCESS(Result) )
        return Result;

    SND_TRACE(L"WDMAUD - GetWdmDeviceCapabilities DeviceType %u DeviceId %u\n", DeviceType, DeviceId);

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.DeviceType = DeviceType;
    DeviceInfo.DeviceIndex = DeviceId;

    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_GETCAPABILITIES,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    /* This is pretty much a big hack right now */
    switch ( DeviceType )
    {
        case MIXER_DEVICE_TYPE:
        {
            LPMIXERCAPSW MixerCaps = (LPMIXERCAPSW) Capabilities;

            DeviceInfo.u.MixCaps.szPname[MAXPNAMELEN-1] = L'\0';
            CopyWideString(MixerCaps->szPname, DeviceInfo.u.MixCaps.szPname);

            MixerCaps->cDestinations = DeviceInfo.u.MixCaps.cDestinations;
            MixerCaps->fdwSupport = DeviceInfo.u.MixCaps.fdwSupport;
            MixerCaps->vDriverVersion = DeviceInfo.u.MixCaps.vDriverVersion;
            MixerCaps->wMid = DeviceInfo.u.MixCaps.wMid;
            MixerCaps->wPid = DeviceInfo.u.MixCaps.wPid;
            break;
        }
        case WAVE_OUT_DEVICE_TYPE :
        {
            LPWAVEOUTCAPSW WaveOutCaps = (LPWAVEOUTCAPSW) Capabilities;

            DeviceInfo.u.WaveOutCaps.szPname[MAXPNAMELEN-1] = L'\0';
            WaveOutCaps->wMid = DeviceInfo.u.WaveOutCaps.wMid;
            WaveOutCaps->wPid = DeviceInfo.u.WaveOutCaps.wPid;

            WaveOutCaps->vDriverVersion = DeviceInfo.u.WaveOutCaps.vDriverVersion;
            CopyWideString(WaveOutCaps->szPname, DeviceInfo.u.WaveOutCaps.szPname);

            WaveOutCaps->dwFormats = DeviceInfo.u.WaveOutCaps.dwFormats;
            WaveOutCaps->wChannels = DeviceInfo.u.WaveOutCaps.wChannels;
            WaveOutCaps->dwSupport = DeviceInfo.u.WaveOutCaps.dwSupport;
            break;
        }
        case WAVE_IN_DEVICE_TYPE :
        {
            LPWAVEINCAPSW WaveInCaps = (LPWAVEINCAPSW) Capabilities;

            DeviceInfo.u.WaveInCaps.szPname[MAXPNAMELEN-1] = L'\0';

            WaveInCaps->wMid = DeviceInfo.u.WaveInCaps.wMid;
            WaveInCaps->wPid = DeviceInfo.u.WaveInCaps.wPid;

            WaveInCaps->vDriverVersion = DeviceInfo.u.WaveInCaps.vDriverVersion;
            CopyWideString(WaveInCaps->szPname, DeviceInfo.u.WaveInCaps.szPname);

            WaveInCaps->dwFormats = DeviceInfo.u.WaveInCaps.dwFormats;
            WaveInCaps->wChannels = DeviceInfo.u.WaveInCaps.wChannels;
            WaveInCaps->wReserved1 = 0;
            break;
        }
        case MIDI_IN_DEVICE_TYPE :
        {
            LPMIDIINCAPSW MidiInCaps = (LPMIDIINCAPSW)Capabilities;

            DeviceInfo.u.MidiInCaps.szPname[MAXPNAMELEN-1] = L'\0';

            MidiInCaps->vDriverVersion = DeviceInfo.u.MidiInCaps.vDriverVersion;
            MidiInCaps->wMid = DeviceInfo.u.MidiInCaps.wMid;
            MidiInCaps->wPid = DeviceInfo.u.MidiInCaps.wPid;
            MidiInCaps->dwSupport = DeviceInfo.u.MidiInCaps.dwSupport;

            CopyWideString(MidiInCaps->szPname, DeviceInfo.u.MidiInCaps.szPname);
            break;
        }
        case MIDI_OUT_DEVICE_TYPE :
        {
            LPMIDIOUTCAPSW MidiOutCaps = (LPMIDIOUTCAPSW)Capabilities;

            DeviceInfo.u.MidiOutCaps.szPname[MAXPNAMELEN-1] = L'\0';

            MidiOutCaps->vDriverVersion = DeviceInfo.u.MidiOutCaps.vDriverVersion;
            MidiOutCaps->wMid = DeviceInfo.u.MidiOutCaps.wMid;
            MidiOutCaps->wPid = DeviceInfo.u.MidiOutCaps.wPid;
            MidiOutCaps->dwSupport = DeviceInfo.u.MidiOutCaps.dwSupport;

            CopyWideString(MidiOutCaps->szPname, DeviceInfo.u.MidiOutCaps.szPname);
            break;
        }
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudOpenSoundDeviceByLegacy(
    IN PSOUND_DEVICE SoundDevice,
    OUT PVOID *Handle)
{
    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    GUID SWBusGuid = {STATIC_KSCATEGORY_WDMAUD};
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData;

    if ( KernelHandle == INVALID_HANDLE_VALUE )
    {
        hDevInfo = SetupDiGetClassDevsW(&SWBusGuid, NULL, NULL,  DIGCF_DEVICEINTERFACE| DIGCF_PRESENT);
        if (!hDevInfo)
        {
            // failed
            return MMSYSERR_ERROR;
        }

        DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &SWBusGuid, 0, &DeviceInterfaceData))
        {
            // failed
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return MMSYSERR_ERROR;
        }

        DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR) + sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W));
        if (!DeviceInterfaceDetailData)
        {
            // failed
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return MMSYSERR_ERROR;
        }

        DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(hDevInfo,  &DeviceInterfaceData, DeviceInterfaceDetailData,MAX_PATH * sizeof(WCHAR) + sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W), NULL, NULL))
        {
            // failed
            HeapFree(GetProcessHeap(), 0, DeviceInterfaceDetailData);
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return MMSYSERR_ERROR;
        }
        SND_TRACE(L"Opening wdmaud device '%s'\n",DeviceInterfaceDetailData->DevicePath);
        KernelHandle = CreateFileW(DeviceInterfaceDetailData->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL);

        HeapFree(GetProcessHeap(), 0, DeviceInterfaceDetailData);
        SetupDiDestroyDeviceInfoList(hDevInfo);
    }


    if ( KernelHandle == INVALID_HANDLE_VALUE )
        return MMSYSERR_ERROR;

    ++ OpenCount;
        return MMSYSERR_NOERROR;

}

MMRESULT
WdmAudCloseSoundDeviceByLegacy(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  PVOID Handle)
{
    WDMAUD_DEVICE_INFO DeviceInfo;
    MMRESULT Result;
    MMDEVICE_TYPE DeviceType;
    PSOUND_DEVICE SoundDevice;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    if ( OpenCount == 0 )
    {
        return MMSYSERR_NOERROR;
    }

    SND_ASSERT( KernelHandle != INVALID_HANDLE_VALUE );

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    if (SoundDeviceInstance->Handle != (PVOID)KernelHandle)
    {
        ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));

        DeviceInfo.DeviceType = DeviceType;
        DeviceInfo.hDevice = SoundDeviceInstance->Handle;

         /* First stop the stream */
         if (DeviceType != MIXER_DEVICE_TYPE)
         {
             DeviceInfo.u.State = KSSTATE_PAUSE;
             SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_SETDEVICE_STATE,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                            sizeof(WDMAUD_DEVICE_INFO),
                                            NULL);

             DeviceInfo.u.State = KSSTATE_ACQUIRE;
             SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_SETDEVICE_STATE,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                            sizeof(WDMAUD_DEVICE_INFO),
                                            NULL);


             DeviceInfo.u.State = KSSTATE_STOP;
             SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_SETDEVICE_STATE,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                            sizeof(WDMAUD_DEVICE_INFO),
                                            NULL);
        }

        SyncOverlappedDeviceIoControl(KernelHandle,
                                      IOCTL_CLOSE_WDMAUD,
                                      (LPVOID) &DeviceInfo,
                                      sizeof(WDMAUD_DEVICE_INFO),
                                      (LPVOID) &DeviceInfo,
                                      sizeof(WDMAUD_DEVICE_INFO),
                                      NULL);
    }

    if (DeviceType == MIXER_DEVICE_TYPE)
    {
        SetEvent(SoundDeviceInstance->hStopEvent);
        CloseHandle(SoundDeviceInstance->hStopEvent);
        CloseHandle(SoundDeviceInstance->hNotifyEvent);
    }

    --OpenCount;

    if ( OpenCount < 1 )
    {
        CloseHandle(KernelHandle);
        KernelHandle = INVALID_HANDLE_VALUE;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudSetMixerDeviceFormatByLegacy(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD DeviceId,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    MMRESULT Result;
    WDMAUD_DEVICE_INFO DeviceInfo;
    HANDLE hThread;

    Instance->hNotifyEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if ( ! Instance->hNotifyEvent )
        return MMSYSERR_NOMEM;

    if (Instance->Handle != NULL)
    {
        /* device is already open */
        return MMSYSERR_NOERROR;
    }

    Instance->hStopEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if ( ! Instance->hStopEvent )
        return MMSYSERR_NOMEM;

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.DeviceType = MIXER_DEVICE_TYPE;
    DeviceInfo.DeviceIndex = DeviceId;
    DeviceInfo.u.hNotifyEvent = Instance->hNotifyEvent;

    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_OPEN_WDMAUD,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);

    if ( ! MMSUCCESS(Result) )
    {
        CloseHandle(Instance->hNotifyEvent);
        CloseHandle(Instance->hStopEvent);
        return TranslateInternalMmResult(Result);
    }

    hThread = CreateThread(NULL, 0, MixerEventThreadRoutine, (LPVOID)Instance, 0, NULL);
    if (  hThread )
    {
        CloseHandle(hThread);
    }

    /* Store sound device handle instance handle */
    Instance->Handle = (PVOID)DeviceInfo.hDevice;

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudSetWaveDeviceFormatByLegacy(
    IN  PSOUND_DEVICE_INSTANCE Instance,
    IN  DWORD DeviceId,
    IN  PWAVEFORMATEX WaveFormat,
    IN  DWORD WaveFormatSize)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    PVOID Identifier;
    WDMAUD_DEVICE_INFO DeviceInfo;
    MMDEVICE_TYPE DeviceType;

    Result = GetSoundDeviceFromInstance(Instance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    Result = GetSoundDeviceIdentifier(SoundDevice, &Identifier);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    if (Instance->Handle != NULL)
    {
        /* device is already open */
        return MMSYSERR_NOERROR;
    }

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);

    SND_ASSERT( Result == MMSYSERR_NOERROR );

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.DeviceType = DeviceType;
    DeviceInfo.DeviceIndex = DeviceId;
    DeviceInfo.u.WaveFormatEx.cbSize = sizeof(WAVEFORMATEX); //WaveFormat->cbSize;
    DeviceInfo.u.WaveFormatEx.wFormatTag = WaveFormat->wFormatTag;
#ifdef USERMODE_MIXER
    DeviceInfo.u.WaveFormatEx.nChannels = 2;
    DeviceInfo.u.WaveFormatEx.nSamplesPerSec = 44100;
    DeviceInfo.u.WaveFormatEx.nBlockAlign = 4;
    DeviceInfo.u.WaveFormatEx.nAvgBytesPerSec = 176400;
    DeviceInfo.u.WaveFormatEx.wBitsPerSample = 16;
#else
    DeviceInfo.u.WaveFormatEx.nChannels = WaveFormat->nChannels;
    DeviceInfo.u.WaveFormatEx.nSamplesPerSec = WaveFormat->nSamplesPerSec;
    DeviceInfo.u.WaveFormatEx.nBlockAlign = WaveFormat->nBlockAlign;
    DeviceInfo.u.WaveFormatEx.nAvgBytesPerSec = WaveFormat->nAvgBytesPerSec;
    DeviceInfo.u.WaveFormatEx.wBitsPerSample = (DeviceInfo.u.WaveFormatEx.nAvgBytesPerSec * 8) / (DeviceInfo.u.WaveFormatEx.nSamplesPerSec * DeviceInfo.u.WaveFormatEx.nChannels);
#endif

    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_OPEN_WDMAUD,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    if (WaveFormatSize >= sizeof(WAVEFORMAT))
    {
        /* Store format */
        Instance->WaveFormatEx.wFormatTag = WaveFormat->wFormatTag;
        Instance->WaveFormatEx.nChannels = WaveFormat->nChannels;
        Instance->WaveFormatEx.nSamplesPerSec = WaveFormat->nSamplesPerSec;
        Instance->WaveFormatEx.nBlockAlign = WaveFormat->nBlockAlign;
        Instance->WaveFormatEx.nAvgBytesPerSec = WaveFormat->nAvgBytesPerSec;
    }

    /* store details */
    Instance->WaveFormatEx.cbSize = sizeof(WAVEFORMATEX);
    Instance->WaveFormatEx.wBitsPerSample = (DeviceInfo.u.WaveFormatEx.nAvgBytesPerSec * 8) / (DeviceInfo.u.WaveFormatEx.nSamplesPerSec * DeviceInfo.u.WaveFormatEx.nChannels);

    /* Store sound device handle instance handle */
    Instance->Handle = (PVOID)DeviceInfo.hDevice;

    /* Now determine framing requirements */
    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_GETFRAMESIZE,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);

    if ( MMSUCCESS(Result) )
    {
        if (DeviceInfo.u.FrameSize)
        {
            Instance->FrameSize = DeviceInfo.u.FrameSize * 2;
            Instance->BufferCount = WaveFormat->nAvgBytesPerSec / Instance->FrameSize;
            SND_TRACE(L"FrameSize %u BufferCount %u\n", Instance->FrameSize, Instance->BufferCount);
        }
    }
    else
    {
        // use a default of 100 buffers
        Instance->BufferCount = 100;
    }

    /* Now acquire resources */
    DeviceInfo.u.State = KSSTATE_ACQUIRE;
    SyncOverlappedDeviceIoControl(KernelHandle, IOCTL_SETDEVICE_STATE, (LPVOID) &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), (LPVOID) &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), NULL);

    /* pause the pin */
    DeviceInfo.u.State = KSSTATE_PAUSE;
    SyncOverlappedDeviceIoControl(KernelHandle, IOCTL_SETDEVICE_STATE, (LPVOID) &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), (LPVOID) &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), NULL);

    /* start the pin */
    DeviceInfo.u.State = KSSTATE_RUN;
    SyncOverlappedDeviceIoControl(KernelHandle, IOCTL_SETDEVICE_STATE, (LPVOID) &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), (LPVOID) &DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), NULL);


    return MMSYSERR_NOERROR;
}

VOID
CALLBACK
LegacyCompletionRoutine(
    IN  DWORD dwErrorCode,
    IN  DWORD dwNumberOfBytesTransferred,
    IN  LPOVERLAPPED lpOverlapped)
{
    PSOUND_OVERLAPPED Overlap;
    PWDMAUD_DEVICE_INFO DeviceInfo;

    Overlap = (PSOUND_OVERLAPPED)lpOverlapped;
    DeviceInfo = (PWDMAUD_DEVICE_INFO)Overlap->CompletionContext;

    /* Call mmebuddy overlap routine */
    Overlap->OriginalCompletionRoutine(dwErrorCode, DeviceInfo->Header.DataUsed, lpOverlapped);

    HeapFree(GetProcessHeap(), 0, DeviceInfo);
}

MMRESULT
WdmAudCommitWaveBufferByLegacy(
    IN  PSOUND_DEVICE_INSTANCE SoundDeviceInstance,
    IN  PVOID OffsetPtr,
    IN  DWORD Length,
    IN  PSOUND_OVERLAPPED Overlap,
    IN  LPOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine)
{
    HANDLE Handle;
    MMRESULT Result;
    PWDMAUD_DEVICE_INFO DeviceInfo;
    PSOUND_DEVICE SoundDevice;
    MMDEVICE_TYPE DeviceType;
    BOOL Ret;

    VALIDATE_MMSYS_PARAMETER( SoundDeviceInstance );
    VALIDATE_MMSYS_PARAMETER( OffsetPtr );
    VALIDATE_MMSYS_PARAMETER( Overlap );
    VALIDATE_MMSYS_PARAMETER( CompletionRoutine );

    GetSoundDeviceInstanceHandle(SoundDeviceInstance, &Handle);
    SND_ASSERT(Handle);

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    DeviceInfo = (PWDMAUD_DEVICE_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WDMAUD_DEVICE_INFO));
    if (!DeviceInfo)
    {
        // no memory
        return MMSYSERR_NOMEM;
    }

    DeviceInfo->Header.FrameExtent = Length;
    if (DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        DeviceInfo->Header.DataUsed = Length;
    }
    DeviceInfo->Header.Data = OffsetPtr;
    DeviceInfo->Header.Size = sizeof(WDMAUD_DEVICE_INFO);
    DeviceInfo->Header.PresentationTime.Numerator = 1;
    DeviceInfo->Header.PresentationTime.Denominator = 1;
    DeviceInfo->hDevice = Handle;
    DeviceInfo->DeviceType = DeviceType;


    // create completion event
    Overlap->Standard.hEvent = Handle = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (Overlap->Standard.hEvent == NULL)
    {
        // no memory
        HeapFree(GetProcessHeap(), 0, DeviceInfo);
        return MMSYSERR_NOMEM;
    }

    Overlap->OriginalCompletionRoutine = CompletionRoutine;
    Overlap->CompletionContext = (PVOID)DeviceInfo;

    if (DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        Ret = WriteFileEx(KernelHandle, DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), (LPOVERLAPPED)Overlap, LegacyCompletionRoutine);
        if (Ret)
            WaitForSingleObjectEx (KernelHandle, INFINITE, TRUE);
    }
    else if (DeviceType == WAVE_IN_DEVICE_TYPE)
    {
        Ret = ReadFileEx(KernelHandle, DeviceInfo, sizeof(WDMAUD_DEVICE_INFO), (LPOVERLAPPED)Overlap, LegacyCompletionRoutine);
        if (Ret)
            WaitForSingleObjectEx (KernelHandle, INFINITE, TRUE);
    }

    // close event handle
    CloseHandle(Handle);

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudSetWaveStateByLegacy(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN BOOL bStart)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    WDMAUD_DEVICE_INFO DeviceInfo;
    MMDEVICE_TYPE DeviceType;
    HANDLE Handle;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    Result = GetSoundDeviceInstanceHandle(SoundDeviceInstance, &Handle);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.hDevice = Handle;
    DeviceInfo.DeviceType = DeviceType;

    if (bStart)
        DeviceInfo.u.State = KSSTATE_RUN;
    else
        DeviceInfo.u.State = KSSTATE_PAUSE;
    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_SETDEVICE_STATE,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);

    return Result;
}

MMRESULT
WdmAudGetDeviceInterfaceStringByLegacy(
    IN  MMDEVICE_TYPE DeviceType,
    IN  DWORD DeviceId,
    IN  LPWSTR Interface,
    IN  DWORD  InterfaceLength,
    OUT  DWORD * InterfaceSize)
{
    WDMAUD_DEVICE_INFO DeviceInfo;
    MMRESULT Result;

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.DeviceType = DeviceType;
    DeviceInfo.DeviceIndex = DeviceId;


    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_QUERYDEVICEINTERFACESTRING,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);


    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }


    if (!Interface)
    {
        SND_ASSERT(InterfaceSize);

        *InterfaceSize = DeviceInfo.u.Interface.DeviceInterfaceStringSize;
        return MMSYSERR_NOERROR;
    }

    if (InterfaceLength < DeviceInfo.u.Interface.DeviceInterfaceStringSize)
    {
        /* buffer is too small */
        return MMSYSERR_MOREDATA;
    }

    DeviceInfo.u.Interface.DeviceInterfaceStringSize = InterfaceLength;
    DeviceInfo.u.Interface.DeviceInterfaceString = Interface;

    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_QUERYDEVICEINTERFACESTRING,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);

    if (  MMSUCCESS(Result) && InterfaceLength > 2)
    {
        Interface[1] = L'\\';
        Interface[InterfaceLength-1] = L'\0';
    }

    return Result;
}

MMRESULT
WdmAudGetWavePositionByLegacy(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  MMTIME* Time)
{
    MMRESULT Result;
    PSOUND_DEVICE SoundDevice;
    WDMAUD_DEVICE_INFO DeviceInfo;
    MMDEVICE_TYPE DeviceType;
    HANDLE Handle;

    Result = GetSoundDeviceFromInstance(SoundDeviceInstance, &SoundDevice);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    Result = GetSoundDeviceType(SoundDevice, &DeviceType);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    Result = GetSoundDeviceInstanceHandle(SoundDeviceInstance, &Handle);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.hDevice = Handle;
    DeviceInfo.DeviceType = DeviceType;

    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_OPEN_WDMAUD,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);

    if ( ! MMSUCCESS(Result) )
    {
        return TranslateInternalMmResult(Result);
    }

    Time->wType = TIME_BYTES;
    Time->u.cb = (DWORD)DeviceInfo.u.Position;

    return MMSYSERR_NOERROR;
}


MMRESULT
WdmAudResetStreamByLegacy(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN  MMDEVICE_TYPE DeviceType,
    IN  BOOLEAN bStartReset)
{
    MMRESULT Result;
    HANDLE Handle;
    WDMAUD_DEVICE_INFO DeviceInfo;

    Result = GetSoundDeviceInstanceHandle(SoundDeviceInstance, &Handle);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.hDevice = Handle;
    DeviceInfo.DeviceType = DeviceType;
    DeviceInfo.u.ResetStream = (bStartReset ? KSRESET_BEGIN : KSRESET_END);

    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IOCTL_RESET_STREAM,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);
    return Result;
}

MMRESULT
WdmAudQueryMixerInfoByLegacy(
    IN  struct _SOUND_DEVICE_INSTANCE* SoundDeviceInstance,
    IN DWORD DeviceId,
    IN UINT uMsg,
    IN LPVOID Parameter,
    IN DWORD Flags)
{
    MMRESULT Result;
    WDMAUD_DEVICE_INFO DeviceInfo;
    HANDLE Handle;
    DWORD IoControlCode;
    LPMIXERLINEW MixLine;
    LPMIXERLINECONTROLSW MixControls;
    LPMIXERCONTROLDETAILS MixDetails;

    SND_TRACE(L"uMsg %x Flags %x\n", uMsg, Flags);

    Result = GetSoundDeviceInstanceHandle(SoundDeviceInstance, &Handle);
    SND_ASSERT( Result == MMSYSERR_NOERROR );

    ZeroMemory(&DeviceInfo, sizeof(WDMAUD_DEVICE_INFO));
    DeviceInfo.hDevice = Handle;
    DeviceInfo.DeviceIndex = DeviceId;
    DeviceInfo.DeviceType = MIXER_DEVICE_TYPE;
    DeviceInfo.Flags = Flags;

    MixLine = (LPMIXERLINEW)Parameter;
    MixControls = (LPMIXERLINECONTROLSW)Parameter;
    MixDetails = (LPMIXERCONTROLDETAILS)Parameter;

    switch(uMsg)
    {
        case MXDM_GETLINEINFO:
            RtlCopyMemory(&DeviceInfo.u.MixLine, MixLine, sizeof(MIXERLINEW));
            IoControlCode = IOCTL_GETLINEINFO;
            break;
        case MXDM_GETLINECONTROLS:
            RtlCopyMemory(&DeviceInfo.u.MixControls, MixControls, sizeof(MIXERLINECONTROLSW));
            IoControlCode = IOCTL_GETLINECONTROLS;
            break;
       case MXDM_SETCONTROLDETAILS:
            RtlCopyMemory(&DeviceInfo.u.MixDetails, MixDetails, sizeof(MIXERCONTROLDETAILS));
            IoControlCode = IOCTL_SETCONTROLDETAILS;
            break;
       case MXDM_GETCONTROLDETAILS:
            RtlCopyMemory(&DeviceInfo.u.MixDetails, MixDetails, sizeof(MIXERCONTROLDETAILS));
            IoControlCode = IOCTL_GETCONTROLDETAILS;
            break;
       default:
           SND_ASSERT(0);
           return MMSYSERR_NOTSUPPORTED;
    }

    Result = SyncOverlappedDeviceIoControl(KernelHandle,
                                           IoControlCode,
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           (LPVOID) &DeviceInfo,
                                           sizeof(WDMAUD_DEVICE_INFO),
                                           NULL);

    if ( ! MMSUCCESS(Result) )
    {
        return Result;
    }

    switch(uMsg)
    {
        case MXDM_GETLINEINFO:
        {
            RtlCopyMemory(MixLine, &DeviceInfo.u.MixLine, sizeof(MIXERLINEW));
            break;
        }
    }

    return Result;
}
