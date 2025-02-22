/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Print Monitors
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/monitors.h>

DWORD
_RpcAddMonitor(WINSPOOL_HANDLE pName, WINSPOOL_MONITOR_CONTAINER* pMonitorContainer)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!AddMonitorW(pName, pMonitorContainer->Level, (PBYTE)pMonitorContainer->MonitorInfo.pMonitorInfo2))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcDeleteMonitor(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, WCHAR* pMonitorName)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!DeleteMonitorW( pName, pEnvironment, pMonitorName ))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
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
        ASSERT(Level >= 1 && Level <= 2);
        MarshallDownStructuresArray(pMonitorAligned, *pcReturned, pMonitorInfoMarshalling[Level]->pInfo, pMonitorInfoMarshalling[Level]->cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pMonitor, pMonitorAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}
