/*
 * PROJECT:     ReactOS browseui
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Custom MRU AutoComplete List
 * COPYRIGHT:   Copyright 2017 Mark Jansen (mark.jansen@reactos.org)
 */

#include "precomp.h"

CACLCustomMRU::CACLCustomMRU()
    :m_bDirty(false)
{
}

CACLCustomMRU::~CACLCustomMRU()
{
    PersistMRU();
    m_Key.Close();
}

void CACLCustomMRU::PersistMRU()
{
    WCHAR Key[2] = { 0, 0 };

    if (!m_bDirty)
        return;
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

// *** IACLCustomMRU methods ***
HRESULT STDMETHODCALLTYPE CACLCustomMRU::Initialize(LPCWSTR pwszMRURegKey, DWORD dwMax)
{
    LSTATUS Status = m_Key.Create(HKEY_CURRENT_USER, pwszMRURegKey);
    if (Status != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(Status);

    m_MRUData.RemoveAll();
    dwMax = max(0, dwMax);
    dwMax = min(29, dwMax);
    while (dwMax--)
        m_MRUData.Add(CStringW());

    WCHAR MRUList[40];
    ULONG nChars = _countof(MRUList);

    Status = m_Key.QueryStringValue(L"MRUList", MRUList, &nChars);
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

