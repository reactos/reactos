#include "shellprv.h"

extern "C" {
#include <shellp.h>
#include <shguidp.h>
#include "idlcomm.h"
#include "pidl.h"
#include "fstreex.h"
#include "views.h"
#include "ids.h"
#include "shitemid.h"

#include "brfcasep.h"
} ;

#include "recdocs.h"

#include "sfviewp.h"
#include "brfcase.h"
#include "datautil.h"

void _UpdateDiskFreeSpace(FSSELCHANGEINFO *pfssci)
{
    char szPath[10];
    ULARGE_INTEGER qwTotalFreeCaller, qwDontCare1, qwDontCare2;

    PathBuildRootA(szPath, pfssci->idDrive);

    if (SHGetDiskFreeSpaceExA(szPath, &qwTotalFreeCaller, &qwDontCare1, &qwDontCare2))
        pfssci->cbFree = qwTotalFreeCaller.QuadPart;
}


BOOL IsExplorerModeBrowser(IUnknown *psite)
{
    BOOL bRes = FALSE;
    IShellBrowser *psb;
    if (SUCCEEDED(IUnknown_QueryService(psite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb))))
    {
        HWND hwndTree;
        bRes = SUCCEEDED(psb->GetControlWindow(FCW_TREE, &hwndTree)) && hwndTree;
        psb->Release();
    }
    return bRes;
}


void FSShowNoSelectionState(IUnknown *psite, FSSELCHANGEINFO *pfssci)
{
    TCHAR szTemp[30], szTempHidden[30], szFreeSpace[30];
    UINT ids = IDS_FSSTATUSBASE;

    // Assume we don't need freespace info
    szFreeSpace[0] = TEXT('\0');

    // See if we need the freespace info (idDrive != -1)
    ULONGLONG cbFree = -1;
    if (pfssci->idDrive != -1 && IsExplorerModeBrowser(psite))
    {
        if (pfssci->cbFree == -1)
            _UpdateDiskFreeSpace(pfssci);

        // cbFree couldstill be -1 if GetDiskFreeSpace didn't get any info
        cbFree = pfssci->cbFree;
        if (cbFree != -1)
        {
            ShortSizeFormat64(pfssci->cbFree, szFreeSpace);
            ids += DIDS_FSSPACE;            // Also show freespace
        }
    }

    //
    //  If there are hidden files, then show "and nn hidden".
    //
    if (pfssci->cHiddenFiles)
        ids += DIDS_FSHIDDEN;

    // Get the status string
    LPTSTR pszStatus = ShellConstructMessageString(HINST_THISDLL, (LPCTSTR)ids,
                AddCommas(pfssci->cFiles, szTemp),
                AddCommas(pfssci->cHiddenFiles, szTempHidden),
                szFreeSpace);

    // Get the size portion
    StrFormatByteSize64(pfssci->cbSize, szTemp, ARRAYSIZE(szTemp));

    LPCTSTR rgpsz[] = { pszStatus, szTemp };
    SetStatusText(psite, rgpsz, 0, 1);

    if (pszStatus)
        LocalFree(pszStatus);
}

void FSShowSelectionState(IUnknown *psite, FSSELCHANGEINFO *pfssci)
{
    TCHAR szTemp[20], szBytes[30];
    LPTSTR pszStatus = NULL;

    if (pfssci->nItems > 1)
    {
        pszStatus = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_FSSTATUSSELECTED),
                            AddCommas(pfssci->nItems, szTemp));
    }

    if (pfssci->cNonFolders)
        ShortSizeFormat64(pfssci->cbBytes, szBytes);
    else
        szBytes[0] = 0;

    LPCTSTR rgpsz[] = { pszStatus, szBytes };
    SetStatusText(psite, rgpsz, 0, 1);

    if (pszStatus)
        LocalFree(pszStatus);
}


HRESULT FSUpdateStatusBar(IUnknown *psite, FSSELCHANGEINFO *pfssci)
{
    IShellBrowser *psb;
    HRESULT hres = IUnknown_QueryService(psite, SID_STopLevelBrowser, IID_IShellBrowser, (void **)&psb);
    if (SUCCEEDED(hres))
    {
        hres = S_OK;
        switch (pfssci->nItems)
        {
        case 0:
            FSShowNoSelectionState(psite, pfssci);
            hres = S_OK;
            break;

        case 1:
            FSShowSelectionState(psite, pfssci); //Set the Size only.
            hres = SFVUSB_INITED;   // Make defview set infotip as text
            break;

        default:
            FSShowSelectionState(psite, pfssci);
            hres = S_OK;
            break;

        }

        psb->Release();
    }
    return hres;
}

void FSOnInsertDeleteItem(LPCITEMIDLIST pidlParent, FSSELCHANGEINFO *pfssci, LPCITEMIDLIST pidl, int iMul)
{
    LPCIDFOLDER pidf = FS_IsValidID(pidl);
    if (pidf) 
    {
        ULONGLONG ull;

        FS_GetSize(pidlParent, pidf, &ull);
        pfssci->cFiles += iMul;
        pfssci->cbSize += iMul * ull;
        if (pfssci->cFiles <= 0)
        {
            pfssci->cbSize = 0;
            pfssci->cFiles = 0;
        }
    } 
    else 
    {
        // means a delete all
        pfssci->cFiles = 0;
        pfssci->cbSize = 0;
        pfssci->nItems = 0;
        pfssci->cbBytes = 0;
        pfssci->cNonFolders = 0;
        pfssci->cHiddenFiles = 0;
    }
}

void FSOnSelChange(LPCITEMIDLIST pidlParent, SFVM_SELCHANGE_DATA* pdvsci, FSSELCHANGEINFO *pfssci)
{
    LPCIDFOLDER pidf = FS_IsValidID((LPCITEMIDLIST)pdvsci->lParamItem);
    if (pidf)
    {
        int iMul = -1;
        ULONGLONG cbSize;

        // Update selection count
        if (pdvsci->uNewState & LVIS_SELECTED)
            iMul = 1;
        else
            ASSERT(0 != pfssci->nItems);

        // assert that soemthing changed
        ASSERT((pdvsci->uOldState & LVIS_SELECTED) != (pdvsci->uNewState & LVIS_SELECTED));

        pfssci->nItems += iMul;
        FS_GetSize(pidlParent, pidf, &cbSize);

        pfssci->cbBytes += (iMul * cbSize);
        if (!FS_IsFolder(pidf))
            pfssci->cNonFolders += iMul;
    }
}

STDAPI CFSFolder_GetCCHMax(CFSFolder *that, LPCIDFOLDER pidf, UINT *pcchMax)
{
    TCHAR szPath[MAX_PATH];
    DWORD dwMaxLength;

    //
    // Get the maximum file name length.
    //  MAX_PATH - ('\\' + '\0' + 1) - lstrlen(szParent)
    //  (the -1 above is b/c the copyengine only supports MAX_PATH-2).
    //  And of course if the file system does not support
    //  long file names we should also restrict it somemore.
    //
    CFSFolder_GetPathForItem(that, NULL, szPath);
    *pcchMax = ARRAYSIZE(szPath) - 3 - lstrlen(szPath);

    // Now make sure that that size is valid for the
    // type of drive that we are talking to
    PathStripToRoot(szPath);

    if (GetVolumeInformation(szPath, NULL, 0, NULL, &dwMaxLength, NULL, NULL, 0))
    {
        if (*pcchMax > (int)dwMaxLength)
        {
            // can't be loner than the FS allows
            *pcchMax = (int)dwMaxLength;
        }
    }
    else
    {
        dwMaxLength = 255;  // Assume LFN for now...
    }

    //
    // Adjust the cchMax if we are hiding the extension
    //
    if (pidf)
    {
        FS_CopyName(pidf, szPath, ARRAYSIZE(szPath));
        UINT cchCur = lstrlen(szPath);

        // If our code above restricted smaller than current size reset
        // back to current size...
        if (*pcchMax < cchCur)
        {
            // NOTE: this is bullshit. if we ended up with a max smaller than what
            // we currently have, the we are clearly doing something wrong!
            *pcchMax = cchCur;
        }

        if (!FS_ShowExtension(pidf))
        {
            *pcchMax -= lstrlen(PathFindExtension(szPath));
            if ((dwMaxLength <= 12) && (*pcchMax > 8))
            {
                // if max filesys path is <=12, then assume we are dealing w/ 8.3 names
                // and since we are not showing the extension, cap the filename at 8.
                *pcchMax = 8;
            }
        }


        if (FS_IsFolder(pidf))
        {
#ifdef WINNT
            // On NT, a directory must be able to contain an
            // 8.3 name and STILL be less than MAX_PATH.  The
            // "12" below is the length of an 8.3 name (8+1+3).
            *pcchMax -= 12;
#else
            // on win9x, we make sure that there is enough room
            // to append \*.* in the directory case.
            *pcchMax -= 4;
#endif
        }

        ASSERT((int)*pcchMax > 0);
    }
    return S_OK;
}

class CFSFolderShellFolderViewCB : public CBaseShellFolderViewCB
{
public:
    CFSFolderShellFolderViewCB(IShellFolder* psf, CFSFolder *pFSFolder, LONG lEvents)
        : CBaseShellFolderViewCB(psf, pFSFolder->_pidl, lEvents), m_pFSFolder(pFSFolder)
    { 
        memset(&m_fssci, 0, sizeof(m_fssci));

        m_fssci.idDrive = -1;       // these fields use -1 to mean
        m_fssci.cbFree = -1;        // "unknown" / "not available"
    }

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    FSSELCHANGEINFO m_fssci;
    CFSFolder* m_pFSFolder;

private:

    HRESULT OnMergeMenu(DWORD pv, QCMINFO*lP)
    {
        CDefFolderMenu_MergeMenu(HINST_THISDLL, 0, POPUP_FSVIEW_POPUPMERGE, lP);
        return S_OK;
    }

    HRESULT OnSize(DWORD pv, UINT cx, UINT cy)
    {
        ResizeStatus(_punkSite, cx);
        return S_OK;
    }

    HRESULT OnGetPane(DWORD pv, LPARAM dwPaneID, DWORD *pdwPane)
    {
        if (PANE_ZONE == dwPaneID)
            *pdwPane = 2;
        return S_OK;
    }

    HRESULT OnInvokeCommand(DWORD pv, UINT wParam)
    {
        DebugMsg(DM_TRACE, TEXT("sh TR - FS_FSNCallBack DVN_INVOKECOMMAND (id=%x)"), wParam);

        switch(wParam)
        {
        case FSIDM_SORTBYNAME:
        case FSIDM_SORTBYSIZE:
        case FSIDM_SORTBYTYPE:
        case FSIDM_SORTBYDATE:
            ShellFolderView_ReArrange(m_hwndMain, FSSortIDToICol(wParam));
            break;
        }

        return S_OK;
    }

    HRESULT OnGetCCHMax(DWORD pv, LPCITEMIDLIST pidlItem, UINT *pcchMax)
    {
        return CFSFolder_GetCCHMax(m_pFSFolder, FS_IsValidID(pidlItem), pcchMax);
    }

    HRESULT OnGetHelpText(DWORD pv, UINT wPl, UINT wPh, LPTSTR psz)
    {
        LoadString(HINST_THISDLL, wPl + IDS_MH_FSIDM_FIRST, psz, wPh);
        return S_OK;
    }

    HRESULT OnWindowCreated(DWORD pv, HWND wP)
    {
        TCHAR szPath[MAX_PATH];
        CFSFolder_GetPathForItem(m_pFSFolder, NULL, szPath);

        m_fssci.idDrive = PathGetDriveNumber(szPath);  // keep track of this for later
        m_fssci.cbFree = -1;                            // not known yet

        InitializeStatus(_punkSite);
        return S_OK;
    }

    HRESULT OnInsertDeleteItem(int iMul, LPCITEMIDLIST wP)
    {
        FSOnInsertDeleteItem(m_pFSFolder->_pidl, &m_fssci, wP, iMul);

        // Tell the FSFolder that it needs to update the extended columns
        // when we get an insert item.  This will cause the next call to
        // IColumnProvider::GetItemData to flush it's row-wise cache.
        if ( 1 == iMul )
        {
            m_pFSFolder->_bUpdateExtendedCols = TRUE;
        }
        return S_OK;
    }

    HRESULT OnSelChange(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA*lP)
    {
        FSOnSelChange(m_pFSFolder->_pidl, lP, &m_fssci);
        return S_OK;
    }

    HRESULT OnUpdateStatusBar(DWORD pv, BOOL wP)
    {
        // if initializing, force refresh of disk free space
        if (wP)
            m_fssci.cbFree = -1;
        return FSUpdateStatusBar(_punkSite, &m_fssci);
    }

    HRESULT OnRefresh(DWORD pv, BOOL fPreRefresh)
    {
        // pre refresh...
        if ( fPreRefresh )
        {
            // in case our attributes change invalidate our notion of our own
            // attributes
            m_pFSFolder->_dwAttributes = -1;
        }
        else
        {
            m_fssci.cHiddenFiles = m_pFSFolder->cHiddenFiles;
            m_fssci.cbSize = m_pFSFolder->cbSize;
        }
        return S_OK;
    }

    HRESULT OnSelectAll(DWORD pv)
    {
        HRESULT hres = S_OK;

        if (m_fssci.cHiddenFiles > 0) 
        {
            if (ShellMessageBox(HINST_THISDLL, m_hwndMain, 
                MAKEINTRESOURCE(IDS_SELECTALLBUTHIDDEN), 
                MAKEINTRESOURCE(IDS_SELECTALL), MB_OKCANCEL | MB_SETFOREGROUND | MB_ICONWARNING, 
                m_fssci.cHiddenFiles) == IDCANCEL)
            {
                hres = S_FALSE;
            }
        }
        return hres;
    }

    HRESULT OnGetWorkingDir(DWORD pv, UINT wP, LPTSTR lP)
    {
        return CFSFolder_GetPathForItem(m_pFSFolder, NULL, lP);
    }

    HRESULT OnGetColSaveStream(DWORD pv, WPARAM wP, IStream**lP)
    {

        // 99/05/14 vtan: As of Windows 2000 each PIDL persists column information
        // individually rather than globally. This is the general trend in the shell.

        // Should the old global behavior be desired uncomment the code below. Note
        // the code below used to test for aggregation using the presence of
        // punkOuter which is no longer valid. punkOuter always exists and is
        // NON-NULL. Therefore the test should be against the internal IUnknown.

        return E_FAIL;
#if 0
        // Don't provide a default stream if we are aggregated
        // Added to prevent favorites folder (with 9 columns) from using FS data (with 5 columns)
        if (m_pFSFolder->punkOuter != &m_pFSFolder->iunk)
            return E_FAIL;
        *lP = OpenRegStream(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, TEXT("DirectoryColsX"), (DWORD)wP);
        return *lP ? S_OK : E_FAIL;
#endif
    }

    HRESULT OnGetViews(DWORD pv, SHELLVIEWID *pvid, IEnumSFVViews **ppObj);

    HRESULT OnSupportsIdentity(DWORD pv)
    {
        return S_OK;
    }

    HRESULT OnQueryReuseExtView(DWORD pv, BOOL *pfReuseAllowed)
    {
        if (pfReuseAllowed)
        {
            *pfReuseAllowed = TRUE;
            return NOERROR;
        }

        return E_INVALIDARG;
    }
};

HRESULT CFSFolderShellFolderViewCB::OnGetViews(DWORD pv, SHELLVIEWID *pvid, IEnumSFVViews **ppObj)
{
    *ppObj = NULL;

    CViewsList cViews;

    // Add base class stuff
    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("folder"));

    // Add this class stuff
    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("directory"));

    // Add this instance stuff
    TCHAR szHere[MAX_PATH];

    if (SUCCEEDED(CFSFolder_GetPath(m_pFSFolder, szHere)))
    {
        BOOL fForceIni = FALSE;
    
        // Add root-of-drive stuff
        if (PathIsRoot(szHere))
        {
            fForceIni = TRUE;

            if (PathIsUNC(szHere))
            {
                cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("directory\\net"));
            }
            else
            {
                int idDrive = (int)(szHere[0] - 'A');
                int type = RealDriveType(idDrive, FALSE);

                if (type == DRIVE_REMOVABLE)
                {
                    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("directory\\removeable"));
                }
                else if (type == DRIVE_FIXED)
                {
                    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("directory\\fixed"));
                }
                else if (type == DRIVE_REMOTE)
                {
                    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("directory\\net"));
                }
                else if (type == DRIVE_CDROM)
                {
                    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("directory\\cdrom"));

                    // win95 cdrom drivers don't support the read/system bits. ???
                    fForceIni = TRUE;
                }
                else if (type == DRIVE_RAMDISK)
                {
                    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("directory\\ramdisk"));
                }
            }
        }
        else
        {
            switch (CFSFolder_GetCSIDL(m_pFSFolder))
            {
            case CSIDL_SYSTEM:
            case CSIDL_WINDOWS:
            case CSIDL_PERSONAL:
                fForceIni = TRUE;
                break;
            }
        }
        
        // Add desktop.ini stuff
        if (fForceIni || (CFSFolder_Attributes(m_pFSFolder) & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_SYSTEM)))
        {
            TCHAR szIniFile[MAX_PATH];
            PathCombine(szIniFile, szHere, c_szDesktopIni);
            cViews.AddIni(szIniFile, szHere);
        }
    }

    cViews.GetDef(pvid);

    return CreateEnumCViewList(&cViews, ppObj);

    // Note the automatic destructor will free any views still left
}

const CLSID *c_rgFilePages[] = {
    &CLSID_FileTypes,
    &CLSID_OfflineFilesOptions
};

// add optional pages to Explore/Options.

HRESULT SFVCB_OnAddPropertyPages(DWORD pv, SFVM_PROPPAGE_DATA *ppagedata)
{
    for (int i = 0; i < ARRAYSIZE(c_rgFilePages); i++)
    {
        IShellPropSheetExt * pspse;

        HRESULT hres = SHCoCreateInstance(NULL, c_rgFilePages[i], NULL, IID_IShellPropSheetExt, (void **)&pspse);
        if (SUCCEEDED(hres))
        {
            pspse->AddPages(ppagedata->pfn, ppagedata->lParam);
            pspse->Release();
        }
    }

    return S_OK;
}

STDMETHODIMP CFSFolderShellFolderViewCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMergeMenu);
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnInvokeCommand);
    HANDLE_MSG(0, SFVM_GETCCHMAX, OnGetCCHMax);
    HANDLE_MSG(0, SFVM_GETHELPTEXT, OnGetHelpText);
    HANDLE_MSG(0, SFVM_WINDOWCREATED, OnWindowCreated);
    HANDLE_MSG(1 , SFVM_INSERTITEM, OnInsertDeleteItem);
    HANDLE_MSG(-1, SFVM_DELETEITEM, OnInsertDeleteItem);
    HANDLE_MSG(0, SFVM_SELCHANGE, OnSelChange);
    HANDLE_MSG(0, SFVM_UPDATESTATUSBAR, OnUpdateStatusBar);
    HANDLE_MSG(0, SFVM_REFRESH, OnRefresh);
    HANDLE_MSG(0, SFVM_SELECTALL, OnSelectAll);
    HANDLE_MSG(0, SFVM_GETWORKINGDIR, OnGetWorkingDir);
    HANDLE_MSG(0, SFVM_GETCOLSAVESTREAM, OnGetColSaveStream);
    HANDLE_MSG(0, SFVM_GETVIEWS, OnGetViews);
    HANDLE_MSG(0, SFVM_SUPPORTSIDENTITY, OnSupportsIdentity);
    HANDLE_MSG(0, SFVM_ADDPROPERTYPAGES, SFVCB_OnAddPropertyPages);
    HANDLE_MSG(0, SFVM_QUERYREUSEEXTVIEW, OnQueryReuseExtView);
    HANDLE_MSG(0, SFVM_SIZE, OnSize);
    HANDLE_MSG(0, SFVM_GETPANE, OnGetPane);

    default:
        return E_FAIL;
    }

    return NOERROR;
}


STDAPI CFSFolderCallback_Create(IShellFolder* psf, CFSFolder *pFSFolder, LONG lEvents, IShellFolderViewCB **ppsfvcb)
{
    *ppsfvcb = new CFSFolderShellFolderViewCB(psf, pFSFolder, lEvents);
    return *ppsfvcb ? S_OK : E_OUTOFMEMORY;
}


//
// Briefcase stuff
//


const TBBUTTON c_tbBrfCase[] = {
    { 0, FSIDM_UPDATEALL,       TBSTATE_ENABLED, TBSTYLE_BUTTON, {0,0}, 0L, -1 },
    { 1, FSIDM_UPDATESELECTION, 0,               TBSTYLE_BUTTON, {0,0}, 0L, -1 },
    { 0,  0,                    TBSTATE_ENABLED, TBSTYLE_SEP   , {0,0}, 0L, -1 },
    };

void BrfView_OnGetButtons(PBRFVIEW that, HWND hwndMain, UINT idCmdFirst, LPTBBUTTON ptbbutton)
{
    IShellBrowser * psb = FileCabinet_GetIShellBrowser(hwndMain);
    if (psb)
    {
        UINT i;
        LRESULT iBtnOffset;
        TBADDBITMAP ab;
    
        // add the toolbar button bitmap, get it's offset
        ab.hInst = HINST_THISDLL;
        ab.nID   = IDB_BRF_TB_SMALL;        // std bitmaps
        psb->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 2, (LPARAM)&ab, &iBtnOffset);
    
        for (i = 0; i < ARRAYSIZE(c_tbBrfCase); i++)
        {
            ptbbutton[i] = c_tbBrfCase[i];
        
            if (!(c_tbBrfCase[i].fsStyle & TBSTYLE_SEP))
            {
                ptbbutton[i].idCommand += idCmdFirst;
                ptbbutton[i].iBitmap += (int) iBtnOffset;
            }
        }
    }
}


#define BRFVIEW_EVENTS \
    SHCNE_DISKEVENTS | \
    SHCNE_ASSOCCHANGED | \
    SHCNE_GLOBALEVENTS

class CBrfViewSFVCB : public CBaseShellFolderViewCB
{
public:
    CBrfViewSFVCB(IShellFolder* psf, PBRFVIEW pbv) : CBaseShellFolderViewCB(psf, pbv->pidl, BRFVIEW_EVENTS)
    { 
        m_bv = *pbv;
        // m_pidl is cloned in the base class thus will have the right lifetime
        // (although pbv->pidl is stored in the shell folder that has the same life
        // as this object)
        ASSERT(ILIsEqual(m_bv.pidl, m_pidl));
        m_bv.pidl = m_pidl; 
    }

    ~CBrfViewSFVCB()
    {
        if (m_bv.pbrfstg)
            m_bv.pbrfstg->Release();

        if (m_bv.pidlRoot)
            ILFree(m_bv.pidlRoot);
    }

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    BrfView m_bv;

    HRESULT OnWINDOWCREATED(DWORD pv, HWND wP)
    {
        BrfView_OnCreate(&m_bv, m_hwndMain, wP);
        return S_OK;
    }

    HRESULT OnWINDOWDESTROY(DWORD pv, HWND wP)
    {
        BrfView_OnDestroy(&m_bv, wP);
        return S_OK;
    }

    HRESULT OnMERGEMENU(DWORD pv, QCMINFO*lP)
    {
        BrfView_MergeMenu(&m_bv, lP);
        return S_OK;
    }

    HRESULT OnINVOKECOMMAND(DWORD pv, UINT wP)
    {
        BrfView_Command(&m_bv, m_pshf, m_hwndMain, wP);
        return S_OK;
    }

    HRESULT OnGetHelpOrTooltipText(BOOL bHelp, UINT wPl, UINT cch, LPTSTR psz)
    {
        LoadString(HINST_THISDLL, wPl + (bHelp ? IDS_MH_FSIDM_FIRST : IDS_TT_FSIDM_FIRST), psz, cch);
        return S_OK;
    }

    HRESULT OnINITMENUPOPUP(DWORD pv, UINT wPl, UINT wPh, HMENU lP)
    {
        BrfView_InitMenuPopup(&m_bv, m_hwndMain, wPl, wPh, lP);
        return S_OK;
    }

    HRESULT OnGETBUTTONINFO(DWORD pv, TBINFO* ptbinfo)
    {
        ptbinfo->cbuttons = ARRAYSIZE(c_tbBrfCase);
        ptbinfo->uFlags = TBIF_PREPEND;
        return S_OK;
    }

    HRESULT OnGETBUTTONS(DWORD pv, UINT wPl, UINT wPh, TBBUTTON*lP)
    {
        BrfView_OnGetButtons(&m_bv, m_hwndMain, wPl, lP);
        return S_OK;
    }

    HRESULT OnSELCHANGE(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA*lP)
    {
        return(BrfView_OnSelChange(&m_bv, m_hwndMain, wPl));
    }

    HRESULT OnQUERYFSNOTIFY(DWORD pv, SHChangeNotifyEntry*lP)
    {
        return(BrfView_OnQueryFSNotify(&m_bv, lP));
    }

    HRESULT OnFSNOTIFY(DWORD pv, LPCITEMIDLIST*wP, LPARAM lP)
    {
        return(BrfView_OnFSNotify(&m_bv, m_hwndMain, (LONG) lP, wP));
    }

    HRESULT OnQUERYCOPYHOOK(DWORD pv)
    {
        return S_OK;
    }

    HRESULT OnNOTIFYCOPYHOOK(DWORD pv, COPYHOOKINFO*lP)
    {
        return(BrfView_OnNotifyCopyHook(&m_bv, m_hwndMain, lP));
    }

    HRESULT OnINSERTITEM(DWORD pv, LPCITEMIDLIST wP)
    {
        return(BrfView_OnInsertItem(&m_bv, m_hwndMain, wP));
    }

    HRESULT OnDEFVIEWMODE(DWORD pv, FOLDERVIEWMODE*lP)
    {
        *lP = FVM_DETAILS;
        return S_OK;
    }

    HRESULT OnSupportsIdentity(DWORD pv)
    {
        return S_OK;
    }

    HRESULT OnGetViews(DWORD pv, SHELLVIEWID *pvid, IEnumSFVViews **ppObj);

    HRESULT OnGetHelpTopic(DWORD pv, SFVM_HELPTOPIC_DATA * phtd)
    {
        StrCpyW(phtd->wszHelpFile, L"brief.chm");
        return S_OK;
    }
} ;

HRESULT CBrfViewSFVCB::OnGetViews(DWORD pv, SHELLVIEWID *pvid, IEnumSFVViews **ppObj)
{
    *ppObj = NULL;

    CViewsList cViews;

    // Add base class stuff
    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("folder"));

    // Add 2nd base class stuff
    cViews.AddReg(HKEY_CLASSES_ROOT, TEXT("directory"));

    // Add this class stuff
    cViews.AddCLSID(&CLSID_Briefcase);

    // Add this instance stuff
    TCHAR szHere[MAX_PATH];
    if (SHGetPathFromIDList(m_pidl, szHere))
    {
        TCHAR szIniFile[MAX_PATH];
        PathCombine(szIniFile, szHere, c_szDesktopIni);
        cViews.AddIni(szIniFile, szHere);
    }

    cViews.GetDef(pvid);

    return CreateEnumCViewList(&cViews, ppObj);

    // Note the automatic destructor will free any views still left
}

STDMETHODIMP CBrfViewSFVCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_WINDOWCREATED, OnWINDOWCREATED);
    HANDLE_MSG(0, SFVM_WINDOWDESTROY, OnWINDOWDESTROY);
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMERGEMENU);
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnINVOKECOMMAND);
    HANDLE_MSG(TRUE , SFVM_GETHELPTEXT   , OnGetHelpOrTooltipText);
    HANDLE_MSG(FALSE, SFVM_GETTOOLTIPTEXT, OnGetHelpOrTooltipText);
    HANDLE_MSG(0, SFVM_INITMENUPOPUP, OnINITMENUPOPUP);
    HANDLE_MSG(0, SFVM_GETBUTTONINFO, OnGETBUTTONINFO);
    HANDLE_MSG(0, SFVM_GETBUTTONS, OnGETBUTTONS);
    HANDLE_MSG(0, SFVM_SELCHANGE, OnSELCHANGE);
    HANDLE_MSG(0, SFVM_QUERYFSNOTIFY, OnQUERYFSNOTIFY);
    HANDLE_MSG(0, SFVM_FSNOTIFY, OnFSNOTIFY);
    HANDLE_MSG(0, SFVM_QUERYCOPYHOOK, OnQUERYCOPYHOOK);
    HANDLE_MSG(0, SFVM_NOTIFYCOPYHOOK, OnNOTIFYCOPYHOOK);
    HANDLE_MSG(0, SFVM_INSERTITEM, OnINSERTITEM);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, OnDEFVIEWMODE);
    HANDLE_MSG(0, SFVM_GETVIEWS, OnGetViews);
    HANDLE_MSG(0, SFVM_ADDPROPERTYPAGES, SFVCB_OnAddPropertyPages);
    HANDLE_MSG(0, SFVM_SUPPORTSIDENTITY, OnSupportsIdentity);
    HANDLE_MSG(0, SFVM_GETHELPTOPIC, OnGetHelpTopic);

    default:
        return E_FAIL;
    }

    return NOERROR;
}


IShellFolderViewCB* BrfView_CreateSFVCB(IShellFolder* psf, PBRFVIEW pbv)
{
    return new CBrfViewSFVCB(psf, pbv);
}


class CFolderExtractImage : public IExtractImage,
                            public IPersist,
                            public IRunnableTask
{
    public:
        CFolderExtractImage();
        
        STDMETHOD (QueryInterface)(REFIID riid, void **ppv);
        STDMETHOD_(ULONG, AddRef) ();
        STDMETHOD_(ULONG, Release) ();

        // IExtractImage/IExtractLogo
        STDMETHOD (GetLocation) ( LPWSTR pszPathBuffer,
                                  DWORD cch,
                                  DWORD * pdwPriority,
                                  const SIZE * prgSize,
                                  DWORD dwRecClrDepth,
                                  DWORD *pdwFlags );
        STDMETHOD (Extract)(HBITMAP * phBmpThumbnail);

        // IPersist
        STDMETHOD(GetClassID)(LPCLSID lpClassID);

        // IRunnableTask
        STDMETHOD (Run)(void);
        STDMETHOD (Kill)(BOOL fWait);
        STDMETHOD (Suspend)(void);
        STDMETHOD (Resume)(void);
        STDMETHOD_(ULONG, IsRunning)(void);

        STDMETHOD(Init)(LPCTSTR pszPath);
    private:
        ~CFolderExtractImage();
        LPCTSTR _GetImagePath(UINT cx);

        IExtractImage * m_pExtract;
        IRunnableTask * m_pRun;
        long            _cRef;
        TCHAR           m_szFolder[MAX_PATH];
        TCHAR           m_szLogo[MAX_PATH];
        TCHAR           m_szWideLogo[MAX_PATH];
        SIZEL           m_rgSize;
};

STDAPI CFolderExtractImage_Create(LPCTSTR pszPath, REFIID riid, void **ppvObj)
{
    HRESULT hr = E_OUTOFMEMORY;
    CFolderExtractImage * pObj = new CFolderExtractImage;
    if (pObj)
    {
        hr = pObj->Init(pszPath);
        if (SUCCEEDED(hr))
            hr = pObj->QueryInterface(riid, ppvObj);
        pObj->Release();
    }
    return hr;
}

CFolderExtractImage::CFolderExtractImage( ) : _cRef (1)
{
}

CFolderExtractImage::~CFolderExtractImage()
{
    ATOMICRELEASE(m_pExtract);
    ATOMICRELEASE(m_pRun);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CFolderExtractImage::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFolderExtractImage, IExtractImage),
        QITABENTMULTI2(CFolderExtractImage, IID_IExtractLogo, IExtractImage),
        QITABENT(CFolderExtractImage, IPersist),
        QITABENT(CFolderExtractImage, IRunnableTask),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFolderExtractImage::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFolderExtractImage::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

LPCTSTR CFolderExtractImage::_GetImagePath(UINT cx)
{
    if (0 == m_szLogo[0])
    {
        if (_GetFolderString(m_szFolder, NULL, m_szLogo, ARRAYSIZE(m_szLogo), SZ_CANBEUNICODE TEXT("Logo")))
        {
            if (_GetFolderString(m_szFolder, NULL, m_szWideLogo, ARRAYSIZE(m_szWideLogo), SZ_CANBEUNICODE TEXT("WideLogo")))
                PathCombine(m_szWideLogo, m_szFolder, m_szWideLogo);   // relative path support
    
            PathCombine(m_szLogo, m_szFolder, m_szLogo);   // relative path support
        }
#if 0
        // this is the auto-folder image code. since the image generated here
        // looks too much like a picture we need to turn this off. once we fix
        // the overlay code so it is more obvious what is a folder we can re-enable this
        else
        {
            TCHAR szFind[MAX_PATH];

            PathCombine(szFind, m_szFolder, TEXT("folder.jpg"));
            if (PathFileExists(szFind))
            {
                lstrcpyn(m_szLogo, szFind, ARRAYSIZE(m_szLogo));
            }
            else
            {
                PathCombine(szFind, m_szFolder, TEXT("*.jpg"));

                HANDLE hfind;
                WIN32_FIND_DATA fd;
                if (S_OK == SHFindFirstFile(szFind, &fd, &hfind))
                {
                    PathCombine(m_szLogo, m_szFolder, fd.cFileName);
                    FindClose(hfind);
                }
            }
        }
#endif
    }

    LPCTSTR psz = ((cx > 120) && m_szWideLogo[0]) ? m_szWideLogo : m_szLogo;
    return *psz ? psz : NULL;
}

STDMETHODIMP CFolderExtractImage::GetLocation(LPWSTR pszPathBuffer, DWORD cch,
                                              DWORD * pdwPriority, const SIZE * prgSize,
                                              DWORD dwRecClrDepth, DWORD *pdwFlags)
{
    HRESULT hr;
    LPCTSTR pszLogo = _GetImagePath(prgSize->cx);
    if (pszLogo)
    {
        ATOMICRELEASE(m_pExtract);
        ATOMICRELEASE(m_pRun);

        LPITEMIDLIST pidl;
        hr = SHILCreateFromPath(pszLogo, &pidl, NULL);
        if (SUCCEEDED(hr))
        {
            hr = SHGetUIObjectFromFullPIDL(pidl, NULL, IID_PPV_ARG(IExtractImage, &m_pExtract));
            if (SUCCEEDED(hr))
            {
                m_pExtract->QueryInterface(IID_PPV_ARG(IRunnableTask, &m_pRun));    // optional

                hr = m_pExtract->GetLocation(pszPathBuffer, cch, pdwPriority, prgSize, dwRecClrDepth, pdwFlags);
            }
            ILFree(pidl);
        }
    }
    else
        hr = E_FAIL;
    return hr;
}

STDMETHODIMP CFolderExtractImage::Extract(HBITMAP * phBmpThumbnail)
{
    return m_pExtract ? m_pExtract->Extract(phBmpThumbnail) : E_FAIL;
}

STDMETHODIMP CFolderExtractImage::GetClassID(CLSID *pClassID)
{
    return E_NOTIMPL;
}

STDMETHODIMP CFolderExtractImage::Init(LPCTSTR pszPath)
{
    lstrcpyn(m_szFolder, pszPath, ARRAYSIZE(m_szFolder));
    return S_OK;
}

STDMETHODIMP CFolderExtractImage::Run(void)
{
    return m_pRun ? m_pRun->Run() : E_NOTIMPL;
}

STDMETHODIMP CFolderExtractImage::Kill(BOOL fWait)
{
    return m_pRun ? m_pRun->Kill( fWait ) : E_NOTIMPL;
}

STDMETHODIMP CFolderExtractImage::Suspend(void)
{
    return m_pRun ? m_pRun->Suspend() : E_NOTIMPL;
}

STDMETHODIMP CFolderExtractImage::Resume(void)
{
    return m_pRun ? m_pRun->Resume() : E_NOTIMPL;
}

STDMETHODIMP_(ULONG) CFolderExtractImage::IsRunning(void)
{
    return m_pRun ? m_pRun->IsRunning() : E_NOTIMPL;
}


class CFileSysEnum : public IEnumIDList
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();

    // IEnumIDList
    STDMETHOD(Next)(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumIDList **ppenum);
    
    CFileSysEnum(CFSFolder *pfsf, IUnknown *punk, HWND hwnd, DWORD grfFlags);
    HRESULT Init();

private:
    ~CFileSysEnum();
    BOOL _FindNextFile();

    LONG _cRef;
    IUnknown *_punk;        // BUGBUG: since this C++ code can't use the C way to hold a ref on CFSFolder use this

    CFSFolder *_pfsf;
    DWORD _grfFlags;
    HWND _hwnd;

    HANDLE _hfind;
    TCHAR _szFolder[MAX_PATH];
    BOOL _fMoreToEnum;
    WIN32_FIND_DATA _fd;
    int _cHiddenFiles;
    ULONGLONG _cbSize;

    BOOL _fRecentDocs;      // we are in the recent docs mode
    int _iIndexMRU;

    BOOL _fShowSuperHidden;
};

CFileSysEnum::CFileSysEnum(CFSFolder *pfsf, IUnknown *punk, HWND hwnd, DWORD grfFlags) : 
    _cRef(1), _pfsf(pfsf), _punk(punk), _hwnd(hwnd), _grfFlags(grfFlags), _hfind(INVALID_HANDLE_VALUE)
{
    _fShowSuperHidden = ShowSuperHidden();

    _punk->AddRef();
}

CFileSysEnum::~CFileSysEnum()
{
    if (_hfind != INVALID_HANDLE_VALUE)
    {
        //  this handle can be the find file or MRU list in the case of RECENTDOCSDIR
        if (_fRecentDocs)
            FreeMRUList(_hfind);
        else
            FindClose(_hfind);

        _hfind = INVALID_HANDLE_VALUE;
    }
    _punk->Release();
}

HRESULT CFileSysEnum::Init()
{
    TCHAR szPath[MAX_PATH];
    HRESULT hres = CFSFolder_GetPath(_pfsf, _szFolder);
    if (SUCCEEDED(hres) &&
        PathCombine(szPath, _szFolder, c_szStarDotStar))
    {
        // let name mapper see the path/PIDL pair (for UNC root mapping)
        NPTRegisterNameToPidlTranslation(_szFolder, _pfsf->_pidlTarget ? _pfsf->_pidlTarget:_pfsf->_pidl);

        if (_grfFlags == SHCONTF_FOLDERS)
        {
            // use mask to only find folders, mask is in the hi byte of dwFileAttributes
            // algorithm: (((attrib_on_disk & mask) ^ mask) == 0)
            _fd.dwFileAttributes = (FILE_ATTRIBUTE_DIRECTORY << 8) |
                    FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_DIRECTORY;
            _fd.dwReserved0 = 0x56504347;      // signature to tell kernel to use the attribs specified
        }

        // BUGBUG: We should supply a punkEnableModless in order to go modal during UI.
        hres = SHFindFirstFileRetry(_hwnd, NULL, szPath, &_fd, &_hfind, SHPPFW_NONE);
        if (SUCCEEDED(hres))
        {
            ASSERT(hres == S_OK ? (_hfind != INVALID_HANDLE_VALUE) : TRUE);

            _fMoreToEnum = (hres == S_OK);

            if (!(_grfFlags & SHCONTF_INCLUDEHIDDEN))
            {
                if (CFSFolder_IsCSIDL(_pfsf, CSIDL_RECENT))
                {
                    _fRecentDocs = TRUE;

                    // open it now so that each open within the enum is fast.
                    // close at release time
                    if (_hfind != INVALID_HANDLE_VALUE)
                        FindClose(_hfind);

                    _hfind = CreateSharedRecentMRUList(NULL, NULL, SRMLF_COMPNAME);
                }
            }
        }
        else
        {
            // SHFindFirstFileRetry() will display the error message so prevent from displaying
            // them in the future
            hres = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }
    else if (hres == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND))
    {
        // Tracking target doesn't exist; return an empty enumerator
        _fMoreToEnum = FALSE;
        hres = S_OK;
    }
    else
    {
        // CFSFolder_GetPathForItem & PathCombine() fail when path is too long
        if (_hwnd)
        {
            ShellMessageBox(HINST_THISDLL, _hwnd, MAKEINTRESOURCE(IDS_ENUMERR_PATHTOOLONG),
                NULL, MB_OK | MB_ICONHAND);
        }

        // This error value tells callers that we have already displayed error UI so skip
        // displaying errors.
        hres = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    return hres;
}



STDMETHODIMP CFileSysEnum::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CFileSysEnum, IEnumIDList),                        // IID_IEnumIDList
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CFileSysEnum::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFileSysEnum::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

BOOL CFileSysEnum::_FindNextFile()
{
    BOOL fMoreToEnum = FALSE;

    if (_fRecentDocs)
    {
        LPITEMIDLIST pidl;

        while (-1 != EnumSharedRecentMRUList(_hfind, _iIndexMRU, NULL, &pidl))
        {
            // confirm that the item stil exists in the file system, fill in the _fd data
            TCHAR szPath[MAX_PATH];
            HANDLE h;

            CFSFolder_GetPathForItem(_pfsf, FS_IsValidID(pidl), szPath);
            ILFree(pidl);

            h = FindFirstFile(szPath, &_fd);
            if (h != INVALID_HANDLE_VALUE)
            {
                fMoreToEnum = TRUE;
                _iIndexMRU++;
                FindClose(h);
                break;
            }
            else
            {
                //
                //  WARNING - if the list is corrupt we torch it - ZekeL 19-JUN-98
                //  we could do some special crap, i guess, to weed out the bad
                //  items, but it seems simpler to just blow it away.
                //  the only reason this should happen is if somebody
                //  has been mushing around with RECENT directory directly,
                //  which they shouldnt do since it is hidden...
                //
                
                //  kill this invalid entry, and then try the same index again...
                DelMRUString(_hfind, _iIndexMRU);
            }
        }
    }
    else
        fMoreToEnum = FindNextFile(_hfind, &_fd);

    return fMoreToEnum;
}

#define FILE_ATTRIBUTE_SUPERHIDDEN (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN) 
#define IS_SYSTEM_HIDDEN(dw) ((dw & FILE_ATTRIBUTE_SUPERHIDDEN) == FILE_ATTRIBUTE_SUPERHIDDEN) 

STDMETHODIMP CFileSysEnum::Next(ULONG celt, LPITEMIDLIST *ppidl, ULONG *pceltFetched)
{
    HRESULT hres;

    for (; _fMoreToEnum; _fMoreToEnum = _FindNextFile())
    {
        if (_fMoreToEnum == (BOOL)42)
            continue;   // we already processed the current item, skip it now

        if (PathIsDotOrDotDot(_fd.cFileName))
            continue;

        if (_fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if (!(_grfFlags & SHCONTF_FOLDERS))
                continue;   // item is folder but client does not want folders
        }
        else if (!(_grfFlags & SHCONTF_NONFOLDERS))
            continue;   // item is file, but client only wants folders

        // skip hidden and system things unconditionally, don't even count them
        if (!_fShowSuperHidden && IS_SYSTEM_HIDDEN(_fd.dwFileAttributes))
            continue;

        _cbSize += MAKELONGLONG(_fd.nFileSizeLow, _fd.nFileSizeHigh);

        if (!(_grfFlags & SHCONTF_INCLUDEHIDDEN) &&
             (_fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
        {
            _cHiddenFiles++;
            continue;
        }
        break;
    }

    if (_fMoreToEnum)
    {
        hres = CFSFolder_CreateIDList(_pfsf, &_fd, ppidl);
        _fMoreToEnum = (BOOL)42;    // we have processed the current item, skip it next time
    }
    else
    {
        *ppidl = NULL;
        hres = S_FALSE; // no more items
        // completed the enum, stash some items back into the folder (lame!)
        _pfsf->cHiddenFiles = _cHiddenFiles;
        _pfsf->cbSize = _cbSize;
    }

    if (pceltFetched)
        *pceltFetched = (hres == S_OK) ? 1 : 0;

    return hres;
}


STDMETHODIMP CFileSysEnum::Skip(ULONG celt) 
{
    return E_NOTIMPL;
}

STDMETHODIMP CFileSysEnum::Reset() 
{
    return S_OK;
}

STDMETHODIMP CFileSysEnum::Clone(IEnumIDList **ppenum) 
{
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDAPI CFSFolder_CreateEnum(CFSFolder *pfsf, IUnknown *punk, HWND hwnd, DWORD grfFlags, IEnumIDList **ppenum)
{
    HRESULT hres;
    CFileSysEnum *penum = new CFileSysEnum(pfsf, punk, hwnd, grfFlags);
    if (penum)
    {
        hres = penum->Init();
        if (SUCCEEDED(hres))
            penum->QueryInterface(IID_IEnumIDList, (void **)ppenum);
        penum->Release();
    }
    else
    {
        hres = E_OUTOFMEMORY;
        *ppenum = NULL;
    }
    return hres;
}
