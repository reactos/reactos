/*
 * PROJECT:     ReactOS Audio Subsystem
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     WDM Audio Driver Mapper (User-mode part)
 * COPYRIGHT:   Copyright 2007-2008 Andrew Greenwood (silverblade@reactos.org)
 *              Copyright 2009-2010 Johannes Anderwald
 *              Copyright 2022 Oleg Dubinskiy (oleg.dubinskij30@gmail.com)
 */

#include "wdmaud.h"

#include <mmixer.h>

#define YDEBUG
#include <debug.h>

extern MIXER_CONTEXT MixerContext;
HANDLE KernelHandle = INVALID_HANDLE_VALUE;
DWORD OpenCount = 0;

/*
    This is a wrapper around DeviceIoControl which provides control over
    instantiated sound devices. It waits for I/O to complete (since an
    instantiated sound device is opened _In_ overlapped mode, this is necessary).
*/
MMRESULT
WdmAudIoControl(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_opt_ ULONG BufferSize,
    _In_opt_ PVOID Buffer,
    _In_ DWORD IoControlCode)
{
    MMRESULT Result = MMSYSERR_NOERROR;
    OVERLAPPED Overlapped;
    DWORD Transferred = 0;
    BOOL IoResult;

    /* Overlapped I/O is done here - this is used for waiting for completion */
    ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! Overlapped.hEvent )
        return Win32ErrorToMmResult(GetLastError());

    if (DeviceInfo->DeviceType != AUX_DEVICE_TYPE &&
        DeviceInfo->DeviceType != MIXER_DEVICE_TYPE)
    {
        EnterCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);
    }

    /* Store input data */
    DeviceInfo->BufferSize = BufferSize;
    DeviceInfo->Buffer = Buffer;

    /* Talk to the device */
    IoResult = DeviceIoControl(KernelHandle,
                               IoControlCode,
                               DeviceInfo,
                               sizeof(WDMAUD_DEVICE_INFO),
                               DeviceInfo,
                               sizeof(WDMAUD_DEVICE_INFO),
                               &Transferred,
                               &Overlapped);

    /* If failure occurs, make sure it's not just due to the overlapped I/O */
    if ( ! IoResult )
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
            /* Wait for I/O complete */
            WaitForSingleObject(Overlapped.hEvent, INFINITE);
        }
        Result = Win32ErrorToMmResult(GetLastError());
    }

    if (DeviceInfo->DeviceType != AUX_DEVICE_TYPE &&
        DeviceInfo->DeviceType != MIXER_DEVICE_TYPE)
    {
        LeaveCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);
    }

    /* Don't need this any more */
    CloseHandle(Overlapped.hEvent);

    DPRINT("Transferred %d bytes in Sync overlapped I/O\n", Transferred);

    return Result;
}

DWORD
WINAPI
WaveThreadRoutine(
    LPVOID Parameter)
{
    PWDMAUD_DEVICE_INFO DeviceInfo = (PWDMAUD_DEVICE_INFO)Parameter;
    PWAVEHDR_EXTENSION HeaderExtension;
    DWORD dwResult = WAIT_FAILED;
    PWAVEHDR WaveHeader;
#ifndef USE_MMIXER_LIB
    MMRESULT Result;
#endif
    HANDLE hEvent;

    while (TRUE)
    {
        DPRINT("WaveQueue %p\n", DeviceInfo->DeviceState->WaveQueue);
        EnterCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);
        WaveHeader = DeviceInfo->DeviceState->WaveQueue;
        if (WaveHeader && (WaveHeader->dwFlags & (WHDR_BEGINLOOP | WHDR_DONE |
            WHDR_ENDLOOP | WHDR_INQUEUE | WHDR_PREPARED)) && WaveHeader->reserved)
        {
            DPRINT("1\n");
            HeaderExtension = (PWAVEHDR_EXTENSION)WaveHeader->reserved;
            if (HeaderExtension && HeaderExtension->Overlapped &&
                HeaderExtension->Overlapped->hEvent)
            {
                hEvent = HeaderExtension->Overlapped->hEvent;
                DPRINT("2\n");

                LeaveCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);
                /* Wait for I/O complete */
                dwResult = WaitForSingleObject(hEvent, INFINITE);
                DPRINT("dwResult %d\n", dwResult);
                EnterCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);

                DPRINT("3\n");
                if (DeviceInfo &&
                    DeviceInfo->DeviceState->hNotifyEvent &&
                    DeviceInfo->DeviceState->hStopEvent)
                {
                    /* Complete current header */
                    CompleteWaveHeader(DeviceInfo);
                }
                else
                {
                    DPRINT1("Invalid device info data %p\n", DeviceInfo);
                    break;
                }
            }
            else
            {
                /* Failed, move to the next header */
                DeviceInfo->DeviceState->WaveQueue = DeviceInfo->DeviceState->WaveQueue->lpNext;
            }
            LeaveCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);
        }
        else
        {
            DPRINT("6\n");
            if (DeviceInfo->DeviceState->bStart)
            {
                DPRINT("7\n");
#ifdef USE_MMIXER_LIB
                /* make sure the pin is stopped */
                MMixerSetWaveStatus(&MixerContext, DeviceInfo->hDevice, KSSTATE_PAUSE);
                MMixerSetWaveStatus(&MixerContext, DeviceInfo->hDevice, KSSTATE_ACQUIRE);
                MMixerSetWaveStatus(&MixerContext, DeviceInfo->hDevice, KSSTATE_STOP);
#else
                /* Stop streaming if no more data to stream */
                Result = WdmAudIoControl(DeviceInfo,
                                         0,
                                         NULL,
                                         DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                                         IOCTL_PAUSE_PLAYBACK : IOCTL_PAUSE_CAPTURE);
                if (!MMSUCCESS(Result))
                {
                    DPRINT1("Call to %s failed with %d\n",
                            DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                            "IOCTL_PAUSE_PLAYBACK" : "IOCTL_PAUSE_CAPTURE",
                            GetLastError());
                    break;
                }
#endif
                DeviceInfo->DeviceState->bStart = FALSE;
            }

            LeaveCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);

            /* Wait for stop event complete */
            WaitForSingleObject(DeviceInfo->DeviceState->hNotifyEvent, INFINITE);

            /* Done */
            break;
        }
    }

    EnterCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);
    CloseHandle(DeviceInfo->DeviceState->hNotifyEvent);
    DeviceInfo->DeviceState->hNotifyEvent = NULL;
    DeviceInfo->DeviceState->bStartInThread = FALSE;
    SetEvent(DeviceInfo->DeviceState->hStopEvent);
    LeaveCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);

    /* Done */
    return 0;
}

MMRESULT
WdmAudCreateCompletionThread(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo)
{
    if (!DeviceInfo->DeviceState->hThread)
    {
        if (!DeviceInfo->DeviceState->hNotifyEvent)
        {
            DeviceInfo->DeviceState->hNotifyEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
            if (!DeviceInfo->DeviceState->hNotifyEvent)
            {
                DPRINT1("CreateEventW failed with %d\n", GetLastError());
                DeviceInfo->DeviceState->bStartInThread = FALSE;
                return Win32ErrorToMmResult(GetLastError());
            }
        }

        if (!DeviceInfo->DeviceState->hStopEvent)
        {
            DeviceInfo->DeviceState->hStopEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
            if (!DeviceInfo->DeviceState->hStopEvent)
            {
                DPRINT1("CreateEventW failed with %d\n", GetLastError());
                DeviceInfo->DeviceState->bStartInThread = FALSE;
                /* Close previous handle */
                CloseHandle(DeviceInfo->DeviceState->hNotifyEvent);
                return Win32ErrorToMmResult(GetLastError());
            }
        }

        DeviceInfo->DeviceState->hThread = CreateThread(NULL,
                                                        0,
                                                        WaveThreadRoutine,
                                                        DeviceInfo,
                                                        0,
                                                        NULL);
        if (!DeviceInfo->DeviceState->hThread)
        {
            DPRINT1("CreateThread failed with %d\n", GetLastError());
            DeviceInfo->DeviceState->bStartInThread = FALSE;
            /* Close previous handles */
            CloseHandle(DeviceInfo->DeviceState->hNotifyEvent);
            CloseHandle(DeviceInfo->DeviceState->hStopEvent);
            return Win32ErrorToMmResult(GetLastError());
        }
        SetThreadPriority(DeviceInfo->DeviceState->hThread, THREAD_PRIORITY_TIME_CRITICAL);
    }

    DeviceInfo->DeviceState->bStartInThread = TRUE;

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudDestroyCompletionThread(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo)
{
    EnterCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);

    if (DeviceInfo->DeviceState->hThread)
    {
        if (DeviceInfo->DeviceState->hNotifyEvent)
        {
            if (DeviceInfo->DeviceState->bStartInThread)
            {
                SetEvent(DeviceInfo->DeviceState->hNotifyEvent);
            }
        }
        LeaveCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);

        if (DeviceInfo->DeviceState->hStopEvent)
        {
            WaitForSingleObject(DeviceInfo->DeviceState->hStopEvent, INFINITE);
        }

        EnterCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);
        CloseHandle(DeviceInfo->DeviceState->hThread);
        DeviceInfo->DeviceState->hThread = NULL;

        if (DeviceInfo->DeviceState->hStopEvent)
        {
            CloseHandle(DeviceInfo->DeviceState->hStopEvent);
            DeviceInfo->DeviceState->hStopEvent = NULL;
        }
    }
    LeaveCriticalSection(DeviceInfo->DeviceState->QueueCriticalSection);

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudCleanupByLegacy(VOID)
{
    if (KernelHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(KernelHandle);
        KernelHandle = INVALID_HANDLE_VALUE;
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudAddRemoveDeviceNode(
    _In_ SOUND_DEVICE_TYPE DeviceType,
    _In_ BOOL bAdd)
{
    MMRESULT Result;
    PWDMAUD_DEVICE_INFO DeviceInfo;

    VALIDATE_MMSYS_PARAMETER( IS_VALID_SOUND_DEVICE_TYPE(DeviceType) );

    DPRINT("WDMAUD - AddRemoveDeviceNode DeviceType %u\n", DeviceType);

    DeviceInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WDMAUD_DEVICE_INFO));
    if (!DeviceInfo)
    {
        /* No memory */
        DPRINT1("Failed to allocate WDMAUD_DEVICE_INFO structure\n");
        return MMSYSERR_NOMEM;
    }

    DeviceInfo->DeviceType = DeviceType;

    Result = WdmAudIoControl(DeviceInfo,
                             0,
                             NULL,
                             bAdd ?
                             IOCTL_ADD_DEVNODE :
                             IOCTL_REMOVE_DEVNODE);

    HeapFree(GetProcessHeap(), 0, DeviceInfo);

    if ( ! MMSUCCESS( Result ) )
    {
        DPRINT1("Call to %ls failed with %d\n",
                bAdd ? L"IOCTL_ADD_DEVNODE" : L"IOCTL_REMOVE_DEVNODE",
                GetLastError());
        return TranslateInternalMmResult(Result);
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudGetNumWdmDevsByLegacy(
    _In_  SOUND_DEVICE_TYPE DeviceType,
    _Out_ DWORD* DeviceCount)
{
    MMRESULT Result;
    PWDMAUD_DEVICE_INFO DeviceInfo;

    VALIDATE_MMSYS_PARAMETER( KernelHandle != INVALID_HANDLE_VALUE );
    VALIDATE_MMSYS_PARAMETER( IS_VALID_SOUND_DEVICE_TYPE(DeviceType) );
    VALIDATE_MMSYS_PARAMETER( DeviceCount );

    DPRINT("WDMAUD - GetNumWdmDevs DeviceType %u\n", DeviceType);

    DeviceInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WDMAUD_DEVICE_INFO));
    if (!DeviceInfo)
    {
        /* No memory */
        DPRINT1("Failed to allocate WDMAUD_DEVICE_INFO structure\n");
        return MMSYSERR_NOMEM;
    }

    DeviceInfo->DeviceType = DeviceType;

    Result = WdmAudIoControl(DeviceInfo,
                             0,
                             NULL,
                             IOCTL_GETNUMDEVS_TYPE);

    if ( ! MMSUCCESS( Result ) )
    {
        DPRINT1("Call to IOCTL_GETNUMDEVS_TYPE failed with %d\n", GetLastError());
        *DeviceCount = 0;
        HeapFree(GetProcessHeap(), 0, DeviceInfo);
        return TranslateInternalMmResult(Result);
    }

    *DeviceCount = DeviceInfo->DeviceIndex;

    HeapFree(GetProcessHeap(), 0, DeviceInfo);

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudGetCapabilitiesByLegacy(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _Out_ PVOID Capabilities,
    _In_  DWORD CapabilitiesSize)
{
    MMRESULT Result;

    ASSERT( Capabilities );

    DPRINT("WDMAUD - GetWdmDeviceCapabilities DeviceType %u DeviceId %u\n", DeviceInfo->DeviceType, DeviceInfo->DeviceIndex);

    Result = WdmAudIoControl(DeviceInfo,
                             CapabilitiesSize,
                             Capabilities,
                             IOCTL_GETCAPABILITIES);

    if ( ! MMSUCCESS(Result) )
    {
        DPRINT1("Call to IOCTL_GETCAPABILITIES failed with %d\n", GetLastError());
        Capabilities = NULL;
        return TranslateInternalMmResult(Result);
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudOpenKernelSoundDeviceByLegacy(VOID)
{
    DWORD dwSize;
    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    GUID SWBusGuid = {STATIC_KSCATEGORY_WDMAUD};
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData;

    if ( KernelHandle == INVALID_HANDLE_VALUE )
    {
        hDevInfo = SetupDiGetClassDevsW(&SWBusGuid, NULL, NULL,  DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
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

        SetupDiGetDeviceInterfaceDetailW(hDevInfo,  &DeviceInterfaceData, NULL, 0, &dwSize, NULL);

        DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)HeapAlloc(GetProcessHeap(), 0, dwSize);
        if (!DeviceInterfaceDetailData)
        {
            // failed
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return MMSYSERR_ERROR;
        }

        DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(hDevInfo,  &DeviceInterfaceData, DeviceInterfaceDetailData, dwSize, &dwSize, NULL))
        {
            // failed
            HeapFree(GetProcessHeap(), 0, DeviceInterfaceDetailData);
            SetupDiDestroyDeviceInfoList(hDevInfo);
            return MMSYSERR_ERROR;
        }

        DPRINT("Opening wdmaud device '%ls'\n", DeviceInterfaceDetailData->DevicePath);
        KernelHandle = CreateFileW(DeviceInterfaceDetailData->DevicePath,
                                   GENERIC_READ | GENERIC_WRITE,
                                   0,
                                   NULL,
                                   OPEN_EXISTING,
                                   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
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
WdmAudOpenSoundDeviceByLegacy(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_ PWAVEFORMATEX WaveFormat)
{
    MMRESULT Result;

    if ( OpenCount == 0 )
    {
        return MMSYSERR_NOERROR;
    }

    ASSERT( KernelHandle != INVALID_HANDLE_VALUE );

    DPRINT("WDMAUD - OpenWdmSoundDevice DeviceType %u\n", DeviceInfo->DeviceType);

    /* Open device handle */
    Result = WdmAudIoControl(DeviceInfo,
                             sizeof(WAVEFORMATEX),
                             WaveFormat,
                             IOCTL_OPEN_WDMAUD);

    if ( ! MMSUCCESS( Result ) )
    {
        DPRINT1("Call to IOCTL_OPEN_WDMAUD failed with %d\n", GetLastError());
        return TranslateInternalMmResult(Result);
    }

    if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE || DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        DeviceInfo->DeviceState->bStart = TRUE;

        /* Start device */
        Result = WdmAudIoControl(DeviceInfo,
                                 0,
                                 NULL,
                                 DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                                 IOCTL_START_PLAYBACK : IOCTL_START_CAPTURE);

        if ( ! MMSUCCESS( Result ) )
        {
            DPRINT1("Call to %s failed with %d\n",
                    DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                    "IOCTL_START_PLAYBACK" : "IOCTL_START_CAPTURE",
                    GetLastError());
            return TranslateInternalMmResult(Result);
        }
    }

    DeviceInfo->DeviceState->Samples = WaveFormat->wBitsPerSample * WaveFormat->nChannels;

#ifdef RESAMPLING_ENABLED
    DeviceInfo->Buffer = WaveFormat;
    DeviceInfo->BufferSize = sizeof(WAVEFORMATEX);
#endif

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudCloseSoundDeviceByLegacy(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  PVOID Handle)
{
    MMRESULT Result;

    if ( OpenCount == 0 )
    {
        return MMSYSERR_NOERROR;
    }

    ASSERT( KernelHandle != INVALID_HANDLE_VALUE );

    DPRINT("WDMAUD - CloseWdmSoundDevice DeviceType %u\n", DeviceInfo->DeviceType);

    if (DeviceInfo->hDevice != KernelHandle)
    {
        if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE || DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
        {
            DeviceInfo->DeviceState->bStart = FALSE;

            /* Destroy I/O thread */
            Result = WdmAudDestroyCompletionThread(DeviceInfo);
            ASSERT(Result == MMSYSERR_NOERROR);

            /* Stop device */
            Result = WdmAudIoControl(DeviceInfo,
                                     0,
                                     NULL,
                                     DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                                     IOCTL_PAUSE_PLAYBACK : IOCTL_PAUSE_CAPTURE);

            if ( ! MMSUCCESS( Result ) )
            {
                DPRINT1("Call to %s failed with %d\n",
                        DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                        "IOCTL_PAUSE_PLAYBACK" : "IOCTL_PAUSE_CAPTURE",
                        GetLastError());
                return TranslateInternalMmResult(Result);
            }
        }

        /* Close device handle */
        Result = WdmAudIoControl(DeviceInfo,
                                 0,
                                 NULL,
                                 IOCTL_CLOSE_WDMAUD);

        if ( ! MMSUCCESS(Result) )
        {
            DPRINT1("Call to IOCTL_CLOSE_WDMAUD failed with %d\n", GetLastError());
            return TranslateInternalMmResult(Result);
        }
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
WdmAudSubmitWaveHeaderByLegacy(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_ PWAVEHDR WaveHeader)
{
    PWDMAUD_DEVICE_INFO LocalDeviceInfo;
    //PWAVEHDR_EXTENSION HeaderExtension;
    MMRESULT Result = MMSYSERR_NOERROR;
    OVERLAPPED Overlapped;
    DWORD Transferred = 0;
    BOOL IoResult;
    DWORD IoCtl;

    VALIDATE_MMSYS_PARAMETER( DeviceInfo );

    DPRINT("WDMAUD - SubmitWaveHeader DeviceType %u\n", DeviceInfo->DeviceType);

#ifdef RESAMPLING_ENABLED
    /* Resample the stream */
    Result = WdmAudResampleStream(DeviceInfo, WaveHeader);
    ASSERT( Result == MMSYSERR_NOERROR );
#endif

    ZeroMemory(&Overlapped, sizeof(OVERLAPPED));
    Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! Overlapped.hEvent )
        return Win32ErrorToMmResult(GetLastError());

    LocalDeviceInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WDMAUD_DEVICE_INFO));
    if (!LocalDeviceInfo)
    {
        /* No memory */
        DPRINT1("Failed to allocate WDMAUD_DEVICE_INFO structure\n");
        return MMSYSERR_NOMEM;
    }
#if 0
    HeaderExtension = (PWAVEHDR_EXTENSION)WaveHeader->reserved;
    HeaderExtension->DeviceInfo = LocalDeviceInfo;
#endif
    LocalDeviceInfo->DeviceType = DeviceInfo->DeviceType;
    LocalDeviceInfo->DeviceIndex = DeviceInfo->DeviceIndex;
    LocalDeviceInfo->hDevice = DeviceInfo->hDevice;
    LocalDeviceInfo->Buffer = WaveHeader;
    LocalDeviceInfo->BufferSize = sizeof(WAVEHDR);

    if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
        IoCtl = IOCTL_WRITEDATA;
    else if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
        IoCtl = IOCTL_READDATA;

    IoResult = DeviceIoControl(KernelHandle,
                               IoCtl,
                               LocalDeviceInfo,
                               sizeof(WDMAUD_DEVICE_INFO),
                               LocalDeviceInfo,
                               sizeof(WDMAUD_DEVICE_INFO),
                               &Transferred,
                               &Overlapped); // HeaderExtension->Overlapped
    HeapFree(GetProcessHeap(), 0, LocalDeviceInfo);

    if (!IoResult)
    {
        if (GetLastError() != ERROR_IO_PENDING)
        {
            DPRINT1("Call to %s failed with %d\n",
                    DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                    "IOCTL_WRITEDATA" : "IOCTL_READDATA",
                    GetLastError());
        }
        Result = Win32ErrorToMmResult(GetLastError());
    }
    DPRINT("Result %x\n", Result);
    DPRINT("GetLastError %d\n", GetLastError());
#if 0
    if (MMSUCCESS(Result))
    {
        /* Create I/O thread */
        Result = WdmAudCreateCompletionThread(DeviceInfo);
        if (!MMSUCCESS(Result))
        {
            /* Failed */
            DPRINT1("Failed to create sound thread with error %d\n", GetLastError());
            return TranslateInternalMmResult(Result);
        }
    }
#else
    /* Wait for I/O complete */
    WaitForSingleObject(Overlapped.hEvent, INFINITE);

    /* Complete current header */
    CompleteWaveHeader(DeviceInfo);

    /* Don't need this any more */
    CloseHandle(Overlapped.hEvent);
#endif

    DPRINT("Transferred %d bytes in Sync overlapped I/O\n", Transferred);

    return Result;
}

MMRESULT
WdmAudSetWaveStateByLegacy(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_ BOOL bStart)
{
    MMRESULT Result;
    DWORD IoCtl;

    DPRINT("WDMAUD - SetWaveState DeviceType %u\n", DeviceInfo->DeviceType);

    DeviceInfo->DeviceState->bStart = bStart;
#if 0
    if (bStart)
    {
        Result = WdmAudCreateCompletionThread(DeviceInfo);
        if (!MMSUCCESS(Result))
        {
            /* Failed */
            return TranslateInternalMmResult(Result);
        }
    }
#endif
    if (DeviceInfo->DeviceType == WAVE_IN_DEVICE_TYPE)
    {
        IoCtl = bStart ? IOCTL_START_CAPTURE : IOCTL_PAUSE_CAPTURE;
    }
    else if (DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE)
    {
        IoCtl = bStart ? IOCTL_START_PLAYBACK : IOCTL_PAUSE_PLAYBACK;
    }

    Result = WdmAudIoControl(DeviceInfo,
                             0,
                             NULL,
                             IoCtl);

    if ( ! MMSUCCESS( Result ) )
    {
        DPRINT1("Call to %s failed with %d\n",
                DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                (bStart ? "IOCTL_START_PLAYBACK" : "IOCTL_PAUSE_PLAYBACK") :
                (bStart ? "IOCTL_START_CAPTURE" : "IOCTL_PAUSE_CAPTURE"),
                GetLastError());
        return TranslateInternalMmResult(Result);
    }

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudResetStreamByLegacy(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  BOOL bStartReset)
{
    MMRESULT Result;
    DWORD IoCtl;

    DPRINT("WDMAUD - ResetWaveStream DeviceType %u\n", DeviceInfo->DeviceType);

    DeviceInfo->DeviceState->bReset = bStartReset;
    DeviceInfo->DeviceState->bStart = FALSE;

    IoCtl = DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
            IOCTL_RESET_PLAYBACK : IOCTL_RESET_CAPTURE;

    Result = WdmAudIoControl(DeviceInfo,
                             0,
                             NULL,
                             IoCtl);

    if ( ! MMSUCCESS(Result) )
    {
        DPRINT1("Call to %s failed with %d\n",
                DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                "IOCTL_RESET_PLAYBACK" : "IOCTL_RESET_CAPTURE",
                GetLastError());
        return TranslateInternalMmResult(Result);
    }

    Result = WdmAudDestroyCompletionThread(DeviceInfo);
    ASSERT(Result == MMSYSERR_NOERROR);

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudGetWavePositionByLegacy(
    _In_  PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_  MMTIME* Time)
{
    MMRESULT Result;
    DWORD Position;
    DWORD IoCtl;

    DPRINT("WDMAUD - GetWavePosition DeviceType %u\n", DeviceInfo->DeviceType);

    IoCtl = DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
            IOCTL_GETOUTPOS : IOCTL_GETINPOS;

    Result = WdmAudIoControl(DeviceInfo,
                             sizeof(DWORD),
                             &Position,
                             IoCtl);

    if ( ! MMSUCCESS(Result) )
    {
        DPRINT1("Call to %s failed with %d\n",
                DeviceInfo->DeviceType == WAVE_OUT_DEVICE_TYPE ?
                "IOCTL_GETOUTPOS" : "IOCTL_GETINPOS",
                GetLastError());
        return TranslateInternalMmResult(Result);
    }

    if (Time->wType == TIME_BYTES)
        Time->u.cb = Position;
    else if (Time->wType == TIME_SAMPLES)
        Time->u.sample = Position * 8 / DeviceInfo->DeviceState->Samples;

    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudQueryMixerInfoByLegacy(
    _In_ PWDMAUD_DEVICE_INFO DeviceInfo,
    _In_ DWORD DeviceId,
    _In_ UINT uMsg,
    _In_ LPVOID Parameter,
    _In_ DWORD Flags)
{
    MMRESULT Result;
    DWORD IoControlCode;

    DPRINT("WDMAUD - QueryMixerInfo: uMsg %x Flags %x\n", uMsg, Flags);

    DeviceInfo->DeviceIndex = DeviceId;
    DeviceInfo->DeviceType = MIXER_DEVICE_TYPE;
    DeviceInfo->Flags = Flags;

    switch(uMsg)
    {
        case MXDM_GETLINEINFO:
            IoControlCode = IOCTL_GETLINEINFO;
            break;
        case MXDM_GETLINECONTROLS:
            IoControlCode = IOCTL_GETLINECONTROLS;
            break;
        case MXDM_SETCONTROLDETAILS:
            IoControlCode = IOCTL_SETCONTROLDETAILS;
            break;
        case MXDM_GETCONTROLDETAILS:
            IoControlCode = IOCTL_GETCONTROLDETAILS;
            break;
        default:
            ASSERT(0);
            break;
    }

    Result = WdmAudIoControl(DeviceInfo,
                             sizeof(Parameter),
                             Parameter,
                             IoControlCode);

    if ( ! MMSUCCESS(Result) )
    {
        DPRINT1("Call to 0x%lx failed with %d\n", IoControlCode, GetLastError());
        return TranslateInternalMmResult(Result);
    }

    return MMSYSERR_NOERROR;
}
