//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: netfind.c
//
// Description: This file contains the net specific search code that is
// needed for the find computer code.
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop
#include "shellids.h" // new help ids are stored here
#include "docfind.h"
#include "netview.h"
#include "prop.h"

#define NET_TIMINGS

#ifdef NET_TIMINGS
int     NTF_cNoPEnum = 0;
int     NTF_dtNoPEnum = 0;
int     NTF_cNextItem = 0;
int     NTF_dtNextItem = 0;
int     NTF_dtTime = 0;
#endif

STDAPI GetDomainWorkgroupIDList(LPITEMIDLIST *ppidl);
STDAPI CNetFindInDS_CreateInstance(LPCTSTR pszCompName, IShellFolder *psf, IDFEnum **ppdfe);

enum
{
    INFCOL_NAME = 0,
    INFCOL_PATH,
    INFCOL_COMMENT,
};

const  COL_DATA c_findcomp_cols[] = {
    {INFCOL_NAME,       IDS_NAME_COL,       20, LVCFMT_LEFT,    &SCID_NAME},
    {INFCOL_PATH,       IDS_WORKGROUP_COL,  20, LVCFMT_LEFT,    &SCID_DIRECTORY},
    {INFCOL_COMMENT,    IDS_COMMENT_COL,    20, LVCFMT_LEFT,    &SCID_Comment}
};


#define DFM_DEFERINIT   (WM_USER+42)
//
// REVIEW:: The recursive code in this module has been totally neutered to
// make the ITG group happy.  IE we mad this functional mostly usless and
// wasted a lot of time doing so...  The ifdefs are under #ifdef CASTRATED
//


//===========================================================================
// Define the Default data filter data structures
//===========================================================================

// From netviewx.c
STDMETHODIMP CNETDetails_GetDetailsOf(IShellDetails * psd, LPCITEMIDLIST pidl, UINT iCol, LPSHELLDETAILS lpDetails);
//
// Define the internal structure of our default filter
typedef struct _CNETFilter   // fff
{
    IDocFindFileFilter  dfff;
    UINT                cRef;

    HWND                hwndTabs;

    HANDLE              hMRUSpecs;

    LPITEMIDLIST        pidlStart;      // Where to start the search from.

    // Data associated with the file name.
    LPTSTR              pszCompName;   // the one we do compares with
    TCHAR               szUserInputCompName[MAX_PATH];  // User input

} CNETFilter;


// Define common page data for each of our pages
// WARNING the fields in this must align the same as the definition
// in docfind2.c

typedef struct { // dfpsp
    PROPSHEETPAGE   psp;
    HANDLE          hThreadInit;
    HWND            hwndDlg;
    CNETFilter *    pdff;
    DWORD           dwState;
} DOCFINDPROPSHEETPAGE, * LPDOCFINDPROPSHEETPAGE;


#define DFPAGE_INIT     0x0001          /* This page has been initialized */
#define DFPAGE_CHANGE   0x0002          /*  The user has modified the page */

//===========================================================================
// Prototypes of some of the internal functions.
//===========================================================================

BOOL_PTR CALLBACK DocFind_CCOMPFNameLocDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam);



//===========================================================================
// Define some other module global data
//===========================================================================

const DFPAGELIST  s_CCOMPFplComp[] =
{
    {DLG_NFNAMELOC, DocFind_CCOMPFNameLocDlgProc},
};

// Some global strings...
const TCHAR s_szCompSpecMRU[] = REGSTR_PATH_EXPLORER TEXT("\\FindComputerMRU");




//==========================================================================
//
// Create the default filter for our find code...  They should be completly
// self contained...
//

//===========================================================================
// CNETFilter : member prototype
//===========================================================================
STDMETHODIMP CNETFilter_QueryInterface(IDocFindFileFilter *pnetf, REFIID riid, void **ppvObj);
STDMETHODIMP_(ULONG) CNETFilter_AddRef(IDocFindFileFilter *pnetf);
STDMETHODIMP_(ULONG) CNETFilter_Release(IDocFindFileFilter *pnetf);
STDMETHODIMP CNETFilter_GetIconsAndMenu (IDocFindFileFilter *pdfff,
        HWND hwndDlg, HICON *phiconSmall, HICON *phiconLarge, HMENU *phmenu);
STDMETHODIMP CNETFilter_GetStatusMessageIndex (IDocFindFileFilter *pdfff,
        UINT uContext, UINT *puMsgIndex);
STDMETHODIMP CNETFilter_GetFolderMergeMenuIndex (IDocFindFileFilter *pdfff,
        UINT *puBGMainMergeMenu, UINT *puBGPopupMergeMenu);
STDMETHODIMP CNETFilter_AddPages(IDocFindFileFilter *pnetf, HWND hwndTabs,
        LPITEMIDLIST pidlStart);
STDMETHODIMP CNetFilter_FFilterChanged(IDocFindFileFilter *pdfff);
STDMETHODIMP CNETFilter_GenerateTitle(IDocFindFileFilter *pnetf,
        LPTSTR *ppszTitle, BOOL fFileName);
STDMETHODIMP CNETFilter_ClearSearchCriteria(IDocFindFileFilter *pnetf);
STDMETHODIMP CNETFilter_PrepareToEnumObjects(IDocFindFileFilter *pnetf, DWORD *pdwFlags);
STDMETHODIMP CNETFilter_GetDetails(IDocFindFileFilter *pnetf, HDPA hdpaPidf, int iItem, UINT iCol, PDETAILSINFO pdi);
STDMETHODIMP CNETFilter_ColumnClick(IDocFindFileFilter *pnetf, HWND hwndDlg, UINT iColumn);
STDMETHODIMP CNETFilter_EnumObjects (IDocFindFileFilter *pnetf, IShellFolder *psf, LPITEMIDLIST pidl,
            DWORD grfFlags, int iCOlSort, LPTSTR pszProgressText, IRowsetWatchNotify *prwn, IDFEnum **ppdfenum) PURE;
STDMETHODIMP CNETFilter_GetDetailsOf(IDocFindFileFilter *pnetf, HDPA hdpaPidf, LPCITEMIDLIST pidl, UINT *piColumn, LPSHELLDETAILS pdi);
STDMETHODIMP CNETFilter_FDoesItemMatchFilter(IDocFindFileFilter *pnetf,
        LPTSTR pszFolder, WIN32_FIND_DATA * pfinddata,
        IShellFolder *psf, LPITEMIDLIST  pidl);
STDMETHODIMP CNETFilter_SaveCriteria(IDocFindFileFilter *pnetf, IStream *pstm, WORD fCharType);
STDMETHODIMP CNETFilter_RestoreCriteria(IDocFindFileFilter *pnetf,
        IStream * pstm, int cCriteria, WORD fCharType);
STDMETHODIMP CNETFilter_DeclareFSNotifyInterest(IDocFindFileFilter *pnetf, HWND hwndDlg, UINT uMsg);
STDMETHODIMP CNETFilter_GetColSaveStream(IDocFindFileFilter *pnetf, WPARAM wparam, LPSTREAM *ppstm);
STDMETHODIMP CNETFilter_GenerateQueryRestrictions(IDocFindFileFilter *pnetf, LPWSTR *ppwszQuery, DWORD *pdwGQRFlags);
STDMETHODIMP CNETFilter_ReleaseQuery(IDocFindFileFilter *pnetf);
STDMETHODIMP CNetFilter_UpdateField(IDocFindFileFilter *pdfff, BSTR szField, VARIANT vValue);
STDMETHODIMP CNetFilter_ResetFieldsToDefaults(IDocFindFileFilter *pdfff);
STDMETHODIMP CNetFilter_GetItemContextMenu(IDocFindFileFilter *pdfff, HWND hwndOwner, IDocFindFolder* pdfFolder, IContextMenu **ppcm);
STDMETHODIMP CNetFilter_GetDefaultSearchGUID(IDocFindFileFilter *pdfff, IShellFolder2 *psf2, LPGUID lpGuid);
STDMETHODIMP CNetFilter_EnumSearches(IDocFindFileFilter *pdfff, IShellFolder2 *psf2, LPENUMEXTRASEARCH *ppenum);
STDMETHODIMP CNetFilter_GetSearchFolderClassId(IDocFindFileFilter *pdfff, LPGUID lpGuid);
STDMETHODIMP CNetFilter_GetNextConstraint(IDocFindFileFilter *pdfff, VARIANT_BOOL fReset, BSTR *pName, VARIANT *pValue, VARIANT_BOOL *pfFound);
STDMETHODIMP CNetFilter_GetQueryLanguageDialect(IDocFindFileFilter *pdfff, ULONG * pulDialect) ;

IDocFindFileFilterVtbl c_CCOMPFFilterVtbl =
{
    CNETFilter_QueryInterface, CNETFilter_AddRef, CNETFilter_Release,
    CNETFilter_GetIconsAndMenu,
    CNETFilter_GetStatusMessageIndex,
    CNETFilter_GetFolderMergeMenuIndex,
    CNetFilter_FFilterChanged,
    CNETFilter_GenerateTitle,
    CNETFilter_PrepareToEnumObjects,
    CNETFilter_ClearSearchCriteria,
    CNETFilter_EnumObjects,
    CNETFilter_GetDetailsOf,
    CNETFilter_FDoesItemMatchFilter,
    CNETFilter_SaveCriteria,
    CNETFilter_RestoreCriteria,
    CNETFilter_DeclareFSNotifyInterest,
    CNETFilter_GetColSaveStream,
    CNETFilter_GenerateQueryRestrictions,
    CNETFilter_ReleaseQuery,
    CNetFilter_UpdateField,
    CNetFilter_ResetFieldsToDefaults,
    CNetFilter_GetItemContextMenu,
    CNetFilter_GetDefaultSearchGUID,
    CNetFilter_EnumSearches,
    CNetFilter_GetSearchFolderClassId,
    CNetFilter_GetNextConstraint,
    CNetFilter_GetQueryLanguageDialect,
};


//==========================================================================
// Creation function to create default find filter...
//==========================================================================
IDocFindFileFilter * CreateDefaultComputerFindFilter()
{
    CNETFilter *pfff = (void*)LocalAlloc(LPTR, SIZEOF(CNETFilter));
    if (pfff == NULL)
        return NULL;

    pfff->dfff.lpVtbl = &c_CCOMPFFilterVtbl;
    pfff->cRef = 1;

    // We should now simply return the filter
    return &pfff->dfff;

}

//==========================================================================
// Query interface for the docfind filter interface...
//==========================================================================

STDMETHODIMP CNETFilter_QueryInterface(IDocFindFileFilter *pnetf, REFIID riid, LPVOID FAR* ppvObj)
{
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);
    if (IsEqualIID(riid, &IID_IUnknown) || 
        IsEqualIID(riid, &IID_IDocFindFileFilter))
    {
        *ppvObj = pnetf;
    } 
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    CNETFilter_AddRef(pnetf);
    return NOERROR;    
}

//==========================================================================
// IDocFindFileFilter::AddRef
//==========================================================================
STDMETHODIMP_(ULONG) CNETFilter_AddRef(IDocFindFileFilter *pnetf)
{
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);
    this->cRef++;
    return(this->cRef);
}

//==========================================================================
// IDocFindFileFilter::Release
//==========================================================================
STDMETHODIMP_(ULONG) CNETFilter_Release(IDocFindFileFilter *pnetf)
{
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);
    this->cRef--;
    if (this->cRef>0)
    {
        return(this->cRef);
    }

    // Destroy the MRU Lists...

    if (this->hMRUSpecs)
        FreeMRUList(this->hMRUSpecs);

    // unless we do not have a combobox
    if (this->pidlStart)
        ILFree(this->pidlStart);

    Str_SetPtr(&(this->pszCompName), NULL);

    LocalFree((HLOCAL)this);
    return(0);
}

//==========================================================================
// IDocFindFileFilter::GetIconsAndMenu
//==========================================================================
STDMETHODIMP CNETFilter_GetIconsAndMenu (IDocFindFileFilter *pdfff,
        HWND hwndDlg, HICON *phiconSmall, HICON *phiconLarge, HMENU *phmenu)
{
    *phiconSmall = LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_COMPFIND),
            IMAGE_ICON, g_cxSmIcon, g_cySmIcon, LR_DEFAULTCOLOR);
    *phiconLarge  = LoadIcon(HINST_THISDLL, MAKEINTRESOURCE(IDI_COMPFIND));

    // Now for the menu
    *phmenu = LoadMenu(HINST_THISDLL, MAKEINTRESOURCE(MENU_FINDCOMPDLG));

    // BUGBUG:: Still menu to process!

    return NOERROR;
}

//==========================================================================
// Function to get the string resource index number that is proper for the
// current type of search.
//==========================================================================
STDMETHODIMP CNETFilter_GetStatusMessageIndex (IDocFindFileFilter *pdfff,
        UINT uContext, UINT *puMsgIndex)
{
    // Currently context is not used
    *puMsgIndex = IDS_COMPUTERSFOUND;

    return NOERROR;
}

//==========================================================================
// Function to let find know which menu to load to merge for the folder
//==========================================================================
STDMETHODIMP CNETFilter_GetFolderMergeMenuIndex (IDocFindFileFilter *pdfff,
        UINT *puBGMainMergeMenu, UINT *puBGPopupMergeMenu)
{
    *puBGPopupMergeMenu = POPUP_NETFIND_POPUPMERGE;
    return NOERROR;
}


STDMETHODIMP CNetFilter_GetItemContextMenu(IDocFindFileFilter *pdfff, HWND hwndOwner, IDocFindFolder* pdfFolder, IContextMenu **ppcm)
{
    *ppcm = CDFFolderContextMenuItem_Create(hwndOwner, pdfFolder);
    return (*ppcm) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CNetFilter_GetDefaultSearchGUID(IDocFindFileFilter *pdfff, IShellFolder2 *psf2, LPGUID lpGuid)
{
    *lpGuid = SRCID_SFindComputer;
    return S_OK;
}

STDMETHODIMP CNetFilter_EnumSearches(IDocFindFileFilter *pdfff, IShellFolder2 *psf2, LPENUMEXTRASEARCH *ppenum)
{
    return CNetwork_EnumSearches(psf2, ppenum);
}

STDMETHODIMP CNetFilter_GetSearchFolderClassId(IDocFindFileFilter *pdfff, LPGUID lpGuid)
{
    *lpGuid = CLSID_ComputerFindFolder;
    return S_OK;
}

STDMETHODIMP CNetFilter_GetNextConstraint(IDocFindFileFilter *pdfff, VARIANT_BOOL fReset, BSTR *pName, VARIANT *pValue, VARIANT_BOOL *pfFound)
{
    *pName = NULL;
    *pfFound = FALSE;
    VariantClear(pValue);                            
    return E_NOTIMPL;
}

STDMETHODIMP CNetFilter_GetQueryLanguageDialect(IDocFindFileFilter *pdfff, ULONG * pulDialect)
{
    if( pulDialect )
        *pulDialect = 0 ;
    return E_NOTIMPL ;
}


//==========================================================================
// IDocFindFileFilter::FFilterChanged - Returns S_OK if nothing changed.
//==========================================================================
STDMETHODIMP CNetFilter_FFilterChanged(IDocFindFileFilter *pdfff)
{
    // Currently not saving so who cares?
    return S_FALSE;
}


//==========================================================================
// IDocFindFileFilter::GenerateTitle - Generates the title given the current
// search criteria.
//==========================================================================
STDMETHODIMP CNETFilter_GenerateTitle(IDocFindFileFilter *pnetf,
        LPTSTR *ppszTitle, BOOL fFileName)
{
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);

    // Now lets construct the message from the resource
    *ppszTitle = ShellConstructMessageString(HINST_THISDLL,
            MAKEINTRESOURCE(IDS_FIND_TITLE_COMPUTER), fFileName ? TEXT(" #") : TEXT(":"));

    return *ppszTitle ? S_OK : E_OUTOFMEMORY;
}

//==========================================================================
// IDocFindFileFilter::ClearSearchCriteria
//==========================================================================
STDMETHODIMP CNETFilter_ClearSearchCriteria(IDocFindFileFilter *pnetf)
{
    int cPages;
    HWND    hwndMainDlg;
    TC_DFITEMEXTRA tie;
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);

    hwndMainDlg = GetParent(this->hwndTabs);
    for (cPages = TabCtrl_GetItemCount(this->hwndTabs) -1; cPages >= 0; cPages--)
    {
        tie.tci.mask = TCIF_PARAM;
        TabCtrl_GetItem(this->hwndTabs, cPages, &tie.tci);
        SendNotify(tie.hwndPage, hwndMainDlg, PSN_RESET, NULL);
    }

    return NOERROR;
}

//==========================================================================
// IDocFindFileFilter::PrepareToEnumObjects
//==========================================================================
STDMETHODIMP CNETFilter_PrepareToEnumObjects(IDocFindFileFilter *pnetf, DWORD *pdwFlags)
{
    int cPages;
    HWND    hwndMainDlg;
    TC_DFITEMEXTRA tie;
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);

    if (this->hwndTabs)
    {
        hwndMainDlg = GetParent(this->hwndTabs);
        for (cPages = TabCtrl_GetItemCount(this->hwndTabs) -1; cPages >= 0; cPages--)
        {
            tie.tci.mask = TCIF_PARAM;
            TabCtrl_GetItem(this->hwndTabs, cPages, &tie.tci);
            SendNotify(tie.hwndPage, hwndMainDlg, PSN_APPLY, NULL);
        }
    }

    // Update the flags and buffer strings

    *pdwFlags &= ~FFLT_INCLUDESUBDIRS;
    // Also lets convert the Computer name  pattern into the strings
    // will do the compares against.
    if ((this->szUserInputCompName[0] == TEXT('\\')) &&
            (this->szUserInputCompName[1] == TEXT('\\')))
    {
        Str_SetPtr(&(this->pszCompName), this->szUserInputCompName);
    }
    else
    {
        DocFind_SetupWildCardingOnFileSpec(this->szUserInputCompName,
               &this->pszCompName);
    }

    return NOERROR;
}

//==========================================================================
// IDocFindFileFilter::GetDetails
//==========================================================================
STDMETHODIMP CNETFilter_GetDetailsOf(IDocFindFileFilter *pnetf, HDPA hdpaPidf,
        LPCITEMIDLIST pidl, UINT *piColumn, SHELLDETAILS *pdi)
{
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);
    HRESULT hr = S_OK;

    if (*piColumn >= ARRAYSIZE(c_findcomp_cols))
        return E_NOTIMPL;

    pdi->str.uType = STRRET_CSTR;
    pdi->str.cStr[0] = 0;

    if (!pidl)
    {
        pdi->fmt = c_findcomp_cols[*piColumn].iFmt;
        pdi->cxChar = c_findcomp_cols[*piColumn].cchCol;
        hr = ResToStrRet(c_findcomp_cols[*piColumn].ids, &pdi->str);
    }
    else 
    {
        // We need to now get to the idlist of the items folder.
        DFFolderListItem *pdffli = DPA_GetPtr(hdpaPidf, DF_IFOLDER(pidl));

        hr = E_FAIL;

        if (pdffli)
        {
            if (*piColumn == INFCOL_PATH)
            {
                TCHAR szName[MAX_PATH];
                if (SUCCEEDED(SHGetNameAndFlags(&pdffli->idl, SHGDN_NORMAL, szName, SIZECHARS(szName), NULL)))
                    hr = StringToStrRet(szName, &pdi->str);
            }
            else if (*piColumn < ARRAYSIZE(c_findcomp_cols))
            {
                IShellFolder2 *psf;
                if (SUCCEEDED(SHBindToObject(NULL, &IID_IShellFolder2, &pdffli->idl, (void **)&psf)))
                {
                    hr = MapSCIDToDetailsOf(psf, pidl, c_findcomp_cols[*piColumn].pscid, pdi);
                    psf->lpVtbl->Release(psf);
                }
            }
        }
    }
    return hr;
}

//==========================================================================
// IDocFindFileFilter::FDoesItemMatchFilter
//==========================================================================
STDMETHODIMP CNETFilter_FDoesItemMatchFilter(IDocFindFileFilter *pnetf,
        LPTSTR pszFolder, WIN32_FIND_DATA * pfinddata,
        IShellFolder *psf, LPITEMIDLIST pidl)
{
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);

    // Make sure that we only return computers...
    BYTE bType = SIL_GetType(pidl);

    // First pass dont push anything that is below a computer...
    if ((bType & (SHID_NET | SHID_INGROUPMASK)) != SHID_NET_SERVER)
        return NOERROR;     // does not match

    // Here is where I start getting in bed with the network enumerator
    // format of IDLists.
    if (this->pszCompName && this->pszCompName[0])
    {
        // Although for now not much...
        STRRET str;
        TCHAR szPath[MAX_PATH];

        if (FAILED(psf->lpVtbl->GetDisplayNameOf(psf, pidl, SHGDN_NORMAL, &str)) ||
            FAILED(StrRetToBuf(&str, pidl, szPath, ARRAYSIZE(szPath))) ||
            !PathMatchSpec(szPath, this->pszCompName))
        {
            return NOERROR;     // does not match
        }
    }

    return S_FALSE;    // return TRUE to imply yes!
}

//==========================================================================
// IDocFindFileFilter::SaveCriteria
//==========================================================================
STDMETHODIMP CNETFilter_SaveCriteria(IDocFindFileFilter *pnetf, IStream *pstm, WORD fCharType)
{
    //
#ifdef NOT_DONE_YET
#endif

    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);
    int cCriteria = 0;

    return (MAKE_SCODE(0, 0, cCriteria));
}

//==========================================================================
// IDocFindFileFilter::RestoreCriteria
//==========================================================================
STDMETHODIMP CNETFilter_RestoreCriteria(IDocFindFileFilter *pnetf,
        IStream *pstm, int cCriteria, WORD fCharType)
{
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);

#ifdef NOT_DONE_YET
#endif
    return NOERROR;
}


//==========================================================================
// IDocFindFileFilter::GetColSaveStream
//==========================================================================
STDMETHODIMP CNETFilter_GetColSaveStream(IDocFindFileFilter *pnetf, WPARAM wparam, LPSTREAM *ppstm)
{
    *ppstm = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CNETFilter_GenerateQueryRestrictions(IDocFindFileFilter *pnetf, LPWSTR *ppszQuery, DWORD *pdwGQR_Flags)
{
    if (ppszQuery)
        *ppszQuery = NULL;
    *pdwGQR_Flags = 0;
    return E_NOTIMPL;
}

STDMETHODIMP CNETFilter_ReleaseQuery(IDocFindFileFilter *pnetf)
{
    return S_OK;
}

HRESULT CNetFilter_UpdateField(IDocFindFileFilter *pnetf, BSTR szField, VARIANT vValue)
{
    // BUGBUG:: Coppied from Docfind2, can combine and simplify
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);
    HRESULT         hr;
    VARIANT         vConvertedValue;
    int             iField;
    LPTSTR          pszValue = NULL;

    typedef enum 
    {
        CNFFUFE_SearchFor = 0,
        CNFFUFE_LookIn,
    } CNFFUFE;

    static const CDFFUF s_cdffuf[] = // Warning: index of fields is used below in case...
    {
        {L"SearchFor", VT_BSTR, CNFFUFE_SearchFor},        
        {L"LookIn", VT_BSTR, CNFFUFE_LookIn},
        {NULL, VT_EMPTY, 0}
    };


    hr = E_FAIL;
    if ((iField = CDFFilter_UpdateFieldChangeType(szField, vValue, s_cdffuf, &vConvertedValue, &pszValue)) < 0)
        return E_FAIL;

    switch (iField)
    {
    case CNFFUFE_SearchFor:
        StrNCpy(this->szUserInputCompName, pszValue, ARRAYSIZE(this->szUserInputCompName));
        break;

    case CNFFUFE_LookIn:
        break;
    }

    VariantClear(&vConvertedValue);                            
    if (pszValue)
        LocalFree(pszValue);

    return S_OK;
}

HRESULT CNetFilter_ResetFieldsToDefaults(IDocFindFileFilter *pnetf)
{
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);
    this->szUserInputCompName[0] = TEXT('\0');
    return S_OK;
}

STDMETHODIMP CNETFilter_DeclareFSNotifyInterest(IDocFindFileFilter *pnetf, HWND hwndDlg, UINT uMsg)
{
    CNETFilter *this = IToClass(CNETFilter, dfff, pnetf);
    SHChangeNotifyEntry fsne;

    fsne.fRecursive = TRUE;
    fsne.pidl = this ->pidlStart;

    if (fsne.pidl) 
    {
        SHChangeNotifyRegister(hwndDlg, SHCNRF_NewDelivery | SHCNRF_ShellLevel, SHCNE_DISKEVENTS, uMsg, 1, &fsne);
    }

    return NOERROR;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Now starting the code for the name and location page
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////




//==========================================================================
//
// Process the WM_SIZE of the details page
//
void DocFind_CCOMPFNameLocOnSize(HWND hwndDlg, UINT state, int cx, int cy)
{
    RECT rc;
    int cxMargin;
    if (state == SIZE_MINIMIZED)
        return;         // don't bother when we are minimized...

    // Get the location of first static to calculate margin
    GetWindowRect(GetDlgItem(hwndDlg, IDD_STATIC), &rc);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (POINT *)&rc, 2);
    cxMargin = rc.left;
    cx -= cxMargin;

    DocFind_SizeControl(hwndDlg, IDD_FILESPEC, cx, TRUE);

}


//==========================================================================
// Helper to helper to add an item to the combobox.
//==========================================================================

HRESULT _GetDisplayName(IShellFolder *psfGP, LPCITEMIDLIST pidl, LPTSTR pszRet, UINT cchMax)
{
    LPITEMIDLIST pidlParent = ILClone(pidl);
    HRESULT hres;
    VDATEINPUTBUF(pszRet, TCHAR, cchMax);

    if (pidlParent)
    {
        IShellFolder *psfParent;
        ILRemoveLastID(pidlParent);

        hres = SHBindToObject(psfGP, &IID_IShellFolder, pidlParent, &psfParent);

        if (SUCCEEDED(hres))
        {
            STRRET str;
            pidl = ILFindLastID(pidl);
            hres = psfParent->lpVtbl->GetDisplayNameOf(psfParent, pidl, SHGDN_NORMAL, &str);
            if (SUCCEEDED(hres))
                hres = StrRetToBuf(&str, pidl, pszRet, cchMax);

            psfParent->lpVtbl->Release(psfParent);
        }

        ILFree(pidlParent);
    }
    else
    {
        hres = E_OUTOFMEMORY;
    }
    return hres;
}

int DocFind_LocCBAddItem(HWND hwndCtl, LPCITEMIDLIST pidlAbs,
                       int iIndex, int iIndent, int iImage, LPTSTR lpszText)
{
    COMBOBOXEXITEM cei;

    cei.mask           = CBEIF_TEXT|CBEIF_IMAGE|CBEIF_SELECTEDIMAGE
                          |CBEIF_LPARAM|CBEIF_INDENT;
    cei.pszText        = lpszText;
    cei.lParam         = (LPARAM)pidlAbs;
    cei.iImage         = iImage;
    cei.iSelectedImage = iImage;
    cei.iItem          = iIndex;
    cei.iIndent        = iIndent;

    return (int) SendMessage(hwndCtl, CBEM_INSERTITEM, 0, (LPARAM)&cei);
}

int DocFind_LocCBAddPidl(HWND hwndCtl, IShellFolder *psf,
       LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlAbs,
       BOOL fFullName, UINT iIndent)
{
    TCHAR    szPath[MAX_PATH];
    int     iItem = -1;
    int     iImage;
    LPITEMIDLIST pidlAbs;

    pidlAbs = ILCombine(pidlParent, pidl);
    if (!pidlAbs)
        return(-1);

    if (fFullName)
    {
        if (!SHGetPathFromIDList(pidlAbs, szPath))
        {
            ILFree(pidlAbs);
            return(-1);
        }
    }
    else if (FAILED(_GetDisplayName(psf, pidl, szPath, ARRAYSIZE(szPath))))
    {
        ILFree(pidlAbs);
        return(-1);
    }

    iImage = SHMapPIDLToSystemImageListIndex(psf, pidl, NULL);

    iItem = DocFind_LocCBAddItem(hwndCtl, pidlAbs,
                                 -1,    // At the end...
                                 iIndent, iImage, szPath);

    if (ppidlAbs)
        *ppidlAbs = pidlAbs;
    return(iItem);
}

//==========================================================================
// Helper function to see if an Pidl is aready in the list...
//==========================================================================
int DocFind_LocCBFindPidl(HWND hwnd, LPITEMIDLIST pidl)
{
    int i;
    LPITEMIDLIST pidlT;

    for (i = (int) SendMessage(hwnd, CB_GETCOUNT, 0, 0); i >= 0; i--)
    {
        pidlT = (LPITEMIDLIST)SendMessage(hwnd, CB_GETITEMDATA, i, 0);

        if ((pidlT != NULL) && (pidlT != (LPITEMIDLIST)CB_ERR) &&
                ILIsEqual(pidl, pidlT))
            break;
    }
    return(i);
}


//==========================================================================
// Initialize the Name and loacation page
//==========================================================================
void DocFind_CCOMPFNameLocInit(LPDOCFINDPROPSHEETPAGE pdfpsp)
{
    CNETFilter *pdff = pdfpsp->pdff;
    TCHAR szPath[MAX_PATH];
    LPITEMIDLIST pidlWindows;

    // We want to set the default search drive to the windows drive.
    // I am going to be a bit slimmy, but...
    //
    GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
    if (szPath[1] == TEXT(':'))
        szPath[3] = TEXT('\0');

    pidlWindows = ILCreateFromPath(szPath);

    if ((pdfpsp->dwState & DFPAGE_INIT) == 0)
    {
        pdff->hMRUSpecs = DocFind_UpdateMRUItem(NULL, pdfpsp->hwndDlg, IDD_FILESPEC,
                s_szCompSpecMRU, pdff->szUserInputCompName, szNULL);

        // Update our state to let us know that we have already initialized...
        pdfpsp->dwState |= DFPAGE_INIT;
    }


    ILFree(pidlWindows);

}


//==========================================================================
// Validate the page to make sure that the data is valid.  If it is not
// we need to display a message to the user and also set the focus to
// the invalid field.
//==========================================================================
void DocFind_CCOMPFNameLocValidatePage(LPDOCFINDPROPSHEETPAGE pdfpsp)
{

    // No validation is needed here (At least not now).

}


//==========================================================================
//
// Apply any changes that happened in the name loc page to the filter
//
void DocFind_CCOMPFNameLocApply(LPDOCFINDPROPSHEETPAGE pdfpsp)
{
    CNETFilter *pdff = pdfpsp->pdff;

    GetDlgItemText(pdfpsp->hwndDlg, IDD_FILESPEC, pdff->szUserInputCompName,
            ARRAYSIZE(pdff->szUserInputCompName));

    DocFind_UpdateMRUItem(pdff->hMRUSpecs, pdfpsp->hwndDlg, IDD_FILESPEC,
            s_szCompSpecMRU, pdff->szUserInputCompName, NULL);

}



//==========================================================================
//
// DocFind_OnCommand - Process the WM_COMMAND messages
//
void DocFind_CCOMPFNameLocOnCommand(HWND hwndDlg, UINT id, HWND hwndCtl, UINT code)
{
}

//==========================================================================
//
// This function is the dialog (or property sheet page) for the name and
// location page.
//

const static TCHAR szHelpFile[] = TEXT("network.hlp");
const static DWORD aGeneralHelpIds[] = {  // Context Help IDs
    IDD_STATIC,    IDH_FINDCOMP_NAME,
    IDD_FILESPEC,  IDH_FINDCOMP_NAME,

    0, 0
};

BOOL_PTR CALLBACK DocFind_CCOMPFNameLocDlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LPDOCFINDPROPSHEETPAGE pdfpsp = (LPDOCFINDPROPSHEETPAGE)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (msg) {
    HANDLE_MSG(hwndDlg, WM_COMMAND, DocFind_CCOMPFNameLocOnCommand);
    HANDLE_MSG(hwndDlg, WM_SIZE, DocFind_CCOMPFNameLocOnSize);

    case WM_INITDIALOG:
        SetWindowLongPtr(hwndDlg, DWLP_USER, lParam);
        pdfpsp = (LPDOCFINDPROPSHEETPAGE)lParam;
        pdfpsp->hwndDlg = hwndDlg;
        break;


    case WM_NCDESTROY:
        LocalFree(pdfpsp);
        SetWindowLongPtr(hwndDlg, DWLP_USER, 0);
        return FALSE;   // We MUST return FALSE to avoid mem-leak

    case DFM_ENABLECHANGES:
        EnableWindow(GetDlgItem(hwndDlg, IDD_FILESPEC), (BOOL)wParam);
        break;

    case WM_HELP:
        WinHelp((HWND) ((LPHELPINFO) lParam)->hItemHandle, szHelpFile, HELP_WM_HELP,
            (ULONG_PTR) (LPTSTR) aGeneralHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, szHelpFile, HELP_CONTEXTMENU,
            (ULONG_PTR) (LPTSTR) aGeneralHelpIds);
        break;


    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case PSN_KILLACTIVE:
            DocFind_CCOMPFNameLocValidatePage(pdfpsp);
            break;
        case PSN_SETACTIVE:
            DocFind_CCOMPFNameLocInit(pdfpsp);

            break;

        case PSN_APPLY:
            if ((pdfpsp->dwState & DFPAGE_INIT) != 0)
                DocFind_CCOMPFNameLocApply(pdfpsp);

            break;
        case PSN_RESET:
            if ((pdfpsp->dwState & DFPAGE_INIT) != 0)
            {
                // Null the filespec
                SetDlgItemText(hwndDlg, IDD_FILESPEC, c_szNULL);

            }
            break;
        }
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

//===========================================================================
// CNETFEnum: class definition
//===========================================================================


typedef struct                          // Doc find Container
{
    IDFEnum         dfenum;
    UINT            cRef;
    IShellFolder    *psf;               // Pointer to shell folder

    // Stuff to use in the search
    DWORD            grfFlags;          // Flags that control things like recursion

    // filter info...
    LPTSTR           pszDisplayText;     // Place to write feadback text into
    CNETFilter *    pnetf;              // Pointer to the net filter...

    // enumeration state

    IShellFolder *  psfEnum;            // Pointer to shell folder for the object.
    LPENUMIDLIST    penum;              // Enumerator in use.
    LPITEMIDLIST    pidl;               // The idlist of the currently processing
    LPITEMIDLIST    pidlStart;          // Pointer to the starting point.
    int             iFolder;            // Which folder are we adding items for?
    BOOL            fAddedSubDirs;
    BOOL            fObjReturnedInDir;  // Has an object been returned in this dir?
    BOOL            fFindUNC;           // Find UNC.
    int             iPassCnt;           // Used to control when to reiterat...
} CNETFEnum;

//===========================================================================
// CNETFEnum : member prototype - Docfind Folder implementation
//===========================================================================
STDMETHODIMP CNETFEnum_QueryInterface(IDFEnum * pdfenum, REFIID riid, LPVOID FAR* ppvObj);
STDMETHODIMP_(ULONG) CNETFEnum_AddRef(IDFEnum * pdfenum);
STDMETHODIMP_(ULONG) CNETFEnum_Release(IDFEnum * pdfenum);

STDMETHODIMP CNETFEnum_Next(IDFEnum * pdfenum, LPITEMIDLIST *ppidl,
               int *pcObjectSearched, int *pcFoldersSearched, volatile BOOL *pfContinue, int *pState, HWND hwnd);


STDMETHODIMP CDefDFEnum_Skip(IDFEnum * pdfenum, int celt);
STDMETHODIMP CDefDFEnum_Reset(IDFEnum * pdfenum);
STDMETHODIMP CDefDFEnum_StopSearch(IDFEnum * pdfenum);
STDMETHODIMP_(BOOL) CDefDFEnum_FQueryIsAsync(IDFEnum * pdfenum);
STDMETHODIMP CDefDFEnum_GetAsyncCount(IDFEnum * pdfenum, DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone);
STDMETHODIMP CDefDFEnum_GetItemIDList(IDFEnum * pdfenum, UINT iItem, LPITEMIDLIST *ppidl);
STDMETHODIMP CDefDFEnum_GetExtendedDetailsOf(IDFEnum * pdfenum, LPCITEMIDLIST pidl, UINT iCol, LPSHELLDETAILS pdi);
STDMETHODIMP CDefDFEnum_GetExtendedDetailsULong(IDFEnum *pdfenum, LPCITEMIDLIST pidl, UINT iCol, ULONG *pul);
STDMETHODIMP CDefDFEnum_GetItemID(IDFEnum * pdfenum, UINT iItem, DWORD *puWorkID);
STDMETHODIMP CDefDFEnum_SortOnColumn(IDFEnum * pdfenum, UINT iCOl, BOOL fAscending);



//===========================================================================
// CNETFEnum : Vtable
//===========================================================================
IDFEnumVtbl c_CCOMPFFIterVtbl =
{
    CNETFEnum_QueryInterface, CNETFEnum_AddRef, CNETFEnum_Release,
    CNETFEnum_Next,
    CDefDFEnum_Skip,
    CDefDFEnum_Reset,
    CDefDFEnum_StopSearch,
    CDefDFEnum_FQueryIsAsync,
    CDefDFEnum_GetAsyncCount,
    CDefDFEnum_GetItemIDList,
    CDefDFEnum_GetExtendedDetailsOf,
    CDefDFEnum_GetExtendedDetailsULong,
    CDefDFEnum_GetItemID,
    CDefDFEnum_SortOnColumn
};


//==========================================================================
// CNETFEnum::QueryInterface
//==========================================================================

STDMETHODIMP CNETFEnum_QueryInterface(IDFEnum * pdfenum, REFIID riid, LPVOID FAR* ppvObj)
{
    return E_NOTIMPL;
}

//==========================================================================
// IDFEnum::AddRef
//==========================================================================
STDMETHODIMP_(ULONG) CNETFEnum_AddRef(IDFEnum * pdfenum)
{
    CNETFEnum *this = IToClass(CNETFEnum, dfenum, pdfenum);
    this->cRef++;
    return this->cRef;
}
//==========================================================================
// CNETFilter_EnumObjects - Get The real recursive filtered enumerator...
//==========================================================================

STDMETHODIMP CNETFilter_EnumObjects(IDocFindFileFilter * pdfff,
                                    IShellFolder *psf, LPITEMIDLIST pidlStart, 
                                    DWORD grfFlags, int iColSort,
                                    TCHAR szDisplayText[MAX_PATH], 
                                    IRowsetWatchNotify *prsn,
                                    IDFEnum **ppdfenum)
{
    CNETFilter *pnetf = (CNETFilter *)pdfff;
    CNETFEnum *pdfenum;

#ifdef USE_DS_TO_FIND
    // try searching the DS if we can
    HRESULT hres = CNetFindInDS_CreateInstance(pnetf->szUserInputCompName, psf, ppdfenum);
    if ( FAILED(hres) || (hres == S_OK) ) 
        return hres;
#endif

    // We need to construct the iterator
    pdfenum = LocalAlloc(LPTR, SIZEOF(CNETFEnum));        

    *ppdfenum = NULL;
    if (pdfenum == NULL)
        return (E_OUTOFMEMORY);

    // Now initialize the data structures.
    pdfenum->dfenum.lpVtbl = &c_CCOMPFFIterVtbl;
    pdfenum->cRef = 1;
    pdfenum->psf = psf;
    pdfenum->pszDisplayText = szDisplayText;
    pdfenum->grfFlags = grfFlags;
    pdfenum->pnetf = pnetf;

    if (pnetf->pidlStart)
        pdfenum->pidlStart = ILClone(pnetf->pidlStart);
    else
        GetDomainWorkgroupIDList(&pdfenum->pidlStart);

    pdfenum->iPassCnt = 0;

    // See if this is a UNC Search
    if (pdfenum->pidlStart)
    {
        if (pnetf->pszCompName && (pnetf->pszCompName[0] == TEXT('\\')))
            pdfenum->fFindUNC = TRUE;
    
        // Save away the filter pointer
        pnetf->dfff.lpVtbl->AddRef(pdfff);
    
        // The rest of the fields should be zero/NULL
    
#ifdef NET_TIMINGS
        // Reset Counters
        NTF_cNoPEnum = NTF_dtNoPEnum = 0;
        NTF_cNextItem = NTF_dtNextItem = 0;
#endif
    
        *ppdfenum = &pdfenum->dfenum;       // Return the appropriate value;
        return NOERROR;
    }
    else
    {
        LocalFree((HLOCAL)pdfenum);
        return (E_OUTOFMEMORY);
    }
}

//==========================================================================
// CNETFEnum::Release
//==========================================================================
STDMETHODIMP_(ULONG) CNETFEnum_Release(IDFEnum * pdfenum)
{
    CNETFEnum *this = IToClass(CNETFEnum, dfenum, pdfenum);

    this->cRef--;
    if (this->cRef > 0)
    {
        return this->cRef;
    }

    // Release any open enumerator and open IShell folder we may have.
    if (this->psfEnum != NULL)
        this->psfEnum->lpVtbl->Release(this->psfEnum);
    if (this->penum != NULL)
        this->penum->lpVtbl->Release(this->penum);


    // Release our use of the filter
    if (this->pnetf)
        this->pnetf->dfff.lpVtbl->Release(&this->pnetf->dfff);

    if (this->pidlStart)
        ILFree(this->pidlStart);
    if (this->pidl)
        ILFree(this->pidl);

    LocalFree((HLOCAL)this);

#ifdef NET_TIMINGS
    // Output some timings.
    if (!this->fFindUNC)
    {
        DebugMsg(DM_TRACE, TEXT("CNETFEnum:: Start Enums(%d), Time(%d), Per item(%d)"),
                NTF_cNoPEnum, NTF_dtNoPEnum, NTF_cNoPEnum ? NTF_dtNoPEnum/NTF_cNoPEnum : 0);

        DebugMsg(DM_TRACE, TEXT("CNETFEnum:: Count Next(%d), Time(%d), Per item(%d)"),
                NTF_cNextItem, NTF_dtNextItem, NTF_cNextItem ? NTF_dtNextItem/NTF_cNextItem : 0);
    }
#endif
    return(0);
}

//===========================================================================
// _UNCExtractServer
// Helper function to chop off all of an UNC path except the 
// \\server portion
//===========================================================================
void _UNCExtractServer(LPTSTR pszUNC)
{
    for (pszUNC += 2; *pszUNC; pszUNC = CharNext(pszUNC))
    {
        if (*pszUNC == TEXT('\\'))
        {
            // found something after server name, so get rid of it
            *pszUNC = TEXT('\0');
            break;
        }
    }

}

//===========================================================================
// _FindCompByUNCName -
//    Helper function to the next function to help process find computer
//    on returning computers by UNC names...
//
//
//===========================================================================
STDMETHODIMP _FindCompByUNCName(CNETFEnum * this, LPITEMIDLIST *ppidl,
               int *piState)
{
    LPTSTR pszT;
    LPITEMIDLIST pidl;

    //
    // Two cases, There is a UNC name entered.  If so we need to process
    // this by extracting everythign off after the server name...
    //
    pszT = this->pnetf->pszCompName;

    if ((pszT==NULL) || (*pszT == TEXT('\0')))
    {
        *piState = GNF_DONE;
        return NOERROR;
    }

    if (PathIsUNC(pszT))
    {
        _UNCExtractServer(pszT);
    }
    else
    {
        // They did not enter a unc name, but lets try to convert to
        // unc name
        LPTSTR pszTmp = LocalReAlloc(pszT,(3 + lstrlen(pszT))*SIZEOF(TCHAR),
                                LMEM_MOVEABLE );
        if (pszTmp)
        {
            pszT = pszTmp;
            this->pnetf->pszCompName = pszT;
            *pszT++ = TEXT('\\');
            *pszT++ = TEXT('\\');
            lstrcpy(pszT, this->pnetf->szUserInputCompName);
            _UNCExtractServer(pszT);
        }
        else
            return E_OUTOFMEMORY;
    }

    // Now parse the displayname - Argh we convert to Unicode, such
    // that we can uncovert at the other side.
    //
    pidl = ILCreateFromPath(this->pnetf->pszCompName);
    if (pidl != NULL)
    {
        //  clone the child to transform into a DocFind pidl
        LPITEMIDLIST   pidlChild = ILClone(ILFindLastID(pidl));

        if (pidlChild)
        {
            IDocFindFolder *pdfFolder;

            ILRemoveLastID(pidl);       // Remove the last id (computer)
            
            if (SUCCEEDED(this->psf->lpVtbl->QueryInterface(this->psf, &IID_IDocFindFolder, (void **)&pdfFolder))) 
            {
                pdfFolder->lpVtbl->AddFolderToFolderList(pdfFolder, pidl, FALSE, &this->iFolder);  
                pdfFolder->lpVtbl->Release(pdfFolder);
            }
            *ppidl = DocFind_AppendIFolder(pidlChild, this->iFolder);
            *piState = GNF_MATCH;
        }
        ILFree(pidl);
    }
    else
        *piState = GNF_DONE;

    // And Return;
    return NOERROR;
}


//===========================================================================
// CNETFEnum::Next Recursive Iterator that is very special to the docfind.
//             It will walk each directory, breath first, it will call the
//             defined callback function to determine if it is an
//             interesting file to us.  It will also return additional
//             information, such as:  The number of folders and files
//             searched, so we can give results back to the user.  It
//             will return control to the caller whenever:
//                  a) Finds a match.
//                  b) runs out of things to search.
//                  c) Starts searching in another directory
//                  d) when the callback says to...
//
//
//===========================================================================
STDMETHODIMP CNETFEnum_Next(IDFEnum * pdfenum, LPITEMIDLIST *ppidl,
               int *pcObjectSearched, int *pcFoldersSearched,
               volatile BOOL *pfContinue, int *piState, HWND hwnd)
{
    // If we aren't enumerating a directory, then get the next directory
    // name from the dir list, and begin enumerating its contents...
    //
    CNETFEnum * this = IToClass(CNETFEnum, dfenum, pdfenum);
    BOOL fContinue = TRUE;
    STRRET strret;
    IShellFolder *psfDesktop;

    SHGetDesktopFolder(&psfDesktop);

    //
    // Special case to find UNC Names quickly.  It will ignore all other
    // things.
    if (this->fFindUNC)
    {
        // If not the first time through return that we are done!
        if (this->iPassCnt)
        {
            *piState = GNF_DONE;
            return NOERROR;
        }

        this->iPassCnt = 1;

        return _FindCompByUNCName(this, ppidl, piState);
    }

    do
    {
        if (this->penum)
        {
            LPITEMIDLIST pidl;
#ifdef NET_TIMINGS
            NTF_dtTime = GetTickCount();
#endif
            NextIDL(this->penum, &pidl);
#ifdef NET_TIMINGS
            NTF_cNextItem++;
            NTF_dtNextItem += GetTickCount() - NTF_dtTime;
#endif

            if (pidl)
            {
                // Now see if this is someone we might want to return.
                // Our Match function take esither find data or idlist...
                // for networks we work off of the idlist,
                fContinue = FALSE;  // We can exit the loop;
                (*pcObjectSearched)++;
                
                if (this->pnetf->dfff.lpVtbl->FDoesItemMatchFilter(&this->pnetf->dfff,
                        this->pszDisplayText, NULL, this->psfEnum, pidl) != 0)
                {
                    *piState = GNF_MATCH;

                    // Now see if we have to add this folder to our
                    // list.
                    if (!this->fObjReturnedInDir)
                    {
                        IDocFindFolder *pdfFolder;
                        this->fObjReturnedInDir = TRUE;
                        if (SUCCEEDED(this->psf->lpVtbl->QueryInterface(this->psf, &IID_IDocFindFolder, (void **)&pdfFolder))) 
                        {
                            pdfFolder->lpVtbl->AddFolderToFolderList(pdfFolder, this->pidl, FALSE, &this->iFolder);  
                            pdfFolder->lpVtbl->Release(pdfFolder);
                        }   
                    }

                    // Now lets muck up the IDList to put ur index number
                    // onto the end of the idlist.
                    pidl = DocFind_AppendIFolder(pidl, this->iFolder);

                    if (pidl)
                    {
                        *ppidl = pidl;
                        
                        if ((this->iPassCnt == 1) && this->pnetf->pszCompName && *this->pnetf->pszCompName)
                        {
                            // See if this is an exact match of the name
                            // we are looking for.  If it is we set pass=2
                            // as to not add the item twice.
                            STRRET str;
                            TCHAR szName[MAX_PATH];

                            if (SUCCEEDED(this->psf->lpVtbl->GetDisplayNameOf(this->psf, pidl, SHGDN_NORMAL, &str)) &&
                                SUCCEEDED(StrRetToBuf(&str, pidl, szName, ARRAYSIZE(szName))) &&
                                (0 == lstrcmpi(szName, this->pnetf->szUserInputCompName)))
                            {
                                this->iPassCnt = 2;
                            }
                        }
                        break;
                    }

                }
                else

                {
                    // Release the IDList that did not match
                    ILFree(pidl);
                    *piState = GNF_NOMATCH;
                }
            }
            else
            {
                // Close out the shell folder and the enumeration function.
                this->penum->lpVtbl->Release(this->penum);
                this->penum = NULL;

                this->psfEnum->lpVtbl->Release(this->psfEnum);
                this->psfEnum = NULL;
            }
        }
        if (!this->penum)
        {
            switch (this->iPassCnt)
            {
            case 1:
                // We went through all of the items see if there is
                // an exact match...
                this->iPassCnt = 2;

                return _FindCompByUNCName(this, ppidl, piState);

            case 2:
                // We looped through everything so return done!
                *piState = GNF_DONE;
                return NOERROR;

            case 0:
                // This is the main pass through here...
                // Need to clone the idlist
                this->pidl = ILClone(this->pidlStart);
                if (this->pidl == NULL)
                {
                   *piState = GNF_ERROR;
                   return (E_OUTOFMEMORY);
                }
                this->iPassCnt = 1;

                // We will do the first on in our own thread.
                if (SUCCEEDED(psfDesktop->lpVtbl->BindToObject(psfDesktop,
                        this->pidl, NULL, &IID_IShellFolder,
                        &this->psfEnum)))
                {

                    // BUGBUG:: Need more flags to control this!
                    if (FAILED(this->psfEnum->lpVtbl->EnumObjects(this->psfEnum,
                            (HWND)NULL, // BUGBUG: hwndOwner
                            SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &this->penum)))
                    {
                        // Failed to get iterator so release folder.
                        this->psfEnum->lpVtbl->Release(this->psfEnum);
                        this->psfEnum = NULL;
                        this->penum = NULL;
                    }
                }
                
                break;
            }

            // We are now read to get the IShellfolder for this guy!
            (*pcFoldersSearched)++;

            // Need to put something here to show what is being searched!
            strret.uType = STRRET_OFFSET;
            if (SUCCEEDED(psfDesktop->lpVtbl->GetDisplayNameOf(psfDesktop,
                    this->pidl, SHGDN_NORMAL, &strret)))
            {
                 StrRetToBuf(&strret, this->pidl, this->pszDisplayText, MAX_PATH);
            }

#ifdef NET_TIMINGS
            NTF_cNoPEnum++;
#endif
        }
    } while (fContinue && *pfContinue);

    return NOERROR;
}

//===========================================================================
// CDefDFEnum::Skip Recursive Iterator that is very special to the docfind.
//===========================================================================
STDMETHODIMP CDefDFEnum_Skip(IDFEnum * pdfenum, int celt)
{
    return (E_NOTIMPL);
}

//===========================================================================
// CDefDFEnum::Reset
//===========================================================================
STDMETHODIMP CDefDFEnum_Reset(IDFEnum * pdfenum)
{
    return (E_NOTIMPL);
}

//===========================================================================
// CDefDFEnum::StopSearch
//===========================================================================
STDMETHODIMP CDefDFEnum_StopSearch(IDFEnum * pdfenum)
{
    return (E_NOTIMPL);
}


//===========================================================================
// CDefDFEnum::GetAsyncCount
//===========================================================================
STDMETHODIMP_(BOOL) CDefDFEnum_FQueryIsAsync(IDFEnum * pdfenum)
{
    return FALSE;
}

//===========================================================================
// CDefDFEnum::GetAsyncCount
//===========================================================================
STDMETHODIMP CDefDFEnum_GetAsyncCount(IDFEnum * pdfenum, DBCOUNTITEM *pdwTotalAsync, 
        int *pnPercentComplete, BOOL *pfQueryDone)
{
    return (E_NOTIMPL);
}

//===========================================================================
// CDefDFEnum::GetItemIDList
//===========================================================================
STDMETHODIMP CDefDFEnum_GetItemIDList(IDFEnum * pdfenum, UINT iItem, LPITEMIDLIST *ppidl)
{
    return (E_NOTIMPL);
}

//===========================================================================
// CDefDFEnum::GetExtendedDetailsOf
//===========================================================================
STDMETHODIMP CDefDFEnum_GetExtendedDetailsOf(IDFEnum * pdfenum, LPCITEMIDLIST pidl, UINT iCol, LPSHELLDETAILS pdi)
{
    return (E_NOTIMPL);
}

STDMETHODIMP CDefDFEnum_GetExtendedDetailsULong(IDFEnum *pdfenum, LPCITEMIDLIST pidl, UINT iCol, ULONG *pul)
{
    return (E_NOTIMPL);
}

//===========================================================================
// CDefDFEnum::GetItemID
//===========================================================================
STDMETHODIMP CDefDFEnum_GetItemID(IDFEnum * pdfenum, UINT iItem, DWORD *puWorkID)
{
    *puWorkID = (UINT)-1;
    return (E_NOTIMPL);
}

//===========================================================================
// CDefDFEnum::SortOnColumn
//===========================================================================
STDMETHODIMP CDefDFEnum_SortOnColumn(IDFEnum * pdfenum, UINT iCOl, BOOL fAscending)
{
    return E_NOTIMPL;
}
