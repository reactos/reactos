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


////////////////////////////////////////////////////////////////////////////////
// Global Variables:
//

static WNDPROC g_orgListWndProc;


////////////////////////////////////////////////////////////////////////////////
// Local module support methods
//



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
//    Entry* entry = (Entry*)plvdi->item.lParam;
//    ASSERT(entry);
    plvdi->item.pszText = NULL;

    switch (plvdi->item.iSubItem) {
    case 0:
//        plvdi->item.pszText = entry->data.cFileName; 
        break;
    case 1:
        break;
    case 2:
        break;
    case 3:
        break;
    case 4:
        break;
    default:
        _tcscpy(buffer, _T(" "));
        plvdi->item.pszText = buffer;
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
/*
        if (entry != NULL) {
            TRACE("RefreshList(...) entry name: %s\n", entry->data.cFileName);
    	    InsertListEntries(hWnd, entry, -1);
        }
 */
    }
}

