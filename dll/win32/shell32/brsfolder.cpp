/*
 * PROJECT:     ReactOS shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     SHBrowseForFolderA/W functions
 * COPYRIGHT:   Copyright 1999 Juergen Schmied
 *              Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

// FIXME: Many flags unimplemented

#include "precomp.h"

#include <ui/layout.h> // Resizable window

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define SHV_CHANGE_NOTIFY (WM_USER + 0x1111)

struct BrsFolder
{
    LPBROWSEINFOW    lpBrowseInfo;
    HWND             hWnd;
    HWND             hwndTreeView;
    PIDLIST_ABSOLUTE pidlRet;
    LAYOUT_DATA*     layout;  // Filled by LayoutInit, used by LayoutUpdate
    SIZE             szMin;
    ULONG            hChangeNotify; // Change notification handle
};

struct BrsItemData
{
    CComPtr<IShellFolder>            lpsfParent; // IShellFolder of the parent
    CComHeapPtr<ITEMIDLIST_RELATIVE> pidlChild;  // PIDL relative to parent
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlFull;   // Fully qualified PIDL
    CComPtr<IEnumIDList>             pEnumIL;    // Children iterator
};

static const LAYOUT_INFO g_layout_info[] =
{
    { IDC_BROWSE_FOR_FOLDER_TITLE,       BF_TOP | BF_LEFT | BF_RIGHT },
    { IDC_BROWSE_FOR_FOLDER_STATUS,      BF_TOP | BF_LEFT | BF_RIGHT },
    { IDC_BROWSE_FOR_FOLDER_TREEVIEW,    BF_TOP | BF_BOTTOM | BF_LEFT | BF_RIGHT },
    { IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT, BF_TOP | BF_LEFT | BF_RIGHT },
    { IDC_BROWSE_FOR_FOLDER_NEW_FOLDER,  BF_BOTTOM | BF_LEFT },
    { IDOK,                              BF_BOTTOM | BF_RIGHT },
    { IDCANCEL,                          BF_BOTTOM | BF_RIGHT },
};

#define SUPPORTED_FLAGS (BIF_STATUSTEXT | BIF_BROWSEFORCOMPUTER | BIF_RETURNFSANCESTORS | \
                         BIF_RETURNONLYFSDIRS | BIF_NONEWFOLDERBUTTON | BIF_NEWDIALOGSTYLE | \
                         BIF_BROWSEINCLUDEFILES)

static HTREEITEM
BrsFolder_InsertItem(
    BrsFolder *info,
    IShellFolder *lpsf,
    PCIDLIST_RELATIVE pidlChild,
    PCIDLIST_ABSOLUTE pidlParent,
    IEnumIDList* pEnumIL,
    HTREEITEM hParent);

static inline DWORD
BrowseFlagsToSHCONTF(UINT ulFlags)
{
    return SHCONTF_FOLDERS | ((ulFlags & BIF_BROWSEINCLUDEFILES) ? SHCONTF_NONFOLDERS : 0);
}

static void
BrsFolder_Callback(LPBROWSEINFOW lpBrowseInfo, HWND hWnd, UINT uMsg, LPARAM lParam)
{
    if (!lpBrowseInfo->lpfn)
        return;
    lpBrowseInfo->lpfn(hWnd, uMsg, lParam, lpBrowseInfo->lParam);
}

static BrsItemData *
BrsFolder_GetItemData(BrsFolder *info, HTREEITEM hItem)
{
    TVITEMW item = { TVIF_HANDLE | TVIF_PARAM };
    item.hItem = hItem;
    if (!TreeView_GetItem(info->hwndTreeView, &item))
    {
        ERR("TreeView_GetItem failed\n");
        return NULL;
    }
    return (BrsItemData *)item.lParam;
}

/******************************************************************************
 * BrsFolder_InitTreeView [Internal]
 *
 * Called from WM_INITDIALOG handler.
 */
static void
BrsFolder_InitTreeView(BrsFolder *info)
{
    HIMAGELIST hImageList;
    HRESULT hr;
    CComPtr<IShellFolder> lpsfParent, lpsfRoot;
    CComPtr<IEnumIDList> pEnumChildren;
    HTREEITEM hItem;

    Shell_GetImageLists(NULL, &hImageList);

    if (hImageList)
        TreeView_SetImageList(info->hwndTreeView, hImageList, 0);

    /* We want to call BrsFolder_InsertItem down the code, in order to insert
     * the root item of the treeview. Due to BrsFolder_InsertItem's signature,
     * we need the following to do this:
     *
     * + An ITEMIDLIST corresponding to _the parent_ of root.
     * + An ITEMIDLIST, which is a relative path from root's parent to root
     *   (containing a single SHITEMID).
     * + An IShellFolder interface pointer of root's parent folder.
     *
     * If root is 'Desktop', then root's parent is also 'Desktop'.
     */

    PCIDLIST_ABSOLUTE pidlRoot = info->lpBrowseInfo->pidlRoot;
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlParent(ILClone(pidlRoot));
    ILRemoveLastID(pidlParent);
    CComHeapPtr<ITEMIDLIST_RELATIVE> pidlChild(ILClone(ILFindLastID(pidlRoot)));

    if (_ILIsDesktop(pidlParent))
    {
        hr = SHGetDesktopFolder(&lpsfParent);
        if (FAILED_UNEXPECTEDLY(hr))
            return;
    }
    else
    {
        CComPtr<IShellFolder> lpsfDesktop;
        hr = SHGetDesktopFolder(&lpsfDesktop);
        if (FAILED_UNEXPECTEDLY(hr))
            return;

        hr = lpsfDesktop->BindToObject(pidlParent, NULL, IID_PPV_ARG(IShellFolder, &lpsfParent));
        if (FAILED_UNEXPECTEDLY(hr))
            return;
    }

    if (!_ILIsEmpty(pidlChild))
        hr = lpsfParent->BindToObject(pidlChild, NULL, IID_PPV_ARG(IShellFolder, &lpsfRoot));
    else
        lpsfRoot.Attach(lpsfParent);

    if (FAILED_UNEXPECTEDLY(hr))
        return;

    DWORD flags = BrowseFlagsToSHCONTF(info->lpBrowseInfo->ulFlags);
    hr = lpsfRoot->EnumObjects(info->hWnd, flags, &pEnumChildren);
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    TreeView_DeleteItem(info->hwndTreeView, TVI_ROOT);
    hItem = BrsFolder_InsertItem(info, lpsfParent, pidlChild, pidlParent, pEnumChildren, TVI_ROOT);
    TreeView_Expand(info->hwndTreeView, hItem, TVE_EXPAND);
}

static INT
BrsFolder_GetIcon(PCIDLIST_ABSOLUTE pidl, UINT uFlags)
{
    SHFILEINFOW sfi;
    SHGetFileInfoW((LPCWSTR)pidl, 0, &sfi, sizeof(sfi), uFlags);
    return sfi.iIcon;
}

static void
BrsFolder_GetIconPair(PCIDLIST_ABSOLUTE pidl, LPTVITEMW pItem)
{
    DWORD flags;

    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlDesktop;
    if (!pidl)
    {
        pidlDesktop.Attach(_ILCreateDesktop());
        pidl = pidlDesktop;
    }

    flags = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON;
    pItem->iImage = BrsFolder_GetIcon(pidl, flags);

    flags = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON;
    pItem->iSelectedImage = BrsFolder_GetIcon(pidl, flags);
}

/******************************************************************************
 * BrsFolder_GetName [Internal]
 *
 * Query a shell folder for the display name of one of its children
 *
 * PARAMS
 *  lpsf           [I] IShellFolder interface of the folder to be queried.
 *  pidlChild      [I] ITEMIDLIST of the child, relative to parent
 *  dwFlags        [I] as in IShellFolder::GetDisplayNameOf
 *  lpFriendlyName [O] The desired display name in unicode
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
static BOOL
BrsFolder_GetName(
    IShellFolder *lpsf,
    PCIDLIST_RELATIVE pidlChild,
    DWORD dwFlags,
    LPWSTR lpFriendlyName)
{
    BOOL   bSuccess = FALSE;
    STRRET str;

    TRACE("%p %p %x %p\n", lpsf, pidlChild, dwFlags, lpFriendlyName);
    if (!FAILED_UNEXPECTEDLY(lpsf->GetDisplayNameOf(pidlChild, dwFlags, &str)))
        bSuccess = StrRetToStrNW(lpFriendlyName, MAX_PATH, &str, pidlChild);

    TRACE("-- %s\n", debugstr_w(lpFriendlyName));
    return bSuccess;
}

/******************************************************************************
 * BrsFolder_InsertItem [Internal]
 *
 * PARAMS
 *  info       [I] data for the dialog
 *  lpsf       [I] IShellFolder interface of the item's parent shell folder
 *  pidlChild  [I] ITEMIDLIST of the child to insert, relative to parent
 *  pidlParent [I] ITEMIDLIST of the parent shell folder
 *  pEnumIL    [I] Iterator for the children of the item to be inserted
 *  hParent    [I] The treeview-item that represents the parent shell folder
 *
 * RETURNS
 *  Success: Handle to the created and inserted treeview-item
 *  Failure: NULL
 */
static HTREEITEM
BrsFolder_InsertItem(
    BrsFolder *info,
    IShellFolder *lpsf,
    PCIDLIST_RELATIVE pidlChild,
    PCIDLIST_ABSOLUTE pidlParent,
    IEnumIDList* pEnumIL,
    HTREEITEM hParent)
{
    WCHAR szName[MAX_PATH];
    if (!BrsFolder_GetName(lpsf, pidlChild, SHGDN_NORMAL, szName))
        return NULL;

    BrsItemData *pItemData = new BrsItemData();

    TVITEMW item = { TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN };
    item.cChildren  = (pEnumIL ? 1 : 0);
    item.pszText    = szName;
    item.cchTextMax = _countof(szName);
    item.lParam     = (LPARAM)pItemData;

    PIDLIST_ABSOLUTE pidlFull =
        (pidlParent ? ILCombine(pidlParent, pidlChild) : ILClone(pidlChild));
    BrsFolder_GetIconPair(pidlFull, &item);

    pItemData->lpsfParent.Attach(lpsf);
    pItemData->pidlChild.Attach(ILClone(pidlChild));
    pItemData->pidlFull.Attach(pidlFull);
    pItemData->pEnumIL.Attach(pEnumIL);

    TVINSERTSTRUCTW tvins = { hParent };
    tvins.item = item;
    return TreeView_InsertItem(info->hwndTreeView, &tvins);
}

/******************************************************************************
 * BrsFolder_Expand [Internal]
 *
 * For each child (given by pEnum) of the parent shell folder, which is given by
 * lpsf and whose PIDL is pidl, insert a treeview-item right under hParent
 *
 * PARAMS
 *  info    [I] data for the dialog
 *  lpsf    [I] IShellFolder interface of the parent shell folder
 *  pidl    [I] ITEMIDLIST of the parent shell folder
 *  hParent [I] The treeview item that represents the parent shell folder
 *  pEnum   [I] An iterator for the children of the parent shell folder
 */
static void
BrsFolder_Expand(
    BrsFolder *info,
    IShellFolder *lpsf,
    PCIDLIST_ABSOLUTE pidl,
    HTREEITEM hParent,
    IEnumIDList* pEnum)
{
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlTemp;
    ULONG    ulFetched;
    HRESULT  hr;
    HWND     hwnd = GetParent(info->hwndTreeView);

    TRACE("%p %p %p %p\n", lpsf, pidl, hParent, pEnum);

    // No IEnumIDList -> No children
    if (!pEnum)
        return;

    SetCapture(hwnd);
    SetCursor(LoadCursorW(NULL, (LPWSTR)IDC_WAIT));

    while (S_OK == pEnum->Next(1, &pidlTemp, &ulFetched))
    {
        ULONG ulAttrs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER;
        CComPtr<IEnumIDList> pEnumIL;
        CComPtr<IShellFolder> pSFChild;
        lpsf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidlTemp, &ulAttrs);
        if (ulAttrs & SFGAO_FOLDER)
        {
            hr = lpsf->BindToObject(pidlTemp, NULL, IID_PPV_ARG(IShellFolder, &pSFChild));
            if (!FAILED_UNEXPECTEDLY(hr))
            {
                DWORD flags = BrowseFlagsToSHCONTF(info->lpBrowseInfo->ulFlags);
                hr = pSFChild->EnumObjects(hwnd, flags, &pEnumIL);
                if (hr == S_OK)
                {
                    if ((pEnumIL->Skip(1) != S_OK) || FAILED(pEnumIL->Reset()))
                    {
                        pEnumIL.Release();
                    }
                }
            }
        }
        if (ulAttrs != (ulAttrs & SFGAO_FOLDER))
        {
            if (!BrsFolder_InsertItem(info, lpsf, pidlTemp, pidl, pEnumIL, hParent))
                goto done;
        }
        pidlTemp.Free(); // Finally, free the pidl that the shell gave us...
    }

done:
    ReleaseCapture();
    SetCursor(LoadCursorW(NULL, (LPWSTR)IDC_ARROW));
}

static inline BOOL
PIDLIsType(LPCITEMIDLIST pidl, PIDLTYPE type)
{
    LPPIDLDATA data = _ILGetDataPointer(pidl);
    if (!data)
        return FALSE;
    return (data->type == type);
}

static void
BrsFolder_CheckValidSelection(BrsFolder *info, BrsItemData *pItemData)
{
    LPBROWSEINFOW lpBrowseInfo = info->lpBrowseInfo;
    PCIDLIST_RELATIVE pidlChild = pItemData->pidlChild;
    DWORD dwAttributes;
    HRESULT hr;

    BOOL bEnabled = TRUE;
    if ((lpBrowseInfo->ulFlags & BIF_BROWSEFORCOMPUTER) && !PIDLIsType(pidlChild, PT_COMP))
        bEnabled = FALSE;

    if (lpBrowseInfo->ulFlags & BIF_RETURNFSANCESTORS)
    {
        dwAttributes = SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM;
        hr = pItemData->lpsfParent->GetAttributesOf(1, (LPCITEMIDLIST *)&pItemData->pidlChild,
                                                    &dwAttributes);
        if (FAILED(hr) || !(dwAttributes & (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM)))
            bEnabled = FALSE;
    }

    dwAttributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
    hr = pItemData->lpsfParent->GetAttributesOf(1, (LPCITEMIDLIST *)&pItemData->pidlChild,
                                                &dwAttributes);
    if (FAILED_UNEXPECTEDLY(hr) ||
        ((dwAttributes & (SFGAO_FOLDER | SFGAO_FILESYSTEM)) != (SFGAO_FOLDER | SFGAO_FILESYSTEM)))
    {
        if (lpBrowseInfo->ulFlags & BIF_RETURNONLYFSDIRS)
            bEnabled = FALSE;
        EnableWindow(GetDlgItem(info->hWnd, IDC_BROWSE_FOR_FOLDER_NEW_FOLDER), FALSE);
    }
    else
    {
        EnableWindow(GetDlgItem(info->hWnd, IDC_BROWSE_FOR_FOLDER_NEW_FOLDER), TRUE);
    }

    SendMessageW(info->hWnd, BFFM_ENABLEOK, 0, bEnabled);
}

static LRESULT
BrsFolder_Treeview_Delete(BrsFolder *info, NMTREEVIEWW *pnmtv)
{
    BrsItemData *pItemData = (BrsItemData *)pnmtv->itemOld.lParam;

    TRACE("TVN_DELETEITEMA/W %p\n", pItemData);

    delete pItemData;
    return 0;
}

static LRESULT
BrsFolder_Treeview_Expand(BrsFolder *info, NMTREEVIEWW *pnmtv)
{
    BrsItemData *pItemData = (BrsItemData *)pnmtv->itemNew.lParam;

    TRACE("TVN_ITEMEXPANDINGA/W\n");

    if ((pnmtv->itemNew.state & TVIS_EXPANDEDONCE))
        return 0;

    HRESULT hr = S_OK;
    CComPtr<IShellFolder> lpsf2;
    if (!_ILIsEmpty(pItemData->pidlChild))
    {
        hr = pItemData->lpsfParent->BindToObject(pItemData->pidlChild, NULL,
                                                 IID_PPV_ARG(IShellFolder, &lpsf2));
    }
    else
    {
        lpsf2.Attach(pItemData->lpsfParent);
    }

    HTREEITEM hItem = pnmtv->itemNew.hItem;
    if (!FAILED_UNEXPECTEDLY(hr))
        BrsFolder_Expand(info, lpsf2, pItemData->pidlFull, hItem, pItemData->pEnumIL);

    // My Computer is already sorted and trying to do a simple text
    // sort will only mess things up
    if (!_ILIsMyComputer(pItemData->pidlChild))
        TreeView_SortChildren(info->hwndTreeView, hItem, FALSE);

    return 0;
}

static HRESULT
BrsFolder_Treeview_Changed(BrsFolder *info, NMTREEVIEWW *pnmtv)
{
    BrsItemData *pItemData = (BrsItemData *)pnmtv->itemNew.lParam;

    ILFree(info->pidlRet);
    info->pidlRet = ILClone(pItemData->pidlFull);

    WCHAR szName[MAX_PATH];
    if (BrsFolder_GetName(pItemData->lpsfParent, pItemData->pidlChild, SHGDN_NORMAL, szName))
        SetDlgItemTextW(info->hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT, szName);

    BrsFolder_Callback(info->lpBrowseInfo, info->hWnd, BFFM_SELCHANGED, (LPARAM)info->pidlRet);
    BrsFolder_CheckValidSelection(info, pItemData);
    return S_OK;
}

static LRESULT
BrsFolder_Treeview_Rename(BrsFolder *info, NMTVDISPINFOW *pnmtv)
{
    WCHAR old_path[MAX_PATH], new_path[MAX_PATH];
    NMTREEVIEWW nmtv;
    TVITEMW item;

    if (!pnmtv->item.pszText)
        return 0;

    item.hItem = TreeView_GetSelection(info->hwndTreeView);
    BrsItemData *item_data = BrsFolder_GetItemData(info, item.hItem);

    SHGetPathFromIDListW(item_data->pidlFull, old_path);
    lstrcpynW(new_path, old_path, _countof(new_path));
    PathRemoveFileSpecW(new_path);
    PathAppendW(new_path, pnmtv->item.pszText);

    if (!MoveFileW(old_path, new_path))
        return 0;

    item_data->pidlFull.Free();
    item_data->pidlChild.Free();
    item_data->pidlFull.Attach(SHSimpleIDListFromPathW(new_path));
    item_data->lpsfParent->ParseDisplayName(NULL, NULL, pnmtv->item.pszText, NULL,
                                            &item_data->pidlChild, NULL);

    item.mask = TVIF_HANDLE | TVIF_TEXT;
    item.pszText = pnmtv->item.pszText;
    TreeView_SetItem(info->hwndTreeView, &item);

    nmtv.itemNew.lParam = item.lParam;
    BrsFolder_Treeview_Changed(info, &nmtv);
    return 0;
}

static HRESULT
BrsFolder_Rename(BrsFolder *info, HTREEITEM hItem)
{
    TreeView_SelectItem(info->hwndTreeView, hItem);
    TreeView_EditLabel(info->hwndTreeView, hItem);
    return S_OK;
}

static void
BrsFolder_Delete(BrsFolder *info, HTREEITEM hItem)
{
    SHFILEOPSTRUCTW fileop = { info->hwndTreeView };
    WCHAR szzFrom[MAX_PATH + 1];

    // Get item_data
    BrsItemData *item_data = BrsFolder_GetItemData(info, hItem);

    // Get the path
    if (!SHGetPathFromIDListW(item_data->pidlFull, szzFrom))
    {
        ERR("SHGetPathFromIDListW failed\n");
        return;
    }
    szzFrom[lstrlenW(szzFrom) + 1] = UNICODE_NULL; // Double NULL-terminated
    fileop.pFrom = szzFrom;

    // Delete folder
    fileop.fFlags = FOF_ALLOWUNDO;
    fileop.wFunc = FO_DELETE;
    SHFileOperationW(&fileop);
}

static LRESULT
BrsFolder_Treeview_Keydown(BrsFolder *info, LPNMTVKEYDOWN keydown)
{
    // Old dialog doesn't support those advanced features
    if (!(info->lpBrowseInfo->ulFlags & BIF_USENEWUI))
        return 0;

    HTREEITEM hItem = TreeView_GetSelection(info->hwndTreeView);

    switch (keydown->wVKey)
    {
        case VK_F2:
            BrsFolder_Rename(info, hItem);
            break;
        case VK_DELETE:
            BrsFolder_Delete(info, hItem);
            break;
    }
    return 0;
}

static LRESULT
BrsFolder_OnNotify(BrsFolder *info, UINT CtlID, LPNMHDR lpnmh)
{
    NMTREEVIEWW *pnmtv = (NMTREEVIEWW *)lpnmh;

    TRACE("%p %x %p msg=%x\n", info, CtlID, lpnmh, pnmtv->hdr.code);

    if (pnmtv->hdr.idFrom != IDC_BROWSE_FOR_FOLDER_TREEVIEW)
        return 0;

    switch (pnmtv->hdr.code)
    {
        case TVN_DELETEITEMA:
        case TVN_DELETEITEMW:
            return BrsFolder_Treeview_Delete(info, pnmtv);

        case TVN_ITEMEXPANDINGA:
        case TVN_ITEMEXPANDINGW:
            return BrsFolder_Treeview_Expand(info, pnmtv);

        case TVN_SELCHANGEDA:
        case TVN_SELCHANGEDW:
            return BrsFolder_Treeview_Changed(info, pnmtv);

        case TVN_ENDLABELEDITA:
        case TVN_ENDLABELEDITW:
            return BrsFolder_Treeview_Rename(info, (LPNMTVDISPINFOW)pnmtv);

        case TVN_KEYDOWN:
            return BrsFolder_Treeview_Keydown(info, (LPNMTVKEYDOWN)pnmtv);

        default:
            WARN("unhandled (%d)\n", pnmtv->hdr.code);
            break;
    }

    return 0;
}

static BOOL
BrsFolder_OnInitDialog(HWND hWnd, BrsFolder *info)
{
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlDesktop;
    SHChangeNotifyEntry ntreg;
    LPBROWSEINFOW lpBrowseInfo = info->lpBrowseInfo;

    info->hWnd = hWnd;
    SetPropW(hWnd, L"__WINE_BRSFOLDERDLG_INFO", info);

    if (lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE)
        FIXME("flags BIF_NEWDIALOGSTYLE partially implemented\n");

    if (lpBrowseInfo->ulFlags & ~SUPPORTED_FLAGS)
        FIXME("flags %x not implemented\n", (lpBrowseInfo->ulFlags & ~SUPPORTED_FLAGS));

    if (lpBrowseInfo->ulFlags & BIF_USENEWUI)
    {
        RECT rcWnd;

        // Resize the treeview if there's not editbox
        if ((lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE) &&
            !(lpBrowseInfo->ulFlags & BIF_EDITBOX))
        {
            RECT rcEdit, rcTreeView;
            GetWindowRect(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT), &rcEdit);
            GetWindowRect(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_TREEVIEW), &rcTreeView);
            LONG cy = rcTreeView.top - rcEdit.top;
            MapWindowPoints(NULL, hWnd, (LPPOINT)&rcTreeView, sizeof(RECT) / sizeof(POINT));
            rcTreeView.top -= cy;
            MoveWindow(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_TREEVIEW),
                       rcTreeView.left, rcTreeView.top,
                       rcTreeView.right - rcTreeView.left,
                       rcTreeView.bottom - rcTreeView.top, TRUE);
        }

        if (lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE)
            info->layout = LayoutInit(hWnd, g_layout_info, _countof(g_layout_info));
        else
            info->layout = NULL;

        // TODO: Windows allows shrinking the windows a bit
        GetWindowRect(hWnd, &rcWnd);
        info->szMin.cx = rcWnd.right - rcWnd.left;
        info->szMin.cy = rcWnd.bottom - rcWnd.top;
    }
    else
    {
        info->layout = NULL;
    }

    if (lpBrowseInfo->lpszTitle)
        SetDlgItemTextW(hWnd, IDC_BROWSE_FOR_FOLDER_TITLE, lpBrowseInfo->lpszTitle);
    else
        ShowWindow(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_TITLE), SW_HIDE);

    if (!(lpBrowseInfo->ulFlags & BIF_STATUSTEXT) || (lpBrowseInfo->ulFlags & BIF_USENEWUI))
        ShowWindow(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_STATUS), SW_HIDE);

    // Hide "Make New Folder" Button?
    if ((lpBrowseInfo->ulFlags & BIF_NONEWFOLDERBUTTON) ||
        !(lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE))
    {
        ShowWindow(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_NEW_FOLDER), SW_HIDE);
    }

    // Hide the editbox?
    if (!(lpBrowseInfo->ulFlags & BIF_EDITBOX))
    {
        ShowWindow(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT), SW_HIDE);
    }

    info->hwndTreeView = GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_TREEVIEW);
    if (info->hwndTreeView)
        BrsFolder_InitTreeView(info);
    else
        ERR("treeview control missing!\n");

    // Register for change notifications
    SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, &pidlDesktop);

    ntreg.pidl = pidlDesktop;
    ntreg.fRecursive = TRUE;
    info->hChangeNotify = SHChangeNotifyRegister(hWnd, SHCNRF_InterruptLevel, SHCNE_ALLEVENTS,
                                                 SHV_CHANGE_NOTIFY, 1, &ntreg);

    SetFocus(info->hwndTreeView);
    BrsFolder_Callback(info->lpBrowseInfo, hWnd, BFFM_INITIALIZED, 0);

    SHAutoComplete(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT),
                   (SHACF_FILESYS_ONLY | SHACF_URLHISTORY | SHACF_FILESYSTEM));
    return TRUE;
}

static HRESULT
BrsFolder_NewFolder(BrsFolder *info)
{
    DWORD flags = BrowseFlagsToSHCONTF(info->lpBrowseInfo->ulFlags);
    CComPtr<IShellFolder> desktop, cur;
    WCHAR wszNewFolder[25], path[MAX_PATH], name[MAX_PATH];

    HRESULT hr = SHGetDesktopFolder(&desktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    if (info->pidlRet)
    {
        hr = desktop->BindToObject(info->pidlRet, NULL, IID_PPV_ARG(IShellFolder, &cur));
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        hr = SHGetPathFromIDListW(info->pidlRet, path);
    }
    else
    {
        cur.Attach(desktop);
        hr = SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, path);
    }

    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = E_FAIL;
    if (!LoadStringW(shell32_hInstance, IDS_NEWFOLDER, wszNewFolder, _countof(wszNewFolder)))
        return hr;

    if (!PathYetAnotherMakeUniqueName(name, path, NULL, wszNewFolder))
        return hr;

    INT len = lstrlenW(path);
    if (len < MAX_PATH && name[len] == L'\\')
        len++;

    if (!CreateDirectoryW(name, NULL))
        return hr;

    // Update parent of newly created directory
    HTREEITEM hParent = TreeView_GetSelection(info->hwndTreeView);
    if (!hParent)
        return hr;

    TreeView_Expand(info->hwndTreeView, hParent, TVE_EXPAND);

    TVITEMW item = { TVIF_PARAM | TVIF_STATE };
    item.hItem = hParent;
    TreeView_GetItem(info->hwndTreeView, &item);
    BrsItemData *item_data = (BrsItemData *)item.lParam;
    if (!item_data)
        return hr;

    if (item_data->pEnumIL)
        item_data->pEnumIL.Release();
    hr = cur->EnumObjects(info->hwndTreeView, flags, &item_data->pEnumIL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    // Update treeview
    if (!(item.state & TVIS_EXPANDEDONCE))
    {
        item.mask = TVIF_STATE;
        item.state = item.stateMask = TVIS_EXPANDEDONCE;
        TreeView_SetItem(info->hwndTreeView, &item);
    }

    CComHeapPtr<ITEMIDLIST_RELATIVE> pidlNew;
    hr = cur->ParseDisplayName(NULL, NULL, name + len, NULL, &pidlNew, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    HTREEITEM hAdded = BrsFolder_InsertItem(info, cur, pidlNew, item_data->pidlFull, NULL, hParent);

    TreeView_SortChildren(info->hwndTreeView, hParent, FALSE);
    return BrsFolder_Rename(info, hAdded);
}

static void
BrsFolder_OnCommand(BrsFolder *info, UINT id)
{
    LPBROWSEINFOW lpBrowseInfo = info->lpBrowseInfo;
    WCHAR szPath[MAX_PATH];

    switch (id)
    {
        case IDOK:
        {
            // Get the text
            GetDlgItemTextW(info->hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT, szPath, _countof(szPath));
            StrTrimW(szPath, L" \t");

            // The original pidl is owned by the treeview and will be free'd.
            if (!PathIsRelativeW(szPath) && PathIsDirectoryW(szPath))
            {
                // It's valid path
                info->pidlRet = ILCreateFromPathW(szPath);
            }
            else
            {
                info->pidlRet = ILClone(info->pidlRet);
            }
            if (info->pidlRet == NULL) // A null pidl would mean a cancel
                info->pidlRet = _ILCreateDesktop();
            pdump(info->pidlRet);
            if (lpBrowseInfo->pszDisplayName)
            {
                SHFILEINFOW fileInfo = { NULL };
                lpBrowseInfo->pszDisplayName[0] = UNICODE_NULL;
                if (SHGetFileInfoW((LPCWSTR)info->pidlRet, 0, &fileInfo, sizeof(fileInfo),
                                   SHGFI_PIDL | SHGFI_DISPLAYNAME))
                {
                    lstrcpynW(lpBrowseInfo->pszDisplayName, fileInfo.szDisplayName, MAX_PATH);
                }
            }
            EndDialog(info->hWnd, IDOK);
            break;
        }
        case IDCANCEL:
        {
            EndDialog(info->hWnd, IDCANCEL);
            break;
        }
        case IDC_BROWSE_FOR_FOLDER_NEW_FOLDER:
        {
            BrsFolder_NewFolder(info);
            break;
        }
    }
}

static BOOL
BrsFolder_OnSetExpandedPidl(BrsFolder *info, LPITEMIDLIST pidlSelection, HTREEITEM *phItem)
{
    if (_ILIsDesktop(pidlSelection))
    {
        if (phItem)
            *phItem = TVI_ROOT;
        return TRUE;
    }

    // Move pidlCurrent behind the SHITEMIDs in pidlSelection, which are the root of
    // the sub-tree currently displayed.
    PCIDLIST_ABSOLUTE pidlRoot = info->lpBrowseInfo->pidlRoot;
    LPITEMIDLIST pidlCurrent = pidlSelection;
    while (!_ILIsEmpty(pidlRoot) && _ILIsEqualSimple(pidlRoot, pidlCurrent))
    {
        pidlRoot = ILGetNext(pidlRoot);
        pidlCurrent = ILGetNext(pidlCurrent);
    }

    // The given ID List is not part of the SHBrowseForFolder's current sub-tree.
    if (!_ILIsEmpty(pidlRoot))
    {
        if (phItem)
            *phItem = NULL;
        return FALSE;
    }

    // Initialize item to point to the first child of the root folder.
    TVITEMEXW item = { TVIF_PARAM };
    item.hItem = TreeView_GetRoot(info->hwndTreeView);

    if (item.hItem)
        item.hItem = TreeView_GetChild(info->hwndTreeView, item.hItem);

    // Walk the tree along the nodes corresponding to the remaining ITEMIDLIST
    while (item.hItem && !_ILIsEmpty(pidlCurrent))
    {
        TreeView_GetItem(info->hwndTreeView, &item);
        BrsItemData *pItemData = (BrsItemData *)item.lParam;

        if (_ILIsEqualSimple(pItemData->pidlChild, pidlCurrent))
        {
            pidlCurrent = ILGetNext(pidlCurrent);
            if (!_ILIsEmpty(pidlCurrent))
            {
                // Only expand current node and move on to its first child,
                // if we didn't already reach the last SHITEMID
                TreeView_Expand(info->hwndTreeView, item.hItem, TVE_EXPAND);
                item.hItem = TreeView_GetChild(info->hwndTreeView, item.hItem);
            }
        }
        else
        {
            item.hItem = TreeView_GetNextSibling(info->hwndTreeView, item.hItem);
        }
    }

    if (phItem)
        *phItem = item.hItem;

    return (_ILIsEmpty(pidlCurrent) && item.hItem);
}

static BOOL
BrsFolder_OnSetExpandedString(BrsFolder *info, LPWSTR pszString, HTREEITEM *phItem)
{
    CComPtr<IShellFolder> psfDesktop;
    HRESULT hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlSelection;
    hr = psfDesktop->ParseDisplayName(NULL, NULL, pszString, NULL, &pidlSelection, NULL);
    if (FAILED_UNEXPECTEDLY(hr))
        return FALSE;

    return BrsFolder_OnSetExpandedPidl(info, pidlSelection, phItem);
}

static BOOL
BrsFolder_OnSetSelectionPidl(BrsFolder *info, LPITEMIDLIST pidlSelection)
{
    if (!pidlSelection)
        return FALSE;

    HTREEITEM hItem;
    BOOL ret = BrsFolder_OnSetExpandedPidl(info, pidlSelection, &hItem);
    if (ret)
        TreeView_SelectItem(info->hwndTreeView, hItem);
    return ret;
}

static BOOL
BrsFolder_OnSetSelectionW(BrsFolder *info, LPWSTR pszSelection)
{
    if (!pszSelection)
        return FALSE;

    HTREEITEM hItem;
    BOOL ret = BrsFolder_OnSetExpandedString(info, pszSelection, &hItem);
    if (ret)
        TreeView_SelectItem(info->hwndTreeView, hItem);
    return ret;
}

static BOOL
BrsFolder_OnSetSelectionA(BrsFolder *info, LPSTR pszSelectionA)
{
    if (!pszSelectionA)
        return FALSE;

    CComHeapPtr<WCHAR> pszSelectionW;
    __SHCloneStrAtoW(&pszSelectionW, pszSelectionA);
    if (!pszSelectionW)
        return FALSE;

    return BrsFolder_OnSetSelectionW(info, pszSelectionW);
}

static void
BrsFolder_OnDestroy(BrsFolder *info)
{
    if (info->layout)
    {
        LayoutDestroy(info->layout);
        info->layout = NULL;
    }

    SHChangeNotifyDeregister(info->hChangeNotify);
}

// Find a treeview node by recursively walking the treeview
static HTREEITEM
BrsFolder_FindItemByPidl(BrsFolder *info, PCIDLIST_ABSOLUTE pidlFull, HTREEITEM hItem)
{
    BrsItemData *item_data = BrsFolder_GetItemData(info, hItem);

    HRESULT hr = item_data->lpsfParent->CompareIDs(0, item_data->pidlFull, pidlFull);
    if (SUCCEEDED(hr) && !HRESULT_CODE(hr))
        return hItem;

    for (hItem = TreeView_GetChild(info->hwndTreeView, hItem); hItem;
         hItem = TreeView_GetNextSibling(info->hwndTreeView, hItem))
    {
        HTREEITEM newItem = BrsFolder_FindItemByPidl(info, pidlFull, hItem);
        if (newItem)
            return newItem;
    }

    return NULL;
}

static void
BrsFolder_OnChange(BrsFolder *info, const PCIDLIST_ABSOLUTE *pidls, LONG event)
{
    TRACE("(%p)->(%p, %p, 0x%08x)\n", info, pidls[0], pidls[1], event);

    switch (event)
    {
        case SHCNE_RMDIR:
        case SHCNE_DELETE:
        {
            HTREEITEM hRoot = TreeView_GetRoot(info->hwndTreeView);
            HTREEITEM hItem = BrsFolder_FindItemByPidl(info, pidls[0], hRoot);
            if (hItem)
                TreeView_DeleteItem(info->hwndTreeView, hItem);
            break;
        }
    }
}

/*************************************************************************
 *             BrsFolderDlgProc32  (not an exported API function)
 */
static INT_PTR CALLBACK
BrsFolderDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
        return BrsFolder_OnInitDialog(hWnd, (BrsFolder *)lParam);

    BrsFolder *info = (BrsFolder *)GetPropW(hWnd, L"__WINE_BRSFOLDERDLG_INFO");
    if (!info)
        return 0;

    switch (uMsg)
    {
        case WM_NOTIFY:
            return BrsFolder_OnNotify(info, (UINT)wParam, (LPNMHDR)lParam);

        case WM_COMMAND:
            BrsFolder_OnCommand(info, wParam);
            break;

        case WM_GETMINMAXINFO:
            ((LPMINMAXINFO)lParam)->ptMinTrackSize.x = info->szMin.cx;
            ((LPMINMAXINFO)lParam)->ptMinTrackSize.y = info->szMin.cy;
            break;

        case WM_SIZE:
            if (info->layout)  // New style dialogs
                LayoutUpdate(hWnd, info->layout, g_layout_info, _countof(g_layout_info));
            break;

        case BFFM_SETSTATUSTEXTA:
            SetDlgItemTextA(hWnd, IDC_BROWSE_FOR_FOLDER_STATUS, (LPSTR)lParam);
            break;

        case BFFM_SETSTATUSTEXTW:
            SetDlgItemTextW(hWnd, IDC_BROWSE_FOR_FOLDER_STATUS, (LPWSTR)lParam);
            break;

        case BFFM_ENABLEOK:
            EnableWindow(GetDlgItem(hWnd, IDOK), lParam != 0);
            break;

        case BFFM_SETOKTEXT: // Unicode only
            SetDlgItemTextW(hWnd, IDOK, (LPWSTR)lParam);
            break;

        case BFFM_SETSELECTIONA:
            if (wParam) // String
                return BrsFolder_OnSetSelectionA(info, (LPSTR)lParam);
            else // PIDL
                return BrsFolder_OnSetSelectionPidl(info, (LPITEMIDLIST)lParam);

        case BFFM_SETSELECTIONW:
            if (wParam) // String
                return BrsFolder_OnSetSelectionW(info, (LPWSTR)lParam);
            else // PIDL
                return BrsFolder_OnSetSelectionPidl(info, (LPITEMIDLIST)lParam);

        case BFFM_SETEXPANDED: // Unicode only
            if (wParam) // String
                return BrsFolder_OnSetExpandedString(info, (LPWSTR)lParam, NULL);
            else // PIDL
                return BrsFolder_OnSetExpandedPidl(info, (LPITEMIDLIST)lParam, NULL);

        case SHV_CHANGE_NOTIFY:
            BrsFolder_OnChange(info, (const PCIDLIST_ABSOLUTE *)wParam, (LONG)lParam);
            break;

        case WM_DESTROY:
            BrsFolder_OnDestroy(info);
            break;
    }

    return 0;
}

/*************************************************************************
 * SHBrowseForFolderA [SHELL32.@]
 * SHBrowseForFolder  [SHELL32.@]
 */
EXTERN_C
LPITEMIDLIST WINAPI
SHBrowseForFolderA(LPBROWSEINFOA lpbi)
{
    BROWSEINFOW bi;
    bi.hwndOwner = lpbi->hwndOwner;
    bi.pidlRoot = lpbi->pidlRoot;

    WCHAR szName[MAX_PATH];
    bi.pszDisplayName = (lpbi->pszDisplayName ? szName : NULL);

    CComHeapPtr<WCHAR> pszTitle;
    if (lpbi->lpszTitle)
        __SHCloneStrAtoW(&pszTitle, lpbi->lpszTitle);
    bi.lpszTitle = pszTitle;

    bi.ulFlags = lpbi->ulFlags;
    bi.lpfn = lpbi->lpfn;
    bi.lParam = lpbi->lParam;
    bi.iImage = lpbi->iImage;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);

    if (bi.pszDisplayName)
        SHUnicodeToAnsi(bi.pszDisplayName, lpbi->pszDisplayName, MAX_PATH);

    lpbi->iImage = bi.iImage;
    return pidl;
}

/*************************************************************************
 * SHBrowseForFolderW [SHELL32.@]
 */
EXTERN_C
LPITEMIDLIST WINAPI
SHBrowseForFolderW(LPBROWSEINFOW lpbi)
{
    TRACE("%p\n", lpbi);

    BrsFolder info = { lpbi };

    HRESULT hr = OleInitialize(NULL);

    INT id = ((lpbi->ulFlags & BIF_USENEWUI) ? IDD_BROWSE_FOR_FOLDER_NEW : IDD_BROWSE_FOR_FOLDER);
    INT_PTR ret = DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(id), lpbi->hwndOwner,
                                  BrsFolderDlgProc, (LPARAM)&info);
    if (SUCCEEDED(hr))
        OleUninitialize();

    if (ret != IDOK)
    {
        ILFree(info.pidlRet);
        return NULL;
    }

    return info.pidlRet;
}
