/*
 *	Gdi handle viewer
 *
 *	mainwnd.c
 *
 *	Copyright (C) 2007	Timo Kreuzer <timo <dot> kreuzer <at> web.de>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "gdihv.h"

INT g_Separator;

INT_PTR CALLBACK
MainWindow_WndProc(HWND hMainWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			RECT rect;

			SendMessage(hMainWnd, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAIN)));
			GetClientRect(hMainWnd, &rect);
			g_Separator = (rect.right / 2);
			HandleList_Create(GetDlgItem(hMainWnd, IDC_HANDLELIST));
			ProcessList_Create(GetDlgItem(hMainWnd, IDC_PROCESSLIST));
			MainWindow_OnSize(hMainWnd);

			break;
		}
		case WM_SIZE:
		{
			MainWindow_OnSize(hMainWnd);
			break;
		}
		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
				{
					EndDialog(hMainWnd, IDOK);
					break;
				}
				case IDC_REFRESHHANDLE:
				{
					LV_ITEM item;
					HWND hProcessListCtrl = GetDlgItem(hMainWnd, IDC_PROCESSLIST);
					memset(&item, 0, sizeof(LV_ITEM));
					item.mask = LVIF_PARAM;
					item.iItem = ListView_GetSelectionMark(hProcessListCtrl);
					(void)ListView_GetItem(hProcessListCtrl, &item);
					HandleList_Update(GetDlgItem(hMainWnd, IDC_HANDLELIST), (HANDLE)item.lParam);
					break;
				}
				case IDC_REFRESHPROCESS:
				{
					ProcessList_Update(GetDlgItem(hMainWnd, IDC_PROCESSLIST));
					break;
				}
				default:
				{
					return FALSE;
				}
			}
			break;
		}
		case WM_NOTIFY:
		{
			if (((LPNMLISTVIEW)lParam)->hdr.code == LVN_ITEMCHANGED
			        && (wParam == IDC_PROCESSLIST)
			        && (((LPNMLISTVIEW)lParam)->uNewState & LVIS_SELECTED)
			        && !(((LPNMLISTVIEW)lParam)->uOldState & LVIS_SELECTED))
			{
				LV_ITEM item;
				memset(&item, 0, sizeof(LV_ITEM));
				item.mask = LVIF_PARAM;
				item.iItem = ((LPNMLISTVIEW)lParam)->iItem;
				(void)ListView_GetItem(GetDlgItem(hMainWnd, IDC_PROCESSLIST), &item);
				HandleList_Update(GetDlgItem(hMainWnd, IDC_HANDLELIST), (HANDLE)item.lParam);
				return TRUE;
				break;
			}
		}
		default:
		{
			return FALSE;
		}
	}
	return TRUE;
}

VOID
MainWindow_OnSize(HWND hMainWnd)
{
	HWND hProcessListctrl, hHandleListCtrl, hProcessRefresh, hHandleRefresh;
	RECT rect;

	hProcessListctrl = GetDlgItem(hMainWnd, IDC_PROCESSLIST);
	hHandleListCtrl = GetDlgItem(hMainWnd, IDC_HANDLELIST);
	hProcessRefresh = GetDlgItem(hMainWnd, IDC_REFRESHPROCESS);
	hHandleRefresh = GetDlgItem(hMainWnd, IDC_REFRESHHANDLE);

	GetClientRect(hMainWnd, &rect);

//g_Separator = (rect.right / 2);
	MoveWindow(hProcessListctrl, 5, 5, g_Separator - 5, rect.bottom - 40, TRUE);
	MoveWindow(hHandleListCtrl, g_Separator + 5, 5, rect.right - g_Separator - 5, rect.bottom - 40, TRUE);
	MoveWindow(hProcessRefresh, g_Separator - 90, rect.bottom - 30, 90, 25, TRUE);
	MoveWindow(hHandleRefresh, rect.right - 90, rect.bottom - 30, 90, 25, TRUE);
	return;
}
