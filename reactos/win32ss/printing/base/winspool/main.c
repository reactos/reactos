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
    Status = RpcStringBindingComposeW(NULL, L"ncacn_np", wszStringBinding, L"\\pipe\\spoolss", NULL, &wszStringBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcStringBindingComposeW failed with status %u!\n", Status);
        return NULL;
    }

    // Get a handle_t binding handle from the string binding handle
    Status = RpcBindingFromStringBindingW(wszStringBinding, &hBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcBindingFromStringBindingW failed with status %u!\n", Status);
        return NULL;
    }

    // Free the string binding handle
    Status = RpcStringFreeW(&wszStringBinding);
    if (Status != RPC_S_OK)
    {
        ERR("RpcStringFreeW failed with status %u!\n", Status);
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
        ERR("RpcBindingFree failed with status %u!\n", Status);
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
    DEVMODEW wDevMode;
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
            ERR("HeapAlloc failed for pwszPrinterName with last error %u!\n", GetLastError());
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
                ERR("HeapAlloc failed for pwszDatatype with last error %u!\n", GetLastError());
                goto Cleanup;
            }

            MultiByteToWideChar(CP_ACP, 0, pDefault->pDatatype, -1, pwszDatatype, StringLength);
            wDefault.pDatatype = pwszDatatype;
        }

        if (pDefault->pDevMode)
        {
            // Fixed size strings, so no additional memory needs to be allocated
            MultiByteToWideChar(CP_ACP, 0, pDefault->pDevMode->dmDeviceName, -1, wDevMode.dmDeviceName, sizeof(wDevMode.dmDeviceName) / sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pDefault->pDevMode->dmFormName, -1, wDevMode.dmFormName, sizeof(wDevMode.dmFormName) / sizeof(WCHAR));

            // Use CopyMemory to copy over several structure values in one step
            CopyMemory(&wDevMode.dmSpecVersion, &pDefault->pDevMode->dmSpecVersion, (ULONG_PTR)&wDevMode.dmCollate - (ULONG_PTR)&wDevMode.dmSpecVersion);
            CopyMemory(&wDevMode.dmLogPixels, &pDefault->pDevMode->dmLogPixels, (ULONG_PTR)&wDevMode.dmPanningHeight - (ULONG_PTR)&wDevMode.dmLogPixels);

            wDefault.pDevMode = &wDevMode;
        }
    }

    ReturnValue = OpenPrinterW(pwszPrinterName, phPrinter, &wDefault);

Cleanup:
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
            ERR("_RpcOpenPrinter failed with error %u!\n", ErrorCode);
        }

        ReturnValue = (ErrorCode == ERROR_SUCCESS);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("_RpcOpenPrinter failed with exception code %u!\n", RpcExceptionCode());
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
