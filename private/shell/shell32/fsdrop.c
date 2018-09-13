#include "shellprv.h"
#pragma  hdrstop

#include "bookmk.h"
#include "fstreex.h"
#include "datautil.h"
#include "copy.h"
#include "_security.h"

STDAPI_(void) FS_PositionFileFromDrop(HWND hwnd, LPCTSTR pszFile, PDROPHISTORY pdh)
{
    LPITEMIDLIST pidl = SHSimpleIDListFromPath(pszFile);
    if (pidl)
    {
        LPITEMIDLIST pidlNew = ILFindLastID(pidl);
        SFM_SAP sap;
        HWND hwndView;
        
        SHChangeNotifyHandleEvents();
        
        hwndView = DV_HwndMain2HwndView(hwnd);
        
        //
        // Fill in some easy SAP fields first.
        //
        sap.uSelectFlags = SVSI_SELECT;
        sap.fMove = TRUE;
        sap.pidl = pidlNew;
        
        //
        // Now compute the x,y coordinates.
        // If we have a drop history, use it to determine the
        // next point.
        //
        if (pdh)
        {
            // fill in the anchor point first...
            if (!pdh->fInitialized)
            {
                ITEMSPACING is;
                
                ShellFolderView_GetDropPoint(hwnd, &pdh->ptOrigin);
                
                //
                // Compute the first point.
                //
                
                pdh->pt = pdh->ptOrigin;
                
                //
                // Compute the point deltas.
                //
                if (ShellFolderView_GetItemSpacing(hwnd, &is))
                {
                    pdh->cxItem = is.cxSmall;
                    pdh->cyItem = is.cySmall;
                    pdh->xDiv = is.cxLarge;
                    pdh->yDiv = is.cyLarge;
                    pdh->xMul = is.cxSmall;
                    pdh->yMul = is.cySmall;
                }
                else
                {
                    pdh->cxItem = g_cxIcon;
                    pdh->cyItem = g_cyIcon;
                    pdh->xDiv = pdh->yDiv = pdh->xMul = pdh->yMul = 1;
                }
                
                //
                // First point gets special flags.
                //
                sap.uSelectFlags |= SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS | SVSI_FOCUSED;
                
                //
                // We be initialized.
                //
                pdh->fInitialized = TRUE;
            }
            // if we have no list of offsets, then just inc by icon size..
            else if ( !pdh->pptOffset )
            {
                //
                // Simple computation of the next point.
                //
                pdh->pt.x += pdh->cxItem;
                pdh->pt.y += pdh->cyItem;
            }
            
            // do this after the above stuff so that we always get our position relative to the anchor
            // point, if we use the anchor point as the first one things get screwy...
            if (pdh->pptOffset)
            {
                //
                // Transform the old offset to our coordinates.
                //
                pdh->pt.x = ((pdh->pptOffset[pdh->iItem].x * pdh->xMul) / pdh->xDiv) + pdh->ptOrigin.x;
                pdh->pt.y = ((pdh->pptOffset[pdh->iItem].y * pdh->yMul) / pdh->yDiv) + pdh->ptOrigin.y;
            }
            
            //
            // Copy the next point from the drop history.
            //
            sap.pt = pdh->pt;
        }
        else
        {
            // Preinitialize this puppy in case the folder view doesn't
            // know what the drop point is (e.g., if it didn't come from
            // a drag/drop but rather from a paste or a ChangeNotify.)
            sap.pt.x = 0x7FFFFFFF;      // "don't know"
            sap.pt.y = 0x7FFFFFFF;

            //
            // Get the drop point, conveniently already in
            // defview's screen coordinates.
            //
            // pdv->bDropAnchor should be TRUE at this point,
            // see DefView's GetDropPoint() for details.
            //
            ShellFolderView_GetDropPoint(hwnd, &sap.pt);
            
            //
            // Only point gets special flags.
            //
            sap.uSelectFlags |= SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS | SVSI_FOCUSED;
        }
        
        SendMessage(hwndView, SVM_SELECTANDPOSITIONITEM, 1, (LPARAM)&sap);
        
        ILFree(pidl);
    }
}

void FS_FreeMoveCopyList(LPITEMIDLIST *ppidl, UINT cidl)
{
    UINT i;
    // free everything
    for (i = 0; i < cidl; i++) 
    {
        ILFree(ppidl[i]);
    }
    LocalFree(ppidl);
}

void FS_PositionItems(HWND hwndOwner, UINT cidl, const LPITEMIDLIST *ppidl, IDataObject *pdtobj, POINT *pptOrigin, BOOL fMove)
{
    SFM_SAP *psap;

    if (!ppidl || !IsWindow(hwndOwner))
        return;

    psap = GlobalAlloc(GPTR, SIZEOF(SFM_SAP) * cidl);
    if (psap) 
    {
        UINT i, cxItem, cyItem;
        int xMul, yMul, xDiv, yDiv;
        STGMEDIUM medium;
        POINT *pptItems = NULL;
        POINT pt;
        ITEMSPACING is;
        // select those objects;
        // this had better not fail
        HWND hwnd = DV_HwndMain2HwndView(hwndOwner);

        if (fMove)
        {
            FORMATETC fmte = {g_cfOFFSETS, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            if (SUCCEEDED(pdtobj->lpVtbl->GetData(pdtobj, &fmte, &medium)) &&
                medium.hGlobal)
            {
                pptItems = (POINT *)GlobalLock(medium.hGlobal);
                pptItems++; // The first point is the anchor
            }
            else
            {
                // By default, drop at (-g_cxIcon/2, -g_cyIcon/2), and increase
                // x and y by icon dimension for each icon
                pt.x = ((-3 * g_cxIcon) / 2) + pptOrigin->x;
                pt.y = ((-3 * g_cyIcon) / 2) + pptOrigin->y;
                medium.hGlobal = NULL;
            }

            if (ShellFolderView_GetItemSpacing(hwndOwner, &is))
            {
                xDiv = is.cxLarge;
                yDiv = is.cyLarge;
                xMul = is.cxSmall;
                yMul = is.cySmall;
                cxItem = is.cxSmall;
                cyItem = is.cySmall;
            }
            else
            {
                xDiv = yDiv = xMul = yMul = 1;
                cxItem = g_cxIcon;
                cyItem = g_cyIcon;
            }
        }

        for (i = 0; i < cidl; i++)
        {
            if (ppidl[i])
            {
                psap[i].pidl = ILFindLastID(ppidl[i]);
                psap[i].fMove = fMove;
                if (fMove)
                {
                    if (pptItems)
                    {
                        psap[i].pt.x = ((pptItems[i].x * xMul) / xDiv) + pptOrigin->x;
                        psap[i].pt.y = ((pptItems[i].y * yMul) / yDiv) + pptOrigin->y;
                    }
                    else
                    {
                        pt.x += cxItem;
                        pt.y += cyItem;
                        psap[i].pt = pt;
                    }
                }

                // do regular selection from all of the rest of the items
                psap[i].uSelectFlags = SVSI_SELECT;
            }
        }

        // do this special one for the first only
        psap[0].uSelectFlags = SVSI_SELECT | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS | SVSI_FOCUSED;

        SendMessage(hwnd, SVM_SELECTANDPOSITIONITEM, cidl, (LPARAM)psap);

        if (fMove && medium.hGlobal)
            ReleaseStgMediumHGLOBAL(NULL, &medium);

        GlobalFree(psap);
    }
}


void FS_MapName(void *hNameMappings, LPTSTR pszPath)
{
    int i;
    LPSHNAMEMAPPING pNameMapping;

    if (!hNameMappings)
        return;

    for (i = 0; (pNameMapping = SHGetNameMappingPtr(hNameMappings, i)) != NULL; i++)
    {
        if (lstrcmpi(pszPath, pNameMapping->pszOldPath) == 0)
        {
            lstrcpy(pszPath, pNameMapping->pszNewPath);
            break;
        }
    }
}



// convert null separated/terminated file list to array of pidls
int FileListToPidlList(LPCTSTR lpszFiles, void *hNameMappings, LPITEMIDLIST **pppidl)
{
    int nItems = CountFiles(lpszFiles);
    int i = 0;
    LPITEMIDLIST * ppidl = (void*)LocalAlloc(LPTR, nItems * SIZEOF(LPITEMIDLIST));
    if (ppidl)
    {
        *pppidl = ppidl;

        while (*lpszFiles)
        {
            TCHAR szPath[MAX_PATH];
            lstrcpy(szPath, lpszFiles);
            FS_MapName(hNameMappings, szPath);

            ppidl[i] = SHSimpleIDListFromPath(szPath);

            lpszFiles += lstrlen(lpszFiles) + 1;
            i++;
        }
    }
    return i;
}

// create the pidl array that contains the destination file names. this is
// done by taking the source file names, and translating them through the
// name mapping returned by the copy engine.
//
//
// in:
//      pszDir          desitination of operation
//      pdtobj          HIDA containg data object
//      hNameMappings   used to translate names
//
// out:
//      *pppidl         id array of length return value
//      # of items in pppida

int FS_CreateMoveCopyList(IDataObject *pdtobj, void *hNameMappings, LPITEMIDLIST **pppidl)
{
    int nItems = 0;
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    HRESULT hres = pdtobj->lpVtbl->GetData(pdtobj, &fmte, &medium);
    if (SUCCEEDED(hres))
    {
        HDROP hDrop = medium.hGlobal;
        nItems = DragQueryFile(hDrop, (UINT)-1, NULL, 0);
        *pppidl = (void*)LocalAlloc(LPTR, nItems * SIZEOF(LPITEMIDLIST));
        if (*pppidl)
        {
            int i;
            for (i=nItems-1; i >= 0; i--)
            {
                TCHAR szPath[MAX_PATH];
                DragQueryFile(hDrop, i, szPath, ARRAYSIZE(szPath));
                FS_MapName(hNameMappings, szPath);
                (*pppidl)[i] = SHSimpleIDListFromPath(szPath);
            }
        }

        ReleaseStgMedium(&medium);
    }
    return nItems;
}

//
// in:
//      pszPath         destination path of the operation
//      pszFiles        source files list, may be NULL
//      hNameMappings   name mappings result from the copy operaton
//

void FS_MoveSelectIcons(FSTHREADPARAM *pfsthp, void *hNameMappings, LPCTSTR pszFiles, BOOL fMove)
{
    LPITEMIDLIST *ppidl = NULL;
    int cidl;

    if (pszFiles) 
    {
        cidl = FileListToPidlList(pszFiles, hNameMappings, &ppidl);
    } 
    else 
    {
        cidl = FS_CreateMoveCopyList(pfsthp->pDataObj, hNameMappings, &ppidl);
    }

    if (ppidl)
    {
        FS_PositionItems(pfsthp->hwndOwner, cidl, ppidl, pfsthp->pDataObj, &pfsthp->ptDrop, fMove);
        FS_FreeMoveCopyList(ppidl, cidl);
    }
}

// this is the ILIsParent which matches up the desktop with the desktop directory.
BOOL FSILIsParent(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    LPITEMIDLIST pidlUse1, pidlUse2;
    BOOL fSame;

    pidlUse1 = SHLogILFromFSIL(pidl1);
    if (pidlUse1)
        pidl1 = pidlUse1;

    pidlUse2 = SHLogILFromFSIL(pidl2);
    if (pidlUse2)
        pidl2 = pidlUse2;

    fSame = ILIsParent(pidl1, pidl2, TRUE);

    if (pidlUse1)
        ILFree(pidlUse1);
    if (pidlUse2)
        ILFree(pidlUse2);

    return fSame;
}

// in:
//      pszDestDir      destination dir for new file names
//      pszDestSpecs    double null list of destination specs
//
// returns:
//      double null list of fully qualified destination file names to be freed
//      with LocalFree()
//

LPTSTR RemapDestNamesW(LPCTSTR pszDestDir, LPCWSTR pszDestSpecs)
{
    UINT cbDestSpec = lstrlen(pszDestDir) * SIZEOF(TCHAR) + SIZEOF(TCHAR);
    LPCWSTR pszTemp;
    LPTSTR pszRet;
    UINT cbAlloc = SIZEOF(TCHAR);       // for double NULL teriminaion of entire string

    // compute length of buffer to aloc
    for (pszTemp = pszDestSpecs; *pszTemp; pszTemp += lstrlenW(pszTemp) + 1)
    {
        // +1 for null teriminator
        cbAlloc += cbDestSpec + lstrlenW(pszTemp) * SIZEOF(TCHAR) + SIZEOF(TCHAR);
    }

    pszRet = LocalAlloc(LPTR, cbAlloc);
    if (pszRet)
    {
        LPTSTR pszDest = pszRet;

        for (pszTemp = pszDestSpecs; *pszTemp; pszTemp += lstrlenW(pszTemp) + 1)
        {
            // PathCombine requires dest buffer of MAX_PATH size or it'll rip in call
            // to PathCanonicalize (IsBadWritePtr)
            TCHAR szTempDest[MAX_PATH];
#ifdef UNICODE
            PathCombine(szTempDest, pszDestDir, pszTemp);
#else
            char szTemp[MAX_PATH];
            SHUnicodeToAnsi(pszTemp, szTemp, ARRAYSIZE(szTemp));
            PathCombine(szTempDest, pszDestDir, szTemp);
#endif
            lstrcpy(pszDest, szTempDest);
            pszDest += lstrlen(pszDest) + 1;

            ASSERT((UINT)((BYTE *)pszDest - (BYTE *)pszRet) < cbAlloc);
            ASSERT(*pszDest == 0);      // zero init alloc
        }
        ASSERT((LPTSTR)((BYTE *)pszRet + cbAlloc - SIZEOF(TCHAR)) >= pszDest);
        ASSERT(*pszDest == 0);  // zero init alloc

    }
    return pszRet;
}

LPTSTR RemapDestNamesA(LPCTSTR pszDestDir, LPCSTR pszDestSpecs)
{
    UINT cbDestSpec = lstrlen(pszDestDir) * SIZEOF(TCHAR) + SIZEOF(TCHAR);
    LPCSTR pszTemp;
    LPTSTR pszRet;
    UINT cbAlloc = SIZEOF(TCHAR);       // for double NULL teriminaion of entire string

    // compute length of buffer to aloc
    for (pszTemp = pszDestSpecs; *pszTemp; pszTemp += lstrlenA(pszTemp) + 1)
    {
        // +1 for null teriminator
        cbAlloc += cbDestSpec + lstrlenA(pszTemp) * SIZEOF(TCHAR) + SIZEOF(TCHAR);
    }

    pszRet = LocalAlloc(LPTR, cbAlloc);
    if (pszRet)
    {
        LPTSTR pszDest = pszRet;

        for (pszTemp = pszDestSpecs; *pszTemp; pszTemp += lstrlenA(pszTemp) + 1)
        {
            // PathCombine requires dest buffer of MAX_PATH size or it'll rip in call
            // to PathCanonicalize (IsBadWritePtr)
            TCHAR szTempDest[MAX_PATH];
#ifdef UNICODE
            WCHAR wszTemp[MAX_PATH];
            SHAnsiToUnicode(pszTemp, wszTemp, ARRAYSIZE(wszTemp));
            PathCombine(szTempDest, pszDestDir, wszTemp);
#else
            PathCombine(szTempDest, pszDestDir, pszTemp);
#endif
            lstrcpy(pszDest, szTempDest);
            pszDest += lstrlen(pszDest) + 1;

            ASSERT((UINT)((BYTE *)pszDest - (BYTE *)pszRet) < cbAlloc);
            ASSERT(*pszDest == 0);      // zero init alloc
        }
        ASSERT((LPTSTR)((BYTE *)pszRet + cbAlloc - SIZEOF(TCHAR)) >= pszDest);
        ASSERT(*pszDest == 0);  // zero init alloc

    }
    return pszRet;
}

BOOL HandleSneakernetDrop(FSTHREADPARAM *pfsthp, LPCITEMIDLIST pidlParent, LPCTSTR pszTarget);

void _HandleMoveOrCopy(FSTHREADPARAM *pfsthp, HDROP hDrop, LPCTSTR pszPath)
{
    DRAGINFO di;

    // BUGBUG: we don't deal with non TCHAR stuff here
    di.uSize = SIZEOF(di);
    if (!DragQueryInfo(hDrop, &di))
    {
        // NOTE: Win95/NT4 dont have this fix, you will fault if you hit this case!
        AssertMsg(FALSE, TEXT("hDrop contains the opposite TCHAR (UNICODE when on ANSI)"));
        return;
    }


    switch (pfsthp->dwEffect) {
    case DROPEFFECT_MOVE:

        if (pfsthp->fSameHwnd)
        {
            FS_MoveSelectIcons(pfsthp, NULL, NULL, TRUE);
            break;
        }

        // fall through...

    case DROPEFFECT_COPY:
        {
            SHFILEOPSTRUCT fo = {
                pfsthp->hwndOwner,
                (pfsthp->dwEffect == DROPEFFECT_COPY) ? FO_COPY : FO_MOVE,
                di.lpFileList,
                pszPath,
                FOF_WANTMAPPINGHANDLE | FOF_ALLOWUNDO
            };
            LPTSTR pszDestNames = NULL;
            STGMEDIUM medium;
            FORMATETC fmte = {g_cfFileNameMapW, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            HRESULT hres;

            // if they are in the same hwnd or to and from
            // the same directory, turn on the automatic rename on collision flag
            if (pfsthp->fSameHwnd)
            {
                fo.fFlags |=  FOF_RENAMEONCOLLISION;
            }
            else
            {
                LPIDA pida = DataObj_GetHIDA(pfsthp->pDataObj, &medium);
                if (pida)
                {
                    LPCITEMIDLIST pidlParent = IDA_GetIDListPtr(pida, (UINT)-1);

                    // make sure stuff immediately under the desktop
                    // compares ok to stuff under the desktop directory
                    if (pidlParent)
                    {
                        INT i;
                        BOOL fMoveToSame = FALSE;

                        for (i = 0; i < (INT)pida->cidl; i++) {
                            LPCITEMIDLIST pidl   = IDA_GetIDListPtr(pida, (UINT)i);
                            LPITEMIDLIST pidlAbs = ILCombine (pidlParent, pidl);
                            if (!pidlAbs)
                                continue;

                            // if we're doing keyboard cut/copy/paste
                            //  to and from the same directories
                            // This is needed for common desktop support - BobDay/EricFlo
                            if (FSILIsParent(pfsthp->pidl, pidlAbs))
                            {
                                if (pfsthp->dwEffect == DROPEFFECT_MOVE)
                                {
                                    // if they're the same, do nothing on move
                                    fMoveToSame = TRUE;
                                }
                                else
                                {
                                    // do rename on collision for copy;
                                    fo.fFlags |= FOF_RENAMEONCOLLISION;
                                }
                            }
                            ILFree(pidlAbs);
                        }

                        if (fMoveToSame)
                            goto DoneWithData;

                        // Handle sneaker-net for briefcase; did briefcase
                        // handle it?
                        if (HandleSneakernetDrop(pfsthp, pidlParent, pszPath))
                        {
                            // Yes; don't do anything
                            DebugMsg(TF_FSTREE, TEXT("Briefcase handled drop"));
                            HIDA_ReleaseStgMedium(pida, &medium);
                            goto Exit;
                        }
                    }
DoneWithData:
                    HIDA_ReleaseStgMedium(pida, &medium);
                }
            }

            // see if there is a rename mapping from recycle bin (or someone else)
            ASSERT(fmte.cfFormat == g_cfFileNameMapW);

            hres = pfsthp->pDataObj->lpVtbl->GetData(pfsthp->pDataObj, &fmte, &medium);
            if (hres != S_OK)
            {
                fmte.cfFormat = g_cfFileNameMapA;
                hres = pfsthp->pDataObj->lpVtbl->GetData(pfsthp->pDataObj, &fmte, &medium);
            }

            if (hres == S_OK)
            {
                DebugMsg(TF_FSTREE, TEXT("Got rename mapping"));

                if (fmte.cfFormat == g_cfFileNameMapW)
                    pszDestNames = RemapDestNamesW(pszPath, (LPWSTR)GlobalLock(medium.hGlobal));
                else
                    pszDestNames = RemapDestNamesA(pszPath, (LPSTR)GlobalLock(medium.hGlobal));

                if (pszDestNames)
                {
                    fo.pTo = pszDestNames;
                    fo.fFlags |= FOF_MULTIDESTFILES;
                    // HACK, this came from the recycle bin, don't allow undo
                    fo.fFlags &= ~FOF_ALLOWUNDO;
#ifdef DEBUG
                    {
                    UINT cFrom = 0, cTo = 0;
                    LPCTSTR pszTemp;
                    for (pszTemp = fo.pTo; *pszTemp; pszTemp += lstrlen(pszTemp) + 1)
                        cTo++;
                    for (pszTemp = fo.pFrom; *pszTemp; pszTemp += lstrlen(pszTemp) + 1)
                        cFrom++;

                    AssertMsg(cFrom == cTo, TEXT("dest count does not equal source"));
                    }
#endif
                }
                ReleaseStgMediumHGLOBAL(medium.hGlobal, &medium);
            }

            // Check if there were any errors
            if (SHFileOperation(&fo) == 0 && !fo.fAnyOperationsAborted)
            {
                if (pfsthp->fBkDropTarget)
                    ShellFolderView_SetRedraw(pfsthp->hwndOwner, 0);

                SHChangeNotifyHandleEvents();   // force update now
                if (pfsthp->fBkDropTarget) 
                {
                    FS_MoveSelectIcons(pfsthp, fo.hNameMappings, pszDestNames, pfsthp->fDragDrop);
                    ShellFolderView_SetRedraw(pfsthp->hwndOwner, TRUE);
                }
            }

            if (fo.hNameMappings)
                SHFreeNameMappings(fo.hNameMappings);

            if (pszDestNames)
            {
                LocalFree((HLOCAL)pszDestNames);

                // HACK, this usually comes from the bitbucket
                // but in our shell, we don't handle the moves from the source
                if (pfsthp->dwEffect == DROPEFFECT_MOVE)
                    BBCheckRestoredFiles(di.lpFileList);
            }
        }

        break;
    }
Exit:
    SHFree(di.lpFileList);
}

STDAPI_(VOID) FreeFSThreadParam(FSTHREADPARAM* pfsthp)
{
    ASSERT(NULL != pfsthp);
    ASSERT(NULL != pfsthp->pDataObj);

    ATOMICRELEASE(pfsthp->pDataObj);
    ATOMICRELEASE(pfsthp->pstmDataObj);

    ILFree(pfsthp->pidl);

    LocalFree((HLOCAL)pfsthp);
}

const UINT c_rgFolderShortcutTargets[] = {
    CSIDL_STARTMENU,
    CSIDL_COMMON_STARTMENU,
    CSIDL_PROGRAMS,
    CSIDL_COMMON_PROGRAMS,
    CSIDL_NETHOOD,
    -1
};


BOOL _ShouldCreateFolderShortcut(LPCTSTR pszFolder)
{
    return PathIsEqualOrSubFolderOf(c_rgFolderShortcutTargets, pszFolder);
}


//
// This is the entry of "drop thread"
//
DWORD CALLBACK FileDropTargetThreadProc(void *pv)
{
    FSTHREADPARAM *pfsthp = (FSTHREADPARAM *)pv;
    HRESULT hr = E_FAIL;

    // Sleep(10 * 1000);   // to debug async case

    if (pfsthp->pDataObj == NULL)
    {
        CoGetInterfaceAndReleaseStream(pfsthp->pstmDataObj, &IID_IDataObject, (void **)&pfsthp->pDataObj);
        pfsthp->pstmDataObj = NULL;
    }

    if (pfsthp->pDataObj)
    {
        STGMEDIUM medium;
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
        //
        // If the link is the only choice and this is a default drag & drop,
        // and it is not forced by the user, we should tell the user.
        //
        if (((pfsthp->grfKeyState & (MK_LBUTTON | MK_CONTROL | MK_SHIFT | MK_ALT)) ==
            MK_LBUTTON) && pfsthp->fLinkOnly)
        {
            //
            //  Note that we can not pass hwnd, because it might
            // not be activated.
            //

            // BUGBUG: This blocks the main UI thread.  Other shell windows don't paint
            // when this is up. -BryanSt It's necessary to move this call to a background
            // thread always.
            UINT idMBox = ShellMessageBox(HINST_THISDLL, pfsthp->hwndOwner,
                    MAKEINTRESOURCE(IDS_WOULDYOUCREATELINK),
                    MAKEINTRESOURCE(IDS_LINKTITLE),
                    MB_YESNO | MB_ICONQUESTION);

            ASSERT(pfsthp->dwEffect == DROPEFFECT_LINK);

            if (idMBox != IDYES)
                pfsthp->dwEffect = 0;
        }

        switch (pfsthp->dwEffect)
        {
        case DROPEFFECT_MOVE:
        case DROPEFFECT_COPY:

            hr = pfsthp->pDataObj->lpVtbl->GetData(pfsthp->pDataObj, &fmte, &medium);
            if (SUCCEEDED(hr))
            {
                AssertMsg((NULL != pfsthp->hwndOwner), TEXT("You are calling _HandleMoveOrCopy() with out an hwnd which will prevent us from displaying insert disk UI"));
                
                _HandleMoveOrCopy(pfsthp, (HDROP)medium.hGlobal, pfsthp->szPath);
                ReleaseStgMedium(&medium);
            }
            break;

        case DROPEFFECT_LINK:
            {
                int i;
                UINT uCreateFlags = 0;
                LPITEMIDLIST *ppidl;

                if (pfsthp->fBkDropTarget)
                {
                    i = DataObj_GetHIDACount(pfsthp->pDataObj);
                    ppidl = (void*)LocalAlloc(LPTR, SIZEOF(LPITEMIDLIST) * i);
                }
                else
                    ppidl = NULL;

                if (pfsthp->grfKeyState)
                    uCreateFlags = SHCL_USETEMPLATE;

                if (_ShouldCreateFolderShortcut(pfsthp->szPath))
                    uCreateFlags |= SHCL_MAKEFOLDERSHORTCUT;

                ShellFolderView_SetRedraw(pfsthp->hwndOwner, FALSE);
                // passing ppidl == NULL is correct in failure case
                hr = SHCreateLinks(pfsthp->hwndOwner, pfsthp->szPath, pfsthp->pDataObj, uCreateFlags, ppidl);
                if (ppidl)
                {
                    FS_PositionItems(pfsthp->hwndOwner, i, ppidl, pfsthp->pDataObj, &pfsthp->ptDrop, TRUE);
                    FS_FreeMoveCopyList(ppidl, i);
                }
                ShellFolderView_SetRedraw(pfsthp->hwndOwner, TRUE);
            }
            break;
        }

        if (SUCCEEDED(hr) && pfsthp->dwEffect)
        {
            DataObj_SetDWORD(pfsthp->pDataObj, g_cfLogicalPerformedDropEffect, pfsthp->dwEffect);
            DataObj_SetDWORD(pfsthp->pDataObj, g_cfPerformedDropEffect, pfsthp->dwEffect);
        }

        SHChangeNotifyHandleEvents();       // force update now
    }
    FreeFSThreadParam(pfsthp);

    CoFreeUnusedLibraries();

    return 0;
}

STDAPI_(BOOL) AllRegisteredPrograms(HDROP hDrop)
{
    UINT i;
    TCHAR szPath[MAX_PATH];

    for (i = 0; DragQueryFile(hDrop, i, szPath, ARRAYSIZE(szPath)); i++)
    {
        if (!PathIsRegisteredProgram(szPath))
            return FALSE;
    }
    return TRUE;
}

#ifdef SYNC_BRIEFCASE

BOOL IsBriefcaseRoot(IDataObject *pDataObj)
{
    BOOL bRet = FALSE;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pDataObj, &medium);
    if (pida)
    {
        // Is there a briefcase root in this pDataObj?
        IShellFolder2 *psf;
        LPCITEMIDLIST pidlParent = IDA_GetIDListPtr(pida, (UINT)-1);
        if (pidlParent &&
            SUCCEEDED(SHBindToObject(NULL, &IID_IShellFolder2, pidlParent, (void **)&psf)))
        {
            UINT i;
            for (i = 0; i < pida->cidl; i++) 
            {
                CLSID clsid;
                bRet = SUCCEEDED(GetItemCLSID(psf, IDA_GetIDListPtr(pida, i), &clsid)) &&
                        IsEqualCLSID(&clsid, &CLSID_Briefcase);
                if (bRet)
                    break;
            }
            psf->lpVtbl->Release(psf);
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return bRet;
}

//
// Returns:
//  If the data object does NOT contain HDROP -> "none"
//  else if the source is root or registered progam -> "link"
//   else if this is within a volume   -> "move"
//   else if this is a briefcase       -> "move"
//   else                              -> "copy"
//
DWORD _PickDefFSOperation(CIDLDropTarget *this)
{
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    DWORD dwDefEffect = 0;      // assume no HDROP
    STGMEDIUM medium;

    if (SUCCEEDED(this->pdtobj->lpVtbl->GetData(this->pdtobj, &fmte, &medium)))
    {
        TCHAR szPath[MAX_PATH], szFolder[MAX_PATH];

        CIDLDropTarget_GetPath(this, szFolder);
        DragQueryFile(medium.hGlobal, 0, szPath, ARRAYSIZE(szPath)); // focused item

        // Determine the default operation depending on the item.
        if (PathIsRoot(szPath) || AllRegisteredPrograms(medium.hGlobal))
        {
            dwDefEffect = DROPEFFECT_LINK;
        }
        else if (PathIsSameRoot(szPath, szFolder))
        {
            dwDefEffect = DROPEFFECT_MOVE;
        }
        else if (IsBriefcaseRoot(this->pdtobj))
        {
            // a briefcase is in the data object
            // default to "move" even if across volumes
            DebugMsg(TF_FSTREE, TEXT("FS::Drop the object is the briefcase"));
            dwDefEffect = DROPEFFECT_MOVE;
        }
        else
        {
            dwDefEffect = DROPEFFECT_COPY;
        }
        ReleaseStgMedium(&medium);
    }
    else // if (SUCCEEDED(...))
    {
        // GetData failed. Let's see if QueryGetData failed or not.

        if (SUCCEEDED(this->pdtobj->lpVtbl->QueryGetData(this->pdtobj, &fmte)))
        {
            // this means this data object has HDROP but can't
            // provide it until it is dropped. Let's assume we are copying.
            dwDefEffect = DROPEFFECT_COPY;
        }
    }
    return dwDefEffect;
}

//
// make sure that the default effect is among the allowed effects
//
DWORD _LimitDefaultEffect(DWORD dwDefEffect, DWORD dwEffectsAllowed)
{
    if (dwDefEffect & dwEffectsAllowed)
        return dwDefEffect;

    if (dwEffectsAllowed & DROPEFFECT_COPY)
        return DROPEFFECT_COPY;

    if (dwEffectsAllowed & DROPEFFECT_MOVE)
        return DROPEFFECT_MOVE;

    if (dwEffectsAllowed & DROPEFFECT_LINK)
        return DROPEFFECT_LINK;

    return DROPEFFECT_NONE;
}

// {F20DA720-C02F-11CE-927B-0800095AE340}
const GUID CLSID_CPackage = {0xF20DA720L, 0xC02F, 0x11CE, 0x92, 0x7B, 0x08, 0x00, 0x09, 0x5A, 0xE3, 0x40};
// old packager guid...
// {0003000C-0000-0000-C000-000000000046}
const GUID CLSID_OldPackage = {0x0003000CL, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46};

//
// This function returns the default effect.
// This function also modified *pdwEffect to indicate "available" operation.
//
DWORD CFSIDLDropTarget_GetDefaultEffect(CIDLDropTarget *this, DWORD grfKeyState, LPDWORD pdwEffectInOut, UINT *pidMenu)
{
    DWORD dwDefEffect;
    UINT idMenu = POPUP_NONDEFAULTDD;
    DWORD dwEffectAvail = 0;

    //
    // First try file system operation (HDROP).
    //
    if (this->dwData & DTID_HDROP)
    {
        //
        // If HDROP exists, ignore the rest of formats.
        //
        dwEffectAvail |= DROPEFFECT_COPY | DROPEFFECT_MOVE;

        //
        //  We don't support 'links' from HDROP (only from HIDA).
        // This is a known limitation and we have no plan to implement
        // it for Win95.
        //
        if (this->dwData & DTID_HIDA)
            dwEffectAvail |= DROPEFFECT_LINK;

        dwDefEffect = _PickDefFSOperation(this);

        ASSERT(dwDefEffect);
    }
    else
    {
        BOOL fContents = ((this->dwData & (DTID_CONTENTS | DTID_FDESCA)) == (DTID_CONTENTS | DTID_FDESCA) ||
                          (this->dwData & (DTID_CONTENTS | DTID_FDESCW)) == (DTID_CONTENTS | DTID_FDESCW));

        if (fContents || (this->dwData & DTID_HIDA))
        {
            if (this->dwData & DTID_HIDA)
            {
                dwEffectAvail = DROPEFFECT_LINK;
                dwDefEffect = DROPEFFECT_LINK;
            }

            if (fContents)
            {
                //
                // HACK: if there is a preferred drop effect and no HIDA
                // then just take the preferred effect as the available effects
                // this is because we didn't actually check the FD_LINKUI bit
                // back when we assembled dwData! (performance)
                //
                if ((this->dwData & (DTID_PREFERREDEFFECT | DTID_HIDA)) ==
                    DTID_PREFERREDEFFECT)
                {
                    dwEffectAvail = this->dwEffectPreferred;
                    // dwDefEffect will be set below
                }
                else if (this->dwData & DTID_FD_LINKUI)
                {
                    dwEffectAvail = DROPEFFECT_LINK;
                    dwDefEffect = DROPEFFECT_LINK;
                }
                else
                {
                    dwEffectAvail |= DROPEFFECT_COPY | DROPEFFECT_MOVE;
                    dwDefEffect = DROPEFFECT_COPY;
                }
                idMenu = POPUP_FILECONTENTS;
            }
        }
    }

    //
    // BUGBUG this should be moved to OLE's clipboard/dataobject code
    // (ie anybody provides these formats and OLE provides FILECONTENTS)
    // this will be more important as random containers get implemented
    //
    if (!dwEffectAvail)
    {
        BOOL fPackage = FALSE;
        if (this->dwData & DTID_EMBEDDEDOBJECT)
        {
            FORMATETC fmte = {g_cfObjectDescriptor, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
            STGMEDIUM medium;
            ASSERT(NULL != this->pdtobj);
            if (SUCCEEDED(this->pdtobj->lpVtbl->GetData(this->pdtobj, &fmte, &medium)))
            {
                // we've got an object descriptor
                OBJECTDESCRIPTOR* pOD = GlobalLock(medium.hGlobal);
                if (pOD)
                {
                    if (IsEqualGUID(&CLSID_OldPackage, &pOD->clsid) ||
                        IsEqualGUID(&CLSID_CPackage, &pOD->clsid))
                    {
                        dwEffectAvail |= DROPEFFECT_COPY | DROPEFFECT_MOVE;
                        dwDefEffect = DROPEFFECT_COPY;
                        idMenu = POPUP_EMBEDDEDOBJECT;
                        fPackage = TRUE;
                    }
                    GlobalUnlock(medium.hGlobal);
                }
                ReleaseStgMedium(&medium);
            }
        }
        if (!fPackage)
        {
            //
            // Try scrap and doc-shortcut
            //
            if (this->dwData & DTID_OLELINK)
            {
                dwEffectAvail |= DROPEFFECT_LINK;
                dwDefEffect = DROPEFFECT_LINK;
                idMenu = POPUP_SCRAP;
            }

            if (this->dwData & DTID_OLEOBJ)
            {
                dwEffectAvail |= DROPEFFECT_COPY | DROPEFFECT_MOVE;
                dwDefEffect = DROPEFFECT_COPY;
                idMenu = POPUP_SCRAP;
            }
        }
    }

    *pdwEffectInOut &= dwEffectAvail;

    //
    // Alter the default effect depending on modifier keys.
    //
    switch (grfKeyState & (MK_CONTROL | MK_SHIFT | MK_ALT))
    {
    case MK_CONTROL:            dwDefEffect = DROPEFFECT_COPY; break;
    case MK_SHIFT:              dwDefEffect = DROPEFFECT_MOVE; break;
    case MK_SHIFT | MK_CONTROL: dwDefEffect = DROPEFFECT_LINK; break;
    case MK_ALT:                dwDefEffect = DROPEFFECT_LINK; break;
    default:
        //
        // no modifier keys:
        // if the data object contains a preferred drop effect, try to use it
        //
        if (this->dwData & DTID_PREFERREDEFFECT)
        {
            DWORD dwPreferred = this->dwEffectPreferred & dwEffectAvail;

            if (dwPreferred)
            {
                if (dwPreferred & DROPEFFECT_MOVE)
                {
                    dwDefEffect = DROPEFFECT_MOVE;
                }
                else if (dwPreferred & DROPEFFECT_COPY)
                {
                    dwDefEffect = DROPEFFECT_COPY;
                }
                else if (dwPreferred & DROPEFFECT_LINK)
                {
                    dwDefEffect = DROPEFFECT_LINK;
                }
            }
        }
        break;
    }

    if (pidMenu)
        *pidMenu = idMenu;

    DebugMsg(TF_FSTREE, TEXT("CFSDT::GetDefaultEffect dwD=%x, *pdw=%x, idM=%d"),
             dwDefEffect, *pdwEffectInOut, idMenu);

    return _LimitDefaultEffect(dwDefEffect, *pdwEffectInOut);
}


BOOL IsInsideBriefcase(LPCITEMIDLIST pidlIn)
{
    BOOL bRet = FALSE;
    LPITEMIDLIST pidl = ILClone(pidlIn);
    if (pidl)
    {
        do
        {
            CLSID clsid;
            if (SUCCEEDED(GetCLSIDFromIDList(pidl, &clsid)) &&
                IsEqualCLSID(&clsid, &CLSID_Briefcase))
            {
                bRet = TRUE;    // it is a briefcase
                break;
            }
        } while (ILRemoveLastID(pidl));
        ILFree(pidl);
    }
    return bRet;
}


/*----------------------------------------------------------
Purpose: Determines if pidl listed in the hida is a briefcase
         on removable media

Returns: TRUE if the above is true

Cond:    --
*/
BOOL IsFromSneakernetBriefcase(LPCITEMIDLIST pidlSource, LPCTSTR pszTarget)
{
    BOOL bRet = FALSE;
    TCHAR szSource[MAX_PATH];

    if (SHGetPathFromIDList(pidlSource, szSource))
    {
        // is source on removable device?
        if (IsRemovableDrive(DRIVEID(szSource)))
        {
            // is the target fixed media?
            if (PathIsUNC(pszTarget) || !IsRemovableDrive(DRIVEID(pszTarget)))
            {
                bRet = IsInsideBriefcase(pidlSource);
            }
        }
    }
    return bRet;
}

BOOL HandleSneakernetDrop(FSTHREADPARAM *pfsthp, LPCITEMIDLIST pidlParent, LPCTSTR pszTarget)
{
    BOOL bRet = FALSE;

    ASSERT(pidlParent);
    ASSERT(pszTarget);

    // Is it being dragged from a mobile briefcase?
    if ((DROPEFFECT_COPY == pfsthp->dwEffect) && pfsthp->bSyncCopy)
    {
        // Yes
        IBriefcaseStg *pbrfstg;

        // Perform a sneakernet addition to the briefcase
        if (SUCCEEDED(BrfStg_CreateInstance(pidlParent, pfsthp->hwndOwner, &pbrfstg)))
        {
            // (Even if AddObject fails, return TRUE to prevent caller
            // from handling this)
            bRet = (S_FALSE != pbrfstg->lpVtbl->AddObject(pbrfstg, pfsthp->pDataObj, pszTarget,
                (DDIDM_SYNCCOPYTYPE == pfsthp->idCmd) ? AOF_FILTERPROMPT : AOF_DEFAULT,
                pfsthp->hwndOwner));
            pbrfstg->lpVtbl->Release(pbrfstg);
        }
    }
    return bRet;
}

extern BOOL DroppingAnyFolders(HDROP hDrop);

void SneakernetHook(IDataObject *pdtobj, LPCTSTR pszTarget, UINT *pidMenu, BOOL *pbSyncCopy)
{
    // Is this the sneakernet case?
    STGMEDIUM medium;
    LPIDA pida;

    // (Default: leave *pidMenu as it is passed in)
    *pbSyncCopy = FALSE;        // Default

    pida = DataObj_GetHIDA(pdtobj, &medium);
    if (pida)
    {
        LPCITEMIDLIST pidlParent = IDA_GetIDListPtr(pida, (UINT)-1);
        if (pidlParent)
        {
            if (IsFromSneakernetBriefcase(pidlParent, pszTarget))
            {
                // Yes; show the non-default briefcase cm
                FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
                STGMEDIUM mediumT;

                if (SUCCEEDED(pdtobj->lpVtbl->GetData(pdtobj, &fmte, &mediumT)))
                {
                    if (DroppingAnyFolders(mediumT.hGlobal))
                        *pidMenu = POPUP_BRIEFCASE_FOLDER_NONDEFAULTDD;   // Yes
                    else
                        *pidMenu = POPUP_BRIEFCASE_NONDEFAULTDD;          // No

                    *pbSyncCopy = TRUE;
                    ReleaseStgMedium(&mediumT);
                }
            }
        }
        HIDA_ReleaseStgMedium(pida, &medium);
    }
}

#endif


// BUGBUG:
//    This code has lots of problems.  We need to fix this the text time we touch this code
// outside of ship mode.  TO FIX:
// 1. Use SHAnsiToUnicode(CP_UTF8, ) to convert pszHTML to unicode.  This will allow international
//    paths to work.
// 2. Obey the selected range.
// 3. Use MSHTML to get the image.  You can have trident parse the HTML via IHTMLTxtRange::pasteHTML.
//    MS HTML has a special collection of images.  Ask for the first image in that collection, or
//    the first image in that collection within the selected range.  (#1 isn't needed with this)
BOOL ExtractImageURLFromCFHTML(IN LPSTR pszHTML, IN SIZE_T cbHTMLSize, OUT LPSTR szImg, IN DWORD dwSize)
{
    BOOL fSucceeded = FALSE;

    // NT #391669: pszHTML isn't terminated, so terminate it now.
    LPSTR pszCopiedHTML = (LPSTR) LocalAlloc(LPTR, cbHTMLSize + 1);
    if (pszCopiedHTML)
    {
        LPSTR szBase;
        LPSTR szImgSrc;
        LPSTR szTemp;
        DWORD dwLen = dwSize;
        BOOL  bRet = TRUE;
        LPSTR szImgSrcOrig;

        StrCpyNA(pszCopiedHTML, pszHTML, ((int)cbHTMLSize) + 1);

        //DANGER WILL ROBINSON:
        // HTML is comming in as UFT-8 encoded. Neither Unicode or Ansi,
        // We've got to do something.... I'm going to party on it as if it were
        // Ansi. This code will choke on escape sequences.....

        //Find the base URL
        //Locate <!--StartFragment-->
        //Read the <IMG SRC="
        //From there to the "> should be the Image URL
        //Determine if it's an absolute or relative URL
        //If relative, append to BASE url. You may need to lop off from the
        // last delimiter to the end of the string.

        //Pull out the SourceURL

        szBase = StrStrIA(pszCopiedHTML,"SourceURL:"); // Point to the char after :
        if(szBase)
        {
            szBase += sizeof("SourceURL:")-1;

            //Since each line can be terminated by a CR, CR/LF or LF check each case...
            szTemp = StrChrA(szBase,'\n');
            if(!szTemp)
                szTemp = StrChrA(szBase,'\r');

            if(szTemp)
                *szTemp = '\0';
            szTemp++;
        }
        else
            szTemp = pszCopiedHTML;


        //Pull out the Img Src
        szImgSrc = StrStrIA(szTemp,"IMG");
        if(szImgSrc != NULL)
        {
            szImgSrc = StrStrIA(szImgSrc,"SRC");
            if(szImgSrc != NULL)
            {
                szImgSrcOrig = szImgSrc;
                szImgSrc = StrChrA(szImgSrc,'\"');
                if(szImgSrc)
                {
                    szImgSrc++;     // Skip over the quote at the beginning of the src path.
                    szTemp = StrChrA(szImgSrc,'\"');    // Find the end of the path.
                }
                else
                {
                    LPSTR pszTemp1;
                    LPSTR pszTemp2;

                    szImgSrc = StrChrA(szImgSrcOrig,'=');
                    szImgSrc++;     // Skip past the equals to the first char in the path.
                                    // Someday we may need to handle spaces between '=' and the path.

                    pszTemp1 = StrChrA(szImgSrc,' ');   // Since the path doesn't have quotes around it, assume a space will terminate it.
                    pszTemp2 = StrChrA(szImgSrc,'>');   // Since the path doesn't have quotes around it, assume a space will terminate it.

                    szTemp = pszTemp1;      // Assume quote terminates path.
                    if (!pszTemp1)
                        szTemp = pszTemp2;  // Use '>' if quote not found.

                    if (pszTemp1 && pszTemp2 && (pszTemp2 < pszTemp1))
                        szTemp = pszTemp2;  // Change to having '>' terminate path if both exist and it comes first.
                }

                *szTemp = '\0'; // Terminate path.

                //At this point, I've reduced the 2 important strings. Now see if I need to
                //Join them.

                //If this fails, then we don't have a full URL, Only a relative.
                if(!UrlIsA(szImgSrc,URLIS_URL) && szBase)
                {
                    if(SUCCEEDED(UrlCombineA(szBase, szImgSrc, szImg, &dwLen,0)))
                        fSucceeded = TRUE;
                }
                else
                {
                    if(lstrlenA(szImgSrc) <= (int)dwSize)
                        lstrcpyA(szImg, szImgSrc);

                    fSucceeded = TRUE;
                }
            }
        }

        LocalFree(pszCopiedHTML);
    }

    return fSucceeded;
}
