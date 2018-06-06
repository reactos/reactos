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

static VOID
GeneralDlg_UpdateIcons(HWND hDlg)
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

    // Clean up
    if (hTaskIcon)
        DeleteObject(hTaskIcon);
    if (hFolderIcon)
        DeleteObject(hFolderIcon);
    if (hClickIcon)
        DeleteObject(hClickIcon);
}

INT_PTR CALLBACK
FolderOptionsGeneralDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            // FIXME
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_FOLDER_OPTIONS_COMMONTASKS:
                case IDC_FOLDER_OPTIONS_CLASSICFOLDERS:
                case IDC_FOLDER_OPTIONS_SAMEWINDOW:
                case IDC_FOLDER_OPTIONS_OWNWINDOW:
                case IDC_FOLDER_OPTIONS_SINGLECLICK:
                case IDC_FOLDER_OPTIONS_DOUBLECLICK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        GeneralDlg_UpdateIcons(hwndDlg);

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
                    break;
            }
            break;
        }

        case WM_DESTROY:
            break;

        default:
             return FALSE;
    }
    return FALSE;
}
