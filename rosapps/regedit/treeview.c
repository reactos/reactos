/*
 *  ReactOS regedit
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
    
#include <windowsx.h>
#include <assert.h>
#define ASSERT assert
#include "main.h"
#include "treeview.h"


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

HTREEITEM AddItemToTree(HWND hwndTV, LPTSTR lpszItem, int nLevel)
{ 
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST; 
    static HTREEITEM hPrevRootItem = NULL; 
    static HTREEITEM hPrevLev2Item = NULL; 
    HTREEITEM hti; 
 
    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN  | TVIF_PARAM; 
 
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

static void init_output(HWND hWnd)
{
//	TCHAR b[16];
//	HFONT old_font;
	HDC hdc = GetDC(hWnd);

//	if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, _T("1000"), 0, b, 16) > 4)
//		Globals.num_sep = b[1];
//	else
//		Globals.num_sep = _T('.');

//	old_font = SelectFont(hdc, Globals.hFont);
//	GetTextExtentPoint32(hdc, _T(" "), 1, &Globals.spaceSize);
//	SelectFont(hdc, old_font);
	ReleaseDC(hWnd, hdc);
}
/*
HTREEITEM AddEntryToTree(HWND hwndTV, Entry* entry)
{ 
    HTREEITEM hItem = 0;
    return hItem;
}
 */

static BOOL InitTreeViewItems(HWND hwndTV) 
{ 
    HTREEITEM hItem;
    
    hItem = AddItemToTree(hwndTV, _T("My Computer"), 1); 
    AddItemToTree(hwndTV, _T("HKEY_CLASSES_ROOT"), 2); 
    AddItemToTree(hwndTV, _T("HKEY_CURRENT_USER"), 2); 
    AddItemToTree(hwndTV, _T("HKEY_LOCAL_MACHINE"), 2); 
    AddItemToTree(hwndTV, _T("HKEY_USERS"), 2); 
    AddItemToTree(hwndTV, _T("HKEY_CURRENT_CONFIG"), 2); 

    TreeView_Expand(hwndTV, hItem, TVE_EXPAND);
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
    hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_OPEN_FILE)); 
    Image_Open = ImageList_Add(himl, hbmp, (HBITMAP) NULL); 
    DeleteObject(hbmp); 

    hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CLOSED_FILE)); 
    Image_Closed = ImageList_Add(himl, hbmp, (HBITMAP) NULL); 
    DeleteObject(hbmp); 

    hbmp = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_ROOT)); 
    Image_Root = ImageList_Add(himl, hbmp, (HBITMAP) NULL); 
    DeleteObject(hbmp); 

    // Fail if not all of the images were added. 
    if (ImageList_GetImageCount(himl) < 3) 
        return FALSE; 

    // Associate the image list with the tree view control. 
    TreeView_SetImageList(hwndTV, himl, TVSIL_NORMAL); 

    return TRUE; 
} 

#ifndef _MSC_VER
#define NMTVDISPINFO TV_DISPINFO
#define NMTVDISPINFO TV_DISPINFO
#endif

static void OnGetDispInfo(NMTVDISPINFO* ptvdi)
{
/*
    Entry* entry = (Entry*)ptvdi->item.lParam;
    ASSERT(entry);

    if (ptvdi->item.mask & TVIF_CHILDREN ) {
        ptvdi->item.cChildren = 5;
    }
    if (ptvdi->item.mask & TVIF_IMAGE) {
        ptvdi->item.iImage = Image_Root; 
    }
    if (ptvdi->item.mask & TVIF_SELECTEDIMAGE) {
        ptvdi->item.iSelectedImage = Image_Closed; 
    }
    if (ptvdi->item.mask & TVIF_TEXT) {
        ptvdi->item.pszText = entry->data.cFileName; 
        ptvdi->item.cchTextMax = lstrlen(entry->data.cFileName); 
    }
static BOOL OnExpand(int flag, HTREEITEM* pti)
{ 
    TRACE(_T("TreeWndProc(...) OnExpand()\n"));
    return TRUE;
} 

static BOOL OnExpanding(HWND hWnd, NMTREEVIEW* pnmtv)
{ 
    static int expanding;

    if (expanding) return FALSE;
    expanding = TRUE;
    expanding = FALSE;
    return TRUE;
} 
 */
} 

/*
static BOOL OnSelChanged(NMTREEVIEW* pnmtv)
{ 
    LPARAM parm = pnmtv->itemNew.lParam;
    ChildWnd* child = (ChildWnd*)pnmtv->itemNew.lParam;
    return TRUE;
} 
 */

////////////////////////////////////////////////////////////////////////////////
static WNDPROC g_orgTreeWndProc;

static LRESULT CALLBACK TreeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
//	ChildWnd* child = (ChildWnd*)GetWindowLong(GetParent(hWnd), GWL_USERDATA);
//	Pane* pane = (Pane*)GetWindowLong(hWnd, GWL_USERDATA);
//	ASSERT(child);
	switch (message) {
    default:
        break;
	}
	return CallWindowProc(g_orgTreeWndProc, hWnd, message, wParam, lParam);
}

// CreateTreeView - creates a tree view control. 
// Returns the handle to the new control if successful,
// or NULL otherwise. 
// hwndParent - handle to the control's parent window. 

HWND CreateTreeView(HWND hwndParent/*, Pane* pane*/, int id, LPTSTR lpszPathName) 
{ 
    RECT rcClient;  // dimensions of client area 
    HWND hwndTV;    // handle to tree view control 
//	static int s_init = 0;
//	Entry* entry = pane->root;
//	pane->treePane = 1;
 
    // Get the dimensions of the parent window's client area, and create 
    // the tree view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndTV = CreateWindowEx(0, WC_TREEVIEW, _T("Tree View"), 
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)id, hInst, NULL); 
    // Initialize the image list, and add items to the control. 
    if (!InitTreeViewImageLists(hwndTV) || !InitTreeViewItems(hwndTV)) { 
        DestroyWindow(hwndTV); 
        return NULL; 
    } 

	SetWindowLong(hwndTV, GWL_USERDATA, (LPARAM)0);
	g_orgTreeWndProc = SubclassWindow(hwndTV, TreeWndProc);
	//SendMessage(hwndTV, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);

	 // insert entries into treectrl
//    if (entry) {
//		insert_tree_entries(hwndTV, entry, 0);
//    }

	// calculate column widths
//	if (!s_init) {
//		s_init = 1;
//		init_output(hwndTV);
//	}

    return hwndTV;
} 

