/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dlls\win32\msports\classinst.c
 * PURPOSE:     Ports class installer
 * PROGRAMMERS: Copyright 2011 Eric Kohl
 */

#include <windows.h>
#include <setupapi.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(msports);


typedef enum _PORT_TYPE
{
    UnknownPort,
    ParallelPort,
    SerialPort
} PORT_TYPE;


static DWORD
InstallSerialPort(IN HDEVINFO DeviceInfoSet,
                  IN PSP_DEVINFO_DATA DeviceInfoData)
{
    LPWSTR pszFriendlyName = L"Serial Port (COMx)";

    TRACE("InstallSerialPort(%p, %p)\n",
          DeviceInfoSet, DeviceInfoData);

    /* Install the device */
    if (!SetupDiInstallDevice(DeviceInfoSet,
                              DeviceInfoData))
    {
        return GetLastError();
    }

    /* Set the friendly name for the device */
    SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet,
                                      DeviceInfoData,
                                      SPDRP_FRIENDLYNAME,
                                      (LPBYTE)pszFriendlyName,
                                      (wcslen(pszFriendlyName) + 1) * sizeof(WCHAR));

    return ERROR_SUCCESS;
}


static DWORD
InstallParallelPort(IN HDEVINFO DeviceInfoSet,
                    IN PSP_DEVINFO_DATA DeviceInfoData)
{
     FIXME("InstallParallelPort(%p, %p)\n",
           DeviceInfoSet, DeviceInfoData);
     return ERROR_DI_DO_DEFAULT;
}


VOID
InstallDeviceData(IN HDEVINFO DeviceInfoSet,
                  IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    HKEY hKey = NULL;
    HINF hInf = INVALID_HANDLE_VALUE;
    SP_DRVINFO_DATA DriverInfoData;
    PSP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    WCHAR InfSectionWithExt[256];
    BYTE buffer[2048];
    DWORD dwRequired;

    TRACE("InstallDeviceData()\n");

    hKey = SetupDiCreateDevRegKeyW(DeviceInfoSet,
                                   DeviceInfoData,
                                   DICS_FLAG_GLOBAL,
                                   0,
                                   DIREG_DRV,
                                   NULL,
                                   NULL);
    if (hKey == NULL)
        goto done;

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (!SetupDiGetSelectedDriverW(DeviceInfoSet,
                                   DeviceInfoData,
                                   &DriverInfoData))
    {
        goto done;
    }

    DriverInfoDetailData = (PSP_DRVINFO_DETAIL_DATA)buffer;
    DriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    if (!SetupDiGetDriverInfoDetailW(DeviceInfoSet,
                                     DeviceInfoData,
                                     &DriverInfoData,
                                     DriverInfoDetailData,
                                     2048,
                                     &dwRequired))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            goto done;
    }

    TRACE("Inf file name: %S\n", DriverInfoDetailData->InfFileName);

    hInf = SetupOpenInfFileW(DriverInfoDetailData->InfFileName,
                             NULL,
                             INF_STYLE_WIN4,
                             NULL);
    if (hInf == INVALID_HANDLE_VALUE)
        goto done;

    TRACE("Section name: %S\n", DriverInfoDetailData->SectionName);

    SetupDiGetActualSectionToInstallW(hInf,
                                      DriverInfoDetailData->SectionName,
                                      InfSectionWithExt,
                                      256,
                                      NULL,
                                      NULL);

    TRACE("InfSectionWithExt: %S\n", InfSectionWithExt);

    SetupInstallFromInfSectionW(NULL,
                                hInf,
                                InfSectionWithExt,
                                SPINST_REGISTRY,
                                hKey,
                                NULL,
                                0,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

    TRACE("Done\n");

done:;
    if (hKey != NULL)
        RegCloseKey(hKey);

    if (hInf != INVALID_HANDLE_VALUE)
        SetupCloseInfFile(hInf);
}



PORT_TYPE
GetPortType(IN HDEVINFO DeviceInfoSet,
            IN PSP_DEVINFO_DATA DeviceInfoData)
{
    HKEY hKey = NULL;
    DWORD dwSize;
    DWORD dwType = 0;
    BYTE bData = 0;
    PORT_TYPE PortType = UnknownPort;
    LONG lError;

    TRACE("GetPortType()\n");

    hKey = SetupDiCreateDevRegKeyW(DeviceInfoSet,
                                   DeviceInfoData,
                                   DICS_FLAG_GLOBAL,
                                   0,
                                   DIREG_DRV,
                                   NULL,
                                   NULL);
    if (hKey == NULL)
    {
        goto done;
    }

    dwSize = sizeof(BYTE);
    lError = RegQueryValueExW(hKey,
                              L"PortSubClass",
                              NULL,
                              &dwType,
                              &bData,
                              &dwSize);

    TRACE("lError: %ld\n", lError);
    TRACE("dwSize: %lu\n", dwSize);
    TRACE("dwType: %lu\n", dwType);

    if (lError == ERROR_SUCCESS &&
        dwSize == sizeof(BYTE) &&
        dwType == REG_BINARY)
    {
        if (bData == 0)
            PortType = ParallelPort;
        else
            PortType = SerialPort;
    }

done:;
    if (hKey != NULL)
        RegCloseKey(hKey);

    TRACE("GetPortType() returns %u \n", PortType);

    return PortType;
}


static DWORD
InstallPort(IN HDEVINFO DeviceInfoSet,
            IN PSP_DEVINFO_DATA DeviceInfoData)
{
    PORT_TYPE PortType;

    InstallDeviceData(DeviceInfoSet, DeviceInfoData);

    PortType = GetPortType(DeviceInfoSet, DeviceInfoData);
    switch (PortType)
    {
        case ParallelPort:
            return InstallParallelPort(DeviceInfoSet, DeviceInfoData);

        case SerialPort:
            return InstallSerialPort(DeviceInfoSet, DeviceInfoData);

        default:
            return ERROR_DI_DO_DEFAULT;
    }
}


DWORD
WINAPI
PortsClassInstaller(IN DI_FUNCTION InstallFunction,
                    IN HDEVINFO DeviceInfoSet,
                    IN PSP_DEVINFO_DATA DeviceInfoData OPTIONAL)
{
    TRACE("PortsClassInstaller(%lu, %p, %p)\n",
          InstallFunction, DeviceInfoSet, DeviceInfoData);

    switch (InstallFunction)
    {
        case DIF_INSTALLDEVICE:
            return InstallPort(DeviceInfoSet, DeviceInfoData);

        default:
            TRACE("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}

/* EOF */
