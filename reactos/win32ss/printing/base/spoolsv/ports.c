/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Ports
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD
_RpcEnumPorts(WINSPOOL_HANDLE pName, DWORD Level, BYTE* pPort, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    EnumPortsW(pName, Level, pPort, cbBuf, pcbNeeded, pcReturned);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
