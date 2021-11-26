/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/directx/dsound_new/devicelist.c
 * PURPOSE:         Enumeration of audio devices
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#include "precomp.h"

ULONG
GetPinIdFromFilter(
    LPFILTERINFO Filter,
    BOOL bCapture,
    ULONG Offset)
{
    ULONG Index;

    for(Index = Offset; Index < Filter->PinCount; Index++)
    {
        if (Filter->Pin[Index] == PIN_TYPE_PLAYBACK && !bCapture)
            return Index;

        if (Filter->Pin[Index] == PIN_TYPE_RECORDING && bCapture)
            return Index;
    }
    return ULONG_MAX;
}


DWORD
OpenDeviceList(
    IN LPGUID InterfaceGuid,
    OUT HDEVINFO * OutHandle)
{
    HDEVINFO DeviceHandle;

    DeviceHandle = SetupDiGetClassDevs(InterfaceGuid,
                                       NULL,
                                       NULL,
                                       DIGCF_DEVICEINTERFACE); //DIGCF_PRESENT

    /* check for success */
    if (DeviceHandle == INVALID_HANDLE_VALUE)
    {
        /* failed to create device list */
        return GetLastError();
    }

    /* store result */
    *OutHandle = DeviceHandle;

    return ERROR_SUCCESS;
}

BOOL
CloseDeviceList(
    HDEVINFO Handle)
{
    return SetupDiDestroyDeviceInfoList(Handle);
}

BOOL
GetDeviceListInterfaces(
    HDEVINFO DeviceHandle,
    IN LPGUID InterfaceGuid,
    LPFILTERINFO *OutPath)
{
    ULONG Length, Index = 0;
    SP_DEVICE_INTERFACE_DATA InterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DetailData;
    SP_DEVINFO_DATA DeviceData;
    LPFILTERINFO LastDevice = NULL, RootDevice = NULL, CurDevice;
    BOOL Result;


    do
    {
        InterfaceData.cbSize = sizeof(InterfaceData);
        InterfaceData.Reserved = 0;

        /* query device interface */
        Result = SetupDiEnumDeviceInterfaces(DeviceHandle,
                                NULL,
                                InterfaceGuid,
                                Index,
                                &InterfaceData);

        if (!Result)
        {
            /* failed */
            DPRINT("SetupDiEnumDeviceInterfaces Index %u failed with %lx\n", Index, GetLastError());
            break;
        }

        /* allocate device interface struct */
        Length = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W) + MAX_PATH * sizeof(WCHAR);
        DetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)HeapAlloc(GetProcessHeap(),
                                                                 HEAP_ZERO_MEMORY,
                                                                 Length);

        if (!DetailData)
        {
            /* insufficient memory */
            break;
        }

        /* initialize device interface detail struct */
        DetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

        DeviceData.cbSize = sizeof(DeviceData);
        DeviceData.Reserved = 0;

        Result = SetupDiGetDeviceInterfaceDetailW(DeviceHandle,
                                                  &InterfaceData,
                                                  DetailData,
                                                  Length,
                                                  NULL,
                                                  &DeviceData);

       if (!Result)
       {
           /* failed */
           DPRINT("SetupDiGetDeviceInterfaceDetail failed with %x\n", GetLastError());
           HeapFree(GetProcessHeap(), 0, DetailData);

           break;
       }

       /* allocate device path struct */
       CurDevice = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(FILTERINFO));
       if (!CurDevice)
       {
           /* no memory */
           HeapFree(GetProcessHeap(), 0, DetailData);
           break;
       }

       /* store device path */
       CopyMemory(&CurDevice->DeviceData, &DeviceData, sizeof(SP_DEVINFO_DATA));
       wcscpy(CurDevice->DevicePath, DetailData->DevicePath);
       CurDevice->MappedId[0] = ULONG_MAX;
       CurDevice->MappedId[1] = ULONG_MAX;

       DPRINT("DevicePath %S\n", CurDevice->DevicePath);

       if (!RootDevice)
           RootDevice = CurDevice;

       if (LastDevice)
       {
           LastDevice->lpNext = CurDevice;
       }

       /* set as last device */
       LastDevice = CurDevice;

        /* free device interface struct */
        HeapFree(GetProcessHeap(), 0, DetailData);

       /* increment device interface index */
       Index++;
    }while(TRUE);

    /* store result */
    *OutPath = RootDevice;

    if (!RootDevice)
        return FALSE;


    return TRUE;
}

DWORD
OpenDeviceKey(
    HDEVINFO Handle,
    PSP_DEVINFO_DATA  FILTERINFOData,
    DWORD KeyType,
    REGSAM DesiredAccess,
    OUT HKEY * OutKey)
{
    HKEY hKey;

    /* try open device registry key */
    hKey = SetupDiOpenDevRegKey(Handle, FILTERINFOData, DICS_FLAG_CONFIGSPECIFIC, 0, KeyType, DesiredAccess);

    if (hKey == INVALID_HANDLE_VALUE)
        return GetLastError();

    /* store result */
    *OutKey = hKey;

    return ERROR_SUCCESS;
}

VOID
FindAudioFilterPins(
    LPFILTERINFO CurInfo,
    OUT PULONG WaveInPins,
    OUT PULONG WaveOutPins)
{
    ULONG Index;
    KSPIN_COMMUNICATION Communication;
    KSPIN_DATAFLOW DataFlow;

    *WaveInPins = 0;
    *WaveOutPins = 0;

    /* traverse all pins */
    for(Index = 0; Index < CurInfo->PinCount; Index++)
    {
        if (GetFilterPinCommunication(CurInfo->hFilter, Index, &Communication) == ERROR_SUCCESS &&
            GetFilterPinDataFlow(CurInfo->hFilter, Index, &DataFlow) == ERROR_SUCCESS)
        {
            if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_IN)
            {
                /* found a wave out device */
                CurInfo->Pin[Index] = PIN_TYPE_PLAYBACK;
                (*WaveOutPins)++;
            }
            else if (Communication == KSPIN_COMMUNICATION_SINK && DataFlow == KSPIN_DATAFLOW_OUT)
            {
                /* found a wave in device */
                CurInfo->Pin[Index] = PIN_TYPE_RECORDING;
                (*WaveInPins)++;
            }
            else
            {
                /* bridge pin / topology pin etc */
                CurInfo->Pin[Index] = PIN_TYPE_NONE;
            }
        }
        else
        {
            /* bridge pin / topology pin etc */
            CurInfo->Pin[Index] = PIN_TYPE_NONE;
        }
    }
}

BOOL
FindWinMMDeviceIndex(
    LPFILTERINFO CurInfo,
    BOOL bRecord)
{
    ULONG DeviceCount, Index;
    WCHAR Buffer[MAX_PATH];
    DWORD Size, dwResult;

    if (bRecord)
        DeviceCount = waveInGetNumDevs();
    else
        DeviceCount = waveOutGetNumDevs();

    /* sanity check */
    //ASSERT(DeviceCount);

    for(Index = 0; Index < DeviceCount; Index++)
    {
        Size = 0;

        /* query device interface size */
        if (bRecord)
            dwResult = waveInMessage(UlongToHandle(Index), DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&Size, 0);
        else
            dwResult = waveOutMessage(UlongToHandle(Index), DRV_QUERYDEVICEINTERFACESIZE, (DWORD_PTR)&Size, 0);

        if (dwResult != MMSYSERR_NOERROR)
        {
            DPRINT("Failed DRV_QUERYDEVICEINTERFACESIZE with %lx bRecord %u Index %u\n", dwResult, bRecord, Index);
            continue;
        }

        /* sanity check */
        ASSERT(Size < MAX_PATH);

        /* now get the device interface string */
        if (bRecord)
            dwResult = waveInMessage(UlongToHandle(Index), DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)Buffer, MAX_PATH);
        else
            dwResult = waveOutMessage(UlongToHandle(Index), DRV_QUERYDEVICEINTERFACE, (DWORD_PTR)Buffer, MAX_PATH);

        if (dwResult != MMSYSERR_NOERROR)
        {
            DPRINT("Failed DRV_QUERYDEVICEINTERFACE with %lx bRecord %u Index %u\n", dwResult, bRecord, Index);
            continue;
        }

        if (!wcsicmp(CurInfo->DevicePath, Buffer))
        {
            if (bRecord)
                CurInfo->MappedId[0] = Index;
            else
                CurInfo->MappedId[1] = Index;

            return TRUE;
        }
    }

    DPRINT1("Failed to find device %ws bRecord %u Count %u\n", CurInfo->DevicePath, bRecord, DeviceCount);

    // HACK
    if (bRecord)
        CurInfo->MappedId[0] = 0;
    else
        CurInfo->MappedId[1] = 0;


    return TRUE;
}

HRESULT
EnumerateAudioFilter(
    LPFILTERINFO CurInfo,
    OUT PULONG WaveInCount,
    OUT PULONG WaveOutCount)
{
    DWORD Status;
    ULONG PinCount, WaveInPins, WaveOutPins;

    /* first step open filter */
    Status = OpenFilter((LPCWSTR)CurInfo->DevicePath, &CurInfo->hFilter);
    if (Status != ERROR_SUCCESS)
    {
        DPRINT("Failed to open filter with %lx Path %ws\n", Status, CurInfo->DevicePath);
        return E_FAIL;
    }

    /* get filter pin count */
    Status = GetFilterPinCount(CurInfo->hFilter, &PinCount);
    if (Status != ERROR_SUCCESS)
    {
        DPRINT("Failed to get pin count with %lx\n", Status);
        return E_FAIL;
    }

    /* sanity check */
    ASSERT(PinCount);

    /* store pin count */
    CurInfo->PinCount = PinCount;

    /* now allocate an pin array */
    CurInfo->Pin = HeapAlloc(GetProcessHeap(), 0, PinCount * sizeof(ULONG));
    if (!CurInfo->Pin)
    {
        /* no memory */
        return E_FAIL;
    }

    /* no try to find playback / recording pins */
    FindAudioFilterPins(CurInfo, &WaveInPins, &WaveOutPins);

    DPRINT("WaveInPins %u WaveOutPins %u %S\n", WaveInPins, WaveOutPins, CurInfo->DevicePath);

    if (WaveOutPins)
    {
        /* create a unique guid for this playback device */
        if (FindWinMMDeviceIndex(CurInfo, TRUE))
        {
            (*WaveOutCount)++;
            INIT_GUID(CurInfo->DeviceGuid[0], 0xbd6dd71a, 0x3deb, 0x11d1, 0xb1, 0x71, 0x00, 0xc0, 0x4f, 0xc2, 0x00, 0x00 + *WaveInCount);
        }
    }


    if (WaveInPins)
    {
        if (FindWinMMDeviceIndex(CurInfo, FALSE))
        {
            /* create a unique guid for this record device */
            (*WaveInCount)++;
            INIT_GUID(CurInfo->DeviceGuid[1], 0xbd6dd71b, 0x3deb, 0x11d1, 0xb1, 0x71, 0x00, 0xc0, 0x4f, 0xc2, 0x00, 0x00 + *WaveOutCount);
        }
    }

    return S_OK;
}


HRESULT
EnumAudioDeviceInterfaces(
    LPFILTERINFO *OutRootInfo)
{
    HDEVINFO hList;
    DWORD Status;
    HRESULT hResult;
    ULONG WaveOutCount, WaveInCount;
    GUID AudioDeviceGuid = {STATIC_KSCATEGORY_AUDIO};
    LPFILTERINFO CurInfo;

    /* try open the device list */
    Status = OpenDeviceList(&AudioDeviceGuid, &hList);

    if (Status != ERROR_SUCCESS)
    {
        DPRINT1("OpenDeviceList failed with %lx\n", Status);
        return E_FAIL;
    }

    if (!GetDeviceListInterfaces(hList, &AudioDeviceGuid, OutRootInfo))
    {
        DPRINT1("No devices found\n");
        CloseDeviceList(hList);
        return S_FALSE;
    }

    /* sanity check */
    ASSERT(*OutRootInfo);

    CurInfo = *OutRootInfo;

    WaveOutCount = 0;
    WaveInCount = 0;

    /* now check all audio filters */
    while(CurInfo)
    {
        /* now check details of the audio filter */
        hResult = EnumerateAudioFilter(CurInfo, &WaveInCount, &WaveOutCount);

        if (hResult != S_OK)
        {
           DPRINT1("EnumerateAudioFilter failed with %lx\n", Status);
           break;
        }

        /* move to next filter */
        CurInfo = CurInfo->lpNext;
    }

    /* close device list */
    CloseDeviceList(hList);

    /* done */
    return hResult;
}

BOOL
FindDeviceByMappedId(
    IN ULONG DeviceID,
    LPFILTERINFO *Filter,
    BOOL bPlayback)
{
    LPFILTERINFO CurInfo;
    if (!RootInfo)
        return FALSE;

    /* get first entry */
    CurInfo = RootInfo;

    while(CurInfo)
    {
        if ((bPlayback && CurInfo->MappedId[1] == DeviceID) ||
            (!bPlayback && CurInfo->MappedId[0] == DeviceID))
        {
            /* found filter */
            *Filter = CurInfo;
            return TRUE;
        }

        CurInfo = CurInfo->lpNext;
    }
    return FALSE;
}

BOOL
FindDeviceByGuid(
    LPCGUID pGuidSrc,
    LPFILTERINFO *Filter)
{
    LPFILTERINFO CurInfo;
    if (!RootInfo)
        return FALSE;

    /* get first entry */
    CurInfo = RootInfo;

    while(CurInfo)
    {
        if (IsEqualGUID(&CurInfo->DeviceGuid[0], pGuidSrc) ||
            IsEqualGUID(&CurInfo->DeviceGuid[1], pGuidSrc))
        {
            /* found filter */
            *Filter = CurInfo;
            return TRUE;
        }

        CurInfo = CurInfo->lpNext;
    }

    return FALSE;
}
