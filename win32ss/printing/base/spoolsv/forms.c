/*
 * PROJECT:     ReactOS Print Spooler Service
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Forms
 * COPYRIGHT:   Copyright 2015 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/forms.h>

DWORD
_RpcAddForm(WINSPOOL_PRINTER_HANDLE hPrinter, WINSPOOL_FORM_CONTAINER* pFormInfoContainer)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!AddFormW(hPrinter, pFormInfoContainer->Level, (PBYTE)pFormInfoContainer->FormInfo.pFormInfo1))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcDeleteForm(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pFormName)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!DeleteFormW(hPrinter, pFormName))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}

DWORD
_RpcEnumForms(WINSPOOL_PRINTER_HANDLE hPrinter, DWORD Level, BYTE* pForm, DWORD cbBuf, DWORD* pcbNeeded, DWORD* pcReturned)
{
    DWORD dwErrorCode;
    PBYTE pFormsEnumAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pFormsEnumAligned = AlignRpcPtr(pForm, &cbBuf);

    if (EnumFormsW(hPrinter, Level, pFormsEnumAligned, cbBuf, pcbNeeded, pcReturned))
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        ASSERT(Level >= 1 && Level <= 2);
        MarshallDownStructuresArray(pFormsEnumAligned, *pcReturned, pFormInfoMarshalling[Level]->pInfo, pFormInfoMarshalling[Level]->cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pForm, pFormsEnumAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcGetForm(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pFormName, DWORD Level, BYTE* pForm, DWORD cbBuf, DWORD* pcbNeeded)
{
    DWORD dwErrorCode;
    PBYTE pFormAligned;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    pFormAligned = AlignRpcPtr(pForm, &cbBuf);

    if (GetFormW(hPrinter, pFormName, Level, pFormAligned, cbBuf, pcbNeeded))
    {
        // Replace absolute pointer addresses in the output by relative offsets.
        ASSERT(Level >= 1 && Level <= 2);
        MarshallDownStructure(pFormAligned, pFormInfoMarshalling[Level]->pInfo, pFormInfoMarshalling[Level]->cbStructureSize, TRUE);
    }
    else
    {
        dwErrorCode = GetLastError();
    }

    RpcRevertToSelf();
    UndoAlignRpcPtr(pForm, pFormAligned, cbBuf, pcbNeeded);

    return dwErrorCode;
}

DWORD
_RpcSetForm(WINSPOOL_PRINTER_HANDLE hPrinter, WCHAR* pFormName, WINSPOOL_FORM_CONTAINER* pFormInfoContainer)
{
    DWORD dwErrorCode;

    dwErrorCode = RpcImpersonateClient(NULL);
    if (dwErrorCode != ERROR_SUCCESS)
    {
        ERR("RpcImpersonateClient failed with error %lu!\n", dwErrorCode);
        return dwErrorCode;
    }

    if (!SetFormW(hPrinter, pFormName, pFormInfoContainer->Level, (PBYTE)pFormInfoContainer->FormInfo.pFormInfo1))
        dwErrorCode = GetLastError();

    RpcRevertToSelf();
    return dwErrorCode;
}
