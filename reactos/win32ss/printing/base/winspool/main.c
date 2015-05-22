/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

handle_t __RPC_USER
WINSPOOL_HANDLE_bind(WINSPOOL_HANDLE wszName)
{
    handle_t hBinding;
    PWSTR wszStringBinding;
    RPC_STATUS Status;

    // Get us a string binding handle from the supplied connection information
    Status = RpcStringBindingComposeW(NULL, L"ncacn_np", wszName, L"\\pipe\\spoolss", NULL, &wszStringBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcStringBindingComposeW failed with status %ld!\n", Status);
        return NULL;
    }

    // Get a handle_t binding handle from the string binding handle
    Status = RpcBindingFromStringBindingW(wszStringBinding, &hBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcBindingFromStringBindingW failed with status %ld!\n", Status);
        return NULL;
    }

    // Free the string binding handle
    Status = RpcStringFreeW(&wszStringBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcStringFreeW failed with status %ld!\n", Status);
        return NULL;
    }

    return hBinding;
}

void __RPC_USER
WINSPOOL_HANDLE_unbind(WINSPOOL_HANDLE wszName, handle_t hBinding)
{
    RPC_STATUS Status;

    Status = RpcBindingFree(&hBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcBindingFree failed with status %ld!\n", Status);
    }
}

void __RPC_FAR* __RPC_USER
midl_user_allocate(SIZE_T len)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
}

void __RPC_USER
midl_user_free(void __RPC_FAR* ptr)
{
    HeapFree(GetProcessHeap(), 0, ptr);
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
EnumPrintersA(DWORD Flags, LPSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    return FALSE;
}

BOOL WINAPI
EnumPrintersW(DWORD Flags, LPWSTR Name, DWORD Level, LPBYTE pPrinterEnum, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    return FALSE;
}

BOOL WINAPI
EnumPrintProcessorDatatypesA(LPSTR pName, LPSTR pPrintProcessorName, DWORD Level, LPBYTE pDatatypes, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
{
    return FALSE;
}

BOOL WINAPI
EnumPrintProcessorDatatypesW(LPWSTR pName, LPWSTR pPrintProcessorName, DWORD Level, LPBYTE pDatatypes, DWORD cbBuf, LPDWORD pcbNeeded, LPDWORD pcReturned)
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
GetPrintProcessorDirectoryW(LPWSTR pName, LPWSTR pEnvironment, DWORD Level, LPBYTE pPrintProcessorInfo, DWORD cbBuf, LPDWORD pcbNeeded)
{
    return FALSE;
}

BOOL WINAPI
OpenPrinterA(LPSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSA pDefault)
{
    BOOL ReturnValue = FALSE;
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

    ReturnValue = OpenPrinterW(pwszPrinterName, phPrinter, &wDefault);

Cleanup:
    if (wDefault.pDevMode)
        HeapFree(GetProcessHeap(), 0, wDefault.pDevMode);

    if (pwszPrinterName)
        HeapFree(GetProcessHeap(), 0, pwszPrinterName);

    if (pwszDatatype)
        HeapFree(GetProcessHeap(), 0, pwszDatatype);

    return ReturnValue;
}

BOOL WINAPI
OpenPrinterW(LPWSTR pPrinterName, LPHANDLE phPrinter, LPPRINTER_DEFAULTSW pDefault)
{
    BOOL ReturnValue = FALSE;
    DWORD ErrorCode;
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
        ErrorCode = _RpcOpenPrinter(pPrinterName, phPrinter, pDatatype, pDevModeContainer, AccessRequired);
        if (ErrorCode)
        {
            ERR("_RpcOpenPrinter failed with error %lu!\n", ErrorCode);
        }

        ReturnValue = (ErrorCode == ERROR_SUCCESS);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("_RpcOpenPrinter failed with exception code %lu!\n", RpcExceptionCode());
    }
    RpcEndExcept;

    return ReturnValue;
}

BOOL WINAPI
SpoolerInit()
{
    BOOL ReturnValue = FALSE;
    DWORD ErrorCode;

    // Nothing to initialize here yet, but pass this call to the Spool Service as well.
    RpcTryExcept
    {
        ErrorCode = _RpcSpoolerInit();
        if (ErrorCode)
        {
            ERR("_RpcSpoolerInit failed with error %lu!\n", ErrorCode);
        }

        ReturnValue = (ErrorCode == ERROR_SUCCESS);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("_RpcSpoolerInit failed with exception code %lu!\n", RpcExceptionCode());
    }
    RpcEndExcept;

    return ReturnValue;
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
