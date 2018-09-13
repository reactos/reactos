#include "shellprv.h"
#pragma hdrstop

#include "bitbuck.h"
#include "fstreex.h"
#include "util.h"
#include "copy.h"
#include "mrsw.h"
#include "prop.h" // for COL_DATA

#include "datautil.h"

STDAPI_(LPITEMIDLIST) ILResize(LPITEMIDLIST pidl, UINT cbRequired, UINT cbExtra);

// mulprsht.c
void SetDateTimeText(HWND hdlg, int id, const FILETIME *pftUTC);

// defext.c
STDMETHODIMP CCommonShellPropSheetExt_ReplacePage(IShellPropSheetExt * pspx, UINT uPageID, LPFNADDPROPSHEETPAGE lpfnReplaceWith, LPARAM lParam);

// fstreex.c
int  FS_CreateMoveCopyList(IDataObject *pdtobj, void *hNameMappings, LPITEMIDLIST **pppidl);
void FS_PositionItems(HWND hwndOwner, UINT cidl, const LPITEMIDLIST *ppidl, IDataObject *pdtobj, POINT *pptOrigin, BOOL fMove);
void FS_FreeMoveCopyList(LPITEMIDLIST *ppidl, UINT cidl);
HRESULT CFSIDLData_QueryGetData(IDataObject * pdtobj, FORMATETC * pformatetc); // subclass member function to support CF_HDROP and CF_NETRESOURCE


//
// Prototypes
//
void BBGetDisplayName(LPCIDFOLDER pidf, LPTSTR pszPath);
void BBGetItemPath(LPCIDFOLDER pidf, LPTSTR pszPath);
STDMETHODIMP CBitBucket_PS_AddPages(IShellPropSheetExt * pspx, LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
LPITEMIDLIST DeletedFilePathToBBPidl(LPTSTR lpszPath);

//
// structs
//
typedef struct _bbpropsheetinfo
{
    PROPSHEETPAGE psp;

    int idDrive;

    BOOL fNukeOnDelete;
    BOOL fOriginalNukeOnDelete;

    int iPercent;
    int iOriginalPercent;

    // the following two fields are valid only for the "global" tab, where they represent the state
    // of the "Configure drives independently" / "Use one setting for all drives" checkbox
    BOOL fUseGlobalSettings;
    BOOL fOriginalUseGlobalSettings;

    // this is a pointer to the global property sheet page after it has been copied somewhere by the 
    // CreatePropertySheetPage(), we use this to get to the global state of the % slider and fNukeOnDelete
    // from the other tabs
    struct _bbpropsheetinfo* pGlobal;

} BBPROPSHEETINFO, *LPBBPROPSHEETINFO;


const static DWORD aBitBucketPropHelpIDs[] = {  // Context Help IDs
    IDD_ATTR_GROUPBOX,  IDH_COMM_GROUPBOX,
    IDC_INDEPENDENT,    IDH_RECYCLE_CONFIG_INDEP,
    IDC_GLOBAL,         IDH_RECYCLE_CONFIG_ALL,
    IDC_DISKSIZE,       IDH_RECYCLE_DRIVE_SIZE,
    IDC_DISKSIZEDATA,   IDH_RECYCLE_DRIVE_SIZE,
    IDC_BYTESIZE,       IDH_RECYCLE_BIN_SIZE,
    IDC_BYTESIZEDATA,   IDH_RECYCLE_BIN_SIZE,
    IDC_NUKEONDELETE,   IDH_RECYCLE_PURGE_ON_DEL,
    IDC_BBSIZE,         IDH_RECYCLE_MAX_SIZE,
    IDC_BBSIZETEXT,     IDH_RECYCLE_MAX_SIZE,
    IDC_CONFIRMDELETE,  IDH_DELETE_CONFIRM_DLG,
    IDC_TEXT,           NO_HELP,
    0, 0
};

const static DWORD aBitBucketHelpIDs[] = {  // Context Help IDs
    IDD_LINE_1,        NO_HELP,
    IDD_LINE_2,        NO_HELP,
    IDD_ITEMICON,      IDH_FPROP_GEN_ICON,
    IDD_NAME,          IDH_FPROP_GEN_NAME,
    IDD_FILETYPE_TXT,  IDH_FPROP_GEN_TYPE,
    IDD_FILETYPE,      IDH_FPROP_GEN_TYPE,
    IDD_FILESIZE_TXT,  IDH_FPROP_GEN_SIZE,
    IDD_FILESIZE,      IDH_FPROP_GEN_SIZE,
    IDD_LOCATION_TXT,  IDH_FCAB_DELFILEPROP_LOCATION,
    IDD_LOCATION,      IDH_FCAB_DELFILEPROP_LOCATION,
    IDD_DELETED_TXT,   IDH_FCAB_DELFILEPROP_DELETED,
    IDD_DELETED,       IDH_FCAB_DELFILEPROP_DELETED,
    IDD_CREATED_TXT,   IDH_FPROP_GEN_DATE_CREATED,
    IDD_CREATED,       IDH_FPROP_GEN_DATE_CREATED,
    IDD_READONLY,      IDH_FCAB_DELFILEPROP_READONLY,
    IDD_HIDDEN,        IDH_FCAB_DELFILEPROP_HIDDEN,
    IDD_ARCHIVE,       IDH_FCAB_DELFILEPROP_ARCHIVE,
    IDD_ATTR_GROUPBOX, IDH_COMM_GROUPBOX,
    0, 0
};

//
// for the pidls
//
enum
{
    ICOL_NAME = 0,
    ICOL_ORIGINAL,
    ICOL_MODIFIED,
    ICOL_TYPE,
    ICOL_SIZE,
};

const COL_DATA c_bb_cols[] = {
    {ICOL_NAME,     IDS_NAME_COL,           20, LVCFMT_LEFT,    &SCID_NAME},
    {ICOL_ORIGINAL, IDS_DELETEDFROM_COL,    20, LVCFMT_LEFT,    &SCID_ORIGINALLOCATION},
    {ICOL_MODIFIED, IDS_DATEDELETED_COL,    18, LVCFMT_LEFT,    &SCID_DATEDELETED},
    {ICOL_TYPE,     IDS_TYPE_COL,           20, LVCFMT_LEFT,    &SCID_TYPE},
    {ICOL_SIZE,     IDS_SIZE_COL,           8,  LVCFMT_RIGHT,   &SCID_SIZE}
};

//========================================================================
// CBitBucket members
//========================================================================
STDMETHODIMP CBitBucket_SF_QueryInterface(IShellFolder2 *psf, REFIID riid, void **ppvObj)
{
    CBitBucket *this = IToClass(CBitBucket, isf, psf);


    if (IsEqualIID(riid, &IID_IShellFolder)  ||
        IsEqualIID(riid, &IID_IShellFolder2) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObj = &this->isf;
    }
    else if (IsEqualIID(riid, &IID_IPersistFolder) ||
             IsEqualIID(riid, &IID_IPersistFolder2) ||
             IsEqualIID(riid, &IID_IPersist))
    {
        *ppvObj = &this->ipf;
    }
    else if (IsEqualIID(riid, &IID_IContextMenu) )
    {
        *ppvObj = &this->icm;
    }
    else if (IsEqualIID(riid, &IID_IShellPropSheetExt))
    {
        *ppvObj = &this->ips;
    }
    else if (IsEqualIID(riid, &IID_IShellExtInit))
    {
        *ppvObj = &this->isei;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    this->cRef++;
    return NOERROR;
}


STDMETHODIMP_(ULONG) CBitBucket_SF_Release(IShellFolder2 *psf)
{
    CBitBucket *this = IToClass(CBitBucket, isf, psf);
    if (InterlockedDecrement(&this->cRef))
        return this->cRef;

    ILFree(this->pidl);
    LocalFree((HLOCAL)this);
    return 0;
}


STDMETHODIMP_(ULONG) CBitBucket_SF_AddRef(IShellFolder2 *psf)
{
    CBitBucket *this = IToClass(CBitBucket, isf, psf);
    return InterlockedIncrement(&this->cRef);
}


BOOL BBGetOriginalPath(LPBBDATAENTRYIDA pbbidl, TCHAR *pszOrig, UINT cch)
{
#ifdef UNICODE
    if (pbbidl->cb == SIZEOF(BBDATAENTRYIDW)) 
    {
        LPBBDATAENTRYIDW pbbidlw = DATAENTRYIDATOW(pbbidl);
        ualstrcpyn(pszOrig, pbbidlw->wszOriginal, cch);
        return (BOOL)*pszOrig;
    }
#endif
    SHAnsiToTChar(pbbidl->bbde.szOriginal, pszOrig, cch);
    return (BOOL)*pszOrig;
}


//
// We need to be able to compare the names of two bbpidls.  Since either of
// them could be a unicode name, we might have to convert both to unicode.
//
int _BBCompareOriginal(LPBBDATAENTRYIDA lpbbidl1, LPBBDATAENTRYIDA lpbbidl2 )
{
    TCHAR szOrig1[MAX_PATH];
    TCHAR szOrig2[MAX_PATH];
    BBGetOriginalPath(lpbbidl1, szOrig1, ARRAYSIZE(szOrig1));
    BBGetOriginalPath(lpbbidl2, szOrig2, ARRAYSIZE(szOrig2));

    PathRemoveFileSpec(szOrig1);
    PathRemoveFileSpec(szOrig2);
    return lstrcmpi(szOrig1,szOrig2);
}


STDMETHODIMP CBitBucket_SF_CompareIDs(IShellFolder2 *psf, LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hres = ResultFromShort(-1);
    LPBBDATAENTRYIDA pbbidl1, pbbidl2;
    LPCIDFOLDER pidf1 = FS_IsValidID(pidl1);
    LPCIDFOLDER pidf2 = FS_IsValidID(pidl2);
    short nCmp;

    if (!pidf1 || !pidf2)
    {
        ASSERT(0);
        return E_INVALIDARG;
    }
    
    pbbidl1 = PIDLTODATAENTRYID(pidl1);
    pbbidl2 = PIDLTODATAENTRYID(pidl2);

    // don't worry about recursing down because we don't have children.
    switch (lParam) 
    {
        case ICOL_TYPE:
            nCmp = _CompareFileTypes((IShellFolder *)psf, pidf1, pidf2);
            if (nCmp)
            {
                return ResultFromShort(nCmp);
            }
            else
            {
                goto CompareNames;
            }
            break;

        case ICOL_SIZE:
        {
            ULONGLONG qw1, qw2;

            // BUGBUG (reinerf) - directories are not dealt with properly
            // when sorting by size.

            FS_GetSize(NULL, pidf1, &qw1);
            FS_GetSize(NULL, pidf2, &qw2);

            if (qw1 < qw2)
            {
                return ResultFromShort(-1);
            }
            if (qw1 > qw2)
            {
                return ResultFromShort(1);
            }
        } // else fall through

CompareNames:
        case ICOL_NAME:
            hres = FS_CompareNamesCase(pidf1, pidf2);
            // compare the real filenames first, if they are different,
            // try comparing the display name
            if ((hres != ResultFromShort(0)) && (BB_IsRealID(pidl1) && BB_IsRealID(pidl2)))
            {
                HRESULT hresOld = hres;
                TCHAR szName1[MAX_PATH], szName2[MAX_PATH];

                BBGetDisplayName(pidf1, szName1);
                BBGetDisplayName(pidf2, szName2);

                hres = ResultFromShort(lstrcmpi(szName1, szName2));
                // if they are from the same location, sort them by delete times
                if (hres == ResultFromShort(0))
                {
                    // if the items are same in title, sort by drive
                    hres = ResultFromShort(pbbidl1->bbde.idDrive - pbbidl2->bbde.idDrive);

                    if (hres == ResultFromShort(0))
                    {
                        // if the items are still the same, sort by index
                        hres = ResultFromShort(pbbidl1->bbde.iIndex - pbbidl2->bbde.iIndex);

                        // once we're not equal, we can never be equal again
                        ASSERT(hres != ResultFromShort(0));
                        if (hres == ResultFromShort(0))
                        {
                            hres = hresOld;
                        }
                    }
                }
            }
            break;

        case ICOL_ORIGINAL:
            hres = ResultFromShort(_BBCompareOriginal(pbbidl1, pbbidl2));
            break;

        case ICOL_MODIFIED:
            if (pbbidl1->bbde.ft.dwHighDateTime < pbbidl2->bbde.ft.dwHighDateTime) 
            {
                hres = ResultFromShort(-1);
            }
            else if (pbbidl1->bbde.ft.dwHighDateTime > pbbidl2->bbde.ft.dwHighDateTime)
            {
                hres = ResultFromShort(1);
            }
            else
            {
                if (pbbidl1->bbde.ft.dwLowDateTime < pbbidl2->bbde.ft.dwLowDateTime) 
                {
                    hres = ResultFromShort(-1);
                }
                else if (pbbidl1->bbde.ft.dwLowDateTime > pbbidl2->bbde.ft.dwLowDateTime)
                {
                    hres = ResultFromShort(1);
                }
                else
                {
                    return 0;
                }
            }
            break;
    }
    return hres;
}


STDMETHODIMP CBitBucket_SF_GetAttributesOf(IShellFolder2 *psf, UINT cidl, LPCITEMIDLIST * apidl, ULONG * rgfOut)
{
    CBitBucket *this = IToClass(CBitBucket, isf, psf);
    ULONG gfOut = *rgfOut;

    // asking about the root itself
    if (cidl == 0)
    {
        gfOut = SFGAO_HASPROPSHEET;
    }
    else
    {
        LPCIDFOLDER pidf = FS_IsValidID(apidl[0]);

        gfOut &= (SFGAO_CANMOVE | SFGAO_CANDELETE | SFGAO_HASPROPSHEET | SFGAO_FILESYSANCESTOR | SFGAO_FILESYSTEM);

        if (*rgfOut & SFGAO_LINK)
        {
            if (SHGetClassFlags(pidf) & SHCF_IS_LINK)
            {
                gfOut |= SFGAO_LINK;
            }
        }
    }

    *rgfOut = gfOut;

    return NOERROR;
}


int DataObjToFileOpString(IDataObject * pdtobj, LPTSTR * ppszSrc, LPTSTR * ppszDest)
{
    int cItems = 0;
    STGMEDIUM medium;
    LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
    
    if (pida)
    {
        LPTSTR lpszSrc, lpszDest;
        int i, cchSrc, cchDest;

        cItems = pida->cidl;

        // start with null terminated strings
        cchSrc = cchDest = 1;
        
        if (ppszSrc)
            lpszSrc = (void*)LocalAlloc(LPTR, cchSrc * SIZEOF(TCHAR));

        if (ppszDest)
            lpszDest = (void*)LocalAlloc(LPTR, cchDest * SIZEOF(TCHAR));

        for (i = 0 ; i < cItems ; i++) 
        {
            LPCITEMIDLIST pidl = IDA_GetIDListPtr(pida, i);
            LPBBDATAENTRYIDA pbbidl = PIDLTODATAENTRYID(pidl);

            if (ppszSrc) 
            {
                TCHAR szSrc[MAX_PATH];
                int cchSrcFile;
                LPTSTR psz;

                BBGetItemPath((LPIDFOLDER)pidl, szSrc);

                cchSrcFile = lstrlen(szSrc) + 1;
                psz = (LPTSTR)LocalReAlloc((HLOCAL)lpszSrc, (cchSrc + cchSrcFile) * SIZEOF(TCHAR), LMEM_MOVEABLE | LMEM_ZEROINIT);
                if (!psz)
                {
                    // out of memory!
                    // bugbug: do something real
                    LocalFree((HLOCAL)lpszSrc);
                    if (ppszDest)
                        LocalFree((HLOCAL)lpszDest);
                    cItems = 0;
                    goto Bail;
                }
                lpszSrc = psz;
                lstrcpy(lpszSrc + cchSrc - 1, szSrc);
                cchSrc += cchSrcFile;
            }

            if (ppszDest) 
            {
                TCHAR szOrig[MAX_PATH];
                int cchDestFile;
                LPTSTR psz;

                BBGetOriginalPath(pbbidl, szOrig, ARRAYSIZE(szOrig));

                cchDestFile = lstrlen(szOrig) + 1;

                psz = (LPTSTR)LocalReAlloc((HLOCAL)lpszDest, (cchDest + cchDestFile) * SIZEOF(TCHAR), LMEM_MOVEABLE | LMEM_ZEROINIT);
                if (!psz)
                {
                    // out of memory!
                    LocalFree((HLOCAL)lpszDest);
                    if (ppszSrc) 
                        LocalFree((HLOCAL)lpszSrc);
                    cItems = 0;
                    goto Bail;
                }
                lpszDest = psz;
                lstrcpy(lpszDest + cchDest - 1, szOrig);
                cchDest += cchDestFile;
            }
        }

        if (ppszSrc)
            *ppszSrc = lpszSrc;

        if (ppszDest)
            *ppszDest = lpszDest;

Bail:
        HIDA_ReleaseStgMedium(pida, &medium);
    }
    return cItems;
}


//
// restores the list of files in the IDataObject
//
void BBRestoreFileList(CBitBucket *this, HWND hwndOwner, IDataObject * pdtobj)
{
    LPTSTR pszSrc, pszDest;

    if (DataObjToFileOpString(pdtobj, &pszSrc, &pszDest))
    {
        // now do the actual restore.
        SHFILEOPSTRUCT sFileOp = {hwndOwner,
                                  FO_MOVE,
                                  pszSrc,
                                  pszDest,
                                  FOF_MULTIDESTFILES | FOF_SIMPLEPROGRESS | FOF_NOCONFIRMMKDIR,
                                  FALSE,
                                  NULL,
                                  MAKEINTRESOURCE(IDS_BB_RESTORINGFILES)};

        DECLAREWAITCURSOR;

        SetWaitCursor();

        if (SHFileOperation(&sFileOp) == ERROR_SUCCESS)
        {
            // success!;
            SHChangeNotifyHandleEvents();

            BBCheckRestoredFiles(pszSrc);
        }

        LocalFree((HLOCAL)pszSrc);
        LocalFree((HLOCAL)pszDest);

        ResetWaitCursor();
    }
}


//
// nukes the list of files in the IDataObject
//
void BBNukeFileList(CBitBucket *this, HWND hwndOwner, IDataObject * pdtobj)
{
    LPTSTR lpszSrc, lpszDest;
    int nFiles = DataObjToFileOpString(pdtobj, &lpszSrc, &lpszDest);
    
    if (nFiles)
    {
        // now do the actual nuke.
        int iItems;
        WIN32_FIND_DATA fd;
        CONFIRM_DATA cd = {CONFIRM_DELETE_FILE | CONFIRM_DELETE_FOLDER | CONFIRM_PROGRAM_FILE | CONFIRM_MULTIPLE, 0};
        SHFILEOPSTRUCT sFileOp ={hwndOwner,
                                 FO_DELETE,
                                 lpszSrc,
                                 NULL,
                                 FOF_NOCONFIRMATION | FOF_SIMPLEPROGRESS,
                                 FALSE,
                                 NULL,
                                 MAKEINTRESOURCE(IDS_BB_DELETINGWASTEBASKETFILES)};

        DECLAREWAITCURSOR;

        SetWaitCursor();

        fd.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
        if (ConfirmFileOp(hwndOwner, NULL, &cd, nFiles, 0, CONFIRM_DELETE_FILE | CONFIRM_WASTEBASKET_PURGE, lpszDest, &fd, NULL, &fd, NULL) == IDYES)
        {
            SHFileOperation(&sFileOp);
            
            SHChangeNotifyHandleEvents();

            // update the icon if there are objects left in the list
            iItems = (int) ShellFolderView_GetObjectCount(hwndOwner);
            UpdateIcon(iItems);
        }

        LocalFree((HLOCAL)lpszSrc);
        LocalFree((HLOCAL)lpszDest);

        ResetWaitCursor();
    }
}


void EnableTrackbarAndFamily(HWND hDlg, BOOL f)
{
    EnableWindow(GetDlgItem(hDlg, IDC_BBSIZE), f);
    EnableWindow(GetDlgItem(hDlg, IDC_BBSIZETEXT), f);
    EnableWindow(GetDlgItem(hDlg, IDC_TEXT), f);
}


void BBGlobalPropOnCommand(HWND hDlg, int id, HWND hwndCtl, UINT codeNotify)
{
    LPBBPROPSHEETINFO ppsi = (LPBBPROPSHEETINFO)GetWindowLongPtr(hDlg, DWLP_USER);
    BOOL fNukeOnDelete;

    switch (id)
    {
        case IDC_GLOBAL:
        case IDC_INDEPENDENT:
            fNukeOnDelete = IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE);

            ppsi->fUseGlobalSettings = (IsDlgButtonChecked(hDlg, IDC_GLOBAL) == BST_CHECKED) ? TRUE : FALSE;
            EnableWindow(GetDlgItem(hDlg, IDC_NUKEONDELETE), ppsi->fUseGlobalSettings);
            EnableTrackbarAndFamily(hDlg, ppsi->fUseGlobalSettings && !fNukeOnDelete);

            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
            break;

        case IDC_NUKEONDELETE:
            fNukeOnDelete = IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE);

	    if (fNukeOnDelete)
            {                
	        // In order to help protect users, when they turn on "Remove files immedately" we also
	        // check the "show delete confimation" box automatically for them. Thus, they will have
	        // to explicitly uncheck it if they do not want confimation that their files will be nuked.
		CheckDlgButton(hDlg, IDC_CONFIRMDELETE, BST_CHECKED);
            }

            EnableTrackbarAndFamily(hDlg, !fNukeOnDelete);
            // fall through
        case IDC_CONFIRMDELETE:
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;

    }
}


void  RelayMessageToChildren(HWND hwnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
    HWND hwndChild;

    for (hwndChild = GetWindow(hwnd, GW_CHILD); hwndChild != NULL; hwndChild = GetWindow(hwndChild, GW_HWNDNEXT))
    {
        SendMessage(hwndChild, uMessage, wParam, lParam);
    }
}


//
// This is the dlg proc for the "Global" tab on the recycle bin
//
BOOL_PTR CALLBACK BBGlobalPropDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPBBPROPSHEETINFO ppsi = (LPBBPROPSHEETINFO)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) 
    {
    HANDLE_MSG(hDlg, WM_COMMAND, BBGlobalPropOnCommand);

    case WM_INITDIALOG: 
    {
        HWND  hwndTrack = GetDlgItem(hDlg, IDC_BBSIZE);
	SHELLSTATE ss;

	// make sure the info we have is current
	RefreshAllBBDriveSettings();

        ppsi = (LPBBPROPSHEETINFO)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        SendMessage(hwndTrack, TBM_SETTICFREQ, 10, 0);
        SendMessage(hwndTrack, TBM_SETRANGE, FALSE, MAKELONG(0, 100));
        SendMessage(hwndTrack, TBM_SETPOS, TRUE, ppsi->iOriginalPercent);

        EnableWindow(GetDlgItem(hDlg, IDC_NUKEONDELETE), ppsi->fUseGlobalSettings);
        EnableTrackbarAndFamily(hDlg, ppsi->fUseGlobalSettings && !ppsi->fNukeOnDelete);
        CheckDlgButton(hDlg, IDC_NUKEONDELETE, ppsi->fNukeOnDelete);
        CheckRadioButton(hDlg, IDC_INDEPENDENT, IDC_GLOBAL, ppsi->fUseGlobalSettings ? IDC_GLOBAL : IDC_INDEPENDENT);

        SHGetSetSettings(&ss, SSF_NOCONFIRMRECYCLE, FALSE);
        CheckDlgButton(hDlg, IDC_CONFIRMDELETE, !ss.fNoConfirmRecycle);
    }
    // fall through to set iGlobalPercent
    case WM_HSCROLL: 
    {
        TCHAR szPercent[20];
        HWND hwndTrack = GetDlgItem(hDlg, IDC_BBSIZE);

        ppsi->iPercent = (int) SendMessage(hwndTrack, TBM_GETPOS, 0, 0);
        wsprintf(szPercent, TEXT("%d%%"), ppsi->iPercent);
        SetDlgItemText(hDlg, IDC_BBSIZETEXT, szPercent);

        if (ppsi->iPercent != ppsi->iOriginalPercent)
        {
            SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);

	    if (ppsi->iPercent == 0)
            {
	        // In order to help protect users, when they set the % slider to zero we also
	        // check the "show delete confimation" box automatically for them. Thus, they will have
	        // to explicitly uncheck it if they do not want confimation that their files will be nuked.
		CheckDlgButton(hDlg, IDC_CONFIRMDELETE, BST_CHECKED);
            }
	}

        return TRUE;
    }

    case WM_HELP:
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
            HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aBitBucketPropHelpIDs);
        return TRUE;

    case WM_CONTEXTMENU:
        WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
            (ULONG_PTR)(LPVOID) aBitBucketPropHelpIDs);
        return TRUE;

    case WM_WININICHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_DISPLAYCHANGE:
        RelayMessageToChildren(hDlg, uMsg, wParam, lParam);
        break;

    case WM_DESTROY:
        CheckCompactAndPurge();
        SHUpdateRecycleBinIcon();
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code)
        {
            case PSN_APPLY:
            {
                SHELLSTATE ss;

                ss.fNoConfirmRecycle = !IsDlgButtonChecked(hDlg, IDC_CONFIRMDELETE);
                SHGetSetSettings(&ss, SSF_NOCONFIRMRECYCLE, TRUE);
                
                ppsi->fNukeOnDelete = (IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE) == BST_CHECKED) ? TRUE : FALSE;
                ppsi->fUseGlobalSettings = (IsDlgButtonChecked(hDlg, IDC_INDEPENDENT) == BST_CHECKED) ? FALSE : TRUE;

		// if anything on the global tab changed, update all the drives
		if (ppsi->fUseGlobalSettings != ppsi->fOriginalUseGlobalSettings    ||
                    ppsi->fNukeOnDelete != ppsi->fOriginalNukeOnDelete              ||
                    ppsi->iPercent != ppsi->iOriginalPercent)
                {
                    int i;

                    // NOTE: We get a PSN_APPLY after all the drive tabs. This has to be this way so that
                    // if global settings change, then the global tab will re-apply all the most current settings
                    // bassed on the global variables that get set above.

                    // this sets the new global settings in the registry
                    if (!PersistGlobalSettings(ppsi->fUseGlobalSettings, ppsi->fNukeOnDelete, ppsi->iPercent))
                    {
                        // we failed, so show the error dialog and bail
                        ShellMessageBox(HINST_THISDLL,
                                        hDlg,
                                        MAKEINTRESOURCE(IDS_BB_CANNOTCHANGESETTINGS),
                                        MAKEINTRESOURCE(IDS_WASTEBASKET),
                                        MB_OK | MB_ICONEXCLAMATION);

                        SetDlgMsgResult(hDlg, WM_NOTIFY, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }

                    for (i = 0; i < MAX_BITBUCKETS ; i++)
                    {
                        if (MakeBitBucket(i))
                        {
                            BOOL bPurge = TRUE;

                            // we need to purge all the drives in this case
                            RegSetValueEx(g_pBitBucket[i]->hkeyPerUser, TEXT("NeedToPurge"), 0, REG_DWORD, (LPBYTE)&bPurge, SIZEOF(bPurge));

                            RefreshBBDriveSettings(i);
                        }
                    }

                    ppsi->fOriginalUseGlobalSettings = ppsi->fUseGlobalSettings;
                    ppsi->fOriginalNukeOnDelete = ppsi->fNukeOnDelete; 
                    ppsi->iOriginalPercent = ppsi->iPercent;
                }
            }
        }
        break;

        SetDlgMsgResult(hDlg, WM_NOTIFY, 0);
        return TRUE;
    }

    return FALSE;
}

BOOL_PTR CALLBACK BBDriveDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LPBBPROPSHEETINFO ppsi = (LPBBPROPSHEETINFO)GetWindowLongPtr(hDlg, DWLP_USER);
    TCHAR szDiskSpace[40];
    HWND hwndTrack;

    switch (uMsg) 
    {

        case WM_INITDIALOG: 
        {
            hwndTrack = GetDlgItem(hDlg, IDC_BBSIZE);

            ppsi = (LPBBPROPSHEETINFO)lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            SendMessage(hwndTrack, TBM_SETTICFREQ, 10, 0);
            SendMessage(hwndTrack, TBM_SETRANGE, FALSE, MAKELONG(0, 100));
            SendMessage(hwndTrack, TBM_SETPOS, TRUE, ppsi->iPercent);
            CheckDlgButton(hDlg, IDC_NUKEONDELETE, ppsi->fNukeOnDelete);

            // set the disk space info
            StrFormatByteSize64(g_pBitBucket[ppsi->idDrive]->qwDiskSize, szDiskSpace, ARRAYSIZE(szDiskSpace));
            SetDlgItemText(hDlg, IDC_DISKSIZEDATA, szDiskSpace);
            wParam = 0;
        }
        // fall through

        case WM_HSCROLL:
        {
            ULARGE_INTEGER ulBucketSize;
            HWND hwndTrack = GetDlgItem(hDlg, IDC_BBSIZE);
            ppsi->iPercent = (int)SendMessage(hwndTrack, TBM_GETPOS, 0, 0);

            wsprintf(szDiskSpace, TEXT("%d%%"), ppsi->iPercent);
            SetDlgItemText(hDlg, IDC_BBSIZETEXT, szDiskSpace);
                       
            if (ppsi->iPercent != ppsi->iOriginalPercent) 
            {
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
            }

            // we peg the max size of the recycle bin to 4 gig
            ulBucketSize.QuadPart = (ppsi->pGlobal->fUseGlobalSettings ? ppsi->pGlobal->iPercent : ppsi->iPercent) * (g_pBitBucket[ppsi->idDrive]->qwDiskSize / 100);
            StrFormatByteSize64(ulBucketSize.HighPart ? (DWORD)-1 : ulBucketSize.LowPart, szDiskSpace, ARRAYSIZE(szDiskSpace));
            SetDlgItemText(hDlg, IDC_BYTESIZEDATA, szDiskSpace);
            return TRUE;
        }

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aBitBucketPropHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (ULONG_PTR)(LPVOID) aBitBucketPropHelpIDs);
            return TRUE;

        case WM_COMMAND:
        {
            WORD wCommandID = GET_WM_COMMAND_ID(wParam, lParam);
            
            if (wCommandID == IDC_NUKEONDELETE)
            {
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
                EnableTrackbarAndFamily(hDlg, !IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE));
                EnableWindow(GetDlgItem(hDlg, IDC_BYTESIZE), !IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE));
                EnableWindow(GetDlgItem(hDlg, IDC_BYTESIZEDATA), !IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE));
            }
            break;
        }

        case WM_NOTIFY:
            switch (((NMHDR *)lParam)->code) 
            {
                case PSN_APPLY:
                {
                    ppsi->fNukeOnDelete = (IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE) == BST_CHECKED) ? TRUE : FALSE;

                    // update the info in the registry
                    if (!PersistBBDriveSettings(ppsi->idDrive, ppsi->iPercent, ppsi->fNukeOnDelete))
                    {
                        // we failed, so show the error dialog and bail
                        ShellMessageBox(HINST_THISDLL,
                                        hDlg,
                                        MAKEINTRESOURCE(IDS_BB_CANNOTCHANGESETTINGS),
                                        MAKEINTRESOURCE(IDS_WASTEBASKET),
                                        MB_OK | MB_ICONEXCLAMATION);

                        SetDlgMsgResult(hDlg, WM_NOTIFY, PSNRET_INVALID_NOCHANGEPAGE);
                        return TRUE;
                    }
                    
                    // only purge this drive if the user set the slider to a smaller value       
                    if (ppsi->iPercent < ppsi->iOriginalPercent)
                    {
                        BOOL bPurge = TRUE;

                        // since this drive just shrunk, we need to purge the files in it
                        RegSetValueEx(g_pBitBucket[ppsi->idDrive]->hkeyPerUser, TEXT("NeedToPurge"), 0, REG_DWORD, (LPBYTE)&bPurge, SIZEOF(bPurge));
                    }

                    ppsi->iOriginalPercent = ppsi->iPercent;
                    ppsi->fOriginalNukeOnDelete = ppsi->fNukeOnDelete;
                    
                    // update the g_pBitBucket[] for this drive

                    // NOTE: We get a PSN_APPLY before the global tab does. This has to be this way so that
                    // if global settings change, then the global tab will re-apply all the most current settings
                    // bassed on the global variables that get set in his tab.
                    RefreshBBDriveSettings(ppsi->idDrive);
                }
                break;

                case PSN_SETACTIVE:
                {   
                    BOOL fNukeOnDelete = IsDlgButtonChecked(hDlg, IDC_NUKEONDELETE);

                    EnableWindow(GetDlgItem(hDlg, IDC_NUKEONDELETE), !ppsi->pGlobal->fUseGlobalSettings);
                    EnableTrackbarAndFamily(hDlg, !ppsi->pGlobal->fUseGlobalSettings && !fNukeOnDelete);
                    EnableWindow(GetDlgItem(hDlg, IDC_BYTESIZE), !fNukeOnDelete);
                    EnableWindow(GetDlgItem(hDlg, IDC_BYTESIZEDATA), !fNukeOnDelete);

                    // send this to make sure that the "space reserved" field is accurate when using global settings
                    SendMessage(hDlg, WM_HSCROLL, 0, 0);
                }
                break;
            }

            SetDlgMsgResult(hDlg, WM_NOTIFY, 0);
            return TRUE;
    }

    return FALSE;
}


//
// this is the property sheet page for a file/folder in the bitbucket
//
BOOL_PTR CALLBACK BBFilePropDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BBFILEPROPINFO * pbbfpi = (BBFILEPROPINFO *)GetWindowLongPtr(hDlg, DWLP_USER);
    TCHAR szTemp[MAX_PATH];

    switch (uMsg)
    {

        case WM_INITDIALOG:
        {
            LPCITEMIDLIST pidl;
            LPBBDATAENTRYIDA pbbidl;
            HICON hIcon;
            ULONGLONG cbSize;

            pbbfpi = (BBFILEPROPINFO *)lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, lParam);

            pidl = pbbfpi->pidl;
            pbbidl = PIDLTODATAENTRYID(pidl);

            BBGetOriginalPath(pbbidl, szTemp, ARRAYSIZE(szTemp));

#ifdef WINNT
            //
            // We don't allow user to change compression attribute on a deleted file
            // but we do show the current compressed state
            //
            {
               TCHAR szRoot[_MAX_PATH + 1];
               DWORD dwVolumeFlags = 0;
               DWORD dwFileAttributes;
               TCHAR szFSName[12];

               //
               // If file's volume doesn't support compression, don't show
               // "Compressed" checkbox.
               // If compression is supported, show the checkbox and check/uncheck
               // it to indicate compression state of the file.
               // Perform this operation while szTemp contains complete path name.
               //
               lstrcpy(szRoot, szTemp);
               PathQualify(szRoot);
               PathStripToRoot(szRoot);

               dwFileAttributes = GetFileAttributes(szTemp);

               if (GetVolumeInformation(szRoot, NULL, 0, NULL, NULL, &dwVolumeFlags, szFSName, ARRAYSIZE(szFSName)) &&
                   (dwFileAttributes != (DWORD)-1))
               {
                    if (dwVolumeFlags & FS_FILE_COMPRESSION)
                    {
                        if (dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED)
                        {
                            CheckDlgButton(hDlg, IDD_COMPRESS, 1);
                        }
                        ShowWindow(GetDlgItem(hDlg, IDD_COMPRESS), SW_SHOW);
                    }

                    if (g_bRunOnNT5)
                    {
                     // BUGBUG (ccteng) - HACK
                     // Before NTFS implements FS_FILE_ENCRYPTION, we check the compression
                     // flag instead, which works as long as we also check for g_bRunOnNT5.
                     // After we switch to FS_FILE_ENCRYPTION,
                     // we can also remove if (g_bRunOnNT5).

                        // if (dwVolumeFlags & FS_FILE_ENCRYPTION)
                        if (dwVolumeFlags & FS_FILE_COMPRESSION)
                        {
                            if (dwFileAttributes & FILE_ATTRIBUTE_ENCRYPTED)
                            {
                                CheckDlgButton(hDlg, IDD_ENCRYPT, 1);
                            }
                            ShowWindow(GetDlgItem(hDlg, IDD_ENCRYPT), SW_SHOW);
                        }
                    }
               }
            }
#else
            //
            // Win95 doesn't support compression/encryption
            //
#endif

            PathRemoveExtension(szTemp);
            SetDlgItemText(hDlg, IDD_NAME, PathFindFileName(szTemp));

            // origin
            PathRemoveFileSpec(szTemp);
            SetDlgItemText(hDlg, IDD_LOCATION, PathFindFileName(szTemp));

            // Type
            FS_GetTypeName((LPIDFOLDER)pidl, szTemp, ARRAYSIZE(szTemp));
            SetDlgItemText(hDlg, IDD_FILETYPE, szTemp);

            // Size
            if (FS_IsFolder((LPIDFOLDER)pidl))
                cbSize = pbbidl->bbde.dwSize;
            else
                FS_GetSize(NULL, (LPIDFOLDER)pidl, &cbSize);

            StrFormatByteSize64(cbSize, szTemp, ARRAYSIZE(szTemp));
            SetDlgItemText(hDlg, IDD_FILESIZE, szTemp);

            // deleted time
            {
                FILETIME ft = pbbidl->bbde.ft;
                SetDateTimeText(hDlg, IDD_DELETED, &ft);
            }

            {
                HANDLE hfind;
                WIN32_FIND_DATA fd;

                BBGetItemPath((LPIDFOLDER)pidl, szTemp);

                hfind = FindFirstFile(szTemp, &fd);
                if (hfind != INVALID_HANDLE_VALUE)
                {
                    SetDateTimeText(hDlg, IDD_CREATED, &fd.ftCreationTime);
                    FindClose(hfind);
                }

                // file attributes
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
                    CheckDlgButton(hDlg, IDD_READONLY, 1);
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE)
                    CheckDlgButton(hDlg, IDD_ARCHIVE, 1);
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
                    CheckDlgButton(hDlg, IDD_HIDDEN, 1);

                // icon
                hIcon = SHGetFileIcon(NULL, szTemp, fd.dwFileAttributes, SHGFI_LARGEICON|SHGFI_USEFILEATTRIBUTES);
                if (hIcon)
                {
                    hIcon = (HICON)SendDlgItemMessage(hDlg, IDD_ITEMICON, STM_SETICON, (WPARAM)hIcon, 0L);
                    if (hIcon)
                        DestroyIcon(hIcon);
                }
            }
            break;
        }

        case WM_WININICHANGE:
        case WM_SYSCOLORCHANGE:
        case WM_DISPLAYCHANGE:
            RelayMessageToChildren(hDlg, uMsg, wParam, lParam);
            break;

        case WM_NOTIFY:
            switch (((NMHDR *)lParam)->code) 
            {
                case PSN_APPLY:
                case PSN_SETACTIVE:
                case PSN_KILLACTIVE:
                    return TRUE;
            }
            break;

        case WM_HELP:
            WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, NULL,
                HELP_WM_HELP, (ULONG_PTR)(LPTSTR) aBitBucketHelpIDs);
            return TRUE;

        case WM_CONTEXTMENU:
            WinHelp((HWND) wParam, NULL, HELP_CONTEXTMENU,
                (ULONG_PTR)(LPVOID) aBitBucketHelpIDs);
            return TRUE;
    }

    return FALSE;
}

void BBGetDriveDisplayName(int idDrive, LPTSTR pszName, UINT cchSize)
{
    TCHAR szDrive[MAX_PATH];
    SHFILEINFO sfi;

    VDATEINPUTBUF(pszName, TCHAR, cchSize);

    DriveIDToBBRoot(idDrive, szDrive);

    if (SHGetFileInfo(szDrive, 0, &sfi, SIZEOF(sfi), SHGFI_DISPLAYNAME)) 
    {
        lstrcpyn(pszName, sfi.szDisplayName, cchSize);
    }
    
    // If SERVERDRIVE, attempt to overwrite the default display name with the display
    // name for the mydocs folder on the desktop, since SERVERDRIVE==mydocs for now
    if (idDrive == SERVERDRIVE) 
    {
        GetMyDocumentsDisplayName(pszName, cchSize);
    }
}


BOOL CALLBACK _BB_AddPage(HPROPSHEETPAGE psp, LPARAM lParam)
{
    LPPROPSHEETHEADER ppsh = (LPPROPSHEETHEADER)lParam;

    ppsh->phpage[ppsh->nPages++] = psp;
    return TRUE;
}


DWORD CALLBACK _BB_PropertiesThread(IDataObject * pdtobj)
{
    HPROPSHEETPAGE ahpage[MAXPROPPAGES];
    TCHAR szTitle[80];
    PROPSHEETHEADER psh;
    UNIQUESTUBINFO usi;
    LPITEMIDLIST pidlBitBucket = SHCloneSpecialIDList(NULL, CSIDL_BITBUCKET, FALSE);
    BOOL fUnique;

    psh.dwSize = SIZEOF(psh);
    psh.dwFlags = PSH_PROPTITLE;
    psh.hInstance = HINST_THISDLL;
//  psh.hwndParent = NULL;      // will be filled in later
    psh.nStartPage = 0;
    psh.phpage = ahpage;
    psh.nPages = 0;

    if (pdtobj)
    {
        // this is the recycled file properties case, 
        // we only show the proeprties for the first file if
        // there is a multiple selection
        BBFILEPROPINFO bbfpi;
        STGMEDIUM medium;
        LPITEMIDLIST pidlSave;
        TCHAR szTemp[MAX_PATH];
        LPBBDATAENTRYIDA lpbbidl;
        LPIDA pida;

        pida = DataObj_GetHIDA(pdtobj, &medium);

        pidlSave = ILCombine(pidlBitBucket, IDA_GetIDListPtr(pida, 0));
        fUnique = EnsureUniqueStub(pidlSave, STUBCLASS_PROPSHEET, NULL, &usi);
        ILFree(pidlSave);

        if (!fUnique) // found other window
            goto Cleanup;

        bbfpi.psp.dwFlags = 0;
        bbfpi.psp.dwSize = SIZEOF(bbfpi);
        bbfpi.psp.hInstance = HINST_THISDLL;
        bbfpi.psp.pszTemplate = MAKEINTRESOURCE(DLG_DELETEDFILEPROP);
        bbfpi.psp.pfnDlgProc = BBFilePropDlgProc;
        bbfpi.psp.pszTitle = szTitle;
        bbfpi.hida = medium.hGlobal;
        bbfpi.pidl = IDA_GetIDListPtr(pida, 0);

        lpbbidl = PIDLTODATAENTRYID(bbfpi.pidl);

        BBGetOriginalPath(lpbbidl, szTemp, ARRAYSIZE(szTemp));

        lstrcpyn(szTitle, PathFindFileName(szTemp), ARRAYSIZE(szTitle));
        PathRemoveExtension(szTitle);

        psh.phpage[0] = CreatePropertySheetPage(&bbfpi.psp);

        HIDA_ReleaseStgMedium(pida, &medium);

        psh.nPages = 1;
        psh.pszCaption = szTitle;
    }
    else
    {
        // this is the recycle bin property sheet case
        fUnique = EnsureUniqueStub(pidlBitBucket, STUBCLASS_PROPSHEET, NULL, &usi);

        if (!fUnique)
            goto Cleanup;

        CBitBucket_PS_AddPages(NULL, _BB_AddPage, (LPARAM)&psh);

        psh.pszCaption = MAKEINTRESOURCE(IDS_WASTEBASKET);
    }

    psh.hwndParent = usi.hwndStub;
    PropertySheet(&psh);

Cleanup:

    if (pidlBitBucket)
        ILFree(pidlBitBucket);

    FreeUniqueStub(&usi);

    return TRUE;
}

typedef struct _bb_threaddata {
    CBitBucket* pbb;
    HWND hwndOwner;
    IDataObject * pdtobj;
    IStream *pstmDataObj;
    ULONG_PTR idCmd;
    POINT ptDrop;
    BOOL fSameHwnd;
    BOOL fDragDrop;
} BBTHREADDATA;

DWORD WINAPI BB_DropThreadInit(BBTHREADDATA *pbbtd)
{
    STGMEDIUM medium;
    FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

    if (SUCCEEDED(pbbtd->pdtobj->lpVtbl->GetData(pbbtd->pdtobj, &fmte, &medium))) 
    {
        // call delete here so that files will be moved in
        // their respective bins, not necessarily this one.
        DRAGINFO di;

        di.uSize = SIZEOF(DRAGINFO);

        if (DragQueryInfo(medium.hGlobal, &di))
        {
            // Since BBWillRecycle() can return true even when the file will NOT be
            // recycled (eg the file will be nuked), we want to warn the user when we 
            // are going to nuke something that they initiall thought that it would
            // be recycled
            UINT fOptions = SD_WARNONNUKE; 

            if (!BBWillRecycle(di.lpFileList, NULL) ||
                (di.lpFileList && (di.lpFileList[lstrlen(di.lpFileList)+1] == 0)
                 && PathIsShortcutToProgram(di.lpFileList)))
                fOptions = SD_USERCONFIRMATION; 

            if (IsFileInBitBucket(di.lpFileList)) 
            {
                LPITEMIDLIST *ppidl = NULL;
                int cidl = FS_CreateMoveCopyList(pbbtd->pdtobj, NULL, &ppidl);
                if (ppidl) 
                {
                    FS_PositionItems(pbbtd->hwndOwner, cidl, ppidl, pbbtd->pdtobj, &pbbtd->ptDrop, pbbtd->fDragDrop);
                    FS_FreeMoveCopyList(ppidl, cidl);
                }
            } 
            else 
            {
                TransferDelete(pbbtd->hwndOwner, medium.hGlobal, fOptions);
            }

            SHChangeNotifyHandleEvents();
            SHFree(di.lpFileList);
        }
        ReleaseStgMedium(&medium);
    }
    return 0;
}


DWORD CALLBACK _BB_DispatchThreadProc(void *pv)
{
    BBTHREADDATA *pbbtd = (BBTHREADDATA *)pv;

    if (pbbtd->pstmDataObj)
    {
        CoGetInterfaceAndReleaseStream(pbbtd->pstmDataObj, &IID_IDataObject, (void **)&pbbtd->pdtobj);
        pbbtd->pstmDataObj = NULL;  // this is dead
    }

    switch(pbbtd->idCmd)
    {
    case DFM_CMD_MOVE:
        if (pbbtd->pdtobj)
            BB_DropThreadInit(pbbtd);
        break;

    case DFM_CMD_PROPERTIES:
    case FSIDM_PROPERTIESBG:    // NULL pdtobj is valid

        _BB_PropertiesThread(pbbtd->pdtobj);
        break;

    case DFM_CMD_DELETE:
        if (pbbtd->pdtobj)
            BBNukeFileList(pbbtd->pbb, pbbtd->hwndOwner, pbbtd->pdtobj);
        break;

    case FSIDM_RESTORE:
        if (pbbtd->pdtobj)
            BBRestoreFileList(pbbtd->pbb, pbbtd->hwndOwner, pbbtd->pdtobj);
        break;
    }

    if (pbbtd->pdtobj)
        pbbtd->pdtobj->lpVtbl->Release(pbbtd->pdtobj);

    if (pbbtd->pbb)
        pbbtd->pbb->isf.lpVtbl->Release(&pbbtd->pbb->isf);

    LocalFree((HLOCAL)pbbtd);

    return 0;
}


HRESULT BB_LaunchThread(CBitBucket *this, HWND hwndOwner, IDataObject * pdtobj, WPARAM idCmd)
{
    HRESULT hr = E_OUTOFMEMORY;
    BBTHREADDATA *pbbtd = (BBTHREADDATA *)LocalAlloc(LPTR, SIZEOF(*pbbtd));
    if (pbbtd)
    {
        pbbtd->pbb = this;
        pbbtd->hwndOwner = hwndOwner;
        pbbtd->idCmd = idCmd;

        if (idCmd == DFM_CMD_MOVE)
        {
            pbbtd->fDragDrop = (BOOL) ShellFolderView_GetDropPoint(hwndOwner, &pbbtd->ptDrop);
        }

        if (this)
            this->isf.lpVtbl->AddRef(&this->isf);

        if (pdtobj)
            CoMarshalInterThreadInterfaceInStream(&IID_IDataObject, (IUnknown *)pdtobj, &pbbtd->pstmDataObj);

        if (SHCreateThread(_BB_DispatchThreadProc, pbbtd, CTF_COINIT, NULL))
        {
            hr = NOERROR;
        }
        else
        {
            if (pbbtd->pstmDataObj)
                pbbtd->pstmDataObj->lpVtbl->Release(pbbtd->pstmDataObj);

            if (this)
                this->isf.lpVtbl->Release(&this->isf);

            LocalFree((HLOCAL)pbbtd);
        }
    }
    return hr;
}

const TCHAR c_szUnDelete[] = TEXT("undelete");
const TCHAR c_szPurgeAll[] = TEXT("empty");

HRESULT GetVerb(UINT_PTR idCmd, LPSTR pszName, UINT cchMax, BOOL bUnicode)
{
    HRESULT hres;
    LPCTSTR pszNameT;

    switch (idCmd)
    {
        case FSIDM_RESTORE:
            pszNameT = c_szUnDelete;
            break;
        case FSIDM_PURGEALL:
            pszNameT = c_szPurgeAll;
            break;
        default:
            return E_NOTIMPL;
    }

    if (bUnicode)
        hres = SHTCharToUnicode(pszNameT, (LPWSTR)pszName, cchMax);
    else
        hres = SHTCharToAnsi(pszNameT, (LPSTR)pszName, cchMax);

    return hres;
}

//
// Callback from DefCM
//
HRESULT CALLBACK CBitBucket_DFMCallBack(IShellFolder *psf, HWND hwndOwner,
                IDataObject * pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBitBucket *this = IToClass(CBitBucket, isf, psf);
    HRESULT hres = NOERROR;     // assume no error

    switch(uMsg)
    {
        case DFM_MERGECONTEXTMENU:
            CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_BITBUCKET_ITEM, 0, (LPQCMINFO)lParam);
            hres = ResultFromShort(-1); // return 1 so default reg commands won't be added
            break;

        case DFM_MAPCOMMANDNAME:
            if (lstrcmpi((LPCTSTR)lParam, c_szUnDelete) == 0)
            {
                *(int *)wParam = FSIDM_RESTORE;
            }
            else
            {
                // command not found
                hres = E_FAIL;
            }
            break;

        case DFM_INVOKECOMMAND:
            switch (wParam) 
            {
                case FSIDM_RESTORE:
                case DFM_CMD_DELETE:
                case DFM_CMD_PROPERTIES:
                    hres = BB_LaunchThread(this, hwndOwner, pdtobj, wParam);
                    break;

                default:
                    hres = S_FALSE;
                    break;
            }
            break;

        case DFM_GETHELPTEXT:
            LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));;
            break;

        case DFM_GETHELPTEXTW:
            LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));;
            break;

        case DFM_GETVERBA:
        case DFM_GETVERBW:
            hres = GetVerb((UINT_PTR)(LOWORD(wParam)), (LPSTR)lParam, (UINT)(HIWORD(wParam)), uMsg == DFM_GETVERBW);
            break;

        default:
            hres = E_NOTIMPL;
            break;
    }

    return hres;
}



//
// CBitBucketIDLDropTarget::DragEnter
//
//  This function puts DROPEFFECT_LINK in *pdwEffect, only if the data object
//  contains one or more net resource.
//
STDMETHODIMP CBitBucketIDLDropTarget_DragEnter(IDropTarget *pdropt, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdropt);

    TraceMsg(TF_BITBUCKET, "Bitbucket: CBitBucketIDLDropTarget::DragEnter");

    // Call the base class first
    CIDLDropTarget_DragEnter(pdropt, pDataObj, grfKeyState, pt, pdwEffect);

    // we don't really care what is in the data object, as long as move
    // is supported by the source we say you can move it to the wastbasket
    // in the case of files we will do the regular recycle bin stuff, if
    // it is not files we will just say it is moved and let the source delete it
    *pdwEffect &= DROPEFFECT_MOVE;

    this->dwEffectLastReturned = *pdwEffect;

    return NOERROR;
}


//
// CBitBucketIDLDropTarget::Drop
//
//  This function creates a connection to a dropped net resource object.
//
STDMETHODIMP CBitBucketIDLDropTarget_Drop(IDropTarget * pdropt, IDataObject * pDataObj, DWORD grfKeyState, POINTL pt, LPDWORD pdwEffect)
{
    CIDLDropTarget *this = IToClass(CIDLDropTarget, dropt, pdropt);
    HRESULT hres;
    BOOL fWebFoldersHack;

    // only move operation is allowed
    *pdwEffect &= DROPEFFECT_MOVE;
    fWebFoldersHack = FALSE;

    if (*pdwEffect)
    {
        hres = CIDLDropTarget_DragDropMenu(this, DROPEFFECT_MOVE, pDataObj,
                pt, pdwEffect, NULL, NULL, POPUP_NONDEFAULTDD, grfKeyState);

        if (hres == S_FALSE)
        {
            // let callers know where this is about to go
            // Defview cares where it went so it can handle non-filesys items
            // SHScrap cares because it needs to close the file so we can delete it
            DataObj_SetDropTarget(pDataObj, &CLSID_RecycleBin);

            if (DataObj_GetDWORD(pDataObj, g_cfNotRecyclable, 0))
            {
                if (ShellMessageBox(HINST_THISDLL, NULL,
                                    MAKEINTRESOURCE(IDS_CONFIRMNOTRECYCLABLE),
                                    MAKEINTRESOURCE(IDS_RECCLEAN_NAMETEXT),
                                    MB_SETFOREGROUND | MB_ICONQUESTION | MB_YESNO) == IDNO)
                {
                    *pdwEffect = DROPEFFECT_NONE;
                    goto lCancel;
                }
            }

            if (this->dwData & DTID_HDROP)  // CF_HDROP
            {
                BB_LaunchThread(NULL, this->hwndOwner, pDataObj, DFM_CMD_MOVE);

                // since we will move the file ourself, known as an optimised move, 
                // we return zero here. this is per the OLE spec

                *pdwEffect = DROPEFFECT_NONE;
            }
            else
            {
                // if it was not files, we just say we moved the data, letting the
                // source deleted it. lets hope they support undo...

                *pdwEffect = DROPEFFECT_MOVE;

                // HACK: Put up a "you can't undo this" warning for web folders.
                {
                    LPIDA pida;
                    STGMEDIUM stgmed;
                    pida = DataObj_GetHIDA (pDataObj, &stgmed);
                    if (pida)
                    {
                        CLSID clsidSource;
                        IPersist *pPers;
                        LPCITEMIDLIST pidl;

                        pidl = IDA_GetIDListPtr (pida, -1);
                        if (pidl)
                        {
                            hres = SHBindToIDListParent (pidl, &IID_IPersist, (void **) &pPers, NULL);
                            if (FAILED(hres))
                            {
                                IShellFolder *psf;
                                hres = SHBindToObject(NULL, &IID_IShellFolder, pidl, (void **) &psf);
                                if (SUCCEEDED(hres))
                                {
                                    hres = psf->lpVtbl->QueryInterface (psf, &IID_IPersist, (void **) &pPers);
                                    psf->lpVtbl->Release(psf);
                                }
                            }
                            if (SUCCEEDED(hres))
                            {
                                hres = pPers->lpVtbl->GetClassID (pPers, &clsidSource);
                                if (SUCCEEDED(hres) &&
                                    IsEqualGUID (&clsidSource, &CLSID_WebFolders))
                                {
                                    if (ShellMessageBox(HINST_THISDLL, NULL,
                                                        MAKEINTRESOURCE(IDS_CONFIRMNOTRECYCLABLE),
                                                        MAKEINTRESOURCE(IDS_RECCLEAN_NAMETEXT),
                                                        MB_SETFOREGROUND | MB_ICONQUESTION | MB_YESNO) == IDNO)
                                    {
                                        *pdwEffect = DROPEFFECT_NONE;
                                        pPers->lpVtbl->Release(pPers);
                                        HIDA_ReleaseStgMedium (pida, &stgmed);
                                        goto lCancel;
                                    }
                                    else
                                    {
                                        fWebFoldersHack = TRUE;
                                    }
                                }
                                pPers->lpVtbl->Release(pPers);
                            }
                        }
                        HIDA_ReleaseStgMedium (pida, &stgmed);
                    }
                }
            }
lCancel:
            if (!fWebFoldersHack)
            {
                DataObj_SetDWORD(pDataObj, g_cfPerformedDropEffect, *pdwEffect);
                DataObj_SetDWORD(pDataObj, g_cfLogicalPerformedDropEffect, DROPEFFECT_MOVE);
            }
            else
            {
                // Make web folders really delete its source file.
                DataObj_SetDWORD (pDataObj, g_cfPerformedDropEffect, 0);
            }
        }
    }

    CIDLDropTarget_DragLeave(pdropt);

    return S_OK;
}


const IDropTargetVtbl c_CBBDropTargetVtbl =
{
    CIDLDropTarget_QueryInterface, CIDLDropTarget_AddRef, CIDLDropTarget_Release,
    CBitBucketIDLDropTarget_DragEnter,
    CIDLDropTarget_DragOver,
    CIDLDropTarget_DragLeave,
    CBitBucketIDLDropTarget_Drop,
};


void BBInitializeViewWindow(HWND hwndView)
{
    int i;

    for (i = 0 ; i < MAX_BITBUCKETS; i++)
    {
        SHChangeNotifyEntry fsne;

        fsne.fRecursive = FALSE;

        // make it if it's there so that we'll get any events
        if (MakeBitBucket(i))
        {
            UINT u;
            fsne.pidl = g_pBitBucket[i]->pidl;

            u = SHChangeNotifyRegister(hwndView,
                                       SHCNRF_NewDelivery | SHCNRF_ShellLevel | SHCNRF_InterruptLevel,
                                       SHCNE_DISKEVENTS,
                                       WM_DSV_FSNOTIFY, 
                                       1, 
                                       &fsne);

#ifdef DEBUG
            {
                TCHAR szTemp[MAX_PATH];

                SHGetPathFromIDList(fsne.pidl, szTemp);
                TraceMsg(TF_BITBUCKET, "Bitbucket: SHChangeNotifyRegister returned %d on path %s", u ,szTemp);
            }
#endif
        }
    }
}


HRESULT BBHandleFSNotify(HWND hwnd, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hres = S_OK;
    TCHAR szPath[MAX_PATH];
    LPTSTR pszFileName;

    // pidls must be child of drives or network
    // (actually only drives work for right now)
    // that way we won't get duplicate events
    if ((!ILIsParent((LPCITEMIDLIST)&c_idlDrives, pidl1, FALSE) && !ILIsParent((LPCITEMIDLIST)&c_idlNet, pidl1, FALSE)) ||
        (pidl2 && !ILIsParent((LPCITEMIDLIST)&c_idlDrives, pidl2, FALSE) && !ILIsParent((LPCITEMIDLIST)&c_idlNet, pidl2, FALSE)))
    {
        return S_FALSE;
    }

    SHGetPathFromIDList(pidl1, szPath);
    pszFileName = PathFindFileName(szPath);

    if (!lstrcmpi(pszFileName, c_szInfo2) ||
        !lstrcmpi(pszFileName, c_szInfo) ||
        !lstrcmpi(pszFileName, c_szDesktopIni))
    {
        // we ignore changes to these files because they mean we were simply doing bookeeping 
        // (eg updating the info file, re-creating the desktop.ini, etc)
        return S_FALSE;
    }

    TraceMsg(TF_BITBUCKET, "Bitbucket: BBHandleFSNotify event %x on path %s", lEvent, szPath);


    switch (lEvent)
    {
        case SHCNE_RENAMEFOLDER:
        case SHCNE_RENAMEITEM:
        {
            int idDrive;

            // if the rename's target is in a bitbucket, then do a create.
            // otherwise, return NOERROR..

            idDrive = DriveIDFromBBPath(szPath);

            if (MakeBitBucket(idDrive) && ILIsParent(g_pBitBucket[idDrive]->pidl, pidl1, TRUE))
            {
                hres = BBHandleFSNotify(hwnd, (lEvent == SHCNE_RENAMEITEM) ? SHCNE_DELETE : SHCNE_RMDIR, pidl1, NULL);
            }
        }
        break;

        case SHCNE_CREATE:
        case SHCNE_MKDIR:
        {
            LPITEMIDLIST pidl;

            pidl = DeletedFilePathToBBPidl(szPath);
            
            if (pidl)
            {
                ShellFolderView_AddObject(hwnd, pidl);
                hres = S_FALSE;
            }
        }
        break;

        case SHCNE_DELETE:
        case SHCNE_RMDIR: 
        {
            // if this was a delete into the recycle bin, pidl2 will exist
            if (pidl2)
            {
                hres = BBHandleFSNotify(hwnd, (lEvent == SHCNE_DELETE) ? SHCNE_CREATE : SHCNE_MKDIR, pidl2, NULL);
            }
            else
            {
                ShellFolderView_RemoveObject(hwnd, ILFindLastID(pidl1));
                hres = S_FALSE;
            }
        }
        break;

        case SHCNE_UPDATEDIR:
        {
            // we recieved an updatedir, which means we probably had more than 10 fsnotify events come in,
            // so we just refresh our brains out.
            ShellFolderView_RefreshAll(hwnd);
        }
        break;

        default:
        {
            // didn't handle this message
            hres = S_FALSE;
        }
        break;
    }

    return hres;
}


void BBSort(HWND hwndOwner, int id)
{
    switch(id) 
    {
        case FSIDM_SORTBYNAME:
            ShellFolderView_ReArrange(hwndOwner, 0);
            break;

        case FSIDM_SORTBYORIGIN:
            ShellFolderView_ReArrange(hwndOwner, 1);
            break;

        case FSIDM_SORTBYDELETEDDATE:
            ShellFolderView_ReArrange(hwndOwner, 2);
            break;

        case FSIDM_SORTBYTYPE:
            ShellFolderView_ReArrange(hwndOwner, 3);
            break;

        case FSIDM_SORTBYSIZE:
            ShellFolderView_ReArrange(hwndOwner, 4);
            break;
    }
}


HRESULT CALLBACK CBitBucket_DFMCallBackBG(IShellFolder *psf, HWND hwndOwner,
                IDataObject * pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CBitBucket *this = IToClass(CBitBucket, isf, psf);
    HRESULT hres = NOERROR;

    switch(uMsg)
    {
        case DFM_MERGECONTEXTMENU:
            if (!(wParam & CMF_DVFILE)) //In the case of the file menu
                CDefFolderMenu_MergeMenu(HINST_THISDLL, POPUP_BITBUCKET_BACKGROUND, POPUP_BITBUCKET_POPUPMERGE, (LPQCMINFO)lParam);
            break;

        case DFM_GETHELPTEXT:
            LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));
            break;

        case DFM_GETHELPTEXTW:
            LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));
            break;

        case DFM_INVOKECOMMAND:
            switch(wParam)
            {
                case FSIDM_SORTBYNAME:
                case FSIDM_SORTBYORIGIN:
                case FSIDM_SORTBYDELETEDDATE:
                case FSIDM_SORTBYTYPE:
                case FSIDM_SORTBYSIZE:
                    BBSort(hwndOwner, (int) wParam);
                    break;

                case FSIDM_PROPERTIESBG:
                    hres = BB_LaunchThread(NULL, hwndOwner, NULL, FSIDM_PROPERTIESBG);
                    break;

                case DFM_CMD_PASTE:
                case DFM_CMD_PROPERTIES:
                    // GetAttributesOf cidl==0 has SFGAO_HASPROPSHEET,
                    // let defcm handle this
                    hres = S_FALSE;
                    break;


                default:
                    // GetAttributesOf cidl==0 does not set _CANMOVE, _CANDELETE, etc,
                    // BUT accelerator keys will get these unavailable menu items...
                    // so we need to return failure here
                    hres = E_NOTIMPL;
                    break;
            }
            break;

        default:
            hres = E_NOTIMPL;
            break;
    }

    return hres;
}


STDMETHODIMP CBitBucket_SF_CreateViewObject(IShellFolder2 *psf, HWND hwnd, REFIID riid, void **ppvOut)
{
    HRESULT hres;
    CBitBucket *this = IToClass(CBitBucket, isf, psf);

    if (IsEqualIID(riid, &IID_IShellView))
    {
        SFV_CREATE sSFV;

        sSFV.cbSize   = sizeof(sSFV);
        sSFV.pshf     = (IShellFolder *)psf;
        sSFV.psvOuter = NULL;
        sSFV.psfvcb   = BitBucket_CreateSFVCB((IShellFolder *)psf, this);

        hres = SHCreateShellFolderView(&sSFV, (IShellView**)ppvOut);

        if (sSFV.psfvcb)
            sSFV.psfvcb->lpVtbl->Release(sSFV.psfvcb);

    }
    else if (IsEqualIID(riid, &IID_IDropTarget))
    {
        hres = CIDLDropTarget_Create(hwnd, &c_CBBDropTargetVtbl, this->pidl, (IDropTarget **)ppvOut);
    }
    else if (IsEqualIID(riid, &IID_IContextMenu))
    {
        hres = CDefFolderMenu_Create(NULL, hwnd, 0, NULL, (IShellFolder *)psf, CBitBucket_DFMCallBackBG,
                                     NULL, NULL, (IContextMenu **)ppvOut);
    }
    else
    {
        *ppvOut = NULL;
        hres = E_NOINTERFACE;
    }

    return hres;
}


STDMETHODIMP CBitBucket_SF_ParseDisplayName(IShellFolder2 *psf, HWND hwnd, LPBC pbc, 
                                            LPOLESTR pwszDisplayName, ULONG *pchEaten, LPITEMIDLIST *ppidl, ULONG * pdwAttributes)
{
    *ppidl = NULL;
    return E_NOTIMPL;
}


STDMETHODIMP CBBIDLData_QueryGetData(IDataObject * pdtobj, FORMATETC * pformatetc)
{
    ASSERT(g_cfFileNameMap);

    if (pformatetc->cfFormat == g_cfFileNameMap && (pformatetc->tymed & TYMED_HGLOBAL))
    {
        return NOERROR; // same as S_OK
    }
    return CFSIDLData_QueryGetData(pdtobj, pformatetc);
}

// subclass member function to support CF_HDROP and CF_NETRESOURCE
// in:
//      hida    bitbucket id array
//
// out:
//      HGLOBAL with double NULL terminated string list of destination names
//

HGLOBAL BuildDestSpecs(LPIDA pida)
{
    LPCITEMIDLIST pidl;
    LPBBDATAENTRYIDA pbbidl;
    LPTSTR pszRet;
    TCHAR szTemp[MAX_PATH];
    UINT i, cbAlloc = SIZEOF(TCHAR);    // for double NULL termination

    for (i = 0; pidl = IDA_GetIDListPtr(pida, i); i++)
    {
        pbbidl = PIDLTODATAENTRYID(pidl);

        BBGetOriginalPath(pbbidl, szTemp, ARRAYSIZE(szTemp));

        cbAlloc += lstrlen(PathFindFileName(szTemp)) * SIZEOF(TCHAR) + SIZEOF(TCHAR);
    }
    pszRet = LocalAlloc(LPTR, cbAlloc);
    if (pszRet)
    {
        LPTSTR pszDest = pszRet;
        for (i = 0; pidl = IDA_GetIDListPtr(pida, i); i++)
        {
            pbbidl = PIDLTODATAENTRYID(pidl);
            BBGetOriginalPath(pbbidl, szTemp, ARRAYSIZE(szTemp));
            lstrcpy(pszDest, PathFindFileName(szTemp));
            pszDest += lstrlen(pszDest) + 1;

            ASSERT((ULONG_PTR)((LPBYTE)pszDest - (LPBYTE)pszRet) < cbAlloc);
            ASSERT(*(pszDest) == 0);    // zero init alloc
        }
        ASSERT((LPTSTR)((LPBYTE)pszRet + cbAlloc - SIZEOF(TCHAR)) == pszDest);
        ASSERT(*pszDest == 0);  // zero init alloc
    }
    return pszRet;
}

extern HRESULT CFSIDLData_GetData(IDataObject * pdtobj, FORMATETC * pformatetcIn, STGMEDIUM * pmedium);

STDMETHODIMP CBBIDLData_GetData(IDataObject * pdtobj, FORMATETC * pformatetcIn, STGMEDIUM * pmedium)
{
    HRESULT hres = E_INVALIDARG;

    ASSERT(g_cfFileNameMap);

    if (pformatetcIn->cfFormat == g_cfFileNameMap && (pformatetcIn->tymed & TYMED_HGLOBAL))
    {
        STGMEDIUM medium;

        LPIDA pida = DataObj_GetHIDA(pdtobj, &medium);
        if (medium.hGlobal)
        {
            pmedium->hGlobal = BuildDestSpecs(pida);
            pmedium->tymed = TYMED_HGLOBAL;
            pmedium->pUnkForRelease = NULL;

            HIDA_ReleaseStgMedium(pida, &medium);

            hres = pmedium->hGlobal ? NOERROR : E_OUTOFMEMORY;
        }
    }
    else
    {
        hres = CFSIDLData_GetData(pdtobj, pformatetcIn, pmedium);
    }

    return hres;
}


const IDataObjectVtbl c_CBBIDLDataVtbl = {
    CIDLData_QueryInterface, CIDLData_AddRef, CIDLData_Release,
    CBBIDLData_GetData,
    CIDLData_GetDataHere,
    CBBIDLData_QueryGetData,
    CIDLData_GetCanonicalFormatEtc,
    CIDLData_SetData,
    CIDLData_EnumFormatEtc,
    CIDLData_Advise,
    CIDLData_Unadvise,
    CIDLData_EnumAdvise
};


HRESULT _CreateDefExtIcon(LPCIDFOLDER pidf, REFIID riid, void **ppxicon)
{
    HRESULT hres = E_OUTOFMEMORY;

    if (FS_IsFileFolder(pidf))
    {
        return SHCreateDefExtIcon(NULL, II_FOLDER, II_FOLDEROPEN, GIL_PERCLASS, riid, ppxicon);
    }
    else
    {
        DWORD dwFlags = SHGetClassFlags(pidf);
        if (dwFlags & SHCF_ICON_PERINSTANCE)
        {
            TCHAR szPath[MAX_PATH];
             
            BBGetItemPath(pidf, szPath);

            if (dwFlags & SHCF_HAS_ICONHANDLER)
            {
                LPITEMIDLIST pidlFull = SHSimpleIDListFromPath(szPath);
                if (pidlFull)
                {
                    // We hit this case for .lnk files in the recycle bin. We used to call FSLoadHandler
                    // but guz decided he only wanted the fstree code using that function (and it takes a fs psf
                    // nowdays anyway). So we just bind our brains out.
                    hres = SHGetUIObjectFromFullPIDL(pidlFull, NULL, riid, ppxicon);
                    ILFree(pidlFull);
                }
                else
                {
                    hres = E_OUTOFMEMORY;
                }
            }
            else
            {
                DWORD uid = FS_GetUID(pidf);

                hres = SHCreateDefExtIcon(szPath, uid, uid, GIL_PERINSTANCE | GIL_NOTFILENAME, riid, ppxicon);
            }
        }
        else
        {
            UINT iIcon = (dwFlags & SHCF_ICON_INDEX);
            hres = SHCreateDefExtIcon(c_szStar, iIcon, iIcon, GIL_PERCLASS | GIL_NOTFILENAME, riid, ppxicon);
        }
        return hres;
    }
}


STDMETHODIMP CBitBucket_GetUIObjectOf(IShellFolder2 *psf, HWND hwnd, UINT cidl, LPCITEMIDLIST *apidl,
                                      REFIID riid, UINT *prgfInOut, void **ppvOut)
{
    CBitBucket *this = IToClass(CBitBucket, isf, psf);
    HRESULT hres;

    *ppvOut = NULL;

    if (cidl && IsEqualIID(riid, &IID_IDataObject))
    {
        hres = CIDLData_CreateFromIDArray2(&c_CBBIDLDataVtbl, this->pidl, cidl, apidl, (IDataObject **)ppvOut);
    }
    else if ((cidl == 1) && 
        (IsEqualIID(riid, &IID_IExtractIconA) || IsEqualIID(riid, &IID_IExtractIconW)))
    {
        hres = _CreateDefExtIcon(FS_IsValidID(apidl[0]), riid, ppvOut);
    }
    else if (IsEqualIID(riid, &IID_IContextMenu))
    {
        hres = CDefFolderMenu_Create(this->pidl, hwnd, cidl, apidl,
            (IShellFolder *)psf, CBitBucket_DFMCallBack, NULL, NULL, (IContextMenu**)ppvOut);
    }
    else
    {
        hres = E_NOTIMPL;
    }
    return hres;
}

HRESULT FindDataFromBBPidl(LPCITEMIDLIST bbpidl, WIN32_FIND_DATAW *pfd)
{
    LPBBDATAENTRYIDA pbbdei = PIDLTODATAENTRYID(bbpidl);
    LPIDFOLDER pidf = (LPIDFOLDER)bbpidl;
    ULONGLONG cbSize;                       // 64 bits

    ZeroMemory(pfd, sizeof(*pfd));

    if (bbpidl->mkid.cb < sizeof(BBDATAENTRYIDA))
        return E_INVALIDARG;

    // this code copied from the size column in details view, so webview will match that
    if (FS_IsFolder(pidf))
    {
        cbSize = pbbdei->bbde.dwSize;       // we cache the size of the folder, rounded to cluster
    }
    else
    {
        FS_GetSize(NULL, pidf, &cbSize);
    }

    pfd->nFileSizeHigh = HIDWORD(cbSize);
    pfd->nFileSizeLow = LODWORD(cbSize);
    pfd->dwFileAttributes = pidf->fs.wAttrs; // right?

    DosDateTimeToFileTime(pidf->fs.dateModified, pidf->fs.timeModified, &pfd->ftLastWriteTime);

    return S_OK;
}

LPITEMIDLIST BBDataEntryToPidl(int idDrive, LPBBDATAENTRYW pbbde)
{
    WIN32_FIND_DATA fd;
    TCHAR chDrive;
    TCHAR szPath[MAX_PATH];
    TCHAR szDeletedPath[MAX_PATH];
    TCHAR szOriginalFileName[MAX_PATH];
    LPITEMIDLIST pidl = NULL;
    LPITEMIDLIST pidlRet = NULL;
    BBDATAENTRYIDW bbpidl;
    void *lpv;

    if (g_pBitBucket[idDrive]->fIsUnicode)
    {
        WCHAR szTemp[MAX_PATH];

        // save off the original filename so we have the extension (needed to construct the delete file name)
        SHUnicodeToTChar(pbbde->szOriginal, szOriginalFileName, ARRAYSIZE(szOriginalFileName));

        bbpidl.bbde.iIndex  = pbbde->iIndex;
        bbpidl.bbde.idDrive = pbbde->idDrive;
        bbpidl.bbde.ft      = pbbde->ft;
        bbpidl.bbde.dwSize  = pbbde->dwSize;

        SHAnsiToUnicode(pbbde->szShortName, szTemp, ARRAYSIZE(szTemp));
        
        if (StrCmpW(pbbde->szOriginal, szTemp) == 0)
        {
            // The short and long names match, so we can use an ansi pidl
            bbpidl.cb = SIZEOF(BBDATAENTRYIDA);
            lpv = &bbpidl.cb;
        }
        else
        {
            // The short and long names DO NOT match, so create a full 
            // blown unicode pidl (both ansi and unicode names)
            bbpidl.cb = SIZEOF(BBDATAENTRYIDW);
            StrCpyW(bbpidl.wszOriginal, pbbde->szOriginal);
            lpv = &bbpidl;
        }

        bbpidl.bbde = *((LPBBDATAENTRYA)pbbde);
    }
    else
    {
        LPBBDATAENTRYA pbbdea = (LPBBDATAENTRYA)pbbde;

        // save off the original filename so we have the extension (needed to construct the delete file name)
        SHAnsiToTChar(pbbdea->szOriginal, szOriginalFileName, ARRAYSIZE(szOriginalFileName));

        // just create an ansi pidl
        bbpidl.cb = SIZEOF(BBDATAENTRYIDA);
        bbpidl.bbde = *pbbdea;
        lpv = &bbpidl.cb;
    }

    chDrive = TEXT('a') + pbbde->idDrive;

    if (chDrive == (TEXT('a') + SERVERDRIVE))
    {
        chDrive = TEXT('@');
    }

    DriveIDToBBPath(idDrive, szPath);
    lstrcpyn(szDeletedPath, szPath, ARRAYSIZE(szDeletedPath));

    // create the full path to the delete file so we can get its attributes
    wsprintf(&szDeletedPath[lstrlen(szDeletedPath)], TEXT("\\D%c%d%s"), chDrive, pbbde->iIndex, PathFindExtension(szOriginalFileName));
    
    fd.dwFileAttributes = GetFileAttributes(szDeletedPath);
    
    if (fd.dwFileAttributes == -1)
    {
        TraceMsg(TF_BITBUCKET, "Bitbucket: unable to get file attributes for path %s , cannot create pidl!!", szDeletedPath);
        return NULL;
    }

    fd.ftCreationTime = pbbde->ft;
    fd.ftLastAccessTime = pbbde->ft;
    fd.ftLastWriteTime = pbbde->ft;
    fd.nFileSizeHigh = 0;
    fd.nFileSizeLow = pbbde->dwSize;
    fd.dwReserved0 = 0;
    fd.dwReserved1 = 0;
    lstrcpyn(fd.cFileName, PathFindFileName(szDeletedPath), ARRAYSIZE(fd.cFileName));  
    fd.cAlternateFileName[0] = TEXT('\0'); // no one uses this anyway...

    SHCreateFSIDList(szPath, &fd, &pidl);

    if (pidl)
    {
        UINT cbSize = ILGetSize(pidl);
        pidlRet = ILResize(pidl, cbSize + bbpidl.cb,0);
        if (pidlRet)
        {
            // Append this BBDATAENTRYID (A or W) onto the end
            memcpy(_ILSkip(pidlRet,cbSize - SIZEOF(pidl->mkid.cb)), lpv, bbpidl.cb);
            // And 0 terminate the thing
            _ILSkip(pidlRet, cbSize + bbpidl.cb - SIZEOF(pidl->mkid.cb))->mkid.cb = 0;
            // Now edit it into one larger id
            pidlRet->mkid.cb += bbpidl.cb;
            ASSERT(ILGetSize(pidlRet) == cbSize + bbpidl.cb);
        }
    }
    return pidlRet;
}


__inline int CALLBACK BBFDCompare(void *p1, void *p2, LPARAM lParam)
{
    return ((LPBBFINDDATA)p1)->iIndex - ((LPBBFINDDATA)p2)->iIndex;
}


__inline int CALLBACK BBFDIndexCompare(void *iIndex, void *p2, LPARAM lParam)
{
    return (int)((INT_PTR)iIndex - ((LPBBFINDDATA)p2)->iIndex);
}

BOOL CALLBACK BBEnumDPADestroyCallback(LPVOID pidl, LPVOID pData)
{
    ILFree(pidl);
    return TRUE;
}

LPITEMIDLIST BBEnum_GetNextPidl(LPENUMDELETED ped)
{
    LPITEMIDLIST pidlRet = FALSE;

    ASSERT(NULL != ped);
    if (NULL == ped->hdpa)
    {
        // This is the first Next() call - so snapshot the info files:
        if (NULL != (ped->hdpa = DPA_CreateEx(0, NULL)))
        {
            int iBitBucket;
            int nItem = 0;
            // loop through the bitbucket drives to find an info file
            for (iBitBucket = 0; iBitBucket < MAX_BITBUCKETS; iBitBucket++)
            {
                if (MakeBitBucket(iBitBucket)) 
                {
                    HANDLE hFile;
                    int cDeleted = 0;
                    // since we are going to start reading this bitbucket, we take the mrsw
#ifdef BB_USE_MRSW
                    MRSW_EnterRead(g_pBitBucket[iBitBucket]->pmrsw);
#endif // BB_USE_MRSW
                    hFile = OpenBBInfoFile(iBitBucket, OPENBBINFO_WRITE, 0);

                    if (INVALID_HANDLE_VALUE != hFile)
                    {
                        BBDATAENTRYW bbdew;
                        DWORD dwDataEntrySize = g_pBitBucket[iBitBucket]->fIsUnicode ? SIZEOF(BBDATAENTRYW) : SIZEOF(BBDATAENTRYA);

                        while (ReadNextDataEntry(hFile, &bbdew, FALSE, dwDataEntrySize, iBitBucket))
                        {
                            LPITEMIDLIST pidl = NULL;

                            if (IsDeletedEntry(&bbdew))
                            {
                                cDeleted++;
                            }
                            else
                            {
                                pidl = BBDataEntryToPidl(iBitBucket, &bbdew);
                            }

                            if (pidl)
                            {
                                DPA_SetPtr(ped->hdpa, nItem++, pidl);
                            }
                        }

                        if (cDeleted > BB_DELETED_ENTRY_MAX)
                        {
                            BOOL bTrue = TRUE;

                            // set the registry key so that we will compact the info file after the next delete operation
                            RegSetValueEx(g_pBitBucket[iBitBucket]->hkeyPerUser, TEXT("NeedToCompact"), 0, REG_DWORD, (LPBYTE)&bTrue, SIZEOF(bTrue));
                        }
                        CloseBBInfoFile(hFile, iBitBucket);
                    }
#ifdef BB_USE_MRSW
                    MRSW_LeaveRead(g_pBitBucket[iBitBucket]->pmrsw);
#endif // BB_USE_MRSW
                }
            }
        }
    }

    if (NULL != ped->hdpa)
    {
        pidlRet = DPA_GetPtr(ped->hdpa, ped->nItem);
        if (NULL != pidlRet)
        {
            // We're returning an allocated pidl, so replace the pointer
            // in the DPA with NULL so that we don't free it later:
            DPA_SetPtr(ped->hdpa, ped->nItem, NULL);
        }
        else
        {
            // We've reached the end, so destroy our snapshot:
            DPA_DestroyCallback(ped->hdpa, BBEnumDPADestroyCallback, NULL);
            ped->hdpa = NULL;
        }
        ped->nItem++;
    }

    return pidlRet;
}

//
// To be called back from within SHCreateEnumObjects
//
HRESULT CALLBACK CBitBucket_EnumCallBack(LPARAM lParam, void *pvData, UINT ecid, UINT index)
{
    HRESULT hres = NOERROR;
    ENUMDELETED * ped = (ENUMDELETED *)pvData;

    switch (ecid)
    {
        case ECID_SETNEXTID:
        {
            LPITEMIDLIST pidl;

            if (!(ped->grfFlags & SHCONTF_NONFOLDERS))
                return S_FALSE; //  "no more element"

            pidl = BBEnum_GetNextPidl(ped);

            if (!pidl)
            {
                hres = S_FALSE; //  "no more element"
            }
            else
            {
                CDefEnum_SetReturn(lParam, pidl);

                TraceMsg(TF_BITBUCKET, "Bitbucket: EnumCallBack,  returns %S", PIDLTODATAENTRYID(pidl)->bbde.szOriginal);
                
                //hres = NOERROR; // in success
            }
            break;
        }
        
        case ECID_RESET:
            if (NULL != ped->hdpa)
            {
                DPA_DestroyCallback(ped->hdpa, BBEnumDPADestroyCallback, NULL);
                ped->hdpa = NULL;
            }
            ped->nItem = 0;
            break;

        case ECID_RELEASE:
            if (NULL != ped->hdpa)
            {
                DPA_DestroyCallback(ped->hdpa, BBEnumDPADestroyCallback, NULL);
            }

            LocalFree((HLOCAL)ped);
            break;
    }

    return hres;
}


STDMETHODIMP CBitBucket_SF_EnumObjects(IShellFolder2 *psf, HWND hwndOwner,
            DWORD grfFlags, LPENUMIDLIST * ppenumUnknown)
{
    CBitBucket *this = IToClass(CBitBucket, isf, psf);
    ENUMDELETED * ped = (void*)LocalAlloc(LPTR, SIZEOF(ENUMDELETED));
    if (ped) 
    {
        ped->grfFlags = grfFlags;
        ped->pbb = this;
        return SHCreateEnumObjects(hwndOwner, ped, CBitBucket_EnumCallBack, ppenumUnknown);
    }
    return E_OUTOFMEMORY;
}


STDMETHODIMP CBitBucket_BindToObject(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}


STDMETHODIMP CBitBucket_BindToStorage(IShellFolder2 *psf, LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;
    return E_NOTIMPL;
}


// get the path name of the file in the file system
void BBGetItemPath(LPCIDFOLDER pidf, LPTSTR pszPath)
{
    TCHAR szFile[MAX_PATH];
    LPBBDATAENTRYIDA pbbidl = PIDLTODATAENTRYID(pidf);

    DriveIDToBBPath(pbbidl->bbde.idDrive, pszPath);
    PathAppend(pszPath, FS_CopyName(pidf, szFile, ARRAYSIZE(szFile)));
}


// get the friendly looking name of this file
void BBGetDisplayName(LPCIDFOLDER pidf, LPTSTR pszName)
{
    TCHAR szTemp[MAX_PATH];
    LPBBDATAENTRYIDA pbbid = PIDLTODATAENTRYID(pidf);

    BBGetOriginalPath(pbbid, szTemp, ARRAYSIZE(szTemp));
    lstrcpy(pszName, PathFindFileName(szTemp));

    if (!FS_ShowExtension(pidf))
        PathRemoveExtension(pszName);
}


STDMETHODIMP CBitBucket_SF_GetDisplayNameOf(IShellFolder2 *psf, LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET pStrRet)
{
    LPCIDFOLDER pidf;

    pStrRet->uType = STRRET_CSTR;
    pStrRet->cStr[0] = 0;

    pidf = FS_IsValidID(pidl);
    if (pidf)
    {
        TCHAR szName[MAX_PATH];

        if (uFlags & SHGDN_FORPARSING)
            BBGetItemPath(pidf, szName);
        else
            BBGetDisplayName(pidf, szName);

        return StringToStrRet(szName, pStrRet);
    }
    return E_INVALIDARG;
}


STDMETHODIMP CBitBucket_SetNameOf(IShellFolder2 *psf, HWND hwnd,
        LPCITEMIDLIST pidl, LPCOLESTR lpszName, DWORD dwReserved, LPITEMIDLIST * ppidlOut)
{
    return E_FAIL;
}


STDMETHODIMP CBitBucket_EnumSearches(IShellFolder2 *psf, LPENUMEXTRASEARCH *ppenum)
{
    *ppenum = NULL;
    return E_NOTIMPL;
}


STDMETHODIMP CBitBucket_GetDefaultColumn(IShellFolder2 *psf, DWORD dwRes, ULONG *pSort, ULONG *pDisplay)
{
    return E_NOTIMPL;
}


STDMETHODIMP CBitBucket_GetDefaultColumnState(IShellFolder2 *psf, UINT iColumn, DWORD *pbState)
{
    return E_NOTIMPL;
}


STDMETHODIMP CBitBucket_GetDetailsEx(IShellFolder2 *psf, LPCITEMIDLIST pidl, const SHCOLUMNID *pscid, VARIANT *pv)
{
    CBitBucket *this = IToClass(CBitBucket, isf, psf);
    HRESULT hres = E_NOTIMPL;
    if (IsEqualSCID(*pscid, SCID_FINDDATA))
    {
        WIN32_FIND_DATAW wfd;
        hres = FindDataFromBBPidl(pidl, &wfd);

        if (SUCCEEDED(hres))
        {
            hres = InitVariantFromBuffer(pv, (PVOID)&wfd, sizeof(wfd));
        }
    }
    return hres;
}


STDMETHODIMP CBitBucket_GetDetailsOf(IShellFolder2 *psf, LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS *pDetails)
{
    CBitBucket *this = IToClass(CBitBucket, isf, psf);
    HRESULT hres = NOERROR;

    if (iColumn >= ARRAYSIZE(c_bb_cols))
        return E_NOTIMPL;

    pDetails->str.uType = STRRET_CSTR;
    pDetails->str.cStr[0] = 0;

    if (!pidl) 
    {
        // getting the headers
        hres = ResToStrRet(c_bb_cols[iColumn].ids, &pDetails->str);
        pDetails->fmt = c_bb_cols[iColumn].iFmt;
        pDetails->cxChar = c_bb_cols[iColumn].cchCol;
    } 
    else 
    {
        TCHAR  szTemp[MAX_PATH];
        LPCIDFOLDER pidf = FS_IsValidID(pidl);
        UNALIGNED BBDATAENTRYIDA * pbbidl = PIDLTODATAENTRYID(pidl);

        switch (iColumn) {
        case ICOL_NAME:
            BBGetDisplayName(pidf, szTemp);
            hres = StringToStrRet(szTemp, &pDetails->str);
            break;

        case ICOL_SIZE:
            {
                ULONGLONG cbSize;

                if (FS_IsFolder(pidf))
                    cbSize = pbbidl->bbde.dwSize;
                else
                    FS_GetSize(NULL, pidf, &cbSize);

                StrFormatKBSize(cbSize, szTemp, ARRAYSIZE(szTemp));
                hres = StringToStrRet(szTemp, &pDetails->str);
            }
            break;

        case ICOL_ORIGINAL:
            BBGetOriginalPath(pbbidl, szTemp, ARRAYSIZE(szTemp));
            PathRemoveFileSpec(szTemp);
            hres = StringToStrRet(szTemp, &pDetails->str);
            break;

        case ICOL_TYPE:
            FS_GetTypeName(pidf, szTemp, ARRAYSIZE(szTemp));
            hres = StringToStrRet(szTemp, &pDetails->str);
            break;

        case ICOL_MODIFIED:
            {
            // need stack ft since pbbidl is UNALIGNED
            FILETIME ft = pbbidl->bbde.ft;
            DWORD dwFlags = FDTF_DEFAULT;

            switch (pDetails->fmt)
            {
                case LVCFMT_LEFT_TO_RIGHT :
                    dwFlags |= FDTF_LTRDATE;
                break;

                case LVCFMT_RIGHT_TO_LEFT :
                    dwFlags |= FDTF_RTLDATE;
                break;
            }

            SHFormatDateTime(&ft, &dwFlags, szTemp, ARRAYSIZE(szTemp));
            hres = StringToStrRet(szTemp, &pDetails->str);
            break;
            }
        }
    }
    return hres;
}


STDMETHODIMP CBitBucket_MapColumnToSCID(IShellFolder2 *psf, UINT iColumn, SHCOLUMNID *pscid)
{
    return MapColumnToSCIDImpl(c_bb_cols, ARRAYSIZE(c_bb_cols), iColumn, pscid);
}


IShellFolder2Vtbl c_CBitBucketSFVtbl =
{
    CBitBucket_SF_QueryInterface, CBitBucket_SF_AddRef, CBitBucket_SF_Release,
    CBitBucket_SF_ParseDisplayName,
    CBitBucket_SF_EnumObjects,
    CBitBucket_BindToObject,
    CBitBucket_BindToStorage,
    CBitBucket_SF_CompareIDs,
    CBitBucket_SF_CreateViewObject,
    CBitBucket_SF_GetAttributesOf,
    CBitBucket_GetUIObjectOf,
    CBitBucket_SF_GetDisplayNameOf,
    CBitBucket_SetNameOf,
    FindFileOrFolders_GetDefaultSearchGUID,
    CBitBucket_EnumSearches,
    CBitBucket_GetDefaultColumn,
    CBitBucket_GetDefaultColumnState,
    CBitBucket_GetDetailsEx,
    CBitBucket_GetDetailsOf,
    CBitBucket_MapColumnToSCID,
};

//========================================================================
// CBitBucket's PersistFile  members
//========================================================================

STDMETHODIMP CBitBucket_PF_QueryInterface(IPersistFolder2 *ppf, REFIID riid, void **ppvObj)
{
    CBitBucket *this = IToClass(CBitBucket, ipf, ppf);
    return CBitBucket_SF_QueryInterface(&this->isf, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CBitBucket_PF_Release(IPersistFolder2 *ppf)
{
    CBitBucket *this = IToClass(CBitBucket, ipf, ppf);
    return CBitBucket_SF_Release(&this->isf);
}

STDMETHODIMP_(ULONG) CBitBucket_PF_AddRef(IPersistFolder2 *ppf)
{
    CBitBucket *this = IToClass(CBitBucket, ipf, ppf);
    return CBitBucket_SF_AddRef(&this->isf);
}

STDMETHODIMP CBitBucket_PF_GetClassID(IPersistFolder2 *ppf, LPCLSID lpClassID)
{
    *lpClassID = CLSID_RecycleBin;
    return NOERROR;
}

STDMETHODIMP CBitBucket_PF_Initialize(IPersistFolder2 *ppf, LPCITEMIDLIST pidl)
{
    CBitBucket *this = IToClass(CBitBucket, ipf, ppf);
    ASSERT(this->pidl == NULL);
    this->pidl = ILClone(pidl);
    return this->pidl ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CBitBucket_PF_GetCurFolder(IPersistFolder2 *ppf, LPITEMIDLIST *ppidl)
{
    CBitBucket *this = IToClass(CBitBucket, ipf, ppf);
    return GetCurFolderImpl(this->pidl, ppidl);
}

IPersistFolder2Vtbl c_CBitBucketPFVtbl =
{
    CBitBucket_PF_QueryInterface, CBitBucket_PF_AddRef, CBitBucket_PF_Release,
    CBitBucket_PF_GetClassID,
    CBitBucket_PF_Initialize,
    CBitBucket_PF_GetCurFolder
};


STDMETHODIMP CBitBucket_SEI_QueryInterface(IShellExtInit* psei, REFIID riid, void **ppvObj)
{
    CBitBucket *this = IToClass(CBitBucket, isei, psei);
    return CBitBucket_SF_QueryInterface(&this->isf, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CBitBucket_SEI_Release(IShellExtInit* psei)
{
    CBitBucket *this = IToClass(CBitBucket, isei, psei);
    return CBitBucket_SF_Release(&this->isf);
}

STDMETHODIMP_(ULONG) CBitBucket_SEI_AddRef(IShellExtInit* psei)
{
    CBitBucket *this = IToClass(CBitBucket, isei, psei);
    return CBitBucket_SF_AddRef(&this->isf);
}

STDMETHODIMP_(ULONG) CBitBucket_SEI_Initialize(IShellExtInit* psei,
                                                    LPCITEMIDLIST pidlFolder,
                                                    IDataObject * pdtobj, HKEY hkeyProgID)
{
    return NOERROR;
}



STDMETHODIMP CBitBucket_CM_QueryInterface(IContextMenu* pcm, REFIID riid, void **ppvObj)
{
    CBitBucket *this = IToClass(CBitBucket, icm, pcm);
    return CBitBucket_SF_QueryInterface(&this->isf, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CBitBucket_CM_Release(IContextMenu* pcm)
{
    CBitBucket *this = IToClass(CBitBucket, icm, pcm);
    return CBitBucket_SF_Release(&this->isf);
}

STDMETHODIMP_(ULONG) CBitBucket_CM_AddRef(IContextMenu* pcm)
{
    CBitBucket *this = IToClass(CBitBucket, icm, pcm);
    return CBitBucket_SF_AddRef(&this->isf);
}


STDMETHODIMP CBitBucket_QueryContextMenu(IContextMenu * pcm,
        HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast,
        UINT uFlags)
{
    int idMax = idCmdFirst;
    HMENU hmMerge = SHLoadPopupMenu(HINST_THISDLL, POPUP_BITBUCKET_POPUPMERGE);
    
    if (hmMerge)
    {
        idMax = Shell_MergeMenus(hmenu, hmMerge, indexMenu, idCmdFirst, idCmdLast, 0);
        
        if (IsRecycleBinEmpty())
        {
            EnableMenuItem(hmenu, idCmdFirst + FSIDM_PURGEALL, MF_GRAYED | MF_BYCOMMAND);
        }

        DestroyMenu(hmMerge);
    }

    return ResultFromShort(idMax - idCmdFirst);
}

STDMETHODIMP CBitBucket_InvokeCommand(IContextMenu* pcm,
                                           LPCMINVOKECOMMANDINFO pici)
{
    CBitBucket *this = IToClass(CBitBucket, icm, pcm);

    TraceMsg(TF_BITBUCKET, "Bitbucket: BitBucket_invokeCommand %d %d", pici->lpVerb, FSIDM_PURGEALL);

    switch ((ULONG_PTR)pici->lpVerb)
    {
    case FSIDM_PURGEALL:
        BBPurgeAll(this, pici->hwnd, 0);
        break;
    }
    return NOERROR;
}

STDMETHODIMP CBitBucket_GetCommandString(IContextMenu *pcm,
        UINT_PTR idCmd, UINT  wFlags, UINT * pwReserved, LPSTR pszName, UINT cchMax)
{
    TraceMsg(TF_BITBUCKET, "Bitbucket: GetCommandString, idCmd = %d", idCmd);

    switch(wFlags)
    {
        case GCS_VERBA:
        case GCS_VERBW:
            return GetVerb(idCmd, pszName, cchMax, wFlags == GCS_VERBW);

        case GCS_HELPTEXTA:
            return LoadStringA(HINST_THISDLL,
                              (UINT)(idCmd + IDS_MH_FSIDM_FIRST),
                              pszName, cchMax) ? NOERROR : E_OUTOFMEMORY;
        case GCS_HELPTEXTW:
            return LoadStringW(HINST_THISDLL,
                              (UINT)(idCmd + IDS_MH_FSIDM_FIRST),
                              (LPWSTR)pszName, cchMax) ? NOERROR : E_OUTOFMEMORY;
        default:
            return E_NOTIMPL;
    }
}

STDMETHODIMP CBitBucket_PS_QueryInterface(LPSHELLPROPSHEETEXT pps, REFIID riid,
                                        void **ppvObj)
{
    CBitBucket *this = IToClass(CBitBucket, ips, pps);
    return CBitBucket_SF_QueryInterface(&this->isf, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CBitBucket_PS_Release(LPSHELLPROPSHEETEXT pps)
{
    CBitBucket *this = IToClass(CBitBucket, ips, pps);
    return CBitBucket_SF_Release(&this->isf);
}

STDMETHODIMP_(ULONG) CBitBucket_PS_AddRef(LPSHELLPROPSHEETEXT pps)
{
    CBitBucket *this = IToClass(CBitBucket, ips, pps);
    return CBitBucket_SF_AddRef(&this->isf);
}

//
//  Callback function that saves the location of the HPROPSHEETPAGE's
//  LPPROPSHEETPAGE so we can pass it to other propsheet pages.
//
UINT CALLBACK BBGlobalSettingsCalback(HWND hwnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
{
    LPBBPROPSHEETINFO ppsiTemplate;
    LPBBPROPSHEETINFO ppsiGlobal;

    switch (uMsg)
    {
        case PSPCB_ADDREF:
            // we save off the address of the "real" ppsi in the pGlobal param of the
            // the template, so that the other drives can get to the global page information
            ppsiGlobal = (LPBBPROPSHEETINFO)ppsp;
            ppsiTemplate = (LPBBPROPSHEETINFO)ppsp->lParam;
            ppsiTemplate->pGlobal = ppsiGlobal;
            ppsiGlobal->pGlobal = ppsiGlobal;
            break;

        case PSPCB_CREATE:
            return TRUE;                    // Yes, please create me
    }
    return 0;
}

STDMETHODIMP CBitBucket_PS_AddPages(IShellPropSheetExt * pspx,
                                    LPFNADDPROPSHEETPAGE lpfnAddPage,
                                    LPARAM lParam)
{
    HPROPSHEETPAGE hpage;
    int idDrive;
    int iPage;
    BBPROPSHEETINFO bbpsp;
    TCHAR szTitle[MAX_PATH];
    DWORD dwSize1, dwSize2, dwSize3;
    
    // read in the global settings
    dwSize1 = SIZEOF(bbpsp.fUseGlobalSettings);
    dwSize2 = SIZEOF(bbpsp.iOriginalPercent);
    dwSize3 = SIZEOF(bbpsp.fOriginalNukeOnDelete);
    if (RegQueryValueEx(g_hkBitBucket, TEXT("UseGlobalSettings"), NULL, NULL, (LPBYTE)&bbpsp.fOriginalUseGlobalSettings, &dwSize1) != ERROR_SUCCESS ||
        RegQueryValueEx(g_hkBitBucket, TEXT("Percent"), NULL, NULL, (LPBYTE)&bbpsp.iOriginalPercent, &dwSize2) != ERROR_SUCCESS ||
        RegQueryValueEx(g_hkBitBucket, TEXT("NukeOnDelete"), NULL, NULL, (LPBYTE)&bbpsp.fOriginalNukeOnDelete, &dwSize3) != ERROR_SUCCESS)
    {
        ASSERTMSG(FALSE, "Bitbucket: could not read global settings from the registry, re-regsvr32 shell32.dll!!");
        bbpsp.fUseGlobalSettings = TRUE;
        bbpsp.iOriginalPercent = 10;
        bbpsp.fOriginalNukeOnDelete = FALSE;
    }

    bbpsp.fUseGlobalSettings = bbpsp.fOriginalUseGlobalSettings;
    bbpsp.fNukeOnDelete = bbpsp.fOriginalNukeOnDelete;
    bbpsp.iPercent = bbpsp.iOriginalPercent;

    bbpsp.psp.dwSize = SIZEOF(bbpsp);
    bbpsp.psp.dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
    bbpsp.psp.hInstance = HINST_THISDLL;
    bbpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_BITBUCKET_GENCONFIG);
    bbpsp.psp.pfnDlgProc = BBGlobalPropDlgProc;
    bbpsp.psp.lParam = (LPARAM)&bbpsp;
    // the callback will fill the bbpsp.pGlobal with the pointer to the "real" psp after it has been copied
    // so that the other drive pages can get to the global information
    bbpsp.psp.pfnCallback = BBGlobalSettingsCalback;

    // add the "Global" settings page
    hpage = CreatePropertySheetPage(&bbpsp.psp);

#ifdef UNICODE
    // If this assertion fires, it means that comctl32 lost
    // backwards-compatibility with Win95 shell, WinNT4 shell,
    // and IE4 shell, all of which relied on this undocumented
    // behavior.
    ASSERT(bbpsp.pGlobal == (LPBBPROPSHEETINFO)((LPBYTE)hpage + 2 * sizeof(LPVOID)));
#else
    ASSERT(bbpsp.pGlobal == (LPBBPROPSHEETINFO)hpage);
#endif

    lpfnAddPage(hpage, lParam);

    // now create the pages for the individual drives
    bbpsp.psp.dwFlags = PSP_USETITLE;
    bbpsp.psp.pszTemplate = MAKEINTRESOURCE(DLG_BITBUCKET_CONFIG);
    bbpsp.psp.pfnDlgProc = BBDriveDlgProc;
    bbpsp.psp.pszTitle = szTitle;

    for (idDrive = 0, iPage = 1; (idDrive < MAX_BITBUCKETS) && (iPage < MAXPROPPAGES); idDrive++)
    {
        if (MakeBitBucket(idDrive))
        {
            dwSize1 = SIZEOF(bbpsp.iOriginalPercent);
            dwSize2 = SIZEOF(bbpsp.fOriginalNukeOnDelete);
            if (RegQueryValueEx(g_pBitBucket[idDrive]->hkey, TEXT("Percent"), NULL, NULL, (LPBYTE)&bbpsp.iOriginalPercent, &dwSize1) != ERROR_SUCCESS ||
                RegQueryValueEx(g_pBitBucket[idDrive]->hkey, TEXT("NukeOnDelete"), NULL, NULL, (LPBYTE)&bbpsp.fOriginalNukeOnDelete, &dwSize2) != ERROR_SUCCESS)
            {
                TraceMsg(TF_BITBUCKET, "Bitbucket: could not read settings from the registry for drive %d, using lame defaults", idDrive);
                bbpsp.iOriginalPercent = 10;
                bbpsp.fNukeOnDelete = FALSE;
            }

            bbpsp.iPercent = bbpsp.iOriginalPercent;
            bbpsp.fNukeOnDelete = bbpsp.fOriginalNukeOnDelete;

            bbpsp.idDrive = idDrive;

            BBGetDriveDisplayName(idDrive, szTitle, ARRAYSIZE(szTitle));
            hpage = CreatePropertySheetPage(&bbpsp.psp);
            lpfnAddPage(hpage, lParam);
        }
    }

    return NOERROR;
}


IContextMenuVtbl c_CBitBucketCMVtbl =
{
    CBitBucket_CM_QueryInterface,
    CBitBucket_CM_AddRef,
    CBitBucket_CM_Release,
    CBitBucket_QueryContextMenu,
    CBitBucket_InvokeCommand,
    CBitBucket_GetCommandString
};

IShellPropSheetExtVtbl c_CBitBucketPSVtbl =
{
    CBitBucket_PS_QueryInterface,
    CBitBucket_PS_AddRef,
    CBitBucket_PS_Release,
    CBitBucket_PS_AddPages,
    CCommonShellPropSheetExt_ReplacePage,
};

IShellExtInitVtbl c_CBitBucketSEIVtbl =
{
    CBitBucket_SEI_QueryInterface,
    CBitBucket_SEI_AddRef,
    CBitBucket_SEI_Release,
    CBitBucket_SEI_Initialize
};

HRESULT CBitBucket_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres;
    CBitBucket *pbb = (void*)LocalAlloc(LPTR, SIZEOF(CBitBucket));
    if (pbb && InitBBGlobals())
    {
        pbb->isf.lpVtbl  = &c_CBitBucketSFVtbl;
        pbb->ipf.lpVtbl  = &c_CBitBucketPFVtbl;
        pbb->icm.lpVtbl  = &c_CBitBucketCMVtbl;
        pbb->isei.lpVtbl = &c_CBitBucketSEIVtbl;
        pbb->ips.lpVtbl  = &c_CBitBucketPSVtbl;
        pbb->cRef = 1;
        hres = CBitBucket_SF_QueryInterface(&pbb->isf, riid, ppvOut);
        CBitBucket_SF_Release(&pbb->isf);
    }
    else
    {
        *ppvOut = NULL;
        hres = E_OUTOFMEMORY;
    }
    return hres;
}


//
// takes a full path to a file in a bucket and creates a pidl for it.
//
LPITEMIDLIST DeletedFilePathToBBPidl(LPTSTR pszPath)
{
    BBDATAENTRYW bbdew;

    LPITEMIDLIST pidl = NULL;
    int idDrive = DriveIDFromBBPath(pszPath);
    int iIndex;
    DWORD dwDataEntrySize;
    HANDLE hFile;

    ASSERT(idDrive >= 0);       // general UNC case will generate -1

    iIndex = BBPathToIndex(pszPath);

    if (iIndex == -1)
        return NULL;

    dwDataEntrySize = g_pBitBucket[idDrive]->fIsUnicode ? SIZEOF(BBDATAENTRYW) : SIZEOF(BBDATAENTRYA);

#ifdef BB_USE_MRSW
    MRSW_EnterRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW
    
    hFile = OpenBBInfoFile(idDrive, OPENBBINFO_WRITE, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        // read records (skipping deleted)
        // until we find an index match
        while(ReadNextDataEntry(hFile, &bbdew, TRUE, dwDataEntrySize, idDrive))
        {
            if (bbdew.iIndex == iIndex)
            {
                pidl = BBDataEntryToPidl(idDrive, &bbdew);
                break;
            }
        }
        CloseBBInfoFile(hFile, idDrive);
    }
    
#ifdef BB_USE_MRSW
    MRSW_LeaveRead(g_pBitBucket[idDrive]->pmrsw);
#endif // BB_USE_MRSW

    return pidl;
}


BOOL_PTR CALLBACK NewDiskFullDlgProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {
    case WM_INITDIALOG:
    {
        int idDrive = (int)lParam;
        TCHAR szNewText[MAX_PATH];
        TCHAR szText[MAX_PATH];

        GetDlgItemText(hDlg, IDD_TEXT, szText, ARRAYSIZE(szText));
        wsprintf(szNewText, szText, TEXT('A') + idDrive);
        SetDlgItemText(hDlg, IDD_TEXT, szNewText);

        break;
    }

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wParam, lParam)) {
        case IDC_DISKFULL_CLEANUP:
        case IDCANCEL:
            EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
            break;
        }
        break;

    default:
        return FALSE;
    }
    return TRUE;
}


void WINAPI HandleDiskFull(HWND hwnd, int idDrive)
{
    INT_PTR ret;
    if ((idDrive >= 0) && (idDrive < MAX_DRIVES))
    {
       if (IsBitBucketableDrive(idDrive)) 
       {
          ret = DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_DISKFULL_NEW),
                            hwnd, NewDiskFullDlgProc, (LPARAM)idDrive);
          switch(ret)
          {
             case IDC_DISKFULL_CLEANUP:
                LaunchDiskCleanup(hwnd, idDrive);
                break;


             default:
                break;
           }
       }
    }
}


STDAPI_(void) SHHandleDiskFull(HWND hwnd, int idDrive)
{

    // We will only do anything if noone has created the following named event 
    HANDLE hDisable = OpenEvent(EVENT_ALL_ACCESS, FALSE, TEXT("DisableLowDiskWarning"));
    if (!hDisable)
    {
        if (GetDiskCleanupPath(NULL, 0) && IsBitBucketableDrive(idDrive))
        {
            HandleDiskFull(hwnd, idDrive);
        }
    }
    else
    {
       CloseHandle(hDisable);
    }
}
