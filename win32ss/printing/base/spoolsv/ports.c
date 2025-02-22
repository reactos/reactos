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
    DWORD dwErrorCode;

    FIXME("AddPort(%S, %p, %s)\n", pName, hWnd, debugstr_w(pMonitorName));

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!AddPortW( pName, (HWND)hWnd, pMonitorName ))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcAddPortEx(WINSPOOL_HANDLE pName, WINSPOOL_PORT_CONTAINER* pPortContainer, WINSPOOL_PORT_VAR_CONTAINER* pPortVarContainer, WCHAR* pMonitorName)
{
    DWORD dwErrorCode, Level = pPortContainer->Level;
    WINSPOOL_PORT_INFO_FF PortInfoFF;
    PBYTE lpBuffer;

    FIXME("AddPortEx(%S, %lu, %s)\n", pName, Level, debugstr_w(pMonitorName));

    switch (Level)
    {
        case 1:
           lpBuffer = (PBYTE)pPortContainer->PortInfo.pPortInfo1;
           break;

        case 0xFFFFFFFF:
           PortInfoFF.pPortName = pPortContainer->PortInfo.pPortInfoFF->pPortName;
           PortInfoFF.cbMonitorData = pPortVarContainer->cbMonitorData;
           PortInfoFF.pMonitorData =  pPortVarContainer->pMonitorData;
           lpBuffer = (PBYTE)&PortInfoFF;
           break;

        default:
           ERR("Level = %d, unsupported!\n", Level);
           return ERROR_INVALID_LEVEL;
    }

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!AddPortExW(pName, Level, lpBuffer, pMonitorName ))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcConfigurePort(WINSPOOL_HANDLE pName, ULONG_PTR hWnd, WCHAR* pPortName)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!ConfigurePortW( pName, (HWND)hWnd, pPortName ))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcDeletePort(WINSPOOL_HANDLE pName, ULONG_PTR hWnd, WCHAR* pPortName)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!DeletePortW( pName, (HWND)hWnd, pPortName ))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
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
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!SetPortW(pName, pPortName, pPortContainer->Level, (PBYTE)pPortContainer->PortInfo.pPortInfo3))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
