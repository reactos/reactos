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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * FIXME:
 *  - many memory leaks
 *  - many flags unimplemented
 */

#include <stdlib.h>
#include <string.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "wine/debug.h"
#include "undocshell.h"
#include "shlguid.h"
#include "pidl.h"
#include "shell32_main.h"
#include "shellapi.h"
#include "shresdef.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static HWND		hwndTreeView;
static LPBROWSEINFOW	lpBrowseInfo;
static LPITEMIDLIST	pidlRet;

static void FillTreeView(LPSHELLFOLDER lpsf, LPITEMIDLIST  lpifq, HTREEITEM hParent, IEnumIDList* lpe);
static HTREEITEM InsertTreeViewItem(IShellFolder * lpsf, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlParent, IEnumIDList* pEnumIL, HTREEITEM hParent);

#define SUPPORTEDFLAGS (BIF_STATUSTEXT | \
                        BIF_BROWSEFORCOMPUTER | \
                        BIF_RETURNFSANCESTORS | \
                        BIF_RETURNONLYFSDIRS | \
                        BIF_BROWSEINCLUDEFILES)

static inline DWORD BrowseFlagsToSHCONTF(UINT ulFlags)
{
    return SHCONTF_FOLDERS | (ulFlags & BIF_BROWSEINCLUDEFILES ? SHCONTF_NONFOLDERS : 0);
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
static void InitializeTreeView(HWND hwndParent, LPCITEMIDLIST root)
{
    LPITEMIDLIST pidlParent, pidlChild;
    HIMAGELIST hImageList;
    HRESULT hr;
    IShellFolder *lpsfParent, *lpsfRoot;
    IEnumIDList * pEnumChildren = NULL;

    TRACE("dlg=%p tree=%p\n", hwndParent, hwndTreeView );
    
    hwndTreeView = GetDlgItem (hwndParent, IDD_TREEVIEW);
    if (!hwndTreeView) {
        FIXME("Could not get handle to treeview control! Error: %08lx\n", GetLastError());
        return;
    }
    Shell_GetImageList(NULL, &hImageList);

    if (hImageList)
        TreeView_SetImageList(hwndTreeView, hImageList, 0);

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
            WARN("SHGetDesktopFolder failed! hr = %08lx\n", hr);
            return;
        }
        hr = IShellFolder_BindToObject(lpsfDesktop, pidlParent, 0, &IID_IShellFolder, (LPVOID*)&lpsfParent);
        IShellFolder_Release(lpsfDesktop);
    }
    
    if (!SUCCEEDED(hr)) {
        WARN("Could not bind to parent shell folder! hr = %08lx\n", hr);
        return;
    }

    if (pidlChild && pidlChild->mkid.cb) {
        hr = IShellFolder_BindToObject(lpsfParent, pidlChild, 0, &IID_IShellFolder, (LPVOID*)&lpsfRoot);
    } else {
        lpsfRoot = lpsfParent;
        hr = IShellFolder_AddRef(lpsfParent);
    }
    
    if (!SUCCEEDED(hr)) {
        WARN("Could not bind to root shell folder! hr = %08lx\n", hr);
        IShellFolder_Release(lpsfParent);
        return;
    }

    hr = IShellFolder_EnumObjects(lpsfRoot, hwndParent, BrowseFlagsToSHCONTF(lpBrowseInfo->ulFlags), &pEnumChildren);
    if (!SUCCEEDED(hr)) {
        WARN("Could not get child iterator! hr = %08lx\n", hr);
        IShellFolder_Release(lpsfParent);
        IShellFolder_Release(lpsfRoot);
        return;
    }

    TreeView_DeleteAllItems(hwndTreeView);
    TreeView_Expand(hwndTreeView, InsertTreeViewItem(lpsfParent, pidlChild, pidlParent, pEnumChildren,  TVI_ROOT), TVE_EXPAND);

    IShellFolder_Release(lpsfRoot);
    IShellFolder_Release(lpsfParent);
}

static int GetIcon(LPITEMIDLIST lpi, UINT uFlags)
{
	SHFILEINFOW    sfi;
	SHGetFileInfoW((LPCWSTR)lpi, 0 ,&sfi, sizeof(SHFILEINFOW), uFlags);
	return sfi.iIcon;
}

static void GetNormalAndSelectedIcons(LPITEMIDLIST lpifq, LPTVITEMW lpTV_ITEM)
{
	LPITEMIDLIST pidlDesktop = NULL;

	TRACE("%p %p\n",lpifq, lpTV_ITEM);

	if (!lpifq)
	{
	    pidlDesktop = _ILCreateDesktop();
	    lpifq = pidlDesktop;
	}

	lpTV_ITEM->iImage = GetIcon(lpifq, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON);
	lpTV_ITEM->iSelectedImage = GetIcon(lpifq, SHGFI_PIDL | SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_OPENICON);

	if (pidlDesktop)
	    ILFree(pidlDesktop);

	return;
}

typedef struct tagID
{
   LPSHELLFOLDER lpsfParent; /* IShellFolder of the parent */
   LPITEMIDLIST  lpi;        /* PIDL relativ to parent */
   LPITEMIDLIST  lpifq;      /* Fully qualified PIDL */
   IEnumIDList*  pEnumIL;    /* Children iterator */ 
} TV_ITEMDATA, *LPTV_ITEMDATA;

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

	TRACE("%p %p %lx %p\n", lpsf, lpi, dwFlags, lpFriendlyName);
	if (SUCCEEDED(IShellFolder_GetDisplayNameOf(lpsf, lpi, dwFlags, &str)))
	{
	  if (FAILED(StrRetToStrNW(lpFriendlyName, MAX_PATH, &str, lpi)))
	  {
	      bSuccess = FALSE;
	  }
	}
	else
	  bSuccess = FALSE;

	TRACE("-- %s\n", debugstr_w(lpFriendlyName));
	return bSuccess;
}

/******************************************************************************
 * InsertTreeViewItem [Internal]
 *
 * PARAMS
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
static HTREEITEM InsertTreeViewItem(IShellFolder * lpsf, LPCITEMIDLIST pidl, 
    LPCITEMIDLIST pidlParent, IEnumIDList* pEnumIL, HTREEITEM hParent)
{
	TVITEMW 	tvi;
	TVINSERTSTRUCTW	tvins;
	WCHAR		szBuff[MAX_PATH];
	LPTV_ITEMDATA	lptvid=0;

	tvi.mask  = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;

	tvi.cChildren= pEnumIL ? 1 : 0;
	tvi.mask |= TVIF_CHILDREN;

	if (!(lptvid = (LPTV_ITEMDATA)SHAlloc(sizeof(TV_ITEMDATA))))
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

	return (HTREEITEM)TreeView_InsertItemW(hwndTreeView, &tvins);
}

/******************************************************************************
 * FillTreeView [Internal]
 *
 * For each child (given by lpe) of the parent shell folder, which is given by 
 * lpsf and whose PIDL is pidl, insert a treeview-item right under hParent
 *
 * PARAMS
 *  lpsf    [I] IShellFolder interface of the parent shell folder
 *  pidl    [I] ITEMIDLIST of the parent shell folder
 *  hParent [I] The treeview item that represents the parent shell folder
 *  lpe     [I] An iterator for the children of the parent shell folder
 */
static void FillTreeView(IShellFolder * lpsf, LPITEMIDLIST  pidl, HTREEITEM hParent, IEnumIDList* lpe)
{
	HTREEITEM	hPrev = 0;
	LPITEMIDLIST	pidlTemp = 0;
	ULONG		ulFetched;
	HRESULT		hr;
	HWND		hwnd=GetParent(hwndTreeView);

	TRACE("%p %p %x %p\n",lpsf, pidl, (INT)hParent, lpe);

	/* No IEnumIDList -> No children */
	if (!lpe) return;
	
	SetCapture(GetParent(hwndTreeView));
	SetCursor(LoadCursorA(0, (LPSTR)IDC_WAIT));

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
	            hr = IShellFolder_EnumObjects(pSFChild, hwnd, BrowseFlagsToSHCONTF(lpBrowseInfo->ulFlags), &pEnumIL);
                    if (SUCCEEDED(hr))
                    {
                        if ((IEnumIDList_Skip(pEnumIL, 1) != S_OK) || FAILED(IEnumIDList_Reset(pEnumIL)))
                        {
                            IEnumIDList_Release(pEnumIL);
                            pEnumIL = NULL;
                        }
                    }
                    IShellFolder_Release(pSFChild);
                }
	    }

	    if (!(hPrev = InsertTreeViewItem(lpsf, pidlTemp, pidl, pEnumIL, hParent)))
	        goto Done;
	    SHFree(pidlTemp);  /* Finally, free the pidl that the shell gave us... */
	    pidlTemp=NULL;
	}

Done:
	ReleaseCapture();
	SetCursor(LoadCursorW(0, (LPWSTR)IDC_ARROW));

	if (pidlTemp)
	  SHFree(pidlTemp);
}

static inline BOOL PIDLIsType(LPCITEMIDLIST pidl, PIDLTYPE type)
{
    LPPIDLDATA data = _ILGetDataPointer(pidl);
    if (!data)
        return FALSE;
    return (data->type == type);
}

static void BrsFolder_CheckValidSelection(HWND hWndTree, LPTV_ITEMDATA lptvid)
{
    LPCITEMIDLIST pidl = lptvid->lpi;
    BOOL bEnabled = TRUE;
    DWORD dwAttributes;
    if ((lpBrowseInfo->ulFlags & BIF_BROWSEFORCOMPUTER) &&
        !PIDLIsType(pidl, PT_COMP))
        bEnabled = FALSE;
    if (lpBrowseInfo->ulFlags & BIF_RETURNFSANCESTORS)
    {
        dwAttributes = SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM;
        if (FAILED(IShellFolder_GetAttributesOf(lptvid->lpsfParent, 1, (LPCITEMIDLIST*)&lptvid->lpi, &dwAttributes)) ||
            !dwAttributes)
            bEnabled = FALSE;
    }
    if (lpBrowseInfo->ulFlags & BIF_RETURNONLYFSDIRS)
    {
        dwAttributes = SFGAO_FOLDER | SFGAO_FILESYSTEM;
        if (FAILED(IShellFolder_GetAttributesOf(lptvid->lpsfParent, 1, (LPCITEMIDLIST*)&lptvid->lpi, &dwAttributes)) ||
            (dwAttributes != (SFGAO_FOLDER | SFGAO_FILESYSTEM)))
            bEnabled = FALSE;
    }
    SendMessageW(hWndTree, BFFM_ENABLEOK, 0, (LPARAM)bEnabled);
}

static LRESULT MsgNotify(HWND hWnd,  UINT CtlID, LPNMHDR lpnmh)
{
	NMTREEVIEWW	*pnmtv   = (NMTREEVIEWW *)lpnmh;
	LPTV_ITEMDATA	lptvid;  /* Long pointer to TreeView item data */
	IShellFolder *	lpsf2=0;


	TRACE("%p %x %p msg=%x\n", hWnd,  CtlID, lpnmh, pnmtv->hdr.code);

	switch (pnmtv->hdr.idFrom)
	{ case IDD_TREEVIEW:
	    switch (pnmtv->hdr.code)
	    {
	      case TVN_DELETEITEMA:
	      case TVN_DELETEITEMW:
                TRACE("TVN_DELETEITEMA/W\n");
	        lptvid=(LPTV_ITEMDATA)pnmtv->itemOld.lParam;
	        IShellFolder_Release(lptvid->lpsfParent);
	        if (lptvid->pEnumIL)
	          IEnumIDList_Release(lptvid->pEnumIL);
	        SHFree(lptvid->lpi);
	        SHFree(lptvid->lpifq);
	        SHFree(lptvid);
	        break;

	      case TVN_ITEMEXPANDINGA:
	      case TVN_ITEMEXPANDINGW:
		{
                  TRACE("TVN_ITEMEXPANDINGA/W\n");
		  if ((pnmtv->itemNew.state & TVIS_EXPANDEDONCE))
	            break;

	          lptvid=(LPTV_ITEMDATA)pnmtv->itemNew.lParam;
	          if (SUCCEEDED(IShellFolder_BindToObject(lptvid->lpsfParent, lptvid->lpi,0,(REFIID)&IID_IShellFolder,(LPVOID *)&lpsf2)))
	          { FillTreeView( lpsf2, lptvid->lpifq, pnmtv->itemNew.hItem, lptvid->pEnumIL);
	          }
	          /* My Computer is already sorted and trying to do a simple text
	           * sort will only mess things up */
	          if (!_ILIsMyComputer(lptvid->lpi))
	            TreeView_SortChildren(hwndTreeView, pnmtv->itemNew.hItem, FALSE);
		}
	        break;
	      case TVN_SELCHANGEDA:
	      case TVN_SELCHANGEDW:
	        lptvid=(LPTV_ITEMDATA)pnmtv->itemNew.lParam;
		pidlRet = lptvid->lpifq;
		if (lpBrowseInfo->lpfn)
		   (lpBrowseInfo->lpfn)(hWnd, BFFM_SELCHANGED, (LPARAM)pidlRet, lpBrowseInfo->lParam);
		BrsFolder_CheckValidSelection(hWnd, lptvid);
	        break;

	      default:
	        WARN("unhandled (%d)\n", pnmtv->hdr.code);
		break;
	    }
	    break;

	  default:
	    break;
	}

	return 0;
}


/*************************************************************************
 *             BrsFolderDlgProc32  (not an exported API function)
 */
static INT_PTR CALLBACK BrsFolderDlgProc(HWND hWnd, UINT msg, WPARAM wParam,
				     LPARAM lParam )
{
	TRACE("hwnd=%p msg=%04x 0x%08x 0x%08lx\n", hWnd,  msg, wParam, lParam );

	switch(msg)
	{ case WM_INITDIALOG:
	    pidlRet = NULL;
	    lpBrowseInfo = (LPBROWSEINFOW) lParam;
	    if (lpBrowseInfo->ulFlags & ~SUPPORTEDFLAGS)
	      FIXME("flags %x not implemented\n", lpBrowseInfo->ulFlags & ~SUPPORTEDFLAGS);
	    if (lpBrowseInfo->lpszTitle) {
	       SetWindowTextW(GetDlgItem(hWnd, IDD_TITLE), lpBrowseInfo->lpszTitle);
	    } else {
	       ShowWindow(GetDlgItem(hWnd, IDD_TITLE), SW_HIDE);
	    }
	    if (!(lpBrowseInfo->ulFlags & BIF_STATUSTEXT))
	       ShowWindow(GetDlgItem(hWnd, IDD_STATUS), SW_HIDE);

	    InitializeTreeView(hWnd, lpBrowseInfo->pidlRoot);

	    if (lpBrowseInfo->lpfn)
	       (lpBrowseInfo->lpfn)(hWnd, BFFM_INITIALIZED, 0, lpBrowseInfo->lParam);

	    return TRUE;

	  case WM_NOTIFY:
	    MsgNotify( hWnd, (UINT)wParam, (LPNMHDR)lParam);
	    break;

	  case WM_COMMAND:
	    switch (wParam)
	    { case IDOK:
	        pdump ( pidlRet );
		if (lpBrowseInfo->pszDisplayName)
	            SHGetPathFromIDListW(pidlRet, lpBrowseInfo->pszDisplayName);
	        EndDialog(hWnd, (DWORD) ILClone(pidlRet));
	        return TRUE;

	      case IDCANCEL:
	        EndDialog(hWnd, 0);
	        return TRUE;
	    }
	    break;
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
	   if (wParam)
	      FIXME("Set selection %s\n", debugstr_a((LPSTR)lParam));
	   else
	      FIXME("Set selection %p\n", (void*)lParam);
	   break;
	case BFFM_SETSELECTIONW:
	   if (wParam)
	      FIXME("Set selection %s\n", debugstr_w((LPWSTR)lParam));
	   else
	      FIXME("Set selection %p\n", (void*)lParam);
	   break;
	case BFFM_SETEXPANDED: /* unicode only */
	   if (wParam)
	      FIXME("Set expanded %s\n", debugstr_w((LPWSTR)lParam));
	   else
	      FIXME("Set expanded %p\n", (void*)lParam);
	   break;
	}
	return FALSE;
}

static const WCHAR swBrowseTempName[] = {'S','H','B','R','S','F','O','R','F','O','L','D','E','R','_','M','S','G','B','O','X',0};

/*************************************************************************
 * SHBrowseForFolderA [SHELL32.@]
 * SHBrowseForFolder  [SHELL32.@]
 */
LPITEMIDLIST WINAPI SHBrowseForFolderA (LPBROWSEINFOA lpbi)
{
	BROWSEINFOW bi;
	LPITEMIDLIST lpid;
	INT len;
	
	TRACE("(%p{lpszTitle=%s,owner=%p})\n", lpbi,
	    lpbi ? debugstr_a(lpbi->lpszTitle) : NULL, lpbi ? lpbi->hwndOwner : NULL);

	if (!lpbi)
	  return NULL;

	bi.hwndOwner = lpbi->hwndOwner;
	bi.pidlRoot = lpbi->pidlRoot;
	if (lpbi->pszDisplayName)
	{
	  /*lpbi->pszDisplayName is assumed to be MAX_PATH (MSDN) */
	  bi.pszDisplayName = HeapAlloc(GetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
	  MultiByteToWideChar(CP_ACP, 0, lpbi->pszDisplayName, -1, bi.pszDisplayName, MAX_PATH);
	}
	else
	  bi.pszDisplayName = NULL;

	if (lpbi->lpszTitle)
	{
	  len = MultiByteToWideChar(CP_ACP, 0, lpbi->lpszTitle, -1, NULL, 0);
	  bi.lpszTitle = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	  MultiByteToWideChar(CP_ACP, 0, lpbi->lpszTitle, -1, (LPWSTR)bi.lpszTitle, len);
	}
	else
	  bi.lpszTitle = NULL;

	bi.ulFlags = lpbi->ulFlags;
	bi.lpfn = lpbi->lpfn;
	bi.lParam = lpbi->lParam;
	bi.iImage = lpbi->iImage;
	lpid = (LPITEMIDLIST) DialogBoxParamW(shell32_hInstance,
	                                      swBrowseTempName, lpbi->hwndOwner,
	                                      BrsFolderDlgProc, (INT)&bi);
	if (bi.pszDisplayName)
	{
	  WideCharToMultiByte(CP_ACP, 0, bi.pszDisplayName, -1, lpbi->pszDisplayName, MAX_PATH, 0, NULL);
	  HeapFree(GetProcessHeap(), 0, bi.pszDisplayName);
	}
        HeapFree(GetProcessHeap(), 0, (LPVOID)bi.lpszTitle);
	lpbi->iImage = bi.iImage;
	return lpid;
}


/*************************************************************************
 * SHBrowseForFolderW [SHELL32.@]
 */
LPITEMIDLIST WINAPI SHBrowseForFolderW (LPBROWSEINFOW lpbi)
{
	TRACE("((%p->{lpszTitle=%s,owner=%p})\n", lpbi,
	    lpbi ? debugstr_w(lpbi->lpszTitle) : NULL, lpbi ? lpbi->hwndOwner : 0);

	if (!lpbi)
	  return NULL;

	return (LPITEMIDLIST) DialogBoxParamW(shell32_hInstance,
	                                      swBrowseTempName, lpbi->hwndOwner,
	                                      BrsFolderDlgProc, (INT)lpbi);
}
