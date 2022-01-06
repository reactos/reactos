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
 *    - implement editbox
 */

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <shlobj.h>
#include <undocshell.h>
#include <shellapi.h>
#include <wine/debug.h>
#include <wine/unicode.h>

#include "pidl.h"
#include "shell32_main.h"
#include "shresdef.h"
#ifdef __REACTOS__
    #include <shlwapi.h>
    #include "ui/layout.h" /* Resizable window */
#endif

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define SHV_CHANGE_NOTIFY (WM_USER + 0x1111)

#ifndef __REACTOS__ /* Defined in "layout.h" */
/* original margins and control size */
typedef struct tagLAYOUT_DATA
{
    LONG left, width, right;
    LONG top, height, bottom;
} LAYOUT_DATA;
#endif

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

#ifndef __REACTOS__ /* Defined in "layout.h" */
typedef struct tagLAYOUT_INFO
{
    int iItemId;          /* control id */
    DWORD dwAnchor;       /* BF_* flags specifying which margins should remain constant */
} LAYOUT_INFO;
#endif

static const LAYOUT_INFO g_layout_info[] =
{
    {IDC_BROWSE_FOR_FOLDER_TITLE,         BF_TOP|BF_LEFT|BF_RIGHT},
    {IDC_BROWSE_FOR_FOLDER_STATUS,        BF_TOP|BF_LEFT|BF_RIGHT},
#ifndef __REACTOS__ /* Duplicated */
    {IDC_BROWSE_FOR_FOLDER_FOLDER,        BF_TOP|BF_LEFT|BF_RIGHT},
#endif
    {IDC_BROWSE_FOR_FOLDER_TREEVIEW,      BF_TOP|BF_BOTTOM|BF_LEFT|BF_RIGHT},
#ifdef __REACTOS__
    {IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT,   BF_TOP|BF_LEFT|BF_RIGHT},
#else
    {IDC_BROWSE_FOR_FOLDER_FOLDER,        BF_BOTTOM|BF_LEFT},
    {IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT,    BF_BOTTOM|BF_LEFT|BF_RIGHT},
#endif
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

#ifndef __REACTOS__ /* Defined in "layout.h" */
static LAYOUT_DATA *LayoutInit(HWND hwnd, const LAYOUT_INFO *layout_info, int layout_count)
{
    LAYOUT_DATA *data;
    RECT rcWnd;
    int i;

    GetClientRect(hwnd, &rcWnd);
    data = SHAlloc(sizeof(LAYOUT_DATA)*layout_count);
    for (i = 0; i < layout_count; i++)
    {
        RECT r;
        HWND hItem = GetDlgItem(hwnd, layout_info[i].iItemId);

        if (hItem == NULL)
            ERR("Item %d not found\n", i);
        GetWindowRect(hItem, &r);
        MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&r, 2);

        data[i].left = r.left;
        data[i].right = rcWnd.right - r.right;
        data[i].width = r.right - r.left;

        data[i].top = r.top;
        data[i].bottom = rcWnd.bottom - r.bottom;
        data[i].height = r.bottom - r.top;
    }
    return data;
}

static void LayoutUpdate(HWND hwnd, LAYOUT_DATA *data, const LAYOUT_INFO *layout_info, int layout_count)
{
    RECT rcWnd;
    int i;

    GetClientRect(hwnd, &rcWnd);
    for (i = 0; i < layout_count; i++)
    {
        RECT r;
        HWND hItem = GetDlgItem(hwnd, layout_info[i].iItemId);

        GetWindowRect(hItem, &r);
        MapWindowPoints(HWND_DESKTOP, hwnd, (LPPOINT)&r, 2);

        if (layout_info[i].dwAnchor & BF_RIGHT)
        {
            r.right = rcWnd.right - data[i].right;
            if (!(layout_info[i].dwAnchor & BF_LEFT))
                r.left = r.right - data[i].width;
        }

        if (layout_info[i].dwAnchor & BF_BOTTOM)
        {
            r.bottom = rcWnd.bottom - data[i].bottom;
            if (!(layout_info[i].dwAnchor & BF_TOP))
                r.top = r.bottom - data[i].height;
        }

        SetWindowPos(hItem, NULL, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER);
    }
}
#endif


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
        if (FAILED(hr)) {
            WARN("SHGetDesktopFolder failed! hr = %08x\n", hr);
            ILFree(pidlChild);
            ILFree(pidlParent);
            return;
        }
        hr = IShellFolder_BindToObject(lpsfDesktop, pidlParent, 0, &IID_IShellFolder, (LPVOID*)&lpsfParent);
        IShellFolder_Release(lpsfDesktop);
    }

    if (FAILED(hr)) {
        WARN("Could not bind to parent shell folder! hr = %08x\n", hr);
        ILFree(pidlChild);
        ILFree(pidlParent);
        return;
    }

    if (!_ILIsEmpty(pidlChild)) {
        hr = IShellFolder_BindToObject(lpsfParent, pidlChild, 0, &IID_IShellFolder, (LPVOID*)&lpsfRoot);
    } else {
        lpsfRoot = lpsfParent;
        hr = IShellFolder_AddRef(lpsfParent);
    }

    if (FAILED(hr)) {
        WARN("Could not bind to root shell folder! hr = %08x\n", hr);
        IShellFolder_Release(lpsfParent);
        ILFree(pidlChild);
        ILFree(pidlParent);
        return;
    }

    flags = BrowseFlagsToSHCONTF( info->lpBrowseInfo->ulFlags );
    hr = IShellFolder_EnumObjects( lpsfRoot, info->hWnd, flags, &pEnumChildren );
    if (FAILED(hr)) {
        WARN("Could not get child iterator! hr = %08x\n", hr);
        IShellFolder_Release(lpsfParent);
        IShellFolder_Release(lpsfRoot);
        ILFree(pidlChild);
        ILFree(pidlParent);
        return;
    }

    SendMessageW( info->hwndTreeView, TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT );
    item = InsertTreeViewItem( info, lpsfParent, pidlChild,
                               pidlParent, pEnumChildren, TVI_ROOT );
    SendMessageW( info->hwndTreeView, TVM_EXPAND, TVE_EXPAND, (LPARAM)item );

    ILFree(pidlChild);
    ILFree(pidlParent);
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

	lptvid = SHAlloc( sizeof(TV_ITEMDATA) );
	if (!lptvid)
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

	while (S_OK == IEnumIDList_Next(lpe,1,&pidlTemp,&ulFetched))
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

	    if (!InsertTreeViewItem(info, lpsf, pidlTemp, pidl, pEnumIL, hParent))
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

    dwAttributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
    r = IShellFolder_GetAttributesOf(lptvid->lpsfParent, 1,
            (LPCITEMIDLIST*)&lptvid->lpi, &dwAttributes);
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

    if (!_ILIsEmpty(lptvid->lpi)) {
        r = IShellFolder_BindToObject( lptvid->lpsfParent, lptvid->lpi, 0,
                                       &IID_IShellFolder, (void**)&lpsf2 );
    } else {
        lpsf2 = lptvid->lpsfParent;
        IShellFolder_AddRef(lpsf2);
        r = S_OK;
    }

    if (SUCCEEDED(r))
    {
        FillTreeView( info, lpsf2, lptvid->lpifq, pnmtv->itemNew.hItem, lptvid->pEnumIL);
        IShellFolder_Release( lpsf2 );
    }

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

    item.mask = TVIF_HANDLE|TVIF_PARAM;
    item.hItem = (HTREEITEM)SendMessageW(info->hwndTreeView, TVM_GETNEXTITEM, TVGN_CARET, 0);
    SendMessageW(info->hwndTreeView, TVM_GETITEMW, 0, (LPARAM)&item);
    item_data = (LPTV_ITEMDATA)item.lParam;

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
    IShellFolder_ParseDisplayName(item_data->lpsfParent, NULL, NULL,
            pnmtv->item.pszText, NULL, &item_data->lpi, NULL);

    item.mask = TVIF_HANDLE|TVIF_TEXT;
    item.pszText = pnmtv->item.pszText;
    SendMessageW(info->hwndTreeView, TVM_SETITEMW, 0, (LPARAM)&item);

    nmtv.itemNew.lParam = item.lParam;
    BrsFolder_Treeview_Changed(info, &nmtv);
    return 0;
}

static HRESULT BrsFolder_Rename(browse_info *info, HTREEITEM rename)
{
    SendMessageW(info->hwndTreeView, TVM_SELECTITEM, TVGN_CARET, (LPARAM)rename);
    SendMessageW(info->hwndTreeView, TVM_EDITLABELW, 0, (LPARAM)rename);
    return S_OK;
}

#ifdef __REACTOS__
static void
BrsFolder_Delete(browse_info *info, HTREEITEM selected_item)
{
    TV_ITEMW item;
    TV_ITEMDATA *item_data;
    SHFILEOPSTRUCTW fileop = { info->hwndTreeView };
    WCHAR szzFrom[MAX_PATH + 1];

    /* get item_data */
    item.mask = TVIF_HANDLE | TVIF_PARAM;
    item.hItem = selected_item;
    if (!SendMessageW(info->hwndTreeView, TVM_GETITEMW, 0, (LPARAM)&item))
    {
        ERR("TVM_GETITEMW failed\n");
        return;
    }
    item_data = (TV_ITEMDATA *)item.lParam;

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
#endif
static LRESULT BrsFolder_Treeview_Keydown(browse_info *info, LPNMTVKEYDOWN keydown)
{
    HTREEITEM selected_item;

    /* Old dialog doesn't support those advanced features */
#ifdef __REACTOS__
    if (!(info->lpBrowseInfo->ulFlags & BIF_USENEWUI))
#else
    if (!(info->lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE))
#endif
        return 0;

    selected_item = (HTREEITEM)SendMessageW(info->hwndTreeView, TVM_GETNEXTITEM, TVGN_CARET, 0);

    switch (keydown->wVKey)
    {
    case VK_F2:
        BrsFolder_Rename(info, selected_item);
        break;
    case VK_DELETE:
        {
#ifdef __REACTOS__
            BrsFolder_Delete(info, selected_item);
#else
            const ITEMIDLIST *item_id;
            ISFHelper *psfhlp;
            HRESULT hr;
            TVITEMW item;
            TV_ITEMDATA *item_data;

            item.mask  = TVIF_PARAM;
            item.mask  = TVIF_HANDLE|TVIF_PARAM;
            item.hItem = selected_item;
            SendMessageW(info->hwndTreeView, TVM_GETITEMW, 0, (LPARAM)&item);
            item_data = (TV_ITEMDATA *)item.lParam;
            item_id = item_data->lpi;

            hr = IShellFolder_QueryInterface(item_data->lpsfParent, &IID_ISFHelper, (void**)&psfhlp);
            if(FAILED(hr))
                return 0;

            /* perform the item deletion - tree view gets updated over shell notification */
            ISFHelper_DeleteItems(psfhlp, 1, &item_id);
            ISFHelper_Release(psfhlp);
#endif
        }
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
    SetPropW( hWnd, szBrowseFolderInfo, info );

    if (lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE)
        FIXME("flags BIF_NEWDIALOGSTYLE partially implemented\n");
    if (lpBrowseInfo->ulFlags & ~SUPPORTEDFLAGS)
	FIXME("flags %x not implemented\n", lpBrowseInfo->ulFlags & ~SUPPORTEDFLAGS);

#ifdef __REACTOS__
    if (lpBrowseInfo->ulFlags & BIF_USENEWUI)
#else
    if (lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE)
#endif
    {
        RECT rcWnd;

#ifdef __REACTOS__
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
#else
        info->layout = LayoutInit(hWnd, g_layout_info, LAYOUT_INFO_COUNT);
#endif

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
#ifdef __REACTOS__
        || (lpBrowseInfo->ulFlags & BIF_USENEWUI))
#else
        || (lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE))
#endif
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

#ifndef __REACTOS__
        /* Resize the treeview if there's not editbox */
        if ((lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE)
            && !(lpBrowseInfo->ulFlags & BIF_EDITBOX))
        {
            RECT rc;
            GetClientRect(info->hwndTreeView, &rc);
            SetWindowPos(info->hwndTreeView, HWND_TOP, 0, 0,
                         rc.right, rc.bottom + 40, SWP_NOMOVE);
        }
#endif
    }
    else
        ERR("treeview control missing!\n");

    /* Register for change notifications */
    SHGetFolderLocation(NULL, CSIDL_DESKTOP, NULL, 0, &computer_pidl);

    ntreg.pidl = computer_pidl;
    ntreg.fRecursive = TRUE;

    info->hNotify = SHChangeNotifyRegister(hWnd, SHCNRF_InterruptLevel, SHCNE_ALLEVENTS, SHV_CHANGE_NOTIFY, 1, &ntreg);

#ifdef __REACTOS__
    SetFocus(info->hwndTreeView);
#endif
    browsefolder_callback( info->lpBrowseInfo, hWnd, BFFM_INITIALIZED, 0 );

#ifdef __REACTOS__
    SHAutoComplete(GetDlgItem(hWnd, IDC_BROWSE_FOR_FOLDER_FOLDER_TEXT),
                   (SHACF_FILESYS_ONLY | SHACF_URLHISTORY | SHACF_FILESYSTEM));
    return TRUE;
#else
    return TRUE;
#endif
}

static HRESULT BrsFolder_NewFolder(browse_info *info)
{
    DWORD flags = BrowseFlagsToSHCONTF(info->lpBrowseInfo->ulFlags);
    IShellFolder *desktop, *cur;
#ifdef __REACTOS__
    WCHAR wszNewFolder[25];
    WCHAR path[MAX_PATH];
#else
    ISFHelper *sfhelper;
#endif
    WCHAR name[MAX_PATH];
    HTREEITEM parent, added;
    LPTV_ITEMDATA item_data;
    LPITEMIDLIST new_item;
    TVITEMW item;
    HRESULT hr;
    int len;

#ifdef __REACTOS__
    hr = SHGetDesktopFolder(&desktop);
    if(FAILED(hr))
        return hr;

    if (info->pidlRet)
    {
        hr = IShellFolder_BindToObject(desktop, info->pidlRet, 0, &IID_IShellFolder, (void**)&cur);
        IShellFolder_Release(desktop);
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
#else
    if(!info->pidlRet) {
        ERR("Make new folder button should be disabled\n");
        return E_FAIL;
    }

    /* Create new directory */
    hr = SHGetDesktopFolder(&desktop);
    if(FAILED(hr))
        return hr;

    hr = IShellFolder_BindToObject(desktop, info->pidlRet, 0, &IID_IShellFolder, (void**)&cur);
    IShellFolder_Release(desktop);
    if(FAILED(hr))
        return hr;

    hr = IShellFolder_QueryInterface(cur, &IID_ISFHelper, (void**)&sfhelper);
    if(FAILED(hr))
        return hr;

    if(!SHGetPathFromIDListW(info->pidlRet, name)) {
        hr = E_FAIL;
        goto cleanup;
    }

    len = strlenW(name);
    if(len<MAX_PATH)
        name[len++] = '\\';
    hr = ISFHelper_GetUniqueName(sfhelper, &name[len], MAX_PATH-len);
    ISFHelper_Release(sfhelper);
    if(FAILED(hr))
        goto cleanup;
#endif

    hr = E_FAIL;
    if(!CreateDirectoryW(name, NULL))
        goto cleanup;

    /* Update parent of newly created directory */
    parent = (HTREEITEM)SendMessageW(info->hwndTreeView, TVM_GETNEXTITEM, TVGN_CARET, 0);
    if(!parent)
        goto cleanup;

    SendMessageW(info->hwndTreeView, TVM_EXPAND, TVE_EXPAND, (LPARAM)parent);

    memset(&item, 0, sizeof(TVITEMW));
    item.mask = TVIF_PARAM|TVIF_STATE;
    item.hItem = parent;
    SendMessageW(info->hwndTreeView, TVM_GETITEMW, 0, (LPARAM)&item);
    item_data = (LPTV_ITEMDATA)item.lParam;
    if(!item_data)
        goto cleanup;

    if(item_data->pEnumIL)
        IEnumIDList_Release(item_data->pEnumIL);
    hr = IShellFolder_EnumObjects(cur, info->hwndTreeView, flags, &item_data->pEnumIL);
    if(FAILED(hr))
        goto cleanup;

    /* Update treeview */
    if(!(item.state&TVIS_EXPANDEDONCE)) {
        item.mask = TVIF_STATE;
        item.state = TVIS_EXPANDEDONCE;
        item.stateMask = TVIS_EXPANDEDONCE;
        SendMessageW(info->hwndTreeView, TVM_SETITEMW, 0, (LPARAM)&item);
    }

    hr = IShellFolder_ParseDisplayName(cur, NULL, NULL, name+len, NULL, &new_item, NULL);
    if(FAILED(hr))
        goto cleanup;

    added = InsertTreeViewItem(info, cur, new_item, item_data->lpifq, NULL, parent);
    IShellFolder_Release(cur);
    SHFree(new_item);

    SendMessageW(info->hwndTreeView, TVM_SORTCHILDREN, FALSE, (LPARAM)parent);
    return BrsFolder_Rename(info, added);

cleanup:
    return hr;
}

static BOOL BrsFolder_OnCommand( browse_info *info, UINT id )
{
    LPBROWSEINFOW lpBrowseInfo = info->lpBrowseInfo;
#ifdef __REACTOS__
    WCHAR szPath[MAX_PATH];
#endif

    switch (id)
    {
    case IDOK:
#ifdef __REACTOS__
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
#endif
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

    case IDC_BROWSE_FOR_FOLDER_NEW_FOLDER:
        BrsFolder_NewFolder(info);
        return TRUE;
    }
    return FALSE;
}

static BOOL BrsFolder_OnSetExpanded(browse_info *info, LPVOID selection, 
    BOOL is_str, HTREEITEM *pItem)
{
    LPITEMIDLIST pidlSelection = selection;
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

        hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, 
                     selection, NULL, &pidlSelection, NULL);
        IShellFolder_Release(psfDesktop);
        if (FAILED(hr)) 
            goto done;
    }
#ifdef __REACTOS__
    if (_ILIsDesktop(pidlSelection))
    {
        item.hItem = TVI_ROOT;
        bResult = TRUE;
        goto done;
    }
#endif

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
    item.hItem = (HTREEITEM)SendMessageW(info->hwndTreeView, TVM_GETNEXTITEM, TVGN_ROOT, 0);

    if (item.hItem)
        item.hItem = (HTREEITEM)SendMessageW(info->hwndTreeView, TVM_GETNEXTITEM, TVGN_CHILD,
                                             (LPARAM)item.hItem);

    /* Walk the tree along the nodes corresponding to the remaining ITEMIDLIST */
    while (item.hItem && !_ILIsEmpty(pidlCurrent)) {
        LPTV_ITEMDATA pItemData;

        SendMessageW(info->hwndTreeView, TVM_GETITEMW, 0, (LPARAM)&item);
        pItemData = (LPTV_ITEMDATA)item.lParam;

        if (_ILIsEqualSimple(pItemData->lpi, pidlCurrent)) {
            pidlCurrent = ILGetNext(pidlCurrent);
            if (!_ILIsEmpty(pidlCurrent)) {
                /* Only expand current node and move on to its first child,
                 * if we didn't already reach the last SHITEMID */
                SendMessageW(info->hwndTreeView, TVM_EXPAND, TVE_EXPAND, (LPARAM)item.hItem);
                item.hItem = (HTREEITEM)SendMessageW(info->hwndTreeView, TVM_GETNEXTITEM, TVGN_CHILD,
                                             (LPARAM)item.hItem);
            }
        } else {
            item.hItem = (HTREEITEM)SendMessageW(info->hwndTreeView, TVM_GETNEXTITEM, TVGN_NEXT,
                                             (LPARAM)item.hItem);
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
        SendMessageW(info->hwndTreeView, TVM_SELECTITEM, TVGN_CARET, (LPARAM)hItem );
    return bResult;
}

static BOOL BrsFolder_OnSetSelectionA(browse_info *info, LPVOID selection, BOOL is_str) {
    LPWSTR selectionW = NULL;
    BOOL result = FALSE;
    int length;
    
    if (!is_str)
        return BrsFolder_OnSetSelectionW(info, selection, is_str);

    if ((length = MultiByteToWideChar(CP_ACP, 0, selection, -1, NULL, 0)) &&
        (selectionW = HeapAlloc(GetProcessHeap(), 0, length * sizeof(WCHAR))) &&
        MultiByteToWideChar(CP_ACP, 0, selection, -1, selectionW, length))
    {
        result = BrsFolder_OnSetSelectionW(info, selectionW, is_str);
    }

    HeapFree(GetProcessHeap(), 0, selectionW);
    return result;
}

#ifndef __REACTOS__ /* This is a buggy way (resize on title bar) */
static LRESULT BrsFolder_OnWindowPosChanging(browse_info *info, WINDOWPOS *pos)
{
#ifdef __REACTOS__
    if ((info->lpBrowseInfo->ulFlags & BIF_USENEWUI) && !(pos->flags & SWP_NOSIZE))
#else
    if ((info->lpBrowseInfo->ulFlags & BIF_NEWDIALOGSTYLE) && !(pos->flags & SWP_NOSIZE))
#endif
    {
        if (pos->cx < info->szMin.cx)
            pos->cx = info->szMin.cx;
        if (pos->cy < info->szMin.cy)
            pos->cy = info->szMin.cy;
    }
    return 0;
}
#endif

static INT BrsFolder_OnDestroy(browse_info *info)
{
    if (info->layout)
    {
#ifdef __REACTOS__
        LayoutDestroy(info->layout);
#else
        SHFree(info->layout);
#endif
        info->layout = NULL;
    }

    SHChangeNotifyDeregister(info->hNotify);

    return 0;
}

/* Find a treeview node by recursively walking the treeview */
static HTREEITEM BrsFolder_FindItemByPidl(browse_info *info, LPCITEMIDLIST pidl, HTREEITEM hItem)
{
    TV_ITEMW item;
    TV_ITEMDATA *item_data;
    HRESULT hr;

    item.mask = TVIF_HANDLE | TVIF_PARAM;
    item.hItem = hItem;
    SendMessageW(info->hwndTreeView, TVM_GETITEMW, 0, (LPARAM)&item);
    item_data = (TV_ITEMDATA *)item.lParam;

    hr = IShellFolder_CompareIDs(item_data->lpsfParent, 0, item_data->lpifq, pidl);
    if(SUCCEEDED(hr) && !HRESULT_CODE(hr))
        return hItem;

    hItem = (HTREEITEM)SendMessageW(info->hwndTreeView, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hItem);

    while (hItem)
    {
        HTREEITEM newItem = BrsFolder_FindItemByPidl(info, pidl, hItem);
        if (newItem)
            return newItem;
        hItem = (HTREEITEM)SendMessageW(info->hwndTreeView, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hItem);
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
            HTREEITEM handle_root = (HTREEITEM)SendMessageW(info->hwndTreeView, TVM_GETNEXTITEM, TVGN_ROOT, 0);
            HTREEITEM handle_item = BrsFolder_FindItemByPidl(info, pidls[0], handle_root);

            if (handle_item)
                SendMessageW(info->hwndTreeView, TVM_DELETEITEM, 0, (LPARAM)handle_item);

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

    info = GetPropW( hWnd, szBrowseFolderInfo );

    switch (msg)
    {
    case WM_NOTIFY:
        return BrsFolder_OnNotify( info, (UINT)wParam, (LPNMHDR)lParam);

    case WM_COMMAND:
        return BrsFolder_OnCommand( info, wParam );

#ifdef __REACTOS__
    case WM_GETMINMAXINFO:
        ((LPMINMAXINFO)lParam)->ptMinTrackSize.x = info->szMin.cx;
        ((LPMINMAXINFO)lParam)->ptMinTrackSize.y = info->szMin.cy;
        return 0;
#else /* This is a buggy way (resize on title bar) */
    case WM_WINDOWPOSCHANGING:
        return BrsFolder_OnWindowPosChanging( info, (WINDOWPOS *)lParam);
#endif

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

#ifndef __REACTOS__
static const WCHAR swBrowseTemplateName[] = {
    'S','H','B','R','S','F','O','R','F','O','L','D','E','R','_','M','S','G','B','O','X',0};
static const WCHAR swNewBrowseTemplateName[] = {
    'S','H','N','E','W','B','R','S','F','O','R','F','O','L','D','E','R','_','M','S','G','B','O','X',0};
#endif

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
        bi.pszDisplayName = HeapAlloc( GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR) );
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
#ifdef __REACTOS__
    WORD wDlgId;
#else
    const WCHAR * templateName;
    INITCOMMONCONTROLSEX icex;
#endif

    info.hWnd = 0;
    info.pidlRet = NULL;
    info.lpBrowseInfo = lpbi;
    info.hwndTreeView = NULL;

#ifdef __REACTOS__
    info.layout = NULL;
#else
    icex.dwSize = sizeof( icex );
    icex.dwICC = ICC_TREEVIEW_CLASSES;
    InitCommonControlsEx( &icex );
#endif

    hr = OleInitialize(NULL);

#ifdef __REACTOS__
    if (lpbi->ulFlags & BIF_USENEWUI)
#else
    if (lpbi->ulFlags & BIF_NEWDIALOGSTYLE)
#endif
        wDlgId = IDD_BROWSE_FOR_FOLDER_NEW;
    else
        wDlgId = IDD_BROWSE_FOR_FOLDER;
    r = DialogBoxParamW( shell32_hInstance, MAKEINTRESOURCEW(wDlgId), lpbi->hwndOwner,
	                 BrsFolderDlgProc, (LPARAM)&info );
    if (SUCCEEDED(hr)) 
        OleUninitialize();
    if (!r)
    {
        ILFree(info.pidlRet);
        return NULL;
    }

    return info.pidlRet;
}
