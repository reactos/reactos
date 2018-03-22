/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Print Processors
 * COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/printprocessors.h>

DWORD
_RpcAddPrintProcessor(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, WCHAR* pPathName, WCHAR* pPrintProcessorName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcDeletePrintProcessor(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, WCHAR* pPrintProcessorName)
{
    UNIMPLEMENTED;
    return ERROR_INVALID_FUNCTION;
}

DWORD
_RpcEnumPrintProcessorDatatypes(WINSPOOL_HANDLE pName, WCHAR* pPrintProcessorName, DWORD Level, BYTE* pDatatypes, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;
    PBYTE pDatatypesAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pDatatypesAligned = AlignRpcPtr(pDatatypes, &cbBuf);

    if (EnumPrintProcessorDatatypesW(pName, pPrintProcessorName, Level, pDatatypesAligned, cbBuf, pcbNeeded, pcReturned))
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        MarshallDownStructuresArray(pDatatypesAligned, *pcReturned, DatatypesInfo1Marshalling.pInfo, DatatypesInfo1Marshalling.cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pDatatypes, pDatatypesAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcEnumPrintProcessors(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, DWORD Level, BYTE* pPrintProcessorInfo, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;
    PBYTE pPrintProcessorInfoAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pPrintProcessorInfoAligned = AlignRpcPtr(pPrintProcessorInfo, &cbBuf);

    if (EnumPrintProcessorsW(pName, pEnvironment, Level, pPrintProcessorInfoAligned, cbBuf, pcbNeeded, pcReturned))
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        MarshallDownStructuresArray(pPrintProcessorInfoAligned, *pcReturned, PrintProcessorInfo1Marshalling.pInfo, PrintProcessorInfo1Marshalling.cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pPrintProcessorInfo, pPrintProcessorInfoAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcGetPrintProcessorDirectory(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, DWORD Level, BYTE* pPrintProcessorDirectory, DWORD cbBuf, DWORD* pcbNeeded)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!GetPrintProcessorDirectoryW(pName, pEnvironment, Level, pPrintProcessorDirectory, cbBuf, pcbNeeded))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
