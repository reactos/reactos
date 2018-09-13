// NOTE! Shdoc401 issue: Start | Find needs to check what version of shell32 user is running 
//                       because of the way we pack icons in menu item info
//                       that code is under #ifdef SHDOC401_DLL ... #endif (reljai)

#include "stdafx.h"
#pragma hdrstop
#include <shlobjp.h>
#include <initguid.h>
#include "apithk.h"
#include "resource.h"
#include <runtask.h>


// CLSID copied from the Network Connections folder.
const CLSID CLSID_ConnectionFolder = {0x7007ACC7,0x3202,0x11D1,{0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E}};


#define REGSTR_EXPLORER_ADVANCED REGSTR_PATH_EXPLORER TEXT("\\Advanced")
#define REGSTR_EXPLORER_WINUPDATE REGSTR_PATH_EXPLORER TEXT("\\WindowsUpdate")

#define TF_MENUBAND         0x00002000      // Menu band messages

#define IDM_TOPLEVELSTARTMENU  0

// StartMenuInit Flags
#define STARTMENU_DISPLAYEDBEFORE       0x00000001
#define STARTMENU_CHEVRONCLICKED        0x00000002

// New item counts for UEM stuff
#define UEM_NEWITEMCOUNT 2


// Menuband per pane user data
typedef struct
{
    BITBOOL _fInitialized;
} SMUSERDATA;

// for g_hdpaDarwinAds
BOOL g_fCritSectionInit = FALSE; 
CRITICAL_SECTION g_csDarwinAds = {0};

#define ENTERCRITICAL_DARWINADS EnterCriticalSection(&g_csDarwinAds)
#define LEAVECRITICAL_DARWINADS LeaveCriticalSection(&g_csDarwinAds)

// The threading concern with this variable is create/delete/add/remove. We will only remove an item 
// and delete the hdpa on the main thread. We will however add and create on both threads.
// We need to serialize access to the dpa, so we're going to grab the shell crisec.
HDPA g_hdpaDarwinAds = NULL;
int  g_cStartMenuTask = 0;

class CDarwinAd
{
public:
    LPITEMIDLIST    _pidl;
    LPTSTR          _pszDescriptor;
    BOOL            _fIsAd;

    CDarwinAd(LPITEMIDLIST pidl, LPTSTR psz)
    {
        // I take ownership of this pidl
        _pidl = pidl;
        Str_SetPtr(&_pszDescriptor, psz);
        CheckInstalled();
    }

    void CheckInstalled()
    {
        _fIsAd = IsDarwinAd(_pszDescriptor);
    }

    BOOL IsAd()
    {
        return _fIsAd;
    }

    ~CDarwinAd()
    {
        ILFree(_pidl);
        Str_SetPtr(&_pszDescriptor, NULL);
    }
};


BOOL AreIntelliMenusEnbaled()
{
    DWORD dwRest = SHRestricted(REST_INTELLIMENUS);
    if (dwRest != RESTOPT_INTELLIMENUS_USER)
        return (dwRest == RESTOPT_INTELLIMENUS_ENABLED);

    return SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("IntelliMenus"),
                               FALSE, TRUE); // Don't ignore HKCU, Enable Menus by default
}

BOOL FeatureEnabled(LPTSTR pszFeature)
{
    return SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, pszFeature,
                        FALSE, // Don't ignore HKCU
                        FALSE); // Disable this cool feature.
}


// Since we can be presented with an Augmented shellfolder and we need a Full pidl,
// we have been given the responsibility to unwrap it for perf reasons.
LPITEMIDLIST FullPidlFromSMData(LPSMDATA psmd)
{
    LPITEMIDLIST pidlItem;
    LPITEMIDLIST pidlFolder;
    LPITEMIDLIST pidlFull = NULL;
    IAugmentedShellFolder2* pasf2;
    if (SUCCEEDED(psmd->psf->QueryInterface(IID_IAugmentedShellFolder2, (LPVOID*)&pasf2)))
    {

        if( SUCCEEDED(pasf2->UnWrapIDList(psmd->pidlItem, 1, NULL, &pidlFolder, &pidlItem, NULL)))
        {
            pidlFull = ILCombine(pidlFolder, pidlItem);
            ILFree(pidlFolder);
            ILFree(pidlItem);
        }
        pasf2->Release();
    }

    if (!pidlFolder)
    {
        pidlFull = ILCombine(psmd->pidlFolder, psmd->pidlItem);
    }

    return pidlFull;
}

BOOL ProcessDarwinAd(IShellLinkDataList* psldl, LPCITEMIDLIST pidlFull)
{
    // This function does not check for the existance of a member before adding it,
    // so it is entirely possible for there to be duplicates in the list....
    BOOL fIsLoaded = FALSE;
    BOOL fFreesldl = FALSE;
    BOOL fRetVal = FALSE;

    // If the critical section has not been initialized yet, then we don't need to process this
    // because the background thread will get it.
    if (!g_fCritSectionInit)
        return FALSE;

    if (!psldl)
    {
        // We will detect failure of this at use time.
        if (FAILED(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC, 
            IID_IShellLinkDataList, (void**)&psldl)))
        {
            return FALSE;
        }

        fFreesldl = TRUE;

        IPersistFile* ppf;
        OLECHAR sz[MAX_PATH];
        if (SHGetPathFromIDListW(pidlFull, sz))
        {
            if (SUCCEEDED(psldl->QueryInterface(IID_IPersistFile, (void**)&ppf)))
            {
                if (SUCCEEDED(ppf->Load(sz, 0)))
                {
                    fIsLoaded = TRUE;
                }
                ppf->Release();
            }
        }
    }
    else
        fIsLoaded = TRUE;

    CDarwinAd* pda = NULL;
    if (fIsLoaded)
    {
        EXP_DARWIN_LINK* pexpDarwin;

        if (SUCCEEDED(psldl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin)))
        {
#ifdef UNICODE
            pda = new CDarwinAd(ILClone(pidlFull), pexpDarwin->szwDarwinID);
#else
            pda = new CDarwinAd(ILClone(pidlFull), pexpDarwin->szDarwinID);
#endif
            LocalFree(pexpDarwin);
        }
    }

    if (pda)
    {
        // When accessing the global HDPA, get the critical section.
        ENTERCRITICAL_DARWINADS;
        // Do we have a global cache?
        if (g_hdpaDarwinAds == NULL)
        {
            // No; This is either the first time this is called, or we
            // failed the last time.
            g_hdpaDarwinAds = DPA_Create(5);
        }

        if (g_hdpaDarwinAds)
        {
            // DPA_AppendPtr returns the zero based index it inserted it at.
            if(DPA_AppendPtr(g_hdpaDarwinAds, (void*)pda) >= 0)
            {
                fRetVal = TRUE;
            }

        }
        LEAVECRITICAL_DARWINADS;

        if (!fRetVal)
        {
            // if we failed to create a dpa, delete this.
            delete pda;
        }
    }

    if (fFreesldl)
        psldl->Release();

    return fRetVal;
}

// This routine creates the IShellFolder and pidl for the Fast items
// section of the start menu.
HRESULT GetFastItemFolder(IShellFolder** ppsf, LPITEMIDLIST* ppidl)
{
    HRESULT hres;
    LPITEMIDLIST  pidlFast = NULL;

#ifndef NO_MERGEDSHELLFOLDER
    IAugmentedShellFolder2 * pasf = NULL;
    hres = CoCreateInstance(CLSID_AugmentedShellFolder2, NULL, CLSCTX_INPROC, 
                            IID_IAugmentedShellFolder2, (void **)&pasf);
#else
    IAugmentedShellFolder * pasf = NULL;
    hres = CoCreateInstance(CLSID_AugmentedShellFolder, NULL, CLSCTX_INPROC, 
                            IID_IAugmentedShellFolder, (void **)&pasf);
#endif
    if (SUCCEEDED(hres))
    {
        hres = SHGetSpecialFolderLocation(NULL, CSIDL_STARTMENU, &pidlFast);
        if (SUCCEEDED(hres))
        {
            IShellFolder* psfFast; //For Fast items on Start Menu

            hres = SHBindToObject(NULL, IID_IShellFolder, pidlFast, (void **)&psfFast);
            if (SUCCEEDED(hres))
            {
                pasf->AddNameSpace(NULL, psfFast, pidlFast, ASFF_DEFAULT | ASFF_DEFNAMESPACE_ALL);
                psfFast->Release();
            }

#ifdef WINNT
            LPITEMIDLIST pidlFastCommon;
            if (!SHRestricted(REST_NOCOMMONGROUPS) &&
                SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_STARTMENU, &pidlFastCommon)))
            {
                IShellFolder* psfFastCommon;
                if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidlFastCommon, (void **)&psfFastCommon)))
                {
                    pasf->AddNameSpace(NULL, psfFastCommon, pidlFastCommon, ASFF_DEFAULT);
                    psfFastCommon->Release();
                }

                ILFree(pidlFastCommon);
            }
#endif
        }
    }

    if (FAILED(hres))
    {
        ATOMICRELEASE(pasf);
        ILFree(pidlFast);
        pidlFast = NULL;
    }

    *ppidl = pidlFast;
    *ppsf = pasf;
    return hres;
}

HRESULT GetNetConnectPidl(LPITEMIDLIST* ppidl)
{
    // We now need to generate a pidl for this item.
    // To do this, we create a string that contains ::<CLSID>
    // then pass it through Desktop::ParseDisplayName.
    // This generates a pidl to the parent.
    DWORD dwEaten;
    IShellFolder* psfDesktop = NULL;
    HRESULT hres = E_FAIL;

    WCHAR wszItem[MAX_PATH] = L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}\\" // CLSID_MyComputer
                              L"::{21EC2020-3AEA-1069-A2DD-08002B30309D}\\" // CLSID_ControlPanel
                              L"::{7007ACC7-3202-11D1-AAD2-00805FC1270E}";  // CLSID_NetworkConnections

    SHGetDesktopFolder(&psfDesktop);
    if (psfDesktop)
    {
        hres = psfDesktop->ParseDisplayName(NULL, NULL, wszItem, 
            &dwEaten, ppidl, NULL);
        psfDesktop->Release();
    }

    return hres;
}

void SetFilesystemInfo(IShellFolder* psf, LPCITEMIDLIST pidlRoot, int csidl)
{
    ASSERT(psf);
    IPersistFolder3* ppf;

    if (SUCCEEDED(psf->QueryInterface(IID_IPersistFolder3, (void**)&ppf)))
    {
        PERSIST_FOLDER_TARGET_INFO pfti = {0};
        pfti.dwAttributes = -1;
        pfti.csidl = csidl;
        ppf->InitializeEx(NULL, pidlRoot, &pfti);
        ppf->Release();
    }
}

HRESULT GetFilesystemInfo(IShellFolder* psf, LPITEMIDLIST* ppidlRoot, int* pcsidl)
{
    ASSERT(psf);
    IPersistFolder3* ppf;
    HRESULT hres = E_FAIL;

    *pcsidl = 0;
    *ppidlRoot = 0;
    if (SUCCEEDED(psf->QueryInterface(IID_IPersistFolder3, (void**)&ppf)))
    {
        PERSIST_FOLDER_TARGET_INFO pfti = {0};

        if(SUCCEEDED(ppf->GetFolderTargetInfo(&pfti)))
        {
            *pcsidl = pfti.csidl;
            if (-1 != pfti.csidl)
                hres = S_OK;

            ILFree(pfti.pidlTargetFolder);
        }

        if (SUCCEEDED(hres))
            hres = ppf->GetCurFolder(ppidlRoot);
            
        ppf->Release();
    }
    return hres;
}

HRESULT ExecStaticStartMenuItem(int idCmd, BOOL fAllUsers, BOOL fOpen)
{
    int csidl = -1;
    HRESULT hres = E_OUTOFMEMORY;
    SHELLEXECUTEINFO shei = {0};
    switch(idCmd)
    {
    case IDM_PROGRAMS:          csidl = fAllUsers ? CSIDL_COMMON_PROGRAMS : CSIDL_PROGRAMS; break;
    case IDM_FAVORITES:         csidl = CSIDL_FAVORITES; break;
    case IDM_MYDOCUMENTS:       shei.lpIDList = MyDocsIDList(); break;
    case IDM_CONTROLS:          csidl = CSIDL_CONTROLS;  break;
    case IDM_PRINTERS:          csidl = CSIDL_PRINTERS;  break;
    case IDM_NETCONNECT:        GetNetConnectPidl((LPITEMIDLIST*)&shei.lpIDList); break;
    default:
        return E_FAIL;
    }

    if (csidl != -1)
    {
        SHGetSpecialFolderLocation(NULL, csidl, (LPITEMIDLIST*)&shei.lpIDList);
    }

    if (shei.lpIDList)
    {
        shei.cbSize     = sizeof(shei);
        shei.fMask      = SEE_MASK_IDLIST;
        shei.nShow      = SW_SHOWNORMAL;
        shei.lpVerb     = fOpen? TEXT("open") : TEXT("explore");
        hres = ShellExecuteEx(&shei)? S_OK: E_FAIL;
        ILFree((LPITEMIDLIST)shei.lpIDList);
    }

    return hres;
}


// IShellMenuCallback implementation
class CStartMenuCallback : public IShellMenuCallback,
                           public CObjectWithSite
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface (REFIID riid, void ** ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG)  Release();

    // *** IShellMenuCallback methods ***
    STDMETHODIMP CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    // *** IObjectWithSite methods ***
    STDMETHODIMP SetSite(IUnknown* punk);
    STDMETHODIMP GetSite(REFIID riid, void** ppvOut);

    CStartMenuCallback();
private:
    virtual ~CStartMenuCallback();
    int _cRef;

    DEBUG_CODE( DWORD _dwThreadID; )   // Cache the thread of the object

    IContextMenu*   _pcmFind;
    TCHAR           _szPrograms[MAX_PATH];
    TCHAR           _szProgramsPath[MAX_PATH];
    TCHAR           _szCommonProgramsPath[MAX_PATH];
    UINT            _cchCommonProgramsPath;
    UINT            _cchProgramsPath;
    LPTSTR          _pszWindowsUpdate;
    LPTSTR          _pszAdminTools;
    ITrayPriv*      _ptp;
    IUnknown*       _punkSite;
    IOleCommandTarget* _poct;
    BITBOOL         _fExpandoMenus: 1;
    BITBOOL         _fAddOpenFolder: 1;
    BITBOOL         _fCascadeMyDocuments: 1;
    BITBOOL         _fCascadePrinters: 1;
    BITBOOL         _fCascadeControlPanel: 1;
    BITBOOL         _fScrollPrograms: 1;
    BITBOOL         _fFindMenuInvalid: 1;
    BITBOOL         _fShowAdminTools: 1;
    BITBOOL         _fCascadeNetConnections: 1;
    BITBOOL         _fInitPrograms: 1;
    BITBOOL         _fShowInfoTip: 1;
    BITBOOL         _fBkThreadDoneOnLastShow: 1;
    

    TCHAR           _szFindMeumonic[2];

    HWND            _hwnd;

    HANDLE          _hmruRecent;
    DWORD           _cRecentDocs;

    DWORD           _dwFlags;
    DWORD           _dwChevronCount;
    
    HRESULT _ExecHmenuItem(LPSMDATA psmdata);
    HRESULT _Init(SMDATA* psmdata);
    HRESULT _Create(SMDATA* psmdata, void** pvUserData);
    HRESULT _Destroy(SMDATA* psmdata);
    HRESULT _GetHmenuInfo(SMDATA* psmd, SMINFO*sminfo);
    HRESULT _GetSFInfo(SMDATA* psmd, SMINFO* psminfo);
    HRESULT _GetObject(LPSMDATA psmd, REFIID riid, void** ppvObj);
    HRESULT _CheckRestricted(DWORD dwRestrict, BOOL* fRestricted);
    HRESULT _FilterPidl(UINT uParent, IShellFolder* psf, LPCITEMIDLIST pidl);
    HRESULT _FilterFavorites(IShellFolder* psf, LPCITEMIDLIST pidl);
    HRESULT _FilterRecentPidl(IShellFolder* psf, LPCITEMIDLIST pidl);
    HRESULT _Demote(LPSMDATA psmd);
    HRESULT _Promote(LPSMDATA psmd);
    HRESULT _HandleNew(LPSMDATA psmd);
    HRESULT _GetTip(LPTSTR pstrTitle, LPTSTR pstrTip);
    DWORD _GetDemote(SMDATA* psmd);
    HRESULT _ProcessChangeNotify(SMDATA* psmd, LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    HRESULT _HandleAccelerator(TCHAR ch, SMDATA* psmdata);
    HRESULT _GetDefaultIcon(TCHAR* psz, int* piIndex);
    void _GetStaticStartMenu(HMENU* phmenu, HWND* phwnd);
    HRESULT _GetStaticInfoTip(SMDATA* psmd, LPTSTR pszTip, int cch);
    void _ReValidateDarwinCache();

    static STDMETHODIMP_(int) s_DarwinAdsDestroyCallback(LPVOID pData1, LPVOID pData2);

    // helper functions
    DWORD GetInitFlags();
    void  SetInitFlags(DWORD dwFlags);
    HRESULT _InitializeFindMenu(IShellMenu* psm);
    int _GetDarwinIndex(LPCITEMIDLIST pidlFull, CDarwinAd** ppda);
    BOOL _IsDarwinAdvertisement(LPCITEMIDLIST pidlFull);
    HRESULT _ExecItem(LPSMDATA, UINT);
    HRESULT VerifyCSIDL(int idCmd, int csidl, IShellMenu* psm);
    HRESULT VerifyMergedGuy(BOOL fPrograms, IShellMenu* psm);
    void _InitializePrograms();
    void _UpdateMyDocsMenuItemName(IShellMenu* psm);

public: // Make these public to this file. This is for the CreateInstance
    // Sub Menu creation
    HRESULT InitializeFastItemsShellMenu(IShellMenu* psm);
    HRESULT InitializeProgramsShellMenu(IShellMenu* psm);
    HRESULT InitializeCSIDLShellMenu(int uId, int csidl, LPTSTR pszRoot, LPTSTR pszValue,
                                 DWORD dwPassInitFlags, DWORD dwSetFlags, BOOL fAddOpen, 
                                 IShellMenu* psm);
    HRESULT InitializeDocumentsShellMenu(IShellMenu* psm);
    HRESULT InitializeSubShellMenu(int idCmd, IShellMenu* psm);
    HRESULT InitializeNetworkConnections(IShellMenu* psm);
};


class CStartContextMenu : IContextMenu
{
public:
    // IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG,AddRef)(void);
    STDMETHOD_(ULONG,Release)(void);
    
    // IContextMenu
    STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
    STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
    STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax);

    CStartContextMenu(int idCmd) : _idCmd(idCmd), _cRef(1) {};
private:
    int _cRef;
    virtual ~CStartContextMenu() {};

    int _idCmd;
};

CStartMenuCallback::CStartMenuCallback() : _cRef(1), _cRecentDocs(-1)
{
    DEBUG_CODE( _dwThreadID = GetCurrentThreadId() );

    TCHAR szWindowsUpdate[MAX_PATH];
    DWORD cbSize = ARRAYSIZE(szWindowsUpdate);

    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_EXPLORER_WINUPDATE, TEXT("ShortcutName"), 
        NULL, szWindowsUpdate, &cbSize))
    {
        Str_SetPtr(&_pszWindowsUpdate, szWindowsUpdate);
    }

    TCHAR szPath[MAX_PATH];
    if (SHGetSpecialFolderPath(NULL, szPath, CSIDL_COMMON_ADMINTOOLS, TRUE))
    {
        Str_SetPtr(&_pszAdminTools, PathFindFileName(szPath));
    }

    LoadString(g_hinst, IDS_FIND_MEUMONIC, _szFindMeumonic, ARRAYSIZE(_szFindMeumonic));
}

int CStartMenuCallback::s_DarwinAdsDestroyCallback(LPVOID pData1, LPVOID pData2)
{
    CDarwinAd* pda = (CDarwinAd*)pData1;
    if (pda)
        delete pda;
    return 0;
}

CStartMenuCallback::~CStartMenuCallback()
{
    ASSERT( _dwThreadID == GetCurrentThreadId() );
    if (_hmruRecent)
        FreeMRUList(_hmruRecent);
        
    ATOMICRELEASE(_pcmFind);
    ATOMICRELEASE(_ptp);

    Str_SetPtr(&_pszWindowsUpdate, NULL);
    Str_SetPtr(&_pszAdminTools, NULL);

    if (g_fCritSectionInit)
    {
        ENTERCRITICAL_DARWINADS;
        HDPA hdpa = g_hdpaDarwinAds;
        g_hdpaDarwinAds = NULL;
        LEAVECRITICAL_DARWINADS;
        if (hdpa)
            DPA_DestroyCallback(hdpa, s_DarwinAdsDestroyCallback, NULL);
    }

}

// *** IUnknown methods ***
STDMETHODIMP CStartMenuCallback::QueryInterface(REFIID riid, void ** ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CStartMenuCallback, IShellMenuCallback),
        QITABENT(CStartMenuCallback, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


STDMETHODIMP_(ULONG) CStartMenuCallback::AddRef()
{
    return ++_cRef;
}


STDMETHODIMP_(ULONG) CStartMenuCallback::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if( _cRef > 0)
        return _cRef;

    delete this;
    return 0;
}


STDMETHODIMP CStartMenuCallback::SetSite(IUnknown* punk)
{
    ATOMICRELEASE(_punkSite);
    _punkSite = punk;
    if (punk)
    {
        _punkSite->AddRef();
    }

    return NOERROR;
}


STDMETHODIMP CStartMenuCallback::GetSite(REFIID riid, void**ppvOut)
{
    if (_ptp)
        return _ptp->QueryInterface(riid, ppvOut);
    else
        return E_NOINTERFACE;
}

#ifdef DEBUG
void DBUEMQueryEvent(const IID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam)
{
#if 1
    return;
#else
    UEMINFO uei;

    uei.cbSize = SIZEOF(uei);
    uei.dwMask = ~0;    // UEIM_HIT etc.
    UEMQueryEvent(pguidGrp, eCmd, wParam, lParam, &uei);

    TCHAR szBuf[20];
    wsprintf(szBuf, TEXT("hit=%d"), uei.cHit);
    MessageBox(NULL, szBuf, TEXT("UEM"), MB_OKCANCEL);

    return;
#endif
}
#endif

DWORD CStartMenuCallback::GetInitFlags()
{
    DWORD dwType;
    DWORD cbSize = sizeof(DWORD);
    DWORD dwFlags = 0;
    SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuInit"), 
            &dwType, (BYTE*)&dwFlags, &cbSize);
    return dwFlags;
}

void CStartMenuCallback::SetInitFlags(DWORD dwFlags)
{
    SHSetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuInit"), REG_DWORD, &dwFlags, sizeof(DWORD));
}

DWORD GetClickCount()
{

    //This function retrieves the number of times the user has clicked on the chevron item.

    DWORD dwType;
    DWORD cbSize = sizeof(DWORD);
    DWORD dwCount = 1;      // Default to three clicks before we give up.
                            // PMs what it to 1 now. Leaving back end in case they change their mind.
    SHGetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuChevron"), 
            &dwType, (BYTE*)&dwCount, &cbSize);

    return dwCount;

}

void SetClickCount(DWORD dwClickCount)
{
    SHSetValue(HKEY_CURRENT_USER, REGSTR_EXPLORER_ADVANCED, TEXT("StartMenuChevron"), REG_DWORD, &dwClickCount, sizeof(DWORD));
}


STDMETHODIMP CStartMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = S_FALSE;
    switch (uMsg)
    {

    case SMC_CREATE:
        hres = _Create(psmd, (void**)lParam);
        break;

    case SMC_DESTROY:
        hres = _Destroy(psmd);
        break;

    case SMC_INITMENU:
        _Init(psmd);
        break;

    case SMC_SFEXEC:
        hres = _ExecItem(psmd, uMsg);
        break;

    case SMC_EXEC:
        hres = _ExecHmenuItem(psmd);
        break;

    case SMC_GETOBJECT:
        hres = _GetObject(psmd, (GUID)*((GUID*)wParam), (void**)lParam);
        break;

    case SMC_GETINFO:
        hres = _GetHmenuInfo(psmd, (SMINFO*)lParam);
        break;

    case SMC_GETSFINFOTIP:
        if (!_fShowInfoTip)
            hres = E_FAIL;  // E_FAIL means don't show. S_FALSE means show default
        break;

    case SMC_GETINFOTIP:
        hres = _GetStaticInfoTip(psmd, (LPTSTR)wParam, (int)lParam);
        break;

    case SMC_GETSFINFO:
        hres = _GetSFInfo(psmd, (SMINFO*)lParam);
        break;

    case SMC_BEGINENUM:
        if (psmd->uIdParent == IDM_TOPLEVELSTARTMENU &&
            SHRestricted(REST_NOSTARTMENUSUBFOLDERS))
        {
            ASSERT(wParam != NULL);
            *(DWORD*)wParam = SHCONTF_NONFOLDERS;
            hres = S_OK;
        }
        else if (psmd->uIdParent == IDM_RECENT)
        {
            ASSERT(_cRecentDocs == -1);
            ASSERT(!_hmruRecent);
            _hmruRecent = CreateSharedRecentMRUList(NULL, NULL, SRMLF_COMPPIDL);

            _cRecentDocs = 0;
            hres = S_OK;
        }
        break;

    case SMC_ENDENUM:
         if (psmd->uIdParent == IDM_RECENT)
         {
             ASSERT(_cRecentDocs != -1);
             if (_hmruRecent)
             {
                 FreeMRUList(_hmruRecent);
                 _hmruRecent = NULL;
             }
             
             _cRecentDocs = -1;
             hres = S_OK;
         }
         break;
    
    case SMC_FILTERPIDL:
        ASSERT(psmd->dwMask & SMDM_SHELLFOLDER);

        if (psmd->uIdAncestor == IDM_FAVORITES)
        {
            hres = _FilterFavorites(psmd->psf, psmd->pidlItem);
        }
        else if (psmd->uIdParent == IDM_RECENT)
        {
            //  we need to filter out all but the first MAXRECENTITEMS
            //  and no folders allowed!
            hres = _FilterRecentPidl(psmd->psf, psmd->pidlItem);
        }
        else
        {
            hres = _FilterPidl(psmd->uIdParent, psmd->psf, psmd->pidlItem);
        }
        break;

    case SMC_INSERTINDEX:
        ASSERT(lParam && IS_VALID_WRITE_PTR(lParam, int));
        *((int*)lParam) = 0;
        hres = S_OK;
        break;

    case SMC_SHCHANGENOTIFY:
        {
            PSMCSHCHANGENOTIFYSTRUCT pshf = (PSMCSHCHANGENOTIFYSTRUCT)lParam;
            hres = _ProcessChangeNotify(psmd, pshf->lEvent, pshf->pidl1, pshf->pidl2);
        }
        break;

    case SMC_REFRESH:
        {
            if (psmd->uIdParent == IDM_TOPLEVELSTARTMENU)
            {

                // We need to refresh Shell32's cache.
                TCHAR szSetting[MAX_PATH];
                LPTSTR psz = NULL;
                // NOTE: All parameters passed to this interface are UNICODE. We need to convert
                // to shell32's character format.
                if (lParam)
                {
                    SHUnicodeToTChar((LPWSTR)lParam, szSetting, ARRAYSIZE(szSetting));
                    psz = szSetting;
                }

                SHSettingsChanged(wParam, (LPARAM)psz);

                // Refresh is only called on the top level.
                HMENU hmenu;
                IShellMenu* psm;
                _GetStaticStartMenu(&hmenu, &_hwnd);
                if (psmd->punk && SUCCEEDED(psmd->punk->QueryInterface(IID_IShellMenu, (LPVOID*)&psm)))
                {
                    psm->SetMenu(hmenu, _hwnd, SMSET_BOTTOM | SMSET_MERGE);
                    psm->Release();
                }

                _fExpandoMenus = AreIntelliMenusEnbaled();
                _fCascadeMyDocuments = FeatureEnabled(TEXT("CascadeMyDocuments"));
                _fCascadePrinters = FeatureEnabled(TEXT("CascadePrinters"));
                _fCascadeControlPanel = FeatureEnabled(TEXT("CascadeControlPanel"));
                _fScrollPrograms = FeatureEnabled(TEXT("StartMenuScrollPrograms"));
                _fShowAdminTools = FeatureEnabled(TEXT("StartMenuAdminTools"));
                _fCascadeNetConnections = FeatureEnabled(TEXT("CascadeNetworkConnections"));
                _fAddOpenFolder = FeatureEnabled(TEXT("StartMenuOpen"));
                _fShowInfoTip = FeatureEnabled(TEXT("ShowInfoTip"));
                _fFindMenuInvalid = TRUE;
                _dwFlags = GetInitFlags();
                hres = S_OK;
            }
        }
        break;

    case SMC_DEMOTE:
        hres = _Demote(psmd);
        break;

    case SMC_PROMOTE:
        hres = _Promote(psmd);
        break;

    case SMC_NEWITEM:
        hres = _HandleNew(psmd);
        break;

    case SMC_MAPACCELERATOR:
        hres = _HandleAccelerator((TCHAR)wParam, (SMDATA*)lParam);
        break;

    case SMC_DEFAULTICON:
        ASSERT(psmd->uIdAncestor == IDM_FAVORITES); // This is only valid for the Favorites menu
        hres = _GetDefaultIcon((LPTSTR)wParam, (int*)lParam);
        break;
    case SMC_GETMINPROMOTED:
        // Only do this for the programs menu
        if (psmd->uIdParent == IDM_PROGRAMS)
            *((int*)lParam) = 4;        // 4 was choosen by RichSt 9.15.98
        break;


    case SMC_CHEVRONEXPAND:

        // Has the user already seen the chevron tip enough times? (We set the bit when the count goes to zero.
        if (!(_dwFlags & STARTMENU_CHEVRONCLICKED))
        {
            // No; Then get the current count from the registry. We set a default of 3, but an admin can set this
            // to -1, that would make it so that they user sees it all the time.
            DWORD dwClickCount = GetClickCount();
            if (dwClickCount > 0)
            {
                // Since they clicked, take one off.
                dwClickCount--;

                // Set it back in.
                SetClickCount(dwClickCount);
            }

            if (dwClickCount == 0)
            {
                // Ah, the user has seen the chevron tip enought times... Stop being annoying.
                _dwFlags |= STARTMENU_CHEVRONCLICKED;
                SetInitFlags(_dwFlags);
            }
        }
        hres = S_OK;
        break;

    case SMC_DISPLAYCHEVRONTIP:

        // We only want to see the tip on the top level programs case, no where else. We also don't
        // want to see it if they've had enough.
        if (psmd->uIdParent == IDM_PROGRAMS && 
            !(_dwFlags & STARTMENU_CHEVRONCLICKED) &&
            !SHRestricted(REST_NOSMBALLOONTIP))
        {
            hres = S_OK;
        }
        break;

    case SMC_CHEVRONGETTIP:
        if (!SHRestricted(REST_NOSMBALLOONTIP))
            hres = _GetTip((LPTSTR)wParam, (LPTSTR)lParam);
        break;
    }

    return hres;
}

// For the Favorites menu, since their icon handler is SO slow, we're going to fake the icon
// and have it get the real ones on the background thread...
HRESULT CStartMenuCallback::_GetDefaultIcon(TCHAR* psz, int* piIndex)
{
    HRESULT hres;
    DWORD cbSize = sizeof(TCHAR) * MAX_PATH;

    hres = SHGetValue(HKEY_CLASSES_ROOT, TEXT("InternetShortcut\\DefaultIcon"), NULL, NULL, psz, &cbSize);
    if (ERROR_SUCCESS == hres)
    {
        *piIndex = PathParseIconLocation(psz);
    }
    
    return hres;
}

HRESULT CStartMenuCallback::_ExecItem(LPSMDATA psmd, UINT uMsg)
{
    ASSERT( _dwThreadID == GetCurrentThreadId() );
    return _ptp->ExecItem(psmd->psf, psmd->pidlItem);
}

HRESULT CStartMenuCallback::_Demote(LPSMDATA psmd)
{
    //We want to for the UEM to demote pidlFolder, 
    // then tell the Parent menuband (If there is one)
    // to invalidate this pidl.
    HRESULT hres = S_FALSE;

    if (_fExpandoMenus && 
        (psmd->uIdAncestor == IDM_PROGRAMS ||
         psmd->uIdAncestor == IDM_FAVORITES))
    {
        UEMINFO uei;
        uei.cbSize = SIZEOF(uei);
        uei.dwMask = UEIM_HIT;
        uei.cHit = 0;
        hres = UEMSetEvent(psmd->uIdAncestor == IDM_PROGRAMS? &UEMIID_SHELL : &UEMIID_BROWSER, 
            UEME_RUNPIDL, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem, &uei);
    }
    return hres;
}

HRESULT CStartMenuCallback::_Promote(LPSMDATA psmd)
{
    if (_fExpandoMenus && 
        (psmd->uIdAncestor == IDM_PROGRAMS ||
         psmd->uIdAncestor == IDM_FAVORITES))
    {
        UEMFireEvent(psmd->uIdAncestor == IDM_PROGRAMS? &UEMIID_SHELL : &UEMIID_BROWSER, 
            UEME_RUNPIDL, UEMF_XEVENT, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem);
    }
    return S_OK;
}

HRESULT CStartMenuCallback::_HandleNew(LPSMDATA psmd)
{
    HRESULT hres = S_FALSE;
    if (_fExpandoMenus && 
        (psmd->uIdAncestor == IDM_PROGRAMS ||
         psmd->uIdAncestor == IDM_FAVORITES))
    {
        UEMINFO uei;
        uei.cbSize = SIZEOF(uei);
        uei.dwMask = UEIM_HIT;
        uei.cHit = UEM_NEWITEMCOUNT;
        hres = UEMSetEvent(psmd->uIdAncestor == IDM_PROGRAMS? &UEMIID_SHELL : &UEMIID_BROWSER, 
            UEME_RUNPIDL, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem, &uei);
    }

    if (psmd->uIdAncestor == IDM_PROGRAMS)
    {
        LPITEMIDLIST pidlFull = FullPidlFromSMData(psmd);
        if (pidlFull)
        {
            ProcessDarwinAd(NULL, pidlFull);
            ILFree(pidlFull);
        }
    }

    return hres;
}


void ShowPidl(HWND hwnd, LPITEMIDLIST pidl, UINT uFlags)
{
    SHELLEXECUTEINFO shei = { 0 };

    shei.cbSize     = sizeof(shei);
    shei.fMask      = SEE_MASK_IDLIST;
    shei.nShow      = SW_SHOWNORMAL;
    shei.lpVerb     = TEXT("open");
    shei.lpIDList   = pidl;
    ShellExecuteEx(&shei);
}

void ShowFolder(HWND hwnd, UINT csidl, UINT uFlags)
{
    LPITEMIDLIST pidl;
    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, (LPITEMIDLIST*)&pidl)))
    {
        ShowPidl(hwnd, pidl, uFlags);
        ILFree(pidl);
    }
}

void _ExecRegValue(LPCTSTR pszValue)
{
    TCHAR szPath[MAX_PATH];
    DWORD cbSize = ARRAYSIZE(szPath);

    if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, REGSTR_EXPLORER_ADVANCED, pszValue, 
        NULL, szPath, &cbSize))
    {
        SHELLEXECUTEINFO shei= { 0 };
        shei.cbSize = sizeof(shei);
        shei.nShow  = SW_SHOWNORMAL;
        shei.lpParameters = PathGetArgs(szPath);
        PathRemoveArgs(szPath);
        shei.lpFile = szPath;
        ShellExecuteEx(&shei);
    }
}

HRESULT CStartMenuCallback::_ExecHmenuItem(LPSMDATA psmd)
{

    HRESULT hres = S_FALSE;
    if (IsInRange(psmd->uId, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST) && _pcmFind)
    {
        CMINVOKECOMMANDINFOEX ici = { 0 };
        ici.cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);
        ici.lpVerb = (LPSTR)MAKEINTRESOURCE(psmd->uId - TRAY_IDM_FINDFIRST);
        ici.nShow = SW_NORMAL;
        
        // record if shift or control was being held down
        SetICIKeyModifiers(&ici.fMask);

        _pcmFind->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
        hres = S_OK;
    }
    else
    {
        switch (psmd->uId)
        {
        case IDM_OPEN_FOLDER:
            switch(psmd->uIdParent)
            {
            case IDM_CONTROLS:
                ShowFolder(NULL, CSIDL_CONTROLS, 0);
                hres = NOERROR;
                break;

            case IDM_PRINTERS:
                ShowFolder(NULL, CSIDL_PRINTERS, 0);
                hres = NOERROR;
                break;

            case IDM_NETCONNECT:
                goto ShowNetConnect;

            case IDM_MYDOCUMENTS:
                goto ShowMyDocs;
            }
            break;

        case IDM_NETCONNECT:
ShowNetConnect:
            {
                LPITEMIDLIST pidl;
                if (SUCCEEDED(GetNetConnectPidl(&pidl)))
                {
                    ASSERT(NULL != pidl);
                    ShowPidl(NULL, pidl, 0);
                    hres = NOERROR;
                    ILFree(pidl);
                }
            }
            break;

        case IDM_MYDOCUMENTS:
ShowMyDocs:
            {
                LPITEMIDLIST pidl = MyDocsIDList();
                if (NULL != pidl)
                {
                    ShowPidl(NULL, pidl, 0);
                    hres = NOERROR;
                    ILFree(pidl);
                }
            }
            break;

        case IDM_CSC:
            _ExecRegValue(TEXT("StartMenuSyncAll"));
            break;

        default:
            hres = ExecStaticStartMenuItem(psmd->uId, FALSE, TRUE);
            break;


        }
    }

    return hres;
}

void CStartMenuCallback::_GetStaticStartMenu(HMENU* phmenu, HWND* phwnd)
{
    IMenuPopup* pmp;
    // The first one should be the bar that the start menu is sitting in.
    if (_punkSite && 
        SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SMenuPopup, IID_IMenuPopup, (void**)&pmp)))
    {
        IObjectWithSite* pows;
        if (SUCCEEDED(pmp->QueryInterface(IID_IObjectWithSite, (void**)&pows)))
        {
            // It's site should be CStartMenuHost;
            if (SUCCEEDED(pows->GetSite(IID_ITrayPriv, (void**)&_ptp)))
            {
                IOleWindow* pow;
                _ptp->GetStaticStartMenu(phmenu);
                if (SUCCEEDED(_ptp->QueryInterface(IID_IOleWindow, (void**)&pow)))
                {
                    pow->GetWindow(phwnd);
                    pow->Release();
                }

                if (!_poct)
                    _ptp->QueryInterface(IID_IOleCommandTarget, (void**)&_poct);
            }
            else
                TraceMsg(TF_MENUBAND, "CStartMenuCallback::_SetSite : Failed to aquire CStartMenuHost");
            pows->Release();
        }
        pmp->Release();
    }
}

HRESULT CStartMenuCallback::_Create(SMDATA* psmdata, void** ppvUserData)
{
    *ppvUserData = new SMUSERDATA;

    return S_OK;
}

HRESULT CStartMenuCallback::_Destroy(SMDATA* psmdata)
{
    if (psmdata->pvUserData)
    {
        delete (SMUSERDATA*)psmdata->pvUserData;
        psmdata->pvUserData = NULL;
    }

    return S_OK;
}

void CStartMenuCallback::_InitializePrograms()
{
    if (!_fInitPrograms)
    {
        // We're either initing these, or reseting them.
        SHGetSpecialFolderPath(NULL, _szProgramsPath, CSIDL_PROGRAMS, FALSE);
        lstrcpy(_szPrograms, PathFindFileName(_szProgramsPath));
        _cchProgramsPath = lstrlen(_szProgramsPath);

        SHGetSpecialFolderPath(NULL, _szCommonProgramsPath, CSIDL_COMMON_PROGRAMS, FALSE);
        _cchCommonProgramsPath = lstrlen(_szCommonProgramsPath);
        _fInitPrograms = TRUE;
    }
}



// Given a CSIDL and a Shell menu, this will verify if the IShellMenu
// is pointing at the same place as the CSIDL is. If not, then it will
// update the shell menu to the new location.
HRESULT CStartMenuCallback::VerifyCSIDL(int idCmd, int csidl, IShellMenu* psm)
{
    DWORD dwFlags;
    LPITEMIDLIST pidl;
    IShellFolder* psf;
    HRESULT hres = S_OK;
    if (SUCCEEDED(psm->GetShellFolder(&dwFlags, &pidl, IID_IShellFolder, (void**)&psf)))
    {
        psf->Release();

        LPITEMIDLIST pidlCSIDL;
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &pidlCSIDL)))
        {
            // If the pidl of the IShellMenu is not equal to the
            // SpecialFolder Location, then we need to update it so they are...
            if (!ILIsEqual(pidlCSIDL, pidl))
            {
                hres = InitializeSubShellMenu(idCmd, psm);
            }
            ILFree(pidlCSIDL);
        }
        ILFree(pidl);
    }

    return hres;
}

// This code special cases the Programs and Fast items shell menus. It
// understands Merging and will check both shell folders in a merged case
// to verify that the shell folder is still pointing at that location
HRESULT CStartMenuCallback::VerifyMergedGuy(BOOL fPrograms, IShellMenu* psm)
{
    DWORD dwFlags;
    LPITEMIDLIST pidl;
    HRESULT hres = S_OK;
    IAugmentedShellFolder2* pasf;
    if (SUCCEEDED(psm->GetShellFolder(&dwFlags, &pidl, IID_IAugmentedShellFolder2, (void**)&pasf)))
    {
        IShellFolder* psf;
        // There are 2 things in the merged namespace: CSIDL_PROGRAMS and CSIDL_COMMON_PROGRAMS
        for (int i = 0; i < 2; i++)
        {
            if (SUCCEEDED(pasf->QueryNameSpace(i, 0, &psf)))
            {
                int csidl;
                LPITEMIDLIST pidlFolder;

                if (SUCCEEDED(GetFilesystemInfo(psf, &pidlFolder, &csidl)))
                {
                    LPITEMIDLIST pidlCSIDL;
                    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, &pidlCSIDL)))
                    {
                        // If the pidl of the IShellMenu is not equal to the
                        // SpecialFolder Location, then we need to update it so they are...
                        if (!ILIsEqual(pidlCSIDL, pidlFolder))
                        {

                            // Since one of these things has changed,
                            // we need to update the string cache
                            // so that we do proper filtering of 
                            // the programs item.
                            _fInitPrograms = FALSE;
                            if (fPrograms)
                                hres = InitializeProgramsShellMenu(psm);
                            else
                                hres = InitializeFastItemsShellMenu(psm);

                            i = 100;   // break out of the loop.
                        }
                        ILFree(pidlCSIDL);
                    }
                    ILFree(pidlFolder);
                }
                psf->Release();
            }
        }

        ILFree(pidl);
        pasf->Release();
    }


    return hres;
}

void CStartMenuCallback::_UpdateMyDocsMenuItemName(IShellMenu* psm)
{
   if (SHRestricted(REST_NOSMMYDOCS))
       return;

   HMENU hMenu;
    ASSERT(NULL != psm);
    if (SUCCEEDED(psm->GetMenu(&hMenu, NULL, NULL)))
    {
        MENUITEMINFO mii;
        TCHAR szMenuName[256];
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_TYPE;
        mii.dwTypeData = szMenuName;
        mii.cch = ARRAYSIZE(szMenuName);
        if (::GetMenuItemInfo(hMenu, IDM_MYDOCUMENTS, FALSE, &mii))
        {
            TCHAR szMyDocsName[256];
            if (SUCCEEDED(GetMyDocumentsDisplayName(szMyDocsName, ARRAYSIZE(szMyDocsName))))
            {
                if (0 != StrCmp(szMenuName, szMyDocsName))
                {
                    // The mydocs name has changed, update the menu item:
                    mii.dwTypeData = szMyDocsName;
                    if (::SetMenuItemInfo(hMenu, IDM_MYDOCUMENTS, FALSE, &mii))
                    {
                        SMDATA smd;
                        smd.dwMask = SMDM_HMENU;
                        smd.uId = IDM_MYDOCUMENTS;
                        psm->InvalidateItem(&smd, SMINV_ID | SMINV_REFRESH);
                    }
                }
            }
        }
    }
}

HRESULT CStartMenuCallback::_Init(SMDATA* psmdata)
{
    HRESULT hres = S_FALSE;
    IShellMenu* psm;
    if (psmdata->punk && SUCCEEDED(hres = psmdata->punk->QueryInterface(IID_IShellMenu, (void**)&psm)))
    {
        switch(psmdata->uIdParent)
        {
        case IDM_TOPLEVELSTARTMENU:
            if (psmdata->pvUserData && !((SMUSERDATA*)psmdata->pvUserData)->_fInitialized)
            {
                TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : Initializing Toplevel Start Menu");
                ((SMUSERDATA*)psmdata->pvUserData)->_fInitialized = TRUE;

                HMENU hmenu;

                TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : First Time, and correct parameters");

                _GetStaticStartMenu(&hmenu, &_hwnd);

                if (hmenu)
                {
                    psm->SetMenu(hmenu, _hwnd, SMSET_BOTTOM);
                    TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : SetMenu(HMENU 0x%x, HWND 0x%x", hmenu, _hwnd);
                }

                _fExpandoMenus = AreIntelliMenusEnbaled();
                _fCascadeMyDocuments = FeatureEnabled(TEXT("CascadeMyDocuments"));
                _fCascadePrinters = FeatureEnabled(TEXT("CascadePrinters"));
                _fCascadeControlPanel = FeatureEnabled(TEXT("CascadeControlPanel"));
                _fScrollPrograms = FeatureEnabled(TEXT("StartMenuScrollPrograms"));
                _fShowAdminTools = FeatureEnabled(TEXT("StartMenuAdminTools"));
                _fCascadeNetConnections = FeatureEnabled(TEXT("CascadeNetworkConnections"));
                _fAddOpenFolder = FeatureEnabled(TEXT("StartMenuOpen"));
                _fShowInfoTip = FeatureEnabled(TEXT("ShowInfoTip"));
                _dwFlags = GetInitFlags();
            }
            else if (g_cStartMenuTask == 0 && !_fBkThreadDoneOnLastShow)
            {
                _fBkThreadDoneOnLastShow = TRUE;
                psm->InvalidateItem(NULL, SMINV_REFRESH);
            }

            // Verify that the Fast items is still pointing to the right location
            hres = VerifyMergedGuy(FALSE, psm);
            break;
        case IDM_MENU_FIND:
            if (_fFindMenuInvalid)
            {
                hres = _InitializeFindMenu(psm);
                _fFindMenuInvalid = FALSE;
            }
            break;

        case IDM_PROGRAMS:
            // Verify the programs menu is still pointing to the right location
            hres = VerifyMergedGuy(TRUE, psm);
            break;

        case IDM_FAVORITES:
            hres = VerifyCSIDL(IDM_FAVORITES, CSIDL_FAVORITES, psm);
            break;

        case IDM_MYDOCUMENTS:
            hres = VerifyCSIDL(IDM_MYDOCUMENTS, CSIDL_PERSONAL, psm);
            break;
        case IDM_RECENT:
            _UpdateMyDocsMenuItemName(psm);
            hres = VerifyCSIDL(IDM_RECENT, CSIDL_RECENT, psm);
            break;
        case IDM_CONTROLS:
            hres = VerifyCSIDL(IDM_CONTROLS, CSIDL_CONTROLS, psm);
            break;
        case IDM_PRINTERS:
            hres = VerifyCSIDL(IDM_PRINTERS, CSIDL_PRINTERS, psm);
            break;
        }

        psm->Release();
    }

    return hres;
}


HRESULT CStartMenuCallback::_GetStaticInfoTip(SMDATA* psmd, LPTSTR pszTip, int cch)
{
    if (!_fShowInfoTip)
        return E_FAIL;

    HRESULT hres = E_FAIL;

    const static struct 
    {
        UINT idCmd;
        UINT idInfoTip;
    } s_mpcmdTip[] = 
    {
#if 0   // No tips for the Toplevel. Keep this here because I bet that someone will want them...
       { IDM_PROGRAMS,       IDS_PROGRAMS_TIP },
       { IDM_FAVORITES,      IDS_FAVORITES_TIP },
       { IDM_RECENT,         IDS_RECENT_TIP },
       { IDM_SETTINGS,       IDS_SETTINGS_TIP },
       { IDM_MENU_FIND,      IDS_FIND_TIP },
       { IDM_HELPSEARCH,     IDS_HELP_TIP },        // Redundant?
       { IDM_FILERUN,        IDS_RUN_TIP },
       { IDM_LOGOFF,         IDS_LOGOFF_TIP },
       { IDM_EJECTPC,        IDS_EJECT_TIP },
       { IDM_EXITWIN,        IDS_SHUTDOWN_TIP },
#endif
       // Settings Submenu
       { IDM_CONTROLS,       IDS_CONTROL_TIP },
       { IDM_PRINTERS,       IDS_PRINTERS_TIP },
       { IDM_TRAYPROPERTIES, IDS_TRAYPROP_TIP },
       { IDM_NETCONNECT,     IDS_NETCONNECT_TIP},

       // Recent Folder
       { IDM_MYDOCUMENTS,    IDS_MYDOCS_TIP},
     };


    for (int i = 0; i < ARRAYSIZE(s_mpcmdTip); i++)
    {
        if (s_mpcmdTip[i].idCmd == psmd->uId)
        {
            if (LoadString(g_hinst, s_mpcmdTip[i].idInfoTip, pszTip, cch))
                hres = S_OK;
            break;
        }
    }

    return hres;
}

typedef struct _SEARCHEXTDATA
{
    WCHAR wszMenuText[MAX_PATH];
    WCHAR wszHelpText[MAX_PATH];
    int   iIcon;
} SEARCHEXTDATA, FAR* LPSEARCHEXTDATA;


HRESULT CStartMenuCallback::_GetHmenuInfo(SMDATA* psmd, SMINFO* psminfo)
{
    const static struct 
    {
        UINT idCmd;
        int  iImage;
    } s_mpcmdimg[] = { // Top level menu
                       { IDM_PROGRAMS,       II_STPROGS },
                       { IDM_FAVORITES,      II_STFAVORITES },
                       { IDM_RECENT,         II_STDOCS },
                       { IDM_SETTINGS,       II_STSETNGS },
                       { IDM_MENU_FIND,      II_STFIND },
                       { IDM_HELPSEARCH,     II_STHELP },
                       { IDM_FILERUN,        II_STRUN },
                       { IDM_LOGOFF,         II_STLOGOFF },
                       { IDM_EJECTPC,        II_STEJECT },
                       { IDM_EXITWIN,        II_STSHUTD },
#ifdef WINNT // hydra specfic menu items
                       { IDM_MU_SECURITY,    II_MU_STSECURITY },
                       { IDM_MU_DISCONNECT,  II_MU_STDISCONN  },
#endif

                       // Settings Submenu
                       { IDM_SETTINGSASSIST, II_STSETNGS },
                       { IDM_CONTROLS,       II_STCPANEL },
                       { IDM_PRINTERS,       II_STPRNTRS },
                       { IDM_TRAYPROPERTIES, II_STTASKBR },
                       { IDM_MYDOCUMENTS,    II_STDOCS},
                       { IDM_CSC,            -IDI_CSC},
                       { IDM_NETCONNECT,     -IDI_NETCONNECT},
                     };

    ASSERT(IS_VALID_WRITE_PTR(psminfo, SMINFO));

    int iIcon = -1;
    DWORD dwFlags = psminfo->dwFlags;
    MENUITEMINFO mii = {0};
    HRESULT hres = S_FALSE;

    if (psminfo->dwMask & SMIM_ICON)
    {
        if (IsInRange(psmd->uId, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST))
        {
            // The find menu extensions pack their icon into their data member of
            // Menuiteminfo....
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_DATA;
            if (GetMenuItemInfo(psmd->hmenu, psmd->uId, MF_BYCOMMAND, &mii))
            {
#ifdef SHDOC401_DLL
                if (g_uiShell32 >= 5) // g_uiShell32 not defined!! see browseui\sccls.c
#endif
                {
                    // on nt5 we make find menu owner draw so as a result
                    // we pack icons differently
                    LPSEARCHEXTDATA psed = (LPSEARCHEXTDATA)mii.dwItemData;

                    if (psed)
                        psminfo->iIcon = psed->iIcon;
                    else
                        psminfo->iIcon = -1;
                }
#ifdef SHDOC401_DLL
                else
                {
                    // however if we are not running new shell32
                    // the icon is in the good old place...
                    psminfo->iIcon = mii.dwItemData;
                }
#endif
                dwFlags |= SMIF_ICON;
                hres = S_OK;
            }
        }
        else
        {
            UINT uIdLocal = psmd->uId;
            if (uIdLocal == IDM_OPEN_FOLDER)
                uIdLocal = psmd->uIdAncestor;

            for (int i = 0; i < ARRAYSIZE(s_mpcmdimg); i++)
            {
                if (s_mpcmdimg[i].idCmd == uIdLocal)
                {
                    iIcon = s_mpcmdimg[i].iImage;
                    break;
                }
            }

            if (iIcon != -1)
            {
                dwFlags |= SMIF_ICON;
                psminfo->iIcon = Shell_GetCachedImageIndex(TEXT("shell32.dll"), iIcon, 0);
                hres = S_OK;
            }

        }
    }

    if(psminfo->dwMask & SMIM_FLAGS)
    {
        psminfo->dwFlags = dwFlags;

        if ( (psmd->uId == IDM_CONTROLS    && _fCascadeControlPanel   ) ||
             (psmd->uId == IDM_PRINTERS    && _fCascadePrinters       ) ||
             (psmd->uId == IDM_MYDOCUMENTS && _fCascadeMyDocuments    ) ||
             (psmd->uId == IDM_NETCONNECT  && _fCascadeNetConnections ) )
        {
            psminfo->dwFlags |= SMIF_SUBMENU;
            hres = S_OK;
        }
        else switch (psmd->uId)
        {
        case IDM_FAVORITES:
        case IDM_PROGRAMS:
            psminfo->dwFlags |= SMIF_DROPCASCADE;
            hres = S_OK;
            break;

        }
    }

    return hres;
}

DWORD CStartMenuCallback::_GetDemote(SMDATA* psmd)
{
    UEMINFO uei;
    DWORD dwFlags;

    dwFlags = 0;

    uei.cbSize = SIZEOF(uei);
    uei.dwMask = UEIM_HIT;
    if (SUCCEEDED(UEMQueryEvent(psmd->uIdAncestor == IDM_PROGRAMS? &UEMIID_SHELL : &UEMIID_BROWSER, 
        UEME_RUNPIDL, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem, &uei)))
    {
        if (uei.cHit == 0) 
        {
            dwFlags |= SMIF_DEMOTED;
        }
    }

    return dwFlags;
}

int CStartMenuCallback::_GetDarwinIndex(LPCITEMIDLIST pidlFull, CDarwinAd** ppda)
{
    int iRet = -1;
    if (g_hdpaDarwinAds && g_fCritSectionInit)
    {
        ENTERCRITICAL_DARWINADS;
        int chdpa = DPA_GetPtrCount(g_hdpaDarwinAds);
        for (int ihdpa = 0; ihdpa < chdpa; ihdpa++)
        {
            *ppda = (CDarwinAd*)DPA_FastGetPtr(g_hdpaDarwinAds, ihdpa);
            if (*ppda)
            {
                if (ILIsEqual((*ppda)->_pidl, pidlFull))
                {
                    iRet = ihdpa;
                    break;
                }
            }
        }
        LEAVECRITICAL_DARWINADS;
    }
    return iRet;
}

BOOL CStartMenuCallback::_IsDarwinAdvertisement(LPCITEMIDLIST pidlFull)
{
    // What this is doing is comparing the passed in pidl with the
    // list of pidls in g_hdpaDarwinAds. That hdpa contains a list of
    // pidls that are darwin ads.

    // If the background thread is not done, then we must assume that
    // it has not processed the shortcut that we are on. That is why we process it
    // in line.

    // NOTE: There can be two items in the hdpa. This is ok.
    BOOL fAd = FALSE;
    CDarwinAd* pda;
    int iIndex = _GetDarwinIndex(pidlFull, &pda);
    // Are there any ads?
    if (iIndex != -1)
    {
        //This is a Darwin pidl. Is it installed?
        fAd = pda->IsAd();
    }

    return fAd;
}

// BUGBUG (lamadio): There is a duplicate of this helper in browseui\browmenu.cpp
//                   When modifying this, rev that one as well.
void UEMRenamePidl(const GUID *pguidGrp1, IShellFolder* psf1, LPCITEMIDLIST pidl1,
                   const GUID *pguidGrp2, IShellFolder* psf2, LPCITEMIDLIST pidl2)
{
    UEMINFO uei;
    uei.cbSize = SIZEOF(uei);
    uei.dwMask = UEIM_HIT;
    if (SUCCEEDED(UEMQueryEvent(pguidGrp1, 
                                UEME_RUNPIDL, (WPARAM)psf1, 
                                (LPARAM)pidl1, &uei)) &&
                                uei.cHit > 0)
    {
        UEMSetEvent(pguidGrp2, 
            UEME_RUNPIDL, (WPARAM)psf2, (LPARAM)pidl2, &uei);

        uei.cHit = 0;
        UEMSetEvent(pguidGrp1, 
            UEME_RUNPIDL, (WPARAM)psf1, (LPARAM)pidl1, &uei);
    }
}

// BUGBUG (lamadio): There is a duplicate of this helper in browseui\browmenu.cpp
//                   When modifying this, rev that one as well.
void UEMDeletePidl(const GUID *pguidGrp, IShellFolder* psf, LPCITEMIDLIST pidl)
{
    UEMINFO uei;
    uei.cbSize = SIZEOF(uei);
    uei.dwMask = UEIM_HIT;
    uei.cHit = 0;
    UEMSetEvent(pguidGrp, UEME_RUNPIDL, (WPARAM)psf, (LPARAM)pidl, &uei);
}

HRESULT CStartMenuCallback::_ProcessChangeNotify(SMDATA* psmd, LONG lEvent, 
                                                 LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    switch (lEvent)
    {
    case SHCNE_ASSOCCHANGED:
        _ReValidateDarwinCache();
        return S_OK;

    case SHCNE_RENAMEFOLDER:
        // BUGBUG (lamadio): We should move the MenuOrder stream as well. 5.5.99
    case SHCNE_RENAMEITEM:
        {
            LPITEMIDLIST pidlPrograms;
            LPITEMIDLIST pidlProgramsCommon;
            LPITEMIDLIST pidlFavorites;
            SHGetFolderLocation(NULL, CSIDL_PROGRAMS, NULL, 0, &pidlPrograms);
            SHGetFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, NULL, 0, &pidlProgramsCommon);
            SHGetFolderLocation(NULL, CSIDL_FAVORITES, NULL, 0, &pidlFavorites);

            BOOL fPidl1InStartMenu =    ILIsParent(pidlPrograms, pidl1, FALSE) ||
                                        ILIsParent(pidlProgramsCommon, pidl1, FALSE);
            BOOL fPidl1InFavorites =    ILIsParent(pidlFavorites, pidl1, FALSE);


            // If we're renaming something from the Start Menu
            if ( fPidl1InStartMenu ||fPidl1InFavorites)
            {
                IShellFolder* psfFrom;
                LPCITEMIDLIST pidlFrom;
                if (SUCCEEDED(SHBindToParent(pidl1, IID_IShellFolder, (void**)&psfFrom, &pidlFrom)))
                {
                    // Into the Start Menu
                    BOOL fPidl2InStartMenu =    ILIsParent(pidlPrograms, pidl2, FALSE) ||
                                                ILIsParent(pidlProgramsCommon, pidl2, FALSE);
                    BOOL fPidl2InFavorites =    ILIsParent(pidlFavorites, pidl2, FALSE);
                    if (fPidl2InStartMenu || fPidl2InFavorites)
                    {
                        IShellFolder* psfTo;
                        LPCITEMIDLIST pidlTo;

                        if (SUCCEEDED(SHBindToParent(pidl2, IID_IShellFolder, 
                            (void**)&psfTo, &pidlTo)))
                        {
                            // Then we need to rename it
                            UEMRenamePidl(fPidl1InStartMenu ? &UEMIID_SHELL: &UEMIID_BROWSER, 
                                            psfFrom, pidlFrom, 
                                          fPidl2InStartMenu ? &UEMIID_SHELL: &UEMIID_BROWSER, 
                                            psfTo, pidlTo);
                            psfTo->Release();
                        }
                    }
                    else
                    {
                        // Otherwise, we delete it.
                        UEMDeletePidl(fPidl1InStartMenu ? &UEMIID_SHELL : &UEMIID_BROWSER, 
                            psfFrom, pidlFrom);
                    }

                    psfFrom->Release();
                }
            }

            ILFree(pidlPrograms);
            ILFree(pidlProgramsCommon);
            ILFree(pidlFavorites);
        }
        break;

    case SHCNE_DELETE:
        // BUGBUG (lamadio): We should nuke the MenuOrder stream as well. 5.5.99
    case SHCNE_RMDIR:
        {
            IShellFolder* psf;
            LPCITEMIDLIST pidl;

            if (SUCCEEDED(SHBindToParent(pidl1, IID_IShellFolder, (void**)&psf, &pidl)))
            {
                // NOTE favorites is the only that will be initialized
                UEMDeletePidl(psmd->uIdAncestor == IDM_FAVORITES? &UEMIID_BROWSER : &UEMIID_SHELL, 
                    psf, pidl);
                psf->Release();
            }

        }
        break;

    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        {
            IShellFolder* psf;
            LPCITEMIDLIST pidl;

            if (SUCCEEDED(SHBindToParent(pidl1, IID_IShellFolder, (void**)&psf, &pidl)))
            {
                UEMINFO uei;
                uei.cbSize = SIZEOF(uei);
                uei.dwMask = UEIM_HIT;
                uei.cHit = UEM_NEWITEMCOUNT;
                UEMSetEvent(psmd->uIdAncestor == IDM_FAVORITES? &UEMIID_BROWSER: &UEMIID_SHELL, 
                    UEME_RUNPIDL, (WPARAM)psf, (LPARAM)pidl, &uei);
                psf->Release();
            }

        }
        break;
    }

    return S_FALSE;
}

HRESULT CStartMenuCallback::_GetSFInfo(SMDATA* psmd, SMINFO* psminfo)
{
    if (psminfo->dwMask & SMIM_FLAGS &&
        (psmd->uIdAncestor == IDM_PROGRAMS ||
         psmd->uIdAncestor == IDM_FAVORITES))
    {
        if (_fExpandoMenus)
        {
            psminfo->dwFlags |= _GetDemote(psmd);
        }

        // This is a little backwards. If the Restriction is On, Then we allow the feature.
        if (SHRestricted(REST_GREYMSIADS) &&
            psmd->uIdAncestor == IDM_PROGRAMS)
        {
            LPITEMIDLIST pidlFull = FullPidlFromSMData(psmd);
            if (pidlFull)
            {
                if (_IsDarwinAdvertisement(pidlFull))
                {
                    psminfo->dwFlags |= SMIF_ALTSTATE;
                }
                ILFree(pidlFull);
            }
        }
    }
    return S_OK;
}

void CStartMenuCallback::_ReValidateDarwinCache()
{
    if (g_hdpaDarwinAds && g_fCritSectionInit)
    {
        ENTERCRITICAL_DARWINADS;
        int chdpa = DPA_GetPtrCount(g_hdpaDarwinAds);
        for (int ihdpa = 0; ihdpa < chdpa; ihdpa++)
        {
            CDarwinAd* pda = (CDarwinAd*)DPA_FastGetPtr(g_hdpaDarwinAds, ihdpa);
            if (pda)
            {
                pda->CheckInstalled();
            }
        }
        LEAVECRITICAL_DARWINADS;
    }

}

// Determines if a CSIDL is a child of another CSIDL
// e.g.
//  CSIDL_STARTMENU = c:\foo\bar\Start Menu
//  CSIDL_PROGRAMS  = c:\foo\bar\Start Menu\Programs
//  Return true
BOOL IsCSIDLChild(int csidlParent, int csidlChild)
{
    BOOL fChild = FALSE;
    TCHAR sz1[MAX_PATH];
    TCHAR sz2[MAX_PATH];
    TCHAR szCommonRoot[MAX_PATH];
    if (SUCCEEDED(SHGetSpecialFolderPath(NULL, sz1, csidlParent, FALSE)))
    {
        if (SUCCEEDED(SHGetSpecialFolderPath(NULL, sz2, csidlChild, FALSE)))
        {
            if (PathCommonPrefix(sz1, sz2, szCommonRoot) ==
                lstrlen(sz1))
            {
                fChild = TRUE;
            }
        }
    }

    return fChild;
}

// Creates the "Start Menu\\Programs" section of the start menu by
// generating a Merged Shell folder, setting the locations into that item
// then sets it into the passed IShellMenu.
HRESULT CStartMenuCallback::InitializeProgramsShellMenu(IShellMenu* psm)
{
    HRESULT hres;
    DWORD dwDisp;
    HKEY hkeyPrograms = NULL;
    LPITEMIDLIST pidl = NULL;

    DWORD dwInitFlags = SMINIT_VERTICAL;
    if (!_fScrollPrograms)
        dwInitFlags |= SMINIT_MULTICOLUMN;

    if (SHRestricted(REST_NOCHANGESTARMENU))
        dwInitFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;

    EVAL(SUCCEEDED(psm->Initialize(this, IDM_PROGRAMS, IDM_PROGRAMS, dwInitFlags)));

    _InitializePrograms();

    RegCreateKeyEx(HKEY_CURRENT_USER, STRREG_STARTMENU TEXT("\\Programs"), NULL, NULL,
        REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
        NULL, &hkeyPrograms, &dwDisp);

#ifdef WINNT
    IAugmentedShellFolder2* psf = NULL;
    hres = CoCreateInstance(CLSID_AugmentedShellFolder2, NULL, CLSCTX_INPROC, 
                            IID_IAugmentedShellFolder2, (void **)&psf);
    if (SUCCEEDED(hres))
    {
        IShellFolder* psfFolder;
        hres = SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidl);
        if (pidl && SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidl, (void **)&psfFolder)))
        {
            SetFilesystemInfo(psfFolder, pidl, CSIDL_PROGRAMS);
            hres = psf->AddNameSpace(NULL, psfFolder, pidl, ASFF_DEFAULT | ASFF_DEFNAMESPACE_ALL);
            if (!SHRestricted(REST_NOCOMMONGROUPS))
            {
                LPITEMIDLIST pidl2 = NULL;
                IShellFolder* psfCommon;
                SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidl2);
                if (pidl2 && 
                    SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidl2, (void **)&psfCommon)))
                {
                    SetFilesystemInfo(psfCommon, pidl2, CSIDL_COMMON_PROGRAMS);
                    psf->AddNameSpace(NULL, psfCommon, pidl2, ASFF_DEFAULT);
                    psfCommon->Release();
                }
                ILFree(pidl2);
            }

            psfFolder->Release();
        }

        if (SUCCEEDED(hres))
        {

#else
        // Windows 9x  does not have a Common group.
    IShellFolder* psf = NULL;
    SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidl);
    if (pidl && SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidl, (void **)&psf)))
    {
        SetFilesystemInfo(psf, pidl, CSIDL_PROGRAMS);
        {
#endif

            // We should have a pidl from CSIDL_Programs
            ASSERT(pidl);

            // We should have a shell folder from the bind.
            ASSERT(psf);

            // We used to register for change notify at CSIDL_STARTMENU and assumed 
            // that CSIDL_PROGRAMS was a child of CSIDL_STARTMENU. Since this wasn't always the 
            // case, I removed the optimization.

            // Both panes are registered recursive. So, When CSIDL_PROGRAMS _IS_ a 
            // child of CSIDL_STARTMENU we can enter a code path where when destroying 
            // CSIDL_PROGRAMS, we unregister it. This will flush the change nofiy queue 
            // of CSIDL_STARTMENU, and blow away all of the children, including CSIDL_PROGRAMS, 
            // while we are in the middle of destroying it... See the problem? I have been adding 
            // reentrance "Blockers" but this only delayed where we crashed. 
            // What was needed was to determine if Programs was a child of the Start Menu directory.
            // if it was we need to add the optmimization. If it's not we don't have a problem.

            // BUGBUG (lamadio): If one of the two is redirected, then this will get optimized
            // we can't do better than this because both are registed recursive, and this will fault...
            BOOL fOptimize = IsCSIDLChild(CSIDL_STARTMENU, CSIDL_PROGRAMS)
#ifdef WINNT
                || IsCSIDLChild(CSIDL_COMMON_STARTMENU, CSIDL_COMMON_PROGRAMS)
#endif
                ;

            hres = psm->SetShellFolder(psf, pidl, hkeyPrograms, SMSET_TOP | (fOptimize?SMSET_DONTREGISTERCHANGENOTIFY:0));
        }
    }

    if (psf)
        psf->Release();

    ILFree(pidl);                        

    if (FAILED(hres))
        RegCloseKey(hkeyPrograms);

    return hres;
}

// Creates the "Start Menu\\<csidl>" section of the start menu by
// looking up the csidl, generating the Hkey from HKCU\pszRoot\pszValue,
//  Initializing the IShellMenu with dwPassInitFlags, then setting the locations 
// into the passed IShellMenu passing the flags dwSetFlags.
HRESULT CStartMenuCallback::InitializeCSIDLShellMenu(int uId, int csidl, LPTSTR pszRoot, LPTSTR pszValue,
                                                 DWORD dwPassInitFlags, DWORD dwSetFlags,  BOOL fAddOpen,
                                                 IShellMenu* psm)
{
    DWORD dwDisp;
    HKEY hKey = NULL;
    LPITEMIDLIST pidl;
    HRESULT hres = E_FAIL;
    TCHAR szPath[MAX_PATH];
    DWORD dwInitFlags = SMINIT_VERTICAL | dwPassInitFlags;

    if (SHRestricted(REST_NOCHANGESTARMENU))
        dwInitFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;

    EVAL(SUCCEEDED(psm->Initialize(this, uId, uId, dwInitFlags)));

    SHGetSpecialFolderLocation(NULL, csidl, &pidl);

    // Do we have a pidl?
    if (pidl)
    {
        IShellFolder* psfFolder;
        // Yes; this menu must want to enumerate a shellfolder
        if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidl, (void **)&psfFolder)))
        {
            SetFilesystemInfo(psfFolder, pidl, csidl);

            if (pszRoot)
            {
                StrCpyN(szPath, pszRoot, ARRAYSIZE(szPath));
                if (pszValue)
                {
                    PathAppend(szPath, pszValue);
                }

                RegCreateKeyEx(HKEY_CURRENT_USER, szPath, NULL, NULL,
                    REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                    NULL, &hKey, &dwDisp);
            }

            // Point the menu to the shellfolder
            hres = psm->SetShellFolder(psfFolder, pidl, hKey, dwSetFlags);
            psfFolder->Release();       //IMenuPopup hangs onto this, but they do an addref

            if (SUCCEEDED(hres))
            {
                if (fAddOpen && _fAddOpenFolder)
                {
                    HMENU hMenu = SHLoadMenuPopup(HINST_THISDLL, MENU_STARTMENU_OPENFOLDER);
                    if (EVAL(hMenu))
                    {
                        psm->SetMenu(hMenu, _hwnd, SMSET_BOTTOM);
                    }
                }
            }
            else
                RegCloseKey(hKey);
        }
        ILFree(pidl); // Free the pidl returned from SHGetSpecialFolderLocation
    }

    return hres;
}

// This generates the Recent | My Documents sub menu.
HRESULT CStartMenuCallback::InitializeDocumentsShellMenu(IShellMenu* psm)
{
    HRESULT hres = E_FAIL;

    hres = InitializeCSIDLShellMenu(IDM_RECENT, CSIDL_RECENT, NULL, NULL,
                                SMINIT_RESTRICT_DRAGDROP, SMSET_BOTTOM, FALSE,
                                psm);

    // Allowed to show the my documents item?
    if (!SHRestricted(REST_NOSMMYDOCS))
    {
        LPITEMIDLIST pidlMyDocs;
        SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidlMyDocs);
        if (pidlMyDocs != NULL)
        {   
            HMENU hMenu = SHLoadMenuPopup(HINST_THISDLL, MENU_STARTMENU_MYDOCS);
            if (EVAL(hMenu))
            {
                psm->SetMenu(hMenu, _hwnd, SMSET_TOP);
            }
            ILFree(pidlMyDocs);
        }
    }
    return hres;
}

HRESULT CStartMenuCallback::InitializeFastItemsShellMenu(IShellMenu* psm)
{
    HRESULT hres = E_FAIL;
    DWORD dwDisp;
    DWORD dwFlags = SMINIT_TOPLEVEL | SMINIT_VERTICAL;

    if (SHRestricted(REST_NOCHANGESTARMENU))
        dwFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;

    psm->Initialize(this, 0, ANCESTORDEFAULT, dwFlags);

    _InitializePrograms();

    // Add the fast item folder to the top of the menu
    IShellFolder* psfFast;
    LPITEMIDLIST pidlFast;

    if (SUCCEEDED(GetFastItemFolder(&psfFast, &pidlFast)))
    {
        HKEY hMenuKey = NULL;   // WARNING: pmb2->Initialize() will always owns hMenuKey, so don't close it

        RegCreateKeyEx(HKEY_CURRENT_USER, STRREG_STARTMENU, NULL, NULL,
            REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
            NULL, &hMenuKey, &dwDisp);

        TraceMsg(TF_MENUBAND, "Root Start Menu Key Is %d", hMenuKey);
        hres = psm->SetShellFolder(psfFast, pidlFast, hMenuKey, SMSET_TOP | SMSET_NOEMPTY);

        psfFast->Release();
        ILFree(pidlFast);
    }

    return hres;
}

HRESULT CStartMenuCallback::_InitializeFindMenu(IShellMenu* psm)
{
    HRESULT hres = E_FAIL;

    psm->Initialize(this, IDM_MENU_FIND, IDM_MENU_FIND, SMINIT_VERTICAL);

    HMENU hmenu = CreatePopupMenu();
    if(hmenu)
    {
        ATOMICRELEASE(_pcmFind);

        if (_ptp)
        {
            if (SUCCEEDED(_ptp->GetFindCM(hmenu, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST, &_pcmFind)))
            {
                IContextMenu2 *pcm2;
                _pcmFind->QueryInterface(IID_IContextMenu2, (void **)&pcm2);
                if (pcm2)
                {
                    pcm2->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)hmenu, (LPARAM)0);
                    pcm2->Release();
                }
            }

            if (_pcmFind)
            {
                hres = psm->SetMenu(hmenu, NULL, SMSET_TOP);
                // Don't Release _pcmFind
            }
        }

        // Since we failed to create the ShellMenu
        // we need to dispose of this HMENU
        if (FAILED(hres))
            DestroyMenu(hmenu);
    }

    return hres;
}

HRESULT CStartMenuCallback::InitializeNetworkConnections(IShellMenu* psm)
{
    IShellFolder* psfFolder;
    HRESULT hres = CoCreateInstance (CLSID_ConnectionFolder, NULL, CLSCTX_INPROC, 
            IID_IShellFolder, (LPVOID *)&psfFolder);

    psm->Initialize(this, IDM_NETCONNECT, IDM_NETCONNECT, SMINIT_VERTICAL);

    if (SUCCEEDED(hres))
    {
        LPITEMIDLIST pidl;
        hres = GetNetConnectPidl(&pidl);
        if (SUCCEEDED(hres))
        {
            hres = psm->SetShellFolder(psfFolder, pidl, NULL, SMSET_TOP);

            if (_fAddOpenFolder)
            {
                HMENU hMenu = SHLoadMenuPopup(HINST_THISDLL, MENU_STARTMENU_OPENFOLDER);
                if (EVAL(hMenu))
                {
                    psm->SetMenu(hMenu, _hwnd, SMSET_BOTTOM);
                }
            }
            ILFree(pidl);
        }
                
        psfFolder->Release();
    }

    return hres;
}



HRESULT CStartMenuCallback::InitializeSubShellMenu(int idCmd, IShellMenu* psm)
{
    HRESULT hres = E_FAIL;

    switch(idCmd)
    {
    case IDM_PROGRAMS:
        hres = InitializeProgramsShellMenu(psm);
        break;

    case IDM_RECENT:
        hres = InitializeDocumentsShellMenu(psm);
        break;

    case IDM_MENU_FIND:
        hres = _InitializeFindMenu(psm);
        break;

    case IDM_FAVORITES:
        hres = InitializeCSIDLShellMenu(IDM_FAVORITES, CSIDL_FAVORITES, STRREG_FAVORITES,
                             NULL, 0, SMSET_HASEXPANDABLEFOLDERS | SMSET_USEBKICONEXTRACTION, FALSE,
                             psm);
        break;
    
    case IDM_CONTROLS:
        hres = InitializeCSIDLShellMenu(IDM_CONTROLS, CSIDL_CONTROLS, STRREG_STARTMENU,
                             TEXT("ControlPanel"), 0, 0,  TRUE,
                             psm);
        break;

    case IDM_PRINTERS:
        hres = InitializeCSIDLShellMenu(IDM_PRINTERS, CSIDL_PRINTERS, STRREG_STARTMENU,
                             TEXT("Printers"), 0, 0,  TRUE,
                             psm);
        break;

    case IDM_MYDOCUMENTS:
        hres = InitializeCSIDLShellMenu(IDM_MYDOCUMENTS, CSIDL_PERSONAL, STRREG_STARTMENU,
                             TEXT("MyDocuments"), 0, 0,  TRUE,
                             psm);
        break;

    case IDM_NETCONNECT:
        hres = InitializeNetworkConnections(psm);
        break;
    }

    return hres;
}

HRESULT CStartMenuCallback::_GetObject(LPSMDATA psmd, REFIID riid, void** ppvOut)
{
    HRESULT hres = E_FAIL;
    UINT    uId = psmd->uId;

    ASSERT(ppvOut);
    ASSERT(IS_VALID_READ_PTR(psmd, SMDATA));

    *ppvOut = NULL;

    if (IsEqualGUID(riid, IID_IShellMenu))
    {
        IShellMenu* psm = NULL;
        hres = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC, IID_IShellMenu, (void**)&psm);

        if (SUCCEEDED(hres))
        {
            hres = InitializeSubShellMenu(uId, psm);
 
            if (FAILED(hres))
            {
                psm->Release();
                psm = NULL;
            }
        }

        *ppvOut = psm;
    }
    else if (IsEqualGUID(riid, IID_IContextMenu))
    {
        //
        //  NOTE - we dont allow users to open the recent folder this way - ZekeL - 1-JUN-99
        //  because this is really an internal folder and not a user folder.
        //
        
        switch (uId)
        {
        case IDM_PROGRAMS:
        case IDM_FAVORITES:
        case IDM_MYDOCUMENTS:
        case IDM_CONTROLS:
        case IDM_PRINTERS:
        case IDM_NETCONNECT:
            {
                CStartContextMenu* pcm = new CStartContextMenu(uId);
                if (pcm)
                {
                    hres = pcm->QueryInterface(riid, ppvOut);
                    pcm->Release();
                }
                else
                    hres = E_OUTOFMEMORY;
            }
        }
    }


    return hres;
}

HRESULT CStartMenuCallback::_FilterFavorites(IShellFolder* psf, LPCITEMIDLIST pidl)
{
    HRESULT hres = S_FALSE;
    if (psf && pidl)
    {
        DWORD dwAttribs = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_LINK;
        if (SUCCEEDED(psf->GetAttributesOf(1, &pidl, &dwAttribs)))
        {
            // include all non file system objects, folders and links
            // (non file system things like the Channel folders contents)

            if (!(dwAttribs & (SFGAO_FOLDER | SFGAO_LINK)) &&
                (dwAttribs & SFGAO_FILESYSTEM))
            {
                hres = S_OK;
            }
        }
    }
    return hres;
}


HRESULT CStartMenuCallback::_FilterPidl(UINT uParent, IShellFolder* psf, LPCITEMIDLIST pidl)
{
    HRESULT hres = S_FALSE;
    STRRET strret;

    ASSERT(IS_VALID_PIDL(pidl));
    ASSERT(IS_VALID_CODE_PTR(psf, IShellFolder));

    if (uParent == IDM_PROGRAMS || uParent == IDM_TOPLEVELSTARTMENU)
    {
        TCHAR szChild[MAX_PATH];
        if (SUCCEEDED(psf->GetDisplayNameOf(pidl, SHGDN_INFOLDER, &strret)) &&
            SUCCEEDED(StrRetToBuf(&strret, pidl, szChild, ARRAYSIZE(szChild))))
        {
            //
            // HACKHACK (lamadio): This code assumes that the Display name
            // of the Programs and Commons Programs folders are the same. It
            // also assumes that the "programs" folder in the Start Menu folder
            // is the same name as the one pointed to by CSIDL_PROGRAMS.
            // 

            // We filter: Programs, Windows Update and Administrative tools.
            if (uParent == IDM_TOPLEVELSTARTMENU)
            {
                if (lstrcmpi(szChild, _szPrograms) == 0 || 
                    SHRestricted(REST_NOUPDATEWINDOWS) && _pszWindowsUpdate && lstrcmpi(szChild, _pszWindowsUpdate) == 0)
                {
                  hres = S_OK;
                }
            }
            else if ( /*uParent == IDM_PROGRAMS && */                
                      !_fShowAdminTools && _pszAdminTools && lstrcmpi(szChild, _pszAdminTools) == 0)
            {
                hres = S_OK;
            }
        }
    }

    return hres;
}

BOOL LinkGetInnerPidl(IShellFolder *psf, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlOut, DWORD *pdwAttr)
{
    *ppidlOut = NULL;

    IShellLink *psl;
    HRESULT hr = psf->GetUIObjectOf(NULL, 1, &pidl, IID_IShellLink, NULL, (void **)&psl);
    if (SUCCEEDED(hr))
    {
        psl->GetIDList(ppidlOut);

        if (*ppidlOut)
        {
            if (FAILED(SHGetAttributesOf(*ppidlOut, pdwAttr)))
            {
                ILFree(*ppidlOut);
                *ppidlOut = NULL;
            }
        }
        psl->Release();
    }
    return (*ppidlOut != NULL);
}


//
//  _FilterRecentPidl() 
//  the Recent Documents folder can now (NT5) have more than 15 or 
//  so documents, but we only want to show the 15 most recent that we always have on
//  the start menu.  this means that we need to filter out all folders and 
//  anything more than MAXRECENTDOCS
//
HRESULT CStartMenuCallback::_FilterRecentPidl(IShellFolder* psf, LPCITEMIDLIST pidl)
{
    HRESULT hres = S_OK;

    ASSERT(IS_VALID_PIDL(pidl));
    ASSERT(IS_VALID_CODE_PTR(psf, IShellFolder));
    ASSERT(_cRecentDocs != -1);
    
    ASSERT(_cRecentDocs <= MAXRECENTDOCS);
    
    //  if we already reached our limit, dont go over...
    if (_hmruRecent && (_cRecentDocs < MAXRECENTDOCS))
    {
        //  we now must take a looksee for it...
        int iItem = FindMRUData(_hmruRecent, (void *) pidl, ILGetSize(pidl), NULL);
        DWORD dwAttr = SFGAO_FOLDER | SFGAO_BROWSABLE;
        LPITEMIDLIST pidlTrue;

        //  need to find out if the link points to a folder...
        //  because we dont want
        if ((iItem != -1) && LinkGetInnerPidl(psf, pidl, &pidlTrue, &dwAttr))
        {
            if (!(dwAttr & SFGAO_FOLDER))
            {
                //  we have a link to something that isnt a folder 
                hres = S_FALSE;
                _cRecentDocs++;
            }

            ILFree(pidlTrue);
        }
    }
                
    //  return S_OK if you dont want to show this item...

    return hres;
}


HRESULT CStartMenuCallback::_HandleAccelerator(TCHAR ch, SMDATA* psmdata)
{
    // Since we renamed the 'Find' menu to 'Search' the PMs wanted to have
    // an upgrade path for users (So they can continue to use the old accelerator
    // on the new menu item.)
    // To enable this, when toolbar detects that there is not an item in the menu
    // that contains the key that has been pressed, then it sends a TBN_ACCL.
    // This is intercepted by mnbase, and translated into SMC_ACCEL. 
    if (CharUpper((LPTSTR)ch) == CharUpper((LPTSTR)_szFindMeumonic[0]))
    {
        psmdata->uId = IDM_MENU_FIND;
        return S_OK;
    }

    return S_FALSE;
}

HRESULT CStartMenuCallback::_GetTip(LPTSTR pstrTitle, LPTSTR pstrTip)
{
    LoadString(HINST_THISDLL, IDS_CHEVRONTIPTITLE, pstrTitle, MAX_PATH);
    LoadString(HINST_THISDLL, IDS_CHEVRONTIP, pstrTip, MAX_PATH);

    // Why would this fail?
    if (EVAL(pstrTitle[0] != TEXT('\0') && pstrTip[0] != TEXT('\0')))
        return S_OK;

    return S_FALSE;
}


STDAPI  CStartMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres = E_FAIL;
    IMenuPopup* pmp = NULL;

    *ppvOut = NULL;

    CStartMenuCallback* psmc = new CStartMenuCallback();

    if (psmc)
    {
        IShellMenu* psm;

        hres = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, IID_IShellMenu, (void**)&psm);
        if (SUCCEEDED(hres))
        {
            hres = CoCreateInstance(CLSID_MenuDeskBar, punkOuter, CLSCTX_INPROC_SERVER, IID_IMenuPopup, (void**)&pmp);
            if (SUCCEEDED(hres)) 
            {
                IBandSite* pbs;
                hres = CoCreateInstance(CLSID_MenuBandSite, NULL, CLSCTX_INPROC_SERVER, IID_IBandSite, (void **)&pbs);
                if (SUCCEEDED(hres)) 
                {
                    hres = pmp->SetClient(pbs);
                    if (SUCCEEDED(hres)) 
                    {
                        IDeskBand* pdb;
                        hres = psm->QueryInterface(IID_IDeskBand, (void**)&pdb);
                        if (SUCCEEDED(hres))
                        {
                           hres = pbs->AddBand(pdb);
                           pdb->Release();
                        }
                    }
                    pbs->Release();
                }
                // Don't free pmp. We're using it below.
            }

            if (SUCCEEDED(hres))
            {
                // This is so the ref counting happens correctly.
                psm->Initialize(psmc, 0, 0, SMINIT_VERTICAL | SMINIT_TOPLEVEL);
                hres = psmc->InitializeFastItemsShellMenu(psm);
            }

            psm->Release();
        }
        psmc->Release();
    }

    if (SUCCEEDED(hres))
    {
        hres = pmp->QueryInterface(riid, ppvOut);
    }
    else
    {
        // We need to do this so that it does a cascading delete
        IUnknown_SetSite(pmp, NULL);        
    }

    if (pmp)
        pmp->Release();

    return hres;
}



////////////////////////////////////////////////////////////////
/// start menu and desktop hotkey tasks

#define START_BANNER_COLOR_NT   RGB(0, 0, 0)
#define START_BANNER_COLOR_95   RGB(128, 128, 128)




//
// CStartMenuTask object.  This task enumerates the start menu for hotkeys 
// and to cache the icons.
//


#if 0
#define TF_SMTASK TF_CUSTOM1
#else
#define TF_SMTASK 0
#endif

#undef  THISCLASS   
#undef  SUPERCLASS
#define SUPERCLASS  CRunnableTask


class CStartMenuTask : public CRunnableTask,
                       public IStartMenuTask
{
public:
    // *** IUnknown Methods ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return SUPERCLASS::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void){ return SUPERCLASS::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
    
    // *** IShellFolderTask Methods ***
    virtual STDMETHODIMP InitTaskSFT(IShellFolder *psfParent, LPITEMIDLIST pidlFull, 
                                     LONG nMaxRecursionLevel, DWORD dwFlags, DWORD dwTaskPriority);

    // *** IStartMenuTask Methods ***
    virtual STDMETHODIMP InitTaskSMT(IShellHotKey * pshk, int iThreadPriority);

    // *** CRunnableTask virtual methods
    virtual STDMETHODIMP RunInitRT(void);
    virtual STDMETHODIMP InternalResumeRT(void);

protected:
    friend HRESULT CStartMenuTask_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvOut);

    CStartMenuTask();
    ~CStartMenuTask();

    HRESULT     _CreateNewTask(IShellFolder * psf, LPCITEMIDLIST pidl);
    
    int             _iThreadPriority;
    DWORD           _dwTaskPriority;
    DWORD           _dwFlags;
    LONG            _nMaxRecursionLevel ;
    IShellFolder*   _psf;
    LPITEMIDLIST    _pidlFolder;
    IShellHotKey *  _pshk;
    IEnumIDList *   _pei;
    IShellLinkDataList*     _psldl;
};

HRESULT CStartMenuTask_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvOut)
{
    CStartMenuTask* pTask = new CStartMenuTask();
    if (pTask)
    {
        HRESULT hres = pTask->QueryInterface(riid, ppvOut);
        pTask->Release();
        return hres;
    }
    return E_OUTOFMEMORY;
}


// constructor
CStartMenuTask::CStartMenuTask() : CRunnableTask(RTF_SUPPORTKILLSUSPEND),
    _nMaxRecursionLevel(-1)
{
    InterlockedIncrement((PLONG)&g_cStartMenuTask);
}


// destructor
CStartMenuTask::~CStartMenuTask()
{
    if (_pidlFolder)
        ILFree(_pidlFolder);
        
    ATOMICRELEASE(_pei);
    ATOMICRELEASE(_psf);
    ATOMICRELEASE(_pshk);
    ATOMICRELEASE(_psldl);
    InterlockedDecrement((PLONG)&g_cStartMenuTask);
}


STDMETHODIMP CStartMenuTask::QueryInterface(REFIID riid, void ** ppvObj)
{
    HRESULT hres;
    static const QITAB qit[] = {
        QITABENT(CStartMenuTask, IShellFolderTask),
        QITABENT(CStartMenuTask, IStartMenuTask),
        { 0 },
    };

    hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = SUPERCLASS::QueryInterface(riid, ppvObj);
        
    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolderTask::InitTaskSFT method

*/
STDMETHODIMP CStartMenuTask::InitTaskSFT(IShellFolder *psfParent, LPITEMIDLIST pidlFull,
                                         LONG nMaxRecursionLevel, DWORD dwFlags, DWORD dwTaskPriority)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (!psfParent)
        return E_INVALIDARG;

    ASSERT(IS_VALID_PIDL(pidlFull));
    
    _pidlFolder = ILClone(pidlFull);
    if (_pidlFolder)
    {
        // we have a pidlChild, so bind to him
        IShellFolder* psf;
        hr = psfParent->BindToObject(ILFindLastID(_pidlFolder), NULL, IID_IShellFolder, (void **)&psf);
        if (SUCCEEDED(hr))
        {
            _psf = psf;
            _dwTaskPriority = dwTaskPriority;
            _dwFlags = dwFlags;
            _nMaxRecursionLevel = nMaxRecursionLevel ;

#ifdef DEBUG
            TCHAR szPath[MAX_PATH];
            
            if (pidlFull)
                SHGetPathFromIDList(pidlFull, szPath);
            else
                szPath[0] = TEXT('0');

            TraceMsg(TF_SMTASK, "StartMenuTask: task %#lx (pri %d) is '%s'", _dwTaskID, dwTaskPriority, szPath);
#endif
        }
    }

    return hr;
}


/*----------------------------------------------------------
Purpose: IStartMenuTask::InitTaskSMT method

*/
STDMETHODIMP CStartMenuTask::InitTaskSMT(IShellHotKey * pshk, int iThreadPriority)
{
    ASSERT(pshk);

    _pshk = pshk;
    if (_pshk)
        _pshk->AddRef();

    _iThreadPriority = iThreadPriority;
    
    return S_OK;
}


HRESULT CStartMenuTask::_CreateNewTask(IShellFolder * psf, LPCITEMIDLIST pidl)
{   
    HRESULT hres;
    LPSHELLTASKSCHEDULER pScheduler;
    
    // Get the shared task scheduler
    // BUGBUG : this needs changing so the pointer to the scheduler is passed around, not
    // BUGBUG : a CoCreate each time....
    hres = CoCreateInstance( CLSID_SharedTaskScheduler, NULL, CLSCTX_INPROC, 
            IID_IShellTaskScheduler, (void **) & pScheduler );
    if (SUCCEEDED(hres))
    {
        // create a new task
        IStartMenuTask * psmt;
        
        if (SUCCEEDED(CoCreateInstance(CLSID_StartMenuTask, NULL, CLSCTX_INPROC,
                                       IID_IStartMenuTask, (void **)&psmt)))
        {
            // Initialize the task
            LPITEMIDLIST pidlFull = ILCombine(_pidlFolder, pidl);

            if (pidlFull)
            {
                if (SUCCEEDED(psmt->InitTaskSFT(_psf, pidlFull, _nMaxRecursionLevel - 1, 
                                                _dwTaskPriority - 1, THREAD_PRIORITY_NORMAL)))
                {
                    IRunnableTask * ptask;

                    psmt->InitTaskSMT(_pshk, _iThreadPriority);
                    
                    if (SUCCEEDED(psmt->QueryInterface(IID_IRunnableTask, (void **)&ptask)))
                    {
                        // Add it to the scheduler, with the priority lower than
                        // the parent folders. 
                        hres = pScheduler->AddTask(ptask, CLSID_StartMenuTask, 0, _dwTaskPriority - 1);
                        
                        ptask->Release();
                    }
                }
                ILFree(pidlFull);
            }
            psmt->Release();
        }

        // release the explorer task scheduler
        pScheduler->Release();
    }
    return hres;
}


/*----------------------------------------------------------
Purpose: CRunnableTask::RunInitRT method

*/
HRESULT CStartMenuTask::RunInitRT(void)
{
    // This is the first time that ::Run has been called on this task,
    // so create our pEnumIDList interface.

    ASSERT(IS_VALID_CODE_PTR(_psf, IShellFolder));

/*    // We will detect failure of this at use time.
    CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC, IID_IShellLinkDataList, (void**)&_psldl);*/

    if (!g_fCritSectionInit)
    {
        g_fCritSectionInit = TRUE;
        InitializeCriticalSection(&g_csDarwinAds);
    }

    return _psf->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &_pei);
}


/*----------------------------------------------------------
Purpose: CRunnableTask::InternalResumeRT method

*/

//#define _SM_LOADLINKONCE
#ifdef _SM_LOADLINKONCE
HRESULT _GetILIndexGivenPXIcon(IExtractIcon *pxicon, UINT uFlags, LPCITEMIDLIST pidl, int *piImage, BOOL fAnsiCrossOver);
#endif

HRESULT CStartMenuTask::InternalResumeRT(void)
{
    HRESULT hr = NOERROR;
    LPITEMIDLIST pidl;
    ULONG cElements;
    int iOldThreadPriority;
    HANDLE hThread = GetCurrentThread();

    ASSERT(_pei);

    // Change the thread priority for this task?
    iOldThreadPriority = GetThreadPriority(hThread);
    if (iOldThreadPriority != _iThreadPriority)
    {
        // Yes
        SetThreadPriority(hThread, _iThreadPriority);
        
        DEBUG_CODE( TraceMsg(TF_SMTASK, "StartMenuTask (%#lx): Set thread priority to %x", _dwTaskID, _iThreadPriority); )
    }

    // Enumerate the entire tree
    while (NOERROR == _pei->Next(1, &pidl, &cElements))
    {
#ifdef FULL_DEBUG
        LPITEMIDLIST pidlFull = ILCombine( _pidlFolder, pidl ) ;
        TCHAR szPath[MAX_PATH];
        SHGetPathFromIDList(pidlFull, szPath);
        // TraceMsg(TF_SMTASK, "StartMenuTask (%#lx): caching %s", _dwTaskID, PathFindFileName(szPath));
        DWORD dwCount = GetTickCount();
#endif

        // Enumerate folders.  Don't enumerate folders that behave like
        // shortcuts (channels).
        
        DWORD dwAttrib = SFGAO_FOLDER | SFGAO_LINK | SFGAO_FILESYSTEM;
        _psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidl, &dwAttrib);

        // Is this a subfolder?
        if ((dwAttrib & SFGAO_FOLDER) && 
            (dwAttrib & SFGAO_FILESYSTEM) && 
            (_dwFlags & ITSFT_RECURSE))
        {
            // Yes; is this a channel?
            if (!SHIsExpandableFolder(_psf, pidl))
            {
                // Yes; skip it
#ifdef FULL_DEBUG
                TCHAR szPath[MAX_PATH];
                TraceMsg(TF_SMTASK, "StartMenuTask (%#lx): skipping %s", _dwTaskID, PathFindFileName(szPath));
#endif
                goto MoveOn;
            }

            // is this a Folder Shortcut? if so don't recurse into it.
            if ((dwAttrib & (SFGAO_FOLDER | SFGAO_LINK)) == (SFGAO_FOLDER | SFGAO_LINK))
            {
                goto MoveOn;
            }

            // PERF: We don't want to recurse into the common Admin Tools Folder. This is not on by default
            // and sucks in a bunch of DLLs that we don't need. 

            STRRET str;
            TCHAR szFolder[MAX_PATH];
            if (SUCCEEDED(_psf->GetDisplayNameOf(pidl, SHGDN_FORPARSING, &str)) &&
                SUCCEEDED(StrRetToBuf(&str, pidl, szFolder, ARRAYSIZE(szFolder))))
            {
                if (PathIsEqualOrSubFolder(MAKEINTRESOURCE(CSIDL_COMMON_ADMINTOOLS), szFolder))
                    goto MoveOn;
            }

            // create a task to enumerate the folder
            _CreateNewTask(_psf, pidl);
        }

//        StartCAP();
#ifndef _SM_LOADLINKONCE
        // Register the hotkey
        if (_pshk)
            _pshk->RegisterHotKey(_psf, _pidlFolder, pidl);
        
        // Cache the icon
        if( _nMaxRecursionLevel >= 0 )
            SHMapPIDLToSystemImageListIndex(_psf, pidl, NULL);
#ifdef FULL_DEBUG
        else
        {
            TraceMsg(TF_SMTASK, "StartMenuTask (%#lx): skipping icon cache at recursion level %d for %s", 
                     _dwTaskID, _nMaxRecursionLevel, PathFindFileName(szPath));
        }
#endif

        // if it is a shortcut try to load it
        if (dwAttrib & SFGAO_LINK)
        {
            LPITEMIDLIST pidlFull = ILCombine(_pidlFolder, pidl);
            if (pidlFull)
            {
                // Cache the Darwin info
                ProcessDarwinAd(_psldl, pidlFull);
                ILFree(pidlFull);
            }
        }
//        StopCAP();
#else
//        StartCAP();

        // Is this a shortcut? 
        if (dwAttrib & SFGAO_LINK)
        {
            // Yes

            IShellLink * pLink;
            UINT rgfInOut = 0; 

            HRESULT hres = _psf->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST*)&pidl,
                IID_IShellLink, &rgfInOut, (void**)&pLink); 

            if (SUCCEEDED(hres))
            {
                ASSERT(pLink);

                //
                // Hotkey stuff
                //
                WORD wHotkey;
                pLink->GetHotkey(&wHotkey);

                if (wHotkey)
                {
                    _pshk->RegisterHotKey(NULL, wHotkey, _pidlFolder, pidl);
                }

                //
                // Icon stuff
                //
                IExtractIcon* pxi;
                hres = pLink->QueryInterface(IID_IExtractIcon, (void**)&pxi);
            
                if (SUCCEEDED(hres))
                {
                    ASSERT(pxi);

                    int iUnusedImage;
                    hres = _GetILIndexGivenPXIcon(pxi, 0, pidl, &iUnusedImage, FALSE);

                    pxi->Release();
                }

                //
                // Darwin stuff
                //
                IShellLinkDataList* psldl;
                hres = pLink->QueryInterface(IID_IShellLinkDataList, (void**)&psldl);

                if (SUCCEEDED(hres))
                {
                    ASSERT(psldl);

                    LPITEMIDLIST pidlFull = ILCombine(_pidlFolder, pidl);
                    ProcessDarwinAd(psldl, pidlFull);

                    psldl->Release();
                }

                pLink->Release();

/*                IShellLinkDataList* psldl;
                CDarwinAd* pda = NULL;

                hres = pLink->QueryInterface(IID_IShellLinkDataList, (void**)&psldl);

                if (SUCCEEDED(hres))
                {
                    ASSERT(psldl);

                    EXP_DARWIN_LINK* pexpDarwin = NULL;

                    if (SUCCEEDED(psldl->CopyDataBlock(EXP_DARWIN_ID_SIG, (void**)&pexpDarwin)))
                    {
                        ASSERT(pexpDarwin);

                        LPITEMIDLIST pidlFull = ILCombine(_pidlFolder, pidl);
#ifdef UNICODE
                        pda = new CDarwinAd(pidlFull, pexpDarwin->szwDarwinID);
#else
                        pda = new CDarwinAd(pidlFull, pexpDarwin->szDarwinID);
#endif
                        LocalFree(pexpDarwin);
                    }

                }

                if (pda)
                {
                    // When accessing the global HDPA, get the critical section.
                    ENTERCRITICAL;
                    // Do we have a global cache?
                    if (g_hdpaDarwinAds == NULL)
                    {
                        // No; This is either the first time this is called, or we
                        // failed the last time.
                        g_hdpaDarwinAds = DPA_Create(5);
                    }

                    BOOL fRetVal = FALSE;
                    if (g_hdpaDarwinAds)
                    {
                        // DPA_AppendPtr returns the zero based index it inserted it at.
                        if(DPA_AppendPtr(g_hdpaDarwinAds, (void*)pda) >= 0)
                        {
                            fRetVal = TRUE;
                        }

                    }
                    LEAVECRITICAL;

                    if (!fRetVal)
                    {
                        // if we failed to create a dpa, delete this.
                        delete pda;
                    }
                }*/
            }
        }
        else
        {
            // No it's not a shortcut
            // Cache the icon
            SHMapPIDLToSystemImageListIndex(_psf, pidl, NULL);
        }
//        StopCAP();
#endif

#ifdef FULL_DEBUG
        TraceMsg(TF_SMTASK, "StartMenuTask (%#lx): cached '%s' (time: %ld)", _dwTaskID, PathFindFileName(szPath), GetTickCount()-dwCount);
#endif

MoveOn:
        ILFree(pidl);

        if (WAIT_OBJECT_0 == WaitForSingleObject(_hDone, 0))
        {
            // the _hDone is signaled, so we are either being suspended or killed.
            // TraceMsg(TF_SMTASK, "StartMenuTask (%#lx): ", _dwTaskID, PathFindFileName(szPath), GetTickCount()-dwCount);

            if (_hDone)
                ResetEvent(_hDone);
        
            hr = E_PENDING;
            break;
        }
    }

    if (iOldThreadPriority != _iThreadPriority)
    {
        // bump the thread priority back to what it was
        // before we executed this task
        SetThreadPriority(hThread, iOldThreadPriority);
    }


    if (_lState == IRTIR_TASK_PENDING)
    {
        // We were told to quit, so do it.
        hr = E_FAIL;
    }

    if (_lState != IRTIR_TASK_SUSPENDED)
    {
        // We weren't suspended, so we must be finished
        SUPERCLASS::InternalResumeRT();

#ifdef FULL_DEBUG
        TraceMsg(TF_SMTASK, "StartMenuTask (%#lx): Finished. Total time = %ld", _dwTaskID, GetTickCount()-_dwTaskID);
#endif
    }
    
//    StopCAP();

    return hr;
}


//
// CDesktopTask object.  This task enumerates the desktop for hotkeys 
//


#undef  THISCLASS   
#undef  SUPERCLASS
#define SUPERCLASS  CRunnableTask


class CDesktopTask : public CRunnableTask,
                     public IStartMenuTask
{
public:
    // *** IUnknown Methods ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return SUPERCLASS::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void){ return SUPERCLASS::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj);
    
    // *** IShellFolderTask Methods ***
    virtual STDMETHODIMP InitTaskSFT(IShellFolder *psfParent, LPITEMIDLIST pidlFull, 
                                     LONG nMaxRecursionLevel, DWORD dwFlags, DWORD dwTaskPriority);

    // *** IStartMenuTask Methods ***
    virtual STDMETHODIMP InitTaskSMT(IShellHotKey * pshk, int iThreadPriority);

    // *** CRunnableTask virtual methods
    virtual STDMETHODIMP RunInitRT(void);
    virtual STDMETHODIMP InternalResumeRT(void);

protected:
    friend HRESULT CDesktopTask_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvOut);

    CDesktopTask();
    ~CDesktopTask();

    DWORD           _dwTaskPriority;
    IShellFolder*   _psf;
    LPITEMIDLIST    _pidlFolder;
    IShellHotKey *  _pshk;
    IEnumIDList *   _pei;
};


HRESULT CDesktopTask_CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppvOut)
{
    CDesktopTask* pTask = new CDesktopTask();
    if (pTask)
    {
        HRESULT hres = pTask->QueryInterface(riid, ppvOut);
        pTask->Release();
        return hres;
    }
    return E_OUTOFMEMORY;
}


// constructor
CDesktopTask::CDesktopTask() : CRunnableTask(RTF_SUPPORTKILLSUSPEND)
{
}


// destructor
CDesktopTask::~CDesktopTask()
{
    if (_pidlFolder)
        ILFree(_pidlFolder);
        
    ATOMICRELEASE(_pei);
    ATOMICRELEASE(_psf);
    ATOMICRELEASE(_pshk);
}


STDMETHODIMP CDesktopTask::QueryInterface(REFIID riid, void ** ppvObj)
{
    HRESULT hres;
    static const QITAB qit[] = {
        QITABENT(CDesktopTask, IShellFolderTask),
        QITABENT(CDesktopTask, IStartMenuTask),
        { 0 },
    };

    hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = SUPERCLASS::QueryInterface(riid, ppvObj);
        
    return hres;
}


/*----------------------------------------------------------
Purpose: IShellFolderTask::InitTaskSFT method

*/
STDMETHODIMP CDesktopTask::InitTaskSFT(IShellFolder *psfParent, LPITEMIDLIST pidlFull,
                                       LONG nMaxRecursionLevel, DWORD dwFlags, DWORD dwTaskPriority)
{
    HRESULT hr = E_OUTOFMEMORY;
    IShellFolder* psf;

    if (!psfParent)
        return E_INVALIDARG;

    if (NULL == pidlFull)
    {
        psf = psfParent;
        psf->AddRef();
        hr = S_OK;
    }
    else
    {
        ASSERT(IS_VALID_PIDL(pidlFull));
        
        _pidlFolder = ILClone(pidlFull);
        if (_pidlFolder)
        {
            // we have a pidlChild, so bind to him
            hr = psfParent->BindToObject(ILFindLastID(_pidlFolder), NULL, IID_IShellFolder, (void **)&psf);
        }
    }

    if (SUCCEEDED(hr))
    {
        _psf = psf;
        _dwTaskPriority = dwTaskPriority;

    }
    return hr;
}


/*----------------------------------------------------------
Purpose: IStartMenuTask::InitTaskSMT method

*/
STDMETHODIMP CDesktopTask::InitTaskSMT(IShellHotKey * pshk, int iThreadPriority)
{
    ASSERT(pshk);

    _pshk = pshk;
    if (_pshk)
        _pshk->AddRef();

    return S_OK;
}


/*----------------------------------------------------------
Purpose: CRunnableTask::RunInitRT method

*/
HRESULT CDesktopTask::RunInitRT(void)
{
    // This is the first time that ::Run has been called on this task,
    // so create our pEnumIDList interface.

    ASSERT(IS_VALID_CODE_PTR(_psf, IShellFolder));

    return _psf->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &_pei);
}


/*----------------------------------------------------------
Purpose: CRunnableTask::InternalResumeRT method

*/
HRESULT CDesktopTask::InternalResumeRT(void)
{
    HRESULT hr = NOERROR;
    LPITEMIDLIST pidl;
    ULONG cElements;
    HANDLE hThread = GetCurrentThread();

    ASSERT(_pei);

    // Enumerate the entire tree
    while (NOERROR == _pei->Next(1, &pidl, &cElements))
    {
        // Register the hotkey
        if (_pshk)
#ifndef _SM_LOADLINKONCE
            _pshk->RegisterHotKey(_psf, _pidlFolder, pidl);
#else
            _pshk->RegisterHotKey(_psf, 0, _pidlFolder, pidl);
#endif
        ILFree(pidl);

        if (WAIT_OBJECT_0 == WaitForSingleObject(_hDone, 0))
        {
            // The _hDone is signaled, so we are either being suspended or killed.
            if (_hDone)
                ResetEvent(_hDone);
        
            hr = E_PENDING;
            break;
        }
    }

    if (_lState == IRTIR_TASK_PENDING)
    {
        // We were told to quit, so do it.
        hr = E_FAIL;
    }

    if (_lState != IRTIR_TASK_SUSPENDED)
    {
        // We weren't suspended, so we must be finished
        SUPERCLASS::InternalResumeRT();

#ifdef FULL_DEBUG
        TraceMsg(TF_SMTASK, "DesktopTask (%#lx): Finished. Total time = %ld", _dwTaskID, GetTickCount()-_dwTaskID);
#endif
    }
    
    return hr;
}



// IUnknown
STDMETHODIMP CStartContextMenu::QueryInterface(REFIID riid, void **ppvObj)
{

    static const QITAB qit[] = 
    {
        QITABENT(CStartContextMenu, IContextMenu),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

STDMETHODIMP_(ULONG) CStartContextMenu::AddRef(void)
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CStartContextMenu::Release(void)
{
    ASSERT(_cRef > 0);
    _cRef--;

    if( _cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

// IContextMenu
STDMETHODIMP CStartContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT hres = E_FAIL;
    HMENU hmenuStartMenu = SHLoadMenuPopup(HINST_THISDLL, MENU_STARTMENUSTATICITEMS);
    if (hmenuStartMenu)
    {
        TCHAR szCommon[MAX_PATH];
        BOOL fAddCommon = (S_OK == SHGetFolderPath(NULL, CSIDL_COMMON_STARTMENU, NULL, 0, szCommon));

        if (fAddCommon)
            fAddCommon = IsUserAnAdmin();

        // Since we don't show this on the start button when the user is not an admin, don't show it here... I guess...
        if (_idCmd != IDM_PROGRAMS || !fAddCommon)
        {
            DeleteMenu(hmenuStartMenu, SMCM_OPEN_ALLUSERS, MF_BYCOMMAND);
            DeleteMenu(hmenuStartMenu, SMCM_EXPLORE_ALLUSERS, MF_BYCOMMAND);
        }

        if (Shell_MergeMenus(hmenu, hmenuStartMenu, 0, indexMenu, idCmdLast, uFlags))
        {
            SetMenuDefaultItem(hmenu, 0, MF_BYPOSITION);
            _SHPrettyMenu(hmenu);
            hres = ResultFromShort(GetMenuItemCount(hmenuStartMenu));
        }

        DestroyMenu(hmenuStartMenu);
    }
    
    return hres;
}

STDMETHODIMP CStartContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
    HRESULT hres = E_FAIL;
    if (HIWORD64(lpici->lpVerb) == 0)
    {
        BOOL fAllUsers = FALSE;
        BOOL fOpen = TRUE;
        switch(LOWORD(lpici->lpVerb))
        {
        case SMCM_OPEN_ALLUSERS:
            fAllUsers = TRUE;
        case SMCM_OPEN:
            // fOpen = TRUE;
            break;

        case SMCM_EXPLORE_ALLUSERS:
            fAllUsers = TRUE;
        case SMCM_EXPLORE:
            fOpen = FALSE;
            break;

        default:
            return S_FALSE;
        }

        hres = ExecStaticStartMenuItem(_idCmd, fAllUsers, fOpen);
    }

    // Ahhh Don't handle verbs!!!
    return hres;

}

STDMETHODIMP CStartContextMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pRes, LPSTR pszName, UINT cchMax)
{
    return E_NOTIMPL;
}

