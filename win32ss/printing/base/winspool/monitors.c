/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Print Monitors
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/monitors.h>

BOOL WINAPI
AddMonitorA(PSTR pName, DWORD Level, PBYTE pMonitors)
{
    TRACE("AddMonitorA(%s, %lu, %p)\n", pName, Level, pMonitors);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddMonitorW(PWSTR pName, DWORD Level, PBYTE pMonitors)
{
    TRACE("AddMonitorW(%S, %lu, %p)\n", pName, Level, pMonitors);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeleteMonitorA(PSTR pName, PSTR pEnvironment, PSTR pMonitorName)
{
    TRACE("DeleteMonitorA(%s, %s, %s)\n", pName, pEnvironment, pMonitorName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeleteMonitorW(PWSTR pName, PWSTR pEnvironment, PWSTR pMonitorName)
{
    TRACE("DeleteMonitorW(%S, %S, %S)\n", pName, pEnvironment, pMonitorName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumMonitorsA(PSTR pName, DWORD Level, PBYTE pMonitors, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    TRACE("EnumMonitorsA(%s, %lu, %p, %lu, %p, %p)\n", pName, Level, pMonitors, cbBuf, pcbNeeded, pcReturned);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumMonitorsW(PWSTR pName, DWORD Level, PBYTE pMonitors, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;

    TRACE("EnumMonitorsW(%S, %lu, %p, %lu, %p, %p)\n", pName, Level, pMonitors, cbBuf, pcbNeeded, pcReturned);

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
        ASSERT(Level >= 1 && Level <= 2);
        MarshallUpStructuresArray(cbBuf, pMonitors, *pcReturned, pMonitorInfoMarshalling[Level]->pInfo, pMonitorInfoMarshalling[Level]->cbStructureSize, TRUE);
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
