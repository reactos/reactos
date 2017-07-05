/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Print Monitors
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallDownMonitorInfo(PBYTE* ppMonitorInfo, DWORD Level)
{
    PMONITOR_INFO_2W pMonitorInfo2 = (PMONITOR_INFO_2W)(*ppMonitorInfo);        // MONITOR_INFO_1W is a subset of MONITOR_INFO_2W

    // Replace absolute pointer addresses in the output by relative offsets.
    pMonitorInfo2->pName = (PWSTR)((ULONG_PTR)pMonitorInfo2->pName - (ULONG_PTR)pMonitorInfo2);

    if (Level == 1)
    {
        *ppMonitorInfo += sizeof(MONITOR_INFO_1W);
    }
    else
    {
        pMonitorInfo2->pDLLName = (PWSTR)((ULONG_PTR)pMonitorInfo2->pDLLName - (ULONG_PTR)pMonitorInfo2);
        pMonitorInfo2->pEnvironment = (PWSTR)((ULONG_PTR)pMonitorInfo2->pEnvironment - (ULONG_PTR)pMonitorInfo2);
        *ppMonitorInfo += sizeof(MONITOR_INFO_2W);
    }
}

DWORD
_RpcAddMonitor(WINSPOOL_HANDLE pName, WINSPOOL_MONITOR_CONTAINER* pMonitorContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeleteMonitor(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, WCHAR* pMonitorName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumMonitors(WINSPOOL_HANDLE pName, DWORD Level, BYTE* pMonitor, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;
    PBYTE pMonitorAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pMonitorAligned = AlignRpcPtr(pMonitor, &cbBuf);

    if(EnumMonitorsW(pName, Level, pMonitorAligned, cbBuf, pcbNeeded, pcReturned))
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        DWORD i;
        PBYTE p = pMonitorAligned;

        for (i = 0; i < *pcReturned; i++)
            _MarshallDownMonitorInfo(&p, Level);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pMonitor, pMonitorAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}
