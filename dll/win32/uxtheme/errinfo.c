/*
 * PROJECT:     ReactOS uxtheme.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     UXTHEME error reporting helpers
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "uxthemep.h"
#include <stdlib.h>
#include <strsafe.h>

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
    LPCWSTR args[2] = { pErrInfo->szParam1, pErrInfo->szParam2 };

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
    HRESULT hr;

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
    hr = (ret ? S_OK : UXTHEME_MakeLastError());
    FreeLibrary(hUxTheme);

    return hr;
}

// Parser should use this function on failure
HRESULT
UXTHEME_MakeParseError(
    _In_ UINT nID,
    _In_ LPCWSTR pszParam1,
    _In_ LPCWSTR pszParam2,
    _In_ LPCWSTR pszFile,
    _In_ LPCWSTR pszLine,
    _In_ INT nLineNo)
{
    PTMERRINFO pErrInfo = UXTHEME_GetParseErrorInfo(TRUE);
    if (pErrInfo)
    {
        pErrInfo->nID = nID;
        pErrInfo->nLineNo = nLineNo;
        StringCchCopyW(pErrInfo->szParam1, _countof(pErrInfo->szParam1), pszParam1);
        StringCchCopyW(pErrInfo->szParam2, _countof(pErrInfo->szParam2), pszParam2);
        StringCchCopyW(pErrInfo->szFile, _countof(pErrInfo->szFile), pszFile);
        StringCchCopyW(pErrInfo->szLine, _countof(pErrInfo->szLine), pszLine);
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

    hr = UXTHEME_FormatParseMessage(pErrInfo, pInfo->szDescription, _countof(pInfo->szDescription));
    if (FAILED(hr))
        return hr;

    pInfo->nID = pErrInfo->nID;
    pInfo->nLineNo = pErrInfo->nLineNo;
    StringCchCopyW(pInfo->szFile, _countof(pInfo->szFile), pErrInfo->szFile);
    StringCchCopyW(pInfo->szLine, _countof(pInfo->szLine), pErrInfo->szLine);
    return hr;
}
