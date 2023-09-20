/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Utility functions
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/*************************************************************************
 *                SHGetShellStyleHInstance (SHELL32.749)
 */
EXTERN_C HINSTANCE
WINAPI
SHGetShellStyleHInstance(VOID)
{
    HINSTANCE hInst = NULL;
    WCHAR szPath[MAX_PATH], szColorName[100];
    HRESULT hr;
    CStringW strShellStyle;

    TRACE("\n");

    /* First, attempt to load the shellstyle dll from the current active theme */
    hr = GetCurrentThemeName(szPath, _countof(szPath), szColorName, _countof(szColorName), NULL, 0);
    if (FAILED(hr))
        goto DoDefault;

    /* Strip the theme filename */
    PathRemoveFileSpecW(szPath);

    strShellStyle = szPath;
    strShellStyle += L"\\Shell\\";
    strShellStyle += szColorName;
    strShellStyle += L"\\ShellStyle.dll";

    hInst = LoadLibraryExW(strShellStyle, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (hInst)
        return hInst;

    /* Otherwise, use the version stored in the System32 directory */
DoDefault:
    if (!ExpandEnvironmentStringsW(L"%SystemRoot%\\System32\\ShellStyle.dll",
                                   szPath, _countof(szPath)))
    {
        ERR("Expand failed\n");
        return NULL;
    }
    return LoadLibraryExW(szPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
}

/*************************************************************************
 *                SHCreatePropertyBag (SHELL32.715)
 */
EXTERN_C HRESULT
WINAPI
SHCreatePropertyBag(_In_ REFIID riid, _Out_ void **ppvObj)
{
    return SHCreatePropertyBagOnMemory(STGM_READWRITE, riid, ppvObj);
}

/*************************************************************************
 *                SheRemoveQuotesA (SHELL32.@)
 */
EXTERN_C LPSTR
WINAPI
SheRemoveQuotesA(LPSTR psz)
{
    PCHAR pch;

    if (*psz == '"')
    {
        for (pch = psz + 1; *pch && *pch != '"'; ++pch)
        {
            *(pch - 1) = *pch;
        }

        if (*pch == '"')
            *(pch - 1) = ANSI_NULL;
    }

    return psz;
}

/*************************************************************************
 *                SheRemoveQuotesW (SHELL32.@)
 *
 * ExtractAssociatedIconExW uses this function.
 */
EXTERN_C LPWSTR
WINAPI
SheRemoveQuotesW(LPWSTR psz)
{
    PWCHAR pch;

    if (*psz == L'"')
    {
        for (pch = psz + 1; *pch && *pch != L'"'; ++pch)
        {
            *(pch - 1) = *pch;
        }

        if (*pch == L'"')
            *(pch - 1) = UNICODE_NULL;
    }

    return psz;
}

/*************************************************************************
 *  SHFindComputer [SHELL32.91]
 *
 * Invokes the shell search in My Computer. Used in SHFindFiles.
 * Two parameters are ignored.
 */
EXTERN_C BOOL
WINAPI
SHFindComputer(LPCITEMIDLIST pidlRoot, LPCITEMIDLIST pidlSavedSearch)
{
    UNREFERENCED_PARAMETER(pidlRoot);
    UNREFERENCED_PARAMETER(pidlSavedSearch);

    TRACE("%p %p\n", pidlRoot, pidlSavedSearch);

    IContextMenu *pCM;
    HRESULT hr = CoCreateInstance(CLSID_ShellSearchExt, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IContextMenu, (void **)&pCM);
    if (FAILED(hr))
    {
        ERR("0x%08X\n", hr);
        return hr;
    }

    CMINVOKECOMMANDINFO InvokeInfo = { sizeof(InvokeInfo) };
    InvokeInfo.lpParameters = "{996E1EB1-B524-11D1-9120-00A0C98BA67D}";
    InvokeInfo.nShow = SW_SHOWNORMAL;
    hr = pCM->InvokeCommand(&InvokeInfo);
    pCM->Release();

    return SUCCEEDED(hr);
}
