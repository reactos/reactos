/*
 *  ReactOS
 *  Copyright (C) 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 *
 * PROJECT:         		ReactOS Software Control Panel
 * FILE:            		dll/cpl/appwiz/updates.c
 * PURPOSE:         		ReactOS Software Control Panel
 * PROGRAMMERS:	Gero Kuehn (reactos.filter@gkware.com)
 *				Dmitry Chapyshev (lentind@yandex.ru)
 * UPDATE HISTORY:
 *	06-17-2004  Created
 *	09-25-2007 Modify
 */

#include "appwiz.h"

HWND UpdInfoDialog;

static
INT_PTR CALLBACK
InfoPropDlgProc(HWND hDlg,
               UINT message,
               WPARAM wParam,
               LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (message)
    {
        case WM_INITDIALOG:
        {
            CallInformation(hDlg, UpdInfoDialog, IDC_UPDATESLIST);
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hDlg,LOWORD(wParam));
                break;
            }
        }
        break;
    }

    return FALSE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
UpdatesPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR Buf[256];

    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            AddItemsToViewControl(hwndDlg, IDC_UPD_VIEW_COMBO);
            LoadString(hApplet, IDS_UPDATESLIST, Buf, sizeof(Buf) / sizeof(TCHAR));
            AddListColumn(GetDlgItem(hwndDlg, IDC_UPDATESLIST),Buf);
            FillSoftwareList(hwndDlg, TRUE, IDC_UPDATESLIST);
            UpdInfoDialog = hwndDlg;
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_UPD_FIND_EDIT:
                    if (HIWORD(wParam) == EN_CHANGE)
                    {
                        FindItems(hwndDlg,IDC_UPDATESLIST,IDC_UPD_FIND_EDIT,IDC_UPD_REMOVE,IDC_UPD_INFO_BUTTON);
                    }
                break;
                case ID_UPD_INFORMATION:
                case IDC_UPD_INFO_BUTTON:
                {
                    if (IsItemSelected(hwndDlg, IDC_SOFTWARELIST))
                        DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_INFORMATION),
                              hwndDlg,
                              InfoPropDlgProc);
                }
                break;
                case ID_UPD_REMOVE:
                case IDC_UPD_REMOVE:
                    CallUninstall(hwndDlg, IDC_UPDATESLIST, IDC_UPD_REMOVE, IDC_UPD_INFO_BUTTON, TRUE);
                break;
                case IDC_UPD_VIEW_COMBO:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                        GetCurrentView(hwndDlg, IDC_UPD_VIEW_COMBO, IDC_UPDATESLIST);
                break;
            }
        break;
        case WM_NOTIFY:
            switch (LOWORD(wParam))
            {
                case IDC_UPDATESLIST:
                    switch (((LPNMHDR)lParam)->code)
                    {
                        case NM_DBLCLK:
                            CallUninstall(hwndDlg, IDC_UPDATESLIST, IDC_UPD_REMOVE, IDC_UPD_INFO_BUTTON, TRUE);
                        break;
                        case NM_CLICK:
                            ButtonStatus(hwndDlg, (IsItemSelected(hwndDlg, IDC_UPDATESLIST)), IDC_UPD_REMOVE, IDC_UPD_INFO_BUTTON);
                        break;
                    }
                break;
            }
        break;
        case WM_CONTEXTMENU:
        {
            ShowPopupMenu(hwndDlg,
                          IDR_POPUP_UPD,
                          GET_X_LPARAM(lParam),
                          GET_Y_LPARAM(lParam),
                          IDC_UPDATESLIST);
        }
        break;
    }

    return FALSE;
}
