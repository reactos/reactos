/*
 * PROJECT:     ReactOS Spooler Router
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Ports of the Print Monitors
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

BOOL WINAPI
EnumPortsW(PWSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    BOOL bReturnValue = TRUE;
    DWORD cbCallBuffer;
    DWORD cbNeeded;
    DWORD dwReturned;
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

        // Call the EnumPorts function of this Print Provider.
        cbNeeded = 0;
        dwReturned = 0;
        bReturnValue = pPrintProvider->PrintProvider.fpEnumPorts(pName, Level, pCallBuffer, cbCallBuffer, &cbNeeded, &dwReturned);

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

        // Check if we shall not ask other Print Providers.
        if (bReturnValue == ROUTER_STOP_ROUTING)
            break;
    }

    return bReturnValue;
}
