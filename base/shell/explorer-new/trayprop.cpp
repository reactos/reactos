/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
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

typedef struct _PROPSHEET_INFO
{
    HWND hTaskbarWnd;
    HWND hStartWnd;
    HWND hNotiWnd;
    HWND hToolWnd;

    HBITMAP hTaskbarBitmap;
} PROPSHEET_INFO, *PPROPSHEET_INFO;


static BOOL
UpdateTaskbarBitmap(PPROPSHEET_INFO pPropInfo)
{
    HWND hwndLock, hwndHide, hwndGroup, hwndShowQL;
    HWND hwndBitmap;
    BOOL bLock, bHide, bGroup, bShowQL;
    LPTSTR lpImageName = NULL;
    BOOL bRet = FALSE;

    hwndLock = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_LOCK);
    hwndHide = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_HIDE);
    hwndGroup = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_GROUP);
    hwndShowQL = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_SHOWQL);

    if (hwndLock && hwndHide && hwndGroup && hwndShowQL)
    {
        bLock = (SendMessage(hwndLock, BM_GETCHECK, 0, 0) == BST_CHECKED);
        bHide = (SendMessage(hwndHide, BM_GETCHECK, 0, 0) == BST_CHECKED);
        bGroup = (SendMessage(hwndGroup, BM_GETCHECK, 0, 0) == BST_CHECKED);
        bShowQL = (SendMessage(hwndShowQL, BM_GETCHECK, 0, 0) == BST_CHECKED);

        if (bHide)
            lpImageName = MAKEINTRESOURCE(IDB_TASKBARPROP_AUTOHIDE);
        else if (bLock  && bGroup  && bShowQL)
            lpImageName = MAKEINTRESOURCE(IDB_TASKBARPROP_LOCK_GROUP_QL);
        else if (bLock  && !bGroup && !bShowQL)
            lpImageName = MAKEINTRESOURCE(IDB_TASKBARPROP_LOCK_NOGROUP_NOQL);
        else if (bLock  && bGroup  && !bShowQL)
            lpImageName = MAKEINTRESOURCE(IDB_TASKBARPROP_LOCK_GROUP_NOQL);
        else if (bLock  && !bGroup && bShowQL)
            lpImageName = MAKEINTRESOURCE(IDB_TASKBARPROP_LOCK_NOGROUP_QL);
        else if (!bLock && !bGroup && !bShowQL)
            lpImageName = MAKEINTRESOURCE(IDB_TASKBARPROP_NOLOCK_NOGROUP_NOQL);
        else if (!bLock && bGroup  && !bShowQL)
            lpImageName = MAKEINTRESOURCE(IDB_TASKBARPROP_NOLOCK_GROUP_NOQL);
        else if (!bLock && !bGroup && bShowQL)
            lpImageName = MAKEINTRESOURCE(IDB_TASKBARPROP_NOLOCK_NOGROUP_QL);
        else if (!bLock && bGroup  && bShowQL)
            lpImageName = MAKEINTRESOURCE(IDB_TASKBARPROP_NOLOCK_GROUP_QL);

        if (lpImageName)
        {
            if (pPropInfo->hTaskbarBitmap)
            {
                DeleteObject(pPropInfo->hTaskbarBitmap);
            }

            pPropInfo->hTaskbarBitmap = (HBITMAP)LoadImage(hExplorerInstance,
                                                  lpImageName,
                                                  IMAGE_BITMAP,
                                                  0,
                                                  0,
                                                  LR_DEFAULTCOLOR);
            if (pPropInfo->hTaskbarBitmap)
            {
                hwndBitmap = GetDlgItem(pPropInfo->hTaskbarWnd,
                                        IDC_TASKBARPROP_TASKBARBITMAP);
                if (hwndBitmap)
                {
                    SendMessage(hwndBitmap,
                                STM_SETIMAGE,
                                IMAGE_BITMAP,
                                (LPARAM)pPropInfo->hTaskbarBitmap);
                }
            }
        }
    }

    return bRet;
}

static VOID
OnCreateTaskbarPage(HWND hwnd,
                    PPROPSHEET_INFO pPropInfo)
{
    SetWindowLongPtr(hwnd,
                     GWLP_USERDATA,
                     (LONG_PTR)pPropInfo);

    pPropInfo->hTaskbarWnd = hwnd;

    // FIXME: check buttons

    UpdateTaskbarBitmap(pPropInfo);
}

INT_PTR CALLBACK
TaskbarPageProc(HWND hwndDlg,
                UINT uMsg,
                WPARAM wParam,
                LPARAM lParam)
{
    PPROPSHEET_INFO pPropInfo;

    /* Get the window context */
    pPropInfo = (PPROPSHEET_INFO)GetWindowLongPtrW(hwndDlg,
                                                   GWLP_USERDATA);
    if (pPropInfo == NULL && uMsg != WM_INITDIALOG)
    {
        goto HandleDefaultMessage;
    }

    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnCreateTaskbarPage(hwndDlg, (PPROPSHEET_INFO)lParam);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_TASKBARPROP_LOCK:
                case IDC_TASKBARPROP_HIDE:
                case IDC_TASKBARPROP_GROUP:
                case IDC_TASKBARPROP_SHOWQL:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        UpdateTaskbarBitmap(pPropInfo);

                        /* Enable the 'Apply' button */
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
            if (pPropInfo->hTaskbarBitmap)
            {
                DeleteObject(pPropInfo->hTaskbarBitmap);
            }
            break;

HandleDefaultMessage:
        default:
            return FALSE;
    }

    return FALSE;
}

static INT_PTR CALLBACK
AdvancedSettingsPageProc(HWND hwndDlg,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            CheckDlgButton(hwndDlg, IDC_TASKBARPROP_SECONDS, AdvancedSettings.bShowSeconds ? BST_CHECKED : BST_UNCHECKED);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_TASKBARPROP_SECONDS:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
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
                    AdvancedSettings.bShowSeconds = IsDlgButtonChecked(hwndDlg, IDC_TASKBARPROP_SECONDS);
                    SaveSettingDword(szAdvancedSettingsKey, TEXT("ShowSeconds"), AdvancedSettings.bShowSeconds);
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

static INT_PTR CALLBACK
StartMenuPageProc(HWND hwndDlg,
                  UINT uMsg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;

        case WM_DESTROY:
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
    }

    return FALSE;
}


static INT_PTR CALLBACK
NotificationPageProc(HWND hwndDlg,
                     UINT uMsg,
                     WPARAM wParam,
                     LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;

        case WM_DESTROY:
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
    }

    return FALSE;
}


static INT_PTR CALLBACK
ToolbarsPageProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;

        case WM_DESTROY:
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
    }

    return FALSE;
}


static VOID
InitPropSheetPage(PROPSHEETPAGE *psp,
                  WORD idDlg,
                  DLGPROC DlgProc,
                  LPARAM lParam)
{
    ZeroMemory(psp, sizeof(PROPSHEETPAGE));
    psp->dwSize = sizeof(PROPSHEETPAGE);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hExplorerInstance;
    psp->pszTemplate = MAKEINTRESOURCE(idDlg);
    psp->lParam = lParam;
    psp->pfnDlgProc = DlgProc;
}


VOID
DisplayTrayProperties(IN HWND hwndOwner)
{
    PROPSHEET_INFO propInfo;
    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp[5];
    WCHAR szCaption[256];

    if (!LoadString(hExplorerInstance,
                    IDS_TASKBAR_STARTMENU_PROP_CAPTION,
                    szCaption,
                    sizeof(szCaption) / sizeof(szCaption[0])))
    {
        return;
    }

    ZeroMemory(&psh, sizeof(PROPSHEETHEADER));
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hExplorerInstance;
    psh.hIcon = NULL;
    psh.pszCaption = szCaption;
    psh.nPages = sizeof(psp) / sizeof(PROPSHEETPAGE);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_TASKBARPROP_TASKBAR, TaskbarPageProc, (LPARAM)&propInfo);
    InitPropSheetPage(&psp[1], IDD_TASKBARPROP_STARTMENU, StartMenuPageProc, (LPARAM)&propInfo);
    InitPropSheetPage(&psp[2], IDD_TASKBARPROP_NOTIFICATION, NotificationPageProc, (LPARAM)&propInfo);
    InitPropSheetPage(&psp[3], IDD_TASKBARPROP_TOOLBARS, ToolbarsPageProc, (LPARAM)&propInfo);
    InitPropSheetPage(&psp[4], IDD_TASKBARPROP_ADVANCED, AdvancedSettingsPageProc, (LPARAM)&propInfo);

    PropertySheet(&psh);
}
