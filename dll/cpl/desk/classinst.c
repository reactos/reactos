/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Display Control Panel
 * FILE:            dll/cpl/desk/classinst.c
 * PURPOSE:         Class installers
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "desk.h"

//#define NDEBUG
#include <debug.h>

DWORD WINAPI
DisplayClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    SP_DEVINSTALL_PARAMS InstallParams;
    SP_DRVINFO_DATA DriverInfoData;
    HINF hInf = INVALID_HANDLE_VALUE;
    TCHAR SectionName[MAX_PATH];
    TCHAR ServiceName[MAX_SERVICE_NAME_LEN];
    TCHAR DeviceName[12];
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    DISPLAY_DEVICE DisplayDevice;
    HKEY hDriverKey = INVALID_HANDLE_VALUE; /* SetupDiOpenDevRegKey returns INVALID_HANDLE_VALUE in case of error! */
    HKEY hSettingsKey = NULL;
    HKEY hServicesKey = NULL;
    HKEY hServiceKey = NULL;
    HKEY hDeviceSubKey = NULL;
    DWORD disposition, cchMax, cbData;
    WORD wIndex;
    BOOL result;
    LONG rc;
    HRESULT hr;

    if (InstallFunction != DIF_INSTALLDEVICE)
        return ERROR_DI_DO_DEFAULT;

    /* Set DI_DONOTCALLCONFIGMG flag */
    InstallParams.cbSize = sizeof(InstallParams);
    result = SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &InstallParams);
    if (!result)
    {
        rc = GetLastError();
        DPRINT("SetupDiGetDeviceInstallParams() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    InstallParams.Flags |= DI_DONOTCALLCONFIGMG;

    result = SetupDiSetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &InstallParams);
    if (!result)
    {
        rc = GetLastError();
        DPRINT("SetupDiSetDeviceInstallParams() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    /* Do normal install */
    result = SetupDiInstallDevice(DeviceInfoSet, DeviceInfoData);
    if (!result)
    {
        rc = GetLastError();
        DPRINT("SetupDiInstallDevice() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    /* Get .inf file name and section name */
    DriverInfoData.cbSize = sizeof(DriverInfoData);
    result = SetupDiGetSelectedDriver(DeviceInfoSet, DeviceInfoData, &DriverInfoData);
    if (!result)
    {
        rc = GetLastError();
        DPRINT("SetupDiGetSelectedDriver() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    DriverInfoDetailData.cbSize = sizeof(DriverInfoDetailData);
    result = SetupDiGetDriverInfoDetail(DeviceInfoSet, DeviceInfoData,
                                        &DriverInfoData, &DriverInfoDetailData,
                                        sizeof(DriverInfoDetailData), NULL);
    if (!result && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
    {
        rc = GetLastError();
        DPRINT("SetupDiGetDriverInfoDetail() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    hInf = SetupOpenInfFile(DriverInfoDetailData.InfFileName, NULL, INF_STYLE_WIN4, NULL);
    if (hInf == INVALID_HANDLE_VALUE)
    {
        rc = GetLastError();
        DPRINT("SetupOpenInfFile() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    cchMax = MAX_PATH - (sizeof(_T(".SoftwareSettings")) / sizeof(TCHAR));
    result = SetupDiGetActualSectionToInstall(hInf,
                                              DriverInfoDetailData.SectionName,
                                              SectionName,
                                              cchMax,
                                              NULL,
                                              NULL);
    if (!result)
    {
        rc = GetLastError();
        DPRINT("SetupDiGetActualSectionToInstall() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    hr = StringCbCat(SectionName, sizeof(SectionName), _T(".SoftwareSettings"));
    if (FAILED(hr))
    {
        rc = ERROR_INSUFFICIENT_BUFFER;
        goto cleanup;
    }

    /* Open driver registry key and create Settings subkey */
    hDriverKey = SetupDiOpenDevRegKey(
        DeviceInfoSet, DeviceInfoData,
        DICS_FLAG_GLOBAL, 0, DIREG_DRV,
        KEY_CREATE_SUB_KEY);
    if (hDriverKey == INVALID_HANDLE_VALUE)
    {
        rc = GetLastError();
        DPRINT("SetupDiOpenDevRegKey() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    rc = RegCreateKeyEx(
        hDriverKey, L"Settings",
        0, NULL, REG_OPTION_NON_VOLATILE,
#if _WIN32_WINNT >= 0x502
        KEY_READ | KEY_WRITE,
#else
        KEY_ALL_ACCESS,
#endif
        NULL, &hSettingsKey, &disposition);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT("RegCreateKeyEx() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    /* Install .SoftwareSettings to Settings subkey */
    result = SetupInstallFromInfSection(
        InstallParams.hwndParent, hInf, SectionName,
        SPINST_REGISTRY, hSettingsKey,
        NULL, 0, NULL, NULL,
        NULL, NULL);
    if (!result)
    {
        rc = GetLastError();
        DPRINT("SetupInstallFromInfSection() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    /* Get service name and open service registry key */
    result = SetupDiGetDeviceRegistryProperty(
        DeviceInfoSet, DeviceInfoData,
        SPDRP_SERVICE, NULL,
        (PBYTE)ServiceName, MAX_SERVICE_NAME_LEN * sizeof(TCHAR), NULL);
    if (!result)
    {
        rc = GetLastError();
        DPRINT("SetupDiGetDeviceRegistryProperty() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    rc = RegOpenKeyEx(
        HKEY_LOCAL_MACHINE, _T("SYSTEM\\CurrentControlSet\\Services"),
        0, KEY_ENUMERATE_SUB_KEYS, &hServicesKey);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT("RegOpenKeyEx() failed with error 0x%lx\n", rc);
        goto cleanup;
    }
    rc = RegOpenKeyEx(
        hServicesKey, ServiceName,
        0, KEY_CREATE_SUB_KEY, &hServiceKey);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT("RegOpenKeyEx() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    /* Create a new DeviceX subkey */
    for (wIndex = 0; wIndex < 9999; wIndex++)
    {
        _stprintf(DeviceName, _T("Device%hu"), wIndex);

        rc = RegCreateKeyEx(
            hServiceKey, DeviceName, 0, NULL,
            REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, NULL,
            &hDeviceSubKey, &disposition);
        if (rc != ERROR_SUCCESS)
        {
            DPRINT("RegCreateKeyEx() failed with error 0x%lx\n", rc);
            goto cleanup;
        }

        if (disposition == REG_CREATED_NEW_KEY)
            break;

        if (wIndex == 9999)
        {
            rc = ERROR_GEN_FAILURE;
            DPRINT("RegCreateKeyEx() failed\n");
            goto cleanup;
        }

        RegCloseKey(hDeviceSubKey);
        hDeviceSubKey = NULL;
    }

    /* Install SoftwareSettings section */
    /* Yes, we're installing this section for the second time.
     * We don't want to create a link to Settings subkey */
    result = SetupInstallFromInfSection(
        InstallParams.hwndParent, hInf, SectionName,
        SPINST_REGISTRY, hDeviceSubKey,
        NULL, 0, NULL, NULL,
        NULL, NULL);
    if (!result)
    {
        rc = GetLastError();
        DPRINT("SetupInstallFromInfSection() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    /* Add Device Description string */
    cbData = (DWORD)(_tcslen(DriverInfoData.Description) + 1) * sizeof(TCHAR);
    rc = RegSetValueEx(hDeviceSubKey,
                       _T("Device Description"),
                       0,
                       REG_SZ,
                       (const BYTE*)DriverInfoData.Description,
                       cbData);
    if (rc != ERROR_SUCCESS)
    {
        DPRINT("RegSetValueEx() failed with error 0x%lx\n", rc);
        goto cleanup;
    }

    /* FIXME: install OpenGLSoftwareSettings section */

    /* Reenumerate display devices ; this will rescan for potential new devices */
    DisplayDevice.cb = sizeof(DISPLAY_DEVICE);
    EnumDisplayDevices(NULL, 0, &DisplayDevice, 0);

    rc = ERROR_SUCCESS;

cleanup:
    if (hInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(hInf);
    if (hDriverKey != INVALID_HANDLE_VALUE)
    {
        /* SetupDiOpenDevRegKey returns INVALID_HANDLE_VALUE in case of error! */
        RegCloseKey(hDriverKey);
    }
    if (hSettingsKey != NULL)
        RegCloseKey(hSettingsKey);
    if (hServicesKey != NULL)
        RegCloseKey(hServicesKey);
    if (hServiceKey != NULL)
        RegCloseKey(hServiceKey);
    if (hDeviceSubKey != NULL)
        RegCloseKey(hDeviceSubKey);

    return rc;
}

DWORD WINAPI
MonitorClassInstaller(
    IN DI_FUNCTION InstallFunction,
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    return ERROR_DI_DO_DEFAULT;
}
