/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Ports
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/ports.h>

BOOL WINAPI
AddPortA(PSTR pName, HWND hWnd, PSTR pMonitorName)
{
    TRACE("AddPortA(%s, %p, %s)\n", pName, hWnd, pMonitorName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddPortExA(PSTR pName, DWORD Level, PBYTE lpBuffer, PSTR lpMonitorName)
{
    TRACE("AddPortExA(%s, %lu, %p, %s)\n", pName, Level, lpBuffer, lpMonitorName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddPortExW(PWSTR pName, DWORD Level, PBYTE lpBuffer, PWSTR lpMonitorName)
{
    TRACE("AddPortExA(%S, %lu, %p, %S)\n", pName, Level, lpBuffer, lpMonitorName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddPortW(PWSTR pName, HWND hWnd, PWSTR pMonitorName)
{
    TRACE("AddPortW(%S, %p, %S)\n", pName, hWnd, pMonitorName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
ConfigurePortA(PSTR pName, HWND hWnd, PSTR pPortName)
{
    TRACE("ConfigurePortA(%s, %p, %s)\n", pName, hWnd, pPortName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
ConfigurePortW(PWSTR pName, HWND hWnd, PWSTR pPortName)
{
    TRACE("ConfigurePortW(%S, %p, %S)\n", pName, hWnd, pPortName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePortA(PSTR pName, HWND hWnd, PSTR pPortName)
{
    TRACE("DeletePortA(%s, %p, %s)\n", pName, hWnd, pPortName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePortW(PWSTR pName, HWND hWnd, PWSTR pPortName)
{
    TRACE("DeletePortW(%S, %p, %S)\n", pName, hWnd, pPortName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPortsA(PSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    TRACE("EnumPortsA(%s, %lu, %p, %lu, %p, %p)\n", pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPortsW(PWSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;

    TRACE("EnumPortsW(%S, %lu, %p, %lu, %p, %p)\n", pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned);

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumPorts(pName, Level, pPorts, cbBuf, pcbNeeded, pcReturned);
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
        MarshallUpStructuresArray(cbBuf, pPorts, *pcReturned, pPortInfoMarshalling[Level]->pInfo, pPortInfoMarshalling[Level]->cbStructureSize, TRUE);
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
SetPortA(PSTR pName, PSTR pPortName, DWORD dwLevel, PBYTE pPortInfo)
{
    TRACE("SetPortA(%s, %s, %lu, %p)\n", pName, pPortName, dwLevel, pPortInfo);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
SetPortW(PWSTR pName, PWSTR pPortName, DWORD dwLevel, PBYTE pPortInfo)
{
    TRACE("SetPortW(%S, %S, %lu, %p)\n", pName, pPortName, dwLevel, pPortInfo);
    UNIMPLEMENTED;
    return FALSE;
}
