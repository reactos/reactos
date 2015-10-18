/*
 * ReactOS Device Manager Applet
 * Copyright (C) 2004 - 2005 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * PROJECT:         ReactOS devmgr.dll
 * FILE:            lib/devmgr/misc.c
 * PURPOSE:         ReactOS Device Manager
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      2005/11/24  Created
 */

#include "precomp.h"
#include <devmgr/devmgr.h>
#include "properties.h"
#include "resource.h"


INT
LengthOfStrResource(IN HINSTANCE hInst,
                    IN UINT uID)
{
    HRSRC hrSrc;
    HGLOBAL hRes;
    LPWSTR lpName, lpStr;

    if (hInst == NULL)
    {
        return -1;
    }

    /* There are always blocks of 16 strings */
    lpName = (LPWSTR)MAKEINTRESOURCE((uID >> 4) + 1);

    /* Find the string table block */
    if ((hrSrc = FindResourceW(hInst, lpName, (LPWSTR)RT_STRING)) &&
        (hRes = LoadResource(hInst, hrSrc)) &&
        (lpStr = (LPWSTR)LockResource(hRes)))
    {
        UINT x;

        /* Find the string we're looking for */
        uID &= 0xF; /* position in the block, same as % 16 */
        for (x = 0; x < uID; x++)
        {
            lpStr += (*lpStr) + 1;
        }

        /* Found the string */
        return (int)(*lpStr);
    }
    return -1;
}


static INT
AllocAndLoadString(OUT LPWSTR *lpTarget,
                   IN HINSTANCE hInst,
                   IN UINT uID)
{
    INT ln;

    ln = LengthOfStrResource(hInst,
                             uID);
    if (ln++ > 0)
    {
        (*lpTarget) = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                         ln * sizeof(WCHAR));
        if ((*lpTarget) != NULL)
        {
            INT Ret;
            if (!(Ret = LoadStringW(hInst, uID, *lpTarget, ln)))
            {
                LocalFree((HLOCAL)(*lpTarget));
            }
            return Ret;
        }
    }
    return 0;
}


static INT
AllocAndLoadStringsCat(OUT LPWSTR *lpTarget,
                       IN HINSTANCE hInst,
                       IN UINT *uID,
                       IN UINT nIDs)
{
    INT ln = 0;
    UINT i;

    for (i = 0;
         i != nIDs;
         i++)
    {
        ln += LengthOfStrResource(hInst,
                                  uID[i]);
    }

    if (ln != 0)
    {
        (*lpTarget) = (LPWSTR)LocalAlloc(LMEM_FIXED,
                                         (ln + 1) * sizeof(WCHAR));
        if ((*lpTarget) != NULL)
        {
            LPWSTR s = *lpTarget;
            INT Ret = 0;

            for (i = 0;
                 i != nIDs;
                 i++)
            {
                if (!(Ret = LoadStringW(hInst, uID[i], s, ln)))
                {
                    LocalFree((HLOCAL)(*lpTarget));
                    return 0;
                }

                s += Ret;
            }

            return s - *lpTarget;
        }
    }
    return 0;
}


DWORD
LoadAndFormatString(IN HINSTANCE hInstance,
                    IN UINT uID,
                    OUT LPWSTR *lpTarget,
                    ...)
{
    DWORD Ret = 0;
    LPWSTR lpFormat;
    va_list lArgs;

    if (AllocAndLoadString(&lpFormat,
                           hInstance,
                           uID) != 0)
    {
        va_start(lArgs, lpTarget);
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                             lpFormat,
                             0,
                             0,
                             (LPWSTR)lpTarget,
                             0,
                             &lArgs);
        va_end(lArgs);

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}


DWORD
LoadAndFormatStringsCat(IN HINSTANCE hInstance,
                        IN UINT *uID,
                        IN UINT nIDs,
                        OUT LPWSTR *lpTarget,
                        ...)
{
    DWORD Ret = 0;
    LPWSTR lpFormat;
    va_list lArgs;

    if (AllocAndLoadStringsCat(&lpFormat,
                               hInstance,
                               uID,
                               nIDs) != 0)
    {
        va_start(lArgs, lpTarget);
        /* let's use FormatMessage to format it because it has the ability to allocate
           memory automatically */
        Ret = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING,
                             lpFormat,
                             0,
                             0,
                             (LPWSTR)lpTarget,
                             0,
                             &lArgs);
        va_end(lArgs);

        LocalFree((HLOCAL)lpFormat);
    }

    return Ret;
}


LPARAM
ListViewGetSelectedItemData(IN HWND hwnd)
{
    int Index;

    Index = ListView_GetNextItem(hwnd,
                                 -1,
                                 LVNI_SELECTED);
    if (Index != -1)
    {
        LVITEM li;

        li.mask = LVIF_PARAM;
        li.iItem = Index;
        li.iSubItem = 0;

        if (ListView_GetItem(hwnd,
                             &li))
        {
            return li.lParam;
        }
    }

    return 0;
}


LPWSTR
ConvertMultiByteToUnicode(IN LPCSTR lpMultiByteStr,
                          IN UINT uCodePage)
{
    LPWSTR lpUnicodeStr;
    INT nLength;

    nLength = MultiByteToWideChar(uCodePage,
                                  0,
                                  lpMultiByteStr,
                                  -1,
                                  NULL,
                                  0);
    if (nLength == 0)
        return NULL;

    lpUnicodeStr = (LPWSTR)HeapAlloc(GetProcessHeap(),
                                     0,
                                     nLength * sizeof(WCHAR));
    if (lpUnicodeStr == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    if (!MultiByteToWideChar(uCodePage,
                             0,
                             lpMultiByteStr,
                             nLength,
                             lpUnicodeStr,
                             nLength))
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpUnicodeStr);
        return NULL;
    }

    return lpUnicodeStr;
}


BOOL
GetDeviceManufacturerString(IN HDEVINFO DeviceInfoSet,
                            IN PSP_DEVINFO_DATA DeviceInfoData,
                            OUT LPWSTR szBuffer,
                            IN DWORD BufferSize)
{
    DWORD RegDataType;
    BOOL Ret = FALSE;

    if (!SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_MFG,
                                          &RegDataType,
                                          (PBYTE)szBuffer,
                                          BufferSize * sizeof(WCHAR),
                                          NULL) ||
        RegDataType != REG_SZ)
    {
        szBuffer[0] = L'\0';
        if (LoadString(hDllInstance,
                       IDS_UNKNOWN,
                       szBuffer,
                       BufferSize))
        {
            Ret = TRUE;
        }
    }
    else
    {
        /* FIXME - check string for NULL termination! */
        Ret = TRUE;
    }

    return Ret;
}


BOOL
GetDeviceLocationString(IN HDEVINFO DeviceInfoSet,
                        IN PSP_DEVINFO_DATA DeviceInfoData,
                        IN DEVINST dnParentDevInst  OPTIONAL,
                        OUT LPWSTR szBuffer,
                        IN DWORD BufferSize)
{
    DWORD RegDataType;
    ULONG DataSize;
    CONFIGRET cRet;
    LPWSTR szFormatted;
    HKEY hKey;
    DWORD dwSize, dwType;
    BOOL Ret = FALSE;

    DataSize = BufferSize * sizeof(WCHAR);
    szBuffer[0] = L'\0';

    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_QUERY_VALUE);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        /* query the LocationInformationOverride value */
        dwSize = BufferSize;
        if (RegQueryValueEx(hKey,
                            L"LocationInformationOverride",
                            NULL,
                            &dwType,
                            (LPBYTE)szBuffer,
                            &dwSize) == ERROR_SUCCESS &&
            dwType == REG_SZ &&
            szBuffer[0] != L'\0')
        {
            Ret = TRUE;
        }
        else
        {
            szBuffer[0] = L'\0';
        }

        RegCloseKey(hKey);
    }


    if (!Ret)
    {
        if (dnParentDevInst != 0)
        {
            /* query the parent node name */
            if (CM_Get_DevNode_Registry_Property(dnParentDevInst,
                                                 CM_DRP_DEVICEDESC,
                                                 &RegDataType,
                                                 szBuffer,
                                                 &DataSize,
                                                 0) == CR_SUCCESS &&
                 RegDataType == REG_SZ &&
                 LoadAndFormatString(hDllInstance,
                                     IDS_DEVONPARENT,
                                     &szFormatted,
                                     szBuffer) != 0)
            {
                wcsncpy(szBuffer,
                        szFormatted,
                        BufferSize - 1);
                szBuffer[BufferSize - 1] = L'\0';
                LocalFree((HLOCAL)szFormatted);
                Ret = TRUE;
            }
        }
        else if (DeviceInfoData->DevInst != 0)
        {
            cRet = CM_Get_DevNode_Registry_Property(DeviceInfoData->DevInst,
                                                    CM_DRP_LOCATION_INFORMATION,
                                                    &RegDataType,
                                                    szBuffer,
                                                    &DataSize,
                                                    0);
            if (cRet == CR_SUCCESS && RegDataType == REG_SZ)
            {
                /* FIXME - check string for NULL termination! */
                Ret = TRUE;
            }

            if (Ret && szBuffer[0] >= L'0' && szBuffer[0] <= L'9')
            {
                /* convert the string to an integer value and create a
                   formatted string */
                ULONG ulLocation = (ULONG)wcstoul(szBuffer,
                                                  NULL,
                                                  10);
                if (LoadAndFormatString(hDllInstance,
                                        IDS_LOCATIONSTR,
                                        &szFormatted,
                                        ulLocation,
                                        szBuffer) != 0)
                {
                    wcsncpy(szBuffer,
                            szFormatted,
                            BufferSize - 1);
                    szBuffer[BufferSize - 1] = L'\0';
                    LocalFree((HLOCAL)szFormatted);
                }
                else
                    Ret = FALSE;
            }
        }
    }

    if (!Ret &&
        LoadString(hDllInstance,
                   IDS_UNKNOWN,
                   szBuffer,
                   BufferSize))
    {
        Ret = TRUE;
    }

    return Ret;
}


BOOL
GetDeviceStatusString(IN DEVINST DevInst,
                      IN HMACHINE hMachine,
                      OUT LPWSTR szBuffer,
                      IN DWORD BufferSize)
{
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    UINT MessageId = IDS_UNKNOWN;
    BOOL Ret = FALSE;

    szBuffer[0] = L'\0';
    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DevInst,
                                  0,
                                  hMachine);
    if (cr == CR_SUCCESS)
    {
        if (Status & DN_HAS_PROBLEM)
        {
            UINT uRet;

            uRet = DeviceProblemTextW(hMachine,
                                     DevInst,
                                     ProblemNumber,
                                     szBuffer,
                                     (BufferSize != 0 ? BufferSize : BufferSize - 1));

            Ret = (uRet != 0 && uRet < BufferSize);
        }
        else
        {
            if (!(Status & (DN_DRIVER_LOADED | DN_STARTED)))
            {
                MessageId = IDS_NODRIVERLOADED;
            }
            else
            {
                MessageId = IDS_DEV_NO_PROBLEM;
            }

            goto GeneralMessage;
        }
    }
    else
    {
GeneralMessage:
        if (LoadString(hDllInstance,
                        MessageId,
                        szBuffer,
                        (int)BufferSize))
        {
            Ret = TRUE;
        }
    }

    return Ret;
}


BOOL
GetDriverProviderString(IN HDEVINFO DeviceInfoSet,
                        IN PSP_DEVINFO_DATA DeviceInfoData,
                        OUT LPWSTR szBuffer,
                        IN DWORD BufferSize)
{
    HKEY hKey;
    DWORD dwSize, dwType;
    BOOL Ret = FALSE;

    szBuffer[0] = L'\0';

    /* get driver provider, date and version */
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_QUERY_VALUE);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        /* query the driver provider */
        dwSize = BufferSize;
        if (RegQueryValueEx(hKey,
                            REGSTR_VAL_PROVIDER_NAME,
                            NULL,
                            &dwType,
                            (LPBYTE)szBuffer,
                            &dwSize) == ERROR_SUCCESS &&
            dwType == REG_SZ &&
            szBuffer[0] != L'\0')
        {
            Ret = TRUE;
        }
        else
        {
            szBuffer[0] = L'\0';
        }

        RegCloseKey(hKey);
    }

    if (szBuffer[0] == L'\0')
    {
        /* unable to query the information */
        if (LoadString(hDllInstance,
                       IDS_UNKNOWN,
                       szBuffer,
                       BufferSize))
        {
            Ret = TRUE;
        }
    }

    return Ret;
}


BOOL
GetDriverVersionString(IN HDEVINFO DeviceInfoSet,
                       IN PSP_DEVINFO_DATA DeviceInfoData,
                       OUT LPWSTR szBuffer,
                       IN DWORD BufferSize)
{
    HKEY hKey;
    DWORD dwSize, dwType;
    BOOL Ret = FALSE;

    szBuffer[0] = L'\0';

    /* get driver provider, date and version */
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_QUERY_VALUE);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        /* query the driver provider */
        dwSize = BufferSize;
        if (RegQueryValueEx(hKey,
                            L"DriverVersion",
                            NULL,
                            &dwType,
                            (LPBYTE)szBuffer,
                            &dwSize) == ERROR_SUCCESS &&
            dwType == REG_SZ &&
            szBuffer[0] != L'\0')
        {
            Ret = TRUE;
        }
        else
        {
            szBuffer[0] = L'\0';
        }

        RegCloseKey(hKey);
    }

    if (szBuffer[0] == L'\0')
    {
        /* unable to query the information */
        if (LoadString(hDllInstance,
                       IDS_NOTAVAILABLE,
                       szBuffer,
                       BufferSize))
        {
            Ret = TRUE;
        }
    }

    return Ret;
}

BOOL
GetDriverDateString(IN HDEVINFO DeviceInfoSet,
                    IN PSP_DEVINFO_DATA DeviceInfoData,
                    OUT LPWSTR szBuffer,
                    IN DWORD BufferSize)
{
    HKEY hKey;
    FILETIME DriverDate;
    SYSTEMTIME SystemTime, LocalTime;
    DWORD dwSize, dwType;
    BOOL Ret = FALSE;

    szBuffer[0] = L'\0';

    /* get driver provider, date and version */
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_QUERY_VALUE);
    if (hKey != INVALID_HANDLE_VALUE)
    {
        /* query the driver provider */
        dwSize = sizeof(FILETIME);
        if (RegQueryValueEx(hKey,
                            L"DriverDateData",
                            NULL,
                            &dwType,
                            (LPBYTE)&DriverDate,
                            &dwSize) == ERROR_SUCCESS &&
            dwType == REG_BINARY &&
            dwSize == sizeof(FILETIME) &&
            FileTimeToSystemTime(&DriverDate,
                                 &SystemTime) &&
            SystemTimeToTzSpecificLocalTime(NULL,
                                            &SystemTime,
                                            &LocalTime) &&
            GetDateFormat(LOCALE_USER_DEFAULT,
                          DATE_SHORTDATE,
                          &LocalTime,
                          NULL,
                          szBuffer,
                          BufferSize) != 0)
        {
            Ret = TRUE;
        }

        RegCloseKey(hKey);
    }

    if (!Ret)
    {
        /* unable to query the information */
        if (LoadString(hDllInstance,
                       IDS_NOTAVAILABLE,
                       szBuffer,
                       BufferSize))
        {
            Ret = TRUE;
        }
    }

    return Ret;
}



BOOL
IsDeviceHidden(IN DEVINST DevInst,
               IN HMACHINE hMachine,
               OUT BOOL *IsHidden)
{
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    BOOL Ret = FALSE;

    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DevInst,
                                  0,
                                  hMachine);
    if (cr == CR_SUCCESS)
    {
        *IsHidden = ((Status & DN_NO_SHOW_IN_DM) != 0);
        Ret = TRUE;
    }

    return Ret;
}


BOOL
CanDisableDevice(IN DEVINST DevInst,
                 IN HMACHINE hMachine,
                 OUT BOOL *CanDisable)
{
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    BOOL Ret = FALSE;

    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DevInst,
                                  0,
                                  hMachine);
    if (cr == CR_SUCCESS)
    {
        *CanDisable = ((Status & DN_DISABLEABLE) != 0);
        Ret = TRUE;
    }

    return Ret;
}


BOOL
IsDeviceStarted(IN DEVINST DevInst,
                IN HMACHINE hMachine,
                OUT BOOL *IsStarted)
{
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    BOOL Ret = FALSE;

    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DevInst,
                                  0,
                                  hMachine);
    if (cr == CR_SUCCESS)
    {
        *IsStarted = ((Status & DN_STARTED) != 0);
        Ret = TRUE;
    }

    return Ret;
}


BOOL
IsDriverInstalled(IN DEVINST DevInst,
                  IN HMACHINE hMachine,
                  OUT BOOL *Installed)
{
    CONFIGRET cr;
    ULONG Status, ProblemNumber;
    BOOL Ret = FALSE;

    cr = CM_Get_DevNode_Status_Ex(&Status,
                                  &ProblemNumber,
                                  DevInst,
                                  0,
                                  hMachine);
    if (cr == CR_SUCCESS)
    {
        *Installed = ((Status & DN_HAS_PROBLEM) != 0 ||
                      (Status & (DN_DRIVER_LOADED | DN_STARTED)) != 0);
        Ret = TRUE;
    }

    return Ret;
}


BOOL
EnableDevice(IN HDEVINFO DeviceInfoSet,
             IN PSP_DEVINFO_DATA DevInfoData  OPTIONAL,
             IN BOOL bEnable,
             IN DWORD HardwareProfile  OPTIONAL,
             OUT BOOL *bNeedReboot  OPTIONAL)
{
    SP_PROPCHANGE_PARAMS pcp;
    SP_DEVINSTALL_PARAMS dp;
    DWORD LastErr;
    BOOL Ret = FALSE;

    pcp.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
    pcp.HwProfile = HardwareProfile;

    if (bEnable)
    {
        /* try to enable the device on the global profile */
        pcp.StateChange = DICS_ENABLE;
        pcp.Scope = DICS_FLAG_GLOBAL;

        /* ignore errors */
        LastErr = GetLastError();
        if (SetupDiSetClassInstallParams(DeviceInfoSet,
                                         DevInfoData,
                                         &pcp.ClassInstallHeader,
                                         sizeof(SP_PROPCHANGE_PARAMS)))
        {
            SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                      DeviceInfoSet,
                                      DevInfoData);
        }
        SetLastError(LastErr);
    }

    /* try config-specific */
    pcp.StateChange = (bEnable ? DICS_ENABLE : DICS_DISABLE);
    pcp.Scope = DICS_FLAG_CONFIGSPECIFIC;

    if (SetupDiSetClassInstallParams(DeviceInfoSet,
                                     DevInfoData,
                                     &pcp.ClassInstallHeader,
                                     sizeof(SP_PROPCHANGE_PARAMS)) &&
        SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                  DeviceInfoSet,
                                  DevInfoData))
    {
        dp.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                          DevInfoData,
                                          &dp))
        {
            if (bNeedReboot != NULL)
            {
                *bNeedReboot = ((dp.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)) != 0);
            }

            Ret = TRUE;
        }
    }
    return Ret;
}


BOOL
GetDeviceTypeString(IN PSP_DEVINFO_DATA DeviceInfoData,
                    OUT LPWSTR szBuffer,
                    IN DWORD BufferSize)
{
    BOOL Ret = FALSE;

    if (!SetupDiGetClassDescription(&DeviceInfoData->ClassGuid,
                                    szBuffer,
                                    BufferSize,
                                    NULL))
    {
        szBuffer[0] = L'\0';
        if (LoadString(hDllInstance,
                       IDS_UNKNOWN,
                       szBuffer,
                       BufferSize))
        {
            Ret = TRUE;
        }
    }
    else
    {
        /* FIXME - check string for NULL termination! */
        Ret = TRUE;
    }

    return Ret;
}


BOOL
GetDeviceDescriptionString(IN HDEVINFO DeviceInfoSet,
                           IN PSP_DEVINFO_DATA DeviceInfoData,
                           OUT LPWSTR szBuffer,
                           IN DWORD BufferSize)
{
    DWORD RegDataType;
    BOOL Ret = FALSE;

    if ((SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_FRIENDLYNAME,
                                          &RegDataType,
                                          (PBYTE)szBuffer,
                                          BufferSize * sizeof(WCHAR),
                                          NULL) ||
         SetupDiGetDeviceRegistryProperty(DeviceInfoSet,
                                          DeviceInfoData,
                                          SPDRP_DEVICEDESC,
                                          &RegDataType,
                                          (PBYTE)szBuffer,
                                          BufferSize * sizeof(WCHAR),
                                          NULL)) &&
        RegDataType == REG_SZ)
    {
        /* FIXME - check string for NULL termination! */
        Ret = TRUE;
    }
    else
    {
        szBuffer[0] = L'\0';
        if (LoadString(hDllInstance,
                       IDS_UNKNOWNDEVICE,
                       szBuffer,
                       BufferSize))
        {
            Ret = TRUE;
        }
    }

    return Ret;
}


BOOL
FindCurrentDriver(IN HDEVINFO DeviceInfoSet,
                  IN PSP_DEVINFO_DATA DeviceInfoData,
                  OUT PSP_DRVINFO_DATA DriverInfoData)
{
    HKEY hKey = (HKEY)INVALID_HANDLE_VALUE;
    SP_DEVINSTALL_PARAMS InstallParams = {0};
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData = {0};
    WCHAR InfPath[MAX_PATH];
    WCHAR InfSection[LINE_LEN];
    DWORD dwType, dwLength;
    DWORD i = 0;
    LONG rc;
    BOOL Ret = FALSE;

    /* Steps to find the right driver:
     * 1) Get the device install parameters
     * 2) Open the driver registry key
     * 3) Read the .inf file name
     * 4) Update install params, by setting DI_ENUMSINGLEINF and .inf file name
     * 5) Build class driver list
     * 6) Read inf section and inf section extension from registry
     * 7) Enumerate drivers
     * 8) Find the one who is in the same section as current driver?
     */

    /* 1) Get the install params */
    InstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (!SetupDiGetDeviceInstallParams(DeviceInfoSet,
                                       DeviceInfoData,
                                       &InstallParams))
    {
        ERR("SetupDiSetDeviceInstallParams() failed with error 0x%lx\n", GetLastError());
        goto Cleanup;
    }

#ifdef DI_FLAGSEX_INSTALLEDDRIVER
    InstallParams.FlagsEx |= (DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);
    if (SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                      DeviceInfoData,
                                      &InstallParams))
    {
        if (SetupDiBuildDriverInfoList(DeviceInfoSet,
                                       DeviceInfoData,
                                       SPDIT_CLASSDRIVER) &&
            SetupDiEnumDriverInfo(DeviceInfoSet,
                                  DeviceInfoData,
                                  SPDIT_CLASSDRIVER,
                                  0,
                                  DriverInfoData))
        {
            Ret = TRUE;
        }

        goto Cleanup;
    }
    InstallParams.FlagsEx &= ~(DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);
#endif

    /* 2) Open the driver registry key */
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_QUERY_VALUE);
    if (hKey == INVALID_HANDLE_VALUE)
    {
        ERR("SetupDiOpenDevRegKey() failed with error 0x%lx\n", GetLastError());
        goto Cleanup;
    }

    /* 3) Read the .inf file name */
    dwLength = (sizeof(InfPath) / sizeof(InfPath[0])) - 1;
    rc = RegQueryValueEx(hKey,
                         REGSTR_VAL_INFPATH,
                         0,
                         &dwType,
                         (LPBYTE)InfPath,
                         &dwLength);
    if (rc != ERROR_SUCCESS)
    {
        ERR("RegQueryValueEx() failed with error 0x%lx\n", GetLastError());
        SetLastError(rc);
        goto Cleanup;
    }
    else if (dwType != REG_SZ)
    {
        ERR("Expected registry type REG_SZ (%lu), got %lu\n", REG_SZ, dwType);
        SetLastError(ERROR_GEN_FAILURE);
        goto Cleanup;
    }
    InfPath[(dwLength / sizeof(WCHAR)) - 1] = L'\0';

    /* 4) Update install params, by setting DI_ENUMSINGLEINF and .inf file name */
    InstallParams.Flags |= DI_ENUMSINGLEINF;
    InstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;
    wcscpy(InstallParams.DriverPath, InfPath);
    if (!SetupDiSetDeviceInstallParams(DeviceInfoSet,
                                       DeviceInfoData,
                                       &InstallParams))
    {
        ERR("SetupDiSetDeviceInstallParams() failed with error 0x%lx\n", GetLastError());
        goto Cleanup;
    }

    /* 5) Build class driver list */
    if (!SetupDiBuildDriverInfoList(DeviceInfoSet,
                                    DeviceInfoData,
                                    SPDIT_CLASSDRIVER))
    {
        ERR("SetupDiBuildDriverInfoList() failed with error 0x%lx\n", GetLastError());
        goto Cleanup;
    }

    /* 6) Read inf section and from registry */
    dwLength = (sizeof(InfSection) / sizeof(InfSection[0])) - 1;
    rc = RegQueryValueEx(hKey,
                         REGSTR_VAL_INFSECTION,
                         0,
                         &dwType,
                         (LPBYTE)InfSection,
                         &dwLength);
    if (rc != ERROR_SUCCESS)
    {
        ERR("RegQueryValueEx() failed with error 0x%lx\n", GetLastError());
        SetLastError(rc);
        goto Cleanup;
    }
    else if (dwType != REG_SZ)
    {
        ERR("Expected registry type REG_SZ (%lu), got %lu\n", REG_SZ, dwType);
        SetLastError(ERROR_GEN_FAILURE);
        goto Cleanup;
    }
    InfPath[(dwLength / sizeof(WCHAR)) - 1] = L'\0';

    /* 7) Enumerate drivers */
    DriverInfoData->cbSize = sizeof(SP_DRVINFO_DATA);
    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
    while (SetupDiEnumDriverInfo(DeviceInfoSet,
                                 DeviceInfoData,
                                 SPDIT_CLASSDRIVER,
                                 i,
                                 DriverInfoData))
    {
        /* 8) Find the one who is in the same section as current driver */
        if (!SetupDiGetDriverInfoDetail(DeviceInfoSet,
                                        DeviceInfoData,
                                        DriverInfoData,
                                        &DriverInfoDetailData,
                                        DriverInfoDetailData.cbSize,
                                        NULL) &&
            GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            ERR("SetupDiGetDriverInfoDetail() failed with error 0x%lx\n", GetLastError());
            goto Cleanup;
        }
        if (!_wcsicmp(DriverInfoDetailData.SectionName,
                     InfSection) != 0)
        {
            /* We have found the right driver */
            Ret = TRUE;
            goto Cleanup;
        }

        i++;
    }
    if (GetLastError() != ERROR_NO_MORE_ITEMS)
    {
        ERR("SetupDiEnumDriverInfo() failed with error 0x%lx\n", GetLastError());
        goto Cleanup;
    }

    SetLastError(ERROR_NO_DRIVER_SELECTED);

Cleanup:
    if (hKey != INVALID_HANDLE_VALUE)
        RegCloseKey(hKey);
    return Ret;
}


HINSTANCE
LoadAndInitComctl32(VOID)
{
    typedef VOID (WINAPI *PINITCOMMONCONTROLS)(VOID);
    PINITCOMMONCONTROLS pInitCommonControls;
    HINSTANCE hComCtl32;

    hComCtl32 = LoadLibrary(L"comctl32.dll");
    if (hComCtl32 != NULL)
    {
        /* initialize the common controls */
        pInitCommonControls = (PINITCOMMONCONTROLS)GetProcAddress(hComCtl32,
                                                                  "InitCommonControls");
        if (pInitCommonControls == NULL)
        {
            FreeLibrary(hComCtl32);
            return NULL;
        }

        pInitCommonControls();
    }

    return hComCtl32;
}


BOOL
GetDeviceAndComputerName(LPWSTR lpString,
                         WCHAR szDeviceID[],
                         WCHAR szMachineName[])
{
    BOOL ret = FALSE;

    szDeviceID[0] = L'\0';
    szMachineName[0] = L'\0';

    while (*lpString != L'\0')
    {
        if (*lpString == L'/')
        {
            lpString++;
            if (!_wcsnicmp(lpString, L"DeviceID", 8))
            {
                lpString += 9;
                if (*lpString != L'\0')
                {
                    int i = 0;
                    while ((*lpString != L' ') &&
                           (*lpString != L'\0') &&
                           (i <= MAX_DEVICE_ID_LEN))
                    {
                        szDeviceID[i++] = *lpString++;
                    }
                    szDeviceID[i] = L'\0';
                    ret = TRUE;
                }
            }
            else if (!_wcsnicmp(lpString, L"MachineName", 11))
            {
                lpString += 12;
                if (*lpString != L'\0')
                {
                    int i = 0;
                    while ((*lpString != L' ') &&
                           (*lpString != L'\0') &&
                           (i <= MAX_COMPUTERNAME_LENGTH))
                    {
                        szMachineName[i++] = *lpString++;
                    }
                    szMachineName[i] = L'\0';
                }
            }
            /* knock the pointer back one and let the next
            * pointer deal with incrementing, otherwise we
            * go past the end of the string */
            lpString--;
        }
        lpString++;
    }

    return ret;
}
