#include "shellprv.h"
#include "caggunk.h"
#include "views.h"
#include "drives.h"
#include "netview.h"
#include "propsht.h"
#include "infotip.h"
#include "sfviewp.h"
#include "mtpt.h"
#include "prop.h"

extern "C"
{
#include "fstreex.h"
#include "ovrlaymn.h"
#include "idmk.h"
}

#include "shitemid.h"
#include "shobjprv.h"

#include "deskfldr.h"
#include "datautil.h"

#ifdef WINNT
#include <ntddcdrm.h>
#else
#define Not_VxD
#include <vwin32.h>     // DeviceIOCtl calls
#endif


// BUGBUG: At some point we should check an ini switch or something
#define ShowDriveInfo(_iDrive)  (!IsRemovableDrive(_iDrive))

#define CDRIVES_REGITEM_CONTROL 0
#define IDLIST_DRIVES           ((LPCITEMIDLIST)&c_idlDrives)

// These are the sort order for items in MyComputer
#define CONTROLS_SORT_INDEX             30
#define CDRIVES_REGITEM_CONTROL          0

REQREGITEM g_asDrivesReqItems[] =
{
    { &CLSID_ControlPanel, IDS_CONTROLPANEL, c_szShell32Dll, CONTROLS_SORT_INDEX, -IDI_CPLFLD, SFGAO_FOLDER | SFGAO_HASSUBFOLDER, NULL},
};

REGITEMSINFO g_sDrivesRegInfo =
{
    REGSTR_PATH_EXPLORER TEXT("\\MyComputer\\NameSpace"),
    NULL,
    TEXT(':'),
    SHID_COMPUTER_REGITEM,
    -1,
    SFGAO_CANLINK,
    ARRAYSIZE(g_asDrivesReqItems),
    g_asDrivesReqItems,
    RIISA_ORIGINAL,
    NULL,
    0,
    0,
};


class CDrivesViewCallback;

class CDrivesFolder : public CAggregatedUnknown, IShellFolder2, IPersistFolder2, IShellIconOverlay
{
public:
    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj)
                { return CAggregatedUnknown::QueryInterface(riid, ppvObj); };
    STDMETHODIMP_(ULONG) AddRef(void) 
                { return CAggregatedUnknown::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) 
                { return CAggregatedUnknown::Release(); };

    // *** IShellFolder methods ***
    STDMETHODIMP ParseDisplayName(HWND hwnd, LPBC pbc, LPOLESTR pszDisplayName,
                                  ULONG* pchEaten, LPITEMIDLIST* ppidl, ULONG* pdwAttributes);
    STDMETHODIMP EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList ** ppenumIDList);
    STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppvOut);
    STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppvObj);
    STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    STDMETHODIMP CreateViewObject (HWND hwndOwner, REFIID riid, void** ppvOut);
    STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* rgfInOut);
    STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST* apidl,
                               REFIID riid, UINT* prgfInOut, void** ppvOut);
    STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET *pName);
    STDMETHODIMP SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, LPCOLESTR pszName, DWORD uFlags,
                           LPITEMIDLIST* ppidlOut);

    // *** IShellFolder2 methods ***
    STDMETHODIMP GetDefaultSearchGUID(LPGUID lpGuid);
    STDMETHODIMP EnumSearches(LPENUMEXTRASEARCH* ppenum);
    STDMETHODIMP GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay);
    STDMETHODIMP GetDefaultColumnState(UINT iColumn, DWORD* pbState);
    STDMETHODIMP GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv);
    STDMETHODIMP GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, SHELLDETAILS* pDetails);
    STDMETHODIMP MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid);

    // *** IPersist methods ***
    STDMETHODIMP GetClassID(CLSID* pClassID);
    // *** IPersistFolder methods ***
    STDMETHODIMP Initialize(LPCITEMIDLIST pidl);
    // *** IPersistFolder2 methods ***
    STDMETHODIMP GetCurFolder(LPITEMIDLIST* ppidl);

    // *** IShellIconOverlay methods ***
    STDMETHODIMP GetOverlayIndex(LPCITEMIDLIST pidl, int* pIndex);
    STDMETHODIMP GetOverlayIconIndex(LPCITEMIDLIST pidl, int* pIconIndex);

protected:
    CDrivesFolder(IUnknown* punkOuter);
    ~CDrivesFolder();

    // used by the CAggregatedUnknown stuff
    HRESULT v_InternalQueryInterface(REFIID riid, void** ppvObj);
    BOOL v_HandleDelete(PLONG pcRef);
    
    STDMETHODIMP CompareItemIDs(LPCIDDRIVE pidd1, LPCIDDRIVE pidd2);
    BOOL TypeIsRemoveable(LPCIDDRIVE pidd);
    static BOOL _GetFreeSpace(LPCIDDRIVE pidd, ULONGLONG *pSize, ULONGLONG *pFree);
    static BOOL _GetFreeSpaceString(LPCIDDRIVE pidd, LPTSTR szBuf, UINT cchBuf);
    static HRESULT _OnChangeNotify(LPARAM lNotification, LPCITEMIDLIST *ppidl);
    static void _FillIDDrive(DRIVE_IDLIST *piddl, int iDrive, BOOL bSimple);
    static LPCIDDRIVE _IsValidID(LPCITEMIDLIST pidl);
    static int _GetSHID(int iDrive);
    static HRESULT _GetCCHMax(LPCITEMIDLIST pidlItem, UINT *pcchMax);
    static HRESULT _GetDisplayNameStrRet(LPCIDDRIVE pidd, STRRET *pStrRet);
    static HRESULT _GetDisplayName(LPCIDDRIVE pidd, LPTSTR pszName, UINT cchMax);
    static HRESULT _CreateFSFolder(LPCITEMIDLIST pidlDrive, LPCIDDRIVE pidd, REFIID riid, void **ppv);
    static void _GetTypeString(BYTE bFlags, LPTSTR pszName, UINT cbName);
    static HRESULT _GetEditTextStrRet(LPCIDDRIVE pidd, STRRET *pstr);
    static BOOL _IsReg(LPCIDDRIVE pidd) { return pidd->bFlags == SHID_COMPUTER_REGITEM; }
    static HRESULT _GetIconOverlayInfo(LPCIDDRIVE pidd, int *pIndex, DWORD dwFlags);

    static CDrivesFolder* _spThis;
    
private:
    friend HRESULT CDrives_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv);
    friend void CDrives_Terminate(void);

    friend CDrivesViewCallback;

    static HRESULT CALLBACK EnumCallBack(LPARAM lParam, void *pvData, UINT ecid, UINT index);
    IUnknown* _punkReg;
};  

#define DRIVES_EVENTS \
    SHCNE_DRIVEADD | \
    SHCNE_DRIVEREMOVED | \
    SHCNE_MEDIAINSERTED | \
    SHCNE_MEDIAREMOVED | \
    SHCNE_NETSHARE | \
    SHCNE_NETUNSHARE | \
    SHCNE_DELETE | \
    SHCNE_RENAMEITEM | \
    SHCNE_RENAMEFOLDER | \
    SHCNE_UPDATEITEM


HRESULT _GetSelectedObjectsFromSite(IUnknown *psite, LPCITEMIDLIST **pppidl, UINT *pcCount)
{
    if (pcCount)
        *pcCount = 0;

    if (pppidl)
        *pppidl = NULL;

    IShellFolderView *psfv;
    HRESULT hres = IUnknown_QueryService(psite, SID_DefView, IID_IShellFolderView, (void **)&psfv); 
    if (SUCCEEDED(hres))
    {
        hres = psfv->GetSelectedObjects(pppidl, pcCount);
        psfv->Release();
    }
    return hres;
}

//
//  Obtains the pidl for the selected item from the defview.  If no objects
//  are selected or more than one object is selected, then returns NULL.
//
//  The returned pidl should NOT be freed.  It belongs to the view.
//
STDAPI_(LPCITEMIDLIST) GetSelectedObjectFromSite(IUnknown *psite)
{
    LPCITEMIDLIST pidl = NULL;
    IShellBrowser *psb;
    if (SUCCEEDED(IUnknown_QueryService(psite, SID_STopLevelBrowser, IID_IShellBrowser, (void **)&psb)))
    {
        UINT cSelected;
        if (SUCCEEDED(_GetSelectedObjectsFromSite(psite, NULL, &cSelected)) && (cSelected == 1))
        {
            LPCITEMIDLIST *apidl;
            if (SUCCEEDED(_GetSelectedObjectsFromSite(psite, &apidl, &cSelected)))
            {
                pidl = apidl[0];
                LocalFree(apidl);
            }
        }
        psb->Release();
    }
    return pidl;
}

HRESULT CDrivesFolder::_GetCCHMax(LPCITEMIDLIST pidlItem, UINT *pcchMax)
{
    LPCIDDRIVE pidd = _IsValidID(pidlItem);
    if (pidd)
    {
        if (pidd->bFlags == SHID_COMPUTER_REGITEM)
            *pcchMax = MAX_REGITEMCCH;
        else
        {
            HRESULT hres = E_FAIL;
            TCHAR szLabel[MAX_LABEL_NTFS + 1];

            CMountPoint* pMtPt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));

            if (pMtPt)
                hres = pMtPt->GetLabel(szLabel, ARRAYSIZE(szLabel));

            if (SUCCEEDED(hres))
            {
                if (pMtPt->IsNTFS())
                    *pcchMax = MAX_LABEL_NTFS;
                else
                    *pcchMax = MAX_LABEL_FAT;
            }

            if (pMtPt)
                pMtPt->Release();
        }
    }
    return NOERROR;
}

class CDrivesViewCallback : public CBaseShellFolderViewCB
{
public:
    CDrivesViewCallback(IShellFolder* psf)
        : CBaseShellFolderViewCB(psf, (LPCITEMIDLIST)&c_idlDrives, DRIVES_EVENTS)
    { 
    }

    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

private:

    HRESULT OnMergeMenu(DWORD pv, QCMINFO*lP)
    {
        CDefFolderMenu_MergeMenu(HINST_THISDLL, 0, POPUP_DRIVES_POPUPMERGE, lP);
        return NOERROR;
    }

    HRESULT OnInvokeCommand(DWORD pv, UINT wP)
    {
        return CDrives_InvokeCommand(m_hwndMain, wP);
    }

    HRESULT OnGETHELPTEXT(DWORD pv, UINT id, UINT cch, LPTSTR psz)
    {
#ifdef UNICODE
        return CDrives_GetHelpText(id, TRUE, (LPARAM)psz, cch);
#else
        return CDrives_GetHelpText(id, FALSE, (LPARAM)psz, cch);
#endif
    }

    HRESULT OnInsertItem(DWORD pv, LPCITEMIDLIST wP)
    {
        LPIDDRIVE pidd = (LPIDDRIVE)wP;
        if (pidd && pidd->bFlags != SHID_COMPUTER_REGITEM)
        {
            // clear the size info
            pidd->qwSize = pidd->qwFree = 0;
        }
        return NOERROR;
    }

    HRESULT OnWindowCreated(DWORD pv, HWND wP)
    {
        InitializeStatus(_punkSite);
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

    HRESULT OnUpdateStatusBar(DWORD pv, BOOL fIniting)
    {
        // Ask DefView to set the default text but not initialize
        // since we did the initialization in our OnSize handler.
        return SFVUSB_INITED;
    }

    HRESULT OnFSNotify(DWORD pv, LPCITEMIDLIST*wP, LPARAM lP)
    {
        return CDrivesFolder::_OnChangeNotify(lP, wP);
    }

    HRESULT OnBACKGROUNDENUM(DWORD pv)
    {
        return S_OK;
    }

    HRESULT OnDEFITEMCOUNT(DWORD pv, UINT*lP)
    {
        // if we time out enuming items make a guess at how many items there
        // will be to make sure we get large icon mode and a reasonable window size

        *lP = 20;
        return S_OK;
    }

    HRESULT OnGetCCHMax(DWORD pv, LPCITEMIDLIST pidlItem, UINT *pcchMax)
    {
        return CDrivesFolder::_GetCCHMax(pidlItem, pcchMax);
    }

    HRESULT OnSupportsIdentity(DWORD pv)
    {
        return S_OK;
    }
};

STDMETHODIMP CDrivesViewCallback::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_MERGEMENU, OnMergeMenu);
    HANDLE_MSG(0, SFVM_INVOKECOMMAND, OnInvokeCommand);
    HANDLE_MSG(0, SFVM_GETHELPTEXT, OnGETHELPTEXT);
    HANDLE_MSG(0, SFVM_INSERTITEM, OnInsertItem);
    HANDLE_MSG(0, SFVM_UPDATESTATUSBAR, OnUpdateStatusBar);
    HANDLE_MSG(0, SFVM_FSNOTIFY, OnFSNotify);
    HANDLE_MSG(0, SFVM_BACKGROUNDENUM, OnBACKGROUNDENUM);
    HANDLE_MSG(0, SFVM_DEFITEMCOUNT, OnDEFITEMCOUNT);
    HANDLE_MSG(0, SFVM_ADDPROPERTYPAGES, SFVCB_OnAddPropertyPages);
    HANDLE_MSG(0, SFVM_WINDOWCREATED, OnWindowCreated);
    HANDLE_MSG(0, SFVM_SIZE, OnSize);
    HANDLE_MSG(0, SFVM_GETPANE, OnGetPane);
    HANDLE_MSG(0, SFVM_GETCCHMAX, OnGetCCHMax);
    HANDLE_MSG(0, SFVM_SUPPORTSIDENTITY, OnSupportsIdentity);

    default:
        return E_FAIL;
    }

    return NOERROR;
}

STDAPI_(IShellFolderViewCB*) CDrives_CreateSFVCB(IShellFolder* psf)
{
    return new CDrivesViewCallback(psf);
}

typedef struct
{
    DWORD       dwDrivesMask;
    int         nLastFoundDrive;
    DWORD       dwRestricted;
    DWORD       dwSavedErrorMode;
    DWORD       grfFlags;
} EnumDrives;

typedef enum
{
    DRIVES_ICOL_NAME = 0,
    DRIVES_ICOL_TYPE,
    DRIVES_ICOL_SIZE,
    DRIVES_ICOL_FREE,
    DRIVES_ICOL_COMMENT,
    DRIVES_ICOL_HTMLINFOTIPFILE,
} EnumDrivesCols;

const COL_DATA c_drives_cols[] = {
    { DRIVES_ICOL_NAME,         IDS_DRIVES_NAME, 20,    LVCFMT_LEFT,    &SCID_NAME},
    { DRIVES_ICOL_TYPE,         IDS_DRIVES_TYPE, 25,    LVCFMT_LEFT,    &SCID_TYPE},
    { DRIVES_ICOL_SIZE,         IDS_DRIVES_SIZE, 15,    LVCFMT_RIGHT,   &SCID_SIZE},
    { DRIVES_ICOL_FREE,         IDS_DRIVES_FREE, 15,    LVCFMT_RIGHT,   &SCID_FREESPACE},
    { DRIVES_ICOL_COMMENT,      IDS_COMMENT_COL, 20,    LVCFMT_LEFT,    &SCID_Comment},
    { DRIVES_ICOL_HTMLINFOTIPFILE, IDS_COMMENT_COL, 20, LVCFMT_LEFT,    &SCID_HTMLINFOTIPFILE}
};

const TCHAR c_szVolumeNamePrefix[]  = TEXT("\\\\?\\Volume{");

CDrivesFolder* CDrivesFolder::_spThis = NULL;

HRESULT CDrives_CreateInstance(IUnknown* punkOuter, REFIID riid, void** ppv)
{
    HRESULT hres;
    ASSERT(NULL != ppv);
    
    ENTERCRITICAL;
    if (NULL != CDrivesFolder::_spThis)
    {
        hres = CDrivesFolder::_spThis->QueryInterface(riid, ppv);
        LEAVECRITICAL;
    }
    else
    {
        LEAVECRITICAL;
        CDrivesFolder* pDF = new CDrivesFolder(punkOuter);
        if (NULL != pDF)
        {
            ASSERT(NULL == pDF->_punkReg);
            if (!SHRestricted(REST_NOSETFOLDERS))
            {
                if (SHRestricted(REST_NOCONTROLPANEL))
                    g_asDrivesReqItems[CDRIVES_REGITEM_CONTROL].dwAttributes |= SFGAO_NONENUMERATED;
                    
                CRegFolder_CreateInstance(&g_sDrivesRegInfo,
                                          (IUnknown*) (IShellFolder2*) pDF,
                                          IID_IUnknown,
                                          (void **) &pDF->_punkReg);
            }

            if (SHInterlockedCompareExchange((void**) &CDrivesFolder::_spThis, pDF, NULL))
            {
                // Someone else snuck in and initialized a CDrivesFolder first,
                // so release our object and then recurse so we should get the other instance
                pDF->Release();
                hres = CDrives_CreateInstance(punkOuter, riid, ppv);
            }
            else
            {
                hres = pDF->QueryInterface(riid, ppv);

                // release the self-reference, but keep _spThis intact
                // (it will be reset to NULL in the destructor)
                pDF->Release();
            }
        }
        else
        {
            hres = E_OUTOFMEMORY;
            *ppv = NULL;
        }
    }
    return hres;
}

// This should only be called during process detach
void CDrives_Terminate(void)
{
    if (NULL != CDrivesFolder::_spThis)
    {
        delete CDrivesFolder::_spThis;
    }
}

CDrivesFolder::CDrivesFolder(IUnknown* punkOuter) : 
    CAggregatedUnknown      (punkOuter),
    _punkReg                (NULL)
{
    DllAddRef();
}

CDrivesFolder::~CDrivesFolder()
{
    SHReleaseInnerInterface(SAFECAST(this, IShellFolder *), &_punkReg);
    SHInterlockedCompareExchange((void**) &CDrivesFolder::_spThis, NULL, this);
    DllRelease();
}

HRESULT CDrivesFolder::v_InternalQueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENT(CDrivesFolder, IShellFolder2),                        // IID_IShellFolder2
        QITABENTMULTI(CDrivesFolder, IShellFolder, IShellFolder2),     // IID_IShellFolder
        QITABENT(CDrivesFolder, IPersistFolder2),                      // IID_IPersistFolder2
        QITABENTMULTI(CDrivesFolder, IPersistFolder, IPersistFolder2), // IID_IPersistFolder
        QITABENTMULTI(CDrivesFolder, IPersist, IPersistFolder2),       // IID_IPersist
        QITABENTMULTI2(CDrivesFolder, IID_IPersistFreeThreadedObject, IPersist), // IID_IPersistFreeThreadedObject
        QITABENT(CDrivesFolder, IShellIconOverlay),                    // IID_IShellIconOverlay
        { 0 },
    };

    if (_punkReg && (IsEqualIID(riid, IID_IShellFolder) || IsEqualIID(riid, IID_IShellFolder2)))
    {
        return _punkReg->QueryInterface(riid, ppv);
    }
    else
    {
        HRESULT hr = QISearch(this, qit, riid, ppv);
        if ((E_NOINTERFACE == hr) && (NULL != _punkReg))
        {
            return _punkReg->QueryInterface(riid, ppv);
        }
        else
        {
            return hr;
        }
    }
}

BOOL CDrivesFolder::v_HandleDelete(PLONG pcRef)
{
    ASSERT(NULL != pcRef);
    ENTERCRITICAL;

    //
    //  The same bad thing can happen here as in
    //  CNetRootFolder::v_HandleDelete.  See that function for gory details.
    //
    if (this == _spThis && 0 == *pcRef)
    {
        *pcRef = 1000; // protect against cached pointers bumping us up then down
        delete this;
    }
    LEAVECRITICAL;
    // return TRUE to indicate that we've implemented this function
    // (regardless of whether or not this object was actually deleted)
    return TRUE;
}


HRESULT CDrivesFolder::_GetDisplayName(LPCIDDRIVE pidd, LPTSTR pszName, UINT cchMax)
{
    HRESULT hres = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));

    if (pMtPt)
    {
        hres = pMtPt->GetDisplayName(pszName, cchMax);

        pMtPt->Release();
    }

    return hres;
}

HRESULT CDrivesFolder::_GetDisplayNameStrRet(LPCIDDRIVE pidd, STRRET *pStrRet)
{
    HRESULT hres = E_FAIL;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));
    if (pMtPt)
    {
        TCHAR szName[MAX_DISPLAYNAME];

        hres = pMtPt->GetDisplayName(szName, ARRAYSIZE(szName));
        if (SUCCEEDED(hres))
            hres = StringToStrRet(szName, pStrRet);

        pMtPt->Release();
    }
    return hres;
}

int CDrivesFolder::_GetSHID(int iDrive)
{
    int bType = SHID_COMPUTER_MISC;
    CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

    if (pMtPt)
    {
        bType = pMtPt->GetSHIDType(FALSE);
        pMtPt->Release();
    }

    return bType;
}

// hash the display name to detect volume lable and other changes on the drive

WORD DriveNameHash(LPCTSTR pszDriveName)
{
    BYTE bCheckPlus, bCheckXor;

    bCheckXor = bCheckPlus = (BYTE)*pszDriveName;
    for (pszDriveName = CharNext(pszDriveName); *pszDriveName; pszDriveName = CharNext(pszDriveName))
    {
        bCheckPlus += (BYTE)*pszDriveName;
        bCheckXor = (bCheckXor << 1) ^ (BYTE)*pszDriveName;
    }
    return (WORD)bCheckPlus << 8 | (WORD)bCheckXor;
}

void CDrivesFolder::_FillIDDrive(DRIVE_IDLIST *piddl, int iDrive, BOOL bSimple)
{
    TCHAR szDriveName[MAX_PATH];

    if (!CMountPoint::GetDriveIDList(iDrive, piddl))
    {
        memset(piddl, 0, sizeof(*piddl));

        piddl->idd.bFlags = bSimple ? SHID_COMPUTER_MISC : (BYTE)_GetSHID(iDrive);
        PathBuildRoot(szDriveName, iDrive);
        SHTCharToAnsi(szDriveName, piddl->idd.cName, ARRAYSIZE(piddl->idd.cName));

        if (!bSimple)
        {
            _GetDisplayName(&piddl->idd, szDriveName, ARRAYSIZE(szDriveName));
            piddl->idd.wChecksum = DriveNameHash(szDriveName);
        }
        piddl->idd.cb = SIZEOF(IDDRIVE);
        ASSERT(piddl->cbNext == 0);

        if (!bSimple)
            CMountPoint::SetDriveIDList(iDrive, piddl);
    }
}


STDMETHODIMP CDrivesFolder::ParseDisplayName(HWND hwnd, LPBC pbc,
                                             LPOLESTR pwzDisplayName, ULONG* pchEaten,
                                             LPITEMIDLIST* ppidlOut, ULONG* pdwAttributes)
{
    HRESULT hres = E_INVALIDARG;

    *ppidlOut = NULL;   // assume error

    if (pwzDisplayName && pwzDisplayName[0] && 
        pwzDisplayName[1] == TEXT(':') && pwzDisplayName[2] == TEXT('\\'))
    {
        DRIVE_IDLIST idlDrive;
        UINT iDrive;

        if (InRange(*pwzDisplayName, 'a', 'z'))
        {
            iDrive = *pwzDisplayName - 'a';
        }
        else if (InRange(*pwzDisplayName, 'A', 'Z'))
        {
            iDrive = *pwzDisplayName - 'A';
        }
        else
        {
            TraceMsg(TF_WARNING, "CDrivesFolder::ParseDisplayName(), failing:%hs", pwzDisplayName);
            return E_INVALIDARG;
        }

        _FillIDDrive(&idlDrive, iDrive, SHIsFileSysBindCtx(pbc, NULL) == S_OK);

        // Check if there are any subdirs
        if (pwzDisplayName[3])
        {
            IShellFolder *psfDrive;
            hres = BindToObject((LPITEMIDLIST)&idlDrive, pbc, IID_IShellFolder, (void **) &psfDrive);
            if (SUCCEEDED(hres))
            {
                ULONG chEaten;
                LPITEMIDLIST pidlDir;
                hres = psfDrive->ParseDisplayName(  hwnd,
                                                    pbc,
                                                    pwzDisplayName + 3,
                                                    &chEaten,
                                                    &pidlDir,
                                                    pdwAttributes);
                if (SUCCEEDED(hres))
                {
                    hres = SHILCombine((LPCITEMIDLIST)&idlDrive, pidlDir, ppidlOut);
                    SHFree(pidlDir);
                }
                psfDrive->Release();
            }
        }
        else
        {
            hres = SHILClone((LPITEMIDLIST)&idlDrive, ppidlOut);
            if (pdwAttributes && *pdwAttributes)
                GetAttributesOf(1, (LPCITEMIDLIST*) ppidlOut, pdwAttributes);
        }
    }
#ifdef WINNT
    else
    {//check if dealing with mounteddrive
     //something like: "\\?\Volume{9e2df3f5-c7f1-11d1-84d5-000000000000}\" (without quotes)
        if (0 == StrCmpNI(c_szVolumeNamePrefix, pwzDisplayName, ARRAYSIZE(c_szVolumeNamePrefix)))
        {
            TraceMsg(TF_WARNING, "CDrivesFolder::ParseDisplayName(), Volume GUID (%ls)", pwzDisplayName);
            //fail anyway, not implemented yet
        }
    }
#endif
    if (FAILED(hres))
        TraceMsg(TF_WARNING, "CDrivesFolder::ParseDisplayName(), hres:%x %ls", hres, pwzDisplayName);

    return hres;
}

STDMETHODIMP CDrivesFolder::EnumObjects(HWND hwnd, DWORD grfFlags, IEnumIDList ** ppenumUnknown)
{
    // We should always create enum objects with this helper call.
    EnumDrives * pedrv = (EnumDrives *)LocalAlloc(LPTR, SIZEOF(EnumDrives));
    if (pedrv)
    {
        pedrv->dwDrivesMask = GetLogicalDrives();
        pedrv->nLastFoundDrive = -1;
        pedrv->dwRestricted = SHRestricted(REST_NODRIVES);
        pedrv->dwSavedErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        pedrv->grfFlags = grfFlags;
        return SHCreateEnumObjects(hwnd, pedrv, EnumCallBack, ppenumUnknown);
    }
    else
    {
        *ppenumUnknown = NULL;
        return E_OUTOFMEMORY;
    }
}


LPCIDDRIVE CDrivesFolder::_IsValidID(LPCITEMIDLIST pidl)
{
    if (pidl && (SIL_GetType(pidl) & SHID_GROUPMASK) == SHID_COMPUTER)
        return (LPCIDDRIVE)pidl;
    return NULL;
}

HRESULT CDrivesFolder::_CreateFSFolder(LPCITEMIDLIST pidlDrive, LPCIDDRIVE pidd, REFIID riid, void **ppv)
{
    PERSIST_FOLDER_TARGET_INFO pfti = {0};
    
    pfti.pidlTargetFolder = (LPITEMIDLIST)pidlDrive;
    SHAnsiToUnicode(pidd->cName, pfti.szTargetParsingName, ARRAYSIZE(pfti.szTargetParsingName));
    pfti.dwAttributes = FILE_ATTRIBUTE_DIRECTORY; // maybe add system?
    pfti.csidl = -1;

    return CFSFolder_CreateFolder(NULL, pidlDrive, &pfti, riid, ppv);
}

STDMETHODIMP CDrivesFolder::BindToObject(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppv)
{
    HRESULT hres;
    LPCIDDRIVE pidd;

    *ppv = NULL;

    pidd = _IsValidID(pidl);
    if (pidd)
    {
        LPCITEMIDLIST pidlNext = _ILNext(pidl);
        LPITEMIDLIST pidlDrive = ILCombineParentAndFirst(IDLIST_DRIVES, pidl, pidlNext);
        if (pidlDrive)
        {
            //  we only try ask for the riid at the end of the pidl binding.
            if (ILIsEmpty(pidlNext))
            {
                hres = _CreateFSFolder(pidlDrive, pidd, riid, ppv);
            }
            else
            {
                //  now we need to get the subfolder from which to grab our goodies
                IShellFolder *psfDrive;
                hres = _CreateFSFolder(pidlDrive, pidd, IID_IShellFolder, (void **)&psfDrive);
                if (SUCCEEDED(hres))
                {
                    //  this means that there is more to bind to, we must pass it on...
                    hres = psfDrive->BindToObject(pidlNext, pbc, riid, ppv);
                    psfDrive->Release();
                }
            }
            ILFree(pidlDrive);
        }
        else
            hres = E_OUTOFMEMORY;
    }
    else
    {
        hres = E_INVALIDARG;
        TraceMsg(TF_WARNING, "CDrivesFolder::BindToObject(), bad PIDL %s", DumpPidl(pidl));
    }

    return hres;
}

STDMETHODIMP CDrivesFolder::BindToStorage(LPCITEMIDLIST pidl, LPBC pbc, REFIID riid, void** ppv)
{
    ASSERT(NULL != ppv);
    *ppv = NULL;
    return E_NOTIMPL;
}

STDAPI_(void) CDrivesFolder::_GetTypeString(BYTE bFlags, LPTSTR pszName, UINT cbName)
{
    CMountPoint::GetTypeString(bFlags, pszName, cbName);
}

BOOL CDrivesFolder::_GetFreeSpace(LPCIDDRIVE pidd, ULONGLONG *pSize, ULONGLONG *pFree)
{
    BOOL bRet = FALSE;
    if (!_IsReg(pidd) && ShowDriveInfo(DRIVEID(pidd->cName)))
    {
        if (pidd->qwSize || pidd->qwFree)
        {
            *pSize = pidd->qwSize;      // cache hit
            *pFree = pidd->qwFree;
            bRet = TRUE;
        }
        else
        {
            int iDrive = DRIVEID(pidd->cName);
            // Don't wake up sleeping net connections
            if (!IsRemoteDrive(iDrive) || !IsDisconnectedNetDrive(iDrive))
            {
                // Call our helper function Who understands
                // OSR2 and NT as well as old W95...
                ULARGE_INTEGER qwFreeUser, qwTotal, qwTotalFree;
                bRet = SHGetDiskFreeSpaceExA(pidd->cName, &qwFreeUser, &qwTotal, &qwTotalFree);
                if (bRet)
                {
                    *pSize = qwTotal.QuadPart;
                    *pFree = qwFreeUser.QuadPart;
                }
            }
        }
    }
    return bRet;
}

STDMETHODIMP CDrivesFolder::CompareIDs(LPARAM iCol, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    HRESULT hres;
    LPCIDDRIVE pidd1 = _IsValidID(pidl1);
    LPCIDDRIVE pidd2 = _IsValidID(pidl2);

    if (!pidd1 || !pidd2)
    {
        TraceMsg(TF_WARNING, "CDrivesFolder::CompareIDs(), bad(s) pidl11:%s, pidl2:%s", DumpPidl(pidl1), DumpPidl(pidl2));
        return E_INVALIDARG;
    }

    //
    //  For any column other than DRIVES_ICOL_NAME, we force an
    //  all-fields comparison to break ties.
    //
    if ((iCol & SHCIDS_COLUMNMASK) != DRIVES_ICOL_NAME) 
        iCol |= SHCIDS_ALLFIELDS;

    switch (iCol & SHCIDS_COLUMNMASK) {

    default:                    // If asking for unknown column, just use name
    case DRIVES_ICOL_NAME:
        hres = ResultFromShort(StrCmpICA(pidd1->cName, pidd2->cName));
        break;

    case DRIVES_ICOL_TYPE:
    {
        TCHAR szName1[80], szName2[80];
        _GetTypeString(pidd1->bFlags, szName1, ARRAYSIZE(szName1));
        _GetTypeString(pidd2->bFlags, szName2, ARRAYSIZE(szName2));
        hres = ResultFromShort(ustrcmpi(szName1, szName2));
        break;
    }

    case DRIVES_ICOL_SIZE:
    case DRIVES_ICOL_FREE:
        {
            ULONGLONG qwSize1, qwFree1;
            ULONGLONG qwSize2, qwFree2;

            BOOL fGotInfo1 = _GetFreeSpace(pidd1, &qwSize1, &qwFree1);
            BOOL fGotInfo2 = _GetFreeSpace(pidd2, &qwSize2, &qwFree2);

            if (fGotInfo1 && fGotInfo2) 
            {
                ULONGLONG i1, i2;  // this is a "guess" at the disk size and free space

                if ((iCol & SHCIDS_COLUMNMASK) == DRIVES_ICOL_SIZE)
                {
                    i1 = qwSize1;
                    i2 = qwSize2;
                } 
                else 
                {
                    i1 = qwFree1;
                    i2 = qwFree2;
                }

                if (i1 == i2)
                    hres = ResultFromShort(0);
                else if (i1 < i2)
                    hres = ResultFromShort(-1);
                else
                    hres = ResultFromShort(1);
            } 
            else if (!fGotInfo1 && !fGotInfo2) 
            {
                hres = ResultFromShort(0);
            } 
            else 
            {
                hres = ResultFromShort(fGotInfo1 - fGotInfo2);
            }
        }
        break;
    }

    // if they were the same so far, and we forcing an all-fields
    // comparison, then use the all-fields comparison to break ties.
    //
    if (hres == S_OK && (iCol & SHCIDS_ALLFIELDS)) 
        hres = CompareItemIDs(pidd1, pidd2);

    //
    //  If the items are still the same, then ask ILCompareRelIDs
    //  to walk recursively to the next ids.
    //
    if (hres == S_OK)
        hres = ILCompareRelIDs((IShellFolder*) this, pidl1, pidl2);

    return hres;
}

STDMETHODIMP CDrivesFolder::CreateViewObject(HWND hwnd, REFIID riid, void** ppv)
{
    // We should not get here unless we have initialized properly

    if (IsEqualIID(riid, IID_IShellView))
    {
        SFV_CREATE sSFV;
        HRESULT hres;

        sSFV.cbSize   = sizeof(sSFV);
        sSFV.psvOuter = NULL;
        sSFV.psfvcb   = CDrives_CreateSFVCB((IShellFolder*) this);

        QueryInterface(IID_IShellFolder, (void **)&sSFV.pshf);   // in case we are agregated

        hres = SHCreateShellFolderView(&sSFV, (IShellView**)ppv);

        if (sSFV.pshf)
            sSFV.pshf->Release();

        if (sSFV.psfvcb)
            sSFV.psfvcb->Release();

        return hres;
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        return CIDLDropTarget_Create(hwnd, &c_CDrivesDropTargetVtbl,
                        IDLIST_DRIVES, (IDropTarget **)ppv);
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        return CDefFolderMenu_Create(IDLIST_DRIVES, hwnd, 0, NULL, 
            (IShellFolder*) this, CDrives_DFMCallBackBG, NULL, NULL, (IContextMenu **)ppv);
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP CDrivesFolder::GetAttributesOf(UINT cidl, LPCITEMIDLIST* apidl, ULONG* prgfInOut)
{
    UINT rgfOut = SFGAO_HASSUBFOLDER | SFGAO_CANLINK | SFGAO_CANMONIKER
                | SFGAO_DROPTARGET | SFGAO_HASPROPSHEET | SFGAO_FOLDER
                | SFGAO_FILESYSTEM | SFGAO_FILESYSANCESTOR | SFGAO_FOLDER;

    if (cidl == 0)
    {
        if (*prgfInOut & SFGAO_VALIDATE)
            InvalidateDriveType(-1);    // nuke all the cached drive types on a real refresh (F5)

        // We are getting the attributes for the "MyComputer" folder itself.
        rgfOut = (*prgfInOut & g_asDesktopReqItems[CDESKTOP_REGITEM_DRIVES].dwAttributes);
    }
    else if (cidl == 1)
    {
        TCHAR szDrive[MAX_PATH];
        LPIDDRIVE pidd = (LPIDDRIVE)apidl[0];

        SHAnsiToTChar(pidd->cName, szDrive, ARRAYSIZE(szDrive));

        // If caller wants compression status, we need to ask the filesystem

        if (*prgfInOut & SFGAO_COMPRESSED)
        {
            int iDrive = DRIVEID(pidd->cName);

            // Don't wake up sleeping net connections
            if (!IsRemoteDrive(iDrive) || !IsDisconnectedNetDrive(iDrive))
            {
                if (CMountPoint::IsCompressed(iDrive))
                    rgfOut |= SFGAO_COMPRESSED;
            }
        }

        if (*prgfInOut & SFGAO_SHARE)
        {
            if (IsShared(szDrive, FALSE))
                rgfOut |= SFGAO_SHARE;
        }

        if ((*prgfInOut & SFGAO_REMOVABLE) && TypeIsRemoveable(pidd))
            rgfOut |= SFGAO_REMOVABLE;

        // we need to also handle the SFGAO_READONLY bit.
        if (*prgfInOut & SFGAO_READONLY)
        {
            DWORD dwErrMode = SetErrorMode(SEM_FAILCRITICALERRORS);
            DWORD dwAttributes = GetFileAttributes(szDrive);
    
            SetErrorMode(dwErrMode);
    
            if (dwAttributes != -1 && dwAttributes & FILE_ATTRIBUTE_READONLY)
                rgfOut |= SFGAO_READONLY;
        }

        if ((*prgfInOut & SFGAO_CANRENAME) && 
            (TypeIsRemoveable(pidd) || pidd->bFlags == SHID_COMPUTER_FIXED
            || pidd->bFlags == SHID_COMPUTER_NETDRIVE))
        {
            rgfOut |= SFGAO_CANRENAME;
        }
    }

    *prgfInOut = rgfOut;
    return NOERROR;
}

HRESULT CDrivesFolder::_GetEditTextStrRet(LPCIDDRIVE pidd, STRRET *pstr)
{
    HRESULT hres = E_FAIL;
    TCHAR szEdit[MAX_PATH];

    CMountPoint* pMtPt = CMountPoint::GetMountPoint(DRIVEID(pidd->cName));

    if (pMtPt)
    {
        hres = pMtPt->GetLabel(szEdit, ARRAYSIZE(szEdit));
        pMtPt->Release();
    }

    if (SUCCEEDED(hres))
        hres = StringToStrRet(szEdit, pstr);

    return hres;
}


STDMETHODIMP CDrivesFolder::GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, STRRET* pStrRet)
{
    HRESULT hres;
    LPCIDDRIVE pidd = _IsValidID(pidl);
    if (pidd)
    {
        TCHAR szDrive[ARRAYSIZE(pidd->cName)];
        LPCITEMIDLIST pidlNext = _ILNext(pidl); // Check if pidl contains more than one ID

        SHAnsiToTChar(pidd->cName, szDrive, ARRAYSIZE(szDrive));

        if (ILIsEmpty(pidlNext))
        {
            if (uFlags & SHGDN_FORPARSING)
            {
                hres = StringToStrRet(szDrive, pStrRet);
            }
            else if (uFlags & SHGDN_FOREDITING)
            {
                hres = _GetEditTextStrRet(pidd, pStrRet);
            }
            else
                hres = _GetDisplayNameStrRet(pidd, pStrRet);
        }
        else
        {
            LPITEMIDLIST pidlDrive = ILCombineParentAndFirst(IDLIST_DRIVES, pidl, pidlNext);
            if (pidlDrive)
            {
                //  now we need to get the subfolder from which to grab our goodies
                IShellFolder *psfDrive;
                hres = _CreateFSFolder(pidlDrive, pidd, IID_IShellFolder, (void **)&psfDrive);
                if (SUCCEEDED(hres))
                {
                    hres = psfDrive->GetDisplayNameOf(pidlNext, uFlags, pStrRet);
                    psfDrive->Release();
                }
                ILFree(pidlDrive);
            }
            else
                hres = E_OUTOFMEMORY;
        }
    }
    else
    {
        hres = E_INVALIDARG;
        TraceMsg(TF_WARNING, "CDrivesFolder::GetDisplayNameOf() bad PIDL %s", DumpPidl(pidl));
    }
    return hres;
}

STDMETHODIMP CDrivesFolder::SetNameOf(HWND hwnd, LPCITEMIDLIST pidl, 
                                      LPCWSTR pszName, DWORD dwReserved, LPITEMIDLIST* ppidlOut)
{
    HRESULT hr = E_INVALIDARG;
    LPCIDDRIVE pidd = _IsValidID(pidl);
    if (pidd)
    {
        TCHAR szName[MAX_LABEL_NTFS + 1];

        if (ppidlOut)
            *ppidlOut = NULL;

        SHUnicodeToTChar(pszName, szName, ARRAYSIZE(szName));

        // BUGBUG: We need to impl ::SetSite() and pass it to SetDriveLabel
        //         to go modal if we display UI.
        hr = SetDriveLabel(hwnd, NULL, DRIVEID(pidd->cName), szName);
    }
    return hr;
}

BOOL CDrivesFolder::_GetFreeSpaceString(LPCIDDRIVE pidd, LPTSTR szBuf, UINT cchBuf)
{
    ULONGLONG qwSize, qwFree;
    BOOL bRet = _GetFreeSpace(pidd, &qwSize, &qwFree);
    if (bRet)
    {
        TCHAR szFree[30], szTotal[30];

        StrFormatByteSize64(qwSize, szTotal, ARRAYSIZE(szTotal));
        StrFormatByteSize64(qwFree, szFree, ARRAYSIZE(szFree));

        LPTSTR psz = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_DRIVESSTATUSTEMPLATE),
                        szFree, szTotal);
        if (psz)
        {
            StrCpyN(szBuf, psz, cchBuf);
            LocalFree(psz);
        }
        else
        {
            bRet = FALSE;
        }
    }
    return bRet;
}

HKEY _GetDriveKey(int iDrive)
{
    HKEY hkRet = NULL;

    if (CMountPoint::IsAudioCD(iDrive))
    {
        RegOpenKey(HKEY_CLASSES_ROOT, TEXT("AudioCD"), &hkRet);
    }
    else if (CMountPoint::IsAutoRun(iDrive))
    {
        CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

        if (pMtPt)
        {
            hkRet = pMtPt->GetRegKey();
            pMtPt->Release();
        }
    }
    else if (CMountPoint::IsDVD(iDrive))
    {
        RegOpenKey(HKEY_CLASSES_ROOT, TEXT("DVD"), &hkRet);
    }
    else
    {
        CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

        if (pMtPt)
        {
            hkRet = pMtPt->GetRegKey();
            pMtPt->Release();
        }
    }
    return hkRet;
}

void CDrives_GetKeys(LPCIDDRIVE pidd, HKEY *keys)
{
    keys[0] = NULL;
    keys[1] = NULL;
    keys[2] = NULL;

    if (pidd)
        keys[0] = _GetDriveKey(DRIVEID(pidd->cName));

    RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Drive"), &keys[1]);
    RegOpenKey(HKEY_CLASSES_ROOT, c_szFolderClass, &keys[2]);
}

HRESULT CDrives_AssocCreate(LPCIDDRIVE pidd, REFIID riid, void **ppv)
{
    IQueryAssociations *pqa;
    HRESULT hr = AssocCreate(CLSID_QueryAssociations, IID_IQueryAssociations, (LPVOID*)&pqa);

    *ppv = NULL;
    
    if (SUCCEEDED(hr))
    {
        HKEY hkDrive = _GetDriveKey(DRIVEID(pidd->cName));

        if (hkDrive)
        {
            hr = pqa->Init(ASSOCF_INIT_DEFAULTTOFOLDER, NULL, hkDrive, NULL);
            RegCloseKey(hkDrive);
        }
        else
            hr = pqa->Init(ASSOCF_INIT_DEFAULTTOFOLDER, L"Drive", NULL, NULL);

        if (SUCCEEDED(hr))
            hr = pqa->QueryInterface(riid, ppv);

        pqa->Release();
    }
    return hr;
}

// BUGBUG (stephstm) - remove this
const ICONMAP c_aicmpDrive[] = {
    { SHID_COMPUTER_DRIVE525 , II_DRIVE525      },
    { SHID_COMPUTER_DRIVE35  , II_DRIVE35       },
    { SHID_COMPUTER_FIXED    , II_DRIVEFIXED    },
    { SHID_COMPUTER_REMOTE   , II_DRIVEFIXED    },
    { SHID_COMPUTER_CDROM    , II_DRIVECD       },
    { SHID_COMPUTER_NETDRIVE , II_DRIVENET      },
    { SHID_COMPUTER_REMOVABLE, II_DRIVEREMOVE   },
    { SHID_COMPUTER_RAMDISK  , II_DRIVERAM      },
};

const int c_nicmpDrives = ARRAYSIZE(c_aicmpDrive);

STDMETHODIMP CDrivesFolder::GetUIObjectOf(HWND hwnd, UINT cidl, LPCITEMIDLIST* apidl,
                                          REFIID riid, UINT* prgfInOut, void** ppv)
{
    HRESULT hres;
    HKEY keys[3];
    IDDRIVE idDrive;
    LPCIDDRIVE pidd = (cidl && apidl) ? _IsValidID(apidl[0]) : NULL;

    *ppv = NULL;

    if (pidd == NULL)
        return E_INVALIDARG;

    if (pidd->bFlags == SHID_COMPUTER_MISC)
    {
        // deal with simple IDLISTs
        idDrive.cb = SIZEOF(idDrive);
        idDrive.cName[0] = pidd->cName[0];
        idDrive.cName[1] = TEXT(':');
        idDrive.cName[2] = 0;
        idDrive.bFlags = (BYTE) _GetSHID(DRIVEID(pidd->cName));
        pidd = &idDrive;
    }

    if (IsEqualIID(riid, IID_IExtractIconA) || IsEqualIID(riid, IID_IExtractIconW) && pidd)
    {
        UINT iIndex = SILGetIconIndex((LPCITEMIDLIST)pidd, c_aicmpDrive, c_nicmpDrives);

        CDrives_GetKeys(pidd, keys);

        switch (pidd->bFlags & SHID_TYPEMASK) 
        {
        case SHID_COMPUTER_CDROM:
        {
            TCHAR szIconPath[MAX_PATH];
            LONG cbIconPath = ARRAYSIZE(szIconPath) * sizeof(TCHAR);

            if (CMountPoint::IsAudioCD(DRIVEID(pidd->cName)))
                iIndex = II_CDAUDIO;

            // BUGBUG: (?) maybe we should check for the presence of a CD instead.  Here if the icon
            // is in shell32.dll for ex, then even if there is no CD in the drive we'll still have the
            // icon (stephstm)
            if (SHRegQueryValue(keys[0], c_szDefaultIcon, szIconPath, &cbIconPath) == ERROR_SUCCESS &&
                szIconPath[0])
            {
                int iIcon = PathParseIconLocation(szIconPath);

                if (!PathFileExistsAndAttributes(szIconPath, NULL))
                {
                    SHDeleteValue(keys[0], c_szDefaultIcon, NULL);
                }
            }

            break;
        }
        case SHID_COMPUTER_NETDRIVE:
            if (IsUnavailableNetDrive(DRIVEID(pidd->cName)) ||
                IsDisconnectedNetDrive(DRIVEID(pidd->cName)))
            {
                iIndex = II_DRIVENETDISABLED;
            }

            
            break;

        // bFlags stays SHID_COMPUTER_MISC if the drive is bogus.
        case SHID_COMPUTER_MISC:
            iIndex = -IDI_DRIVEUNKNOWN;
            break;
        }

        hres = SHCreateDefExtIconKey(keys[0], NULL, iIndex, iIndex, GIL_PERCLASS, riid, ppv);

        SHRegCloseKeys(keys, ARRAYSIZE(keys));
    }
    else if (IsEqualIID(riid, IID_IContextMenu))
    {
        CDrives_GetKeys(pidd, keys);

        hres = CDefFolderMenu_Create2(IDLIST_DRIVES, hwnd, cidl, apidl, 
            (IShellFolder*) this, CDrives_DFMCallBack, ARRAYSIZE(keys), keys, (IContextMenu **)ppv);

        SHRegCloseKeys(keys, ARRAYSIZE(keys));
    }
    else if (IsEqualIID(riid, IID_IDataObject))
    {
        hres = FS_CreateFSIDArray(IDLIST_DRIVES, cidl, apidl, NULL, (IDataObject **)ppv);
    }
    else if (IsEqualIID(riid, IID_IDropTarget) && pidd)
    {
        IShellFolder *psfT;
        hres = BindToObject((LPCITEMIDLIST)pidd, NULL, IID_IShellFolder, (void **)&psfT);
        if (SUCCEEDED(hres))
        {
            hres = psfT->CreateViewObject(hwnd, IID_IDropTarget, ppv);
            psfT->Release();
        }
    }
    else if (IsEqualIID(riid, IID_IQueryInfo) && pidd)
    {
        TCHAR szStatus[80];
        if (_GetFreeSpaceString(pidd, szStatus, ARRAYSIZE(szStatus)))
            hres = CreateInfoTipFromText(szStatus, riid, ppv);
        else
            hres = E_NOINTERFACE;
    }
    else if (IsEqualIID(riid, IID_IQueryAssociations))
    {
        hres = CDrives_AssocCreate(pidd, riid, ppv);
    }
    else
    {
        hres = E_NOINTERFACE;
    }

    return hres;
}

STDMETHODIMP CDrivesFolder::GetDefaultSearchGUID(LPGUID pGuid)
{
    return FindFileOrFolders_GetDefaultSearchGUID((IShellFolder2*) this, pGuid);
}

STDMETHODIMP CDrivesFolder::EnumSearches(LPENUMEXTRASEARCH* ppenum)
{
    ASSERT(NULL != ppenum);
    *ppenum = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CDrivesFolder::GetDefaultColumn(DWORD dwRes, ULONG* pSort, ULONG* pDisplay)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDrivesFolder::GetDefaultColumnState(UINT iColumn, DWORD* pdwState)
{
    HRESULT hres;

    *pdwState = 0;
    if (iColumn < ARRAYSIZE(c_drives_cols))
    {
        if ((iColumn != DRIVES_ICOL_COMMENT) && (iColumn != DRIVES_ICOL_HTMLINFOTIPFILE))
        {
            *pdwState |= SHCOLSTATE_ONBYDEFAULT;    // The comment column is off by default
        }
        else
        {
            *pdwState |= SHCOLSTATE_SLOW; // It takes a long time to extract the comment from drives

            if (iColumn == DRIVES_ICOL_HTMLINFOTIPFILE)
            {
                *pdwState |= SHCOLSTATE_HIDDEN;
            }
        }
        
        if (iColumn == DRIVES_ICOL_SIZE || iColumn == DRIVES_ICOL_FREE)
        {
            *pdwState |= SHCOLSTATE_TYPE_INT;
        }
        else
        {
            *pdwState |= SHCOLSTATE_TYPE_STR;
        }
        hres = S_OK;
    }
    else
    {
        hres = E_INVALIDARG;
    }
    return hres;
}

STDMETHODIMP CDrivesFolder::GetDetailsEx(LPCITEMIDLIST pidl, const SHCOLUMNID* pscid, VARIANT* pv)
{
    HRESULT hr = E_INVALIDARG;
    LPCIDDRIVE pidd = _IsValidID(pidl);
    if (pidd)
    {
        if (IsEqualSCID(*pscid, SCID_DESCRIPTIONID))
        {
            SHDESCRIPTIONID did;

            switch(pidd->bFlags)
            {
            case SHID_COMPUTER_FIXED:       did.dwDescriptionId = SHDID_COMPUTER_FIXED;       break;
            case SHID_COMPUTER_RAMDISK:     did.dwDescriptionId = SHDID_COMPUTER_RAMDISK;     break;
            case SHID_COMPUTER_CDROM:       did.dwDescriptionId = SHDID_COMPUTER_CDROM;       break;
            case SHID_COMPUTER_NETDRIVE:    did.dwDescriptionId = SHDID_COMPUTER_NETDRIVE;    break;
            case SHID_COMPUTER_DRIVE525:    did.dwDescriptionId = SHDID_COMPUTER_DRIVE525;    break;
            case SHID_COMPUTER_DRIVE35:     did.dwDescriptionId = SHDID_COMPUTER_DRIVE35;     break;
            case SHID_COMPUTER_REMOVABLE:   did.dwDescriptionId = SHDID_COMPUTER_REMOVABLE;   break;
            default:                        did.dwDescriptionId = SHDID_COMPUTER_OTHER;       break;
            }

            did.clsid = CLSID_NULL;
            hr = InitVariantFromBuffer(pv, (PVOID)&did, sizeof(did));
        }
        else
        {
            int iCol = FindSCID(c_drives_cols, ARRAYSIZE(c_drives_cols), pscid);
            if (iCol >= 0)
            {
                switch (iCol)
                {
                case DRIVES_ICOL_SIZE:
                case DRIVES_ICOL_FREE:
                    {
                        ULONGLONG ullSize, ullFree;
                        
                        hr = E_FAIL;

                        if (_GetFreeSpace(pidd, &ullSize, &ullFree))
                        {
                            pv->vt = VT_UI8;
                            pv->ullVal = iCol == DRIVES_ICOL_SIZE ? ullSize : ullFree;
                            hr = S_OK;
                        }
                    }
                    break;

                default:
                    {
                        SHELLDETAILS sd;

                        hr = GetDetailsOf(pidl, iCol, &sd);
                        if (SUCCEEDED(hr))
                        {
                            hr = InitVariantFromStrRet(&sd.str, pidl, pv);
                        }
                    }
                }
            }
        }
    }
    return hr;
}

STDMETHODIMP CDrivesFolder::GetDetailsOf(LPCITEMIDLIST pidl, UINT iColumn, LPSHELLDETAILS lpDetails)
{
    TCHAR szTemp[INFOTIPSIZE];
    
    if (iColumn >= ARRAYSIZE(c_drives_cols))
        return E_NOTIMPL;
    
    lpDetails->str.uType = STRRET_CSTR;
    lpDetails->str.cStr[0] = 0;

    szTemp[0] = 0;
    
    if (!pidl)
    {
        lpDetails->fmt = c_drives_cols[iColumn].iFmt;
        lpDetails->cxChar = c_drives_cols[iColumn].cchCol;
        return ResToStrRet(c_drives_cols[iColumn].ids, &lpDetails->str);
    }

    LPCIDDRIVE pidd = _IsValidID(pidl);
    ASSERTMSG(pidd != NULL, "someone passed us a bad pidl");
    if (!pidd)
        return E_FAIL;  // protect faulting code below
    
    switch (iColumn)
    {
    case DRIVES_ICOL_NAME:
        _GetDisplayName(pidd, szTemp, ARRAYSIZE(szTemp));
        break;
        
    case DRIVES_ICOL_TYPE:
        _GetTypeString(pidd->bFlags, szTemp, ARRAYSIZE(szTemp));
        break;
        
    case DRIVES_ICOL_COMMENT:
        GetDriveComment(DRIVEID(pidd->cName), szTemp, ARRAYSIZE(szTemp));
        break;

    case DRIVES_ICOL_HTMLINFOTIPFILE:
        GetDriveHTMLInfoTipFile(DRIVEID(pidd->cName), szTemp, ARRAYSIZE(szTemp));
        break;

    case DRIVES_ICOL_SIZE:
    case DRIVES_ICOL_FREE:
        {
            ULONGLONG ullSize, ullFree;

            if (_GetFreeSpace(pidd, &ullSize, &ullFree))
            {
                StrFormatByteSize64((iColumn == DRIVES_ICOL_SIZE) ? ullSize : ullFree, szTemp, ARRAYSIZE(szTemp));
            }
        }
        break;
    }
    return StringToStrRet(szTemp, &lpDetails->str);
}

HRESULT CDrivesFolder::MapColumnToSCID(UINT iColumn, SHCOLUMNID* pscid)
{
    return MapColumnToSCIDImpl(c_drives_cols, ARRAYSIZE(c_drives_cols), iColumn, pscid);
}

STDMETHODIMP CDrivesFolder::GetClassID(CLSID* pCLSID)
{
    *pCLSID = CLSID_MyComputer;
    return NOERROR;
}

STDMETHODIMP CDrivesFolder::Initialize(LPCITEMIDLIST pidl)
{
    // Only allow the Drives root on the desktop
    return !IsIDListInNameSpace(pidl, &CLSID_MyComputer) || !ILIsEmpty(_ILNext(pidl)) ? E_INVALIDARG : S_OK;
}

STDMETHODIMP CDrivesFolder::GetCurFolder(LPITEMIDLIST *ppidl)
{
    return GetCurFolderImpl(IDLIST_DRIVES, ppidl);
}



STDMETHODIMP CDrivesFolder::_GetIconOverlayInfo(LPCIDDRIVE pidd, int *pIndex, DWORD dwFlags)
{
    HRESULT hres = E_FAIL;
    IShellIconOverlayManager *psiom;
    if (SUCCEEDED(hres = GetIconOverlayManager(&psiom)))
    {
        WCHAR wszDrive[10];
        SHAnsiToUnicode(pidd->cName, wszDrive, ARRAYSIZE(wszDrive));
#ifdef UNICODE
        if (IsShared(wszDrive, FALSE))
#else
        if (IsShared(pidd->cName, FALSE))
#endif
        {
            hres = psiom->GetReservedOverlayInfo(wszDrive, 0, pIndex, SIOM_OVERLAYINDEX, SIOM_RESERVED_SHARED);
        }
        else
        {
            hres = psiom->GetFileOverlayInfo(wszDrive, 0, pIndex, dwFlags);
        }            
        psiom->Release();
    }
    return hres;
}    


STDMETHODIMP CDrivesFolder::GetOverlayIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hres = E_FAIL;
    LPCIDDRIVE pidd = _IsValidID(pidl);
    if (pidd)
    {
        if (!_IsReg(pidd))
        {
            hres = _GetIconOverlayInfo(pidd, pIndex, SIOM_OVERLAYINDEX);
        }            
    }
    return hres;
}



STDMETHODIMP CDrivesFolder::GetOverlayIconIndex(LPCITEMIDLIST pidl, int *pIndex)
{
    HRESULT hres = E_FAIL;
    LPCIDDRIVE pidd = _IsValidID(pidl);
    if (pidd)
    {
        if (!_IsReg(pidd))
        {
            hres = _GetIconOverlayInfo(pidd, pIndex, SIOM_ICONINDEX);
        }            
    }
    return hres;
}


// NOTE: we don't include SHID_COMPUTER_CDROM as this test really implies (this is a floppy thing)

BOOL CDrivesFolder::TypeIsRemoveable(LPCIDDRIVE pidd)
{
    switch (pidd->bFlags) {
    case SHID_COMPUTER_REMOVABLE:
    case SHID_COMPUTER_DRIVE525:
    case SHID_COMPUTER_DRIVE35:
        return TRUE;
    }
    return FALSE;
}

STDMETHODIMP CDrivesFolder::CompareItemIDs(LPCIDDRIVE pidd1, LPCIDDRIVE pidd2)
{
    // Compare the drive letter for sorting purpose.
    int iRes = StrCmpICA(pidd1->cName, pidd2->cName);   // don't need local goo

    // Then, compare the volume label (if any) for identity.
    if (iRes == 0
        && pidd1->bFlags != SHID_COMPUTER_MISC
        && pidd2->bFlags != SHID_COMPUTER_MISC)
    {
        iRes = pidd1->wChecksum - pidd2->wChecksum;
        if (iRes == 0)
            iRes = pidd1->bFlags - pidd2->bFlags;
    }

    return ResultFromShort(iRes);
}

#ifdef WINNT
#define DoesDriveWantToHideItself(iDrive) FALSE
#else
BOOL DoesDriveWantToHideItself(int iDrive)
{
    // Only supported on Win95 OPK2 or beyond
    static BOOL s_fCheckToHideDrives = (BOOL)42;   // Preload some value to say lets calculate...

    BOOL fWantToHide = FALSE;

    if (s_fCheckToHideDrives == (BOOL)42)
    {
        OSVERSIONINFO osvi;
        osvi.dwOSVersionInfoSize = sizeof(osvi);
        GetVersionEx(&osvi);

        // On Win95 OSR2 and higher (dwBuildNumber is the key!) try this IOCTL
        // to allow the drive to hide itself.

        s_fCheckToHideDrives = (LOWORD(osvi.dwBuildNumber) > 1000) ||
                               (osvi.dwMajorVersion > 4) ||
                               (osvi.dwMinorVersion > 0);
    }

    if (s_fCheckToHideDrives)
    {
        CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive);

        if (pMtPt)
        {
            fWantToHide = pMtPt->WantToHide();
            pMtPt->Release();
        }
    }

    return fWantToHide;
}
#endif

BOOL IsShareable(int iDrive)
{
    return !IsRemoteDrive(iDrive);
}

HRESULT CALLBACK CDrivesFolder::EnumCallBack(LPARAM lParam, void *pvData, UINT ecid, UINT index)
{
    HRESULT hres = NOERROR;
    EnumDrives * pedrv = (EnumDrives *)pvData;

    if (ecid == ECID_SETNEXTID)
    {
        hres = S_FALSE; // assume "no more element"

        for (int iDrive = pedrv->nLastFoundDrive + 1; iDrive < 26; iDrive++)
        {
            if (pedrv->dwRestricted & (1 << iDrive))
            {
                TraceMsg(DM_TRACE, "s.cd_ecb: Drive %d restricted.", iDrive);
            }
            else if ((pedrv->dwDrivesMask & (1 << iDrive)) || IsUnavailableNetDrive(iDrive))
            {
                //
                if (DoesDriveWantToHideItself(iDrive))
                {
                    TraceMsg(DM_TRACE, "s.cd_ecb: Drive %d Drivespace says should be hidden.", iDrive);
                }
                else if (!(SHCONTF_SHAREABLE & pedrv->grfFlags) || IsShareable(iDrive))
                {
                    DRIVE_IDLIST *piddl = (DRIVE_IDLIST *)_ILCreate(SIZEOF(DRIVE_IDLIST));
                    if (piddl)
                    {
                        _FillIDDrive(piddl, iDrive, FALSE);

                        // If the drive is bogus, we get SHID_COMPUTER_MISC back
                        if (piddl->idd.bFlags != SHID_COMPUTER_MISC)
                        {
                            CMountPoint* pMtPt = CMountPoint::GetMountPoint(iDrive, FALSE);
                            if (pMtPt)
                            {
                                pMtPt->ChangeNotifyRegisterAlias();
                                pMtPt->Release();
                            }
                            CDefEnum_SetReturn(lParam, (LPITEMIDLIST)piddl);

                            pedrv->nLastFoundDrive = iDrive;
                            return NOERROR;
                        }
                        ILFree((LPITEMIDLIST)piddl);
                    }
                    else
                    {
                        return E_OUTOFMEMORY;
                    }
                }
            }
        }
    }
    else if (ecid == ECID_RELEASE)
    {
        SetErrorMode(pedrv->dwSavedErrorMode);  // restore this
        LocalFree((HLOCAL)pedrv);
    }
    return hres;
}

HRESULT CDrivesFolder::_OnChangeNotify(LPARAM lNotification, LPCITEMIDLIST *ppidl)
{
    // Get to the last part of this id list...
    DWORD dwRestricted;
    LPCIDDRIVE pidd;

    if ((lNotification != SHCNE_DRIVEADD) || (ppidl == NULL) || (*ppidl == NULL))
        return NOERROR;

    dwRestricted = SHRestricted(REST_NODRIVES);
    if (dwRestricted == 0)
        return NOERROR;   // no drives restricted... (majority case)

    pidd = (LPCIDDRIVE)ILFindLastID(*ppidl);

    if (((1 << DRIVEID(pidd->cName)) & dwRestricted) || 
        DoesDriveWantToHideItself(DRIVEID(pidd->cName)))
    {
        TraceMsg(DM_TRACE, "Drive not added due to restrictions or Drivespace says it should be hidden");
        return S_FALSE;
    }
    return NOERROR;
}
