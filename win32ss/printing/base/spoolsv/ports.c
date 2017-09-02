/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Ports
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallDownPortInfo(PBYTE* ppPortInfo, DWORD Level)
{
    // Replace absolute pointer addresses in the output by relative offsets.
    if (Level == 1)
    {
        PPORT_INFO_1W pPortInfo1 = (PPORT_INFO_1W)(*ppPortInfo);

        pPortInfo1->pName = (PWSTR)((ULONG_PTR)pPortInfo1->pName - (ULONG_PTR)pPortInfo1);

        *ppPortInfo += sizeof(PORT_INFO_1W);
    }
    else if (Level == 2)
    {
        PPORT_INFO_2W pPortInfo2 = (PPORT_INFO_2W)(*ppPortInfo);

        pPortInfo2->pPortName = (PWSTR)((ULONG_PTR)pPortInfo2->pPortName - (ULONG_PTR)pPortInfo2);
        pPortInfo2->pDescription = (PWSTR)((ULONG_PTR)pPortInfo2->pDescription - (ULONG_PTR)pPortInfo2);
        pPortInfo2->pMonitorName = (PWSTR)((ULONG_PTR)pPortInfo2->pMonitorName - (ULONG_PTR)pPortInfo2);

        *ppPortInfo += sizeof(PORT_INFO_2W);
    }
    else if (Level == 3)
    {
        PPORT_INFO_3W pPortInfo3 = (PPORT_INFO_3W)(*ppPortInfo);

        pPortInfo3->pszStatus = (PWSTR)((ULONG_PTR)pPortInfo3->pszStatus - (ULONG_PTR)pPortInfo3);

        *ppPortInfo += sizeof(PORT_INFO_3W);
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
        DWORD i;
        PBYTE p = pPortAligned;

        for (i = 0; i < *pcReturned; i++)
            _MarshallDownPortInfo(&p, Level);
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
