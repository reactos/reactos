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
#include <compat_undoc.h> // RosGetProcessEffectiveVersion

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define SHV_CHANGE_NOTIFY (WM_USER + 0x1111)

static LPITEMIDLIST
ILCloneToDepth(LPCITEMIDLIST pidlSrc, UINT depth)
{
    SIZE_T cb = 0;
    for (LPCITEMIDLIST pidl = pidlSrc; pidl && depth--; pidl = ILGetNext(pidl))
        cb += pidl->mkid.cb;

    LPITEMIDLIST pidlOut = (LPITEMIDLIST)SHAlloc(cb + sizeof(WORD));
    if (pidlOut)
    {
        CopyMemory(pidlOut, pidlSrc, cb);
        ZeroMemory(((BYTE*)pidlOut) + cb, sizeof(WORD));
    }
    return pidlOut;
}

static INT
GetIconIndex(PCIDLIST_ABSOLUTE pidl, UINT uFlags)
{
    SHFILEINFOW sfi;
    SHGetFileInfoW((LPCWSTR)pidl, 0, &sfi, sizeof(sfi), uFlags);
    return sfi.iIcon;
}

struct BrFolder
{
    LPBROWSEINFOW    lpBrowseInfo;
    HWND             hWnd;
    HWND             hwndTreeView;
    PIDLIST_ABSOLUTE pidlRet;
    LAYOUT_DATA*     layout;  // Filled by LayoutInit, used by LayoutUpdate
    SIZE             szMin;
    ULONG            hChangeNotify; // Change notification handle
    IContextMenu*    pContextMenu; // Active context menu
};

struct BrItemData
{
    CComPtr<IShellFolder>            lpsfParent; // IShellFolder of the parent
    PCIDLIST_RELATIVE                pidlChild;  // PIDL relative to parent
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlFull;   // Fully qualified PIDL
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
BrFolder_InsertItem(
    _Inout_ BrFolder *info,
    _Inout_ IShellFolder *lpsf,
    _In_ PCUITEMID_CHILD pidlChild,
    _In_ PCIDLIST_ABSOLUTE pidlParent,
    _In_ HTREEITEM hParent);

static inline DWORD
BrowseFlagsToSHCONTF(UINT ulFlags)
{
    return SHCONTF_FOLDERS | ((ulFlags & BIF_BROWSEINCLUDEFILES) ? SHCONTF_NONFOLDERS : 0);
}

static void
BrFolder_Callback(LPBROWSEINFOW lpBrowseInfo, HWND hWnd, UINT uMsg, LPARAM lParam)
{
    if (!lpBrowseInfo->lpfn)
        return;
    lpBrowseInfo->lpfn(hWnd, uMsg, lParam, lpBrowseInfo->lParam);
}

static BrItemData *
BrFolder_GetItemData(BrFolder *info, HTREEITEM hItem)
{
    TVITEMW item = { TVIF_HANDLE | TVIF_PARAM };
    item.hItem = hItem;
    if (!TreeView_GetItem(info->hwndTreeView, &item))
    {
        ERR("TreeView_GetItem failed\n");
        return NULL;
    }
    return (BrItemData *)item.lParam;
}

static SFGAOF
BrFolder_GetItemAttributes(BrFolder *info, HTREEITEM hItem, SFGAOF Att)
{
    BrItemData *item = BrFolder_GetItemData(info, hItem);
    HRESULT hr = item ? item->lpsfParent->GetAttributesOf(1, &item->pidlChild, &Att) : E_FAIL;
    return SUCCEEDED(hr) ? Att : 0;
}

static HRESULT
BrFolder_GetChildrenEnum(
    _In_ BrFolder *info,
    _In_ BrItemData *pItemData,
    _Outptr_opt_ IEnumIDList **ppEnum)
{
    if (!pItemData)
        return E_FAIL;

    CComPtr<IEnumIDList> pEnumIL;
    PCUITEMID_CHILD pidlRef = pItemData->pidlChild;
    ULONG attrs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER;
    IShellFolder *lpsf = pItemData->lpsfParent;
    HRESULT hr = lpsf->GetAttributesOf(1, &pidlRef, &attrs);
    if (FAILED_UNEXPECTEDLY(hr) || !(attrs & SFGAO_FOLDER))
        return E_FAIL;

    CComPtr<IShellFolder> psfChild;
    if (_ILIsDesktop(pItemData->pidlFull))
        hr = SHGetDesktopFolder(&psfChild);
    else
        hr = lpsf->BindToObject(pidlRef, NULL, IID_PPV_ARG(IShellFolder, &psfChild));
    if (FAILED_UNEXPECTEDLY(hr))
        return E_FAIL;

    DWORD flags = BrowseFlagsToSHCONTF(info->lpBrowseInfo->ulFlags);
    hr = psfChild->EnumObjects(info->hWnd, flags, &pEnumIL);
    if (hr == S_OK)
    {
        if ((pEnumIL->Skip(1) != S_OK) || FAILED(pEnumIL->Reset()))
            pEnumIL = NULL;
    }

    if (!pEnumIL || !(attrs & SFGAO_HASSUBFOLDER))
        return E_FAIL; // No children

    if (ppEnum)
        *ppEnum = pEnumIL.Detach();

    return S_OK; // There are children
}

/******************************************************************************
 * BrFolder_InitTreeView [Internal]
 *
 * Called from WM_INITDIALOG handler.
 */
static void
BrFolder_InitTreeView(BrFolder *info)
{
    HIMAGELIST hImageList;
    HRESULT hr;
    HTREEITEM hItem;

    Shell_GetImageLists(NULL, &hImageList);
    if (hImageList)
        TreeView_SetImageList(info->hwndTreeView, hImageList, 0);

    /* We want to call BrFolder_InsertItem down the code, in order to insert
     * the root item of the treeview. Due to BrFolder_InsertItem's signature,
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
    PCIDLIST_RELATIVE pidlChild = ILFindLastID(pidlRoot);

    CComPtr<IShellFolder> lpsfParent;
    hr = SHBindToObject(NULL, pidlParent, /*NULL, */ IID_PPV_ARG(IShellFolder, &lpsfParent));
    if (FAILED_UNEXPECTEDLY(hr))
        return;

    TreeView_DeleteItem(info->hwndTreeView, TVI_ROOT);
    hItem = BrFolder_InsertItem(info, lpsfParent, pidlChild, pidlParent, TVI_ROOT);
    TreeView_Expand(info->hwndTreeView, hItem, TVE_EXPAND);
}

static void
BrFolder_GetIconPair(PCIDLIST_ABSOLUTE pidl, LPTVITEMW pItem)
{
    static const ITEMIDLIST idlDesktop = { };
    if (!pidl)
        pidl = &idlDesktop;
    DWORD flags = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON;
    pItem->iImage = GetIconIndex(pidl, flags);
    pItem->iSelectedImage = GetIconIndex(pidl, flags | SHGFI_OPENICON);
}

/******************************************************************************
 * BrFolder_GetName [Internal]
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
BrFolder_GetName(
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

static BOOL
BrFolder_IsTreeItemInEnum(
    _Inout_ BrFolder *info,
    _In_ HTREEITEM hItem,
    _Inout_ IEnumIDList *pEnum)
{
    BrItemData *pItemData = BrFolder_GetItemData(info, hItem);
    if (!pItemData)
        return FALSE;

    pEnum->Reset();

    CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
    while (pEnum->Next(1, &pidlTemp, NULL) == S_OK)
    {
        if (ILIsEqual(pidlTemp, pItemData->pidlChild))
            return TRUE;

        pidlTemp.Free();
    }

    return FALSE;
}

static BOOL
BrFolder_TreeItemHasThisChild(
    _In_ BrFolder *info,
    _In_ HTREEITEM hItem,
    _Outptr_opt_ PITEMID_CHILD pidlChild)
{
    for (hItem = TreeView_GetChild(info->hwndTreeView, hItem); hItem;
         hItem = TreeView_GetNextSibling(info->hwndTreeView, hItem))
    {
        BrItemData *pItemData = BrFolder_GetItemData(info, hItem);
        if (ILIsEqual(pItemData->pidlChild, pidlChild))
            return TRUE;
    }

    return FALSE;
}

static void
BrFolder_UpdateItem(
    _In_ BrFolder *info,
    _In_ HTREEITEM hItem)
{
    BrItemData *pItemData = BrFolder_GetItemData(info, hItem);
    if (!pItemData)
        return;

    TVITEMW item = { TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN };
    item.hItem = hItem;

    BrFolder_GetIconPair(pItemData->pidlFull, &item);

    item.cChildren = 0;
    CComPtr<IEnumIDList> pEnum;
    if (BrFolder_GetChildrenEnum(info, pItemData, &pEnum) == S_OK)
    {
        CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
        if (pEnum->Next(1, &pidlTemp, NULL) == S_OK)
            ++item.cChildren;
    }

    TreeView_SetItem(info->hwndTreeView, &item);
}

/******************************************************************************
 * BrFolder_InsertItem [Internal]
 *
 * PARAMS
 *  info       [I] data for the dialog
 *  lpsf       [I] IShellFolder interface of the item's parent shell folder
 *  pidlChild  [I] ITEMIDLIST of the child to insert, relative to parent
 *  pidlParent [I] ITEMIDLIST of the parent shell folder
 *  hParent    [I] The treeview-item that represents the parent shell folder
 *
 * RETURNS
 *  Success: Handle to the created and inserted treeview-item
 *  Failure: NULL
 */
static HTREEITEM
BrFolder_InsertItem(
    _Inout_ BrFolder *info,
    _Inout_ IShellFolder *lpsf,
    _In_ PCUITEMID_CHILD pidlChild,
    _In_ PCIDLIST_ABSOLUTE pidlParent,
    _In_ HTREEITEM hParent)
{
    if (!(BrowseFlagsToSHCONTF(info->lpBrowseInfo->ulFlags) & SHCONTF_NONFOLDERS))
    {
#ifdef BIF_BROWSEFILEJUNCTIONS
        if (!(info->lpBrowseInfo->ulFlags & BIF_BROWSEFILEJUNCTIONS))
#endif
            if (SHGetAttributes(lpsf, pidlChild, SFGAO_STREAM) & SFGAO_STREAM)
                return NULL; // .zip files have both FOLDER and STREAM attributes set
    }

    WCHAR szName[MAX_PATH];
    if (!BrFolder_GetName(lpsf, pidlChild, SHGDN_NORMAL, szName))
        return NULL;

    BrItemData *pItemData = new BrItemData();

    TVITEMW item = { TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN };
    item.pszText    = szName;
    item.cchTextMax = _countof(szName);
    item.lParam     = (LPARAM)pItemData;

    PIDLIST_ABSOLUTE pidlFull =
        (pidlParent ? ILCombine(pidlParent, pidlChild) : ILClone(pidlChild));
    BrFolder_GetIconPair(pidlFull, &item);

    pItemData->lpsfParent = lpsf;
    pItemData->pidlFull.Attach(pidlFull);
    pItemData->pidlChild = ILFindLastID(pItemData->pidlFull);

    if (BrFolder_GetChildrenEnum(info, pItemData, NULL) == S_OK)
        item.cChildren = 1;

    TVINSERTSTRUCTW tvins = { hParent };
    tvins.item = item;
    return TreeView_InsertItem(info->hwndTreeView, &tvins);
}

/******************************************************************************
 * BrFolder_Expand [Internal]
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
BrFolder_Expand(
    _In_ BrFolder *info,
    _In_ IShellFolder *lpsf,
    _In_ PCIDLIST_ABSOLUTE pidlFull,
    _In_ HTREEITEM hParent)
{
    TRACE("%p %p %p\n", lpsf, pidlFull, hParent);

    // No IEnumIDList -> No children
    BrItemData *pItemData = BrFolder_GetItemData(info, hParent);
    CComPtr<IEnumIDList> pEnum;
    HRESULT hr = BrFolder_GetChildrenEnum(info, pItemData, &pEnum);
    if (FAILED(hr))
        return;

    SetCapture(info->hWnd);
    SetCursor(LoadCursorW(NULL, (LPWSTR)IDC_WAIT));

    CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
    ULONG ulFetched;
    while (S_OK == pEnum->Next(1, &pidlTemp, &ulFetched))
    {
        if (!BrFolder_InsertItem(info, lpsf, pidlTemp, pidlFull, hParent))
            break;
        pidlTemp.Free(); // Finally, free the pidl that the shell gave us...
    }

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
BrFolder_CheckValidSelection(BrFolder *info, BrItemData *pItemData)
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
        hr = pItemData->lpsfParent->GetAttributesOf(1, &pidlChild, &dwAttributes);
        if (FAILED(hr) || !(dwAttributes & (SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM)))
            bEnabled = FALSE;
    }

    dwAttributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
    hr = pItemData->lpsfParent->GetAttributesOf(1, &pidlChild, &dwAttributes);
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
BrFolder_Treeview_Delete(BrFolder *info, NMTREEVIEWW *pnmtv)
{
    BrItemData *pItemData = (BrItemData *)pnmtv->itemOld.lParam;

    TRACE("TVN_DELETEITEMA/W %p\n", pItemData);

    delete pItemData;
    return 0;
}

static LRESULT
BrFolder_Treeview_Expand(BrFolder *info, NMTREEVIEWW *pnmtv)
{
    BrItemData *pItemData = (BrItemData *)pnmtv->itemNew.lParam;

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
        lpsf2 = pItemData->lpsfParent;
    }

    HTREEITEM hItem = pnmtv->itemNew.hItem;
    if (!FAILED_UNEXPECTEDLY(hr))
        BrFolder_Expand(info, lpsf2, pItemData->pidlFull, hItem);

    // My Computer is already sorted and trying to do a simple text
    // sort will only mess things up
    if (!_ILIsMyComputer(pItemData->pidlChild))
        TreeView_SortChildren(info->hwndTreeView, hItem, FALSE);

    return 0;
}

static HRESULT
BrFolder_Treeview_Changed(BrFolder *info, NMTREEVIEWW *pnmtv)
{
    BrItemData *pItemData = (BrItemData *)pnmtv->itemNew.lParam;

    ILFree(info->pidlRet);
    info->pidlRet = ILClone(pItemData->pidlFull);

    WCHAR szName[MAX_PATH];
    if (BrFolder_GetName(pItemData->lpsfParent, pItemData->pidlChild, SHGDN_NORMAL, szName))
        SetDlgItemTextW(info->hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT, szName);

    BrFolder_Callback(info->lpBrowseInfo, info->hWnd, BFFM_SELCHANGED, (LPARAM)info->pidlRet);
    BrFolder_CheckValidSelection(info, pItemData);
    return S_OK;
}

static LRESULT
BrFolder_Treeview_Rename(BrFolder *info, NMTVDISPINFOW *pnmtv)
{
    if (!pnmtv->item.pszText)
        return FALSE;

    HTREEITEM hItem = TreeView_GetSelection(info->hwndTreeView);
    BrItemData *data = BrFolder_GetItemData(info, hItem);
    ASSERT(data);
    ASSERT(BrFolder_GetItemAttributes(info, hItem, SFGAO_CANRENAME) & SFGAO_CANRENAME);

    PITEMID_CHILD newChild;
    HRESULT hr = data->lpsfParent->SetNameOf(info->hWnd, data->pidlChild,
                                             pnmtv->item.pszText, SHGDN_NORMAL, &newChild);
    if (FAILED(hr))
        return FALSE;

    LPITEMIDLIST newFull;
    if (SUCCEEDED(hr = SHILClone(data->pidlFull, &newFull)))
    {
        ILRemoveLastID(newFull);
        if (SUCCEEDED(hr = SHILAppend(newChild, &newFull)))
        {
            data->pidlFull.Free();
            data->pidlFull.Attach(newFull);
            data->pidlChild = ILFindLastID(data->pidlFull);
        }
        newChild = NULL; // SHILAppend is nuts and frees this
    }
    ILFree(newChild);

    NMTREEVIEWW nmtv;
    nmtv.itemNew.lParam = (LPARAM)data;
    BrFolder_Treeview_Changed(info, &nmtv);
    return TRUE;
}

static HRESULT
BrFolder_Rename(BrFolder *info, HTREEITEM hItem)
{
    TreeView_SelectItem(info->hwndTreeView, hItem);
    TreeView_EditLabel(info->hwndTreeView, hItem);
    return S_OK;
}

static void
BrFolder_Delete(BrFolder *info, HTREEITEM hItem)
{
    SHFILEOPSTRUCTW fileop = { info->hwndTreeView };
    WCHAR szzFrom[MAX_PATH + 1];

    // Get item_data
    BrItemData *item_data = BrFolder_GetItemData(info, hItem);

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

static void
BrFolder_Refresh(_Inout_ BrFolder *info);

static LRESULT
BrFolder_Treeview_Keydown(BrFolder *info, LPNMTVKEYDOWN keydown)
{
    // Old dialog doesn't support those advanced features
    if (!(info->lpBrowseInfo->ulFlags & BIF_USENEWUI))
        return 0;

    HTREEITEM hItem = TreeView_GetSelection(info->hwndTreeView);

    switch (keydown->wVKey)
    {
        case VK_F2:
            BrFolder_Rename(info, hItem);
            break;
        case VK_DELETE:
            BrFolder_Delete(info, hItem);
            break;
        case 'R':
        {
            if (GetKeyState(VK_CONTROL) < 0) // Ctrl+R
                BrFolder_Refresh(info);
            break;
        }
        case VK_F5:
        {
            BrFolder_Refresh(info);
            break;
        }
    }
    return 0;
}

static LRESULT
BrFolder_OnNotify(BrFolder *info, UINT CtlID, LPNMHDR lpnmh)
{
    NMTREEVIEWW *pnmtv = (NMTREEVIEWW *)lpnmh;

    TRACE("%p %x %p msg=%x\n", info, CtlID, lpnmh, pnmtv->hdr.code);

    if (pnmtv->hdr.idFrom != IDC_BROWSE_FOR_FOLDER_TREEVIEW)
        return 0;

    switch (pnmtv->hdr.code)
    {
        case TVN_DELETEITEMA:
        case TVN_DELETEITEMW:
            return BrFolder_Treeview_Delete(info, pnmtv);

        case TVN_ITEMEXPANDINGA:
        case TVN_ITEMEXPANDINGW:
            return BrFolder_Treeview_Expand(info, pnmtv);

        case TVN_SELCHANGEDA:
        case TVN_SELCHANGEDW:
            return BrFolder_Treeview_Changed(info, pnmtv);

        case TVN_BEGINLABELEDITA:
        case TVN_BEGINLABELEDITW:
        {
            NMTVDISPINFO &tvdi = *(NMTVDISPINFO*)lpnmh;
            UINT att = BrFolder_GetItemAttributes(info, tvdi.item.hItem, SFGAO_CANRENAME);
            return !(att & SFGAO_CANRENAME);
        }

        case TVN_ENDLABELEDITW:
            return BrFolder_Treeview_Rename(info, (LPNMTVDISPINFOW)pnmtv);

        case TVN_KEYDOWN:
            return BrFolder_Treeview_Keydown(info, (LPNMTVKEYDOWN)pnmtv);

        default:
            WARN("unhandled (%d)\n", pnmtv->hdr.code);
            break;
    }

    return 0;
}

static BOOL
BrFolder_OnInitDialog(HWND hWnd, BrFolder *info)
{
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlDesktop;
    SHChangeNotifyEntry ntreg;
    LPBROWSEINFOW lpBrowseInfo = info->lpBrowseInfo;

    info->hWnd = hWnd;
    SetWindowLongPtrW(hWnd, DWLP_USER, (LONG_PTR)info);

    if (lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE)
        FIXME("flags BIF_NEWDIALOGSTYLE partially implemented\n");

    if (lpBrowseInfo->ulFlags & ~SUPPORTED_FLAGS)
        FIXME("flags %x not implemented\n", (lpBrowseInfo->ulFlags & ~SUPPORTED_FLAGS));

    info->layout = NULL;
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

        // TODO: Windows allows shrinking the windows a bit
        GetWindowRect(hWnd, &rcWnd);
        info->szMin.cx = rcWnd.right - rcWnd.left;
        info->szMin.cy = rcWnd.bottom - rcWnd.top;
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
        BrFolder_InitTreeView(info);
    else
        ERR("treeview control missing!\n");

    // Register for change notifications
    SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, &pidlDesktop);

    ntreg.pidl = pidlDesktop;
    ntreg.fRecursive = TRUE;
    info->hChangeNotify = SHChangeNotifyRegister(hWnd,
                                                 SHCNRF_ShellLevel | SHCNRF_NewDelivery,
                                                 SHCNE_ALLEVENTS,
                                                 SHV_CHANGE_NOTIFY, 1, &ntreg);

    if (!lpBrowseInfo->pidlRoot)
    {
        UINT csidl = (lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE) ? CSIDL_PERSONAL : CSIDL_DRIVES;
        LPITEMIDLIST pidl = SHCloneSpecialIDList(NULL, csidl, TRUE);
        if (pidl)
        {
            SendMessageW(info->hWnd, BFFM_SETSELECTION, FALSE, (LPARAM)pidl);
            if (csidl == CSIDL_DRIVES)
                SendMessageW(info->hWnd, BFFM_SETEXPANDED, FALSE, (LPARAM)pidl);
            ILFree(pidl);
        }
    }

    BrFolder_Callback(info->lpBrowseInfo, hWnd, BFFM_INITIALIZED, 0);

    SHAutoComplete(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT),
                   (SHACF_FILESYS_ONLY | SHACF_URLHISTORY | SHACF_FILESYSTEM));
    return TRUE;
}

static HRESULT
BrFolder_NewFolder(BrFolder *info)
{
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
        cur = desktop;
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
    BrItemData *item_data = (BrItemData *)item.lParam;
    if (!item_data)
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

    HTREEITEM hAdded = BrFolder_InsertItem(info, cur, pidlNew, item_data->pidlFull, hParent);
    TreeView_SortChildren(info->hwndTreeView, hParent, FALSE);
    return BrFolder_Rename(info, hAdded);
}

static void
BrFolder_OnOK(BrFolder *info)
{
    // Get the text
    WCHAR szPath[MAX_PATH];
    GetDlgItemTextW(info->hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT, szPath, _countof(szPath));
    StrTrimW(szPath, L" \t");

    // The original pidl is owned by the treeview and will be free'd.
    if (!PathIsRelativeW(szPath) && PathIsDirectoryW(szPath))
        info->pidlRet = ILCreateFromPathW(szPath); // It's valid path
    else
        info->pidlRet = ILClone(info->pidlRet);

    if (!info->pidlRet) // A null pidl would mean a cancel
        info->pidlRet = _ILCreateDesktop();

    pdump(info->pidlRet);

    LPBROWSEINFOW lpBrowseInfo = info->lpBrowseInfo;
    if (!lpBrowseInfo->pszDisplayName)
        return;

    SHFILEINFOW fileInfo = { NULL };
    lpBrowseInfo->pszDisplayName[0] = UNICODE_NULL;
    if (SHGetFileInfoW((LPCWSTR)info->pidlRet, 0, &fileInfo, sizeof(fileInfo),
                       SHGFI_PIDL | SHGFI_DISPLAYNAME))
    {
        lstrcpynW(lpBrowseInfo->pszDisplayName, fileInfo.szDisplayName, MAX_PATH);
    }
}

static void
BrFolder_OnCommand(BrFolder *info, UINT id)
{
    switch (id)
    {
        case IDOK:
        {
            BrFolder_OnOK(info);
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
            BrFolder_NewFolder(info);
            break;
        }
    }
}

static void
GetTreeViewItemContextMenuPos(HWND hWnd, HTREEITEM hItem, POINT *ppt)
{
    RECT rc;
    if (TreeView_GetItemRect(hWnd, hItem, &rc, TRUE))
    {
        ppt->x = (rc.left + rc.right) / 2;
        ppt->y = (rc.top + rc.bottom) / 2;
    }
    ClientToScreen(hWnd, ppt);
}

static void
BrFolder_OnContextMenu(BrFolder &info, LPARAM lParam)
{
    enum { IDC_TOGGLE = 1, ID_FIRSTCMD, ID_LASTCMD = 0xffff };
    HTREEITEM hSelected = TreeView_GetSelection(info.hwndTreeView);
    CMINVOKECOMMANDINFOEX ici = { sizeof(ici), CMIC_MASK_PTINVOKE, info.hWnd };
    ici.nShow = SW_SHOW;
    ici.ptInvoke.x = GET_X_LPARAM(lParam);
    ici.ptInvoke.y = GET_Y_LPARAM(lParam);
    if ((int)lParam == -1) // Keyboard
    {
        GetTreeViewItemContextMenuPos(info.hwndTreeView, hSelected, &ici.ptInvoke);
    }
    else // Get correct item for right-click on not current item
    {
        TVHITTESTINFO hti = { ici.ptInvoke };
        ScreenToClient(info.hwndTreeView, &hti.pt);
        hSelected = TreeView_HitTest(info.hwndTreeView, &hti);
    }
    BrItemData *item = BrFolder_GetItemData(&info, hSelected);
    if (!item)
        return; // Not on an item

    TV_ITEM tvi;
    tvi.mask = TVIF_STATE | TVIF_CHILDREN;
    tvi.stateMask = TVIS_EXPANDED;
    tvi.hItem = hSelected;
    TreeView_GetItem(info.hwndTreeView, &tvi);

    CComPtr<IContextMenu> pcm;
    HRESULT hr = item->lpsfParent->GetUIObjectOf(info.hWnd, 1, &item->pidlChild,
                                                 IID_IContextMenu, NULL, (void**)&pcm);
    if (FAILED(hr))
        return;

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu)
        return;
    info.pContextMenu = pcm;
    UINT cmf = ((GetKeyState(VK_SHIFT) < 0) ? CMF_EXTENDEDVERBS : 0) | CMF_CANRENAME;
    hr = pcm->QueryContextMenu(hMenu, 0, ID_FIRSTCMD, ID_LASTCMD, CMF_EXPLORE | cmf);
    if (hr > 0)
        _InsertMenuItemW(hMenu, 0, TRUE, 0, MFT_SEPARATOR, NULL, 0);
    _InsertMenuItemW(hMenu, 0, TRUE, IDC_TOGGLE, MFT_STRING,
        MAKEINTRESOURCEW((tvi.state & TVIS_EXPANDED) ? IDS_COLLAPSE : IDS_EXPAND),
        MFS_DEFAULT | (tvi.cChildren ? 0 : MFS_GRAYED));

    UINT cmd = TrackPopupMenuEx(hMenu, TPM_RETURNCMD, ici.ptInvoke.x, ici.ptInvoke.y, info.hWnd, NULL);
    ici.lpVerb = MAKEINTRESOURCEA(cmd - ID_FIRSTCMD);
    if (cmd == IDC_TOGGLE)
    {
        TreeView_SelectItem(info.hwndTreeView, hSelected);
        TreeView_Expand(info.hwndTreeView, hSelected, TVE_TOGGLE);
    }
    else if (cmd != 0 && GetDfmCmd(pcm, ici.lpVerb) == DFM_CMD_RENAME)
    {
        BrFolder_Rename(&info, hSelected);
    }
    else if (cmd != 0)
    {
        if (GetKeyState(VK_SHIFT) < 0)
            ici.fMask |= CMIC_MASK_SHIFT_DOWN;
        if (GetKeyState(VK_CONTROL) < 0)
            ici.fMask |= CMIC_MASK_CONTROL_DOWN;
        pcm->InvokeCommand((CMINVOKECOMMANDINFO*)&ici);
    }
    info.pContextMenu = NULL;
    DestroyMenu(hMenu);
}

static BOOL
BrFolder_ExpandToPidl(BrFolder *info, LPITEMIDLIST pidlSelection, HTREEITEM *phItem)
{
    LPITEMIDLIST pidlCurrent = pidlSelection;
    if (_ILIsDesktop(pidlSelection))
    {
        if (phItem)
            *phItem = TVI_ROOT;
        return TRUE;
    }

    // Initialize item to point to the first child of the root folder.
    TVITEMEXW item = { TVIF_PARAM };
    item.hItem = TreeView_GetRoot(info->hwndTreeView);
    if (item.hItem)
        item.hItem = TreeView_GetChild(info->hwndTreeView, item.hItem);

    // Walk the tree along the nodes corresponding to the remaining ITEMIDLIST
    UINT depth = _ILGetDepth(info->lpBrowseInfo->pidlRoot);
    while (item.hItem && pidlCurrent)
    {
        LPITEMIDLIST pidlNeedle = ILCloneToDepth(pidlSelection, ++depth);
        if (_ILIsEmpty(pidlNeedle))
        {
            ILFree(pidlNeedle);
            item.hItem = NULL; // Failure
            break;
        }
next:
        TreeView_GetItem(info->hwndTreeView, &item);
        const BrItemData *pItemData = (BrItemData *)item.lParam;
        if (ILIsEqual(pItemData->pidlFull, pidlNeedle))
        {
            BOOL done = _ILGetDepth(pidlSelection) == _ILGetDepth(pidlNeedle);
            if (done)
            {
                pidlCurrent = NULL; // Success
            }
            else
            {
                TreeView_Expand(info->hwndTreeView, item.hItem, TVE_EXPAND);
                item.hItem = TreeView_GetChild(info->hwndTreeView, item.hItem);
            }
        }
        else
        {
            item.hItem = TreeView_GetNextSibling(info->hwndTreeView, item.hItem);
            if (item.hItem)
                goto next;
        }
        ILFree(pidlNeedle);
    }

    if (phItem)
        *phItem = item.hItem;

    return (_ILIsEmpty(pidlCurrent) && item.hItem);
}

static BOOL
BrFolder_ExpandToString(BrFolder *info, LPWSTR pszString, HTREEITEM *phItem)
{
    CComHeapPtr<ITEMIDLIST_ABSOLUTE> pidlSelection;
    HRESULT hr = SHParseDisplayName(pszString, NULL, &pidlSelection, 0, NULL);
    return SUCCEEDED(hr) && BrFolder_ExpandToPidl(info, pidlSelection, phItem);
}

static BOOL
BrFolder_OnSetExpanded(BrFolder *info, LPITEMIDLIST pidlSelection, LPWSTR pszString)
{
    HTREEITEM hItem;
    BOOL ret;
    if (pszString)
        ret = BrFolder_ExpandToString(info, pszString, &hItem);
    else
        ret = BrFolder_ExpandToPidl(info, pidlSelection, &hItem);

    if (ret)
        TreeView_Expand(info->hwndTreeView, hItem, TVE_EXPAND);
    return ret;
}

static BOOL
BrFolder_OnSetSelectionPidl(BrFolder *info, LPITEMIDLIST pidlSelection)
{
    if (!pidlSelection)
        return FALSE;

    HTREEITEM hItem;
    BOOL ret = BrFolder_ExpandToPidl(info, pidlSelection, &hItem);
    if (ret)
        TreeView_SelectItem(info->hwndTreeView, hItem);
    return ret;
}

static BOOL
BrFolder_OnSetSelectionW(BrFolder *info, LPWSTR pszSelection)
{
    if (!pszSelection)
        return FALSE;

    HTREEITEM hItem;
    BOOL ret = BrFolder_ExpandToString(info, pszSelection, &hItem);
    if (ret)
        TreeView_SelectItem(info->hwndTreeView, hItem);
    return ret;
}

static BOOL
BrFolder_OnSetSelectionA(BrFolder *info, LPSTR pszSelectionA)
{
    if (!pszSelectionA)
        return FALSE;

    CComHeapPtr<WCHAR> pszSelectionW;
    __SHCloneStrAtoW(&pszSelectionW, pszSelectionA);
    if (!pszSelectionW)
        return FALSE;

    return BrFolder_OnSetSelectionW(info, pszSelectionW);
}

static void
BrFolder_OnDestroy(BrFolder *info)
{
    if (info->layout)
    {
        LayoutDestroy(info->layout);
        info->layout = NULL;
    }

    SHChangeNotifyDeregister(info->hChangeNotify);
}

static void
BrFolder_RefreshRecurse(
    _Inout_ BrFolder *info,
    _In_ HTREEITEM hTarget)
{
    // Get enum
    CComPtr<IEnumIDList> pEnum;
    BrItemData *pItemData = BrFolder_GetItemData(info, hTarget);
    HRESULT hrEnum = BrFolder_GetChildrenEnum(info, pItemData, &pEnum);

    // Insert new items
    if (SUCCEEDED(hrEnum))
    {
        CComHeapPtr<ITEMIDLIST_RELATIVE> pidlTemp;
        while (S_OK == pEnum->Next(1, &pidlTemp, NULL))
        {
            if (!BrFolder_TreeItemHasThisChild(info, hTarget, pidlTemp))
            {
                BrFolder_InsertItem(info, pItemData->lpsfParent, pidlTemp, pItemData->pidlFull,
                                    hTarget);
            }
            pidlTemp.Free();
        }
    }

    // Delete zombie items and update items
    HTREEITEM hItem, hNextItem;
    for (hItem = TreeView_GetChild(info->hwndTreeView, hTarget); hItem; hItem = hNextItem)
    {
        hNextItem = TreeView_GetNextSibling(info->hwndTreeView, hItem);

        if (FAILED(hrEnum) || !BrFolder_IsTreeItemInEnum(info, hItem, pEnum))
        {
            TreeView_DeleteItem(info->hwndTreeView, hItem);
            hNextItem = TreeView_GetChild(info->hwndTreeView, hTarget);
            continue;
        }

        BrFolder_UpdateItem(info, hItem);
        BrFolder_RefreshRecurse(info, hItem);
    }

    if (SUCCEEDED(hrEnum))
        TreeView_SortChildren(info->hwndTreeView, hTarget, FALSE);
}

static void
BrFolder_Refresh(_Inout_ BrFolder *info)
{
    HTREEITEM hRoot = TreeView_GetRoot(info->hwndTreeView);

    SendMessageW(info->hwndTreeView, WM_SETREDRAW, FALSE, 0);

    BrFolder_RefreshRecurse(info, hRoot);

    SendMessageW(info->hwndTreeView, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(info->hwndTreeView, NULL, TRUE);
}

static void
BrFolder_OnChangeEx(
    _Inout_ BrFolder *info,
    _In_ PCIDLIST_ABSOLUTE pidl0,
    _In_ PCIDLIST_ABSOLUTE pidl1,
    _In_ LONG event)
{
    TRACE("(%p)->(%p, %p, 0x%lX)\n", info, pidl0, pidl1, event);

    switch (event)
    {
        case SHCNE_DRIVEADD:
        case SHCNE_MKDIR:
        case SHCNE_CREATE:
        case SHCNE_DRIVEREMOVED:
        case SHCNE_RMDIR:
        case SHCNE_DELETE:
        case SHCNE_RENAMEFOLDER:
        case SHCNE_RENAMEITEM:
        case SHCNE_UPDATEDIR:
        case SHCNE_UPDATEITEM:
        {
            // FIXME: Avoid full refresh and optimize for speed. Use pidl0 and pidl1
            BrFolder_Refresh(info);
            break;
        }
    }
}

// SHV_CHANGE_NOTIFY
static void
BrFolder_OnChange(BrFolder *info, WPARAM wParam, LPARAM lParam)
{
    // We use SHCNRF_NewDelivery method
    HANDLE hChange = (HANDLE)wParam;
    DWORD dwProcID = (DWORD)lParam;

    PIDLIST_ABSOLUTE *ppidl = NULL;
    LONG event;
    HANDLE hLock = SHChangeNotification_Lock(hChange, dwProcID, &ppidl, &event);
    if (hLock == NULL)
    {
        ERR("hLock == NULL\n");
        return;
    }

    BrFolder_OnChangeEx(info, ppidl[0], ppidl[1], (event & ~SHCNE_INTERRUPT));

    SHChangeNotification_Unlock(hLock);
}

/*************************************************************************
 *             BrFolderDlgProc32  (not an exported API function)
 */
static INT_PTR CALLBACK
BrFolderDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
        return BrFolder_OnInitDialog(hWnd, (BrFolder *)lParam);

    BrFolder *info = (BrFolder *)GetWindowLongPtrW(hWnd, DWLP_USER);
    if (!info)
        return 0;

    if (info->pContextMenu)
    {
        LRESULT result;
        if (SHForwardContextMenuMsg(info->pContextMenu, uMsg, wParam,
                                    lParam, &result, TRUE) == S_OK)
        {
            SetWindowLongPtr(hWnd, DWLP_MSGRESULT, result);
            return TRUE;
        }
    }

    switch (uMsg)
    {
        case WM_NOTIFY:
            SetWindowLongPtr(hWnd, DWLP_MSGRESULT, BrFolder_OnNotify(info, (UINT)wParam, (LPNMHDR)lParam));
            return TRUE;

        case WM_COMMAND:
            BrFolder_OnCommand(info, wParam);
            break;

        case WM_CONTEXTMENU:
            if (info->hwndTreeView == (HWND)wParam)
                BrFolder_OnContextMenu(*info, lParam);
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
                return BrFolder_OnSetSelectionA(info, (LPSTR)lParam);
            else // PIDL
                return BrFolder_OnSetSelectionPidl(info, (LPITEMIDLIST)lParam);

        case BFFM_SETSELECTIONW:
            if (wParam) // String
                return BrFolder_OnSetSelectionW(info, (LPWSTR)lParam);
            else // PIDL
                return BrFolder_OnSetSelectionPidl(info, (LPITEMIDLIST)lParam);

        case BFFM_SETEXPANDED: // Unicode only
            if (wParam) // String
                return BrFolder_OnSetExpanded(info, NULL, (LPWSTR)lParam);
            else // PIDL
                return BrFolder_OnSetExpanded(info, (LPITEMIDLIST)lParam, NULL);

        case SHV_CHANGE_NOTIFY:
            BrFolder_OnChange(info, wParam, lParam);
            break;

        case WM_DESTROY:
            BrFolder_OnDestroy(info);
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

    // MSDN says the caller must initialize COM. We do it anyway in case the caller forgot.
    COleInit OleInit;
    BrFolder info = { lpbi };

    INT id = ((lpbi->ulFlags & BIF_USENEWUI) ? IDD_BROWSE_FOR_FOLDER_NEW : IDD_BROWSE_FOR_FOLDER);
    INT_PTR ret = DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(id), lpbi->hwndOwner,
                                  BrFolderDlgProc, (LPARAM)&info);
    if (ret == IDOK && !(lpbi->ulFlags & BIF_NOTRANSLATETARGETS) &&
        RosGetProcessEffectiveVersion() >= _WIN32_WINNT_WINXP)
    {
        PIDLIST_ABSOLUTE pidlTarget;
        if (SHELL_GetIDListTarget(info.pidlRet, &pidlTarget) == S_OK)
        {
            ILFree(info.pidlRet);
            info.pidlRet = pidlTarget;
        }
    }
    if (ret != IDOK)
    {
        ILFree(info.pidlRet);
        return NULL;
    }

    return info.pidlRet;
}
