/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll\win32\batt\batt.c
 * PURPOSE:     Battery Class installers
 * PROGRAMMERS: Copyright 2010 Eric Kohl
 */


#include <windows.h>
#include <setupapi.h>

#define NDEBUG
#include <debug.h>


BOOL
WINAPI
DllMain(HINSTANCE hinstDll,
        DWORD dwReason,
        LPVOID reserved)
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


DWORD
WINAPI
BatteryClassCoInstaller(IN DI_FUNCTION InstallFunction,
                        IN HDEVINFO DeviceInfoSet,
                        IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    switch (InstallFunction)
    {
        default:
            DPRINT("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}


DWORD
WINAPI
BatteryClassInstall(IN DI_FUNCTION InstallFunction,
                    IN HDEVINFO DeviceInfoSet,
                    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    switch (InstallFunction)
    {
        default:
            DPRINT("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}

/* EOF */
