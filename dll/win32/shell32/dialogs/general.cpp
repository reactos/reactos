/*
 *     'General' tab property sheet of Folder Options
 *
 * Copyright 2007 Johannes Anderwald <johannes.anderwald@reactos.org>
 * Copyright 2016-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL (fprop);

static const LPCWSTR s_pszExplorerKey =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer";

/////////////////////////////////////////////////////////////////////////////
// Shell settings

EXTERN_C void
SHELL32_GetDefaultShellState(LPSHELLSTATE pss)
{
    ZeroMemory(pss, sizeof(*pss));
    pss->fShowAllObjects = TRUE;
    pss->fShowExtensions = TRUE;
    pss->fShowCompColor = TRUE;
    pss->fDoubleClickInWebView = TRUE;
    pss->fShowAttribCol = TRUE; // ROS defaults to Details view with this column on
    pss->fShowInfoTip = TRUE;
    pss->fShowSuperHidden = FALSE;
    pss->lParamSort = SHFSF_COL_NAME;
    pss->iSortDirection = 1;
    pss->version = REGSHELLSTATE_VERSION;
    pss->fSepProcess = FALSE;
    pss->fStartPanelOn = FALSE; // Note: This should be changed to TRUE when the modern start menu is implemented
}

EXTERN_C LSTATUS
SHELL32_WriteRegShellState(PREGSHELLSTATE prss)
{
    prss->dwSize = REGSHELLSTATE_SIZE;
    prss->ss.version = REGSHELLSTATE_VERSION;
    return SHSetValueW(HKEY_CURRENT_USER, s_pszExplorerKey, L"ShellState",
                       REG_BINARY, prss, prss->dwSize);
}

// bDoubleClick is TRUE if "Double-click to open an item (single-click to select)".
// bDoubleClick is FALSE if "Single-click to open an item (point to select)".
//////////////////////////////////////////////////////////////////////////////
// API Monitor:
// SHLWAPI.dll	RegOpenKeyExW ( 0x000000c8, NULL, 0, MAXIMUM_ALLOWED, 0x00def234 )	ERROR_SUCCESS		0.0000243
// SHLWAPI.dll	RegSetValueExW ( 0x000003e8, "ShellState", 0, REG_BINARY, 0x000c2050, 36 )	ERROR_SUCCESS		0.0001028
// SHLWAPI.dll	RegCloseKey ( 0x000003e8 )	ERROR_SUCCESS		0.0000081
// Explorer.EXE	SHSettingsChanged ( 0, "ShellState" )			0.0000131
static BOOL
IntSetShellStateSettings(BOOL bDoubleClick, BOOL bUseCommonTasks)
{
    SHELLSTATE shellstate;
    shellstate.fDoubleClickInWebView = !!bDoubleClick;
    shellstate.fWebView = !!bUseCommonTasks;
    SHGetSetSettings(&shellstate, SSF_DOUBLECLICKINWEBVIEW | SSF_WEBVIEW, TRUE);

    SHSettingsChanged(0, L"ShellState");
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
// API Monitor:
// SHLWAPI.dll	RegOpenKeyExW ( 0x000000c8, NULL, 0, MAXIMUM_ALLOWED, 0x0007e484 )	ERROR_SUCCESS		0.0000388
// SHLWAPI.dll	RegQueryValueExW ( 0x000005a8, "ShellState", NULL, 0x0007e474, 0x000c2050, 0x0007e4fc )	ERROR_SUCCESS		0.0000271
// SHLWAPI.dll	RegCloseKey ( 0x000005a8 )	ERROR_SUCCESS		0.0000112
EXTERN_C BOOL
SHELL32_ReadRegShellState(PREGSHELLSTATE prss)
{
    DWORD dwSize = sizeof(REGSHELLSTATE);
    LSTATUS err = SHGetValueW(HKEY_CURRENT_USER, s_pszExplorerKey,
                              L"ShellState", NULL, prss, &dwSize);
    return err == ERROR_SUCCESS && prss->dwSize >= REGSHELLSTATE_SIZE;
}

// bIconUnderline is TRUE if "Underline icon titles only when I point at them".
// bIconUnderline is FALSE if "Underline icon titles consistent with my browser".
//////////////////////////////////////////////////////////////////////////////
// API Monitor:
// SHELL32.dll	SHRegGetUSValueW ( "Software\Microsoft\Windows\CurrentVersion\Explorer", "IconUnderline", NULL, 0x00d2f324, 0x00d2f328, FALSE, 0x00d2f32c, 4 )	ERROR_SUCCESS		0.0002484
// SHELL32.dll	IsDlgButtonChecked ( 0x0005009e, 30104 )	BST_CHECKED		0.0000212
// SHELL32.dll	SHRegSetUSValueW ( "Software\Microsoft\Windows\CurrentVersion\Explorer", "IconUnderline", 0, 0x00d2f314, 4, 6 )	ERROR_SUCCESS		0.0008300
// Explorer.EXE	SHSettingsChanged ( 0, "Software\Microsoft\Windows\CurrentVersion\Explorer\IconUnderline" )			0.0000092
static BOOL IntSetUnderlineState(BOOL bIconUnderline)
{
    LSTATUS Status;
    DWORD dwValue = (bIconUnderline ? 3 : 2), dwSize = sizeof(DWORD);
    Status = SHRegSetUSValue(s_pszExplorerKey, L"IconUnderline", REG_NONE,
                             &dwValue, dwSize, SHREGSET_FORCE_HKCU | SHREGSET_HKLM);
    if (Status != ERROR_SUCCESS)
        return FALSE;

    SHSettingsChanged(0, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\IconUnderline");
    return TRUE;
}

static BOOL IntGetUnderlineState(VOID)
{
    DWORD dwValue, dwDefault = 2, dwSize = sizeof(DWORD);
    SHRegGetUSValue(s_pszExplorerKey, L"IconUnderline", NULL, &dwValue, &dwSize, FALSE, &dwDefault, sizeof(DWORD));
    return dwValue == 3;
}

// bNewWindowMode is TRUE if "Open each folder in its own window".
// bNewWindowMode is FALSE if "Open each folder in the same window".
//////////////////////////////////////////////////////////////////////////////
// API Monitor:
// SHELL32.dll	RegCreateKeyExW ( HKEY_CURRENT_USER, "Software\Microsoft\Windows\CurrentVersion\Explorer\CabinetState", 0, NULL, 0, KEY_SET_VALUE, NULL, 0x00d2f2b8, NULL )	ERROR_SUCCESS		0.0000455
// SHELL32.dll	RegSetValueExW ( 0x00000854, "Settings", 0, REG_BINARY, 0x0210f170, 12 )	ERROR_SUCCESS		0.0001472
// SHELL32.dll	RegSetValueExW ( 0x00000854, "FullPath", 0, REG_DWORD, 0x00d2f2ac, 4 )	ERROR_SUCCESS		0.0000168
// SHELL32.dll	RegCloseKey ( 0x00000854 )	ERROR_SUCCESS		0.0000000
static HRESULT IntSetNewWindowMode(BOOL bNewWindowMode)
{
    CABINETSTATE cs;
    if (!ReadCabinetState(&cs, sizeof(cs)))
        return E_FAIL;

    BOOL changed = !!cs.fNewWindowMode != !!bNewWindowMode;
    cs.fNewWindowMode = !!bNewWindowMode;
    return WriteCabinetState(&cs) ? (changed ? S_OK : S_FALSE) : E_FAIL;
}

static BOOL IntGetNewWindowMode(VOID)
{
    CABINETSTATE cs;
    if (!ReadCabinetState(&cs, sizeof(cs)))
        return FALSE;

    return !!cs.fNewWindowMode;
}

/////////////////////////////////////////////////////////////////////////////
// GeneralDlg

typedef struct GENERAL_DIALOG
{
    HICON hTaskIcon;
    HICON hFolderIcon;
    HICON hClickIcon;
} GENERAL_DIALOG, *PGENERAL_DIALOG;

static VOID
GeneralDlg_UpdateIcons(HWND hDlg, UINT nCtrlID, PGENERAL_DIALOG pGeneral)
{
    HICON hTaskIcon = NULL, hFolderIcon = NULL, hClickIcon = NULL;
    LPTSTR lpTaskIconName = NULL, lpFolderIconName = NULL, lpClickIconName = NULL;

    // Show task setting icon.
    if (IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_COMMONTASKS) == BST_CHECKED)
        lpTaskIconName = MAKEINTRESOURCE(IDI_SHELL_SHOW_COMMON_TASKS);
    else if (IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_CLASSICFOLDERS) == BST_CHECKED)
        lpTaskIconName = MAKEINTRESOURCE(IDI_SHELL_CLASSIC_FOLDERS);

    if (lpTaskIconName)
    {
        hTaskIcon = (HICON)LoadImage(shell32_hInstance, lpTaskIconName,
                                     IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
        if (hTaskIcon)
        {
            HWND hwndTaskIcon = GetDlgItem(hDlg, IDC_FOLDER_OPTIONS_TASKICON);
            SendMessage(hwndTaskIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hTaskIcon);
        }
    }

    // Show Folder setting icons
    if (IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_SAMEWINDOW) == BST_CHECKED)
        lpFolderIconName = MAKEINTRESOURCE(IDI_SHELL_OPEN_IN_SOME_WINDOW);
    else if (IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_OWNWINDOW) == BST_CHECKED)
        lpFolderIconName = MAKEINTRESOURCE(IDI_SHELL_OPEN_IN_NEW_WINDOW);

    if (lpFolderIconName)
    {
        hFolderIcon = (HICON)LoadImage(shell32_hInstance, lpFolderIconName,
                                       IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
        if (hFolderIcon)
        {
            HWND hwndFolderIcon = GetDlgItem(hDlg, IDC_FOLDER_OPTIONS_FOLDERICON);
            SendMessage(hwndFolderIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hFolderIcon);
        }
    }

    // Show click setting icon
    if (IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_SINGLECLICK) == BST_CHECKED)
        lpClickIconName = MAKEINTRESOURCE(IDI_SHELL_SINGLE_CLICK_TO_OPEN);
    else if (IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_DOUBLECLICK) == BST_CHECKED)
        lpClickIconName = MAKEINTRESOURCE(IDI_SHELL_DOUBLE_CLICK_TO_OPEN);

    if (lpClickIconName)
    {
        hClickIcon = (HICON)LoadImage(shell32_hInstance, lpClickIconName,
                                      IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
        if (hClickIcon)
        {
            HWND hwndClickIcon = GetDlgItem(hDlg, IDC_FOLDER_OPTIONS_CLICKICON);
            SendMessage(hwndClickIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hClickIcon);
        }
    }

    // Replace icons
    if (hTaskIcon)
    {
        DestroyIcon(pGeneral->hTaskIcon);
        pGeneral->hTaskIcon = hTaskIcon;
    }
    if (hFolderIcon)
    {
        DestroyIcon(pGeneral->hFolderIcon);
        pGeneral->hFolderIcon = hFolderIcon;
    }
    if (hClickIcon)
    {
        DestroyIcon(pGeneral->hClickIcon);
        pGeneral->hClickIcon = hClickIcon;
    }

    if (nCtrlID == IDC_FOLDER_OPTIONS_SINGLECLICK)
    {
        EnableWindow(GetDlgItem(hDlg, IDC_FOLDER_OPTIONS_ULBROWSER), TRUE);
        EnableWindow(GetDlgItem(hDlg, IDC_FOLDER_OPTIONS_ULPOINT), TRUE);
    }

    if (nCtrlID == IDC_FOLDER_OPTIONS_DOUBLECLICK)
    {
        CheckRadioButton(hDlg, IDC_FOLDER_OPTIONS_ULBROWSER, IDC_FOLDER_OPTIONS_ULPOINT, IDC_FOLDER_OPTIONS_ULBROWSER);
        EnableWindow(GetDlgItem(hDlg, IDC_FOLDER_OPTIONS_ULBROWSER), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_FOLDER_OPTIONS_ULPOINT), FALSE);
    }
}

static void
GeneralDlg_StoreToUI(HWND hwndDlg, BOOL bDoubleClick, BOOL bUseCommonTasks,
                     BOOL bUnderline, BOOL bNewWindowMode, PGENERAL_DIALOG pGeneral)
{
    EnableWindow(GetDlgItem(hwndDlg, IDC_FOLDER_OPTIONS_COMMONTASKS), bUseCommonTasks); // FIXME: ROS DefView does not support WebView nor the tasks pane

    if (bUseCommonTasks)
        CheckRadioButton(hwndDlg, IDC_FOLDER_OPTIONS_COMMONTASKS, IDC_FOLDER_OPTIONS_CLASSICFOLDERS, IDC_FOLDER_OPTIONS_COMMONTASKS);
    else
        CheckRadioButton(hwndDlg, IDC_FOLDER_OPTIONS_COMMONTASKS, IDC_FOLDER_OPTIONS_CLASSICFOLDERS, IDC_FOLDER_OPTIONS_CLASSICFOLDERS);

    if (bDoubleClick)
        CheckRadioButton(hwndDlg, IDC_FOLDER_OPTIONS_SINGLECLICK, IDC_FOLDER_OPTIONS_DOUBLECLICK, IDC_FOLDER_OPTIONS_DOUBLECLICK);
    else
        CheckRadioButton(hwndDlg, IDC_FOLDER_OPTIONS_SINGLECLICK, IDC_FOLDER_OPTIONS_DOUBLECLICK, IDC_FOLDER_OPTIONS_SINGLECLICK);

    if (bNewWindowMode)
        CheckRadioButton(hwndDlg, IDC_FOLDER_OPTIONS_SAMEWINDOW, IDC_FOLDER_OPTIONS_OWNWINDOW, IDC_FOLDER_OPTIONS_OWNWINDOW);
    else
        CheckRadioButton(hwndDlg, IDC_FOLDER_OPTIONS_SAMEWINDOW, IDC_FOLDER_OPTIONS_OWNWINDOW, IDC_FOLDER_OPTIONS_SAMEWINDOW);

    if (!bDoubleClick)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_FOLDER_OPTIONS_ULBROWSER), TRUE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_FOLDER_OPTIONS_ULPOINT), TRUE);
        if (bUnderline)
            CheckRadioButton(hwndDlg, IDC_FOLDER_OPTIONS_ULBROWSER, IDC_FOLDER_OPTIONS_ULPOINT, IDC_FOLDER_OPTIONS_ULPOINT);
        else
            CheckRadioButton(hwndDlg, IDC_FOLDER_OPTIONS_ULBROWSER, IDC_FOLDER_OPTIONS_ULPOINT, IDC_FOLDER_OPTIONS_ULBROWSER);
    }
    else
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_FOLDER_OPTIONS_ULBROWSER), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_FOLDER_OPTIONS_ULPOINT), FALSE);
        CheckRadioButton(hwndDlg, IDC_FOLDER_OPTIONS_ULBROWSER, IDC_FOLDER_OPTIONS_ULPOINT, IDC_FOLDER_OPTIONS_ULBROWSER);
    }
}

static BOOL
GeneralDlg_OnInitDialog(HWND hwndDlg, PGENERAL_DIALOG pGeneral)
{
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_DOUBLECLICKINWEBVIEW | SSF_WEBVIEW, FALSE);
    BOOL bDoubleClick = !!ss.fDoubleClickInWebView;
    BOOL bUseCommonTasks = !!ss.fWebView;
    BOOL bUnderline = IntGetUnderlineState();
    BOOL bNewWindowMode = IntGetNewWindowMode();

    GeneralDlg_StoreToUI(hwndDlg, bDoubleClick, bUseCommonTasks, bUnderline, bNewWindowMode, pGeneral);
    GeneralDlg_UpdateIcons(hwndDlg, 0, pGeneral);

    return TRUE;
}

static void
GeneralDlg_OnRestoreDefaults(HWND hwndDlg, PGENERAL_DIALOG pGeneral)
{
    // default values
    BOOL bDoubleClick = TRUE;
    BOOL bUseCommonTasks = TRUE;
    BOOL bUnderline = FALSE;
    BOOL bNewWindowMode = (_WIN32_WINNT < _WIN32_WINNT_WIN2K);

    GeneralDlg_StoreToUI(hwndDlg, bDoubleClick, bUseCommonTasks, bUnderline, bNewWindowMode, pGeneral);
    GeneralDlg_UpdateIcons(hwndDlg, 0, pGeneral);
}

static BOOL
GeneralDlg_OnApply(HWND hwndDlg, PGENERAL_DIALOG pGeneral)
{
    BOOL bDoubleClick = !(IsDlgButtonChecked(hwndDlg, IDC_FOLDER_OPTIONS_SINGLECLICK) == BST_CHECKED);
    BOOL bUseCommonTasks = (IsDlgButtonChecked(hwndDlg, IDC_FOLDER_OPTIONS_COMMONTASKS) == BST_CHECKED);
    BOOL bUnderline = (IsDlgButtonChecked(hwndDlg, IDC_FOLDER_OPTIONS_ULPOINT) == BST_CHECKED);
    BOOL bNewWindowMode = !(IsDlgButtonChecked(hwndDlg, IDC_FOLDER_OPTIONS_SAMEWINDOW) == BST_CHECKED);

    IntSetUnderlineState(bUnderline);
    BOOL updateCabinets = IntSetNewWindowMode(bNewWindowMode) == S_OK;
    IntSetShellStateSettings(bDoubleClick, bUseCommonTasks);
    if (updateCabinets)
        PostCabinetMessage(CWM_STATECHANGE, 0, 0);
    return TRUE;
}

INT_PTR CALLBACK
FolderOptionsGeneralDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    static GENERAL_DIALOG general;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            general.hTaskIcon = NULL;
            general.hFolderIcon = NULL;
            general.hClickIcon = NULL;
            return GeneralDlg_OnInitDialog(hwndDlg, &general);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_FOLDER_OPTIONS_COMMONTASKS:
                case IDC_FOLDER_OPTIONS_CLASSICFOLDERS:
                case IDC_FOLDER_OPTIONS_SAMEWINDOW:
                case IDC_FOLDER_OPTIONS_OWNWINDOW:
                case IDC_FOLDER_OPTIONS_SINGLECLICK:
                case IDC_FOLDER_OPTIONS_DOUBLECLICK:
                case IDC_FOLDER_OPTIONS_ULBROWSER:
                case IDC_FOLDER_OPTIONS_ULPOINT:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GeneralDlg_UpdateIcons(hwndDlg, LOWORD(wParam), &general);

                        // Enable the 'Apply' button
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                case IDC_FOLDER_OPTIONS_RESTORE:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GeneralDlg_OnRestoreDefaults(hwndDlg, &general);

                        // Enable the 'Apply' button
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    break;

                case PSN_APPLY:
                    return GeneralDlg_OnApply(hwndDlg, &general);
            }
            break;
        }

        case WM_DESTROY:
            DestroyIcon(general.hTaskIcon);
            DestroyIcon(general.hFolderIcon);
            DestroyIcon(general.hClickIcon);
            break;

        default:
             return FALSE;
    }
    return FALSE;
}
