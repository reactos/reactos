#define NOSHELLDEBUG    // don't take shell versions of this

// todo:
//
//      "New Folder" capability
//
//      dlb click on folders should expand/collapse
//
//	delayed keyboard selection so keyboard navigation does not generate a sel change
//	
//	drag and drop image drawing (not just OLE cursors)
//
//	name space change notification. hook up notify handler
//
//	default keyboard accelerators (F2 = Rename, etc)
//
//      Partial expanded nodes in the tree "Tree Down" (TVIS_EXPANDPARTIAL) for net cases
//
//      Programbility:
//          notifies - sel changed, node expanded, verb executed, etc.
//          cmds - do verb, get item, etc.

#include <windows.h>
#include <shlobj.h>
#include <shlobjp.h>    // for SHChangeNotifyReigster

#include "idlist.h"

#include "nsc.h"
#include "dropsrc.h"
#include "common.h"

#define ID_CONTROL  100

#define WM_CHANGENOTIFY  (WM_USER + 11)

#define SHCNE_FOLDERS                           \
    (SHCNE_MKDIR         | SHCNE_RMDIR |        \
     SHCNE_MEDIAINSERTED | SHCNE_MEDIAREMOVED | \
     SHCNE_DRIVEREMOVED  | SHCNE_DRIVEADD |     \
     SHCNE_RENAMEFOLDER  |                      \
     SHCNE_UPDATEDIR     |                      \
     SHCNE_UPDATEITEM    |                      \
     SHCNE_SERVERDISCONNECT |                   \
     SHCNE_UPDATEIMAGE      |                   \
     SHCNE_DRIVEADDGUI)

#define SHCNE_ITEMS (SHCNE_CREATE | SHCNE_DELETE | SHCNE_RENAMEITEM | SHCNE_ASSOCCHANGED)


void _RegisterNotify(NSC *pns, BOOL fReRegister)
{
    if (pns->nChangeNotifyID)
    {
        SHChangeNotifyDeregister(pns->nChangeNotifyID);
        pns->nChangeNotifyID = 0;
    }

    if (fReRegister)
    {
        SHChangeNotifyEntry fsne;

        fsne.pidl = pns->pidlRoot;
        fsne.fRecursive = TRUE;

        pns->nChangeNotifyID = SHChangeNotifyRegister(pns->hwnd,
            // SHCNRF_NewDelivery | 
            SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
            pns->style & NSS_SHOWNONFOLDERS ? (SHCNE_FOLDERS | SHCNE_ITEMS) : (SHCNE_FOLDERS),
            WM_CHANGENOTIFY, 1, &fsne);
    }
}

LRESULT _OnNCCreate(HWND hwnd, LPCREATESTRUCT pcs)
{
    NSC *pns = LocalAlloc(LPTR, sizeof(NSC));
    if (pns)
    {
	pns->hwnd = hwnd;
	pns->hwndParent = pcs->hwndParent;
	pns->style = pcs->style;
	pns->id = (UINT)pcs->hMenu;

	// remove border styles from our window that we propogated to
	// our child control

	SetWindowLong(hwnd, GWL_STYLE, pcs->style & ~WS_BORDER);
	SetWindowLong(hwnd, GWL_EXSTYLE, pcs->dwExStyle & ~WS_EX_CLIENTEDGE);

	pns->hwndTree = CreateWindowEx(pcs->dwExStyle, WC_TREEVIEW, NULL,
	    (pcs->style & 0xFFFF0000) | (WS_CHILD | TVS_HASBUTTONS | TVS_EDITLABELS | TVS_SHOWSELALWAYS), // TVS_HASLINES | 
	    pcs->x, pcs->y, pcs->cx, pcs->cy,
	    hwnd, (HMENU)ID_CONTROL, pcs->hInstance, NULL);
	if (pns->hwndTree)
	{
	    SHFILEINFO sfi;
	    HIMAGELIST himl = (HIMAGELIST)SHGetFileInfo("C:\\", 0, &sfi, sizeof(SHFILEINFO),  SHGFI_SYSICONINDEX | SHGFI_SMALLICON);

	    SetWindowLong(hwnd, 0, (LONG)pns);

	    TreeView_SetImageList(pns->hwndTree, himl, TVSIL_NORMAL);

	    if (pns->style & NSS_DROPTARGET)
		CTreeDropTarget_Register(pns);

	    return TRUE;	// success
	}
	LocalFree(pns);
    }
    DebugMsg(DM_ERROR, "Failing NameSpaceControl create");

    return FALSE;	// fail the create
}

void _ReleaseCachedShellFolder(NSC *pns)
{
    if (pns->psfCache)
    {
        Release(pns->psfCache);

        pns->psfCache = NULL;
        pns->htiCache = NULL;
    }
}

void _ReleaseRootFolder(NSC *pns)
{
    if (pns->psfRoot)
    {
        Release(pns->psfRoot);
	pns->psfRoot = NULL;

        if (pns->pidlRoot)
	{
	    ILFree(pns->pidlRoot);
	    pns->pidlRoot = NULL;
	}
    }
}

void _OnNCDestroy(NSC *pns)
{
    _ReleaseRootFolder(pns);

    Assert(pns->pidlRoot == NULL);

    // ILFree(pns->pidlRoot);
    // pns->pidlRoot = NULL;

    _ReleaseCachedShellFolder(pns);

    if (pns->style & NSS_DROPTARGET)
        CTreeDropTarget_Revoke(pns);

    _RegisterNotify(pns, FALSE);


    LocalFree(pns);
}

// builds a fully qualified IDLIST from a given tree node by walking up the tree
// be sure to free this when you are done!

LPITEMIDLIST _GetFullIDList(HWND hwndTree, HTREEITEM hti)
{
    LPITEMIDLIST pidl;
    TV_ITEM tvi;

    Assert(hti);

    // now lets get the information about the item
    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
    tvi.hItem = hti;
    if (!TreeView_GetItem(hwndTree, &tvi))
    {
        DebugMsg(DM_ERROR, "bogus tree item passed");
        return NULL;
    }

    pidl = ILClone((LPITEMIDLIST)tvi.lParam);

    // Now walk up parents.
    while ((tvi.hItem = TreeView_GetParent(hwndTree, tvi.hItem)) && pidl)
    {
        LPITEMIDLIST pidlT;

        if (!TreeView_GetItem(hwndTree, &tvi))
            return pidl;   // will assume I screwed up...

        pidlT = ILCombine((LPITEMIDLIST)tvi.lParam, pidl);

        ILFree(pidl);

        pidl = pidlT;

    }
    return pidl;
}

/*
pitem->flags;
pitem->hitem;
pitem->psf;
pitem->pidl;
pitem->dwAttributes;
*/

BOOL _GetItem(NSC *pns, NSC_ITEMINFO *pitem)
{
    // BUGBUG: validate pitem->hitem

    if (pitem->flags & NSIF_HITEM)
    {

    }

    if (pitem->flags & NSIF_FOLDER)
    {
        Assert(!(pitem->flags & NSIF_PARENTFOLDER));	// should be exclusive

    }
    else if (pitem->flags & NSIF_PARENTFOLDER)
    {

    }

    if (pitem->flags & (NSIF_IDLIST | NSIF_FULLIDLIST))
    {
        pitem->pidl = NULL;

        if (pitem->flags & NSIF_FULLIDLIST)
	        pitem->pidl = _GetFullIDList(pns->hwndTree, (HTREEITEM)pitem->hitem);
	}
	else
	{
	    TV_ITEM tvi;

	    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
	    tvi.hItem = (HTREEITEM)pitem->hitem;
	    if (TreeView_GetItem(pns->hwndTree, &tvi))
	    {
	        pitem->pidl = (LPITEMIDLIST)tvi.lParam;
	    }
    }

    if (pitem->flags & NSIF_ATTRIBUTES)
    {

    }
    return TRUE;
}

BOOL _SetItemNotify(NSC *pns, NSI_FLAGS flags)
{
    // NS_NOTIFY nsn;

    return TRUE;
}

// Some helper functions for processing the dialog

HTREEITEM _AddItemToTree(HWND hwndTree, HTREEITEM htiParent, LPCITEMIDLIST pidl, int cChildren)
{
    TV_INSERTSTRUCT tii;

    // Initialize item to add with callback for everything
    tii.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_CHILDREN;
    tii.hParent = htiParent;
    tii.hInsertAfter = TVI_FIRST;
    tii.item.iImage = I_IMAGECALLBACK;
    tii.item.iSelectedImage = I_IMAGECALLBACK;
    tii.item.pszText = LPSTR_TEXTCALLBACK;   //
    tii.item.cChildren = cChildren; //  Assume it has children
    tii.item.lParam = (LPARAM)pidl;

    return TreeView_InsertItem(hwndTree, &tii);
}

void _ExpandTree(NSC *pns, HTREEITEM htiRoot, int iDepth)
{
    HTREEITEM hti;

    if (iDepth == 0)
        return;

    TreeView_Expand(pns->hwndTree, htiRoot, TVE_EXPAND);

    if (iDepth == 1)
        return;     // avoid useless loop

    // recurse to children, expanding them
    for (hti = TreeView_GetChild(pns->hwndTree, htiRoot); hti; hti = TreeView_GetNextSibling(pns->hwndTree, hti))
    {
        _ExpandTree(pns, hti, iDepth - 1);
    }
}

// set the root of the name space control.
//
// in:
//	pidlRoot    NULL means the desktop
//		    HIWORD 0 -> LOWORD == ID of special folder (CSIDL_* values)

BOOL _OnSetRoot(NSC *pns, NSC_SETROOT *psr)
{
    _ReleaseRootFolder(pns);

    if (psr->psf)
    {
        pns->psfRoot = psr->psf;
    }
    else if (FAILED(SHGetDesktopFolder(&pns->psfRoot)))
    {
	DebugMsg(DM_ERROR, "Failed to get desktop folder");
	return FALSE;
    }

    AddRef(pns->psfRoot);	// we hang on to this

    // HIWORD/LOWORD stuff is to support pidl IDs instead of full pidl here
    if (HIWORD(psr->pidlRoot))
        pns->pidlRoot = ILClone(psr->pidlRoot);
    else
    {
	Assert(psr->psf == NULL);	// special folders are only valid for desktop shell folder

	SHGetSpecialFolderLocation(NULL, LOWORD(psr->pidlRoot) ? LOWORD(psr->pidlRoot) : CSIDL_DESKTOP, &pns->pidlRoot);
    }

    if (pns->pidlRoot)
    {
        HTREEITEM htiRoot = _AddItemToTree(pns->hwndTree, TVI_ROOT, pns->pidlRoot, 1);
	if (htiRoot)
	{
            _ExpandTree(pns, htiRoot, psr->iExpandDepth);

            TreeView_SelectItem(pns->hwndTree, htiRoot);

            _RegisterNotify(pns, TRUE);

	    return TRUE;
	}
    }

    DebugMsg(DM_ERROR, "set root failed");

    _ReleaseRootFolder(pns);

    return FALSE;
}


// cache the shell folder for a given tree item
// in:
//	hti	tree node to cache shell folder for. this my be
//		NULL indicating the root item.
//

BOOL _CacheShellFolder(NSC *pns, HTREEITEM hti)
{
    // in the cache?
    if ((hti != pns->htiCache) || (pns->psfCache == NULL))
    {
	// cache miss, do the work
        LPITEMIDLIST pidl;

	_ReleaseCachedShellFolder(pns);

	if (hti)
            pidl = _GetFullIDList(pns->hwndTree, hti);
	else
	{
	    // root item...
	    pidl = ILClone(pns->pidlRoot);
	    if (pidl && pidl->mkid.cb != 0)
	    {
	        ILRemoveLastID(pidl);
	    }
	}

	if (pidl)
	{
	    // special case for root of evil...
	    if (pidl->mkid.cb == 0)
	    {
		pns->psfCache = pns->psfRoot;
		AddRef(pns->psfCache);  // to match ref count
	    }
	    else
	    {
		pns->psfRoot->lpVtbl->BindToObject(pns->psfRoot, pidl, NULL, &IID_IShellFolder, &pns->psfCache);
	    }

	    ILFree(pidl);

	    if (pns->psfCache)
	    {
		pns->htiCache = hti;	// this is for the cache match
		return TRUE;
	    }
	    else
		DebugMsg(DM_ERROR, "failed to get cached shell folder");
	}
	else
	    DebugMsg(DM_ERROR, "failed to get create PIDL for cached shell folder");

	return FALSE;
    }
    return TRUE;
}

// pidlItem is typically a relative pidl, except in the case of the root where
// it can be a fully qualified pidl

LPITEMIDLIST _CacheParentShellFolder(NSC *pns, HTREEITEM hti, LPITEMIDLIST pidl)
{
    Assert(hti);

    if (_CacheShellFolder(pns, TreeView_GetParent(pns->hwndTree, hti)))
    {
	if (pidl == NULL)
	{
	    TV_ITEM tvi;
	    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
	    tvi.hItem = hti;
	    if (!TreeView_GetItem(pns->hwndTree, &tvi))
		return NULL;

	    pidl = (LPITEMIDLIST)tvi.lParam;
	}

        return ILFindLastID(pidl);
    }

    return NULL;
}

int CALLBACK _TreeCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    IShellFolder *psf = (IShellFolder *)lParamSort;
    HRESULT hres = psf->lpVtbl->CompareIDs(psf, 0, (LPITEMIDLIST)lParam1, (LPITEMIDLIST)lParam2);

    Assert(SUCCEEDED(hres));

    return (short)SCODE_CODE(hres);
}

void _Sort(NSC *pns, HTREEITEM hti, IShellFolder *psf)
{
    TV_SORTCB scb;

    scb.hParent = hti;
    scb.lpfnCompare = _TreeCompare;

    scb.lParam = (LPARAM)psf;
    TreeView_SortChildrenCB(pns->hwndTree, &scb, FALSE);
}

// filter function... let clients filter what gets added here

BOOL _ShouldAdd(NSC *pns, LPCITEMIDLIST pidl)
{
#if 0
    //
    // We need to special case here in the netcase where we onlyu
    // browse down to workgroups...
    //
    //
    // Here is where I also need to special case to not go below
    // workgroups when the appropriate option is set.
    //
    bType = SIL_GetType(pidl);
    if ((pns->ulFlags & BIF_DONTGOBELOWDOMAIN) && (bType & SHID_NET))
    {
	switch (bType & (SHID_NET | SHID_INGROUPMASK))
	{
	case SHID_NET_SERVER:
	    ILFree(pidl);       // Dont want to add this one
	    continue;           // Try the next one
	case SHID_NET_DOMAIN:
	    cChildren = 0;      // Force to not have children;
	}
    }
    else if ((pns->ulFlags & BIF_BROWSEFORCOMPUTER) && (bType & SHID_NET))
    {
	if ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SERVER)
	    cChildren = 0;  // Don't expand below it...
    }
    else if (fPrinterTest)
    {
	// Special case when we are only allowing printers.
	// for now I will simply key on the fact that it is non-FS.
	ULONG ulAttr = SFGAO_FILESYSTEM;

	psf->lpVtbl->GetAttributesOf(psf, 1, &pidl, &ulAttr);

	if ((ulAttr & SFGAO_FILESYSTEM)== 0)
	{
	    cChildren = 0;      // Force to not have children;
	}
	else
	{
	    ILFree(pidl);       // Dont want to add this one
	    continue;           // Try the next one
	}
    }
#endif
    // send notify up to partent to let them filter
    return TRUE;
}

BOOL _OnItemExpanding(NSC *pns, NM_TREEVIEW *pnm)
{
    IShellFolder *psf;
    IEnumIDList *penum;              // Enumerator in use.
    DWORD grfFlags = SHCONTF_FOLDERS;
    int cAdded = 0;

    if ((pnm->action != TVE_EXPAND) || (pnm->itemNew.state & TVIS_EXPANDEDONCE))
        return FALSE;

    Assert(pnm->itemNew.hItem);

    if (!_CacheShellFolder(pns, pnm->itemNew.hItem))
        return FALSE;

    psf = pns->psfCache;
    AddRef(psf);	// hang on as adding items may change the cached psfCache

#if 0
    // Need to do a couple of special cases here to allow us to
    // browse for a network printer.  In this case if we are at server
    // level we then need to change what we search for non folders when
    // we are the level of a server.
    if (pns->ulFlags & BIF_BROWSEFORPRINTER)
    {
        grfFlags = SHCONTF_FOLDERS | SHCONTF_NETPRINTERSRCH;
        pidl = ILFindLastID(pidlToExpand);
        bType = SIL_GetType(pidl);
        fPrinterTest = ((bType & (SHID_NET|SHID_INGROUPMASK))==SHID_NET_SERVER);
        if (fPrinterTest)
            grfFlags |= SHCONTF_NONFOLDERS;
    }
    else
#endif

    if (pns->style & NSS_SHOWNONFOLDERS)
        grfFlags |= SHCONTF_NONFOLDERS;

    if (pns->style & NSS_SHOWHIDDEN)
        grfFlags |= SHCONTF_INCLUDEHIDDEN;

    // passing NULL hwnd makes the enum not put up UI. this is what we want
    // in auto-expand cases

    if (SUCCEEDED(psf->lpVtbl->EnumObjects(psf, 
        pns->fAutoExpanding ? NULL : pns->hwnd, grfFlags, &penum)))
    {
        UINT celt;
	LPITEMIDLIST pidl;

        while (penum->lpVtbl->Next(penum, 1, &pidl, &celt) == S_OK && celt == 1)
	{
	    int cChildren = I_CHILDRENCALLBACK;  // Do call back for children

            if (_ShouldAdd(pns, pidl))
            {
                _AddItemToTree(pns->hwndTree, pnm->itemNew.hItem, pidl, cChildren);
                cAdded++;
            }
            else
            {
                ILFree(pidl);
            }
	}
	ReleaseLast(penum);

	_Sort(pns, pnm->itemNew.hItem, psf);
    }

    Release(psf);

    // If we did not add anything we should update this item to let
    // the user know something happened.
    //
    if (cAdded == 0)
    {
        TV_ITEM tvi;
        tvi.mask = TVIF_CHILDREN | TVIF_HANDLE;   // only change the number of children
        tvi.hItem = pnm->itemNew.hItem;
        tvi.cChildren = 0;

        TreeView_SetItem(pns->hwndTree, &tvi);
    }

    return TRUE;
}

void _OnDeleteItem(NSC *pns, NM_TREEVIEW *pnm)
{
    ILFree((LPITEMIDLIST)pnm->itemOld.lParam);
}

void _GetIconIndex(NSC *pns, LPITEMIDLIST pidl, ULONG ulAttrs, TVITEM *pitem)
{
    IShellIcon *psi;

    if (SUCCEEDED(QueryInterface(pns->psfCache, &IID_IShellIcon, &psi)))
    {
        if (psi->lpVtbl->GetIconOf(psi, pidl, 0, &pitem->iImage) == S_OK)
        {
            if (!(ulAttrs & SFGAO_FOLDER) || FAILED(psi->lpVtbl->GetIconOf(psi, pidl, GIL_OPENICON, &pitem->iSelectedImage)))
            {
                pitem->iSelectedImage = pitem->iImage;
            }

	    Release(psi);
            return;
        }
	Release(psi);
    }

    {
        // slow way...

        LPITEMIDLIST pidlFull = _GetFullIDList(pns->hwndTree, pitem->hItem);
	if (pidlFull)
	{
	    SHFILEINFO sfi;

	    SHGetFileInfo((LPCSTR)pidlFull, 0, &sfi, sizeof(SHFILEINFO), SHGFI_PIDL | SHGFI_SYSICONINDEX); //  | SHGFI_SMALLICON
	    pitem->iImage = sfi.iIcon;

	    if (!(ulAttrs & SFGAO_FOLDER))
                pitem->iSelectedImage = pitem->iImage;
            else
            {
                SHGetFileInfo((LPCSTR)pidlFull, 0, &sfi, sizeof(SHFILEINFO), SHGFI_PIDL | SHGFI_OPENICON | SHGFI_SYSICONINDEX);
                pitem->iSelectedImage = sfi.iIcon;
            }

	    ILFree(pidlFull);
	}
    }
}


int _GetChildren(NSC *pns, IShellFolder *psf, LPCITEMIDLIST pidl, ULONG ulAttrs)
{
    int cChildren = 0;  // assume none

    if (ulAttrs & SFGAO_FOLDER)
    {
        if (pns->style & NSS_SHOWNONFOLDERS)
        {
            // there is no SFGAO_ bit that includes non folders so we need to enum
            IShellFolder *psfItem;
	    if (SUCCEEDED(psf->lpVtbl->BindToObject(psf, pidl, NULL, &IID_IShellFolder, &psfItem)))
            {
                // if we are showing non folders we have to do an enum to peek down at items below
                IEnumIDList *penum;
                DWORD grfFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS;

                if (pns->style & NSS_SHOWHIDDEN)
                    grfFlags |= SHCONTF_INCLUDEHIDDEN;

                if (SUCCEEDED(psfItem->lpVtbl->EnumObjects(psfItem, NULL, grfFlags, &penum)))
                {
                    UINT celt;
	            LPITEMIDLIST pidlTemp;

                    if (penum->lpVtbl->Next(penum, 1, &pidlTemp, &celt) == S_OK && celt == 1)
	            {
                        ILFree(pidlTemp);
                        cChildren = 1;
                    }
                    Release(penum);
                }
                Release(psfItem);
            }
        }
        else
        {
            // if just folders we can peek at the attributes
            ULONG ulAttrs = SFGAO_HASSUBFOLDER;
            psf->lpVtbl->GetAttributesOf(psf, 1, &pidl, &ulAttrs);

            cChildren = (ulAttrs & SFGAO_HASSUBFOLDER) ? 1 : 0;
        }
    }
    return cChildren;
}

void _OnGetDisplayInfo(NSC *pns, TV_DISPINFO *pnm)
{
    LPITEMIDLIST pidl = _CacheParentShellFolder(pns, pnm->item.hItem, (LPITEMIDLIST)pnm->item.lParam);

    Assert(pidl);

    if (pidl == NULL)
        return;

    Assert(pns->psfCache);

    Assert(pnm->item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN));

    if (pnm->item.mask & TVIF_TEXT)
    {
        STRRET str;
        pns->psfCache->lpVtbl->GetDisplayNameOf(pns->psfCache, pidl, SHGDN_INFOLDER, &str);

        StrRetToStrN(pnm->item.pszText, pnm->item.cchTextMax, &str, pidl);
    }

    // make sure we set the attributes for those flags that need them
    if (pnm->item.mask & (TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE))
    {
        ULONG ulAttrs = SFGAO_FOLDER;
        pns->psfCache->lpVtbl->GetAttributesOf(pns->psfCache, 1, &pidl, &ulAttrs);

        // Also see if this guy has any child folders
        if (pnm->item.mask & TVIF_CHILDREN)
        {
            pnm->item.cChildren = _GetChildren(pns, pns->psfCache, pidl, ulAttrs);
        }

        if (pnm->item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE))
        {
            // We now need to map the item into the right image index.
	    _GetIconIndex(pns, pidl, ulAttrs, &pnm->item);
        }
    }

    // force the treeview to store this so we don't get called back again
    pnm->item.mask |= TVIF_DI_SETITEM;	
}

// send up the sel changed to let clients enable/disable buttons, etc.

void _OnSelChanged(NSC *pns, LPNM_TREEVIEW pnm)
{

#if 0
    LPITEMIDLIST pidl;
    ULONG ulAttrs = SFGAO_FILESYSTEM;
    BYTE bType;

    // We only need to do anything if we only want to return File system
    // level objects.
    if ((pns->ulFlags & (BIF_RETURNONLYFSDIRS | BIF_RETURNFSANCESTORS | BIF_BROWSEFORPRINTER | BIF_BROWSEFORCOMPUTER)) == 0)
        goto NotifySelChange;

    // We need to get the attributes of this object...
    if (_CacheParentShellFolder(pns, pnm->itemNew.hItem, (LPITEMIDLIST)pnm->itemNew.lParam))
    {
        BOOL fEnable = TRUE;

        bType = SIL_GetType(pidl);
        if ((pns->ulFlags & (BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS)) != 0)
        {
            int i;
            // if this is the root pidl, then do a get attribs on 0
            // so that we'll get the attributes on the root, rather than
            // random returned values returned by FSFolder
            if (ILIsEmpty(pidl)) {
                i = 0;
            } else
                i = 1;

            pns->psfCache->lpVtbl->GetAttributesOf(pns->psfCache,
                                                      i, &pidl, &ulAttrs);

            fEnable = (((ulAttrs & SFGAO_FILESYSTEM) && (pns->ulFlags & BIF_RETURNONLYFSDIRS)) ||
                ((ulAttrs & SFGAO_FILESYSANCESTOR) && (pns->ulFlags & BIF_RETURNFSANCESTORS))) ||
                    ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SERVER);
        }
        else if ((pns->ulFlags & BIF_BROWSEFORCOMPUTER) != 0)
	{
            fEnable = ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SERVER);
	}
        else if ((pns->ulFlags & BIF_BROWSEFORPRINTER) != 0)
        {
            // Printers are of type Share and usage Print...
            fEnable = ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SHARE);
        }

        EnableWindow(GetDlgItem(pns->hwnd, IDOK), fEnable);
    }

NotifySelChange:
    if (pns->lpfn) 
    {
        pidl = _GetFullIDList(pns->hwndTree, pnm->itemNew.hItem);
        BFSFCallback(pns, BFFM_SELCHANGED, (LPARAM)pidl);
        ILFree(pidl);
    }
#endif
}

const char c_szCut[] = "cut";
const char c_szRename[] = "rename";

LRESULT _ContextMenu(NSC *pns, short x, short y)
{
    HTREEITEM hti;
    POINT ptPopup;	// in screen coordinate

    if (x == -1 && y == -1)
    {
	// Keyboard-driven: Get the popup position from the selected item.
        hti = TreeView_GetSelection(pns->hwndTree);
	if (hti)
	{
	    RECT rc;
	    //
	    // Note that TV_GetItemRect returns it in client coordinate!
	    //
	    TreeView_GetItemRect(pns->hwndTree, hti, &rc, TRUE);
	    ptPopup.x = (rc.left + rc.right) / 2;
	    ptPopup.y = (rc.top + rc.bottom) / 2;
	    MapWindowPoints(pns->hwndTree, HWND_DESKTOP, &ptPopup, 1);
	}
    }
    else
    {
        TV_HITTESTINFO tvht;

	// Mouse-driven: Pick the treeitem from the position.
	ptPopup.x = x;
	ptPopup.y = y;

	tvht.pt = ptPopup;
	ScreenToClient(pns->hwndTree, &tvht.pt);

	hti = TreeView_HitTest(pns->hwndTree, &tvht);
    }

    if (hti)
    {
        LPCITEMIDLIST pidl = _CacheParentShellFolder(pns, hti, NULL);
	if (pidl)
	{
            IContextMenu *pcm;

	    TreeView_SelectDropTarget(pns->hwndTree, hti);

            if (SUCCEEDED(pns->psfCache->lpVtbl->GetUIObjectOf(pns->psfCache, pns->hwnd, 1, &pidl, &IID_IContextMenu, NULL, &pcm)))
            {
                HMENU hmenu = CreatePopupMenu();
                if (hmenu)
                {
                    UINT idCmd;

		    pns->pcm = pcm; // for IContextMenu2 code

                    pcm->lpVtbl->QueryContextMenu(pcm, hmenu, 0, 1, 0x7fff,
			    CMF_EXPLORE | CMF_CANRENAME);

                    // use pns->hwnd so menu msgs go there and I can forward them
                    // using IContextMenu2 so "Sent To" works

                    idCmd = TrackPopupMenu(hmenu,
                        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                        ptPopup.x, ptPopup.y, 0, pns->hwnd, NULL);

	    	    if (idCmd)
                    {
			char szCommandString[64];
                        BOOL fHandled = FALSE;
			BOOL fCutting = FALSE;

			// We need to special case the rename command
			if (SUCCEEDED(pcm->lpVtbl->GetCommandString(pcm, idCmd - 1,
                            0, NULL, szCommandString, sizeof(szCommandString))))
			{
			    if (lstrcmpi(szCommandString, c_szRename)==0) 
			    {
                                TreeView_EditLabel(pns->hwndTree, hti);
                                fHandled = TRUE;
                            } 
			    else if (!lstrcmpi(szCommandString, c_szCut)) 
			    {
				fCutting = TRUE;
                            }
			}

                        if (!fHandled)
			{
                            CMINVOKECOMMANDINFO ici = {
                                sizeof(CMINVOKECOMMANDINFO),
                                0L,
                                pns->hwndTree,
                                MAKEINTRESOURCE(idCmd - 1),
                                NULL, NULL,
                                SW_NORMAL,
                            };

			    HRESULT hres = pcm->lpVtbl->InvokeCommand(pcm, &ici);
			    if (fCutting && SUCCEEDED(hres))
			    {
				TV_ITEM tvi;
				tvi.mask = TVIF_STATE;
				tvi.stateMask = TVIS_CUT;
				tvi.state = TVIS_CUT;
				tvi.hItem = hti;
				TreeView_SetItem(pns->hwndTree, &tvi);

				// pns->hwndNextViewer = SetClipboardViewer(pns->hwndTree);
				// pns->htiCut = hti;
			    }
			}
	    	    }
	            DestroyMenu(hmenu);
		    pns->pcm = NULL;
                }
                ReleaseLast(pcm);
            }
	    TreeView_SelectDropTarget(pns->hwndTree, NULL);
        }
    }
    return 0;
}


LRESULT _OnBeginLabelEdit(NSC *pns, TV_DISPINFO *ptvdi)
{
    BOOL fCantRename = TRUE;

    LPCITEMIDLIST pidl = _CacheParentShellFolder(pns, ptvdi->item.hItem, NULL);
    if (pidl)
    {
	DWORD dwAttribs = SFGAO_CANRENAME;
	pns->psfCache->lpVtbl->GetAttributesOf(pns->psfCache, 1, &pidl, &dwAttribs);
	if (dwAttribs & SFGAO_CANRENAME)
	    fCantRename = FALSE;
    }

    if (fCantRename)
        MessageBeep(0);

    return fCantRename;
}

LRESULT _OnEndLabelEdit(NSC *pns, TV_DISPINFO *ptvdi)
{
    LPCITEMIDLIST pidl;

    // See if the user cancelled
    if (ptvdi->item.pszText == NULL)
        return TRUE;       // Nothing to do here.

    Assert(ptvdi->item.hItem);

    pidl = _CacheParentShellFolder(pns, ptvdi->item.hItem, NULL);
    if (pidl)
    {
	UINT cch = lstrlen(ptvdi->item.pszText)+1;
	LPOLESTR pwsz = (LPOLESTR)LocalAlloc(LPTR, cch * sizeof(WCHAR));
	if (pwsz)
	{
	    StrToOleStrN(pwsz, cch, ptvdi->item.pszText, -1);

	    if (SUCCEEDED(pns->psfCache->lpVtbl->SetNameOf(pns->psfCache, pns->hwnd, pidl, pwsz, 0, NULL)))
	    {
		// SHChangeNotifyHandleEvents();

		// NOTES: pidl is no longer valid here.
	    
		//
		// Set the handle to NULL in the notification to let
		// the system know that the pointer is probably not
		// valid anymore.
		//
		ptvdi->item.hItem = NULL;
	    }
	    else
	    {
		SendMessage(pns->hwndTree, TVM_EDITLABEL, (WPARAM)ptvdi->item.pszText, (LPARAM)ptvdi->item.hItem);
	    }
	    LocalFree((HLOCAL)pwsz);
        }
    }

    return 0;	// We always return 0, "we handled it".
}

void _OnBeginDrag(NSC *pns, NM_TREEVIEW *pnmhdr)
{
    LPCITEMIDLIST pidl = _CacheParentShellFolder(pns, pnmhdr->itemNew.hItem, NULL);
    if (pidl)
    {
	DWORD dwEffect = DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;

	pns->psfCache->lpVtbl->GetAttributesOf(pns->psfCache, 1, &pidl, &dwEffect);

	dwEffect &= DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK;

	if (dwEffect)
	{
	    IDataObject *pdtobj;
	    HRESULT hres = pns->psfCache->lpVtbl->GetUIObjectOf(pns->psfCache, pns->hwnd, 1, &pidl, &IID_IDataObject, NULL, &pdtobj);

    	    if (SUCCEEDED(hres))
	    {
		hres = OleInitialize(NULL);

		if (SUCCEEDED(hres))
		{
		    IDropSource *pdsrc;

		    if (SUCCEEDED(CDropSource_CreateInstance(&pdsrc)))
		    {
		        DWORD dwRet;

                        pns->htiDragging = pnmhdr->itemNew.hItem;

			DoDragDrop(pdtobj, pdsrc, dwEffect, &dwRet);

                        pns->htiDragging = NULL;

			DebugMsg(DM_TRACE, "DoDragDrop returns dwRet: %d", dwRet);

			Release(pdsrc);
		    }
		    OleUninitialize();
		}
    #if 0
		HIMAGELIST himlDrag = TreeView_CreateDragImage(pns->hwndTree, pnmhdr->itemNew.hItem);
		if (himlDrag) 
		{
		    if (DAD_SetDragImage(himlDrag, NULL))
		    {
			SHDoDragDrop(hwndOwner, pdtobj, NULL, dwEffect, &dwEffect);

			DAD_SetDragImage((HIMAGELIST)-1, NULL);
		    }
		    else
		    {
			DebugMsg(DM_TRACE, "sh ER - Tree_OnBeginDrag DAD_SetDragImage failed");
			Assert(0);
		    }
		    ImageList_Destroy(himlDrag);
		}
    #endif
		ReleaseLast(pdtobj);
	    }
	}
    }
}

void _InvokeContextMenu(IShellFolder *psf, LPCITEMIDLIST pidl, HWND hwnd, LPCSTR pszVerb)
{
    IContextMenu *pcm;
    if (SUCCEEDED(psf->lpVtbl->GetUIObjectOf(psf, hwnd, 1, &pidl, &IID_IContextMenu, NULL, &pcm)))
    {
        HMENU hmenu = CreatePopupMenu();
        if (hmenu)
        {
            pcm->lpVtbl->QueryContextMenu(pcm, hmenu, 0, 1, 255, pszVerb ? 0 : CMF_DEFAULTONLY);
            if (pszVerb == NULL)
                pszVerb = MAKEINTRESOURCE(GetMenuDefaultItem(hmenu, MF_BYCOMMAND, 0) - 1);
            if (pszVerb)
            {
                CMINVOKECOMMANDINFOEX ici = {
                    sizeof(CMINVOKECOMMANDINFOEX),
                    0L,
                    hwnd,
                    pszVerb,
                    NULL, NULL,
                    SW_NORMAL,
                };

                pcm->lpVtbl->InvokeCommand(pcm, (LPCMINVOKECOMMANDINFO)&ici);
            }
            DestroyMenu(hmenu);
        }
        Release(pcm);
    }
}

void _DoVerb(NSC *pns, HTREEITEM hti, LPCSTR pszVerb)
{
    hti = hti ? hti : TreeView_GetSelection(pns->hwndTree);
    if (hti)
    {
        LPCITEMIDLIST pidl = _CacheParentShellFolder(pns, hti, NULL);
	if (pidl)
	{
            _InvokeContextMenu(pns->psfCache, pidl, pns->hwnd, pszVerb);
        }
    }
}

BOOL _DoDlbClick(NSC *pns)
{
    HTREEITEM hti = TreeView_GetSelection(pns->hwndTree);
    if (hti)
    {
        LPCITEMIDLIST pidl = _CacheParentShellFolder(pns, hti, NULL);
        if (pidl)
        {
            ULONG ulAttrs = SFGAO_FOLDER;
            pns->psfCache->lpVtbl->GetAttributesOf(pns->psfCache, 1, &pidl, &ulAttrs);

            if (ulAttrs & SFGAO_FOLDER)
                return FALSE;       // do default action (expand/collapse)

            _InvokeContextMenu(pns->psfCache, pidl, pns->hwnd, NULL);
        }
    }
    return TRUE;
}


HTREEITEM _GetNodeFromIDList(LPITEMIDLIST pidl)
{
    return NULL;
}

BOOL TryQuickRename(LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra)
{
#if 0
    LPOneTreeNode lpnSrc;
    BOOL fRet = FALSE;

    // This can happen when a folder is moved from a "rooted" Explorer outside
    // of the root
    if (!pidl || !pidlExtra)
        return FALSE;

    // this one was deleted
    lpnSrc = _GetNodeFromIDList(pidl, 0);
    if (!lpnSrc)
        return FALSE;

    if (lpnSrc == s_lpnRoot) 
    {
        OTInvalidateRoot();
        return TRUE;
    } 
    else 
    {
        // this one was created
        LPITEMIDLIST pidlClone = ILClone(pidlExtra);
        if (pidlClone) 
        {
            LPOneTreeNode lpnDestParent;

            ILRemoveLastID(pidlClone);

            // if the parent isn't created yet, let's not bother
            lpnDestParent = _GetNodeFromIDList(pidlClone, 0);
            ILFree(pidlClone);

            if (lpnDestParent) 
            {
                LPITEMIDLIST pidlLast = OTGetRealFolderIDL(lpnDestParent, ILFindLastID(pidlExtra));
                if (pidlLast) 
                {
                    LPSHELLFOLDER psf = OTBindToFolder(lpnDestParent);
                    if (psf) 
                    {
                        OTAddRef(lpnSrc); // addref because AdoptKid doesn't and OTAbandonKid releases

                        // remove lpnSrc from its parent's list.
                        OTAbandonKid(lpnSrc->lpnParent, lpnSrc);

                        // invalidate the new node's parent to get any children flags right
                        OTInvalidateNode(lpnDestParent);

                        // free any cached folders
                        OTSweepFolders(lpnSrc);
                        SFCFreeNode(lpnSrc);

                        OTFreeNodeData(lpnSrc);
                        lpnSrc->pidl = pidlLast;
                        lpnSrc->lpnParent = lpnDestParent;
                        OTUpdateNodeName(psf, lpnSrc);
                        AdoptKid(lpnDestParent, lpnSrc);

                        fRet = TRUE;

                        IUnknown_Release(psf);
                    }
                    else
                    {
                        ILFree(pidlLast);
                    }
                }
            }
        }
    }
    return fRet;
#else
    return FALSE;
#endif
}

void _DoChangeNotify(NSC *pns, LONG lEvent, LPITEMIDLIST pidl, LPITEMIDLIST pidlExtra)
{
#if 0
    switch(lEvent)
    {
    case SHCNE_RENAMEFOLDER:
        // first try to just swap the nodes if it's  true rename (not a move)
        if (!TryQuickRename(pidl, pidlExtra))
        {
            // Rename is special.  We need to invalidate both
            // the pidl and the pidlExtra. so we call ourselves
            _DoHandleChangeNotify(pns, 0, pidlExtra, NULL);
        }
        break;

    case SHCNE_RMDIR:
        if (ILIsEmpty(pidl)) {
            // we've deleted the desktop dir.
            lpNode = s_lpnRoot;
            OTInvalidateRoot();
            break;
        }

        // Sitemaps are "inserted" items. We need the ability to remove them.
        // Unless the "fInserted" flag is reset, we can not remove them.
        if(lpNode = _GetNodeFromIDList(pidl, 0))
        {
            if(lpNode->fInserted)
                lpNode->fInserted = FALSE;
            lpNode = OTGetParent(lpNode);
            break;
        }

    case 0:
    case SHCNE_MKDIR:
    case SHCNE_DRIVEADD:
    case SHCNE_DRIVEREMOVED:
        if (pidl)
        {
            LPITEMIDLIST pidlClone = ILClone(pidl);
            if (pidlClone)
            {
                ILRemoveLastID(pidlClone);
                lpNode = _GetNodeFromIDList(pidlClone, 0);
                ILFree(pidlClone);
            }
        }
        break;

    case SHCNE_MEDIAINSERTED:
    case SHCNE_MEDIAREMOVED:
        lpNode = _GetNodeFromIDList(pidl, 0);
        if (lpNode)
            lpNode = lpNode->lpnParent;
        break;

    case SHCNE_DRIVEADDGUI:
    case SHCNE_UPDATEITEM:
    case SHCNE_NETSHARE:
    case SHCNE_NETUNSHARE:
    case SHCNE_UPDATEDIR:
        lpNode = _GetNodeFromIDList(pidl, 0);
        break;

    case SHCNE_SERVERDISCONNECT:
        // nuke all our kids and mark ourselves invalid
        lpNode = _GetNodeFromIDList(pidl, 0);
        if (lpNode && NodeHasKids(lpNode))
        {
            int i;

            for (i = GetKidCount(lpNode) -1; i >= 0; i--) {
                OTRelease(GetNthKid(lpNode, i));
            }
            DPA_Destroy(lpNode->hdpaKids);
            lpNode->hdpaKids = KIDSUNKNOWN;
            OTInvalidateNode(lpNode);
            SFCFreeNode(lpNode);
        } else {
            lpNode = NULL;
        }
        break;

    case SHCNE_ASSOCCHANGED:
        break;

    case SHCNE_UPDATEIMAGE:
        if (pidl) {
            InvalidateImageIndices();
            DoInvalidateAll(s_lpnRoot, *(int UNALIGNED *)((BYTE*)pidl + 2));
        }
        break;
    }
#endif
}

#if 0
void _OnChangeNotify(NSC *pns, WPARAM wParam, LPARAM lParam)
{
    LPITEMIDLIST *ppidl;
    LONG lEvent;
    LPSHChangeNotificationLock pshcnl;

    pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
    if (pshcnl)
    {
        _DoChangeNotify(pns, lEvent, ppidl[0], ppidl[1]);

        SHChangeNotification_Unlock(pshcnl);
    }
}
#endif

LRESULT CALLBACK NameSpaceWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    NSC *pns = (NSC *)GetWindowLong(hwnd, 0);

    switch (uMsg) {
    case WM_NCCREATE:
        Assert(pns == NULL);
        return _OnNCCreate(hwnd, (LPCREATESTRUCT)lParam);

    case WM_NCDESTROY:
        if (pns)
            _OnNCDestroy(pns);
        break;

    case WM_SIZE:
        if (pns->hwndTree)
	    MoveWindow(pns->hwndTree, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
	break;

    case WM_STYLECHANGED:
        if (pns && wParam == GWL_STYLE)
            pns->style = ((LPSTYLESTRUCT)lParam)->styleNew;
        break;

    case WM_SETFOCUS:
        if (pns && pns->hwndTree)
            SetFocus(pns->hwndTree);
        break;

    case WM_SETFONT:
    case WM_GETFONT:
	if (pns && pns->hwndTree)
	    return SendMessage(pns->hwndTree, uMsg, wParam, lParam);
        break;

    case WM_NOTIFY:
        Assert(((NMHDR *)lParam)->idFrom == ID_CONTROL);

        switch (((NMHDR *)lParam)->code) {

        // we track this through WM_CONTEXTMENU
        // case NM_RCLICK:

        case NM_RETURN:
        case NM_DBLCLK:
            return _DoDlbClick(pns);

        case TVN_GETDISPINFO:
            _OnGetDisplayInfo(pns, (TV_DISPINFO *)lParam);
            break;

        case TVN_ITEMEXPANDING:
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            _OnItemExpanding(pns, (LPNM_TREEVIEW)lParam);
            break;

        case TVN_ITEMEXPANDED:
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            break;

        case TVN_DELETEITEM:
            _OnDeleteItem(pns, (LPNM_TREEVIEW)lParam);
            break;

        case TVN_SELCHANGED:
            _OnSelChanged(pns, (LPNM_TREEVIEW)lParam);
            break;

	case TVN_BEGINLABELEDIT:
	    return _OnBeginLabelEdit(pns, (TV_DISPINFO *)lParam);

	case TVN_ENDLABELEDIT:
	    return _OnEndLabelEdit(pns, (TV_DISPINFO *)lParam);

	case TVN_BEGINDRAG:
	case TVN_BEGINRDRAG:
	    _OnBeginDrag(pns, (NM_TREEVIEW *)lParam);
	    break;
        }
        break;

    case WM_CONTEXTMENU:
        _ContextMenu(pns, (short)LOWORD(lParam), (short)HIWORD(lParam));
	break;

    case WM_INITMENUPOPUP:
    case WM_DRAWITEM:
    case WM_MEASUREITEM:
        if (pns->pcm)
	{
            IContextMenu2 *pcm2;
            if (SUCCEEDED(QueryInterface(pns->pcm, &IID_IContextMenu2, &pcm2)))
            {
                pcm2->lpVtbl->HandleMenuMsg(pcm2, uMsg, wParam, lParam);
                Release(pcm2);
            }
	}
	break;

    case WM_CHANGENOTIFY:
        #define ppidl ((LPITEMIDLIST *)wParam)
        _DoChangeNotify(pns, (LONG)lParam, ppidl[0], ppidl[1]);
        break;

    case NSM_SETROOT:
        return _OnSetRoot(pns, (NSC_SETROOT *)lParam);

    case NSM_GETIDLIST:
        if (wParam)
	    return (LRESULT)_GetFullIDList(pns->hwndTree, (HTREEITEM)lParam);
	else
	{
	    TV_ITEM tvi;

	    // now lets get the information about the item
	    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
	    tvi.hItem = (HTREEITEM)lParam;
	    if (!TreeView_GetItem(pns->hwndTree, &tvi))
	        return 0;

	    return (LRESULT)tvi.lParam;	// relative PIDL
	}
	break;

    case NSM_DOVERB:
        _DoVerb(pns, NULL, (LPCSTR)lParam);
        return TRUE;

    default:
    	return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

const char c_szNameSpaceClass[] = NAME_SPACE_CLASS;

BOOL NameSpace_RegisterClass(HINSTANCE hinst)
{
    WNDCLASS wc;

    InitCommonControls();

    if (!GetClassInfo(hinst, c_szNameSpaceClass, &wc)) 
    {
    	wc.lpfnWndProc     = NameSpaceWndProc;
    	wc.hCursor         = NULL;
    	wc.hIcon           = NULL;
    	wc.lpszMenuName    = NULL;
    	wc.hInstance       = hinst;
    	wc.lpszClassName   = c_szNameSpaceClass;
    	wc.hbrBackground   = NULL;
    	wc.style           = 0;
    	wc.cbWndExtra      = sizeof(NSC *);
    	wc.cbClsExtra      = 0;

        return RegisterClass(&wc);
    }
    return TRUE;
}


