//###########################################################################
// Code for the Browse For Starting Folder
//###########################################################################
// Structure to pass information to browse for folder dialog
typedef struct _bfsf
{
    HWND        hwndOwner;
    LPCITEMIDLIST pidlRoot;      // Root of search.  Typically desktop or my net
    LPSTR        pszDisplayName;// Return display name of item selected.
    int          *piImage;      // where to return the Image index.
    LPCSTR      lpszTitle;      // resource (or text to go in the banner over the tree.
    UINT         ulFlags;       // Flags that control the return stuff
    BFFCALLBACK  lpfn;
    LPARAM      lParam;
    HWND         hwndDlg;       // The window handle to the dialog
    HWND         hwndTree;      // The tree control.
    HTREEITEM    htiCurParent;  // tree item associated with Current shell folder
    IShellFolder * psfParent;    // Cache of the last IShell folder I needed...
    LPITEMIDLIST pidlCurrent;   // IDlist of current folder to select
    BOOL         fShowAllObjects; // Should we Show all ?
} BFSF, *PBFSF;


BOOL CALLBACK _BFSFDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);
LPITEMIDLIST _BFSFUpdateISHCache(PBFSF pbfsf, HTREEITEM hti, LPITEMIDLIST pidlItem);


// _BrowseForStartingFolder - Browse for a folder to start the
//         search from.



// BUGBUG, give them a way to turn off the ok button.

LPITEMIDLIST WINAPI SHBrowseForFolder(LPBROWSEINFO lpbi)
{
    LPITEMIDLIST lpRet;
    BFSF bfsf =
        {
          lpbi->hwndOwner,
          lpbi->pidlRoot,
          lpbi->pszDisplayName,
          &lpbi->iImage,
          lpbi->lpszTitle,
          lpbi->ulFlags,
          lpbi->lpfn,
          lpbi->lParam,
        };
    HCURSOR hcOld = SetCursor(LoadCursor(NULL,IDC_WAIT));
    SHELLSTATE ss;

    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
    bfsf.fShowAllObjects = ss.fShowAllObjects;

    // Now Create the dialog that will be doing the browsing.
    if (DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_BROWSEFORFOLDER),
                       lpbi->hwndOwner, _BFSFDlgProc, (LPARAM)&bfsf))
        lpRet = bfsf.pidlCurrent;
    else
        lpRet = NULL;

    if (hcOld)
        SetCursor(hcOld);

    return lpRet;
}

void BFSFCallback(PBFSF pbfsf, UINT uMsg, LPARAM lParam)
{
    if (pbfsf->lpfn) {
        pbfsf->lpfn(pbfsf->hwndDlg, uMsg, lParam, pbfsf->lParam);
    }
}


// Some helper functions for processing the dialog

HTREEITEM _AddItemToTree(HWND hwndTree, HTREEITEM htiParent, LPITEMIDLIST pidl, int cChildren)
{
    TV_INSERTSTRUCT tii;

    // Initialize item to add with callback for everything
    tii.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE |
            TVIF_PARAM | TVIF_CHILDREN;
    tii.hParent = htiParent;
    tii.hInsertAfter = TVI_FIRST;
    tii.item.iImage = I_IMAGECALLBACK;
    tii.item.iSelectedImage = I_IMAGECALLBACK;
    tii.item.pszText = LPSTR_TEXTCALLBACK;   //
    tii.item.cChildren = cChildren; //  Assume it has children
    tii.item.lParam = (LPARAM)pidl;

    return TreeView_InsertItem(hwndTree, &tii);
}


LPITEMIDLIST _GetIDListFromTreeItem(HWND hwndTree, HTREEITEM hti)
{
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlT;
    TV_ITEM tvi;

    // If no hti passed in, get the selected on.
    if (hti == NULL)
    {
        hti = TreeView_GetSelection(hwndTree);
        if (hti == NULL)
            return(NULL);
    }

    // now lets get the information about the item
    tvi.mask = TVIF_PARAM | TVIF_HANDLE;
    tvi.hItem = hti;
    if (!TreeView_GetItem(hwndTree, &tvi))
        return NULL;   // Failed again

    pidl = ILClone((LPITEMIDLIST)tvi.lParam);

    // Now walk up parents.
    while ((tvi.hItem = TreeView_GetParent(hwndTree, tvi.hItem)) && pidl)
    {
        if (!TreeView_GetItem(hwndTree, &tvi))
            return(pidl);   // will assume I screwed up...
        pidlT = ILCombine((LPITEMIDLIST)tvi.lParam, pidl);

        ILFree(pidl);

        pidl = pidlT;

    }
    return(pidl);
}


int CALLBACK _BFSFTreeCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    IShellFolder *psfParent = (IShellFolder *)lParamSort;
    HRESULT hres = psfParent->lpVtbl->CompareIDs(psfParent, 0, (LPITEMIDLIST)lParam1, (LPITEMIDLIST)lParam2);

    Assert(SUCCEEDED(hres));

    return (short)SCODE_CODE(GetScode(hres));
}

void _BFSFSort(PBFSF pbfsf, HTREEITEM hti, IShellFolder * psf)
{
    TV_SORTCB sSortCB;

    sSortCB.hParent = hti;
    sSortCB.lpfnCompare = _BFSFTreeCompare;

    psf->lpVtbl->AddRef(psf);
    sSortCB.lParam = (LPARAM)psf;
    TreeView_SortChildrenCB(pbfsf->hwndTree, &sSortCB, FALSE);
    psf->lpVtbl->Release(psf);
}


BOOL _BFSFHandleItemExpanding(PBFSF pbfsf, LPNM_TREEVIEW pnmtv)
{
    LPITEMIDLIST pidlToExpand;
    LPITEMIDLIST pidl;
    IShellFolder * psf;
    IShellFolder * psfDesktop = Desktop_GetShellFolder(TRUE);
    BYTE bType;
    DWORD grfFlags;
    BOOL fPrinterTest = FALSE;
    int cAdded = 0;
    TV_ITEM tvi;

    IEnumIDList *   penum;              // Enumerator in use.

    if (pnmtv->action != TVE_EXPAND)
        return FALSE;

    if ((pnmtv->itemNew.state & TVIS_EXPANDEDONCE))
        return FALSE;

    // set this bit now because we might be reentered via the wnet apis
    tvi.mask = TVIF_STATE;
    tvi.hItem = pnmtv->itemNew.hItem;
    tvi.state = TVIS_EXPANDEDONCE;
    tvi.stateMask = TVIS_EXPANDEDONCE;
    TreeView_SetItem(pbfsf->hwndTree, &tvi);


    if (pnmtv->itemNew.hItem == NULL)
    {
        pnmtv->itemNew.hItem = TreeView_GetSelection(pbfsf->hwndTree);
        if (pnmtv->itemNew.hItem == NULL)
            return FALSE;
    }

    pidlToExpand = _GetIDListFromTreeItem(pbfsf->hwndTree, pnmtv->itemNew.hItem);

    if (pidlToExpand == NULL)
        return FALSE;

    // Now lets get the IShellFolder and iterator for this object
    // special case to handle if the Pidl is the desktop
    // This is rather gross, but the desktop appears to be simply a pidl
    // of length 0 and ILIsEqual will not work...
    if (pidlToExpand->mkid.cb == 0)
    {
        psf = psfDesktop;
        psfDesktop->lpVtbl->AddRef(psf);
    }
    else
    {
        if (FAILED(psfDesktop->lpVtbl->BindToObject(psfDesktop,
                pidlToExpand, NULL, &IID_IShellFolder, &psf)))
        {
            ILFree(pidlToExpand);
            return FALSE; // Could not get IShellFolder.
        }
    }

    // Need to do a couple of special cases here to allow us to
    // browse for a network printer.  In this case if we are at server
    // level we then need to change what we search for non folders when
    // we are the level of a server.
    if (pbfsf->ulFlags & BIF_BROWSEFORPRINTER)
    {
        grfFlags = SHCONTF_FOLDERS | SHCONTF_NETPRINTERSRCH;
        pidl = ILFindLastID(pidlToExpand);
        bType = SIL_GetType(pidl);
        fPrinterTest = ((bType & (SHID_NET|SHID_INGROUPMASK))==SHID_NET_SERVER);
        if (fPrinterTest)
            grfFlags |= SHCONTF_NONFOLDERS;
    }
    else
        grfFlags = SHCONTF_FOLDERS;

    if (pbfsf->fShowAllObjects)
        grfFlags |= SHCONTF_INCLUDEHIDDEN;



    if (FAILED(psf->lpVtbl->EnumObjects(psf, pbfsf->hwndDlg, grfFlags, &penum)))
    {
        psf->lpVtbl->Release(psf);
        ILFree(pidlToExpand);
        return FALSE;
    }
    // psf->lpVtbl->AddRef(psf);

    while (pidl = _NextIDL(psf, penum))
    {
        int cChildren = I_CHILDRENCALLBACK;  // Do call back for children
        //
        // We need to special case here in the netcase where we onlyu
        // browse down to workgroups...
        //
        //
        // Here is where I also need to special case to not go below
        // workgroups when the appropriate option is set.
        //
        bType = SIL_GetType(pidl);
        if ((pbfsf->ulFlags & BIF_DONTGOBELOWDOMAIN) && (bType & SHID_NET))
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

        else if ((pbfsf->ulFlags & BIF_BROWSEFORCOMPUTER) && (bType & SHID_NET))
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

        _AddItemToTree(pbfsf->hwndTree, pnmtv->itemNew.hItem,
                pidl, cChildren);
        cAdded++;
    }

    // Now Cleanup after ourself
    penum->lpVtbl->Release(penum);

    _BFSFSort(pbfsf, pnmtv->itemNew.hItem, psf);
    psf->lpVtbl->Release(psf);
    ILFree(pidlToExpand);

    // If we did not add anything we should update this item to let
    // the user know something happened.
    //
    if (cAdded == 0)
    {
        TV_ITEM tvi;
        tvi.mask = TVIF_CHILDREN | TVIF_HANDLE;   // only change the number of children
        tvi.hItem = pnmtv->itemNew.hItem;
        tvi.cChildren = 0;

        TreeView_SetItem(pbfsf->hwndTree, &tvi);

    }

    return TRUE;
}



void _BFSFHandleDeleteItem(PBFSF pbfsf, LPNM_TREEVIEW pnmtv)
{
    // We need to free the IDLists that we allocated previously
    if (pnmtv->itemOld.lParam != 0)
        ILFree((LPITEMIDLIST)pnmtv->itemOld.lParam);
}


LPITEMIDLIST _BFSFUpdateISHCache(PBFSF pbfsf, HTREEITEM hti,
        LPITEMIDLIST pidlItem)
{
    HTREEITEM htiParent;
    IShellFolder * psfDesktop = Desktop_GetShellFolder(TRUE);

    if (pidlItem == NULL)
        return(NULL);

    // Need to handle the root case here!
    htiParent = TreeView_GetParent(pbfsf->hwndTree, hti);
    if ((htiParent != pbfsf->htiCurParent) || (pbfsf->psfParent == NULL))
    {
        LPITEMIDLIST pidl;

        if (pbfsf->psfParent)
        {

            if (pbfsf->psfParent != psfDesktop)
                pbfsf->psfParent->lpVtbl->Release(pbfsf->psfParent);
            pbfsf->psfParent = NULL;
        }

        if (htiParent)
        {
            pidl = _GetIDListFromTreeItem(pbfsf->hwndTree, htiParent);
        }
        else
        {
            //
            // If No Parent then the item here is one of our roots which
            // should be fully qualified.  So try to get the parent by
            // decomposing the ID.
            //
            LPITEMIDLIST pidlT = (LPITEMIDLIST)ILFindLastID(pidlItem);
            if (pidlT != pidlItem)
            {
                pidl = ILClone(pidlItem);
                ILRemoveLastID(pidl);
                pidlItem = pidlT;
            }
            else
                pidl = NULL;
        }

        pbfsf->htiCurParent = htiParent;

        // If still NULL then we use root of evil...
        if (pidl == NULL || (pidl->mkid.cb == 0))
        {
            // Still one m
            pbfsf->psfParent = psfDesktop;

            if (pidl)
                ILFree(pidl);
        }

        else
        {
            psfDesktop->lpVtbl->BindToObject(psfDesktop,
                     pidl, NULL, &IID_IShellFolder, &pbfsf->psfParent);
            ILFree(pidl);

            if (pbfsf->psfParent == NULL)
                return NULL;
        }
    }
    return(ILFindLastID(pidlItem));
}



void _BFSFGetDisplayInfo(PBFSF pbfsf, TV_DISPINFO *pnm)
{
    TV_ITEM ti;
    LPITEMIDLIST pidlItem = (LPITEMIDLIST)pnm->item.lParam;

    if ((pnm->item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_TEXT | TVIF_CHILDREN)) == 0)
        return; // nothing for us to do here.

    pidlItem = _BFSFUpdateISHCache(pbfsf, pnm->item.hItem, pidlItem);

    ti.mask = 0;
    ti.hItem = (HTREEITEM)pnm->item.hItem;

    // They are asking for IconIndex.  See if we can find it now.
    // Once found update their list, such that they wont call us back for
    // it again.
    if (pnm->item.mask & (TVIF_IMAGE | TVIF_SELECTEDIMAGE))
    {
        // We now need to map the item into the right image index.
        ti.iImage = pnm->item.iImage = SHMapPIDLToSystemImageListIndex(
                pbfsf->psfParent, pidlItem, &ti.iSelectedImage);
        // we should save it back away to
        pnm->item.iSelectedImage = ti.iSelectedImage;
        ti.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    }
    // Also see if this guy has any child folders
    if (pnm->item.mask & TVIF_CHILDREN)
    {
        ULONG ulAttrs;

        ulAttrs = SFGAO_HASSUBFOLDER;
        pbfsf->psfParent->lpVtbl->GetAttributesOf(pbfsf->psfParent,
                1, &pidlItem, &ulAttrs);

        ti.cChildren = pnm->item.cChildren =
                (ulAttrs & SFGAO_HASSUBFOLDER)? 1 : 0;

        ti.mask |= TVIF_CHILDREN;

    }

    if (pnm->item.mask & TVIF_TEXT)
    {
        STRRET str;
        pbfsf->psfParent->lpVtbl->GetDisplayNameOf(pbfsf->psfParent,
                pidlItem, SHGDN_INFOLDER, &str);

        StrRetToStrN(pnm->item.pszText, pnm->item.cchTextMax, &str, pidlItem);
        ti.mask |= TVIF_TEXT;
        ti.pszText = pnm->item.pszText;
    }

    // Update the item now
    TreeView_SetItem(pbfsf->hwndTree, &ti);
}


void _BFSFHandleSelChanged(PBFSF pbfsf, LPNM_TREEVIEW pnmtv)
{
    LPITEMIDLIST pidl;
    ULONG ulAttrs = SFGAO_FILESYSTEM;
    BYTE bType;

    // We only need to do anything if we only want to return File system
    // level objects.
    if ((pbfsf->ulFlags & (BIF_RETURNONLYFSDIRS|BIF_RETURNFSANCESTORS|BIF_BROWSEFORPRINTER|BIF_BROWSEFORCOMPUTER)) == 0)
        goto NotifySelChange;

    // We need to get the attributes of this object...
    pidl = _BFSFUpdateISHCache(pbfsf, pnmtv->itemNew.hItem,
            (LPITEMIDLIST)pnmtv->itemNew.lParam);

    if (pidl)
    {
        BOOL fEnable;

        bType = SIL_GetType(pidl);
        if ((pbfsf->ulFlags & (BIF_RETURNFSANCESTORS|BIF_RETURNONLYFSDIRS)) != 0)
        {
            int i;
            // if this is the root pidl, then do a get attribs on 0
            // so that we'll get the attributes on the root, rather than
            // random returned values returned by FSFolder
            if (ILIsEmpty(pidl)) {
                i = 0;
            } else
                i = 1;

            pbfsf->psfParent->lpVtbl->GetAttributesOf(pbfsf->psfParent,
                                                      i, &pidl, &ulAttrs);

            fEnable = (((ulAttrs & SFGAO_FILESYSTEM) && (pbfsf->ulFlags & BIF_RETURNONLYFSDIRS)) ||
                ((ulAttrs & SFGAO_FILESYSANCESTOR) && (pbfsf->ulFlags & BIF_RETURNFSANCESTORS))) ||
                    ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SERVER);
        }
        else if ((pbfsf->ulFlags & BIF_BROWSEFORCOMPUTER) != 0)
            fEnable = ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SERVER);
        else if ((pbfsf->ulFlags & BIF_BROWSEFORPRINTER) != 0)
        {
            // Printers are of type Share and usage Print...
            fEnable = ((bType & (SHID_NET | SHID_INGROUPMASK)) == SHID_NET_SHARE);
        }

        EnableWindow(GetDlgItem(pbfsf->hwndDlg, IDOK),fEnable);

    }

NotifySelChange:
    if (pbfsf->lpfn) {
        pidl = _GetIDListFromTreeItem(pbfsf->hwndTree, pnmtv->itemNew.hItem);
        BFSFCallback(pbfsf, BFFM_SELCHANGED, (LPARAM)pidl);
        ILFree(pidl);
    }
}

BOOL BrowseSelectPidl(PBFSF pbfsf, LPCITEMIDLIST pidl)
{
    HTREEITEM htiParent;
    LPITEMIDLIST pidlTemp;
    LPITEMIDLIST pidlNext = NULL;
    LPITEMIDLIST pidlParent = NULL;
    BOOL fRet = FALSE;

    htiParent = TreeView_GetChild(pbfsf->hwndTree, NULL);
    if (htiParent) {

        // step through each item of the pidl
        for (;;) {
            TreeView_Expand(pbfsf->hwndTree, htiParent, TVE_EXPAND);
            pidlParent = _GetIDListFromTreeItem(pbfsf->hwndTree, htiParent);
            if (!pidlParent)
                break;

            pidlNext = ILClone(pidl);
            if (!pidlNext)
                break;

            pidlTemp = ILFindChild(pidlParent, pidlNext);
            if (!pidlTemp)
                break;

            if (ILIsEmpty(pidlTemp)) {
                // found it!
                //
                TreeView_SelectItem(pbfsf->hwndTree, htiParent);
                fRet = TRUE;
                break;
            } else {
                // loop to find the next item
                HTREEITEM htiChild;

                pidlTemp = ILGetNext(pidlTemp);
                if (!pidlTemp)
                    break;
                else
                    pidlTemp->mkid.cb = 0;


                htiChild = TreeView_GetChild(pbfsf->hwndTree, htiParent);
                while (htiChild) {
                    BOOL fEqual;
                    pidlTemp = _GetIDListFromTreeItem(pbfsf->hwndTree, htiChild);
                    if (!pidlTemp) {
                        htiChild = NULL;
                        break;
                    }
                    fEqual = ILIsEqual(pidlTemp, pidlNext);

                    ILFree(pidlTemp);
                    if (fEqual) {
                        break;
                    } else {
                        htiChild = TreeView_GetNextSibling(pbfsf->hwndTree, htiChild);
                    }
                }

                if (!htiChild) {
                    // we didn't find the next one... bail
                    break;
                } else {
                    // the found child becomes the next parent
                    htiParent = htiChild;
                    ILFree(pidlParent);
                    ILFree(pidlNext);
                }
            }

        }
    }

    if (pidlParent) ILFree(pidlParent);
    if (pidlNext) ILFree(pidlNext);
    return fRet;
}


// _BFSFOnInitDlg - Process the init dialog

BOOL _BFSFOnInitDlg(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    HTREEITEM hti;
    PBFSF pbfsf = (PBFSF)lParam;
    HIMAGELIST himl;
    LPSTR lpsz;
    char szTitle[80];    // no title should be bigger than this!
    HWND hwndTree;

    lpsz = ResourceCStrToStr(HINST_THISDLL, pbfsf->lpszTitle);
    SetDlgItemText(hwnd, IDD_BROWSETITLE, lpsz);
    if (lpsz != pbfsf->lpszTitle)
    {
        LocalFree(lpsz);
        lpsz = NULL;
    }


    SetWindowLong(hwnd, DWL_USER, (LONG)lParam);
    pbfsf->hwndDlg = hwnd;
    hwndTree = pbfsf->hwndTree = GetDlgItem(hwnd, IDD_FOLDERLIST);

    if (hwndTree)
    {
        UINT swpFlags = SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER
                | SWP_NOACTIVATE;
        RECT rc;
        POINT pt = {0,0};

        if (!(pbfsf->ulFlags & BIF_STATUSTEXT)) {
            HWND hwndStatus = GetDlgItem(hwnd, IDD_BROWSESTATUS);
            // nuke the status window
            ShowWindow(hwndStatus, SW_HIDE);
            MapWindowPoints(hwndStatus, hwnd, &pt, 1);
            GetClientRect(hwndTree, &rc);
            MapWindowPoints(hwndTree, hwnd, (POINT*)&rc, 2);
            rc.top = pt.y;
            swpFlags =  SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE;
        }

        Shell_GetImageLists(NULL, &himl);
	TreeView_SetImageList(hwndTree, himl, TVSIL_NORMAL);

        SetWindowLong(hwndTree, GWL_EXSTYLE,
                GetWindowLong(hwndTree, GWL_EXSTYLE) | WS_EX_CLIENTEDGE);

        // Now try to get this window to know to recalc
        SetWindowPos(hwndTree, NULL, rc.left, rc.top,
                     rc.right - rc.left, rc.bottom - rc.top, swpFlags);

    }

    // If they passed in a root, add it, else add the contents of the
    // Root of evil... to the list as ROOT objects.
    if (pbfsf->pidlRoot)
    {
        LPITEMIDLIST pidl;
        if (!HIWORD(pbfsf->pidlRoot)) {
            pidl = SHCloneSpecialIDList(NULL, (UINT)pbfsf->pidlRoot, TRUE);
        } else {
            pidl = ILClone(pbfsf->pidlRoot);
        }
        // Now lets insert the Root object
	hti = _AddItemToTree(hwndTree, TVI_ROOT, pidl, 1);
	// Still need to expand below this point. to the starting location
	// That was passed in. But for now expand the first level.
	TreeView_Expand(hwndTree, hti, TVE_EXPAND);
    }
    else
    {
        LPCITEMIDLIST pidlDrives = GetSpecialFolderIDList(NULL, CSIDL_DRIVES, FALSE);
	LPITEMIDLIST pidlDesktop = SHCloneSpecialIDList(NULL, CSIDL_DESKTOP, FALSE);
	HTREEITEM htiRoot = _AddItemToTree(hwndTree, TVI_ROOT, pidlDesktop, 1);

        // Expand the first level under the desktop
        TreeView_Expand(hwndTree, htiRoot, TVE_EXPAND);

        // Lets Preexpand the Drives portion....
        hti = TreeView_GetChild(hwndTree, htiRoot);
        while (hti)
        {
            LPITEMIDLIST pidl = _GetIDListFromTreeItem(hwndTree, hti);
            if (ILIsEqual(pidl, pidlDrives))
            {

                TreeView_Expand(hwndTree, hti, TVE_EXPAND);

                TreeView_SelectItem(hwndTree, hti);
                ILFree(pidl);
                break;
            }
            ILFree(pidl);
            hti = TreeView_GetNextSibling(hwndTree, hti);
        }
    }

    // go to our internal selection changed code to do any window enabling needed
    {
        NM_TREEVIEW nmtv;
        hti = TreeView_GetSelection(hwndTree);
        if (hti) {
            TV_ITEM ti;
            ti.mask = TVIF_PARAM;
            ti.hItem = hti;
            TreeView_GetItem(hwndTree, &ti);
            nmtv.itemNew.hItem = hti;
            nmtv.itemNew.lParam = ti.lParam;

            _BFSFHandleSelChanged(pbfsf, &nmtv);
        }
    }

    if ((pbfsf->ulFlags & BIF_BROWSEFORCOMPUTER) != 0)
    {
        LoadString(HINST_THISDLL, IDS_FINDSEARCH_COMPUTER, szTitle, sizeof(szTitle));
        SetWindowText(hwnd, szTitle);
    }
    else if ((pbfsf->ulFlags & BIF_BROWSEFORPRINTER) != 0)
    {
        LoadString(HINST_THISDLL, IDS_FINDSEARCH_PRINTER, szTitle, sizeof(szTitle));
        SetWindowText(hwnd, szTitle);
    }

    BFSFCallback(pbfsf, BFFM_INITIALIZED, 0);

    return TRUE;
}

void _BFSFSetStatusText(PBFSF pbfsf, LPCSTR lpszText)
{
    char szText[100];
    if (!HIWORD(lpszText)) {
        LoadString(HINST_THISDLL, LOWORD(lpszText), szText, sizeof(szText));
        lpszText = szText;
    }
    SetDlgItemText(pbfsf->hwndDlg, IDD_BROWSESTATUS, lpszText);
}



// _BFSFOnCommand - Process the WM_COMMAND message

void _BFSFOnCommand(PBFSF pbfsf, int id, HWND hwndCtl, UINT codeNotify)
{
    HTREEITEM hti;
    switch (id)
    {
    case IDOK:
        // We can now update the structure with the idlist of the item selected
        hti = TreeView_GetSelection(pbfsf->hwndTree);
        pbfsf->pidlCurrent = _GetIDListFromTreeItem(pbfsf->hwndTree,
                hti);
        if (pbfsf->pszDisplayName || pbfsf->piImage)
        {
            TV_ITEM tvi;
            tvi.mask = (pbfsf->pszDisplayName)? (TVIF_TEXT | TVIF_IMAGE) :
                    TVIF_IMAGE;
            tvi.hItem = hti;
            tvi.pszText = pbfsf->pszDisplayName;
            tvi.cchTextMax = MAX_PATH;
            TreeView_GetItem(pbfsf->hwndTree, &tvi);

            if (pbfsf->piImage)
                *pbfsf->piImage = tvi.iImage;
        }
        EndDialog(pbfsf->hwndDlg, 1);     // To return TRUE.
        break;
    case IDCANCEL:
        EndDialog(pbfsf->hwndDlg, 0);     // to return FALSE from this.
        break;
    }
}




// _BSFSDlgProc - The dialog procedure for processing the browse
//          for starting folder dialog.

#pragma data_seg(".text", "CODE")
const static DWORD aBrowseHelpIDs[] = {  // Context Help IDs
    IDD_BROWSETITLE,  NO_HELP,
    IDD_BROWSESTATUS, NO_HELP,
    IDD_FOLDERLIST,   IDH_BROWSELIST,

    0, 0
};
#pragma data_seg()

BOOL CALLBACK _BFSFDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    PBFSF pbfsf = (PBFSF)GetWindowLong(hwndDlg, DWL_USER);

    switch (msg) {
    HANDLE_MSG(pbfsf, WM_COMMAND, _BFSFOnCommand);
    HANDLE_MSG(hwndDlg, WM_INITDIALOG, _BFSFOnInitDlg);

    case WM_DESTROY:
        if (pbfsf->psfParent && (pbfsf->psfParent != Desktop_GetShellFolder(TRUE)))
        {
            pbfsf->psfParent->lpVtbl->Release(pbfsf->psfParent);
            pbfsf->psfParent = NULL;
        }
        break;

    case BFFM_SETSTATUSTEXT:
        _BFSFSetStatusText(pbfsf, (LPCSTR)lParam);
        break;

    case BFFM_SETSELECTION:
    {
        BOOL fRet;

        // wParam TRUE means path, not pidl
        if (wParam) {
            lParam = (LPARAM)SHSimpleIDListFromPath((LPSTR)lParam);
            if (!lParam)
                return FALSE;
        }
        fRet = BrowseSelectPidl(pbfsf, (LPITEMIDLIST)lParam);

        if (wParam)
            ILFree((LPITEMIDLIST)lParam);
        return fRet;
    }

    case BFFM_ENABLEOK:
        EnableWindow(GetDlgItem(hwndDlg, IDOK), lParam);
        break;


    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
        case TVN_GETDISPINFO:
            _BFSFGetDisplayInfo(pbfsf, (TV_DISPINFO *)lParam);
            break;

        case TVN_ITEMEXPANDING:
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            _BFSFHandleItemExpanding(pbfsf, (LPNM_TREEVIEW)lParam);
            break;

        case TVN_ITEMEXPANDED:
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            break;

        case TVN_DELETEITEM:
            _BFSFHandleDeleteItem(pbfsf, (LPNM_TREEVIEW)lParam);
            break;

        case TVN_SELCHANGED:
            _BFSFHandleSelChanged(pbfsf, (LPNM_TREEVIEW)lParam);
            break;
        }
        break;

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (DWORD)(LPSTR) aBrowseHelpIDs);
        break;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (DWORD)(LPVOID) aBrowseHelpIDs);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

