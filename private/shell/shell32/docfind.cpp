#include "shellprv.h"

extern "C" {
#include <shellp.h>
#include "idlcomm.h"
#include "pidl.h"
#include "fstreex.h"
#include "views.h"
#include "ids.h"
#include "shitemid.h"
#include <limits.h>
};


#include "docfind.h"
#include "docfindx.h"

#include "sfviewp.h"
#include "shguidp.h"
#include "netview.h"
#ifdef WINNT
#include "mtpt.h"
#endif

// in defviewx.c
STDAPI SHGetIconFromPIDL(IShellFolder *psf, IShellIcon *psi, LPCITEMIDLIST pidl, UINT flags, int *piImage);

typedef struct
{
    IShellFolder *pshf;     // The shell folder
    UINT          iCol;     // Which Column
} DFSINFO;

int CALLBACK DFSCompareItems(void *p1, void *p2, LPARAM lParam);

#define FILE_ATTRIBUTE_SUPERHIDDEN (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN)
extern "C" BOOL IsSuperHidden(DWORD dwAttribs)
{
    BOOL bRet = FALSE;

    if (!ShowSuperHidden())
    {
        bRet = (dwAttribs & FILE_ATTRIBUTE_SUPERHIDDEN) == FILE_ATTRIBUTE_SUPERHIDDEN;
    }
    return bRet;
}

#ifdef WINNT

extern "C" HRESULT _IsNTFSDrive(int iDrive)
{
    HRESULT hr = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        hr = pMtPt->IsNTFS()? S_OK : S_FALSE;
        pMtPt->Release();
    }
    return hr;
}

#endif

HRESULT CDocFindSFVCB::OnMERGEMENU(DWORD pv, QCMINFO*lP)
{
    int i;

    if (m_pDFFolder->pdfff == NULL)
        return S_FALSE;

    DebugMsg(DM_TRACE, TEXT("sh TR - DF_FSNCallBack DVN_MERGEMENU"));

    UINT idBGMain = 0, idBGPopup = 0;
    m_pDFFolder->pdfff->GetFolderMergeMenuIndex(&idBGMain, &idBGPopup);
    CDefFolderMenu_MergeMenu(HINST_THISDLL, 0, idBGPopup, lP);

    // Lets remove some menu items that are not useful to us.
    HMENU hmenu = lP->hmenu;
    DeleteMenu(hmenu, SFVIDM_EDIT_PASTE, MF_BYCOMMAND);
    DeleteMenu(hmenu, SFVIDM_EDIT_PASTELINK, MF_BYCOMMAND);
    // DeleteMenu(hmenu, SFVIDM_EDIT_PASTESPECIAL, MF_BYCOMMAND);

    // This is sortof bogus but if after the merge one of the
    // menus has no items in it, remove the menu.

    for (i = GetMenuItemCount(hmenu)-1; i >= 0; i--)
    {
        HMENU hmenuSub;

        if ((hmenuSub = GetSubMenu(hmenu, i)) &&
                (GetMenuItemCount(hmenuSub) == 0))
        {
            DeleteMenu(hmenu, i, MF_BYPOSITION);
        }
    }
    return S_OK;
}

HRESULT CDocFindSFVCB::OnReArrange(DWORD pv, LPARAM lp)
{   
    DebugMsg(DM_TRACE, TEXT("sh TR - DF_FSNCallBack ONREARRANGE iCOl=%d,"), lp);
    return SortOnColumn((UINT)lp);
}

HRESULT CDocFindSFVCB::OnGETWORKINGDIR(DWORD pv, UINT wP, LPTSTR lP)
{
    HRESULT hres = E_FAIL;
    IShellFolderView *psfv;
    if (_punkSite && SUCCEEDED(_punkSite->QueryInterface(IID_IShellFolderView, (void**)&psfv)))
    {   
        LPCITEMIDLIST *ppidls;      // pointer to a list of pidls.
        UINT cpidls = 0;            // Count of pidls that were returned.

        psfv->GetSelectedObjects( &ppidls, &cpidls);
        
        if (cpidls > 0)
        {
            LPITEMIDLIST pidl;
            m_pDFFolder->GetParentsPIDL(ppidls[0], &pidl); 
            SHGetPathFromIDList(pidl, lP);
            LocalFree( (void *) ppidls );
            hres = S_OK;
        }
        else
        {
            hres = E_FAIL;
        }
        psfv->Release();
    }
    return hres;
}

HRESULT CDocFindSFVCB::OnINVOKECOMMAND(DWORD pv, UINT wP)
{
    DebugMsg(DM_TRACE, TEXT("sh TR - DF_FSNCallBack DVN_INVOKECOMMAND (id=%x)"), wP);
    INSTRUMENT_INVOKECOMMAND(SHCNFI_DEFFOLDER_FNV_INVOKE, m_hwndMain, wP);
    switch(wP)
    {
    case FSIDM_SORTBYLOCATION:
    case FSIDM_SORTBYNAME:
    case FSIDM_SORTBYSIZE:
    case FSIDM_SORTBYTYPE:
    case FSIDM_SORTBYDATE:
        return SortOnColumn(DFSortIDToICol(wP));
        break;
    }
    return(S_OK);
}

HRESULT CDocFindSFVCB::OnGETCOLSAVESTREAM(DWORD pv, WPARAM wP, IStream**lP)
{
    if (m_pDFFolder->pdfff == NULL) {
        *lP = NULL;
        return E_FAIL;
    }

    return(m_pDFFolder->pdfff->GetColSaveStream(wP, lP));
}

HRESULT CDocFindSFVCB::OnGETITEMIDLIST(DWORD pv, WPARAM iItem, LPITEMIDLIST *ppidl)
{
    ESFItem *pesfi;


    if (SUCCEEDED(m_pDFFolder->GetItem((int) iItem, &pesfi)) && pesfi)
    {
        *ppidl = &pesfi->idl;
        return S_OK;
    }

    *ppidl = NULL;
    return E_FAIL;
}

HRESULT CDocFindSFVCB::OnGetItemIconIndex(DWORD pv, WPARAM iItem, int *piIcon)
{
    ESFItem *pesfi;

    *piIcon = -1;
    
    if (SUCCEEDED(m_pDFFolder->GetItem((int) iItem, &pesfi)) && pesfi)
    {
        if (pesfi->iIcon == -1)
        {
            IShellFolder* psf = (IShellFolder*)m_pDFFolder;
            SHGetIconFromPIDL(psf, NULL, &pesfi->idl, 0, &pesfi->iIcon);
        }

        *piIcon = pesfi->iIcon;
        return S_OK;
    }

    return E_FAIL;
}


HRESULT CDocFindSFVCB::OnSetItemIconOverlay(DWORD pv, WPARAM iItem, int iOverlayIndex)
{
    HRESULT hres = E_FAIL;
    ESFItem *pesfi;
    if (SUCCEEDED(m_pDFFolder->GetItem((int) iItem, &pesfi)) && pesfi)
    {
        pesfi->dwMask |= ESFITEM_ICONOVERLAYSET;
        pesfi->dwState |= INDEXTOOVERLAYMASK(iOverlayIndex) & LVIS_OVERLAYMASK;
        hres = S_OK;
    }

    return hres;
}

HRESULT CDocFindSFVCB::OnGetItemIconOverlay(DWORD pv, WPARAM iItem, int * piOverlayIndex)
{
    HRESULT hres = E_FAIL;
    *piOverlayIndex = SFV_ICONOVERLAY_DEFAULT;
    ESFItem *pesfi;
    if (SUCCEEDED(m_pDFFolder->GetItem((int) iItem, &pesfi)) && pesfi)
    {
        if (pesfi->dwMask & ESFITEM_ICONOVERLAYSET)
        {
            *piOverlayIndex = OVERLAYMASKTO1BASEDINDEX(pesfi->dwState & LVIS_OVERLAYMASK);
        }
        else
            *piOverlayIndex = SFV_ICONOVERLAY_UNSET;
        hres = S_OK;
    }

    return hres;
    
}


HRESULT CDocFindSFVCB::OnSETITEMIDLIST(DWORD pv, WPARAM iItem, LPITEMIDLIST pidl)
{
    ESFItem *pesfi;

    m_pDFFolder->_iGetIDList = (int) iItem;   // remember the last one we retrieved...    

    if (SUCCEEDED(m_pDFFolder->GetItem((int) iItem, &pesfi)) && pesfi)
    {
        ESFItem *pesfiNew;
        
        if (SUCCEEDED(m_pDFFolder->AddPidl((int) iItem, pidl, 0, &pesfiNew) && pesfiNew)) 
        {
            pesfiNew->dwState = pesfi->dwState;
            LocalFree((HLOCAL)pesfi);   // Free the old one...
        }
        return S_OK;
    }

    return E_FAIL;
}

BOOL DF_ILIsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    BOOL bRet = (pidl1 == pidl2);

    if (!bRet)
    {
        PCHIDDENDOCFINDDATA phdfd1 = (PCHIDDENDOCFINDDATA) ILFindHiddenID(pidl1, IDLHID_DOCFINDDATA);
        PCHIDDENDOCFINDDATA phdfd2 = (PCHIDDENDOCFINDDATA) ILFindHiddenID(pidl2, IDLHID_DOCFINDDATA);

        if (phdfd1 && phdfd2)
            bRet = (phdfd1->iFolder == phdfd2->iFolder) && ILIsEqual(pidl1, pidl2);
    }
    return bRet;
}

HRESULT CDocFindSFVCB::OnGetIndexForItemIDList(DWORD pv, int * piItem, LPITEMIDLIST pidl)
{
    INT cItems;

    // Try to short circuit searching for pidls...
    if (SUCCEEDED(m_pDFFolder->GetItemCount(&cItems)) && m_pDFFolder->_iGetIDList < cItems)
    {
        ESFItem *pesfi;
                
        if (SUCCEEDED(m_pDFFolder->GetItem(m_pDFFolder->_iGetIDList, &pesfi)) && pesfi)
        {
            if (DF_ILIsEqual(&pesfi->idl, pidl))
            {
                // Yep it was ours so return the index quickly..
                *piItem = m_pDFFolder->_iGetIDList;
                return S_OK;
            }
        }
    }

    // Otherwise let it search the old fashion way...
    return E_FAIL;
}

HRESULT CDocFindSFVCB::OnDeleteItem(DWORD pv, LPCITEMIDLIST pidl)
{
    // We simply need to remove this item from our list.  The
    // underlying listview will decrement the count on their end...
    ESFItem *pesfi;
    int iItem;
    int cItems;
    BOOL bFound;

    if (!pidl)
    {
        m_pDFFolder->SetAsyncEnum(NULL);
        return NOERROR;     // special case telling us all items deleted...
    }

    bFound = FALSE;
    
    if (SUCCEEDED(m_pDFFolder->GetItem(m_pDFFolder->_iGetIDList, &pesfi)) 
        && pesfi
        && (DF_ILIsEqual(&pesfi->idl, pidl)))
    {
        iItem = m_pDFFolder->_iGetIDList;
        bFound = TRUE;
    }
    else
    {
        if (SUCCEEDED(m_pDFFolder->GetItemCount(&cItems))) {
            for (iItem = 0; iItem < cItems; iItem++)
            {                
                if (SUCCEEDED(m_pDFFolder->GetItem(iItem, &pesfi)) && pesfi && (DF_ILIsEqual(&pesfi->idl, pidl)))
                {
                    bFound = TRUE;
                    break;
                }
            }
        }
    }

    if (bFound)
    {
        m_pDFFolder->DeleteItem(iItem);
    }

    return NOERROR;
}


HRESULT CDocFindSFVCB::OnODFindItem(DWORD pv, int * piItem, NM_FINDITEM* pnmfi)
{
    DebugMsg(DM_TRACE, TEXT("sh TR - DF_FSNCallBack SFVM_ODFINDITEM"));
    // We have to do the subsearch ourself to find the correct item...
    // As the listview has no information saved in it...

    int iItem = pnmfi->iStart;
    int cItem;
    UINT flags = pnmfi->lvfi.flags;

    if (FAILED(m_pDFFolder->GetItemCount(&cItem))) 
        return E_FAIL;

    if ((flags & LVFI_STRING) == 0)
        return E_FAIL;      // Not sure what type of search this is...

    int cbString = lstrlen(pnmfi->lvfi.psz);

    for (int j = cItem; j-- != 0; )
    {
        if (iItem >= cItem)
        {
            if (flags & LVFI_WRAP)
                iItem = 0;
            else
                break;
        }

        // Now we need to get the Display name for this item...
        ESFItem *pesfi;
        STRRET strret;
        TCHAR szPath[MAX_PATH];
        IShellFolder* psf = (IShellFolder*)m_pDFFolder;

        if (SUCCEEDED(m_pDFFolder->GetItem(iItem, &pesfi)) && pesfi && SUCCEEDED(psf->GetDisplayNameOf(&pesfi->idl, NULL, &strret)) &&
            SUCCEEDED(StrRetToBuf(&strret, &pesfi->idl, szPath, ARRAYSIZE(szPath))))
        {
            if (flags & (LVFI_PARTIAL|LVFI_SUBSTRING))
            {
                if (CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                        pnmfi->lvfi.psz, cbString, szPath, cbString) == 2)
                {
                    *piItem = iItem;
                    return S_OK;
                }

            }
            else if (lstrcmpi(pnmfi->lvfi.psz, szPath) == 0)
            {
                *piItem = iItem;
                return S_OK;
            }
        }

        ++iItem;
    }
    return E_FAIL;
}

HRESULT CDocFindSFVCB::OnSelChange(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA*lP)
{
    // Try to remember which item is focused...
    if (lP->uNewState & LVIS_FOCUSED)
        m_iFocused = wPh;

    return(S_OK);
}

HRESULT CDocFindSFVCB::OnGetEmptyText(DWORD pv, UINT cchTextMax, LPTSTR pszText)
{
    if (cchTextMax && pszText)
    {
        if( *m_pDFFolder->_szEmptyText )
        {
            lstrcpyn( pszText, m_pDFFolder->_szEmptyText, cchTextMax ) ;
            return S_OK ;
        }

        if (!LoadString(HINST_THISDLL, IDS_FINDVIEWEMPTYINIT, 
                       pszText, cchTextMax))
        {
            ASSERT( FALSE ) ; // bad resource string identifier.
            return E_UNEXPECTED ;
        }
        return S_OK;
    }

    return E_INVALIDARG;
}

HRESULT CDocFindSFVCB::SortOnColumn(UINT wP)
{
    int iStart;
    int iEnd;

    // See if their is any controller object registered that may want to take over this...
    // if we are in a mixed query and we have already sucked up the async items, simply sort
    // the dpa's...
    IDFEnum *pidfenum;
    m_pDFFolder->GetAsyncEnum(&pidfenum);

    if (pidfenum && !((pidfenum->FQueryIsAsync() == DF_QUERYISMIXED) && m_pDFFolder->_fAllAsyncItemsCached))
    {
        if (m_pDFFolder->_pdfcn)
        {
            // if they return S_FALSE it implies that they handled it and they do not
            // want the default processing to happen...
            if (m_pDFFolder->_pdfcn->DoSortOnColumn(wP, m_iColSort == wP) == S_FALSE)
            {
                m_iColSort = wP;
                return S_OK;
            }
        }

        // If we are running in the ROWSET way, we may want to have the ROWSET do the work...
        else 
        {
            HRESULT hres = S_OK;
            // pass one we spawn off a new search with the right column sorted
            if (m_iColSort != wP)
            {
                m_iColSort = wP;      
            }
    
            // Warning the call above may release our AsyncEnum and generate a new one so
            // Don't rely on it's existence here...
            return hres;
        }
    }

    // we must pull in all the results from ci
    if (pidfenum && pidfenum->FQueryIsAsync() && !m_pDFFolder->_fAllAsyncItemsCached)
        m_pDFFolder->CacheAllAsyncItems();

#ifdef DEBUG
#define MAX_LISTVIEWITEMS  (100000000 & ~0xFFFF)
#define SANE_ITEMCOUNT(c)  ((int)min(c, MAX_LISTVIEWITEMS))

    if (pidfenum && pidfenum->FQueryIsAsync())
    {
        ASSERT(DPA_GetPtrCount(m_pDFFolder->hdpaItems) >= SANE_ITEMCOUNT(m_pDFFolder->_cAsyncItems));
        for (int i = 0; i < SANE_ITEMCOUNT(m_pDFFolder->_cAsyncItems); i++)
        {
            ESFItem *pesfi = (ESFItem *)DPA_GetPtr(m_pDFFolder->hdpaItems, i);

            ASSERT(pesfi);
            if (!pesfi)
            {
                ASSERT(SUCCEEDED(m_pDFFolder->GetItem(i, &pesfi)));
            }
        }
    }
#endif
    EnterCriticalSection(&m_pDFFolder->_csSearch);
    // First mark the focused item in the list so we can find it later...
    ESFItem *pesfi;
    pesfi = (ESFItem*)DPA_GetPtr(m_pDFFolder->hdpaItems, m_iFocused);    // indirect
    if (pesfi)
        pesfi->dwState |= LVIS_FOCUSED;

    // Being that we are in virtual mode now, we need to do the sort
    // internal...
    // First remember which item is focused(BUGBUG)
    HDPA hdpa = m_pDFFolder->hdpaItems;
    UINT cItems = DPA_GetPtrCount(hdpa);

    if (wP != m_iColSort || (m_pDFFolder->fItemsChangedSinceSort))
    {
        DFSINFO dfsinfo = {m_pshf, wP};
        DPA_Sort(hdpa, DFSCompareItems, (LPARAM)&dfsinfo);
        m_pDFFolder->fItemsChangedSinceSort = FALSE;
        m_iColSort = wP;      
    }
    else
    {
        // We simply need to reverse the items in the dpa
        iStart = 0;
        iEnd = (int)cItems - 1;
        while (iStart < iEnd)
        {
            void *pv = DPA_FastGetPtr(hdpa, iStart);
            DPA_SetPtr(hdpa, iStart, DPA_FastGetPtr(hdpa, iEnd));
            DPA_SetPtr(hdpa, iEnd, pv);
            iStart++;
            iEnd--;
        }
    }

    // Now find the focused item and scroll it into place...
    //
    
    IShellView *psv;
    if (_punkSite && SUCCEEDED(_punkSite->QueryInterface(IID_IShellView, (void **)&psv)))
    {
        int iFocused = -1;

        // Tell the view we need to reshuffle....
        // Gross, this one defaults to invalidate all which for this one is fine...
        DocFind_SetObjectCount(psv, DPA_GetPtrCount(hdpa), SFVSOC_INVALIDATE_ALL); // Invalidate all

        for (iEnd = DPA_GetPtrCount(hdpa)-1; iEnd >= 0; iEnd--)
        {
            pesfi = (ESFItem*)DPA_GetPtr(m_pDFFolder->hdpaItems, iEnd);    // indirect
            if (pesfi && pesfi->dwState & LVIS_FOCUSED)
                iFocused = iEnd;
        }
        // Now handle the focused item...
        if (iFocused != -1)
        {
            m_pDFFolder->_iGetIDList = iFocused;   // remember the last one we retrieved...
            pesfi = (ESFItem*)DPA_GetPtr(m_pDFFolder->hdpaItems, iFocused);    // indirect
            if (pesfi)
            {
                // flags depend on first one and also if selected?
                psv->SelectItem(&pesfi->idl, SVSI_FOCUSED | SVSI_ENSUREVISIBLE | SVSI_SELECT);
                pesfi->dwState &= ~LVIS_FOCUSED;    // don't keep it around to get lost later...
            }
        }

        m_iFocused = iFocused;

        // Release our use of the view...
        m_fIgnoreSelChange = FALSE;
        psv->Release();
    }
    LeaveCriticalSection(&m_pDFFolder->_csSearch);

    return S_OK;
}

// BUGBUG:: Need a cleaner way to handle getting listview first item and number of items on page...
HRESULT CDocFindSFVCB::OnWindowCreated(DWORD pv, HWND hwnd)
{
    HWND hwndT;
    LONG id;
    #define ID_LISTVIEW 1

    hwndT = GetWindow(hwnd, GW_CHILD);
    while(hwndT)
    {
        id = GetWindowLong(hwndT, GWL_ID);
        if (id == ID_LISTVIEW)
        {
            m_pDFFolder->_hwndLV = hwndT;   // remember the listview for later...
            break;
        }
        hwndT = GetWindow(hwndT, GW_HWNDNEXT);
    }

    return(S_OK);
}

HRESULT CDocFindSFVCB::OnWindowDestroy(DWORD pv, HWND wP)
{
    if (m_pDFFolder->_pdfcn)
        m_pDFFolder->_pdfcn->StopSearch();
    // The search may have a circular set of pointers.  So call the 
    // delete items and folders here to remove these back references...
    m_pDFFolder->ClearItemList();
    m_pDFFolder->ClearFolderList();

    IDocFindControllerNotify *pdfcn;
    if (m_pDFFolder->GetControllerNotifyObject(&pdfcn) == S_OK)
    {
        ASSERT(pdfcn);
        pdfcn->ViewDestroyed();
        pdfcn->Release();
    }
    return S_OK;
}

HRESULT CDocFindSFVCB::OnIsOwnerData(DWORD pv, BOOL *pfIsOwnerData)
{
    *pfIsOwnerData = TRUE;

    return S_OK;
}

HRESULT CDocFindSFVCB::OnGetODRangeObject(DWORD pv, WPARAM wWhich, ILVRange **plvr)
{
    HRESULT hres = E_FAIL;
    switch (wWhich)
    {
    case LVSR_SELECTION:
        hres = m_pDFFolder->_dflvrSel.QueryInterface(IID_ILVRange, (void **)plvr);
        break;
    case LVSR_CUT:
        hres = m_pDFFolder->_dflvrCut.QueryInterface(IID_ILVRange, (void **)plvr);
        break;
    }
    return S_OK;
}

HRESULT CDocFindSFVCB::OnODCacheHint(DWORD pv, NMLVCACHEHINT* pnmlvc)
{
    // The listview is giving us a hint of the items it is about to do something in a range
    // so make sure we have pidls for each of the items in the range...
    int i;
    int iTo;
    
    m_pDFFolder->GetItemCount(&iTo);
    if (iTo >= pnmlvc->iTo)
        iTo = pnmlvc->iTo;
    else
        iTo--;

    for (i=pnmlvc->iFrom; i <= iTo; i++)
    {
        ESFItem *pesfi;
        if (FAILED(m_pDFFolder->GetItem(i, &pesfi)))
            break;
    }

    return S_OK;
}

HRESULT CDocFindSFVCB::OnDEFVIEWMODE(DWORD pv, FOLDERVIEWMODE*lP)
{
    *lP = FVM_DETAILS;
    return S_OK;
}

HRESULT CDocFindSFVCB::OnGetViews(DWORD pv, SHELLVIEWID *pvid, IEnumSFVViews **ppObj)
{
    // BUGBUG:: may want to allow some template to specified, but for now not...
    CViewsList cViews;

    if (m_pDFFolder->pdfff != NULL)
    {
        CLSID clsid;
        if (SUCCEEDED(m_pDFFolder->pdfff->GetSearchFolderClassId(&clsid)))
            cViews.AddCLSID(&clsid);
    }

    *pvid = VID_Details;    // We want to default to report view...

    // Add this class stuff
    // cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("directory"));

    return CreateEnumCViewList(&cViews, ppObj);
}

HRESULT CDocFindSFVCB::OnOverrideItemCount(DWORD pv, UINT*lP)
{
    // make sure we have room for a lot of items, since normally
    // we have 0 and the window gets sized too small...
    *lP = 4000;
    return S_OK;
}

HRESULT CDocFindSFVCB::OnGetIPersistHistory(DWORD pv, IPersistHistory **ppph)
{
    // If they call us with ppph == NULL they simply want to know if we support
    // the history so return S_OK;
    if (ppph == NULL)
        return S_OK;

    // get the persist history from us and we hold folder and view objects
    *ppph = NULL;

    CDocFindPersistHistory *pdfph = new CDocFindPersistHistory();
    if (!pdfph)
        return E_OUTOFMEMORY;

    HRESULT hres = pdfph->QueryInterface(IID_IPersistHistory, (void**)ppph);
    pdfph->Release();
    return hres;
}

HRESULT CDocFindSFVCB::OnRefresh(DWORD pv, BOOL fPreRefresh)
{
    EnterCriticalSection(&m_pDFFolder->_csSearch);

    m_pDFFolder->_fInRefresh = BOOLIFY(fPreRefresh);
    // If we have old results tell defview the new count now...
    if (!fPreRefresh && m_pDFFolder->hdpaItems)
    {
        IShellFolderView *psfv;
        UINT cItems = DPA_GetPtrCount(m_pDFFolder->hdpaItems);
        if (cItems && _punkSite && SUCCEEDED(_punkSite->QueryInterface(IID_IShellFolderView, (void**)&psfv)))
        {   
            psfv->SetObjectCount(cItems, SFVSOC_NOSCROLL);
            psfv->Release();
        }
    }
    LeaveCriticalSection(&m_pDFFolder->_csSearch);
    return S_OK;
}

HRESULT CDocFindSFVCB::OnGetHelpTopic(DWORD pv, SFVM_HELPTOPIC_DATA *phtd)
{
    StrCpyW( phtd->wszHelpFile, L"find.chm" );
    return S_OK;
}

STDMETHODIMP CDocFindSFVCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMERGEMENU);
    HANDLE_MSG(0, SFVM_GETWORKINGDIR, OnGETWORKINGDIR);
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnINVOKECOMMAND);
    HANDLE_MSG(0, SFVM_GETCOLSAVESTREAM, OnGETCOLSAVESTREAM);
    HANDLE_MSG(0, SFVM_GETITEMIDLIST, OnGETITEMIDLIST);
    HANDLE_MSG(0, SFVM_SETITEMIDLIST, OnSETITEMIDLIST);
    HANDLE_MSG(0, SFVM_SELCHANGE, OnSelChange);
    HANDLE_MSG(0, SFVM_INDEXOFITEMIDLIST, OnGetIndexForItemIDList);
    HANDLE_MSG(0, SFVM_DELETEITEM, OnDeleteItem);
    HANDLE_MSG(0, SFVM_ODFINDITEM, OnODFindItem);
    HANDLE_MSG(0, SFVM_ARRANGE, OnReArrange);
    HANDLE_MSG(0, SFVM_GETEMPTYTEXT, OnGetEmptyText);
    HANDLE_MSG(0, SFVM_GETITEMICONINDEX, OnGetItemIconIndex);
    HANDLE_MSG(0, SFVM_SETICONOVERLAY, OnSetItemIconOverlay);
    HANDLE_MSG(0, SFVM_GETICONOVERLAY, OnGetItemIconOverlay);
    HANDLE_MSG(0, SFVM_ISOWNERDATA, OnIsOwnerData);
    HANDLE_MSG(0, SFVM_WINDOWCREATED, OnWindowCreated);
    HANDLE_MSG(0, SFVM_WINDOWDESTROY, OnWindowDestroy);
    HANDLE_MSG(0, SFVM_GETODRANGEOBJECT, OnGetODRangeObject);
    HANDLE_MSG(0, SFVM_ODCACHEHINT, OnODCacheHint);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, OnDEFVIEWMODE);
    HANDLE_MSG(0, SFVM_GETVIEWS, OnGetViews);
    HANDLE_MSG(0, SFVM_OVERRIDEITEMCOUNT, OnOverrideItemCount);
    HANDLE_MSG(0, SFVM_GETIPERSISTHISTORY, OnGetIPersistHistory);
    HANDLE_MSG(0, SFVM_REFRESH, OnRefresh);
    HANDLE_MSG(0, SFVM_GETHELPTOPIC, OnGetHelpTopic);

    default:
        return E_FAIL;
    }

    return NOERROR;
}

CDocFindSFVCB::~CDocFindSFVCB()
{
    ATOMICRELEASE(m_pDFFolder);
}

CDocFindSFVCB* DocFind_CreateSFVCB(IShellFolder* psf, CDFFolder* pDFFolder)
{
    return(new CDocFindSFVCB(psf, pDFFolder));
}



//-----------------------------------------------------------------------------------
// Docfind Persistent history implemention.

CDocFindPersistHistory::CDocFindPersistHistory()
{
}


STDAPI CDocFindPersistHistory_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppv)
{
    HRESULT   hr;

    CDocFindPersistHistory *pdfph = new CDocFindPersistHistory();
    if (!pdfph)
    {
        *ppv = NULL;
        return E_OUTOFMEMORY;
    }

    hr = pdfph->QueryInterface(riid, ppv);
    pdfph->Release();
    return hr;    
}


// Functions to support persisting the document into the history stream...
STDMETHODIMP CDocFindPersistHistory::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_DocFindPersistHistory;
    return S_OK;
}


IDocFindFolder *CDocFindPersistHistory::_GetDocFindFolder()
{
    IDocFindFolder *pdff = NULL;
    IShellFolder   *psf;
    IDefViewFrame   *pdvf;

    // the _punksite is to the defview so we can simply QI for frame...
    if (SUCCEEDED(_punkSite->QueryInterface(IID_IDefViewFrame, (void **)&pdvf))) 
    {
        if (SUCCEEDED(pdvf->GetShellFolder(&psf))) 
        {
            psf->QueryInterface(IID_IDocFindFolder, (void **)&pdff);
            psf->Release();
        }
        pdvf->Release();
    }

    return pdff;
}

STDMETHODIMP CDocFindPersistHistory::LoadHistory(IStream *pstm, IBindCtx *pbc)
{
    int cItems = 0;
    IDocFindFolder *pdff = _GetDocFindFolder();
    if (pdff)
    {
        pdff->RestoreFolderList(pstm);
        pdff->RestoreItemList(pstm, &cItems);
        pdff->Release();
    }

    IShellFolderView *psfv;
    if (_punkSite && SUCCEEDED(_punkSite->QueryInterface(IID_IShellFolderView, (void**)&psfv)))
    {   
        psfv->SetObjectCount(cItems, SFVSOC_NOSCROLL);
        psfv->Release();
    }

    // call our base class to allow it to restore it's stuff as well.
    return CDefViewPersistHistory::LoadHistory(pstm, pbc);
}


STDMETHODIMP CDocFindPersistHistory::SaveHistory(IStream *pstm)
{
    IShellFolderView *psfv = NULL;
    IDocFindFolder *pdff = _GetDocFindFolder();

    if (pdff)
    {
        pdff->SaveFolderList(pstm);       
        pdff->SaveItemList(pstm);       
        pdff->Release();
    }

    // Let base class save out as well
    return CDefViewPersistHistory::SaveHistory(pstm);
}



//-----------------------------------------------------------------------------------
int CALLBACK DFSCompareItems(void *p1, void *p2, LPARAM lParam)
{
    DFSINFO *pdfsinfo = (DFSINFO*)lParam;
    HRESULT hres = E_FAIL;

    // we should not have NULL items in DPA!!!!
    if (EVAL(p1 && p2))
    {
        hres = pdfsinfo->pshf->CompareIDs(pdfsinfo->iCol,
               &(((ESFItem*)p1)->idl), &(((ESFItem*)p2)->idl));
    }
    return (short)SCODE_CODE(GetScode(hres));
}

// -----------------------------------------------------------------------
// CDocFindLVRange object - Object to manage the selection states for an
// owner data listview...
// -----------------------------------------------------------------------

STDMETHODIMP_(ULONG) CDocFindLVRange::AddRef()
{
    return m_pdff->AddRef();
}
STDMETHODIMP_(ULONG) CDocFindLVRange::Release()
{ 
    return m_pdff->Release();
}

STDMETHODIMP CDocFindLVRange::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDocFindLVRange, ILVRange),          // IID_ILVRange
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


    // ILVRange methods
STDMETHODIMP CDocFindLVRange::IncludeRange(LONG iBegin, LONG iEnd)
{
    // Including the range must load the elements as we need the object ptr...
    ESFItem *pesfi;
    long i;
    int  iTotal;

    m_pdff->GetItemCount(&iTotal);
    if (iEnd > iTotal)
        iEnd = iTotal-1;
        
    for (i = iBegin; i <= iEnd;i++)
    {
        if (SUCCEEDED(m_pdff->GetItem(i, &pesfi)) && pesfi)
        {
            if ((pesfi->dwState & m_dwMask) == 0)
            {
                m_cIncluded++;
                pesfi->dwState |= m_dwMask;
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CDocFindLVRange::ExcludeRange(LONG iBegin, LONG iEnd)    
{
    // Excluding the range is OK to not load the elements as this would be to deslect all...
    long i;

    ESFItem *pesfi;

    EnterCriticalSection(&m_pdff->_csSearch);
    if (iEnd >= DPA_GetPtrCount(m_pdff->hdpaItems))
        iEnd = DPA_GetPtrCount(m_pdff->hdpaItems)-1;

    for (i = iBegin; i <= iEnd; i++)
    {
        pesfi = (ESFItem*)DPA_FastGetPtr(m_pdff->hdpaItems, i);
        if (pesfi)
        {
            if (pesfi->dwState & m_dwMask)
            {
                m_cIncluded--;
                pesfi->dwState &= ~m_dwMask;
            }
        }
    }
    LeaveCriticalSection(&m_pdff->_csSearch);

    return S_OK;
}

STDMETHODIMP CDocFindLVRange::InvertRange(LONG iBegin, LONG iEnd)
{
    // Including the range must load the elements as we need the object ptr...

    ESFItem *pesfi;
    long i;
    int  iTotal;

    m_pdff->GetItemCount(&iTotal);
    if (iEnd > iTotal)
        iEnd = iTotal-1;

    for (i = iBegin; i <= iEnd;i++)
    {
        if (SUCCEEDED(m_pdff->GetItem(i, &pesfi)) && pesfi)
        {
            if ((pesfi->dwState & m_dwMask) == 0)
            {
                m_cIncluded++;
                pesfi->dwState |= m_dwMask;
            }
            else
            {
                m_cIncluded--;
                pesfi->dwState &= ~m_dwMask;
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CDocFindLVRange::InsertItem(LONG iItem)
{
    // We already maintain the list anyway...
    return S_OK;
}

STDMETHODIMP CDocFindLVRange::RemoveItem(LONG iItem)
{
    // We maintain the list so don't do anything...
    return S_OK;
}

STDMETHODIMP CDocFindLVRange::Clear()
{
    // If there are things selected, need to unselect them now...
    if (m_cIncluded)
        ExcludeRange(0, LONG_MAX);

    m_cIncluded = 0;
    m_pdff->ClearSaveStateList();
    return S_OK;
}

STDMETHODIMP CDocFindLVRange::IsSelected(LONG iItem)
{
    // Don't force the items to be generated if they were not before...
    ESFItem *pesfi;

    EnterCriticalSection(&m_pdff->_csSearch);
    pesfi = (ESFItem*)DPA_GetPtr(m_pdff->hdpaItems, iItem);
    LeaveCriticalSection(&m_pdff->_csSearch);
    if (pesfi)
        return (pesfi->dwState & m_dwMask)? S_OK : S_FALSE;

    // Assume not selected if we don't have the item yet...
    return S_FALSE;
}

STDMETHODIMP CDocFindLVRange::IsEmpty()
{
    return m_cIncluded ? S_FALSE : S_OK;
}

STDMETHODIMP CDocFindLVRange::NextSelected(LONG iItem, LONG *piItem)
{
    EnterCriticalSection(&m_pdff->_csSearch);
    LONG cItems = DPA_GetPtrCount(m_pdff->hdpaItems);

    while (iItem < cItems)
    {
        ESFItem *pesfi;
        pesfi = (ESFItem*)DPA_GetPtr(m_pdff->hdpaItems, iItem);
        if (pesfi && (pesfi->dwState & m_dwMask))
        {
            *piItem = iItem;
            LeaveCriticalSection(&m_pdff->_csSearch);
            return S_OK;
        }
        iItem++;
    }
    LeaveCriticalSection(&m_pdff->_csSearch);
    *piItem = -1;
    return S_FALSE;
}

STDMETHODIMP CDocFindLVRange::NextUnSelected(LONG iItem, LONG *piItem)
{
    EnterCriticalSection(&m_pdff->_csSearch);
    LONG cItems = DPA_GetPtrCount(m_pdff->hdpaItems);

    while (iItem < cItems)
    {
        ESFItem *pesfi;
        pesfi = (ESFItem*)DPA_GetPtr(m_pdff->hdpaItems, iItem);
        if (!pesfi || ((pesfi->dwState & m_dwMask) == 0))
        {
            *piItem = iItem;
            LeaveCriticalSection(&m_pdff->_csSearch);
            return S_OK;
        }
        iItem++;
    }
    LeaveCriticalSection(&m_pdff->_csSearch);
    *piItem = -1;
    return S_FALSE;
}

STDMETHODIMP CDocFindLVRange::CountIncluded(LONG *pcIncluded)
{
    *pcIncluded = m_cIncluded;

    // Sortof Gross, but if looking at selection then also include the list of items
    // that are selected in our save list...
    if (m_dwMask & LVIS_SELECTED)
        *pcIncluded += m_pdff->_cSaveStateSelected;
    return S_OK;
}



// -----------------------------------------------------------------------
// OLEDB search
// -----------------------------------------------------------------------

#ifdef WINNT

#ifdef CI_ROWSETNOTIFY
class CDFOleDBRowsetWatchNotify;
#endif //#ifdef CI_ROWSETNOTIFY

// Define OleDBEnum translation structure...
typedef struct _dfodbet         // DFET for short
{
    struct _dfodbet *pdfetNext;
    LPWSTR  pwszFrom;
    int     cbFrom;
    LPWSTR  pwszTo;
} DFODBET;


class CDFOleDBEnum : public IDFEnum, public IShellService
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef) () ;
    STDMETHOD_(ULONG,Release) ();

    // *** IDFEnum methods (sortof stander iterator methods ***
    STDMETHOD(Next)(LPITEMIDLIST *ppidl,
                   int *pcObjectSearched,
                   int *pcFoldersSearched,
                   BOOL *pfContinue,
                   int *pState,
                   HWND hwnd);
    STDMETHOD(Skip)(int celt);
    STDMETHOD(Reset)();
    STDMETHOD(StopSearch)();
    STDMETHOD_(BOOL,FQueryIsAsync)();
    STDMETHOD(GetAsyncCount)(DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone);
    STDMETHOD(GetItemIDList)(UINT iItem, LPITEMIDLIST *ppidl);
    STDMETHOD(GetExtendedDetailsOf)(LPCITEMIDLIST pidl, UINT iCol, LPSHELLDETAILS pdi);
    STDMETHOD(GetExtendedDetailsULong)(LPCITEMIDLIST pidl, UINT iCol, ULONG *pul);
    STDMETHOD(GetItemID)(UINT iItem, DWORD *puWorkID);
    STDMETHOD(SortOnColumn)(UINT iCOl, BOOL fAscending);

    // *** IShellService methods (sortof stander iterator methods ***
    STDMETHOD(SetOwner)(struct IUnknown* punkOwner);
    
    CDFOleDBEnum(
        IDocFindFileFilter * pdfff,
        IDocFindFolder *pdfFolder,
        DWORD grfFlags,
        int iColSort, 
        LPTSTR pszProgressText,
        IRowsetWatchNotify *prwn);
    ~CDFOleDBEnum();

protected:
    LONG _cRef;
    IDocFindFileFilter*     _pdfff;
    IRowsetWatchNotify*     _prwn;
    IDocFindFolder*         _pdfFolder;
    int                     _iColSort; 
    DWORD                   _grfFlags;
    DWORD                   _grfWarnings;
    LPTSTR                  _pszProgressText;
    TCHAR       _szCurrentDir[MAX_PATH];
    IShellFolder *_psfCurrentDir;
    int         _iFolder;
    ICommand*   _pCommand;
    IRowsetLocate* _pRowset;
    IRowsetAsynch* _pRowsetAsync;
    HACCESSOR   _hAccessor;
    HACCESSOR   _hAccessorWorkID;
    HROW        _ahrow[100];          // Cache 100 hrows out for now
    UINT        _ihrowFirst;         // The index of which row is cached out first
    DBCOUNTITEM _cRows;               // number of hrows in _ahrow
#ifdef CI_ROWSETNOTIFY
    CDFOleDBRowsetWatchNotify   *_pdbrWatchNotify;   // We hold onto the notify...
    DWORD       _dwAdviseID;    // Remember our notify ID...
    IConnectionPoint *_pcpointWatchNotify;
#endif
    DFODBET     *_pdfetFirst;    // Name translation list.
    
    DWORD       _fDone:1;

    // called from our WatchNotifyClass...
    void HandleWatchNotify(DBWATCHNOTIFY eChangeReason);

    HRESULT _BuildAndSetCommandTree(int iCol, BOOL fReverse);

    HRESULT _SetCmdProp(ICommand *pCommand);
    HRESULT _MapColumns(
        IUnknown *   pUnknown,
        DBORDINAL    cCols,
        DBBINDING *  pBindings,
        const DBID * pDbCols,
        HACCESSOR &  hAccessor );
    void _ReleaseAccessor();
    HRESULT _CacheRowSet(UINT iItem);

    HRESULT DoQuery(LPWSTR *apwszPaths, UINT *pcPaths);

    friend class CDFOleDBRowsetWatchNotify;
    friend HRESULT DocFind_CreateOleDBEnum(
                    IDocFindFileFilter * pdfff,
                    IShellFolder *psf,
                    LPWSTR *apwszPaths,
                    UINT    *pcPaths,
                    DWORD grfFlags,
                    int iColSort,
                    LPTSTR pszProgressText,
                    IRowsetWatchNotify *prwn,
                    IDFEnum **ppdfenum);
};

#ifdef CI_ROWSETNOTIFY
// Define RowsetWatchNotify class...
class CDFOleDBRowsetWatchNotify : public IRowsetNotify
{
public:
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef) () ;
    STDMETHOD_(ULONG,Release) ();

    // *** IRowsetWatchNotify methods ***
    STDMETHOD(OnFieldChange)(IRowset *pRowset, HROW hrow, ULONG cColumns, 
                             ULONG rgColumns[], DBREASON eReason, 
                             DBEVENTPHASE ePhase, BOOL fCantDeny)
    {
        return DB_S_UNWANTEDREASON;
    }
    STDMETHOD(OnRowChange)(IRowset *pRowset, ULONG cColumns, const HROW rgColumns[], 
                           DBREASON eReason, DBEVENTPHASE ePhase, BOOL fCantDeny);
    STDMETHOD(OnRowsetChange)(IRowset *pRowset,  DBREASON eReason, DBEVENTPHASE ePhase, BOOL fCantDeny)
    {
        return DB_S_UNWANTEDREASON;
    }

    // *** Constructor and destructor ***
    CDFOleDBRowsetWatchNotify(CDFOleDBEnum *pdfoe);
    ~CDFOleDBRowsetWatchNotify();

protected:
    LONG _cRef;
    CDFOleDBEnum    *_pdfoe;

};
#endif //#ifdef CI_ROWSETNOTIFY

EXTERN_C HMODULE g_hmodQuery;

// -----------------------------------------------------------------------
// Create an instance of docfind OleDB enum (CDFOleDBEnum) object.
// -----------------------------------------------------------------------
HRESULT DocFind_CreateOleDBEnum(
    IDocFindFileFilter * pdfff,
    IShellFolder *psf,
    LPWSTR *apwszPaths,
    UINT    *pcPaths,
    DWORD grfFlags,
    int iColSort,
    LPTSTR pszProgressText,
    IRowsetWatchNotify *prwn,
    IDFEnum **ppdfenum)
{
    HRESULT hres;

    // BUGBUG !!! this is a temp hack.
    // detect if query client is installed.
    //
    // currently just check if query.dll exist.
    // and we do it here instead of dllload.c so
    // that we don't assert if query.dll does not exist.
    // will switch to a registry look up when it's available.
    if (!g_hmodQuery)
    {
        g_hmodQuery = LoadLibrary(TEXT("query.dll"));
        if (!g_hmodQuery)
            return E_NOTIMPL;
    }
    
    IDocFindFolder *pdfFolder;
    psf->QueryInterface(IID_IDocFindFolder, (void **)&pdfFolder);
    CDFOleDBEnum* pdfenum = new CDFOleDBEnum(pdfff, pdfFolder, grfFlags, iColSort, pszProgressText, prwn);
    if (pdfFolder)
        pdfFolder->Release();
    
    if (pdfenum)
    {
        hres = pdfenum->DoQuery(apwszPaths, pcPaths);
        if (hres == S_OK)       // We only continue to use this if query returne S_OK...
            *ppdfenum = (IDFEnum*)pdfenum;
        else
        {
            pdfenum->Release();     // release the memory we allocated
            *ppdfenum = NULL;
        }
            
        return hres;
    }
    else
        return E_OUTOFMEMORY;
}


// =======================================================================

const DBID c_aDbCols[] =
{
    {{PSGUID_STORAGE}, DBKIND_GUID_PROPID, {(LPOLESTR)(ULONG_PTR)(ULONG)PID_STG_NAME}},
    {{PSGUID_STORAGE}, DBKIND_GUID_PROPID, {(LPOLESTR)(ULONG_PTR)(ULONG)PID_STG_PATH}},
    {{PSGUID_STORAGE}, DBKIND_GUID_PROPID, {(LPOLESTR)(ULONG_PTR)(ULONG)PID_STG_ATTRIBUTES}},
    {{PSGUID_STORAGE}, DBKIND_GUID_PROPID, {(LPOLESTR)(ULONG_PTR)(ULONG)PID_STG_SIZE}},
    {{PSGUID_STORAGE}, DBKIND_GUID_PROPID, {(LPOLESTR)(ULONG_PTR)(ULONG)PID_STG_WRITETIME}},
    {{PSGUID_QUERY_D}, DBKIND_GUID_PROPID, {(LPOLESTR)                  PROPID_QUERY_RANK}},
};

const DBID c_aDbWorkIDCols[] =
{
    {{PSGUID_QUERY_D}, DBKIND_GUID_PROPID, {(LPOLESTR)PROPID_QUERY_WORKID}}
};

const WCHAR c_wszColNames[] = L"FileName,Path,Attrib,Size,Write,Rank,WorkID";
#define ICOLNAME_RANK 5
const LPWSTR c_awszColSortNames[] = {L"FileName[a],Path[a]", L"Path[a],FileName[a]", L"Size[a]", NULL, L"Write[a]", L"Rank[d]"};

const ULONG c_cDbCols = ARRAYSIZE(c_aDbCols);
const DBID c_dbcolNull = { {0,0,0,{0,0,0,0,0,0,0,0}},DBKIND_GUID_PROPID,0};
const GUID c_guidQueryExt = DBPROPSET_QUERYEXT;
const GUID c_guidRowsetProps = {0xc8b522be,0x5cf3,0x11ce,{0xad,0xe5,0x00,0xaa,0x00,0x44,0x77,0x3d}}; 



// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
CDFOleDBEnum::CDFOleDBEnum(
    IDocFindFileFilter * pdfff,
    IDocFindFolder * pdfFolder,
    DWORD grfFlags,
    int iColSort, 
    LPTSTR pszProgressText,
    IRowsetWatchNotify *prwn
) :
    _cRef(1),
    _ihrowFirst((UINT)-1),
    _pdfff(pdfff),
    _pdfFolder(pdfFolder),
    _prwn(prwn),
    _grfFlags(grfFlags),
    _grfWarnings(DFW_DEFAULT),
    _iColSort(iColSort),
    _pszProgressText(pszProgressText)
{
    *_szCurrentDir = TEXT('\0');

    ASSERT(_pRowset == 0);
    ASSERT(_pRowsetAsync == 0);
    ASSERT(_pCommand == 0);
    ASSERT(_hAccessor == 0);
    ASSERT(_hAccessorWorkID ==0);
    ASSERT(_cRows == 0);
    ASSERT(_fDone == 0);
#ifdef CI_ROWSETNOTIFY
    ASSERT(_pdbrWatchNotify == NULL);
    ASSERT(_dwAdviseID == 0);
    ASSERT(_pcpointWatchNotify == NULL);
#endif

    if (_pdfff)
    {
        _pdfff->AddRef();
        _pdfff->GetWarningFlags( &_grfWarnings );
    }
        
    if (_pdfFolder)              
        _pdfFolder->AddRef();

    if (_prwn)
        _prwn->AddRef();
}

CDFOleDBEnum::~CDFOleDBEnum()
{
    ATOMICRELEASE(_pdfff);
    ATOMICRELEASE(_pdfFolder);
    ATOMICRELEASE(_prwn);
    ATOMICRELEASE(_psfCurrentDir);
        
    if (_pRowset)
    {
#ifdef CI_ROWSETNOTIFY
        // If we have an advise going, remove it now
        if (_pdbrWatchNotify)
        {    
            if (_pcpointWatchNotify)
            {
                _pcpointWatchNotify->Unadvise(_dwAdviseID);
                _pcpointWatchNotify->Release();
            }
            _pdbrWatchNotify->Release();
        }
#endif


        ATOMICRELEASE(_pRowsetAsync);

        // Release any cached rows.
       _CacheRowSet((UINT)-1);
          
        if (_hAccessor || _hAccessorWorkID)
            _ReleaseAccessor();

        _pRowset->Release();
    }

    ATOMICRELEASE(_pCommand);

    // Release any name translations we may have allocated.
    DFODBET *pdfet = _pdfetFirst;
    while (pdfet)
    {
        DFODBET *pdfetT = pdfet;
        pdfet = pdfet->pdfetNext;      // First setup to look at the next item before we free stuff...
        LocalFree((HLOCAL)pdfetT->pwszFrom);
        LocalFree((HLOCAL)pdfetT->pwszTo);
        LocalFree((HLOCAL)pdfetT);
    }
}


HRESULT CDFOleDBEnum::QueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CDFOleDBEnum, IUnknown, IDFEnum), // IID_IUNKNOWN
        QITABENT(CDFOleDBEnum, IShellService),          // IID_IShellService
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

ULONG CDFOleDBEnum::AddRef()
{
    TraceMsg(TF_DOCFIND, "CDFOleDBEnum.AddRefe %d",_cRef+1);
    return InterlockedIncrement(&_cRef);
}

ULONG CDFOleDBEnum::Release()
{
    TraceMsg(TF_DOCFIND, "CDFOleDBEnum.Release %d",_cRef-1);
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CDFOleDBEnum::Next(
    LPITEMIDLIST *ppidl,
    int *pcObjectSearched,
    int *pcFoldersSearched,
    BOOL *pfContinue,
    int *pState,
    HWND hwnd
)
{
    // we are async from the main docind search loop...
    return E_PENDING;       // as good a return as any to say that we are async...
}

HRESULT CDFOleDBEnum::Skip(int celt)
{
    return E_NOTIMPL;
}

HRESULT CDFOleDBEnum::Reset()
{
    // overload Reset to mean dump the rowset cache!!!
    _CacheRowSet(-1);    
    // still return failiure
    return E_NOTIMPL;
}

HRESULT CDFOleDBEnum::StopSearch()
{
    // Lets see if we can find one that works...
    // BUGBUG:: Should we try everything in case different rowsets use different ways to abort?
    HRESULT hres = _pCommand->Cancel();
    if (FAILED(hres))
        hres = _pRowsetAsync->Stop();
    if (FAILED(hres))
    {
        IDBAsynchStatus *pdbas;
        if (SUCCEEDED(_pRowset->QueryInterface(IID_IDBAsynchStatus, (void **)&pdbas)))
        {
            hres = pdbas->Abort(DB_NULL_HCHAPTER, DBASYNCHOP_OPEN);
            pdbas->Release();
        }
    }
    return hres; 
}

BOOL CDFOleDBEnum::FQueryIsAsync()
{
    return TRUE;
}

HRESULT CDFOleDBEnum::GetAsyncCount(DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone)
{
    DBCOUNTITEM dwDen;
    DBCOUNTITEM dwNum;
    BOOL fMore;

    if (!_pRowsetAsync)
        return E_FAIL;

    HRESULT hres = _pRowsetAsync->RatioFinished( &dwDen, &dwNum, pdwTotalAsync, &fMore );

    if (SUCCEEDED(hres))
    {
        *pfQueryDone =  dwDen == dwNum;
        *pnPercentComplete = dwDen? (int)((dwNum * 100)/dwDen) : 100;
    }
    else
        *pfQueryDone = TRUE;    // in case that is all they are looking at...

#ifdef WANT_CI_QUERY_STATUS
    if (*pfQueryDone)
    {
        IRowsetInfo *pinfo;
        HRESULT hr = _pRowsetAsync->QueryInterface(IID_IRowsetInfo, (void **)&pinfo);

        if ( SUCCEEDED( hr ) )
        {
            // This rowset property is Indexing-Service specific

            DBPROPID propId = MSIDXSPROP_ROWSETQUERYSTATUS;
            DBPROPIDSET propSet;
            propSet.rgPropertyIDs = &propId;
            propSet.cPropertyIDs = 1;
            const GUID guidRowsetExt = DBPROPSET_MSIDXS_ROWSETEXT;
            propSet.guidPropertySet = guidRowsetExt;

            ULONG cPropertySets = 0;
            DBPROPSET * pPropertySets;

            hr = pinfo->GetProperties(1,
                                      &propSet,
                                      &cPropertySets,
                                      &pPropertySets );

            if ( SUCCEEDED( hr ) )
            {
                DWORD dwStatus = pPropertySets->rgProperties->vValue.ulVal;

                CoTaskMemFree( pPropertySets->rgProperties );
                CoTaskMemFree( pPropertySets );

                DWORD dwFill = QUERY_FILL_STATUS( dwStatus );
                DWORD dwReliability = QUERY_RELIABILITY_STATUS( dwStatus );
            }
        }
    } //DisplayRowsetStatus

#endif
    return hres;
}

HRESULT CDFOleDBEnum::GetItemIDList(UINT iItem, LPITEMIDLIST *ppidl)
{
    HRESULT hres;
    
    *ppidl = NULL;

    hres = _CacheRowSet(iItem);
    if (hres != S_OK)
    {
        return E_FAIL;    // we could not get the item someone asked for, so error...
    }

    PROPVARIANT* data[c_cDbCols];
    
    hres = _pRowset->GetData(_ahrow[iItem-_ihrowFirst], _hAccessor, &data);
    if (S_OK == hres)
    {
        // data[0].pwszVal is the file name
        // data[1].pwszVal is the full path (including file name)
        // data[2].ulVal is the attribute
        // data[3].ulVal is the size in byte
        // data[4].filetime is the last write time in UTC
        // data[5].ulVal is the rank of the item...

        WIN32_FIND_DATA fd = {0};
        WCHAR wszParent[MAX_PATH];
        LPITEMIDLIST pidl = NULL;

        fd.dwFileAttributes = data[2]->ulVal;
        fd.nFileSizeLow = data[3]->ulVal;
        fd.ftLastWriteTime = data[4]->filetime;
        
        StrCpyW(fd.cFileName, data[0]->pwszVal);
        StrCpyW(wszParent, data[1]->pwszVal);
        // get the parent folder name.
                
        PathRemoveFileSpec(wszParent);
       
        // If it is a UNC it might be one we need to translate, to handle the case that 
        // content index does not support redirected drives.
        WCHAR wszTranslatedParent[MAX_PATH];
        BOOL fTranslated = FALSE;
        BOOL fFolderChanged = TRUE;

        if (PathIsUNC(wszParent))
        {
            DFODBET *pdfet;
            for (pdfet = _pdfetFirst; pdfet; pdfet = pdfet->pdfetNext)
            {
                if ((StrCmpNIW(wszParent, pdfet->pwszFrom, pdfet->cbFrom) == 0)
                        && (wszParent[pdfet->cbFrom] == L'\\'))
                {
                    // Ok we have a translation to use.
                    fTranslated = TRUE;
                    StrCpyW(wszTranslatedParent, pdfet->pwszTo);
                    // need + 1 here or we'll get something like w:\\winnt! bogus path, that is.
                    StrCatW(wszTranslatedParent, &wszParent[pdfet->cbFrom+1]);
                }
            }
        }
        
        if (fTranslated)
            hres = SHILCreateFromPath(wszTranslatedParent, &pidl, NULL);
        else
            hres = SHILCreateFromPath(wszParent, &pidl, NULL);

        // we've moved into another folder.
        if (SUCCEEDED(hres))
        { 
            if (lstrcmp(wszParent, _szCurrentDir) != 0)
            {
                // folders are different, must clear the cached folder
                ATOMICRELEASE(_psfCurrentDir);
                
                hres = _pdfFolder->AddFolderToFolderList(pidl, TRUE, &_iFolder);
                if (SUCCEEDED(hres))
                {
                    DFFolderListItem *pdffli;
                    // save the folder name away so we don't add it
                    // to the list again.
                    lstrcpy(_szCurrentDir, wszParent);
                    
                    hres = _pdfFolder->GetFolderListItem(_iFolder, &pdffli);
                    if (SUCCEEDED(hres))
                    {
                        ASSERT(pdffli);

                        _psfCurrentDir = DocFind_GetObjectsIFolder(_pdfFolder, pdffli, NULL);// no addref here!!!
                        if (!_psfCurrentDir)
                            hres = E_FAIL;
                    }
                }
                else
                {
                    // failed to add folder to the list.
                    hres = E_OUTOFMEMORY;
                }
            }
            else
            {
                fFolderChanged = FALSE;
            }
        }
        
        if (SUCCEEDED(hres))
        {
            DWORD dwItemID;

            GetItemID(iItem, &dwItemID);

            hres = E_FAIL;
            if (EVAL(_psfCurrentDir))
            { 
                hres = DocFind_CreateIDList(_psfCurrentDir, &fd, _iFolder, DFDF_EXTRADATA, iItem, dwItemID, data[5]->ulVal, ppidl);
            }
        }

        ASSERT(ShowSuperHidden() || !IsSuperHidden(fd.dwFileAttributes));        
        if (FAILED(hres) && hres != E_OUTOFMEMORY)
        {
            if (!fTranslated && fFolderChanged) // don't add net paths to the list of special folders
            {
                HKEY  hkey;
                // we could not get pidl for this item for some reason.  we have to put 
                // it in the list of bad items so that we can tell ci not to give it to
                // us the next time we do search
                if (RegCreateKeyExW(HKEY_CURRENT_USER, CI_SPECIAL_FOLDERS, 0, L"", 0, KEY_WRITE | KEY_QUERY_VALUE, NULL, &hkey, NULL) == ERROR_SUCCESS)
                {
                    DWORD dwInsert = 0; // init to zero in case query info bellow fails
                    WCHAR wszName[10];
                    WCHAR wsz[MAX_PATH];
                    int   iEnd;

                    // no pidl for parent folder.  it is probably a subfolder of a special folder
                    // so keep going up until we can find the first non special folder
                    if (!pidl)
                    {
                        StrCpyNW(wsz, wszParent, ARRAYSIZE(wsz));
                        PathRemoveBackslashW(wsz);
                        do
                        {
                            if (!PathRemoveFileSpecW(wsz))
                                break;
                            hres = SHILCreateFromPath(wsz, &pidl, NULL);
                        }
                        while (FAILED(hres) && hres != E_OUTOFMEMORY);

                        if (pidl)
                        {
                            StrCpyNW(wszParent, wsz, ARRAYSIZE(wszParent));
                        }
                    }

                    
                    RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &dwInsert, NULL, NULL, NULL, NULL);
                    // start from the end as there is a high chance we added this at the end
                    for (int i=dwInsert-1; i>=0; i--)
                    {                        
                        DWORD cb = ARRAYSIZE(wsz)*SIZEOF(WCHAR);
                        
                        wsprintfW(wszName, L"%d", i);
                        if (RegQueryValueExW(hkey, wszName, NULL, NULL, (BYTE *)wsz, &cb) == ERROR_SUCCESS)
                        {
                            LPWSTR wszTemp = StrStrIW(wsz+1, wszParent); // +1 to pass " that's at the beginning of the string
                            if (wszTemp && wszTemp == wsz+1)
                            {
                                dwInsert = i; // overwrite this value
                                break;
                            }
                            else
                            {
                                iEnd = lstrlen(wsz);
                                if (EVAL(iEnd > 1))
                                {
                                    int iBackslash = iEnd-3;
                                    ASSERT(wsz[iBackslash] == L'\\');
                                    wsz[iBackslash] = L'\0';
                                    wszTemp = StrStrIW(wszParent, wsz+1);
                                    wsz[iBackslash] = L'\\';
                                    if (wszTemp && wszTemp == wszParent)
                                    {
                                        dwInsert = -1;
                                        break;
                                    }
                                }
                            }
                        }
                    }

                    if (dwInsert != -1)
                    {
                        wsprintfW(wszName, L"%d", dwInsert);
                        WCHAR wszPath[MAX_PATH+2]; // cannot party on wszParent since we may need it bellow

                        PathCombine(wszPath+1, wszParent, L"*");
                        wszPath[0] = L'"';
                        iEnd = lstrlenW(wszPath);
                        wszPath[iEnd] = L'"';
                        wszPath[iEnd+1] = L'\0';
                        RegSetValueExW(hkey, wszName, 0, REG_SZ, (BYTE *)wszPath, (lstrlenW(wszPath)+1)*SIZEOF(WCHAR));
                    }
                    RegCloseKey(hkey);
                }
            }

            // now that we took care of this folder for the future, let's produce some kind of pidl so that defview
            // doesn't have blank items...
            if (!pidl)
            {
                LPWSTR pwszFolder;

                if (fTranslated)
                    pwszFolder = wszTranslatedParent;
                else
                    pwszFolder = wszParent;
                PathRemoveBackslashW(pwszFolder);
                do
                {
                    if (!PathRemoveFileSpecW(pwszFolder))
                        break;
                    hres = SHILCreateFromPath(pwszFolder, &pidl, NULL);
                }
                while (FAILED(hres) && hres != E_OUTOFMEMORY);
            }

            if (pidl)
            {
                LPITEMIDLIST pidlChild = ILClone(ILFindLastID(pidl));

                ASSERT(!*ppidl);
                if (pidlChild)
                {
                    if (ILRemoveLastID(pidl))
                    {
                        hres = _pdfFolder->AddFolderToFolderList(pidl, TRUE, &_iFolder);
                        // clear the cache
                        ATOMICRELEASE(_psfCurrentDir);
                        _szCurrentDir[0] = TEXT('\0');
                        // now we have to patch the pidl to be docfind pidl.  for that we use the special items finddata
                        // we could hit the disk but is there any point in doing so?  we have to use the item's rank value
                        // just so that sorting can appear to work (on the fly -- done by ci).
                        if (SUCCEEDED(hres))
                        {
                            DWORD dwItemID;
                            
                            GetItemID(iItem, &dwItemID);
                            *ppidl = DocFind_AppendDocFindData(pidlChild, _iFolder, DFDF_EXTRADATA, iItem, dwItemID, data[5]->ulVal);
                            // pidlChild is destroyed by DocFind_AppendDocFindData so cannot free it here..
                            pidlChild = NULL;
                        }
                    }
                    ILFree(pidlChild);
                }
            }
            hres = *ppidl? S_OK : E_FAIL;
         }
        ILFree(pidl);
#ifdef DEBUG
        if (FAILED(hres))
            TraceMsg(TF_DOCFIND, "CI: skipping %ls\\%ls \n", wszParent, fd.cFileName);
#endif
    }

    return hres;
}


HRESULT CDFOleDBEnum::GetExtendedDetailsOf(LPCITEMIDLIST pidl, UINT iCol, LPSHELLDETAILS pdi)
{
#ifdef IF_ADD_MORE_COLS

    // 
    PCHIDDENDOCFINDDATA phdfd = (PCHIDDENDOCFINDDATA) ILFindHiddenID(pidl, IDLHID_DOCFINDDATA);
    if (!phdfd || !(phdfd->wFlags & DFDF_EXTRADATA))
        return E_FAIL;

    HRESULT hres = _CacheRowSet(phdfd->uRow);
    if (hres != S_OK)
        return hres;    // either error or end of list...

    PROPVARIANT* data[c_cDbCols];
    
    hres = _pRowset->GetData(_ahrow[phdfd->uRow-_ihrowFirst], _hAccessor, &data);
    if (S_OK == hres)
    {
        // The first members are associated with PIDL and not returned directly
        //     data[0].pwszVal is the file name
        //     data[1].pwszVal is the full path (including file name)
        //     data[2].ulVal is the attribute
        //     data[3].ulVal is the size in byte
        //     data[4].filetime is the last write time in UTC
        // The first column to remove
        //     data[5].ulVal is the rank of the item... - ICol == 0
        // BUGBUG:: currently hard wacked to only support COL = 0 (rank...)
        ASSERT (iCol == 0);
        TCHAR   szNum[MAX_INT64_SIZE];
        AddCommas(data[5]->ulVal, szNum);
#ifdef UNICODE
        pdi->str.pOleStr = (LPOLESTR)SHAlloc((lstrlen(szNum)+1)*SIZEOF(TCHAR));
        if ( pdi->str.pOleStr != NULL ) {
            pdi->str.uType = STRRET_WSTR;
            lstrcpy(pdi->str.pOleStr, szNum);
        } else {
            return E_OUTOFMEMORY;
        }
#else
        lstrcpy(pdi->str.cStr, szNum);
#endif
    }
    return hres;
#else
    return E_FAIL;
#endif
}


HRESULT CDFOleDBEnum::GetExtendedDetailsULong(LPCITEMIDLIST pidl, UINT iCol, ULONG *pul)
{
    PCHIDDENDOCFINDDATA phdfd = (PCHIDDENDOCFINDDATA) ILFindHiddenID(pidl, IDLHID_DOCFINDDATA);

    *pul = 0;
    if (!phdfd || !(phdfd->wFlags & DFDF_EXTRADATA))
        return E_FAIL;

    HRESULT hres = _CacheRowSet(phdfd->uRow);
    if (hres != S_OK)
        return hres;    // either error or end of list...

    PROPVARIANT* data[c_cDbCols];
    
    hres = _pRowset->GetData(_ahrow[phdfd->uRow-_ihrowFirst], _hAccessor, &data);
    if (S_OK == hres)
    {
        // The first members are associated with PIDL and not returned directly
        //     data[0].pwszVal is the file name
        //     data[1].pwszVal is the full path (including file name)
        //     data[2].ulVal is the attribute
        //     data[3].ulVal is the size in byte
        //     data[4].filetime is the last write time in UTC
        // The first column to remove
        //     data[5].ulVal is the rank of the item... - ICol == 0
        // BUGBUG:: currently hard wacked to only support COL = 0 (rank...)
        ASSERT (iCol == 0);
        *pul = data[5]->ulVal;
    }
    return hres;
}


HRESULT CDFOleDBEnum::GetItemID(UINT iItem, DWORD *puItemID)
{
    *puItemID = (UINT)-1;
    HRESULT hres = _CacheRowSet(iItem);
    if (hres != S_OK)
        return hres;    // either error or end of list...

    PROPVARIANT* data[1];
    
    hres = _pRowset->GetData(_ahrow[iItem-_ihrowFirst], _hAccessorWorkID, &data);
    if (S_OK == hres)
    {
        // Only one data column so this is easy...
        // The ULVal is the thing we are after...
        *puItemID = data[0]->ulVal;
    }

    return hres;
}

HRESULT CDFOleDBEnum::SortOnColumn(UINT iCol, BOOL fAscending)
{
    // Ok We need to generate the Sort String... 
    return _BuildAndSetCommandTree(iCol, fAscending);             
}

HRESULT CDFOleDBEnum::SetOwner(struct IUnknown* punkOwner)
{
    // Used to set the docfind folder and from that the filter.
    ATOMICRELEASE(_pdfff);
    ATOMICRELEASE(_pdfFolder);

    if (punkOwner)
    {
        punkOwner->QueryInterface(IID_IDocFindFolder, (void **)&_pdfFolder);
        if (_pdfFolder)
            _pdfFolder->GetDocFindFilter(&_pdfff);
    }
    return S_OK;
}


//
//
//
HRESULT CDFOleDBEnum::_MapColumns(
    IUnknown *   pUnknown,
    DBORDINAL    cCols,
    DBBINDING *  pBindings,
    const DBID * pDbCols,
    HACCESSOR &  hAccessor )
{
    IColumnsInfo * pColumnsInfo = 0;
    DBORDINAL aMappedColumnIDs[c_cDbCols];

    HRESULT sc = pUnknown->QueryInterface( IID_IColumnsInfo,
                                         (void **)&pColumnsInfo);
    if ( FAILED( sc ) )
        return sc;

    sc = pColumnsInfo->MapColumnIDs(cCols, pDbCols, aMappedColumnIDs);

    pColumnsInfo->Release();

    if ( FAILED( sc ) )
        return sc;

    for (ULONG i = 0; i < cCols; i++)
        pBindings[i].iOrdinal = aMappedColumnIDs[i];

    IAccessor * pIAccessor = 0;

    sc = pUnknown->QueryInterface( IID_IAccessor, (void **)&pIAccessor);
    if ( FAILED( sc ) )
        return sc;

    hAccessor = 0;
    sc = pIAccessor->CreateAccessor( DBACCESSOR_ROWDATA,
                                     cCols, pBindings, 0, &hAccessor, 0 );

    pIAccessor->Release();

    return sc;
}


//
//
//
void CDFOleDBEnum::_ReleaseAccessor()
{
    IAccessor * pIAccessor = 0;

    HRESULT sc = _pRowset->QueryInterface(IID_IAccessor, (void **)&pIAccessor);
    if (FAILED(sc))
        return;

    if (_hAccessor)
        pIAccessor->ReleaseAccessor(_hAccessor, 0);
    if (_hAccessorWorkID)
        pIAccessor->ReleaseAccessor(_hAccessorWorkID, 0);

    pIAccessor->Release();
}

HRESULT CDFOleDBEnum::_CacheRowSet(UINT iItem)
{
    HRESULT hres=S_OK;

    if (!_pRowset)
        return E_FAIL;
    
    if (!_cRows || !InRange(iItem, _ihrowFirst, _ihrowFirst+(UINT)_cRows-1) || (iItem == (UINT)-1))
    {
        // Release the last cached element we had.
        if (_cRows != 0)
            _pRowset->ReleaseRows(ARRAYSIZE(_ahrow), _ahrow, 0, 0, 0);

        // See if we are simply releasing our cached data...
        _cRows = 0;
        _ihrowFirst = (UINT)-1;
        if (iItem == (UINT)-1)
            return S_OK;

        // Ok try to read in the next on...
        BYTE bBookMark = (BYTE) DBBMK_FIRST;
        HROW *rghRows = (HROW *)_ahrow;

        // change this to fetch 100 or so rows at the time -- huge perf improvment
        hres = _pRowset->GetRowsAt(0, 0, sizeof(bBookMark), &bBookMark, iItem, ARRAYSIZE(_ahrow), &_cRows, &rghRows);
        if (FAILED(hres))
            return hres;
            
        _ihrowFirst = iItem;

        if ((DB_S_ENDOFROWSET == hres) || (_cRows == 0))
        {
            if (_cRows == 0)
                _ihrowFirst = -1;
            else
                hres = S_OK;  // we got some items and caller expects S_OK so change DB_S_ENDOFROWSET to noerror
        }
    }

    return hres;
}

void CDFOleDBEnum::HandleWatchNotify(DBWATCHNOTIFY eChangeReason)
{
    // For now we will simply Acknoledge the change...
    if (_prwn)
        _prwn->OnChange(NULL, eChangeReason);

    // Lets see what all is in the list to see if it works or not...
#ifdef RICHER_NOTIFY_IS_FIXED
    ULONG ulChanges;
    DBROWWATCHCHANGE *pdbrwc;

    DebugMsg(TF_DOCFIND, TEXT("sh TR - CDFOleDBEnum::HandleWatchNotify Reason(%d) RowsetWatch->Refresh called hres=%lx,  changes %d"),
             eChangeReason,hres, ulChanges);
    if (hres == S_OK)
    {
        ULONG i;
        for (i=0; i < ulChanges; i++)
        {
            DebugMsg(TF_DOCFIND, TEXT("          I:%d, Change:%d iRow:%d"),
                     i, pdbrwc->eChangeKind, pdbrwc->iRow);
            if (pdbrwc->hRow)
                _pRowset->ReleaseRows(1, &pdbrwc->hRow, 0, 0, 0);
            pdbrwc++;
        }
    }
#endif
}



//
// Notes:
//
HRESULT CDFOleDBEnum::_SetCmdProp(ICommand *pCommand)
{
#define MAX_PROPS 8

    DBPROPSET aPropSet[MAX_PROPS];
    DBPROP aProp[MAX_PROPS];
    ULONG cProps = 0;
    HRESULT sc;

    // asynchronous query

    aProp[cProps].dwPropertyID = DBPROP_IRowsetAsynch;
    aProp[cProps].dwOptions = 0;
    aProp[cProps].dwStatus = 0;
    aProp[cProps].colid = c_dbcolNull;
    aProp[cProps].vValue.vt = VT_BOOL;
    aProp[cProps].vValue.boolVal = VARIANT_TRUE;

    aPropSet[cProps].rgProperties = &aProp[cProps];
    aPropSet[cProps].cProperties = 1;
    aPropSet[cProps].guidPropertySet = c_guidRowsetProps;

    cProps++;

    // don't timeout queries

    aProp[cProps].dwPropertyID = DBPROP_COMMANDTIMEOUT;
    aProp[cProps].dwOptions = DBPROPOPTIONS_SETIFCHEAP;
    aProp[cProps].dwStatus = 0;
    aProp[cProps].colid = c_dbcolNull;
    aProp[cProps].vValue.vt = VT_I4;
    aProp[cProps].vValue.lVal = 0;

    aPropSet[cProps].rgProperties = &aProp[cProps];
    aPropSet[cProps].cProperties = 1;
    aPropSet[cProps].guidPropertySet = c_guidRowsetProps;

    cProps++;

    // We can handle PROPVARIANTs

    aProp[cProps].dwPropertyID = DBPROP_USEEXTENDEDDBTYPES;
    aProp[cProps].dwOptions = DBPROPOPTIONS_SETIFCHEAP;
    aProp[cProps].dwStatus = 0;
    aProp[cProps].colid = c_dbcolNull;
    aProp[cProps].vValue.vt = VT_BOOL;
    aProp[cProps].vValue.boolVal = VARIANT_TRUE;

    aPropSet[cProps].rgProperties = &aProp[cProps];
    aPropSet[cProps].cProperties = 1;
    aPropSet[cProps].guidPropertySet = c_guidQueryExt;

    cProps++;

#ifdef CI_ROWSETNOTIFY
    aProp[cProps].dwPropertyID = DBPROP_ROWSET_ASYNCH;
    aProp[cProps].dwOptions = DBPROPOPTIONS_REQUIRED;
    aProp[cProps].dwStatus = 0;
    aProp[cProps].colid = c_dbcolNull;
    aProp[cProps].vValue.vt = VT_I4;
    aProp[cProps].vValue.iVal = DBPROPVAL_ASYNCH_INITIALIZE | 
                                DBPROPVAL_ASYNCH_SEQUENTIALPOPULATION |
                                DBPROPVAL_ASYNCH_RANDOMPOPULATION;

    aPropSet[cProps].rgProperties = &aProp[cProps];
    aPropSet[cProps].cProperties = 1;
    aPropSet[cProps].guidPropertySet = c_guidRowsetProps;

    cProps++;
#endif
    ICommandProperties * pCmdProp = 0;
    sc = pCommand->QueryInterface( IID_ICommandProperties,
                                   (void **)&pCmdProp );
    if (SUCCEEDED(sc))
    {
        sc = pCmdProp->SetProperties( cProps, aPropSet );
        pCmdProp->Release();
    }

    return sc;
}


//
// Notes:
//
HRESULT CDFOleDBEnum::_BuildAndSetCommandTree(int iCol, BOOL fReverse)
{
    HRESULT hres;

    ICommandTree *pCmdTree = NULL;
    DBCOMMANDTREE * pTree = NULL;
               
    LPWSTR pwszRestrictions = NULL;
    ULONG  ulDialect = ISQLANG_V2 ;
    DWORD  dwGQRFlags;
    LPWSTR pwszSortBy = NULL;
        
    // create the query command string
    if (FAILED(hres = _pdfff->GenerateQueryRestrictions(&pwszRestrictions, &dwGQRFlags)))
        goto Abort;

    if( FAILED(hres = _pdfff->GetQueryLanguageDialect( &ulDialect )) )
        goto Abort;

    // BUGBUG: This is hard coded to our current list of columns, but what the heck...
    WCHAR  wszSort[80];      // use this to sort by different columns...

    if ((iCol >= 0) && (c_awszColSortNames[iCol] != NULL))
    {
        // BUGBUG we are ignoring direction right now...
        StrCpyW(wszSort, c_awszColSortNames[iCol]); 
        StrCatW(wszSort, (LPWSTR)L",Path[a],FileName[a]");
        pwszSortBy = wszSort;
    }
        
    hres = CITextToFullTreeEx(pwszRestrictions,  // the query
                          ulDialect,            
                          c_wszColNames, // return these columns
                          pwszSortBy,
                          0,          // no grouping
                          &pTree,
                          0,          // no custom properties
                          0,          // no custom properties
                          LOCALE_USER_DEFAULT);

    if (FAILED(hres)) 
    {
        // Map this to one that I know about
        // Note: We will only do this if we require CI else we will try to fallback to old search...
        // Note we are running into problems where CI says we are contained in a Catalog even if
        // CI process is not running... So try to avoid this if possible
        if (dwGQRFlags & GQR_REQUIRES_CI)
            hres = MAKE_HRESULT(3, FACILITY_SEARCHCOMMAND, SCEE_CONSTRAINT);
        goto Abort;
    }

    hres = _pCommand->QueryInterface(IID_ICommandTree, (void **)&pCmdTree);
    if (SUCCEEDED(hres))
    {    
        hres = pCmdTree->SetCommandTree(&pTree, DBCOMMANDREUSE_NONE, FALSE);
        pCmdTree->Release();
    }
    
    // Fall through to cleanup code...

Abort:                
    // Warning... Since a failure return from this function will
    // release this class, all of the allocated items up till the failure should
    // be released...
    if (pwszRestrictions)
        LocalFree((HLOCAL)pwszRestrictions);

    return hres;
}

extern "C" HRESULT QueryCIStatus(LPDWORD pdwStatus, LPBOOL pbConfigAccess); //in fsearch.cpp

HRESULT CDFOleDBEnum::DoQuery(LPWSTR *apwszPaths, UINT *pcPaths)
{
    UINT nPaths = *pcPaths;
    WCHAR** aScopes;
    WCHAR** aScopesOrig;
    ULONG* aDepths = NULL;
    WCHAR** aCatalogs;
    WCHAR** aMachines;
    
    // List of paths to search...

    WCHAR wszPath[MAX_PATH];
    LPWSTR pwszPath = wszPath;
    LPWSTR pwszMachine, pwszCatalog;
    UINT i;
    UINT iPath = 0;
    HRESULT hres;
    DWORD dwGQR_Flags;

    // Initiailize all of our query values back to unused 
    _hAccessor = NULL;
    _hAccessorWorkID = NULL;
    _pRowset = NULL;
    _pRowsetAsync = NULL;
    _pCommand = NULL;

    DBBINDING aPropMainCols[c_cDbCols] =
    {
        { 0,4*0,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
        { 0,4*1,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
        { 0,4*2,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
        { 0,4*3,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
        { 0,4*4,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 },
        { 0,4*5,0,0,0,0,0, DBPART_VALUE, DBMEMOWNER_PROVIDEROWNED, DBPARAMIO_NOTPARAM, 0, 0,  DBTYPE_VARIANT|DBTYPE_BYREF, 0, 0 }
    };

    // Get array of search paths...
#define MAX_MACHINE_NAME_LEN    32
#define MAX_CATALOG_NAME_LEN    MAX_PATH

    
    DWORD dwStatus;
    BOOL  bPermission;
    BOOL  bIsCIRunning = TRUE;
    if (SUCCEEDED(QueryCIStatus(&dwStatus, &bPermission)))
    {
        if (!(dwStatus == SERVICE_START_PENDING || dwStatus == SERVICE_RUNNING || dwStatus == SERVICE_CONTINUE_PENDING))
        {
            bIsCIRunning = FALSE;
        }
    }
    
    // First pass see if we have anything that make use at all of CI if not lets simply bail and let
    // old code walk the list...
    hres = _pdfff->GenerateQueryRestrictions(NULL, &dwGQR_Flags);
    if (FAILED(hres))
        goto Abort;

    if ((dwGQR_Flags & GQR_MAKES_USE_OF_CI) == 0)
    {
        hres = S_FALSE;
        goto Abort;
    }

    // allocate the arrays that we need to pass to CIMakeICommand and
    // the buffers needed for the machine name and catalog name
    //
    // i is the size we need for one search path.
    // nPaths * (1 ULONG + 4 WCHAR* + machine name buffer + catalog name buffer).
    i = sizeof(ULONG) + (4 * sizeof(WCHAR*)) + 
        ((MAX_MACHINE_NAME_LEN + MAX_CATALOG_NAME_LEN) * sizeof(WCHAR));

    aDepths = (ULONG*)LocalAlloc(LPTR, nPaths * i);
    if (!aDepths)
    {
        hres = E_OUTOFMEMORY;
        goto Abort;
    }

    // This following loop does two things,
    //  1. Check if all the scopes are indexed, if any one scope is not,
    //      fail the call and we'll do the win32 find.
    //  2. Prepare the arrays of parameters that we need to pass to
    //      CIMakeICommand().
    //
    aScopes = (WCHAR**)&aDepths[nPaths];
    aScopesOrig = (WCHAR**)&aScopes[nPaths];
    aCatalogs = (WCHAR**)&aScopesOrig[nPaths];
    aMachines = (WCHAR**)&aCatalogs[nPaths];
    pwszCatalog = (LPWSTR)&aMachines[nPaths];
    pwszMachine = (LPWSTR)&pwszCatalog[MAX_CATALOG_NAME_LEN];
    
    for (i = 0; i < nPaths; i++)
    {
        // specify the depth

        // assign the search to scope
        
        ULONG cchMachine = MAX_MACHINE_NAME_LEN;
        ULONG cchCatalog = MAX_CATALOG_NAME_LEN;
        WCHAR wszUNCPath[MAX_PATH];
        bool fRemapped = FALSE;

        // if CI is not running we can still do ci queries on a remote drive (if it is  indexed)
        // so we cannot just bail if ci is not running on user's machine
        if (!bIsCIRunning && !PathIsUNCW(apwszPaths[i]) && !IsRemoteDrive(PathGetDriveNumberW(apwszPaths[i])))
            continue;  // do grep on this one
        hres = LocateCatalogsW(apwszPaths[i], 0, pwszMachine, &cchMachine, pwszCatalog, &cchCatalog);
        if (hres != S_OK)
        {
            // see if by chance this is a network redirected drive.  If so we CI does not handle
            // these.  See if we can remap to UNC path to ask again...
            if (!PathIsUNC(apwszPaths[i]))
            {
                DWORD dwType;
                DWORD nLength = ARRAYSIZE(wszUNCPath);
                     
                // BUGBUG:: this api takes TCHAR, but we only compile this part for
                // WINNT...
                dwType = SHWNetGetConnection(apwszPaths[i], wszUNCPath, &nLength);
                if ((dwType == NO_ERROR) || (dwType == ERROR_CONNECTION_UNAVAIL))
                {
                    LPWSTR  pwsz;
                    
                    fRemapped = TRUE;
                    pwsz = PathSkipRootW(apwszPaths[i]);
                    if (pwsz)
                        PathAppendW(wszUNCPath, pwsz);
                    hres = LocateCatalogsW(wszUNCPath, 0, pwszMachine, &cchMachine, pwszCatalog, &cchCatalog);
                }
            }
        }
        if (hres != S_OK)
        {
            if (_grfFlags & DFOO_OLDFINDCODE)
            {
                hres = E_FAIL;  // OLD UI did not know about mixing CI and NON-CI...
                goto Abort;
            }
            else
                continue;   // this one is not indexed.
        }

        CI_STATE state={0};

        state.cbStruct = SIZEOF(state);
        if (SUCCEEDED(CIState(pwszCatalog, pwszMachine, &state)))
        {
            BOOL fUpToDate = ((0 == state.cDocuments ) &&
                              (0 == (state.eState & CI_STATE_SCANNING)) &&
                              (0 == (state.eState & CI_STATE_READING_USNS)) &&
                              (0 == (state.eState & CI_STATE_STARTING)) &&
                              (0 == (state.eState & CI_STATE_RECOVERING)));

            if (!fUpToDate)
            {
                if (dwGQR_Flags & GQR_REQUIRES_CI)
                {
                    // ci not up to date and we must use it..
                    // inform the user that results may not be complete
                    if (!(_grfWarnings & DFW_IGNORE_INDEXNOTCOMPLETE))
                    {
                        hres = MAKE_HRESULT(3, FACILITY_SEARCHCOMMAND, SCEE_INDEXNOTCOMPLETE);
                        goto Abort;
                    }
                    //else use ci although index is not complete
                }
                else
                {
                    // ci is not upto date so just use grep for this drive so user can get
                    // complete results
                    pwszCatalog[0] = L'\0'; 
                    pwszMachine[0] = L'\0';
                    continue;
                }
            }
        }

        aDepths[iPath] = (_grfFlags & DFOO_INCLUDESUBDIRS) ? QUERY_DEEP : QUERY_SHALLOW;
        aScopesOrig[iPath] = apwszPaths[i];
        if (fRemapped)
        {
            aScopes[iPath] = (LPWSTR)LocalAlloc(LPTR, (lstrlenW(wszUNCPath)+1)*sizeof(WCHAR));
            if (aScopes[iPath] == NULL)
            {
                hres = E_OUTOFMEMORY;
                goto Abort;
            }
            StrCpyW(aScopes[iPath], wszUNCPath);
        }
        else
            aScopes[iPath] = apwszPaths[i];

        aCatalogs[iPath] = pwszCatalog;
        aMachines[iPath] = pwszMachine;
        
        // advance the catalog and machine name buffer
        pwszCatalog = (LPWSTR)&pwszMachine[MAX_MACHINE_NAME_LEN];
        pwszMachine = (LPWSTR)&pwszCatalog[MAX_CATALOG_NAME_LEN];
        iPath++;    // increment our out pointer...
    }

    if (iPath == 0) 
    {
        // nothing found;  - We should check to see if by chance the user specified a query that
        // is CI based if so error apapropriately...
        hres = (dwGQR_Flags & GQR_REQUIRES_CI)? MAKE_HRESULT(3, FACILITY_SEARCHCOMMAND, SCEE_INDEXSEARCH) : S_FALSE;
        goto Abort;
    }

    // Get ICommand.
    hres = CIMakeICommand(&_pCommand,
                               iPath,
                               aDepths,
                               (WCHAR const * const *) aScopes,
                               (WCHAR const * const *) aCatalogs,
                               (WCHAR const * const *) aMachines);

    if (FAILED(hres))
        goto Abort;

        
    // create the query command string - Assume default sort...
    if (FAILED(hres = _BuildAndSetCommandTree(_iColSort, FALSE)))
        goto Abort; 

// commenting out for now. in the future we should inform the user that not all the drives are indexed and ask
// if they want to continue doing search on the ones that are indexed.
    if ((dwGQR_Flags & GQR_REQUIRES_CI) && (nPaths != iPath))
    {
        //  check warning flags to see if we should ignore and continue
        if (0 == (_grfWarnings & DFW_IGNORE_CISCOPEMISMATCH))
        {
            hres = MAKE_HRESULT(3, FACILITY_SEARCHCOMMAND, SCEE_SCOPEMISMATCH);
            goto Abort;
        }
    }

    // Get IRowset.
    _SetCmdProp(_pCommand);
    if (FAILED(hres = _pCommand->Execute( 0,              // no aggr. IUnknown
                               IID_IRowsetLocate,    // IID for i/f to return
                               0,              // disp. params
                               0,              // chapter
                               (IUnknown **) &_pRowset )))
        goto Abort;

    // we have the IRowset.
    // Real work to get the Accessor
    if (FAILED(hres = _MapColumns( _pRowset, c_cDbCols, aPropMainCols,
                     c_aDbCols, _hAccessor )))
        goto Abort;

    // OK lets also get the accessor for the WorkID...
    if (FAILED(hres = _MapColumns( _pRowset, ARRAYSIZE(c_aDbWorkIDCols), aPropMainCols,
                     c_aDbWorkIDCols, _hAccessorWorkID )))
        goto Abort;

    if (FAILED(hres = _pRowset->QueryInterface( IID_IRowsetAsynch,
                                        (void **) &_pRowsetAsync)))
        goto Abort;

#ifdef CI_ROWSETNOTIFY
    // And setup our notification object and register it with the rowset
    IConnectionPointContainer *pcont;

    // If we have an advise going, remove it now
    // Will not totally bail out if these are not supported for the query.
    _pdbrWatchNotify = new CDFOleDBRowsetWatchNotify(this);
    if (_pdbrWatchNotify)
    {    
        if (SUCCEEDED(_pRowset->QueryInterface(IID_IConnectionPointContainer, (PVOID*)&pcont)))
        {
            // Hold on to the connection point as if we release it, it looks like they
            // release the rest...
            if (SUCCEEDED(pcont->FindConnectionPoint(IID_IRowsetNotify, &_pcpointWatchNotify)))
            {
                if (_pcpointWatchNotify)
                    _pcpointWatchNotify->Advise((IUnknown*)_pdbrWatchNotify, &_dwAdviseID);
            }
            pcont->Release();
        }
    }
#endif //#ifdef CI_ROWSETNOTIFY

    // If we got here than at least some of our paths are indexed
    // we may need to compress the list down of the ones we did not handle...
    *pcPaths = (nPaths - iPath);  // Let caller know how many we did not process

    // need to move all the ones we did not process to the start of the list...
    // we always process this list here as we may need to allocate translation lists to be used to
    // translate the some UNCS back to the mapped drive the user passed in.

    UINT j;
    UINT iInsert;
    iPath--;    // make it easy to detect 
    j = 0;
    iInsert = 0;
    for (i = 0; i < nPaths; i++) 
    {
        if (aScopesOrig[j] == apwszPaths[i])
        {
            if (aScopesOrig[j] != aScopes[j])
            {
                // There is a translation in place.
                DFODBET *pdfet = (DFODBET*)LocalAlloc(LPTR, sizeof(DFODBET));
                if (pdfet)
                {
                    pdfet->pdfetNext = _pdfetFirst;
                    _pdfetFirst = pdfet;
                    pdfet->pwszFrom = aScopes[j];
                    pdfet->cbFrom = lstrlenW(pdfet->pwszFrom);
                    pdfet->pwszTo = aScopesOrig[j];
                    aScopes[j] = aScopesOrig[j];    // Make sure loop below does not delete pwszFrom
                    apwszPaths[i] = NULL;           // Likewise for pswsTo...
                }

            }
            if (apwszPaths[i])
                LocalFree((HLOCAL)apwszPaths[i]);
            if (j < iPath)
                j++;
        }
        else
            apwszPaths[iInsert++] = apwszPaths[i]; // move to right place
    }
    iPath++;    // setup to go through cleanupcode...

     // Fall through to cleanup code...

Abort:                
    // Warning... Since a failure return from this function will
    // release this class, most all of the allocated items up till the failure should
    // be released...   Also cleanup any paths we may have allocated...
    for (i = 0; i < iPath; i++) 
    {
        if (aScopesOrig[i] != aScopes[i])
            LocalFree(aScopes[i]);
    }

    if (aDepths)
        LocalFree(aDepths);

    return hres;
}

#ifdef CI_ROWSETNOTIFY
//=============================================================================================
// Implemention of notification callback class for above 
CDFOleDBRowsetWatchNotify::CDFOleDBRowsetWatchNotify(CDFOleDBEnum *pdfoe):
    _cRef(1)
{
    // We did not addref as this could cause circular reference...
    // need to be carefull in our managment of this pointer...
    _pdfoe = pdfoe;
}


CDFOleDBRowsetWatchNotify::~CDFOleDBRowsetWatchNotify()
{
}

HRESULT CDFOleDBRowsetWatchNotify::QueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CDFOleDBRowsetWatchNotify, IRowsetNotify),          //IID_IRowsetWatchNotify
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CDFOleDBRowsetWatchNotify::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CDFOleDBRowsetWatchNotify::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}
                                                 
HRESULT CDFOleDBRowsetWatchNotify::OnRowChange(
        IRowset *pRowset, ULONG cColumns, const HROW rgColumns[], DBREASON eReason, DBEVENTPHASE ePhase, BOOL fCantDeny)
{
    if (eReason == DBREASON_ROW_ASYNCHINSERT || eReason == DBREASON_ROW_INSERT)
    {
        _pdfoe->HandleWatchNotify(eReason);
        return S_OK;
    }
    return DB_S_UNWANTEDREASON;
}
#endif // #ifdef CI_ROWSETNOTIFY

#endif
