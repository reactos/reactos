/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Print Providers
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"

BOOL WINAPI
AddPrintProvidorA(PSTR pName, DWORD Level, PBYTE pProviderInfo)
{
    LPWSTR nameW = NULL;
    PROVIDOR_INFO_1W pi1W;
    PROVIDOR_INFO_2W pi2W;
    DWORD len;
    BOOL res;
    PBYTE pPI = NULL;

    TRACE("AddPrintProvidorA(%s, %lu, %p)\n", pName, Level, pProviderInfo);

    ZeroMemory(&pi1W, sizeof(PROVIDOR_INFO_1W));
    pi2W.pOrder = NULL;

    switch (Level)
    {
        case 1:
        {
            PROVIDOR_INFO_1A *pi1A = (PROVIDOR_INFO_1A*)pProviderInfo;
            if (pi1A->pName)
            {
                len = MultiByteToWideChar(CP_ACP, 0, pi1A->pName, -1, NULL, 0);
                pi1W.pName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
                MultiByteToWideChar(CP_ACP, 0, pi1A->pName, -1, pi1W.pName, len);
            }
            if (pi1A->pEnvironment)
            {
                len = MultiByteToWideChar(CP_ACP, 0, pi1A->pEnvironment, -1, NULL, 0);
                pi1W.pEnvironment = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
                MultiByteToWideChar(CP_ACP, 0, pi1A->pEnvironment, -1, pi1W.pEnvironment, len);
            }
            if (pi1A->pDLLName)
            {
                len = MultiByteToWideChar(CP_ACP, 0, pi1A->pDLLName, -1, NULL, 0);
                pi1W.pDLLName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
                MultiByteToWideChar(CP_ACP, 0, pi1A->pDLLName, -1, pi1W.pDLLName, len);
            }
            pPI = (PBYTE)&pi1W;
        }
            break;

        case 2:
        {
            PROVIDOR_INFO_2A *pi2A = (PROVIDOR_INFO_2A*)pProviderInfo;
            if (pi2A->pOrder)
            {
                len = MultiByteToWideChar(CP_ACP, 0, pi2A->pOrder, -1, NULL, 0);
                pi2W.pOrder = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
                MultiByteToWideChar(CP_ACP, 0, pi2A->pOrder, -1, pi2W.pOrder, len);
            }
            pPI = (PBYTE)&pi2W;
        }
            break;

        default:
            SetLastError(ERROR_INVALID_LEVEL);
            return FALSE;
    }

    if (pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pName, -1, NULL, 0);
        nameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pName, -1, nameW, len);
    }

    res = AddPrintProvidorW( nameW, Level, pPI );

    if (pName) HeapFree(GetProcessHeap(), 0, nameW);
    if (pi1W.pName) HeapFree(GetProcessHeap(), 0, pi1W.pName);
    if (pi1W.pEnvironment) HeapFree(GetProcessHeap(), 0, pi1W.pEnvironment);
    if (pi1W.pDLLName) HeapFree(GetProcessHeap(), 0, pi1W.pDLLName);
    if (pi2W.pOrder) HeapFree(GetProcessHeap(), 0, pi2W.pOrder);

    return res;
}

BOOL WINAPI
AddPrintProvidorW(PWSTR pName, DWORD Level, PBYTE pProviderInfo)
{
    DWORD dwErrorCode;
    WINSPOOL_PROVIDOR_CONTAINER ProvidorContainer;

    TRACE("AddPrintProvidorW(%S, %lu, %p)\n", pName, Level, pProviderInfo);

    if ((Level < 1) || (Level > 2))
    {
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    ProvidorContainer.ProvidorInfo.pProvidorInfo1 = (WINSPOOL_PROVIDOR_INFO_1*)pProviderInfo;
    ProvidorContainer.Level = Level;

    RpcTryExcept
    {
        dwErrorCode = _RpcAddPrintProvidor( pName, &ProvidorContainer );
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcEnumPorts failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
DeletePrintProvidorA(PSTR pName, PSTR pEnvironment, PSTR pPrintProviderName)
{
    UNICODE_STRING NameW, EnvW, ProviderW;
    BOOL Ret;

    TRACE("DeletePrintProvidorW(%s, %s, %s)\n", pName, pEnvironment, pPrintProviderName);

    AsciiToUnicode(&NameW, pName);
    AsciiToUnicode(&EnvW, pEnvironment);
    AsciiToUnicode(&ProviderW, pPrintProviderName);

    Ret = DeletePrintProvidorW(NameW.Buffer, EnvW.Buffer, ProviderW.Buffer);

    RtlFreeUnicodeString(&ProviderW);
    RtlFreeUnicodeString(&EnvW);
    RtlFreeUnicodeString(&NameW);

    return Ret;
}

BOOL WINAPI
DeletePrintProvidorW(PWSTR pName, PWSTR pEnvironment, PWSTR pPrintProviderName)
{
    DWORD dwErrorCode;

    TRACE("DeletePrintProvidorW(%S, %S, %S)\n", pName, pEnvironment, pPrintProviderName);

    RpcTryExcept
    {
        dwErrorCode = _RpcDeletePrintProvidor( pName, pEnvironment, pPrintProviderName );
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcEnumPorts failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
