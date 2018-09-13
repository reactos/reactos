//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: docfind2.c
//
// Description: This file should contains most of the document find code
// that is specific to the default search filters.
//
//
// History:
//  12-29-93 KurtE      Created.
//
//---------------------------------------------------------------------------

#include "shellprv.h"
#pragma  hdrstop
#include "fstreex.h"
#include "docfind.h"
#include "prop.h"
#ifndef MAXUSHORT
#define MAXUSHORT 0xffff
#endif

//===========================================================================
// Define the Default data filter data structures
//===========================================================================
//
// Define the internal structure of our default filter
typedef struct _CDFFilter   // fff
{
    IDocFindFileFilter  dfff;
    LONG                cRef;

    HWND                hwndTabs;

    HANDLE              hMRUSpecs;

    // Added support for Query results to be async...
    IDFEnum             *pdfenumAsync;
    // Here are the paths that we are to search on...

    // Data associated with the Look in field
    IShellFolder *      psfMyComputer;
    int                 iMyComputer;
    int                 iWindowsDrive;
    int                 iDocumentFolders;
    int                 iDesktopFolders;
    int                 ipidlStart;

    // Data associated with the file name.
    BOOL                fNameChanged;       // The name changed earlier
    LPTSTR              pszFileSpec;        // $$ the one we do compares with
    LPTSTR              pszSpecs;           // same as pszFileSpec but with '\0's for ';'s
    LPTSTR *            apszFileSpecs;      // pointers into pszSpecs for each token
    int                 cFileSpecs;         // count of specs

    LPITEMIDLIST        pidlStart;          // Starting location ID list.
    TCHAR               szPath[MAX_PATH];   // Location of where to start search from
    TCHAR               szUserInputFileSpec[MAX_PATH];  // File pattern.
    TCHAR               szText[MAXSTRLEN];  // Limit text to max editable size
    DWORD               uFixedDrives;       // The list of fixed drives...

#ifdef UNICODE
    CHAR                szTextA[MAXSTRLEN];
#endif

    BOOL                fTopLevelOnly;      // Search on top level only?
    BOOL                fShowAllObjects;    // $$ Should we show all files?
    BOOL                fFilterChanged;     // Something in the filter changed.
    BOOL                fWeRestoredSomeCriteria; // We need to initilize the pages...

    // Fields associated with the file type
    BOOL                fTypeChanged;       // Type changed;
    int                 iType;              // Index of the type.
    int                 iTypeLast;          // Save away last type...
    PHASHITEM           phiType;            // Save away hash item
    TCHAR               szTypeName[80];     // The display name for type
    TCHAR               szTypeFilePatterns[MAX_PATH]; // $$ The file patterns associated with type
    LPTSTR              pszIndexedSearch;   // what to search for... (Maybe larger than MAX_PATH because it's a list of paths.
    ULONG               ulQueryDialect;    // ISQLANG_V1 or ISQLANG_V2
    DWORD               dwWarningFlags;    // Warning bits (DFW_xxx).

    LPGREPINFO          lpgi;               // $$ Grep information.

    int                 iSizeType;          // $$ What type of size 0 - none, 1 > 2 <
    DWORD               dwSize;             // $$ Size comparison
    WORD                wDateType;          // $$ 0 - none, 1 days before, 2 months before...
    WORD                wDateValue;         //  (Num of months or days)
    WORD                dateModifiedBefore; // $$
    WORD                dateModifiedAfter;  // $$
    BITBOOL             fFoldersOnly:1;     // $$ Are we searching for folders?
    BITBOOL             fTextCaseSen:1;     // $$ Case sensitive searching...
    BITBOOL             fTextReg:1;         // $$ regular expressions.
    BITBOOL             fSearchSlowFiles:1;  // && probably missleading as file over a 300baud modem is also slow
    int                 iNextConstraint;    // which constraint to look at next...

} CDFFilter;

// Lets define some constants to use to define which types of date we are searching on
#define DFF_DATE_ALL        (IDD_MDATE_ALL-IDD_MDATE_ALL)
#define DFF_DATE_DAYS       (IDD_MDATE_DAYS-IDD_MDATE_ALL)
#define DFF_DATE_MONTHS     (IDD_MDATE_MONTHS-IDD_MDATE_ALL)
#define DFF_DATE_BETWEEN    (IDD_MDATE_BETWEEN-IDD_MDATE_ALL)

#define DFF_DATE_RANGEMASK  0x00ff
#define DFF_DATE_TYPEMASK   0xff00

#define DFF_DATE_MODIFIED   0x0000
#define DFF_DATE_CREATED    0x0100
#define DFF_DATE_ACCESSED   0x0200

// Define new criteria to be saved in file...
#define DFSC_SEARCHFOR          0x5000

// Define common page data for each of our pages
typedef struct { // dfpsp
    PROPSHEETPAGE   psp;
    HANDLE          hThreadInit;
    HWND            hwndDlg;
    CDFFilter *     pdff;
    DWORD           dwState;
} DOCFINDPROPSHEETPAGE, * LPDOCFINDPROPSHEETPAGE;

typedef struct {    // pdfsli
    DWORD   dwVer;  // Version
    DWORD   dwType; // Type of data (defined below)
} DOCFINDSAVELOOKIN;

// Used to enum top level paths
//
typedef struct {
    // Stuff to use in the search
    LPTSTR       pszPath;            // Passed in path from creater

    // Handle cases where we search one level deep like at \\compname and the like
    //
    LPCITEMIDLIST pidlStart;        // Passed in pidl to begin from
    IShellFolder *psfTopLevel;      // Top level shellfolder
    IEnumIDList *penumTopLevel;     // Top level enum function.
    BOOL        fFirstPass;         // Is this the first pass?
    LPTSTR      pszPathNext;        // filter path enumeration state
} CDFEStartPaths;



const COL_DATA c_df_cols[] = {
    {IDFCOL_NAME,       IDS_NAME_COL,     20,   LVCFMT_LEFT,    &SCID_NAME},
    {IDFCOL_PATH,       IDS_PATH_COL,     20,   (short int)(LVCFMT_LEFT | LVCFMT_COL_HAS_IMAGES), &SCID_DIRECTORY},
    {IDFCOL_RANK,       IDS_RANK_COL,     10,   LVCFMT_RIGHT,   &SCID_RANK},
    {IDFCOL_SIZE,       IDS_SIZE_COL,     10,   LVCFMT_RIGHT,   &SCID_SIZE},
    {IDFCOL_TYPE,       IDS_TYPE_COL,     20,   LVCFMT_LEFT,    &SCID_TYPE},
    {IDFCOL_MODIFIED,   IDS_MODIFIED_COL, 30,   LVCFMT_LEFT,    &SCID_WRITETIME}
};

// Functions used enumerate the top level find strings, handling this like MyComputer \\AServer
HRESULT DF_GetSearchPaths(CDFFilter *this, LPCITEMIDLIST pidlStart,  LPWSTR *papwszPaths[], UINT *pcPaths);
BOOL DF_EnumNextTopPath(CDFEStartPaths * pdfesp, LPTSTR pszOut, int cch);
BOOL IsSuperHidden(DWORD dwAttribs); // for now in docfind.cpp
HRESULT _IsNTFSDrive(int iDrive);  // in docfind.cpp

typedef struct {
    IDFEnum     dfenum;
    UINT        cRef;
    IShellFolder *psf;              // Pointer to shell folder

    // Stuff to use in the search
    LPITEMIDLIST    pidlStart;      // PidlStart...

    // List of start paths...
    LPWSTR      *apwszPaths;        // array of paths
    UINT        cPaths;             // how many in it...
    UINT        iPathNext;          // which one to process next...

    // We may have an Async Enum that does some of the paths...
    IDFEnum     *pdfenumAsync;      

    IShellFolder *psfStart;         // The starting folder for the search.
    int         cchStart;           // How many characters are in the

    DWORD       grfFlags;           // Flags that control things like recursion


    WIN32_FIND_DATA finddata;       // Win32 file data to use

    // filter info...
    LPTSTR       pszProgressText;    // Path Buffer pointer
    IDocFindFileFilter  * pdfff;// The file filter to use...
    TCHAR       szTempInternetCachePath[MAX_PATH];   // temporary internet path...

    // enumeration state
    int         ichPathFirst;       // ich into path string of current path...
    int         iFolder;            // Which folder are we adding items for?
    BOOL        fAddedSubDirs : 1;
    BOOL        fObjReturnedInDir : 1;  // Has an object been returned in this dir?
    BOOL        fFindFirstSucceed : 1;  // Did a find first file succeed.
    int         depth;              // directory level (relative to pszPath)
    HANDLE      hfind;
    DIRBUF * pdbStack;           // Linked list of DIRBUFs to enum
    DIRBUF * pdbReuse;
} CDFEnum;

#define DFSLI_VER                   0

#define DFSLI_TYPE_PIDL             0   // Pidl is streamed after this
#define DFSLI_TYPE_STRING           1   // cb follows this for length then string...

// Document folders and children - Warning we assume the order of items after Document Folders
#define DFSLI_TYPE_DOCUMENTFOLDERS  0x10
#define DFSLI_TYPE_DESKTOP          0x11
#define DFSLI_TYPE_PERSONAL         0x12
// My computer and children...
#define DFSLI_TYPE_MYCOMPUTER       0x20
#define DFSLI_TYPE_LOCALDRIVES      0x21


// BUGBUG I don't get it... why the manual calculation of the structure size?

#define DOCFINDPSHTSIZE (SIZEOF(PROPSHEETPAGE)+SIZEOF(HANDLE)+SIZEOF(HWND)+SIZEOF(CDFFilter *)+SIZEOF(DWORD))


#define DFPAGE_INIT     0x0001          /* This page has been initialized */
#define DFPAGE_CHANGE   0x0002          /*  The user has modified the page */

//------------------------------------------------------------------------------
// Use same enum and string table between updatefield and getting the constraints
// back out...
typedef enum 
{
    CDFFUFE_IndexedSearch = 0,
    CDFFUFE_LookIn,
    CDFFUFE_IncludeSubFolders,
    CDFFUFE_Named,
    CDFFUFE_ContainingText,
    CDFFUFE_FileType,
    CDFFUFE_WhichDate,
    CDFFUFE_DateLE,
    CDFFUFE_DateGE,
    CDFFUFE_DateNDays,
    CDFFUFE_DateNMonths,
    CDFFUFE_SizeLE,
    CDFFUFE_SizeGE,
    CDFFUFE_TextCaseSen,
    CDFFUFE_TextReg,
    CDFFUFE_SearchSlowFiles,
    CDFFUFE_QueryDialect,
    CDFFUFE_WarningFlags,
} CDFFUFE;

static const CDFFUF s_cdffuf[] = // Warning: index of fields is used below in case...
{
    {L"IndexedSearch", VT_BSTR, CDFFUFE_IndexedSearch},        
    {L"LookIn", VT_BSTR, CDFFUFE_LookIn},
    {L"IncludeSubFolders", VT_BOOL, CDFFUFE_IncludeSubFolders},
    {L"Named",VT_BSTR, CDFFUFE_Named},
    {L"ContainingText", VT_BSTR, CDFFUFE_ContainingText},
    {L"FileType", VT_BSTR, CDFFUFE_FileType},
    {L"WhichDate", VT_I4, CDFFUFE_WhichDate},
    {L"DateLE", VT_DATE, CDFFUFE_DateLE},
    {L"DateGE", VT_DATE, CDFFUFE_DateGE},
    {L"DateNDays", VT_I4, CDFFUFE_DateNDays},
    {L"DateNMonths", VT_I4, CDFFUFE_DateNMonths},
    {L"SizeLE", VT_UI4, CDFFUFE_SizeLE},
    {L"SizeGE", VT_UI4, CDFFUFE_SizeGE},
    {L"CaseSensitive", VT_BOOL, CDFFUFE_TextCaseSen},
    {L"RegularExpressions", VT_BOOL, CDFFUFE_TextReg},
    {L"SearchSlowFiles", VT_BOOL, CDFFUFE_SearchSlowFiles},
    {L"QueryDialect", VT_UI4, CDFFUFE_QueryDialect},
    {L"WarningFlags", VT_UI4 /*DFW_xxx bits*/, CDFFUFE_WarningFlags},
    {NULL, VT_EMPTY, 0}
};


//===========================================================================
// Copied from comctl32.  This is all a hack.  Docfind used this internal
// prsht function because it was too lazy to call CreateDialogIndirect.
// But comctl32's internal structure has changed, so we have to do it
// ourselves.  Nevermind that the actual function is tiny.
//===========================================================================

#include <pshpack2.h>

typedef struct                           
{                                        
    WORD    wDlgVer;                     
    WORD    wSignature;                  
    DWORD   dwHelpID;                    
    DWORD   dwExStyle;                   
    DWORD   dwStyle;                     
    WORD    cDlgItems;
    WORD    x;                           
    WORD    y;                           
    WORD    cx;                          
    WORD    cy;                          
}   DLGEXTEMPLATE, FAR *LPDLGEXTEMPLATE;

#include <poppack.h> /* Resume normal packing */

HWND DocFindCreatePageDialog(LPPROPSHEETPAGE hpage, HWND hwndParent, LPDLGTEMPLATE pDlgTemplate)
{
    DWORD lSaveStyle;
    LPDLGEXTEMPLATE pDlgExTemplate = (LPDLGEXTEMPLATE) pDlgTemplate;

    // Note:  Don't need to restore the style since we're going
    //        to free the memory anyway.

    if (pDlgExTemplate->wSignature == 0xFFFF)
    {
        lSaveStyle = pDlgExTemplate->dwStyle;
        pDlgExTemplate->dwStyle = (lSaveStyle & (DS_SETFONT | DS_LOCALEDIT | WS_CLIPCHILDREN))
                                | WS_CHILD | WS_TABSTOP | DS_3DLOOK | DS_CONTROL;
    }
    else
    {
        lSaveStyle = pDlgTemplate->style;
        pDlgTemplate->style = (lSaveStyle & (DS_SETFONT | DS_LOCALEDIT | WS_CLIPCHILDREN))
                                | WS_CHILD | WS_TABSTOP | DS_3DLOOK | DS_CONTROL;
    }

    return CreateDialogIndirectParam(
                    hpage->hInstance,
                    (LPCDLGTEMPLATE)pDlgTemplate,
                    hwndParent,
                    hpage->pfnDlgProc, (LPARAM)hpage);

}

HWND DocFindCreatePage(LPPROPSHEETPAGE ppsp, HWND hwndParent)
{
    HWND hwndPage = NULL; // NULL indicates an error
    HRSRC hRes;

    // Comctl32.CreatePage supported these flags but we don't use them
    // so we won't support them either.
    ASSERT(!(ppsp->dwFlags & (PSP_USECALLBACK | PSP_IS16 | PSP_DLGINDIRECT)));

    hRes = FindResource(ppsp->hInstance, ppsp->pszTemplate, RT_DIALOG);
    if (hRes)
    {
        const DLGTEMPLATE * pDlgTemplate = LoadResource(ppsp->hInstance, hRes);
        if (pDlgTemplate)
        {
            ULONG cbTemplate=SizeofResource(ppsp->hInstance, hRes);
            LPDLGTEMPLATE pdtCopy = (LPDLGTEMPLATE)Alloc(cbTemplate);

            ASSERT(cbTemplate>=sizeof(DLGTEMPLATE));

            if (pdtCopy)
            {
                hmemcpy(pdtCopy, pDlgTemplate, cbTemplate);
                hwndPage=DocFindCreatePageDialog(ppsp, hwndParent, pdtCopy);
                Free(pdtCopy);
            }

        }
    }

    return hwndPage;
}

//===========================================================================
// Define some other module global data
//===========================================================================

const DWORD aNameHelpIDs[] = {
        IDD_STATIC,         IDH_FINDFILENAME_NAME,
        IDD_FILESPEC,       IDH_FINDFILENAME_NAME,
        IDD_PATH,           IDH_FINDFILENAME_LOOKIN,
        IDD_BROWSE,         IDH_FINDFILENAME_BROWSE,
        IDD_TOPLEVELONLY,   IDH_FINDFILENAME_TOPLEVEL,
        IDD_CONTAINS,   IDH_FINDFILECRIT_CONTTEXT,

        0, 0
};

const DWORD aCriteriaHelpIDs[] = {
        IDD_STATIC,     IDH_FINDFILECRIT_OFTYPE,
        IDD_TYPECOMBO,  IDH_FINDFILECRIT_OFTYPE,
        IDD_SIZECOMP,   IDH_FINDFILECRIT_SIZEIS,
        IDD_SIZEVALUE,  IDH_FINDFILECRIT_K,
        IDD_SIZEUPDOWN, IDH_FINDFILECRIT_K,
        IDD_SIZELBL,    IDH_FINDFILECRIT_K,

        0, 0
};

const DWORD aDateHelpIDs[] = {
        IDD_MDATE_FROM,         IDH_FINDFILEDATE_FROM,
        IDD_MDATE_AND,          IDH_FINDFILEDATE_TO,
        IDD_MDATE_TO,           IDH_FINDFILEDATE_TO,
        IDD_MDATE_ALL,          IDH_FINDFILEDATE_ALLFILES,
        IDD_MDATE_PARTIAL,      IDH_FINDFILEDATE_CREATEORMOD,
        IDD_MDATE_DAYS,         IDH_FINDFILEDATE_DAYS,
        IDD_MDATE_DAYLBL,       IDH_FINDFILEDATE_DAYS,
        IDD_MDATE_MONTHS,       IDH_FINDFILEDATE_MONTHS,
        IDD_MDATE_MONTHLBL,     IDH_FINDFILEDATE_MONTHS,
        IDD_MDATE_BETWEEN,      IDH_FINDFILEDATE_RANGE,
        IDD_MDATE_NUMDAYS,      IDH_FINDFILEDATE_DAYS,
        IDD_MDATE_DAYSUPDOWN,   IDH_FINDFILEDATE_DAYS,
        IDD_MDATE_NUMMONTHS,    IDH_FINDFILEDATE_MONTHS,
        IDD_MDATE_MONTHSUPDOWN, IDH_FINDFILEDATE_MONTHS,
        IDD_MDATE_FROM,         IDH_FINDFILEDATE_FROM,
        IDD_MDATE_TO,           IDH_FINDFILEDATE_TO,

        0, 0
};

//==========================================================================
//
// Create the default filter for our find code...  They should be completly
// self contained...
//
extern IDocFindFileFilterVtbl c_DFFilterVtbl;   // forward

IDocFindFileFilter * CreateDefaultDocFindFilter()
{
    CDFFilter *pfff = (void*)LocalAlloc(LPTR, SIZEOF(CDFFilter));
    if (pfff == NULL)
        return(NULL);

    pfff->dfff.lpVtbl = &c_DFFilterVtbl;
    pfff->cRef = 1;
    pfff->wDateType = DFF_DATE_ALL | DFF_DATE_MODIFIED;
    pfff->ulQueryDialect = ISQLANG_V2;

    // We should now simply return the filter
    return &pfff->dfff;

}

STDMETHODIMP_(ULONG) CDFFilter_AddRef(IDocFindFileFilter *pdfff);

STDMETHODIMP CDFFilter_QueryInterface(IDocFindFileFilter *pdfff, REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, &IID_IUnknown) || 
        IsEqualIID(riid, &IID_IDocFindFileFilter))
    {
        *ppvObj = pdfff;
    } 
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    CDFFilter_AddRef(pdfff);
    return NOERROR;    
}

STDMETHODIMP_(ULONG) CDFFilter_AddRef(IDocFindFileFilter *pdfff)
{
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    TraceMsg(TF_DOCFIND, "CDFFilter.AddRef %d",this->cRef+1);
    return InterlockedIncrement(&this->cRef);
}

STDMETHODIMP_(ULONG) CDFFilter_Release(IDocFindFileFilter *pdfff)
{
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    TraceMsg(TF_DOCFIND, "CDFFilter.Release %d",this->cRef-1);
    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    // Destroy the MRU Lists...

    if (this->hMRUSpecs)
        FreeMRUList(this->hMRUSpecs);

    if (this->lpgi)
    {
        FreeGrepBufs(this->lpgi);
        this->lpgi = NULL;
    }

    // Release our usage of My computer
    if (this->psfMyComputer)
    {
        this->psfMyComputer->lpVtbl->Release(this->psfMyComputer);
    }

    if (this->pidlStart)
        ILFree(this->pidlStart);

    Str_SetPtr(&(this->pszFileSpec), NULL);
    Str_SetPtr(&(this->pszSpecs), NULL);
    LocalFree(this->apszFileSpecs); // elements point to pszSpecs so no free for them
    
    Str_SetPtr(&(this->pszIndexedSearch), NULL);

    LocalFree((HLOCAL)this);
    return(0);
}

//==========================================================================
// Function to let the find know which icons to display and the top level menu
//==========================================================================
STDMETHODIMP CDFFilter_GetIconsAndMenu (IDocFindFileFilter *pdfff,
        HWND hwndDlg, HICON *phiconSmall, HICON *phiconLarge, HMENU *phmenu)
{
    *phiconSmall = LoadImage(HINST_THISDLL, MAKEINTRESOURCE(IDI_DOCFIND),
            IMAGE_ICON, g_cxSmIcon, g_cySmIcon, LR_DEFAULTCOLOR);
    *phiconLarge  = LoadIcon(HINST_THISDLL, MAKEINTRESOURCE(IDI_DOCFIND));

    // BUGBUG:: Still menu to process!
    *phmenu = LoadMenu(HINST_THISDLL, MAKEINTRESOURCE(MENU_FINDDLG));
    return (S_OK);
}

//==========================================================================
// Function to get the string resource index number that is proper for the
// current type of search.
//==========================================================================
STDMETHODIMP CDFFilter_GetStatusMessageIndex (IDocFindFileFilter *pdfff,
        UINT uContext, UINT *puMsgIndex)
{
    // Currently context is not used
    *puMsgIndex = IDS_FILESFOUND;

    return (S_OK);
}


//==========================================================================
// Function to let find know which menu to load to merge for the folder
//==========================================================================
STDMETHODIMP CDFFilter_GetFolderMergeMenuIndex (IDocFindFileFilter *pdfff,
        UINT *puBGMainMergeMenu, UINT *puBGPopupMergeMenu)
{
    *puBGMainMergeMenu = POPUP_DOCFIND_MERGE;
    *puBGPopupMergeMenu = POPUP_DOCFIND_POPUPMERGE;
    return (S_OK);
}

STDMETHODIMP CDFFilter_GetItemContextMenu (IDocFindFileFilter *pdfff, HWND hwndOwner, IDocFindFolder* pdfFolder, IContextMenu **ppcm)
{
    *ppcm = CDFFolderContextMenuItem_Create(hwndOwner, pdfFolder);
    return (*ppcm) ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CDFFilter_GetDefaultSearchGUID(IDocFindFileFilter *pdfff, IShellFolder2 *psf2, LPGUID lpGuid)
{
    return FindFileOrFolders_GetDefaultSearchGUID(psf2, lpGuid);
}

STDMETHODIMP CDFFilter_EnumSearches(IDocFindFileFilter *pdfff, IShellFolder2 *psf2, LPENUMEXTRASEARCH *ppenum)
{
    return CFSFolder_EnumSearches(psf2, ppenum);
}

STDMETHODIMP CDFFilter_GetSearchFolderClassId(IDocFindFileFilter *pdfff, LPGUID lpGuid)
{
    *lpGuid = CLSID_DocFindFolder;
    return S_OK;
}


//==========================================================================
// Helper function for add page to the IDocFindFileFilter::AddPages
//==========================================================================
HRESULT DocFind_AddPages(IDocFindFileFilter *pdfff, HWND hwndTabs,
        const DFPAGELIST *pdfpl, int cdfpl)
{
    int i;
    TCHAR szTemp[128+50];
    RECT rc;
    int dxMax = 0;
    int dyMax = 0;
    TC_DFITEMEXTRA tie;
    LPDOCFINDPROPSHEETPAGE pdfpsp;
    HWND hwndDlg;

    tie.tci.mask = TCIF_TEXT | TCIF_PARAM;
    tie.hwndPage = NULL;
    tie.tci.pszText = szTemp;

    TabCtrl_SetItemExtra(hwndTabs, CB_DFITEMEXTRA);
    hwndDlg = GetParent(hwndTabs);

    // First go through and create all of the dialog pages.
    //
    for (i=0; i < cdfpl; i++)
    {
        pdfpsp = LocalAlloc(LPTR, SIZEOF(DOCFINDPROPSHEETPAGE));
        if (pdfpsp == NULL)
            break;

        pdfpsp->psp.dwSize = DOCFINDPSHTSIZE;
        pdfpsp->psp.dwFlags = PSP_DEFAULT;
        pdfpsp->psp.hInstance = HINST_THISDLL;
        pdfpsp->psp.lParam = 0;
        pdfpsp->psp.pszTemplate = MAKEINTRESOURCE(pdfpl[i].id);
        pdfpsp->psp.pfnDlgProc = pdfpl[i].pfn;
        pdfpsp->pdff = (struct _CDFFilter *)pdfff;
        pdfpsp->hThreadInit = NULL;
        tie.hwndPage = DocFindCreatePage(&pdfpsp->psp, hwndDlg);
        if (tie.hwndPage != NULL)
        {
            GetWindowText(tie.hwndPage, szTemp, ARRAYSIZE(szTemp));
            GetWindowRect(tie.hwndPage, &rc);
            if ((rc.bottom - rc.top) > dyMax)
                dyMax = rc.bottom - rc.top;
            if ((rc.right - rc.left) > dxMax)
                dxMax = rc.right - rc.left;
            TabCtrl_InsertItem(hwndTabs, 1000, &tie.tci);
        }
    }

    // We now need to resize everything to fit with the dialog templates
    rc.left = rc.top = 0;
    rc.right = dxMax;
    rc.bottom = dyMax;
    TabCtrl_AdjustRect(hwndTabs, TRUE, &rc);

    // Size the page now
    SetWindowPos(hwndTabs, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    // Now set the first page as active.  We should be able to do this by
    // simply posting a WM_COMMAND to the main dialog
    SendNotify(hwndDlg, hwndTabs, TCN_SELCHANGE, NULL);

    return (S_OK);
}

void WaitForPageInitToComplete(LPDOCFINDPROPSHEETPAGE pdfpsp)
{
    if (pdfpsp && pdfpsp->hThreadInit)
    {
        WaitForSendMessageThread(pdfpsp->hThreadInit, INFINITE);
        CloseHandle(pdfpsp->hThreadInit);
        pdfpsp->hThreadInit = NULL;
    }
}

//==========================================================================
// IDocFindFileFilter::FFilterChanged - Returns S_OK if nothing changed.
//==========================================================================
STDMETHODIMP CDFFilter_FFilterChanged(IDocFindFileFilter *pdfff)
{
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    BOOL fFilterChanged = this->fFilterChanged;
    this->fFilterChanged = FALSE;
    return fFilterChanged? S_FALSE : S_OK;
}

//==========================================================================
// IDocFindFileFilter::GenerateTitle - Generates the title given the current
// search criteria.
//==========================================================================
STDMETHODIMP CDFFilter_GenerateTitle(IDocFindFileFilter *pdfff,
        LPTSTR *ppszTitle, BOOL fFileName)
{
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    BOOL  fFilePattern;
    int iRes;
    TCHAR szFindName[80];    // German should not exceed this find: ->???

    LPTSTR pszFileSpec = this->szUserInputFileSpec;
    LPTSTR pszText = this->szText;

    //
    // Lets generate a title for the search.  The title will depend on
    // the file patern(s), the type field and the containing text field
    // Complicate this a bit with the search for field...
    //

    fFilePattern = (pszFileSpec[0] != TEXT('\0')) &&
                (lstrcmp(pszFileSpec, c_szStarDotStar) != 0);

    if (!fFilePattern && (this->pdfenumAsync == NULL) && this->pszIndexedSearch)
    {
        pszFileSpec = this->pszIndexedSearch;
        fFilePattern = (pszFileSpec[0] != TEXT('\0')) &&
                    (lstrcmp(pszFileSpec, c_szStarDotStar) != 0);
    }

    if ((pszText[0] == TEXT('\0')) && (this->pdfenumAsync != NULL) && this->pszIndexedSearch)
        pszText = this->pszIndexedSearch;

    // First see if there is a type field
    if (this->iType > 0)
    {
        // We have a type field no check for content...
        if (pszText[0] != TEXT('\0'))
        {
            // There is text!
            // Should now use type but...
            // else see if the name field is not NULL and not *.*
            if (fFilePattern)
                iRes = IDS_FIND_TITLE_TYPE_NAME_TEXT;
            else
                iRes = IDS_FIND_TITLE_TYPE_TEXT;
        }
        else
        {
            // No type or text, see if file pattern
            // Containing not found, first search for type then named
            if (fFilePattern)
                iRes = IDS_FIND_TITLE_TYPE_NAME;
            else
                iRes = IDS_FIND_TITLE_TYPE;
        }
    }
    else
    {
        // No Type field ...
        // first see if there is text to be searched for!
        if (pszText[0] != TEXT('\0'))
        {
            // There is text!
            // Should now use type but...
            // else see if the name field is not NULL and not *.*
            if (fFilePattern)
                iRes = IDS_FIND_TITLE_NAME_TEXT;
            else
                iRes = IDS_FIND_TITLE_TEXT;
        }
        else
        {
            // No type or text, see if file pattern
            // Containing not found, first search for type then named
            if (fFilePattern)
                iRes = IDS_FIND_TITLE_NAME;
            else
                iRes = IDS_FIND_TITLE_ALL;
        }
    }


    // We put : in for first spot for title bar.  For name creation
    // we remove it which will put the number at the end...
    if (!fFileName)
        LoadString(HINST_THISDLL, IDS_FIND_TITLE_FIND,
                szFindName, ARRAYSIZE(szFindName));
    *ppszTitle = ShellConstructMessageString(HINST_THISDLL,
            MAKEINTRESOURCE(iRes),
            fFileName? szNULL : szFindName,
            this->szTypeName, pszFileSpec, pszText);

    return *ppszTitle ? S_OK : E_OUTOFMEMORY;
}

//==========================================================================
// IDocFindFileFilter::ClearSearchCriteria
//==========================================================================
STDMETHODIMP CDFFilter_ClearSearchCriteria(IDocFindFileFilter *pdfff)
{
    int cPages;
    HWND    hwndMainDlg;
    TC_DFITEMEXTRA tie;
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);

    hwndMainDlg = GetParent(this->hwndTabs);
    for (cPages = TabCtrl_GetItemCount(this->hwndTabs) -1; cPages >= 0; cPages--)
    {
        tie.tci.mask = TCIF_PARAM;
        TabCtrl_GetItem(this->hwndTabs, cPages, &tie.tci);
        SendNotify(tie.hwndPage, hwndMainDlg, PSN_RESET, NULL);
    }

    // Also clear out a few other fields...
    this->szUserInputFileSpec[0] = TEXT('\0');
    this->iType = 0;
    this->szText[0] = TEXT('\0');

    return (S_OK);
}

//==========================================================================
// DocFind_SetupWildCardingOnFileSpec - returns TRUE if wildards are in
//      extension. Both "*" and "?" are treated as wildcards.
//==========================================================================
BOOL DocFind_SetupWildCardingOnFileSpec(LPTSTR pszSpecIn, LPTSTR *ppszSpecOut)
{
    LPTSTR pszIn = pszSpecIn;
    LPTSTR pszOut;
    LPTSTR pszStar;
    LPTSTR pszAnyC;
    BOOL fQuote;
    TCHAR szSpecOut[3*MAX_PATH];   // Rather large...

    // allocate a buffer that should be able to hold the resultant
    // string.  When all is said and done we'll re-allocate to the
    // correct size.


    pszOut = szSpecOut;
    while (*pszIn != TEXT('\0'))
    {
        LPTSTR pszT;
        int     ich;
        TCHAR  c;

        // Strip in leading spaces out of there
        while (*pszIn == TEXT(' '))
            pszIn++;
        if (*pszIn == TEXT('\0'))
            break;

        if (pszOut != szSpecOut)
            *pszOut++ = TEXT(';');
        if (FALSE != (fQuote = (*pszIn == TEXT('"'))))
        {
            // The user asked for something litteral.
           pszT = pszIn = CharNext(pszIn);
           while (*pszT && (*pszT != TEXT('"')))
               pszT = CharNext(pszT);
        }
        else
        {
            pszT = pszIn + (ich = StrCSpn(pszIn, TEXT(",; ")));
        }

        c = *pszT;       // Save away the seperator character that was found
        *pszT = TEXT('\0');    //

        // Put in a couple of tests for * and *.*
        if ((lstrcmp(pszIn, c_szStar) == 0) ||
                (lstrcmp(pszIn, c_szStarDotStar) == 0))
        {
            // Complete wild card so set a null criteria

            *pszT = c;  // Restore char;
            pszOut = szSpecOut;   // Set to start of string
            break;
        }
        if (fQuote)
        {
            lstrcpy(pszOut, pszIn);
            pszOut += lstrlen(pszIn);
        }
        else
        {
            // both "*" and "?" are wildcards.  When checking for wildcards check
            // for both before we conclude there are no wildcards.  If a search
            // string contains both "*" and "?" then we need for pszStar to point
            // to the last occorance of either one (this is assumed in the code
            // below which will add a ".*" when pszStar is the last character).
            // NOTE: I wish there was a StrRPBrk function to do this for me.
            pszStar = StrRChr(pszIn, NULL, TEXT('*'));
            pszAnyC = StrRChr(pszIn, NULL, TEXT('?'));
            if (pszAnyC > pszStar)
                pszStar = pszAnyC;
            if (pszStar == NULL)
            {
                // No wildcards were used:
                *pszOut++ = TEXT('*');
                lstrcpy(pszOut, pszIn);
                pszOut += ich;
                *pszOut++ = TEXT('*');
            }
            else
            {
                // Includes wild cards
                lstrcpy(pszOut, pszIn);
                pszOut += ich;

                // if no extension 
                pszAnyC = StrRChr(pszIn, NULL, TEXT('.'));
                if ( pszAnyC == NULL )
                {
                    // No extension is given
                    if ((*(pszStar+1) == TEXT('\0')) && (*pszStar == TEXT('*')))
                    {
                        // The last character is an "*" so this single string will
                        // match everything you would expect.
                    }
                    else
                    {
                        // in order to get the expected behavior we need to search
                        // for two things, the actual string entered and the string
                        // with any extension.  I.E. given "a*a" we need to search
                        // for "a*a" and "a*a.*".  Otherwise we won't find both the
                        // file "abba" and "abba.wav".  As a bonus we also pick up
                        // "abc.cba" this way when using "*".  This also helps for
                        // "a?", allowing it to find "a1.txt" as well as "aa"
                        *pszOut++ = TEXT(';');  // seperate the two strings
                        lstrcpy(pszOut, pszIn); // add the second variant
                        pszOut += ich;
                        *pszOut++ = TEXT('.');
                        *pszOut++ = TEXT('*');  // Add on .* to the name
                    }
                }
            }
        }

        *pszT = c;  // Restore char;
        if (c == TEXT('\0'))
            break;

        // Skip beyond quotes
        if (*pszT == TEXT('"'))
            pszT++;

        if (*pszT != TEXT('\0'))
            pszT++;
        pszIn = pszT;   // setup for the next item
    }

    *pszOut++ = TEXT('\0');

    // re-alloc the buffer down to the actual size of the string...
    Str_SetPtr(ppszSpecOut, szSpecOut);
    return TRUE;
}

//==========================================================================
// IDocFindFileFilter::PrepareToEnumObjects
//==========================================================================
STDMETHODIMP CDFFilter_PrepareToEnumObjects(IDocFindFileFilter *pdfff, DWORD *pdwFlags)
{
    int cPages;
    TC_DFITEMEXTRA tie;
    SHELLSTATE ss;
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);

    if (this->hwndTabs)
    {
        HWND    hwndMainDlg;
        hwndMainDlg = GetParent(this->hwndTabs);
        for (cPages = TabCtrl_GetItemCount(this->hwndTabs) -1; cPages >= 0; cPages--)
        {
            tie.tci.mask = TCIF_PARAM;
            TabCtrl_GetItem(this->hwndTabs, cPages, &tie.tci);
            SendNotify(tie.hwndPage, hwndMainDlg, PSN_APPLY, NULL);
        }
    }

    // Update the flags and buffer strings

    if (this->fTopLevelOnly)
        *pdwFlags &= ~DFOO_INCLUDESUBDIRS;
    else
        *pdwFlags |= DFOO_INCLUDESUBDIRS;

    if (this->fTextCaseSen)
        *pdwFlags |= FFLT_CASESEN;

    if (this->fTextReg)
        *pdwFlags |= FFLT_REGULAR;

    // Also get the shell state variables to see if we should show extensions and the like
    SHGetSetSettings(&ss, SSF_SHOWEXTENSIONS|SSF_SHOWALLOBJECTS, FALSE);

    if (ss.fShowExtensions)
        *pdwFlags |= DFOO_SHOWEXTENSIONS;
    else
        *pdwFlags &= ~DFOO_SHOWEXTENSIONS;

    this->fShowAllObjects = ss.fShowAllObjects;
    if (ss.fShowAllObjects)
        *pdwFlags |= DFOO_SHOWALLOBJECTS;
    else
        *pdwFlags &= ~DFOO_SHOWALLOBJECTS;

    // Now lets generate the file patern we will ask the system to look for
    // for now we will simply copy the file spec in...

    // Here is where we try to put some smarts into the file patterns stuff
    // It will go something like:
    // look between each; or , and see if there are any wild cards.  If not
    // do something like *patern*.
    // Also if there is no search pattern or if it is * or *.*, set the
    // filter to NULL as to speed it up.
    //

    DocFind_SetupWildCardingOnFileSpec(this->szUserInputFileSpec, &this->pszFileSpec);

    this->cFileSpecs = 0;
    if (this->pszFileSpec && this->pszFileSpec[0])
    {
        Str_SetPtr(&(this->pszSpecs), this->pszFileSpec);

        if (this->pszSpecs)
        {
            int cTokens = 0;
            LPTSTR pszToken = this->pszSpecs;

            while (pszToken)
            {
                // let's walk pszFileSpec to see how many specs we have...
                pszToken = StrChr(pszToken, TEXT(';'));
                if (pszToken)
                    pszToken++;
                cTokens++;
            }

            if (cTokens)
            {
                int i;
                // cleanup the previous search
                if (this->apszFileSpecs)
                    LocalFree(this->apszFileSpecs);
                this->apszFileSpecs = (LPTSTR *)LocalAlloc(LPTR, cTokens*SIZEOF(LPTSTR *));
                if (this->apszFileSpecs)
                {
                    this->cFileSpecs = cTokens;
                    pszToken = this->pszSpecs;
                    for (i = 0; i < cTokens; i++)
                    {
                        this->apszFileSpecs[i] = pszToken;
                        pszToken = StrChr(pszToken, TEXT(';'));
                        if (pszToken)
                            *pszToken++ = TEXT('\0');
                    }
                }
            }
        }
    }
    //
    // Also if there is a search string associated with this search
    // criteria, we need to initialize the search to allow greping on
    // it.
    // First check to see if we have an old one to release...
    if (this->lpgi)
    {
        FreeGrepBufs(this->lpgi);
        this->lpgi = NULL;
    }


    if (this->szText[0] != TEXT('\0'))
    {
#ifdef UNICODE
        LPSTR lpszText;
        UINT cchLength;

        cchLength = lstrlen(this->szText)+1;

        lpszText = this->szTextA;

        cchLength = WideCharToMultiByte(CP_ACP, 0,
                                        this->szText, cchLength,
                                        this->szTextA, ARRAYSIZE(this->szTextA),
                                        NULL, NULL);
        // Must double NULL terminate lpszText.  InitGrepInfo requires
        // this format!
        lpszText[cchLength] = '\0'; // Do not wrap with TEXT(); should be ANSI
#ifdef DOCFIND_RESUPPORT
        this->lpgi = InitGrepInfo(lpszText, (LPSTR)szNULL,
                *pdwFlags & (FFLT_REGULAR | FFLT_CASESEN));
#else
        this->lpgi = InitGrepInfo(lpszText, (LPSTR)szNULL,
                *pdwFlags & (FFLT_CASESEN));
#endif
#else
#ifdef DOCFIND_RESUPPORT
        this->lpgi = InitGrepInfo(this->szText, (LPTSTR)szNULL,
                *pdwFlags & (FFLT_REGULAR | FFLT_CASESEN));
#else
        this->lpgi = InitGrepInfo(this->szText, (LPTSTR)szNULL,
                *pdwFlags & (FFLT_CASESEN));
#endif
#endif

    }

    return S_OK;
}

//==========================================================================
// IDocFindFileFilter::GetDetailsof
//==========================================================================

STDMETHODIMP CDFFilter_GetDetailsOf(IDocFindFileFilter *pdfff, HDPA hdpaPidf,
        LPCITEMIDLIST pidl, UINT *piColumn, LPSHELLDETAILS pdi)
{
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    HRESULT hr = S_OK;

    if (*piColumn >= ARRAYSIZE(c_df_cols))
        return E_NOTIMPL;

    pdi->str.uType = STRRET_CSTR;
    pdi->str.cStr[0] = 0;

    if (!pidl)
    {
        pdi->fmt = c_df_cols[*piColumn].iFmt;
        pdi->cxChar = c_df_cols[*piColumn].cchCol;
        hr = ResToStrRet(c_df_cols[*piColumn].ids, &pdi->str);
    }
    else
    {
        TCHAR szTemp[MAX_PATH];
        if (*piColumn == IDFCOL_PATH)
        {
            // We need to now get to the idlist of the items folder.
            DFFolderListItem *pdffli = (DFFolderListItem *)DPA_GetPtr(hdpaPidf, DF_IFOLDER(pidl));
            if (pdffli)
            {
                SHGetPathFromIDList(&pdffli->idl, szTemp);
                hr = StringToStrRet(szTemp, &pdi->str);
                if (pdffli->iImage == -1)
                {
                    SHFILEINFO sfi;
                    SHGetFileInfo((LPCTSTR)&pdffli->idl, 0, &sfi, sizeof(sfi), SHGFI_PIDL | SHGFI_SYSICONINDEX);
                    pdffli->iImage = sfi.iIcon;
                }
            }
        } 
        else if (*piColumn == IDFCOL_RANK)
        {
            // See if the pidl has extra data or not.  If so use it
            PCHIDDENDOCFINDDATA phdfd = (PCHIDDENDOCFINDDATA) ILFindHiddenID(pidl, IDLHID_DOCFINDDATA);
            if (phdfd && (phdfd->wFlags & DFDF_EXTRADATA))
            {
                AddCommas(phdfd->ulRank, szTemp);
                hr = StringToStrRet(szTemp, &pdi->str);
            }
        }
#ifdef IF_ADD_MORE_COLS
        else if (*piColumn >= IDFCOL_FIRST_QUERY)
        {
            // OK simply return S_FALSE and if we have a query active the caller will handle it.
            *piColumn -= IDFCOL_FIRST_QUERY;
            hr = S_FALSE; // tell caller to do it
        }
#endif
        else if (*piColumn < ARRAYSIZE(c_df_cols))
        {
            // We need to now get to the idlist of the items folder.
            DFFolderListItem *pdffli = (DFFolderListItem *)DPA_GetPtr(hdpaPidf, DF_IFOLDER(pidl));
            // Let the file system function do it for us...
            if (pdffli)
            {
                IShellFolder *psf = DocFind_GetObjectsIFolder(NULL, pdffli, NULL);
                
                hr = E_FAIL;
                if (psf)
                {
                    IShellFolder2 *psf2;
                    
                    hr = psf->lpVtbl->QueryInterface(psf, &IID_IShellFolder2, (void **)&psf2);
                    if (SUCCEEDED(hr))
                    {
                        hr = MapSCIDToDetailsOf(psf2, pidl, c_df_cols[*piColumn].pscid, pdi);
                        psf2->lpVtbl->Release(psf2);
                    }
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
STDMETHODIMP CDFFilter_FDoesItemMatchFilter(IDocFindFileFilter *pdfff,
        LPTSTR pszFolder, WIN32_FIND_DATA * pfd,
        IShellFolder *psf, LPITEMIDLIST pidl)
{
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    SCODE sc = MAKE_SCODE(0, 0, 1);
    WORD   wFileDate, wFileTime;
    FILETIME ftLocal;
    TCHAR    szPath[MAX_PATH];
    WIN32_FIND_DATA finddata;

    if (psf)
    {
        STRRET strret;
        HANDLE hfind;
        LPITEMIDLIST pidlLast = ILFindLastID(pidl);
        // This came in through a notify.. We should get enough info to make it work
        // properly... For now this will be gross and we will hit file system...
        // Also we passed through the full pidl to make sure that we could
        // verify properly if this in the tree we are interested in...
        // BUGBUG:: Need to better handle multiple paths!
        if ((this->pidlStart && !ILIsParent(this->pidlStart, pidl, (BOOL)this->fTopLevelOnly))
        || (!this->pidlStart && (BOOL)this->fTopLevelOnly))
            return 0;

        pfd = &finddata;
        if (FAILED(psf->lpVtbl->GetDisplayNameOf(psf, pidlLast, SHGDN_FORPARSING, &strret)))
            return 0;
        if (FAILED(StrRetToBuf(&strret, pidlLast, szPath, ARRAYSIZE(szPath))))
            return 0;
        hfind = FindFirstFile(szPath, pfd);
        if (INVALID_HANDLE_VALUE == hfind)
            return 0;
        FindClose(hfind);
    }

    // Note: We do not use the IDList in this one...
    // This function does filtering of the file information for
    // things that are not part of the standard file filter

    // First things we dont show hidden files
    // If show all is set then we should include hidden files also...

    if (!this->fShowAllObjects &&
            (pfd->dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
        return (0);     // does not match

    if (IsSuperHidden(pfd->dwFileAttributes))
        return (0);     // does not match
    // Process the case where we are looking for folders only
    if (this->fFoldersOnly &&
            ((pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0))
        return (0);     // does not match

    switch (this->iSizeType)
    {
    case 1:   // >
        if (!(pfd->nFileSizeLow > this->dwSize))
            return (0);     // does not match
        break;
    case 2:   // <
        if (!(pfd->nFileSizeLow < this->dwSize))
            return (0);     // does not match
        break;
    }

    // See if we should compare dates...
    switch (this->wDateType & DFF_DATE_TYPEMASK)
    {
    case DFF_DATE_ACCESSED:
        FileTimeToLocalFileTime(&pfd->ftLastAccessTime, &ftLocal);
        break;
    case DFF_DATE_CREATED:
        FileTimeToLocalFileTime(&pfd->ftCreationTime, &ftLocal);
        break;

    case DFF_DATE_MODIFIED:
        FileTimeToLocalFileTime(&pfd->ftLastWriteTime, &ftLocal);
    }

    FileTimeToDosDateTime(&ftLocal, &wFileDate, &wFileTime);

    if (this->dateModifiedBefore != 0)
    {
        if (!(wFileDate <= this->dateModifiedBefore))
            return (0);     // does not match
    }

    if (this->dateModifiedAfter != 0)
    {
        if (!(wFileDate >= this->dateModifiedAfter))
            return (0);     // does not match
    }

    // Match file specificaitions.
    if (this->pszFileSpec && this->pszFileSpec[0])
    {
        // if we have split up version of the specs we'll use it because PathMatchSpec is pretty stupid 
        // and can take up to 5-6 hours for more than 10 wildcard specs
        if (this->cFileSpecs)
        {
            int i;
            BOOL bMatch = FALSE;
            
            for (i = 0; i < this->cFileSpecs; i++)
            {
                bMatch = PathMatchSpec(pfd->cFileName, this->apszFileSpecs[i]);
                if (bMatch)
                    break;
            }
            if (!bMatch)
                return (0);
        }
        else if (!PathMatchSpec(pfd->cFileName, this->pszFileSpec))
        {
        //short file name is never displayed to the user so don't use it to find match
        //&& !PathMatchSpec(pfd->cAlternateFileName, this->pszFileSpec))
            return (0);     // does not match
        }
    }

    if (this->szTypeFilePatterns[0])
    {
        // if looking for folders only and file pattern is all folders then no need to check
        // if folder name matches the pattern -- we know it is the folder, otherwise we
        // would have bailed out earlier in the function
        if (!(this->fFoldersOnly && lstrcmp(this->szTypeFilePatterns, TEXT(".")) == 0))
        {
            if (!PathMatchSpec(pfd->cFileName, this->szTypeFilePatterns))
                return (0);     // does not match
        }
    }

    //
    // See if we need to do a grep of the file
    if (this->lpgi)
    {
        HANDLE hfil;
        BOOL fMatch = FALSE;
        DWORD dwCFOpts = FILE_FLAG_SEQUENTIAL_SCAN;
#ifdef WINNT
        if (g_bRunOnNT5 && PathIsHighLatency(pfd->cFileName, pfd->dwFileAttributes))
        {
            if (this->fSearchSlowFiles)
                dwCFOpts |= FILE_FLAG_OPEN_NO_RECALL;
            else 
                return (0);  // No match returned if files are high latency and user hasn't selected to search high latency files
        }
#endif

        // Don't grep files with the system bit set.
        // This was added explicitly to not search things like the
        // swap file and the like.
        if (pfd->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
            return (0);     // does not match

        if (!psf)
        {
            lstrcpy(szPath, pszFolder);
            PathAppend(szPath, pfd->cFileName);
        }

        // Pass in File_write_attributes so we can change the file access time. Ya, wierd name
        // to pass in as a access type but this is what kernel looks for...
        // Also try to pass through the no recall flags... If we get access denied again then
        // dond't pass this flag, my guess is that some networks like netware probably don't have
        // support for this flag...
        hfil = CreateFile(szPath,
                GENERIC_READ | FILE_WRITE_ATTRIBUTES,
                FILE_SHARE_READ ,
                 0, OPEN_EXISTING, dwCFOpts, 0);

        if (hfil == INVALID_HANDLE_VALUE)
        {
            // Some readonly shares don't like the FILE_WRITE_ATTRIBUTE, try without
            if (GetLastError() ==  ERROR_ACCESS_DENIED)
                hfil = CreateFile(szPath, GENERIC_READ, FILE_SHARE_READ , 0, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, 0);
        }

        if (hfil != INVALID_HANDLE_VALUE)
        {
            // Go get around wimpy APIS that set the access date...
            FILETIME ftLastAccess;

            if (GetFileTime(hfil, NULL, &ftLastAccess, NULL))
                SetFileTime(hfil, NULL, &ftLastAccess, NULL);

            fMatch = FileFindGrep(this->lpgi, hfil, FIND_FILE, NULL) != 0;
            CloseHandle(hfil);
        }
        if (!fMatch)
            return (0);     // does not match
    }


    return (sc);    // return TRUE to imply yes!
}

//==========================================================================
// Helper function for save criteria that will output the string and
// and id to the specified file.  it will also test for NULL and the like
//==========================================================================
int Docfind_SaveCriteriaItem(IStream * pstm, WORD wNum,
        LPTSTR psz, WORD fCharType)
{
    if ((psz == NULL) || (*psz == TEXT('\0')))
        return 0;
    else
    {
        LPVOID pszText = (LPVOID)psz; // Ptr to output text. Defaults to source.
#ifdef WINNT
        //
        // These are required to support ANSI-unicode conversions.
        //
        LPSTR pszAnsi  = NULL; // For unicode-to-ansi conversion.
        LPWSTR pszWide = NULL; // For ansi-to-unicode conversion.
#endif
        DFCRITERIA dfc;
        dfc.wNum = wNum;
        dfc.cbText = (WORD) ((lstrlen(psz) + 1) * SIZEOF(TCHAR));

#ifdef WINNT
#ifdef UNICODE
        //
        // Source string is Unicode but caller wants to save as ANSI.
        //
        if (DFC_FMT_ANSI == fCharType)
        {
           // Convert to ansi and write ansi.
           dfc.cbText = (WORD) WideCharToMultiByte(CP_ACP, 0L, psz, -1, pszAnsi, 0, NULL, NULL);

           if ((pszAnsi = (LPSTR)LocalAlloc(LMEM_FIXED, dfc.cbText)) != NULL)
           {
              WideCharToMultiByte(CP_ACP, 0L, psz, -1, pszAnsi, dfc.cbText / sizeof(pszAnsi[0]), NULL, NULL);
              pszText = (LPVOID)pszAnsi;
           }
        }
#else
        //
        // Source string is ANSI but caller wants to save as Unicode.
        //
        if (DFC_FMT_UNICODE == fCharType)
        {
           // Convert to unicode and write unicode.
           dfc.cbText = MultiByteToWideChar(CP_ACP, 0L, psz, -1, pszWide, 0);

           if ((pszWide = (LPWSTR)LocalAlloc(LMEM_FIXED, dfc.cbText)) != NULL)
           {
              MultiByteToWideChar(CP_ACP, 0L, psz, -1, pszWide, dfc.cbText / sizeof(pszWide[0]));
              pszText = (LPVOID)pszWide;
           }
        }
#endif  // UNICODE
#endif  // WINNT

        pstm->lpVtbl->Write(pstm, (LPTSTR)&dfc, SIZEOF(dfc), NULL);   // Output index
        pstm->lpVtbl->Write(pstm, pszText, dfc.cbText, NULL);  // output string + NULL

#ifdef WINNT
        //
        // Free up conversion buffers if any were created.
        //
        if (NULL != pszAnsi)
           LocalFree(pszAnsi);
        if (NULL != pszWide)
           LocalFree(pszWide);
#endif

    }

    return(1);
}

//==========================================================================
// IDocFindFileFilter::SaveCriteria
//==========================================================================
STDMETHODIMP CDFFilter_SaveCriteria(IDocFindFileFilter *pdfff, IStream * pstm, WORD fCharType)
{
    const TCHAR c_szPercentD[] = TEXT("%d");
    //
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    int cCriteria;
    TCHAR szTemp[40];    // some random size
    LPITEMIDLIST pidlMyComputer;

    // The caller should have already validated the stuff and updated
    // everything for the current filter information.

    // we need to walk through and check each of the items to see if we
    // have a criteria to save away. this includes:
    //      (Name, Path, Type, Contents, size, modification dates)
    cCriteria = Docfind_SaveCriteriaItem(pstm, IDD_FILESPEC, this->szUserInputFileSpec, fCharType);

    pidlMyComputer = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, FALSE);
    if (this->pidlStart && ILIsEqual(this->pidlStart, pidlMyComputer))
    {
        cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_PATH,
                TEXT("::"), fCharType);
    }
    else
        cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_PATH,
                this->szPath, fCharType);
    ILFree(pidlMyComputer);
    cCriteria += Docfind_SaveCriteriaItem(pstm, DFSC_SEARCHFOR,
            this->pszIndexedSearch, fCharType);
    cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_TYPECOMBO,
            this->szTypeFilePatterns, fCharType);
    cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_CONTAINS,
            this->szText, fCharType);
    
    // Also save away the state of the top level only
    wsprintf(szTemp, c_szPercentD, this->fTopLevelOnly);
    cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_TOPLEVELONLY, szTemp, fCharType);

    // The Size field is little more fun!
    if (this->iSizeType != 0)
    {
        wsprintf(szTemp, TEXT("%d %ld"), this->iSizeType, this->dwSize);
        cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_SIZECOMP, szTemp, fCharType);
    }

    // Likewise for the dates, should be fun as we need to save it depending on
    // how the date was specified
    switch (this->wDateType & DFF_DATE_RANGEMASK)
    {
    case DFF_DATE_ALL:
        // nothing to store
        break;
    case DFF_DATE_DAYS:
        wsprintf(szTemp, c_szPercentD, this->wDateValue);
        cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_MDATE_NUMDAYS, szTemp, fCharType);
        break;
    case DFF_DATE_MONTHS:
        wsprintf(szTemp, c_szPercentD, this->wDateValue);
        cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_MDATE_NUMMONTHS, szTemp, fCharType);
        break;
    case DFF_DATE_BETWEEN:
        if (this->dateModifiedAfter != 0)
        {
            wsprintf(szTemp, c_szPercentD, this->dateModifiedAfter);
            cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_MDATE_FROM, szTemp, fCharType);
        }

        if (this->dateModifiedBefore != 0)
        {
            wsprintf(szTemp, c_szPercentD, this->dateModifiedBefore);
            cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_MDATE_TO, szTemp, fCharType);
        }
        break;
    }

    if (((this->wDateType & DFF_DATE_RANGEMASK) != DFF_DATE_ALL) &&
        (this->wDateType & DFF_DATE_TYPEMASK))
    {
        wsprintf(szTemp, c_szPercentD, this->wDateType & DFF_DATE_TYPEMASK);
        cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_MDATE_TYPE, szTemp, fCharType);
    }

    if( this->fTextCaseSen )
    {
        wsprintf( szTemp, TEXT("%d"), this->fTextCaseSen );
        cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_TEXTCASESEN, szTemp, fCharType);
    }

    if( this->fTextReg )
    {
        wsprintf( szTemp, TEXT("%d"), this->fTextReg );
        cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_TEXTREG, szTemp, fCharType);
    }

    if( this->fSearchSlowFiles )
    {
        wsprintf( szTemp, TEXT("%d"), this->fSearchSlowFiles );
        cCriteria += Docfind_SaveCriteriaItem(pstm, IDD_SEARCHSLOWFILES, szTemp, fCharType);
    }

    return (MAKE_SCODE(0, 0, cCriteria));
}

//==========================================================================
// IDocFindFileFilter::RestoreCriteria
//==========================================================================
STDMETHODIMP CDFFilter_RestoreCriteria(IDocFindFileFilter *pdfff,
        IStream * pstm, int cCriteria, WORD fCharType)
{
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    TCHAR szTemp[MAX_PATH];    // some random size

    if (cCriteria > 0)
        this->fWeRestoredSomeCriteria = TRUE;

    while (cCriteria--)
    {
        // BUGBUG(DavePl) I'm assuming that if UNICODE chars are written in this
                // stream, the cb is written accordingly.  ie: what you put in the stream
                // is your own business, but write the cb as a byte count, not char count

        DFCRITERIA dfc;
        int cb;

        if (FAILED(pstm->lpVtbl->Read(pstm, &dfc, SIZEOF(dfc), &cb))
                || (cb != SIZEOF(dfc)) || (dfc.cbText > SIZEOF(szTemp)))
            break;
#ifdef WINNT
#ifdef UNICODE
        if (DFC_FMT_UNICODE == fCharType)
        {
           //
           // Destination is Unicode and we're reading Unicode data from stream.
           // No conversion required.
           //
           if (FAILED(pstm->lpVtbl->Read(pstm, szTemp, dfc.cbText, &cb))
                   || (cb != dfc.cbText))
               break;
        }
        else
        {
           char szAnsi[MAX_PATH];

           //
           // Destination is Unicode but we're reading ANSI data from stream.
           // Read ansi.  Convert to unicode.
           //
           if (FAILED(pstm->lpVtbl->Read(pstm, szAnsi, dfc.cbText, &cb))
                   || (cb != dfc.cbText))
               break;

           MultiByteToWideChar(CP_ACP, 0L, szAnsi, -1, szTemp, ARRAYSIZE(szTemp));
        }
#else
        if (DFC_FMT_ANSI == fCharType)
        {
           //
           // Destination is ANSI and we're reading ANSI data from stream.
           // No conversion required.
           //
           if (FAILED(pstm->lpVtbl->Read(pstm, szTemp, dfc.cbText, &cb))
                   || (cb != dfc.cbText))
               break;
        }
        else
        {
           //
           // Destination is ANSI but we're reading Unicode data from stream.
           // Read unicode.  Convert to ansi.
           //
           WCHAR szWide[MAX_PATH];

           if (FAILED(pstm->lpVtbl->Read(pstm, szWide, dfc.cbText, &cb))
                   || (cb != dfc.cbText))
               break;

           WideCharToMultiByte(CP_ACP, 0L, szWide, -1, szTemp, ARRAYSIZE(szTemp), NULL, NULL);
        }

#endif  // UNICODE

#else
        if (FAILED(pstm->lpVtbl->Read(pstm, &szTemp, dfc.cbText, &cb))
                || (cb != dfc.cbText))
            break;

#endif  // WINNT

        switch (dfc.wNum)
        {
        case IDD_FILESPEC:
            lstrcpy(this->szUserInputFileSpec, szTemp);
            break;

        case DFSC_SEARCHFOR:
            Str_SetPtr(&(this->pszIndexedSearch), szTemp);
            break;

        case IDD_PATH:
            if (lstrcmp(szTemp, TEXT("::")) == 0)
            {
                this->pidlStart = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, FALSE);
                szTemp[0] = TEXT('\0');
            }
            else if (StrChr(szTemp,TEXT(';')) == NULL)
            {
                // Simple pidl...
                this->pidlStart = ILCreateFromPath(szTemp);
            }
            lstrcpy(this->szPath, szTemp);
            break;

        case IDD_TOPLEVELONLY:
            this->fTopLevelOnly = StrToInt(szTemp);
            break;

        case IDD_TYPECOMBO:
            lstrcpy(this->szTypeFilePatterns, szTemp);
            break;

        case IDD_CONTAINS:
            lstrcpy(this->szText, szTemp);
            break;

        case IDD_SIZECOMP:
            // we need to extract off the two parts, the type and
            // the value

            this->iSizeType = szTemp[0] - TEXT('0');
            this->dwSize = StrToInt(&szTemp[2]);
            break;

        case IDD_MDATE_NUMDAYS:
            this->wDateType = DFF_DATE_DAYS;
            this->wDateValue = (WORD) StrToInt(szTemp);
            break;
        case IDD_MDATE_NUMMONTHS:
            this->wDateType = DFF_DATE_MONTHS;
            this->wDateValue = (WORD) StrToInt(szTemp);
            break;

        case IDD_MDATE_FROM:
            this->wDateType = DFF_DATE_BETWEEN;
            this->dateModifiedAfter = (WORD) StrToInt(szTemp);
            break;

        case IDD_MDATE_TO:
            this->wDateType = DFF_DATE_BETWEEN;
            this->dateModifiedBefore = (WORD) StrToInt(szTemp);
            break;
        case IDD_MDATE_TYPE:
            this->wDateType |= (WORD)StrToInt(szTemp);
            break;

        case IDD_TEXTCASESEN:
            this->fTextCaseSen = (BITBOOL)StrToInt(szTemp);
            break;

        case IDD_TEXTREG:
            this->fTextReg = (BITBOOL)StrToInt(szTemp);
            break;

        case IDD_SEARCHSLOWFILES:
            this->fSearchSlowFiles = (BITBOOL)StrToInt(szTemp);
            break;
        }
    }
    return (S_OK);
}

//==========================================================================
// IDocFindFileFilter::GetColSaveStream
//==========================================================================
STDMETHODIMP CDFFilter_GetColSaveStream(IDocFindFileFilter *pnetf, WPARAM wParam, IStream **ppstm)
{
    *ppstm = OpenRegStream(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, TEXT("DocFindColsX"), (DWORD) wParam);
    return(*ppstm ? S_OK : E_FAIL);
}


DWORD AddToQuery(LPWSTR *ppszBuf, DWORD *pcchBuf, LPWSTR pszAdd)
{
    DWORD cchAdd = lstrlenW(pszAdd);

    if (*ppszBuf && *pcchBuf > cchAdd)
    {
        StrCpyNW(*ppszBuf, pszAdd, *pcchBuf);
        *pcchBuf -= cchAdd;
        *ppszBuf += cchAdd;
    }
    return cchAdd;
}

DWORD AddQuerySep(DWORD *pcchBuf, LPWSTR *ppszCurrent, WCHAR  bSep)
{
    LPWSTR pszCurrent = *ppszCurrent;
    // make sure we have room for us plus terminator...
    if (*ppszCurrent && *pcchBuf >= 4)
    {
        *pszCurrent++ = L' ';
        *pszCurrent++ = bSep;
        *pszCurrent++ = L' ';

        *ppszCurrent = pszCurrent;
        *pcchBuf -= 3;
    }
    return 3; // size necessary
}

DWORD PrepareQueryParam(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent)
{
    if (*pbFirst)
    {
        *pbFirst = FALSE;
        return 0;  // no size necessary
    }
        
    // we're not the first property
    return AddQuerySep(pcchBuf, ppszCurrent, L'&');
}

// pick the longest date query so we can avoid checking the buffer size each time we
// add something to the string
#define LONGEST_DATE  50 //lstrlen(TEXT("{prop name=access} <= 2000/12/31 23:59:59{/prop}"))+2

DWORD QueryDosDate(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, WORD wDate, WORD wDateType, BOOL bBefore)
{
    FILETIME ftLocal, ftGMT;
    SYSTEMTIME st;
    LPWSTR pszCurrent = *ppszCurrent;
    DWORD  cchNeeded = PrepareQueryParam(pbFirst, pcchBuf, &pszCurrent);
    
    if (pszCurrent && *pcchBuf > LONGEST_DATE)
    {
        DosDateTimeToFileTime(wDate, 0, &ftLocal);
        LocalFileTimeToFileTime(&ftLocal, &ftGMT);
        FileTimeToSystemTime(&ftGMT, &st);
        
        switch (wDateType & DFF_DATE_TYPEMASK)
        {
        case DFF_DATE_ACCESSED:
            StrCpyNW(pszCurrent, L"{prop name=access} ", *pcchBuf);
            break;
        case DFF_DATE_CREATED:
            StrCpyNW(pszCurrent, L"{prop name=create} ", *pcchBuf);
            break;
        case DFF_DATE_MODIFIED:
            StrCpyNW(pszCurrent, L"{prop name=write} ", *pcchBuf);
            break;
        }
    
        pszCurrent += lstrlenW(pszCurrent);
        if (bBefore)
        {
            *pszCurrent++ = L'<';
            // BUGBUG:: if you ask for a range like: 2/20/98 - 2/20/98 then we get no time at all
            // So for before, convert H:m:ss to 23:59:59...
            st.wHour = 23;
            st.wMinute = 59; 
            st.wSecond = 59;
        }
        else
            *pszCurrent++ = L'>';
        
        *pszCurrent++ = L'=';

        wnsprintfW(pszCurrent, *pcchBuf, L" %d/%d/%d %d:%d:%d{/prop}", st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond);
        pszCurrent += lstrlenW(pszCurrent);
        
        *ppszCurrent = pszCurrent;
        *pcchBuf -= LONGEST_DATE;
    }
    return cchNeeded + LONGEST_DATE;
}

DWORD CIQueryFilePatterns(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, LPWSTR pszFilePatterns)
{
    WCHAR szNextPattern[MAX_PATH];  // overkill in size
    BOOL fFirst = TRUE;
    LPCWSTR pszNextPattern = pszFilePatterns;
    DWORD cchNeeded = PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);

    // Currently will have to long hand the query, may try to find shorter format once bugs
    // are fixed...
    // 
    cchNeeded += AddToQuery(ppszCurrent, pcchBuf, L"(");
    while ((pszNextPattern = NextPathW(pszNextPattern, szNextPattern, ARRAYSIZE(szNextPattern))) != NULL)
    {
        if (!fFirst)
        {
            cchNeeded += AddToQuery(ppszCurrent, pcchBuf, L" | ");
        }
        fFirst = FALSE;
        cchNeeded += AddToQuery(ppszCurrent, pcchBuf, L"#filename ");
        cchNeeded += AddToQuery(ppszCurrent, pcchBuf, szNextPattern);
    }
    cchNeeded += AddToQuery(ppszCurrent, pcchBuf, L")");
    return cchNeeded;
}


DWORD CIQueryTextPatterns(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, LPWSTR pszText, BOOL bTextReg)
{
    DWORD cchNeeded = PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);

    cchNeeded += AddToQuery(ppszCurrent, pcchBuf, L"{prop name=all}");
    cchNeeded += AddToQuery(ppszCurrent, pcchBuf, bTextReg? L"{regex}" : L"{phrase}");
    cchNeeded += AddToQuery(ppszCurrent, pcchBuf, pszText);
    cchNeeded += AddToQuery(ppszCurrent, pcchBuf, bTextReg? L"{/regex}{/prop}" : L"{/phrase}{/prop}");

    return cchNeeded;
}

#define MAX_DWORD_LEN  18

DWORD CIQuerySize(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, DWORD dwSize, int iSizeType)
{
    WCHAR szSize[MAX_DWORD_LEN+8]; // +8 for " {/prop}"
    DWORD cchNeeded = PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);

    cchNeeded += AddToQuery(ppszCurrent, pcchBuf, L"{prop name=size} ");
    cchNeeded += AddToQuery(ppszCurrent, pcchBuf, iSizeType == 1? L">" : L"<");
            
    wnsprintfW(szSize, *pcchBuf, L" %d{/prop}", dwSize);
    cchNeeded += AddToQuery(ppszCurrent, pcchBuf, szSize);

    return cchNeeded;
}

DWORD CIQueryIndex(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent, LPWSTR pszText)
{
    DWORD cchNeeded = PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);

    cchNeeded += AddToQuery(ppszCurrent, pcchBuf, pszText);
    return cchNeeded;
}

DWORD CIQueryShellSettings(BOOL *pbFirst, DWORD *pcchBuf, LPWSTR *ppszCurrent)
{
    DWORD cchNeeded = 0;
    
    if (!ShowSuperHidden())
    {
        cchNeeded += PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);
        cchNeeded += AddToQuery(ppszCurrent, pcchBuf, L"NOT @attrib ^a 0x6 ");// don't show files w/ hidden and system bit on
    }

    {
        SHELLSTATE ss;
        
        SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
        if (!ss.fShowAllObjects)
        {
            cchNeeded += PrepareQueryParam(pbFirst, pcchBuf, ppszCurrent);
            cchNeeded += AddToQuery(ppszCurrent, pcchBuf, L"NOT @attrib ^a 0x2 "); // don't show files w/ hidden bit on
        }
    }
    return cchNeeded;
}

void CDFFilter_GenerateQueryHelper(CDFFilter *this, LPWSTR pwszQuery, DWORD *pcchQuery)
{
    DWORD cchNeeded = 0, cchLeft = *pcchQuery;
    LPWSTR pszCurrent = pwszQuery;
    BOOL  bFirst = TRUE; // first property
    USES_CONVERSION;

    if (this->pszFileSpec && this->pszFileSpec[0])
    {
        cchNeeded += CIQueryFilePatterns(&bFirst, &cchLeft, &pszCurrent, T2W(this->pszFileSpec));
    }

    // fFoldersOnly = TRUE implies szTypeFilePatterns = "."
    // we cannot pass "." to CI because they won't understand it as give me the folder types
    // we could check for @attrib ^a FILE_ATTRIBUTE_DIRECTORY (0x10) but ci doesn't index the folder names by default
    // so we normally won't get any results...
    if (!this->fFoldersOnly && this->szTypeFilePatterns[0])
    {
        cchNeeded += CIQueryFilePatterns(&bFirst, &cchLeft, &pszCurrent, T2W(this->szTypeFilePatterns));
    }
    
    // Date:
    if (this->dateModifiedBefore != 0)
    {           
        cchNeeded += QueryDosDate(&bFirst, &cchLeft, &pszCurrent, this->dateModifiedBefore, this->wDateType, TRUE);
    }
    
    if (this->dateModifiedAfter != 0)
    {
        cchNeeded += QueryDosDate(&bFirst, &cchLeft, &pszCurrent, this->dateModifiedAfter, this->wDateType, FALSE);
    }

    // Size:
    if (this->iSizeType != 0)
    {
        cchNeeded += CIQuerySize(&bFirst, &cchLeft, &pszCurrent, this->dwSize, this->iSizeType);
    }

    // Indexed Search: raw query
    if (this->pszIndexedSearch && (this->pszIndexedSearch[0] != TEXT('\0')))
    {
        // HACK Alert if first Char is ! then we assume Raw and pass it through directly to CI...
        // Likewise if it starts with @ or # pass through, but remember the @...
        cchNeeded += CIQueryIndex(&bFirst, &cchLeft, &pszCurrent, T2W(this->pszIndexedSearch));
    }

    // Containing Text:
    if (this->szText[0] != TEXT('\0'))
    {
        // Try not to quote the strings unless we need to.  This allows more flexability to do the
        // searching for example: "cat near dog" is different than: cat near dog
        cchNeeded += CIQueryTextPatterns(&bFirst, &cchLeft, &pszCurrent, T2W(this->szText), this->fTextReg);
    }

    cchNeeded += CIQueryShellSettings(&bFirst, &cchLeft, &pszCurrent);
#ifdef WINNT
    // assume TCHAR = WCHAR bellow
    {
        HKEY  hkey;
        UINT  cPaths;
        LPWSTR *apwszPaths;
        TCHAR szPath[MAX_PATH];

        // don't search recycle bin folder.  we add both nt4's recycled and nt5's recycler
        // for every drive we search.
        if (SUCCEEDED(DF_GetSearchPaths(this, NULL, &apwszPaths, &cPaths)))
        {
            int i;
            
            for (i = cPaths-1; i >= 0; i--)
            {
                if (PathStripToRootW(apwszPaths[i]))
                {
                    int iBin=0, cBins=2;
                    HRESULT hr;

                    static LPCWSTR s_awszRecycleBins[] = { L"Recycled\\*", L"Recycler\\*", };
#define NTFS_RBIN  1  // recycle bin on an ntfs drive is called recycler
#define FAT_RBIN   0  //              ||                        recycled

#define CCH_EXTRA  lstrlen(TEXT(" & !#PATH "))

                    hr = _IsNTFSDrive(PathGetDriveNumber(apwszPaths[i]));
                    // in the failiure case we exclude both recycler and recycled
                    if (SUCCEEDED(hr))
                    {
                        iBin = hr == S_OK? NTFS_RBIN : FAT_RBIN;
                        cBins = 1;
                    }
                    for (; iBin < ARRAYSIZE(s_awszRecycleBins) && cBins > 0; iBin++, cBins--)
                    {
                        if (PathCombine(szPath, apwszPaths[i], s_awszRecycleBins[iBin]))
                        {
                            DWORD cchSize = lstrlen(szPath)+CCH_EXTRA;

                            // don't bail out early if we are asked for size of query
                            if (pwszQuery && cchSize > cchLeft)
                                break;

                            cchNeeded += AddToQuery(&pszCurrent, &cchLeft, L" & !#PATH ");
                            cchNeeded += AddToQuery(&pszCurrent, &cchLeft, szPath);
                        }
                    }
                }
            }
            // we must exclude the special folders from the results or ci will find items that 
            // we cannot get pidls for.
            if (RegOpenKeyEx(HKEY_CURRENT_USER, CI_SPECIAL_FOLDERS, 0, KEY_READ, &hkey) == ERROR_SUCCESS)
            {
                DWORD cValues = 0; // init to zero in case query info bellow fails
                TCHAR szName[10];
            
                RegQueryInfoKey(hkey, NULL, NULL, NULL, NULL, NULL, NULL, &cValues, NULL, NULL, NULL, NULL);
                if (cValues)
                {
                    DWORD i;
                    
                    for (i=0; i < cValues; i++)
                    {
                        DWORD cb = ARRAYSIZE(szPath)*SIZEOF(TCHAR);;

                        wsprintf(szName, TEXT("%d"), i);
                        if (RegQueryValueEx(hkey, szName, NULL, NULL, (BYTE *)szPath, &cb) == ERROR_SUCCESS)
                        {
                            // no +1 since 0 is already in szQuery
                            DWORD cchSize;
                            UINT  iDrive;
                            BOOL  bInclude = FALSE;

                            for (iDrive=0; iDrive < cPaths; iDrive++)
                            {
                                // szPath has " as the first char so skip it..
                                if (PathGetDriveNumber(apwszPaths[iDrive]) == PathGetDriveNumber(szPath+1))
                                {
                                    bInclude = TRUE;
                                    break;
                                }
                            }
                            if (!bInclude)
                                continue;
                            cchSize = lstrlen(szPath)+CCH_EXTRA;
                            // don't bail out early if we are asked for size of query
                            if (pwszQuery && cchSize > cchLeft)
                                break;

                            cchNeeded += AddToQuery(&pszCurrent, &cchLeft, L" & !#PATH ");
                            cchNeeded += AddToQuery(&pszCurrent, &cchLeft, szPath);
                        }
                    }
                }
                RegCloseKey(hkey);
            }
            // clean up
            for (i = cPaths-1; i >= 0; i--)
            {
                LocalFree((HLOCAL)apwszPaths[i]);
            }
            LocalFree((HLOCAL)apwszPaths);
        }
    }
#endif
    // If no constraints are set, CI errors out, so instead give it a dummy one of filename
    // *.*
    if (pwszQuery && pszCurrent == pwszQuery)
        CIQueryFilePatterns(&bFirst, &cchLeft, &pszCurrent, L"*.*");

    if (pszCurrent)
    {
        // Make sure we terminate the string at the end...
        *pszCurrent = TEXT('\0');
    }

    if (!pwszQuery)
    {
        *pcchQuery = cchNeeded;
    }
    else
    {
        ASSERT(*pcchQuery > cchNeeded);
    }
}
//==========================================================================
// Create a query command string out of the search criteria
//==========================================================================
STDMETHODIMP CDFFilter_GenerateQueryRestrictions(IDocFindFileFilter *pdfff, LPWSTR *ppwszQuery, 
         DWORD *pdwGQR_Flags)
{
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    // we should be able to make use of ci no matter what (exceptions at the end of the function)
    DWORD dwGQR_Flags = GQR_MAKES_USE_OF_CI; 
    HRESULT hres = S_OK;

    // Named: or Of type:
    // Match file specificaitions.

    if (ppwszQuery)
    {
        DWORD cchNeeded = 0;
        
        CDFFilter_GenerateQueryHelper(this, NULL, &cchNeeded);
        cchNeeded++;  // for \0
        
        *ppwszQuery = (LPWSTR)LocalAlloc(LMEM_FIXED, cchNeeded * sizeof(WCHAR));
        if (! *ppwszQuery)
            return E_OUTOFMEMORY;
        CDFFilter_GenerateQueryHelper(this, *ppwszQuery, &cchNeeded);
    }
    
    if (this->pszIndexedSearch && (this->pszIndexedSearch[0] != TEXT('\0')))
        dwGQR_Flags |= GQR_REQUIRES_CI;

    // ci is not case sensitive, so if user wanted case sensitive search we cannot use ci
    // also ci doesn't index folder names by default so to be safe we just default to our
    // disk traversal algorithm...
    if (this->fTextCaseSen || this->fFoldersOnly)
    {    
        if (dwGQR_Flags & GQR_REQUIRES_CI && this->fTextCaseSen)
            hres = MAKE_HRESULT(3, FACILITY_SEARCHCOMMAND, SCEE_CASESENINDEX);
        else if (dwGQR_Flags & GQR_MAKES_USE_OF_CI)
            dwGQR_Flags &= ~GQR_MAKES_USE_OF_CI;
    }
        
    *pdwGQR_Flags = dwGQR_Flags;  // return calculated Flags...
    return hres;
}

STDMETHODIMP CDFFilter_ReleaseQuery(IDocFindFileFilter *pdfff)
{
#ifdef WINNT
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    if (this->pdfenumAsync)
    {
        this->pdfenumAsync->lpVtbl->Release(this->pdfenumAsync);
        this->pdfenumAsync = NULL;
    }
#endif
    return S_OK;
}
                       
STDMETHODIMP CDFFilter_GetQueryLanguageDialect(IDocFindFileFilter *pdfff, ULONG* pulDialect)
{
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    
    if(NULL == this || NULL == pulDialect)
        return E_POINTER;

    *pulDialect = this->ulQueryDialect;
    return S_OK;
}

STDMETHODIMP CDFFilter_GetWarningFlags(IDocFindFileFilter *pdfff, DWORD* pdwWarningFlags)
{
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);

    if(NULL == this || NULL == pdwWarningFlags)
        return E_POINTER;

    *pdwWarningFlags = this->dwWarningFlags;
    return S_OK;
}


////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// Now starting the code for the name and location page
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


//=========================================================================
void DocFind_SizeControl(HWND hwndDlg, int id, int cx, BOOL fCombo)
{
    RECT rc;
    RECT rcList;
    HWND hwndCtl;

    GetWindowRect(hwndCtl = GetDlgItem(hwndDlg, id), &rc);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (POINT *)&rc, 2);

    if (fCombo)
    {
        // These guys are comboboxes so work with them...
        SendMessage(hwndCtl, CB_GETDROPPEDCONTROLRECT, 0,
                (LPARAM)(RECT *)&rcList);
        rc.bottom += (rcList.bottom - rcList.top);
    }

    SetWindowPos(hwndCtl, NULL, 0, 0, cx - rc.left,
            rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}



//==========================================================================
//
// Process the WM_SIZE of the details page
//
void DocFind_DFNameLocOnSize(HWND hwndDlg, UINT state, int cx, int cy)
{
    RECT rc;
    int cxMargin;
    HWND hwndCtl;
    if (state == SIZE_MINIMIZED)
        return;         // don't bother when we are minimized...

    // Get the location of first static to calculate margin
    GetWindowRect(GetDlgItem(hwndDlg, IDD_STATIC), &rc);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (POINT *)&rc, 2);
    cxMargin = rc.left;
    cx -= cxMargin;

    DocFind_SizeControl(hwndDlg, IDD_FILESPEC, cx, TRUE);
    DocFind_SizeControl(hwndDlg, IDD_CONTAINS, cx, FALSE);

    // Now move the browse button
    GetWindowRect(hwndCtl = GetDlgItem(hwndDlg, IDD_BROWSE), &rc);
    MapWindowPoints(HWND_DESKTOP, hwndDlg, (POINT *)&rc, 2);
    SetWindowPos(hwndCtl, NULL, cx - (rc.right - rc.left), rc.top, 0, 0,
            SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    // And size the path field
#if 1
    DocFind_SizeControl(hwndDlg, IDD_PATH, cx, TRUE);
#else
    DocFind_SizeControl(hwndDlg, IDD_PATH, cx - cxMargin - (rc.right - rc.left), TRUE);
#endif            
}

//==========================================================================
// Add a string to our name location list...
//==========================================================================
int DFNameLocAddString(LPDOCFINDPROPSHEETPAGE pdfpsp, LPTSTR pszPath)
{
    LPTSTR pszT;
    LPITEMIDLIST pidl;
    int iSel;
    LPITEMIDLIST pidlT;
    HWND hwndCtl = GetDlgItem(pdfpsp->hwndDlg, IDD_PATH);
    TCHAR szPath[MAX_PATH];
    int iNew = CB_ERR;

    // Don't muck with original as it might have been passed to us by someone who
    // allocated an exact size...
    lstrcpy(szPath, pszPath);

    // If we only have one path
    pszT = szPath + StrCSpn(szPath, TEXT(",;"));
    if (*pszT == TEXT('\0'))
    {
        // Try to parse the display name into a Pidl
        PathQualify(szPath);
        pidl = ILCreateFromPath(szPath);

        if (!pidl)
        {
            TCHAR szDisplayName[MAX_PATH];
            int cch;
            // See if the beginning of the string matches the
            // start of an item in the list.  If so we should
            // Try to see if we can convert it to a valid
            // pidl/string...

            for (iSel=(int)SendMessage(hwndCtl,
                    CB_GETCOUNT, 0, 0); iSel >= 1; iSel--)
            {
                SendMessage(hwndCtl, CB_GETLBTEXT,
                        (WPARAM)iSel, (LPARAM)szDisplayName);
                cch = lstrlen(szDisplayName);
                if ((CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                        szDisplayName, cch, szPath, cch) == 2) &&
                        (szPath[cch] == TEXT('\\')))
                {
                    pidlT = (LPITEMIDLIST)SendMessage(hwndCtl,
                            CB_GETITEMDATA, (WPARAM)iSel, 0);

                    // Don't blow up if we get a failure here

                    if (pidlT != (LPITEMIDLIST)-1)
                    {
                        SHGetPathFromIDList(pidlT, szDisplayName);
                        PathAppend(szDisplayName, CharNext(szPath+cch));
                        pidl = ILCreateFromPath(szDisplayName);
                        if (pidl)
                        {
                            lstrcpy(szPath, szDisplayName);
                            break;
                        }
                    }
#ifdef DEBUG
                    else
                    {
                        DebugMsg(DM_TRACE, TEXT("DFNameLocAddString - CBItem ptr = -1"));
                    }
#endif
                }
            }
        }

    }
    else
    {
        // Multiple things let it run by itself...
        pidl = NULL;      // dont have a pidl to begin with...
    }

    if (pidl)
    {
        IShellFolder *psfParent;
        HRESULT hres = SHBindToIDListParent(pidl, &IID_IShellFolder, (void **)&psfParent, NULL);
        if (SUCCEEDED(hres))
        {
            // DocFind_LocCBAddPidl fist ILCombine()s parent and child, since
            // we didn't separate the parent from the child, pass in the full
            // pidl and NULL child -- they'll get ILCombine()d appropriately.
            iNew = DocFind_LocCBAddPidl(hwndCtl, psfParent, pidl, NULL, NULL, TRUE, 0);
            psfParent->lpVtbl->Release(psfParent);
        }

        ILFree(pidl);
    }
    else
    {
        // Add the whole string as an item...
        iNew = DocFind_LocCBAddItem(hwndCtl, NULL, -1, 0, -1, szPath);
    }
    if (iNew != CB_ERR)
        SendMessage(hwndCtl, CB_SETCURSEL, iNew, 0);

    return iNew;
}



/*----------------------------------------------------------------------------
/ build_drive_string implementation
/ ------------------
/ Purpose:
/   Convert from a bit stream ( 1 bit per drive ) to a comma seperated
/   list of drives.
/
/ Notes:
/
/ In:
/   uDrives = 1 bit per drive (bit 0 == A:, bit 25 == Z:) bit == 1 indicates drive
/              to be listed.
/   pszBiffer -> buffer to place text into
/   iBufferSize = size of the buffer.
/   pSepStr -> seperating string used to seperate drive names
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
static void build_drive_string( UINT uDrives, LPTSTR pBuffer, int iSize, LPTSTR pSepStr )
{
    TCHAR szDrive[] = TEXT("A:");
    int iMaxStrLen = lstrlen( szDrive ) + lstrlen( pSepStr );

    ASSERT( pBuffer != NULL );                      // sanitise the parameters
    ASSERT( iSize > 0 );

    *pBuffer = L'\0';

    while ( uDrives && ( iSize > iMaxStrLen ) )
    {
        if ( uDrives & 1 )
        {
            lstrcat( pBuffer, szDrive );

            if ( uDrives & ~1 )
                lstrcat( pBuffer, pSepStr );

            iSize -= iMaxStrLen;
        }

        szDrive[0]++;
        uDrives >>= 1;
    }
}

//=============================================================================
// Helper function to build the string of
//     "<Desktop Directory>;<My Documents Directory>"
//=============================================================================
void build_documentfolders_string(LPDOCFINDPROPSHEETPAGE pdfpsp, LPTSTR pszPath, BOOL bDesktopOnly)
{
    LPITEMIDLIST pidlDeskDir;
    TCHAR szTemp[MAX_PATH];
    
    *pszPath = TEXT('\0');

    // add the all user desktop directory...
    if (SHGetSpecialFolderLocation(pdfpsp->hwndDlg, CSIDL_COMMON_DESKTOPDIRECTORY, &pidlDeskDir) == S_OK)
    {
        SHGetPathFromIDList(pidlDeskDir, pszPath);
        ILFree(pidlDeskDir);
        pidlDeskDir = NULL;
    }
    
    // Add the desktop directory...
    if (SHGetSpecialFolderLocation(pdfpsp->hwndDlg, CSIDL_DESKTOPDIRECTORY, &pidlDeskDir) == S_OK)
    {
        SHGetPathFromIDList(pidlDeskDir, szTemp);
        if (szTemp[0] && lstrlen(szTemp)+lstrlen(pszPath) + 1 < ARRAYSIZE(szTemp))
        {
            lstrcat(pszPath,TEXT(";"));
            lstrcat(pszPath,szTemp);
        }
    }

    if (!bDesktopOnly)
    {
        LPITEMIDLIST pidlMyDocuments;
        
        // Check to see if the "My Documents" directory should be added or
        // whether it will implicitly be picked up because of disk hierarchy
        // and search options...
        if (SHGetSpecialFolderLocation(pdfpsp->hwndDlg, CSIDL_PERSONAL, &pidlMyDocuments) == S_OK)
        {
            BOOL fParent = ILIsParent(pidlDeskDir, pidlMyDocuments, FALSE);
    
            if (!fParent || (fParent && !pdfpsp->pdff->fTopLevelOnly))
            {
                SHGetPathFromIDList(pidlMyDocuments, szTemp);
    
                if (szTemp[0] && lstrlen(szTemp)+lstrlen(pszPath) + 1 < ARRAYSIZE(szTemp))
                {
                    lstrcat(pszPath,TEXT(";"));
                    lstrcat(pszPath,szTemp);
                }
            }
        }
    
        if (pidlMyDocuments)
            ILFree(pidlMyDocuments);
    }

    if (pidlDeskDir)
        ILFree(pidlDeskDir);
}

STDAPI_(BOOL) NextIDL(IEnumIDList *penum, LPITEMIDLIST *ppidl)
{
    UINT celt;

    *ppidl = NULL;

    return penum->lpVtbl->Next(penum, 1, ppidl, &celt) == NOERROR && celt == 1;
}


//==========================================================================
// Initialize the Name and loacation page
//==========================================================================
DWORD CALLBACK DocFind_RealDFNameLocInit(LPVOID lpThreadParameters)
{
    LPDOCFINDPROPSHEETPAGE pdfpsp = lpThreadParameters;
    CDFFilter *pdff = pdfpsp->pdff;    

    // We process the message after the WM_CREATE or WM_INITDLG as to
    // allow the dialog to come up quicker...

    TCHAR szPath[MAX_PATH];
    LPITEMIDLIST pidlWindows;
    IShellFolder * psf;
    HWND hwndCtl;
    HRESULT hres;
    LPITEMIDLIST pidlAbs, pidlTemp, pidlStart = pdff->pidlStart;
    int ipidlStart = -1;        // If pidl passed in and in our list already...
    int iLocalDrives = -1;
    HANDLE hEnum;

    UINT uFixedDrives = 0;      // 1 bit per 'fixed' drive, start at 0.
    int iMyComputer;
    int iDrive;
    IShellFolder *psfDesktop;
    DOCFINDSAVELOOKIN dfsli = {DFSLI_VER, DFSLI_TYPE_DOCUMENTFOLDERS};

    CoInitialize(0);

    SHGetDesktopFolder(&psfDesktop);

    hwndCtl = GetDlgItem(pdfpsp->hwndDlg, IDD_PATH);
    SendMessage(hwndCtl, CBEM_SETEXSTYLE, CBES_EX_PATHWORDBREAKPROC, 0L);

    // If we do not have a pidl Start, default to last place we searched.
    if (!pidlStart)
    {
        // May move this to helper function...
        // Stream format:
        //  DWORD:  Version=0
        //  DWORD:  Type: 0 - Pidl, 1 = Local Drives, 2 = Document Folder
        //      Pidl if Type = 0
        IStream *pstm = OpenRegStream(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, TEXT("DocFindLastLookIn"), STGM_READ);
        if (pstm)
        {
            int cb;
            if (SUCCEEDED(pstm->lpVtbl->Read(pstm, &dfsli, SIZEOF(dfsli), &cb))
                    && (cb == SIZEOF(dfsli)) && (dfsli.dwVer == DFSLI_VER))
            {
                if (dfsli.dwType == DFSLI_TYPE_PIDL)
                {
                    if (FAILED(ILLoadFromStream(pstm, &pidlStart)))
                    {
                        dfsli.dwType = DFSLI_TYPE_DOCUMENTFOLDERS;
                    }
                }

                else if ((dfsli.dwType == DFSLI_TYPE_STRING) && (pdff->szPath[0] == 0))
                {
                    WORD cbString;
                    if (SUCCEEDED(pstm->lpVtbl->Read(pstm, &cbString, SIZEOF(cbString), &cb))
                            && (cb == SIZEOF(cbString))
                            && (cb <= MAX_PATH))
                    {
                        if (FAILED(pstm->lpVtbl->Read(pstm, pdff->szPath, cbString, &cb))
                                || (cb != cbString))
                            pdff->szPath[0] = TEXT('\0');
                    }
                }
            }
            else
                dfsli.dwType = DFSLI_TYPE_DOCUMENTFOLDERS;
            pstm->lpVtbl->Release(pstm);
        }
    }

    LoadString(HINST_THISDLL, IDS_DOCUMENTFOLDERS, szPath, ARRAYSIZE(szPath));
    pdff->iDocumentFolders = DocFind_LocCBAddItem(hwndCtl, NULL, -1, 0, II_FOLDER, szPath);

    LoadString(HINST_THISDLL, IDS_DESKTOP, szPath, ARRAYSIZE(szPath));
    pdff->iDesktopFolders = DocFind_LocCBAddItem(hwndCtl, NULL, -1, 1, II_DESKTOP, szPath);

    pidlTemp = SHCloneSpecialIDList(NULL, CSIDL_PERSONAL, FALSE);
    if (pidlTemp)
    {
        IShellFolder *psf;
        LPITEMIDLIST pidlLast = ILClone(ILFindLastID(pidlTemp));

        ILRemoveLastID(pidlTemp);

        hres = psfDesktop->lpVtbl->BindToObject(psfDesktop, pidlTemp, NULL, &IID_IShellFolder, &psf);
        if (SUCCEEDED(hres))
        {
            DocFind_LocCBAddPidl(hwndCtl, psf, pidlTemp, pidlLast, NULL, FALSE, 1);
            psf->lpVtbl->Release(psf);
        }
        ILFree(pidlLast);
        ILFree(pidlTemp);
    }

    GetWindowsDirectory(szPath, ARRAYSIZE(szPath));
    if (szPath[1] == TEXT(':'))
        szPath[3] = TEXT('\0');

    pidlWindows = SHSimpleIDListFromPath(szPath);

    // We need to initialize the look in list with the list of
    // directories that are under "My Computer"
    if (pdff->psfMyComputer == NULL)
    {
        LPITEMIDLIST pidlMyComputer = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, FALSE);

        /* We need the index for my computer so that we can insert the all local storage
        /  item in after it. */

        pdff->iMyComputer = iMyComputer = DocFind_LocCBAddPidl(hwndCtl, psfDesktop, &c_idlDesktop,
                                           pidlMyComputer, &pidlAbs, FALSE, 0);

        if (pidlStart && ILIsEqual(pidlAbs, pidlStart))
        {
            if (dfsli.dwType == DFSLI_TYPE_PIDL) // If we loaded it
                ILFree(pidlStart);
            pidlStart = NULL;   // So I wont keep looking
            ipidlStart = iMyComputer;     // First item in the list!
        }

        if (SUCCEEDED(psfDesktop->lpVtbl->BindToObject(psfDesktop,
                pidlMyComputer, NULL, &IID_IShellFolder, &pdff->psfMyComputer)))
        {
            // We now need to iterate over the children under this guy...
            IEnumIDList *penum;
            psf = pdff->psfMyComputer;

            hres = psf->lpVtbl->EnumObjects(psf, pdfpsp->hwndDlg, SHCONTF_FOLDERS, &penum);
            if (SUCCEEDED(hres))
            {
                LPITEMIDLIST pidl;
                
                while (NextIDL(penum, &pidl))
                {
                    ULONG ulAttrs = SFGAO_FILESYSTEM;

                    // We only want to add this object if it is a
                    // file system level object, like drives.
                    //
                    psf->lpVtbl->GetAttributesOf(psf, 1, &pidl, &ulAttrs);

                    if (ulAttrs & SFGAO_FILESYSTEM)
                    {
                        int i = DocFind_LocCBAddPidl(hwndCtl, psf, pidlMyComputer,
                                pidl, &pidlAbs, FALSE, 1);
                        if (ILIsEqual(pidlAbs, pidlWindows))
                            pdff->iWindowsDrive = i;

                        if (pidlStart && ILIsEqual(pidlAbs, pidlStart))
                        {
                            if (dfsli.dwType == DFSLI_TYPE_PIDL) // If we loaded it
                                ILFree(pidlStart);
                            pidlStart = NULL;
                            ipidlStart = i;     // First item in the list!
                        }

                        /* Update the bit mask to reflect if this is a fixed drive, the flags do not define
                        /  this therefore we must get the drive number and the type of drive from that.  UNC
                        /  drives will have a drive number of -1, and therefore can be skipped in our checks. */

                        SHGetPathFromIDList( pidlAbs, szPath );

                        if ( ( iDrive = PathGetDriveNumber( szPath ) ) >= 0 )
                        {
                            // BUGBUG !!!
                            // This doesn't work for JAZ drive!
                            
                            if ( ( RealDriveTypeFlags( iDrive, FALSE ) & DRIVE_TYPE ) == DRIVE_FIXED )
                            {
                                ASSERT( ( uFixedDrives & ( 1 << iDrive ) ) == 0 );          // must be an empty drive slot
                                uFixedDrives |= 1 << iDrive;
                            }
                        }
                    }
                    ILFree(pidl);
                }
                penum->lpVtbl->Release(penum);
            }
        }
        ILFree(pidlMyComputer);

        /* If we found some fixed drives then attempt to add a new entry to the list box. This is placed below
        /  the 'My Computer' entry and maps to all the fixed drives.  As this kind of path (referencing multiple roots)
        /  cannot be expresed using a PIDL we set the PIDL to NULL, and store the bit array containing the drive
        /  mappings in the structure.
        /
        /  Because of this we must be careful to catch the PIDL being NULL, and composing the correct search path
        /  when we validate the page. */

        pdff->uFixedDrives = uFixedDrives;
        if ( uFixedDrives != 0 && (uFixedDrives & (uFixedDrives - 1)) != 0) // math magic to make sure that not only 1 bit is set
        {
            LPVOID pMessage;
            TCHAR szDrives[ MAX_PATH ];
            LPTSTR ppDrives[] = { szDrives, NULL };
            TCHAR szBuffer[ 256 ];

            build_drive_string( uFixedDrives, szDrives, ARRAYSIZE(szDrives), TEXT(",") );
            LoadString( HINST_THISDLL, IDS_FINDSEARCH_ALLDRIVES, szBuffer, ARRAYSIZE( szBuffer ) );
            FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                           szBuffer,                        // our template string to format from
                           0, 0,                            // id and locale (ignored)
                           (LPTSTR) &pMessage, 0,           // buffer and min size fo allocated
                           (va_list*) ppDrives );           // argument strings
            ASSERT( pMessage);

            if ( pMessage )
            {
                iLocalDrives = DocFind_LocCBAddItem(hwndCtl, NULL,
                                             iMyComputer + 1,   // After MyComputer
                                             1, II_DRIVEFIXED,
                                             pMessage);

                // Adjust special indices if needed for this insertion...
                if ( iLocalDrives != CB_ERRSPACE )
                {
                    if ( ipidlStart > iMyComputer )
                        ipidlStart++;

                    if ( pdff->iWindowsDrive > iMyComputer )
                        pdff->iWindowsDrive++;

                }

                LocalFree( (HGLOBAL) pMessage );
            }
        }

        // Also see if there are any UNC style connections that the
        // user might be interested in seeing in this list
        //
#ifdef WINNT
        if (WNetOpenEnum(RESOURCE_CONNECTED, RESOURCETYPE_DISK,
                RESOURCEUSAGE_CONTAINER | RESOURCEUSAGE_ATTACHED, NULL, &hEnum) == WN_SUCCESS)
#else
        if (WNetOpenEnum(RESOURCE_CONNECTED, RESOURCETYPE_DISK,
                RESOURCEUSAGE_CONTAINER, NULL, &hEnum) == WN_SUCCESS)
#endif
        {
            DWORD dwCount=1;
            union
            {
                NETRESOURCE nr;         // Large stack usage but I
                TCHAR    buf[1024];      // Dont think it is thunk to 16 bits...
            }nrb;

            DWORD   dwBufSize = SIZEOF(nrb);

            while (WNetEnumResource(hEnum, &dwCount, &nrb.buf,
                    &dwBufSize) == WN_SUCCESS)
            {
                // We only want to add items if they do not have a local
                // name.  If they had a local name we would have already
                // added them!
                if ((nrb.nr.lpRemoteName != NULL) &&
                    ((nrb.nr.lpLocalName == NULL) || (*nrb.nr.lpLocalName == TEXT('\0'))))
                {
                    LPITEMIDLIST pidl = ILCreateFromPath(nrb.nr.lpRemoteName);
                    if (pidl)
                    {
                        DocFind_LocCBAddPidl(hwndCtl, psfDesktop, &c_idlDesktop,
                                pidl, NULL, FALSE, 1);
                    }
                }
            }
            WNetCloseEnum(hEnum);
        }
    }

    if (pidlStart)
    {
        // We were passed in a pidl that is not already in our list so add it!
        LPITEMIDLIST pidlParent = ILClone(pidlStart);

        ILRemoveLastID(pidlParent);

        if (SUCCEEDED(psfDesktop->lpVtbl->BindToObject(psfDesktop,
                pidlParent, NULL, &IID_IShellFolder, &psf)))
        {
            SetWindowText(hwndCtl, pdff->szPath);
            ipidlStart = DocFind_LocCBAddPidl(hwndCtl, psf, pidlParent,
                    (LPITEMIDLIST)ILFindLastID(pidlStart), NULL, TRUE, 0);
            psf->lpVtbl->Release(psf);
        }
        ILFree(pidlParent);
    }


    // If we have starting text for path set it as the text for it now...
    if ((ipidlStart == -1) && (pdff->szPath[0] != TEXT('\0')))
        SetWindowText(hwndCtl, pdff->szPath);

    else
    {
        if (ipidlStart == -1)
        {
            switch (dfsli.dwType)
            {
            case DFSLI_TYPE_DESKTOP:
                ipidlStart = pdff->iDocumentFolders + 1;
                break;
            case DFSLI_TYPE_PERSONAL:
                ipidlStart = pdff->iDocumentFolders + 2;
                break;
            case DFSLI_TYPE_MYCOMPUTER:
                ipidlStart = iMyComputer;
                break;
            case DFSLI_TYPE_LOCALDRIVES:
                if (iLocalDrives >= 0)
                {
                    ipidlStart = iLocalDrives;
                    break;
                }
                // Fall through
            default:
                ipidlStart = pdff->iDocumentFolders;
            }
        }

        // Save this if we need to revert..
        pdff->ipidlStart = ipidlStart;

        // Finally select the right one in the list.
        SendMessage(hwndCtl, CB_SETCURSEL, ipidlStart, 0);
    }

    SendMessage(hwndCtl, CB_SETEDITSEL, 0, MAKELPARAM(0,-1));

    if (dfsli.dwType == DFSLI_TYPE_PIDL)
        ILFree(pidlStart);

    ILFree(pidlWindows);

    CoUninitialize();

    return 0;
}



void DocFind_DFNameLocInit(LPDOCFINDPROPSHEETPAGE pdfpsp)
{
    CDFFilter *pdff = pdfpsp->pdff;


    // We want to set the default search drive to the windows drive.
    // I am going to be a bit slimmy, but...
    //
    if ((pdfpsp->dwState & DFPAGE_INIT) == 0)
    {
        DWORD idThread;
        HIMAGELIST himl;

        Shell_GetImageLists(NULL, &himl);
        SendDlgItemMessage(pdfpsp->hwndDlg, IDD_PATH, CBEM_SETIMAGELIST, 0,
                (LPARAM)himl);

        CheckDlgButton(pdfpsp->hwndDlg, IDD_TOPLEVELONLY, !pdff->fTopLevelOnly);

        // If the filter contains search text we need to initialize the
        // string in the dialog
        SendDlgItemMessage(pdfpsp->hwndDlg, IDD_CONTAINS,
                EM_SETLIMITTEXT, MAXSTRLEN-1, 0);
        if (pdff->szText[0] != TEXT('0'))
            SetDlgItemText(pdfpsp->hwndDlg, IDD_CONTAINS, pdff->szText);
                                 
        // Do most of the init code offline in a second thread...
        pdfpsp->hThreadInit = CreateThread(NULL, 0,
                DocFind_RealDFNameLocInit, (LPVOID)pdfpsp, 0, &idThread);
        pdff->hMRUSpecs = DocFind_UpdateMRUItem(NULL, pdfpsp->hwndDlg, IDD_FILESPEC,
                s_szDocSpecMRU, pdff->szUserInputFileSpec, szNULL);

        // Update our state to let us know that we have already initialized...
        pdfpsp->dwState |= DFPAGE_INIT;

    }

    if (pdff->fTypeChanged)
    {
        // We need to see if the file named field has an extension
        // if it does, we will black it out as the user changed the
        // type field.
        if (StrChr(pdff->szUserInputFileSpec, TEXT('.')) != NULL)
            SetDlgItemText(pdfpsp->hwndDlg, IDD_FILESPEC, szNULL);
        pdff->fTypeChanged = FALSE;
    }


}

//==========================================================================
// Helper function to display a message box, set the focus to the the
// item with the problem, set the abort condition and return.
//==========================================================================
void DocFind_ReportItemValueError(HWND hwndDlg, int idCtl,
        int iMsg, LPTSTR pszValue)
{
    // We may pass through these pages at init time to make sure that
    // everything is initialize properly when we restore a search...
    if (!IsWindowVisible(hwndDlg))
        return;

    ShellMessageBox(HINST_THISDLL, hwndDlg,
        MAKEINTRESOURCE(iMsg),
        MAKEINTRESOURCE(IDS_FINDFILES), MB_OK|MB_ICONERROR, pszValue);
    SetFocus(GetDlgItem(hwndDlg, idCtl));
    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, TRUE);    // Tell it to abort
    return;

}

//==========================================================================
//
// DocFind_UpdateMRUItem - Initializes and Updates an MRU list item
//
HANDLE DocFind_UpdateMRUItem(HANDLE hMRU, HWND hwndDlg, int iDlgItem,
        LPCTSTR szSection, LPTSTR pszInitString, LPCTSTR pszAddIfEmpty)
{
    HANDLE hCB;
    int i, nMax;
    TCHAR szItem[MAX_PATH];
    BOOL fAllowEmptyItem = (pszAddIfEmpty == NULL) || (pszAddIfEmpty && (*pszAddIfEmpty == TEXT('\0')));

    hCB = GetDlgItem(hwndDlg, iDlgItem);

    if (hMRU == NULL) {
        MRUINFO mi = {
            SIZEOF(MRUINFO),
            10,
            0L,
            HKEY_CURRENT_USER,
            szSection,
            NULL
        };
        hMRU = CreateMRUList(&mi);
    }

    if (hMRU == NULL)
        return(NULL);


    SendMessage(hCB, CB_RESETCONTENT, 0, 0L);

    // Only Allow empty string if the AddifEmpty is set to empty string
    if (((pszInitString != NULL) && (*pszInitString != TEXT('\0'))) || fAllowEmptyItem)
        AddMRUString(hMRU, pszInitString);

    if (((nMax = EnumMRUList(hMRU, -1, NULL, 0)) == 0) && (!fAllowEmptyItem))
    {
        AddMRUString(hMRU, pszAddIfEmpty);
        nMax++;
    }

    for (i=0; i<nMax; ++i)
    {
        if ((EnumMRUList(hMRU, i, szItem, ARRAYSIZE(szItem)) > 0) ||
                fAllowEmptyItem)
        {
            /* The command to run goes in the combobox.
             */
            SendMessage(hCB, CB_ADDSTRING, 0, (LPARAM)(LPTSTR)szItem);

            if (szItem[0] == TEXT('\0'))
                fAllowEmptyItem = FALSE;        // only allow 1
        }
    }

    SendMessage(hCB, CB_SETCURSEL, 0, 0L);

    return(hMRU);

}


//==========================================================================
// Validate the page to make sure that the data is valid.  If it is not
// we need to display a message to the user and also set the focus to
// the invalid field.
//==========================================================================
void DocFind_DFNameGetPathOrPidl(LPDOCFINDPROPSHEETPAGE pdfpsp,
        LPTSTR pszPath, LPITEMIDLIST *ppidl)
{
    int iSel;
    LPITEMIDLIST pidlT;

    // if the pidl was previously set, clear it now.
    if (*ppidl)
        ILFree(*ppidl);

    // If the user types in a fully qualified name in the filespec
    // we will use the path part to override the look in field
    GetDlgItemText(pdfpsp->hwndDlg, IDD_FILESPEC, pszPath, MAX_PATH);
    if (!PathIsRelative(pszPath))
    {
        // First Update the file pattern
        if (PathIsRoot(pszPath) || PathIsDirectory(pszPath))
        {
            SetDlgItemText(pdfpsp->hwndDlg, IDD_FILESPEC, c_szNULL);
        }
        else
        {
            SetDlgItemText(pdfpsp->hwndDlg, IDD_FILESPEC,
                    PathFindFileName(pszPath));
            PathRemoveFileSpec(pszPath);    // remove the last part...
        }


        // Setup the text of the look in field
        SendDlgItemMessage(pdfpsp->hwndDlg, IDD_PATH, CB_SETCURSEL, (WPARAM)-1, 0);
        SetDlgItemText(pdfpsp->hwndDlg, IDD_PATH, pszPath);
    }

    iSel = (int)SendDlgItemMessage(pdfpsp->hwndDlg, IDD_PATH, CB_GETCURSEL, 0, 0);
    if (iSel != CB_ERR)
    {
        pidlT = (LPITEMIDLIST)SendDlgItemMessage(pdfpsp->hwndDlg, IDD_PATH,
                CB_GETITEMDATA, iSel, 0);

        /* If pidl == NULL then we special case and expand the fixed drive flags out to a path
        /  containing the drives that we are going to search. */

        if ( pidlT )
        {
            SHGetPathFromIDList(pidlT, pszPath);
            *ppidl = ILClone(pidlT);
        }
        else
        {
            if (iSel == pdfpsp->pdff->iDocumentFolders)
            {
                build_documentfolders_string(pdfpsp, pszPath, FALSE);
            }
            else if (iSel == pdfpsp->pdff->iDesktopFolders)
            {
                build_documentfolders_string(pdfpsp, pszPath, TRUE);
            }
            else
            {
                ASSERT( pdfpsp->pdff->uFixedDrives );
                build_drive_string( pdfpsp->pdff->uFixedDrives, pszPath, MAX_PATH, TEXT("\\;") );
            }
            *ppidl = NULL;
        }
    }
    else
    {
        LPTSTR pszT;
        GetDlgItemText(pdfpsp->hwndDlg, IDD_PATH, pszPath, MAX_PATH);

        // See if we have an Exact match in the combobox...
        // This handles the case the user types directly in a path...

        iSel = (int)SendDlgItemMessage(pdfpsp->hwndDlg, IDD_PATH,
                CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pszPath);
        if (iSel != CB_ERR)
        {
            pidlT = (LPITEMIDLIST)SendDlgItemMessage(pdfpsp->hwndDlg, IDD_PATH,
                    CB_GETITEMDATA, iSel, 0);

            /* If pidl == NULL then we special case and expand the fixed drive flags out to a path
            /  containing the drives that we are going to search. */

            if ( pidlT )
            {
                *ppidl = ILClone(pidlT);
            }
            else
            {
                if (iSel == pdfpsp->pdff->iDocumentFolders)
                {
                    build_documentfolders_string(pdfpsp, pszPath, FALSE);
                }
                else if (iSel == pdfpsp->pdff->iDesktopFolders)
                {
                    build_documentfolders_string(pdfpsp, pszPath, TRUE);
                }
                else
                {
                    ASSERT( pdfpsp->pdff->uFixedDrives );
                    build_drive_string( pdfpsp->pdff->uFixedDrives, pszPath, MAX_PATH, TEXT("\\;") );
                }
                *ppidl = NULL;
            }
        }
        else
        {
            // If we only have one path
            pszT = pszPath + StrCSpn(pszPath, TEXT(",;"));
            if (*pszT == TEXT('\0'))
            {
                // Try to parse the display name into a Pidl
                PathQualify(pszPath);
                *ppidl = ILCreateFromPath(pszPath);

                if (!*ppidl)
                {
                    TCHAR szDisplayName[MAX_PATH];
                    int cch;
                    // See if the beginning of the string matches the
                    // start of an item in the list.  If so we should
                    // Try to see if we can convert it to a valid
                    // pidl/string...

                    for (iSel=(int)SendDlgItemMessage(pdfpsp->hwndDlg, IDD_PATH,
                            CB_GETCOUNT, 0, 0); iSel >= 1; iSel--)
                    {
                        GetDlgItemText(pdfpsp->hwndDlg, IDD_PATH, pszPath, MAX_PATH);
                        SendDlgItemMessage(pdfpsp->hwndDlg, IDD_PATH, CB_GETLBTEXT,
                                (WPARAM)iSel, (LPARAM)szDisplayName);
                        cch = lstrlen(szDisplayName);
                        if ((CompareString(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                                szDisplayName, cch, pszPath, cch) == 2) &&
                                (pszPath[cch] == TEXT('\\')))
                        {
                            pidlT = (LPITEMIDLIST)SendDlgItemMessage(pdfpsp->hwndDlg, IDD_PATH,
                                    CB_GETITEMDATA, (WPARAM)iSel, 0);

                            // Don't blow up if we get a failure here

                            if (pidlT != (LPITEMIDLIST)-1)
                            {
                                SHGetPathFromIDList(pidlT, szDisplayName);
                                PathAppend(szDisplayName, CharNext(pszPath+cch));
                                *ppidl = ILCreateFromPath(szDisplayName);
                                if (*ppidl)
                                {
                                    lstrcpy(pszPath, szDisplayName);
                                    break;
                                }
                            }
#ifdef DEBUG
                            else
                            {
                                DebugMsg(DM_TRACE, TEXT("DocFind_DFNameGetPathOrPidl - CBItem ptr = -1"));
                            }
#endif
                        }
                    }
                }

            }
            else
            {
                // Multiple things let it run by itself...
                *ppidl = NULL;      // dont have a pidl to begin with...
            }
        }
    }
}

#define SFGAO_FS_SEARCH (SFGAO_FILESYSANCESTOR | SFGAO_FOLDER)

// we should not search into folders that don't have file system as ancestors

BOOL ShouldSearchItem(LPCITEMIDLIST pidl)
{
    DWORD dwFlags = SFGAO_FS_SEARCH;
    SHGetAttributesOf(pidl, &dwFlags);

    return (dwFlags & SFGAO_FS_SEARCH) == SFGAO_FS_SEARCH;
}



//==========================================================================
// Validate the page to make sure that the data is valid.  If it is not
// we need to display a message to the user and also set the focus to
// the invalid field.
//==========================================================================
void DocFind_DFNameLocValidatePage(LPDOCFINDPROPSHEETPAGE pdfpsp)
{
    CDFFilter *pdff = pdfpsp->pdff;
    HWND hwndDlg = pdfpsp->hwndDlg;
    TCHAR szTemp[MAX_PATH];
    LPTSTR pszT;
    LPITEMIDLIST pidl = NULL;   // init sinced used as in/out param

    // Make sure the look in field init code has completed..
    WaitForPageInitToComplete(pdfpsp);
    DocFind_DFNameGetPathOrPidl(pdfpsp, szTemp, &pidl);

    if (pidl)
    {
        BOOL bSouldSearch = ShouldSearchItem(pidl);
        ILFree(pidl);

        if (!bSouldSearch)
        {
            DocFind_ReportItemValueError(hwndDlg, IDD_PATH, IDS_FINDNOTFINDABLE, szTemp);
            return;
        }
    }
    else
    {
        if (szTemp[0] == 0)
        {
            DocFind_ReportItemValueError(hwndDlg, IDD_PATH, IDS_FINDDATAREQUIRED, szTemp);
            return;
        }

        // BUGBUG: this code does not validate the input for valid search paths
        // using ShouldSearchItem(). it can be fooled with c:\recycled;d:\

        // Try to make input like: "C:\;d:\" work.  Only validate the first one.
        pszT = szTemp + StrCSpn(szTemp, TEXT(",;"));
        *pszT = TEXT('\0');   // Null out the point...

        PathQualify(szTemp);
        if (!PathIsDirectory(szTemp))
        {
            DocFind_ReportItemValueError(hwndDlg, IDD_PATH, IDS_FINDWRONGPATH, szTemp);
            return;
        }
    }

    // Now Check the file specification.
    GetDlgItemText(pdfpsp->hwndDlg, IDD_FILESPEC, szTemp, ARRAYSIZE(szTemp));

    if (lstrcmp(szTemp, pdff->szUserInputFileSpec) != 0)
    {
        // Make sure that the file spec does not contain invalid characters
        pszT = szTemp + StrCSpn(szTemp, TEXT(":\\"));
        if (*pszT != 0)
        {
            DocFind_ReportItemValueError(hwndDlg, IDD_FILESPEC, IDS_FINDINVALIDFILENAME, szTemp);
            return;
        }

        // The file pattern changed.
        lstrcpy(pdff->szUserInputFileSpec, szTemp);

        // See if we should invalidate the type field
        if (StrChr(szTemp, TEXT('.')) != NULL)
        {
            pdff->fNameChanged = TRUE;
            pdff->szTypeFilePatterns[0] = TEXT('\0'); // null it out...
        }
    }
}


//==========================================================================
//
// Apply any changes that happened in the name loc page to the filter
//
void DocFind_DFNameLocApply(LPDOCFINDPROPSHEETPAGE pdfpsp)
{
    CDFFilter *pdff = pdfpsp->pdff;
    HWND hwndDlg = pdfpsp->hwndDlg;

    // Get the contain text
    GetDlgItemText(hwndDlg, IDD_CONTAINS, pdff->szText, ARRAYSIZE(pdff->szText) - 1);
    pdff->szText[lstrlen(pdff->szText)+1] = TEXT('\0'); // double \0

    // Get the path name and qualify it
    DocFind_DFNameGetPathOrPidl(pdfpsp, pdff->szPath, &pdff->pidlStart);


    // Also get the file specification.
    if (pdff->fTypeChanged)
    {
        // We need to see if the file named field has an extension
        // if it does, we will black it out as the user changed the
        // type field.
        if (StrChr(pdff->szUserInputFileSpec, TEXT('.')) != NULL)
            SetDlgItemText(pdfpsp->hwndDlg, IDD_FILESPEC, szNULL);
        pdff->fTypeChanged = FALSE;
    }

    GetDlgItemText(pdfpsp->hwndDlg, IDD_FILESPEC, pdff->szUserInputFileSpec,
            ARRAYSIZE(pdff->szUserInputFileSpec));
    DocFind_UpdateMRUItem(pdff->hMRUSpecs, pdfpsp->hwndDlg, IDD_FILESPEC,
            s_szDocSpecMRU, pdff->szUserInputFileSpec, NULL);

    pdff->fTopLevelOnly = !IsDlgButtonChecked(pdfpsp->hwndDlg, IDD_TOPLEVELONLY);
}



//==========================================================================
//
// DocFind_OnCommand - Process the WM_COMMAND messages
//
void DocFind_DFNameLocOnCommand(HWND hwndDlg, UINT id, HWND hwndCtl, UINT code)
{
    LPDOCFINDPROPSHEETPAGE pdfpsp = (LPDOCFINDPROPSHEETPAGE)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (id) {
    case IDD_BROWSE:
        if (code == BN_CLICKED)
        {
            LPITEMIDLIST pidlCurrent;
            TCHAR szDisplayName[MAX_PATH];
            BROWSEINFO bi = {
                hwndDlg,
                NULL,
                szDisplayName,
                MAKEINTRESOURCE(IDS_FINDSEARCHTITLE),
                BIF_RETURNONLYFSDIRS,
                NULL
            };

#ifdef DEBUG
            if (GetKeyState(VK_SHIFT) < 0)
                bi.ulFlags = BIF_BROWSEFORPRINTER;
#endif

            pidlCurrent = SHBrowseForFolder(&bi);

            if (pidlCurrent && ILIsEmpty(pidlCurrent))
            {
                // The user chose the desktop folder. Try converting
                // it over to the desktop folder directory...
                ILFree(pidlCurrent);
                pidlCurrent = SHCloneSpecialIDList(NULL, CSIDL_DESKTOPDIRECTORY, FALSE);
            }
            // Now convert into a path name to update the drop down list
            // with...
            if (pidlCurrent)
            {
                int i;
                HWND hwndCtl = GetDlgItem(hwndDlg, IDD_PATH);

                // Make sure the look in field init code has completed..
                WaitForPageInitToComplete(pdfpsp);
                i = DocFind_LocCBFindPidl(hwndCtl, pidlCurrent);
                if (i >= 0)
                {
                    SendMessage(hwndCtl, CB_SETCURSEL, i, 0);
                    ILFree(pidlCurrent);
                }
                else
                {
                    // Lets try getting the full display name for these items...
                    TCHAR szTemp[MAX_PATH];

                    if (SHGetNameAndFlags(pidlCurrent, SHGDN_FORPARSING, szTemp, ARRAYSIZE(szTemp), NULL))
                        lstrcpy(szDisplayName, szTemp);

                    // Add this item to our Drop Down list at the end
                    i = DocFind_LocCBAddItem(hwndCtl, pidlCurrent, -1, 0, bi.iImage, szTemp);

                    SendMessage(hwndCtl, CB_SETCURSEL, i, 0);
                }
            }
        }
        break;

    case IDD_PATH:
        // Make sure the look in field init code has completed..
        break;
    case IDD_FILESPEC:
        if ((code == CBN_SELCHANGE) || (code == CBN_EDITCHANGE))
            pdfpsp->pdff->fFilterChanged = TRUE;
        break;
    case IDD_CONTAINS:
        if (code == EN_CHANGE)
            pdfpsp->pdff->fFilterChanged = TRUE;
        break;
    }
}


void DocFind_DFNameLocEndEdit(LPDOCFINDPROPSHEETPAGE pdfpsp, LPNMCBEENDEDIT pnm)
{
    if (pnm->fChanged && (pnm->iNewSelection == CB_ERR))
    {
        pnm->iNewSelection = DFNameLocAddString(pdfpsp, pnm->szText);
    }
}


//==========================================================================
//
// This function is the dialog (or property sheet page) for the name and
// location page.
//
void Docfind_SaveLocInValue(LPDOCFINDPROPSHEETPAGE pdfpsp)
{
    int i;
    LPITEMIDLIST pidlT = NULL;
    DOCFINDSAVELOOKIN dfsli= {DFSLI_VER, DFSLI_TYPE_PIDL};
    IStream *pstm;
    TCHAR   szPath[MAX_PATH];
    WORD    cb;


    // We could also move this off as a helper function
    i = (int)SendDlgItemMessage(pdfpsp->hwndDlg, IDD_PATH, CB_GETCURSEL, 0, 0);
    if (i != CB_ERR)
    {
        /* If pidl == NULL then we special case and expand the fixed drive flags out to a path
        /  containing the drives that we are going to search. */
        pidlT = NULL;

        if (i == pdfpsp->pdff->iDocumentFolders)
            dfsli.dwType = DFSLI_TYPE_DOCUMENTFOLDERS;

        else if (i == (pdfpsp->pdff->iDocumentFolders + 1))
            dfsli.dwType = DFSLI_TYPE_DESKTOP;

        else if (i == (pdfpsp->pdff->iDocumentFolders + 2))
            dfsli.dwType = DFSLI_TYPE_PERSONAL;

        else if (i == pdfpsp->pdff->iMyComputer)
            dfsli.dwType = DFSLI_TYPE_MYCOMPUTER;

        else
        {
            pidlT = (LPITEMIDLIST)SendDlgItemMessage(pdfpsp->hwndDlg, IDD_PATH,
                    CB_GETITEMDATA, i, 0);
            if (!pidlT)
                dfsli.dwType = DFSLI_TYPE_LOCALDRIVES;
        }
    }
    else
    {
        // get the string
        dfsli.dwType = DFSLI_TYPE_STRING;
        GetDlgItemText(pdfpsp->hwndDlg, IDD_PATH, szPath, MAX_PATH);
        cb = (WORD)((lstrlen(szPath) + 1) * sizeof(TCHAR));
    }

    // Now try to get a stream to write to.
    pstm = OpenRegStream(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, TEXT("DocFindLastLookIn"), STGM_WRITE);
    if (pstm)
    {
        // Write out header.
        pstm->lpVtbl->Write(pstm, (LPTSTR)&dfsli, SIZEOF(dfsli), NULL);

        if (pidlT)
            ILSaveToStream(pstm, pidlT);

        if (dfsli.dwType == DFSLI_TYPE_STRING)
        {
            pstm->lpVtbl->Write(pstm, (LPTSTR)&cb, SIZEOF(cb), NULL);
            pstm->lpVtbl->Write(pstm, (LPTSTR)szPath, cb, NULL);
        }

        pstm->lpVtbl->Release(pstm);
    }
}

WORD DocFind_GetTodaysDosDateMinusNDays(int nDays)
{
    SYSTEMTIME st;
    union
    {
        FILETIME ft;
        LARGE_INTEGER li;
    }ftli;

    WORD FatTime;
    WORD FatDate;

    // Now we need to
    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ftli.ft);
    FileTimeToLocalFileTime(&ftli.ft, &ftli.ft);

    // Now decrement the file time by the count of days * the number of
    // 100NS time units per day.  Assume that nDays is positive.
    if (nDays > 0)
    {
        #define NANO_SECONDS_PER_DAY 864000000000
        ftli.li.QuadPart = ftli.li.QuadPart - ((__int64)nDays * NANO_SECONDS_PER_DAY);
    }

    FileTimeToDosDateTime(&ftli.ft, &FatDate,&FatTime);
    DebugMsg(DM_TRACE, TEXT("DocFind %d days = %x"), nDays, FatDate);
    return(FatDate);
}


WORD DocFind_GetTodaysDosDateMinusNMonths(int nMonths)
{

    SYSTEMTIME st;
    FILETIME ft;
    WORD FatTime;
    WORD FatDate;

    GetSystemTime(&st);
    st.wYear -= (WORD) nMonths / 12;
    nMonths = nMonths % 12;
    if (nMonths < st.wMonth)
        st.wMonth -= (WORD) nMonths;
    else
    {
        st.wYear--;
        st.wMonth = (WORD)(12 - (nMonths - st.wMonth));
    }

    // Now normalize back to a valid date.
    while (!SystemTimeToFileTime(&st, &ft))
    {
        st.wDay--;  // must not be valid date for month...
    }
    FileTimeToLocalFileTime(&ft, &ft);
    FileTimeToDosDateTime(&ft, &FatDate,&FatTime);
    DebugMsg(DM_TRACE, TEXT("DocFind %d months = %x"), nMonths, FatDate);
    return(FatDate);
}


VOID DocFind_DosDateToSystemtime(WORD wFatDate, LPSYSTEMTIME pst)
{
    ZeroMemory(pst, SIZEOF(*pst));
    pst->wDay = wFatDate & 0x001F;
    pst->wMonth = (wFatDate >> 5) & 0x000F;
    pst->wYear = 1980 + ((wFatDate >> 9) & 0x007F);
    DebugMsg(DM_TRACE, TEXT("DocFind %x -> %d/%d/%d"), wFatDate, pst->wDay, pst->wMonth, pst->wYear);
}


#ifdef DEBUG
WORD DocFind_SystemtimeToDosDate(LPSYSTEMTIME pst)
{
    WORD wFatDate = ((((pst)->wDay)&0x001F) | ((((pst)->wMonth)&0x000F) << 5) | (((((pst)->wYear)-1980)&0x007F) << 9));
    DebugMsg(DM_TRACE, TEXT("DocFind %d/%d/%d -> %x"), pst->wDay, pst->wMonth, pst->wYear, wFatDate);
    return wFatDate;
}
#else
#define DocFind_SystemtimeToDosDate(pst) ((((pst)->wDay)&0x001F) | ((((pst)->wMonth)&0x000F) << 5) | (((((pst)->wYear)-1980)&0x007F) << 9))
#endif

#define DIRBUF_CBGROW       ((MAX_PATH+1)*SIZEOF(TCHAR))


//===========================================================================
// CDFEnum : member prototype - Docfind Folder implementation
//===========================================================================
STDMETHODIMP CDFEnum_QueryInterface(IDFEnum * pdfenum, REFIID riid, LPVOID * ppvObj);
STDMETHODIMP_(ULONG) CDFEnum_AddRef(IDFEnum * pdfenum);
STDMETHODIMP_(ULONG) CDFEnum_Release(IDFEnum * pdfenum);
STDMETHODIMP CDFEnum_Next(IDFEnum * pdfenum, LPITEMIDLIST *ppidl,
               int *pcObjectSearched, int *pcFoldersSearched, volatile BOOL *pfContinue, int *pState, HWND hwnd);

STDMETHODIMP CDefDFEnum_Skip(IDFEnum * pdfenum, int celt);
STDMETHODIMP CDefDFEnum_Reset(IDFEnum * pdfenum);
STDMETHODIMP CDefDFEnum_StopSearch(IDFEnum * pdfenum);
STDMETHODIMP_(BOOL) CDFEnum_FQueryIsAsync(IDFEnum * pdfenum);
STDMETHODIMP CDefDFEnum_GetAsyncCount(IDFEnum * pdfenum, DBCOUNTITEM *pdwTotalAsync, int *pnPercentComplete, BOOL *pfQueryDone);
STDMETHODIMP CDefDFEnum_GetItemIDList(IDFEnum * pdfenum, UINT iItem, LPITEMIDLIST *ppidl);
STDMETHODIMP CDefDFEnum_GetExtendedDetailsOf(IDFEnum * pdfenum, LPCITEMIDLIST pidl, UINT iCol, LPSHELLDETAILS pdi);
STDMETHODIMP CDefDFEnum_GetItemID(IDFEnum * pdfenum, UINT iItem, DWORD *puWorkID);
STDMETHODIMP CDefDFEnum_SortOnColumn(IDFEnum * pdfenum, UINT iCOl, BOOL fAscending);

// These all call the Async one if we have one...
STDMETHODIMP CDFEnum_StopSearch(IDFEnum * pdfenum);
STDMETHODIMP_(BOOL) CDFEnum_FQueryIsAsync(IDFEnum * pdfenum);
STDMETHODIMP CDFEnum_GetAsyncCount(IDFEnum * pdfenum, DBCOUNTITEM *pdwTotalAsync, 
        int *pnPercentComplete, BOOL *pfQueryDone);
STDMETHODIMP CDFEnum_GetItemIDList(IDFEnum * pdfenum, UINT iItem, LPITEMIDLIST *ppidl);
STDMETHODIMP CDFEnum_GetExtendedDetailsOf(IDFEnum * pdfenum, LPCITEMIDLIST pidl, UINT iCol, LPSHELLDETAILS pdi);
STDMETHODIMP CDFEnum_GetExtendedDetailsULong(IDFEnum *pdfenum, LPCITEMIDLIST pidl, UINT iCol, ULONG *pul);
STDMETHODIMP CDFEnum_GetItemID(IDFEnum * pdfenum, UINT iItem, DWORD *puWorkID);
STDMETHODIMP CDFEnum_SortOnColumn(IDFEnum * pdfenum, UINT iCOl, BOOL fAscending);

//===========================================================================
// CDFEnum : Vtable
//===========================================================================

extern IDFEnumVtbl c_DFFIterVtbl;   // forward

IDFEnumVtbl c_DFFIterVtbl =
{
    CDFEnum_QueryInterface, CDFEnum_AddRef, CDFEnum_Release,
    CDFEnum_Next,
    CDefDFEnum_Skip,
    CDefDFEnum_Reset,
    CDFEnum_StopSearch,
    CDFEnum_FQueryIsAsync,
    CDFEnum_GetAsyncCount,
    CDFEnum_GetItemIDList,
    CDFEnum_GetExtendedDetailsOf,
    CDFEnum_GetExtendedDetailsULong,
    CDFEnum_GetItemID,
    CDFEnum_SortOnColumn
};

//==========================================================================
// CDFEnum - Helper Functions
//==========================================================================
DIRBUF * DFDirBuf_Pop(CDFEnum *this)
{
    DIRBUF * pdb = this->pdbStack;
    this->pdbStack = pdb->pdbNext;

    this->depth--;
#ifdef FIND_TRACE
        DebugMsg(DM_ERROR, TEXT("CDFEnum::Next: Pop (%d)%s"), this->depth, pdb->psz);
#endif

    if (!this->pdbReuse)
    {
        this->pdbReuse = pdb;
    }
    else
    {
        ASSERT(pdb->psz);
        LocalFree((HLOCAL)pdb->psz);

        LocalFree((HLOCAL)pdb);
    }
    return this->pdbStack;
}

STDMETHODIMP CDFEnum_QueryInterface(IDFEnum * pdfenum, REFIID riid, LPVOID * ppvObj)
{
    return E_NOTIMPL;
}

STDMETHODIMP_(ULONG) CDFEnum_AddRef(IDFEnum * pdfenum)
{
    CDFEnum *this = IToClass(CDFEnum, dfenum, pdfenum);
    TraceMsg(TF_DOCFIND, "CDFEnum.AddRef %d",this->cRef+1);
    return InterlockedIncrement(&this->cRef);
}
//==========================================================================
// CDFFilter_EnumObjects - Get The real recursive filtered enumerator...
//==========================================================================
STDMETHODIMP CDFFilter_EnumObjects(IDocFindFileFilter * pdfff, 
                                   IShellFolder *psf, LPITEMIDLIST pidlStart, DWORD grfFlags, int iColSort,
                                   LPTSTR pszProgressText, IRowsetWatchNotify *prwn, IDFEnum **ppdfenum)
{
    // We need to construct the iterator
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    CDFEnum *pdfenum;
    
    LPWSTR *apwszPaths;
    UINT cPaths;
    HRESULT hres;
    
    *ppdfenum = NULL;
    if (FAILED(hres  = DF_GetSearchPaths(this, pidlStart? pidlStart : this->pidlStart, &apwszPaths, &cPaths)))
        return hres;
    
#ifdef WINNT
    // content indexing query client is not supported on W95/Memphis yet.
    //    if (g_bRunOnNT5)
    {
        DWORD dwFlags;
        
        hres = DocFind_CreateOleDBEnum(pdfff, psf, apwszPaths, &cPaths, grfFlags, iColSort, pszProgressText, prwn, ppdfenum);
        if (hres == S_OK)
        {
            this->pdfenumAsync = *ppdfenum;
            this->pdfenumAsync->lpVtbl->AddRef(this->pdfenumAsync);

            // All paths processed, Simply need to return this enumerator... 
            if (cPaths == 0)
                goto EndFunc;   // Need to free the apwazPaths...
        }
        if (FAILED(hres) && (HRESULT_FACILITY(hres) == FACILITY_SEARCHCOMMAND))
            goto EndFunc;

        // user specified CI query cannot grep for that!! or all the drives are indexed
        if (SUCCEEDED(pdfff->lpVtbl->GenerateQueryRestrictions(pdfff, NULL, &dwFlags)) && (dwFlags & GQR_REQUIRES_CI))
        {
            if (SUCCEEDED(hres) && hres != S_OK)
                hres = E_FAIL;
            goto EndFunc;
        }
    }
#endif  
    
    pdfenum = LocalAlloc(LPTR, SIZEOF(CDFEnum));
    if (pdfenum == NULL)
    {
        DebugMsg(DM_WARNING, TEXT("Docfind E_OUTOFMEMORY: %s line %d"), __FILE__,  __LINE__);
        hres = E_OUTOFMEMORY;
        goto EndFunc;
    }
    
    // Now initialize the data structures.
    pdfenum->dfenum.lpVtbl = &c_DFFIterVtbl;
    pdfenum->cRef = 1;
    pdfenum->psf = psf;
    ASSERT(!pdfenum->fFindFirstSucceed);
    
    // See if we have any async processing..
    pdfenum->pdfenumAsync = *ppdfenum;
    
    if (pdfenum->pdfenumAsync)
        pdfenum->pdfenumAsync->lpVtbl->AddRef(pdfenum->pdfenumAsync);
    
    // Remember the top level paths we are supposed to process
    pdfenum->apwszPaths = apwszPaths;
    pdfenum->cPaths = cPaths;
    ASSERT(pdfenum->iPathNext == 0);
    
    
    pdfenum->pidlStart = ILClone(pidlStart? pidlStart : this->pidlStart);
    pdfenum->pszProgressText = pszProgressText;
    pdfenum->grfFlags = grfFlags;
    pdfenum->pdfff = pdfff;
    pdfenum->hfind = INVALID_HANDLE_VALUE;
    

    #if 0 // this doesn't seem necessary.  we don't know how to parse index query!!!
    // BUGBUG:: Not sure where to add the processing for fields that work differently when used
    // in CI search and non-ci search... Here for now
    // Need to rearrange to process merge
    // for now would rather have failed ! fail everything...
    if (this->pszIndexedSearch && (this->pszIndexedSearch[0] != TEXT('\0')))
    {
        // only do if not some other file spec specified...
        if (!(this->pszFileSpec && this->pszFileSpec[0]))
        {
            DocFind_SetupWildCardingOnFileSpec(this->pszIndexedSearch, &this->pszFileSpec);
        }
    }
    #endif
    
    
    // Save away the filter pointer
    pdfff->lpVtbl->AddRef(pdfff);
    
    // The rest of the fields should be zero/NULL
    *ppdfenum = &pdfenum->dfenum;       // Return the appropriate value;
    
    return NOERROR;
    
    
EndFunc:
    // This may have succeeded or aborted but we need to cleanup up the path array...
    while (cPaths)
    {
        cPaths--;
        LocalFree((HLOCAL)apwszPaths[cPaths]);
    }
    LocalFree((HLOCAL)apwszPaths);
    return hres;
}

STDMETHODIMP_(ULONG) CDFEnum_Release(IDFEnum * pdfenum)
{
    CDFEnum *this = IToClass(CDFEnum, dfenum, pdfenum);
    TraceMsg(TF_DOCFIND, "CDFEnum.Release %d",this->cRef-1);
    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    // If we still have an open File Handle close it now.
    if (this->hfind != INVALID_HANDLE_VALUE)
    {
        FindClose(this->hfind);
        this->hfind = INVALID_HANDLE_VALUE;
    }

    // Release any Directory buffers we may have queued up
    while (this->pdbStack)
        DFDirBuf_Pop(this);

    if (this->pdbReuse)
    {
        ASSERT(this->pdbReuse->psz);
        LocalFree((HLOCAL)this->pdbReuse->psz);

        LocalFree((HLOCAL)this->pdbReuse);
    }

    if (this->pdfff)
        this->pdfff->lpVtbl->Release(this->pdfff);

    // Release top level enum and shell folder
    if (this->psfStart)
        this->psfStart->lpVtbl->Release(this->psfStart);

    if (this->pdfenumAsync)
        this->pdfenumAsync->lpVtbl->Release(this->pdfenumAsync);


    // BUGBUG:: Remove the Release from the interfaces...
    if (this->apwszPaths)
    {
        int i;
        for (i = this->cPaths-1; i >= 0; i--)
            LocalFree((HLOCAL)this->apwszPaths[i]);
        LocalFree((HLOCAL)this->apwszPaths);
    }

    if (this->pidlStart)
        ILFree(this->pidlStart);

    LocalFree((HLOCAL)this);
    return(0);
}

BOOL DF_EnumNextTopPath(CDFEStartPaths * pdfesp, LPTSTR pszOut, int cch)
{
    ULONG ulAttrs;
    LPITEMIDLIST pidl;

    if (pdfesp->fFirstPass)
    {
        if (pdfesp->pidlStart)
        {
            ulAttrs = SFGAO_FILESYSTEM;

            SHGetAttributesOf(pdfesp->pidlStart, &ulAttrs);

            if ((ulAttrs & SFGAO_FILESYSTEM) == 0)
            {
                // this guy is not a file system item, but maybe his direct decendants are
                // for example a \\server

                if (SUCCEEDED(SHBindToObject(NULL, &IID_IShellFolder, pdfesp->pidlStart, &pdfesp->psfTopLevel)))
                {
                    if (SUCCEEDED(pdfesp->psfTopLevel->lpVtbl->EnumObjects(
                            pdfesp->psfTopLevel, NULL, SHCONTF_FOLDERS,
                            &pdfesp->penumTopLevel)))
                    {
                        // goodness
                    }
                    else
                    {
                        pdfesp->psfTopLevel->lpVtbl->Release(pdfesp->psfTopLevel);
                        pdfesp->psfTopLevel = NULL;   // dont release twice!
                        return FALSE;
                    }
                } 
                else
                {
                    return FALSE;      // bail
                }
            }
            else
            {
                // Normal one so no problem.
                SHGetPathFromIDList(pdfesp->pidlStart, pdfesp->pszPath);
                pdfesp->pszPathNext = pdfesp->pszPath;
            }
        }
        else
        {
            pdfesp->pszPathNext = pdfesp->pszPath;
        }

        pdfesp->fFirstPass = FALSE;
    }

    // See if we need to enumerate using the penum or simply look for the
    // next sub-string
    if (pdfesp->penumTopLevel)
    {
        // We need to get the next IDlist to search.

        while (NextIDL(pdfesp->penumTopLevel, &pidl))
        {
            ulAttrs = SFGAO_FS_SEARCH;
            if (SUCCEEDED(pdfesp->psfTopLevel->lpVtbl->GetAttributesOf(
                    pdfesp->psfTopLevel, 1, &pidl, &ulAttrs)))
            {
                if ((ulAttrs & SFGAO_FS_SEARCH) == SFGAO_FS_SEARCH)
                    break;  // found the next one to process
            }
        }

        if (pidl)
        {
            STRRET strret;  // Convert to a file name

            if (SUCCEEDED(pdfesp->psfTopLevel->lpVtbl->GetDisplayNameOf(pdfesp->psfTopLevel, pidl, SHGDN_FORPARSING, &strret)) &&
                SUCCEEDED(StrRetToBuf(&strret, pidl, pdfesp->pszPath, MAX_PATH)))
            {
                pdfesp->pszPathNext = pdfesp->pszPath;
            }
            ILFree(pidl);
        }
        else
            return FALSE;
    }

    // No more directory names: copy a path from
    // the filter path and start again...
    //
    pdfesp->pszPathNext = (LPTSTR)NextPath(pdfesp->pszPathNext, pszOut, cch);

    return pdfesp->pszPathNext != NULL;
}


//==========================================================================
// This function enumerates through the PIDL or the search list and finds all of the top
// level search places that we will either use the Content Index Or FindFirst/FindNext to enumerate...
//==========================================================================
HRESULT DF_GetSearchPaths(CDFFilter *this, LPCITEMIDLIST pidlStart, 
       LPWSTR *papwszPaths[], UINT *pcPaths)
{
#define MAX_NUM_SEARCH_PATHS 100
    // For now assume that there is a MAX number of paths we support...
    LPWSTR  apwszPaths[MAX_NUM_SEARCH_PATHS];
    TCHAR   szNextPath[MAX_PATH];
    TCHAR   szExpanded[MAX_PATH];
    LPWSTR  pwszT;
    int iPath = 0;
    CDFEStartPaths dfesp;

    // First Initialize the  insternal data we use
    dfesp.fFirstPass = TRUE;
    dfesp.pszPath = this->szPath;
    dfesp.pidlStart = pidlStart;
    dfesp.psfTopLevel = NULL;
    dfesp.penumTopLevel = NULL;
    dfesp.pszPathNext = NULL;

    // Now enumerate through all of the Items...
    while ((iPath < MAX_NUM_SEARCH_PATHS) && DF_EnumNextTopPath(&dfesp, szNextPath, MAX_PATH))
    {
        int cchT;
        
        // This left the next path in this->pszProgressText
        // Make sure no trailing backslash...
        PathRemoveBackslash(szNextPath);
        cchT = SHExpandEnvironmentStrings(szNextPath, szExpanded, ARRAYSIZE(szExpanded)); // count includes NULL terminator
        if (0==cchT)
            break;
        pwszT = apwszPaths[iPath] = LocalAlloc(LPTR, cchT*sizeof(WCHAR));
        if (!pwszT)
            break;  // Could not alloc mem... Argh...
        SHTCharToUnicode(szExpanded, pwszT, cchT);
        iPath++;
    }

    // Cleanup the data that we used for the above loop...
    if (dfesp.psfTopLevel)
        dfesp.psfTopLevel->lpVtbl->Release(dfesp.psfTopLevel);
    if (dfesp.penumTopLevel)
        dfesp.penumTopLevel->lpVtbl->Release(dfesp.penumTopLevel);

    // Now build the data to return back to the caller
    *pcPaths = iPath;
    if (iPath) {
        *papwszPaths = (LPWSTR*)LocalAlloc(LPTR, iPath * sizeof(LPWSTR));
        if (!*papwszPaths) 
        {
            while (iPath > 0)
            {
                iPath--;
                LocalFree((HLOCAL)apwszPaths[iPath]);
            }
            return E_OUTOFMEMORY;
        }

        // normally simply fall through and copy memory and return...
        CopyMemory(*papwszPaths, apwszPaths, iPath * sizeof(LPWSTR));
    }
    return S_OK;
}


//===========================================================================
// _CDFENUM_ShouldWePushFile - Helper function, that given the fact that
// a file is a directory, is this one we should search???
//

BOOL _CDFEnum_ShouldWePushFile(CDFEnum *this)
{
    // Make sure that it is a directory and that we are doing a recursive
    // search.

    if ((this->grfFlags & DFOO_INCLUDESUBDIRS) == 0)
        return FALSE;

    if ((this->finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
        return FALSE;

    // If the directory is marked hidden and we are not doing show all
    // files then also return FALSE
    if ( ((this->grfFlags & DFOO_SHOWALLOBJECTS) == 0) &&
         ((this->finddata.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) != 0))
        return FALSE;

    if (IsSuperHidden(this->finddata.dwFileAttributes))
        return FALSE;
        
    // If it is a system directory we need to do more work
    // BUGBUG - Make work for briefcase...

    if (this->finddata.dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_READONLY))
    {
        TCHAR szPath[MAX_PATH];
        LPITEMIDLIST pidl;
        BOOL bRet = FALSE;

        PathCombine(szPath, this->pszProgressText, this->finddata.cFileName);

        // See if this is the InternetCache.
        if (!this->szTempInternetCachePath[0]) // Assume can use straight strcmp compare...
            SHGetSpecialFolderPath(NULL, this->szTempInternetCachePath, CSIDL_INTERNET_CACHE, FALSE);

        if (StrCmpI(szPath, this->szTempInternetCachePath) == 0)
            return FALSE;   // don't walk in here... 

        pidl = ILCreateFromPath(szPath);
        if (pidl)
        {
            bRet = ShouldSearchItem(pidl);
            ILFree(pidl);
        }
        return bRet;
    }
    return TRUE;    // This is one to be pushed!
}

//===========================================================================
// CDFEnum::Next Recursive Iterator that is very special to the docfind.
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
STDMETHODIMP CDFEnum_Next(IDFEnum * pdfenum, LPITEMIDLIST *ppidl,
               int *pcObjectSearched, int *pcFoldersSearched,
               volatile BOOL *pfContinue, int *piState, HWND hwnd)
{
    // If we aren't enumerating a directory, then get the next directory
    // name from the dir list, and begin enumerating its contents...
    //
    CDFEnum * this = IToClass(CDFEnum, dfenum, pdfenum);
    BOOL fContinue = TRUE;
    HRESULT hres;

    // Default value in case *pfContinue becomes FALSE
    *piState = GNF_NOMATCH;
    *ppidl   = NULL;

    do
    {
        if (this->hfind != INVALID_HANDLE_VALUE)
        {
            if (!FindNextFile(this->hfind, &this->finddata))
            {
                // No more files: close the hfind...
                //
                FindClose(this->hfind);
                this->hfind = INVALID_HANDLE_VALUE;
            }
        }

        if (this->hfind == INVALID_HANDLE_VALUE)
        {

            // If this is our first time through, prime the enumeration
            // by copying the first path from the filter path...
            //
            // First time through should fall out here.

            this->fObjReturnedInDir = FALSE;    // Nothing returned here yet
            do
            {
                DIRBUF * pdb;
                int ichPathEnd;

                if (!*pfContinue)
                {
                    *piState = GNF_DONE;
                    return NOERROR;
                }

                // If at end of current DIRBUF, then pop it
                // off, repeating until we find a DIRBUF that
                // has directory names left to enumerate.
                //
                pdb = this->pdbStack;
                while (pdb && pdb->psz[pdb->ichDirNext] == 0)
                {
                    pdb = DFDirBuf_Pop(this);
                }

                // If we added some subdirectories while
                // enumerating the current directory,
                // then we are now descending another level...
                //
                if (this->fAddedSubDirs)
                {
                    this->fAddedSubDirs = FALSE;
                    this->depth++;
                }

                // If there are still more directory names, append it
                // to the path and start from there.  Otherwise,
                // get the next path from the filter path and start over.
                //
                if (pdb)
                {
                    // Lop off the old directory, and add the current
                    // directory to it...
                    //
                    this->pszProgressText[pdb->ichPathEnd] = 0;
                    this->fAddedSubDirs = FALSE;

                    if (PathCombine(this->pszProgressText,
                            this->pszProgressText,
                            &pdb->psz[pdb->ichDirNext]) == NULL)
                    {
                        this->hfind = INVALID_HANDLE_VALUE;
                        pdb->psz[pdb->ichDirNext] = 0;
                        continue;
                    }

                    // Advance ichDirNext
                    //
                    pdb->ichDirNext += lstrlen(&pdb->psz[pdb->ichDirNext]) + 1;

                }
                else
                {
                    if (this->iPathNext < this->cPaths)
                    {
                        SHUnicodeToTChar(this->apwszPaths[this->iPathNext], this->pszProgressText, MAX_PATH);
                        // the path may be relative so qualify it now.
                        PathQualify(this->pszProgressText);
                        this->iPathNext++;
                    }
                    else
                    {
                        *piState = GNF_DONE;
                        return NOERROR;
                    }
                }

                // append "\*.*" to end of path for FindFirstFile...

                ichPathEnd = lstrlen(this->pszProgressText);

                if (ichPathEnd >= (MAX_PATH - 5))
                    this->hfind = INVALID_HANDLE_VALUE;
                else
                {
                    PathAppend(this->pszProgressText, c_szStarDotStar);

                    // If we get here we will be going to a new directory...
#ifdef FIND_TRACE
                    DebugMsg(DM_ERROR, TEXT("CDFEnum::Next: Find First (%d)%s"), this->depth, this->pszProgressText);
#endif
                    (*pcFoldersSearched)++;   // Increment the number of folders searched
                    
                    // BUGBUG: We should supply a punkEnableModless in order to go modal during UI.
                    SHFindFirstFileRetry(hwnd, NULL, this->pszProgressText, &this->finddata, &this->hfind, SHPPFW_NONE);

                    // Remove the "\*.*"...
                    this->pszProgressText[ichPathEnd] = 0;
                }

                // Loop until we find a directory that contains files...
            }
            while (this->hfind == INVALID_HANDLE_VALUE);
            this->fFindFirstSucceed = TRUE;
        }

        // We've got a WIN32_FIND_DATA to return.
        //

        // Always skip "." and ".."...
        //
        if (PathIsDotOrDotDot(this->finddata.cFileName))
            continue;        // Try the next entry


        (*pcObjectSearched)++;


        // If we are enumerating subdirectories, add directories
        // to the DIRBUF...  But dont add any items that have the
        // system bit set as these are probably junction points.
        //
        if (_CDFEnum_ShouldWePushFile(this))
        {
            // in search.c this was: DirBuf_Add(this, this->finddata.cFileName);
            DIRBUF * pdbT;
            int cb;

            if (!this->fAddedSubDirs)
            {
                this->fAddedSubDirs = TRUE;

                pdbT = this->pdbReuse;
                if (pdbT)
                {
                    this->pdbReuse = NULL;
                    pdbT->cb = 0;   // Set size of data to 0 as to not retry first item
                }
                else
                {
                    pdbT = (DIRBUF *)LocalAlloc(LPTR, SIZEOF(DIRBUF));
                    if (pdbT)
                    {
                        pdbT->cbAlloc = DIRBUF_CBGROW;
                        pdbT->psz = LocalAlloc(LPTR, pdbT->cbAlloc);
                        if (!pdbT->psz)
                        {
                            LocalFree((HLOCAL)pdbT);
                            pdbT = NULL;
                            DebugMsg(DM_TRACE, TEXT("DocFind: LocalAlloc Failed"));
                            ASSERT(FALSE);
                        }
                    }
                    else
                    {
                        DebugMsg(DM_TRACE, TEXT("DocFind: LocalAlloc Failed"));
                        ASSERT(FALSE);
                    }
                }
                // Add to our stack...
                //
                if (pdbT)
                {
                    pdbT->pdbNext = this->pdbStack;
                    this->pdbStack = pdbT;

                    pdbT->ichDirNext = 0;

                    pdbT->ichPathEnd = lstrlen(this->pszProgressText);
                }
            }
            else
                pdbT = this->pdbStack;

            if (pdbT)
            {

                cb = (lstrlen(this->finddata.cFileName) + 1) * SIZEOF(TCHAR);
                if (pdbT->cb + cb + SIZEOF(TCHAR) > pdbT->cbAlloc)
                {
                    LPTSTR pszNew;

                    pdbT->cbAlloc += DIRBUF_CBGROW;

                    pszNew = LocalReAlloc((HLOCAL)pdbT->psz, pdbT->cbAlloc,
                            LMEM_MOVEABLE|LMEM_ZEROINIT);
                    if (!pszNew)
                    {
                        TraceMsg(TF_WARNING, "DocFind: LocalReAlloc Failed");
                        return (E_OUTOFMEMORY);
                    }
                    pdbT->psz = pszNew;
                }
                lstrcpy(pdbT->psz + (pdbT->cb / SIZEOF(TCHAR)), this->finddata.cFileName);

                // Add an extra zero terminator to mark end of list...
                pdbT->psz[(pdbT->cb + cb) / SIZEOF(TCHAR)] = TEXT('\0');
                pdbT->cb += cb;
            }
#ifdef FIND_TRACE
        DebugMsg(DM_ERROR, TEXT("CDFEnum::Next: Push (%d)%s"), this->depth, this->finddata.cFileName);
#endif
        }

        // Here is where we call of to our filter to see if we want the
        // file, but for now we will assume that we do
        //
        fContinue = FALSE;  // We can exit the loop;
        if (this->pdfff->lpVtbl->FDoesItemMatchFilter(this->pdfff,
                this->pszProgressText, &this->finddata, NULL, NULL) != 0)
            *piState = GNF_MATCH;
        else
            *piState = GNF_NOMATCH;

        // Generate the PIDL to return;
        if (*piState == GNF_MATCH)
        {
            LPITEMIDLIST pidl;
            IDocFindFolder *pdfFolder;
            DFFolderListItem *pdffli;

            hres = this->psf->lpVtbl->QueryInterface(this->psf, &IID_IDocFindFolder, (void **)&pdfFolder);
            // if we cannot get docfind folder we cannot continue
            if (FAILED(hres))
            {
                DebugMsg(DM_TRACE, TEXT("DocFind: AddFolderToFolderList Failed"));
                ASSERT(FALSE);
                return hres;
            }
            // See if we Need to Add a directory entry for this item
            if (!this->fObjReturnedInDir)
            {
                this->fObjReturnedInDir = TRUE;

                // Now Create A new File Cabinet Entry.
                pidl = NULL;    // Setup for failure case.
                if (this->psfStart)
                {
                    // We have a starting IDLIST and IShellFolder to use, to parse
                    // the name with.  This is important to use in cases like searching
                    // over the network with UNC names

                    // First properly handle the root one...
                    if (this->cchStart >= lstrlen(this->pszProgressText))
                    {
                        // Root one simply clone the root
                        pidl = ILClone(this->pidlStart);
                    }
                    else
                    {
                        LPITEMIDLIST pidlPartial = NULL;
                        ULONG pcchEaten;
                        WCHAR wszName[MAX_PATH];

                        SHTCharToUnicode(this->pszProgressText + this->cchStart, wszName, ARRAYSIZE(wszName));

                        // Remember to skip over the characters that are common with our
                        // psfStart
                        this->psfStart->lpVtbl->ParseDisplayName(this->psfStart,
                                    NULL, NULL, wszName, &pcchEaten, &pidlPartial, NULL);

                        if (pidlPartial)
                        {
                            pidl = ILCombine(this->pidlStart, pidlPartial);
                            ILFree(pidlPartial);
                        }
                    }
                }
                // try to convert from full path...
                if (pidl == NULL)
                {
                    hres = SHILCreateFromPath(this->pszProgressText, &pidl, NULL);

                    if (FAILED(hres))
                    {
                        DebugMsg(DM_TRACE, TEXT("DocFind: Create Pidl for Folder list Failed"));
                        ASSERT(FALSE);
                        pdfFolder->lpVtbl->Release(pdfFolder);
                        return hres;
                    }
                }

                hres = pdfFolder->lpVtbl->AddFolderToFolderList(pdfFolder, pidl, TRUE, &this->iFolder);  
                ILFree(pidl);
                if (FAILED(hres))
                {
                    DebugMsg(DM_TRACE, TEXT("DocFind: AddFolderToFolderList Failed"));
                    ASSERT(FALSE);
                    pdfFolder->lpVtbl->Release(pdfFolder);
                    return hres;
                }
            }

            hres = pdfFolder->lpVtbl->GetFolderListItem(pdfFolder, this->iFolder, &pdffli);
            if (SUCCEEDED(hres))
            {
                IShellFolder *psfParent;

                ASSERT(pdffli);
                
                psfParent = DocFind_GetObjectsIFolder(pdfFolder, pdffli, NULL);
                if (psfParent)
                {
                    hres = DocFind_CreateIDList(psfParent, &this->finddata, this->iFolder, DFDF_NONE, 0, 0, 0, ppidl);
                    psfParent->lpVtbl->Release(psfParent);
                }
            }
            pdfFolder->lpVtbl->Release(pdfFolder);
            
            if (FAILED(hres))
            {
                *piState = GNF_ERROR;
                DebugMsg(DM_WARNING, TEXT("Docfind error # %d: %s line %d"), hres, __FILE__,  __LINE__);
                return (hres);
            }
        }

    } while (fContinue && *pfContinue);

    return NOERROR;
}

LPITEMIDLIST DocFind_AppendDocFindData(LPITEMIDLIST pidl, int iFolder, WORD wFlags, UINT uRow, DWORD dwItemID, ULONG ulRank)
{
    HIDDENDOCFINDDATA hdfd = {SIZEOF(hdfd), IDLHID_DOCFINDDATA, (WORD) iFolder, wFlags, uRow, dwItemID, ulRank};     

    return ILAppendHiddenID(pidl, (PIDHIDDEN)&hdfd);
}

HRESULT DocFind_CreateIDList(IShellFolder *psf, WIN32_FIND_DATA *pfd, int iFolder, WORD wFlags, UINT uRow, DWORD dwItemID, ULONG ulRank, LPITEMIDLIST *ppidl)
{
    HRESULT hr;

    ASSERT(psf && pfd && ppidl);

    *ppidl = NULL;
    // we do a full parse on folders so we can get the correct icon in case of a special folder (e.g. briefcase)
    if (pfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        WCHAR wszPath[MAX_PATH];

        SHTCharToUnicode(pfd->cFileName, wszPath, ARRAYSIZE(wszPath));
        hr = psf->lpVtbl->ParseDisplayName(psf, NULL, NULL, wszPath, NULL, ppidl, NULL);
    }
    else
    {
        hr = SHSimpleIDListFromFindData2(psf, pfd, ppidl);
    }

    if (SUCCEEDED(hr))
    {
        *ppidl = DocFind_AppendDocFindData(*ppidl, iFolder, wFlags, uRow, dwItemID, ulRank);
        if (!*ppidl)
            hr = E_OUTOFMEMORY;
    }
    
    return hr;
}

//===========================================================================
// CDFEnum::StopSearch
//===========================================================================
STDMETHODIMP CDFEnum_StopSearch(IDFEnum * pdfenum)
{
    CDFEnum * this = IToClass(CDFEnum, dfenum, pdfenum);
    if (this->pdfenumAsync)
        return this->pdfenumAsync->lpVtbl->StopSearch(this->pdfenumAsync);

    return (E_NOTIMPL);
}


//===========================================================================
// CDFEnum::GetAsyncCount
//===========================================================================
STDMETHODIMP_(BOOL) CDFEnum_FQueryIsAsync(IDFEnum * pdfenum)
{
    CDFEnum * this = IToClass(CDFEnum, dfenum, pdfenum);
    if (this->pdfenumAsync)
        return DF_QUERYISMIXED;    // non-zero special number to say both...
    return FALSE;
}

//===========================================================================
// CDFEnum::GetAsyncCount
//===========================================================================
STDMETHODIMP CDFEnum_GetAsyncCount(IDFEnum * pdfenum, DBCOUNTITEM *pdwTotalAsync, 
        int *pnPercentComplete, BOOL *pfQueryDone)
{
    CDFEnum * this = IToClass(CDFEnum, dfenum, pdfenum);
    if (this->pdfenumAsync)
        return this->pdfenumAsync->lpVtbl->GetAsyncCount(this->pdfenumAsync, 
                pdwTotalAsync, pnPercentComplete, pfQueryDone);

    *pdwTotalAsync = 0;
    return (E_NOTIMPL);
}

//===========================================================================
// CDFEnum::GetItemIDList
//===========================================================================
STDMETHODIMP CDFEnum_GetItemIDList(IDFEnum * pdfenum, UINT iItem, LPITEMIDLIST *ppidl)
{
    CDFEnum * this = IToClass(CDFEnum, dfenum, pdfenum);
    if (this->pdfenumAsync)
        return this->pdfenumAsync->lpVtbl->GetItemIDList(this->pdfenumAsync, 
                iItem, ppidl);

    *ppidl = NULL;
    return (E_NOTIMPL);
}

//===========================================================================
// CDFEnum::GetExtendedDetailsOf
//===========================================================================
STDMETHODIMP CDFEnum_GetExtendedDetailsOf(IDFEnum * pdfenum, LPCITEMIDLIST pidl, UINT iCol, LPSHELLDETAILS pdi)
{
    CDFEnum * this = IToClass(CDFEnum, dfenum, pdfenum);
    if (this->pdfenumAsync)
        return this->pdfenumAsync->lpVtbl->GetExtendedDetailsOf(this->pdfenumAsync, 
                pidl, iCol, pdi);

    return (E_NOTIMPL);
}


//===========================================================================
// CDFEnum::GetExtendedDetailsULong
//===========================================================================
STDMETHODIMP CDFEnum_GetExtendedDetailsULong(IDFEnum *pdfenum, LPCITEMIDLIST pidl, UINT iCol, ULONG *pul)
{
    CDFEnum * this = IToClass(CDFEnum, dfenum, pdfenum);
    if (this->pdfenumAsync)
        return this->pdfenumAsync->lpVtbl->GetExtendedDetailsULong(this->pdfenumAsync, 
                pidl, iCol, pul);
    *pul = 0;
    return (E_NOTIMPL);
}

//===========================================================================
// CDFEnum::GetItemID
//===========================================================================
STDMETHODIMP CDFEnum_GetItemID(IDFEnum * pdfenum, UINT iItem, DWORD *puWorkID)
{
    CDFEnum * this = IToClass(CDFEnum, dfenum, pdfenum);
    if (this->pdfenumAsync)
        return this->pdfenumAsync->lpVtbl->GetItemID(this->pdfenumAsync, 
                iItem, puWorkID);

    *puWorkID = (UINT)-1;
    return (E_NOTIMPL);
}

//===========================================================================
// CDFEnum::SortOnColumn
//===========================================================================
STDMETHODIMP CDFEnum_SortOnColumn(IDFEnum * pdfenum, UINT iCol, BOOL fAscending)
{
    CDFEnum * this = IToClass(CDFEnum, dfenum, pdfenum);
    if (this->pdfenumAsync)
        return this->pdfenumAsync->lpVtbl->SortOnColumn(this->pdfenumAsync, 
                iCol, fAscending);

    return E_NOTIMPL;
}

/*----------------------------------------------------------------------------
/ CDFFilter_DeclareFSNotifyInterest implementation
/ ---------------------------------
/ Purpose:
/   Registering our interest in FS change notifications.
/
/ Notes:
/   -
/
/ In:
/   pdfff -> description of the find filter
/   hwndDlg = window handle of the find dialog
/   uMsg = message to be sent to window when informing of notify
/
/ Out:
/   -
/----------------------------------------------------------------------------*/
STDMETHODIMP CDFFilter_DeclareFSNotifyInterest(IDocFindFileFilter *pdfff, HWND hwndDlg, UINT uMsg )
{
    CDFFilter *this = IToClass(CDFFilter, dfff, pdfff);
    SHChangeNotifyEntry fsne;
    TCHAR szPath[ MAX_PATH ];
    LPTSTR pszPath;

    fsne.fRecursive = TRUE;

    /* If we have a IDL list then don't bother breaking the path up */
    if ( this ->pidlStart )
    {
        fsne.pidl = this ->pidlStart;
        SHChangeNotifyRegister(hwndDlg, SHCNRF_NewDelivery | SHCNRF_ShellLevel | SHCNRF_InterruptLevel, SHCNE_DISKEVENTS, uMsg, 1, &fsne );
    }
    else
    {
        pszPath = this->szPath;

        /* While we still have elements in the path */
        while ( NULL != ( pszPath = (LPTSTR) NextPath( pszPath, szPath, MAX_PATH ) ) )
        {
            PathAddBackslash( szPath );

            /* Create a PIDL and declare that it is interested in events on that window */
            fsne.pidl = ILCreateFromPath( szPath );

            if ( NULL != fsne.pidl )
            {
                SHChangeNotifyRegister(hwndDlg, SHCNRF_NewDelivery | SHCNRF_ShellLevel, SHCNE_DISKEVENTS, uMsg, 1, &fsne );
                ILFree((LPITEMIDLIST)fsne.pidl);
            }
        }
    }

    return NOERROR;
}


void CDFFilter_UpdateTypeField(CDFFilter *pdff, LPTSTR pszValue)
{
    // Assume if the first one is wildcarded than all are,...
    if (lstrcmp(pszValue, TEXT(".")) == 0)
    {
        // Special searching for folders...
        pdff->fFoldersOnly = TRUE;
        StrNCpy(pdff->szTypeFilePatterns, pszValue, MAX_PATH);
        return;
    }

    if (*pszValue == TEXT('*'))
        StrNCpy(pdff->szTypeFilePatterns, pszValue, MAX_PATH);
    else
    {
        TCHAR szNextPattern[MAX_PATH];  // overkill in size
        BOOL fFirst = TRUE;
        LPCTSTR pszNextPattern = pszValue;
        while ((pszNextPattern = NextPath(pszNextPattern, szNextPattern, ARRAYSIZE(szNextPattern))) != NULL)
        {
            if (!fFirst)
                StrCatBuff(pdff->szTypeFilePatterns, TEXT(";"), MAX_PATH);
            fFirst = FALSE;
            if (szNextPattern[0] != TEXT('*'))
                StrCatBuff(pdff->szTypeFilePatterns, TEXT("*"), MAX_PATH);
            StrCatBuff(pdff->szTypeFilePatterns, szNextPattern, MAX_PATH);
        }
    }
}


// Do the conversion of types, share code with netfind.
int CDFFilter_UpdateFieldChangeType(BSTR szField, VARIANT vValue, const CDFFUF *pcdffuf, VARIANT *pvConvertedValue, 
                                    LPTSTR *ppszValue)
{
    int             iField;

    *ppszValue = NULL;
    VariantInit(pvConvertedValue);

    for (iField = 0; pcdffuf[iField].pwszField; iField++)
    {
        if (StrCmpIW(szField, pcdffuf[iField].pwszField)==0)
        {
            if (SUCCEEDED(VariantChangeType(pvConvertedValue, &vValue, 0, pcdffuf[iField].vt))) 
            {
                if (pcdffuf[iField].vt == VT_BSTR)
                {
                    DWORD cchSize = (lstrlenW(pvConvertedValue->bstrVal) + 1);
                    *ppszValue = (LPTSTR) LocalAlloc(LPTR, (cchSize * sizeof(TCHAR)));
#if UNICODE 
                    StrCpyNW(*ppszValue, pvConvertedValue->bstrVal, cchSize);
#else
                    if (!SHUnicodeToAnsi(pvConvertedValue->bstrVal, *ppszValue, cchSize)) 
                        
                    {
                        TraceMsg(TF_DOCFIND, "CDFilter_UpdateField SHUnicodeToAnsi failed! (gle=%d)\n", GetLastError());
                        break;
                    }
#endif
                }
            }
            else
                break;  // return FAILURE
            return iField;
        }
    }

    return -1;
}




HRESULT CDFFilter_UpdateField(IDocFindFileFilter *pdfff, BSTR szField, VARIANT vValue)
{
    CDFFilter      *pdff = IToClass(CDFFilter, dfff, pdfff);
    VARIANT         vConvertedValue;
    int             iField;
    USHORT          uDosTime;
    LPTSTR          pszValue = NULL;



    pdff->fFilterChanged = TRUE;    // force rebuilding name of files...
    if ((iField = CDFFilter_UpdateFieldChangeType(szField, vValue, s_cdffuf, &vConvertedValue, &pszValue)) < 0)
        return E_FAIL;
    switch (iField)
    {
    case CDFFUFE_IndexedSearch:
        Str_SetPtr(&(pdff->pszIndexedSearch), pszValue);
        break;

    case CDFFUFE_LookIn:
        StrNCpy(pdff->szPath, pszValue, MAX_PATH);
        pdff->pidlStart = ILCreateFromPath(pszValue); 
        break;

    case CDFFUFE_IncludeSubFolders:
        pdff->fTopLevelOnly = !vConvertedValue.boolVal;            
        break;

    case CDFFUFE_Named:
        pdff->fNameChanged = TRUE;
        StrNCpy(pdff->szUserInputFileSpec, pszValue, MAX_PATH);
        break;

    case CDFFUFE_ContainingText:
        StrNCpy(pdff->szText, pszValue, MAXSTRLEN-1);   // Grep code pukes on larger value...
        break;

    case CDFFUFE_FileType:
        CDFFilter_UpdateTypeField(pdff, pszValue);
        break;

    case CDFFUFE_WhichDate:
        pdff->wDateType &= DFF_DATE_TYPEMASK;
        switch (vConvertedValue.iVal)
        {
        case 0:
            pdff->dateModifiedBefore = 0;
            pdff->dateModifiedAfter = 0;
            break;
        case 1:
            pdff->wDateType |= DFF_DATE_MODIFIED;
            break;
        case 2:
            pdff->wDateType |= DFF_DATE_CREATED;
            break;
        case 3:
            pdff->wDateType |= DFF_DATE_ACCESSED;
            break;
        }
        break;

    case CDFFUFE_DateLE:
        pdff->wDateType |= DFF_DATE_BETWEEN;
        VariantTimeToDosDateTime(vConvertedValue.date, &pdff->dateModifiedBefore, &uDosTime); 
        if (pdff->dateModifiedAfter != 0 && pdff->dateModifiedBefore != 0)
        {
            if (pdff->dateModifiedAfter > pdff->dateModifiedBefore)
            {
                WORD wTemp = pdff->dateModifiedAfter;
                pdff->dateModifiedAfter = pdff->dateModifiedBefore;
                pdff->dateModifiedBefore = wTemp;
            }
        }
        break;

    case CDFFUFE_DateGE:
        pdff->wDateType |= DFF_DATE_BETWEEN;
        VariantTimeToDosDateTime(vConvertedValue.date, &pdff->dateModifiedAfter, &uDosTime); 
        if (pdff->dateModifiedAfter != 0 && pdff->dateModifiedBefore != 0)
        {
            if (pdff->dateModifiedAfter > pdff->dateModifiedBefore)
            {
                WORD wTemp = pdff->dateModifiedAfter;
                pdff->dateModifiedAfter = pdff->dateModifiedBefore;
                pdff->dateModifiedBefore = wTemp;
            }
        }
        break;

    case CDFFUFE_DateNDays:
        pdff->wDateType |= DFF_DATE_DAYS;
        pdff->wDateValue = vConvertedValue.iVal;
        pdff->dateModifiedAfter = DocFind_GetTodaysDosDateMinusNDays(vConvertedValue.iVal);
        break;

    case CDFFUFE_DateNMonths:
        pdff->wDateType |= DFF_DATE_MONTHS;
        pdff->wDateValue = vConvertedValue.iVal;
        pdff->dateModifiedAfter = DocFind_GetTodaysDosDateMinusNMonths(vConvertedValue.iVal);
        break;

    case CDFFUFE_SizeLE:
        pdff->iSizeType = 2;
        pdff->dwSize = vConvertedValue.ulVal;
        break;

    case CDFFUFE_SizeGE:
        pdff->iSizeType = 1;
        pdff->dwSize = vConvertedValue.ulVal;
        break;

    case CDFFUFE_TextCaseSen:
        pdff->fTextCaseSen = vConvertedValue.boolVal;            
        break;

    case CDFFUFE_TextReg:
        pdff->fTextReg = vConvertedValue.boolVal;            
        break;

    case CDFFUFE_SearchSlowFiles:
        pdff->fSearchSlowFiles = vConvertedValue.boolVal;            
        break;

    case CDFFUFE_QueryDialect:
        pdff->ulQueryDialect = vConvertedValue.ulVal;
        break;

    case CDFFUFE_WarningFlags:
        pdff->dwWarningFlags = vConvertedValue.ulVal;
    }

    VariantClear(&vConvertedValue);                            
    if (pszValue)
        LocalFree(pszValue);

    return S_OK;
}


HRESULT CDFFilter_ResetFieldsToDefaults(IDocFindFileFilter *pdfff)
{
    CDFFilter *     pdff = IToClass(CDFFilter, dfff, pdfff);

    // Try to reset everything that our UpdateFields may touch to make sure next search gets all
    pdff->szPath[0] = TEXT('\0');
    if (pdff->pidlStart)
    {    
        ILFree(pdff->pidlStart);
        pdff->pidlStart = NULL;
    }
    pdff->fTopLevelOnly = FALSE;
    pdff->szUserInputFileSpec[0] = TEXT('\0');
    pdff->szText[0] = TEXT('\0');
    if (pdff->pszIndexedSearch)
        *pdff->pszIndexedSearch = TEXT('\0');
    pdff->szTypeFilePatterns[0] = TEXT('\0'); 

    pdff->fFoldersOnly = FALSE;
    pdff->wDateType = 0;
    pdff->dateModifiedBefore = 0;
    pdff->dateModifiedAfter = 0;
    pdff->iSizeType = 0;
    pdff->dwSize = 0;
    pdff->fTextCaseSen = FALSE;
    pdff->fTextReg = FALSE;
    pdff->fSearchSlowFiles = FALSE;
    pdff->ulQueryDialect = ISQLANG_V2;
    pdff->dwWarningFlags = DFW_DEFAULT;
    return S_OK;
}


HRESULT CDFFilter_GetNextConstraint(IDocFindFileFilter *pdfff, VARIANT_BOOL fReset, 
        BSTR *pName, VARIANT *pValue, VARIANT_BOOL *pfFound)
{
    CDFFilter *     pdff = IToClass(CDFFilter, dfff, pdfff);
    BOOL            fFound = FALSE;

    *pName = NULL;
    VariantClear(pValue);                            
    *pfFound = FALSE;

    if (fReset)
        pdff->iNextConstraint = 0;

    // we don't go to array size as the last entry is an empty item...
    while (pdff->iNextConstraint < (ARRAYSIZE(s_cdffuf)-1))
    {
        switch (s_cdffuf[pdff->iNextConstraint].cdffufe)
        {
        case CDFFUFE_IndexedSearch:
            fFound = InitVariantFromStr(pValue, pdff->pszIndexedSearch) == S_OK;
            break;
    
        case CDFFUFE_LookIn:
            fFound = InitVariantFromStr(pValue, pdff->szPath) == S_OK;
            break;
    
        case CDFFUFE_IncludeSubFolders:
            pValue->vt = VT_I4;
            pValue->lVal = pdff->fTopLevelOnly? 0 : 1;
            fFound = TRUE;
            break;
    
        case CDFFUFE_Named:
            fFound = InitVariantFromStr(pValue, pdff->szUserInputFileSpec) == S_OK;
            break;
    
        case CDFFUFE_ContainingText:
            fFound = InitVariantFromStr(pValue, pdff->szText) == S_OK;
            break;
    
        case CDFFUFE_FileType:
            fFound = InitVariantFromStr(pValue, pdff->szTypeFilePatterns) == S_OK;
            break;

        case CDFFUFE_WhichDate:
            switch (pdff->wDateType & DFF_DATE_TYPEMASK)
            {
            case DFF_DATE_MODIFIED:
                pValue->lVal = 1;
                break;
            case DFF_DATE_CREATED:
                pValue->lVal = 2;
                break;
            case DFF_DATE_ACCESSED:
                pValue->lVal = 3;
                break;
            }
            if (pValue->lVal)
            {
                pValue->vt = VT_I4;
                fFound = TRUE;
            }
            break;

        case CDFFUFE_DateLE:
            if ((pdff->wDateType & DFF_DATE_RANGEMASK) == DFF_DATE_BETWEEN)
            {
                DosDateTimeToVariantTime(pdff->dateModifiedBefore, 0, &pValue->date); 
                pValue->vt = VT_DATE;
                fFound = TRUE;
            }

            break;

        case CDFFUFE_DateGE:
            if ((pdff->wDateType & DFF_DATE_RANGEMASK) == DFF_DATE_BETWEEN)
            {
                DosDateTimeToVariantTime(pdff->dateModifiedAfter, 0, &pValue->date); 
                pValue->vt = VT_DATE;
                fFound = TRUE;
            }
            break;

        case CDFFUFE_DateNDays:
            if ((pdff->wDateType & DFF_DATE_RANGEMASK) == DFF_DATE_DAYS)
            {
                pValue->vt = VT_I4;
                pValue->lVal = pdff->wDateValue;
                fFound = TRUE;
            }
            break;

        case CDFFUFE_DateNMonths:
            if ((pdff->wDateType & DFF_DATE_RANGEMASK) == DFF_DATE_MONTHS)
            {
                pValue->vt = VT_I4;
                pValue->lVal = pdff->wDateValue;
                fFound = TRUE;
            }
            break;

        case CDFFUFE_SizeLE:
            if (pdff->iSizeType == 2)
            {
                pValue->vt = VT_UI4;
                pValue->ulVal = pdff->dwSize;
                fFound = TRUE;
            }
            break;

        case CDFFUFE_SizeGE:
            if (pdff->iSizeType == 1)
            {
                pValue->vt = VT_UI4;
                pValue->ulVal = pdff->dwSize;
                fFound = TRUE;
            }
            break;

        case CDFFUFE_TextCaseSen:
            pValue->vt = VT_I4;
            pValue->boolVal = pdff->fTextCaseSen ? 1 : 0;
            fFound = TRUE;
            break;

        case CDFFUFE_TextReg:
            pValue->vt = VT_I4;
            pValue->boolVal = pdff->fTextReg ? 1 : 0;
            fFound = TRUE;
            break;

        case CDFFUFE_SearchSlowFiles:
            pValue->vt = VT_I4;
            pValue->boolVal = pdff->fSearchSlowFiles ? 1 : 0;
            fFound = TRUE;
            break;

        case CDFFUFE_QueryDialect:
            pValue->vt = VT_UI4;
            pValue->ulVal = pdff->ulQueryDialect;
            fFound = TRUE;
            break;

        case CDFFUFE_WarningFlags:
            pValue->vt = VT_UI4;
            pValue->ulVal = pdff->dwWarningFlags;
            fFound = TRUE;
            break;
        }

        if (fFound)
            break;
        pdff->iNextConstraint += 1;
    }

    if (fFound)
    {
        *pName = SysAllocString(s_cdffuf[pdff->iNextConstraint].pwszField);
        pdff->iNextConstraint += 1; // for the next one...
        if (!*pName)
        {
            VariantClear(pValue);                            
            return E_OUTOFMEMORY;
        }
        *pfFound = TRUE;
    }
    return S_OK;    // no error let script use the found field...
}


IDocFindFileFilterVtbl c_DFFilterVtbl =
{
    CDFFilter_QueryInterface, CDFFilter_AddRef, CDFFilter_Release,
    CDFFilter_GetIconsAndMenu,
    CDFFilter_GetStatusMessageIndex,
    CDFFilter_GetFolderMergeMenuIndex,
    CDFFilter_FFilterChanged,
    CDFFilter_GenerateTitle,
    CDFFilter_PrepareToEnumObjects,
    CDFFilter_ClearSearchCriteria,
    CDFFilter_EnumObjects,
    CDFFilter_GetDetailsOf,
    CDFFilter_FDoesItemMatchFilter,
    CDFFilter_SaveCriteria,
    CDFFilter_RestoreCriteria,
    CDFFilter_DeclareFSNotifyInterest,
    CDFFilter_GetColSaveStream,
    CDFFilter_GenerateQueryRestrictions,
    CDFFilter_ReleaseQuery,
    CDFFilter_UpdateField,
    CDFFilter_ResetFieldsToDefaults,
    CDFFilter_GetItemContextMenu,
    CDFFilter_GetDefaultSearchGUID,
    CDFFilter_EnumSearches,
    CDFFilter_GetSearchFolderClassId,
    CDFFilter_GetNextConstraint,
    CDFFilter_GetQueryLanguageDialect,
    CDFFilter_GetWarningFlags,
};
