/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 *                  2015 Robert Naumann <gonzomdx@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

#define I_CHECKED   2

// TODO: Windows Explorer appears to be calling NewLinkHere / ConfigStartMenu directly for both items.
VOID OnAddStartMenuItems(HWND hDlg)
{
    WCHAR szPath[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_PROGRAMS, NULL, 0, szPath)))
    {
        WCHAR szCommand[MAX_PATH] = L"appwiz.cpl,NewLinkHere ";
        if (SUCCEEDED(StringCchCatW(szCommand, _countof(szCommand), szPath)))
            ShellExecuteW(hDlg, L"open", L"rundll32.exe", szCommand, NULL, SW_SHOWNORMAL);
    }
}

VOID OnRemoveStartmenuItems(HWND hDlg)
{
    ShellExecuteW(hDlg, L"open", L"rundll32.exe", L"appwiz.cpl,ConfigStartMenu", NULL, SW_SHOWNORMAL);
}

VOID OnAdvancedStartMenuItems()
{
    WCHAR szPath[MAX_PATH];

    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_STARTMENU, NULL, 0, szPath)))
    {
        ShellExecuteW(NULL, L"explore", szPath, NULL, NULL, SW_SHOWNORMAL);
    }
}

static BOOL RecentHasShortcut(HWND hwnd)
{
    WCHAR szPath[MAX_PATH];
    if (FAILED(SHGetFolderPathW(hwnd, CSIDL_RECENT | CSIDL_FLAG_CREATE, NULL, 0, szPath)))
        return FALSE;

    // Find shortcut files in Recent
    WIN32_FIND_DATAW find;
    PathAppendW(szPath, L"*.lnk");
    HANDLE hFind = FindFirstFileW(szPath, &find);
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;

    FindClose(hFind);
    return TRUE;
}

static VOID OnClearRecentItems(HWND hwnd)
{
    SHAddToRecentDocs(SHARD_PIDL, NULL);
    EnableWindow(GetDlgItem(hwnd, IDC_CLASSICSTART_CLEAR), RecentHasShortcut(hwnd));
}

struct CUSTOMIZE_ENTRY;
typedef DWORD (CALLBACK *FN_CUSTOMIZE_READ)(const CUSTOMIZE_ENTRY *entry);
typedef BOOL (CALLBACK *FN_CUSTOMIZE_WRITE)(const CUSTOMIZE_ENTRY *entry, DWORD dwValue);

struct CUSTOMIZE_ENTRY
{
    LPARAM id;
    LPCWSTR name;
    FN_CUSTOMIZE_READ fnRead;
    FN_CUSTOMIZE_WRITE fnWrite;
};

static DWORD CALLBACK CustomizeRead0(const CUSTOMIZE_ENTRY *entry)
{
    return GetExplorerRegValueSet(HKEY_CURRENT_USER, L"Advanced", entry->name, FALSE);
}

static BOOL CALLBACK CustomizeWrite0(const CUSTOMIZE_ENTRY *entry, DWORD dwValue)
{
    return SetExplorerRegValueSet(HKEY_CURRENT_USER, L"Advanced", entry->name, dwValue);
}

static const CUSTOMIZE_ENTRY s_CustomizeEntries[] =
{
    { IDS_ADVANCED_DISPLAY_FAVORITES, L"StartMenuFavorites", CustomizeRead0, CustomizeWrite0 },
    { IDS_ADVANCED_DISPLAY_LOG_OFF,   L"StartMenuLogoff",    CustomizeRead0, CustomizeWrite0 },
    // FIXME: SHRestricted is buggy!
    //{ IDS_ADVANCED_DISPLAY_RUN,       L"NoRun",              CustomizeRead1, CustomizeWrite1 },
};

static VOID AddCustomizeItem(HWND hTreeView, const CUSTOMIZE_ENTRY *entry)
{
    TV_INSERTSTRUCT Insert = { TVI_ROOT, TVI_LAST };
    Insert.item.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;

    WCHAR szText[MAX_PATH];
    LoadStringW(GetModuleHandleW(L"shell32.dll"), entry->id, szText, _countof(szText));
    Insert.item.pszText = szText;
    Insert.item.lParam = entry->id;
    Insert.item.stateMask = TVIS_STATEIMAGEMASK;
    if (entry->fnRead(entry))
        Insert.item.state = INDEXTOSTATEIMAGEMASK(I_CHECKED);
    TreeView_InsertItem(hTreeView, &Insert);
}

static void CustomizeClassic_OnInitDialog(HWND hwnd)
{
    EnableWindow(GetDlgItem(hwnd, IDC_CLASSICSTART_CLEAR), RecentHasShortcut(hwnd));

    HWND hTreeView = GetDlgItem(hwnd, IDC_CLASSICSTART_SETTINGS);

    DWORD_PTR style = GetWindowLongPtrW(hTreeView, GWL_STYLE);
    SetWindowLongPtrW(hTreeView, GWL_STYLE, style | TVS_CHECKBOXES);

    for (auto& entry : s_CustomizeEntries)
    {
        AddCustomizeItem(hTreeView, &entry);
    }
}

static BOOL CustomizeClassic_OnOK(HWND hwnd)
{
    HWND hTreeView = GetDlgItem(hwnd, IDC_CLASSICSTART_SETTINGS);

    for (HTREEITEM hItem = TreeView_GetFirstVisible(hTreeView);
         hItem != NULL;
         hItem = TreeView_GetNextVisible(hTreeView, hItem))
    {
        TV_ITEM item = { TVIF_PARAM | TVIF_STATE };
        item.hItem = hItem;
        item.stateMask = TVIS_STATEIMAGEMASK;
        TreeView_GetItem(hTreeView, &item);

        BOOL bChecked = (item.state & INDEXTOSTATEIMAGEMASK(I_CHECKED));
        for (auto& entry : s_CustomizeEntries)
        {
            if (item.lParam == entry.id)
            {
                entry.fnWrite(&entry, bChecked);
                break;
            }
        }
    }

    SendMessageTimeoutW(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"TraySettings",
                        SMTO_ABORTIFHUNG, 200, NULL);
    return TRUE;
}

INT_PTR CALLBACK CustomizeClassicProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    switch (Message)
    {
        case WM_INITDIALOG:
            CustomizeClassic_OnInitDialog(hwnd);
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_CLASSICSTART_ADD:
                    OnAddStartMenuItems(hwnd);
                    break;
                case IDC_CLASSICSTART_REMOVE:
                    OnRemoveStartmenuItems(hwnd);
                    break;
                case IDC_CLASSICSTART_ADVANCED:
                    OnAdvancedStartMenuItems();
                    break;
                case IDC_CLASSICSTART_CLEAR:
                    OnClearRecentItems(hwnd);
                    break;
                case IDOK:
                    if (CustomizeClassic_OnOK(hwnd))
                    {
                        EndDialog(hwnd, IDOK);
                    }
                    break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

VOID ShowCustomizeClassic(HINSTANCE hInst, HWND hExplorer)
{
    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_CLASSICSTART_CUSTOMIZE), hExplorer, CustomizeClassicProc);
}
