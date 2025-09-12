/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\batt\batt.c
 * PURPOSE:     Battery Class installers
 * PROGRAMMERS: Copyright 2010 Eric Kohl
 */


#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <setupapi.h>

#include <initguid.h>
#include <devguid.h>

#define NDEBUG
#include <debug.h>

static
DWORD
InstallCompositeBattery(
    _In_ HDEVINFO DeviceInfoSet,
    _In_opt_ PSP_DEVINFO_DATA DeviceInfoData,
    _In_ PSP_DEVINSTALL_PARAMS_W DeviceInstallParams)
{
    WCHAR szDeviceId[32];
    SP_DRVINFO_DATA DriverInfoData;
    HDEVINFO NewDeviceInfoSet = INVALID_HANDLE_VALUE;
    PSP_DEVINFO_DATA NewDeviceInfoData = NULL;
    BOOL  bDeviceRegistered = FALSE, bHaveDriverInfoList = FALSE;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("InstallCompositeBattery(%p %p %p)\n",
           DeviceInfoSet, DeviceInfoData, DeviceInstallParams);

    NewDeviceInfoSet = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_SYSTEM,
                                                   DeviceInstallParams->hwndParent);
    if (NewDeviceInfoSet == INVALID_HANDLE_VALUE)
    {
        DPRINT1("SetupDiCreateDeviceInfoList() failed (Error %lu)\n", GetLastError());
        return GetLastError();
    }
 
    NewDeviceInfoData = HeapAlloc(GetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  sizeof(SP_DEVINFO_DATA));
    if (NewDeviceInfoData == NULL)
    {
        dwError = ERROR_OUTOFMEMORY;
        goto done;
    }

    NewDeviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiCreateDeviceInfoW(NewDeviceInfoSet,
                                  L"Root\\COMPOSITE_BATTERY\\0000",
                                  &GUID_DEVCLASS_SYSTEM,
                                  NULL,
                                  DeviceInstallParams->hwndParent,
                                  0,
                                  NewDeviceInfoData))
    {
        dwError = GetLastError();
        if (dwError == ERROR_DEVINST_ALREADY_EXISTS)
        {
            dwError = ERROR_SUCCESS;
            goto done;
        }

        DPRINT1("SetupDiCreateDeviceInfoW() failed (Error %lu 0x%08lx)\n", dwError, dwError);
        goto done;
    }

    if (!SetupDiRegisterDeviceInfo(NewDeviceInfoSet,
                                   NewDeviceInfoData,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL))
    {
        dwError = GetLastError();
        DPRINT1("SetupDiRegisterDeviceInfo() failed (Error %lu 0x%08lx)\n", dwError, dwError);
        goto done;
    }

    bDeviceRegistered = TRUE;

    ZeroMemory(szDeviceId, sizeof(szDeviceId));
    wcscpy(szDeviceId, L"COMPOSITE_BATTERY");

    if (!SetupDiSetDeviceRegistryPropertyW(NewDeviceInfoSet,
                                           NewDeviceInfoData,
                                           SPDRP_HARDWAREID,
                                           (PBYTE)szDeviceId,
                                           (wcslen(szDeviceId) + 2) * sizeof(WCHAR)))
    {
        dwError = GetLastError();
        DPRINT1("SetupDiSetDeviceRegistryPropertyW() failed (Error %lu 0x%08lx)\n", dwError, dwError);
        goto done;
    }

    if (!SetupDiBuildDriverInfoList(NewDeviceInfoSet,
                                    NewDeviceInfoData,
                                    SPDIT_COMPATDRIVER))
    {
        dwError = GetLastError();
        DPRINT1("SetupDiBuildDriverInfoList() failed (Error %lu 0x%08lx)\n", dwError, dwError);
        goto done;
    }

    bHaveDriverInfoList = TRUE;

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (!SetupDiEnumDriverInfo(NewDeviceInfoSet,
                               NewDeviceInfoData,
                               SPDIT_COMPATDRIVER,
                               0,
                               &DriverInfoData))
    {
        dwError = GetLastError();
        DPRINT1("SetupDiEnumDriverInfo() failed (Error %lu 0x%08lx)\n", dwError, dwError);
        goto done;
    }

    if (!SetupDiSetSelectedDriver(NewDeviceInfoSet,
                                  NewDeviceInfoData,
                                  &DriverInfoData))
    {
        dwError = GetLastError();
        DPRINT1("SetupDiSetSelectedDriver() failed (Error %lu 0x%08lx)\n", dwError, dwError);
        goto done;
    }

    if (!SetupDiInstallDevice(NewDeviceInfoSet,
                              NewDeviceInfoData))
    {
        dwError = GetLastError();
        DPRINT1("SetupDiInstallDevice() failed (Error %lu 0x%08lx)\n", dwError, dwError);
        goto done;
    }

    dwError = ERROR_SUCCESS;

done:
    if (bHaveDriverInfoList)
        SetupDiDestroyDriverInfoList(NewDeviceInfoSet,
                                     NewDeviceInfoData,
                                     SPDIT_COMPATDRIVER);

    if (bDeviceRegistered)
        SetupDiDeleteDeviceInfo(NewDeviceInfoSet,
                                NewDeviceInfoData);

    if (NewDeviceInfoData != NULL)
        HeapFree(GetProcessHeap(), 0, NewDeviceInfoData);

    if (NewDeviceInfoSet != INVALID_HANDLE_VALUE)
        SetupDiDestroyDeviceInfoList(NewDeviceInfoSet);

    return dwError;
}


DWORD
WINAPI
BatteryClassInstall(
    _In_ DI_FUNCTION InstallFunction,
    _In_ HDEVINFO DeviceInfoSet,
    _In_opt_ PSP_DEVINFO_DATA DeviceInfoData)
{
    SP_DEVINSTALL_PARAMS_W DeviceInstallParams;
    DWORD dwError;

    DPRINT("BatteryClassInstall(%u %p %p)\n",
           InstallFunction, DeviceInfoSet, DeviceInfoData);

    if (InstallFunction != DIF_INSTALLDEVICE)
        return ERROR_DI_DO_DEFAULT;

    DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);
    if (!SetupDiGetDeviceInstallParamsW(DeviceInfoSet,
                                        DeviceInfoData,
                                        &DeviceInstallParams))
    {
        DPRINT1("SetupDiGetDeviceInstallParamsW() failed (Error %lu)\n", GetLastError());
        return GetLastError();
    }

    /* Install the composite battery device */
    dwError = InstallCompositeBattery(DeviceInfoSet,
                                      DeviceInfoData,
                                      &DeviceInstallParams);
    if (dwError == ERROR_SUCCESS)
    {
        /* Install the battery device */
        dwError = ERROR_DI_DO_DEFAULT;
    }

    return dwError;
}


DWORD
WINAPI
BatteryClassCoInstaller(IN DI_FUNCTION InstallFunction,
                        IN HDEVINFO DeviceInfoSet,
                        IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
                        IN OUT PCOINSTALLER_CONTEXT_DATA Context)
{
    switch (InstallFunction)
    {
        default:
            DPRINT("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}


BOOL
WINAPI
DllMain(
    _In_ HINSTANCE hinstDll,
    _In_ DWORD dwReason,
    _In_ LPVOID reserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hinstDll);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

   return TRUE;
}

/* EOF */
