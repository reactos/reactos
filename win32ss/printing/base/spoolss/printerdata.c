/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printer Configuration Data
 * COPYRIGHT:   Copyright 2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

DWORD WINAPI
DeletePrinterDataExW(HANDLE hPrinter, PCWSTR pKeyName, PCWSTR pValueName)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpDeletePrinterDataEx(pHandle->hPrinter, pKeyName, pValueName);
}

DWORD WINAPI
DeletePrinterDataW(HANDLE hPrinter, PWSTR pValueName)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpDeletePrinterData(pHandle->hPrinter, pValueName);
}

DWORD WINAPI
DeletePrinterKeyW(HANDLE hPrinter, PCWSTR pKeyName)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpDeletePrinterKey(pHandle->hPrinter, pKeyName);
}

DWORD WINAPI
EnumPrinterDataExW(HANDLE hPrinter, PCWSTR pKeyName, PBYTE pEnumValues, DWORD cbEnumValues, PDWORD pcbEnumValues, PDWORD pnEnumValues)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpEnumPrinterDataEx(pHandle->hPrinter, pKeyName, pEnumValues, cbEnumValues, pcbEnumValues, pnEnumValues);
}

DWORD WINAPI
EnumPrinterDataW(HANDLE hPrinter, DWORD dwIndex, PWSTR pValueName, DWORD cbValueName, PDWORD pcbValueName, PDWORD pType, PBYTE pData, DWORD cbData, PDWORD pcbData)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpEnumPrinterData(pHandle->hPrinter, dwIndex, pValueName, cbValueName, pcbValueName, pType, pData, cbData, pcbData);
}

DWORD WINAPI
EnumPrinterKeyW(HANDLE hPrinter, PCWSTR pKeyName, PWSTR pSubkey, DWORD cbSubkey, PDWORD pcbSubkey)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpEnumPrinterKey(pHandle->hPrinter, pKeyName, pSubkey, cbSubkey, pcbSubkey);
}

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
