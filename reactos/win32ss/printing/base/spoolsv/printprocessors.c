/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Print Processors
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallDownDatatypesInfo(PDATATYPES_INFO_1W pDatatypesInfo1)
{
    // Replace absolute pointer addresses in the output by relative offsets.
    pDatatypesInfo1->pName = (PWSTR)((ULONG_PTR)pDatatypesInfo1->pName - (ULONG_PTR)pDatatypesInfo1);
}

static void
_MarshallDownPrintProcessorInfo(PPRINTPROCESSOR_INFO_1W pPrintProcessorInfo1)
{
    // Replace absolute pointer addresses in the output by relative offsets.
    pPrintProcessorInfo1->pName = (PWSTR)((ULONG_PTR)pPrintProcessorInfo1->pName - (ULONG_PTR)pPrintProcessorInfo1);
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
    DWORD i;
    PBYTE p = pDatatypes;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    EnumPrintProcessorDatatypesW(pName, pPrintProcessorName, Level, pDatatypes, cbBuf, pcbNeeded, pcReturned);
    dwErrorCode = GetLastError();

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallDownDatatypesInfo((PDATATYPES_INFO_1W)p);
            p += sizeof(DATATYPES_INFO_1W);
        }
    }

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcEnumPrintProcessors(WINSPOOL_HANDLE pName, WCHAR* pEnvironment, DWORD Level, BYTE* pPrintProcessorInfo, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;
    DWORD i;
    PBYTE p = pPrintProcessorInfo;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    EnumPrintProcessorsW(pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded, pcReturned);
    dwErrorCode = GetLastError();

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallDownPrintProcessorInfo((PPRINTPROCESSOR_INFO_1W)p);
            p += sizeof(PRINTPROCESSOR_INFO_1W);
        }
    }

    RpcRevertToSelf();
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

    GetPrintProcessorDirectoryW(pName, pEnvironment, Level, pPrintProcessorDirectory, cbBuf, pcbNeeded);
    dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
