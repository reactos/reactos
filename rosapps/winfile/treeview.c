/*
 *  ReactOS winfile
 *
 *  treeview.c
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

#include "main.h"
#include "treeview.h"
#include "entries.h"
#include "utils.h"

#include "trace.h"

// Global Variables:
extern HINSTANCE hInst;


// Global variables and constants 
// Image_Open, Image_Closed, and Image_Root - integer variables for 
// indexes of the images. 
// CX_BITMAP and CY_BITMAP - width and height of an icon. 
// NUM_BITMAPS - number of bitmaps to add to the image list. 
int Image_Open; 
int Image_Closed; 
int Image_Root; 

#define CX_BITMAP    16
#define CY_BITMAP    16
#define NUM_BITMAPS  3


// AddItemToTree - adds items to a tree view control. 
// Returns the handle to the newly added item. 
// hwndTV - handle to the tree view control. 
// lpszItem - text of the item to add. 
// nLevel - level at which to add the item. 

HTREEITEM AddItemToTree(HWND hwndTV, LPSTR lpszItem, int nLevel)
{ 
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST; 
    static HTREEITEM hPrevRootItem = NULL; 
    static HTREEITEM hPrevLev2Item = NULL; 
    HTREEITEM hti; 
 
    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM; 
    // Set the text of the item. 
    tvi.pszText = lpszItem; 
    tvi.cchTextMax = lstrlen(lpszItem); 
    // Assume the item is not a parent item, so give it an image. 
    tvi.iImage = Image_Root; 
    tvi.iSelectedImage = Image_Root; 
    tvi.cChildren = 1; 
    // Save the heading level in the item's application-defined data area. 
    tvi.lParam = (LPARAM)nLevel; 
 
    tvins.item = tvi; 
    tvins.hInsertAfter = hPrev; 
 
    // Set the parent item based on the specified level. 
    if (nLevel == 1) 
        tvins.hParent = TVI_ROOT; 
    else if (nLevel == 2) 
        tvins.hParent = hPrevRootItem; 
    else 
        tvins.hParent = hPrevLev2Item; 
 
    // Add the item to the tree view control. 
    hPrev = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins); 
 
    // Save the handle to the item. 
    if (nLevel == 1) 
        hPrevRootItem = hPrev; 
    else if (nLevel == 2) 
        hPrevLev2Item = hPrev; 
 
    // The new item is a child item. Give the parent item a 
    // closed folder bitmap to indicate it now has child items. 
    if (nLevel > 1) { 
        hti = TreeView_GetParent(hwndTV, hPrev); 
        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
        tvi.hItem = hti; 
        tvi.iImage = Image_Closed; 
        tvi.iSelectedImage = Image_Closed; 
        TreeView_SetItem(hwndTV, &tvi); 
    } 
    return hPrev; 
} 

HTREEITEM AddEntryToTree(HWND hwndTV, Entry* entry)
{ 
    HTREEITEM hItem = 0;
#if 0
    hItem = AddItemToTree(hwndTV, entry->data.cFileName, entry->level);
#else
//HTREEITEM AddItemToTree(HWND hwndTV, LPSTR lpszItem, int nLevel)
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST; 
    static HTREEITEM hPrevRootItem = NULL; 
    static HTREEITEM hPrevLev2Item = NULL; 
    HTREEITEM hti; 

    TRACE("AddEntryToTree(level:%u - %s)\n", entry->level, entry->data.cFileName); 
    
    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM; 
    // Set the text of the item. 
    tvi.pszText = entry->data.cFileName; 
    tvi.cchTextMax = lstrlen(entry->data.cFileName); 
    // Assume the item is not a parent item, so give it an image. 
    tvi.iImage = Image_Root; 
    tvi.iSelectedImage = Image_Root; 
    tvi.cChildren = 1; 
    // Save the heading level in the item's application-defined data area. 
    //tvi.lParam = (LPARAM)entry->level; 
    // Save the entry pointer in the item's application-defined data area. 
    tvi.lParam = (LPARAM)entry; 
 
    tvins.item = tvi; 
    tvins.hInsertAfter = hPrev; 
 
    // Set the parent item based on the specified level. 
//    if (entry->level == 0 || entry->level == 1) {
    if (entry->level == 0) {
        tvins.hParent = TVI_ROOT; 
    } else if (entry->level == 1) {
        tvins.hParent = hPrevRootItem; 
        if (hPrevRootItem) {
            //tvins.hParent = entry->up->hTreeItem; 
            int foobar = entry->level * 2; 
        }
    } else {
        tvins.hParent = hPrevLev2Item; 
        if (hPrevLev2Item) {
            tvins.hParent = entry->up->hTreeItem; 
        }
    }
 
    // Add the item to the tree view control. 
    hPrev = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins); 
 
    // Save the handle to the item. 
    if (entry->level == 0) 
        hPrevRootItem = hPrev; 
    else if (entry->level == 1) 
        hPrevLev2Item = hPrev; 
 
    // The new item is a child item. Give the parent item a 
    // closed folder bitmap to indicate it now has child items. 
    if (entry->level > 1) { 
        hti = TreeView_GetParent(hwndTV, hPrev); 
        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
        tvi.hItem = hti; 
        tvi.iImage = Image_Closed; 
        tvi.iSelectedImage = Image_Closed; 
        TreeView_SetItem(hwndTV, &tvi); 
    } 
    hItem = hPrev; 
#endif
    return hItem;
}


// InitTreeViewItems - extracts headings from the specified file and 
// passes them to a function that adds them to a tree view control. 
// Returns TRUE if successful, or FALSE otherwise. 
// hwndTV - handle to the tree view control. 
// lpszFileName - name of file with headings.

static BOOL InitTreeViewItems(HWND hwndTV, LPSTR lpszFileName) 
{ 
/*
    HTREEITEM hItem;
    
    hItem = AddItemToTree(hwndTV, "C:\\ - FAT32", 1); 
    AddItemToTree(hwndTV, "reactos", 2); 
    AddItemToTree(hwndTV, "bin", 3); 
    AddItemToTree(hwndTV, "media", 3); 
    AddItemToTree(hwndTV, "symbols", 3); 
    AddItemToTree(hwndTV, "system32", 3); 
    TreeView_Expand(hwndTV, hItem, TVE_EXPAND);
*/
    return TRUE; 
} 
 
// InitTreeViewImageLists - creates an image list, adds three bitmaps 
// to it, and associates the image list with a tree view control. 
// Returns TRUE if successful, or FALSE otherwise. 
// hwndTV - handle to the tree view control. 

static BOOL InitTreeViewImageLists(HWND hwndTV) 
{ 
    HIMAGELIST himl;  // handle to image list 
    HBITMAP hbmp;     // handle to bitmap 

    // Create the image list. 
    if ((himl = ImageList_Create(CX_BITMAP, CY_BITMAP, 
        FALSE, NUM_BITMAPS, 0)) == NULL) 
        return FALSE; 

    // Add the open file, closed file, and document bitmaps. 
    hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FOLDER_RED)); 
    Image_Open = ImageList_Add(himl, hbmp, (HBITMAP) NULL); 
    DeleteObject(hbmp); 

    hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FOLDER_OPEN)); 
    Image_Closed = ImageList_Add(himl, hbmp, (HBITMAP) NULL); 
    DeleteObject(hbmp); 

    hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_FOLDER)); 
    Image_Root = ImageList_Add(himl, hbmp, (HBITMAP) NULL); 
    DeleteObject(hbmp); 

    // Fail if not all of the images were added. 
    if (ImageList_GetImageCount(himl) < 3) 
        return FALSE; 

    // Associate the image list with the tree view control. 
    TreeView_SetImageList(hwndTV, himl, TVSIL_NORMAL); 

    return TRUE; 
} 

// CreateTreeView - creates a tree view control. 
// Returns the handle to the new control if successful,
// or NULL otherwise. 
// hwndParent - handle to the control's parent window. 
// lpszFileName - name of the file to parse for tree view items.

static HWND CreateTreeView(HWND hwndParent, int id, LPSTR lpszFileName) 
{ 
    RECT rcClient;  // dimensions of client area 
    HWND hwndTV;    // handle to tree view control 
 
    // Get the dimensions of the parent window's client area, and create 
    // the tree view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndTV = CreateWindowEx(0, WC_TREEVIEW, "Tree View", 
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)id, hInst, NULL); 
/* 
    hwndTV = CreateWindow(_T("ListBox"), _T(""), WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|
						LBS_DISABLENOSCROLL|LBS_NOINTEGRALHEIGHT|LBS_OWNERDRAWFIXED|LBS_NOTIFY,
						0, 0, 0, 0, parent, (HMENU)id, Globals.hInstance, 0);
 */
    // Initialize the image list, and add items to the control. 
    if (!InitTreeViewImageLists(hwndTV) || !InitTreeViewItems(hwndTV, lpszFileName)) { 
        DestroyWindow(hwndTV); 
        return NULL; 
    } 
    return hwndTV;
} 

////////////////////////////////////////////////////////////////////////////////
static WNDPROC g_orgTreeWndProc;

static LRESULT CALLBACK TreeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	ChildWnd* child = (ChildWnd*)GetWindowLong(GetParent(hWnd), GWL_USERDATA);
	Pane* pane = (Pane*)GetWindowLong(hWnd, GWL_USERDATA);
	ASSERT(child);

	switch(message) {
#ifndef _NO_EXTENSIONS
		case WM_HSCROLL:
			set_header(pane);
			break;
#endif
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) { 
//            case LVN_GETDISPINFO: 
//                OnGetDispInfo((NMLVDISPINFO*)lParam); 
//                break; 
//            case LVN_ENDLABELEDIT: 
//                return OnEndLabelEdit((NMLVDISPINFO*)lParam);
//                break;
            }
//			break;
            return 0;

		case WM_SETFOCUS:
			child->nFocusPanel = pane == &child->right? 1: 0;
			//ListBox_SetSel(hWnd, TRUE, 1);
			//TODO: check menu items
			break;

		case WM_KEYDOWN:
			if (wParam == VK_TAB) {
				//TODO: SetFocus(Globals.hDriveBar)
				SetFocus(child->nFocusPanel ? child->left.hWnd: child->right.hWnd);
			}
	}

	return CallWindowProc(g_orgTreeWndProc, hWnd, message, wParam, lParam);
}


static void init_output(HWND hWnd)
{
	TCHAR b[16];
	HFONT old_font;
	HDC hdc = GetDC(hWnd);

	if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, _T("1000"), 0, b, 16) > 4)
		Globals.num_sep = b[1];
	else
		Globals.num_sep = _T('.');

	old_font = SelectFont(hdc, Globals.hFont);
	GetTextExtentPoint32(hdc, _T(" "), 1, &Globals.spaceSize);
	SelectFont(hdc, old_font);
	ReleaseDC(hWnd, hdc);
}


// insert treectrl entries after index idx
static void insert_tree_entries(Pane* pane, Entry* parent, int idx)
{
    static HTREEITEM hItemVisible;
    static int hItemVisibleIdx;

    HTREEITEM hItem;
	Entry* entry = parent;

	if (!entry)
		return;

	ShowWindow(pane->hWnd, SW_HIDE);
	for(; entry; entry=entry->next) {
#ifndef _LEFT_FILES
		if (pane->treePane && !(entry->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
			continue;
#endif
		// don't display entries "." and ".." in the left pane
        if (pane->treePane && (entry->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && entry->data.cFileName[0]==_T('.')) {
			if (
#ifndef _NO_EXTENSIONS
				entry->data.cFileName[1]==_T('\0') ||
#endif
                (entry->data.cFileName[1]==_T('.') && entry->data.cFileName[2]==_T('\0'))) {
				continue;
            }
        }
//		if (idx != -1)
//			idx++;
//		ListBox_InsertItemData(pane->hWnd, idx, entry);
        //TRACE("Adding item %u [level:%u] - %s\n", ++idx, entry->level, entry->data.cFileName); 

        //hItem = AddItemToTree(pane->hWnd, entry->data.cFileName, idx); 
        //hItem = AddItemToTree(pane->hWnd, entry->data.cFileName, entry->level); 
        hItem = AddEntryToTree(pane->hWnd, entry); 
        if (hItem != NULL) {
            entry->hTreeItem = hItem; // already done in AddEntryToTree

        }
        //TreeView_Expand(pane->hWnd, hItem, TVE_EXPAND);
        if (pane->treePane && entry->expanded) {
            insert_tree_entries(pane, entry->down, idx + 1);
            TreeView_Expand(pane->hWnd, hItem, TVE_EXPAND);
        }
        if (idx > hItemVisibleIdx) {
            hItemVisibleIdx = idx;
            hItemVisible = hItem;
        }
	}
    if (hItemVisible && idx == 0) {
        TreeView_SelectSetFirstVisible(pane->hWnd, hItemVisible);
    }
	ShowWindow(pane->hWnd, SW_SHOW);
}


//void create_tree_window(HWND parent, Pane* pane, int id, int id_header, LPSTR lpszFileName)
void CreateTreeWnd(HWND parent, Pane* pane, int id, LPSTR lpszFileName)
{
	static int s_init = 0;
	Entry* entry = pane->root;

	pane->treePane = 1;

    pane->hWnd = CreateTreeView(parent, id, lpszFileName);
    SetWindowLong(pane->hWnd, GWL_USERDATA, (LPARAM)pane);
	g_orgTreeWndProc = SubclassWindow(pane->hWnd, TreeWndProc);
	SendMessage(pane->hWnd, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);

	 // insert entries into treectrl
    if (entry) {
		//insert_tree_entries(pane, entry, -1);
		insert_tree_entries(pane, entry, 0);
    }

	 // calculate column widths
	if (!s_init) {
		s_init = 1;
		init_output(pane->hWnd);
	}
//	calc_widths(pane, TRUE);
//#ifndef _NO_EXTENSIONS
//	pane->hwndHeader = create_header(parent, pane, id_header);
//#endif
}



