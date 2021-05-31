/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Functions to read RunOnceEx registry.
 * COPYRIGHT:   Copyright 2021 He Yang <1160386205@qq.com>
 */


#include <cassert>
#include <cstdlib>
#include <windows.h>
#include <winreg.h>

#include "registry.h"

LONG CRegKeyEx::EnumValueName(
    _In_ DWORD iIndex,
    _Out_ LPTSTR pszName,
    _Inout_ LPDWORD pnNameLength)
{
    return RegEnumValueW(m_hKey, iIndex, pszName, pnNameLength,
                         NULL, NULL, NULL, NULL);
}

RunOnceExEntry::RunOnceExEntry(
    _In_ const ATL::CStringW &Name,
    _In_ const ATL::CStringW &Value) :
    m_Name(Name), m_Value(Value)
{ ; }

int RunOnceExEntryCmp(
    _In_ const void *a,
    _In_ const void *b)
{
    return lstrcmpW(((RunOnceExEntry *)a)->m_Name,
                    ((RunOnceExEntry *)b)->m_Name);
}

BOOL RunOnceExSection::HandleValue(
    _In_ CRegKeyEx &hKey,
    _In_ const CStringW &ValueName)
{
    DWORD dwType;
    DWORD cbData;

    // Query data size
    if (hKey.QueryValue(ValueName, &dwType, NULL, &cbData) != ERROR_SUCCESS)
        return FALSE;

    // Validate its format and size.
    if (dwType != REG_SZ)
        return TRUE;

    if (cbData % sizeof(WCHAR) != 0)
        return FALSE;

    CStringW Buffer;
    LPWSTR szBuffer = Buffer.GetBuffer((cbData / sizeof(WCHAR)) + 1);

    if (hKey.QueryValue(ValueName, &dwType, szBuffer, &cbData) != ERROR_SUCCESS)
    {
        Buffer.ReleaseBuffer();
        return FALSE;
    }
    szBuffer[cbData / sizeof(WCHAR)] = L'\0';
    Buffer.ReleaseBuffer();

    if (ValueName.IsEmpty())
    {
        // this is the default value
        m_SectionTitle = Buffer;
    }
    else
    {
        m_EntryList.Add(RunOnceExEntry(ValueName, Buffer));
    }

    return TRUE;
}

RunOnceExSection::RunOnceExSection(
    _In_ CRegKeyEx &hParentKey,
    _In_ const CStringW &lpSubKeyName) :
    m_SectionName(lpSubKeyName)
{
    m_bSuccess = FALSE;
    CRegKeyEx hKey;
    DWORD dwValueNum;
    DWORD dwMaxValueNameLen;
    LSTATUS Error;
    CStringW ValueName;

    if (hKey.Open(hParentKey, lpSubKeyName, KEY_READ) != ERROR_SUCCESS)
        return;

    Error = RegQueryInfoKeyW(hKey, NULL, 0, NULL, NULL, NULL, NULL,
                             &dwValueNum, &dwMaxValueNameLen,
                             NULL, NULL, NULL);
    if (Error != ERROR_SUCCESS)
        return;

    for (DWORD i = 0; i < dwValueNum; i++)
    {
        LPWSTR szValueName;
        DWORD dwcchName = dwMaxValueNameLen + 1;

        szValueName = ValueName.GetBuffer(dwMaxValueNameLen + 1);
        Error = hKey.EnumValueName(i, szValueName, &dwcchName);
        ValueName.ReleaseBuffer();

        if (Error != ERROR_SUCCESS)
        {
            // TODO: error handling
            return;
        }

        if (!HandleValue(hKey, ValueName))
            return;
    }

    // sort entries by name in string order.
    qsort(m_EntryList.GetData(), m_EntryList.GetSize(), sizeof(RunOnceExEntry),
          RunOnceExEntryCmp);

    hKey.Close();
    m_bSuccess = TRUE;
    return;
}

int RunOnceExSectionCmp(_In_ const void *a, _In_ const void *b)
{
    return lstrcmpW(((RunOnceExSection *)a)->m_SectionName,
                    ((RunOnceExSection *)b)->m_SectionName);
}

RunOnceExInstance::RunOnceExInstance(_In_ HKEY BaseKey)
{
    m_bSuccess = FALSE;
    CRegKeyEx hKey;
    DWORD dwSubKeyNum;
    DWORD dwMaxSubKeyNameLen;
    LSTATUS Error;
    CStringW SubKeyName;

    if (hKey.Open(BaseKey,
                  L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx\\",
                  KEY_READ) != ERROR_SUCCESS)
    {
        return;
    }

    Error = RegQueryInfoKeyW(hKey, NULL, 0, NULL,
                             &dwSubKeyNum, &dwMaxSubKeyNameLen,
                             NULL, NULL, NULL, NULL, NULL, NULL);
    if (Error != ERROR_SUCCESS)
        return;

    for (DWORD i = 0; i < dwSubKeyNum; i++)
    {
        LPWSTR szSubKeyName;
        DWORD dwcchName = dwMaxSubKeyNameLen + 1;

        szSubKeyName = SubKeyName.GetBuffer(dwMaxSubKeyNameLen + 1);
        Error = hKey.EnumKey(i, szSubKeyName, &dwcchName);
        SubKeyName.ReleaseBuffer();

        if (Error != ERROR_SUCCESS)
        {
            // TODO: error handling
            return;
        }

        if (!HandleSubKey(hKey, SubKeyName))
            return;
    }

    // sort sections by name in string order.
    qsort(m_SectionList.GetData(), m_SectionList.GetSize(), sizeof(RunOnceExSection),
          RunOnceExSectionCmp);

    hKey.Close();
    m_bSuccess = TRUE;
    return;
}

BOOL RunOnceExInstance::HandleSubKey(
    _In_ CRegKeyEx &hKey,
    _In_ const CStringW& SubKeyName)
{
    RunOnceExSection RunOnceExSection(hKey, SubKeyName);
    if (!RunOnceExSection.m_bSuccess)
    {
        return FALSE;
    }

    m_SectionList.Add(RunOnceExSection);
    return TRUE;
}
