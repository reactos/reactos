/*
 *  ReactOS
 *  Copyright (C) 2007 ReactOS Team
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
 * PROJECT:         input.dll
 * FILE:            dll/win32/input/settings.c
 * PURPOSE:         input.dll
 * PROGRAMMER:      Dmitry Chapyshev (lentind@yandex.ru)
 * UPDATE HISTORY:
 *      06-09-2007  Created
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
#include "input.h"

#define BUFSIZE 256

static BOOL
GetLayoutName(LPCTSTR lcid, LPCTSTR name)
{
    HKEY hKey;
    DWORD dwBufLen;
    TCHAR szBuf[BUFSIZE];

    _stprintf(szBuf, L"SYSTEM\\ControlSet001\\Control\\Keyboard Layouts\\%s",lcid);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, (LPCTSTR)szBuf, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
    {
        dwBufLen = BUFSIZE;
        if (RegQueryValueEx(hKey,L"Layout Text",NULL,NULL,(LPBYTE)name,&dwBufLen) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return TRUE;
        }
    }

    return FALSE;
}

static BOOL
TreeView_GetItemText(HWND hTree, HTREEITEM hItem, LPTSTR name, DWORD buf)
{
    TVITEM tv = {0};

    tv.mask = TVIF_TEXT | TVIF_HANDLE;
    tv.hItem = hItem;
    tv.pszText = name;
    tv.cchTextMax = (int)buf;

    return TreeView_GetItem(hTree, &tv);
}

static HTREEITEM
GetRootHandle(HWND hWnd, TCHAR * Name)
{
    HTREEITEM hNext = 0, hRoot;
    TCHAR Tmp[256];
    HWND hTree;
    DWORD i, count;
    HKEY hKey;

    hTree = GetDlgItem(hWnd, IDC_KEYLAYOUT_TREE);
    hRoot = TreeView_GetRoot(hTree);

    RegOpenKeyEx(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, KEY_QUERY_VALUE, &hKey);
    RegQueryInfoKey(hKey,NULL,NULL,NULL,NULL,NULL,NULL,&count,NULL,NULL,NULL,NULL);
    RegCloseKey(hKey);

    for (i = 0; i < count; i++)
    {
        if (i == 0)
        {
            (VOID) TreeView_GetItemText(hTree, hRoot, (LPTSTR)Tmp, 256);
        }
        else
        {
            if (i == 1)
            {
                hNext = TreeView_GetNextItem(hTree, hRoot, TVGN_NEXT);
            }
            else
            {
                hNext = TreeView_GetNextItem(hTree, hNext, TVGN_NEXT);
            }
            (VOID) TreeView_GetItemText(hTree, hNext, (LPTSTR)Tmp, 256);
        }

        if (_tcscmp(Name,Tmp) == 0)
        {
            if (i == 0) return hRoot;
            return hNext;
        }
    }
    return 0;
}

static BOOL
InitDefaultLangList(HWND hWnd)
{
    HKEY hKey, hSubKey;
    TCHAR szPreload[BUFSIZE], szCount[BUFSIZE], szSub[BUFSIZE];
    TCHAR szName[BUFSIZE], Lang[BUFSIZE];
    DWORD dwBufLen, dwBufCLen, cValues;
    INT Count;
    LCID Lcid;
    TV_INSERTSTRUCT inc;
    HWND TWwnd;
    int Index[3];
    HIMAGELIST hImgList;
    HICON hIcon;

    ZeroMemory(&inc, sizeof(TV_INSERTSTRUCT));
    inc.hInsertAfter = TVI_LAST;
    inc.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
    inc.hParent = TVI_ROOT;

    hImgList = ImageList_Create(16,16,ILC_COLOR8 | ILC_MASK,5,5);
    hIcon = LoadImage(hApplet,MAKEINTRESOURCE(IDI_KEYBOARD_ICO),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
    Index[0] = ImageList_AddIcon(hImgList,hIcon);
    hIcon = LoadImage(hApplet,MAKEINTRESOURCE(IDI_MARKER_ICO),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
    Index[1] = ImageList_AddIcon(hImgList,hIcon);
    hIcon = LoadImage(hApplet,MAKEINTRESOURCE(IDI_INFO_ICO),IMAGE_ICON,16,16,LR_DEFAULTCOLOR);
    Index[2] = ImageList_AddIcon(hImgList,hIcon);
    DestroyIcon(hIcon);

    TWwnd = GetDlgItem(hWnd, IDC_KEYLAYOUT_TREE);

    if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Keyboard Layout\\Preload", 0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
        return FALSE;

    RegQueryInfoKey(hKey,NULL,NULL,NULL,NULL,NULL,NULL,&cValues,NULL,NULL,NULL,NULL);

    if (cValues)
    {
        for (Count = 0; Count < cValues; Count++)
        {
            szCount[0] = L'\0';

            dwBufCLen = BUFSIZE;
            RegEnumValue(hKey,Count,(LPTSTR)szCount,&dwBufCLen,NULL,NULL,NULL,NULL);

            _stprintf(szCount,L"%d",Count + 1);

            dwBufLen = BUFSIZE;
            RegQueryValueEx(hKey,(LPTSTR)szCount,NULL,NULL,(LPBYTE)szPreload,&dwBufLen);

            Lcid = _tcstoul(szPreload, NULL, 16);
            /* FIXME: If it is established more than one English layouts of the keyboard it is incorrectly determined Lang */
            GetLocaleInfo(Lcid, LOCALE_SLANGUAGE, (LPTSTR)Lang, sizeof(Lang));

            if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Keyboard Layout\\Substitutes", 0, KEY_QUERY_VALUE, &hSubKey) != ERROR_SUCCESS)
                return FALSE;

            dwBufLen = BUFSIZE;
            TCHAR szLOName[BUFSIZE], szBuf[BUFSIZE];
            LONG Ret;
            HTREEITEM hRoot = 0, hKeyBrd = 0, hParent;

            Ret = RegQueryValueEx(hSubKey,(LPCTSTR)szPreload,NULL,NULL,(LPBYTE)szSub,&dwBufLen);
            if (Ret == ERROR_SUCCESS) _tcscpy(szPreload, szSub);

            GetLayoutName(szPreload,szLOName);	

            hParent = GetRootHandle(hWnd, Lang);
            if (hParent == 0)
            {
                inc.hParent = NULL;
                inc.item.pszText = Lang;
                inc.item.iImage = Index[2];
                inc.item.iSelectedImage = Index[2];
                if (Count == 0)
                {
                    inc.item.state = TVIS_BOLD;
                    inc.item.stateMask = TVIS_BOLD;
                }
                else
                {
                    inc.item.state = !TVIS_BOLD;
                    inc.item.stateMask = !TVIS_BOLD;
                }
                hRoot = TreeView_InsertItem(TWwnd, &inc);
                if (Count == 0) (VOID) TreeView_SelectItem(TWwnd, hRoot);

                LoadString(hApplet, IDS_KEYBOARD, (LPTSTR)szBuf, BUFSIZE);
                inc.hParent = hRoot;
                inc.item.pszText = szBuf;
                inc.item.iImage = Index[0];
                inc.item.iSelectedImage = Index[0];
                inc.item.state = !TVIS_BOLD;
                inc.item.stateMask = !TVIS_BOLD;
                hKeyBrd = TreeView_InsertItem(TWwnd, &inc);
                inc.hParent = hKeyBrd;
            }
            else
            {
                inc.hParent = TreeView_GetChild(TWwnd, hParent);
            }

            inc.item.pszText = szLOName;
            inc.item.iImage = Index[1];
            inc.item.iSelectedImage = Index[1];
            if (Count == 0)
            {
                inc.item.state = TVIS_BOLD;
                inc.item.stateMask = TVIS_BOLD;
            }
            else
            {
                inc.item.state = !TVIS_BOLD;
                inc.item.stateMask = !TVIS_BOLD;
            }
            (VOID) TreeView_InsertItem(TWwnd, &inc);

            (VOID) TreeView_SetImageList(TWwnd, hImgList, TVSIL_NORMAL);
            (VOID) TreeView_Expand(TWwnd, hRoot, TVM_EXPAND);
            (VOID) TreeView_Expand(TWwnd, hKeyBrd, TVM_EXPAND);

            _stprintf(szName,L"%s - %s",Lang,szLOName);

            RegCloseKey(hSubKey);

            HWND CBwnd;
            CBwnd = GetDlgItem(hWnd, IDC_DEFAULT_INPUT_LANG);
            SendMessage(CBwnd,CB_ADDSTRING,0,(LPARAM)szName);
            SendMessage(CBwnd,CB_SETITEMDATA,0,(LPARAM)Lcid);
            if (Count == 0)
                SendMessage(CBwnd,CB_SELECTSTRING,(WPARAM)-1,(LPARAM)szName);
        }
    }

    RegCloseKey(hKey);

    return TRUE;
}

/* Property page dialog callback */
INT_PTR CALLBACK
SettingPageProc(HWND hwndDlg,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            InitDefaultLangList(hwndDlg);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_LANG_BAR_BUTTON:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_LANGBAR),
                              hwndDlg,
                              LangBarDlgProc);
                    break;

                case IDC_KEY_SETTINGS_BUTTON:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_KEYSETTINGS),
                              hwndDlg,
                              KeySettingsDlgProc);
                    break;

                case IDC_ADD_BUTTON:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_ADD),
                              hwndDlg,
                              AddDlgProc);
                    break;

                case IDC_PROP_BUTTON:
                    DialogBox(hApplet,
                              MAKEINTRESOURCE(IDD_INPUT_LANG_PROP),
                              hwndDlg,
                              InputLangPropDlgProc);
                    break;

                case IDC_DEFAULT_INPUT_LANG:
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;
    }

    return FALSE;
}

/* EOF */
