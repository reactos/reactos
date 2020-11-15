/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Xcv* functions
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

DWORD
_RpcXcvData(WINSPOOL_PRINTER_HANDLE hXcv, const WCHAR* pszDataName, BYTE* pInputData, DWORD cbInputData, BYTE* pOutputData, DWORD cbOutputData, DWORD* pcbOutputNeeded, DWORD* pdwStatus)
{
    DWORD dwErrorCode;

    FIXME("RpcXcvData( %p, %S,,,)\n",hXcv, pszDataName);

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!XcvDataW(hXcv, pszDataName, pInputData, cbInputData, pOutputData, cbOutputData, pcbOutputNeeded, pdwStatus))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
