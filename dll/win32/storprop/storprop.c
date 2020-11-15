/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Storage device properties
 * COPYRIGHT:   2020 Eric Kohl (eric.kohl@reactos.org)
 */

#define WIN32_NO_STATUS
#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <winreg.h>
#include <winuser.h>
#include <commctrl.h>
#include <setupapi.h>

#define NDEBUG
#include <debug.h>

HINSTANCE hInstance = NULL;

DWORD
WINAPI
DiskClassInstaller(
    _In_ DI_FUNCTION InstallFunction,
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    DPRINT("DiskClassInstaller(%u %p %p)\n",
           InstallFunction, DeviceInfoSet, DeviceInfoData);

    return ERROR_DI_DO_DEFAULT;
}

DWORD
WINAPI
DvdClassInstaller(
    _In_ DI_FUNCTION InstallFunction,
    _In_ HDEVINFO DeviceInfoSet,
    _In_ PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    DPRINT("DvdClassInstaller(%u %p %p)\n",
           InstallFunction, DeviceInfoSet, DeviceInfoData);

    return ERROR_DI_DO_DEFAULT;
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
            hInstance = hinstDll;
            break;

        case DLL_PROCESS_DETACH:
            hInstance = NULL;
            break;
    }

   return TRUE;
}

/* EOF */
