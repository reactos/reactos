/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Print Monitors
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD
_RpcEnumMonitors(WINSPOOL_HANDLE pName, DWORD Level, BYTE* pMonitor, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    EnumMonitorsW(pName, Level, pMonitor, cbBuf, pcbNeeded, pcReturned);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
