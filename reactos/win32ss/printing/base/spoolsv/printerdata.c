/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printer Configuration Data
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
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
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcGetPrinterDataEx(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR* pKeyName, const WCHAR* pValueName, DWORD* pType, BYTE* pData, DWORD nSize, DWORD* pcbNeeded)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSetPrinterData(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pValueName, DWORD Type, BYTE* pData, DWORD cbData)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcSetPrinterDataEx(WINSPOOL_PRINTER_HANDLE hPrinter, const WCHAR* pKeyName, const WCHAR* pValueName, DWORD Type, BYTE* pData, DWORD cbData)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}
