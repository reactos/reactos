/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printer Drivers
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include "marshalling/printerdrivers.h"

DWORD
_RpcAddPrinterDriver(WINSPOOL_HANDLE pName, WINSPOOL_DRIVER_CONTAINER* pDriverContainer)
{
    DWORD dwErrorCode;
    PBYTE pDriverInfo = NULL;

    switch ( pDriverContainer->Level )
    {
        case 8:
        {
            WINSPOOL_DRIVER_INFO_8 *pdi = pDriverContainer->DriverInfo.Level8;
            PDRIVER_INFO_8W pdi8w = DllAllocSplMem(sizeof(DRIVER_INFO_8W));
            pDriverInfo = (PBYTE)pdi8w;

            pdi8w->pszPrintProcessor           = pdi->pPrintProcessor;
            pdi8w->pszVendorSetup              = pdi->pVendorSetup;
            pdi8w->pszzColorProfiles           = pdi->pszzColorProfiles;
            pdi8w->pszInfPath                  = pdi->pInfPath;
            pdi8w->pszzCoreDriverDependencies  = pdi->pszzCoreDriverDependencies;
            pdi8w->ftMinInboxDriverVerDate     = pdi->ftMinInboxDriverVerDate;
            pdi8w->dwlMinInboxDriverVerVersion = pdi->dwlMinInboxDriverVerVersion;
        }
        case 6:
        {
            WINSPOOL_DRIVER_INFO_6 *pdi = pDriverContainer->DriverInfo.Level6;
            PDRIVER_INFO_6W pdi6w;

            if ( pDriverInfo == NULL )
            {
                pdi6w = DllAllocSplMem(sizeof(DRIVER_INFO_6W));
                pDriverInfo = (PBYTE)pdi6w;
            }
            else
            {
                pdi6w = (PDRIVER_INFO_6W)pDriverInfo;
            }

            pdi6w->pszMfgName       = pdi->pMfgName;
            pdi6w->pszOEMUrl        = pdi->pOEMUrl;
            pdi6w->pszHardwareID    = pdi->pHardwareID;
            pdi6w->pszProvider      = pdi->pProvider;
            pdi6w->ftDriverDate     = pdi->ftDriverDate;
            pdi6w->dwlDriverVersion = pdi->dwlDriverVersion;
        }
        case 4:
        {
            WINSPOOL_DRIVER_INFO_4 *pdi = pDriverContainer->DriverInfo.Level4;
            PDRIVER_INFO_4W pdi4w;

            if ( pDriverInfo == NULL )
            {
                pdi4w = DllAllocSplMem(sizeof(DRIVER_INFO_4W));
                pDriverInfo = (PBYTE)pdi4w;
            }
            else
            {
                pdi4w = (PDRIVER_INFO_4W)pDriverInfo;
            }

            pdi4w->pszzPreviousNames = pdi->pszzPreviousNames;
        }
        case 3:
        {
            WINSPOOL_DRIVER_INFO_3 *pdi = pDriverContainer->DriverInfo.Level3;
            PDRIVER_INFO_3W pdi3w;

            if ( pDriverInfo == NULL )
            {
                pdi3w = DllAllocSplMem(sizeof(DRIVER_INFO_3W));
                pDriverInfo = (PBYTE)pdi3w;
            }
            else
            {
                pdi3w = (PDRIVER_INFO_3W)pDriverInfo;
            }

            pdi3w->pHelpFile        = pdi->pHelpFile;
            pdi3w->pDependentFiles  = pdi->pDependentFiles;
            pdi3w->pMonitorName     = pdi->pMonitorName;
            pdi3w->pDefaultDataType = pdi->pDefaultDataType;
            pdi3w->pDependentFiles  = pdi->pDependentFiles;
        }
        case 2:
        {
            WINSPOOL_DRIVER_INFO_2 *pdi = pDriverContainer->DriverInfo.Level2;
            PDRIVER_INFO_2W pdi2w;

            if ( pDriverInfo == NULL )
            {
                pdi2w = DllAllocSplMem(sizeof(DRIVER_INFO_2W));
                pDriverInfo = (PBYTE)pdi2w;
            }
            else
            {
                pdi2w = (PDRIVER_INFO_2W)pDriverInfo;
            }

            pdi2w->pName        = pdi->pName;
            pdi2w->pEnvironment = pdi->pEnvironment;
            pdi2w->pDriverPath  = pdi->pDriverPath;
            pdi2w->pDataFile    = pdi->pDataFile;
            pdi2w->pConfigFile  = pdi->pConfigFile;
        }
            break;
        //
        // At this point pDriverInfo is null.
        //
        default:
            return ERROR_INVALID_LEVEL;
    }

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!AddPrinterDriverW( pName, pDriverContainer->Level, pDriverInfo ))
        dwErrorCode = GetLastError();

    if ( pDriverInfo ) DllFreeSplMem( pDriverInfo );

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcAddPrinterDriverEx(WINSPOOL_HANDLE pName, WINSPOOL_DRIVER_CONTAINER* pDriverContainer, DWORD dwFileCopyFlags)
{
    DWORD dwErrorCode;
    PBYTE pDriverInfo = NULL;

    switch ( pDriverContainer->Level )
    {
        case 8:
        {
            WINSPOOL_DRIVER_INFO_8 *pdi = pDriverContainer->DriverInfo.Level8;
            PDRIVER_INFO_8W pdi8w = DllAllocSplMem(sizeof(DRIVER_INFO_8W));
            pDriverInfo = (PBYTE)pdi8w;

            pdi8w->pszPrintProcessor           = pdi->pPrintProcessor;
            pdi8w->pszVendorSetup              = pdi->pVendorSetup;
            pdi8w->pszzColorProfiles           = pdi->pszzColorProfiles;
            pdi8w->pszInfPath                  = pdi->pInfPath;
            pdi8w->pszzCoreDriverDependencies  = pdi->pszzCoreDriverDependencies;
            pdi8w->ftMinInboxDriverVerDate     = pdi->ftMinInboxDriverVerDate;
            pdi8w->dwlMinInboxDriverVerVersion = pdi->dwlMinInboxDriverVerVersion;
        }
        case 6:
        {
            WINSPOOL_DRIVER_INFO_6 *pdi = pDriverContainer->DriverInfo.Level6;
            PDRIVER_INFO_6W pdi6w;

            if ( pDriverInfo == NULL )
            {
                pdi6w = DllAllocSplMem(sizeof(DRIVER_INFO_6W));
                pDriverInfo = (PBYTE)pdi6w;
            }
            else
            {
                pdi6w = (PDRIVER_INFO_6W)pDriverInfo;
            }

            pdi6w->pszMfgName       = pdi->pMfgName;
            pdi6w->pszOEMUrl        = pdi->pOEMUrl;
            pdi6w->pszHardwareID    = pdi->pHardwareID;
            pdi6w->pszProvider      = pdi->pProvider;
            pdi6w->ftDriverDate     = pdi->ftDriverDate;
            pdi6w->dwlDriverVersion = pdi->dwlDriverVersion;
        }
        case 4:
        {
            WINSPOOL_DRIVER_INFO_4 *pdi = pDriverContainer->DriverInfo.Level4;
            PDRIVER_INFO_4W pdi4w;

            if ( pDriverInfo == NULL )
            {
                pdi4w = DllAllocSplMem(sizeof(DRIVER_INFO_4W));
                pDriverInfo = (PBYTE)pdi4w;
            }
            else
            {
                pdi4w = (PDRIVER_INFO_4W)pDriverInfo;
            }

            pdi4w->pszzPreviousNames = pdi->pszzPreviousNames;
        }
        case 3:
        {
            WINSPOOL_DRIVER_INFO_3 *pdi = pDriverContainer->DriverInfo.Level3;
            PDRIVER_INFO_3W pdi3w;

            if ( pDriverInfo == NULL )
            {
                pdi3w = DllAllocSplMem(sizeof(DRIVER_INFO_3W));
                pDriverInfo = (PBYTE)pdi3w;
            }
            else
            {
                pdi3w = (PDRIVER_INFO_3W)pDriverInfo;
            }

            pdi3w->pHelpFile        = pdi->pHelpFile;
            pdi3w->pDependentFiles  = pdi->pDependentFiles;
            pdi3w->pMonitorName     = pdi->pMonitorName;
            pdi3w->pDefaultDataType = pdi->pDefaultDataType;
            pdi3w->pDependentFiles  = pdi->pDependentFiles;
        }
        case 2:
        {
            WINSPOOL_DRIVER_INFO_2 *pdi = pDriverContainer->DriverInfo.Level2;
            PDRIVER_INFO_2W pdi2w;

            if ( pDriverInfo == NULL )
            {
                pdi2w = DllAllocSplMem(sizeof(DRIVER_INFO_2W));
                pDriverInfo = (PBYTE)pdi2w;
            }
            else
            {
                pdi2w = (PDRIVER_INFO_2W)pDriverInfo;
            }

            pdi2w->pName        = pdi->pName;
            pdi2w->pEnvironment = pdi->pEnvironment;
            pdi2w->pDriverPath  = pdi->pDriverPath;
            pdi2w->pDataFile    = pdi->pDataFile;
            pdi2w->pConfigFile  = pdi->pConfigFile;
        }
            break;
        //
        // At this point pDriverInfo is null.
        //
        default:
            return ERROR_INVALID_LEVEL;
    }

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!AddPrinterDriverExW( pName, pDriverContainer->Level, pDriverInfo, dwFileCopyFlags ))
        dwErrorCode = GetLastError();

    if ( pDriverInfo ) DllFreeSplMem( pDriverInfo );

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcDeletePrinterDriver(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, WCHAR* pDriverName)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!DeletePrinterDriverW(pName, pEnvironment, pDriverName))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcDeletePrinterDriverEx(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, WCHAR* pDriverName, DWORD dwDeleteFlag, DWORD dwVersionNum)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!DeletePrinterDriverExW(pName, pEnvironment, pDriverName, dwDeleteFlag, dwVersionNum))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcEnumPrinterDrivers(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, DWORD Level, BYTE* pDrivers, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;
    PBYTE pPrinterDriversEnumAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pPrinterDriversEnumAligned = AlignRpcPtr(pDrivers, &cbBuf);

    if (EnumPrinterDriversW(pName, pEnvironment, Level, pPrinterDriversEnumAligned, cbBuf, pcbNeeded, pcReturned))
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        ASSERT(Level <= 6 || Level == 8);
        MarshallDownStructuresArray(pPrinterDriversEnumAligned, *pcReturned, pPrinterDriverMarshalling[Level]->pInfo, pPrinterDriverMarshalling[Level]->cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pDrivers, pPrinterDriversEnumAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcGetPrinterDriver(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pEnvironment, DWORD Level, BYTE* pDriver, DWORD cbBuf, DWORD* pcbNeeded)
{
    DWORD dwErrorCode;
    PBYTE pDriverAligned;

    TRACE("_RpcGetPrinterDriver(%p, %lu, %lu, %p, %lu, %p)\n", hPrinter, pEnvironment, Level, pDriver, cbBuf, pcbNeeded);

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pDriverAligned = AlignRpcPtr(pDriver, &cbBuf);

    if (GetPrinterDriverW(hPrinter, pEnvironment, Level, pDriverAligned, cbBuf, pcbNeeded))
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level <= 6 || Level == 8);
        MarshallDownStructure(pDriverAligned, pPrinterDriverMarshalling[Level]->pInfo, pPrinterDriverMarshalling[Level]->cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pDriver, pDriverAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

BOOL WINAPI YGetPrinterDriver2(
    HANDLE hPrinter,
    LPWSTR pEnvironment,
    DWORD Level,
    LPBYTE pDriver,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    DWORD dwClientMajorVersion,
    DWORD dwClientMinorVersion,
    PDWORD pdwServerMajorVersion,
    PDWORD pdwServerMinorVersion,
    BOOL bRPC )                    // Seems that all Y fuctions have this.
{
    DWORD dwErrorCode;
    PBYTE pDriverAligned;

    FIXME("_Rpc(Y)GetPrinterDriver2(%p, %lu, %lu, %p, %lu, %p, %lu, %lu, %p, %p)\n", hPrinter, pEnvironment, Level, pDriver, cbBuf, pcbNeeded, dwClientMajorVersion, dwClientMinorVersion, pdwServerMajorVersion, pdwServerMinorVersion);

    if ( bRPC )
    {
        dwErrorCode = RpcImpersonateClient(NULL);
        if (dwErrorCode != ERROR_SUCCESS)
        {
            ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
            return dwErrorCode;
        }
    }

    pDriverAligned = AlignRpcPtr(pDriver, &cbBuf);

    if (GetPrinterDriverExW(hPrinter, pEnvironment, Level, pDriverAligned, cbBuf, pcbNeeded, dwClientMajorVersion, dwClientMinorVersion, pdwServerMajorVersion, pdwServerMinorVersion))
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level <= 6 || Level == 8);
        MarshallDownStructure(pDriverAligned, pPrinterDriverMarshalling[Level]->pInfo, pPrinterDriverMarshalling[Level]->cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    if ( bRPC ) RpcRevertToSelf();
    UndoAlignRpcPtr(pDriver, pDriverAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcGetPrinterDriver2(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pEnvironment, DWORD Level, BYTE* pDriver, DWORD cbBuf, DWORD* pcbNeeded, DWORD dwClientMajorVersion, DWORD dwClientMinorVersion, DWORD* pdwServerMaxVersion, DWORD* pdwServerMinVersion)
{
    DWORD dwErrorCode;
    PBYTE pDriverAligned;

    FIXME("_RpcGetPrinterDriver2(%p, %lu, %lu, %p, %lu, %p, %lu, %lu, %p, %p)\n", hPrinter, pEnvironment, Level, pDriver, cbBuf, pcbNeeded, dwClientMajorVersion, dwClientMinorVersion, pdwServerMaxVersion, pdwServerMinVersion);

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pDriverAligned = AlignRpcPtr(pDriver, &cbBuf);

    if (GetPrinterDriverExW(hPrinter, pEnvironment, Level, pDriverAligned, cbBuf, pcbNeeded, dwClientMajorVersion, dwClientMinorVersion, pdwServerMaxVersion, pdwServerMinVersion))
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level <= 6 || Level == 8);
        MarshallDownStructure(pDriverAligned, pPrinterDriverMarshalling[Level]->pInfo, pPrinterDriverMarshalling[Level]->cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pDriver, pDriverAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcGetPrinterDriverDirectory(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, DWORD Level, BYTE* pDriverDirectory, DWORD cbBuf, DWORD* pcbNeeded)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!GetPrinterDriverDirectoryW(pName, pEnvironment, Level, pDriverDirectory, cbBuf, pcbNeeded))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
