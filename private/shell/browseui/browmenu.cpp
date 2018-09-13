#include "priv.h"
#include "browmenu.h"
#include "resource.h"
#include "menuband.h"
#include <shsemip.h>
#include "uemapp.h"
#include "mluisupp.h"

#define UEM_NEWITEMCOUNT 2
// Exported by shdocvw
STDAPI GetLinkInfo(IShellFolder* psf, LPCITEMIDLIST pidlItem, BOOL* pfAvailable, BOOL* pfSticky);

#define REG_STR_MAIN TEXT("SOFTWARE\\Microsoft\\Internet Explorer\\Main")

BOOL AreIntelliMenusEnbaled()
{
    // This is only garenteed to work on NT5 because the session incrementer is located in the tray
    if (IsOS(OS_WIN2000))
    {
        DWORD dwRest = SHRestricted(REST_INTELLIMENUS);
        if (dwRest != RESTOPT_INTELLIMENUS_USER)
            return (dwRest == RESTOPT_INTELLIMENUS_ENABLED);

        return SHRegGetBoolUSValue(REG_STR_MAIN, TEXT("FavIntelliMenus"),
                                   FALSE, TRUE); // Don't ignore HKCU, Enable Menus by default
    }
    else
        return FALSE;
}


CFavoritesCallback::CFavoritesCallback() : _cRef(1)
{
    _fOffline = BOOLIFY(SHIsGlobalOffline());
}

CFavoritesCallback::~CFavoritesCallback()
{
    ASSERT(_punkSite == NULL);

    ASSERT(_psmFavCache == NULL);
}

/*----------------------------------------------------------
Purpose: IUnknown::QueryInterface method

*/
STDMETHODIMP CFavoritesCallback::QueryInterface (REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CFavoritesCallback, IShellMenuCallback),
        QITABENT(CFavoritesCallback, IObjectWithSite),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


/*----------------------------------------------------------
Purpose: IUnknown::AddRef method

*/
STDMETHODIMP_(ULONG) CFavoritesCallback::AddRef ()
{
    return ++_cRef;
}

/*----------------------------------------------------------
Purpose: IUnknown::Release method

*/
STDMETHODIMP_(ULONG) CFavoritesCallback::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if( _cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

/*----------------------------------------------------------
Purpose: IObjectWithSite::SetSite method

*/
STDMETHODIMP CFavoritesCallback::SetSite(IUnknown* punk)
{
    ATOMICRELEASE(_punkSite);
    _punkSite = punk;
    if (_punkSite)
    {
        _punkSite->AddRef();
    }
    else if (_psmFavCache)
    {
        // Since the top level menu is being destroyed, they are removing
        // our site. We should cleanup.
        DWORD dwFlags;
        UINT uId;
        UINT uIdA;

        _psmFavCache->GetMenuInfo(NULL, &uId, &uIdA, &dwFlags);

        // Tell menuband we're no longer caching it. We need to do this so ClowseDW
        // cleans up the menus.
        dwFlags &= ~SMINIT_CACHED;
        _psmFavCache->Initialize(NULL, uId, uIdA, dwFlags); 

        IDeskBand* pdesk;
        if (SUCCEEDED(_psmFavCache->QueryInterface(IID_IDeskBand, (LPVOID*)&pdesk)))
        {
            pdesk->CloseDW(0);
            pdesk->Release();
        }

        ATOMICRELEASE(_psmFavCache);
    }

    return NOERROR;

}

/*----------------------------------------------------------
Purpose: IShellMenuCallback::CallbackSM method

*/
STDMETHODIMP CFavoritesCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = S_FALSE;
    switch (uMsg)
    {
    case SMC_INITMENU:
        hres = _Init(psmd->hmenu, psmd->uIdParent, psmd->punk);
        break;

    case SMC_EXITMENU:
        hres = _Exit();
        break;

    case SMC_CREATE:
        if (psmd->uIdParent == FCIDM_MENU_FAVORITES)
            _fExpandoMenus = AreIntelliMenusEnbaled();
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

    case SMC_SFEXEC:
        hres = SHNavigateToFavorite(psmd->psf, psmd->pidlItem, _punkSite, SBSP_DEFBROWSER | SBSP_DEFMODE);
        break;

    case SMC_GETINFO:
        hres = _GetHmenuInfo(psmd->hmenu, psmd->uId, (SMINFO*)lParam);
        break;

    case SMC_SFSELECTITEM:
        hres = _SelectItem(psmd->pidlFolder, psmd->pidlItem);
        break;

    case SMC_GETOBJECT:
        hres = _GetObject(psmd, (GUID)*((GUID*)wParam), (void**)lParam);
        break;

    case SMC_DEFAULTICON:
        hres = _GetDefaultIcon((LPTSTR)wParam, (int*)lParam);
        break;

    case SMC_GETSFINFO:
        hres = _GetSFInfo(psmd, (SMINFO*)lParam);
        break;

    case SMC_SHCHANGENOTIFY:
        {
            PSMCSHCHANGENOTIFYSTRUCT pshf = (PSMCSHCHANGENOTIFYSTRUCT)lParam;
            hres = _ProcessChangeNotify(psmd, pshf->lEvent, pshf->pidl1, pshf->pidl2);
        }
        break;

    case SMC_REFRESH:
        _fExpandoMenus = AreIntelliMenusEnbaled();
        break;

    case SMC_CHEVRONGETTIP:
        hres = _GetTip((LPTSTR)wParam, (LPTSTR)lParam);
        break;

    case SMC_CHEVRONEXPAND:
        {
            if (_fShowingTip)
            {
                LPTSTR pszExpanded = TEXT("NO");

                SHRegSetUSValue(REG_STR_MAIN, TEXT("FavChevron"),
                    REG_SZ, pszExpanded, lstrlen(pszExpanded) * sizeof(TCHAR), SHREGSET_FORCE_HKCU);
            }

            _fShowingTip = FALSE;

            hres = S_OK;
        }
        break;

    case SMC_DISPLAYCHEVRONTIP:

        // Should we show the tip?
        _fShowingTip = SHRegGetBoolUSValue(REG_STR_MAIN, TEXT("FavChevron"), FALSE, TRUE);    // Default to YES.

        if (_fShowingTip)
        {
            hres = S_OK;
        }
        break;

    case SMC_SFDDRESTRICTED:
        hres = _AllowDrop((IDataObject*)wParam, (HWND)lParam) ? S_FALSE : S_OK;
        break;
    }

    return hres;
}


HRESULT CFavoritesCallback::_Init(HMENU hMenu, UINT uIdParent, IUnknown* punk)
{
    HRESULT hres = S_FALSE;
    IOleCommandTarget* poct;

#ifdef DEBUG
    if (GetAsyncKeyState(VK_SHIFT) < 0)
    {
        UEMFireEvent(&UEMIID_BROWSER, UEME_CTLSESSION, UEMF_XEVENT, TRUE, -1);
    }
#endif


    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IOleCommandTarget, (void**)&poct)))
    {
        poct->Exec(&CGID_MenuBand, MBANDCID_ENTERMENU, 0, NULL, NULL);
        poct->Release();
        hres = S_OK;
    }

    // Only do this for the favorites dropdown. This was causing 
    // the chevron menu to be invalidated before it was created. This caused some
    // resize problems because the metrics were unavailable.
    if (uIdParent == FCIDM_MENU_FAVORITES)
    {
        // If we switched between online and offline, we need to re-init the menu
        BOOL fOffline = BOOLIFY(SHIsGlobalOffline());
        if (fOffline ^ _fOffline)
        {
            _fOffline = fOffline;
            IShellMenu* psm;
            if (SUCCEEDED(punk->QueryInterface(IID_IShellMenu, (void**)&psm)))
            {
                psm->InvalidateItem(NULL, SMINV_REFRESH);
                psm->Release();
            }
        }
    }
    return hres;
}


HRESULT CFavoritesCallback::_Exit()
{
    HRESULT hres = S_FALSE;
    IOleCommandTarget* poct;

    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_IOleCommandTarget, (void**)&poct)))
    {
        poct->Exec(&CGID_MenuBand, MBANDCID_EXITMENU, 0, NULL, NULL);
        poct->Release();
        hres = S_OK;    // I handled the exit
    }

    return hres;
}

HRESULT CFavoritesCallback::_GetHmenuInfo(HMENU hMenu, UINT uId, SMINFO* psminfo)
{
    if (uId == FCIDM_MENU_FAVORITES)
    {
        if (psminfo->dwMask & SMIM_FLAGS)
            psminfo->dwFlags |= SMIF_DROPCASCADE;
    }
    else
    {
        if (psminfo->dwMask & SMIM_FLAGS)
            psminfo->dwFlags |= SMIF_TRACKPOPUP;
    }

    // No item has icons
    if (psminfo->dwMask & SMIM_ICON)
        psminfo->iIcon = -1;
    
    return S_OK;
}


HRESULT CFavoritesCallback::_GetSFInfo(SMDATA* psmd, SMINFO* psminfo)
{
    BOOL fAvailable;

    //
    // If we are offline and the item is not available, we set the
    // SMIF_ALTSTATE so that the menu item is greyed
    //
    if (psminfo->dwMask & SMIM_FLAGS)
    {
        if (_fOffline &&
            SUCCEEDED(GetLinkInfo(psmd->psf, psmd->pidlItem, &fAvailable, NULL)) &&
            fAvailable == FALSE)
        {
            // Not available, so grey the item
            psminfo->dwFlags |= SMIF_ALTSTATE;
        }

        if (_fExpandoMenus)
            psminfo->dwFlags |= _GetDemote(psmd);
    }
    return S_OK;
}

HRESULT CFavoritesCallback::_SelectItem(LPCITEMIDLIST pidlFolder, LPCITEMIDLIST pidl)
{
    HRESULT hres = S_FALSE;
    LPITEMIDLIST pidlFull = ILCombine(pidlFolder, pidl);
    if (pidlFull)
    {
        VARIANTARG vargIn;
        if (InitVariantFromIDList(&vargIn, pidlFull))
        {
            hres = IUnknown_QueryServiceExec(_punkSite, SID_SMenuBandHandler,
                &CGID_MenuBandHandler, MBHANDCID_PIDLSELECT, 0, &vargIn, NULL);
            VariantClearLazy(&vargIn);
        }
        ILFree(pidlFull);
    }
    return hres;
}

void CFavoritesCallback::_RefreshItem(HMENU hmenu, int idCmd, IShellMenu* psm)
{
    SMDATA smd;
    smd.dwMask = SMDM_HMENU;
    smd.hmenu = hmenu;
    smd.uId = idCmd;

    psm->InvalidateItem(&smd, SMINV_ID | SMINV_REFRESH);
}

HRESULT CFavoritesCallback::_GetObject(LPSMDATA psmd, REFIID riid, void** ppvOut)
{
    HRESULT hres = S_FALSE;
    *ppvOut = NULL;

    if (IsEqualIID(IID_IShellMenu, riid))
    {
        if (psmd->uId == FCIDM_MENU_FAVORITES)
        {
            // Do we have a cached Favorites menu?
            if (_psmFavCache)
            {
                // Yes we do, return it
                _psmFavCache->AddRef();
                *ppvOut = (LPVOID)_psmFavCache;
                hres = S_OK;
            }
            else
            {
                // Nope; We need to create one...
                hres = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC, 
                    IID_IShellMenu, (void**)&_psmFavCache);

                if (SUCCEEDED(hres))
                {
                    HMENU hmenu = NULL;
                    HWND hwnd;

                    _psmFavCache->Initialize(this, FCIDM_MENU_FAVORITES, ANCESTORDEFAULT, 
                        SMINIT_CACHED | SMINIT_VERTICAL); 

                    // We need to grab the Top HMENU portion of the Favorites menu from the current band
                    IShellMenu* psm;
                    if (SUCCEEDED(psmd->punk->QueryInterface(IID_IShellMenu, (LPVOID*)&psm)))
                    {
                        psm->GetMenu(&hmenu, &hwnd, NULL);

                        hmenu = GetSubMenu(hmenu, GetMenuPosFromID(hmenu, FCIDM_MENU_FAVORITES));

                        // Delete the placeholder item (there to keep the separator from getting
                        // lost during shbrowse menu merging, which deletes trailing separators).
                        int iPos = GetMenuPosFromID(hmenu, FCIDM_FAVPLACEHOLDER);
                        if (iPos >= 0)
                            DeleteMenu(hmenu, iPos, MF_BYPOSITION);

                        psm->Release();
                    }

                    if (hmenu)
                    {
                        hres = _psmFavCache->SetMenu(hmenu, hwnd, SMSET_TOP | SMSET_DONTOWN);
                    }
 
                    LPITEMIDLIST pidlFav;
                    if (SUCCEEDED(hres) &&
                        SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidlFav)))
                    {
                        IShellFolder* psf;
                        if (SUCCEEDED(IEBindToObject(pidlFav, &psf)))
                        {
                            HKEY hMenuKey;
                            DWORD dwDisp;

                            RegCreateKeyEx(HKEY_CURRENT_USER, STRREG_FAVORITES, NULL, NULL,
                                REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE,
                                NULL, &hMenuKey, &dwDisp);

                            hres = _psmFavCache->SetShellFolder(psf, pidlFav, hMenuKey, 
                                SMSET_BOTTOM | SMSET_USEBKICONEXTRACTION | SMSET_HASEXPANDABLEFOLDERS);
                            psf->Release();
                        }
                        ILFree(pidlFav);
                    }

                    if (SUCCEEDED(hres))
                    {
                        _psmFavCache->AddRef(); // We're caching this.
                        *ppvOut = _psmFavCache;
                    }
                }
            }
        }
    }
    else if (IsEqualIID(IID_IShellMenuCallback, riid))
    {
        IShellMenuCallback* psmcb = (IShellMenuCallback*) new CFavoritesCallback;

        if (psmcb)
        {
            *ppvOut = (LPVOID)psmcb;
            hres = S_OK;
        }
    }

    return hres;
}


// Short circuit the looking up of a default icon. We're going to assume that all of them
// are URLs, even folders, for the sake of speed. It gives the user feedback directly, then
// we asyncronously render the real icons.
HRESULT CFavoritesCallback::_GetDefaultIcon(TCHAR* psz, int* piIndex)
{
    DWORD cbSize = sizeof(TCHAR) * MAX_PATH;

    if (ERROR_SUCCESS == SHGetValue(HKEY_CLASSES_ROOT, TEXT("InternetShortcut\\DefaultIcon"), NULL, NULL, psz, &cbSize))
        *piIndex = PathParseIconLocation(psz);
        
    return S_OK;
}

DWORD CFavoritesCallback::_GetDemote(SMDATA* psmd)
{
    UEMINFO uei;
    DWORD dwFlags = 0;
    if (_fExpandoMenus)
    {
        uei.cbSize = SIZEOF(uei);
        uei.dwMask = UEIM_HIT;
        if (SUCCEEDED(UEMQueryEvent(&UEMIID_BROWSER, UEME_RUNPIDL, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem, &uei)))
        {
            if (uei.cHit == 0) 
            {
                dwFlags |= SMIF_DEMOTED;
            }
        }
    }

    return dwFlags;
}

HRESULT CFavoritesCallback::_Demote(LPSMDATA psmd)
{
    HRESULT hres = S_FALSE;

    if (_fExpandoMenus)
    {
        UEMINFO uei;
        uei.cbSize = SIZEOF(uei);
        uei.dwMask = UEIM_HIT;
        uei.cHit = 0;
        hres = UEMSetEvent(&UEMIID_BROWSER, UEME_RUNPIDL, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem, &uei);
    }
    return hres;
}

HRESULT CFavoritesCallback::_Promote(LPSMDATA psmd)
{
    if (_fExpandoMenus) 
    {
        UEMFireEvent(&UEMIID_BROWSER, UEME_RUNPIDL, UEMF_XEVENT, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem);
    }
    return S_OK;
}

HRESULT CFavoritesCallback::_HandleNew(LPSMDATA psmd)
{
    HRESULT hres = S_FALSE;
    if (_fExpandoMenus)
    {
        UEMINFO uei;
        uei.cbSize = SIZEOF(uei);
        uei.dwMask = UEIM_HIT;
        uei.cHit = UEM_NEWITEMCOUNT;
        hres = UEMSetEvent(&UEMIID_BROWSER, UEME_RUNPIDL, (WPARAM)psmd->psf, (LPARAM)psmd->pidlItem, &uei);
    }

    return hres;
}

HRESULT CFavoritesCallback::_GetTip(LPTSTR pstrTitle, LPTSTR pstrTip)
{
    MLLoadString(IDS_CHEVRONTIPTITLE, pstrTitle, MAX_PATH);
    MLLoadString(IDS_CHEVRONTIP, pstrTip, MAX_PATH);

    // Why would this fail?
    if (EVAL(pstrTitle[0] != TEXT('\0') && pstrTip[0] != TEXT('\0')))
        return S_OK;

    return S_FALSE;
}

// BUGBUG (lamadio): There is a duplicate of this helper in shell32\unicpp\startmnu.cpp
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

// BUGBUG (lamadio): There is a duplicate of this helper in shell32\unicpp\startmnu.cpp
//                   When modifying this, rev that one as well.
void UEMDeletePidl(const GUID *pguidGrp, IShellFolder* psf, LPCITEMIDLIST pidl)
{
    UEMINFO uei;
    uei.cbSize = SIZEOF(uei);
    uei.dwMask = UEIM_HIT;
    uei.cHit = 0;
    UEMSetEvent(pguidGrp, UEME_RUNPIDL, (WPARAM)psf, (LPARAM)pidl, &uei);
}

HRESULT CFavoritesCallback::_ProcessChangeNotify(SMDATA* psmd, LONG lEvent, 
                                                 LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    switch (lEvent)
    {
    case SHCNE_RENAMEFOLDER:
        // BUGBUG (lamadio): We should move the MenuOrder stream as well. 5.5.99
    case SHCNE_RENAMEITEM:
        {
            LPITEMIDLIST pidlFavorites;
            SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidlFavorites);
            if (ILIsParent(pidlFavorites, pidl1, FALSE))
            {
                IShellFolder* psfFrom;
                LPCITEMIDLIST pidlFrom;
                if (SUCCEEDED(IEBindToParentFolder(pidl1, &psfFrom, &pidlFrom)))
                {
                    if (ILIsParent(pidlFavorites, pidl2, FALSE))
                    {
                        IShellFolder* psfTo;
                        LPCITEMIDLIST pidlTo;

                        if (SUCCEEDED(IEBindToParentFolder(pidl2, &psfTo, &pidlTo)))
                        {
                            // Then we need to rename it
                            UEMRenamePidl(&UEMIID_BROWSER, psfFrom, pidlFrom, 
                                          &UEMIID_BROWSER, psfTo, pidlTo);
                            psfTo->Release();
                        }
                    }
                    else
                    {
                        // Otherwise, we delete it.
                        UEMDeletePidl(&UEMIID_BROWSER, psfFrom, pidlFrom);
                    }

                    psfFrom->Release();
                }
            }

            ILFree(pidlFavorites);
        }
        break;

    case SHCNE_DELETE:
        // BUGBUG (lamadio): We should nuke the MenuOrder stream as well. 5.5.99
    case SHCNE_RMDIR:
        {
            IShellFolder* psf;
            LPCITEMIDLIST pidl;

            if (SUCCEEDED(IEBindToParentFolder(pidl1, &psf, &pidl)))
            {
                UEMDeletePidl(&UEMIID_BROWSER, psf, pidl);
                psf->Release();
            }

        }
        break;

    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        {
            IShellFolder* psf;
            LPCITEMIDLIST pidl;

            if (SUCCEEDED(IEBindToParentFolder(pidl1, &psf, &pidl)))
            {
                UEMINFO uei;
                uei.cbSize = SIZEOF(uei);
                uei.dwMask = UEIM_HIT;
                uei.cHit = UEM_NEWITEMCOUNT;
                UEMSetEvent(&UEMIID_BROWSER, 
                    UEME_RUNPIDL, (WPARAM)psf, (LPARAM)pidl, &uei);
            }

        }
        break;
    }

    return S_FALSE;
}

//
// _Disallow drop returns S_OK if the drop shold not be allowed.  S_FALSE if
// the drop should be allowed.
//
BOOL CFavoritesCallback::_AllowDrop(IDataObject* pIDataObject, HWND hwnd)
{
    ASSERT(pIDataObject);
    ASSERT(NULL == hwnd || IsWindow(hwnd));

    BOOL fRet = True;  // Allow drop.

    if (hwnd)
    {
        LPITEMIDLIST pidl;

        if (SUCCEEDED(SHPidlFromDataObject(pIDataObject, &pidl, NULL, 0)))
        {
            fRet = IEIsLinkSafe(hwnd, pidl, ILS_ADDTOFAV);
            ILFree(pidl);
        }
    }

    return fRet;
}
