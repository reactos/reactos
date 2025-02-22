/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Printers and printing
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

BOOL WINAPI
AbortPrinter(HANDLE hPrinter)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpAbortPrinter(pHandle->hPrinter);
}

//
// See [MS-RPRN] 2.2.1.11 SPLCLIENT_INFO, SPLCLIENT_INFO Level.
//
HANDLE WINAPI
AddPrinterExW( PWSTR pName, DWORD Level, PBYTE pPrinter, PBYTE pClientInfo, DWORD ClientInfoLevel)
{
    BOOL bReturnValue;
    DWORD dwErrorCode = ERROR_INVALID_PRINTER_NAME;
    HANDLE hPrinter = NULL;
    PWSTR pPrinterName = NULL;
    PLIST_ENTRY pEntry;
    PSPOOLSS_PRINTER_HANDLE pHandle;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;

    if ( Level != 2 )
    {
        FIXME( "Unsupported level %d\n", Level );
        SetLastError( ERROR_INVALID_LEVEL );
        return hPrinter;
    }
    else
    {
        PPRINTER_INFO_2W pi2w = (PPRINTER_INFO_2W)pPrinter;
        pPrinterName = pi2w->pPrinterName;
    }

    // Loop through all Print Providers to find one able to open this Printer.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        hPrinter = pPrintProvider->PrintProvider.fpAddPrinterEx(pName, Level, pPrinter, pClientInfo, ClientInfoLevel);

        bReturnValue = GetLastError();

        // Fallback.... ?

        if ( hPrinter == NULL && bReturnValue == ERROR_NOT_SUPPORTED )
        {
            hPrinter = pPrintProvider->PrintProvider.fpAddPrinter(pName, Level, pPrinter);
        }

        bReturnValue = GetLastError();

        if ( bReturnValue == ROUTER_SUCCESS && hPrinter )
        {
            // This Print Provider has opened this Printer.
            // Store this information and return a handle.
            pHandle = DllAllocSplMem(sizeof(SPOOLSS_PRINTER_HANDLE));
            if (!pHandle)
            {
                dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                ERR("DllAllocSplMem failed!\n");
                goto Cleanup;
            }

            pHandle->pPrintProvider = pPrintProvider;
            pHandle->hPrinter = hPrinter;

            dwErrorCode = ERROR_SUCCESS;
            goto Cleanup;
        }
        else if (bReturnValue == ROUTER_STOP_ROUTING)
        {
            ERR("A Print Provider returned ROUTER_STOP_ROUTING for Printer \"%S\"!\n", pPrinterName);
            dwErrorCode = GetLastError();
            goto Cleanup;
        }
    }

Cleanup:
    // ERROR_INVALID_NAME by the Print Provider is translated to ERROR_INVALID_PRINTER_NAME here, but not in other APIs as far as I know.
    if (dwErrorCode == ERROR_INVALID_NAME)
        dwErrorCode = ERROR_INVALID_PRINTER_NAME;

    SetLastError(dwErrorCode);
    return hPrinter;
}

HANDLE WINAPI
AddPrinterW(PWSTR pName, DWORD Level, PBYTE pPrinter)
{
    BOOL bReturnValue;
    DWORD dwErrorCode = ERROR_INVALID_PRINTER_NAME;
    HANDLE hPrinter = NULL;
    PWSTR pPrinterName = NULL;
    PLIST_ENTRY pEntry;
    PSPOOLSS_PRINTER_HANDLE pHandle;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;

    FIXME("AddPrinterW(%S, %lu, %p)\n", pName, Level, pPrinter);

    if ( Level != 2 )
    {
        FIXME( "Unsupported level %d\n", Level );
        SetLastError( ERROR_INVALID_LEVEL );
        return hPrinter;
    }
    else
    {
        PPRINTER_INFO_2W pi2w = (PPRINTER_INFO_2W)pPrinter;
        pPrinterName = pi2w->pPrinterName;
    }

    // Xp return AddPrinterExW( pName, Level, pPrinter, NULL, 0); but,,,, W7u just Forward Direct.

    // Loop through all Print Providers to find one able to open this Printer.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        hPrinter = pPrintProvider->PrintProvider.fpAddPrinter(pName, Level, pPrinter);

        bReturnValue = GetLastError();

        if ( bReturnValue == ROUTER_SUCCESS && hPrinter )
        {
            // This Print Provider has opened this Printer.
            // Store this information and return a handle.
            pHandle = DllAllocSplMem(sizeof(SPOOLSS_PRINTER_HANDLE));
            if (!pHandle)
            {
                dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                ERR("DllAllocSplMem failed!\n");
                goto Cleanup;
            }

            pHandle->pPrintProvider = pPrintProvider;
            pHandle->hPrinter = hPrinter;

            dwErrorCode = ERROR_SUCCESS;
            goto Cleanup;
        }
        else if (bReturnValue == ROUTER_STOP_ROUTING)
        {
            ERR("A Print Provider returned ROUTER_STOP_ROUTING for Printer \"%S\"!\n", pPrinterName);
            dwErrorCode = GetLastError();
            goto Cleanup;
        }
    }

Cleanup:
    // ERROR_INVALID_NAME by the Print Provider is translated to ERROR_INVALID_PRINTER_NAME here, but not in other APIs as far as I know.
    if (dwErrorCode == ERROR_INVALID_NAME)
        dwErrorCode = ERROR_INVALID_PRINTER_NAME;

    SetLastError(dwErrorCode);
    return hPrinter;
}

BOOL WINAPI
ClosePrinter(HANDLE hPrinter)
{
    BOOL bReturnValue;
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    FIXME("ClosePrinter %p\n",hPrinter);

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // FIXME: Call FindClosePrinterChangeNotification for all created change notifications (according to MSDN).

    // Call CloseHandle of the Print Provider.
    bReturnValue = pHandle->pPrintProvider->PrintProvider.fpClosePrinter(pHandle->hPrinter);
    FIXME("ClosePrinter 2\n");
    // Free our handle information.
    DllFreeSplMem(pHandle);
    FIXME("ClosePrinter 3\n");
    return bReturnValue;
}

BOOL WINAPI
DeletePrinter(HANDLE hPrinter)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpDeletePrinter(pHandle->hPrinter);
}

BOOL WINAPI
EndDocPrinter(HANDLE hPrinter)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpEndDocPrinter(pHandle->hPrinter);
}

BOOL WINAPI
EndPagePrinter(HANDLE hPrinter)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpEndPagePrinter(pHandle->hPrinter);
}

BOOL WINAPI
EnumPrintersW(DWORD Flags, PWSTR Name, DWORD Level, PBYTE pPrinterEnum, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
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

    if (cbBuf && !pPrinterEnum)
    {
        dwErrorCode = ERROR_INVALID_USER_BUFFER;
        goto Cleanup;
    }

    // At the beginning, we have the full buffer available.
    cbCallBuffer = cbBuf;
    pCallBuffer = pPrinterEnum;

    // Loop through all Print Providers.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        // Call the EnumPrinters function of this Print Provider.
        cbNeeded = 0;
        dwReturned = 0;
        Ret = pPrintProvider->PrintProvider.fpEnumPrinters(Flags, Name, Level, pCallBuffer, cbCallBuffer, &cbNeeded, &dwReturned);

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
GetPrinterW(HANDLE hPrinter, DWORD Level, PBYTE pPrinter, DWORD cbBuf, PDWORD pcbNeeded)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpGetPrinter(pHandle->hPrinter, Level, pPrinter, cbBuf, pcbNeeded);
}

//
// Forward Dead API to Local/Remote....
//
DWORD WINAPI
PrinterMessageBoxW(HANDLE hPrinter, DWORD Error, HWND hWnd, LPWSTR pText, LPWSTR pCaption, DWORD dwType)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpPrinterMessageBox(pHandle->hPrinter, Error, hWnd, pText, pCaption, dwType);
}

BOOL WINAPI
OpenPrinterW(PWSTR pPrinterName, PHANDLE phPrinter, PPRINTER_DEFAULTSW pDefault)
{
    BOOL bReturnValue;
    DWORD dwErrorCode = ERROR_INVALID_PRINTER_NAME;
    HANDLE hPrinter;
    PLIST_ENTRY pEntry;
    PSPOOLSS_PRINTER_HANDLE pHandle;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;

    // Loop through all Print Providers to find one able to open this Printer.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        bReturnValue = pPrintProvider->PrintProvider.fpOpenPrinter(pPrinterName, &hPrinter, pDefault);
        if (bReturnValue == ROUTER_SUCCESS)
        {
            // This Print Provider has opened this Printer.
            // Store this information and return a handle.
            pHandle = DllAllocSplMem(sizeof(SPOOLSS_PRINTER_HANDLE));
            if (!pHandle)
            {
                dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                ERR("DllAllocSplMem failed!\n");
                goto Cleanup;
            }

            pHandle->pPrintProvider = pPrintProvider;
            pHandle->hPrinter = hPrinter;
            *phPrinter = (HANDLE)pHandle;

            dwErrorCode = ERROR_SUCCESS;
            goto Cleanup;
        }
        else if (bReturnValue == ROUTER_STOP_ROUTING)
        {
            ERR("A Print Provider returned ROUTER_STOP_ROUTING for Printer \"%S\"!\n", pPrinterName);
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
ReadPrinter(HANDLE hPrinter, PVOID pBuf, DWORD cbBuf, PDWORD pNoBytesRead)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpReadPrinter(pHandle->hPrinter, pBuf, cbBuf, pNoBytesRead);
}

BOOL WINAPI
SeekPrinter( HANDLE hPrinter, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER pliNewPointer, DWORD dwMoveMethod, BOOL bWrite )
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpSeekPrinter( pHandle->hPrinter, liDistanceToMove, pliNewPointer, dwMoveMethod, bWrite );
}

BOOL WINAPI
SetPrinterW(HANDLE hPrinter, DWORD Level, PBYTE pPrinter, DWORD Command)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpSetPrinter( pHandle->hPrinter, Level, pPrinter, Command );
}

DWORD WINAPI
StartDocPrinterW(HANDLE hPrinter, DWORD Level, PBYTE pDocInfo)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpStartDocPrinter(pHandle->hPrinter, Level, pDocInfo);
}

BOOL WINAPI
StartPagePrinter(HANDLE hPrinter)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpStartPagePrinter(pHandle->hPrinter);
}

BOOL WINAPI
WritePrinter(HANDLE hPrinter, PVOID pBuf, DWORD cbBuf, PDWORD pcWritten)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hPrinter;

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpWritePrinter(pHandle->hPrinter, pBuf, cbBuf, pcWritten);
}

BOOL WINAPI
XcvDataW(HANDLE hXcv, PCWSTR pszDataName, PBYTE pInputData, DWORD cbInputData, PBYTE pOutputData, DWORD cbOutputData, PDWORD pcbOutputNeeded, PDWORD pdwStatus)
{
    PSPOOLSS_PRINTER_HANDLE pHandle = (PSPOOLSS_PRINTER_HANDLE)hXcv;

    FIXME("XcvDataW( %p, %S,,,)\n",hXcv, pszDataName);

    // Sanity checks.
    if (!pHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return pHandle->pPrintProvider->PrintProvider.fpXcvData(pHandle->hPrinter, pszDataName, pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded, pdwStatus);
}
