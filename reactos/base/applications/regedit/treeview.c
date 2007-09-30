/*
 * Regedit treeview
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
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

#include <regedit.h>

/* Global variables and constants  */
/* Image_Open, Image_Closed, and Image_Root - integer variables for indexes of the images.  */
/* CX_ICON and CY_ICON - width and height of an icon.  */
/* NUM_ICON - number of icons to add to the image list.  */
int Image_Open;
int Image_Closed;
int Image_Root;

static LPTSTR pathBuffer;

#define CX_ICON    16
#define CY_ICON    16
#define NUM_ICONS    3

static BOOL get_item_path(HWND hwndTV, HTREEITEM hItem, HKEY* phKey, LPTSTR* pKeyPath, int* pPathLen, int* pMaxLen)
{
    TVITEM item;
    size_t maxLen, len;
    LPTSTR newStr;

    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    if (!TreeView_GetItem(hwndTV, &item)) return FALSE;

    if (item.lParam) {
	/* found root key with valid key value */
	*phKey = (HKEY)item.lParam;
	return TRUE;
    }

    if(!get_item_path(hwndTV, TreeView_GetParent(hwndTV, hItem), phKey, pKeyPath, pPathLen, pMaxLen)) return FALSE;
    if (*pPathLen) {
        (*pKeyPath)[*pPathLen] = _T('\\');
        ++(*pPathLen);
    }

    do {
        item.mask = TVIF_TEXT;
        item.hItem = hItem;
        item.pszText = *pKeyPath + *pPathLen;
        maxLen = *pMaxLen - *pPathLen;
        item.cchTextMax = (int) maxLen;
        if (!TreeView_GetItem(hwndTV, &item)) return FALSE;
        len = _tcslen(item.pszText);
	if (len < maxLen - 1) {
            *pPathLen += (int) len;
            break;
	}
	newStr = HeapReAlloc(GetProcessHeap(), 0, *pKeyPath, *pMaxLen * 2);
	if (!newStr) return FALSE;
	*pKeyPath = newStr;
	*pMaxLen *= 2;
    } while(TRUE);

    return TRUE;
}

LPCTSTR GetItemPath(HWND hwndTV, HTREEITEM hItem, HKEY* phRootKey)
{
    int pathLen = 0, maxLen;

    *phRootKey = NULL;
    if (!pathBuffer) pathBuffer = HeapAlloc(GetProcessHeap(), 0, 1024);
    if (!pathBuffer) return NULL;
    *pathBuffer = 0;
    maxLen = (int) HeapSize(GetProcessHeap(), 0, pathBuffer);
    if (maxLen == -1) return NULL;
    if (!hItem) hItem = TreeView_GetSelection(hwndTV);
    if (!hItem) return NULL;
    if (!get_item_path(hwndTV, hItem, phRootKey, &pathBuffer, &pathLen, &maxLen)) {
		return NULL;
	}
    return pathBuffer;
}

BOOL DeleteNode(HWND hwndTV, HTREEITEM hItem)
{
    if (!hItem) hItem = TreeView_GetSelection(hwndTV);
    if (!hItem) return FALSE;
    return TreeView_DeleteItem(hwndTV, hItem);
}

/* Add an entry to the tree. Only give hKey for root nodes (HKEY_ constants) */
static HTREEITEM AddEntryToTree(HWND hwndTV, HTREEITEM hParent, LPTSTR label, HKEY hKey, DWORD dwChildren)
{
    TVITEM tvi;
    TVINSERTSTRUCT tvins;

    if (hKey) {
        if (RegQueryInfoKey(hKey, 0, 0, 0, &dwChildren, 0, 0, 0, 0, 0, 0, 0) != ERROR_SUCCESS) {
            dwChildren = 0;
        }
    }

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    tvi.pszText = label;
    tvi.cchTextMax = lstrlen(tvi.pszText);
    tvi.iImage = Image_Closed;
    tvi.iSelectedImage = Image_Open;
    tvi.cChildren = dwChildren;
    tvi.lParam = (LPARAM)hKey;
    tvins.item = tvi;
    tvins.hInsertAfter = (HTREEITEM)(hKey ? TVI_LAST : TVI_FIRST);
    tvins.hParent = hParent;
    return TreeView_InsertItem(hwndTV, &tvins);
}

BOOL RefreshTreeItem(HWND hwndTV, HTREEITEM hItem)
{
    HKEY hRoot, hKey, hSubKey;
    HTREEITEM childItem;
    LPCTSTR KeyPath;
    DWORD dwCount, dwIndex, dwMaxSubKeyLen;
    LPTSTR Name = NULL;
    TVITEM tvItem;
    LPTSTR pszNodes = NULL;
    BOOL bSuccess = FALSE;
    LPTSTR s;
    BOOL bAddedAny;

    KeyPath = GetItemPath(hwndTV, hItem, &hRoot);

    if (*KeyPath) {
        if (RegOpenKeyEx(hRoot, KeyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
            goto done;
        }
    } else {
        hKey = hRoot;
    }

    if (RegQueryInfoKey(hKey, 0, 0, 0, &dwCount, &dwMaxSubKeyLen, 0, 0, 0, 0, 0, 0) != ERROR_SUCCESS) {
        goto done;
    }

    /* Set the number of children again */
    tvItem.mask = TVIF_CHILDREN;
    tvItem.hItem = hItem;
    tvItem.cChildren = dwCount;
    if (!TreeView_SetItem(hwndTV, &tvItem)) {
        goto done;
    }

    /* We don't have to bother with the rest if it's not expanded. */
    if (TreeView_GetItemState(hwndTV, hItem, TVIS_EXPANDED) == 0) {
        RegCloseKey(hKey);
        bSuccess = TRUE;
        goto done;
    }

    dwMaxSubKeyLen++; /* account for the \0 terminator */
    if (!(Name = HeapAlloc(GetProcessHeap(), 0, dwMaxSubKeyLen * sizeof(TCHAR)))) {
        goto done;
    }
    tvItem.cchTextMax = dwMaxSubKeyLen;
    /*if (!(tvItem.pszText = HeapAlloc(GetProcessHeap(), 0, dwMaxSubKeyLen * sizeof(TCHAR)))) {
        goto done;
    }*/

    /* Get all of the tree node siblings in one contiguous block of memory */
    {
        DWORD dwPhysicalSize = 0;
        DWORD dwActualSize = 0;
        DWORD dwNewPhysicalSize;
        LPTSTR pszNewNodes;
        DWORD dwStep = 10000;

        for (childItem = TreeView_GetChild(hwndTV, hItem); childItem;
                childItem = TreeView_GetNextSibling(hwndTV, childItem)) {

            if (dwActualSize + dwMaxSubKeyLen + 1 > dwPhysicalSize)
            {
                dwNewPhysicalSize = dwActualSize + dwMaxSubKeyLen + 1 + dwStep;

                if (pszNodes)
                    pszNewNodes = (LPTSTR) HeapReAlloc(GetProcessHeap(), 0, pszNodes, dwNewPhysicalSize * sizeof(TCHAR));
                else
                    pszNewNodes = (LPTSTR) HeapAlloc(GetProcessHeap(), 0, dwNewPhysicalSize * sizeof(TCHAR));
                if (!pszNewNodes)
                    goto done;

                dwPhysicalSize = dwNewPhysicalSize;
                pszNodes = pszNewNodes;
            }

            tvItem.mask = TVIF_TEXT;
            tvItem.hItem = childItem;
            tvItem.pszText = &pszNodes[dwActualSize];
            tvItem.cchTextMax = dwPhysicalSize - dwActualSize;
            if (!TreeView_GetItem(hwndTV, &tvItem))
                goto done;

            dwActualSize += (DWORD) _tcslen(&pszNodes[dwActualSize]) + 1;
        }

        if (pszNodes)
            pszNodes[dwActualSize] = '\0';
    }

    /* Now go through all the children in the tree, and check if any have to be removed. */
    childItem = TreeView_GetChild(hwndTV, hItem);
    while (childItem) {
        HTREEITEM nextItem = TreeView_GetNextSibling(hwndTV, childItem);
        if (RefreshTreeItem(hwndTV, childItem) == FALSE) {
            (void)TreeView_DeleteItem(hwndTV, childItem);
        }
        childItem = nextItem;
    }

	/* Now go through all the children in the registry, and check if any have to be added. */
    bAddedAny = FALSE;
    for (dwIndex = 0; dwIndex < dwCount; dwIndex++) {
        DWORD cName = dwMaxSubKeyLen, dwSubCount;
        BOOL found;

        found = FALSE;
        if (RegEnumKeyEx(hKey, dwIndex, Name, &cName, 0, 0, 0, NULL) != ERROR_SUCCESS) {
            continue;
        }

        /* Check if the node is already in there. */
        if (pszNodes) {
            for (s = pszNodes; *s; s += _tcslen(s) + 1) {
                if (!_tcscmp(s, Name)) {
                    found = TRUE;
                    break;
                }
            }
        }

        if (found == FALSE) {
            /* Find the number of children of the node. */
            dwSubCount = 0;
            if (RegOpenKeyEx(hKey, Name, 0, KEY_QUERY_VALUE, &hSubKey) == ERROR_SUCCESS) {
                if (RegQueryInfoKey(hSubKey, 0, 0, 0, &dwSubCount, 0, 0, 0, 0, 0, 0, 0) != ERROR_SUCCESS) {
                    dwSubCount = 0;
                }
                RegCloseKey(hSubKey);
            }

            AddEntryToTree(hwndTV, hItem, Name, NULL, dwSubCount);
            bAddedAny = TRUE;
        }
    }
    RegCloseKey(hKey);

    if (bAddedAny)
        SendMessage(hwndTV, TVM_SORTCHILDREN, 0, (LPARAM) hItem);

    bSuccess = TRUE;

done:
    if (pszNodes)
        HeapFree(GetProcessHeap(), 0, pszNodes);
    if (Name)
        HeapFree(GetProcessHeap(), 0, Name);
    return bSuccess;
}

BOOL RefreshTreeView(HWND hwndTV)
{
    HTREEITEM hItem;
    HTREEITEM hSelectedItem;
    HCURSOR hcursorOld;

    hSelectedItem = TreeView_GetSelection(hwndTV);
    hcursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    SendMessage(hwndTV, WM_SETREDRAW, FALSE, 0);

    hItem = TreeView_GetChild(hwndTV, TreeView_GetRoot(hwndTV));
    while (hItem) {
        RefreshTreeItem(hwndTV, hItem);
        hItem = TreeView_GetNextSibling(hwndTV, hItem);
    }

    SendMessage(hwndTV, WM_SETREDRAW, TRUE, 0);
    SetCursor(hcursorOld);
    
    /* We reselect the currently selected node, this will prompt a refresh of the listview. */
    (void)TreeView_SelectItem(hwndTV, hSelectedItem);
    return TRUE;
}

HTREEITEM InsertNode(HWND hwndTV, HTREEITEM hItem, LPTSTR name)
{
    TCHAR buf[MAX_NEW_KEY_LEN];
    HTREEITEM hNewItem = 0;
    TVITEMEX item;

    /* Default to the current selection */
    if (!hItem)
    {
        hItem = TreeView_GetSelection(hwndTV);
        if (!hItem)
            return FALSE;
    }

    memset(&item, 0, sizeof(item));
    item.hItem = hItem;
    item.mask = TVIF_CHILDREN | TVIF_HANDLE | TVIF_STATE;
    if (!TreeView_GetItem(hwndTV, &item))
        return FALSE;

    if ((item.state & TVIS_EXPANDEDONCE) && (item.cChildren > 0))
    {
        hNewItem = AddEntryToTree(hwndTV, hItem, name, 0, 0);
        SendMessage(hwndTV, TVM_SORTCHILDREN, 0, (LPARAM) hItem);
    }
	else
    {
        item.mask = TVIF_CHILDREN | TVIF_HANDLE;
        item.hItem = hItem;
        item.cChildren = 1;
        if (!TreeView_SetItem(hwndTV, &item))
            return FALSE;
    }

    (void)TreeView_Expand(hwndTV, hItem, TVE_EXPAND);
    if (!hNewItem)
    {
        for(hNewItem = TreeView_GetChild(hwndTV, hItem); hNewItem; hNewItem = TreeView_GetNextSibling(hwndTV, hNewItem))
        {
            item.mask = TVIF_HANDLE | TVIF_TEXT;
            item.hItem = hNewItem;
            item.pszText = buf;
            item.cchTextMax = COUNT_OF(buf);
            if (!TreeView_GetItem(hwndTV, &item)) continue;
            if (lstrcmp(name, item.pszText) == 0) break;
        }	
    }
    if (hNewItem) (void)TreeView_SelectItem(hwndTV, hNewItem);

    return hNewItem;
}

HWND StartKeyRename(HWND hwndTV)
{
    HTREEITEM hItem;

    if(!(hItem = TreeView_GetSelection(hwndTV))) return 0;
    return TreeView_EditLabel(hwndTV, hItem);
}

static BOOL InitTreeViewItems(HWND hwndTV, LPTSTR pHostName)
{
    TVITEM tvi;
    TVINSERTSTRUCT tvins;
    HTREEITEM hRoot;

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    /* Set the text of the item.  */
    tvi.pszText = pHostName;
    tvi.cchTextMax = lstrlen(tvi.pszText);
    /* Assume the item is not a parent item, so give it an image.  */
    tvi.iImage = Image_Root;
    tvi.iSelectedImage = Image_Root;
    tvi.cChildren = 5;
    /* Save the heading level in the item's application-defined data area.  */
    tvi.lParam = (LPARAM)NULL;
    tvins.item = tvi;
    tvins.hInsertAfter = (HTREEITEM)TVI_FIRST;
    tvins.hParent = TVI_ROOT;
    /* Add the item to the tree view control.  */
    if (!(hRoot = TreeView_InsertItem(hwndTV, &tvins))) return FALSE;

    if (!AddEntryToTree(hwndTV, hRoot, _T("HKEY_CLASSES_ROOT"), HKEY_CLASSES_ROOT, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, _T("HKEY_CURRENT_USER"), HKEY_CURRENT_USER, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, _T("HKEY_LOCAL_MACHINE"), HKEY_LOCAL_MACHINE, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, _T("HKEY_USERS"), HKEY_USERS, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, _T("HKEY_CURRENT_CONFIG"), HKEY_CURRENT_CONFIG, 1)) return FALSE;

	if (GetVersion() & 0x80000000)
    {
        /* Win9x specific key */
        if (!AddEntryToTree(hwndTV, hRoot, _T("HKEY_DYN_DATA"), HKEY_DYN_DATA, 1)) return FALSE;
	}
    
    /* expand and select host name */
    (void)TreeView_Expand(hwndTV, hRoot, TVE_EXPAND);
    (void)TreeView_Select(hwndTV, hRoot, TVGN_CARET);
    return TRUE;
}


/*
 * InitTreeViewImageLists - creates an image list, adds three bitmaps
 * to it, and associates the image list with a tree view control.
 * Returns TRUE if successful, or FALSE otherwise.
 * hwndTV - handle to the tree view control.
 */
static BOOL InitTreeViewImageLists(HWND hwndTV)
{
    HIMAGELIST himl;  /* handle to image list  */
    HICON hico;       /* handle to icon  */

    /* Create the image list.  */
    if ((himl = ImageList_Create(CX_ICON, CY_ICON,
                                 ILC_MASK|ILC_COLOR32, 0, NUM_ICONS)) == NULL)
        return FALSE;

    /* Add the open file, closed file, and document bitmaps.  */
    hico = LoadIcon(hInst, MAKEINTRESOURCE(IDI_OPEN_FILE));
    Image_Open = ImageList_AddIcon(himl, hico);

    hico = LoadIcon(hInst, MAKEINTRESOURCE(IDI_CLOSED_FILE));
    Image_Closed = ImageList_AddIcon(himl, hico);

    hico = LoadIcon(hInst, MAKEINTRESOURCE(IDI_ROOT));
    Image_Root = ImageList_AddIcon(himl, hico);

    /* Fail if not all of the images were added.  */
    if (ImageList_GetImageCount(himl) < NUM_ICONS)
    {
      return FALSE;
    }

    /* Associate the image list with the tree view control.  */
    (void)TreeView_SetImageList(hwndTV, himl, TVSIL_NORMAL);

    return TRUE;
}

BOOL OnTreeExpanding(HWND hwndTV, NMTREEVIEW* pnmtv)
{
    DWORD dwCount, dwIndex, dwMaxSubKeyLen;
    HKEY hRoot, hNewKey, hKey;
    LPCTSTR keyPath;
    LPTSTR Name;
    LONG errCode;
    HCURSOR hcursorOld;

    static int expanding;
    if (expanding) return FALSE;
    if (pnmtv->itemNew.state & TVIS_EXPANDEDONCE ) {
        return TRUE;
    }
    expanding = TRUE;
    hcursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    SendMessage(hwndTV, WM_SETREDRAW, FALSE, 0);

    keyPath = GetItemPath(hwndTV, pnmtv->itemNew.hItem, &hRoot);
    if (!keyPath) goto done;

    if (*keyPath) {
        errCode = RegOpenKeyEx(hRoot, keyPath, 0, KEY_READ, &hNewKey);
        if (errCode != ERROR_SUCCESS) goto done;
    } else {
	hNewKey = hRoot;
    }

    errCode = RegQueryInfoKey(hNewKey, 0, 0, 0, &dwCount, &dwMaxSubKeyLen, 0, 0, 0, 0, 0, 0);
    if (errCode != ERROR_SUCCESS) goto done;
    dwMaxSubKeyLen++; /* account for the \0 terminator */
    Name = HeapAlloc(GetProcessHeap(), 0, dwMaxSubKeyLen * sizeof(TCHAR));
    if (!Name) goto done;

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++) {
        DWORD cName = dwMaxSubKeyLen, dwSubCount;

        errCode = RegEnumKeyEx(hNewKey, dwIndex, Name, &cName, 0, 0, 0, 0);
        if (errCode != ERROR_SUCCESS) continue;
        errCode = RegOpenKeyEx(hNewKey, Name, 0, KEY_QUERY_VALUE, &hKey);
        if (errCode == ERROR_SUCCESS) {
            errCode = RegQueryInfoKey(hKey, 0, 0, 0, &dwSubCount, 0, 0, 0, 0, 0, 0, 0);
            RegCloseKey(hKey);
        }
        if (errCode != ERROR_SUCCESS) dwSubCount = 0;
        AddEntryToTree(hwndTV, pnmtv->itemNew.hItem, Name, NULL, dwSubCount);
    }

    SendMessage(hwndTV, TVM_SORTCHILDREN, 0, (LPARAM)pnmtv->itemNew.hItem);

    RegCloseKey(hNewKey);
    HeapFree(GetProcessHeap(), 0, Name);

done:
    SendMessage(hwndTV, WM_SETREDRAW, TRUE, 0);
    SetCursor(hcursorOld);
    expanding = FALSE;

    return TRUE;
}


BOOL CreateNewKey(HWND hwndTV, HTREEITEM hItem)
{
    TCHAR szNewKeyFormat[128];
    TCHAR szNewKey[128];
    LPCTSTR pszKeyPath;
    int iIndex = 1;
    HKEY hRootKey;
    HKEY hKey = NULL;
    HKEY hNewKey = NULL;
    BOOL bSuccess = FALSE;
    LONG lResult;
    DWORD dwDisposition;
    HTREEITEM hNewItem;

    pszKeyPath = GetItemPath(g_pChildWnd->hTreeWnd, hItem, &hRootKey);
    if (RegOpenKey(hRootKey, pszKeyPath, &hKey) != ERROR_SUCCESS)
        goto done;

    if (LoadString(hInst, IDS_NEW_KEY, szNewKeyFormat, sizeof(szNewKeyFormat) / sizeof(szNewKeyFormat[0])) <= 0)
        goto done;

    /* Need to create a new key with a unique name */
    do
    {
        _sntprintf(szNewKey, sizeof(szNewKey) / sizeof(szNewKey[0]), szNewKeyFormat, iIndex++);
        lResult = RegCreateKeyEx(hKey, szNewKey, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hNewKey, &dwDisposition);
        if (hNewKey && (dwDisposition == REG_OPENED_EXISTING_KEY))
        {
            RegCloseKey(hNewKey);
            hNewKey = NULL;
        }
    }
    while(!hNewKey);

    /* Insert the new key */
    hNewItem = InsertNode(hwndTV, hItem, szNewKey);
    if (!hNewItem)
        goto done;

    /* The new key's name is probably not appropriate yet */
    (void)TreeView_EditLabel(hwndTV, hNewItem);

    bSuccess = TRUE;

done:
    if (hKey)
        RegCloseKey(hKey);
    if (hNewKey)
        RegCloseKey(hNewKey);
    return bSuccess;
}


/*
 * CreateTreeView - creates a tree view control.
 * Returns the handle to the new control if successful, or NULL otherwise.
 * hwndParent - handle to the control's parent window.
 */
HWND CreateTreeView(HWND hwndParent, LPTSTR pHostName, HMENU id)
{
    RECT rcClient;
    HWND hwndTV;

    /* Get the dimensions of the parent window's client area, and create the tree view control.  */
    GetClientRect(hwndParent, &rcClient);
    hwndTV = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL,
                            WS_VISIBLE | WS_CHILD | WS_TABSTOP | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_EDITLABELS,
                            0, 0, rcClient.right, rcClient.bottom,
                            hwndParent, id, hInst, NULL);
    /* Initialize the image list, and add items to the control.  */
    if (!InitTreeViewImageLists(hwndTV) || !InitTreeViewItems(hwndTV, pHostName)) {
        DestroyWindow(hwndTV);
        return NULL;
    }
    return hwndTV;
}

void DestroyTreeView() {
	if (pathBuffer)
		HeapFree(GetProcessHeap(), 0, pathBuffer);
}

BOOL SelectNode(HWND hwndTV, LPCTSTR keyPath)
{
	HTREEITEM hRoot, hItem;
	HTREEITEM hChildItem;
	TCHAR szPathPart[128];
	TCHAR szBuffer[128];
	LPCTSTR s;
	TVITEM tvi;

    /* Total no-good hack */
    if (!_tcsncmp(keyPath, _T("My Computer\\"), 12))
        keyPath += 12;

	hRoot = TreeView_GetRoot(hwndTV);
	hItem = hRoot;

	while(keyPath[0])
	{
		s = _tcschr(keyPath, TEXT('\\'));
		lstrcpyn(szPathPart, keyPath, s ? s - keyPath + 1 : _tcslen(keyPath) + 1);

		/* Special case for root to expand root key abbreviations */
		if (hItem == hRoot)
		{
			if (!_tcscmp(szPathPart, TEXT("HKCR")))
				_tcscpy(szPathPart, TEXT("HKEY_CLASSES_ROOT"));
			else if (!_tcscmp(szPathPart, TEXT("HKCU")))
				_tcscpy(szPathPart, TEXT("HKEY_CURRENT_USER"));
			else if (!_tcscmp(szPathPart, TEXT("HKLM")))
				_tcscpy(szPathPart, TEXT("HKEY_LOCAL_MACHINE"));
			else if (!_tcscmp(szPathPart, TEXT("HKU")))
				_tcscpy(szPathPart, TEXT("HKEY_USERS"));
			else if (!_tcscmp(szPathPart, TEXT("HKCC")))
				_tcscpy(szPathPart, TEXT("HKEY_CURRENT_CONFIG"));
			else if (!_tcscmp(szPathPart, TEXT("HKDD")))
				_tcscpy(szPathPart, TEXT("HKEY_DYN_DATA"));
		}

		for (hChildItem = TreeView_GetChild(hwndTV, hItem); hChildItem;
			hChildItem = TreeView_GetNextSibling(hwndTV, hChildItem))
		{
			memset(&tvi, 0, sizeof(tvi));
			tvi.hItem = hChildItem;
			tvi.mask = TVIF_TEXT | TVIF_CHILDREN;
			tvi.pszText = szBuffer;
			tvi.cchTextMax = sizeof(szBuffer) / sizeof(szBuffer[0]);

			(void)TreeView_GetItem(hwndTV, &tvi);

			if (!_tcscmp(szBuffer, szPathPart))
				break;
		}

		if (!hChildItem)
			return FALSE;

		if (tvi.cChildren > 0)
		{
			if (!TreeView_Expand(hwndTV, hChildItem, TVE_EXPAND))
				return FALSE;
		}

		keyPath = s ? s + 1 : _T("");
		hItem = hChildItem;
	}

	(void)TreeView_SelectItem(hwndTV, hItem);
	(void)TreeView_EnsureVisible(hwndTV, hItem);

	return TRUE;
}


