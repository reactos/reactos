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

#include "trace.h"


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


static HTREEITEM AddEntryToTree(HWND hwndTV, Entry* entry, LPTSTR label)
{ 
    HTREEITEM hItem = 0;
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
    if (label != NULL) {
        tvi.pszText = label;
        tvi.cchTextMax = _tcslen(label);
    } else {
        tvi.pszText = LPSTR_TEXTCALLBACK;
        tvi.cchTextMax = 0;
    }
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
    hItem = hPrev; 
    return hItem;
}

static Entry* CreateEntry(Entry* pParentEntry, HKEY hKey, LPCTSTR szKeyName)
{
    Entry* pEntry = NULL;

    pEntry = malloc(sizeof(Entry));
    memset(pEntry, 0, sizeof(Entry));
    //pEntry->up = pParentEntry;
    pEntry->level = 1;
    pEntry->hKey = hKey;
    _tcsncpy(pEntry->szName, szKeyName, MAX_NAME_LEN);
//    pEntry->hTreeItem = AddEntryToTree(hwndTV, pEntry, szKeyName); 
    return pEntry;
}

static BOOL InitTreeViewItems(HWND hwndTV, Root* pRoot/*LPCTSTR pHostName*/) 
{ 
//    HKEY hKey;
//    LONG errCode;
    Entry* pEntry = &pRoot->entry;
    HTREEITEM hRootItem;

//    TCHAR* pHostName = _T("My Computer");
//    HTREEITEM hRootItem = AddItemToTree(hwndTV, pHostName, 1); 
//    pEntry = malloc(sizeof(Entry));
//    memset(pEntry, 0, sizeof(Entry));

    pEntry->level = 0;
    pEntry->hTreeItem = AddEntryToTree(hwndTV, pEntry, pRoot->path); 
    hRootItem = pEntry->hTreeItem;

    pEntry = CreateEntry(&pRoot->entry, HKEY_CLASSES_ROOT, _T("HKEY_CLASSES_ROOT")); 
    pEntry->hTreeItem = AddEntryToTree(hwndTV, pEntry, NULL); 
    pEntry = CreateEntry(&pRoot->entry, HKEY_CURRENT_USER, _T("HKEY_CURRENT_USER")); 
    pEntry->hTreeItem = AddEntryToTree(hwndTV, pEntry, NULL); 
    pEntry = CreateEntry(&pRoot->entry, HKEY_LOCAL_MACHINE, _T("HKEY_LOCAL_MACHINE")); 
    pEntry->hTreeItem = AddEntryToTree(hwndTV, pEntry, NULL); 
    pEntry = CreateEntry(&pRoot->entry, HKEY_USERS, _T("HKEY_USERS")); 
    pEntry->hTreeItem = AddEntryToTree(hwndTV, pEntry, NULL); 
    pEntry = CreateEntry(&pRoot->entry, HKEY_CURRENT_CONFIG, _T("HKEY_CURRENT_CONFIG")); 
    pEntry->hTreeItem = AddEntryToTree(hwndTV, pEntry, NULL); 
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
    Entry* pEntry = (Entry*)ptvdi->item.lParam;
    ASSERT(pEntry);

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
/*
        ptvdi->item.pszText = _T("Unknown"); 
        ptvdi->item.cchTextMax = _tcslen(ptvdi->item.pszText); 
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
                ptvdi->item.pszText = Class; 
                ptvdi->item.cchTextMax = cClass; 
            }
        }
 */
        ptvdi->item.pszText = pEntry->szName; 
        ptvdi->item.cchTextMax = lstrlen(pEntry->szName); 
    }
}

static BOOL OnExpand(int flag, HTREEITEM* pti)
{ 
    TRACE(_T("TreeWndProc(...) OnExpand()\n"));
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
    HKEY hKey;
    LONG errCode;

    Entry* entry = (Entry*)pnmtv->itemNew.lParam;
    TRACE(_T("TreeWndProc(...) OnExpanding() entry: %p\n"), entry);
    if (expanding) return FALSE;
    expanding = TRUE;
    if (entry) {
        errCode = RegOpenKeyEx(entry->hKey, NULL, 0, KEY_READ, &hKey);
        if (errCode == ERROR_SUCCESS) {
            TCHAR Name[MAX_NAME_LEN];
            DWORD cName = MAX_NAME_LEN;
            FILETIME LastWriteTime;
            DWORD dwIndex = 0L;
            while (RegEnumKeyEx(hKey, dwIndex, Name, &cName, NULL, NULL, NULL, &LastWriteTime) == ERROR_SUCCESS) {
                Entry* pEntry = CreateEntry(entry, hKey, Name); 
                pEntry->up = entry;
                pEntry->hKey = hKey;
                pEntry->bKey = TRUE;
                pEntry->level = 2;
                pEntry->hTreeItem = AddEntryToTree(hWnd, pEntry, NULL); 
                cName = MAX_NAME_LEN;
                ++dwIndex;
            }
            RegCloseKey(hKey);
        }
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
//	ChildWnd* child = (ChildWnd*)GetWindowLong(GetParent(hWnd), GWL_USERDATA);
//	ASSERT(child);

	switch (message) {
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
//        case TVN_SELCHANGED:
//            return OnSelChanged((NMTREEVIEW*)lParam);
//           break;
#if 0
        case TVN_SINGLEEXPAND:
            TRACE("TreeWndProc(...) TVN_SINGLEEXPAND\n");
            //lpnmtv = (LPNMTREEVIEW)lParam;
            //return TVNRET_DEFAULT;
//            return TVNRET_SKIPOLD; // Skip default processing of the item being unselected. 
//            return TVNRET_SKIPNEW; // Skip default processing of the item being selected. 
            break;
#endif
        }
//        return 0;
		break;
	case WM_KEYDOWN:
		if (wParam == VK_TAB) {
			//SetFocus(child->nFocusPanel ? child->left.hWnd: child->right.hWnd);
		}
        break;
	}
	return CallWindowProc(g_orgTreeWndProc, hWnd, message, wParam, lParam);
}

// CreateTreeView - creates a tree view control. 
// Returns the handle to the new control if successful, or NULL otherwise. 
// hwndParent - handle to the control's parent window. 

HWND CreateTreeView(HWND hwndParent, int id, Root* pRoot) 
{ 
    RECT rcClient;
    HWND hwndTV;
//	Entry* entry = ;
 
    // Get the dimensions of the parent window's client area, and create the tree view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndTV = CreateWindowEx(0, WC_TREEVIEW, _T("Tree View"), 
        WS_VISIBLE | WS_CHILD | WS_BORDER | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)id, hInst, NULL); 
    // Initialize the image list, and add items to the control. 
    if (!InitTreeViewImageLists(hwndTV) || !InitTreeViewItems(hwndTV, pRoot)) { 
        DestroyWindow(hwndTV); 
        return NULL; 
    } 
	SetWindowLong(hwndTV, GWL_USERDATA, (LPARAM)0);
	g_orgTreeWndProc = SubclassWindow(hwndTV, TreeWndProc);
	//SendMessage(hwndTV, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);
    return hwndTV;
} 

