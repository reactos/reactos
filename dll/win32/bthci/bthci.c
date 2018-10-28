/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Bluetooth Class installer
 * COPYRIGHT:   Copyright 2018 Eric Kohl (eric.kohl@reactos.org)
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <setupapi.h>

#define NDEBUG
#include <debug.h>


DWORD
WINAPI
BluetoothClassInstaller(
    _In_ DI_FUNCTION InstallFunction,
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    switch (InstallFunction)
    {
        default:
            DPRINT1("Install function %u\n", InstallFunction);
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
