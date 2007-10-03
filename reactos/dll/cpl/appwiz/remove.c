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
 * FILE:            		dll/cpl/appwiz/appwiz.c
 * PURPOSE:         		ReactOS Software Control Panel
 * PROGRAMMERS:	Gero Kuehn (reactos.filter@gkware.com)
 *				Dmitry Chapyshev (lentind@yandex.ru)
 * UPDATE HISTORY:
 *	06-17-2004  Created
 *	09-25-2007 Modify
 */

#include <windows.h>
#include <commctrl.h>
#include <cpl.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <tchar.h>
#include <process.h>

#include "resource.h"
#include "appwiz.h"

static VOID
CallUninstall(HWND hwndDlg)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    INT nIndex;
    HKEY hKey;
    DWORD dwType;
    TCHAR pszUninstallString[MAX_PATH];
    DWORD dwSize;
	TCHAR Buf[256],Title[256];
	
	nIndex = (INT)SendMessage(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
    if (nIndex == -1)
    {
		LoadString(hApplet, IDS_NOITEM_SELECTED, Buf, sizeof(Buf) / sizeof(TCHAR));
		LoadString(hApplet, IDS_CPLSYSTEMNAME, Title, sizeof(Title) / sizeof(TCHAR));
        MessageBox(hwndDlg,
                   Buf,
                   Title,
                   MB_ICONINFORMATION);
    }
    else
    {
		LVITEM item;
		
		ZeroMemory(&item, sizeof(LVITEM));
		item.mask = LVIF_PARAM;
		item.iItem = nIndex;
		(void)ListView_GetItem(GetDlgItem(hwndDlg,IDC_SOFTWARELIST),&item);
        hKey = (HKEY)item.lParam;

        dwType = REG_SZ;
        dwSize = MAX_PATH;
        if (RegQueryValueEx(hKey,
                            _TEXT("UninstallString"),
                            NULL,
                            &dwType,
                            (LPBYTE)pszUninstallString,
                            &dwSize) == ERROR_SUCCESS)
        {
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.wShowWindow = SW_SHOW;
            if (CreateProcess(NULL,pszUninstallString,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi))
            {
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
            }
        }
        else
        {
			LoadString(hApplet, IDS_UNABLEREAD_UNINSTSTR, Buf, sizeof(Buf) / sizeof(TCHAR));
			LoadString(hApplet, IDS_ERROR, Title, sizeof(Title) / sizeof(TCHAR));
            MessageBox(hwndDlg,
                       Buf,
                       Title,
                       MB_ICONSTOP);
        }
    }
}

static VOID
AddListColumn(HWND hList, LPTSTR Caption)
{
	LV_COLUMN dummy;
	RECT rect;

    GetClientRect(hList, &rect);
    ZeroMemory(&dummy, sizeof(LV_COLUMN));
    dummy.mask      = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    dummy.iSubItem  = 0;
	dummy.pszText 	= Caption;
    dummy.cx        = rect.right - rect.left - GetSystemMetrics(SM_CXVSCROLL);
    (void)ListView_InsertColumn(hList, 0, &dummy);
}

static VOID
AddItemToList(HWND hwndDlg, LPARAM hSubKey, LPTSTR pszDisplayName, INT ItemIndex)
{
	int index;
	HIMAGELIST hImgListSmall;
	HIMAGELIST hImgListLarge;
	HICON hIcon;
	HWND hList;
	LV_ITEM listItem;
	int ColorDepth;
	DEVMODE pDevMode;
	
	/* Icon drawing mode */
	pDevMode.dmSize = sizeof(DEVMODE);
	pDevMode.dmDriverExtra = 0;
	EnumDisplaySettings(NULL,ENUM_CURRENT_SETTINGS,&pDevMode);
	switch (pDevMode.dmBitsPerPel)
	{
		case 32: ColorDepth = ILC_COLOR32; break;
		case 24: ColorDepth = ILC_COLOR24; break;
		case 16: ColorDepth = ILC_COLOR16; break;
		case  8: ColorDepth = ILC_COLOR8;  break;
		case  4: ColorDepth = ILC_COLOR4;  break;
		default: ColorDepth = ILC_COLOR;   break;
	}

	hImgListSmall = ImageList_Create(16,16,ColorDepth | ILC_MASK,5,5);
	hImgListLarge = ImageList_Create(32,32,ColorDepth | ILC_MASK,5,5);

	hIcon = LoadImage(hApplet,MAKEINTRESOURCE(IDI_CPLSYSTEM),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
	index = ImageList_AddIcon(hImgListSmall,hIcon);
	DestroyIcon(hIcon);
	hIcon = LoadImage(hApplet,MAKEINTRESOURCE(IDI_CPLSYSTEM),IMAGE_ICON,32,32,LR_DEFAULTCOLOR);
	ImageList_AddIcon(hImgListLarge,hIcon);
	DestroyIcon(hIcon);
	
	hList = GetDlgItem(hwndDlg, IDC_SOFTWARELIST);
	
	ZeroMemory(&listItem, sizeof(LV_ITEM));
	listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
	listItem.pszText    = (LPTSTR)pszDisplayName;
	listItem.lParam     = (LPARAM)hSubKey;
	listItem.iItem      = (int)ItemIndex;
	listItem.iImage     = index;
	(void)ListView_InsertItem(hList, &listItem);
	
	(void)ListView_SetImageList(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),hImgListSmall,LVSIL_SMALL);
	(void)ListView_SetImageList(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),hImgListLarge,LVSIL_NORMAL);

}

static VOID
FillSoftwareList(HWND hwndDlg)
{
    TCHAR pszName[MAX_PATH];
    TCHAR pszDisplayName[MAX_PATH];
    TCHAR pszParentKeyName[MAX_PATH];
    FILETIME FileTime;
    HKEY hKey;
    HKEY hSubKey;
    DWORD dwType;
    DWORD dwSize = MAX_PATH;
    DWORD dwValue = 0;
    BOOL bIsUpdate = FALSE;
    BOOL bIsSystemComponent = FALSE;
    BOOL bShowUpdates = FALSE;
    INT ItemIndex = 0;
	TCHAR Buf[256],Title[256];
	
    bShowUpdates = (SendMessage(GetDlgItem(hwndDlg, IDC_SHOWUPDATES), BM_GETCHECK, 0, 0) == BST_CHECKED);

    if (RegOpenKey(HKEY_LOCAL_MACHINE,
                   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
                   &hKey) != ERROR_SUCCESS)
    {
		LoadString(hApplet, IDS_UNABLEOPEN_UNINSTKEY, Buf, sizeof(Buf) / sizeof(TCHAR));
		LoadString(hApplet, IDS_ERROR, Title, sizeof(Title) / sizeof(TCHAR));
        MessageBox(hwndDlg,
                   Buf,
                   Title,
                   MB_ICONSTOP);
        return;
    }

    ItemIndex = 0;
    dwSize = MAX_PATH;
    while (RegEnumKeyEx(hKey, ItemIndex, pszName, &dwSize, NULL, NULL, NULL, &FileTime) == ERROR_SUCCESS)
	{
        if (RegOpenKey(hKey,pszName,&hSubKey) == ERROR_SUCCESS)
        {
            dwType = REG_DWORD;
            dwSize = sizeof(DWORD);
            if (RegQueryValueEx(hSubKey,
                                TEXT("SystemComponent"),
                                NULL,
                                &dwType,
                                (LPBYTE)&dwValue,
                                &dwSize) == ERROR_SUCCESS)
            {
                bIsSystemComponent = (dwValue == 0x1);
            }
            else
            {
                bIsSystemComponent = FALSE;
            }

            dwType = REG_SZ;
            dwSize = MAX_PATH;
            bIsUpdate = (RegQueryValueEx(hSubKey,
                                         TEXT("ParentKeyName"),
                                         NULL,
                                         &dwType,
                                         (LPBYTE)pszParentKeyName,
                                         &dwSize) == ERROR_SUCCESS);
			dwSize = MAX_PATH;
            if (RegQueryValueEx(hSubKey,
                                TEXT("DisplayName"),
                                NULL,
                                &dwType,
                                (LPBYTE)pszDisplayName,
                                &dwSize) == ERROR_SUCCESS)
            {
                if ((!bIsUpdate) && (!bIsSystemComponent))
                {
					AddItemToList(hwndDlg, (LPARAM)hSubKey, (LPTSTR)pszDisplayName, ItemIndex);
                }
                else if (bIsUpdate && bShowUpdates)
                {
					AddItemToList(hwndDlg, (LPARAM)hSubKey, (LPTSTR)pszDisplayName, ItemIndex);
                }
            }
        }

        dwSize = MAX_PATH;
        ItemIndex++;
    }

    RegCloseKey(hKey);
}

static VOID
AddItemsToViewControl(HWND hwndDlg)
{
	TCHAR Buf[256];
	int Index;
	HWND hList;
	
	hList = GetDlgItem(hwndDlg, IDC_VIEW_COMBO);
	// Large Icons
	LoadString(hApplet, IDS_LARGEICONS, Buf, sizeof(Buf) / sizeof(TCHAR));
    Index = (int)SendMessage(hList,
							CB_ADDSTRING,
							0,
							(LPARAM)Buf);
    SendMessage(hList,
                CB_SETITEMDATA,
                Index,
                (LPARAM)Index);
	// List
	LoadString(hApplet, IDS_LIST, Buf, sizeof(Buf) / sizeof(TCHAR));
    Index = (int)SendMessage(hList,
							CB_ADDSTRING,
							0,
							(LPARAM)Buf);
    SendMessage(hList,
                CB_SETITEMDATA,
                Index,
                (LPARAM)Index);
	// Details
	LoadString(hApplet, IDS_DETAILS, Buf, sizeof(Buf) / sizeof(TCHAR));
    Index = (int)SendMessage(hList,
							CB_ADDSTRING,
							0,
							(LPARAM)Buf);
    SendMessage(hList,
                CB_SETITEMDATA,
                Index,
                (LPARAM)Index);
				
	// Select string
	SendMessage(hList,
                CB_SELECTSTRING,
                -1,
                (LPARAM)Buf);
}

static VOID
FindItems(HWND hwndDlg)
{
	HWND hList;
	HWND hEdit;
	TCHAR szText[1024], szItemText[1024];
	INT Index,i;
	LV_ITEM listItem;
	BOOL comp = TRUE;
	
	hList = GetDlgItem(hwndDlg, IDC_SOFTWARELIST);
	hEdit = GetDlgItem(hwndDlg, IDC_FIND_EDIT);
	
	SendMessage(hEdit, WM_GETTEXT, 128, (LPARAM)szText);
	
	ZeroMemory(&listItem, sizeof(LV_ITEM));
	listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
	listItem.pszText    = (LPTSTR)szText;
	listItem.iImage     = -1;
	Index = ListView_InsertItem(hList, &listItem);
	
	SendMessage(hList, LVM_DELETEITEM, Index, 0);
	ListView_GetItemText(hList, Index, 0, (LPTSTR)szItemText, 1024);
	
	for (i = 0; i < strlen((char*)szText); i++)
	{
		if (szText[i] != szItemText[i]) comp = FALSE;
	}
	ListView_SetItemState(hList, Index, LVIS_SELECTED | LVIS_FOCUSED, -1);
	if (comp)
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_ADDREMOVE),TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_ADDREMOVE),FALSE);
	}
}

/* Property page dialog callback */
INT_PTR CALLBACK
RemovePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
	TCHAR Buf[256];

    switch (uMsg)
    {
        case WM_INITDIALOG:
			AddItemsToViewControl(hwndDlg);
			LoadString(hApplet, IDS_APPLIST, Buf, sizeof(Buf) / sizeof(TCHAR));
			AddListColumn(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),Buf);
			FillSoftwareList(hwndDlg);
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
                case IDC_SHOWUPDATES:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
						SendMessage(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),LVM_DELETEALLITEMS,0,0);
                        FillSoftwareList(hwndDlg);
                    }
                break;
                case IDC_ADDREMOVE:
                    CallUninstall(hwndDlg);
                break;
				case IDC_VIEW_COMBO:
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						int nCurrSel;
						nCurrSel = (int)SendMessage(GetDlgItem(hwndDlg, IDC_VIEW_COMBO),
													CB_GETCURSEL,
													(WPARAM)0,
													(LPARAM)0);
						switch (nCurrSel)
						{
							case 0:
								SetWindowLong(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),
											  GWL_STYLE, LVS_ICON | LVS_SORTASCENDING | LVS_AUTOARRANGE | LVS_SINGLESEL | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP);
							break;
							case 1:
								SetWindowLong(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),
											  GWL_STYLE,LVS_LIST | LVS_SORTASCENDING | LVS_AUTOARRANGE | LVS_SINGLESEL | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP);
							break;
							case 2:
								SetWindowLong(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),
											  GWL_STYLE,LVS_REPORT | LVS_SORTASCENDING | LVS_AUTOARRANGE | LVS_SINGLESEL | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP);
							break;
						}
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
							CallUninstall(hwndDlg);
						break;
						case NM_CLICK:
						{
							INT nIndex;
							nIndex = (INT)SendMessage(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
							if (nIndex == -1)
							{
								EnableWindow(GetDlgItem(hwndDlg, IDC_ADDREMOVE),FALSE);
							}
							else
							{
								EnableWindow(GetDlgItem(hwndDlg, IDC_ADDREMOVE),TRUE);
							}
						}
						break;
					}
                break;
			}
		break;
    }
	
    return FALSE;
}
