/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Realize CLSID_ACLHistory for auto-completion
 * COPYRIGHT:   Copyright 2021 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

CACLHistory::CACLHistory() : m_iItem(0)
{
    ReLoad();
}

CACLHistory::~CACLHistory()
{
}

HRESULT CACLHistory::ReLoad()
{
    HKEY hKey;
    LONG error;
    WCHAR szName[32], szValue[512];
    DWORD dwType, cbValue;

    m_array.RemoveAll();

    error = RegOpenKeyExW(HKEY_CURRENT_USER,
                          L"SOFTWARE\\Microsoft\\Internet Explorer\\TypedURLs",
                          0, KEY_READ, &hKey);
    if (error)
        return S_OK;

    for (INT iURL = 0; iURL < 64; ++iURL)
    {
        StringCbPrintfW(szName, sizeof(szName), L"url%u", (iURL + 1));
        szValue[0] = 0;
        cbValue = sizeof(szValue);
        error = RegQueryValueExW(hKey, szName, NULL, &dwType, (LPBYTE)szValue, &cbValue);
        if (error || szValue[0] == 0 || dwType != REG_SZ)
            break;
        m_array.Add(szValue);
    }

    RegCloseKey(hKey);
    return S_OK;
}

STDMETHODIMP CACLHistory::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    if (pceltFetched)
        *pceltFetched = 0;
    if (rgelt)
        *rgelt = NULL;
    if (celt != 1)
        return E_NOTIMPL;
    if (m_iItem >= m_array.GetSize())
        return S_FALSE;
    SHStrDupW(m_array[m_iItem], rgelt);
    ++m_iItem;
    if (!*rgelt)
        return E_OUTOFMEMORY;
    *pceltFetched = 1;
    return S_OK;
}

STDMETHODIMP CACLHistory::Reset()
{
    m_iItem = 0;
    return S_OK;
}

STDMETHODIMP CACLHistory::Skip(ULONG celt)
{
    return E_NOTIMPL;
}

STDMETHODIMP CACLHistory::Clone(IEnumString **ppenum)
{
    if (ppenum)
        *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CACLHistory::Expand(LPCWSTR pszExpand)
{
    return E_NOTIMPL;
}
