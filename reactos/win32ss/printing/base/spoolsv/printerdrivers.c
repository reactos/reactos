/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printer Drivers
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

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
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
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
