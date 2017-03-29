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

typedef struct _PROPSHEET_INFO
{
    HWND hTaskbarWnd;
    HWND hStartWnd;

    HBITMAP hTaskbarBitmap;
    HBITMAP hTrayBitmap;
    HBITMAP hStartBitmap;
} PROPSHEET_INFO, *PPROPSHEET_INFO;


static BOOL
UpdateBitmaps(PPROPSHEET_INFO pPropInfo)
{
    HWND hwndLock, hwndHide, hwndGroup, hwndShowQL, hwndClock, hwndSeconds, hwndHideInactive, hwndModernStart, hwndClassicStart;
    HWND hwndTaskbarBitmap, hwndTrayBitmap, hwndStartBitmap;
    HWND hwndCustomizeNotifyButton, hwndCustomizeClassicStartButton, hwndCustomizeModernStartButton;
    BOOL bLock, bHide, bGroup, bShowQL, bShowClock, bShowSeconds, bHideInactive;
    LPTSTR lpTaskBarImageName = NULL, lpTrayImageName = NULL, lpStartImageName = NULL;
    BOOL bRet = FALSE;

    hwndLock = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_LOCK);
    hwndHide = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_HIDE);
    hwndGroup = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_GROUP);
    hwndShowQL = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_SHOWQL);
    
    hwndClock = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_CLOCK);
    hwndSeconds = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_SECONDS);
    hwndHideInactive = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_HIDEICONS);
    
    hwndCustomizeNotifyButton = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_ICONCUST);
    
    hwndModernStart = GetDlgItem(pPropInfo->hStartWnd, IDC_TASKBARPROP_STARTMENU);
    hwndClassicStart = GetDlgItem(pPropInfo->hStartWnd, IDC_TASKBARPROP_STARTMENUCLASSIC);
    
    hwndCustomizeClassicStartButton = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_STARTMENUCLASSICCUST);
    hwndCustomizeModernStartButton = GetDlgItem(pPropInfo->hTaskbarWnd, IDC_TASKBARPROP_STARTMENUCUST);
    
    

    if (hwndLock && hwndHide && hwndGroup && hwndShowQL && hwndClock && hwndSeconds && hwndHideInactive)
    {
        bLock = (SendMessage(hwndLock, BM_GETCHECK, 0, 0) == BST_CHECKED);
        bHide = (SendMessage(hwndHide, BM_GETCHECK, 0, 0) == BST_CHECKED);
        bGroup = (SendMessage(hwndGroup, BM_GETCHECK, 0, 0) == BST_CHECKED);
        bShowQL = (SendMessage(hwndShowQL, BM_GETCHECK, 0, 0) == BST_CHECKED);
        
        bShowClock = (SendMessage(hwndClock, BM_GETCHECK, 0, 0) == BST_CHECKED);
        bShowSeconds = (SendMessage(hwndSeconds, BM_GETCHECK, 0, 0) == BST_CHECKED);
        bHideInactive = (SendMessage(hwndHideInactive, BM_GETCHECK, 0, 0) == BST_CHECKED);

        if (bHide)
            lpTaskBarImageName = MAKEINTRESOURCEW(IDB_TASKBARPROP_AUTOHIDE);
        else if (bLock  && bGroup  && bShowQL)
            lpTaskBarImageName = MAKEINTRESOURCEW(IDB_TASKBARPROP_LOCK_GROUP_QL);
        else if (bLock  && !bGroup && !bShowQL)
            lpTaskBarImageName = MAKEINTRESOURCEW(IDB_TASKBARPROP_LOCK_NOGROUP_NOQL);
        else if (bLock  && bGroup  && !bShowQL)
            lpTaskBarImageName = MAKEINTRESOURCEW(IDB_TASKBARPROP_LOCK_GROUP_NOQL);
        else if (bLock  && !bGroup && bShowQL)
            lpTaskBarImageName = MAKEINTRESOURCEW(IDB_TASKBARPROP_LOCK_NOGROUP_QL);
        else if (!bLock && !bGroup && !bShowQL)
            lpTaskBarImageName = MAKEINTRESOURCEW(IDB_TASKBARPROP_NOLOCK_NOGROUP_NOQL);
        else if (!bLock && bGroup  && !bShowQL)
            lpTaskBarImageName = MAKEINTRESOURCEW(IDB_TASKBARPROP_NOLOCK_GROUP_NOQL);
        else if (!bLock && !bGroup && bShowQL)
            lpTaskBarImageName = MAKEINTRESOURCEW(IDB_TASKBARPROP_NOLOCK_NOGROUP_QL);
        else if (!bLock && bGroup  && bShowQL)
            lpTaskBarImageName = MAKEINTRESOURCEW(IDB_TASKBARPROP_NOLOCK_GROUP_QL);

        
        if (lpTaskBarImageName)
        {
            if (pPropInfo->hTaskbarBitmap)
            {
                DeleteObject(pPropInfo->hTaskbarBitmap);
            }

            pPropInfo->hTaskbarBitmap = (HBITMAP)LoadImageW(hExplorerInstance,
                                                  lpTaskBarImageName,
                                                  IMAGE_BITMAP,
                                                  0,
                                                  0,
                                                  LR_DEFAULTCOLOR);
            if (pPropInfo->hTaskbarBitmap)
            {
                hwndTaskbarBitmap = GetDlgItem(pPropInfo->hTaskbarWnd,
                                        IDC_TASKBARPROP_TASKBARBITMAP);
                if (hwndTaskbarBitmap)
                {
                    SendMessage(hwndTaskbarBitmap,
                                STM_SETIMAGE,
                                IMAGE_BITMAP,
                                (LPARAM)pPropInfo->hTaskbarBitmap);
                }
            }
        }
        
        if (bHideInactive)
        {
            EnableWindow(hwndCustomizeNotifyButton, TRUE);
            if (bShowClock)
            {
                EnableWindow(hwndSeconds, TRUE);
                if (bShowSeconds)
                    lpTrayImageName = MAKEINTRESOURCEW(IDB_SYSTRAYPROP_HIDE_SECONDS);
                else
                    lpTrayImageName = MAKEINTRESOURCEW(IDB_SYSTRAYPROP_HIDE_CLOCK);
            }
            else
            {
                SendMessage(hwndSeconds, BM_SETCHECK, BST_UNCHECKED, 0);
                EnableWindow(hwndSeconds, FALSE);
                lpTrayImageName = MAKEINTRESOURCEW(IDB_SYSTRAYPROP_HIDE_NOCLOCK);
            }
        }
        else
        {
            EnableWindow(hwndCustomizeNotifyButton, FALSE);
            if (bShowClock)
            {
                EnableWindow(hwndSeconds, TRUE);
                if (bShowSeconds)
                    lpTrayImageName = MAKEINTRESOURCEW(IDB_SYSTRAYPROP_SHOW_SECONDS);
                else
                    lpTrayImageName = MAKEINTRESOURCEW(IDB_SYSTRAYPROP_SHOW_CLOCK);
            }
            else
            {
                SendMessage(hwndSeconds, BM_SETCHECK, BST_UNCHECKED, 0);
                EnableWindow(hwndSeconds, FALSE);
                lpTrayImageName = MAKEINTRESOURCEW(IDB_SYSTRAYPROP_SHOW_NOCLOCK);
            }
        }
        
        if (lpTrayImageName)
        {
            if (pPropInfo->hTrayBitmap)
            {
                DeleteObject(pPropInfo->hTrayBitmap);
            }

            pPropInfo->hTrayBitmap = (HBITMAP)LoadImageW(hExplorerInstance,
                                                  lpTrayImageName,
                                                  IMAGE_BITMAP,
                                                  0,
                                                  0,
                                                  LR_DEFAULTCOLOR);
            if (pPropInfo->hTrayBitmap)
            {
                hwndTrayBitmap = GetDlgItem(pPropInfo->hTaskbarWnd,
                                        IDC_TASKBARPROP_NOTIFICATIONBITMAP);
                if (hwndTrayBitmap)
                {
                    SendMessage(hwndTrayBitmap,
                                STM_SETIMAGE,
                                IMAGE_BITMAP,
                                (LPARAM)pPropInfo->hTrayBitmap);
                }
            }
        }
    }
    
    if(hwndClassicStart && hwndModernStart)
    {
        if(SendMessage(hwndModernStart, BM_GETCHECK, 0, 0) == BST_CHECKED)
        {
            EnableWindow(hwndCustomizeModernStartButton, TRUE);
            EnableWindow(hwndCustomizeClassicStartButton, FALSE);
            lpStartImageName = MAKEINTRESOURCEW(IDB_STARTPREVIEW);
        }
        else
        {
            EnableWindow(hwndCustomizeModernStartButton, FALSE);
            EnableWindow(hwndCustomizeClassicStartButton, TRUE);
            lpStartImageName = MAKEINTRESOURCEW(IDB_STARTPREVIEW_CLASSIC);
        }
        
         if (lpStartImageName)
        {
            if (pPropInfo->hStartBitmap)
            {
                DeleteObject(pPropInfo->hStartBitmap);
            }

            pPropInfo->hStartBitmap = (HBITMAP)LoadImageW(hExplorerInstance,
                                                  lpStartImageName,
                                                  IMAGE_BITMAP,
                                                  0,
                                                  0,
                                                  LR_DEFAULTCOLOR);
            if (pPropInfo->hStartBitmap)
            {
                hwndStartBitmap = GetDlgItem(pPropInfo->hStartWnd,
                                        IDC_TASKBARPROP_STARTMENU_BITMAP);
                if (hwndStartBitmap)
                {
                    SendMessage(hwndStartBitmap,
                                STM_SETIMAGE,
                                IMAGE_BITMAP,
                                (LPARAM)pPropInfo->hStartBitmap);
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
    CheckDlgButton(hwnd, IDC_TASKBARPROP_SECONDS, AdvancedSettings.bShowSeconds ? BST_CHECKED : BST_UNCHECKED);

    UpdateBitmaps(pPropInfo);
}

static VOID
OnCreateStartPage(HWND hwnd,
                    PPROPSHEET_INFO pPropInfo)
{
    pPropInfo->hStartWnd = hwnd;

    CheckDlgButton(hwnd, IDC_TASKBARPROP_STARTMENUCLASSIC, 1);    // HACK: This has to be read from registry!
    
    UpdateBitmaps(pPropInfo);
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
            OnCreateTaskbarPage(hwndDlg, (PPROPSHEET_INFO)((LPPROPSHEETPAGE)lParam)->lParam);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_TASKBARPROP_LOCK:
                case IDC_TASKBARPROP_HIDE:
                case IDC_TASKBARPROP_GROUP:
                case IDC_TASKBARPROP_SHOWQL:
                case IDC_TASKBARPROP_HIDEICONS:
                case IDC_TASKBARPROP_CLOCK:
                case IDC_TASKBARPROP_SECONDS:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        UpdateBitmaps(pPropInfo);

                        /* Enable the 'Apply' button */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
                case IDC_TASKBARPROP_ICONCUST:
                    ShowCustomizeNotifyIcons(hExplorerInstance, hwndDlg);
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
            if (pPropInfo->hTaskbarBitmap)
            {
                DeleteObject(pPropInfo->hTaskbarBitmap);
            }
            if (pPropInfo->hTrayBitmap)
            {
                DeleteObject(pPropInfo->hTrayBitmap);
            }
            break;

HandleDefaultMessage:
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
            OnCreateStartPage(hwndDlg, (PPROPSHEET_INFO)((LPPROPSHEETPAGE)lParam)->lParam);
            break;
            
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDC_TASKBARPROP_STARTMENUCLASSICCUST:
                    ShowCustomizeClassic(hExplorerInstance, hwndDlg);
                    break;
            }
            break;
        }

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
    ZeroMemory(psp, sizeof(*psp));
    psp->dwSize = sizeof(*psp);
    psp->dwFlags = PSP_DEFAULT;
    psp->hInstance = hExplorerInstance;
    psp->pszTemplate = MAKEINTRESOURCEW(idDlg);
    psp->lParam = lParam;
    psp->pfnDlgProc = DlgProc;
}


VOID
DisplayTrayProperties(IN HWND hwndOwner)
{
    PROPSHEET_INFO propInfo = {0};
    PROPSHEETHEADER psh;
    PROPSHEETPAGE psp[2];
    WCHAR szCaption[256];

    if (!LoadStringW(hExplorerInstance,
                     IDS_TASKBAR_STARTMENU_PROP_CAPTION,
                     szCaption,
                     _countof(szCaption)))
    {
        return;
    }

    ZeroMemory(&psh, sizeof(psh));
    psh.dwSize = sizeof(psh);
    psh.dwFlags =  PSH_PROPSHEETPAGE | PSH_PROPTITLE;
    psh.hwndParent = hwndOwner;
    psh.hInstance = hExplorerInstance;
    psh.hIcon = NULL;
    psh.pszCaption = szCaption;
    psh.nPages = _countof(psp);
    psh.nStartPage = 0;
    psh.ppsp = psp;

    InitPropSheetPage(&psp[0], IDD_TASKBARPROP_TASKBAR, TaskbarPageProc, (LPARAM)&propInfo);
    InitPropSheetPage(&psp[1], IDD_TASKBARPROP_STARTMENU, StartMenuPageProc, (LPARAM)&propInfo);

    PropertySheet(&psh);
}
