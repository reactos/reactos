/*
 * PROJECT:     ReactOS Local Spooler
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Ports of the Print Monitors
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

// Local Variables
static LIST_ENTRY _PortList;


PLOCAL_PORT
FindPort(PCWSTR pwszName)
{
    PLIST_ENTRY pEntry;
    PLOCAL_PORT pPort;

    TRACE("FindPort(%S)\n", pwszName);

    if (!pwszName)
        return NULL;

    for (pEntry = _PortList.Flink; pEntry != &_PortList; pEntry = pEntry->Flink)
    {
        pPort = CONTAINING_RECORD(pEntry, LOCAL_PORT, Entry);

        if (_wcsicmp(pPort->pwszName, pwszName) == 0)
            return pPort;
    }

    return NULL;
}

BOOL
InitializePortList(void)
{
    BOOL bReturnValue;
    DWORD cbNeeded;
    DWORD cbPortName;
    DWORD dwErrorCode;
    DWORD dwReturned;
    DWORD i;
    PLOCAL_PORT pPort;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    PLIST_ENTRY pEntry;
    PPORT_INFO_1W p;
    PPORT_INFO_1W pPortInfo1 = NULL;

    TRACE("InitializePortList()\n");

    // Initialize an empty list for our Ports.
    InitializeListHead(&_PortList);

    // Loop through all Print Monitors.
    for (pEntry = PrintMonitorList.Flink; pEntry != &PrintMonitorList; pEntry = pEntry->Flink)
    {
        // Cleanup from the previous run.
        if (pPortInfo1)
        {
            DllFreeSplMem(pPortInfo1);
            pPortInfo1 = NULL;
        }

        pPrintMonitor = CONTAINING_RECORD(pEntry, LOCAL_PRINT_MONITOR, Entry);

        // Determine the required buffer size for EnumPorts.
        if (pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnEnumPorts(pPrintMonitor->hMonitor, NULL, 1, NULL, 0, &cbNeeded, &dwReturned);
        else
            bReturnValue = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnEnumPorts(NULL, 1, NULL, 0, &cbNeeded, &dwReturned);

        // Check the returned error code.
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            ERR("Print Monitor \"%S\" failed with error %lu on EnumPorts!\n", pPrintMonitor->pwszName, GetLastError());
            continue;
        }

        // Allocate a buffer large enough.
        pPortInfo1 = DllAllocSplMem(cbNeeded);
        if (!pPortInfo1)
        {
            dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
            ERR("DllAllocSplMem failed!\n");
            goto Cleanup;
        }

        // Get the ports handled by this monitor.
        if (pPrintMonitor->bIsLevel2)
            bReturnValue = ((PMONITOR2)pPrintMonitor->pMonitor)->pfnEnumPorts(pPrintMonitor->hMonitor, NULL, 1, (PBYTE)pPortInfo1, cbNeeded, &cbNeeded, &dwReturned);
        else
            bReturnValue = ((LPMONITOREX)pPrintMonitor->pMonitor)->Monitor.pfnEnumPorts(NULL, 1, (PBYTE)pPortInfo1, cbNeeded, &cbNeeded, &dwReturned);

        // Check the return value.
        if (!bReturnValue)
        {
            ERR("Print Monitor \"%S\" failed with error %lu on EnumPorts!\n", pPrintMonitor->pwszName, GetLastError());
            continue;
        }

        // Loop through all returned ports.
        p = pPortInfo1;

        for (i = 0; i < dwReturned; i++)
        {
            cbPortName = (wcslen(p->pName) + 1) * sizeof(WCHAR);

            // Create a new LOCAL_PORT structure for it.
            pPort = DllAllocSplMem(sizeof(LOCAL_PORT) + cbPortName);
            if (!pPort)
            {
                dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
                ERR("DllAllocSplMem failed!\n");
                goto Cleanup;
            }

            pPort->pPrintMonitor = pPrintMonitor;
            pPort->pwszName = (PWSTR)((PBYTE)pPort + sizeof(LOCAL_PORT));
            CopyMemory(pPort->pwszName, p->pName, cbPortName);

            // Insert it into the list and advance to the next port.
            InsertTailList(&_PortList, &pPort->Entry);
            p++;
        }
    }

    dwErrorCode = ERROR_SUCCESS;

Cleanup:
    // Inside the loop
    if (pPortInfo1)
        DllFreeSplMem(pPortInfo1);

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
LocalEnumPorts(PWSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    BOOL bReturnValue = TRUE;
    DWORD cbCallBuffer;
    DWORD cbNeeded;
    DWORD dwReturned;
    PBYTE pCallBuffer;
    PLOCAL_PRINT_MONITOR pPrintMonitor;
    PLIST_ENTRY pEntry;

    TRACE("LocalEnumPorts(%S, %lu, %p, %lu, %p, %p)\n", pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned);

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
        cbNeeded = 0;
        dwReturned = 0;

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
