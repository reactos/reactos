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

#define FLAGS_NO_STAT_DIALOG 0x00000080

#ifndef UNICODE
#error This project must be compiled with UNICODE!
#endif

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
    ATL::CStringW m_Value;
    ATL::CStringW m_Name;

public:

    RunOnceExEntry(
        _In_ const ATL::CStringW &Name,
        _In_ const ATL::CStringW &Value);

    BOOL Delete(_In_ CRegKeyEx &hParentKey);
    BOOL Exec() const;

    friend int RunOnceExEntryCmp(
        _In_ const void *a,
        _In_ const void *b);
};

class RunOnceExSection
{
private:
    ATL::CStringW m_SectionName;
    CRegKeyEx m_RegKey;

    BOOL HandleValue(
        _In_ CRegKeyEx &hKey,
        _In_ const CStringW &ValueName);

public:
    BOOL m_bSuccess;
    ATL::CStringW m_SectionTitle;
    CSimpleArray<RunOnceExEntry> m_EntryList;

    RunOnceExSection(
        _In_ CRegKeyEx &hParentKey,
        _In_ const CStringW &lpSubKeyName);

    RunOnceExSection(_In_ const RunOnceExSection &Section);

    BOOL CloseAndDelete(_In_ CRegKeyEx &hParentKey);

    UINT GetEntryCnt() const;

    BOOL Exec(
        _Inout_ UINT& iCompleteCnt,
        _In_ const UINT iTotalCnt);

    friend int RunOnceExSectionCmp(
        _In_ const void *a,
        _In_ const void *b);

    friend class RunOnceExInstance;
};

class RunOnceExInstance
{
private:
    CRegKeyEx m_RegKey;

    BOOL HandleSubKey(
        _In_ CRegKeyEx &hKey,
        _In_ const CStringW &SubKeyName);

public:
    BOOL m_bSuccess;
    CSimpleArray<RunOnceExSection> m_SectionList;
    CStringW m_Title;
    DWORD m_dwFlags;
    BOOL m_bShowDialog;

    RunOnceExInstance(_In_ HKEY BaseKey);

    BOOL Exec(_In_opt_ HWND hwnd);
    BOOL Run(_In_ BOOL bSilence);
};
