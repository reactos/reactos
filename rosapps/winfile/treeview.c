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


#if 0
/*
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
 */
#endif


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


HTREEITEM AddEntryToTree(HWND hwndTV, Entry* entry)
{ 
    HTREEITEM hItem = 0;
#if 0
    hItem = AddItemToTree(hwndTV, entry->data.cFileName, entry->level);
#else
//HTREEITEM AddItemToTree(HWND hwndTV, LPTSTR lpszItem, int nLevel)
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST; 
    static HTREEITEM hPrevRootItem = NULL; 
    static HTREEITEM hPrevLev2Item = NULL; 

    //TRACE("AddEntryToTree(level:%u - %s)\n", entry->level, entry->data.cFileName); 
    
    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM; 
/*
    // Set the text of the item. 
    tvi.pszText = entry->data.cFileName; 
    tvi.cchTextMax = lstrlen(entry->data.cFileName); 
    // Assume the item is not a parent item, so give it an image. 
    tvi.iImage = Image_Root; 
    tvi.iSelectedImage = Image_Root; 
    tvi.cChildren = 1; 
    // Save the heading level in the item's application-defined data area. 
    //tvi.lParam = (LPARAM)entry->level; 
 */
    tvi.pszText = LPSTR_TEXTCALLBACK;
    tvi.cchTextMax = 0;
    tvi.iImage = I_IMAGECALLBACK;
    tvi.iSelectedImage = I_IMAGECALLBACK;
    tvi.cChildren = I_CHILDRENCALLBACK;
    // Save the entry pointer in the item's application-defined data area. 
    tvi.lParam = (LPARAM)entry; 
 
    tvins.item = tvi; 
    tvins.hInsertAfter = hPrev; 
 
    // Set the parent item based on the specified level. 
    if (entry->level == 0) {
        tvins.hParent = TVI_ROOT; 
    } else if (entry->level == 1) {
        tvins.hParent = hPrevRootItem; 
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
/* 
    // The new item is a child item. Give the parent item a 
    // closed folder bitmap to indicate it now has child items. 
    if (entry->level > 1) { 
        HTREEITEM hti; 
        hti = TreeView_GetParent(hwndTV, hPrev); 
        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE; 
        tvi.hItem = hti; 
        tvi.iImage = Image_Closed; 
        tvi.iSelectedImage = Image_Closed; 
        TreeView_SetItem(hwndTV, &tvi); 
    } 
 */
    hItem = hPrev; 
#endif
    return hItem;
}


static BOOL InitTreeViewItems(HWND hwndTV) 
{ 
    return TRUE; 
} 
 
// insert treectrl entries after index idx
static void insert_tree_entries(HWND hWnd, Entry* entry, int idx)
{
    static HTREEITEM hItemVisible;
    static int hItemVisibleIdx;

	if (!entry)
		return;

	if (entry->hTreeItem)
		return;

	ShowWindow(hWnd, SW_HIDE);
	for(; entry; entry=entry->next) {
#ifndef _LEFT_FILES
		if (!(entry->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
			continue;
#endif
		// don't display entries "." and ".." in the left pane
        if ((entry->data.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && entry->data.cFileName[0]==_T('.')) {
			if (entry->data.cFileName[1] == _T('\0') ||
               (entry->data.cFileName[1] == _T('.')  && 
                entry->data.cFileName[2] == _T('\0'))) {
				continue;
            }
        }
//		if (idx != -1)
//			idx++;
//		ListBox_InsertItemData(hWnd, idx, entry);
        //TRACE("Adding item %u [level:%u] - %s\n", ++idx, entry->level, entry->data.cFileName); 

	
        if (entry->hTreeItem) continue;


        entry->hTreeItem = AddEntryToTree(hWnd, entry); 
        if (entry->expanded) {
            insert_tree_entries(hWnd, entry->down, idx + 1);
            TreeView_Expand(hWnd, entry->hTreeItem, TVE_EXPAND);
        }
        if (idx > hItemVisibleIdx) {
            hItemVisibleIdx = idx;
            hItemVisible = entry->hTreeItem;
        }
	}
    if (hItemVisible && idx == 0) {
        TreeView_SelectSetFirstVisible(hWnd, hItemVisible);
    }
	ShowWindow(hWnd, SW_SHOW);
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

static HWND CreateTreeView(HWND hwndParent, int id) 
{ 
    RECT rcClient;  // dimensions of client area 
    HWND hwndTV;    // handle to tree view control 
 
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
    return hwndTV;
} 

#ifndef _MSC_VER
#define NMTVDISPINFO TV_DISPINFO
#define NMTVDISPINFO TV_DISPINFO
#endif

static void OnGetDispInfo(NMTVDISPINFO* ptvdi)
{
//    static TCHAR buffer[200];
//    LVITEM* pItem = &(ptvdi->item);
//    Entry* entry = (Entry*)pItem->lParam;
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
} 
/*
typedef struct tagTVITEM{
    UINT      mask;
    HTREEITEM hItem;
    UINT      state;
    UINT      stateMask;
    LPTSTR    pszText;
    int       cchTextMax;
    int       iImage;
    int       iSelectedImage;
    int       cChildren;
    LPARAM    lParam;
} TVITEM, FAR *LPTVITEM;

TVITEM structure that identifies and contains information about the tree view item. The mask member of the TVITEM structure specifies which information is being set or retrieved. It can be one or more of the following values: TVIF_CHILDREN  The cChildren member specifies, or is to receive, a value that indicates whether the item has child items.  
TVIF_IMAGE  The iImage member specifies, or is to receive, the index of the item's nonselected icon in the image list.  
TVIF_SELECTEDIMAGE  The iSelectedImage member specifies, or is to receive, the index of the item's selected icon in the image list.  
TVIF_TEXT  The pszText member specifies the new item text or the address of a buffer that is to receive the item text. 
If the structure is receiving item text, you typically copy the text to the buffer pointed to by the pszText member of the TVITEM structure. However, you can return a string in the pszText member instead. If you do so, you cannot change or delete the string until the corresponding item text is deleted or until two additional TVN_GETDISPINFO notification messages have been sent. 
 
 */

// OnEndLabelEdit - processes the LVN_ENDLABELEDIT notification message. 
// Returns TRUE if the label is changed, or FALSE otherwise. 

static BOOL OnEndLabelEdit(NMTVDISPINFO* ptvdi)
{ 
//    if (ptvdi->item.iItem == -1) 
//        return FALSE; 
 
    // Copy the new label text to the application-defined structure. 
//    lstrcpyn(rgPetInfo[ptvdi->item.iItem].szKind, ptvdi->item.pszText, 10);
    
    return TRUE;
    // To make a more robust application you should send an EM_LIMITTEXT
    // message to the edit control to prevent the user from entering too
    // many characters in the field. 
} 


static BOOL OnExpand(int flag, HTREEITEM* pti)
{ 
    TRACE(_T("TreeWndProc(...) OnExpand()\n"));
//    pnmtv = (NMTREEVIEW) lParam 
    //TRACE("OnExpand(...) entry name: %s\n", entry->data.cFileName);
    /*
TVE_COLLAPSE Collapses the list.  
TVE_COLLAPSERESET Collapses the list and removes the child items. The TVIS_EXPANDEDONCE state flag is reset. This flag must be used with the TVE_COLLAPSE flag. 
TVE_EXPAND Expands the list. 
TVE_EXPANDPARTIAL Version 4.70. Partially expands the list. In this state, the child items are visible and the parent item's plus symbol is displayed. This flag must be used in combination with the TVE_EXPAND flag. 
TVE_TOGGLE Collapses the list if it is expanded or expands it if it is collapsed. 
     */
    return TRUE;
} 

static BOOL OnExpanding(HWND hWnd, NMTREEVIEW* pnmtv)
{ 
    static int expanding;

    //LPARAM parm = pnmtv->itemNew.lParam;
    Entry* entry = (Entry*)pnmtv->itemNew.lParam;

    TRACE(_T("TreeWndProc(...) OnExpanding() entry: %p\n"), entry);

    if (expanding) return FALSE;
    expanding = TRUE;
    if (entry) {
        insert_tree_entries(hWnd, entry->down, 0);
//		insert_tree_entries(hWnd, entry, 0);
    }
    expanding = FALSE;
    return TRUE;
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
	ChildWnd* child = (ChildWnd*)GetWindowLong(GetParent(hWnd), GWL_USERDATA);
	Pane* pane = (Pane*)GetWindowLong(hWnd, GWL_USERDATA);
	ASSERT(child);

	switch (message) {
#ifndef _NO_EXTENSIONS
		case WM_HSCROLL:
			set_header(pane);
			break;
#endif
        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) { 
            case TVM_EXPAND: 
                //return OnExpand((int)wParam, (HTREEITEM*)lParam);
                OnExpand((int)wParam, (HTREEITEM*)lParam);
                break;
            case TVN_GETDISPINFO: 
                OnGetDispInfo((NMTVDISPINFO*)lParam); 
                break; 
            case TVN_ITEMEXPANDING: 
                return OnExpanding(hWnd, (NMTREEVIEW*)lParam);
                break;
//            case TVN_SELCHANGED:
//                return OnSelChanged((NMTREEVIEW*)lParam);
//                break;
#if 0
            case TVN_SINGLEEXPAND:
                TRACE("TreeWndProc(...) TVN_SINGLEEXPAND\n");
                //lpnmtv = (LPNMTREEVIEW)lParam;
                //return TVNRET_DEFAULT;
//                return TVNRET_SKIPOLD; // Skip default processing of the item being unselected. 
//                return TVNRET_SKIPNEW; // Skip default processing of the item being selected. 
                break;
#endif
            case TVN_ENDLABELEDIT: 
                return OnEndLabelEdit((NMTVDISPINFO*)lParam);
                break;
            }
            return 0;
			break;
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
            break;
	}
	return CallWindowProc(g_orgTreeWndProc, hWnd, message, wParam, lParam);
}

//void create_tree_window(HWND parent, Pane* pane, int id, int id_header, LPTSTR lpszFileName)
void CreateTreeWnd(HWND parent, Pane* pane, int id)
{
	static int s_init = 0;
	Entry* entry = pane->root;

	pane->treePane = 1;

    pane->hWnd = CreateTreeView(parent, id);
    SetWindowLong(pane->hWnd, GWL_USERDATA, (LPARAM)pane);
	g_orgTreeWndProc = SubclassWindow(pane->hWnd, TreeWndProc);
	SendMessage(pane->hWnd, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);

	 // insert entries into treectrl
    if (entry) {
		insert_tree_entries(pane->hWnd, entry, 0);
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
