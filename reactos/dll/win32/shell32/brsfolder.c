/*
 * Copyright 1999 Juergen Schmied
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * FIXME:
 *  - many memory leaks
 *  - many flags unimplemented
 *    - implement new dialog style "make new folder" button
 *    - implement editbox
 *    - implement new dialog style resizing
 */

#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "wine/debug.h"
#include "undocshell.h"
#include "pidl.h"
#include "shell32_main.h"
#include "shellapi.h"
#include "shresdef.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

typedef struct tagbrowse_info
{
    HWND          hWnd;
    HWND          hwndTreeView;
    LPBROWSEINFOW lpBrowseInfo;
    LPITEMIDLIST  pidlRet;
} browse_info;

typedef struct tagTV_ITEMDATA
{
   LPSHELLFOLDER lpsfParent; /* IShellFolder of the parent */
   LPITEMIDLIST  lpi;        /* PIDL relativ to parent */
   LPITEMIDLIST  lpifq;      /* Fully qualified PIDL */
   IEnumIDList*  pEnumIL;    /* Children iterator */ 
} TV_ITEMDATA, *LPTV_ITEMDATA;

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

static const WCHAR szBrowseFolderInfo[] = {
    '_','_','W','I','N','E','_',
    'B','R','S','F','O','L','D','E','R','D','L','G','_',
    'I','N','F','O',0
};

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
    
    Shell_GetImageList(NULL, &hImageList);

    if (hImageList)
        SendMessageW( info->hwndTreeView, TVM_SETIMAGELIST, 0, (LPARAM)hImageList );

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
        if (!SUCCEEDED(hr)) {
            WARN("SHGetDesktopFolder failed! hr = %08x\n", hr);
            return;
        }
        hr = IShellFolder_BindToObject(lpsfDesktop, pidlParent, 0, &IID_IShellFolder, (LPVOID*)&lpsfParent);
        IShellFolder_Release(lpsfDesktop);
    }
    
    if (!SUCCEEDED(hr)) {
        WARN("Could not bind to parent shell folder! hr = %08x\n", hr);
        return;
    }

    if (pidlChild && pidlChild->mkid.cb) {
        hr = IShellFolder_BindToObject(lpsfParent, pidlChild, 0, &IID_IShellFolder, (LPVOID*)&lpsfRoot);
    } else {
        lpsfRoot = lpsfParent;
        hr = IShellFolder_AddRef(lpsfParent);
    }
    
    if (!SUCCEEDED(hr)) {
        WARN("Could not bind to root shell folder! hr = %08x\n", hr);
        IShellFolder_Release(lpsfParent);
        return;
    }

    flags = BrowseFlagsToSHCONTF( info->lpBrowseInfo->ulFlags );
    hr = IShellFolder_EnumObjects( lpsfRoot, info->hWnd, flags, &pEnumChildren );
    if (!SUCCEEDED(hr)) {
        WARN("Could not get child iterator! hr = %08x\n", hr);
        IShellFolder_Release(lpsfParent);
        IShellFolder_Release(lpsfRoot);
        return;
    }

    SendMessageW( info->hwndTreeView, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT );
    item = InsertTreeViewItem( info, lpsfParent, pidlChild,
                               pidlParent, pEnumChildren, TVI_ROOT );
    SendMessageW( info->hwndTreeView, TVM_EXPAND, TVE_EXPAND, (LPARAM)item );

    IShellFolder_Release(lpsfRoot);
    IShellFolder_Release(lpsfParent);
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
 * Query a shell folder for the display name of one of it's children
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
	if (SUCCEEDED(IShellFolder_GetDisplayNameOf(lpsf, lpi, dwFlags, &str)))
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
 *  pidl       [I] ITEMIDLIST of the child to insert, relativ to parent 
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

	lptvid = SHAlloc( sizeof(TV_ITEMDATA) );
	if (!lptvid)
	    return NULL;

	if (!GetName(lpsf, pidl, SHGDN_NORMAL, szBuff))
	    return NULL;

	tvi.pszText    = szBuff;
	tvi.cchTextMax = MAX_PATH;
	tvi.lParam = (LPARAM)lptvid;

	IShellFolder_AddRef(lpsf);
	lptvid->lpsfParent = lpsf;
	lptvid->lpi	= ILClone(pidl);
	lptvid->lpifq	= pidlParent ? ILCombine(pidlParent, pidl) : ILClone(pidl);
	lptvid->pEnumIL = pEnumIL;
	GetNormalAndSelectedIcons(lptvid->lpifq, &tvi);

	tvins.u.item       = tvi;
	tvins.hInsertAfter = NULL;
	tvins.hParent      = hParent;

	return (HTREEITEM)TreeView_InsertItemW( info->hwndTreeView, &tvins );
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
	HTREEITEM	hPrev = 0;
	LPITEMIDLIST	pidlTemp = 0;
	ULONG		ulFetched;
	HRESULT		hr;
	HWND		hwnd = GetParent( info->hwndTreeView );

	TRACE("%p %p %p %p\n",lpsf, pidl, hParent, lpe);

	/* No IEnumIDList -> No children */
	if (!lpe) return;
	
	SetCapture( hwnd );
	SetCursor( LoadCursorA( 0, (LPSTR)IDC_WAIT ) );

	while (NOERROR == IEnumIDList_Next(lpe,1,&pidlTemp,&ulFetched))
	{
	    ULONG ulAttrs = SFGAO_HASSUBFOLDER | SFGAO_FOLDER;
	    IEnumIDList* pEnumIL = NULL;
	    IShellFolder* pSFChild = NULL;
	    IShellFolder_GetAttributesOf(lpsf, 1, (LPCITEMIDLIST*)&pidlTemp, &ulAttrs);
	    if (ulAttrs & SFGAO_FOLDER)
	    {
	        hr = IShellFolder_BindToObject(lpsf,pidlTemp,NULL,&IID_IShellFolder,(LPVOID*)&pSFChild);
	        if (SUCCEEDED(hr))
                {
	            DWORD flags = BrowseFlagsToSHCONTF(info->lpBrowseInfo->ulFlags);
	            hr = IShellFolder_EnumObjects(pSFChild, hwnd, flags, &pEnumIL);
                    if (hr == S_OK)
                    {
                        if ((IEnumIDList_Skip(pEnumIL, 1) != S_OK) ||
                             FAILED(IEnumIDList_Reset(pEnumIL)))
                        {
                            IEnumIDList_Release(pEnumIL);
                            pEnumIL = NULL;
                        }
                    }
                    IShellFolder_Release(pSFChild);
                }
	    }

	    if (!(hPrev = InsertTreeViewItem(info, lpsf, pidlTemp, pidl, pEnumIL, hParent)))
	        goto done;
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
        r = IShellFolder_GetAttributesOf(lptvid->lpsfParent, 1,
                                (LPCITEMIDLIST*)&lptvid->lpi, &dwAttributes);
        if (FAILED(r) || !(dwAttributes & (SFGAO_FILESYSANCESTOR|SFGAO_FILESYSTEM)))
            bEnabled = FALSE;
    }
    if (lpBrowseInfo->ulFlags & BIF_RETURNONLYFSDIRS)
    {
        dwAttributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
        r = IShellFolder_GetAttributesOf(lptvid->lpsfParent, 1,
                                (LPCITEMIDLIST*)&lptvid->lpi, &dwAttributes);
        if (FAILED(r) || 
            ((dwAttributes & (SFGAO_FOLDER|SFGAO_FILESYSTEM)) != (SFGAO_FOLDER|SFGAO_FILESYSTEM)))
        {
            bEnabled = FALSE;
        }
    }
    SendMessageW(info->hWnd, BFFM_ENABLEOK, 0, (LPARAM)bEnabled);
}

static LRESULT BrsFolder_Treeview_Delete( browse_info *info, NMTREEVIEWW *pnmtv )
{
    LPTV_ITEMDATA lptvid = (LPTV_ITEMDATA)pnmtv->itemOld.lParam;

    TRACE("TVN_DELETEITEMA/W %p\n", lptvid);

    IShellFolder_Release(lptvid->lpsfParent);
    if (lptvid->pEnumIL)
        IEnumIDList_Release(lptvid->pEnumIL);
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

    if (lptvid->lpi && lptvid->lpi->mkid.cb) {
        r = IShellFolder_BindToObject( lptvid->lpsfParent, lptvid->lpi, 0,
                                      (REFIID)&IID_IShellFolder, (LPVOID *)&lpsf2 );
    } else {
        lpsf2 = lptvid->lpsfParent;
        r = IShellFolder_AddRef(lpsf2);
    }

    if (SUCCEEDED(r))
        FillTreeView( info, lpsf2, lptvid->lpifq, pnmtv->itemNew.hItem, lptvid->pEnumIL);

    /* My Computer is already sorted and trying to do a simple text
     * sort will only mess things up */
    if (!_ILIsMyComputer(lptvid->lpi))
        SendMessageW( info->hwndTreeView, TVM_SORTCHILDREN,
                      FALSE, (LPARAM)pnmtv->itemNew.hItem );

    return 0;
}

static HRESULT BrsFolder_Treeview_Changed( browse_info *info, NMTREEVIEWW *pnmtv )
{
    LPTV_ITEMDATA lptvid = (LPTV_ITEMDATA) pnmtv->itemNew.lParam;

    lptvid = (LPTV_ITEMDATA) pnmtv->itemNew.lParam;
    info->pidlRet = lptvid->lpifq;
    browsefolder_callback( info->lpBrowseInfo, info->hWnd, BFFM_SELCHANGED,
                           (LPARAM)info->pidlRet );
    BrsFolder_CheckValidSelection( info, lptvid );
    return 0;
}

static LRESULT BrsFolder_OnNotify( browse_info *info, UINT CtlID, LPNMHDR lpnmh )
{
    NMTREEVIEWW *pnmtv = (NMTREEVIEWW *)lpnmh;

    TRACE("%p %x %p msg=%x\n", info, CtlID, lpnmh, pnmtv->hdr.code);

    if (pnmtv->hdr.idFrom != IDD_TREEVIEW)
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

    default:
        WARN("unhandled (%d)\n", pnmtv->hdr.code);
        break;
    }

    return 0;
}


static BOOL BrsFolder_OnCreate( HWND hWnd, browse_info *info )
{
    LPBROWSEINFOW lpBrowseInfo = info->lpBrowseInfo;

    info->hWnd = hWnd;
    SetPropW( hWnd, szBrowseFolderInfo, info );

    if (lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE)
        FIXME("flags BIF_NEWDIALOGSTYLE partially implemented\n");
    if (lpBrowseInfo->ulFlags & ~SUPPORTEDFLAGS)
	FIXME("flags %x not implemented\n", lpBrowseInfo->ulFlags & ~SUPPORTEDFLAGS);

    if (lpBrowseInfo->lpszTitle)
	SetWindowTextW( GetDlgItem(hWnd, IDD_TITLE), lpBrowseInfo->lpszTitle );
    else
	ShowWindow( GetDlgItem(hWnd, IDD_TITLE), SW_HIDE );

    if (!(lpBrowseInfo->ulFlags & BIF_STATUSTEXT)
        || (lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE))
	ShowWindow( GetDlgItem(hWnd, IDD_STATUS), SW_HIDE );

    /* Hide "Make New Folder" Button? */
    if ((lpBrowseInfo->ulFlags & BIF_NONEWFOLDERBUTTON)
        || !(lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE))
        ShowWindow( GetDlgItem(hWnd, IDD_MAKENEWFOLDER), SW_HIDE );

    /* Hide the editbox? */
    if (!(lpBrowseInfo->ulFlags & BIF_EDITBOX))
    {
        ShowWindow( GetDlgItem(hWnd, IDD_FOLDER), SW_HIDE );
        ShowWindow( GetDlgItem(hWnd, IDD_FOLDERTEXT), SW_HIDE );
    }

    info->hwndTreeView = GetDlgItem( hWnd, IDD_TREEVIEW );
    if (info->hwndTreeView)
    {
        InitializeTreeView( info );

        /* Resize the treeview if there's not editbox */
        if ((lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE)
            && !(lpBrowseInfo->ulFlags & BIF_EDITBOX))
        {
            RECT rc;
            GetClientRect(info->hwndTreeView, &rc);
            SetWindowPos(info->hwndTreeView, HWND_TOP, 0, 0,
                         rc.right, rc.bottom + 40, SWP_NOMOVE);
        }
    }
    else
        ERR("treeview control missing!\n");

    browsefolder_callback( info->lpBrowseInfo, hWnd, BFFM_INITIALIZED, 0 );

    return TRUE;
}

static BOOL BrsFolder_OnCommand( browse_info *info, UINT id )
{
    LPBROWSEINFOW lpBrowseInfo = info->lpBrowseInfo;

    switch (id)
    {
    case IDOK:
        /* The original pidl is owned by the treeview and will be free'd. */
        info->pidlRet = ILClone(info->pidlRet);
        if (info->pidlRet == NULL) /* A null pidl would mean a cancel */
            info->pidlRet = _ILCreateDesktop();
        pdump( info->pidlRet );
        if (lpBrowseInfo->pszDisplayName)
            SHGetPathFromIDListW( info->pidlRet, lpBrowseInfo->pszDisplayName );
        EndDialog( info->hWnd, 1 );
        return TRUE;

    case IDCANCEL:
        EndDialog( info->hWnd, 0 );
        return TRUE;

    case IDD_MAKENEWFOLDER:
        FIXME("make new folder not implemented\n");
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
    
    /* If 'selection' is a string, convert to a Shell ID List. */ 
    if (is_str) {
        IShellFolder *psfDesktop;
        HRESULT hr;

        hr = SHGetDesktopFolder(&psfDesktop);
        if (FAILED(hr))
            goto done;

        hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, 
                     (LPOLESTR)selection, NULL, &pidlSelection, NULL);
        IShellFolder_Release(psfDesktop);
        if (FAILED(hr)) 
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
    memset(&item, 0, sizeof(item));
    item.mask = TVIF_PARAM;
    item.hItem = TreeView_GetRoot(info->hwndTreeView);
    if (item.hItem) 
        item.hItem = TreeView_GetChild(info->hwndTreeView, item.hItem);

    /* Walk the tree along the nodes corresponding to the remaining ITEMIDLIST */
    while (item.hItem && !_ILIsEmpty(pidlCurrent)) {
        LPTV_ITEMDATA pItemData;

        SendMessageW(info->hwndTreeView, TVM_GETITEMW, 0, (LPARAM)&item);
        pItemData = (LPTV_ITEMDATA)item.lParam;

        if (_ILIsEqualSimple(pItemData->lpi, pidlCurrent)) {
            pidlCurrent = ILGetNext(pidlCurrent);
            if (!_ILIsEmpty(pidlCurrent)) {
                /* Only expand current node and move on to it's first child,
                 * if we didn't already reach the last SHITEMID */
                SendMessageW(info->hwndTreeView, TVM_EXPAND, TVE_EXPAND, (LPARAM)item.hItem);
                item.hItem = TreeView_GetChild(info->hwndTreeView, item.hItem);
            }
        } else {
            item.hItem = TreeView_GetNextSibling(info->hwndTreeView, item.hItem);
        }
    }

    if (_ILIsEmpty(pidlCurrent) && item.hItem) 
        bResult = TRUE;

done:
    if (pidlSelection && pidlSelection != (LPITEMIDLIST)selection)
        ILFree(pidlSelection);

    if (pItem) 
        *pItem = item.hItem;
    
    return bResult;
}

static BOOL BrsFolder_OnSetSelectionW(browse_info *info, LPVOID selection, BOOL is_str) {
    HTREEITEM hItem;
    BOOL bResult;

    bResult = BrsFolder_OnSetExpanded(info, selection, is_str, &hItem);
    if (bResult)
        SendMessageW(info->hwndTreeView, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hItem );
    return bResult;
}

static BOOL BrsFolder_OnSetSelectionA(browse_info *info, LPVOID selection, BOOL is_str) {
    LPWSTR selectionW = NULL;
    BOOL result = FALSE;
    int length;
    
    if (!is_str)
        return BrsFolder_OnSetSelectionW(info, selection, is_str);
    
    if ((length = MultiByteToWideChar(CP_ACP, 0, (LPCSTR)selection, -1, NULL, 0)) &&
        (selectionW = HeapAlloc(GetProcessHeap(), 0, length * sizeof(WCHAR))) &&
        MultiByteToWideChar(CP_ACP, 0, (LPCSTR)selection, -1, selectionW, length))
    {
        result = BrsFolder_OnSetSelectionW(info, selectionW, is_str);
    }

    HeapFree(GetProcessHeap(), 0, selectionW);
    return result;
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

    info = (browse_info*) GetPropW( hWnd, szBrowseFolderInfo );

    switch (msg)
    {
    case WM_NOTIFY:
        return BrsFolder_OnNotify( info, (UINT)wParam, (LPNMHDR)lParam);

    case WM_COMMAND:
        return BrsFolder_OnCommand( info, wParam );

    case BFFM_SETSTATUSTEXTA:
        TRACE("Set status %s\n", debugstr_a((LPSTR)lParam));
        SetWindowTextA(GetDlgItem(hWnd, IDD_STATUS), (LPSTR)lParam);
        break;

    case BFFM_SETSTATUSTEXTW:
        TRACE("Set status %s\n", debugstr_w((LPWSTR)lParam));
        SetWindowTextW(GetDlgItem(hWnd, IDD_STATUS), (LPWSTR)lParam);
        break;

    case BFFM_ENABLEOK:
        TRACE("Enable %ld\n", lParam);
        EnableWindow(GetDlgItem(hWnd, 1), (lParam)?TRUE:FALSE);
        break;

    case BFFM_SETOKTEXT: /* unicode only */
        TRACE("Set OK text %s\n", debugstr_w((LPWSTR)wParam));
        SetWindowTextW(GetDlgItem(hWnd, 1), (LPWSTR)wParam);
        break;

    case BFFM_SETSELECTIONA:
        return BrsFolder_OnSetSelectionA(info, (LPVOID)lParam, (BOOL)wParam);

    case BFFM_SETSELECTIONW:
        return BrsFolder_OnSetSelectionW(info, (LPVOID)lParam, (BOOL)wParam);

    case BFFM_SETEXPANDED: /* unicode only */
        return BrsFolder_OnSetExpanded(info, (LPVOID)lParam, (BOOL)wParam, NULL);
    }
    return FALSE;
}

static const WCHAR swBrowseTemplateName[] = {
    'S','H','B','R','S','F','O','R','F','O','L','D','E','R','_','M','S','G','B','O','X',0};
static const WCHAR swNewBrowseTemplateName[] = {
    'S','H','N','E','W','B','R','S','F','O','R','F','O','L','D','E','R','_','M','S','G','B','O','X',0};

/*************************************************************************
 * SHBrowseForFolderA [SHELL32.@]
 * SHBrowseForFolder  [SHELL32.@]
 */
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
    {
        bi.pszDisplayName = HeapAlloc( GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpbi->pszDisplayName, -1, bi.pszDisplayName, MAX_PATH );
    }
    else
        bi.pszDisplayName = NULL;

    if (lpbi->lpszTitle)
    {
        len = MultiByteToWideChar( CP_ACP, 0, lpbi->lpszTitle, -1, NULL, 0 );
        title = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
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
 *
 * NOTES
 *  crashes when passed a null pointer
 */
LPITEMIDLIST WINAPI SHBrowseForFolderW (LPBROWSEINFOW lpbi)
{
    browse_info info;
    DWORD r;
    HRESULT hr;
    const WCHAR * templateName;

    info.hWnd = 0;
    info.pidlRet = NULL;
    info.lpBrowseInfo = lpbi;
    info.hwndTreeView = NULL;

    hr = OleInitialize(NULL);

    if (lpbi->ulFlags & BIF_NEWDIALOGSTYLE)
        templateName = swNewBrowseTemplateName;
    else
        templateName = swBrowseTemplateName;
    r = DialogBoxParamW( shell32_hInstance, templateName, lpbi->hwndOwner,
	                 BrsFolderDlgProc, (LPARAM)&info );
    if (SUCCEEDED(hr)) 
        OleUninitialize();
    if (!r)
        return NULL;

    return info.pidlRet;
}
