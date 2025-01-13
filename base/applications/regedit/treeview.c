/*
 * Regedit treeview
 *
 * Copyright (C) 2002 Robert Dickenson <robd@reactos.org>
 * LICENSE: LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 */

#include "regedit.h"

/* Global variables and constants */
/* Image_Open, Image_Closed, and Image_Root - integer variables for indexes of the images */
static int Image_Open;
static int Image_Closed;
static int Image_Root;

static LPWSTR pathBuffer;

#define NUM_ICONS   3 /* number of icons to add to the image list */

/* External resources in shell32.dll */
#define IDI_SHELL_FOLDER             4
#define IDI_SHELL_FOLDER_OPEN        5
#define IDI_SHELL_MY_COMPUTER       16

static BOOL get_item_path(HWND hwndTV, HTREEITEM hItem, HKEY* phKey, LPWSTR* pKeyPath, int* pPathLen, int* pMaxLen)
{
    TVITEMW item;
    size_t maxLen, len;
    LPWSTR newStr;

    item.mask = TVIF_PARAM;
    item.hItem = hItem;
    if (!TreeView_GetItem(hwndTV, &item)) return FALSE;

    if (item.lParam)
    {
        /* found root key with valid key value */
        *phKey = (HKEY)item.lParam;
        return TRUE;
    }

    if(!get_item_path(hwndTV, TreeView_GetParent(hwndTV, hItem), phKey, pKeyPath, pPathLen, pMaxLen)) return FALSE;
    if (*pPathLen)
    {
        (*pKeyPath)[*pPathLen] = L'\\';
        ++(*pPathLen);
    }

    do
    {
        item.mask = TVIF_TEXT;
        item.hItem = hItem;
        item.pszText = *pKeyPath + *pPathLen;
        maxLen = *pMaxLen - *pPathLen;
        item.cchTextMax = (int) maxLen;
        if (!TreeView_GetItem(hwndTV, &item)) return FALSE;
        len = wcslen(item.pszText);
        if (len < maxLen - 1)
        {
            *pPathLen += (int) len;
            break;
        }
        newStr = HeapReAlloc(GetProcessHeap(), 0, *pKeyPath, *pMaxLen * 2);
        if (!newStr) return FALSE;
        *pKeyPath = newStr;
        *pMaxLen *= 2;
    }
    while(TRUE);

    return TRUE;
}

LPCWSTR GetItemPath(HWND hwndTV, HTREEITEM hItem, HKEY* phRootKey)
{
    int pathLen = 0, maxLen;

    *phRootKey = NULL;

    if (!pathBuffer)
        pathBuffer = HeapAlloc(GetProcessHeap(), 0, 1024);
    if (!pathBuffer)
        return NULL;

    *pathBuffer = UNICODE_NULL;

    maxLen = (int)HeapSize(GetProcessHeap(), 0, pathBuffer);

    if (!hItem)
    {
        hItem = TreeView_GetSelection(hwndTV);
    }
    if (maxLen == -1 || !hItem || !get_item_path(hwndTV, hItem, phRootKey, &pathBuffer, &pathLen, &maxLen))
    {
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
static HTREEITEM AddEntryToTree(HWND hwndTV, HTREEITEM hParent, LPWSTR label, HKEY hKey, DWORD dwChildren)
{
    TVITEMW tvi;
    TVINSERTSTRUCTW tvins;

    if (hKey)
    {
        if (RegQueryInfoKeyW(hKey, 0, 0, 0, &dwChildren, 0, 0, 0, 0, 0, 0, 0) != ERROR_SUCCESS)
        {
            dwChildren = 0;
        }
    }

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    tvi.pszText = label;
    tvi.cchTextMax = wcslen(tvi.pszText);
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
    LPCWSTR KeyPath;
    DWORD dwCount, dwIndex, dwMaxSubKeyLen;
    LPWSTR Name = NULL;
    TVITEMW tvItem;
    LPWSTR pszNodes = NULL;
    BOOL bSuccess = FALSE;
    LPWSTR s;
    BOOL bAddedAny;

    KeyPath = GetItemPath(hwndTV, hItem, &hRoot);

    if (*KeyPath)
    {
        if (RegOpenKeyExW(hRoot, KeyPath, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        {
            goto done;
        }
    }
    else
    {
        hKey = hRoot;
    }

    if (RegQueryInfoKeyW(hKey, 0, 0, 0, &dwCount, &dwMaxSubKeyLen, 0, 0, 0, 0, 0, 0) != ERROR_SUCCESS)
    {
        goto done;
    }

    /* Set the number of children again */
    tvItem.mask = TVIF_CHILDREN;
    tvItem.hItem = hItem;
    tvItem.cChildren = dwCount;
    if (!TreeView_SetItem(hwndTV, &tvItem))
    {
        goto done;
    }

    /* We don't have to bother with the rest if it's not expanded. */
    if (TreeView_GetItemState(hwndTV, hItem, TVIS_EXPANDED) == 0)
    {
        RegCloseKey(hKey);
        bSuccess = TRUE;
        goto done;
    }

    dwMaxSubKeyLen++; /* account for the \0 terminator */
    if (!(Name = HeapAlloc(GetProcessHeap(), 0, dwMaxSubKeyLen * sizeof(WCHAR))))
    {
        goto done;
    }
    tvItem.cchTextMax = dwMaxSubKeyLen;
    /*if (!(tvItem.pszText = HeapAlloc(GetProcessHeap(), 0, dwMaxSubKeyLen * sizeof(WCHAR)))) {
        goto done;
    }*/

    /* Get all of the tree node siblings in one contiguous block of memory */
    {
        DWORD dwPhysicalSize = 0;
        DWORD dwActualSize = 0;
        DWORD dwNewPhysicalSize;
        LPWSTR pszNewNodes;
        DWORD dwStep = 10000;

        for (childItem = TreeView_GetChild(hwndTV, hItem); childItem;
                childItem = TreeView_GetNextSibling(hwndTV, childItem))
        {

            if (dwActualSize + dwMaxSubKeyLen + 1 > dwPhysicalSize)
            {
                dwNewPhysicalSize = dwActualSize + dwMaxSubKeyLen + 1 + dwStep;

                if (pszNodes)
                    pszNewNodes = (LPWSTR) HeapReAlloc(GetProcessHeap(), 0, pszNodes, dwNewPhysicalSize * sizeof(WCHAR));
                else
                    pszNewNodes = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, dwNewPhysicalSize * sizeof(WCHAR));
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

            dwActualSize += (DWORD) wcslen(&pszNodes[dwActualSize]) + 1;
        }

        if (pszNodes)
            pszNodes[dwActualSize] = L'\0';
    }

    /* Now go through all the children in the tree, and check if any have to be removed. */
    childItem = TreeView_GetChild(hwndTV, hItem);
    while (childItem)
    {
        HTREEITEM nextItem = TreeView_GetNextSibling(hwndTV, childItem);
        if (RefreshTreeItem(hwndTV, childItem) == FALSE)
        {
            (void)TreeView_DeleteItem(hwndTV, childItem);
        }
        childItem = nextItem;
    }

    /* Now go through all the children in the registry, and check if any have to be added. */
    bAddedAny = FALSE;
    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        DWORD cName = dwMaxSubKeyLen, dwSubCount;
        BOOL found;

        found = FALSE;
        if (RegEnumKeyExW(hKey, dwIndex, Name, &cName, 0, 0, 0, NULL) != ERROR_SUCCESS)
        {
            continue;
        }

        /* Check if the node is already in there. */
        if (pszNodes)
        {
            for (s = pszNodes; *s; s += wcslen(s) + 1)
            {
                if (!wcscmp(s, Name))
                {
                    found = TRUE;
                    break;
                }
            }
        }

        if (found == FALSE)
        {
            /* Find the number of children of the node. */
            dwSubCount = 0;
            if (RegOpenKeyExW(hKey, Name, 0, KEY_QUERY_VALUE, &hSubKey) == ERROR_SUCCESS)
            {
                if (RegQueryInfoKeyW(hSubKey, 0, 0, 0, &dwSubCount, 0, 0, 0, 0, 0, 0, 0) != ERROR_SUCCESS)
                {
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
        SendMessageW(hwndTV, TVM_SORTCHILDREN, 0, (LPARAM) hItem);

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
    SendMessageW(hwndTV, WM_SETREDRAW, FALSE, 0);

    hItem = TreeView_GetChild(hwndTV, TreeView_GetRoot(hwndTV));
    while (hItem)
    {
        RefreshTreeItem(hwndTV, hItem);
        hItem = TreeView_GetNextSibling(hwndTV, hItem);
    }

    SendMessageW(hwndTV, WM_SETREDRAW, TRUE, 0);
    SetCursor(hcursorOld);

    /* We reselect the currently selected node, this will prompt a refresh of the listview. */
    (void)TreeView_SelectItem(hwndTV, hSelectedItem);
    return TRUE;
}

HTREEITEM InsertNode(HWND hwndTV, HTREEITEM hItem, LPWSTR name)
{
    WCHAR buf[MAX_NEW_KEY_LEN];
    HTREEITEM hNewItem = 0;
    TVITEMEXW item;

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

    if (item.state & TVIS_EXPANDEDONCE)
    {
        hNewItem = AddEntryToTree(hwndTV, hItem, name, 0, 0);
        SendMessageW(hwndTV, TVM_SORTCHILDREN, 0, (LPARAM) hItem);
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
            item.cchTextMax = ARRAY_SIZE(buf);
            if (!TreeView_GetItem(hwndTV, &item)) continue;
            if (wcscmp(name, item.pszText) == 0) break;
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

static BOOL InitTreeViewItems(HWND hwndTV, LPWSTR pHostName)
{
    TVITEMW tvi;
    TVINSERTSTRUCTW tvins;
    HTREEITEM hRoot;

    tvi.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM;
    /* Set the text of the item.  */
    tvi.pszText = pHostName;
    tvi.cchTextMax = wcslen(tvi.pszText);
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

    if (!AddEntryToTree(hwndTV, hRoot, L"HKEY_CLASSES_ROOT", HKEY_CLASSES_ROOT, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, L"HKEY_CURRENT_USER", HKEY_CURRENT_USER, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, L"HKEY_LOCAL_MACHINE", HKEY_LOCAL_MACHINE, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, L"HKEY_USERS", HKEY_USERS, 1)) return FALSE;
    if (!AddEntryToTree(hwndTV, hRoot, L"HKEY_CURRENT_CONFIG", HKEY_CURRENT_CONFIG, 1)) return FALSE;

    if (GetVersion() & 0x80000000)
    {
        /* Win9x specific key */
        if (!AddEntryToTree(hwndTV, hRoot, L"HKEY_DYN_DATA", HKEY_DYN_DATA, 1)) return FALSE;
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
    INT cx = GetSystemMetrics(SM_CXSMICON);
    INT cy = GetSystemMetrics(SM_CYSMICON);
    HMODULE hShell32 = GetModuleHandleW(L"shell32.dll");

    /* Create the image list.  */
    if ((himl = ImageList_Create(cx, cy, ILC_MASK | ILC_COLOR32, 0, NUM_ICONS)) == NULL)
        return FALSE;

    /* Add the open file, closed file, and document bitmaps.  */
    hico = LoadImageW(hShell32,
                      MAKEINTRESOURCEW(IDI_SHELL_FOLDER_OPEN),
                      IMAGE_ICON,
                      cx,
                      cy,
                      LR_DEFAULTCOLOR);
    if (hico)
    {
        Image_Open = ImageList_AddIcon(himl, hico);
        DestroyIcon(hico);
    }

    hico = LoadImageW(hShell32,
                      MAKEINTRESOURCEW(IDI_SHELL_FOLDER),
                      IMAGE_ICON,
                      cx,
                      cy,
                      LR_DEFAULTCOLOR);
    if (hico)
    {
        Image_Closed = ImageList_AddIcon(himl, hico);
        DestroyIcon(hico);
    }

    hico = LoadImageW(hShell32,
                      MAKEINTRESOURCEW(IDI_SHELL_MY_COMPUTER),
                      IMAGE_ICON,
                      cx,
                      cy,
                      LR_DEFAULTCOLOR);
    if (hico)
    {
        Image_Root = ImageList_AddIcon(himl, hico);
        DestroyIcon(hico);
    }

    /* Fail if not all of the images were added.  */
    if (ImageList_GetImageCount(himl) < NUM_ICONS)
    {
        ImageList_Destroy(himl);
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
    LPCWSTR keyPath;
    LPWSTR Name;
    LONG errCode;
    HCURSOR hcursorOld;

    static int expanding;
    if (expanding) return FALSE;
    if (pnmtv->itemNew.state & TVIS_EXPANDEDONCE )
    {
        return TRUE;
    }
    expanding = TRUE;
    hcursorOld = SetCursor(LoadCursor(NULL, IDC_WAIT));
    SendMessageW(hwndTV, WM_SETREDRAW, FALSE, 0);

    keyPath = GetItemPath(hwndTV, pnmtv->itemNew.hItem, &hRoot);
    if (!keyPath) goto done;

    if (*keyPath)
    {
        errCode = RegOpenKeyExW(hRoot, keyPath, 0, KEY_READ, &hNewKey);
        if (errCode != ERROR_SUCCESS) goto done;
    }
    else
    {
        hNewKey = hRoot;
    }

    errCode = RegQueryInfoKeyW(hNewKey, 0, 0, 0, &dwCount, &dwMaxSubKeyLen, 0, 0, 0, 0, 0, 0);
    if (errCode != ERROR_SUCCESS) goto done;
    dwMaxSubKeyLen++; /* account for the \0 terminator */
    Name = HeapAlloc(GetProcessHeap(), 0, dwMaxSubKeyLen * sizeof(WCHAR));
    if (!Name) goto done;

    for (dwIndex = 0; dwIndex < dwCount; dwIndex++)
    {
        DWORD cName = dwMaxSubKeyLen, dwSubCount;

        errCode = RegEnumKeyExW(hNewKey, dwIndex, Name, &cName, 0, 0, 0, 0);
        if (errCode != ERROR_SUCCESS) continue;
        errCode = RegOpenKeyExW(hNewKey, Name, 0, KEY_QUERY_VALUE, &hKey);
        if (errCode == ERROR_SUCCESS)
        {
            errCode = RegQueryInfoKeyW(hKey, 0, 0, 0, &dwSubCount, 0, 0, 0, 0, 0, 0, 0);
            RegCloseKey(hKey);
        }
        if (errCode != ERROR_SUCCESS) dwSubCount = 0;
        AddEntryToTree(hwndTV, pnmtv->itemNew.hItem, Name, NULL, dwSubCount);
    }

    SendMessageW(hwndTV, TVM_SORTCHILDREN, 0, (LPARAM)pnmtv->itemNew.hItem);

    RegCloseKey(hNewKey);
    HeapFree(GetProcessHeap(), 0, Name);

done:
    SendMessageW(hwndTV, WM_SETREDRAW, TRUE, 0);
    SetCursor(hcursorOld);
    expanding = FALSE;

    return TRUE;
}

BOOL CreateNewKey(HWND hwndTV, HTREEITEM hItem)
{
    WCHAR szNewKeyFormat[128];
    WCHAR szNewKey[128];
    LPCWSTR pszKeyPath;
    int iIndex = 1;
    LONG nResult;
    HKEY hRootKey = NULL, hKey = NULL, hNewKey = NULL;
    BOOL bSuccess = FALSE;
    DWORD dwDisposition;
    HTREEITEM hNewItem;

    pszKeyPath = GetItemPath(hwndTV, hItem, &hRootKey);
    if (!pszKeyPath)
        return bSuccess;
    if (pszKeyPath[0] == L'\0')
        hKey = hRootKey;
    else if (RegOpenKeyW(hRootKey, pszKeyPath, &hKey) != ERROR_SUCCESS)
        goto done;

    if (LoadStringW(hInst, IDS_NEW_KEY, szNewKeyFormat, ARRAY_SIZE(szNewKeyFormat)) <= 0)
        goto done;

    /* Need to create a new key with a unique name */
    do
    {
        wsprintf(szNewKey, szNewKeyFormat, iIndex++);
        nResult = RegCreateKeyExW(hKey, szNewKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hNewKey, &dwDisposition);
        if (hNewKey && dwDisposition == REG_OPENED_EXISTING_KEY)
        {
            RegCloseKey(hNewKey);
            hNewKey = NULL;
        }
        else if (!hNewKey)
        {
            InfoMessageBox(hFrameWnd, MB_OK | MB_ICONERROR, NULL, L"Cannot create new key!\n\nError Code: %d", nResult);
            goto done;
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
    if (hKey != hRootKey && hKey)
        RegCloseKey(hKey);
    if (hNewKey)
        RegCloseKey(hNewKey);
    return bSuccess;
}

BOOL TreeWndNotifyProc(HWND hWnd, WPARAM wParam, LPARAM lParam, BOOL *Result)
{
    UNREFERENCED_PARAMETER(wParam);
    *Result = TRUE;

    switch (((LPNMHDR)lParam)->code)
    {
        case TVN_ITEMEXPANDING:
            *Result = !OnTreeExpanding(g_pChildWnd->hTreeWnd, (NMTREEVIEW*)lParam);
            return TRUE;
        case TVN_SELCHANGED:
        {
            NMTREEVIEW* pnmtv = (NMTREEVIEW*)lParam;
            /* Get the parent of the current item */
            HTREEITEM hParentItem = TreeView_GetParent(g_pChildWnd->hTreeWnd, pnmtv->itemNew.hItem);

            UpdateAddress(pnmtv->itemNew.hItem, NULL, NULL, TRUE);

            /* Disable the Permissions and new key menu items for 'My Computer' */
            EnableMenuItem(hMenuFrame, ID_EDIT_PERMISSIONS, MF_BYCOMMAND | (hParentItem ? MF_ENABLED : MF_GRAYED));
            EnableMenuItem(hMenuFrame, ID_EDIT_NEW_KEY, MF_BYCOMMAND | (hParentItem ? MF_ENABLED : MF_GRAYED));

            /*
             * Disable Delete/Rename menu options for 'My Computer' (first item so doesn't have any parent)
             * and HKEY_* keys (their parent is 'My Computer' and the previous remark applies).
             */
            if (!hParentItem || !TreeView_GetParent(g_pChildWnd->hTreeWnd, hParentItem))
            {
                EnableMenuItem(hMenuFrame , ID_EDIT_DELETE, MF_BYCOMMAND | MF_GRAYED);
                EnableMenuItem(hMenuFrame , ID_EDIT_RENAME, MF_BYCOMMAND | MF_GRAYED);
                EnableMenuItem(hPopupMenus, ID_TREE_DELETE, MF_BYCOMMAND | MF_GRAYED);
                EnableMenuItem(hPopupMenus, ID_TREE_RENAME, MF_BYCOMMAND | MF_GRAYED);
            }
            else
            {
                EnableMenuItem(hMenuFrame , ID_EDIT_DELETE, MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(hMenuFrame , ID_EDIT_RENAME, MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(hPopupMenus, ID_TREE_DELETE, MF_BYCOMMAND | MF_ENABLED);
                EnableMenuItem(hPopupMenus, ID_TREE_RENAME, MF_BYCOMMAND | MF_ENABLED);
            }

            return TRUE;
        }
        case NM_SETFOCUS:
            g_pChildWnd->nFocusPanel = 0;
            break;
        case TVN_BEGINLABELEDIT:
        {
            LPNMTVDISPINFO ptvdi = (LPNMTVDISPINFO)lParam;
            HWND hWndFocus = GetFocus();

            /* cancel label edit for rootkeys */
            if (!TreeView_GetParent(g_pChildWnd->hTreeWnd, ptvdi->item.hItem) ||
                !TreeView_GetParent(g_pChildWnd->hTreeWnd, TreeView_GetParent(g_pChildWnd->hTreeWnd, ptvdi->item.hItem)))
            {
                *Result = TRUE;
            }
            else
            {
                *Result = FALSE;
            }

            /* cancel label edit if VK_DELETE accelerator opened a MessageBox */
            if (hWndFocus != g_pChildWnd->hTreeWnd && hWndFocus != TreeView_GetEditControl(g_pChildWnd->hTreeWnd))
            {
                *Result = TRUE;
            }
            return TRUE;
        }
        case TVN_ENDLABELEDIT:
        {
            LPCWSTR keyPath;
            HKEY hRootKey;
            HKEY hKey = NULL;
            LPNMTVDISPINFO ptvdi = (LPNMTVDISPINFO)lParam;
            LONG nRenResult;
            LONG lResult = TRUE;
            WCHAR szBuffer[MAX_PATH];
            WCHAR Caption[128];

            if (ptvdi->item.pszText)
            {
                keyPath = GetItemPath(g_pChildWnd->hTreeWnd, TreeView_GetParent(g_pChildWnd->hTreeWnd, ptvdi->item.hItem), &hRootKey);
                if (wcslen(keyPath))
                    _snwprintf(szBuffer, ARRAY_SIZE(szBuffer), L"%s\\%s", keyPath, ptvdi->item.pszText);
                else
                    _snwprintf(szBuffer, ARRAY_SIZE(szBuffer), L"%s", ptvdi->item.pszText);
                keyPath = GetItemPath(g_pChildWnd->hTreeWnd, ptvdi->item.hItem, &hRootKey);
                if (RegOpenKeyExW(hRootKey, szBuffer, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
                {
                    lResult = FALSE;
                    RegCloseKey(hKey);
                    TreeView_EditLabel(g_pChildWnd->hTreeWnd, ptvdi->item.hItem);
                }
                else
                {
                    nRenResult = RenameKey(hRootKey, keyPath, ptvdi->item.pszText);
                    if (nRenResult != ERROR_SUCCESS)
                    {
                        LoadStringW(hInst, IDS_ERROR, Caption, ARRAY_SIZE(Caption));
                        ErrorMessageBox(hWnd, Caption, nRenResult);
                        lResult = FALSE;
                    }
                    else
                        UpdateAddress(ptvdi->item.hItem, hRootKey, szBuffer, FALSE);
                }
                *Result = lResult;
            }
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * CreateTreeView - creates a tree view control.
 * Returns the handle to the new control if successful, or NULL otherwise.
 * hwndParent - handle to the control's parent window.
 */
HWND CreateTreeView(HWND hwndParent, LPWSTR pHostName, HMENU id)
{
    RECT rcClient;
    HWND hwndTV;

    /* Get the dimensions of the parent window's client area, and create the tree view control.  */
    GetClientRect(hwndParent, &rcClient);
    hwndTV = CreateWindowExW(WS_EX_CLIENTEDGE, WC_TREEVIEW, NULL,
                            WS_VISIBLE | WS_CHILD | WS_TABSTOP | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT | TVS_EDITLABELS | TVS_SHOWSELALWAYS,
                            0, 0, rcClient.right, rcClient.bottom,
                            hwndParent, id, hInst, NULL);
    if (!hwndTV) return NULL;

    /* Initialize the image list, and add items to the control.  */
    if (!InitTreeViewImageLists(hwndTV) || !InitTreeViewItems(hwndTV, pHostName))
    {
        DestroyWindow(hwndTV);
        return NULL;
    }
    return hwndTV;
}

void DestroyTreeView(HWND hwndTV)
{
    HIMAGELIST himl;

    if (pathBuffer) HeapFree(GetProcessHeap(), 0, pathBuffer);

    /* Destroy the image list associated with the tree view control */
    himl = TreeView_GetImageList(hwndTV, TVSIL_NORMAL);
    if (himl) ImageList_Destroy(himl);
}

BOOL SelectNode(HWND hwndTV, LPCWSTR keyPath)
{
    HTREEITEM hRoot, hItem;
    HTREEITEM hChildItem;
    WCHAR szPathPart[128];
    WCHAR szBuffer[128];
    LPCWSTR s;
    TVITEMW tvi;

    /* Load "My Computer" string... */
    LoadStringW(hInst, IDS_MY_COMPUTER, szBuffer, ARRAY_SIZE(szBuffer));
    StringCbCatW(szBuffer, sizeof(szBuffer), L"\\");

    /* ... and remove it from the key path */
    if (!_wcsnicmp(keyPath, szBuffer, wcslen(szBuffer)))
        keyPath += wcslen(szBuffer);

    /* Reinitialize szBuffer */
    szBuffer[0] = L'\0';

    hRoot = TreeView_GetRoot(hwndTV);
    hItem = hRoot;

    while(keyPath[0])
    {
        size_t copyLength;
        s = wcschr(keyPath, L'\\');
        if (s != NULL)
        {
            copyLength = (s - keyPath) * sizeof(WCHAR);
        }
        else
        {
            copyLength = sizeof(szPathPart);
        }
        StringCbCopyNW(szPathPart, sizeof(szPathPart), keyPath, copyLength);

        /* Special case for root to expand root key abbreviations */
        if (hItem == hRoot)
        {
            if (!_wcsicmp(szPathPart, L"HKCR"))
                StringCbCopyW(szPathPart, sizeof(szPathPart), L"HKEY_CLASSES_ROOT");
            else if (!_wcsicmp(szPathPart, L"HKCU"))
                StringCbCopyW(szPathPart, sizeof(szPathPart), L"HKEY_CURRENT_USER");
            else if (!_wcsicmp(szPathPart, L"HKLM"))
                StringCbCopyW(szPathPart, sizeof(szPathPart), L"HKEY_LOCAL_MACHINE");
            else if (!_wcsicmp(szPathPart, L"HKU"))
                StringCbCopyW(szPathPart, sizeof(szPathPart), L"HKEY_USERS");
            else if (!_wcsicmp(szPathPart, L"HKCC"))
                StringCbCopyW(szPathPart, sizeof(szPathPart), L"HKEY_CURRENT_CONFIG");
            else if (!_wcsicmp(szPathPart, L"HKDD"))
                StringCbCopyW(szPathPart, sizeof(szPathPart), L"HKEY_DYN_DATA");
        }

        for (hChildItem = TreeView_GetChild(hwndTV, hItem); hChildItem;
                hChildItem = TreeView_GetNextSibling(hwndTV, hChildItem))
        {
            memset(&tvi, 0, sizeof(tvi));
            tvi.hItem = hChildItem;
            tvi.mask = TVIF_TEXT | TVIF_CHILDREN;
            tvi.pszText = szBuffer;
            tvi.cchTextMax = ARRAY_SIZE(szBuffer);

            (void)TreeView_GetItem(hwndTV, &tvi);

            if (!_wcsicmp(szBuffer, szPathPart))
                break;
        }

        if (!hChildItem)
            return FALSE;

        if (tvi.cChildren > 0)
        {
            if (!TreeView_Expand(hwndTV, hChildItem, TVE_EXPAND))
                return FALSE;
        }

        keyPath = s ? s + 1 : L"";
        hItem = hChildItem;
    }

    (void)TreeView_SelectItem(hwndTV, hItem);
    (void)TreeView_EnsureVisible(hwndTV, hItem);

    return TRUE;
}

/* EOF */
