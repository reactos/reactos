/*
 *  ReactOS control
 *
 *  listview.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
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

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <cpl.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
    
#include <windowsx.h>
#include "main.h"
#include "listview.h"

#include "assert.h"


////////////////////////////////////////////////////////////////////////////////
// Global and Local Variables:
//

static WNDPROC g_orgListWndProc;

#define MAX_LIST_COLUMNS (IDS_LIST_COLUMN_LAST - IDS_LIST_COLUMN_FIRST + 1)
static int default_column_widths[MAX_LIST_COLUMNS] = { 250, 500, 75 };
static int column_alignment[MAX_LIST_COLUMNS] = { LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT };


////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

static void AddEntryToList(HWND hwndLV, LPTSTR Name, HMODULE hModule, LPTSTR pszComment, DWORD dwCount)
{ 
    LVITEM item;
    int index;

    item.mask = LVIF_TEXT | LVIF_PARAM; 
    item.iItem = 0;//idx; 
    item.iSubItem = 0; 
    item.state = 0; 
    item.stateMask = 0; 
    item.pszText = Name; 
    item.cchTextMax = _tcslen(item.pszText); 
    if (item.cchTextMax == 0)
        item.pszText = LPSTR_TEXTCALLBACK; 
    item.iImage = 0; 
    item.lParam = (LPARAM)hModule;
#if (_WIN32_IE >= 0x0300)
    item.iIndent = 0;
#endif

    index = ListView_InsertItem(hwndLV, &item);
    if (index != -1 && pszComment != NULL) {
        ListView_SetItemText(hwndLV, index, 1, pszComment);
    }
}

static void CreateListColumns(HWND hWndListView)
{
    TCHAR szText[50];
    int index;
    LV_COLUMN lvC;
 
    // Create columns.
    lvC.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvC.pszText = szText;

    // Load the column labels from the resource file.
    for (index = 0; index < MAX_LIST_COLUMNS; index++) {
        lvC.iSubItem = index;
        lvC.cx = default_column_widths[index];
        lvC.fmt = column_alignment[index];
        LoadString(hInst, IDS_LIST_COLUMN_FIRST + index, szText, 50);
        if (ListView_InsertColumn(hWndListView, index, &lvC) == -1) {
            // TODO: handle failure condition...
            break;
        }
    }
}

// OnGetDispInfo - processes the LVN_GETDISPINFO notification message. 
 
static void OnGetDispInfo(NMLVDISPINFO* plvdi)
{
    static TCHAR buffer[200];

    plvdi->item.pszText = NULL;
    plvdi->item.cchTextMax = 0; 

    switch (plvdi->item.iSubItem) {
    case 0:
        plvdi->item.pszText = _T("(Default)");
        break;
    case 1:
        switch (plvdi->item.lParam) {
        default:
            wsprintf(buffer, _T("unknown(%d)"), plvdi->item.lParam);
            plvdi->item.pszText = buffer;
            break;
        }
        break;
    case 2:
        plvdi->item.pszText = _T("(value not set)");
        break;
    case 3:
        plvdi->item.pszText = _T("");
        break;
    }
} 

static int CALLBACK CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    TCHAR buf1[1000];
    TCHAR buf2[1000];

    ListView_GetItemText((HWND)lParamSort, lParam1, 0, buf1, sizeof(buf1)/sizeof(TCHAR));
    ListView_GetItemText((HWND)lParamSort, lParam2, 0, buf2, sizeof(buf2)/sizeof(TCHAR));
    return _tcscmp(buf1, buf2);
}

static void ListViewPopUpMenu(HWND hWnd, POINT pt)
{
}

static BOOL _CmdWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (LOWORD(wParam)) {
//    case ID_FILE_OPEN:
//        break;
	default:
        return FALSE;
	}
	return TRUE;
}

static LRESULT CALLBACK ListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_COMMAND:
        if (!_CmdWndProc(hWnd, message, wParam, lParam)) {
            return CallWindowProc(g_orgListWndProc, hWnd, message, wParam, lParam);
        }
		break;
    case WM_NOTIFY:
        switch (((LPNMHDR)lParam)->code) { 
        case LVN_GETDISPINFO: 
            OnGetDispInfo((NMLVDISPINFO*)lParam); 
            break; 
        case NM_DBLCLK:
            {
            NMITEMACTIVATE* nmitem = (LPNMITEMACTIVATE)lParam;
            LVHITTESTINFO info;

            if (nmitem->hdr.hwndFrom != hWnd) break; 
//            if (nmitem->hdr.idFrom != IDW_LISTVIEW) break; 
//            if (nmitem->hdr.code != ???) break; 
#ifdef _MSC_VER
            switch (nmitem->uKeyFlags) {
            case LVKF_ALT:     //  The ALT key is pressed.  
                // properties dialog box ?
                break;
            case LVKF_CONTROL: //  The CTRL key is pressed.
                // run dialog box for providing parameters...
                break;
            case LVKF_SHIFT:   //  The SHIFT key is pressed.   
                break;
            }
#endif
            info.pt.x = nmitem->ptAction.x;
            info.pt.y = nmitem->ptAction.y;
            if (ListView_HitTest(hWnd, &info) != -1) {
                LVITEM item;
                item.mask = LVIF_TEXT | LVIF_PARAM;
                item.iItem = info.iItem;
                if (ListView_GetItem(hWnd, &item)) {
#if 0
                HMODULE hCpl = LoadLibrary(pszText.pszText);
                if (hCpl) {
                    if (InitCPlApplet(hwndLV, hCpl)) {
                    } else {
                    }
                    FreeLibrary(hCpl);
                }
//                    pCPlApplet(hWnd, CPL_STARTWPARAMS, nSubProgs, (LPARAM)0);
#endif

                }
            }
            }
            break;

        case NM_RCLICK:
            {
            int idx;
            LV_HITTESTINFO lvH;
            NM_LISTVIEW* pNm = (NM_LISTVIEW*)lParam;
            lvH.pt.x = pNm->ptAction.x;
            lvH.pt.y = pNm->ptAction.y;     
            idx = ListView_HitTest(hWnd, &lvH);
            if (idx != -1) {
                POINT pt;
                GetCursorPos(&pt);
                ListViewPopUpMenu(hWnd, pt);
                return idx;
            }
            }
            break;

        default:
            return CallWindowProc(g_orgListWndProc, hWnd, message, wParam, lParam);
        }
		break;
	case WM_KEYDOWN:
		if (wParam == VK_TAB) {
			//TODO: SetFocus(Globals.hDriveBar)
			//SetFocus(child->nFocusPanel? child->left.hWnd: child->right.hWnd);
		}
        // fall thru...
    default:
        return CallWindowProc(g_orgListWndProc, hWnd, message, wParam, lParam);
        break;
	}
	return 0;
}


HWND CreateListView(HWND hwndParent, int id, DWORD style)
{ 
    RECT rcClient;
    HWND hwndLV;
 
    // Get the dimensions of the parent window's client area, and create the list view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndLV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, _T("List View"), 
        WS_VISIBLE | WS_CHILD | style, 
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)id, hInst, NULL); 
    ListView_SetExtendedListViewStyle(hwndLV,  LVS_EX_FULLROWSELECT);
 
    // Initialize the image list, and add items to the control. 
/*
    if (!InitListViewImageLists(hwndLV) || 
            !InitListViewItems(hwndLV, szName)) { 
        DestroyWindow(hwndLV); 
        return FALSE; 
    } 
 */
    CreateListColumns(hwndLV);
	g_orgListWndProc = SubclassWindow(hwndLV, ListWndProc);
    return hwndLV;
} 

typedef LONG (WINAPI *CPlApplet_Ptr)(HWND, UINT, LONG, LONG);
/*
LONG CPlApplet(
    HWND hwndCPl,
    UINT uMsg,
    LONG lParam1,
    LONG lParam2
);
 */

static BOOL InitCPlApplet(HWND hwndLV, HMODULE hCpl)
{
    if (hwndLV && hCpl) {
        CPlApplet_Ptr pCPlApplet;
        pCPlApplet = (CPlApplet_Ptr)(FARPROC)GetProcAddress(hCpl, "CPlApplet");
        if (pCPlApplet)	{
            if (pCPlApplet(hwndLV, CPL_INIT, 0, 0)) {
                int nSubProgs = pCPlApplet(hwndLV, CPL_GETCOUNT, 0, 0);
                while (nSubProgs && nSubProgs--) {
                    CPLINFO cplInfo;
                    memset(&cplInfo, 0, sizeof(CPLINFO));
                    pCPlApplet(hwndLV, CPL_INQUIRE, nSubProgs, (LPARAM)&cplInfo);
                    if (cplInfo.idName == CPL_DYNAMIC_RES) {
                        NEWCPLINFO cplNewInfo;
                        memset(&cplNewInfo, 0, sizeof(NEWCPLINFO));
                        cplNewInfo.dwSize = sizeof(NEWCPLINFO);
                        pCPlApplet(hwndLV, CPL_NEWINQUIRE, nSubProgs, (LPARAM)&cplNewInfo);
                        AddEntryToList(hwndLV, cplNewInfo.szName, hCpl, cplNewInfo.szInfo, 0);
                    } else {
                        TCHAR NameBuf[MAX_PATH];
                        TCHAR InfoBuf[MAX_PATH];
                        HANDLE hIcon;
                        memset(NameBuf, _T('\0'), sizeof(NameBuf));
                        memset(InfoBuf, _T('\0'), sizeof(InfoBuf));
                        if (LoadString(hCpl, cplInfo.idName, NameBuf, MAX_PATH)) {
                        }
                        if (LoadString(hCpl, cplInfo.idInfo, InfoBuf, MAX_PATH)) {
                        }
                        hIcon = LoadImage(hCpl, (LPCTSTR)cplInfo.idIcon, IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
                        if (hIcon) {
                            // add the icon to an image list, pass index to AddEntryToList(...)
                        }
                        AddEntryToList(hwndLV, NameBuf, hCpl, InfoBuf, 0);
                    }
                 }
                 return TRUE;
            }
        }
    }
    return FALSE;
}


BOOL RefreshListView(HWND hwndLV, HKEY hKey, LPTSTR path)
{ 

	WIN32_FIND_DATA	data;
//	BY_HANDLE_FILE_INFORMATION bhfi;
//	BOOL bhfi_valid;
	HANDLE hFind;
	HANDLE hFile;
	TCHAR buffer[MAX_PATH+10], *p;
    UINT length;

    if (hwndLV != NULL) {
        ListView_DeleteAllItems(hwndLV);
    }

    length = GetSystemDirectory(buffer, sizeof(buffer)/sizeof(TCHAR));
    p = &buffer[length];

//  path = buffer;
//	for (p=buffer; *path; ) *p++ = *path++;
	lstrcpy(p, _T("\\*.cpl"));

    memset(&data, 0, sizeof(WIN32_FIND_DATA));
	hFind = FindFirstFile(buffer, &data);

	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				continue;
			}
			lstrcpy(p+1, data.cFileName);
			hFile = CreateFile(buffer, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
								0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
			if (hFile != INVALID_HANDLE_VALUE) {
                HMODULE	hCpl;
//				if (GetFileInformationByHandle(hFile, &bhfi)) bhfi_valid = TRUE;
				CloseHandle(hFile);
                hCpl = LoadLibrary(data.cFileName);
                if (hCpl) {
                    if (InitCPlApplet(hwndLV, hCpl)) {
                    } else {
                    }
                    FreeLibrary(hCpl);
                }
			}
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
    }
    return TRUE;
} 

