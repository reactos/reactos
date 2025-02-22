/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dlls\win32\msports\classinst.c
 * PURPOSE:     Ports class installer
 * PROGRAMMERS: Copyright 2011 Eric Kohl
 */

#include "precomp.h"

#include <wchar.h>

#define NTOS_MODE_USER
#include <ndk/cmtypes.h>

typedef enum _PORT_TYPE
{
    UnknownPort,
    ParallelPort,
    SerialPort
} PORT_TYPE;

LPWSTR pszCom = L"COM";
LPWSTR pszLpt = L"LPT";


BOOL
GetBootResourceList(HDEVINFO DeviceInfoSet,
                    PSP_DEVINFO_DATA DeviceInfoData,
                    PCM_RESOURCE_LIST *ppResourceList)
{
    WCHAR DeviceInstanceIdBuffer[128];
    HKEY hEnumKey = NULL;
    HKEY hDeviceKey = NULL;
    HKEY hConfigKey = NULL;
    LPBYTE lpBuffer = NULL;
    DWORD dwDataSize;
    LONG lError;
    BOOL ret = FALSE;

    FIXME("GetBootResourceList()\n");

    *ppResourceList = NULL;

    if (!SetupDiGetDeviceInstanceIdW(DeviceInfoSet,
                                     DeviceInfoData,
                                     DeviceInstanceIdBuffer,
                                     ARRAYSIZE(DeviceInstanceIdBuffer),
                                     &dwDataSize))
    {
        ERR("SetupDiGetDeviceInstanceIdW() failed\n");
        return FALSE;
    }

    lError = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                           L"SYSTEM\\CurrentControlSet\\Enum",
                           0,
                           KEY_QUERY_VALUE,
                           &hEnumKey);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW() failed (Error %lu)\n", lError);
        goto done;
    }

    lError = RegOpenKeyExW(hEnumKey,
                           DeviceInstanceIdBuffer,
                           0,
                           KEY_QUERY_VALUE,
                           &hDeviceKey);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW() failed (Error %lu)\n", lError);
        goto done;
    }

    lError = RegOpenKeyExW(hDeviceKey,
                           L"LogConf",
                           0,
                           KEY_QUERY_VALUE,
                           &hConfigKey);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegOpenKeyExW() failed (Error %lu)\n", lError);
        goto done;
    }

    /* Get the configuration data size */
    lError = RegQueryValueExW(hConfigKey,
                              L"BootConfig",
                              NULL,
                              NULL,
                              NULL,
                              &dwDataSize);
    if (lError != ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW() failed (Error %lu)\n", lError);
        goto done;
    }

    /* Allocate the buffer */
    lpBuffer = HeapAlloc(GetProcessHeap(), 0, dwDataSize);
    if (lpBuffer == NULL)
    {
        ERR("Failed to allocate the resource list buffer\n");
        goto done;
    }

    /* Retrieve the configuration data */
    lError = RegQueryValueExW(hConfigKey,
                              L"BootConfig",
                              NULL,
                              NULL,
                             (LPBYTE)lpBuffer,
                              &dwDataSize);
    if (lError == ERROR_SUCCESS)
    {
        ERR("RegQueryValueExW() failed (Error %lu)\n", lError);
        ret = TRUE;
    }

done:
    if (ret == FALSE && lpBuffer != NULL)
        HeapFree(GetProcessHeap(), 0, lpBuffer);

    if (hConfigKey)
        RegCloseKey(hConfigKey);

    if (hDeviceKey)
        RegCloseKey(hDeviceKey);

    if (hEnumKey)
        RegCloseKey(hEnumKey);

    if (ret != FALSE)
        *ppResourceList = (PCM_RESOURCE_LIST)lpBuffer;

    return ret;
}


DWORD
GetSerialPortNumber(IN HDEVINFO DeviceInfoSet,
                    IN PSP_DEVINFO_DATA DeviceInfoData)
{
    PCM_RESOURCE_LIST lpResourceList = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR lpResDes;
    ULONG i;
    DWORD dwBaseAddress = 0;
    DWORD dwPortNumber = 0;

    TRACE("GetSerialPortNumber(%p, %p)\n",
          DeviceInfoSet, DeviceInfoData);

    if (GetBootResourceList(DeviceInfoSet,
                            DeviceInfoData,
                            &lpResourceList))
    {
        TRACE("Full resource descriptors: %ul\n", lpResourceList->Count);
        if (lpResourceList->Count > 0)
        {
            TRACE("Partial resource descriptors: %ul\n", lpResourceList->List[0].PartialResourceList.Count);

            for (i = 0; i < lpResourceList->List[0].PartialResourceList.Count; i++)
            {
                lpResDes = &lpResourceList->List[0].PartialResourceList.PartialDescriptors[i];
                TRACE("Type: %u\n", lpResDes->Type);

                switch (lpResDes->Type)
                {
                    case CmResourceTypePort:
                        TRACE("Port: Start: %I64x  Length: %lu\n",
                              lpResDes->u.Port.Start.QuadPart,
                              lpResDes->u.Port.Length);
                        if ((lpResDes->u.Port.Start.HighPart == 0) && (dwBaseAddress == 0))
                            dwBaseAddress = (DWORD)lpResDes->u.Port.Start.LowPart;
                        break;

                    case CmResourceTypeInterrupt:
                        TRACE("Interrupt: Level: %lu  Vector: %lu\n",
                              lpResDes->u.Interrupt.Level,
                              lpResDes->u.Interrupt.Vector);
                        break;
                }
            }
        }

        HeapFree(GetProcessHeap(), 0, lpResourceList);
    }

    switch (dwBaseAddress)
    {
        case 0x3f8:
            dwPortNumber = 1;
            break;

        case 0x2f8:
            dwPortNumber = 2;
            break;

        case 0x3e8:
            dwPortNumber = 3;
            break;

        case 0x2e8:
            dwPortNumber = 4;
            break;
    }

    return dwPortNumber;
}


DWORD
GetParallelPortNumber(IN HDEVINFO DeviceInfoSet,
                      IN PSP_DEVINFO_DATA DeviceInfoData)
{
    PCM_RESOURCE_LIST lpResourceList = NULL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR lpResDes;
    ULONG i;
    DWORD dwBaseAddress = 0;
    DWORD dwPortNumber = 0;

    TRACE("GetParallelPortNumber(%p, %p)\n",
          DeviceInfoSet, DeviceInfoData);

    if (GetBootResourceList(DeviceInfoSet,
                            DeviceInfoData,
                            &lpResourceList))
    {
        TRACE("Full resource descriptors: %ul\n", lpResourceList->Count);
        if (lpResourceList->Count > 0)
        {
            TRACE("Partial resource descriptors: %ul\n", lpResourceList->List[0].PartialResourceList.Count);

            for (i = 0; i < lpResourceList->List[0].PartialResourceList.Count; i++)
            {
                lpResDes = &lpResourceList->List[0].PartialResourceList.PartialDescriptors[i];
                TRACE("Type: %u\n", lpResDes->Type);

                switch (lpResDes->Type)
                {
                    case CmResourceTypePort:
                        TRACE("Port: Start: %I64x  Length: %lu\n",
                              lpResDes->u.Port.Start.QuadPart,
                              lpResDes->u.Port.Length);
                        if ((lpResDes->u.Port.Start.HighPart == 0) && (dwBaseAddress == 0))
                            dwBaseAddress = (DWORD)lpResDes->u.Port.Start.LowPart;
                        break;

                    case CmResourceTypeInterrupt:
                        TRACE("Interrupt: Level: %lu  Vector: %lu\n",
                              lpResDes->u.Interrupt.Level,
                              lpResDes->u.Interrupt.Vector);
                        break;
                }

            }

        }

        HeapFree(GetProcessHeap(), 0, lpResourceList);
    }

    switch (dwBaseAddress)
    {
        case 0x378:
            dwPortNumber = 1;
            break;

        case 0x278:
            dwPortNumber = 2;
            break;
    }

    return dwPortNumber;
}


static DWORD
InstallSerialPort(IN HDEVINFO DeviceInfoSet,
                  IN PSP_DEVINFO_DATA DeviceInfoData)
{
    WCHAR szDeviceDescription[256];
    WCHAR szFriendlyName[256];
    WCHAR szPortName[8];
    DWORD dwPortNumber = 0;
    DWORD dwSize;
    HCOMDB hComDB = HCOMDB_INVALID_HANDLE_VALUE;
    HKEY hKey;
    LONG lError;

    TRACE("InstallSerialPort(%p, %p)\n",
          DeviceInfoSet, DeviceInfoData);

    /* Open the com port database */
    ComDBOpen(&hComDB);

    /* Try to read the 'PortName' value and determine the port number */
    hKey = SetupDiCreateDevRegKeyW(DeviceInfoSet,
                                   DeviceInfoData,
                                   DICS_FLAG_GLOBAL,
                                   0,
                                   DIREG_DEV,
                                   NULL,
                                   NULL);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        dwSize = sizeof(szPortName);
        lError = RegQueryValueEx(hKey,
                                 L"PortName",
                                 NULL,
                                 NULL,
                                 (PBYTE)szPortName,
                                 &dwSize);
        if (lError  == ERROR_SUCCESS)
        {
            if (_wcsnicmp(szPortName, pszCom, wcslen(pszCom)) == 0)
            {
                dwPortNumber = _wtoi(szPortName + wcslen(pszCom));
                TRACE("COM port number found: %lu\n", dwPortNumber);
            }
        }

        RegCloseKey(hKey);
    }

    /* Determine the port number from its resources ... */
    if (dwPortNumber == 0)
        dwPortNumber = GetSerialPortNumber(DeviceInfoSet,
                                           DeviceInfoData);

    if (dwPortNumber != 0)
    {
        /* ... and claim the port number in the database */
        ComDBClaimPort(hComDB,
                       dwPortNumber,
                       FALSE,
                       NULL);
    }
    else
    {
        /* ... or claim the next free port number */
        ComDBClaimNextFreePort(hComDB,
                               &dwPortNumber);
    }

    /* Build the name of the port device */
    swprintf(szPortName, L"%s%u", pszCom, dwPortNumber);

    /* Close the com port database */
    if (hComDB != HCOMDB_INVALID_HANDLE_VALUE)
        ComDBClose(hComDB);

    /* Set the 'PortName' value */
    hKey = SetupDiCreateDevRegKeyW(DeviceInfoSet,
                                   DeviceInfoData,
                                   DICS_FLAG_GLOBAL,
                                   0,
                                   DIREG_DEV,
                                   NULL,
                                   NULL);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        RegSetValueExW(hKey,
                       L"PortName",
                       0,
                       REG_SZ,
                       (LPBYTE)szPortName,
                       (wcslen(szPortName) + 1) * sizeof(WCHAR));

        RegCloseKey(hKey);
    }

    /* Install the device */
    if (!SetupDiInstallDevice(DeviceInfoSet,
                              DeviceInfoData))
    {
        return GetLastError();
    }

    /* Get the device description... */
    if (SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_DEVICEDESC,
                                          NULL,
                                          (LPBYTE)szDeviceDescription,
                                          256 * sizeof(WCHAR),
                                          NULL))
    {
        /* ... and use it to build a new friendly name */
        swprintf(szFriendlyName,
                 L"%s (%s)",
                 szDeviceDescription,
                 szPortName);
    }
    else
    {
        /* ... or build a generic friendly name */
        swprintf(szFriendlyName,
                 L"Serial Port (%s)",
                 szPortName);
    }

    /* Set the friendly name for the device */
    SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet,
                                      DeviceInfoData,
                                      SPDRP_FRIENDLYNAME,
                                      (LPBYTE)szFriendlyName,
                                      (wcslen(szFriendlyName) + 1) * sizeof(WCHAR));

    return ERROR_SUCCESS;
}


static DWORD
InstallParallelPort(IN HDEVINFO DeviceInfoSet,
                    IN PSP_DEVINFO_DATA DeviceInfoData)
{
    WCHAR szDeviceDescription[256];
    WCHAR szFriendlyName[256];
    WCHAR szPortName[8];
    DWORD dwPortNumber = 0;
    DWORD dwSize;
    DWORD dwValue;
    LONG lError;
    HKEY hKey;

    TRACE("InstallParallelPort(%p, %p)\n",
          DeviceInfoSet, DeviceInfoData);

    /* Try to read the 'PortName' value and determine the port number */
    hKey = SetupDiCreateDevRegKeyW(DeviceInfoSet,
                                   DeviceInfoData,
                                   DICS_FLAG_GLOBAL,
                                   0,
                                   DIREG_DEV,
                                   NULL,
                                   NULL);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        dwSize = sizeof(szPortName);
        lError = RegQueryValueEx(hKey,
                                 L"PortName",
                                 NULL,
                                 NULL,
                                 (PBYTE)szPortName,
                                 &dwSize);
        if (lError  == ERROR_SUCCESS)
        {
            if (_wcsnicmp(szPortName, pszLpt, wcslen(pszLpt)) == 0)
            {
                dwPortNumber = _wtoi(szPortName + wcslen(pszLpt));
                TRACE("LPT port number found: %lu\n", dwPortNumber);
            }
        }

        RegCloseKey(hKey);
    }

    /* ... try to determine the port number from its resources */
    if (dwPortNumber == 0)
    {
        dwPortNumber = GetParallelPortNumber(DeviceInfoSet,
                                             DeviceInfoData);
        TRACE("GetParallelPortNumber() returned port number: %lu\n", dwPortNumber);
    }

    if (dwPortNumber == 0)
    {
        /* FIXME */
        FIXME("Got no valid port numer!\n");
    }

    if (dwPortNumber != 0)
    {
        swprintf(szPortName, L"%s%u", pszLpt, dwPortNumber);
    }
    else
    {
        wcscpy(szPortName, L"LPTx");
    }

    if (dwPortNumber != 0)
    {
        /* Set the 'PortName' value */
        hKey = SetupDiCreateDevRegKeyW(DeviceInfoSet,
                                       DeviceInfoData,
                                       DICS_FLAG_GLOBAL,
                                       0,
                                       DIREG_DEV,
                                       NULL,
                                       NULL);
        if (hKey != INVALID_HANDLE_VALUE)
        {
            RegSetValueExW(hKey,
                           L"PortName",
                           0,
                           REG_SZ,
                           (LPBYTE)szPortName,
                           (wcslen(szPortName) + 1) * sizeof(WCHAR));

            /*
             * FIXME / HACK:
             * This is to get the w2k3 parport.sys to work until we have our own.
             * This setting makes the driver accept resources with an IRQ instead
             * of only resources without an IRQ.
             *
             * We should probably also fix IO manager to actually give devices a
             * chance to register without an IRQ. CORE-9645
             */
            dwValue = 0;
            RegSetValueExW(hKey,
                           L"FilterResourceMethod",
                           0,
                           REG_DWORD,
                           (LPBYTE)&dwValue,
                           sizeof(dwValue));

            RegCloseKey(hKey);
        }
    }

    /* Install the device */
    if (!SetupDiInstallDevice(DeviceInfoSet,
                              DeviceInfoData))
    {
        return GetLastError();
    }

    /* Get the device description... */
    if (SetupDiGetDeviceRegistryPropertyW(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_DEVICEDESC,
                                          NULL,
                                          (LPBYTE)szDeviceDescription,
                                          256 * sizeof(WCHAR),
                                          NULL))
    {
        /* ... and use it to build a new friendly name */
        swprintf(szFriendlyName,
                 L"%s (%s)",
                 szDeviceDescription,
                 szPortName);
    }
    else
    {
        /* ... or build a generic friendly name */
        swprintf(szFriendlyName,
                 L"Parallel Port (%s)",
                 szPortName);
    }

    TRACE("Friendly name: %S\n", szFriendlyName);

    /* Set the friendly name for the device */
    SetupDiSetDeviceRegistryPropertyW(DeviceInfoSet,
                                      DeviceInfoData,
                                      SPDRP_FRIENDLYNAME,
                                      (LPBYTE)szFriendlyName,
                                      (wcslen(szFriendlyName) + 1) * sizeof(WCHAR));

    return ERROR_SUCCESS;
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

    if (!SetupDiGetActualSectionToInstallW(hInf,
                                           DriverInfoDetailData->SectionName,
                                           InfSectionWithExt,
                                           256,
                                           NULL,
                                           NULL))
        goto done;

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

done:
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

done:
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


static DWORD
RemovePort(IN HDEVINFO DeviceInfoSet,
           IN PSP_DEVINFO_DATA DeviceInfoData)
{
    PORT_TYPE PortType;
    HCOMDB hComDB = HCOMDB_INVALID_HANDLE_VALUE;
    HKEY hKey;
    LONG lError;
    DWORD dwPortNumber;
    DWORD dwPortNameSize;
    WCHAR szPortName[8];

    /* If we are removing a serial port ... */
    PortType = GetPortType(DeviceInfoSet, DeviceInfoData);
    if (PortType == SerialPort)
    {
        /* Open the port database */
        if (ComDBOpen(&hComDB) == ERROR_SUCCESS)
        {
            /* Open the device key */
            hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                        DeviceInfoData,
                                        DICS_FLAG_GLOBAL,
                                        0,
                                        DIREG_DEV,
                                        KEY_READ);
            if (hKey != INVALID_HANDLE_VALUE)
            {
                /* Query the port name */
                dwPortNameSize = sizeof(szPortName);
                lError = RegQueryValueEx(hKey,
                                         L"PortName",
                                         NULL,
                                         NULL,
                                         (PBYTE)szPortName,
                                         &dwPortNameSize);

                /* Close the device key */
                RegCloseKey(hKey);

                /* If we got a valid port name ...*/
                if (lError == ERROR_SUCCESS)
                {
                    /* Get the port number */
                    dwPortNumber = _wtoi(szPortName + wcslen(pszCom));

                    /* Release the port */
                    ComDBReleasePort(hComDB, dwPortNumber);
                }
            }

            /* Close the port database */
            ComDBClose(hComDB);
        }
    }

    /* Remove the device */
    if (!SetupDiRemoveDevice(DeviceInfoSet, DeviceInfoData))
        return GetLastError();

    return ERROR_SUCCESS;
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

        case DIF_REMOVE:
            return RemovePort(DeviceInfoSet, DeviceInfoData);

        default:
            TRACE("Install function %u ignored\n", InstallFunction);
            return ERROR_DI_DO_DEFAULT;
    }
}

/* EOF */
