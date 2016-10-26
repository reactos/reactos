/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GNU LGPL v2.1 or any later version as published by the Free Software Foundation
 * PURPOSE:     Functions related to Print Processors
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#include "precomp.h"

static void
_MarshallUpDatatypesInfo(PDATATYPES_INFO_1W pDatatypesInfo1)
{
    // Replace relative offset addresses in the output by absolute pointers.
    pDatatypesInfo1->pName = (PWSTR)((ULONG_PTR)pDatatypesInfo1->pName + (ULONG_PTR)pDatatypesInfo1);
}

static void
_MarshallUpPrintProcessorInfo(PPRINTPROCESSOR_INFO_1W pPrintProcessorInfo1)
{
    // Replace relative offset addresses in the output by absolute pointers.
    pPrintProcessorInfo1->pName = (PWSTR)((ULONG_PTR)pPrintProcessorInfo1->pName + (ULONG_PTR)pPrintProcessorInfo1);
}

BOOL WINAPI
AddPrintProcessorW(PWSTR pName, PWSTR pEnvironment, PWSTR pPathName, PWSTR pPrintProcessorName)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePrintProcessorW(PWSTR pName, PWSTR pEnvironment, PWSTR pPrintProcessorName)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPrintProcessorDatatypesA(PSTR pName, LPSTR pPrintProcessorName, DWORD Level, PBYTE pDatatypes, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPrintProcessorDatatypesW(PWSTR pName, LPWSTR pPrintProcessorName, DWORD Level, PBYTE pDatatypes, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    DWORD i;
    PBYTE p = pDatatypes;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumPrintProcessorDatatypes(pName, pPrintProcessorName, Level, pDatatypes, cbBuf, pcbNeeded, pcReturned);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcEnumPrintProcessorDatatypes failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallUpDatatypesInfo((PDATATYPES_INFO_1W)p);
            p += sizeof(DATATYPES_INFO_1W);
        }
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
EnumPrintProcessorsW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    DWORD i;
    PBYTE p = pPrintProcessorInfo;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumPrintProcessors(pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded, pcReturned);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcEnumPrintProcessors failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        for (i = 0; i < *pcReturned; i++)
        {
            _MarshallUpPrintProcessorInfo((PPRINTPROCESSOR_INFO_1W)p);
            p += sizeof(PRINTPROCESSOR_INFO_1W);
        }
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetPrintProcessorDirectoryW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded)
{
    BOOL bReturnValue = FALSE;
    DWORD dwErrorCode;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcGetPrintProcessorDirectory(pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded);
        SetLastError(dwErrorCode);
        bReturnValue = (dwErrorCode == ERROR_SUCCESS);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("_RpcGetPrintProcessorDirectory failed with exception code %lu!\n", RpcExceptionCode());
    }
    RpcEndExcept;

    return bReturnValue;
}
