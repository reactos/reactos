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
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPrinterDriverEx(WINSPOOL_HANDLE pName, WINSPOOL_DRIVER_CONTAINER* pDriverContainer, DWORD dwFileCopyFlags)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterDriver(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, WCHAR* pDriverName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterDriverEx(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, WCHAR* pDriverName, DWORD dwDeleteFlag, DWORD dwVersionNum)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPrinterDrivers(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, DWORD Level, BYTE* pDrivers, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetPrinterDriver(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pEnvironment, DWORD Level, BYTE* pDriver, DWORD cbBuf, DWORD* pcbNeeded)
{
    DWORD dwErrorCode;
    PBYTE pDriverAligned;

    ERR("_RpcGetPrinterDriver(%p, %lu, %lu, %p, %lu, %p)\n", hPrinter, pEnvironment, Level, pDriver, cbBuf, pcbNeeded);

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
        ASSERT(Level >= 1 && Level <= 5);
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
_RpcGetPrinterDriver2(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pEnvironment, DWORD Level, BYTE* pDriver, DWORD cbBuf, DWORD* pcbNeeded, DWORD dwClientMajorVersion, DWORD dwClientMinorVersion, DWORD* pdwServerMaxVersion, DWORD* pdwServerMinVersion)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetPrinterDriverDirectory(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, DWORD Level, BYTE* pDriverDirectory, DWORD cbBuf, DWORD* pcbNeeded)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}
