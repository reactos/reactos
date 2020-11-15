/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printer Drivers
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/printerdrivers.h>

extern const WCHAR wszCurrentEnvironment[];

static int multi_sz_lenA(const char *str)
{
    const char *ptr = str;
    if(!str) return 0;
    do
    {
        ptr += lstrlenA(ptr) + 1;
    } while(*ptr);

    return ptr - str + 1;
}

static int multi_sz_lenW(const WCHAR *str)
{
    const WCHAR *ptr = str;
    if (!str) return 0;
    do
    {
        ptr += lstrlenW(ptr) + 1;
    } while (*ptr);

    return (ptr - str + 1);
}

BOOL WINAPI
AddPrinterDriverA(PSTR pName, DWORD Level, PBYTE pDriverInfo)
{
    TRACE("AddPrinterDriverA(%s, %lu, %p)\n", pName, Level, pDriverInfo);
    return AddPrinterDriverExA(pName, Level, pDriverInfo, APD_COPY_NEW_FILES);
}

BOOL WINAPI
AddPrinterDriverExA(PSTR pName, DWORD Level, PBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    PDRIVER_INFO_8A  pdiA;
    DRIVER_INFO_8W   diW;
    LPWSTR  nameW = NULL;
    DWORD   lenA;
    DWORD   len;
    BOOL    res = FALSE;

    TRACE("AddPrinterDriverExA(%s, %d, %p, 0x%x)\n", debugstr_a(pName), Level, pDriverInfo, dwFileCopyFlags);

    pdiA = (DRIVER_INFO_8A *) pDriverInfo;
    ZeroMemory(&diW, sizeof(diW));

    if (Level < 2 || Level == 5 || Level == 7 || Level > 8)
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (pdiA == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* convert servername to unicode */
    if (pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    /* common fields */
    diW.cVersion = pdiA->cVersion;

    if (pdiA->pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pName, -1, NULL, 0);
        diW.pName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pName, -1, diW.pName, len);
    }

    if (pdiA->pEnvironment)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pEnvironment, -1, NULL, 0);
        diW.pEnvironment = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pEnvironment, -1, diW.pEnvironment, len);
    }

    if (pdiA->pDriverPath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pDriverPath, -1, NULL, 0);
        diW.pDriverPath = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pDriverPath, -1, diW.pDriverPath, len);
    }

    if (pdiA->pDataFile)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pDataFile, -1, NULL, 0);
        diW.pDataFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pDataFile, -1, diW.pDataFile, len);
    }

    if (pdiA->pConfigFile)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pConfigFile, -1, NULL, 0);
        diW.pConfigFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pConfigFile, -1, diW.pConfigFile, len);
    }

    if ((Level > 2) && pdiA->pHelpFile)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pHelpFile, -1, NULL, 0);
        diW.pHelpFile = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pHelpFile, -1, diW.pHelpFile, len);
    }

    if ((Level > 2) && pdiA->pDependentFiles)
    {
        lenA = multi_sz_lenA(pdiA->pDependentFiles);
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pDependentFiles, lenA, NULL, 0);
        diW.pDependentFiles = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pDependentFiles, lenA, diW.pDependentFiles, len);
    }

    if ((Level > 2) && pdiA->pMonitorName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pMonitorName, -1, NULL, 0);
        diW.pMonitorName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pMonitorName, -1, diW.pMonitorName, len);
    }

    if ((Level > 2) && pdiA->pDefaultDataType)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pDefaultDataType, -1, NULL, 0);
        diW.pDefaultDataType = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pDefaultDataType, -1, diW.pDefaultDataType, len);
    }

    if ((Level > 3) && pdiA->pszzPreviousNames)
    {
        lenA = multi_sz_lenA(pdiA->pszzPreviousNames);
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pszzPreviousNames, lenA, NULL, 0);
        diW.pszzPreviousNames = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pszzPreviousNames, lenA, diW.pszzPreviousNames, len);
    }

    if (Level > 5)
    {
        diW.ftDriverDate = pdiA->ftDriverDate;
        diW.dwlDriverVersion = pdiA->dwlDriverVersion;
    }

    if ((Level > 5) && pdiA->pszMfgName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pszMfgName, -1, NULL, 0);
        diW.pszMfgName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pszMfgName, -1, diW.pszMfgName, len);
    }

    if ((Level > 5) && pdiA->pszOEMUrl)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pszOEMUrl, -1, NULL, 0);
        diW.pszOEMUrl = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pszOEMUrl, -1, diW.pszOEMUrl, len);
    }

    if ((Level > 5) && pdiA->pszHardwareID)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pszHardwareID, -1, NULL, 0);
        diW.pszHardwareID = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pszHardwareID, -1, diW.pszHardwareID, len);
    }

    if ((Level > 5) && pdiA->pszProvider)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pszProvider, -1, NULL, 0);
        diW.pszProvider = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pszProvider, -1, diW.pszProvider, len);
    }

    if ((Level > 7) && pdiA->pszPrintProcessor)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pszPrintProcessor, -1, NULL, 0);
        diW.pszPrintProcessor = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pszPrintProcessor, -1, diW.pszPrintProcessor, len);
    }

    if ((Level > 7) && pdiA->pszVendorSetup)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pszVendorSetup, -1, NULL, 0);
        diW.pszVendorSetup = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pszVendorSetup, -1, diW.pszVendorSetup, len);
    }

    if ((Level > 7) && pdiA->pszzColorProfiles)
    {
        lenA = multi_sz_lenA(pdiA->pszzColorProfiles);
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pszzColorProfiles, lenA, NULL, 0);
        diW.pszzColorProfiles = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pszzColorProfiles, lenA, diW.pszzColorProfiles, len);
    }

    if ((Level > 7) && pdiA->pszInfPath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pszInfPath, -1, NULL, 0);
        diW.pszInfPath = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pszInfPath, -1, diW.pszInfPath, len);
    }

    if ((Level > 7) && pdiA->pszzCoreDriverDependencies)
    {
        lenA = multi_sz_lenA(pdiA->pszzCoreDriverDependencies);
        len = MultiByteToWideChar(CP_ACP, 0, pdiA->pszzCoreDriverDependencies, lenA, NULL, 0);
        diW.pszzCoreDriverDependencies = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pdiA->pszzCoreDriverDependencies, lenA, diW.pszzCoreDriverDependencies, len);
    }

    if (Level > 7)
    {
        diW.dwPrinterDriverAttributes = pdiA->dwPrinterDriverAttributes;
        diW.ftMinInboxDriverVerDate = pdiA->ftMinInboxDriverVerDate;
        diW.dwlMinInboxDriverVerVersion = pdiA->dwlMinInboxDriverVerVersion;
    }

    res = AddPrinterDriverExW(nameW, Level, (LPBYTE) &diW, dwFileCopyFlags);

    TRACE("got %u with %u\n", res, GetLastError());
    if (nameW) HeapFree(GetProcessHeap(), 0, nameW);
    if (diW.pName) HeapFree(GetProcessHeap(), 0, diW.pName);
    if (diW.pEnvironment) HeapFree(GetProcessHeap(), 0, diW.pEnvironment);
    if (diW.pDriverPath) HeapFree(GetProcessHeap(), 0, diW.pDriverPath);
    if (diW.pDataFile) HeapFree(GetProcessHeap(), 0, diW.pDataFile);
    if (diW.pConfigFile) HeapFree(GetProcessHeap(), 0, diW.pConfigFile);
    if (diW.pHelpFile) HeapFree(GetProcessHeap(), 0, diW.pHelpFile);
    if (diW.pDependentFiles) HeapFree(GetProcessHeap(), 0, diW.pDependentFiles);
    if (diW.pMonitorName) HeapFree(GetProcessHeap(), 0, diW.pMonitorName);
    if (diW.pDefaultDataType) HeapFree(GetProcessHeap(), 0, diW.pDefaultDataType);
    if (diW.pszzPreviousNames) HeapFree(GetProcessHeap(), 0, diW.pszzPreviousNames);
    if (diW.pszMfgName) HeapFree(GetProcessHeap(), 0, diW.pszMfgName);
    if (diW.pszOEMUrl) HeapFree(GetProcessHeap(), 0, diW.pszOEMUrl);
    if (diW.pszHardwareID) HeapFree(GetProcessHeap(), 0, diW.pszHardwareID);
    if (diW.pszProvider) HeapFree(GetProcessHeap(), 0, diW.pszProvider);
    if (diW.pszPrintProcessor) HeapFree(GetProcessHeap(), 0, diW.pszPrintProcessor);
    if (diW.pszVendorSetup) HeapFree(GetProcessHeap(), 0, diW.pszVendorSetup);
    if (diW.pszzColorProfiles) HeapFree(GetProcessHeap(), 0, diW.pszzColorProfiles);
    if (diW.pszInfPath) HeapFree(GetProcessHeap(), 0, diW.pszInfPath);
    if (diW.pszzCoreDriverDependencies) HeapFree(GetProcessHeap(), 0, diW.pszzCoreDriverDependencies);

    TRACE("=> %u with %u\n", res, GetLastError());
    return res;
}

BOOL WINAPI
AddPrinterDriverExW(PWSTR pName, DWORD Level, PBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    DWORD dwErrorCode = ERROR_SUCCESS;
    WINSPOOL_DRIVER_INFO_8 * pdi = NULL;
    WINSPOOL_DRIVER_CONTAINER pDriverContainer;

    TRACE("AddPrinterDriverExW(%S, %lu, %p, %lu)\n", pName, Level, pDriverInfo, dwFileCopyFlags);

    pDriverContainer.Level = Level;

    switch (Level)
    {
        case 8:
        {
            PDRIVER_INFO_8W pdi8w = (PDRIVER_INFO_8W)pDriverInfo;
            pdi = HeapAlloc(hProcessHeap, 0, sizeof(WINSPOOL_DRIVER_INFO_8));

            pdi->pPrintProcessor   = pdi8w->pszPrintProcessor;
            pdi->pVendorSetup      = pdi8w->pszVendorSetup;

            pdi->pszzColorProfiles = pdi8w->pszzColorProfiles;
            pdi->cchColorProfiles = 0;
            if ( pdi8w->pszzColorProfiles && *pdi8w->pszzColorProfiles )
            {
                pdi->cchColorProfiles = multi_sz_lenW( pdi8w->pszzColorProfiles );
            }

            pdi->pInfPath = pdi8w->pszInfPath;

            pdi->pszzCoreDriverDependencies = pdi8w->pszzCoreDriverDependencies;
            pdi->cchCoreDependencies = 0;
            if ( pdi8w->pszzCoreDriverDependencies && *pdi8w->pszzCoreDriverDependencies )
            {
                pdi->cchCoreDependencies = multi_sz_lenW( pdi8w->pszzCoreDriverDependencies );
            }

            pdi->ftMinInboxDriverVerDate     = pdi8w->ftMinInboxDriverVerDate;
            pdi->dwlMinInboxDriverVerVersion = pdi8w->dwlMinInboxDriverVerVersion;
        }
        case 6:
        {
            PDRIVER_INFO_6W pdi6w = (PDRIVER_INFO_6W)pDriverInfo;
            if ( pdi == NULL ) pdi = HeapAlloc(hProcessHeap, 0, sizeof(WINSPOOL_DRIVER_INFO_6));

            pdi->pMfgName         = pdi6w->pszMfgName;
            pdi->pOEMUrl          = pdi6w->pszOEMUrl;
            pdi->pHardwareID      = pdi6w->pszHardwareID;
            pdi->pProvider        = pdi6w->pszProvider;
            pdi->ftDriverDate     = pdi6w->ftDriverDate;
            pdi->dwlDriverVersion = pdi6w->dwlDriverVersion;
        }
        case 4:
        {
            PDRIVER_INFO_4W pdi4w = (PDRIVER_INFO_4W)pDriverInfo;
            if ( pdi == NULL )  pdi = HeapAlloc(hProcessHeap, 0, sizeof(WINSPOOL_DRIVER_INFO_4));

            pdi->pszzPreviousNames = pdi4w->pszzPreviousNames;
            pdi->cchPreviousNames  = 0;
            if ( pdi4w->pDependentFiles && *pdi4w->pDependentFiles )
            {
               pdi->cchPreviousNames = multi_sz_lenW( pdi4w->pDependentFiles );
            }
        }
        case 3:
        {
            PDRIVER_INFO_3W pdi3w = (PDRIVER_INFO_3W)pDriverInfo;
            if ( pdi == NULL ) pdi = HeapAlloc(hProcessHeap, 0, sizeof(WINSPOOL_DRIVER_INFO_3));

            pdi->pHelpFile        = pdi3w->pHelpFile;
            pdi->pDependentFiles  = pdi3w->pDependentFiles;
            pdi->pMonitorName     = pdi3w->pMonitorName;
            pdi->pDefaultDataType = pdi3w->pDefaultDataType;

            pdi->pDependentFiles = pdi3w->pDependentFiles;
            pdi->cchDependentFiles = 0;
            if ( pdi3w->pDependentFiles && *pdi3w->pDependentFiles )
            {
                pdi->cchDependentFiles = multi_sz_lenW( pdi3w->pDependentFiles );
            }
        }
        case 2:
        {
            PDRIVER_INFO_2W pdi2w = (PDRIVER_INFO_2W)pDriverInfo;
            if ( pdi == NULL ) pdi = HeapAlloc(hProcessHeap, 0, sizeof(WINSPOOL_DRIVER_INFO_2));

            pdi->pName = pdi2w->pName;

            pdi->pEnvironment = pdi2w->pEnvironment;
            if ( !pdi2w->pEnvironment || !*pdi2w->pEnvironment )
            {
                pdi2w->pEnvironment = (PWSTR)wszCurrentEnvironment;
            }

            pdi->pDriverPath = pdi2w->pDriverPath;
            pdi->pDataFile   = pdi2w->pDataFile;
            pdi->pConfigFile = pdi2w->pConfigFile;
        }
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
    }

    pDriverContainer.DriverInfo.Level8 = pdi;

    RpcTryExcept
    {
        dwErrorCode = _RpcAddPrinterDriverEx( pName, &pDriverContainer, dwFileCopyFlags );
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcAddPrinterDriverEx failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if ( pdi ) HeapFree( GetProcessHeap(), 0, pdi );

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
AddPrinterDriverW(PWSTR pName, DWORD Level, PBYTE pDriverInfo)
{
    TRACE("AddPrinterDriverW(%S, %lu, %p)\n", pName, Level, pDriverInfo);
    return AddPrinterDriverExW(pName, Level, pDriverInfo, APD_COPY_NEW_FILES);
}

BOOL WINAPI
DeletePrinterDriverA(PSTR pName, PSTR pEnvironment, PSTR pDriverName)
{
    TRACE("DeletePrinterDriverA(%s, %s, %s)\n", pName, pEnvironment, pDriverName);
    return DeletePrinterDriverExA(pName, pEnvironment, pDriverName, 0, 0);
}

BOOL WINAPI
DeletePrinterDriverExA(PSTR pName, PSTR pEnvironment, PSTR pDriverName, DWORD dwDeleteFlag, DWORD dwVersionFlag)
{
    DWORD dwErrorCode;
    UNICODE_STRING NameW, EnvW, DriverW;
    BOOL ret;

    TRACE("DeletePrinterDriverExA(%s, %s, %s, %lu, %lu)\n", pName, pEnvironment, pDriverName, dwDeleteFlag, dwVersionFlag);

    AsciiToUnicode(&NameW, pName);
    AsciiToUnicode(&EnvW, pEnvironment);
    AsciiToUnicode(&DriverW, pDriverName);

    ret = DeletePrinterDriverExW(NameW.Buffer, EnvW.Buffer, DriverW.Buffer, dwDeleteFlag, dwVersionFlag);

    dwErrorCode = GetLastError();

    RtlFreeUnicodeString(&DriverW);
    RtlFreeUnicodeString(&EnvW);
    RtlFreeUnicodeString(&NameW);

    SetLastError(dwErrorCode);
    return ret;
}

BOOL WINAPI
DeletePrinterDriverExW(PWSTR pName, PWSTR pEnvironment, PWSTR pDriverName, DWORD dwDeleteFlag, DWORD dwVersionFlag)
{
    DWORD dwErrorCode;

    TRACE("DeletePrinterDriverExW(%S, %S, %S, %lu, %lu)\n", pName, pEnvironment, pDriverName, dwDeleteFlag, dwVersionFlag);

    if ( !pDriverName || !*pDriverName )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( !pEnvironment || !*pEnvironment )
    {
        pEnvironment = (PWSTR)wszCurrentEnvironment;
    }

    // Do the RPC call.
    RpcTryExcept
    {
        dwErrorCode = _RpcDeletePrinterDriverEx(pName, pEnvironment, pDriverName, dwDeleteFlag, dwVersionFlag);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcDeletePrinterDriverEx failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);

}

BOOL WINAPI
DeletePrinterDriverW(PWSTR pName, PWSTR pEnvironment, PWSTR pDriverName)
{
    TRACE("DeletePrinterDriverW(%S, %S, %S)\n", pName, pEnvironment, pDriverName);
    return DeletePrinterDriverExW(pName, pEnvironment, pDriverName, 0, 0);
}

BOOL WINAPI
EnumPrinterDriversA(PSTR pName, PSTR pEnvironment, DWORD Level, PBYTE pDriverInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    BOOL ret = FALSE;
    DWORD dwErrorCode, i;
    UNICODE_STRING pNameW, pEnvironmentW;
    PWSTR pwstrNameW, pwstrEnvironmentW;
    PDRIVER_INFO_1W pdi1w = (PDRIVER_INFO_1W)pDriverInfo;
    PDRIVER_INFO_8W pdi8w = (PDRIVER_INFO_8W)pDriverInfo;

    FIXME("EnumPrinterDriversA(%s, %s, %lu, %p, %lu, %p, %p)\n", pName, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded, pcReturned);

    pwstrNameW = AsciiToUnicode(&pNameW, pName);
    pwstrEnvironmentW = AsciiToUnicode(&pEnvironmentW, pEnvironment);

    ret = EnumPrinterDriversW( pwstrNameW, pwstrEnvironmentW, Level, pDriverInfo, cbBuf, pcbNeeded, pcReturned );

    dwErrorCode = GetLastError();

    if (ret)
    {
        for ( i = 0; i < *pcReturned; i++ )
        {
            switch (Level)
            {
                case 1:
                {
                    dwErrorCode = UnicodeToAnsiInPlace(pdi1w[i].pName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    break;
                }
                case 8:
                {
                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pszPrintProcessor);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pszVendorSetup);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiZZInPlace(pdi8w[i].pszzColorProfiles);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pszInfPath);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiZZInPlace(pdi8w[i].pszzCoreDriverDependencies);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                }
                case 6:
                {
                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pszMfgName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }

                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pszOEMUrl);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }

                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pszHardwareID);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }

                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pszProvider);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                }
                case 4:
                {
                    dwErrorCode = UnicodeToAnsiZZInPlace(pdi8w[i].pszzPreviousNames);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                }
                case 3:
                {
                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pHelpFile);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }

                    dwErrorCode = UnicodeToAnsiZZInPlace(pdi8w[i].pDependentFiles);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }

                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pMonitorName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }

                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pDefaultDataType);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                }
                case 2:
                case 5:
                {
                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }

                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pEnvironment);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }

                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pDriverPath);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }

                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pDataFile);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }

                    dwErrorCode = UnicodeToAnsiInPlace(pdi8w[i].pConfigFile);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                }
            }
        }
        dwErrorCode = ERROR_SUCCESS;
    }
Cleanup:
    RtlFreeUnicodeString(&pNameW);
    RtlFreeUnicodeString(&pEnvironmentW);
    SetLastError(dwErrorCode);
    FIXME("EnumPrinterDriversA Exit %d Err %d\n",ret,GetLastError());
    return ret;
}

BOOL WINAPI
EnumPrinterDriversW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pDriverInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;

    FIXME("EnumPrinterDriversW(%S, %S, %lu, %p, %lu, %p, %p)\n", pName, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded, pcReturned);

    // Dismiss invalid levels already at this point.
    if (Level < 1 || Level == 7 || Level > 8)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if ( !pEnvironment || !*pEnvironment )
    {
        pEnvironment = (PWSTR)wszCurrentEnvironment;
    }

    if (cbBuf && pDriverInfo)
        ZeroMemory(pDriverInfo, cbBuf);

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumPrinterDrivers( pName, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded, pcReturned );
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcEnumPrinterDrivers failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level <= 6 || Level == 8);
        MarshallUpStructuresArray(cbBuf, pDriverInfo, *pcReturned, pPrinterDriverMarshalling[Level]->pInfo, pPrinterDriverMarshalling[Level]->cbStructureSize, TRUE);
    }

Cleanup:
    SetLastError(dwErrorCode); FIXME("EnumPrinterDriversW Exit Err %d\n",dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);

}

BOOL WINAPI
GetPrinterDriverA(HANDLE hPrinter, LPSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    /*
     * We are mapping multiple different pointers to the same pDriverInfo pointer here so that
     * we can use the same incoming pointer for different Levels
     */
    PDRIVER_INFO_1W pdi1w = (PDRIVER_INFO_1W)pDriverInfo;
    PDRIVER_INFO_8W pdi8w = (PDRIVER_INFO_8W)pDriverInfo;

    DWORD cch;
    PWSTR pwszEnvironment = NULL;

    TRACE("GetPrinterDriverA(%p, %s, %lu, %p, %lu, %p)\n", hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded);

    // Check for invalid levels here for early error return. Should be 1-6 & 8.
    if (Level <  1 || Level == 7 || Level > 8)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        ERR("Invalid Level! %d\n",Level);
        goto Cleanup;
    }

    if (pEnvironment)
    {
        // Convert pEnvironment to a Unicode string pwszEnvironment.
        cch = strlen(pEnvironment);

        pwszEnvironment = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszEnvironment)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, pwszEnvironment, cch + 1);
    }

    if (!GetPrinterDriverW(hPrinter, pwszEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded))
    {
        dwErrorCode = GetLastError();
        goto Cleanup;
    }

    // Do Unicode to ANSI conversions for strings based on Level
    switch (Level)
    {
        case 1:
        {
            dwErrorCode = UnicodeToAnsiInPlace(pdi1w->pName);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
            break;
        }
        case 8:
        {
            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pszPrintProcessor);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pszVendorSetup);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
            dwErrorCode = UnicodeToAnsiZZInPlace(pdi8w->pszzColorProfiles);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pszInfPath);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
            dwErrorCode = UnicodeToAnsiZZInPlace(pdi8w->pszzCoreDriverDependencies);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }
        case 6:
        {
            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pszMfgName);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pszOEMUrl);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pszHardwareID);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pszProvider);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }
        case 4:
        {
            dwErrorCode = UnicodeToAnsiZZInPlace(pdi8w->pszzPreviousNames);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }
        case 3:
        {
            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pHelpFile);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            dwErrorCode = UnicodeToAnsiZZInPlace(pdi8w->pDependentFiles);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pMonitorName);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pDefaultDataType);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }
        case 2:
        case 5:
        {
            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pConfigFile);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pDataFile);

            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pDriverPath);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pEnvironment);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }

            dwErrorCode = UnicodeToAnsiInPlace(pdi8w->pName);
            if (dwErrorCode != ERROR_SUCCESS)
            {
                goto Cleanup;
            }
        }
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    if (pwszEnvironment)
    {
        HeapFree(hProcessHeap, 0, pwszEnvironment);
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("GetPrinterDriverW(%p, %S, %lu, %p, %lu, %p)\n", hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Dismiss invalid levels already at this point.
    if (Level > 8 || Level == 7 || Level < 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if ( !pEnvironment || !*pEnvironment )
    {
        pEnvironment = (PWSTR)wszCurrentEnvironment;
    }

    if (cbBuf && pDriverInfo)
        ZeroMemory(pDriverInfo, cbBuf);

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcGetPrinterDriver(pHandle->hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcGetPrinterDriver failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level <= 6 || Level == 8);
        MarshallUpStructure(cbBuf, pDriverInfo, pPrinterDriverMarshalling[Level]->pInfo, pPrinterDriverMarshalling[Level]->cbStructureSize, TRUE);
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetPrinterDriverDirectoryA(PSTR pName, PSTR pEnvironment, DWORD Level, PBYTE pDriverDirectory, DWORD cbBuf, PDWORD pcbNeeded)
{
    UNICODE_STRING nameW, environmentW;
    BOOL ret;
    DWORD pcbNeededW;
    INT len = cbBuf * sizeof(WCHAR)/sizeof(CHAR);
    WCHAR *driverDirectoryW = NULL;

    TRACE("GetPrinterDriverDirectoryA(%s, %s, %d, %p, %d, %p)\n", debugstr_a(pName), debugstr_a(pEnvironment), Level, pDriverDirectory, cbBuf, pcbNeeded);

    if (len) driverDirectoryW = HeapAlloc( GetProcessHeap(), 0, len );

    if (pName)
    {
        RtlCreateUnicodeStringFromAsciiz(&nameW, pName);
    }
    else
    {
        nameW.Buffer = NULL;
    }
    if (pEnvironment)
    {
        RtlCreateUnicodeStringFromAsciiz(&environmentW, pEnvironment);
    }
    else
    {
        environmentW.Buffer = NULL;
    }

    ret = GetPrinterDriverDirectoryW( nameW.Buffer, environmentW.Buffer, Level, (LPBYTE)driverDirectoryW, len, &pcbNeededW );

    if (ret)
    {
        DWORD needed =  WideCharToMultiByte( CP_ACP, 0, driverDirectoryW, -1, (LPSTR)pDriverDirectory, cbBuf, NULL, NULL);

        if ( pcbNeeded )
            *pcbNeeded = needed;

        ret = needed <= cbBuf;
    }
    else
    {
        if (pcbNeeded) *pcbNeeded = pcbNeededW * sizeof(CHAR)/sizeof(WCHAR);
    }

    TRACE("required: 0x%x/%d\n", pcbNeeded ? *pcbNeeded : 0, pcbNeeded ? *pcbNeeded : 0);

    HeapFree( GetProcessHeap(), 0, driverDirectoryW );
    RtlFreeUnicodeString(&environmentW);
    RtlFreeUnicodeString(&nameW);

    return ret;
}

BOOL WINAPI
GetPrinterDriverDirectoryW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pDriverDirectory, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;

    TRACE("GetPrinterDriverDirectoryW(%S, %S, %lu, %p, %lu, %p)\n", pName, pEnvironment, Level, pDriverDirectory, cbBuf, pcbNeeded);

    if (Level != 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if ( !pEnvironment || !*pEnvironment )
    {
        pEnvironment = (PWSTR)wszCurrentEnvironment;
    }

    // Do the RPC call.
    RpcTryExcept
    {
        dwErrorCode = _RpcGetPrinterDriverDirectory(pName, pEnvironment, Level, pDriverDirectory, cbBuf, pcbNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcGetPrinterDriverDirectory failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
