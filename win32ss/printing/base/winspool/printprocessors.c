/*
* PROJECT:     ReactOS Spooler API
* LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
* PURPOSE:     Functions related to Print Processors
* COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
*/

#include "precomp.h"
#include <prtprocenv.h>

static void
_MarshallUpDatatypesInfo(PDATATYPES_INFO_1W* ppDatatypesInfo1)
{
    // Replace relative offset addresses in the output by absolute pointers.
    PDATATYPES_INFO_1W pDatatypesInfo1 = *ppDatatypesInfo1;
    pDatatypesInfo1->pName = (PWSTR)((ULONG_PTR)pDatatypesInfo1->pName + (ULONG_PTR)pDatatypesInfo1);
    *ppDatatypesInfo1 += sizeof(DATATYPES_INFO_1W);
}

static void
_MarshallUpPrintProcessorInfo(PPRINTPROCESSOR_INFO_1W* ppPrintProcessorInfo1)
{
    // Replace relative offset addresses in the output by absolute pointers.
    PPRINTPROCESSOR_INFO_1W pPrintProcessorInfo1 = *ppPrintProcessorInfo1;
    pPrintProcessorInfo1->pName = (PWSTR)((ULONG_PTR)pPrintProcessorInfo1->pName + (ULONG_PTR)pPrintProcessorInfo1);
    *ppPrintProcessorInfo1 += sizeof(PRINTPROCESSOR_INFO_1W);
}

BOOL WINAPI
AddPrintProcessorA(PSTR pName, PSTR pEnvironment, PSTR pPathName, PSTR pPrintProcessorName)
{
    TRACE("AddPrintProcessorA(%s, %s, %s, %s)\n", pName, pEnvironment, pPathName, pPrintProcessorName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
AddPrintProcessorW(PWSTR pName, PWSTR pEnvironment, PWSTR pPathName, PWSTR pPrintProcessorName)
{
    TRACE("AddPrintProcessorW(%S, %S, %S, %S)\n", pName, pEnvironment, pPathName, pPrintProcessorName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePrintProcessorA(PSTR pName, PSTR pEnvironment, PSTR pPrintProcessorName)
{
    TRACE("DeletePrintProcessorA(%s, %s, %s)\n", pName, pEnvironment, pPrintProcessorName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
DeletePrintProcessorW(PWSTR pName, PWSTR pEnvironment, PWSTR pPrintProcessorName)
{
    TRACE("DeletePrintProcessorW(%S, %S, %S)\n", pName, pEnvironment, pPrintProcessorName);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPrintProcessorDatatypesA(PSTR pName, LPSTR pPrintProcessorName, DWORD Level, PBYTE pDatatypes, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    TRACE("EnumPrintProcessorDatatypesA(%s, %s, %lu, %p, %lu, %p, %p)\n", pName, pPrintProcessorName, Level, pDatatypes, cbBuf, pcbNeeded, pcReturned);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPrintProcessorDatatypesW(PWSTR pName, LPWSTR pPrintProcessorName, DWORD Level, PBYTE pDatatypes, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;

    TRACE("EnumPrintProcessorDatatypesW(%S, %S, %lu, %p, %lu, %p, %p)\n", pName, pPrintProcessorName, Level, pDatatypes, cbBuf, pcbNeeded, pcReturned);

    // Sanity checks
    if (Level != 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

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
        DWORD i;
        PDATATYPES_INFO_1W p = (PDATATYPES_INFO_1W)pDatatypes;

        for (i = 0; i < *pcReturned; i++)
            _MarshallUpDatatypesInfo(&p);
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
EnumPrintProcessorsA(PSTR pName, PSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    TRACE("EnumPrintProcessorsA(%s, %s, %lu, %p, %lu, %p, %p)\n", pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded, pcReturned);
    UNIMPLEMENTED;
    return FALSE;
}

BOOL WINAPI
EnumPrintProcessorsW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;

    TRACE("EnumPrintProcessorsW(%S, %S, %lu, %p, %lu, %p, %p)\n", pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded, pcReturned);

    // Choose our current environment if the caller didn't give any.
    if (!pEnvironment)
        pEnvironment = (PWSTR)wszCurrentEnvironment;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumPrintProcessors(pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded, pcReturned);
    }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        DWORD i;
        PPRINTPROCESSOR_INFO_1W p = (PPRINTPROCESSOR_INFO_1W)pPrintProcessorInfo;

        for (i = 0; i < *pcReturned; i++)
            _MarshallUpPrintProcessorInfo(&p);
    }

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetPrintProcessorDirectoryA(PSTR pName, PSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded)
{
    BOOL bReturnValue = FALSE;
    DWORD cch;
    PWSTR pwszName = NULL;
    PWSTR pwszEnvironment = NULL;
    PWSTR pwszPrintProcessorInfo = NULL;

    TRACE("GetPrintProcessorDirectoryA(%s, %s, %lu, %p, %lu, %p)\n", pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded);

    if (pName)
    {
        // Convert pName to a Unicode string pwszName.
        cch = strlen(pName);

        pwszName = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszName)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pName, -1, pwszName, cch + 1);
    }

    if (pEnvironment)
    {
        // Convert pEnvironment to a Unicode string pwszEnvironment.
        cch = strlen(pEnvironment);

        pwszEnvironment = HeapAlloc(hProcessHeap, 0, (cch + 1) * sizeof(WCHAR));
        if (!pwszEnvironment)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }

        MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, pwszEnvironment, cch + 1);
    }

    if (cbBuf && pPrintProcessorInfo)
    {
        // Allocate a temporary buffer for the Unicode result.
        // We can just go with cbBuf here. The user should have set it based on pcbNeeded returned in a previous call and our
        // pcbNeeded is the same for the A and W functions.
        pwszPrintProcessorInfo = HeapAlloc(hProcessHeap, 0, cbBuf);
        if (!pwszPrintProcessorInfo)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            ERR("HeapAlloc failed!\n");
            goto Cleanup;
        }
    }

    bReturnValue = GetPrintProcessorDirectoryW(pwszName, pwszEnvironment, Level, (PBYTE)pwszPrintProcessorInfo, cbBuf, pcbNeeded);

    if (bReturnValue)
    {
        // Convert pwszPrintProcessorInfo to an ANSI string pPrintProcessorInfo.
        WideCharToMultiByte(CP_ACP, 0, pwszPrintProcessorInfo, -1, (PSTR)pPrintProcessorInfo, cbBuf, NULL, NULL);
    }

Cleanup:
    if (pwszName)
        HeapFree(hProcessHeap, 0, pwszName);

    if (pwszEnvironment)
        HeapFree(hProcessHeap, 0, pwszEnvironment);

    if (pwszPrintProcessorInfo)
        HeapFree(hProcessHeap, 0, pwszPrintProcessorInfo);

    return bReturnValue;
}

BOOL WINAPI
GetPrintProcessorDirectoryW(PWSTR pName, PWSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;

    TRACE("GetPrintProcessorDirectoryW(%S, %S, %lu, %p, %lu, %p)\n", pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded);

    // Sanity checks
    if (Level != 1)
    {
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Choose our current environment if the caller didn't give any.
    if (!pEnvironment)
        pEnvironment = (PWSTR)wszCurrentEnvironment;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcGetPrintProcessorDirectory(pName, pEnvironment, Level, pPrintProcessorInfo, cbBuf, pcbNeeded);
    }
        RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
    }
    RpcEndExcept;

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
