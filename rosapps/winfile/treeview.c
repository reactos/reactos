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

#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <process.h>
#include <stdio.h>

#include <windowsx.h>
#include <shellapi.h>
#include <ctype.h>
//#include <assert.h>
//#define ASSERT assert

#include "main.h"
#include "treeview.h"
#include "entries.h"
#include "utils.h"

#include "trace.h"


// Global variables and constants 
// Image_Open, Image_Closed, and Image_Root - integer variables for indexes of the images. 
// CX_BITMAP and CY_BITMAP - width and height of an icon. 
// NUM_BITMAPS - number of bitmaps to add to the image list. 
int Image_Open; 
int Image_Closed; 
int Image_Root; 

static int s_init = 0;

#define CX_BITMAP    16
#define CY_BITMAP    16
#define NUM_BITMAPS  3


Root* FindPathRoot(HWND hwndTV, HTREEITEM hItem, LPTSTR szPath, int* pPathLen, int max)
{
	Root* pRoot = NULL;
    TVITEM item;
    item.mask = TVIF_PARAM;
    item.hItem = TreeView_GetParent(hwndTV, hItem);

    if (TreeView_GetItem(hwndTV, &item)) {
        if (item.lParam == 0) {
            // recurse
            pRoot = FindPathRoot(hwndTV, item.hItem, szPath, pPathLen, max);
            szPath[*pPathLen] = _T('\\');
            ++(*pPathLen);
            item.mask = TVIF_TEXT;
            item.hItem = hItem;
            item.pszText = &szPath[*pPathLen];
            item.cchTextMax = max - *pPathLen;
            if (TreeView_GetItem(hwndTV, &item)) {
                *pPathLen += _tcslen(item.pszText);
            }
        } else {
            // found root key with valid key value
            pRoot = (Root*)item.lParam;
            item.mask = TVIF_TEXT;
            item.hItem = hItem;
            item.pszText = szPath;
            item.cchTextMax = max;
            if (TreeView_GetItem(hwndTV, &item)) {
                *pPathLen += _tcslen(item.pszText);
            }
        }
    }
    return pRoot;
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

#if 0

HTREEITEM AddEntryToTree(HWND hwndTV, Entry* entry)
{ 
    HTREEITEM hItem = 0;
    TVITEM tvi; 
    TVINSERTSTRUCT tvins; 
    static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST; 
    static HTREEITEM hPrevRootItem = NULL; 
    static HTREEITEM hPrevLev2Item = NULL; 

    //TRACE("AddEntryToTree(level:%u - %s)\n", entry->level, entry->data.cFileName); 
    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM; 
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
    hItem = hPrev; 
    return hItem;
}

#else

static HTREEITEM AddEntryToTree(HWND hwndTV, HTREEITEM hParent, LPTSTR label, Root* entry, DWORD dwChildren)
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
    tvi.lParam = (LPARAM)entry;
    tvins.item = tvi; 
    if (entry) tvins.hInsertAfter = (HTREEITEM)TVI_LAST; 
    else       tvins.hInsertAfter = (HTREEITEM)TVI_SORT; 
    tvins.hParent = hParent; 
    hItem = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);
    return hItem;
}

#endif

// insert treectrl entries after index idx
static void insert_tree_entries(HWND hWnd, Entry* entry/*Root* pRoot*/, int idx)
{
    static HTREEITEM hItemVisible;
    static int hItemVisibleIdx;
//	Entry* entry = &pRoot->entry;

	if (!entry)	return;
	if (entry->hTreeItem) return;
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
        if (entry->hTreeItem) continue;
//        entry->hTreeItem = AddEntryToTree(hWnd, entry); 

		if (entry->up && entry->up->hTreeItem) {
//            entry->hTreeItem = AddEntryToTree(hWnd, entry->up->hTreeItem, entry->data.cFileName, entry, 0); 
            entry->hTreeItem = AddEntryToTree(hWnd, entry->up->hTreeItem, entry->data.cFileName, NULL, 1); 
		} else {
            entry->hTreeItem = AddEntryToTree(hWnd, TVI_ROOT, entry->data.cFileName, NULL, 0); 
//                AddEntryToTree(hwndTV, pnmtv->itemNew.hItem, Name, NULL, dwCount);
		}
        if (entry->expanded) {
//            insert_tree_entries(hWnd, entry->down, idx + 1);
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

static BOOL InitTreeViewItems(HWND hwndTV, ChildWnd* pChildWnd) 
{ 
    TVITEM tvi;
    TVINSERTSTRUCT tvins;
    HTREEITEM hRoot;
	TCHAR buffer[MAX_PATH];

	wsprintf(buffer, _T("%s - [%s]"), pChildWnd->pRoot->path, pChildWnd->pRoot->fs);
    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM; 
    tvi.pszText = buffer;
    tvi.cchTextMax = lstrlen(tvi.pszText);
    tvi.iImage = Image_Root;
    tvi.iSelectedImage = Image_Root;
    tvi.cChildren = 5;
//    tvi.lParam = (LPARAM)&pChildWnd->pRoot->entry;
    tvi.lParam = (LPARAM)pChildWnd->pRoot;
//    tvi.lParam = (LPARAM)pChildWnd;
    tvins.item = tvi;
    tvins.hInsertAfter = (HTREEITEM)TVI_FIRST;
    tvins.hParent = TVI_ROOT;
    // Add the item to the tree view control.
    hRoot = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM, 0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);
    pChildWnd->pRoot->entry.hTreeItem = hRoot;
//    TreeView_Expand(hwndTV, hRoot, TVE_EXPAND);
//    insert_tree_entries(hwndTV, &pChildWnd->pRoot->entry, 0);
    // insert entries into treectrl
//    if (pChildWnd->pRoot) {
//		insert_tree_entries(hwndTV, &pChildWnd->pRoot->entry, 0);
//    }
//    TreeView_Expand(hwndTV, pChildWnd->pRoot->entry.hTreeItem, TVE_EXPAND);
	// calculate column widths
	if (!s_init) {
		s_init = 1;
		init_output(hwndTV);
	}
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

BOOL OnTreeExpanding(HWND hwndTV, NMTREEVIEW* pnmtv)
{ 
	Root* pRoot = NULL;
    TCHAR szPath[1000];
    int keyPathLen = 0;

    static int expanding;
    if (expanding) return FALSE;
    if (pnmtv->itemNew.state & TVIS_EXPANDEDONCE ) {
        return TRUE;
    }
    expanding = TRUE;
    // check if this is either the root or a subkey item...
    if ((Root*)pnmtv->itemNew.lParam == NULL) {
        szPath[0] = _T('\0');
        pRoot = FindPathRoot(hwndTV, pnmtv->itemNew.hItem, szPath, &keyPathLen, sizeof(szPath)/sizeof(TCHAR));
    } else {
        pRoot = (Root*)pnmtv->itemNew.lParam;
        szPath[0] = _T('\0');
    }
    if (pRoot != NULL) {
//        Root* pNewRoot = NULL;
//        insert_tree_entries(hwndTV, &pRoot->entry, 0);

        insert_tree_entries(hwndTV, pRoot->entry.down, 0);

//		entry->hTreeItem = AddEntryToTree(hWnd, entry->up->hTreeItem, entry->data.cFileName, NULL, 1); 

/*
		HKEY hNewKey;
        LONG errCode = RegOpenKeyEx(hKey, szPath, 0, KEY_READ, &hNewKey);
        if (errCode == ERROR_SUCCESS) {
            TCHAR Name[MAX_PATH];
            DWORD cName = MAX_PATH;
            FILETIME LastWriteTime;
            DWORD dwIndex = 0L;
            //ShowWindow(hwndTV, SW_HIDE);
            while (RegEnumKeyEx(hNewKey, dwIndex, Name, &cName, NULL, NULL, NULL, &LastWriteTime) == ERROR_SUCCESS) {
                DWORD dwCount = 0L;
                errCode = RegOpenKeyEx(hNewKey, Name, 0, KEY_READ, &hKey);
                if (errCode == ERROR_SUCCESS) {
                    TCHAR SubName[MAX_PATH];
                    DWORD cSubName = MAX_PATH;
                    while (RegEnumKeyEx(hKey, dwCount, SubName, &cSubName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                        ++dwCount;
                    }
                }
                RegCloseKey(hKey);
                AddEntryToTree(hwndTV, pnmtv->itemNew.hItem, Name, NULL, dwCount);
                cName = MAX_PATH;
                ++dwIndex;
            }
	        //ShowWindow(hwndTV, SW_SHOWNOACTIVATE);
            RegCloseKey(hNewKey);
        }
 */
    } else {
    }
    expanding = FALSE;
    return TRUE;
} 
/*
static void read_directory_win(Entry* parent, LPCTSTR path)
{
	Entry* entry = (Entry*)malloc(sizeof(Entry));
	int level = parent->level + 1;
	Entry* last = 0;
	HANDLE hFind;
	HANDLE hFile;

	TCHAR buffer[MAX_PATH], *p;
	for(p=buffer; *path; )
		*p++ = *path++;
	lstrcpy(p, _T("\\*"));
    memset(entry, 0, sizeof(Entry));
	hFind = FindFirstFile(buffer, &entry->data);
	if (hFind != INVALID_HANDLE_VALUE) {
		parent->down = entry;
		do {
			entry->down = 0;
			entry->up = parent;
			entry->expanded = FALSE;
			entry->scanned = FALSE;
			entry->level = level;
			entry->unix_dir = FALSE;
			entry->bhfi_valid = FALSE;
			lstrcpy(p+1, entry->data.cFileName);
			hFile = CreateFile(buffer, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
								0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
			if (hFile != INVALID_HANDLE_VALUE) {
				if (GetFileInformationByHandle(hFile, &entry->bhfi))
					entry->bhfi_valid = TRUE;

				CloseHandle(hFile);
			}
			last = entry;
			entry = (Entry*) malloc(sizeof(Entry));
            memset(entry, 0, sizeof(Entry));
			if (last)
				last->next = entry;
		} while(FindNextFile(hFind, &entry->data));
		last->next = 0;
		FindClose(hFind);
	} else {
		parent->down = 0;
	}
	free(entry);
	parent->scanned = TRUE;
}


void read_directory(Entry* parent, LPCTSTR path, int sortOrder)
{
	TCHAR buffer[MAX_PATH];
	Entry* entry;
	LPCTSTR s;
	PTSTR d;

		read_directory_win(parent, path);
		if (Globals.prescan_node) {
			s = path;
			d = buffer;
			while(*s)
				*d++ = *s++;
			*d++ = _T('\\');
			for(entry=parent->down; entry; entry=entry->next)
				if (entry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
					lstrcpy(d, entry->data.cFileName);
					read_directory_win(entry, buffer);
					SortDirectory(entry, sortOrder);
				}
		}
		SortDirectory(parent, sortOrder);
}

Entry* read_tree_win(Root* root, LPCTSTR path, int sortOrder)
{
	TCHAR buffer[MAX_PATH];
	Entry* entry = &root->entry;
	LPCTSTR s = path;
	PTSTR d = buffer;

	entry->unix_dir = FALSE;
	while(entry) {
		while(*s && *s!=_T('\\') && *s!=_T('/'))
			*d++ = *s++;
		while(*s==_T('\\') || *s==_T('/'))
			s++;
		*d++ = _T('\\');
		*d = _T('\0');
		read_directory(entry, buffer, sortOrder);
		if (entry->down)
			entry->expanded = TRUE;
		if (!*s)
			break;
		entry = find_entry_win(entry, s);
	}
	return entry;
}

 */
void OnGetDispInfo(NMTVDISPINFO* ptvdi)
{
//    static TCHAR buffer[200];
//    LVITEM* pItem = &(ptvdi->item);
//    Entry* entry = (Entry*)pItem->lParam;
    Root* entry = (Root*)ptvdi->item.lParam;
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
//        ptvdi->item.pszText = entry->data.cFileName; 
//        ptvdi->item.cchTextMax = lstrlen(entry->data.cFileName); 
    }
} 

void UpdateStatus(HWND hWnd, Entry* pEntry)
{
    int file_count = 0;
    int files_size = 0;
    TCHAR suffix[10];
    TCHAR number[50];
    TCHAR Text[260];

    while (pEntry) {
        if (pEntry->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        } else {
            ++file_count;
            files_size += pEntry->data.nFileSizeLow;
        }
        pEntry = pEntry->next;
    };
    _tcscpy(suffix, _T(" bytes"));
    {
        NUMBERFMT numFmt;
        memset(&numFmt, 0, sizeof(numFmt));
        numFmt.NumDigits = 0;
        numFmt.LeadingZero = 0;
        numFmt.Grouping = 3;
        numFmt.lpDecimalSep = _T(".");
        numFmt.lpThousandSep = _T(",");
        numFmt.NegativeOrder = 0;

        wsprintf(Text, _T("%d"), files_size);
        if (GetNumberFormat(LOCALE_USER_DEFAULT, 0, Text, &numFmt, number, sizeof(number))) {
            wsprintf(Text, _T("Total %d file(s) (%s%s)"), file_count, number, suffix);
        } else {
            wsprintf(Text, _T("Total %d file(s) (%d%s)"), file_count, files_size, suffix);
        }
        SendMessage(Globals.hStatusBar, SB_SETTEXT, 1, (LPARAM)Text);
    }
}

// CreateTreeView - creates a tree view control. 
// Returns the handle to the new control if successful, or NULL otherwise. 
// hwndParent - handle to the control's parent window. 

HWND CreateTreeView(HWND hwndParent, ChildWnd* pChildWnd, int id) 
{ 
    RECT rcClient;
    HWND hwndTV;
 
    // Get the dimensions of the parent window's client area, and create the tree view control. 
    GetClientRect(hwndParent, &rcClient); 
    hwndTV = CreateWindowEx(0, WC_TREEVIEW, _T("Tree View"), 
        WS_VISIBLE | WS_CHILD | WS_BORDER | WS_EX_CLIENTEDGE | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
        0, 0, rcClient.right, rcClient.bottom, 
        hwndParent, (HMENU)id, hInst, NULL); 
    // Initialize the image list, and add items to the control. 
    if (!InitTreeViewImageLists(hwndTV) || !InitTreeViewItems(hwndTV, pChildWnd)) { 
        DestroyWindow(hwndTV); 
        return NULL; 
    } 
	SendMessage(hwndTV, WM_SETFONT, (WPARAM)Globals.hFont, FALSE);
    return hwndTV;
} 

////////////////////////////////////////////////////////////////////////////////

#ifdef _SUBWND_TREEVIEW

static WNDPROC g_orgTreeWndProc;

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
//        case TVM_EXPAND: 
//            OnTreeExpand((int)wParam, (HTREEITEM*)lParam);
//            break;
        case TVN_GETDISPINFO: 
            OnGetDispInfo((NMTVDISPINFO*)lParam); 
            break; 
        case TVN_ITEMEXPANDING: 
            return OnTreeExpanding(hWnd, (NMTREEVIEW*)lParam);
            break;
        case TVN_SELCHANGED:

            UpdateStatus(hWnd, child->left.cur->down);
            break;
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
        if (!child->nFocusPanel) {
            UpdateStatus(hWnd, pane->cur);
        }
		break;
	case WM_KEYDOWN:
		if (wParam == VK_TAB) {
			//TODO: SetFocus(Globals.hDriveBar)
			SetFocus(child->nFocusPanel ? child->hTreeWnd: child->hListWnd);
		}
        break;
	}
	return CallWindowProc(g_orgTreeWndProc, hWnd, message, wParam, lParam);
}

void CreateTreeWnd(HWND parent, Pane* pane, int id)
{
	Entry* entry = pane->root;
    pane->hWnd = CreateTreeView(parent, NULL, id);
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
}

#endif
