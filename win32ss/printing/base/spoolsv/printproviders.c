/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Print Providers
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

DWORD
_RpcAddPrintProvidor(WINSPOOL_HANDLE pName, WINSPOOL_PROVIDOR_CONTAINER* pProvidorContainer)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!AddPrintProvidorW(pName, pProvidorContainer->Level, (PBYTE)pProvidorContainer->ProvidorInfo.pProvidorInfo1))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;

}

DWORD
_RpcDeletePrintProvidor(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, WCHAR* pPrintProviderName)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!DeletePrintProvidorW(pName, pEnvironment, pPrintProviderName))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
