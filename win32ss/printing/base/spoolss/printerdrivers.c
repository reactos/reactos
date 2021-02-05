/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printer Configuration Data
 * COPYRIGHT:   Copyright 2020 ReactOS
 */

#include "precomp.h"

BOOL WINAPI
AddPrinterDriverExW(PWSTR pName, DWORD Level, PBYTE pDriverInfo, DWORD dwFileCopyFlags)
{
    BOOL bReturnValue;
    DWORD dwErrorCode = ERROR_INVALID_PRINTER_NAME;
    PLIST_ENTRY pEntry;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;

    // Loop through all Print Providers.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        bReturnValue = pPrintProvider->PrintProvider.fpAddPrinterDriverEx(pName, Level, pDriverInfo, dwFileCopyFlags);

        if (bReturnValue == ROUTER_SUCCESS)
        {
            dwErrorCode = ERROR_SUCCESS;
            goto Cleanup;
        }
        else if (bReturnValue == ROUTER_STOP_ROUTING)
        {
            ERR("A Print Provider returned ROUTER_STOP_ROUTING for Printer \"%S\"!\n", pName);
            dwErrorCode = GetLastError();
            goto Cleanup;
        }
    }

Cleanup:
    // ERROR_INVALID_NAME by the Print Provider is translated to ERROR_INVALID_PRINTER_NAME here, but not in other APIs as far as I know.
    if (dwErrorCode == ERROR_INVALID_NAME)
        dwErrorCode = ERROR_INVALID_PRINTER_NAME;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
AddPrinterDriverW(PWSTR pName, DWORD Level, PBYTE pDriverInfo)
{
    TRACE("AddPrinterDriverW(%S, %lu, %p)\n", pName, Level, pDriverInfo);
    return AddPrinterDriverExW(pName, Level, pDriverInfo, APD_COPY_NEW_FILES);
}

BOOL WINAPI
DeletePrinterDriverExW(PWSTR pName, PWSTR pEnvironment, PWSTR pDriverName, DWORD dwDeleteFlag, DWORD dwVersionFlag)
{
    BOOL bReturnValue;
    DWORD dwErrorCode = ERROR_INVALID_PRINTER_NAME;
    PLIST_ENTRY pEntry;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;

    // Loop through all Print Providers.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        bReturnValue = pPrintProvider->PrintProvider.fpDeletePrinterDriverEx(pName, pEnvironment, pDriverName, dwDeleteFlag, dwVersionFlag);

        if (bReturnValue == ROUTER_SUCCESS)
        {
            dwErrorCode = ERROR_SUCCESS;
            goto Cleanup;
        }
        else if (bReturnValue == ROUTER_STOP_ROUTING)
        {
            ERR("A Print Provider returned ROUTER_STOP_ROUTING for Printer \"%S\"!\n", pName);
            dwErrorCode = GetLastError();
            goto Cleanup;
        }
    }

Cleanup:
    // ERROR_INVALID_NAME by the Print Provider is translated to ERROR_INVALID_PRINTER_NAME here, but not in other APIs as far as I know.
    if (dwErrorCode == ERROR_INVALID_NAME)
        dwErrorCode = ERROR_INVALID_PRINTER_NAME;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
DeletePrinterDriverW(PWSTR pName, PWSTR pEnvironment, PWSTR pDriverName)
{
    TRACE("DeletePrinterDriverW(%S, %S, %S)\n", pName, pEnvironment, pDriverName);
    return DeletePrinterDriverExW(pName, pEnvironment, pDriverName, 0, 0);
}

BOOL WINAPI
EnumPrinterDriversW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pDriverInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD cbCallBuffer;
    DWORD cbNeeded;
    DWORD dwErrorCode = MAXDWORD;
    DWORD dwReturned;
    PBYTE pCallBuffer;
    BOOL Ret = FALSE;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;
    PLIST_ENTRY pEntry;

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    if ( cbBuf && !pDriverInfo )
    {
        dwErrorCode = ERROR_INVALID_USER_BUFFER;
        goto Cleanup;
    }

    // At the beginning, we have the full buffer available.
    cbCallBuffer = cbBuf;
    pCallBuffer = pDriverInfo;

    // Loop through all Print Providers.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        // Call the EnumPrinters function of this Print Provider.
        cbNeeded = 0;
        dwReturned = 0;

        Ret = pPrintProvider->PrintProvider.fpEnumPrinterDrivers( pName, pEnvironment, Level, pCallBuffer, cbCallBuffer, &cbNeeded, &dwReturned);

        if ( !Ret )
        {
            dwErrorCode = GetLastError();
        }

        // Add the returned counts to the total values.
        *pcbNeeded += cbNeeded;
        *pcReturned += dwReturned;

        // Reduce the available buffer size for the next call without risking an underflow.
        if (cbNeeded < cbCallBuffer)
            cbCallBuffer -= cbNeeded;
        else
            cbCallBuffer = 0;

        // Advance the buffer if the caller provided it.
        if (pCallBuffer)
            pCallBuffer += cbNeeded;

        // dwErrorCode shall not be overwritten if a previous EnumPrinters call already succeeded.
        if (dwErrorCode != ERROR_SUCCESS)
            dwErrorCode = GetLastError();
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetPrinterDriverW(HANDLE hPrinter, PWSTR pEnvironment, DWORD Level, PBYTE pDriverInfo, DWORD cbBuf, PDWORD pcbNeeded)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpGetPrinterDriver(pHandle->hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded);
}


BOOL WINAPI
GetPrinterDriverExW(
    HANDLE hPrinter,
    LPWSTR pEnvironment,
    DWORD Level,
    LPBYTE pDriverInfo,
    DWORD cbBuf,
    LPDWORD pcbNeeded,
    DWORD dwClientMajorVersion,
    DWORD dwClientMinorVersion,
    PDWORD pdwServerMajorVersion,
    PDWORD pdwServerMinorVersion )
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    FIXME("GetPrinterDriverExW(%p, %lu, %lu, %p, %lu, %p, %lu, %lu, %p, %p)\n", hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded, dwClientMajorVersion, dwClientMinorVersion, pdwServerMajorVersion, pdwServerMinorVersion);

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if ( cbBuf && !pDriverInfo )
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpGetPrinterDriverEx(pHandle->hPrinter, pEnvironment, Level, pDriverInfo, cbBuf, pcbNeeded, dwClientMajorVersion, dwClientMinorVersion, pdwServerMajorVersion, pdwServerMinorVersion);
}

BOOL WINAPI
GetPrinterDriverDirectoryW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pDriverDirectory, DWORD cbBuf, PDWORD pcbNeeded)
{
    BOOL bReturnValue;
    DWORD dwErrorCode = ERROR_INVALID_PRINTER_NAME;
    PLIST_ENTRY pEntry;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;

    if ( cbBuf && !pDriverDirectory )
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    // Loop through all Print Providers.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        bReturnValue = pPrintProvider->PrintProvider.fpGetPrinterDriverDirectory(pName, pEnvironment, Level, pDriverDirectory, cbBuf, pcbNeeded);

        if (bReturnValue == ROUTER_SUCCESS)
        {
            dwErrorCode = ERROR_SUCCESS;
            goto Cleanup;
        }
        else if (bReturnValue == ROUTER_STOP_ROUTING)
        {
            ERR("A Print Provider returned ROUTER_STOP_ROUTING for Printer \"%S\"!\n", pName);
            dwErrorCode = GetLastError();
            goto Cleanup;
        }
    }

Cleanup:
    // ERROR_INVALID_NAME by the Print Provider is translated to ERROR_INVALID_PRINTER_NAME here, but not in other APIs as far as I know.
    if (dwErrorCode == ERROR_INVALID_NAME)
        dwErrorCode = ERROR_INVALID_PRINTER_NAME;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
