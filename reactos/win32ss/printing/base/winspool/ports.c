/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Ports
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallUpPortInfo(PBYTE pPortInfo, DWORD Level)
{
    PPORT_INFO_2W pPortInfo2 = (PPORT_INFO_2W)pPortInfo;         // PORT_INFO_1W is a subset of PORT_INFO_2W

    // Replace relative offset addresses in the output by absolute pointers.
    pPortInfo2->pPortName = (PWSTR)((ULONG_PTR)pPortInfo2->pPortName + (ULONG_PTR)pPortInfo2);

    if (Level == 2)
    {
        pPortInfo2->pDescription = (PWSTR)((ULONG_PTR)pPortInfo2->pDescription + (ULONG_PTR)pPortInfo2);
        pPortInfo2->pMonitorName = (PWSTR)((ULONG_PTR)pPortInfo2->pMonitorName + (ULONG_PTR)pPortInfo2);
    }
}

BOOL WINAPI
AddPortW(PWSTR pName, HWND hWnd, PWSTR pMonitorName)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
ConfigurePortW(PWSTR pName, HWND hWnd, PWSTR pPortName)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePortW(PWSTR pName, HWND hWnd, PWSTR pPortName)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPortsW(PWSTR pName, DWORD Level, PBYTE pPorts, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    DWORD i;
    PBYTE p = pPorts;

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
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallUpPortInfo(p, Level);

            if (Level == 1)
                p += sizeof(PORT_INFO_1W);
            else if (Level == 2)
                p += sizeof(PORT_INFO_2W);
        }
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
