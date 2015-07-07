/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Ports of the Print Monitors
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"


BOOL WINAPI
LocalEnumPorts(PWSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    BOOL bReturnValue;
    DWORD cbCallBuffer;
    DWORD cbNeeded;
    DWORD dwReturned;
    PBYTE pCallBuffer;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    PLIST_ENTRY pEntry;

    // Sanity checks.
    if ((cbBuf && !pPorts) || !pcbNeeded || !pcReturned)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // Begin counting.
    *pcbNeeded = 0;
    *pcReturned = 0;

    // At the beginning, we have the full buffer available.
    cbCallBuffer = cbBuf;
    pCallBuffer = pPorts;

    // Loop through all Print Monitors.
    for (pEntry = PrintMonitorList.Flink; pEntry != &PrintMonitorList; pEntry = pEntry->Flink)
    {
        pPrintMonitor = CONTAINING_RECORD(pEntry, LOCAL_PRINT_MONITOR, Entry);

        // Call the EnumPorts function of this Print Monitor.
        if (pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnEnumPorts(pPrintMonitor->hMonitor, pName, Level, pCallBuffer, cbCallBuffer, &cbNeeded, &dwReturned);
        else
            bReturnValue = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnEnumPorts(pName, Level, pCallBuffer, cbCallBuffer, &cbNeeded, &dwReturned);

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
    }

    return bReturnValue;
}
