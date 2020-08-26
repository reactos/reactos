/*
 * PROJECT:     ReactOS Spooler API
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to Forms
 * COPYRIGHT:   Copyright 2017 Colin Finck (colin@reactos.org)
 */

#include "precomp.h"
#include <marshalling/forms.h>

BOOL WINAPI
AddFormA(HANDLE hPrinter, DWORD Level, PBYTE pForm)
{
    FORM_INFO_2W   pfi2W;
    PFORM_INFO_2A  pfi2A;
    DWORD   len;
    BOOL    res;

    pfi2A = (PFORM_INFO_2A)pForm;

    TRACE("AddFormA(%p, %lu, %p)\n", hPrinter, Level, pForm);

    if ((Level < 1) || (Level > 2))
    {
        ERR("Level = %d, unsupported!\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!pfi2A)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ZeroMemory(&pfi2W, sizeof(FORM_INFO_2W));

    if (pfi2A->pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pfi2A->pName, -1, NULL, 0);
        pfi2W.pName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pfi2A->pName, -1, pfi2W.pName, len);
    }

    pfi2W.Flags         = pfi2A->Flags;
    pfi2W.Size          = pfi2A->Size;
    pfi2W.ImageableArea = pfi2A->ImageableArea;

    if (Level > 1)
    {
        if (pfi2A->pKeyword)
        {
            len = MultiByteToWideChar(CP_ACP, 0, pfi2A->pKeyword, -1, NULL, 0);
            pfi2W.pKeyword = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pfi2A->pKeyword, -1, (LPWSTR)pfi2W.pKeyword, len);
        }

        if (pfi2A->pMuiDll)
        {
            len = MultiByteToWideChar(CP_ACP, 0, pfi2A->pMuiDll, -1, NULL, 0);
            pfi2W.pMuiDll = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pfi2A->pMuiDll, -1, (LPWSTR)pfi2W.pMuiDll, len);
        }

        if (pfi2A->pDisplayName)
        {
            len = MultiByteToWideChar(CP_ACP, 0, pfi2A->pDisplayName, -1, NULL, 0);
            pfi2W.pDisplayName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pfi2A->pDisplayName, -1, (LPWSTR)pfi2W.pDisplayName, len);
        }
        pfi2W.StringType   = pfi2A->StringType;
        pfi2W.dwResourceId = pfi2A->dwResourceId;
        pfi2W.wLangId      = pfi2A->wLangId;
    }

    res = AddFormW( hPrinter, Level, (PBYTE)&pfi2W );

    if (pfi2W.pName) HeapFree(GetProcessHeap(), 0, pfi2W.pName);
    if (pfi2W.pKeyword) HeapFree(GetProcessHeap(), 0, (LPWSTR)pfi2W.pKeyword);
    if (pfi2W.pMuiDll) HeapFree(GetProcessHeap(), 0, (LPWSTR)pfi2W.pMuiDll);
    if (pfi2W.pDisplayName) HeapFree(GetProcessHeap(), 0, (LPWSTR)pfi2W.pDisplayName);

    return res;
}

BOOL WINAPI
AddFormW(HANDLE hPrinter, DWORD Level, PBYTE pForm)
{
    DWORD dwErrorCode;
    WINSPOOL_FORM_CONTAINER FormInfoContainer;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("AddFormW(%p, %lu, %p)\n", hPrinter, Level, pForm);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    if ((Level < 1) || (Level > 2))
    {
        ERR("Level = %d, unsupported!\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    FormInfoContainer.FormInfo.pFormInfo1 = (WINSPOOL_FORM_INFO_1*)pForm;
    FormInfoContainer.Level = Level;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcAddForm(pHandle->hPrinter, &FormInfoContainer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcAddForm failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
DeleteFormA(HANDLE hPrinter, PSTR pFormName)
{
    UNICODE_STRING FormNameW;
    BOOL Ret;

    TRACE("DeleteFormA(%p, %s)\n", hPrinter, pFormName);

    AsciiToUnicode(&FormNameW, pFormName);

    Ret = DeleteFormW( hPrinter, FormNameW.Buffer );

    RtlFreeUnicodeString(&FormNameW);

    return Ret;
}

BOOL WINAPI
DeleteFormW(HANDLE hPrinter, PWSTR pFormName)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("DeleteFormW(%p, %S)\n", hPrinter, pFormName);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcDeleteForm(pHandle->hPrinter, pFormName);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcDeleteForm failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
EnumFormsA(HANDLE hPrinter, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode, i;
    PFORM_INFO_1W pfi1w = (PFORM_INFO_1W)pForm;
    PFORM_INFO_2W pfi2w = (PFORM_INFO_2W)pForm;

    TRACE("EnumFormsA(%p, %lu, %p, %lu, %p, %p)\n", hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned);

    if ( EnumFormsW( hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned ) )
    {
        for ( i = 0; i < *pcReturned; i++ )
        {
            switch ( Level )
            {
                case 2:
                    dwErrorCode = UnicodeToAnsiInPlace((LPWSTR)pfi2w[i].pKeyword);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace((LPWSTR)pfi2w[i].pMuiDll);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace((LPWSTR)pfi2w[i].pDisplayName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    dwErrorCode = UnicodeToAnsiInPlace(pfi2w[i].pName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
                    break;
                case 1:
                    dwErrorCode = UnicodeToAnsiInPlace(pfi1w[i].pName);
                    if (dwErrorCode != ERROR_SUCCESS)
                    {
                        goto Cleanup;
                    }
            }
        }
        return TRUE;
    }
Cleanup:
    return FALSE;
}

BOOL WINAPI
EnumFormsW(HANDLE hPrinter, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded, PDWORD pcReturned)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("EnumFormsW(%p, %lu, %p, %lu, %p, %p)\n", hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    if ((Level < 1) || (Level > 2))
    {
        ERR("Level = %d, unsupported!\n", Level);
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcEnumForms(pHandle->hPrinter, Level, pForm, cbBuf, pcbNeeded, pcReturned);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcEnumForms failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level >= 1 && Level <= 2);
        MarshallUpStructuresArray(cbBuf, pForm, *pcReturned, pFormInfoMarshalling[Level]->pInfo, pFormInfoMarshalling[Level]->cbStructureSize, TRUE);
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetFormA(HANDLE hPrinter, PSTR pFormName, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode, len;
    LPWSTR FormNameW = NULL;
    PFORM_INFO_1W pfi1w = (PFORM_INFO_1W)pForm;
    PFORM_INFO_2W pfi2w = (PFORM_INFO_2W)pForm;

    TRACE("GetFormA(%p, %s, %lu, %p, %lu, %p)\n", hPrinter, pFormName, Level, pForm, cbBuf, pcbNeeded);

    if (pFormName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pFormName, -1, NULL, 0);
        FormNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pFormName, -1, FormNameW, len);
    }

    if ( GetFormW( hPrinter, FormNameW, Level, pForm, cbBuf, pcbNeeded ) )
    {
        switch ( Level )
        {
            case 2:
                dwErrorCode = UnicodeToAnsiInPlace((LPWSTR)pfi2w->pKeyword);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace((LPWSTR)pfi2w->pMuiDll);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace((LPWSTR)pfi2w->pDisplayName);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                dwErrorCode = UnicodeToAnsiInPlace(pfi2w->pName);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
                break;
            case 1:
                dwErrorCode = UnicodeToAnsiInPlace(pfi1w->pName);
                if (dwErrorCode != ERROR_SUCCESS)
                {
                    goto Cleanup;
                }
        }
    }
Cleanup:
    if (FormNameW) HeapFree(GetProcessHeap(), 0, FormNameW);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
GetFormW(HANDLE hPrinter, PWSTR pFormName, DWORD Level, PBYTE pForm, DWORD cbBuf, PDWORD pcbNeeded)
{
    DWORD dwErrorCode;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("GetFormW(%p, %S, %lu, %p, %lu, %p)\n", hPrinter, pFormName, Level, pForm, cbBuf, pcbNeeded);

    // Sanity checks.
    if (!pHandle)
    {
        dwErrorCode = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    // Dismiss invalid levels already at this point.
    if ((Level < 1) || (Level > 2))
    {
        ERR("Level = %d, unsupported!\n", Level);
        dwErrorCode = ERROR_INVALID_LEVEL;
        goto Cleanup;
    }

    if (cbBuf && pForm)
        ZeroMemory(pForm, cbBuf);

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcGetForm(pHandle->hPrinter, pFormName, Level, pForm, cbBuf, pcbNeeded);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
        ERR("_RpcGetForm failed with exception code %lu!\n", dwErrorCode);
    }
    RpcEndExcept;

    if (dwErrorCode == ERROR_SUCCESS)
    {
        // Replace relative offset addresses in the output by absolute pointers.
        ASSERT(Level >= 1 && Level <= 2);
        MarshallUpStructure(cbBuf, pForm, pFormInfoMarshalling[Level]->pInfo, pFormInfoMarshalling[Level]->cbStructureSize, TRUE);
    }

Cleanup:
    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}

BOOL WINAPI
SetFormA(HANDLE hPrinter, PSTR pFormName, DWORD Level, PBYTE pForm)
{
    FORM_INFO_2W   pfi2W;
    FORM_INFO_2A * pfi2A;
    LPWSTR FormNameW = NULL;
    DWORD len;
    BOOL res;

    pfi2A = (FORM_INFO_2A *) pForm;

    TRACE("SetFormA(%p, %s, %lu, %p)\n", hPrinter, pFormName, Level, pForm);

    if ((Level < 1) || (Level > 2))
    {
        ERR("Level = %d, unsupported!\n", Level);
        SetLastError(ERROR_INVALID_LEVEL);
        return FALSE;
    }

    if (!pfi2A)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (pFormName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pFormName, -1, NULL, 0);
        FormNameW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pFormName, -1, FormNameW, len);
    }

    ZeroMemory(&pfi2W, sizeof(FORM_INFO_2W));

    if (pfi2A->pName)
    {
        len = MultiByteToWideChar(CP_ACP, 0, pfi2A->pName, -1, NULL, 0);
        pfi2W.pName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, pfi2A->pName, -1, pfi2W.pName, len);
    }

    pfi2W.Flags         = pfi2A->Flags;
    pfi2W.Size          = pfi2A->Size;
    pfi2W.ImageableArea = pfi2A->ImageableArea;

    if (Level > 1)
    {
        if (pfi2A->pKeyword)
        {
            len = MultiByteToWideChar(CP_ACP, 0, pfi2A->pKeyword, -1, NULL, 0);
            pfi2W.pKeyword = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pfi2A->pKeyword, -1, (LPWSTR)pfi2W.pKeyword, len);
        }

        if (pfi2A->pMuiDll)
        {
            len = MultiByteToWideChar(CP_ACP, 0, pfi2A->pMuiDll, -1, NULL, 0);
            pfi2W.pMuiDll = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pfi2A->pMuiDll, -1, (LPWSTR)pfi2W.pMuiDll, len);
        }

        if (pfi2A->pDisplayName)
        {
            len = MultiByteToWideChar(CP_ACP, 0, pfi2A->pDisplayName, -1, NULL, 0);
            pfi2W.pDisplayName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pfi2A->pDisplayName, -1, (LPWSTR)pfi2W.pDisplayName, len);
        }
        pfi2W.StringType   = pfi2A->StringType;
        pfi2W.dwResourceId = pfi2A->dwResourceId;
        pfi2W.wLangId      = pfi2A->wLangId;
    }

    res = SetFormW( hPrinter, FormNameW, Level, (PBYTE)&pfi2W );

    if (FormNameW) HeapFree(GetProcessHeap(), 0, FormNameW);
    if (pfi2W.pName) HeapFree(GetProcessHeap(), 0, pfi2W.pName);
    if (pfi2W.pKeyword) HeapFree(GetProcessHeap(), 0, (LPWSTR)pfi2W.pKeyword);
    if (pfi2W.pMuiDll) HeapFree(GetProcessHeap(), 0, (LPWSTR)pfi2W.pMuiDll);
    if (pfi2W.pDisplayName) HeapFree(GetProcessHeap(), 0, (LPWSTR)pfi2W.pDisplayName);

    return res;
}

BOOL WINAPI
SetFormW(HANDLE hPrinter, PWSTR pFormName, DWORD Level, PBYTE pForm)
{
    DWORD dwErrorCode;
    WINSPOOL_FORM_CONTAINER FormInfoContainer;
    PSPOOLER_HANDLE pHandle = (PSPOOLER_HANDLE)hPrinter;

    TRACE("SetFormW(%p, %S, %lu, %p)\n", hPrinter, pFormName, Level, pForm);

    // Sanity checks.
    if (!pHandle)
    {
        ERR("Level = %d, unsupported!\n", Level);
        dwErrorCode = ERROR_INVALID_HANDLE;
        return FALSE;
    }

    FormInfoContainer.FormInfo.pFormInfo1 = (WINSPOOL_FORM_INFO_1*)pForm;
    FormInfoContainer.Level = Level;

    // Do the RPC call
    RpcTryExcept
    {
        dwErrorCode = _RpcSetForm(pHandle->hPrinter, pFormName, &FormInfoContainer);
    }
    RpcExcept(EXCEPTION_EXECUTE_HANDLER)
    {
        dwErrorCode = RpcExceptionCode();
    }
    RpcEndExcept;

    SetLastError(dwErrorCode);
    return (dwErrorCode == ERROR_SUCCESS);
}
