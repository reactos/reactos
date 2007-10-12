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
 * FILE:            		dll/cpl/appwiz/remove.c
 * PURPOSE:         		ReactOS Software Control Panel
 * PROGRAMMERS:	Gero Kuehn (reactos.filter@gkware.com)
 *				Dmitry Chapyshev (lentind@yandex.ru)
 * UPDATE HISTORY:
 *	06-17-2004  Created
 *	09-25-2007 Modify
 */

#include "appwiz.h"

HWND InfoDialog;

BOOL
GetInfoItem(HWND hwndDlg, HKEY hKey, LPCTSTR RegName, UINT Control)
{
	DWORD dwSize = 1024;
	TCHAR pszInfoString[1024];

    if (RegQueryValueEx(hKey,
                        RegName,
                        NULL,
                        NULL,
                        (LPBYTE)pszInfoString,
                        &dwSize) == ERROR_SUCCESS)
    {
		SendMessage(GetDlgItem(hwndDlg, Control), WM_SETTEXT, -1, (LPARAM)pszInfoString);
		return TRUE;
    }
	else
	{
		dwSize = 1024;
		LoadString(hApplet, IDS_NO_INFORMATION, pszInfoString, sizeof(pszInfoString) / sizeof(TCHAR));
		SendMessage(GetDlgItem(hwndDlg, Control), WM_SETTEXT, -1, (LPARAM)pszInfoString);
		return FALSE;
	}
}

VOID
CallInformation(HWND hwndDlg, HWND infDlg, UINT Control)
{
    INT nIndex;
    HKEY hKey;
	TCHAR Buf[256],Title[256];
	
	nIndex = (INT)SendMessage(GetDlgItem(infDlg, Control),LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
    if (nIndex != -1)
    {
		LVITEM item;
		
		ZeroMemory(&item, sizeof(LVITEM));
		item.mask = LVIF_PARAM;
		item.iItem = nIndex;
		(void)ListView_GetItem(GetDlgItem(infDlg,Control),&item);
        hKey = (HKEY)item.lParam;

		if (!GetInfoItem(hwndDlg, hKey, L"DisplayName", IDS_INFO_DISPNAME))
        {
			LoadString(hApplet, IDS_UNABLEREAD_INFORMATION, Buf, sizeof(Buf) / sizeof(TCHAR));
			LoadString(hApplet, IDS_ERROR, Title, sizeof(Title) / sizeof(TCHAR));
            MessageBox(hwndDlg,
                       Buf,
                       Title,
                       MB_ICONSTOP);
			return;
        }
		
		(void)GetInfoItem(hwndDlg, hKey, L"RegOwner", 		IDS_INFO_REGOWNER);
		(void)GetInfoItem(hwndDlg, hKey, L"ProductID", 		IDS_INFO_PRODUCTID);
		(void)GetInfoItem(hwndDlg, hKey, L"Publisher", 		IDS_INFO_PUBLISHER);
		(void)GetInfoItem(hwndDlg, hKey, L"DisplayVersion", IDS_INFO_VERSION);
		(void)GetInfoItem(hwndDlg, hKey, L"Contact", 		IDS_INFO_CONTACT);
		(void)GetInfoItem(hwndDlg, hKey, L"HelpLink", 		IDS_INFO_SUPPORTINFO);
		(void)GetInfoItem(hwndDlg, hKey, L"HelpTelephone", 	IDS_INFO_SUPPORTPHONE);
		(void)GetInfoItem(hwndDlg, hKey, L"URLUpdateInfo", 	IDS_INFO_PRODUCT_UPDATES);
		(void)GetInfoItem(hwndDlg, hKey, L"Readme", 		IDS_INFO_README);
		(void)GetInfoItem(hwndDlg, hKey, L"Comments", 		IDS_INFO_COMMENTS);
    }
}

VOID
CallUninstall(HWND hwndDlg, UINT Control, UINT RemBtn, UINT InfoBtn, BOOL isUpdate)
{
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    INT nIndex;
    HKEY hKey;
    DWORD dwType;
    TCHAR pszUninstallString[MAX_PATH];
    DWORD dwSize;
	TCHAR Buf[256],Title[256];
	
	nIndex = (INT)SendMessage(GetDlgItem(hwndDlg, Control),LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
    if (nIndex != -1)
    {
		LVITEM item;
		
		ZeroMemory(&item, sizeof(LVITEM));
		item.mask = LVIF_PARAM;
		item.iItem = nIndex;
		(void)ListView_GetItem(GetDlgItem(hwndDlg,Control),&item);
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
				ButtonStatus(hwndDlg, FALSE, RemBtn, InfoBtn);
				WaitForSingleObject(pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
				// Update software list
				(void)ListView_DeleteAllItems(GetDlgItem(hwndDlg, Control));
				FillSoftwareList(hwndDlg, isUpdate, Control);
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

VOID
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
AddItemToList(HWND hwndDlg, LPARAM hSubKey, LPTSTR pszDisplayName, INT ItemIndex, UINT Control)
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
	
	hList = GetDlgItem(hwndDlg, Control);
	
	ZeroMemory(&listItem, sizeof(LV_ITEM));
	listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
	listItem.pszText    = (LPTSTR)pszDisplayName;
	listItem.lParam     = (LPARAM)hSubKey;
	listItem.iItem      = (int)ItemIndex;
	listItem.iImage     = index;
	(void)ListView_InsertItem(hList, &listItem);
	
	(void)ListView_SetImageList(hList,hImgListSmall,LVSIL_SMALL);
	(void)ListView_SetImageList(hList,hImgListLarge,LVSIL_NORMAL);
}

VOID
SetNoneAppMsg(HWND hwndDlg, BOOL IsUpdates)
{
	TCHAR Buf[256];
	
	if (IsUpdates)
	{
		LoadString(hApplet, IDS_NONE_UPD, Buf, sizeof(Buf) / sizeof(TCHAR));
		AddItemToList(hwndDlg, -1, (LPTSTR)Buf, 0, IDC_UPDATESLIST);
		EnableWindow(GetDlgItem(hwndDlg, IDC_UPD_FIND_EDIT),FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_UPD_VIEW_COMBO),FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_UPDATESLIST),FALSE);
	}
	else
	{
		LoadString(hApplet, IDS_NONE_APP, Buf, sizeof(Buf) / sizeof(TCHAR));
		AddItemToList(hwndDlg, -1, (LPTSTR)Buf, 0, IDC_SOFTWARELIST);
		EnableWindow(GetDlgItem(hwndDlg, IDC_FIND_EDIT),FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_VIEW_COMBO),FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),FALSE);
	}
}

VOID
FillSoftwareList(HWND hwndDlg, BOOL bShowUpdates, UINT Control)
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
    INT ItemIndex = 0;
	TCHAR Buf[256],Title[256];
	BOOL IsAdd = FALSE;

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
                if ((!bIsUpdate) && (!bIsSystemComponent) && (!bShowUpdates))
                {
					AddItemToList(hwndDlg, (LPARAM)hSubKey, (LPTSTR)pszDisplayName, ItemIndex, Control);
					IsAdd = TRUE;
                }
                else if (bIsUpdate && bShowUpdates)
                {
					AddItemToList(hwndDlg, (LPARAM)hSubKey, (LPTSTR)pszDisplayName, ItemIndex, Control);
					IsAdd = TRUE;
                }
            }
        }

        dwSize = MAX_PATH;
        ItemIndex++;
    }

    RegCloseKey(hKey);
	
	if (!IsAdd) SetNoneAppMsg(hwndDlg, bShowUpdates);
	
}

VOID
AddItemsToViewControl(HWND hwndDlg, UINT Control)
{
	TCHAR Buf[256];
	int Index;
	HWND hList;
	
	hList = GetDlgItem(hwndDlg, Control);
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

VOID
FindItems(HWND hwndDlg, UINT ListControl, UINT EditControl, UINT RemBtn, UINT InfoBtn)
{
	HWND hList;
	HWND hEdit;
	TCHAR szText[1024], szItemText[1024];
	INT Index;
    size_t i;
	LV_ITEM listItem;
	BOOL comp = TRUE;
	
	hList = GetDlgItem(hwndDlg, ListControl);
	hEdit = GetDlgItem(hwndDlg, EditControl);
	
	SendMessage(hEdit, WM_GETTEXT, 128, (LPARAM)szText);
	
	ZeroMemory(&listItem, sizeof(LV_ITEM));
	listItem.mask       = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
	listItem.pszText    = (LPTSTR)szText;
	listItem.iImage     = -1;
	Index = ListView_InsertItem(hList, &listItem);
	
	SendMessage(hList, LVM_DELETEITEM, Index, 0);
	ListView_GetItemText(hList, Index, 0, (LPTSTR)szItemText, 1024);
	
	for (i = 0; i < _tcslen(szText); i++)
	{
		if (szText[i] != szItemText[i]) comp = FALSE;
	}
	ListView_SetItemState(hList, Index, LVIS_SELECTED | LVIS_FOCUSED, -1);
	if (comp)
		ButtonStatus(hwndDlg, TRUE, RemBtn, InfoBtn);
	else
		ButtonStatus(hwndDlg, FALSE, RemBtn, InfoBtn);
}

VOID
GetCurrentView(HWND hwndDlg, UINT ViewControl, UINT ListControl)
{
	int nCurrSel;
	nCurrSel = (int)SendMessage(GetDlgItem(hwndDlg, ViewControl),
								CB_GETCURSEL,
								(WPARAM)0,
								(LPARAM)0);
	switch (nCurrSel)
	{
		case 0:
			SetWindowLong(GetDlgItem(hwndDlg, ListControl),
						  GWL_STYLE, LVS_ICON | LVS_SORTASCENDING | LVS_AUTOARRANGE | LVS_SINGLESEL | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP);
		break;
		case 1:
			SetWindowLong(GetDlgItem(hwndDlg, ListControl),
						  GWL_STYLE,LVS_LIST | LVS_SORTASCENDING | LVS_AUTOARRANGE | LVS_SINGLESEL | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP);
		break;
		case 2:
			SetWindowLong(GetDlgItem(hwndDlg, ListControl),
						  GWL_STYLE,LVS_REPORT | LVS_SORTASCENDING | LVS_AUTOARRANGE | LVS_SINGLESEL | WS_VISIBLE | WS_CHILD | WS_BORDER | WS_TABSTOP);
		break;
	}
}

VOID
ShowPopupMenu(HWND hwndDlg, UINT ResMenu, INT xPos, INT yPos, UINT Control)
{
	INT nIndex;
	nIndex = (INT)SendMessage(GetDlgItem(hwndDlg, Control),LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
	if ( nIndex != -1)
	{
		POINT pt;
		RECT lvRect;
		HMENU hMenu;
			
		GetCursorPos(&pt);
			
		GetWindowRect(GetDlgItem(hwndDlg, Control), &lvRect);
		if (PtInRect(&lvRect, pt))
		{
			hMenu = GetSubMenu(LoadMenu(hApplet, MAKEINTRESOURCE(ResMenu)),0);
			TrackPopupMenuEx(hMenu,
							 TPM_RIGHTBUTTON,
							 xPos,
							 yPos,
							 hwndDlg,
							 NULL);
			DestroyMenu(hMenu);
		}
	}
}

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
			CallInformation(hDlg, InfoDialog, IDC_SOFTWARELIST);
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

BOOL
IsItemSelected(HWND hwndDlg, UINT Control)
{
	INT nIndex;
	nIndex = (INT)SendMessage(GetDlgItem(hwndDlg, Control),LVM_GETNEXTITEM,-1,LVNI_FOCUSED);
	if (nIndex != -1) return TRUE;
	return FALSE;
}

VOID
ButtonStatus(HWND hwndDlg, BOOL Status, UINT RemBtn, UINT InfoBtn)
{
	EnableWindow(GetDlgItem(hwndDlg, RemBtn),Status);
	EnableWindow(GetDlgItem(hwndDlg, InfoBtn),Status);
}

/* Property page dialog callback */
INT_PTR CALLBACK
RemovePageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR Buf[256];
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
			AddItemsToViewControl(hwndDlg, IDC_VIEW_COMBO);
			LoadString(hApplet, IDS_APPLIST, Buf, sizeof(Buf) / sizeof(TCHAR));
			AddListColumn(GetDlgItem(hwndDlg, IDC_SOFTWARELIST),Buf);
			FillSoftwareList(hwndDlg, FALSE, IDC_SOFTWARELIST);
			InfoDialog = hwndDlg;
        break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
				case IDC_FIND_EDIT:
					if (HIWORD(wParam) == EN_CHANGE)
					{
					    FindItems(hwndDlg,IDC_SOFTWARELIST,IDC_FIND_EDIT,IDC_ADDREMOVE,IDC_INFO_BUTTON);
					}
				break;
				case ID_APP_MODIFYREMOVE:
                case IDC_ADDREMOVE:
                    CallUninstall(hwndDlg, IDC_SOFTWARELIST, IDC_ADDREMOVE, IDC_INFO_BUTTON, FALSE);
                break;
				case ID_APP_INFORMATION:
				case IDC_INFO_BUTTON:
				{
					if (IsItemSelected(hwndDlg, IDC_SOFTWARELIST))
					DialogBox(hApplet,
							  MAKEINTRESOURCE(IDD_INFORMATION),
							  hwndDlg,
							  InfoPropDlgProc);
				}
				break;
				case IDC_VIEW_COMBO:
					if (HIWORD(wParam) == CBN_SELCHANGE)
						GetCurrentView(hwndDlg, IDC_VIEW_COMBO, IDC_SOFTWARELIST);
				break;
            }
            break;
		case WM_NOTIFY:
		{
			switch (LOWORD(wParam))
			{
                case IDC_SOFTWARELIST:
					switch (((LPNMHDR)lParam)->code)
					{
						case NM_DBLCLK:
							CallUninstall(hwndDlg, IDC_SOFTWARELIST, IDC_ADDREMOVE, IDC_INFO_BUTTON, FALSE);
						break;
						case NM_CLICK:
						{
							if (!IsItemSelected(hwndDlg, IDC_SOFTWARELIST))
								ButtonStatus(hwndDlg, FALSE, IDC_ADDREMOVE, IDC_INFO_BUTTON);
							else
								ButtonStatus(hwndDlg, TRUE, IDC_ADDREMOVE, IDC_INFO_BUTTON);
						}
						break;
					}
                break;
			}
		}
		break;
		case WM_CONTEXTMENU:
		{
			ShowPopupMenu(hwndDlg,
						  IDR_POPUP_APP,
						  GET_X_LPARAM(lParam),
						  GET_Y_LPARAM(lParam),
						  IDC_SOFTWARELIST);
		}
		break;
    }
	
    return FALSE;
}
