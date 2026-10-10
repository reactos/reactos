/*
 * PROJECT:     ReactOS Management Console
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Misc Stuff
 * COPYRIGHT:   Copyright 2026 Eric Kohl (eric.kohl@reactos.org)
 */

 #include "precomp.h"

UINT
ViewModeToCmdId(LISTVIEW_MODE ListViewMode)
{
    UINT CmdId;

    switch (ListViewMode)
    {
        case ListView_SmallIcons:
            CmdId = IDM_VIEW_SMALL_ICONS;
            break;

        case ListView_LargeIcons:
            CmdId = IDM_VIEW_LARGE_ICONS;
            break;

        case ListView_List:
            CmdId = IDM_VIEW_LIST;
            break;

        default:
        case ListView_Detail:
            CmdId = IDM_VIEW_DETAILS;
            break;
    }

    return CmdId;
}

LISTVIEW_MODE
CmdIdToViewMode(UINT CmdId)
{
    LISTVIEW_MODE ListViewMode;

    switch (CmdId)
    {
        case IDM_VIEW_SMALL_ICONS:
            ListViewMode = ListView_SmallIcons;
            break;

        case IDM_VIEW_LARGE_ICONS:
            ListViewMode = ListView_LargeIcons;
            break;

        case IDM_VIEW_LIST:
            ListViewMode = ListView_List;
            break;

        default:
        case IDM_VIEW_DETAILS:
            ListViewMode = ListView_Detail;
            break;
    }

    return ListViewMode;
}

LPWSTR
ViewModeToString(LISTVIEW_MODE ListViewMode)
{
    switch (ListViewMode)
    {
        case ListView_SmallIcons:
            return (LPWSTR)L"SmallIcon";

        case ListView_LargeIcons:
            return (LPWSTR)L"Icon";

        case ListView_List:
            return (LPWSTR)L"List";

        default:
        case ListView_Detail:
            return (LPWSTR)L"Report";
    }
    return (LPWSTR)L"-1";
}

LISTVIEW_MODE
StringToViewMode(LPWSTR ViewMode)
{
    if (_wcsicmp(ViewMode, L"SmallIcon") == 0)
    {
        return ListView_SmallIcons;
    }
    else if (_wcsicmp(ViewMode, L"Icon") == 0)
    {
        return ListView_LargeIcons;
    }
    else if (_wcsicmp(ViewMode, L"List") == 0)
    {
        return ListView_List;
    }
    else
    {
        return ListView_Detail;
    }
}

DWORD
ViewModeToStyle(LISTVIEW_MODE ListViewMode)
{
    switch (ListViewMode)
    {
        case ListView_SmallIcons:
            return LVS_SMALLICON;

        case ListView_LargeIcons:
            return LVS_ICON;

        case ListView_List:
            return LVS_LIST;

        default:
        case ListView_Detail:
            return LVS_REPORT;
    }
    return 0;
}

LPWSTR
ShowCmdToString(
    int nShowCmd)
{
    switch (nShowCmd)
    {
        case SW_HIDE:
            return (LPWSTR)L"SW_HIDE";

        case SW_SHOWNORMAL:
            return (LPWSTR)L"SW_SHOWNORMAL";

        case SW_SHOWMINIMIZED:
            return (LPWSTR)L"SW_SHOWMINIMIZED";

        case SW_SHOWMAXIMIZED:
            return (LPWSTR)L"SW_SHOWMAXIMIZED";

        case SW_SHOWNOACTIVATE:
            return (LPWSTR)L"SW_SHOWNOACTIVATE";

        case SW_SHOW:
            return (LPWSTR)L"SW_SHOW";

        case SW_MINIMIZE:
            return (LPWSTR)L"SW_MINIMIZE";
        
        case SW_SHOWMINNOACTIVE:
            return (LPWSTR)L"SW_SHOWMINNOACTIVE";
        
        case SW_SHOWNA:
            return (LPWSTR)L"SW_SHOWNA";

        case SW_RESTORE:
            return (LPWSTR)L"SW_RESTORE";

        case SW_SHOWDEFAULT:
            return (LPWSTR)L"SW_SHOWDEFAULT";

        case SW_FORCEMINIMIZE:
            return (LPWSTR)L"SW_FORCEMINIMIZE";
    }

    return (LPWSTR)L"-1";
}

int
ShowCmdFromString(LPWSTR pszString)
{
    if (_wcsicmp(pszString, L"SW_HIDE") == 0)
        return SW_HIDE;
    else if (_wcsicmp(pszString, L"SW_SHOWNORMAL") == 0)
        return SW_SHOWNORMAL;
    else if (_wcsicmp(pszString, L"SW_SHOWMINIMIZED") == 0)
        return SW_SHOWMINIMIZED;
    else if (_wcsicmp(pszString, L"SW_SHOWMAXIMIZED") == 0)
        return SW_SHOWMAXIMIZED;
    else if (_wcsicmp(pszString, L"SW_SHOWNOACTIVATE") == 0)
        return SW_SHOWNOACTIVATE;
    else if (_wcsicmp(pszString, L"SW_SHOW") == 0)
        return SW_SHOW;
    else if (_wcsicmp(pszString, L"SW_MINIMIZE") == 0)
        return SW_MINIMIZE;
    else if (_wcsicmp(pszString, L"SW_SHOWMINNOACTIVE") == 0)
        return SW_SHOWMINNOACTIVE;
    else if (_wcsicmp(pszString, L"SW_SHOWNA") == 0)
        return SW_SHOWNA;
    else if (_wcsicmp(pszString, L"SW_RESTORE") == 0)
        return SW_RESTORE;
    else if (_wcsicmp(pszString, L"SW_SHOWDEFAULT") == 0)
        return SW_SHOWDEFAULT;
    else if (_wcsicmp(pszString, L"SW_FORCEMINIMIZE") == 0)
        return SW_FORCEMINIMIZE;
    return 0;
}

LPWSTR
DocumentModeToString(DOCUMENT_MODE DocumentMode)
{
    switch (DocumentMode)
    {
        case DocumentMode_Author:
            return (LPWSTR)L"Author";

        case DocumentMode_User:
            return (LPWSTR)L"User";

        case DocumentMode_UserMDI:
            return (LPWSTR)L"UserMDI";

        case DocumentMode_UserSDI:
            return (LPWSTR)L"UserSDI";

        default:
            return (LPWSTR)L"";
    }
}

HRESULT
StringToDocumentMode(
    _In_ LPWSTR pszDocumentMode,
    _Out_ PDOCUMENT_MODE pDocumentMode)
{
    HRESULT hr = S_OK;

    if (_wcsicmp(pszDocumentMode, L"Author") == 0)
        *pDocumentMode = DocumentMode_Author;
    else if (_wcsicmp(pszDocumentMode, L"User") == 0)
        *pDocumentMode = DocumentMode_User;
    else if (_wcsicmp(pszDocumentMode, L"UserMDI") == 0)
        *pDocumentMode = DocumentMode_UserMDI;
    else if (_wcsicmp(pszDocumentMode, L"UserSDI") == 0)
        *pDocumentMode = DocumentMode_UserSDI;
    else
        hr = E_FAIL;

    return hr;
}

