/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     "Customize Start Menu" dialog
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2015 Robert Naumann <gonzomdx@gmail.com>
 *              Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

// TreeView checkbox state indexes (Use with INDEXTOSTATEIMAGEMASK macro)
#define I_UNCHECKED 1
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

static const PCWSTR g_MruKeys[] = 
{
    L"Software\\Microsoft\\Internet Explorer\\TypedURLs",
    L"Explorer\\RunMRU",
    L"Explorer\\Comdlg32\\OpenSaveMRU",
    L"Explorer\\Comdlg32\\LastVisitedMRU",
};

static BOOL HandleMruData(BOOL Delete)
{
    for (UINT i = 0; i < _countof(g_MruKeys); ++i)
    {
        WCHAR szKey[200];
        PCWSTR pszKey = g_MruKeys[i];
        if (*pszKey != 'S') // Keys not starting with S[oftware] are assumed to be relative to "SMWCV"
        {
            wsprintfW(szKey, L"%s\\%s", L"Software\\Microsoft\\Windows\\CurrentVersion", pszKey);
            pszKey = szKey;
        }

        HKEY hKey;
        if (Delete)
        {
            SHDeleteKeyW(HKEY_CURRENT_USER, pszKey);
        }
        else if (RegOpenKeyExW(HKEY_CURRENT_USER, pszKey, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return TRUE;
        }
    }
    return FALSE;
}

VOID ClearRecentAndMru()
{
    SHAddToRecentDocs(SHARD_PIDL, NULL);
    HandleMruData(TRUE);
}

static VOID InitializeClearButton(HWND hwnd)
{
    HWND hWndClear = GetDlgItem(hwnd, IDC_CLASSICSTART_CLEAR);
    BOOL bHasData = RecentHasShortcut(hwnd) || HandleMruData(FALSE);
    if (!bHasData && hWndClear == GetFocus())
        SendMessage(hwnd, WM_NEXTDLGCTL, 0, FALSE);
    EnableWindow(hWndClear, bHasData);
}

struct CUSTOM_ENTRY;

typedef BOOL (CALLBACK *FN_CUSTOM_GET)(const CUSTOM_ENTRY *entry);
typedef VOID (CALLBACK *FN_CUSTOM_SET)(const CUSTOM_ENTRY *entry, BOOL bValue);

struct CUSTOM_ENTRY
{
    LPARAM id;
    LPCWSTR name;
    BOOL bDefaultValue;
    FN_CUSTOM_GET fnGetValue;
    FN_CUSTOM_SET fnSetValue;
    RESTRICTIONS policy1, policy2;
};

static BOOL CALLBACK CustomGetAdvanced(const CUSTOM_ENTRY *entry)
{
    return GetAdvancedBool(entry->name, entry->bDefaultValue);
}

static VOID CALLBACK CustomSetAdvanced(const CUSTOM_ENTRY *entry, BOOL bValue)
{
    SetAdvancedDword(entry->name, bValue);
}

static BOOL CALLBACK CustomGetSmallStartMenu(const CUSTOM_ENTRY *entry)
{
    return g_TaskbarSettings.sr.SmSmallIcons;
}

static VOID CALLBACK CustomSetSmallStartMenu(const CUSTOM_ENTRY *entry, BOOL bValue)
{
    g_TaskbarSettings.sr.SmSmallIcons = bValue;
}

static const CUSTOM_ENTRY s_CustomEntries[] =
{
    {
        IDS_ADVANCED_DISPLAY_ADMINTOOLS, L"StartMenuAdminTools", TRUE,
        CustomGetAdvanced, CustomSetAdvanced,
    },
    {
        IDS_ADVANCED_DISPLAY_FAVORITES, L"StartMenuFavorites", FALSE,
        CustomGetAdvanced, CustomSetAdvanced,
        REST_NOFAVORITESMENU,
    },
    {
        IDS_ADVANCED_DISPLAY_LOG_OFF, L"StartMenuLogoff", FALSE,
        CustomGetAdvanced, CustomSetAdvanced,
        REST_STARTMENULOGOFF,
    },
    {
        IDS_ADVANCED_DISPLAY_RUN, L"StartMenuRun", TRUE,
        CustomGetAdvanced, CustomSetAdvanced,
        REST_NORUN,
    },
    {
        IDS_ADVANCED_EXPAND_MY_DOCUMENTS, L"CascadeMyDocuments", FALSE,
        CustomGetAdvanced, CustomSetAdvanced,
        REST_NOSMMYDOCS,
    },
    {
        IDS_ADVANCED_EXPAND_MY_PICTURES, L"CascadeMyPictures", FALSE,
        CustomGetAdvanced, CustomSetAdvanced,
        REST_NOSMMYPICS,
    },
    {
        IDS_ADVANCED_EXPAND_CONTROL_PANEL, L"CascadeControlPanel", FALSE,
        CustomGetAdvanced, CustomSetAdvanced,
        REST_NOSETFOLDERS, REST_NOCONTROLPANEL,
    },
    {
        IDS_ADVANCED_EXPAND_PRINTERS, L"CascadePrinters", FALSE,
        CustomGetAdvanced, CustomSetAdvanced,
        REST_NOSETFOLDERS,
    },
    {
        IDS_ADVANCED_EXPAND_NET_CONNECTIONS, L"CascadeNetworkConnections", FALSE,
        CustomGetAdvanced, CustomSetAdvanced,
        REST_NOSETFOLDERS, REST_NONETWORKCONNECTIONS,
    },
    {
        IDS_ADVANCED_SMALL_START_MENU, NULL, FALSE,
        CustomGetSmallStartMenu, CustomSetSmallStartMenu,
    },
};

static VOID AddCustomItem(HWND hTreeView, const CUSTOM_ENTRY *entry)
{
    if (SHRestricted(entry->policy1) || SHRestricted(entry->policy2))
    {
        TRACE("%p: Restricted\n", entry->id);
        return; // Restricted. Don't show
    }

    WCHAR szText[MAX_PATH];
    LoadStringW(GetModuleHandleW(L"shell32.dll"), entry->id, szText, _countof(szText));

    BOOL bChecked = entry->fnGetValue(entry);
    TRACE("%p: %d\n", entry->id, bChecked);

    TV_INSERTSTRUCT Insert = { TVI_ROOT, TVI_LAST, { TVIF_TEXT | TVIF_STATE | TVIF_PARAM } };
    Insert.item.pszText = szText;
    Insert.item.lParam = entry->id;
    Insert.item.stateMask = TVIS_STATEIMAGEMASK;
    Insert.item.state = INDEXTOSTATEIMAGEMASK(bChecked ? I_CHECKED : I_UNCHECKED);
    TreeView_InsertItem(hTreeView, &Insert);
}

static void CustomizeClassic_OnInitDialog(HWND hwnd)
{
    InitializeClearButton(hwnd);

    HWND hTreeView = GetDlgItem(hwnd, IDC_CLASSICSTART_SETTINGS);

    DWORD_PTR style = GetWindowLongPtrW(hTreeView, GWL_STYLE);
    SetWindowLongPtrW(hTreeView, GWL_STYLE, style | TVS_CHECKBOXES);

    for (auto& entry : s_CustomEntries)
    {
        AddCustomItem(hTreeView, &entry);
    }
}

static BOOL CustomizeClassic_OnOK(HWND hwnd)
{
    HWND hTreeView = GetDlgItem(hwnd, IDC_CLASSICSTART_SETTINGS);

    for (HTREEITEM hItem = TreeView_GetRoot(hTreeView);
         hItem != NULL;
         hItem = TreeView_GetNextVisible(hTreeView, hItem))
    {
        TV_ITEM item = { TVIF_PARAM | TVIF_STATE, hItem };
        item.stateMask = TVIS_STATEIMAGEMASK;
        TreeView_GetItem(hTreeView, &item);

        BOOL bChecked = !!(item.state & INDEXTOSTATEIMAGEMASK(I_CHECKED));
        for (auto& entry : s_CustomEntries)
        {
            if (SHRestricted(entry.policy1) || SHRestricted(entry.policy2))
                continue;

            if (item.lParam == entry.id)
            {
                TRACE("%p: %d\n", item.lParam, bChecked);
                entry.fnSetValue(&entry, bChecked);
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
                    ClearRecentAndMru();
                    InitializeClearButton(hwnd);
                    break;
                case IDOK:
                    if (CustomizeClassic_OnOK(hwnd))
                        EndDialog(hwnd, IDOK);
                    break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;
        default:
            break;
    }

    return FALSE;
}

VOID ShowCustomizeClassic(HINSTANCE hInst, HWND hExplorer)
{
    DialogBoxW(hInst, MAKEINTRESOURCEW(IDD_CLASSICSTART_CUSTOMIZE), hExplorer, CustomizeClassicProc);
}
