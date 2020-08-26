/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Print Monitors
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

BOOL WINAPI
AddMonitorW(PWSTR pName, DWORD Level, PBYTE pMonitors)
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
        if (!pPrintProvider->PrintProvider.fpAddMonitor)
            continue;

        bReturnValue = pPrintProvider->PrintProvider.fpAddMonitor(pName, Level, pMonitors);

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
DeleteMonitorW(PWSTR pName, PWSTR pEnvironment, PWSTR pMonitorName)
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
        if (!pPrintProvider->PrintProvider.fpDeleteMonitor)
            continue;

        bReturnValue = pPrintProvider->PrintProvider.fpDeleteMonitor(pName, pEnvironment, pMonitorName);

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
EnumMonitorsW(PWSTR pName, DWORD Level, PBYTE pMonitors, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
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
    if (cbBuf && !pMonitors)
    {
        SetLastError(ERROR_INVALID_USER_BUFFER);
        return FALSE;
    }

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    // At the beginning, we have the full buffer available.
    cbCallBuffer = cbBuf;
    pCallBuffer = pMonitors;

    // Loop through all Print Provider.
    for (pEntry = PrintProviderList.Flink; pEntry != &PrintProviderList; pEntry = pEntry->Flink)
    {
        pPrintProvider = CONTAINING_RECORD(pEntry, SPOOLSS_PRINT_PROVIDER, Entry);

        // Check if this Print Provider provides an EnumMonitors function.
        if (!pPrintProvider->PrintProvider.fpEnumMonitors)
            continue;

        // Call the EnumMonitors function of this Print Provider.
        cbNeeded = 0;
        dwReturned = 0;
        bReturnValue = pPrintProvider->PrintProvider.fpEnumMonitors(pName, Level, pCallBuffer, cbCallBuffer, &cbNeeded, &dwReturned);

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
