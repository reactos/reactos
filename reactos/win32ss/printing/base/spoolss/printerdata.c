/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printer Configuration Data
 * COPYRIGHT:   Copyright 2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD WINAPI
GetPrinterDataExW(HANDLE hPrinter, LPCWSTR pKeyName, LPCWSTR pValueName, LPDWORD pType, LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity check.
    if (!pHandle)
    {
        // Yes, Windows checks for the handle here and sets the last error to ERROR_INVALID_HANDLE,
        // but returns FALSE and not the error code.
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    // Call GetPrinterDataEx of the Print Provider.
    return pHandle->pPrintProvider->PrintProvider.fpGetPrinterDataEx(pHandle->hPrinter, pKeyName, pValueName, pType, pData, nSize, pcbNeeded);
}

DWORD WINAPI
GetPrinterDataW(HANDLE hPrinter, LPWSTR pValueName, LPDWORD pType, LPBYTE pData, DWORD nSize, LPDWORD pcbNeeded)
{
    // The ReactOS Printing Stack forwards all GetPrinterData calls to GetPrinterDataEx as soon as possible.
    // This function may only be called if spoolss.dll is used together with Windows Printing Stack components.
    WARN("This function should never be called!\n");
    return GetPrinterDataExW(hPrinter, L"PrinterDriverData", pValueName, pType, pData, nSize, pcbNeeded);
}

DWORD WINAPI
SetPrinterDataExW(HANDLE hPrinter, LPCWSTR pKeyName, LPCWSTR pValueName, DWORD Type, LPBYTE pData, DWORD cbData)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity check.
    if (!pHandle)
    {
        // Yes, Windows checks for the handle here and sets the last error to ERROR_INVALID_HANDLE,
        // but returns FALSE and not the error code.
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    // Call SetPrinterDataEx of the Print Provider.
    return pHandle->pPrintProvider->PrintProvider.fpSetPrinterDataEx(pHandle->hPrinter, pKeyName, pValueName, Type, pData, cbData);
}

DWORD WINAPI
SetPrinterDataW(HANDLE hPrinter, PWSTR pValueName, DWORD Type, PBYTE pData, DWORD cbData)
{
    // The ReactOS Printing Stack forwards all SetPrinterData calls to SetPrinterDataEx as soon as possible.
    // This function may only be called if spoolss.dll is used together with Windows Printing Stack components.
    WARN("This function should never be called!\n");
    return SetPrinterDataExW(hPrinter, L"PrinterDriverData", pValueName, Type, pData, cbData);
}
