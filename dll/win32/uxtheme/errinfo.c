/*
 * PROJECT:     ReactOS uxtheme.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Error information of UXTHEME
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "uxthemep.h"
#include <stdlib.h>
#include <strsafe.h>

HRESULT
UXTHEME_MakeError32(_In_ LONG error)
{
    if (error < 0)
        return (HRESULT)error;
    return HRESULT_FROM_WIN32(error);
}

HRESULT
UXTHEME_MakeLastError(VOID)
{
    return UXTHEME_MakeError32(GetLastError());
}

PTMERRINFO
UXTHEME_GetParseErrorInfo(_In_ BOOL bCreate)
{
    PTMERRINFO pErrInfo;

    if (gdwErrorInfoTlsIndex == TLS_OUT_OF_INDEXES)
        return NULL;

    pErrInfo = TlsGetValue(gdwErrorInfoTlsIndex);
    if (pErrInfo)
        return pErrInfo;

    if (bCreate)
    {
        pErrInfo = LocalAlloc(LPTR, sizeof(*pErrInfo));
        TlsSetValue(gdwErrorInfoTlsIndex, pErrInfo);
    }

    return pErrInfo;
}

VOID
UXTHEME_DeleteParseErrorInfo(VOID)
{
    PTMERRINFO pErrInfo = UXTHEME_GetParseErrorInfo(FALSE);
    if (!pErrInfo)
        return;

    TlsSetValue(gdwErrorInfoTlsIndex, NULL);
    LocalFree(pErrInfo);
}

static BOOL
UXTHEME_FormatLocalMsg(
    _In_ HINSTANCE hInstance,
    _In_ UINT uID,
    _Out_ LPWSTR pszDest,
    _In_ SIZE_T cchDest,
    _In_ LPCWSTR pszDrive,
    _In_ PTMERRINFO pErrInfo)
{
    WCHAR szFormat[MAX_PATH];
    LPCWSTR args[2] = { pErrInfo->szPath0, pErrInfo->szPath1 };

    if (!LoadStringW(hInstance, uID, szFormat, _countof(szFormat)) || !szFormat[0])
        return FALSE;

    return FormatMessageW(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          szFormat, 0, 0, pszDest, cchDest, (va_list *)args) != 0;
}

static HRESULT
UXTHEME_FormatParseMessage(
    _In_ PTMERRINFO pErrInfo,
    _Out_ LPWSTR pszDest,
    _In_ SIZE_T cchDest)
{
    DWORD nID;
    HMODULE hMod, hUxTheme;
    WCHAR szFullPath[_MAX_PATH];
    WCHAR szDrive[_MAX_DRIVE + 1], szDir[_MAX_DIR], szFileName[_MAX_FNAME], szExt[_MAX_EXT];
    BOOL ret;

    nID = LOWORD(pErrInfo->nID);
    if (!GetModuleFileNameW(NULL, szFullPath, _countof(szFullPath)))
        return S_OK;

    _wsplitpath(szFullPath, szDrive, szDir, szFileName, szExt);
    if (lstrcmpiW(szFileName, L"packthem") == 0)
    {
        hMod = GetModuleHandleW(NULL);
        if (UXTHEME_FormatLocalMsg(hMod, nID, pszDest, cchDest, szDrive, pErrInfo))
            return S_OK;
    }

    hUxTheme = LoadLibraryW(L"uxtheme.dll");
    if (!hUxTheme)
        return E_FAIL;
    ret = UXTHEME_FormatLocalMsg(hUxTheme, nID, pszDest, cchDest, szDrive, pErrInfo);
    FreeLibrary(hUxTheme);

    return ret ? S_OK : UXTHEME_MakeLastError();
}

// Parser should use this function on failure
HRESULT
UXTHEME_MakeParseError(
    _In_ UINT nID,
    _In_ LPCWSTR pszPath0,
    _In_ LPCWSTR pszPath1,
    _In_ LPCWSTR pszPath2,
    _In_ LPCWSTR pszPath3,
    _In_ DWORD dwError)
{
    PTMERRINFO pErrInfo = UXTHEME_GetParseErrorInfo(TRUE);
    if (pErrInfo)
    {
        pErrInfo->nID = nID;
        pErrInfo->dwError = dwError;
        StringCchCopyW(pErrInfo->szPath0, _countof(pErrInfo->szPath0), pszPath0);
        StringCchCopyW(pErrInfo->szPath1, _countof(pErrInfo->szPath1), pszPath1);
        StringCchCopyW(pErrInfo->szPath2, _countof(pErrInfo->szPath2), pszPath2);
        StringCchCopyW(pErrInfo->szPath3, _countof(pErrInfo->szPath3), pszPath3);
    }
    return HRESULT_FROM_WIN32(ERROR_UNKNOWN_PROPERTY);
}

/*************************************************************************
 *  GetThemeParseErrorInfo (UXTHEME.48)
 */
HRESULT WINAPI
GetThemeParseErrorInfo(_Inout_ PPARSE_ERROR_INFO pInfo)
{
    PTMERRINFO pErrInfo;
    HRESULT hr;

    if (!pInfo)
        return E_POINTER;

    if (pInfo->cbSize != sizeof(*pInfo))
        return E_INVALIDARG;

    pErrInfo = UXTHEME_GetParseErrorInfo(TRUE);
    if (!pErrInfo)
        return E_OUTOFMEMORY;

    hr = UXTHEME_FormatParseMessage(pErrInfo, pInfo->ErrInfo.szPath0,
                                    _countof(pInfo->ErrInfo.szPath0) +
                                    _countof(pInfo->ErrInfo.szPath1));
    if (FAILED(hr))
        return hr;

    pInfo->ErrInfo.nID = pErrInfo->nID;
    pInfo->ErrInfo.dwError = pErrInfo->dwError;
    StringCchCopyW(pInfo->ErrInfo.szPath2, _countof(pInfo->ErrInfo.szPath2), pErrInfo->szPath2);
    StringCchCopyW(pInfo->ErrInfo.szPath3, _countof(pInfo->ErrInfo.szPath3), pErrInfo->szPath3);
    return hr;
}
