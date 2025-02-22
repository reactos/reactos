/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Ports of the Print Monitors
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

BOOL WINAPI
AddPortExW(PWSTR pName, DWORD Level, PBYTE lpBuffer, PWSTR lpMonitorName)
{
    BOOL bReturnValue = TRUE;
    DWORD dwErrorCode = MAXDWORD;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;
    PLIST_ENTRY pEntry;

    FIXME("AddPortEx(%S, %lu, %p, %s)\n", pName, Level, lpBuffer, debugstr_w(lpMonitorName));

    // Loop through all Print Provider.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        // Check if this Print Provider provides the function.
        if (!pPrintProvider->PrintProvider.fpAddPortEx)
            continue;

        bReturnValue = pPrintProvider->PrintProvider.fpAddPortEx(pName, Level, lpBuffer, lpMonitorName);

        if ( !bReturnValue )
        {
            dwErrorCode = GetLastError();
        }

        // dwErrorCode shall not be overwritten if a previous call already succeeded.
        if (dwErrorCode != ERROR_SUCCESS)
            dwErrorCode = GetLastError();
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
AddPortW(PWSTR pName, HWND hWnd, PWSTR pMonitorName)
{
    BOOL bReturnValue = TRUE;
    DWORD dwErrorCode = MAXDWORD;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;
    PLIST_ENTRY pEntry;

    FIXME("AddPort(%S, %p, %s)\n", pName, hWnd, debugstr_w(pMonitorName));

    // Loop through all Print Provider.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        // Check if this Print Provider provides the function.
        if (!pPrintProvider->PrintProvider.fpAddPort)
            continue;

        bReturnValue = pPrintProvider->PrintProvider.fpAddPort(pName, hWnd, pMonitorName);

        if ( !bReturnValue )
        {
            dwErrorCode = GetLastError();
        }

        // dwErrorCode shall not be overwritten if a previous call already succeeded.
        if (dwErrorCode != ERROR_SUCCESS)
            dwErrorCode = GetLastError();
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
ConfigurePortW(PWSTR pName, HWND hWnd, PWSTR pPortName)
{
    BOOL bReturnValue = TRUE;
    DWORD dwErrorCode = MAXDWORD;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;
    PLIST_ENTRY pEntry;

    // Loop through all Print Provider.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        // Check if this Print Provider provides the function.
        if (!pPrintProvider->PrintProvider.fpConfigurePort)
            continue;

        bReturnValue = pPrintProvider->PrintProvider.fpConfigurePort(pName, hWnd, pPortName);

        if ( !bReturnValue )
        {
            dwErrorCode = GetLastError();
        }

        // dwErrorCode shall not be overwritten if a previous call already succeeded.
        if (dwErrorCode != ERROR_SUCCESS)
            dwErrorCode = GetLastError();
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
DeletePortW(PWSTR pName, HWND hWnd, PWSTR pPortName)
{
    BOOL bReturnValue = TRUE;
    DWORD dwErrorCode = MAXDWORD;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;
    PLIST_ENTRY pEntry;

    // Loop through all Print Provider.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        // Check if this Print Provider provides the function.
        if (!pPrintProvider->PrintProvider.fpDeletePort)
            continue;

        bReturnValue = pPrintProvider->PrintProvider.fpDeletePort(pName, hWnd, pPortName);

        if ( !bReturnValue )
        {
            dwErrorCode = GetLastError();
        }

        // dwErrorCode shall not be overwritten if a previous call already succeeded.
        if (dwErrorCode != ERROR_SUCCESS)
            dwErrorCode = GetLastError();
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
EnumPortsW(PWSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    BOOL bReturnValue = TRUE;
    DWORD cbCallBuffer;
    DWORD cbNeeded;
    DWORD dwReturned;
    DWORD dwErrorCode = MAXDWORD;
    PBYTE pCallBuffer;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;
    PLIST_ENTRY pEntry;

    // Sanity checks.
    if (cbBuf && !pPorts)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    // At the beginning, we have the full buffer available.
    cbCallBuffer = cbBuf;
    pCallBuffer = pPorts;

    // Loop through all Print Provider.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        // Check if this Print Provider provides an EnumPorts function.
        if (!pPrintProvider->PrintProvider.fpEnumPorts)
            continue;

        // Call the EnumPorts function of this Print Provider.
        cbNeeded = 0;
        dwReturned = 0;
        bReturnValue = pPrintProvider->PrintProvider.fpEnumPorts(pName, Level, pCallBuffer, cbCallBuffer, &cbNeeded, &dwReturned);

        if ( !bReturnValue )
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

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
SetPortW(PWSTR pName, PWSTR pPortName, DWORD dwLevel, PBYTE pPortInfo)
{
    BOOL bReturnValue = TRUE;
    DWORD dwErrorCode = MAXDWORD;
    PSPOOLSS_PRINT_PROVIDER pPrintProvider;
    PLIST_ENTRY pEntry;

    // Loop through all Print Provider.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        // Check if this Print Provider provides the function.
        if (!pPrintProvider->PrintProvider.fpSetPort)
            continue;

        bReturnValue = pPrintProvider->PrintProvider.fpSetPort(pName, pPortName, dwLevel, pPortInfo);

        if ( !bReturnValue )
        {
            dwErrorCode = GetLastError();
        }

        // dwErrorCode shall not be overwritten if a previous call already succeeded.
        if (dwErrorCode != ERROR_SUCCESS)
            dwErrorCode = GetLastError();
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
