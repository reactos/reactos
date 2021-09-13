/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Configuration of network devices
 * FILE:            dll/win32/streamci/streamci.c
 * PURPOSE:         Streaming device class installer
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */

#include "precomp.h"

DWORD
PerformIO(IN HANDLE hDevice,
          IN DWORD dwCtlCode,
          IN LPVOID lpBufferIn,
          IN DWORD dwBufferSizeIn,
          OUT LPVOID lpBufferOut,
          OUT DWORD dwBufferSizeOut,
          OUT LPDWORD lpNumberBytes)
{
    OVERLAPPED overlapped;
    DWORD dwResult;

    ZeroMemory(&overlapped, sizeof(OVERLAPPED));
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!overlapped.hEvent)
    {
        // failed to init event
        return GetLastError();
    }

    if (DeviceIoControl(hDevice, dwCtlCode, lpBufferIn, dwBufferSizeIn, lpBufferOut, dwBufferSizeOut, lpNumberBytes, &overlapped))
    {
        dwResult = ERROR_SUCCESS;
    }
    else if (GetLastError() == ERROR_IO_PENDING)
    {
        if (GetOverlappedResult(hDevice, &overlapped, lpNumberBytes, TRUE))
        {
            dwResult = ERROR_SUCCESS;
        }
        else
        {
            dwResult = GetLastError();
        }
    }
    else
    {
        dwResult = GetLastError();
    }
    CloseHandle(overlapped.hEvent);
    return dwResult;
}

DWORD
InstallSoftwareDeviceInterface(IN LPGUID DeviceId,
                               IN LPGUID InterfaceId,
                               IN LPWSTR ReferenceString)
{
    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    GUID SWBusGuid = {STATIC_BUSID_SoftwareDeviceEnumerator};
    PSP_DEVICE_INTERFACE_DETAIL_DATA_W DeviceInterfaceDetailData;
    HANDLE hDevice;
    PSWENUM_INSTALL_INTERFACE InstallInterface;
    DWORD dwResult;

    hDevInfo = SetupDiGetClassDevsW(&SWBusGuid, NULL, NULL,  DIGCF_DEVICEINTERFACE| DIGCF_PRESENT);
    if (!hDevInfo)
    {
        // failed
        return GetLastError();
    }

    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &SWBusGuid, 0, &DeviceInterfaceData))
    {
        // failed
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return GetLastError();
    }

    DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA_W)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR) + sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W));
    if (!DeviceInterfaceDetailData)
    {
        // failed
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return GetLastError();
    }

    DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
    if (!SetupDiGetDeviceInterfaceDetailW(hDevInfo,  &DeviceInterfaceData, DeviceInterfaceDetailData,MAX_PATH * sizeof(WCHAR) + sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W), NULL, NULL))
    {
        // failed
        HeapFree(GetProcessHeap(), 0, DeviceInterfaceDetailData);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return GetLastError();
    }

    hDevice = CreateFileW(DeviceInterfaceDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED|FILE_ATTRIBUTE_NORMAL, NULL);
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        // failed
        HeapFree(GetProcessHeap(), 0, DeviceInterfaceDetailData);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return GetLastError();
    }

    InstallInterface  = (PSWENUM_INSTALL_INTERFACE)HeapAlloc(GetProcessHeap(), 0, sizeof(SWENUM_INSTALL_INTERFACE) + wcslen(ReferenceString) * sizeof(WCHAR));
    if (!InstallInterface)
    {
        // failed
        CloseHandle(hDevice);
        HeapFree(GetProcessHeap(), 0, DeviceInterfaceDetailData);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return GetLastError();
    }

    // init install interface param
    InstallInterface->DeviceId = *DeviceId;
    InstallInterface->InterfaceId = *InterfaceId;
    wcscpy(InstallInterface->ReferenceString, ReferenceString);

    PerformIO(hDevice, IOCTL_SWENUM_INSTALL_INTERFACE, InstallInterface, sizeof(SWENUM_INSTALL_INTERFACE) + wcslen(ReferenceString) * sizeof(WCHAR), NULL, 0, NULL);
    dwResult = HeapFree(GetProcessHeap(), 0, InstallInterface);

    CloseHandle(hDevice);
    HeapFree(GetProcessHeap(), 0, DeviceInterfaceDetailData);
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return dwResult;
}

DWORD
InstallSoftwareDeviceInterfaceInf(IN LPWSTR InfName,
                                  IN LPWSTR SectionName)
{
    HDEVINFO hDevInfo;
    HINF hInf;
    HKEY hKey;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    GUID SWBusGuid = {STATIC_BUSID_SoftwareDeviceEnumerator};

    hDevInfo = SetupDiGetClassDevsW(&SWBusGuid, NULL, NULL, 0);
    if (!hDevInfo)
    {
        // failed
        return GetLastError();
    }

    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &SWBusGuid, 0, &DeviceInterfaceData))
    {
        // failed
        return GetLastError();
    }

    hInf = SetupOpenInfFileW(InfName, NULL, INF_STYLE_WIN4, NULL);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return GetLastError();
    }

    //
    // FIXME check if interface is already installed
    //

    hKey = SetupDiCreateDeviceInterfaceRegKeyW(hDevInfo, &DeviceInterfaceData, 0, KEY_ALL_ACCESS, hInf, SectionName);

    SetupCloseInfFile(hInf);
    SetupDiDestroyDeviceInfoList(hDevInfo);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        RegCloseKey(hKey);
    }
    return ERROR_SUCCESS;
}


VOID
WINAPI
StreamingDeviceSetupW(IN HWND hwnd,
                     IN HINSTANCE hinst,
                     IN LPWSTR lpszCmdLine,
                     IN int nCmdShow)
{
    DWORD Length, dwResult;
    LPWSTR pCmdLine;
    LPWSTR pStr;
    GUID Guids[2];
    WCHAR DevicePath[MAX_PATH];
    HRESULT hResult;
    DWORD Index;

    Length = (wcslen(lpszCmdLine) + 1) * sizeof(WCHAR);

    pCmdLine = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Length);
    if (pCmdLine == NULL)
    {
        // no memory
        return;
    }

    hResult = StringCbCopyExW(pCmdLine, Length, lpszCmdLine, NULL, NULL, STRSAFE_NULL_ON_FAILURE);
    if (hResult != S_OK)
    {
        // failed
        HeapFree(GetProcessHeap(), 0, pCmdLine);
        return;
    }

    pStr = wcstok(pCmdLine, L",\t\"");
    Index = 0;
    do
    {
        if (pStr == NULL)
        {
            // invalid parameter
            HeapFree(GetProcessHeap(), 0, pCmdLine);
            return;
        }

        hResult = IIDFromString(pStr, &Guids[Index]);
        if (hResult != S_OK)
        {
            // invalid parameter
            HeapFree(GetProcessHeap(), 0, pCmdLine);
            return;
        }

        Index++;
        pStr = wcstok(NULL, L",\t\"");


    }while(Index < 2);


    dwResult = InstallSoftwareDeviceInterface(&Guids[0], &Guids[1], pStr);
    if (dwResult == ERROR_SUCCESS)
    {
        pStr = wcstok(NULL, L",\t\"");
        if (pStr != NULL)
        {
            wcscpy(DevicePath, pStr);
            pStr = wcstok(NULL, L",\t\"");
            if (pStr != NULL)
            {
                dwResult = InstallSoftwareDeviceInterfaceInf(DevicePath, pStr);
            }
        }
    }
}