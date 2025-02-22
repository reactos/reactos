/*
* PROJECT:     ReactOS Spooler API
* LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
* PURPOSE:     Functions related to Print Processors
* COPYRIGHT:   Copyright 2015-2018 Colin Finck (colin@reactos.org)
*/

#include "precomp.h"
#include <marshalling/printprocessors.h>
#include <prtprocenv.h>

BOOL WINAPI
AddPrintProcessorA(PSTR pName, PSTR pEnvironment, PSTR pPathName, PSTR pPrintProcessorName)
{
    UNICODE_STRING NameW, EnvW, PathW, ProcessorW;
    BOOL Ret;

    TRACE("AddPrintProcessorA(%s, %s, %s, %s)\n", pName, pEnvironment, pPathName, pPrintProcessorName);

    AsciiToUnicode(&NameW, pName);
    AsciiToUnicode(&EnvW, pEnvironment);
    AsciiToUnicode(&PathW, pPathName);
    AsciiToUnicode(&ProcessorW, pPrintProcessorName);

    Ret = AddPrintProcessorW(NameW.Buffer, EnvW.Buffer, PathW.Buffer, ProcessorW.Buffer);

    RtlFreeUnicodeString(&ProcessorW);
    RtlFreeUnicodeString(&PathW);
    RtlFreeUnicodeString(&EnvW);
    RtlFreeUnicodeString(&NameW);

    return Ret;
}

BOOL WINAPI
AddPrintProcessorW(PWSTR pName, PWSTR pEnvironment, PWSTR pPathName, PWSTR pPrintProcessorName)
{
    DWORD dwErrorCode;

    TRACE("AddPrintProcessorW(%S, %S, %S, %S)\n", pName, pEnvironment, pPathName, pPrintProcessorName);

    RpcTryExcept
    {
        dwErrorCode = _RpcAddPrintProcessor( pName, pEnvironment, pPathName, pPrintProcessorName );
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcPrintProcessor failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
DeletePrintProcessorA(PSTR pName, PSTR pEnvironment, PSTR pPrintProcessorName)
{
    UNICODE_STRING NameW, EnvW, ProcessorW;
    BOOL Ret;

    TRACE("DeletePrintProcessorA(%s, %s, %s)\n", pName, pEnvironment, pPrintProcessorName);

    AsciiToUnicode(&NameW, pName);
    AsciiToUnicode(&EnvW, pEnvironment);
    AsciiToUnicode(&ProcessorW, pPrintProcessorName);

    Ret = DeletePrintProcessorW(NameW.Buffer, EnvW.Buffer, ProcessorW.Buffer);

    RtlFreeUnicodeString(&ProcessorW);
    RtlFreeUnicodeString(&EnvW);
    RtlFreeUnicodeString(&NameW);

    return Ret;
}

BOOL WINAPI
DeletePrintProcessorW(PWSTR pName, PWSTR pEnvironment, PWSTR pPrintProcessorName)
{
    DWORD dwErrorCode;

    TRACE("DeletePrintProcessorW(%S, %S, %S)\n", pName, pEnvironment, pPrintProcessorName);

    RpcTryExcept
    {
        dwErrorCode = _RpcDeletePrintProcessor( pName, pEnvironment, pPrintProcessorName );
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcDeletePrintProcessor failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
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
        // Replace relative offset addresses in the output by absolute pointers.
        MarshallUpStructuresArray(cbBuf, pDatatypes, *pcReturned, DatatypesInfo1Marshalling.pInfo, DatatypesInfo1Marshalling.cbStructureSize, TRUE);
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
EnumPrintProcessorsA(PSTR pName, PSTR pEnvironment, DWORD Level, PBYTE pPrintProcessorInfo, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    BOOL    res;
    LPBYTE  bufferW = NULL;
    LPWSTR  nameW = NULL;
    LPWSTR  envW = NULL;
    DWORD   needed = 0;
    DWORD   numentries = 0;
    INT     len;

    TRACE("EnumPrintProcessorsA(%s, %s, %d, %p, %d, %p, %p)\n", debugstr_a(pName), debugstr_a(pEnvironment), Level, pPrintProcessorInfo, cbBuf, pcbNeeded, pcReturned);

    /* convert names to unicode */
    if (pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }
    if (pEnvironment)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, NULL, 0);
        envW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pEnvironment, -1, envW, len);
    }

    /* alloc (userbuffersize*sizeof(WCHAR) and try to enum the monitors */
    needed = cbBuf * sizeof(WCHAR);
    if (needed) bufferW = HeapAlloc(GetProcessHeap(), 0, needed);
    res = EnumPrintProcessorsW(nameW, envW, Level, bufferW, needed, pcbNeeded, pcReturned);

    if (!res && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
    {
        if (pcbNeeded) needed = *pcbNeeded;
        /* HeapReAlloc return NULL, when bufferW was NULL */
        bufferW = (bufferW) ? HeapReAlloc(GetProcessHeap(), 0, bufferW, needed) :
                              HeapAlloc(GetProcessHeap(), 0, needed);

        /* Try again with the large Buffer */
        res = EnumPrintProcessorsW(nameW, envW, Level, bufferW, needed, pcbNeeded, pcReturned);
    }
    numentries = pcReturned ? *pcReturned : 0;
    needed = 0;

    if (res)
    {
        /* EnumPrintProcessorsW collected all Data. Parse them to calculate ANSI-Size */
        DWORD   index;
        LPSTR   ptr;
        PPRINTPROCESSOR_INFO_1W ppiw;
        PPRINTPROCESSOR_INFO_1A ppia;

        /* First pass: calculate the size for all Entries */
        ppiw = (PPRINTPROCESSOR_INFO_1W) bufferW;
        ppia = (PPRINTPROCESSOR_INFO_1A) pPrintProcessorInfo;
        index = 0;
        while (index < numentries)
        {
            index++;
            needed += sizeof(PRINTPROCESSOR_INFO_1A);
            TRACE("%p: parsing #%d (%s)\n", ppiw, index, debugstr_w(ppiw->pName));

            needed += WideCharToMultiByte(CP_ACP, 0, ppiw->pName, -1,
                                            NULL, 0, NULL, NULL);

            ppiw = (PPRINTPROCESSOR_INFO_1W) (((LPBYTE)ppiw) + sizeof(PRINTPROCESSOR_INFO_1W));
            ppia = (PPRINTPROCESSOR_INFO_1A) (((LPBYTE)ppia) + sizeof(PRINTPROCESSOR_INFO_1A));
        }

        /* check for errors and quit on failure */
        if (cbBuf < needed)
        {
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            res = FALSE;
            goto epp_cleanup;
        }

        len = numentries * sizeof(PRINTPROCESSOR_INFO_1A); /* room for structs */
        ptr = (LPSTR) &pPrintProcessorInfo[len];        /* start of strings */
        cbBuf -= len ;                      /* free Bytes in the user-Buffer */
        ppiw = (PPRINTPROCESSOR_INFO_1W) bufferW;
        ppia = (PPRINTPROCESSOR_INFO_1A) pPrintProcessorInfo;
        index = 0;
        /* Second Pass: Fill the User Buffer (if we have one) */
        while ((index < numentries) && pPrintProcessorInfo)
        {
            index++;
            TRACE("%p: writing PRINTPROCESSOR_INFO_1A #%d\n", ppia, index);
            ppia->pName = ptr;
            len = WideCharToMultiByte(CP_ACP, 0, ppiw->pName, -1,
                                            ptr, cbBuf , NULL, NULL);
            ptr += len;
            cbBuf -= len;

            ppiw = (PPRINTPROCESSOR_INFO_1W) (((LPBYTE)ppiw) + sizeof(PRINTPROCESSOR_INFO_1W));
            ppia = (PPRINTPROCESSOR_INFO_1A) (((LPBYTE)ppia) + sizeof(PRINTPROCESSOR_INFO_1A));

        }
    }
epp_cleanup:
    if (pcbNeeded)  *pcbNeeded = needed;
    if (pcReturned) *pcReturned = (res) ? numentries : 0;

    if (nameW) HeapFree(GetProcessHeap(), 0, nameW);
    if (envW) HeapFree(GetProcessHeap(), 0, envW);
    if (bufferW) HeapFree(GetProcessHeap(), 0, bufferW);

    TRACE("returning %d with %d (%d byte for %d entries)\n", (res), GetLastError(), needed, numentries);

    return (res);

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
        // Replace relative offset addresses in the output by absolute pointers.
        MarshallUpStructuresArray(cbBuf, pPrintProcessorInfo, *pcReturned, PrintProcessorInfo1Marshalling.pInfo, PrintProcessorInfo1Marshalling.cbStructureSize, TRUE);
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
