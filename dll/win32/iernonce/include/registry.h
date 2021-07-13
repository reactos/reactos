/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Functions to read RunOnceEx registry.
 * COPYRIGHT:   Copyright 2021 He Yang <1160386205@qq.com>
 */

#pragma once

#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <atlcoll.h>
#include <atlsimpcoll.h>


class CRegKeyEx : public CRegKey
{
public:
    LONG EnumValueName(
        _In_ DWORD iIndex,
        _Out_ LPTSTR pszName,
        _Inout_ LPDWORD pnNameLength);
};

class RunOnceExEntry
{
private:
    ATL::CStringW m_Name;
    ATL::CStringW m_Value;

public:
    RunOnceExEntry(
        _In_ const ATL::CStringW &Name,
        _In_ const ATL::CStringW &Value);

    friend int RunOnceExEntryCmp(_In_ const void *a, _In_ const void *b);
};

class RunOnceExSection
{
private:
    ATL::CStringW m_SectionName;
    CSimpleArray<RunOnceExEntry> m_EntryList;

    BOOL HandleValue(
        _In_ CRegKeyEx &hKey,
        _In_ const CStringW &ValueName);

public:
    BOOL m_bSuccess;
    ATL::CStringW m_SectionTitle;
    RunOnceExSection(
        _In_ CRegKeyEx &hParentKey,
        _In_ const CStringW &lpSubKeyName);

    friend int RunOnceExSectionCmp(_In_ const void *a, _In_ const void *b);
};

class RunOnceExInstance
{
private:
    CSimpleArray<RunOnceExSection> m_SectionList;

    BOOL HandleSubKey(
        _In_ CRegKeyEx &hKey,
        _In_ const CStringW &SubKeyName);

public:
    BOOL m_bSuccess;
    RunOnceExInstance(_In_ HKEY BaseKey);
};
