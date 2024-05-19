/*
 * PROJECT:     ReactOS shell32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     SHBrowseForFolderA/W functions
 * COPYRIGHT:   Copyright 1999 Juergen Schmied
 *              Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

/*
 * FIXME:
 *  - many memory leaks
 *  - many flags unimplemented
 */

#include "precomp.h"

#include <ui/layout.h> /* Resizable window */

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define SHV_CHANGE_NOTIFY (WM_USER + 0x1111)

typedef struct tagbrowse_info
{
    HWND          hWnd;
    HWND          hwndTreeView;
    LPBROWSEINFOW lpBrowseInfo;
    LPITEMIDLIST  pidlRet;
    LAYOUT_DATA  *layout;  /* filled by LayoutInit, used by LayoutUpdate */
    SIZE          szMin;
    ULONG         hNotify; /* change notification handle */
} browse_info;

typedef struct tagTV_ITEMDATA
{
   LPSHELLFOLDER lpsfParent; /* IShellFolder of the parent */
   LPITEMIDLIST  lpi;        /* PIDL relative to parent */
   LPITEMIDLIST  lpifq;      /* Fully qualified PIDL */
   IEnumIDList*  pEnumIL;    /* Children iterator */ 
} TV_ITEMDATA, *LPTV_ITEMDATA;

static const LAYOUT_INFO g_layout_info[] =
{
    {IDC_BROWSE_FOR_FOLDER_TITLE,         BF_TOP|BF_LEFT|BF_RIGHT},
    {IDC_BROWSE_FOR_FOLDER_STATUS,        BF_TOP|BF_LEFT|BF_RIGHT},
    {IDC_BROWSE_FOR_FOLDER_TREEVIEW,      BF_TOP|BF_BOTTOM|BF_LEFT|BF_RIGHT},
    {IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT,   BF_TOP|BF_LEFT|BF_RIGHT},
    {IDC_BROWSE_FOR_FOLDER_NEW_FOLDER, BF_BOTTOM|BF_LEFT},
    {IDOK,              BF_BOTTOM|BF_RIGHT},
    {IDCANCEL,          BF_BOTTOM|BF_RIGHT}
};

#define LAYOUT_INFO_COUNT (sizeof(g_layout_info)/sizeof(g_layout_info[0]))

#define SUPPORTEDFLAGS (BIF_STATUSTEXT | \
                        BIF_BROWSEFORCOMPUTER | \
                        BIF_RETURNFSANCESTORS | \
                        BIF_RETURNONLYFSDIRS | \
                        BIF_NONEWFOLDERBUTTON | \
                        BIF_NEWDIALOGSTYLE | \
                        BIF_BROWSEINCLUDEFILES)

static void FillTreeView(browse_info*, LPSHELLFOLDER,
               LPITEMIDLIST, HTREEITEM, IEnumIDList*);
static HTREEITEM InsertTreeViewItem( browse_info*, IShellFolder *,
               LPCITEMIDLIST, LPCITEMIDLIST, IEnumIDList*, HTREEITEM);

static inline DWORD BrowseFlagsToSHCONTF(UINT ulFlags)
{
    return SHCONTF_FOLDERS | (ulFlags & BIF_BROWSEINCLUDEFILES ? SHCONTF_NONFOLDERS : 0);
}

static void browsefolder_callback( LPBROWSEINFOW lpBrowseInfo, HWND hWnd,
                                   UINT msg, LPARAM param )
{
    if (!lpBrowseInfo->lpfn)
        return;
    lpBrowseInfo->lpfn( hWnd, msg, param, lpBrowseInfo->lParam );
}

static LPTV_ITEMDATA
BrsFolder_GetDataFromItem(browse_info *info, HTREEITEM hItem)
{
    TVITEMW item = { TVIF_HANDLE | TVIF_PARAM };
    item.hItem = hItem;
    if (!TreeView_GetItem(info->hwndTreeView, &item))
        ERR("TreeView_GetItem failed\n");
    return (LPTV_ITEMDATA)item.lParam;
}

/******************************************************************************
 * InitializeTreeView [Internal]
 *
 * Called from WM_INITDIALOG handler.
 * 
 * PARAMS
 *  hwndParent [I] The BrowseForFolder dialog
 *  root       [I] ITEMIDLIST of the root shell folder
 */
static void InitializeTreeView( browse_info *info )
{
    LPITEMIDLIST pidlParent, pidlChild;
    HIMAGELIST hImageList;
    HRESULT hr;
    IShellFolder *lpsfParent, *lpsfRoot;
    IEnumIDList * pEnumChildren = NULL;
    HTREEITEM item;
    DWORD flags;
    LPCITEMIDLIST root = info->lpBrowseInfo->pidlRoot;

    TRACE("%p\n", info );
    
    Shell_GetImageLists(NULL, &hImageList);

    if (hImageList)
        TreeView_SetImageList(info->hwndTreeView, hImageList, 0);

    /* We want to call InsertTreeViewItem down the code, in order to insert
     * the root item of the treeview. Due to InsertTreeViewItem's signature, 
     * we need the following to do this:
     *
     * + An ITEMIDLIST corresponding to _the parent_ of root. 
     * + An ITEMIDLIST, which is a relative path from root's parent to root 
     *   (containing a single SHITEMID).
     * + An IShellFolder interface pointer of root's parent folder.
     *
     * If root is 'Desktop', then root's parent is also 'Desktop'.
     */

    pidlParent = ILClone(root);
    ILRemoveLastID(pidlParent);
    pidlChild = ILClone(ILFindLastID(root));
    
    if (_ILIsDesktop(pidlParent)) {
        hr = SHGetDesktopFolder(&lpsfParent);
    } else {
        IShellFolder *lpsfDesktop;
        hr = SHGetDesktopFolder(&lpsfDesktop);
        if (FAILED(hr)) {
            WARN("SHGetDesktopFolder failed! hr = %08x\n", hr);
            ILFree(pidlChild);
            ILFree(pidlParent);
            return;
        }
        hr = lpsfDesktop->BindToObject(pidlParent, 0, IID_PPV_ARG(IShellFolder, &lpsfParent));
        lpsfDesktop->Release();
    }

    if (FAILED(hr)) {
        WARN("Could not bind to parent shell folder! hr = %08x\n", hr);
        ILFree(pidlChild);
        ILFree(pidlParent);
        return;
    }

    if (!_ILIsEmpty(pidlChild)) {
        hr = lpsfParent->BindToObject(pidlChild, 0, IID_PPV_ARG(IShellFolder, &lpsfRoot));
    } else {
        lpsfRoot = lpsfParent;
        hr = lpsfParent->AddRef();
    }

    if (FAILED(hr)) {
        WARN("Could not bind to root shell folder! hr = %08x\n", hr);
        lpsfParent->Release();
        ILFree(pidlChild);
        ILFree(pidlParent);
        return;
    }

    flags = BrowseFlagsToSHCONTF( info->lpBrowseInfo->ulFlags );
    hr = lpsfRoot->EnumObjects(info->hWnd, flags, &pEnumChildren );
    if (FAILED(hr)) {
        WARN("Could not get child iterator! hr = %08x\n", hr);
        lpsfParent->Release();
        lpsfRoot->Release();
        ILFree(pidlChild);
        ILFree(pidlParent);
        return;
    }

    TreeView_DeleteItem(info->hwndTreeView, TVI_ROOT);
    item = InsertTreeViewItem( info, lpsfParent, pidlChild,
                               pidlParent, pEnumChildren, TVI_ROOT );
    TreeView_Expand(info->hwndTreeView, item, TVE_EXPAND);

    ILFree(pidlChild);
    ILFree(pidlParent);
    lpsfRoot->Release();
    lpsfParent->Release();
}

static int GetIcon(LPCITEMIDLIST lpi, UINT uFlags)
{
    SHFILEINFOW sfi;
    SHGetFileInfoW((LPCWSTR)lpi, 0 ,&sfi, sizeof(SHFILEINFOW), uFlags);
    return sfi.iIcon;
}

static void GetNormalAndSelectedIcons(LPITEMIDLIST lpifq, LPTVITEMW lpTV_ITEM)
{
    LPITEMIDLIST pidlDesktop = NULL;
    DWORD flags;

    TRACE("%p %p\n",lpifq, lpTV_ITEM);

    if (!lpifq)
    {
        pidlDesktop = _ILCreateDesktop();
        lpifq = pidlDesktop;
    }

    flags = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON;
    lpTV_ITEM->iImage = GetIcon( lpifq, flags );

    flags = SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON;
    lpTV_ITEM->iSelectedImage = GetIcon( lpifq, flags );

    if (pidlDesktop)
        ILFree( pidlDesktop );
}

/******************************************************************************
 * GetName [Internal]
 *
 * Query a shell folder for the display name of one of its children
 *
 * PARAMS
 *  lpsf           [I] IShellFolder interface of the folder to be queried.
 *  lpi            [I] ITEMIDLIST of the child, relative to parent
 *  dwFlags        [I] as in IShellFolder::GetDisplayNameOf
 *  lpFriendlyName [O] The desired display name in unicode
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
static BOOL GetName(LPSHELLFOLDER lpsf, LPCITEMIDLIST lpi, DWORD dwFlags, LPWSTR lpFriendlyName)
{
	BOOL   bSuccess=TRUE;
	STRRET str;

	TRACE("%p %p %x %p\n", lpsf, lpi, dwFlags, lpFriendlyName);
	if (SUCCEEDED(lpsf->GetDisplayNameOf(lpi, dwFlags, &str)))
          bSuccess = StrRetToStrNW(lpFriendlyName, MAX_PATH, &str, lpi);
	else
	  bSuccess = FALSE;

	TRACE("-- %s\n", debugstr_w(lpFriendlyName));
	return bSuccess;
}

/******************************************************************************
 * InsertTreeViewItem [Internal]
 *
 * PARAMS
 *  info       [I] data for the dialog
 *  lpsf       [I] IShellFolder interface of the item's parent shell folder 
 *  pidl       [I] ITEMIDLIST of the child to insert, relative to parent
 *  pidlParent [I] ITEMIDLIST of the parent shell folder
 *  pEnumIL    [I] Iterator for the children of the item to be inserted
 *  hParent    [I] The treeview-item that represents the parent shell folder
 *
 * RETURNS
 *  Success: Handle to the created and inserted treeview-item
 *  Failure: NULL
 */
static HTREEITEM InsertTreeViewItem( browse_info *info, IShellFolder * lpsf,
    LPCITEMIDLIST pidl, LPCITEMIDLIST pidlParent, IEnumIDList* pEnumIL,
    HTREEITEM hParent)
{
	TVITEMW 	tvi;
	TVINSERTSTRUCTW	tvins;
	WCHAR		szBuff[MAX_PATH];
	LPTV_ITEMDATA	lptvid=0;

	tvi.mask  = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

	tvi.cChildren= pEnumIL ? 1 : 0;
	tvi.mask |= TVIF_CHILDREN;

	if (!GetName(lpsf, pidl, SHGDN_NORMAL, szBuff))
	    return NULL;

	lptvid = (LPTV_ITEMDATA)SHAlloc(sizeof(TV_ITEMDATA));
	if (!lptvid)
	    return NULL;

	tvi.pszText    = szBuff;
	tvi.cchTextMax = MAX_PATH;
	tvi.lParam = (LPARAM)lptvid;

	lpsf->AddRef();
	lptvid->lpsfParent = lpsf;
	lptvid->lpi	= ILClone(pidl);
	lptvid->lpifq	= pidlParent ? ILCombine(pidlParent, pidl) : ILClone(pidl);
	lptvid->pEnumIL = pEnumIL;
	GetNormalAndSelectedIcons(lptvid->lpifq, &tvi);

	tvins.item       = tvi;
	tvins.hInsertAfter = NULL;
	tvins.hParent      = hParent;

	return TreeView_InsertItem( info->hwndTreeView, &tvins );
}

/******************************************************************************
 * FillTreeView [Internal]
 *
 * For each child (given by lpe) of the parent shell folder, which is given by 
 * lpsf and whose PIDL is pidl, insert a treeview-item right under hParent
 *
 * PARAMS
 *  info    [I] data for the dialog
 *  lpsf    [I] IShellFolder interface of the parent shell folder
 *  pidl    [I] ITEMIDLIST of the parent shell folder
 *  hParent [I] The treeview item that represents the parent shell folder
 *  lpe     [I] An iterator for the children of the parent shell folder
 */
static void FillTreeView( browse_info *info, IShellFolder * lpsf,
                 LPITEMIDLIST  pidl, HTREEITEM hParent, IEnumIDList* lpe)
{
	LPITEMIDLIST	pidlTemp = 0;
	ULONG		ulFetched;
	HRESULT		hr;
	HWND		hwnd = GetParent( info->hwndTreeView );

	TRACE("%p %p %p %p\n",lpsf, pidl, hParent, lpe);

	/* No IEnumIDList -> No children */
	if (!lpe) return;
	
	SetCapture( hwnd );
	SetCursor( LoadCursorA( 0, (LPSTR)IDC_WAIT ) );

	while (S_OK == lpe->Next(1, &pidlTemp, &ulFetched))
	{
	    ULONG ulAttrs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER;
	    IEnumIDList* pEnumIL = NULL;
	    IShellFolder* pSFChild = NULL;
	    lpsf->GetAttributesOf(1, (LPCITEMIDLIST*)&pidlTemp, &ulAttrs);
	    if (ulAttrs & SFGAO_FOLDER)
	    {
	        hr = lpsf->BindToObject(pidlTemp, NULL, IID_PPV_ARG(IShellFolder, &pSFChild));
	        if (SUCCEEDED(hr))
                {
	            DWORD flags = BrowseFlagsToSHCONTF(info->lpBrowseInfo->ulFlags);
	            hr = pSFChild->EnumObjects(hwnd, flags, &pEnumIL);
                    if (hr == S_OK)
                    {
                        if ((pEnumIL->Skip(1) != S_OK) ||
                             FAILED(pEnumIL->Reset()))
                        {
                            pEnumIL->Release();
                            pEnumIL = NULL;
                        }
                    }
                    pSFChild->Release();
                }
	    }
        if (ulAttrs != (ulAttrs & SFGAO_FOLDER))
        {
	        if (!InsertTreeViewItem(info, lpsf, pidlTemp, pidl, pEnumIL, hParent))
	            goto done;
	    }
	    SHFree(pidlTemp);  /* Finally, free the pidl that the shell gave us... */
	    pidlTemp=NULL;
	}

done:
	ReleaseCapture();
	SetCursor(LoadCursorW(0, (LPWSTR)IDC_ARROW));
    SHFree(pidlTemp);
}

static inline BOOL PIDLIsType(LPCITEMIDLIST pidl, PIDLTYPE type)
{
    LPPIDLDATA data = _ILGetDataPointer(pidl);
    if (!data)
        return FALSE;
    return (data->type == type);
}

static void BrsFolder_CheckValidSelection( browse_info *info, LPTV_ITEMDATA lptvid )
{
    LPBROWSEINFOW lpBrowseInfo = info->lpBrowseInfo;
    LPCITEMIDLIST pidl = lptvid->lpi;
    BOOL bEnabled = TRUE;
    DWORD dwAttributes;
    HRESULT r;

    if ((lpBrowseInfo->ulFlags & BIF_BROWSEFORCOMPUTER) &&
        !PIDLIsType(pidl, PT_COMP))
        bEnabled = FALSE;
    if (lpBrowseInfo->ulFlags & BIF_RETURNFSANCESTORS)
    {
        dwAttributes = SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM;
        r = lptvid->lpsfParent->GetAttributesOf(1, (LPCITEMIDLIST*)&lptvid->lpi, &dwAttributes);
        if (FAILED(r) || !(dwAttributes & (SFGAO_FILESYSANCESTOR|SFGAO_FILESYSTEM)))
            bEnabled = FALSE;
    }

    dwAttributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
    r = lptvid->lpsfParent->GetAttributesOf(1, (LPCITEMIDLIST*)&lptvid->lpi, &dwAttributes);
    if (FAILED(r) ||
            ((dwAttributes & (SFGAO_FOLDER|SFGAO_FILESYSTEM)) != (SFGAO_FOLDER|SFGAO_FILESYSTEM)))
    {
        if (lpBrowseInfo->ulFlags & BIF_RETURNONLYFSDIRS)
            bEnabled = FALSE;
        EnableWindow(GetDlgItem(info->hWnd, IDC_BROWSE_FOR_FOLDER_NEW_FOLDER), FALSE);
    }
    else
        EnableWindow(GetDlgItem(info->hWnd, IDC_BROWSE_FOR_FOLDER_NEW_FOLDER), TRUE);

    SendMessageW(info->hWnd, BFFM_ENABLEOK, 0, bEnabled);
}

static LRESULT BrsFolder_Treeview_Delete( browse_info *info, NMTREEVIEWW *pnmtv )
{
    LPTV_ITEMDATA lptvid = (LPTV_ITEMDATA)pnmtv->itemOld.lParam;

    TRACE("TVN_DELETEITEMA/W %p\n", lptvid);

    lptvid->lpsfParent->Release();
    if (lptvid->pEnumIL)
        lptvid->pEnumIL->Release();
    SHFree(lptvid->lpi);
    SHFree(lptvid->lpifq);
    SHFree(lptvid);
    return 0;
}

static LRESULT BrsFolder_Treeview_Expand( browse_info *info, NMTREEVIEWW *pnmtv )
{
    IShellFolder *lpsf2 = NULL;
    LPTV_ITEMDATA lptvid = (LPTV_ITEMDATA) pnmtv->itemNew.lParam;
    HRESULT r;

    TRACE("TVN_ITEMEXPANDINGA/W\n");

    if ((pnmtv->itemNew.state & TVIS_EXPANDEDONCE))
        return 0;

    if (!_ILIsEmpty(lptvid->lpi)) {
        r = lptvid->lpsfParent->BindToObject(lptvid->lpi, 0, IID_PPV_ARG(IShellFolder, &lpsf2));
    } else {
        lpsf2 = lptvid->lpsfParent;
        lpsf2->AddRef();
        r = S_OK;
    }

    if (SUCCEEDED(r))
    {
        FillTreeView( info, lpsf2, lptvid->lpifq, pnmtv->itemNew.hItem, lptvid->pEnumIL);
        lpsf2->Release();
    }

    /* My Computer is already sorted and trying to do a simple text
     * sort will only mess things up */
    if (!_ILIsMyComputer(lptvid->lpi))
        TreeView_SortChildren(info->hwndTreeView, pnmtv->itemNew.hItem, FALSE);

    return 0;
}

static HRESULT BrsFolder_Treeview_Changed( browse_info *info, NMTREEVIEWW *pnmtv )
{
    LPTV_ITEMDATA lptvid = (LPTV_ITEMDATA) pnmtv->itemNew.lParam;
    WCHAR name[MAX_PATH];

    ILFree(info->pidlRet);
    info->pidlRet = ILClone(lptvid->lpifq);

    if (GetName(lptvid->lpsfParent, lptvid->lpi, SHGDN_NORMAL, name))
            SetWindowTextW( GetDlgItem(info->hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT), name );

    browsefolder_callback( info->lpBrowseInfo, info->hWnd, BFFM_SELCHANGED,
                           (LPARAM)info->pidlRet );
    BrsFolder_CheckValidSelection( info, lptvid );
    return S_OK;
}

static LRESULT BrsFolder_Treeview_Rename(browse_info *info, NMTVDISPINFOW *pnmtv)
{
    LPTV_ITEMDATA item_data;
    WCHAR old_path[MAX_PATH], new_path[MAX_PATH], *p;
    NMTREEVIEWW nmtv;
    TVITEMW item;

    if(!pnmtv->item.pszText)
        return 0;

    item.hItem = TreeView_GetSelection(info->hwndTreeView);
    item_data = BrsFolder_GetDataFromItem(info, item.hItem);

    SHGetPathFromIDListW(item_data->lpifq, old_path);
    if(!(p = strrchrW(old_path, '\\')))
        return 0;
    p = new_path+(p-old_path+1);
    memcpy(new_path, old_path, (p-new_path)*sizeof(WCHAR));
    strcpyW(p, pnmtv->item.pszText);

    if(!MoveFileW(old_path, new_path))
        return 0;

    SHFree(item_data->lpifq);
    SHFree(item_data->lpi);
    item_data->lpifq = SHSimpleIDListFromPathW(new_path);
    item_data->lpsfParent->ParseDisplayName(NULL, NULL,
            pnmtv->item.pszText, NULL, &item_data->lpi, NULL);

    item.mask = TVIF_HANDLE | TVIF_TEXT;
    item.pszText = pnmtv->item.pszText;
    TreeView_SetItem(info->hwndTreeView, &item);

    nmtv.itemNew.lParam = item.lParam;
    BrsFolder_Treeview_Changed(info, &nmtv);
    return 0;
}

static HRESULT BrsFolder_Rename(browse_info *info, HTREEITEM hItem)
{
    TreeView_SelectItem(info->hwndTreeView, hItem);
    TreeView_EditLabel(info->hwndTreeView, hItem);
    return S_OK;
}

static void
BrsFolder_Delete(browse_info *info, HTREEITEM selected_item)
{
    TV_ITEMDATA *item_data;
    SHFILEOPSTRUCTW fileop = { info->hwndTreeView };
    WCHAR szzFrom[MAX_PATH + 1];

    /* get item_data */
    item_data = BrsFolder_GetDataFromItem(info, selected_item);

    /* get the path */
    if (!SHGetPathFromIDListW(item_data->lpifq, szzFrom))
    {
        ERR("SHGetPathFromIDListW failed\n");
        return;
    }
    szzFrom[lstrlenW(szzFrom) + 1] = 0; /* double NULL-terminated */
    fileop.pFrom = szzFrom;

    /* delete folder */
    fileop.fFlags = FOF_ALLOWUNDO;
    fileop.wFunc = FO_DELETE;
    SHFileOperationW(&fileop);
}

static LRESULT BrsFolder_Treeview_Keydown(browse_info *info, LPNMTVKEYDOWN keydown)
{
    HTREEITEM selected_item;

    /* Old dialog doesn't support those advanced features */
    if (!(info->lpBrowseInfo->ulFlags & BIF_USENEWUI))
        return 0;

    selected_item = TreeView_GetSelection(info->hwndTreeView);

    switch (keydown->wVKey)
    {
    case VK_F2:
        BrsFolder_Rename(info, selected_item);
        break;
    case VK_DELETE:
        BrsFolder_Delete(info, selected_item);
        break;
    }
    return 0;
}

static LRESULT BrsFolder_OnNotify( browse_info *info, UINT CtlID, LPNMHDR lpnmh )
{
    NMTREEVIEWW *pnmtv = (NMTREEVIEWW *)lpnmh;

    TRACE("%p %x %p msg=%x\n", info, CtlID, lpnmh, pnmtv->hdr.code);

    if (pnmtv->hdr.idFrom != IDC_BROWSE_FOR_FOLDER_TREEVIEW)
        return 0;

    switch (pnmtv->hdr.code)
    {
    case TVN_DELETEITEMA:
    case TVN_DELETEITEMW:
        return BrsFolder_Treeview_Delete( info, pnmtv );

    case TVN_ITEMEXPANDINGA:
    case TVN_ITEMEXPANDINGW:
        return BrsFolder_Treeview_Expand( info, pnmtv );

    case TVN_SELCHANGEDA:
    case TVN_SELCHANGEDW:
        return BrsFolder_Treeview_Changed( info, pnmtv );

    case TVN_ENDLABELEDITA:
    case TVN_ENDLABELEDITW:
        return BrsFolder_Treeview_Rename( info, (LPNMTVDISPINFOW)pnmtv );
    
    case TVN_KEYDOWN:
        return BrsFolder_Treeview_Keydown( info, (LPNMTVKEYDOWN)pnmtv );

    default:
        WARN("unhandled (%d)\n", pnmtv->hdr.code);
        break;
    }

    return 0;
}


static BOOL BrsFolder_OnCreate( HWND hWnd, browse_info *info )
{
    LPITEMIDLIST computer_pidl;
    SHChangeNotifyEntry ntreg;
    LPBROWSEINFOW lpBrowseInfo = info->lpBrowseInfo;

    info->hWnd = hWnd;
    SetPropW( hWnd, L"__WINE_BRSFOLDERDLG_INFO", info );

    if (lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE)
        FIXME("flags BIF_NEWDIALOGSTYLE partially implemented\n");
    if (lpBrowseInfo->ulFlags & ~SUPPORTEDFLAGS)
	FIXME("flags %x not implemented\n", lpBrowseInfo->ulFlags & ~SUPPORTEDFLAGS);

    if (lpBrowseInfo->ulFlags & BIF_USENEWUI)
    {
        RECT rcWnd;

        /* Resize the treeview if there's not editbox */
        if ((lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE)
            && !(lpBrowseInfo->ulFlags & BIF_EDITBOX))
        {
            RECT rcEdit, rcTreeView;
            INT cy;
            GetWindowRect(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT), &rcEdit);
            GetWindowRect(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_TREEVIEW), &rcTreeView);
            cy = rcTreeView.top - rcEdit.top;
            MapWindowPoints(NULL, hWnd, (LPPOINT)&rcTreeView, sizeof(RECT) / sizeof(POINT));
            rcTreeView.top -= cy;
            MoveWindow(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_TREEVIEW),
                       rcTreeView.left, rcTreeView.top,
                       rcTreeView.right - rcTreeView.left,
                       rcTreeView.bottom - rcTreeView.top, TRUE);
        }
        if (lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE)
            info->layout = LayoutInit(hWnd, g_layout_info, LAYOUT_INFO_COUNT);
        else
            info->layout = NULL;

        /* TODO: Windows allows shrinking the windows a bit */
        GetWindowRect(hWnd, &rcWnd);
        info->szMin.cx = rcWnd.right - rcWnd.left;
        info->szMin.cy = rcWnd.bottom - rcWnd.top;
    }
    else
    {
        info->layout = NULL;
    }

    if (lpBrowseInfo->lpszTitle)
	SetWindowTextW( GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_TITLE), lpBrowseInfo->lpszTitle );
    else
	ShowWindow( GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_TITLE), SW_HIDE );

    if (!(lpBrowseInfo->ulFlags & BIF_STATUSTEXT)
        || (lpBrowseInfo->ulFlags & BIF_USENEWUI))
	ShowWindow( GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_STATUS), SW_HIDE );

    /* Hide "Make New Folder" Button? */
    if ((lpBrowseInfo->ulFlags & BIF_NONEWFOLDERBUTTON)
        || !(lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE))
        ShowWindow( GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_NEW_FOLDER), SW_HIDE );

    /* Hide the editbox? */
    if (!(lpBrowseInfo->ulFlags & BIF_EDITBOX))
    {
        ShowWindow( GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER), SW_HIDE );
        ShowWindow( GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT), SW_HIDE );
    }

    info->hwndTreeView = GetDlgItem( hWnd, IDC_BROWSE_FOR_FOLDER_TREEVIEW );
    if (info->hwndTreeView)
    {
        InitializeTreeView( info );
    }
    else
        ERR("treeview control missing!\n");

    /* Register for change notifications */
    SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, &computer_pidl);

    ntreg.pidl = computer_pidl;
    ntreg.fRecursive = TRUE;

    info->hNotify = SHChangeNotifyRegister(hWnd, SHCNRF_InterruptLevel, SHCNE_ALLEVENTS, SHV_CHANGE_NOTIFY, 1, &ntreg);

    SetFocus(info->hwndTreeView);
    browsefolder_callback( info->lpBrowseInfo, hWnd, BFFM_INITIALIZED, 0 );

    SHAutoComplete(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT),
                   (SHACF_FILESYS_ONLY | SHACF_URLHISTORY | SHACF_FILESYSTEM));
    return TRUE;
}

static HRESULT BrsFolder_NewFolder(browse_info *info)
{
    DWORD flags = BrowseFlagsToSHCONTF(info->lpBrowseInfo->ulFlags);
    IShellFolder *desktop, *cur;
    WCHAR wszNewFolder[25];
    WCHAR path[MAX_PATH];
    WCHAR name[MAX_PATH];
    HTREEITEM hParent, hAdded;
    LPTV_ITEMDATA item_data;
    LPITEMIDLIST new_item;
    TVITEMW item;
    HRESULT hr;
    int len;

    hr = SHGetDesktopFolder(&desktop);
    if(FAILED(hr))
        return hr;

    if (info->pidlRet)
    {
        hr = desktop->BindToObject(info->pidlRet, 0, IID_PPV_ARG(IShellFolder, &cur));
        desktop->Release();
        if(FAILED(hr))
            return hr;

        hr = SHGetPathFromIDListW(info->pidlRet, path);
    }
    else
    {
        cur = desktop;
        hr = SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, path);
    }
    if(FAILED(hr))
        return hr;

    if (!LoadStringW(shell32_hInstance, IDS_NEWFOLDER, wszNewFolder, _countof(wszNewFolder)))
        return E_FAIL;

    if (!PathYetAnotherMakeUniqueName(name, path, NULL, wszNewFolder))
        return E_FAIL;

    len = strlenW(path);
    if(len<MAX_PATH && name[len] == L'\\')
        len++;

    hr = E_FAIL;
    if(!CreateDirectoryW(name, NULL))
        goto cleanup;

    /* Update parent of newly created directory */
    hParent = TreeView_GetSelection(info->hwndTreeView);
    if(!hParent)
        goto cleanup;

    TreeView_Expand(info->hwndTreeView, hParent, TVE_EXPAND);

    memset(&item, 0, sizeof(TVITEMW));
    item.mask = TVIF_PARAM|TVIF_STATE;
    item.hItem = hParent;
    TreeView_GetItem(info->hwndTreeView, &item);
    item_data = (LPTV_ITEMDATA)item.lParam;
    if(!item_data)
        goto cleanup;

    if(item_data->pEnumIL)
        item_data->pEnumIL->Release();
    hr = cur->EnumObjects(info->hwndTreeView, flags, &item_data->pEnumIL);
    if(FAILED(hr))
        goto cleanup;

    /* Update treeview */
    if(!(item.state&TVIS_EXPANDEDONCE)) {
        item.mask = TVIF_STATE;
        item.state = TVIS_EXPANDEDONCE;
        item.stateMask = TVIS_EXPANDEDONCE;
        TreeView_SetItem(info->hwndTreeView, &item);
    }

    hr = cur->ParseDisplayName(NULL, NULL, name+len, NULL, &new_item, NULL);
    if(FAILED(hr))
        goto cleanup;

    hAdded = InsertTreeViewItem(info, cur, new_item, item_data->lpifq, NULL, hParent);
    cur->Release();
    SHFree(new_item);

    TreeView_SortChildren(info->hwndTreeView, hParent, FALSE);
    return BrsFolder_Rename(info, hAdded);

cleanup:
    return hr;
}

static BOOL BrsFolder_OnCommand( browse_info *info, UINT id )
{
    LPBROWSEINFOW lpBrowseInfo = info->lpBrowseInfo;
    WCHAR szPath[MAX_PATH];

    switch (id)
    {
    case IDOK:
        /* Get the text */
        GetDlgItemTextW(info->hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT, szPath, _countof(szPath));
        StrTrimW(szPath, L" \t");

        /* The original pidl is owned by the treeview and will be free'd. */
        if (!PathIsRelativeW(szPath) && PathIsDirectoryW(szPath))
        {
            /* It's valid path */
            info->pidlRet = ILCreateFromPathW(szPath);
        }
        else
        {
            info->pidlRet = ILClone(info->pidlRet);
        }
        if (info->pidlRet == NULL) /* A null pidl would mean a cancel */
            info->pidlRet = _ILCreateDesktop();
        pdump( info->pidlRet );
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
        EndDialog( info->hWnd, 1 );
        return TRUE;

    case IDCANCEL:
        EndDialog( info->hWnd, 0 );
        return TRUE;

    case IDC_BROWSE_FOR_FOLDER_NEW_FOLDER:
        BrsFolder_NewFolder(info);
        return TRUE;
    }
    return FALSE;
}

static BOOL BrsFolder_OnSetExpanded(browse_info *info, LPVOID selection, 
    BOOL is_str, HTREEITEM *pItem)
{
    LPITEMIDLIST pidlSelection = (LPITEMIDLIST)selection;
    LPCITEMIDLIST pidlCurrent, pidlRoot;
    TVITEMEXW item;
    BOOL bResult = FALSE;

    memset(&item, 0, sizeof(item));

    /* If 'selection' is a string, convert to a Shell ID List. */ 
    if (is_str) {
        IShellFolder *psfDesktop;
        HRESULT hr;

        hr = SHGetDesktopFolder(&psfDesktop);
        if (FAILED(hr))
            goto done;

        hr = psfDesktop->ParseDisplayName(NULL, NULL, (LPWSTR)selection, NULL, &pidlSelection, NULL);
        psfDesktop->Release();
        if (FAILED(hr)) 
            goto done;
    }
    if (_ILIsDesktop(pidlSelection))
    {
        item.hItem = TVI_ROOT;
        bResult = TRUE;
        goto done;
    }

    /* Move pidlCurrent behind the SHITEMIDs in pidlSelection, which are the root of
     * the sub-tree currently displayed. */
    pidlRoot = info->lpBrowseInfo->pidlRoot;
    pidlCurrent = pidlSelection;
    while (!_ILIsEmpty(pidlRoot) && _ILIsEqualSimple(pidlRoot, pidlCurrent)) {
        pidlRoot = ILGetNext(pidlRoot);
        pidlCurrent = ILGetNext(pidlCurrent);
    }

    /* The given ID List is not part of the SHBrowseForFolder's current sub-tree. */
    if (!_ILIsEmpty(pidlRoot))
        goto done;

    /* Initialize item to point to the first child of the root folder. */
    item.mask = TVIF_PARAM;
    item.hItem = TreeView_GetRoot(info->hwndTreeView);

    if (item.hItem)
        item.hItem = TreeView_GetChild(info->hwndTreeView, item.hItem);

    /* Walk the tree along the nodes corresponding to the remaining ITEMIDLIST */
    while (item.hItem && !_ILIsEmpty(pidlCurrent)) {
        LPTV_ITEMDATA pItemData;

        TreeView_GetItem(info->hwndTreeView, &item);
        pItemData = (LPTV_ITEMDATA)item.lParam;

        if (_ILIsEqualSimple(pItemData->lpi, pidlCurrent)) {
            pidlCurrent = ILGetNext(pidlCurrent);
            if (!_ILIsEmpty(pidlCurrent)) {
                /* Only expand current node and move on to its first child,
                 * if we didn't already reach the last SHITEMID */
                TreeView_Expand(info->hwndTreeView, item.hItem, TVE_EXPAND);
                item.hItem = TreeView_GetChild(info->hwndTreeView, item.hItem);
            }
        } else {
            item.hItem = TreeView_GetNextSibling(info->hwndTreeView, item.hItem);
        }
    }

    if (_ILIsEmpty(pidlCurrent) && item.hItem) 
        bResult = TRUE;

done:
    if (pidlSelection && pidlSelection != selection)
        ILFree(pidlSelection);

    if (pItem) 
        *pItem = item.hItem;
    
    return bResult;
}

static BOOL BrsFolder_OnSetSelectionW(browse_info *info, LPVOID selection, BOOL is_str) {
    HTREEITEM hItem;
    BOOL bResult;

    if (!selection) return FALSE;

    bResult = BrsFolder_OnSetExpanded(info, selection, is_str, &hItem);
    if (bResult)
        TreeView_SelectItem(info->hwndTreeView, hItem);
    return bResult;
}

static BOOL BrsFolder_OnSetSelectionA(browse_info *info, LPVOID selection, BOOL is_str) {
    LPWSTR selectionW = NULL;
    BOOL result = FALSE;
    int length;
    
    if (!is_str)
        return BrsFolder_OnSetSelectionW(info, selection, is_str);

    if ((length = MultiByteToWideChar(CP_ACP, 0, (LPSTR)selection, -1, NULL, 0)) &&
        (selectionW = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, length * sizeof(WCHAR))) &&
        MultiByteToWideChar(CP_ACP, 0, (LPSTR)selection, -1, selectionW, length))
    {
        result = BrsFolder_OnSetSelectionW(info, selectionW, is_str);
    }

    HeapFree(GetProcessHeap(), 0, selectionW);
    return result;
}

static INT BrsFolder_OnDestroy(browse_info *info)
{
    if (info->layout)
    {
        LayoutDestroy(info->layout);
        info->layout = NULL;
    }

    SHChangeNotifyDeregister(info->hNotify);

    return 0;
}

/* Find a treeview node by recursively walking the treeview */
static HTREEITEM BrsFolder_FindItemByPidl(browse_info *info, LPCITEMIDLIST pidl, HTREEITEM hItem)
{
    TV_ITEMDATA *item_data;
    HRESULT hr;

    item_data = BrsFolder_GetDataFromItem(info, hItem);

    hr = item_data->lpsfParent->CompareIDs(0, item_data->lpifq, pidl);
    if(SUCCEEDED(hr) && !HRESULT_CODE(hr))
        return hItem;

    hItem = TreeView_GetChild(info->hwndTreeView, hItem);

    while (hItem)
    {
        HTREEITEM newItem = BrsFolder_FindItemByPidl(info, pidl, hItem);
        if (newItem)
            return newItem;
        hItem = TreeView_GetNextSibling(info->hwndTreeView, hItem);
    }
    return NULL;
}

static LRESULT BrsFolder_OnChange(browse_info *info, const LPCITEMIDLIST *pidls, LONG event)
{
    BOOL ret = TRUE;

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
    return ret;
}

/*************************************************************************
 *             BrsFolderDlgProc32  (not an exported API function)
 */
static INT_PTR CALLBACK BrsFolderDlgProc( HWND hWnd, UINT msg, WPARAM wParam,
				          LPARAM lParam )
{
    browse_info *info;

    TRACE("hwnd=%p msg=%04x 0x%08lx 0x%08lx\n", hWnd, msg, wParam, lParam );

    if (msg == WM_INITDIALOG)
        return BrsFolder_OnCreate( hWnd, (browse_info*) lParam );

    info = (browse_info*)GetPropW(hWnd, L"__WINE_BRSFOLDERDLG_INFO");
    if (!info)
        return 0;

    switch (msg)
    {
    case WM_NOTIFY:
        return BrsFolder_OnNotify( info, (UINT)wParam, (LPNMHDR)lParam);

    case WM_COMMAND:
        return BrsFolder_OnCommand( info, wParam );

    case WM_GETMINMAXINFO:
        ((LPMINMAXINFO)lParam)->ptMinTrackSize.x = info->szMin.cx;
        ((LPMINMAXINFO)lParam)->ptMinTrackSize.y = info->szMin.cy;
        return 0;

    case WM_SIZE:
        if (info->layout)  /* new style dialogs */
            LayoutUpdate(hWnd, info->layout, g_layout_info, LAYOUT_INFO_COUNT);
        return 0;

    case BFFM_SETSTATUSTEXTA:
        TRACE("Set status %s\n", debugstr_a((LPSTR)lParam));
        SetWindowTextA(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_STATUS), (LPSTR)lParam);
        break;

    case BFFM_SETSTATUSTEXTW:
        TRACE("Set status %s\n", debugstr_w((LPWSTR)lParam));
        SetWindowTextW(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_STATUS), (LPWSTR)lParam);
        break;

    case BFFM_ENABLEOK:
        TRACE("Enable %ld\n", lParam);
        EnableWindow(GetDlgItem(hWnd, 1), lParam != 0);
        break;

    case BFFM_SETOKTEXT: /* unicode only */
        TRACE("Set OK text %s\n", debugstr_w((LPWSTR)lParam));
        SetWindowTextW(GetDlgItem(hWnd, 1), (LPWSTR)lParam);
        break;

    case BFFM_SETSELECTIONA:
        return BrsFolder_OnSetSelectionA(info, (LPVOID)lParam, (BOOL)wParam);

    case BFFM_SETSELECTIONW:
        return BrsFolder_OnSetSelectionW(info, (LPVOID)lParam, (BOOL)wParam);

    case BFFM_SETEXPANDED: /* unicode only */
        return BrsFolder_OnSetExpanded(info, (LPVOID)lParam, (BOOL)wParam, NULL);

    case SHV_CHANGE_NOTIFY:
        return BrsFolder_OnChange(info, (const LPCITEMIDLIST*)wParam, (LONG)lParam);

    case WM_DESTROY:
        return BrsFolder_OnDestroy(info);
    }
    return FALSE;
}

/*************************************************************************
 * SHBrowseForFolderA [SHELL32.@]
 * SHBrowseForFolder  [SHELL32.@]
 */
EXTERN_C
LPITEMIDLIST WINAPI SHBrowseForFolderA (LPBROWSEINFOA lpbi)
{
    BROWSEINFOW bi;
    LPITEMIDLIST lpid;
    INT len;
    LPWSTR title;

    TRACE("%p\n", lpbi);

    bi.hwndOwner = lpbi->hwndOwner;
    bi.pidlRoot = lpbi->pidlRoot;
    if (lpbi->pszDisplayName)
        bi.pszDisplayName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    else
        bi.pszDisplayName = NULL;

    if (lpbi->lpszTitle)
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpbi->lpszTitle, -1, NULL, 0 );
        title = (LPWSTR)HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpbi->lpszTitle, -1, title, len );
    }
    else
        title = NULL;

    bi.lpszTitle = title;
    bi.ulFlags = lpbi->ulFlags;
    bi.lpfn = lpbi->lpfn;
    bi.lParam = lpbi->lParam;
    bi.iImage = lpbi->iImage;
    lpid = SHBrowseForFolderW( &bi );
    if (bi.pszDisplayName)
    {
        WideCharToMultiByte( CP_ACP, 0, bi.pszDisplayName, -1,
                             lpbi->pszDisplayName, MAX_PATH, 0, NULL);
        HeapFree( GetProcessHeap(), 0, bi.pszDisplayName );
    }
    HeapFree(GetProcessHeap(), 0, title);
    lpbi->iImage = bi.iImage;
    return lpid;
}

/*************************************************************************
 * SHBrowseForFolderW [SHELL32.@]
 */
EXTERN_C
LPITEMIDLIST WINAPI SHBrowseForFolderW(LPBROWSEINFOW lpbi)
{
    browse_info info = { NULL };
    info.lpBrowseInfo = lpbi;

    HRESULT hr = OleInitialize(NULL);

    INT id = ((lpbi->ulFlags & BIF_USENEWUI) ? IDD_BROWSE_FOR_FOLDER_NEW : IDD_BROWSE_FOR_FOLDER);
    INT_PTR ret = DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(id), lpbi->hwndOwner,
                                  BrsFolderDlgProc, (LPARAM)&info);
    if (SUCCEEDED(hr))
        OleUninitialize();

    if (!ret)
    {
        ILFree(info.pidlRet);
        return NULL;
    }

    return info.pidlRet;
}
