/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Print Processors
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallDownDatatypesInfo(PDATATYPES_INFO_1W* ppDatatypesInfo1)
{
    // Replace absolute pointer addresses in the output by relative offsets.
    PDATATYPES_INFO_1W pDatatypesInfo1 = *ppDatatypesInfo1;
    pDatatypesInfo1->pName = (PWSTR)((ULONG_PTR)pDatatypesInfo1->pName - (ULONG_PTR)pDatatypesInfo1);
    *ppDatatypesInfo1 += sizeof(DATATYPES_INFO_1W);
}

static void
_MarshallDownPrintProcessorInfo(PPRINTPROCESSOR_INFO_1W* ppPrintProcessorInfo1)
{
    // Replace absolute pointer addresses in the output by relative offsets.
    PPRINTPROCESSOR_INFO_1W pPrintProcessorInfo1 = *ppPrintProcessorInfo1;
    pPrintProcessorInfo1->pName = (PWSTR)((ULONG_PTR)pPrintProcessorInfo1->pName - (ULONG_PTR)pPrintProcessorInfo1);
    *ppPrintProcessorInfo1 += sizeof(PRINTPROCESSOR_INFO_1W);
}

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
        DWORD i;
        PDATATYPES_INFO_1W p = (PDATATYPES_INFO_1W)pDatatypesAligned;

        for (i = 0; i < *pcReturned; i++)
            _MarshallDownDatatypesInfo(&p);
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
        DWORD i;
        PPRINTPROCESSOR_INFO_1W p = (PPRINTPROCESSOR_INFO_1W)pPrintProcessorInfoAligned;

        for (i = 0; i < *pcReturned; i++)
            _MarshallDownPrintProcessorInfo(&p);
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
