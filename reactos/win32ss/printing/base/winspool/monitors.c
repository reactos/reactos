/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Print Monitors
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallUpMonitorInfo(PBYTE pMonitorInfo, DWORD Level)
{
    PMONITOR_INFO_2W pMonitorInfo2 = (PMONITOR_INFO_2W)pMonitorInfo;        // MONITOR_INFO_1W is a subset of MONITOR_INFO_2W

    // Replace relative offset addresses in the output by absolute pointers.
    pMonitorInfo2->pName = (PWSTR)((ULONG_PTR)pMonitorInfo2->pName + (ULONG_PTR)pMonitorInfo2);

    if (Level == 2)
    {
        pMonitorInfo2->pDLLName = (PWSTR)((ULONG_PTR)pMonitorInfo2->pDLLName + (ULONG_PTR)pMonitorInfo2);
        pMonitorInfo2->pEnvironment = (PWSTR)((ULONG_PTR)pMonitorInfo2->pEnvironment + (ULONG_PTR)pMonitorInfo2);
    }
}

BOOL WINAPI
AddMonitorW(PWSTR pName, DWORD Level, PBYTE pMonitors)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeleteMonitorW(PWSTR pName, PWSTR pEnvironment, PWSTR pMonitorName)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumMonitorsW(PWSTR pName, DWORD Level, PBYTE pMonitors, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    DWORD i;
    PBYTE p = pMonitors;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumMonitors(pName, Level, pMonitors, cbBuf, pcbNeeded, pcReturned);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcEnumPorts failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallUpMonitorInfo(p, Level);

            if (Level == 1)
                p += sizeof(MONITOR_INFO_1W);
            else if (Level == 2)
                p += sizeof(MONITOR_INFO_2W);
        }
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
