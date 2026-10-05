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
