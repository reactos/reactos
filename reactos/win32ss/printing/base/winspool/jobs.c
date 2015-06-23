/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions for managing print jobs
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

BOOL WINAPI
AddJob(HANDLE hPrinter, DWORD Level, LPBYTE pData, DWORD cbBuf, LPDWORD pcbNeeded)
{
    BOOL bReturnValue = FALSE;
    DWORD dwErrorCode;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcAddJob(hPrinter, Level, pData, cbBuf, pcbNeeded);
        SetLastError(dwErrorCode);
        bReturnValue = (dwErrorCode == ERROR_SUCCESS);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("_RpcAddJob failed with exception code %lu!\n", RpcExceptionCode());
    }
    RpcEndExcept;

    return bReturnValue;
}

BOOL WINAPI
GetJob(HANDLE hPrinter, DWORD JobId, DWORD Level, LPBYTE pJob, DWORD cbBuf, LPDWORD pcbNeeded)
{
    BOOL bReturnValue = FALSE;
    DWORD dwErrorCode;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcGetJob(hPrinter, JobId, Level, pJob, cbBuf, pcbNeeded);
        SetLastError(dwErrorCode);
        bReturnValue = (dwErrorCode == ERROR_SUCCESS);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("_RpcGetJob failed with exception code %lu!\n", RpcExceptionCode());
    }
    RpcEndExcept;

    return bReturnValue;
}
