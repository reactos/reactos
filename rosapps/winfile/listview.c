/*
 *  ReactOS winfile
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
    
#include <shellapi.h>
//#include <winspool.h>
#include <windowsx.h>
#include <shellapi.h>
#include <ctype.h>
#include <assert.h>
#define ASSERT assert

#include "winfile.h"
#include "listview.h"
//#include "entries.h"
#include "utils.h"


// Global Variables:
extern HINSTANCE hInst;


HWND CreateListView(HWND hwndParent, LPSTR lpszFileName) 
{ 
    RECT rcClient;  // dimensions of client area 
    HWND hwndLV;    // handle to list view control 
 
    // Get the dimensions of the parent window's client area, and create the list view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndLV = CreateWindowEx(0, WC_LISTVIEW, "List View", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES, 
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)LIST_WINDOW, hInst, NULL); 
 
    // Initialize the image list, and add items to the control. 
/*
    if (!InitListViewImageLists(hwndLV) || 
            !InitListViewItems(hwndLV, lpszFileName)) { 
        DestroyWindow(hwndLV); 
        return FALSE; 
    } 
 */
    return hwndLV;
} 

const static LPTSTR g_pos_names[COLUMNS] = {
	_T(""),			// symbol
	_T("Name"),
	_T("Size"),
	_T("CDate"),
#ifndef _NO_EXTENSIONS
	_T("ADate"),
	_T("MDate"),
	_T("Index/Inode"),
	_T("Links"),
#endif
	_T("Attributes"),
#ifndef _NO_EXTENSIONS
	_T("Security")
#endif
};

const static int g_pos_align[] = {
	0,
	HDF_LEFT,	// Name
	HDF_RIGHT,	// Size
	HDF_LEFT,	// CDate
#ifndef _NO_EXTENSIONS
	HDF_LEFT,	// ADate
	HDF_LEFT,	// MDate
	HDF_LEFT,	// Index
	HDF_CENTER,	// Links
#endif
	HDF_CENTER,	// Attributes
#ifndef _NO_EXTENSIONS
	HDF_LEFT	// Security
#endif
};

void resize_tree(ChildWnd* child, int cx, int cy)
{
	HDWP hdwp = BeginDeferWindowPos(4);
	RECT rt = {0, 0, cx, cy};

	cx = child->split_pos + SPLIT_WIDTH/2;

#ifndef _NO_EXTENSIONS
	{
		WINDOWPOS wp;
		HD_LAYOUT hdl = {&rt, &wp};

		Header_Layout(child->left.hwndHeader, &hdl);

		DeferWindowPos(hdwp, child->left.hwndHeader, wp.hwndInsertAfter,
						wp.x-1, wp.y, child->split_pos-SPLIT_WIDTH/2+1, wp.cy, wp.flags);
		DeferWindowPos(hdwp, child->right.hwndHeader, wp.hwndInsertAfter,
						rt.left+cx+1, wp.y, wp.cx-cx+2, wp.cy, wp.flags);
	}
#endif

	DeferWindowPos(hdwp, child->left.hwnd, 0, rt.left, rt.top, child->split_pos-SPLIT_WIDTH/2-rt.left, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);
	DeferWindowPos(hdwp, child->right.hwnd, 0, rt.left+cx+1, rt.top, rt.right-cx, rt.bottom-rt.top, SWP_NOZORDER|SWP_NOACTIVATE);

	EndDeferWindowPos(hdwp);
}


#ifndef _NO_EXTENSIONS

HWND create_header(HWND parent, Pane* pane, int id)
{
	HD_ITEM hdi = {HDI_TEXT|HDI_WIDTH|HDI_FORMAT};
	int idx;

	HWND hwnd = CreateWindow(WC_HEADER, 0, WS_CHILD|WS_VISIBLE|HDS_HORZ/*TODO: |HDS_BUTTONS + sort orders*/,
								0, 0, 0, 0, parent, (HMENU)id, Globals.hInstance, 0);
	if (!hwnd)
		return 0;

	SendMessage(hwnd, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), FALSE);

	for(idx=0; idx<COLUMNS; idx++) {
		hdi.pszText = g_pos_names[idx];
		hdi.fmt = HDF_STRING | g_pos_align[idx];
		hdi.cxy = pane->widths[idx];
		Header_InsertItem(hwnd, idx, &hdi);
	}

	return hwnd;
}

#endif


////////////////////////////////////////////////////////////////////////////////
static WNDPROC g_orgListWndProc;

LRESULT CALLBACK ListWndProc(HWND hwnd, UINT nmsg, WPARAM wparam, LPARAM lparam)
{
	ChildWnd* child = (ChildWnd*)GetWindowLong(GetParent(hwnd), GWL_USERDATA);
	Pane* pane = (Pane*)GetWindowLong(hwnd, GWL_USERDATA);
	ASSERT(child);

	switch(nmsg) {
#ifndef _NO_EXTENSIONS
		case WM_HSCROLL:
			set_header(pane);
			break;
#endif

		case WM_SETFOCUS:
			child->focus_pane = pane==&child->right? 1: 0;
			ListBox_SetSel(hwnd, TRUE, 1);
			//TODO: check menu items
			break;

		case WM_KEYDOWN:
			if (wparam == VK_TAB) {
				//TODO: SetFocus(Globals.hDriveBar)
				SetFocus(child->focus_pane? child->left.hwnd: child->right.hwnd);
			}
	}

	return CallWindowProc(g_orgListWndProc, hwnd, nmsg, wparam, lparam);
}


static void init_output(HWND hwnd)
{
	TCHAR b[16];
	HFONT old_font;
	HDC hdc = GetDC(hwnd);

	if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, _T("1000"), 0, b, 16) > 4)
		Globals.num_sep = b[1];
	else
		Globals.num_sep = _T('.');

	old_font = SelectFont(hdc, Globals.hFont);
	GetTextExtentPoint32(hdc, _T(" "), 1, &Globals.spaceSize);
	SelectFont(hdc, old_font);
	ReleaseDC(hwnd, hdc);
}


void create_list_window(HWND parent, Pane* pane, int id, int id_header)
{
	static int s_init = 0;
	Entry* entry = pane->root;

	pane->hwnd = CreateWindow(_T("ListBox"), _T(""), WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|
								LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT|LBS_OWNERDRAWFIXED|LBS_NOTIFY,
								0, 0, 0, 0, parent, (HMENU)id, Globals.hInstance, 0);

	SetWindowLong(pane->hwnd, GWL_USERDATA, (LPARAM)pane);
	g_orgListWndProc = SubclassWindow(pane->hwnd, ListWndProc);

	SendMessage(pane->hwnd, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);

	 // insert entries into listbox
	if (entry)
		insert_entries(pane, entry, -1);

	 // calculate column widths
	if (!s_init) {
		s_init = 1;
		init_output(pane->hwnd);
	}

	calc_widths(pane, TRUE);

#ifndef _NO_EXTENSIONS
	pane->hwndHeader = create_header(parent, pane, id_header);
#endif
}



