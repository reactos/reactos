/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

BOOL WINAPI
EnumPrintersA(DWORD Flags, LPSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    return FALSE;
}

BOOL WINAPI
EnumPrintersW(DWORD Flags, LPWSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    BOOL bReturnValue = FALSE;
    DWORD dwErrorCode;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumPrinters(Flags, Name, Level, pPrinterEnum, cbBuf, pcbNeeded, pcReturned);
        SetLastError(dwErrorCode);
        bReturnValue = (dwErrorCode == ERROR_SUCCESS);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("_RpcEnumPrinters failed with exception code %lu!\n", RpcExceptionCode());
    }
    RpcEndExcept;

    return bReturnValue;
}

BOOL WINAPI
ClosePrinter(HANDLE hPrinter)
{
    return FALSE;
}

DWORD WINAPI
DeviceCapabilitiesA(LPCSTR pDevice, LPCSTR pPort, WORD fwCapability, LPSTR pOutput, const DEVMODEA* pDevMode)
{
    return 0;
}

DWORD WINAPI
DeviceCapabilitiesW(LPCWSTR pDevice, LPCWSTR pPort, WORD fwCapability, LPWSTR pOutput, const DEVMODEW* pDevMode)
{
    return 0;
}

LONG WINAPI
DocumentPropertiesA(HWND hWnd, HANDLE hPrinter, LPSTR pDeviceName, PDEVMODEA pDevModeOutput, PDEVMODEA pDevModeInput, DWORD fMode)
{
    return 0;
}

LONG WINAPI
DocumentPropertiesW(HWND hWnd, HANDLE hPrinter, LPWSTR pDeviceName, PDEVMODEW pDevModeOutput, PDEVMODEW pDevModeInput, DWORD fMode)
{
    return 0;
}

BOOL WINAPI
EndDocPrinter(HANDLE hPrinter)
{
    return FALSE;
}

BOOL WINAPI
EndPagePrinter(HANDLE hPrinter)
{
    return FALSE;
}

BOOL WINAPI
GetDefaultPrinterA(LPSTR pszBuffer, LPDWORD pcchBuffer)
{
    return FALSE;
}

BOOL WINAPI
GetDefaultPrinterW(LPWSTR pszBuffer, LPDWORD pcchBuffer)
{
    return FALSE;
}

BOOL WINAPI
GetPrinterA(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD cbBuf, LPDWORD pcbNeeded)
{
    return FALSE;
}

BOOL WINAPI
GetPrinterDriverA(HANDLE hPrinter, LPSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    return FALSE;
}

BOOL WINAPI
GetPrinterDriverW(HANDLE hPrinter, LPWSTR pEnvironment, DWORD Level, LPBYTE pDriverInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    return FALSE;
}

BOOL WINAPI
GetPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pPrinter, DWORD cbBuf, LPDWORD pcbNeeded)
{
    return FALSE;
}

BOOL WINAPI
OpenPrinterA(LPSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSA pDefault)
{
    BOOL bReturnValue = FALSE;
    PWSTR pwszPrinterName = NULL;
    PWSTR pwszDatatype = NULL;
    PRINTER_DEFAULTSW wDefault = { 0 };
    size_t StringLength;

    if (pPrinterName)
    {
        // Convert pPrinterName to a Unicode string pwszPrinterName
        StringLength = strlen(pPrinterName) + 1;

        pwszPrinterName = HeapAlloc(GetProcessHeap(), 0, StringLength * sizeof(WCHAR));
        if (!pwszPrinterName)
        {
            ERR("HeapAlloc failed for pwszPrinterName with last error %lu!\n", GetLastError());
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pPrinterName, -1, pwszPrinterName, StringLength);
    }

    if (pDefault)
    {
        wDefault.DesiredAccess = pDefault->DesiredAccess;

        if (pDefault->pDatatype)
        {
            // Convert pDefault->pDatatype to a Unicode string pwszDatatype that later becomes wDefault.pDatatype
            StringLength = strlen(pDefault->pDatatype) + 1;

            pwszDatatype = HeapAlloc(GetProcessHeap(), 0, StringLength * sizeof(WCHAR));
            if (!pwszDatatype)
            {
                ERR("HeapAlloc failed for pwszDatatype with last error %lu!\n", GetLastError());
                goto Cleanup;
            }

            MultiByteToWideChar(CP_ACP, 0, pDefault->pDatatype, -1, pwszDatatype, StringLength);
            wDefault.pDatatype = pwszDatatype;
        }

        if (pDefault->pDevMode)
            wDefault.pDevMode = GdiConvertToDevmodeW(pDefault->pDevMode);
    }

    bReturnValue = OpenPrinterW(pwszPrinterName, phPrinter, &wDefault);

Cleanup:
    if (wDefault.pDevMode)
        HeapFree(GetProcessHeap(), 0, wDefault.pDevMode);

    if (pwszPrinterName)
        HeapFree(GetProcessHeap(), 0, pwszPrinterName);

    if (pwszDatatype)
        HeapFree(GetProcessHeap(), 0, pwszDatatype);

    return bReturnValue;
}

BOOL WINAPI
OpenPrinterW(LPWSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSW pDefault)
{
    BOOL bReturnValue = FALSE;
    DWORD dwErrorCode;
    PWSTR pDatatype = NULL;
    WINSPOOL_DEVMODE_CONTAINER DevModeContainer;
    WINSPOOL_DEVMODE_CONTAINER* pDevModeContainer = NULL;
    ACCESS_MASK AccessRequired = 0;

    // Prepare the additional parameters in the format required by _RpcOpenPrinter
    if (pDefault)
    {
        pDatatype = pDefault->pDatatype;
        DevModeContainer.cbBuf = sizeof(DEVMODEW);
        DevModeContainer.pDevMode = (BYTE*)pDefault->pDevMode;
        pDevModeContainer = &DevModeContainer;
        AccessRequired = pDefault->DesiredAccess;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcOpenPrinter(pPrinterName, phPrinter, pDatatype, pDevModeContainer, AccessRequired);
        SetLastError(dwErrorCode);
        bReturnValue = (dwErrorCode == ERROR_SUCCESS);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("_RpcOpenPrinter failed with exception code %lu!\n", RpcExceptionCode());
    }
    RpcEndExcept;

    return bReturnValue;
}

BOOL WINAPI
ReadPrinter(HANDLE hPrinter, PVOID pBuf, DWORD cbBuf, PDWORD pNoBytesRead)
{
    return FALSE;
}

DWORD WINAPI
StartDocPrinterW(HANDLE hPrinter, DWORD Level, LPBYTE pDocInfo)
{
    return 0;
}

BOOL WINAPI
StartPagePrinter(HANDLE hPrinter)
{
    return FALSE;
}

BOOL WINAPI
WritePrinter(HANDLE hPrinter, LPVOID pBuf, DWORD cbBuf, LPDWORD pcWritten)
{
    return FALSE;
}

BOOL WINAPI
XcvDataW(HANDLE hXcv, PCWSTR pszDataName, PBYTE pInputData, DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded, PDWORD pdwStatus)
{
    return FALSE;
}
