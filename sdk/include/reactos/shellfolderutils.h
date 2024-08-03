/*
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Utility functions for IShellFolder implementations
 * COPYRIGHT:   Copyright 2024 Whindmar Saksit <whindsaks@proton.me>
 */

#pragma once

#include "shellutils.h"

#ifdef __cplusplus

template <BOOL LOGICALCMP = TRUE>
static HRESULT ShellFolderImpl_CompareItemColumn(IShellFolder2 *psf, UINT column, PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2)
{
    SHELLDETAILS details1, details2;
    LPWSTR str1, str2;
    HRESULT hr;
    if (SUCCEEDED(hr = psf->GetDetailsOf(pidl1, column, &details1)) &&
        SUCCEEDED(hr = StrRetToStrW(&details1.str, pidl1, &str1)))
    {
        if (SUCCEEDED(hr = psf->GetDetailsOf(pidl2, column, &details2)) &&
            SUCCEEDED(hr = StrRetToStrW(&details2.str, pidl2, &str2)))
        {
            int res = LOGICALCMP ? StrCmpLogicalW(str1, str2) : lstrcmpiW(str1, str2);
            hr = MAKE_COMPARE_HRESULT(res);
            SHFree(str2);
        }
        SHFree(str1);
    }
    return hr;
}

template <UINT COLCOUNT, int CANONICAL, BOOL LOGICALCMP = TRUE>
static HRESULT ShellFolderImpl_CompareItemIDs(IShellFolder2 *psf, LPARAM lParam, PCUITEMID_CHILD pidl1, PCUITEMID_CHILD pidl2)
{
    HRESULT hr;
    if (CANONICAL >= 0 && (lParam & SHCIDS_CANONICALONLY))
    {
        hr = ShellFolderImpl_CompareItemColumn<LOGICALCMP>(psf, CANONICAL, pidl1, pidl2);
        if (hr == S_EQUAL || !(lParam & SHCIDS_ALLFIELDS) || FAILED(hr))
            return hr;
    }
    if (lParam & SHCIDS_ALLFIELDS)
    {
        for (UINT i = 0; i < COLCOUNT; ++i)
        {
            hr = ShellFolderImpl_CompareItemColumn<LOGICALCMP>(psf, i, pidl1, pidl2);
            if (hr && SUCCEEDED(hr)) // Only stop if we successfully found a difference
                break;
        }
        return hr;
    }
    const UINT column = (UINT)(lParam & SHCIDS_COLUMNMASK);
    return ShellFolderImpl_CompareItemColumn<LOGICALCMP>(psf, column, pidl1, pidl2);
}

#endif // __cplusplus
