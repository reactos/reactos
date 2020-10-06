/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Custom MRU AutoComplete List
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 *              Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "precomp.h"

#define TYPED_URLS_KEY L"Software\\Microsoft\\Internet Explorer\\TypedURLs"

CACLCustomMRU::CACLCustomMRU()
    : m_bDirty(false), m_bTypedURLs(FALSE), m_ielt(0)
{
}

CACLCustomMRU::~CACLCustomMRU()
{
    PersistMRU();
}

STDMETHODIMP CACLCustomMRU::Next(ULONG celt, LPWSTR *rgelt, ULONG *pceltFetched)
{
    if (!pceltFetched || !rgelt)
        return E_POINTER;

    *pceltFetched = 0;
    if (celt == 0)
        return S_OK;

    *rgelt = NULL;
    if (INT(m_ielt) >= m_MRUData.GetSize())
        return S_FALSE;

    CStringW str = m_MRUData[m_ielt];

    if (!m_bTypedURLs)
    {
        // Erase the last "\\1" etc. (indicates SW_* value)
        INT ich = str.ReverseFind(L'\\');
        if (ich >= 0)
            str = str.Left(ich);
    }

    size_t cb = (str.GetLength() + 1) * sizeof(WCHAR);
    LPWSTR psz = (LPWSTR)CoTaskMemAlloc(cb);
    if (!psz)
        return S_FALSE;

    CopyMemory(psz, (LPCWSTR)str, cb);
    *rgelt = psz;
    *pceltFetched = 1;
    ++m_ielt;
    return S_OK;
}

STDMETHODIMP CACLCustomMRU::Skip(ULONG celt)
{
    return E_NOTIMPL;
}

STDMETHODIMP CACLCustomMRU::Reset()
{
    m_ielt = 0;
    return S_OK;
}

STDMETHODIMP CACLCustomMRU::Clone(IEnumString ** ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CACLCustomMRU::Expand(LPCOLESTR pszExpand)
{
    return E_NOTIMPL;
}

void CACLCustomMRU::PersistMRU()
{
    if (!m_bDirty || m_bTypedURLs)
        return;

    WCHAR Key[2] = { 0, 0 };

    m_bDirty = false;

    if (m_Key.m_hKey)
    {
        m_Key.SetStringValue(L"MRUList", m_MRUList);
        for (int Index = 0; Index < m_MRUList.GetLength(); ++Index)
        {
            Key[0] = Index + 'a';
            m_Key.SetStringValue(Key, m_MRUData[Index]);
        }
    }
}

static LSTATUS
RegQueryCStringW(CRegKey& key, LPCWSTR pszValueName, CStringW& str)
{
    // Check type and size
    DWORD dwType, cbData;
    LSTATUS ret = key.QueryValue(pszValueName, &dwType, NULL, &cbData);
    if (ret != ERROR_SUCCESS)
        return ret;
    if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
        return ERROR_INVALID_DATA;

    // Allocate buffer
    LPWSTR pszBuffer = str.GetBuffer(cbData / sizeof(WCHAR) + 1);
    if (pszBuffer == NULL)
        return ERROR_OUTOFMEMORY;

    // Get the data
    ret = key.QueryValue(pszValueName, NULL, pszBuffer, &cbData);

    // Release buffer
    str.ReleaseBuffer();
    return ret;
}

HRESULT CACLCustomMRU::LoadTypedURLs(DWORD dwMax)
{
    dwMax = max(0, dwMax);
    dwMax = min(29, dwMax);

    WCHAR szName[32];
    CStringW strData;
    LSTATUS status;
    for (DWORD i = 1; i <= dwMax; ++i)
    {
        // Build a registry value name
        StringCbPrintfW(szName, sizeof(szName), L"url%lu", i);

        // Read a registry value
        status = RegQueryCStringW(m_Key, szName, strData);
        if (status != ERROR_SUCCESS)
            break;

        m_MRUData.Add(strData);
    }

    return S_OK;
}

// *** IACLCustomMRU methods ***
HRESULT STDMETHODCALLTYPE CACLCustomMRU::Initialize(LPCWSTR pwszMRURegKey, DWORD dwMax)
{
    m_ielt = 0;

    LSTATUS Status = m_Key.Create(HKEY_CURRENT_USER, pwszMRURegKey);
    if (Status != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(Status);

    m_MRUData.RemoveAll();
    if (lstrcmpiW(pwszMRURegKey, TYPED_URLS_KEY) == 0)
    {
        m_bTypedURLs = TRUE;
        return LoadTypedURLs(dwMax);
    }
    else
    {
        m_bTypedURLs = FALSE;
        return LoadMRUList(dwMax);
    }
}

HRESULT CACLCustomMRU::LoadMRUList(DWORD dwMax)
{
    dwMax = max(0, dwMax);
    dwMax = min(29, dwMax);
    while (dwMax--)
        m_MRUData.Add(CStringW());

    WCHAR MRUList[40];
    ULONG nChars = _countof(MRUList);

    LSTATUS Status = m_Key.QueryStringValue(L"MRUList", MRUList, &nChars);
    if (Status != ERROR_SUCCESS)
        return S_OK;

    if (nChars > 0 && MRUList[nChars-1] == '\0')
        nChars--;

    if (nChars > (ULONG)m_MRUData.GetSize())
        return S_OK;

    for (ULONG n = 0; n < nChars; ++n)
    {
        if (MRUList[n] >= 'a' && MRUList[n] <= '}' && m_MRUList.Find(MRUList[n]) < 0)
        {
            WCHAR Key[2] = { MRUList[n], NULL };
            WCHAR Value[MAX_PATH * 2];
            ULONG nValueChars = _countof(Value);

            m_MRUList += MRUList[n];
            int Index = MRUList[n] - 'a';

            if (Index < m_MRUData.GetSize())
            {
                Status = m_Key.QueryStringValue(Key, Value, &nValueChars);
                if (Status == ERROR_SUCCESS)
                {
                    m_MRUData[Index] = CStringW(Value, nValueChars);
                }
            }
        }
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CACLCustomMRU::AddMRUString(LPCWSTR pwszEntry)
{
    if (m_bTypedURLs)
        return E_FAIL;

    ATLASSERT(m_MRUData.GetSize() <= m_MRUList.GetLength());
    m_bDirty = true;

    CStringW NewElement = pwszEntry;
    WCHAR Key[2] = { 0, 0 };
    int Index = m_MRUData.Find(NewElement);
    if (Index >= 0)
    {
        /* Move the key to the front */
        Key[0] = Index + 'a';
        m_MRUList.Replace(Key, L"");
        m_MRUList = Key + m_MRUList;
        return S_OK;
    }

    int TotalLen = m_MRUList.GetLength();
    if (m_MRUData.GetSize() == TotalLen)
    {
        /* Find oldest element, move that to the front */
        Key[0] = m_MRUList[TotalLen-1];
        m_MRUList = Key + m_MRUList.Left(TotalLen-1);
        Index = Key[0] - 'a';
    }
    else
    {
        /* Find the first empty entry */
        for (Index = 0; Index < m_MRUData.GetSize(); ++Index)
        {
            if (m_MRUData[Index].IsEmpty())
                break;
        }
        Key[0] = Index + 'a';
        m_MRUList = Key + m_MRUList;
    }
    m_MRUData[Index] = NewElement;

    PersistMRU();
    return S_OK;
}

