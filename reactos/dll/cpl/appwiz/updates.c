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

/* Property page dialog callback */
INT_PTR CALLBACK
UpdatesPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(hwndDlg);
	TCHAR Buf[256];

    switch (uMsg)
    {
        case WM_INITDIALOG:
			AddItemsToViewControl(hwndDlg);
			LoadString(hApplet, IDS_UPDATESLIST, Buf, sizeof(Buf) / sizeof(TCHAR));
			AddListColumn(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),Buf);
			FillSoftwareList(hwndDlg, TRUE);
        break;
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
				case IDC_FIND_EDIT:
					if (HIWORD(wParam) == EN_CHANGE)
					{
					    FindItems(hwndDlg);
					}
				break;
				case ID_UPD_INFORMATION:
				case IDC_INFO_BUTTON:
					DialogBox(hApplet,
							  MAKEINTRESOURCE(IDD_INFORMATION),
							  hwndDlg,
							  InfoPropDlgProc);
				break;
				case ID_UPD_REMOVE:
				case IDC_ADDREMOVE:
                    CallUninstall(hwndDlg, IDC_SOFTWARELIST, TRUE);
                break;
				case IDC_VIEW_COMBO:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						GetCurrentView(hwndDlg);
					}
				break;
			}
		break;
		case WM_NOTIFY:
			switch (LOWORD(wParam))
			{
                case IDC_SOFTWARELIST:
					switch (((LPNMHDR)lParam)->code)
					{
						case NM_DBLCLK:
							CallUninstall(hwndDlg, IDC_SOFTWARELIST, TRUE);
						break;
						case NM_CLICK:
						{
							INT nIndex;
							nIndex = (INT)SendMessage(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
							if (nIndex == -1)
							{
								EnableWindow(GetDlgItem(hwndDlg, IDC_ADDREMOVE),FALSE);
								EnableWindow(GetDlgItem(hwndDlg, IDC_INFO_BUTTON),FALSE);
							}
							else
							{
								EnableWindow(GetDlgItem(hwndDlg, IDC_ADDREMOVE),TRUE);
								EnableWindow(GetDlgItem(hwndDlg, IDC_INFO_BUTTON),TRUE);
							}
						}
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
						  GET_Y_LPARAM(lParam));
		}
		break;
    }

    return FALSE;
}
