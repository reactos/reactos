/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Print Processors
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

DWORD
_RpcAddPrintProcessor(WINSPOOL_HANDLE pName, WCHAR *pEnvironment, WCHAR *pPathName, WCHAR *pPrintProcessorName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPrintProcessorDatatypes(WINSPOOL_HANDLE pName, WCHAR *pPrintProcessorName, DWORD Level, BYTE *pDatatypes, DWORD cbBuf, DWORD *pcbNeeded, DWORD *pcReturned)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    dwErrorCode = EnumPrintProcessorDatatypesW(pName, pPrintProcessorName, Level, pDatatypes, cbBuf, pcbNeeded, pcReturned);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("EnumPrintProcessorDatatypesW failed with error %lu!\n", dwErrorCode);
        RpcRevertToSelf();
        return dwErrorCode;
    }

    return RpcRevertToSelf();
}

DWORD
_RpcEnumPrintProcessors(WINSPOOL_HANDLE pName, WCHAR *pEnvironment, DWORD Level, BYTE *pPrintProcessorInfo, DWORD cbBuf, DWORD *pcbNeeded, DWORD *pcReturned)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    dwErrorCode = EnumPrintProcessorsW(pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded, pcReturned);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("EnumPrintProcessorsW failed with error %lu!\n", dwErrorCode);
        RpcRevertToSelf();
        return dwErrorCode;
    }

    return RpcRevertToSelf();
}

DWORD
_RpcGetPrintProcessorDirectory(WINSPOOL_HANDLE pName, WCHAR *pEnvironment, DWORD Level, BYTE *pPrintProcessorDirectory, DWORD cbBuf, DWORD *pcbNeeded)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    dwErrorCode = GetPrintProcessorDirectoryW(pName, pEnvironment, Level, pPrintProcessorDirectory, cbBuf, pcbNeeded);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("EnumPrintProcessorsW failed with error %lu!\n", dwErrorCode);
        RpcRevertToSelf();
        return dwErrorCode;
    }

    return RpcRevertToSelf();
}
