/*
 *  ReactOS regedit
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

#ifdef _MSC_VER
#include "stdafx.h"
#else
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
#endif
    
#include <windowsx.h>
#include <assert.h>
#define ASSERT assert
#include "main.h"
#include "listview.h"

#include "trace.h"


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

static WNDPROC g_orgListWndProc;


////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//

static void AddEntryToList(HWND hwndLV, int idx, Entry* entry)
{ 
    LVITEM item;

    item.mask = LVIF_TEXT | LVIF_PARAM; 
    item.iItem = 0;//idx; 
    item.iSubItem = 0; 
    item.state = 0; 
    item.stateMask = 0; 
//    item.pszText = entry->data.cFileName; 
    item.pszText = LPSTR_TEXTCALLBACK; 
//    item.cchTextMax = strlen(entry->data.cFileName); 
    item.cchTextMax = 0; 
    item.iImage = 0; 
//    item.iImage = I_IMAGECALLBACK; 
    item.lParam = (LPARAM)entry;
#if (_WIN32_IE >= 0x0300)
    item.iIndent = 0;
#endif
    ListView_InsertItem(hwndLV, &item);
}

// insert listctrl entries after index idx
static void InsertListEntries(HWND hWnd, Entry* entry, int idx)
{
  	ShowWindow(hWnd, SW_HIDE);

    if (idx == -1) {
    }
    idx = 0;

	for (; entry; entry = entry->next) {
#ifndef _LEFT_FILES
//    	if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
//	    	continue;
#endif
        //ListBox_InsertItemData(hWnd, idx, entry);
        AddEntryToList(hWnd, idx, entry); 
        ++idx;
    }
    ShowWindow(hWnd, SW_SHOW);
}

#define MAX_LIST_COLUMNS (IDS_LIST_COLUMN_LAST - IDS_LIST_COLUMN_FIRST + 1)
static int default_column_widths[MAX_LIST_COLUMNS] = { 200, 175, 400 };
static int column_alignment[MAX_LIST_COLUMNS] = { LVCFMT_LEFT, LVCFMT_LEFT, LVCFMT_LEFT };

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
        LoadString(hInst, IDS_LIST_COLUMN_FIRST + index, szText, sizeof(szText));
        if (ListView_InsertColumn(hWndListView, index, &lvC) == -1) {
            // TODO: handle failure condition...
            break;
        }
    }
}


// OnGetDispInfo - processes the LVN_GETDISPINFO 
// notification message. 
 
static void OnGetDispInfo(NMLVDISPINFO* plvdi)
{
    static TCHAR buffer[200];

//    FILETIME LastWriteTime;
//    TCHAR Class[MAX_NAME_LEN];
//    DWORD cClass = MAX_NAME_LEN;
//    TCHAR Name[MAX_NAME_LEN];
//    DWORD cName = MAX_NAME_LEN;
//    DWORD dwIndex = 0L;

    Entry* pEntry = (Entry*)plvdi->item.lParam;
    ASSERT(pEntry);

    plvdi->item.pszText = NULL;
    plvdi->item.cchTextMax = 0; 

    switch (plvdi->item.iSubItem) {
    case 0:
/*
        plvdi->item.pszText = _T("(Default)");
        plvdi->item.cchTextMax = _tcslen(plvdi->item.pszText); 
        if (pEntry->bKey == TRUE) {
            DWORD nSubKeys;
            DWORD MaxSubKeyLen;
            DWORD MaxClassLen;
            DWORD ValueCount;
            DWORD MaxValueNameLen;
            DWORD MaxValueLen;
            DWORD SecurityDescriptorLen;

            HKEY hKey = pEntry->hKey;
            LONG result = RegQueryInfoKey(pEntry->hKey, Class, &cClass, 0, 
                &nSubKeys, &MaxSubKeyLen, &MaxClassLen, &ValueCount, 
                &MaxValueNameLen, &MaxValueLen, &SecurityDescriptorLen, 
                &LastWriteTime);
            if (result == ERROR_SUCCESS) {
                plvdi->item.pszText = Class; 
                plvdi->item.cchTextMax = cClass; 
            }
        }
 */
        plvdi->item.pszText = pEntry->szName; 
        plvdi->item.cchTextMax = lstrlen(pEntry->szName); 

        break;
    case 1:
        plvdi->item.pszText = _T("REG_SZ");
        break;
    case 2:
        plvdi->item.pszText = _T("(value not set)");
        break;
    case 3:
        plvdi->item.pszText = _T("");
        break;
    default:
//        _tcscpy(buffer, _T(" "));
//        plvdi->item.pszText = buffer;
        break;
    }
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
////////////////////////////////////////////////////////////////////////////////
void ListViewPopUpMenu(HWND hWnd, POINT pt)
{
}

static LRESULT CALLBACK ListWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
//	ChildWnd* child = (ChildWnd*)GetWindowLong(GetParent(hWnd), GWL_USERDATA);
//	Pane* pane = (Pane*)GetWindowLong(hWnd, GWL_USERDATA);
//	ASSERT(child);

	switch (message) {
/*
    case WM_CREATE:
        //CreateListView(hWnd);
        return 0;
 */
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
                item.mask = LVIF_PARAM;
                item.iItem = info.iItem;
                if (ListView_GetItem(hWnd, &item)) {
//                    Entry* entry = (Entry*)item.lParam;
//                    OpenTarget(hWnd, entry->data.cFileName);
                }
            }
            }
            break;

        case NM_RCLICK:
            {
            int idx;
            LV_HITTESTINFO lvH;
            NM_LISTVIEW *pNm = (NM_LISTVIEW *)lParam;
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
        break;
    default:
        return CallWindowProc(g_orgListWndProc, hWnd, message, wParam, lParam);
        break;
	}
	return 0;
}


HWND CreateListView(HWND hwndParent/*, Pane* pane*/, int id, Root* pRoot/*Entry* pEntry*/)
{ 
    RECT rcClient;  // dimensions of client area 
    HWND hwndLV;    // handle to list view control 
//	Entry* entry = pane->root;
//	pane->treePane = 0;
 
    // Get the dimensions of the parent window's client area, and create the list view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndLV = CreateWindowEx(0, WC_LISTVIEW, _T("List View"), 
        WS_VISIBLE | WS_CHILD | WS_BORDER | LVS_REPORT, 
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)id, hInst, NULL); 
 
    // Initialize the image list, and add items to the control. 
/*
    if (!InitListViewImageLists(hwndLV) || 
            !InitListViewItems(hwndLV, lpszPathName)) { 
        DestroyWindow(hwndLV); 
        return FALSE; 
    } 
 */
    ListView_SetExtendedListViewStyle(hwndLV,  LVS_EX_FULLROWSELECT);
    CreateListColumns(hwndLV);

	SetWindowLong(hwndLV, GWL_USERDATA, (LPARAM)pRoot);
	g_orgListWndProc = SubclassWindow(hwndLV, ListWndProc);
	//SendMessage(hwndLV, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);

	 // insert entries into listbox
//    if (entry) {
//		InsertListEntries(hwndLV, entry, -1);
//    }

    return hwndLV;
} 

void RefreshList(HWND hWnd, Entry* entry)
{
    if (hWnd != NULL) {
        ListView_DeleteAllItems(hWnd);
        if (entry != NULL) {
            TRACE("RefreshList(...) entry name: %p\n", entry);
    	    InsertListEntries(hWnd, entry, -1);
        }
    }
}

