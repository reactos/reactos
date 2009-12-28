#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
#include <ksmedia.h>
#include <mmsystem.h>
#include <mmreg.h>
#include "mmixer.h"

MIXER_CONTEXT MixerContext;
GUID CategoryGuid = {STATIC_KSCATEGORY_AUDIO};

PVOID Alloc(ULONG NumBytes)
{
    //printf("Alloc: %lu\n", NumBytes);
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NumBytes);
}

MIXER_STATUS
Close(HANDLE hDevice)
{
    //printf("Close: Handle %p\n", hDevice);
    if (CloseHandle(hDevice))
        return MM_STATUS_SUCCESS;
    else
        return MM_STATUS_UNSUCCESSFUL;
}

VOID
Free(PVOID Block)
{
    //printf("Free: %p\n", Block);
    HeapFree(GetProcessHeap(), 0, Block);
}

VOID
Copy(PVOID Src, PVOID Dst, ULONG NumBytes)
{
    //printf("Copy: Src %p Dst %p NumBytes %lu\n", Src, Dst, NumBytes);
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
        //wprintf(L" Failed to open %s Error %lu\n", DevicePath, GetLastError());
        return MM_STATUS_UNSUCCESSFUL;
    }
    wprintf(L"Open: %s hDevice %p\n", DevicePath, *hDevice);

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

    //printf("hMixer %p dwIoControlCode %lx lpInBuffer %p nInBufferSize %lu lpOutBuffer %p nOutBufferSize %lu lpBytesReturned %p\n",
    //        hMixer, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned);

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

            //printf("Control: Failed with %lu Transferred %lu\n", GetLastError(), Transferred);

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

    //printf("Transferred %lu bytes in Sync overlapped I/O\n", Transferred);

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
            printf("LastDevice\n");
            return MM_STATUS_NO_MORE_DEVICES;
        }
        printf("SetupDiEnumDeviceInterfaces failed with %lu\n", GetLastError());
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
        printf("SetupDiGetDeviceInterfaceDetailW failed with %lu\n", GetLastError());
        return MM_STATUS_UNSUCCESSFUL;
    }


    *OutKey = SetupDiOpenDeviceInterfaceRegKey(EnumContext, &InterfaceData, 0, KEY_READ);
     if ((HKEY)*OutKey == INVALID_HANDLE_VALUE)
     {
        printf("SetupDiOpenDeviceInterfaceRegKey failed with %lx\n", GetLastError());
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


int main(int argc, char**argv)
{
    MIXER_STATUS Status;
    HDEVINFO DeviceHandle;
    MIXERCAPSW MixCaps1, MixCaps2;
    ULONG Index, SubIndex;
    HANDLE hMixer2;
    HMIXER hMixer1;
    MIXERLINEW MixerLine1, MixerLine2;
    MIXERLINECONTROLSW Controls1, Controls2;

    ZeroMemory(&MixerContext, sizeof(MIXER_CONTEXT));

    DeviceHandle = SetupDiGetClassDevs(&CategoryGuid,
                                       NULL,
                                       NULL,
                                       DIGCF_DEVICEINTERFACE/*|DIGCF_PRESENT */);
    if (DeviceHandle == INVALID_HANDLE_VALUE)
    {
        printf("SetupDiGetClassDevs failed with %lx\n", GetLastError());
        return 0;
    }

    printf("DeviceHandle %p\n", DeviceHandle);

    MixerContext.SizeOfStruct = sizeof(MIXER_CONTEXT);
    MixerContext.Alloc = Alloc;
    MixerContext.Close = Close;
    MixerContext.Control = Control;
    MixerContext.Copy = Copy;
    MixerContext.Free = Free;
    MixerContext.Open = Open;
    MixerContext.OpenKey = OpenKey;
    MixerContext.CloseKey = CloseKey;
    MixerContext.QueryKeyValue = QueryKeyValue;

    Status = MMixerInitialize(&MixerContext, Enum, (PVOID)DeviceHandle);

    printf("Status %x\n", Status);
    printf("NumberOfMixers %lu mixerGetNumDevs %u\n", MMixerGetCount(&MixerContext), mixerGetNumDevs());

    for(Index = 0; Index < MMixerGetCount(&MixerContext); Index++)
    {
        mixerGetDevCapsW(Index, &MixCaps1, sizeof(MIXERCAPSW));
        wprintf(L"WINM: cDestination %u fdwSupport %lx szPname %s vDriverVersion %u wMid %x wPid %x\n", MixCaps1.cDestinations, MixCaps1.fdwSupport, MixCaps1.szPname, MixCaps1.vDriverVersion, MixCaps1.wMid, MixCaps1.wPid);
        MMixerGetCapabilities(&MixerContext, Index, &MixCaps2);
        wprintf(L"MMIX: cDestination %u fdwSupport %lx szPname %s vDriverVersion %u wMid %x wPid %x\n", MixCaps2.cDestinations, MixCaps2.fdwSupport, MixCaps2.szPname, MixCaps2.vDriverVersion, MixCaps2.wMid, MixCaps2.wPid);

        mixerOpen(&hMixer1, Index, 0, 0, MIXER_OBJECTF_HMIXER);
        MMixerOpen(&MixerContext, Index, NULL, NULL, &hMixer2);

        ZeroMemory(&MixerLine1, sizeof(MIXERLINEW));
        ZeroMemory(&MixerLine2, sizeof(MIXERLINEW));
        MixerLine1.cbStruct = sizeof(MIXERLINEW);
        MixerLine2.cbStruct = sizeof(MIXERLINEW);
        mixerGetLineInfoW((HMIXEROBJ)hMixer1, &MixerLine1, MIXER_GETLINEINFOF_DESTINATION);
        MMixerGetLineInfo(&MixerContext, hMixer2, MIXER_GETLINEINFOF_DESTINATION, &MixerLine2);

        wprintf(L"WINM: dwDestination %lx dwSource %lx dwLineID %lx dwUser %lx dwComponentType %lx cChannels %lx cConnections %lx cControls %lx szShortName %s szName %s\n\n",
                MixerLine1.dwDestination, MixerLine1.dwSource, MixerLine1.dwLineID, MixerLine1.dwUser, MixerLine1.dwComponentType, MixerLine1.cChannels, MixerLine1.cConnections, MixerLine1.cControls, MixerLine1.szShortName, MixerLine1.szName);

        wprintf(L"MMIX: dwDestination %lx dwSource %lx dwLineID %lx dwUser %lx dwComponentType %lx cChannels %lx cConnections %lx cControls %lx szShortName %s szName %s\n\n",
                MixerLine2.dwDestination, MixerLine2.dwSource, MixerLine2.dwLineID, MixerLine2.dwUser, MixerLine2.dwComponentType, MixerLine2.cChannels, MixerLine2.cConnections, MixerLine2.cControls, MixerLine2.szShortName, MixerLine2.szName);

        Controls1.cbStruct = sizeof(MIXERLINECONTROLSW);
        Controls2.cbStruct = sizeof(MIXERLINECONTROLSW);

        Controls1.cbmxctrl = sizeof(MIXERCONTROL);
        Controls2.cbmxctrl = sizeof(MIXERCONTROL);

        Controls1.cControls = MixerLine1.cControls;
        Controls2.cControls = MixerLine2.cControls;

        Controls1.dwLineID = MixerLine1.dwLineID;
        Controls2.dwLineID = MixerLine2.dwLineID;



        Controls1.pamxctrl = (LPMIXERCONTROLW)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MIXERCONTROLW) * Controls1.cControls);
        Controls2.pamxctrl = (LPMIXERCONTROLW)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MIXERCONTROLW) * Controls2.cControls);

        for(SubIndex = 0; SubIndex < Controls1.cControls; SubIndex++)
            Controls1.pamxctrl[SubIndex].cbStruct = sizeof(MIXERCONTROLW);

        for(SubIndex = 0; SubIndex < Controls2.cControls; SubIndex++)
            Controls2.pamxctrl[SubIndex].cbStruct = sizeof(MIXERCONTROLW);

        mixerGetLineControlsW((HMIXEROBJ)hMixer1, &Controls1, MIXER_GETLINECONTROLSF_ALL);
        MMixerGetLineControls(&MixerContext, hMixer2, MIXER_GETLINECONTROLSF_ALL, &Controls2);

        wprintf(L"----------------------------------------\n");
        for(SubIndex = 0; SubIndex < Controls1.cControls || SubIndex  < Controls2.cControls; SubIndex++)
        {
            if (SubIndex < Controls1.cControls)
            {
                wprintf(L"WINM: Index %d dwControlID %lx dwControlType %lx fdwControl %lx cMultipleItems %lx szName %s szShortName %s \n", SubIndex, Controls1.pamxctrl[SubIndex].dwControlID, Controls1.pamxctrl[SubIndex].dwControlType, Controls1.pamxctrl[SubIndex].fdwControl, Controls1.pamxctrl[SubIndex].cMultipleItems, Controls1.pamxctrl[SubIndex].szName, Controls1.pamxctrl[SubIndex].szShortName);
            }

            if (SubIndex < Controls2.cControls)
            {
                wprintf(L"MMIX: Index %d dwControlID %lx dwControlType %lx fdwControl %lx cMultipleItems %lx szName %s szShortName %s \n", SubIndex, Controls2.pamxctrl[SubIndex].dwControlID, Controls2.pamxctrl[SubIndex].dwControlType, Controls2.pamxctrl[SubIndex].fdwControl, Controls2.pamxctrl[SubIndex].cMultipleItems, Controls2.pamxctrl[SubIndex].szName, Controls2.pamxctrl[SubIndex].szShortName);
            }

        }
        wprintf(L"----------------------------------------\n");


        wprintf(L"=======================\n");
    }
    return 0;
}
