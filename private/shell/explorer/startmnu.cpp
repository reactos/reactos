#include "cabinet.h"
//#define CPP_FUNCTIONS
#include <crtfree.h>
#include "rcids.h"
#include <shguidp.h>
#include <fsmenu.h>
#include "bandsite.h"
#include "shellp.h"
#include "shdguid.h"
#include <regstr.h> 
#include "startmnu.h"
#include "trayp.h"      // for WMTRAY_*

#include "apithk.h"


//BUGBUG move this to a common location, it's currently in shdocvw\menubar.h
#define  MBCID_GETSIDE   1
#define MENUBAR_LEFT     ABE_LEFT
#define MENUBAR_TOP      ABE_TOP
#define MENUBAR_RIGHT    ABE_RIGHT
#define MENUBAR_BOTTOM   ABE_BOTTOM

#define REGSTR_PATH_ADVANCED REGSTR_PATH_EXPLORER TEXT("\\Advanced")
HMENU GetStaticStartMenu();


//So much for trying to keep it self contained....
extern "C" HMENU Menu_FindSubMenuByFirstID(HMENU hmenu, UINT id); //plucked out of tray.c


// *** IUnknown methods ***
STDMETHODIMP CStartMenuHost::QueryInterface (REFIID riid, LPVOID * ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IOleWindow) ||
        IsEqualIID(riid, IID_IDeskBarClient) ||
        IsEqualIID(riid, IID_IMenuPopup))
    {
        *ppvObj = SAFECAST(this, IMenuPopup*);
    }
    else if(IsEqualIID(riid,IID_ITrayPriv))
        *ppvObj = SAFECAST(this,ITrayPriv*);
    else if(IsEqualIID(riid,IID_IShellService))
        *ppvObj = SAFECAST(this,IShellService*);
    else if(IsEqualIID(riid,IID_IServiceProvider))
        *ppvObj = SAFECAST(this,IServiceProvider*);
    else if(IsEqualIID(riid,IID_IOleCommandTarget))
        *ppvObj = SAFECAST(this,IOleCommandTarget*);
    else if(IsEqualIID(riid,IID_IWinEventHandler))
        *ppvObj = SAFECAST(this,IWinEventHandler*);
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}


STDMETHODIMP_(ULONG) CStartMenuHost::AddRef ()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CStartMenuHost::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if( _cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

/*----------------------------------------------------------
Purpose: ITrayPriv::ExecItem method

*/
STDMETHODIMP CStartMenuHost::ExecItem (IShellFolder* psf, LPCITEMIDLIST pidl)
{
    HRESULT hres = SHInvokeDefaultCommand(v_hwndTray, psf, pidl);

    // HACKHACK (reinerf) - since the proper hres is not returned from SHInvokeDefaultCommand and I dont want to risk changing
    // the context menu code for win2k at this point, we use GetLastError to see if the user cancled the operation so that
    // we can avoid showing our error dialog in the user-cancelled case (ShellExecuteEx will call SetLastError, thank god!)
    if (FAILED(hres) && (GetLastError() != ERROR_CANCELLED))
    {
        TCHAR szLinkPath[MAX_PATH];
        SHGetPathFromIDList(pidl, szLinkPath);
        int iError = IDS_CANTFINDFILE;


        if (szLinkPath[0] == TEXT('\0'))
        {
            iError = IDS_CANTRUN;
        }

        ShellMessageBox(hinstCabinet, v_hwndTray, MAKEINTRESOURCE(iError), 
                        MAKEINTRESOURCE(IDS_CABINET), MB_ICONEXCLAMATION, szLinkPath);


    }

    return hres;
}


/*----------------------------------------------------------
Purpose: ITrayPriv::GetFindCM method

*/
STDMETHODIMP CStartMenuHost::GetFindCM(HMENU hmenu, UINT idFirst, UINT idLast, IContextMenu** ppcmFind)
{
    *ppcmFind = SHFind_InitMenuPopup(hmenu, v_hwndTray, TRAY_IDM_FINDFIRST, TRAY_IDM_FINDLAST);
    if(*ppcmFind)
        return NOERROR;
    else
        return E_FAIL;

}


/*----------------------------------------------------------
Purpose: ITrayPriv::GetStaticStartMenu method

*/
STDMETHODIMP CStartMenuHost::GetStaticStartMenu(HMENU* phmenu)
{
    *phmenu = ::GetStaticStartMenu();
    if(*phmenu)
        return NOERROR;
    else
        return E_FAIL;
}


// *** IServiceProvider ***
STDMETHODIMP CStartMenuHost::QueryService (REFGUID guidService, REFIID riid, void ** ppvObject)
{
    if(IsEqualGUID(guidService,SID_SMenuPopup))
        return QueryInterface(riid,ppvObject);
    else
        return E_NOINTERFACE;
}


// *** IShellService ***

// BUGBUG (scotth): remove this if it's not being used
STDMETHODIMP CStartMenuHost::SetOwner (struct IUnknown* punkOwner)
{
    return E_NOTIMPL;
}


// *** IOleWindow methods ***
STDMETHODIMP CStartMenuHost::GetWindow(HWND * lphwnd)
{
    *lphwnd = v_hwndTray;
    return NOERROR;
}


/*----------------------------------------------------------
Purpose: IMenuPopup::Popup method

*/
STDMETHODIMP CStartMenuHost::Popup(POINTL *ppt, RECTL *prcExclude, DWORD dwFlags)
{
    return E_NOTIMPL;
}


/*----------------------------------------------------------
Purpose: IMenuPopup::OnSelect method

*/
STDMETHODIMP CStartMenuHost::OnSelect(DWORD dwSelectType)
{
    return NOERROR;
}


/*----------------------------------------------------------
Purpose: IMenuPopup::SetSubMenu method

*/
extern "C" UINT g_uStartButtonAllowPopup;

STDMETHODIMP CStartMenuHost::SetSubMenu(IMenuPopup* pmp, BOOL fSet)
{
    if (!fSet)
    {
        g_ts.bMainMenuInit = FALSE;
        // Tell the Start Button that it's allowed to be in the up position now. This
        // prevents the problem where the start menu is displayed but the button is
        // in the up position... This happens when dialog boxes are displayed
        SendMessage(g_ts.hwndStart, g_uStartButtonAllowPopup, 0, 0);

        // Now tell it to be in the up position
        _ForceStartButtonUp();
    }
    return NOERROR;
}


// *** IOleCommandTarget ***
STDMETHODIMP  CStartMenuHost::QueryStatus (const GUID * pguidCmdGroup,
    ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    return E_NOTIMPL;
}

STDMETHODIMP  CStartMenuHost::Exec (const GUID * pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (IsEqualGUID(CGID_MENUDESKBAR,*pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case MBCID_GETSIDE:
            pvarargOut->vt = VT_I4;
            pvarargOut->lVal = MENUBAR_TOP;
            break;
        default:
            break;
        }
    }
    return NOERROR;
}

// *** IWinEventHandler ***
STDMETHODIMP CStartMenuHost::OnWinEvent(HWND h, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *plres)
{
    //Forward events to the tray winproc?
    return E_NOTIMPL;
}

STDMETHODIMP CStartMenuHost::IsWindowOwner(HWND hwnd)
{
    return E_NOTIMPL;
}

CStartMenuHost::CStartMenuHost() : _cRef(1)
{ 
}


HRESULT StartMenuHost_Create(IMenuPopup** ppmp, IMenuBand** ppmb)
{
    HRESULT hres = E_OUTOFMEMORY;
    IMenuPopup * pmp = NULL;
    IMenuBand * pmb = NULL;

    CStartMenuHost *psmh = new CStartMenuHost();
    if (psmh)
    {
        hres = CoCreateInstance(CLSID_StartMenuBar, NULL, CLSCTX_INPROC_SERVER, 
                                IID_IMenuPopup, (LPVOID*)&pmp);
        if (SUCCEEDED(hres))
        {
            IObjectWithSite* pows;

            hres = pmp->QueryInterface(IID_IObjectWithSite, (void**)&pows);
            if(SUCCEEDED(hres))
            {
                IInitializeObject* pio;

                pows->SetSite(SAFECAST(psmh, ITrayPriv*));

                hres = pmp->QueryInterface(IID_IInitializeObject, (void**)&pio);
                if(SUCCEEDED(hres))
                {
                    hres = pio->Initialize();
                    pio->Release();
                }

                if (SUCCEEDED(hres))
                {
                    IUnknown* punk;

                    hres = pmp->GetClient(&punk);
                    if (SUCCEEDED(hres))
                    {
                        IBandSite* pbs;

                        hres = punk->QueryInterface(IID_IBandSite, (void**)&pbs);
                        if(SUCCEEDED(hres))
                        {
                            DWORD dwBandID;

                            pbs->EnumBands(0, &dwBandID);
                            hres = pbs->GetBandObject(dwBandID, IID_IMenuBand, (void**)&pmb);
                            pbs->Release();
                            // Don't release pmb
                        }
                        punk->Release();
                    }
                }

                if (FAILED(hres))
                    pows->SetSite(NULL);

                pows->Release();
            }

            // Don't release pmp
        }
        psmh->Release();
    }

    if (FAILED(hres))
    {
        ATOMICRELEASE(pmp);
        ATOMICRELEASE(pmb);
    }

    *ppmp = pmp;
    *ppmb = pmb;

    return hres;
}



HRESULT IMenuPopup_SetIconSize(IMenuPopup* pmp,DWORD iIcon)
{
    IBanneredBar* pbb;

	if (pmp == NULL)
		return E_FAIL;

    HRESULT hres = pmp->QueryInterface(IID_IBanneredBar,(void**)&pbb);
    if (SUCCEEDED(hres))
    {
        pbb->SetIconSize(iIcon);
        pbb->Release();
    }
    return hres;
}


BOOL _IsValidKey(HKEY hkeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szPathExplorer[MAX_PATH];
    DWORD cbSize = ARRAYSIZE(szPath);
    DWORD dwType;

    lstrcpy(szPathExplorer, REGSTR_PATH_EXPLORER);
    PathCombine(szPathExplorer, szPathExplorer, pszSubKey);
    if (ERROR_SUCCESS == SHGetValue(hkeyRoot, szPathExplorer, pszValue, 
            &dwType, szPath, &cbSize))
    {
        // Zero in the DWORD case or NULL in the string case
        // indicates that this item is not available.
        if (dwType == REG_DWORD)
            return *((DWORD*)szPath) != 0;
        else
            return (TCHAR)szPath[0] != TEXT('\0');    
    }

    return FALSE;
}

BOOL _IsRestricted(HKEY hkeyRoot, BOOL fRestrictedIfNotValid, RESTRICTIONS rest, LPCTSTR pszSubKey, LPCTSTR pszValue)
{
    // If it's shell restricted, remove
    DWORD dwRest = SHRestricted(rest);
    // A restiction of 0 means it's ok, to add, but check to see if some
    // magic reg value is set. A restriction of 2 means, Do it all the time.

    BOOL fValidKey = _IsValidKey(hkeyRoot, pszSubKey, pszValue);

    if (dwRest == 1)
        return TRUE;

    if (dwRest == 2)
        return FALSE;

    return !(fValidKey ^ fRestrictedIfNotValid);  // If it's not a valid key, then it's fRestrictedIfNotValid
}

void HandleFirstTime()
{
    // If this key does not exist, then this is the first boot for this user.
    BOOL fFirstTime = !_IsValidKey(HKEY_CURRENT_USER, TEXT("Advanced"), TEXT("StartMenuInit"));

    if (fFirstTime)
    {
        // If this is the first boot of the shell for this user, then we need to see if it's an upgrade.
        // If it is, then we need set the Logoff option.    PM Decision to have a different 
        // look for upgraded machines...

        TCHAR szPath[MAX_PATH];
        TCHAR szPathExplorer[MAX_PATH];
        DWORD cbSize = ARRAYSIZE(szPath);
        DWORD dwType;
        DWORD dwValue = 1;  // We can be confident that we are not blowing away previous flags.
                            // Why? because we only do this if the key does not exist.

        // Is this an upgrade (Does WindowsUpdate\UpdateURL Exist?)
        PathCombine(szPathExplorer, REGSTR_PATH_EXPLORER, TEXT("WindowsUpdate"));
        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, szPathExplorer, TEXT("UpdateURL"), 
                &dwType, szPath, &cbSize) && 
                szPath[0] != TEXT('\0'))
        {
            // Yes; Then write the option out to the registry.
            SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_ADVANCED, TEXT("StartMenuLogoff"), REG_DWORD, &dwValue, sizeof(DWORD));
        }

        // Mark this so that we know we've been launched once.
        SHSetValue(HKEY_CURRENT_USER, REGSTR_PATH_ADVANCED, TEXT("StartMenuInit"), REG_DWORD, &dwValue, sizeof(DWORD));
    }
}

// This code was obtained from ACPI land.

BOOL CanShowEject()
{
    BOOL bResult = TRUE;

#ifdef WINNT
    HANDLE hToken = NULL;
    PRIVILEGE_SET privilegeSet;

    privilegeSet.PrivilegeCount = 1;
    privilegeSet.Control = 0;
    privilegeSet.Privilege[0].Attributes = 0;

    if (!LookupPrivilegeValue(NULL, SE_UNDOCK_NAME, &privilegeSet.Privilege[0].Luid)) 
    {
    
        //
        // No such privilege?
        //
        ASSERT(0);
        bResult = TRUE;
    }

    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken)) 
    {
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_IMPERSONATE, &hToken))
        {
	
            //
            // Error, can't get privileges.
            //
            bResult = FALSE;
        }
    }


    if (hToken != NULL &&
        !PrivilegeCheck(hToken, &privilegeSet, &bResult)) 
    {
    
	    //
	    // Unprivileged
	    // 
        bResult = FALSE;
    }

    if (hToken)
        CloseHandle(hToken);
#endif
    return bResult; 
}

BOOL GetLogonUserName(LPTSTR pszUsername, DWORD* pcchUsername)
{
    BOOL fSuccess = FALSE;

    HKEY hkeyExplorer = NULL;
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER, 0, KEY_QUERY_VALUE, &hkeyExplorer))
    {
        DWORD dwType;
        DWORD dwSize = (*pcchUsername) * sizeof(TCHAR);

        if (ERROR_SUCCESS == RegQueryValueEx(hkeyExplorer, TEXT("Logon User Name"), 0, &dwType,
            (LPBYTE) pszUsername, &dwSize))
        {
            if ((REG_SZ == dwType) && (*pszUsername))
            {
                fSuccess = TRUE;
            }
        }

        RegCloseKey(hkeyExplorer);
    }

    // Fall back on GetUserName if the Logon User Name isn't set.
    if (!fSuccess)
    {
        fSuccess = GetUserName(pszUsername, pcchUsername);

        if (fSuccess)
        {
            CharUpperBuff(pszUsername, 1);
        }
    }

    return fSuccess;
}

HMENU GetStaticStartMenu()
{
#ifdef WINNT // hydra adds two more items
#define CITEMSMISSING 4
#else
#define CITEMSMISSING 3
#endif

    HMENU hStartMenu = LoadMenuPopup(MAKEINTRESOURCE(MENU_START));
    HMENU hmenu;
    UINT iSep2ItemsMissing = 0;

    //
    // Default to the Win95/NT4 version of the Settings menu.
    //

    // Restictions
    if (SHRestricted(REST_NORUN))
    {
        DeleteMenu(hStartMenu, IDM_FILERUN, MF_BYCOMMAND);
    }

    if (SHRestricted(REST_NOCLOSE))     
    {
        DeleteMenu(hStartMenu, IDM_EXITWIN, MF_BYCOMMAND);
        iSep2ItemsMissing++;     
    }

    if (_IsRestricted(HKEY_CURRENT_USER, TRUE, REST_NOSMHELP, TEXT("Advanced"), TEXT("NoStartMenuHelp")))
    {
        DeleteMenu(hStartMenu, IDM_HELPSEARCH, MF_BYCOMMAND);
    }


    if (_IsRestricted(HKEY_LOCAL_MACHINE, FALSE, REST_NOCSC, TEXT("Advanced"), TEXT("StartMenuSyncAll")))
    {
        DeleteMenu(hStartMenu, IDM_CSC, MF_BYCOMMAND);
        iSep2ItemsMissing++;     
    }

    // We want the Logoff menu on the start menu if:
    //  These MUST both be true
    // 1) It's not restricted
    // 2) We have Logged On.
    //  Any of these three.
    // 3) We've Upgraded from IE4 
    // 4) The user has specified that it should be present
    // 5) It's been "Restricted" On.

    // Behavior also depends on whether we are a remote session or not (dsheldon):
    // Remote session: Logoff brings up shutdown dialog
    // Console session: Logoff directly does logoff
    
    DWORD dwRest = SHRestricted(REST_STARTMENULOGOFF);

    // See if this is an upgrade, and set the user setting accordingly.
    HandleFirstTime();

    BOOL fUserWantsLogoff = _IsValidKey(HKEY_CURRENT_USER, TEXT("Advanced"), TEXT("StartMenuLogoff"));
    BOOL fAdminWantsLogoff = (BOOL)(dwRest == 2) || SHRestricted(REST_FORCESTARTMENULOGOFF);

    if ((dwRest != 1 && (GetSystemMetrics(SM_NETWORK) & RNC_LOGON) != 0) &&
        ( fUserWantsLogoff || fAdminWantsLogoff))
    {
        UINT idMenuRenameToLogoff = IDM_LOGOFF;

        TCHAR szUserName[200];
        TCHAR szTemp[256];
        TCHAR szMenuText[256];
        DWORD dwSize = ARRAYSIZE(szUserName);
        MENUITEMINFO mii;

        mii.cbSize = sizeof(MENUITEMINFO);
        mii.dwTypeData = szTemp;
        mii.fMask = MIIM_TYPE | MIIM_ID | MIIM_SUBMENU | MIIM_STATE | MIIM_DATA;
        mii.cch = ARRAYSIZE(szTemp);
        mii.hSubMenu = NULL;
        mii.fType = MFT_SEPARATOR;                // to avoid ramdom result.
        mii.dwItemData = 0;

        GetMenuItemInfo(hStartMenu,idMenuRenameToLogoff,MF_BYCOMMAND,&mii);

        if (GetLogonUserName(szUserName, &dwSize))
        {
            wsprintf (szMenuText,szTemp, szUserName);
        }
        else if (!LoadString(hinstCabinet, IDS_LOGOFFNOUSER, 
                                          szMenuText, ARRAYSIZE(szMenuText)))
        {
            // mem error, use the current string.
            szUserName[0] = 0;
            wsprintf(szMenuText, szTemp, szUserName);
        }    

        mii.dwTypeData = szMenuText;
        mii.cch = ARRAYSIZE(szMenuText);
        SetMenuItemInfo(hStartMenu,idMenuRenameToLogoff,MF_BYCOMMAND,&mii);
    }
    else
    {
        DeleteMenu(hStartMenu, IDM_LOGOFF, MF_BYCOMMAND);
        iSep2ItemsMissing++;
    }

    if (iSep2ItemsMissing == CITEMSMISSING)
    {
        DeleteMenu(hStartMenu, IDM_SEP2, MF_BYCOMMAND);
    }

    // CanShowEject Queries the user's permission to eject,
    // IsEjectAllowed queries the hardware.
    if (!CanShowEject() || !IsEjectAllowed(FALSE))
    {
        DeleteMenu(hStartMenu, IDM_EJECTPC, MF_BYCOMMAND);
    }

    // Setting stuff.
    hmenu = Menu_FindSubMenuByFirstID(hStartMenu, IDM_CONTROLS);
    if (hmenu)
    {
        int iMissingSettings = 0;

#ifdef WINNT // hydra menu items
        #define CITEMS_SETTINGS     5   // Number of items in settings menu
#else
        #define CITEMS_SETTINGS     4   // Number of items in settings menu
#endif

        
        if (SHRestricted(REST_NOSETTASKBAR))
        {
            DeleteMenu(hStartMenu, IDM_TRAYPROPERTIES, MF_BYCOMMAND);
            iMissingSettings++;
        }

        if (SHRestricted(REST_NOSETFOLDERS) || SHRestricted(REST_NOCONTROLPANEL))
        {
            DeleteMenu(hStartMenu, IDM_CONTROLS, MF_BYCOMMAND);

            // For the separator that now on top
            DeleteMenu(hmenu, 0, MF_BYPOSITION);   
            iMissingSettings++;
        }

        if (SHRestricted(REST_NOSETFOLDERS))
        {
            DeleteMenu(hStartMenu, IDM_PRINTERS, MF_BYCOMMAND);
            iMissingSettings++;
        }

        if (SHRestricted(REST_NOSETFOLDERS) || SHRestricted(REST_NONETWORKCONNECTIONS))
        {
            DeleteMenu(hStartMenu, IDM_NETCONNECT, MF_BYCOMMAND);
            iMissingSettings++;
        }

#ifdef WINNT // hydra menu items
        if (!IsRemoteSession() || SHRestricted(REST_NOSECURITY))
        {
            DeleteMenu(hStartMenu, IDM_MU_SECURITY, MF_BYCOMMAND);
            iMissingSettings++;     
        }
#endif

        // Are all the items missing?
        if (iMissingSettings == CITEMS_SETTINGS)
        {
            // Yes; don't bother showing the menu at all
            DeleteMenu(hStartMenu, IDM_SETTINGS, MF_BYCOMMAND);
        }
    }
    else
    {
        DebugMsg(DM_ERROR, TEXT("c.fm_rui: Settings menu couldn't be found. Restricted items may not have been removed."));
    }

    // Find menu.
    if (SHRestricted(REST_NOFIND))
    {
        DeleteMenu(hStartMenu, IDM_MENU_FIND, MF_BYCOMMAND);
    }

    // Documents menu.
    if (SHRestricted(REST_NORECENTDOCSMENU))
    {
        DeleteMenu(hStartMenu, IDM_RECENT, MF_BYCOMMAND);
    }

    // Favorites menu.
    if (_IsRestricted(HKEY_CURRENT_USER, FALSE, REST_NOFAVORITESMENU, TEXT("Advanced"), TEXT("StartMenuFavorites")))
    {
        DeleteMenu(hStartMenu, IDM_FAVORITES, MF_BYCOMMAND);
    }

    return hStartMenu;
}



//
//  CHotKey class
//


// constructor
CHotKey::CHotKey() : _cRef(1)
{
}


STDMETHODIMP CHotKey::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IShellHotKey))
    {
        *ppvObj = SAFECAST(this, IShellHotKey *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}


STDMETHODIMP_(ULONG) CHotKey::AddRef()
{
    return ++_cRef;
}

STDMETHODIMP_(ULONG) CHotKey::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if( _cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

/*----------------------------------------------------------
Purpose: IShellHotKey::RegisterHotKey method

*/
STDMETHODIMP CHotKey::RegisterHotKey(IShellFolder * psf, LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidl)
{
    WORD wHotkey;

    wHotkey = _GetHotkeyFromFolderItem(psf, pidl);
    if (wHotkey)
    {
        int i = HotkeyList_Add(wHotkey, (LPITEMIDLIST)pidlParent, (LPITEMIDLIST)pidl, TRUE);
        if (i != -1)
        {
            // Register in the context of the tray's thread.
            PostMessage(v_hwndTray, WMTRAY_REGISTERHOTKEY, i, 0);
        }
    }
    return S_OK;
}

STDAPI CHotKey_Create(IShellHotKey ** ppshk)
{
    HRESULT hres = E_OUTOFMEMORY;
    CHotKey * photkey = new CHotKey;

    if (photkey)
    {
        hres = S_OK;
    }

    *ppshk = SAFECAST(photkey, IShellHotKey *);
    return hres;
}
