/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/syssetup/classinst.c
 * PURPOSE:     Class installers
 * PROGRAMMERS: Copyright 2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/*
 * @unimplemented
 */
DWORD
WINAPI
ComputerClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    switch (InstallFunction)
    {
        default:
            DPRINT1("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}


/*
 * @implemented
 */
DWORD
WINAPI
CriticalDeviceCoInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context)
{
    WCHAR szDeviceId[256];
    WCHAR szServiceName[256];
    WCHAR szClassGUID[64];
    DWORD dwRequiredSize;
    HKEY hDriverKey = NULL;
    HKEY hDatabaseKey = NULL, hDeviceKey = NULL;
    DWORD dwDisposition;
    PWSTR Ptr;
    DWORD dwError = ERROR_SUCCESS;

    DPRINT("CriticalDeviceCoInstaller(%lu %p %p %p)\n",
           InstallFunction, DeviceInfoSet, DeviceInfoData, Context);

    if (InstallFunction != DIF_INSTALLDEVICE)
        return ERROR_SUCCESS;

    /* Get the MatchingDeviceId property */
    hDriverKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                      DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DRV,
                                      KEY_READ);
    if (hDriverKey == INVALID_HANDLE_VALUE)
    {
        if (Context->PostProcessing)
        {
            dwError = GetLastError();
            DPRINT1("Failed to open the driver key! (Error %lu)\n", dwError);
            goto done;
        }
        else
        {
            DPRINT("Failed to open the driver key! Postprocessing required!\n");
            return ERROR_DI_POSTPROCESSING_REQUIRED;
        }
    }

    dwRequiredSize = sizeof(szDeviceId);
    dwError = RegQueryValueExW(hDriverKey,
                               L"MatchingDeviceId",
                               NULL,
                               NULL,
                               (PBYTE)szDeviceId,
                               &dwRequiredSize);
    RegCloseKey(hDriverKey);
    if (dwError != ERROR_SUCCESS)
    {
        if (Context->PostProcessing)
        {
            dwError = GetLastError();
            DPRINT1("Failed to read the MatchingDeviceId value! (Error %lu)\n", dwError);
            goto done;
        }
        else
        {
            DPRINT("Failed to read the MatchingDeviceId value! Postprocessing required!\n");
            return ERROR_DI_POSTPROCESSING_REQUIRED;
        }
    }

    DPRINT("MatchingDeviceId: %S\n", szDeviceId);

    /* Get the ClassGUID property */
    dwRequiredSize = 0;
    if (!SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet,
                                           DeviceInfoData,
                                           SPDRP_CLASSGUID,
                                           NULL,
                                           (PBYTE)szClassGUID,
                                           sizeof(szClassGUID),
                                           &dwRequiredSize))
    {
        if (Context->PostProcessing)
        {
            dwError = GetLastError();
            DPRINT1("Failed to read the ClassGUID! (Error %lu)\n", dwError);
            goto done;
        }
        else
        {
            DPRINT("Failed to read the ClassGUID! Postprocessing required!\n");
            return ERROR_DI_POSTPROCESSING_REQUIRED;
        }
    }

    DPRINT("ClassGUID %S\n", szClassGUID);

    /* Get the Service property (optional) */
    dwRequiredSize = 0;
    if (!SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet,
                                           DeviceInfoData,
                                           SPDRP_SERVICE,
                                           NULL,
                                           (PBYTE)szServiceName,
                                           sizeof(szServiceName),
                                           &dwRequiredSize))
    {
        if (Context->PostProcessing)
        {
            dwError = GetLastError();
            if (dwError != ERROR_FILE_NOT_FOUND)
            {
                DPRINT1("Failed to read the Service name! (Error %lu)\n", dwError);
                goto done;
            }
            else
            {
                szServiceName[0] = UNICODE_NULL;
                dwError = ERROR_SUCCESS;
            }
        }
        else
        {
            DPRINT("Failed to read the Service name! Postprocessing required!\n");
            return ERROR_DI_POSTPROCESSING_REQUIRED;
        }
    }

    DPRINT("Service %S\n", szServiceName);

    /* Replace the first backslash by a number sign */
    Ptr = wcschr(szDeviceId, L'\\');
    if (Ptr != NULL)
    {
        *Ptr = L'#';

        /* Terminate the device id at the second backslash */
        Ptr = wcschr(Ptr, L'\\');
        if (Ptr != NULL)
            *Ptr = UNICODE_NULL;
    }

    DPRINT("DeviceId: %S\n", szDeviceId);

    /* Open the critical device database key */
    dwError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                            L"SYSTEM\\CurrentControlSet\\Control\\CriticalDeviceDatabase",
                            0,
                            KEY_WRITE,
                            &hDatabaseKey);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegOpenKeyExW failed (Error %lu)\n", dwError);
        goto done;
    }

    /* Create a new key for the device */
    dwError = RegCreateKeyExW(hDatabaseKey,
                              szDeviceId,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hDeviceKey,
                              &dwDisposition);
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegCreateKeyExW failed (Error %lu)\n", dwError);
        goto done;
    }

    /* Set the ClassGUID value */
    dwError = RegSetValueExW(hDeviceKey,
                             L"ClassGUID",
                             0,
                             REG_SZ,
                             (PBYTE)szClassGUID,
                             (wcslen(szClassGUID) + 1) * sizeof(WCHAR));
    if (dwError != ERROR_SUCCESS)
    {
        DPRINT1("RegSetValueExW failed (Error %lu)\n", dwError);
        goto done;
    }

    /* If available, set the Service value */
    if (szServiceName[0] != UNICODE_NULL)
    {
        dwError = RegSetValueExW(hDeviceKey,
                                 L"Service",
                                 0,
                                 REG_SZ,
                                 (PBYTE)szServiceName,
                                 (wcslen(szServiceName) + 1) * sizeof(WCHAR));
        if (dwError != ERROR_SUCCESS)
        {
            DPRINT1("RegSetValueExW failed (Error %lu)\n", dwError);
            goto done;
        }
    }

done:
    if (hDeviceKey != NULL)
        RegCloseKey(hDeviceKey);

    if (hDatabaseKey != NULL)
        RegCloseKey(hDatabaseKey);

    DPRINT("CriticalDeviceCoInstaller() done (Error %lu)\n", dwError);

    return dwError;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
DeviceBayClassInstaller(
    IN DI_FUNCTION InstallFunction,
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


/*
 * @unimplemented
 */
DWORD
WINAPI
EisaUpHalCoInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context)
{
    switch (InstallFunction)
    {
        default:
            DPRINT1("Install function %u ignored\n", InstallFunction);
            return ERROR_SUCCESS;
    }
}


/*
 * @implemented
 */
DWORD
WINAPI
HdcClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    DPRINT("HdcClassInstaller()\n");
    return ERROR_DI_DO_DEFAULT;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
KeyboardClassInstaller(
    IN DI_FUNCTION InstallFunction,
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


/*
 * @unimplemented
 */
DWORD
WINAPI
MouseClassInstaller(
    IN DI_FUNCTION InstallFunction,
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


/*
 * @unimplemented
 */
DWORD
WINAPI
NtApmClassInstaller(
    IN DI_FUNCTION InstallFunction,
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


/*
 * @unimplemented
 */
DWORD
WINAPI
ScsiClassInstaller(
    IN DI_FUNCTION InstallFunction,
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


/*
 * @unimplemented
 */
DWORD
WINAPI
StorageCoInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL,
    IN OUT PCOINSTALLER_CONTEXT_DATA Context)
{
    switch (InstallFunction)
    {
        default:
            DPRINT1("Install function %u ignored\n", InstallFunction);
            return ERROR_SUCCESS;
    }
}


/*
 * @implemented
 */
DWORD
WINAPI
TapeClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    DPRINT("TapeClassInstaller()\n");
    return ERROR_DI_DO_DEFAULT;
}


/*
 * @implemented
 */
DWORD
WINAPI
VolumeClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    DPRINT("VolumeClassInstaller()\n");
    return ERROR_DI_DO_DEFAULT;
}

/* EOF */
