// NOTE! Shdoc401 issue: Start | Find needs to check what version of shell32 user is running 
//                       because of the way we pack icons in menu item info
//                       that code is under #ifdef SHDOC401_DLL ... #endif (reljai)

#include "stdafx.h"
#pragma hdrstop
//#include "resource.h"
//#include "ids.h"
#include "runtask.h"
//#include <shguidp.h>
//#include "shellp.h"
//#include "shdguid.h"
//#include "startids.h"
//#include "cowsite.h"
#ifdef SHDOC401_UEM
//#include "uemapp.h"
#endif
//#include "clsobj.h"
#ifdef SHDOC401_DLL
//#include "options.h"
#endif

#include <mluisupp.h>

// {DDB008FF-048D-11d1-B9CD-00C04FC2C1D2}
const IID IID_IOldShellFolderTask = {0xddb008ff,0x048d,0x11d1,{0xb9,0xcd,0x0,0xc0,0x4f,0xc2,0xc1,0xd2}};

// {0C55899C-1C3E-11d1-9829-00C04FD91972}
const IID IID_IOldStartMenuTask = {0xc55899c,0x1c3e,0x11d1,{0x98,0x29,0x0,0xc0,0x4f,0xd9,0x19,0x72}};


#define WINUPDATEKEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WindowsUpdate")

#define TF_MENUBAND         0x00002000      // Menu band messages

// CLSID copied from the Network Connections folder.
const CLSID CLSID_ConnectionFolder = {0x7007ACC7,0x3202,0x11D1,{0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E}};


BOOL AreIntelliMenusEnbaled()
{
#define REG_EXPLORER_ADVANCED TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced")

    BOOL fIntelliMenus;

    DWORD dwRest = SHRestricted(REST_INTELLIMENUS);

    if (dwRest != RESTOPT_INTELLIMENUS_USER)
    {
        fIntelliMenus = (dwRest == RESTOPT_INTELLIMENUS_ENABLED);
    }
    else
    {
        fIntelliMenus = SHRegGetBoolUSValue(REG_EXPLORER_ADVANCED, TEXT("IntelliMenus"),
                            FALSE, // Don't ignore HKCU
                            TRUE); // Enable Menus by default
    }

    return fIntelliMenus;
}



// IMenuBandCallback implementation
class CStartMenuCallback : public IShellMenuCallback,
                           public CObjectWithSite
{
public:
    // *** IUnknown methods ***
    STDMETHODIMP QueryInterface (REFIID riid, LPVOID * ppvObj);
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
    IContextMenu*   _pcmFind;
    TCHAR           _szPrograms[MAX_PATH];
    ITrayPriv*      _ptp;
    IOleCommandTarget* _poct;
    IUnknown*       _punkSite;
    BITBOOL         _fFirstTime: 1;
    BITBOOL         _fExpandoMenus: 1;
    BITBOOL         _fSuspend: 1;
    BITBOOL         _fEject: 1;
    BITBOOL         _fFindMenuInvalid;
    BITBOOL         _fScrollPrograms;


    HWND            _hwnd;

    HRESULT _ExecHmenuItem(HMENU hMenu, UINT uId);
    HRESULT _Init(UINT uIdParent, HMENU hMenu, UINT uId, IUnknown* punk);
    HRESULT _GetHmenuInfo(HMENU hMenu, UINT uId, SMINFO*sminfo);
    HRESULT _GetSFInfo(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem, SMINFO* sminfo);
    HRESULT _GetObject(LPSMDATA psmd, REFIID riid, void** ppvObj);
    HRESULT _CheckRestricted(DWORD dwRestrict, BOOL* fRestricted);
    HRESULT _FilterPidl(IShellFolder* psf, LPCITEMIDLIST pidl);
    HRESULT _FilterFavorites(IShellFolder* psf, LPCITEMIDLIST pidl);
    void _OnExecItemCB(LPSMDATA psmd, UINT uMsg);
    HRESULT _GetDefaultIcon(TCHAR* psz, int* piIndex);

    // helper functions
    BOOL _IsAvailable(int idCmd);
    void _RefreshItem(HMENU hmenu, int idCmd, IShellMenu* psm);
    void _SetStartMenu(IUnknown* punk);

};

CStartMenuCallback::CStartMenuCallback() : _cRef(1)
{
    SHGetSpecialFolderPath(NULL, _szPrograms, CSIDL_PROGRAMS, FALSE);
    PathStripPath(_szPrograms);
    _fFirstTime = TRUE;
    _fFindMenuInvalid = TRUE;
}

CStartMenuCallback::~CStartMenuCallback()
{
    ATOMICRELEASE(_pcmFind);
    ATOMICRELEASE(_ptp);
    ATOMICRELEASE(_poct);
}

// *** IUnknown methods ***
STDMETHODIMP CStartMenuCallback::QueryInterface(REFIID riid, LPVOID * ppvObj)
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

#if defined(SHDOC401_UEM) && defined(DEBUG)
void DBUEMGetInfo(const IID *pguidGrp, int eCmd, WPARAM wParam, LPARAM lParam)
{
#if 1
    return;
#else
    UEMINFO uei;

    uei.cbSize = SIZEOF(uei);
    uei.dwMask = ~0;    // UEIM_HIT etc.
    UEMGetInfo(pguidGrp, eCmd, wParam, lParam, &uei);

    TCHAR szBuf[20];
    wsprintf(szBuf, TEXT("hit=%d"), uei.cHit);
    MessageBox(NULL, szBuf, TEXT("UEM"), MB_OKCANCEL);

    return;
#endif
}
#endif

STDMETHODIMP CStartMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = S_FALSE;

    switch (uMsg)
    {
    case SMC_CREATE:
        // We need to reset this because when the menu is created, then
        // the item can't be removed. This bug was found in the favorites menu
        // I'm adding it here, because I'm paranoid.
        _fEject = TRUE; 
        _fSuspend = TRUE;
        _fScrollPrograms =  SHRegGetBoolUSValue(REG_EXPLORER_ADVANCED, TEXT("StartMenuScrollPrograms"),
                            FALSE, // Don't ignore HKCU
                            TRUE); // Enable Scrolling
        break;

    case SMC_INITMENU:
        _Init(psmd->uIdParent, psmd->hmenu, psmd->uId, psmd->punk);
        break;

#ifdef SMC_POPUP
    case SMC_POPUP:
        break;
#endif

    case SMC_SFEXEC:
        _OnExecItemCB(psmd, uMsg);
        hres = _ptp->ExecItem(psmd->psf, psmd->pidlItem);
        break;

    case SMC_EXEC:
        _OnExecItemCB(psmd, uMsg);
        hres = _ExecHmenuItem(psmd->hmenu, psmd->uId);
        break;

    case SMC_GETOBJECT:
        hres = _GetObject(psmd, (GUID)*((GUID*)wParam), (void**)lParam);
        break;

    case SMC_GETINFO:
        hres = _GetHmenuInfo(psmd->hmenu, psmd->uId, (SMINFO*)lParam);
        break;

    case SMC_GETSFINFO:
        ASSERT(psmd->dwMask & SMDM_SHELLFOLDER);

        // We don't want to do anything at the root of start menu.
        if (psmd->uIdParent != 0)
        {   
            hres = _GetSFInfo(psmd->pidlFolder, psmd->pidlItem, (SMINFO*)lParam);
        }
        break;

    case SMC_BEGINENUM:
        if (psmd->uIdParent == 0 &&
            SHRestricted(REST_NOSTARTMENUSUBFOLDERS))
        {
            ASSERT(wParam != NULL);
            *(DWORD*)wParam = SHCONTF_NONFOLDERS;
            hres = S_OK;
        }
        break;

    case SMC_FILTERPIDL:
        ASSERT(psmd->dwMask & SMDM_SHELLFOLDER);

        if (psmd->uIdParent == 0)
        {   
            // I only care about top level
            hres = _FilterPidl(psmd->psf, psmd->pidlItem);
        }
        else if (psmd->uIdAncestor == IDM_FAVORITES)
        {
            hres = _FilterFavorites(psmd->psf, psmd->pidlItem);
        }
        break;

    case SMC_INSERTINDEX:
        ASSERT(lParam && IS_VALID_WRITE_PTR(lParam, int));
        *((int*)lParam) = 0;
        hres = S_OK;
        break;

    case SMC_REFRESH:
        _SetStartMenu(psmd->punk);
        _fFindMenuInvalid = TRUE;
        _fScrollPrograms =  SHRegGetBoolUSValue(REG_EXPLORER_ADVANCED, TEXT("StartMenuScrollPrograms"),
                            FALSE, // Don't ignore HKCU
                            TRUE); // Enable Scrolling
        break; 

    case SMC_DEFAULTICON:
        ASSERT(psmd->uIdAncestor == IDM_FAVORITES); // This is only valid for the Favorites menu
        hres = _GetDefaultIcon((LPTSTR)wParam, (int*)lParam);
        break;
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


//***   CStartMenuCallback::_OnExecItemCB -- log menu invokation
// DESCRIPTION
//  iterate up menu 'tree' from self up to root, logging each guy
// NOTES
//  n.b. this assumes the tracked site chain is still valid.  currently
// mn*.cpp!CMenuSFToolbar::v_Show ensures this.  see comments there.
//  BUGBUG perf: if logging is off, we do the walk anyway
// FEATURE_UASSIST
void CStartMenuCallback::_OnExecItemCB(/*const*/LPSMDATA psmd, UINT uMsg) 
{
    HRESULT hr;
    LPITEMIDLIST pidl;
    SMDATA smd = *psmd;     // gotta make copy since caller assumes const
    BOOL fDestroy = FALSE;  // free up child (all but passed-in smd)
    IShellMenu *psm;

    hr = S_OK;
    // we'll terminate after root *or* when somebody's GetState fails
    while (SUCCEEDED(hr)) {
        //
        // party on current band
        //
        switch (smd.dwMask) {
        case SMDM_SHELLFOLDER:
            pidl = ILCombine(smd.pidlFolder, smd.pidlItem);
            if (pidl) {
#ifdef SHDOC401_UEM
#ifdef DEBUG
                if (!fDestroy)  // hack: leaf
                    DBUEMGetInfo(&UEMIID_SHELL, UEME_RUNPIDL, (WPARAM)CSIDL_PROGRAMS, (LPARAM)pidl);
#endif
                // BUGBUG CSIDL_PROGRAMS isn't always right (e.g. Docs, Favs)
                // eventually use Exec(XXXID_PRIVATEID) to get real one
                UEMFireEvent(&UEMIID_SHELL, UEME_RUNPIDL, (WPARAM)CSIDL_PROGRAMS, (LPARAM)pidl);
#endif // SHDOC401_UEM
                if ( smd.punk && 
                     SUCCEEDED(smd.punk->QueryInterface(IID_IShellMenu, (void**)&psm)) )
                {
                    psm->InvalidateItem(&smd, SMINV_REFRESH);
                    psm->Release();
                }

                ILFree(pidl);
            }
            break;
#ifdef SHDOC401_UEM
        case SMDM_HMENU:
#ifdef DEBUG
            if (!fDestroy)  // hack: leaf
                DBUEMGetInfo(&UEMIID_SHELL, UEME_RUNWMCMD, (WPARAM)-1, (LPARAM)smd.uId);
#endif
            UEMFireEvent(&UEMIID_SHELL, UEME_RUNWMCMD, -1, (LPARAM)smd.uId);
            break;
#endif
        default:
            break;
        }

        //
        // move on to parent
        // smd.punk is band; GetSite is menusite; SID_SMenuBandParent is bar
        //

        IServiceProvider *psite;

        hr = IUnknown_GetSite(smd.punk, IID_IServiceProvider, (void **)&psite);

        if (fDestroy) {
            // free up child
            switch (smd.dwMask) {
            case SMDM_SHELLFOLDER:
                ASSERT(smd.psf);
                ATOMICRELEASE(smd.psf);
                ASSERT(smd.pidlFolder);
                ILFree(smd.pidlFolder);
                ASSERT(smd.pidlItem);
                ILFree(smd.pidlItem);
                break;
#ifdef DEBUG
            case SMDM_HMENU:
                /*NOTHING*/
                break;
#endif
            }
            smd.punk->Release();
        }
        fDestroy = TRUE;    // all parents need to be freed 

        if (SUCCEEDED(hr)) {

            hr = psite->QueryService(SID_SMenuBandParent, IID_IShellMenu, (void **)&psm);
            if (SUCCEEDED(hr)) {
                hr = psm->GetState(&smd);
                ASSERT(SUCCEEDED(hr));
                psm->Release();
            }

            psite->Release();
        }
    }

    return;
}


void ShowFolder(HWND hwnd, UINT csidl, UINT uFlags)
{
    SHELLEXECUTEINFO shei = { 0 };

    shei.cbSize     = sizeof(shei);
    shei.fMask      = SEE_MASK_IDLIST;
    shei.nShow      = SW_SHOWNORMAL;
    shei.lpVerb     = TEXT("open");

    if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, csidl, (LPITEMIDLIST*)&shei.lpIDList)))
    {
        ShellExecuteEx(&shei);
        ILFree((LPITEMIDLIST)shei.lpIDList);
    }
}

BOOL IsActiveDesktopEnabled()
{
    HRESULT hres;
    IActiveDesktop* pad;
    COMPONENTSOPT co = { 0 };
    ASSERT(co.fActiveDesktop == FALSE);

    co.dwSize = sizeof(COMPONENTSOPT);

    hres = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER,
        IID_IActiveDesktop, (void**)&pad);
    if(SUCCEEDED(hres))
    {
        pad->GetDesktopItemOptions(&co, NULL);
        pad->Release();
    }

    return co.fActiveDesktop;
}

BOOL UpdateAllDesktopSubscriptions();
DWORD CALLBACK ActiveDesktopThread(LPVOID dwID)
{
    HRESULT hrInit = SHCoInitialize();

    switch((DWORD)dwID)
    {
    case IDM_DESKTOPHTML_ONOFF:
        {
            IActiveDesktop* pad;
            COMPONENTSOPT co;
            HRESULT hres;
            co.dwSize = sizeof(COMPONENTSOPT);

            hres = CoCreateInstance(CLSID_ActiveDesktop, NULL, CLSCTX_INPROC_SERVER,
                IID_IActiveDesktop, (void**)&pad);
            if(SUCCEEDED(hres))
            {
                pad->GetDesktopItemOptions(&co, NULL);
                co.fActiveDesktop = co.fActiveDesktop? FALSE : TRUE;
                pad->SetDesktopItemOptions(&co, NULL);

                pad->ApplyChanges(AD_APPLY_REFRESH);
                pad->Release();
            }
            break;
        }
    case IDM_DESKTOPHTML_UPDATE:
        UpdateAllDesktopSubscriptions();
        break;
    case IDM_DESKTOPHTML_CUSTOMIZE:
        {
            TCHAR szDesktopWeb[MAX_PATH];
            MLLoadString(IDS_DESKTOPWEBSETTINGS, szDesktopWeb, ARRAYSIZE(szDesktopWeb));
            SHRunControlPanel(szDesktopWeb, NULL);
            break;
        }
    }
    SHCoUninitialize(hrInit);

    return 0;
}


HRESULT CStartMenuCallback::_ExecHmenuItem(HMENU hMenu, UINT uId)
{
    HRESULT hres = S_FALSE;
    HANDLE  hThread;
    DWORD   dwThread;
    if (IsInRange(uId, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST) && _pcmFind)
    {
        CMINVOKECOMMANDINFOEX ici = { 0 };
        ici.cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);
        ici.lpVerb = (LPSTR)MAKEINTRESOURCE(uId - TRAY_IDM_FINDFIRST);
        ici.nShow = SW_NORMAL;

        _pcmFind->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
        hres = S_OK;
    }
    else
    {
        switch (uId)
        {
#ifdef SHDOC401_DLL
        case IDM_FOLDERPROPERTIES:
            DoGlobalFolderOptions();
            break;

        case IDM_DESKTOPHTML_ONOFF:
        case IDM_DESKTOPHTML_CUSTOMIZE:
        case IDM_DESKTOPHTML_UPDATE:
            hThread = CreateThread(NULL, 0, ActiveDesktopThread, (LPVOID)uId, 
                THREAD_PRIORITY_NORMAL, &dwThread);
            if(hThread != NULL)
                CloseHandle(hThread);
            hres = NOERROR;
            break;
#endif

        case IDM_MYDOCUMENTS:
            ShowFolder(NULL, CSIDL_PERSONAL, 0);
            hres = NOERROR;
            break;
        case IDM_UPDATEWIZARD:
            {
                TCHAR szUpdateUrl[MAX_PATH];
                DWORD cbSize = ARRAYSIZE(szUpdateUrl);
                if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, WINUPDATEKEY, TEXT("UpdateURL"), 
                    NULL, szUpdateUrl, &cbSize))
                {
                    SHELLEXECUTEINFO shei= { 0 };
                    shei.cbSize = sizeof(shei);
                    shei.nShow  = SW_SHOWNORMAL;
                    shei.lpParameters = PathGetArgs(szUpdateUrl);
                    PathRemoveArgs(szUpdateUrl);
                    shei.lpFile = szUpdateUrl;
                    ShellExecuteEx(&shei);
                }
            }
            break;
        }
    }

    return hres;
}

BOOL CStartMenuCallback::_IsAvailable(int idCmd)
{
    VARIANT varRet;
    VariantInit(&varRet);

    if (EVAL(_poct) && 
        SUCCEEDED(_poct->Exec(&CGID_MenuBandItem, MBICMDID_IsVisible, 
            idCmd, NULL, &varRet)))
    {
        // Is it not visible?
        if (varRet.vt == VT_BOOL && varRet.boolVal == VARIANT_FALSE)
        {
            // Yes; Then it's not available
            return FALSE;
        }
    }


    // This item is available
    return TRUE;
}

void CStartMenuCallback::_RefreshItem(HMENU hmenu, int idCmd, IShellMenu* psm)
{
    SMDATA smd;
    smd.dwMask = SMDM_HMENU;
    smd.hmenu = hmenu;
    smd.uId = idCmd;

    psm->InvalidateItem(&smd, SMINV_ID | SMINV_REFRESH);
}


void CStartMenuCallback::_SetStartMenu(IUnknown* punk)
{
    IShellMenu* psm;
    HRESULT hres = punk->QueryInterface(IID_IShellMenu, (void**)&psm);
    if (SUCCEEDED(hres))
    {
        HMENU hmenu;
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
                    _ptp->GetStaticStartMenu(&hmenu);
                    if (SUCCEEDED(_ptp->QueryInterface(IID_IOleWindow, (void**)&pow)))
                    {
                        pow->GetWindow(&_hwnd);
                        pow->Release();
                    }

                    _ptp->QueryInterface(IID_IOleCommandTarget, (void**)&_poct);
                }
                else
                    TraceMsg(TF_MENUBAND, "CStartMenuCallback::_SetSite : Failed to aquire CStartMenuHost");
                pows->Release();
            }
            pmp->Release();
        }

        _fSuspend = _IsAvailable(IDM_SUSPEND);
        _fEject = _IsAvailable(IDM_EJECTPC);

        // If Eject PC is not available, we need to remove it
        if (!_fEject)
            DeleteMenu(hmenu, IDM_EJECTPC, MF_BYCOMMAND);

        // If Suspend is not available, we need to remove it
        if (!_fSuspend)
            DeleteMenu(hmenu, IDM_SUSPEND, MF_BYCOMMAND);

        if (hmenu)
        {
            psm->SetMenu(hmenu, _hwnd, SMSET_BOTTOM);
            TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : SetMenu(HMENU 0x%x, HWND 0x%x", hmenu, _hwnd);
        }
        psm->Release();
    }
}


HRESULT CStartMenuCallback::_Init(UINT uIdParent, HMENU hMenu, UINT uId, IUnknown* punk)
{
    if ((uIdParent != 0 && uIdParent != IDM_ACTIVEDESKTOP_PROP) || !punk)
        return S_FALSE;

    IShellMenu* psm;
    HRESULT hres = punk->QueryInterface(IID_IShellMenu, (void**)&psm);

    if (FAILED(hres))
        return hres;


    if (uIdParent == 0)
    {
        TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : Initializing Toplevel Start Menu");
        if (_fFirstTime)
        {
            _fFirstTime = FALSE;
            TraceMsg(TF_MENUBAND, "CStartMenuCallback::_Init : First Time, and correct parameters");

            _SetStartMenu(punk);
        }
        else
        {
            BOOL fSuspend = _IsAvailable(IDM_SUSPEND);
            BOOL fEject = _IsAvailable(IDM_EJECTPC);

            if ((_fSuspend ^ fSuspend) ||
                (_fEject ^ fEject))
            {
                psm->InvalidateItem(NULL, SMINV_REFRESH);

                _fSuspend = fSuspend;
                _fEject = fEject;
            }
        }

    }

    if (uIdParent == IDM_ACTIVEDESKTOP_PROP)
    {
        ASSERT(hMenu);

        DWORD dwFlags = MF_BYCOMMAND;
        if (IsActiveDesktopEnabled())
        {
            dwFlags |= MF_CHECKED;
        }
        else
        {
            dwFlags |= MF_UNCHECKED;
        }

        // Refresh only if we were able to check it.  Refreshing what
        // doesn't exist creates duplicate menu items which suck.
        if (EVAL(CheckMenuItem(hMenu, IDM_DESKTOPHTML_ONOFF, dwFlags) != 0xFFFFFFFF))
        {
            _RefreshItem(hMenu, IDM_DESKTOPHTML_ONOFF, psm);
        }
    }


    psm->Release();

    return NOERROR;
}


typedef struct _SEARCHEXTDATA
{
    WCHAR wszMenuText[MAX_PATH];
    WCHAR wszHelpText[MAX_PATH];
    int   iIcon;
} SEARCHEXTDATA, FAR* LPSEARCHEXTDATA;


HRESULT CStartMenuCallback::_GetHmenuInfo(HMENU hMenu, UINT uId, SMINFO* psminfo)
{
    const static struct 
    {
        UINT idCmd;
        int  iImage;
    } s_mpcmdimg[] = { // Top level menu
                       { IDM_PROGRAMS,       II_STPROGS },
#ifdef IDM_FAVORITES
                       { IDM_FAVORITES,      II_STFAVORITES },
#endif
                       { IDM_RECENT,         II_STDOCS },
                       { IDM_SETTINGS,       II_STSETNGS },
                       { IDM_MENU_FIND,      II_STFIND },
#ifdef FEATURE_BROWSEWEB
                       { IDM_MENU_WEB,       -1 },
#endif
                       { IDM_HELPSEARCH,     II_STHELP },
                       { IDM_FILERUN,        II_STRUN },
                       { IDM_LOGOFF,         II_STLOGOFF },
                       { IDM_EJECTPC,        II_STEJECT },
                       { IDM_EXITWIN,        II_STSHUTD },
#ifdef _HYDRA_      
                       { IDM_MU_SECURITY,    II_MU_STSECURITY },
                       { IDM_MU_DISCONNECT,  II_MU_STDISCONN  },
#endif

                       // Settings Submenu
                       { IDM_SETTINGSASSIST, II_STSETNGS },
                       { IDM_CONTROLS,       II_STCPANEL },
                       { IDM_PRINTERS,       II_STPRNTRS },
                       { IDM_TRAYPROPERTIES, II_STTASKBR },
                       { IDM_MYDOCUMENTS,    II_STDOCS},
                       { IDM_UPDATEWIZARD,   II_WINUPDATE},
#ifdef SHDOC401_DLL
                       { IDM_SUSPEND,        II_STSUSPEND },
                       { IDM_FOLDERPROPERTIES, II_STFLDRPROP},
                       { IDM_ACTIVEDESKTOP_PROP, II_DESKTOP},
#endif
#ifndef SHDOC401_DLL
                       { IDM_NETCONNECT,     -IDI_NETCONNECT},
#endif
                       };

    ASSERT(IS_VALID_WRITE_PTR(psminfo, SMINFO));

    int iIcon = -1;
    DWORD dwFlags = psminfo->dwFlags;
    MENUITEMINFO mii = {0};
    HRESULT hres = S_FALSE;

    if (psminfo->dwMask & SMIM_ICON)
    {
        if (IsInRange(uId, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST))
        {
            // The find menu extensions pack their icon into their data member of
            // Menuiteminfo....
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_DATA;
            if (GetMenuItemInfo(hMenu, uId, MF_BYCOMMAND, &mii))
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
            for (int i = 0; i < ARRAYSIZE(s_mpcmdimg); i++)
            {
                if (s_mpcmdimg[i].idCmd == uId)
                {
                    iIcon = s_mpcmdimg[i].iImage;
                    break;
                }
            }

            if(iIcon != -1)
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
        switch (uId)
        {
        case IDM_PROGRAMS:
#ifdef IDM_FAVORITES
        case IDM_FAVORITES:
#endif
            psminfo->dwFlags |= SMIF_DROPCASCADE;
            hres = S_OK;
            break;

#ifdef IDM_ACTIVEDESKTOP_PROP
        case IDM_ACTIVEDESKTOP_PROP:
            psminfo->dwFlags |= SMIF_SUBMENU;
            hres = S_OK;
            break;
#endif

#ifndef SHDOC401_DLL
        case IDM_NETCONNECT:
            // Put this here, because when there are no connections,
            // This defaults to opening the connections folder.
            psminfo->dwFlags |= SMIF_SUBMENU;
            break;
#endif

        }
    }

    return hres;
}

#ifdef SHDOC401_UEM

// BUGBUG (lamadio): This is a copy from browseui/sftbar.cpp
DWORD _GetDemote(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem)
{
    LPITEMIDLIST pidlFull;
    UEMINFO uei;
    DWORD dwFlags;

    dwFlags = 0;

    if (pidlFolder && pidlItem)
    {
        pidlFull = ILCombine(pidlFolder, pidlItem);
        if (pidlFull) {
            uei.cbSize = SIZEOF(uei);
            uei.dwMask = UEIM_HIT;
            if (SUCCEEDED(UEMQueryEvent(&UEMIID_SHELL, UEME_RUNPIDL, (WPARAM)CSIDL_PROGRAMS, (LPARAM)pidlFull, &uei))) {
                if (uei.cHit == 0) {
                    dwFlags |= SMIF_DEMOTED;
                }
            }
#if 0   
            if (!(dwFlags & SMIF_DEMOTED))
                TraceMsg(DM_TRACE, "bui._gd: !SMIF_DEMOTED pidl=%s", DBPidlToTstr(pidlFull));
#endif
            ILFree(pidlFull);
        }
    }

    return dwFlags;
}
#endif // SHDOC401_UEM

HRESULT CStartMenuCallback::_GetSFInfo(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidlItem, SMINFO* psminfo)
{
#ifdef SHDOC401_UEM
    if ((psminfo->dwMask & SMIM_FLAGS) &&
        _fExpandoMenus)
    {
        psminfo->dwFlags |= _GetDemote(pidlFolder, pidlItem);
    }
#endif
    return S_OK;
}

HRESULT CreateNetworkConnections(IShellMenu* psm, HKEY hKeyRoot)
{
    IShellFolder* psfFolder;
    HRESULT hres = CoCreateInstance (CLSID_ConnectionFolder, NULL, CLSCTX_INPROC, 
            IID_IShellFolder, (LPVOID *)&psfFolder);

    if (SUCCEEDED(hres))
    {
        // We now need to generate a pidl for this item.
        // To do this, we create a string that contains ::<CLSID>
        // then pass it through MyComputer::ParseDisplayName.
        // This generates a pidl to the parent.
        DWORD dwEaten;
        IShellFolder* psfMyComputer;
        LPITEMIDLIST pidlMyComputer;
        LPITEMIDLIST pidl;

        WCHAR wszRegItem[GUIDSTR_MAX + 2] = L"::";
        SHStringFromGUIDW(CLSID_ConnectionFolder, (WCHAR*)(wszRegItem + 2), GUIDSTR_MAX); 

        hres = SHGetSpecialFolderLocation(NULL, CSIDL_DRIVES, &pidlMyComputer);
        if (SUCCEEDED(hres))
        {
            hres = SHBindToObject(NULL, IID_IShellFolder, 
                pidlMyComputer, (void**)&psfMyComputer);
            if (SUCCEEDED(hres))
            {
                hres = psfMyComputer->ParseDisplayName(NULL, NULL, wszRegItem, 
                    &dwEaten, &pidl, NULL);

                if (SUCCEEDED(hres))
                {
                    HKEY hKey = NULL;
                    DWORD dwDisp;
                    if (hKeyRoot)
                    {
                        // It's okay if this fails.  We will pass in a null hkey, which simply
                        // means the user's order preference isn't shown.  But at least the
                        // startmenu still appears!
                        RegCreateKeyEx(hKeyRoot, TEXT("Network Connections"), NULL, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                            NULL, &hKey, &dwDisp);
                    }

                    hres = psm->SetShellFolder(psfFolder, pidl, hKey, SMSET_TOP);

                    ILFree(pidl);
                }
                
                psfMyComputer->Release();
            }

            ILFree(pidlMyComputer);
        }

        psfFolder->Release();
    }

    return hres;
}

#define SZ_REGKEY_MYDOCUMENTS TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\CLSID\\{450D8FBA-AD25-11D0-98A8-0800361B1103}")


HRESULT CStartMenuCallback::_GetObject(LPSMDATA psmd, REFIID riid, void** ppvOut)
{
    HRESULT hres = S_FALSE;
    HKEY    hKeyRoot = NULL;
    UINT    uId = psmd->uId;

    ASSERT(ppvOut);
    ASSERT(IS_VALID_READ_PTR(psmd, SMDATA));

    *ppvOut = NULL;

    // BUGBUG: investigate why this can't return E_FAIL legitimately (rather than S_FALSE).

    if (IsEqualGUID(riid, IID_IShellMenu))
    {
        LPITEMIDLIST   pidl2 = NULL;
        LPITEMIDLIST   pidl = NULL;
        IShellFolder*  psfFolder;
        IShellMenu*    psm;
        MENUITEMINFO   mii;
        HKEY           hKey = NULL;
        DWORD          dwDisp;
        DWORD          dwSetFolderFlags = 0;

        LPTSTR         pszKeyName = NULL;


        // It's okay if this fails.  We will pass in a null hkey, which simply
        // means the user's order preference isn't shown.  But at least the
        // startmenu still appears!
        RegCreateKeyEx(HKEY_CURRENT_USER, STRREG_STARTMENU, NULL, NULL,
            REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
            NULL, &hKeyRoot, &dwDisp);

        hres = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC, IID_IShellMenu, (void**)&psm);

        if (SUCCEEDED(hres))
        {
            DWORD dwInitFlags = SMINIT_VERTICAL | SMINIT_LEGACYMENU;

            if (SHRestricted(REST_NOCHANGESTARMENU))
                dwInitFlags |= SMINIT_RESTRICT_DRAGDROP | SMINIT_RESTRICT_CONTEXTMENU;

            if (uId == IDM_FAVORITES)
                dwInitFlags &= ~SMINIT_LEGACYMENU;

            if (uId == IDM_PROGRAMS && !_fScrollPrograms)
                dwInitFlags |= SMINIT_MULTICOLUMN;

            psm->Initialize(this, uId, uId, dwInitFlags);

            switch(uId)
            {
            case IDM_PROGRAMS:
                pszKeyName = TEXT("&Programs");
                SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidl);
                
#ifdef WINNT
                // The All-Users folder is only available on NT.
                if (!SHRestricted(REST_NOCOMMONGROUPS))
                {
                    if (SUCCEEDED(hres = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_PROGRAMS, &pidl2)))
                    {
                        IAugmentedShellFolder* pasf;
                        IShellFolder*   psfCommon;
                        hres = CoCreateInstance(CLSID_AugmentedShellFolder, NULL, CLSCTX_INPROC, 
                                                IID_IAugmentedShellFolder, (LPVOID *)&pasf);
                        if (SUCCEEDED(hres))
                        {
                            if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidl, (LPVOID*)&psfFolder)))
                            {
                                pasf->AddNameSpace(NULL, psfFolder, pidl, ASFF_DEFAULT);
                                psfFolder->Release();
                            }

                            if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidl2, (LPVOID*)&psfCommon)))
                            {
                                pasf->AddNameSpace(NULL, psfCommon, pidl2, ASFF_DEFAULT);
                                psfCommon->Release();
                            }

                            if (hKeyRoot)
                            {
                                // It's okay if this fails.  We will pass in a null hkey, which simply
                                // means the user's order preference isn't shown.  But at least the
                                // startmenu still appears!
                                RegCreateKeyEx(hKeyRoot, pszKeyName, NULL, NULL,
                                    REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                    NULL, &hKey, &dwDisp);
                            }

                            hres = psm->SetShellFolder(pasf, pidl, hKey, SMSET_TOP);
                            pasf->Release();
                        }

                        if (FAILED(hres))
                        {
                            psm->Release();
                            psm = NULL;
                        }

                        *ppvOut = psm;

                        ILFree(pidl);                        
                        ILFree(pidl2);
                        goto Cleanup;
                    }
                    // if CSIDL_COMMON_PROGRAMS failed, we're probably running
                    // on a down-level shell32, so fall through and
                    // do it the old Win95 way
                }
#endif
                break;

#ifdef IDM_FAVORITES
            case IDM_FAVORITES:
                pszKeyName = TEXT("Favorites");
                // Favorites menu may have Channels, so we need to do the HasExpandable and
                // the icons are REALLY expensive, so use the Background icon extraction.
                dwSetFolderFlags = SMSET_HASEXPANDABLEFOLDERS | SMSET_USEBKICONEXTRACTION; 
                RegCreateKeyEx(HKEY_CURRENT_USER, STRREG_FAVORITES, NULL, NULL,
                    REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                    NULL, &hKey, &dwDisp);

                SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidl);
                break;
#endif

            case IDM_RECENT:
                pszKeyName = TEXT("Documents");
                SHGetSpecialFolderLocation(NULL, CSIDL_RECENT, &pidl);
                SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl2);
                if (pidl2 != NULL)
                {   
                    HMENU hMenu = SHLoadMenuPopup(MLGetHinst(), MENU_STARTMENU_MYDOCS);
                    if (EVAL(hMenu))
                    {
                        TCHAR szDisplay[MAX_PATH] = TEXT("");
                        DWORD dwType;
                        DWORD cbSize = sizeof(szDisplay);
                        SHGetValue(HKEY_CURRENT_USER, SZ_REGKEY_MYDOCUMENTS, TEXT(""), &dwType,
                             szDisplay, &cbSize);
                        if (szDisplay[0])
                        {
                            MENUITEMINFO mii;
    
                            mii.cbSize = sizeof(MENUITEMINFO);
                            mii.dwTypeData = szDisplay; 
                            mii.fMask = MIIM_TYPE | MIIM_ID;
                            mii.cch = cbSize / sizeof(TCHAR) + 1;
                            mii.fType = MFT_STRING;
                            mii.wID = IDM_MYDOCUMENTS;
                            SetMenuItemInfo(hMenu, IDM_MYDOCUMENTS, MF_BYCOMMAND, &mii);
                        }

                        psm->SetMenu(hMenu, _hwnd, SMSET_TOP);


                        IShellFolder*   psf;
                        if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidl, (LPVOID*)&psf)))
                        {
                            RegCreateKeyEx(hKeyRoot, pszKeyName, NULL, NULL,
                                REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                NULL, &hKey, &dwDisp);
                            psm->SetShellFolder(psf, pidl, hKey, SMSET_BOTTOM);
                            psf->Release();
                        }

                        *ppvOut = psm;
                        hres = NOERROR;
                    }
                    ILFree(pidl); // Free the pidl returned from SHGetSpecialFolderLocation
                    ILFree(pidl2); // Free the pidl returned from SHGetSpecialFolderLocation
                    goto Cleanup;
                }
                break;

            case IDM_MENU_FIND:

                if (_fFindMenuInvalid)
                {
                    mii.cbSize = sizeof (mii);
                    mii.fMask = MIIM_SUBMENU | MIIM_ID;
                    GetMenuItemInfo(psmd->hmenu, IDM_MENU_FIND, MF_BYCOMMAND, &mii);
                    if(mii.hSubMenu)
                    {
                        ATOMICRELEASE(_pcmFind);
                        if (_ptp)
                        {
                            if (SUCCEEDED(_ptp->GetFindCM(mii.hSubMenu, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST, &_pcmFind)))
                            {
                                // we don't want to pass wm_initmenupopup to defcm if we don't have shell32 ver >= 5
                                // because it may cause fault
                                // 
#ifdef SHDOC401_DLL
                                if (g_uiShell32 >= 5)
#endif
                                {
                                // hmenu is cached so no need to call init menu popup more than once (causes leaks
                                // and bogus menu text)

                                    IContextMenu2 *pcm2;
                                    _pcmFind->QueryInterface(IID_IContextMenu2, (LPVOID*)&pcm2);
                                    if (pcm2)
                                    {
                                        pcm2->HandleMenuMsg(WM_INITMENUPOPUP, (WPARAM)mii.hSubMenu, (LPARAM)0);
                                        pcm2->Release();
                                    }
                                }
                            }
                        }

                        if (_pcmFind)
                        {
                            psm->SetMenu(mii.hSubMenu, NULL, SMSET_TOP | SMSET_DONTOWN);
                            _fFindMenuInvalid = FALSE;
                            // Don't Release _pcmFind
                        }
            
                        *ppvOut = (LPVOID)psm;
                        hres = NOERROR;
                        goto Cleanup;
                    }
                }
                break;

                
#ifndef SHDOC401_DLL
            case IDM_NETCONNECT: // Start | Settings | Network Connections Cascade
                {
                    hres = CreateNetworkConnections(psm, hKeyRoot);

                    if (FAILED(hres))
                    {
                        psm->Release();
                        psm = NULL;
                    }

                    *ppvOut = psm;

                    return hres;
                }
                break;
#endif

            default:
                psm->Release();
                hres = S_FALSE;
                break;

            }

            // Do we have a pidl?
            if (pidl)
            {
                // Yes; this menu must want to enumerate a shellfolder
                if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidl, (LPVOID*)&psfFolder)))
                {
                    if (!hKey)
                    {
                        RegCreateKeyEx(hKeyRoot, pszKeyName, NULL, NULL,
                            REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                            NULL, &hKey, &dwDisp);
                    }

                    // Point the menu to the shellfolder
                    psm->SetShellFolder(psfFolder, pidl, hKey, dwSetFolderFlags);
                    *ppvOut = psm;
                    psfFolder->Release();       //IMenuPopup hangs onto this, but they do an addref
                    hres = NOERROR;
                }
                ILFree(pidl); // Free the pidl returned from SHGetSpecialFolderLocation
            }
        }
    }

Cleanup:
    if (hKeyRoot)
        RegCloseKey(hKeyRoot);

    return hres;
}


HRESULT CStartMenuCallback::_FilterPidl(IShellFolder* psf, LPCITEMIDLIST pidl)
{
    HRESULT hres = S_FALSE;
    STRRET strret;

    ASSERT(IS_VALID_PIDL(pidl));
    ASSERT(IS_VALID_CODE_PTR(psf, IShellFolder));

    if (SUCCEEDED(psf->GetDisplayNameOf(pidl, SHGDN_INFOLDER, &strret)))
    {
        TCHAR szChild[MAX_PATH];
        StrRetToStrN(szChild, ARRAYSIZE(szChild), &strret, pidl);

        // HACKHACK (lamadio): This code assumes that the Display name
        // of the Programs and Commons Programs folders are the same. It
        // also assumes that the "programs" folder in the Start Menu folder
        // is the same name as the one pointed to by CSIDL_PROGRAMS.

        if (lstrcmpi(szChild, _szPrograms) == 0)
        {
            //They are the same, pull them.
            hres = S_OK;
        }
    }

    return hres;
}


HRESULT GetFastItemFolder(IShellFolder** ppsf, LPITEMIDLIST* ppidl)
{
    HRESULT hres;
    IAugmentedShellFolder * pasf = NULL;
    LPITEMIDLIST  pidlFast = NULL;

    hres = CoCreateInstance(CLSID_AugmentedShellFolder, NULL, CLSCTX_INPROC, 
                            IID_IAugmentedShellFolder, (LPVOID *)&pasf);
    if (SUCCEEDED(hres))
    {
        hres = SHGetSpecialFolderLocation(NULL, CSIDL_STARTMENU, &pidlFast);
        if (SUCCEEDED(hres))
        {
            IShellFolder* psfFast; //For Fast items on Start Menu

            hres = SHBindToObject(NULL, IID_IShellFolder, pidlFast, (LPVOID*)&psfFast);
            if (SUCCEEDED(hres))
            {
                pasf->AddNameSpace(NULL, psfFast, pidlFast, ASFF_DEFAULT);
                psfFast->Release();
            }

#ifdef WINNT
            IShellFolder* psfFastCommon;
            LPITEMIDLIST  pidlFastCommon;

            if(!SHRestricted(REST_NOCOMMONGROUPS))
            {
                if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_STARTMENU, &pidlFastCommon)))
                {
                    
                    if (SUCCEEDED(SHBindToObject(NULL, IID_IShellFolder, pidlFastCommon, (LPVOID*)&psfFastCommon)))
                    {
                        pasf->AddNameSpace(NULL, psfFastCommon, pidlFastCommon, ASFF_DEFAULT);
                        psfFastCommon->Release();
                    }

                    ILFree(pidlFastCommon);
                }
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


STDAPI  CStartMenu_CreateInstance(LPUNKNOWN punkOuter, REFIID riid, void **ppvOut)
{
    HRESULT hres = E_FAIL;
    IMenuPopup* pmp = NULL;

    *ppvOut = NULL;

    IShellMenuCallback* psmc = new CStartMenuCallback();

    if (psmc)
    {
        IShellMenu* psm;

        hres = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, IID_IShellMenu, (void**)&psm);
        if (SUCCEEDED(hres))
        {
            DWORD dwDisp;
            HKEY hMenuKey = NULL;   // WARNING: pmb2->Initialize() will always owns hMenuKey, so don't close it

            RegCreateKeyEx(HKEY_CURRENT_USER, STRREG_STARTMENU, NULL, NULL,
                REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                NULL, &hMenuKey, &dwDisp);

            TraceMsg(TF_MENUBAND, "Root Start Menu Key Is %d", hMenuKey);
            hres = CoCreateInstance(CLSID_MenuDeskBar, punkOuter, CLSCTX_INPROC_SERVER, IID_IMenuPopup, (void**)&pmp);
            if (SUCCEEDED(hres)) 
            {
                IBandSite* pbs;

                hres = CoCreateInstance(CLSID_MenuBandSite, NULL, CLSCTX_INPROC_SERVER, IID_IBandSite, (LPVOID*)&pbs);
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
            
            // Initialize after it's built up. I need to have a separate com object that
            // builds this up....
            psm->Initialize(psmc, 0, 0, SMINIT_TOPLEVEL | SMINIT_VERTICAL | SMINIT_LEGACYMENU);

            // Add the fast item folder to the top of the menu
            IShellFolder* psfFast;
            LPITEMIDLIST pidlFast;

            if (SUCCEEDED(GetFastItemFolder(&psfFast, &pidlFast)))
            {
                psm->SetShellFolder(psfFast, pidlFast, hMenuKey, SMSET_TOP | SMSET_NOEMPTY);

                psfFast->Release();
                ILFree(pidlFast);
            }

            psm->Release();
        }
        psmc->Release();
    }

    if (SUCCEEDED(hres))
    {
        hres = pmp->QueryInterface(riid, ppvOut);
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
                       public IOldStartMenuTask
{
public:
    // *** IUnknown Methods ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return SUPERCLASS::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void){ return SUPERCLASS::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IOldShellFolderTask Methods ***
    virtual STDMETHODIMP InitTaskSFT(IShellFolder *psfParent, LPITEMIDLIST pidlFull, DWORD dwFlags, DWORD dwTaskPriority);

    // *** IOldStartMenuTask Methods ***
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
    IShellFolder*   _psf;
    LPITEMIDLIST    _pidlFolder;
    IShellHotKey *  _pshk;
    IEnumIDList *   _pei;
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
CStartMenuTask::CStartMenuTask() : CRunnableTask(RTF_SUPPORTKILLSUSPEND)
{
}


// destructor
CStartMenuTask::~CStartMenuTask()
{
    if (_pidlFolder)
        ILFree(_pidlFolder);
        
    ATOMICRELEASE(_pei);
    ATOMICRELEASE(_psf);
    ATOMICRELEASE(_pshk);
}


STDMETHODIMP CStartMenuTask::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;
    static const QITAB qit[] = {
        QITABENT(CStartMenuTask, IOldShellFolderTask),
        QITABENT(CStartMenuTask, IOldStartMenuTask),
        { 0 },
    };

    hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = SUPERCLASS::QueryInterface(riid, ppvObj);
        
    return hres;
}


/*----------------------------------------------------------
Purpose: IOldShellFolderTask::InitTaskSFT method

*/
STDMETHODIMP CStartMenuTask::InitTaskSFT(IShellFolder *psfParent, LPITEMIDLIST pidlFull,
                                         DWORD dwFlags, DWORD dwTaskPriority)
{
    HRESULT hr = E_OUTOFMEMORY;
    IShellFolder* psf;

    if (!psfParent)
        return E_INVALIDARG;

    ASSERT(IS_VALID_PIDL(pidlFull));
    
    _pidlFolder = ILClone(pidlFull);
    if (_pidlFolder)
    {
        // we have a pidlChild, so bind to him
        hr = psfParent->BindToObject(ILFindLastID(_pidlFolder), NULL, IID_IShellFolder, (void **)&psf);
        if (SUCCEEDED(hr))
        {
            _psf = psf;
            _dwTaskPriority = dwTaskPriority;
            _dwFlags = dwFlags;

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
Purpose: IOldStartMenuTask::InitTaskSMT method

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
            IID_IShellTaskScheduler, (LPVOID *) & pScheduler );
    if (SUCCEEDED(hres))
    {
        // create a new task
        IOldStartMenuTask * psmt;
        
        if (SUCCEEDED(CoCreateInstance(CLSID_StartMenuTask, NULL, CLSCTX_INPROC,
                                       IID_IOldStartMenuTask, (LPVOID *)&psmt)))
        {
            // Initialize the task
            LPITEMIDLIST pidlFull = ILCombine(_pidlFolder, pidl);

            if (pidlFull)
            {
                if (SUCCEEDED(psmt->InitTaskSFT(_psf, pidlFull, _dwTaskPriority - 1, THREAD_PRIORITY_NORMAL)))
                {
                    IRunnableTask * ptask;

                    psmt->InitTaskSMT(_pshk, _iThreadPriority);
                    
                    if (SUCCEEDED(psmt->QueryInterface(IID_IRunnableTask, (LPVOID *)&ptask)))
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

    return _psf->EnumObjects(NULL, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &_pei);
}


/*----------------------------------------------------------
Purpose: CRunnableTask::InternalResumeRT method

*/
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
        TCHAR szPath[MAX_PATH];
        DWORD dwCount;
        
        SHGetPathFromIDList(pidl, szPath);
        // TraceMsg(TF_SMTASK, "StartMenuTask (%#lx): caching %s", _dwTaskID, PathFindFileName(szPath));
        dwCount = GetTickCount();
#endif

        // Enumerate folders.  Don't enumerate folders that behave like
        // shortcuts (channels).
        
        DWORD dwAttrib = SFGAO_FOLDER;
        _psf->GetAttributesOf(1, (LPCITEMIDLIST *)&pidl, &dwAttrib);

        // Is this a subfolder?
        if ((dwAttrib & SFGAO_FOLDER) && (_dwFlags & ITSFT_RECURSE))
        {
            // Yes; is this a channel?
            if (!SHIsExpandableFolder(_psf, pidl))
            {
                // Yes; skip it
#ifdef FULL_DEBUG
                TraceMsg(TF_SMTASK, "StartMenuTask (%#lx): skipping %s", _dwTaskID, PathFindFileName(szPath));
#endif
                goto MoveOn;
            }
            
            // create a task to enumerate the folder
            _CreateNewTask(_psf, pidl);
        }

        // Register the hotkey
        if (_pshk)
            _pshk->RegisterHotKey(_psf, _pidlFolder, pidl);
        
        // Cache the icon
        SHMapPIDLToSystemImageListIndex(_psf, pidl, NULL);

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
    
    return hr;
}


//
// CDesktopTask object.  This task enumerates the desktop for hotkeys 
//


#undef  THISCLASS   
#undef  SUPERCLASS
#define SUPERCLASS  CRunnableTask


class CDesktopTask : public CRunnableTask,
                     public IOldStartMenuTask
{
public:
    // *** IUnknown Methods ***
    virtual STDMETHODIMP_(ULONG) AddRef(void) { return SUPERCLASS::AddRef(); };
    virtual STDMETHODIMP_(ULONG) Release(void){ return SUPERCLASS::Release(); };
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IOldShellFolderTask Methods ***
    virtual STDMETHODIMP InitTaskSFT(IShellFolder *psfParent, LPITEMIDLIST pidlFull, DWORD dwFlags, DWORD dwTaskPriority);

    // *** IOldStartMenuTask Methods ***
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


STDMETHODIMP CDesktopTask::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;
    static const QITAB qit[] = {
        QITABENT(CDesktopTask, IOldShellFolderTask),
        QITABENT(CDesktopTask, IOldStartMenuTask),
        { 0 },
    };

    hres = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hres))
        hres = SUPERCLASS::QueryInterface(riid, ppvObj);
        
    return hres;
}


/*----------------------------------------------------------
Purpose: IOldShellFolderTask::InitTaskSFT method

*/
STDMETHODIMP CDesktopTask::InitTaskSFT(IShellFolder *psfParent, LPITEMIDLIST pidlFull,
                                       DWORD dwFlags, DWORD dwTaskPriority)
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
Purpose: IOldStartMenuTask::InitTaskSMT method

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
            _pshk->RegisterHotKey(_psf, _pidlFolder, pidl);

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

// For the Favorites menu, since their icon handler is SO slow, we're going to fake the icon
// and have it get the real ones on the background thread...
HRESULT CStartMenuCallback::_GetDefaultIcon(TCHAR* psz, int* piIndex)
{
    DWORD cbSize = sizeof(TCHAR) * MAX_PATH;

    HKEY hkeyT;

    if (ERROR_SUCCESS == RegOpenKey(HKEY_CLASSES_ROOT, TEXT("InternetShortcut\\DefaultIcon"), &hkeyT))
    {
        if (ERROR_SUCCESS == SHQueryValueEx(hkeyT, NULL, NULL, NULL, 
                                          (LPBYTE)psz, &cbSize))
        {
            *piIndex = PathParseIconLocation(psz);
        }
        RegCloseKey(hkeyT);
    }
    return S_OK;
}
