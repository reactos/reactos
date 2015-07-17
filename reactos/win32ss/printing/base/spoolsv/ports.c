/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Ports
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallDownPortInfo(PBYTE pPortInfo, DWORD Level)
{
    PPORT_INFO_2W pPortInfo2 = (PPORT_INFO_2W)pPortInfo;         // PORT_INFO_1W is a subset of PORT_INFO_2W

    // Replace absolute pointer addresses in the output by relative offsets.
    pPortInfo2->pPortName = (PWSTR)((ULONG_PTR)pPortInfo2->pPortName - (ULONG_PTR)pPortInfo2);

    if (Level == 2)
    {
        pPortInfo2->pDescription = (PWSTR)((ULONG_PTR)pPortInfo2->pDescription - (ULONG_PTR)pPortInfo2);
        pPortInfo2->pMonitorName = (PWSTR)((ULONG_PTR)pPortInfo2->pMonitorName - (ULONG_PTR)pPortInfo2);
    }
}

DWORD
_RpcAddPort(WINSPOOL_HANDLE pName, ULONG_PTR hWnd, WCHAR* pMonitorName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcAddPortEx(WINSPOOL_HANDLE pName, WINSPOOL_PORT_CONTAINER* pPortContainer, WINSPOOL_PORT_VAR_CONTAINER* pPortVarContainer, WCHAR* pMonitorName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcConfigurePort(WINSPOOL_HANDLE pName, ULONG_PTR hWnd, WCHAR* pPortName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePort(WINSPOOL_HANDLE pName, ULONG_PTR hWnd, WCHAR* pPortName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPorts(WINSPOOL_HANDLE pName, DWORD Level, BYTE* pPort, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;
    DWORD i;
    PBYTE p = pPort;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    EnumPortsW(pName, Level, pPort, cbBuf, pcbNeeded, pcReturned);
    dwErrorCode = GetLastError();

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallDownPortInfo(p, Level);

            if (Level == 1)
                p += sizeof(PORT_INFO_1W);
            else if (Level == 2)
                p += sizeof(PORT_INFO_2W);
        }
    }

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcSetPort(WINSPOOL_HANDLE pName, WCHAR* pPortName, WINSPOOL_PORT_CONTAINER* pPortContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}
