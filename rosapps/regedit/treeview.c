/*
 *  ReactOS regedit
 *
 *  treeview.c
 *
 *  Copyright (C) 2002  Robert Dickenson <robd@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>
    
#include "main.h"
#include "treeview.h"


// Global variables and constants 
// Image_Open, Image_Closed, and Image_Root - integer variables for indexes of the images. 
// CX_BITMAP and CY_BITMAP - width and height of an icon. 
// NUM_BITMAPS - number of bitmaps to add to the image list. 
int Image_Open; 
int Image_Closed; 
int Image_Root; 

#define CX_BITMAP    16
#define CY_BITMAP    16
#define NUM_BITMAPS  3


HKEY FindRegRoot(HWND hwndTV, HTREEITEM hItem, LPTSTR keyPath, int* pPathLen, int max)
{
    HKEY hKey = NULL;
    TVITEM item;
    item.mask = TVIF_PARAM;
    item.hItem = TreeView_GetParent(hwndTV, hItem);

    if (TreeView_GetItem(hwndTV, &item)) {
        if (item.lParam == 0) {
            // recurse
            hKey = FindRegRoot(hwndTV, item.hItem, keyPath, pPathLen, max);
            keyPath[*pPathLen] = _T('\\');
            ++(*pPathLen);
            item.mask = TVIF_TEXT;
            item.hItem = hItem;
            item.pszText = &keyPath[*pPathLen];
            item.cchTextMax = max - *pPathLen;
            if (TreeView_GetItem(hwndTV, &item)) {
                *pPathLen += _tcslen(item.pszText);
            }
        } else {
            // found root key with valid key value
            hKey = (HKEY)item.lParam;
            item.mask = TVIF_TEXT;
            item.hItem = hItem;
//            item.pszText = &keyPath[*pPathLen];
            item.pszText = keyPath;
            item.cchTextMax = max;
            if (TreeView_GetItem(hwndTV, &item)) {
                *pPathLen += _tcslen(item.pszText);
            }
        }
    }
    return hKey;
}

static HTREEITEM AddEntryToTree(HWND hwndTV, HTREEITEM hParent, LPTSTR label, HKEY hKey, DWORD dwChildren)
{ 
    HTREEITEM hItem = 0;
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM; 
    tvi.pszText = label; 
    tvi.cchTextMax = lstrlen(tvi.pszText); 
    tvi.iImage = Image_Closed; 
    tvi.iSelectedImage = Image_Open; 
    tvi.cChildren = dwChildren; 
    tvi.lParam = (LPARAM)hKey;
    tvins.item = tvi; 
    if (hKey) tvins.hInsertAfter = (HTREEITEM)TVI_LAST; 
    else      tvins.hInsertAfter = (HTREEITEM)TVI_SORT; 
    tvins.hParent = hParent; 
    hItem = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);
    return hItem;
}


static BOOL InitTreeViewItems(HWND hwndTV, LPTSTR pHostName) 
{ 
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    HTREEITEM hRoot; 

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM; 
    // Set the text of the item. 
    tvi.pszText = pHostName; 
    tvi.cchTextMax = lstrlen(tvi.pszText); 
    // Assume the item is not a parent item, so give it an image. 
    tvi.iImage = Image_Root; 
    tvi.iSelectedImage = Image_Root; 
    tvi.cChildren = 5; 
    // Save the heading level in the item's application-defined data area. 
    tvi.lParam = (LPARAM)NULL;
    tvins.item = tvi; 
    tvins.hInsertAfter = (HTREEITEM)TVI_FIRST; 
    tvins.hParent = TVI_ROOT; 
    // Add the item to the tree view control. 
    hRoot = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins); 

    AddEntryToTree(hwndTV, hRoot, _T("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT, 1);
    AddEntryToTree(hwndTV, hRoot, _T("HKEY_CURRENT_USER"), HKEY_CURRENT_USER, 1);
    AddEntryToTree(hwndTV, hRoot, _T("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE, 1);
    AddEntryToTree(hwndTV, hRoot, _T("HKEY_USERS"), HKEY_USERS, 1);
    AddEntryToTree(hwndTV, hRoot, _T("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG, 1);

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

BOOL OnTreeExpanding(HWND hwndTV, NMTREEVIEW* pnmtv)
{ 
    HKEY hKey;
    TCHAR keyPath[1000];
    int keyPathLen = 0;

    static int expanding;
    if (expanding) return FALSE;
    if (pnmtv->itemNew.state & TVIS_EXPANDEDONCE ) {
        return TRUE;
    }
    expanding = TRUE;

    // check if this is either the root or a subkey item...
    if ((HKEY)pnmtv->itemNew.lParam == NULL) {
        keyPath[0] = _T('\0');
        hKey = FindRegRoot(hwndTV, pnmtv->itemNew.hItem, keyPath, &keyPathLen, sizeof(keyPath)/sizeof(TCHAR));
    } else {
        hKey = (HKEY)pnmtv->itemNew.lParam;
        keyPath[0] = _T('\0');
    }

    if (hKey != NULL) {
        HKEY hNewKey;
        LONG errCode = RegOpenKeyEx(hKey, keyPath, 0, KEY_READ, &hNewKey);
        if (errCode == ERROR_SUCCESS) {
            TCHAR Name[MAX_NAME_LEN];
            DWORD cName = MAX_NAME_LEN;
            FILETIME LastWriteTime;
            DWORD dwIndex = 0L;
            //ShowWindow(hwndTV, SW_HIDE);
            while (RegEnumKeyEx(hNewKey, dwIndex, Name, &cName, NULL, NULL, NULL, &LastWriteTime) == ERROR_SUCCESS) {
                DWORD dwCount = 0L;
                errCode = RegOpenKeyEx(hNewKey, Name, 0, KEY_READ, &hKey);
                if (errCode == ERROR_SUCCESS) {
                    TCHAR SubName[MAX_NAME_LEN];
                    DWORD cSubName = MAX_NAME_LEN;
//                    if (RegEnumKeyEx(hKey, 0, SubName, &cSubName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                    while (RegEnumKeyEx(hKey, dwCount, SubName, &cSubName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                        ++dwCount;
                    }
                }
                RegCloseKey(hKey);
                AddEntryToTree(hwndTV, pnmtv->itemNew.hItem, Name, NULL, dwCount);
                cName = MAX_NAME_LEN;
                ++dwIndex;
            }
	        //ShowWindow(hwndTV, SW_SHOWNOACTIVATE);
            RegCloseKey(hNewKey);
        }
    } else {
    }
    expanding = FALSE;
    return TRUE;
} 

// CreateTreeView - creates a tree view control. 
// Returns the handle to the new control if successful, or NULL otherwise. 
// hwndParent - handle to the control's parent window. 

HWND CreateTreeView(HWND hwndParent, LPTSTR pHostName, int id) 
{ 
    RECT rcClient;
    HWND hwndTV;
 
    // Get the dimensions of the parent window's client area, and create the tree view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndTV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, _T("Tree View"), 
        WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)id, hInst, NULL); 
    // Initialize the image list, and add items to the control. 
    if (!InitTreeViewImageLists(hwndTV) || !InitTreeViewItems(hwndTV, pHostName)) { 
        DestroyWindow(hwndTV); 
        return NULL; 
    } 
    return hwndTV;
} 
