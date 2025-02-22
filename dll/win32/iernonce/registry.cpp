/*
 * PROJECT:     ReactOS system libraries
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Functions to read RunOnceEx registry.
 * COPYRIGHT:   Copyright 2021 He Yang <1160386205@qq.com>
 */

#include "iernonce.h"

extern RUNONCEEX_CALLBACK g_Callback;

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
    m_Value(Value), m_Name(Name)
{ ; }

BOOL RunOnceExEntry::Delete(
    _In_ CRegKeyEx &hParentKey)
{
    return hParentKey.DeleteValue(m_Name) == ERROR_SUCCESS;
}

BOOL RunOnceExEntry::Exec() const
{
    CStringW CommandLine;
    if (wcsncmp(m_Value, L"||", 2) == 0)
    {
        // Remove the prefix.
        CommandLine = (LPCWSTR)m_Value + 2;
    }
    else
    {
        CommandLine = m_Value;
    }

    // FIXME: SHEvaluateSystemCommandTemplate is not implemented
    //        using PathGetArgsW, PathRemoveArgsW as a workaround.
    LPWSTR szCommandLine = CommandLine.GetBuffer();
    LPCWSTR szParam = PathGetArgsW(szCommandLine);
    PathRemoveArgsW(szCommandLine);

    SHELLEXECUTEINFOW Info = { 0 };
    Info.cbSize = sizeof(Info);
    Info.fMask = SEE_MASK_NOCLOSEPROCESS;
    Info.lpFile = szCommandLine;
    Info.lpParameters = szParam;
    Info.nShow = SW_SHOWNORMAL;

    BOOL bSuccess = ShellExecuteExW(&Info);

    CommandLine.ReleaseBuffer();

    if (!bSuccess)
    {
        return FALSE;
    }

    if (Info.hProcess)
    {
        WaitForSingleObject(Info.hProcess, INFINITE);
        CloseHandle(Info.hProcess);
    }

    return TRUE;
}

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

    CStringW ExpandStr;
    DWORD dwcchExpand = ExpandEnvironmentStringsW(Buffer, NULL, 0);
    ExpandEnvironmentStringsW(Buffer, ExpandStr.GetBuffer(dwcchExpand + 1), dwcchExpand);
    ExpandStr.ReleaseBuffer();

    if (ValueName.IsEmpty())
    {
        // The default value specifies the section title.
        m_SectionTitle = Buffer;
    }
    else
    {
        m_EntryList.Add(RunOnceExEntry(ValueName, ExpandStr));
    }

    return TRUE;
}

RunOnceExSection::RunOnceExSection(
    _In_ CRegKeyEx &hParentKey,
    _In_ const CStringW &lpSubKeyName) :
    m_SectionName(lpSubKeyName)
{
    m_bSuccess = FALSE;
    DWORD dwValueNum;
    DWORD dwMaxValueNameLen;
    LSTATUS Error;
    CStringW ValueName;

    if (m_RegKey.Open(hParentKey, lpSubKeyName) != ERROR_SUCCESS)
        return;

    Error = RegQueryInfoKeyW(m_RegKey, NULL, 0, NULL, NULL, NULL, NULL,
                             &dwValueNum, &dwMaxValueNameLen,
                             NULL, NULL, NULL);
    if (Error != ERROR_SUCCESS)
        return;

    for (DWORD i = 0; i < dwValueNum; i++)
    {
        LPWSTR szValueName;
        DWORD dwcchName = dwMaxValueNameLen + 1;

        szValueName = ValueName.GetBuffer(dwMaxValueNameLen + 1);
        Error = m_RegKey.EnumValueName(i, szValueName, &dwcchName);
        ValueName.ReleaseBuffer();

        if (Error != ERROR_SUCCESS)
        {
            // TODO: error handling
            return;
        }

        if (!HandleValue(m_RegKey, ValueName))
            return;
    }

    // Sort entries by name in string order.
    qsort(m_EntryList.GetData(), m_EntryList.GetSize(),
          sizeof(RunOnceExEntry), RunOnceExEntryCmp);

    m_bSuccess = TRUE;
    return;
}

// Copy constructor, CSimpleArray needs it.
RunOnceExSection::RunOnceExSection(_In_ const RunOnceExSection& Section) :
    m_SectionName(Section.m_SectionName),
    m_bSuccess(Section.m_bSuccess),
    m_SectionTitle(Section.m_SectionTitle),
    m_EntryList(Section.m_EntryList)
{
    m_RegKey.Attach(Section.m_RegKey);
}

BOOL RunOnceExSection::CloseAndDelete(
    _In_ CRegKeyEx &hParentKey)
{
    m_RegKey.Close();
    return hParentKey.RecurseDeleteKey(m_SectionName) == ERROR_SUCCESS;
}

UINT RunOnceExSection::GetEntryCnt() const
{
    return m_EntryList.GetSize();
}

BOOL RunOnceExSection::Exec(
    _Inout_ UINT& iCompleteCnt,
    _In_ const UINT iTotalCnt)
{
    BOOL bSuccess = TRUE;

    for (int i = 0; i < m_EntryList.GetSize(); i++)
    {
        m_EntryList[i].Delete(m_RegKey);
        bSuccess &= m_EntryList[i].Exec();
        iCompleteCnt++;
        // TODO: the meaning of the third param is still unknown, seems it's always 0.
        if (g_Callback)
            g_Callback(iCompleteCnt, iTotalCnt, NULL);
    }
    return bSuccess;
}

int RunOnceExSectionCmp(
    _In_ const void *a,
    _In_ const void *b)
{
    return lstrcmpW(((RunOnceExSection *)a)->m_SectionName,
                    ((RunOnceExSection *)b)->m_SectionName);
}

RunOnceExInstance::RunOnceExInstance(_In_ HKEY BaseKey)
{
    m_bSuccess = FALSE;
    DWORD dwSubKeyNum;
    DWORD dwMaxSubKeyNameLen;
    LSTATUS Error;
    CStringW SubKeyName;

    Error = m_RegKey.Open(BaseKey,
                          L"Software\\Microsoft\\Windows\\CurrentVersion\\RunOnceEx\\");
    if (Error != ERROR_SUCCESS)
    {
        return;
    }

    ULONG cchTitle;
    Error = m_RegKey.QueryStringValue(L"Title", NULL, &cchTitle);
    if (Error == ERROR_SUCCESS)
    {
        Error = m_RegKey.QueryStringValue(L"Title", m_Title.GetBuffer(cchTitle + 1), &cchTitle);
        m_Title.ReleaseBuffer();
        if (Error != ERROR_SUCCESS)
            return;
    }

    Error = m_RegKey.QueryDWORDValue(L"Flags", m_dwFlags);
    if (Error != ERROR_SUCCESS)
    {
        m_dwFlags = 0;
    }

    Error = RegQueryInfoKeyW(m_RegKey, NULL, 0, NULL,
                             &dwSubKeyNum, &dwMaxSubKeyNameLen,
                             NULL, NULL, NULL, NULL, NULL, NULL);
    if (Error != ERROR_SUCCESS)
        return;

    m_bShowDialog = FALSE;

    for (DWORD i = 0; i < dwSubKeyNum; i++)
    {
        LPWSTR szSubKeyName;
        DWORD dwcchName = dwMaxSubKeyNameLen + 1;

        szSubKeyName = SubKeyName.GetBuffer(dwMaxSubKeyNameLen + 1);
        Error = m_RegKey.EnumKey(i, szSubKeyName, &dwcchName);
        SubKeyName.ReleaseBuffer();

        if (Error != ERROR_SUCCESS)
        {
            // TODO: error handling
            return;
        }

        if (!HandleSubKey(m_RegKey, SubKeyName))
            return;
    }

    // Sort sections by name in string order.
    qsort(m_SectionList.GetData(), m_SectionList.GetSize(),
          sizeof(RunOnceExSection), RunOnceExSectionCmp);

    m_bSuccess = TRUE;
    return;
}

BOOL RunOnceExInstance::Exec(_In_opt_ HWND hwnd)
{
    BOOL bSuccess = TRUE;

    UINT TotalCnt = 0;
    UINT CompleteCnt = 0;
    for (int i = 0; i < m_SectionList.GetSize(); i++)
    {
        TotalCnt += m_SectionList[i].GetEntryCnt();
    }

    // Execute items from registry one by one, and remove them.
    for (int i = 0; i < m_SectionList.GetSize(); i++)
    {
        if (hwnd)
            SendMessageW(hwnd, WM_SETINDEX, i, 0);

        bSuccess &= m_SectionList[i].Exec(CompleteCnt, TotalCnt);
        m_SectionList[i].CloseAndDelete(m_RegKey);
    }

    if (m_RegKey)
    {
        m_RegKey.DeleteValue(L"Title");
        m_RegKey.DeleteValue(L"Flags");
    }

    // Notify the dialog all sections are handled.
    if (hwnd)
        SendMessageW(hwnd, WM_SETINDEX, m_SectionList.GetSize(), bSuccess);
    return bSuccess;
}

BOOL RunOnceExInstance::Run(_In_ BOOL bSilence)
{
    if (bSilence ||
        (m_dwFlags & FLAGS_NO_STAT_DIALOG) ||
        !m_bShowDialog)
    {
        return Exec(NULL);
    }
    else
    {
        // The dialog is responsible to create a thread and execute.
        ProgressDlg dlg(*this);
        return dlg.RunDialogBox();
    }
}

BOOL RunOnceExInstance::HandleSubKey(
    _In_ CRegKeyEx &hKey,
    _In_ const CStringW& SubKeyName)
{
    RunOnceExSection Section(hKey, SubKeyName);
    if (!Section.m_bSuccess)
    {
        return FALSE;
    }

    if (!Section.m_SectionTitle.IsEmpty())
    {
        m_bShowDialog = TRUE;
    }
    m_SectionList.Add(Section);

    // The copy constructor of RunOnceExSection didn't detach
    // the m_RegKey while it's attached to the one in the array.
    // So we have to detach it manually.
    Section.m_RegKey.Detach();
    return TRUE;
}
