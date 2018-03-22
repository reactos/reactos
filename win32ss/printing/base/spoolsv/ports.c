/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Ports
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/ports.h>

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
    PBYTE pPortAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pPortAligned = AlignRpcPtr(pPort, &cbBuf);

    if (EnumPortsW(pName, Level, pPortAligned, cbBuf, pcbNeeded, pcReturned))
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        ASSERT(Level >= 1 && Level <= 2);
        MarshallDownStructuresArray(pPortAligned, *pcReturned, pPortInfoMarshalling[Level]->pInfo, pPortInfoMarshalling[Level]->cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pPort, pPortAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcSetPort(WINSPOOL_HANDLE pName, WCHAR* pPortName, WINSPOOL_PORT_CONTAINER* pPortContainer)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}
