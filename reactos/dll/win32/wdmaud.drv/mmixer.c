/*
 * PROJECT:     ReactOS Sound System
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/wdmaud.drv/mmixer.c
 *
 * PURPOSE:     WDM Audio Mixer API (User-mode part)
 * PROGRAMMERS: Johannes Anderwald
 */

#include "wdmaud.h"


PVOID Alloc(ULONG NumBytes);
MIXER_STATUS Close(HANDLE hDevice);
VOID Free(PVOID Block);
VOID Copy(PVOID Src, PVOID Dst, ULONG NumBytes);
MIXER_STATUS Open(IN LPWSTR DevicePath, OUT PHANDLE hDevice);
MIXER_STATUS Control(IN HANDLE hMixer, IN ULONG dwIoControlCode, IN PVOID lpInBuffer, IN ULONG nInBufferSize, OUT PVOID lpOutBuffer, ULONG nOutBufferSize, PULONG lpBytesReturned);
MIXER_STATUS Enum(IN  PVOID EnumContext, IN  ULONG DeviceIndex, OUT LPWSTR * DeviceName, OUT PHANDLE OutHandle);

MIXER_CONTEXT MixerContext =
{
    sizeof(MIXER_CONTEXT),
    NULL,
    Alloc,
    Control,
    Free,
    Open,
    Close,
    Copy
};

GUID CategoryGuid = {STATIC_KSCATEGORY_AUDIO};

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
    CopyMemory(Src, Dst, NumBytes);
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
    OUT PHANDLE OutHandle)
{
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    SP_DEVINFO_DATA DeviceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DetailData;
    BOOL Result;
    DWORD Length;

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
        return MM_STATUS_UNSUCCESSFUL;
    }

    // copy path
    *DeviceName = (LPWSTR)&DetailData->DevicePath[0];
    return Open(DetailData->DevicePath, OutHandle);
}

BOOL
WdmAudInitUserModeMixer()
{
    HDEVINFO DeviceHandle;
    MIXER_STATUS Status;

    /* create a device list */
    DeviceHandle = SetupDiGetClassDevs(&CategoryGuid,
                                       NULL,
                                       NULL,
                                       DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);

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

    /* completed successfully */
    return TRUE;
}

ULONG
WdmAudGetMixerCount()
{
    /* return number of mixers available */
    return MMixerGetCount(&MixerContext);
}

MMRESULT
WdmAudGetMixerCapabilties(
    IN ULONG DeviceId, 
    LPMIXERCAPSW Capabilities)
{
    if (MMixerGetCapabilities(&MixerContext, DeviceId, Capabilities) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_BADDEVICEID;
}

MMRESULT
WdmAudCloseMixer(
    IN HMIXER Handle,
    IN HANDLE hNotifyEvent)
{
    /* FIXME */
    return MMSYSERR_NOERROR;
}

MMRESULT
WdmAudOpenMixer(
    IN PHANDLE hMixer,
    IN ULONG DeviceId, 
    IN HANDLE hNotifyEvent)
{
    if (MMixerOpen(&MixerContext, DeviceId, hNotifyEvent, NULL /* FIXME */, hMixer) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_BADDEVICEID;
}

MMRESULT
WdmAudGetLineInfo(
    IN HANDLE hMixer,
    IN LPMIXERLINE MixLine,
    IN ULONG Flags)
{
    if (MMixerGetLineInfo(&MixerContext, hMixer, Flags, MixLine)  == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudGetLineControls(
    IN HANDLE hMixer,
    IN LPMIXERLINECONTROLSW MixControls,
    IN ULONG Flags)
{
    if (MMixerGetLineControls(&MixerContext, hMixer, Flags, MixControls) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;
}

MMRESULT
WdmAudSetControlDetails(
    IN HANDLE hMixer,
    IN LPMIXERCONTROLDETAILS MixDetails,
    IN ULONG Flags)
{
    if (MMixerSetControlDetails(&MixerContext, hMixer, Flags, MixDetails) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;

}

MMRESULT
WdmAudGetControlDetails(
    IN HANDLE hMixer,
    IN LPMIXERCONTROLDETAILS MixDetails,
    IN ULONG Flags)
{
    if (MMixerGetControlDetails(&MixerContext, hMixer, Flags, MixDetails) == MM_STATUS_SUCCESS)
        return MMSYSERR_NOERROR;

    return MMSYSERR_ERROR;
}
