/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printer Configuration Data
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD
_RpcDeletePrinterData(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pValueName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterDataEx(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR* pKeyName, const WCHAR* pValueName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrinterKey(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR* pKeyName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPrinterData(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD dwIndex, WCHAR* pValueName, DWORD cbValueName, DWORD* pcbValueName, DWORD* pType, BYTE* pData, DWORD cbData, DWORD* pcbData)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPrinterKey(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR* pKeyName, WCHAR* pSubkey, DWORD cbSubkey, DWORD* pcbSubkey)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPrinterDataEx(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR* pKeyName, BYTE* pEnumValues, DWORD cbEnumValues, DWORD* pcbEnumValues, DWORD* pnEnumValues)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetPrinterData(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pValueName, DWORD* pType, BYTE* pData, DWORD nSize, DWORD* pcbNeeded)
{
    return _RpcGetPrinterDataEx(hPrinter, L"PrinterDriverData", pValueName, pType, pData, nSize, pcbNeeded);
}

DWORD
_RpcGetPrinterDataEx(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR* pKeyName, const WCHAR* pValueName, DWORD* pType, BYTE* pData, DWORD nSize, DWORD* pcbNeeded)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    dwErrorCode = GetPrinterDataExW(hPrinter, pKeyName, pValueName, pType, pData, nSize, pcbNeeded);

    RpcRevertToSelf();

    return dwErrorCode;
}

DWORD
_RpcSetPrinterData(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pValueName, DWORD Type, BYTE* pData, DWORD cbData)
{
    return _RpcSetPrinterDataEx(hPrinter, L"PrinterDriverData", pValueName, Type, pData, cbData);
}

DWORD
_RpcSetPrinterDataEx(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR* pKeyName, const WCHAR* pValueName, DWORD Type, BYTE* pData, DWORD cbData)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    dwErrorCode = SetPrinterDataExW(hPrinter, pKeyName, pValueName, Type, pData, cbData);

    RpcRevertToSelf();

    return dwErrorCode;
}
