/*
 * ReactOS Explorer
 *
 * Copyright 2009 Andrew Hill <ash77 at domain reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
Implements a class that knows how to hold and manage the menu band, brand band,
toolbar, and address band for an explorer window
*/

#include "precomp.h"

#if 1

interface IAugmentedShellFolder : public IShellFolder
{
    virtual HRESULT STDMETHODCALLTYPE AddNameSpace(LPGUID, IShellFolder *, LPCITEMIDLIST, ULONG) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetNameSpaceID(LPCITEMIDLIST, LPGUID) = 0;
    virtual HRESULT STDMETHODCALLTYPE QueryNameSpace(ULONG, LPGUID, IShellFolder **) = 0;
    virtual HRESULT STDMETHODCALLTYPE EnumNameSpace(ULONG, PULONG) = 0;
};

#endif

// navigation controls and menubar just send a message to parent window
/*
TODO:
****Implement BandProxy methods
****Add QueryStatus handler for built-in bands
****Enable/Disable up, search, and folders commands appropriately
  **Why are explorer toolbar separators a nonstandard width?
  **Remove "(Empty)" item from Favorites menu. Probably something missing in CMenuCallback::CallbackSM
  **Chevron menu on menuband doesn't work
  **Fix CInternetToolbar::QueryBand to be generic

****Fix context menu to strip divider when menu shown for menu band
****Fix context menu to have items checked appropriately
****Implement -1 command id update
****When bands are rearranged, resize the internet toolbar and fix height of brand band
****Right clicking on the browse back and forward toolbar buttons displays the same as pulldown menus
    Implement show/hide of bands
    Why is the background color of my toolbars different from explorer?
    Internet Toolbar command handler should get the target for the command and call Exec on the target.
        For commands built in to the Internet Toolbar, its Exec handles the command
    When window width is changed, brand band flashes badly
    Add all bands with correct ids (system bands now add with correct ids)
    Implement IBandSite
    Implement remaining IExplorerToolbar methods
    Fix toolbar buttons to enable/disable correctly
    After toolbar is customized, it may be necessary to patch the widths of separators
    Add theme support
    Check sizes and spacing of toolbars against Explorer
    Implement resizing of the dock bar
    Add missing icons for toolbar items
    Draw History item in forward/back dropdown menus with icon
    Fix toolbar customize dialog to not include separators as possible selections
    Implement save/restore of toolbar state
    Refactor drop down menu code to use a common function since code is so similar
*/

extern HRESULT WINAPI SHBindToFolder(LPCITEMIDLIST path, IShellFolder **newFolder);

HRESULT IUnknown_RelayWinEvent(IUnknown * punk, HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    CComPtr<IWinEventHandler> menuWinEventHandler;
    HRESULT hResult = punk->QueryInterface(IID_PPV_ARG(IWinEventHandler, &menuWinEventHandler));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = menuWinEventHandler->IsWindowOwner(hWnd);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    if (hResult == S_OK)
        return menuWinEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
    return S_FALSE;
}

HRESULT IUnknown_ShowDW(IUnknown * punk, BOOL fShow)
{
    CComPtr<IDockingWindow> dockingWindow;
    HRESULT hResult = punk->QueryInterface(IID_PPV_ARG(IDockingWindow, &dockingWindow));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = dockingWindow->ShowDW(fShow);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return S_OK;
}

HRESULT IUnknown_CloseDW(IUnknown * punk, DWORD dwReserved)
{
    CComPtr<IDockingWindow> dockingWindow;
    HRESULT hResult = punk->QueryInterface(IID_PPV_ARG(IDockingWindow, &dockingWindow));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = dockingWindow->CloseDW(dwReserved);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return S_OK;
}

class CInternetToolbar;

class CDockSite :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDockingWindowSite,
    public IInputObjectSite,
    public IOleCommandTarget,
    public IServiceProvider
{
public:
    enum {
        ITF_NOGRIPPER = 1,
        ITF_NOTITLE = 2,
        ITF_NEWBANDALWAYS = 4,
        ITF_GRIPPERALWAYS = 8,
        ITF_FIXEDSIZE = 16
    };
private:
    CComPtr<IUnknown>                       fContainedBand;         // the band inside us
    CInternetToolbar                        *fToolbar;              // our browser
    HWND                                    fRebarWindow;
    HWND                                    fChildWindow;
    int                                     fBandID;
public:
    int                                     fFlags;
private:
    bool                                    fInitialized;
    // fields of DESKBANDINFO must be preserved between calls to GetBandInfo
    DESKBANDINFO                            fDeskBandInfo;
public:
    CDockSite();
    ~CDockSite();
    HRESULT Initialize(IUnknown *containedBand, CInternetToolbar *browser, HWND hwnd, int bandID, int flags);
    HRESULT GetRBBandInfo(REBARBANDINFOW &bandInfo);
private:

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *lphwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IDockingWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBorderDW(IUnknown* punkObj, LPRECT prcBorder);
    virtual HRESULT STDMETHODCALLTYPE RequestBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw);
    virtual HRESULT STDMETHODCALLTYPE SetBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw);

    // *** IInputObjectSite specific methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus);

    // *** IOleCommandTarget specific methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
        OLECMD prgCmds[  ], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
        DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

BEGIN_COM_MAP(CDockSite)
    COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    COM_INTERFACE_ENTRY_IID(IID_IDockingWindowSite, IDockingWindowSite)
    COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
    COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
    COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
END_COM_MAP()
};

CDockSite::CDockSite()
{
    fToolbar = NULL;
    fRebarWindow = NULL;
    fChildWindow = NULL;
    fBandID = 0;
    fFlags = 0;
    fInitialized = false;
    memset(&fDeskBandInfo, 0, sizeof(fDeskBandInfo));
}

CDockSite::~CDockSite()
{
}

HRESULT CDockSite::Initialize(IUnknown *containedBand, CInternetToolbar *browser, HWND hwnd, int bandID, int flags)
{
    TCHAR                                   textBuffer[40];
    REBARBANDINFOW                          bandInfo;
    HRESULT                                 hResult;

    fContainedBand = containedBand;
    fToolbar = browser;
    fRebarWindow = hwnd;
    fBandID = bandID;
    fFlags = flags;
    hResult = IUnknown_SetSite(containedBand, static_cast<IOleWindow *>(this));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = IUnknown_GetWindow(containedBand, &fChildWindow);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    memset(&bandInfo, 0, sizeof(bandInfo));
    bandInfo.cbSize = sizeof(bandInfo);
    bandInfo.lpText = textBuffer;
    bandInfo.cch = sizeof(textBuffer) / sizeof(TCHAR);
    hResult = GetRBBandInfo(bandInfo);

    SendMessage(fRebarWindow, RB_GETBANDCOUNT, 0, 0);
    SendMessage(fRebarWindow, RB_INSERTBANDW, -1, (LPARAM)&bandInfo);
    fInitialized = true;
    return S_OK;
}

HRESULT CDockSite::GetRBBandInfo(REBARBANDINFOW &bandInfo)
{
    CComPtr<IDeskBand>                      deskBand;
    HRESULT                                 hResult;

    hResult = fContainedBand->QueryInterface(IID_PPV_ARG(IDeskBand, &deskBand));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    fDeskBandInfo.dwMask = DBIM_BKCOLOR | DBIM_MODEFLAGS | DBIM_TITLE | DBIM_ACTUAL |
        DBIM_INTEGRAL | DBIM_MAXSIZE | DBIM_MINSIZE;
    hResult = deskBand->GetBandInfo(fBandID, 0, &fDeskBandInfo);
    // result of call is ignored

    bandInfo.fMask = RBBIM_LPARAM | RBBIM_IDEALSIZE | RBBIM_ID | RBBIM_CHILDSIZE | RBBIM_CHILD |
        RBBIM_TEXT | RBBIM_STYLE;

    bandInfo.fStyle = RBBS_FIXEDBMP;
    if (fDeskBandInfo.dwModeFlags & DBIMF_VARIABLEHEIGHT)
        bandInfo.fStyle |= RBBS_VARIABLEHEIGHT;
    if (fDeskBandInfo.dwModeFlags & DBIMF_USECHEVRON)
        bandInfo.fStyle |= RBBS_USECHEVRON;
    if (fDeskBandInfo.dwModeFlags & DBIMF_BREAK)
        bandInfo.fStyle |= RBBS_BREAK;
    if (fDeskBandInfo.dwModeFlags & DBIMF_TOPALIGN)
        bandInfo.fStyle |= RBBS_TOPALIGN;
    if (fFlags & ITF_NOGRIPPER || fToolbar->fLocked)
        bandInfo.fStyle |= RBBS_NOGRIPPER;
    if (fFlags & ITF_NOTITLE)
        bandInfo.fStyle |= RBBS_HIDETITLE;
    if (fFlags & ITF_GRIPPERALWAYS && !fToolbar->fLocked)
        bandInfo.fStyle |= RBBS_GRIPPERALWAYS;
    if (fFlags & ITF_FIXEDSIZE)
        bandInfo.fStyle |= RBBS_FIXEDSIZE;

    if (fDeskBandInfo.dwModeFlags & DBIMF_BKCOLOR)
    {
        bandInfo.fMask |= RBBIM_COLORS;
        bandInfo.clrFore = CLR_DEFAULT;
        bandInfo.clrBack = fDeskBandInfo.crBkgnd;
    }
    wcsncpy(bandInfo.lpText, fDeskBandInfo.wszTitle, bandInfo.cch);
    bandInfo.hwndChild = fChildWindow;
    bandInfo.cxMinChild = fDeskBandInfo.ptMinSize.x;
    bandInfo.cyMinChild = fDeskBandInfo.ptMinSize.y;
    bandInfo.wID = fBandID;
    bandInfo.cyChild = fDeskBandInfo.ptActual.y;
    bandInfo.cyMaxChild = fDeskBandInfo.ptMaxSize.y;
    bandInfo.cyIntegral = fDeskBandInfo.ptIntegral.y;
    bandInfo.cxIdeal = fDeskBandInfo.ptActual.x;
    bandInfo.lParam = reinterpret_cast<LPARAM>(this);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDockSite::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = fRebarWindow;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CDockSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::GetBorderDW(IUnknown* punkObj, LPRECT prcBorder)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::RequestBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::SetBorderSpaceDW(IUnknown* punkObj, LPCBORDERWIDTHS pbw)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::OnFocusChangeIS (IUnknown *punkObj, BOOL fSetFocus)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
    OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CDockSite::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
    VARIANT *pvaIn, VARIANT *pvaOut)
{
    TCHAR                                   textBuffer[40];
    REBARBANDINFOW                          bandInfo;
    int                                     index;
    HRESULT                                 hResult;

    if (IsEqualIID(*pguidCmdGroup, CGID_DeskBand))
    {
        switch (nCmdID)
        {
            case DBID_BANDINFOCHANGED:
                if (fInitialized == false)
                    return S_OK;
                if (V_VT(pvaIn) != VT_I4)
                    return E_INVALIDARG;
                if (V_I4(pvaIn) != fBandID)
                    return E_FAIL;
                // deskband information changed
                // call GetBandInfo and refresh information in rebar
                memset(&bandInfo, 0, sizeof(bandInfo));
                bandInfo.cbSize = sizeof(bandInfo);
                bandInfo.lpText = textBuffer;
                bandInfo.cch = sizeof(textBuffer) / sizeof(TCHAR);
                hResult = GetRBBandInfo(bandInfo);
                if (FAILED_UNEXPECTEDLY(hResult))
                    return hResult;
                index = (int)SendMessage(fRebarWindow, RB_IDTOINDEX, fBandID, 0);
                SendMessage(fRebarWindow, RB_SETBANDINFOW, index, (LPARAM)&bandInfo);
                return S_OK;
        }
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CDockSite::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    if (IsEqualIID(guidService, SID_SMenuBandParent))
        return this->QueryInterface(riid, ppvObject);

    return fToolbar->QueryService(guidService, riid, ppvObject);
}

CMenuCallback::CMenuCallback()
{
}

CMenuCallback::~CMenuCallback()
{
}

static HRESULT BindToDesktop(LPCITEMIDLIST pidl, IShellFolder ** ppsfResult)
{
    HRESULT hr;
    CComPtr<IShellFolder> psfDesktop;

    *ppsfResult = NULL;

    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED(hr))
        return hr;

    hr = psfDesktop->BindToObject(pidl, NULL, IID_PPV_ARG(IShellFolder, ppsfResult));

    return hr;
}

static HRESULT GetFavoritesFolder(IShellFolder ** ppsfFavorites, LPITEMIDLIST * ppidl)
{
    HRESULT hr;
    LPITEMIDLIST pidlUserFavorites;
    LPITEMIDLIST pidlCommonFavorites;
    CComPtr<IShellFolder> psfUserFavorites;
    CComPtr<IShellFolder> psfCommonFavorites;
    CComPtr<IAugmentedShellFolder> pasf;

    if (ppsfFavorites)
        *ppsfFavorites = NULL;

    if (ppidl)
        *ppidl = NULL;

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_FAVORITES, &pidlUserFavorites);
    if (FAILED(hr))
    {
        WARN("Failed to get the USER favorites folder. Trying to run with just the COMMON one.\n");

        hr = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_FAVORITES, &pidlCommonFavorites);
        if (FAILED_UNEXPECTEDLY(hr))
            return hr;

        TRACE("COMMON favorites obtained.\n");
        *ppidl = pidlCommonFavorites;
        hr = BindToDesktop(pidlCommonFavorites, ppsfFavorites);
        return hr;
    }

    hr = SHGetSpecialFolderLocation(NULL, CSIDL_COMMON_FAVORITES, &pidlCommonFavorites);
    if (FAILED_UNEXPECTEDLY(hr))
    {
        WARN("Failed to get the COMMON favorites folder. Will use only the USER contents.\n");
        *ppidl = pidlCommonFavorites;
        hr = BindToDesktop(pidlUserFavorites, ppsfFavorites);
        return hr;
    }

    TRACE("Both COMMON and USER favorites folders obtained, merging them...\n");

    hr = BindToDesktop(pidlUserFavorites, &psfUserFavorites);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = BindToDesktop(pidlCommonFavorites, &psfCommonFavorites);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = CMergedFolder_CreateInstance(IID_PPV_ARG(IAugmentedShellFolder, &pasf));
    if (FAILED_UNEXPECTEDLY(hr))
    {
        *ppsfFavorites = psfUserFavorites.Detach();
        *ppidl = pidlUserFavorites;
        ILFree(pidlCommonFavorites);
        return hr;
    }

    hr = pasf->AddNameSpace(NULL, psfUserFavorites, pidlUserFavorites, 0xFF00);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pasf->AddNameSpace(NULL, psfCommonFavorites, pidlCommonFavorites, 0);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = pasf->QueryInterface(IID_PPV_ARG(IShellFolder, ppsfFavorites));
    pasf.Release();

    // TODO: obtain the folder's PIDL

    ILFree(pidlCommonFavorites);
    ILFree(pidlUserFavorites);

    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuCallback::GetObject(LPSMDATA psmd, REFIID riid, void **ppvObject)
{
    CComPtr<IShellMenu>                     parentMenu;
    CComPtr<IShellMenu>                     newMenu;
    CComPtr<IShellFolder>                   favoritesFolder;
    LPITEMIDLIST                            favoritesPIDL;
    HWND                                    ownerWindow;
    HMENU                                   parentHMenu;
    HMENU                                   favoritesHMenu;
    HKEY                                    orderRegKey;
    DWORD                                   disposition;
    HRESULT                                 hResult;
    static const TCHAR szFavoritesKey[] =
        _T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\MenuOrder\\Favorites");

    if (!IsEqualIID(riid, IID_IShellMenu))
        return E_FAIL;
    if (psmd->uId != FCIDM_MENU_FAVORITES)
        return E_FAIL;

    // create favorites menu
    hResult = psmd->punk->QueryInterface(IID_PPV_ARG(IShellMenu, &parentMenu));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = parentMenu->GetMenu(&parentHMenu, &ownerWindow, NULL);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    favoritesHMenu = GetSubMenu(parentHMenu, 3);
    if (favoritesHMenu == NULL)
        return E_FAIL;

    if (fFavoritesMenu.p == NULL)
    {
        hResult = CMenuBand_CreateInstance(IID_PPV_ARG(IShellMenu, &newMenu));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        hResult = newMenu->Initialize(this, FCIDM_MENU_FAVORITES, -1, SMINIT_VERTICAL | SMINIT_CACHED);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;

        RegCreateKeyEx(HKEY_CURRENT_USER, szFavoritesKey,
                0, NULL, 0, KEY_READ | KEY_WRITE, NULL, &orderRegKey, &disposition);

        hResult = GetFavoritesFolder(&favoritesFolder, &favoritesPIDL);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;

        hResult = newMenu->SetShellFolder(favoritesFolder, favoritesPIDL, orderRegKey, SMSET_BOTTOM | SMINIT_CACHED | SMINV_ID);
        if (favoritesPIDL)
            ILFree(favoritesPIDL);

        if (FAILED(hResult))
            return hResult;

        fFavoritesMenu = newMenu;
    }

    hResult = fFavoritesMenu->SetMenu(favoritesHMenu, ownerWindow, SMSET_TOP | SMSET_DONTOWN);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    return fFavoritesMenu->QueryInterface(riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CMenuCallback::CallbackSM(LPSMDATA psmd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case SMC_INITMENU:
            break;
        case SMC_CREATE:
            break;
        case SMC_EXITMENU:
            break;
        case SMC_GETINFO:
        {
            SMINFO *infoPtr = reinterpret_cast<SMINFO *>(lParam);
            if ((infoPtr->dwMask & SMIM_FLAGS) != 0)
            {
                if (psmd->uId == FCIDM_MENU_FAVORITES)
                {
                    infoPtr->dwFlags |= SMIF_DROPCASCADE;
                }
                else
                {
                    infoPtr->dwFlags |= SMIF_TRACKPOPUP;
                }
            }
            if ((infoPtr->dwMask & SMIM_ICON) != 0)
                infoPtr->iIcon = -1;
            return S_OK;
        }
        case SMC_GETSFINFO:
            break;
        case SMC_GETOBJECT:
            return GetObject(psmd, *reinterpret_cast<IID *>(wParam), reinterpret_cast<void **>(lParam));
        case SMC_GETSFOBJECT:
            break;
        case SMC_EXEC:
            PostMessageW(psmd->hwnd, WM_COMMAND, psmd->uId, 0);
            break;
        case SMC_SFEXEC:
            SHInvokeDefaultCommand(psmd->hwnd, psmd->psf, psmd->pidlItem);
            break;
        case SMC_SFSELECTITEM:
            break;
        case 13:
            // return tooltip
            break;
        case SMC_REFRESH:
            break;
        case SMC_DEMOTE:
            break;
        case SMC_PROMOTE:
            break;
        case 0x13:
            break;
        case SMC_DEFAULTICON:
            break;
        case SMC_NEWITEM:
            break;
        case SMC_CHEVRONEXPAND:
            break;
        case SMC_DISPLAYCHEVRONTIP:
            break;
        case SMC_SETSFOBJECT:
            break;
        case SMC_SHCHANGENOTIFY:
            break;
        case SMC_CHEVRONGETTIP:
            break;
        case SMC_SFDDRESTRICTED:
            break;
        case 0x35:
            break;
        case 49:
            break;
        case 0x10000000:
            break;
    }
    return S_FALSE;
}

CInternetToolbar::CInternetToolbar()
{
    fMainReBar = NULL;
    fLocked = false;
    fMenuBandWindow = NULL;
    fNavigationWindow = NULL;
    fMenuCallback = new CComObject<CMenuCallback>();
    fToolbarWindow = NULL;
    fAdviseCookie = 0;

    fMenuCallback->AddRef();
}

CInternetToolbar::~CInternetToolbar()
{
}

void CInternetToolbar::AddDockItem(IUnknown *newItem, int bandID, int flags)
{
    CComPtr<CDockSite> newSite;

    newSite = new CComObject<CDockSite>;
    newSite->Initialize(newItem, this, fMainReBar, bandID, flags);
}

HRESULT CInternetToolbar::ReserveBorderSpace(LONG maxHeight)
{
    CComPtr<IDockingWindowSite>             dockingWindowSite;
    RECT                                    availableBorderSpace;

    HRESULT hResult = fSite->QueryInterface(IID_PPV_ARG(IDockingWindowSite, &dockingWindowSite));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = dockingWindowSite->GetBorderDW(static_cast<IDockingWindow *>(this), &availableBorderSpace);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    if (maxHeight && availableBorderSpace.bottom - availableBorderSpace.top > maxHeight)
    {
        availableBorderSpace.bottom = availableBorderSpace.top + maxHeight;
    }

    return ResizeBorderDW(&availableBorderSpace, fSite, FALSE);
}

HRESULT CInternetToolbar::CreateMenuBar(IShellMenu **pMenuBar)
{
    CComPtr<IShellMenu>                     menubar;
    CComPtr<IShellMenuCallback>             callback;
    VARIANT                                 menuOut;
    HWND                                    ownerWindow;
    HRESULT                                 hResult;

    if (!pMenuBar)
        return E_POINTER;

    *pMenuBar = NULL;

    hResult = CMenuBand_CreateInstance(IID_PPV_ARG(IShellMenu, &menubar));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = fMenuCallback->QueryInterface(IID_PPV_ARG(IShellMenuCallback, &callback));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = menubar->Initialize(callback, -1, ANCESTORDEFAULT, SMINIT_HORIZONTAL | SMINIT_TOPLEVEL);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    // Set Menu
    {
        hResult = IUnknown_Exec(fSite, CGID_Explorer, 0x35, 0, NULL, &menuOut);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;

        if (V_VT(&menuOut) != VT_INT_PTR || V_INTREF(&menuOut) == NULL)
            return E_FAIL;

        hResult = IUnknown_GetWindow(fSite, &ownerWindow);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;

        HMENU hMenuBar = (HMENU) V_INTREF(&menuOut);

        // FIXME: Figure out the proper way to do this.
        HMENU hMenuFavs = GetSubMenu(hMenuBar, 3);
        if (hMenuFavs)
        {
            DeleteMenu(hMenuFavs, IDM_FAVORITES_EMPTY, MF_BYCOMMAND);
        }

        hResult = menubar->SetMenu(hMenuBar, ownerWindow, SMSET_DONTOWN);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }

    hResult = IUnknown_Exec(menubar, CGID_MenuBand, 3, 1, NULL, NULL);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    *pMenuBar = menubar.Detach();

    return S_OK;
}

HRESULT CInternetToolbar::LockUnlockToolbars(bool locked)
{
    REBARBANDINFOW                          rebarBandInfo;
    int                                     bandCount;
    CDockSite                               *dockSite;
    HRESULT                                 hResult;

    if (locked != fLocked)
    {
        fLocked = locked;
        rebarBandInfo.cbSize = sizeof(rebarBandInfo);
        rebarBandInfo.fMask = RBBIM_STYLE | RBBIM_LPARAM;
        bandCount = (int)SendMessage(fMainReBar, RB_GETBANDCOUNT, 0, 0);
        for (INT x  = 0; x < bandCount; x++)
        {
            SendMessage(fMainReBar, RB_GETBANDINFOW, x, (LPARAM)&rebarBandInfo);
            dockSite = reinterpret_cast<CDockSite *>(rebarBandInfo.lParam);
            if (dockSite != NULL)
            {
                rebarBandInfo.fStyle &= ~(RBBS_NOGRIPPER | RBBS_GRIPPERALWAYS);
                if (dockSite->fFlags & CDockSite::ITF_NOGRIPPER || fLocked)
                    rebarBandInfo.fStyle |= RBBS_NOGRIPPER;
                if (dockSite->fFlags & CDockSite::ITF_GRIPPERALWAYS && !fLocked)
                    rebarBandInfo.fStyle |= RBBS_GRIPPERALWAYS;
                SendMessage(fMainReBar, RB_SETBANDINFOW, x, (LPARAM)&rebarBandInfo);
            }
        }
        hResult = ReserveBorderSpace(0);

        // TODO: refresh view menu?
    }
    return S_OK;
}

HRESULT CInternetToolbar::SetState(const GUID *pguidCmdGroup, long commandID, OLECMD* pcmd)
{
    long state = 0;
    if (pcmd->cmdf & OLECMDF_ENABLED)
        state |= TBSTATE_ENABLED;
    if (pcmd->cmdf & OLECMDF_LATCHED)
        state |= TBSTATE_CHECKED;
    return SetState(pguidCmdGroup, commandID, state);
}

HRESULT CInternetToolbar::CommandStateChanged(bool newValue, int commandID)
{
    HRESULT                                 hResult;

    hResult = S_OK;
    switch (commandID)
    {
        case -1:
            // loop through buttons
            //for buttons in CLSID_CommonButtons
            //    if up, QueryStatus for up state and update it
            //
            //for buttons in fCommandCategory, update with QueryStatus of fCommandTarget

            OLECMD commandList[4];
            commandList[0].cmdID = 0x1c;
            commandList[1].cmdID = 0x1d;
            commandList[2].cmdID = 0x1e;
            commandList[3].cmdID = 0x23;
            IUnknown_QueryStatus(fSite, CGID_Explorer, 4, commandList, NULL);
            SetState(&CLSID_CommonButtons, gSearchCommandID, &commandList[0]);
            SetState(&CLSID_CommonButtons, gFoldersCommandID, &commandList[3]);
            //SetState(&CLSID_CommonButtons, gFavoritesCommandID, &commandList[2]);
            //SetState(&CLSID_CommonButtons, gHistoryCommandID, &commandList[1]);

            break;
        case 1:
            // forward
            hResult = SetState(&CLSID_CommonButtons, IDM_GOTO_FORWARD, newValue ? TBSTATE_ENABLED : 0);
            break;
        case 2:
            // back
            hResult = SetState(&CLSID_CommonButtons, IDM_GOTO_BACK, newValue ? TBSTATE_ENABLED : 0);
            break;
        case 3:
            // up
            hResult = SetState(&CLSID_CommonButtons, IDM_GOTO_UPONELEVEL, newValue ? TBSTATE_ENABLED : 0);
            break;
    }
    return hResult;
}

HRESULT CInternetToolbar::CreateAndInitBandProxy()
{
    CComPtr<IServiceProvider>               serviceProvider;
    HRESULT                                 hResult;

    hResult = fSite->QueryInterface(IID_PPV_ARG(IServiceProvider, &serviceProvider));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = serviceProvider->QueryService(SID_IBandProxy, IID_PPV_ARG(IBandProxy, &fBandProxy));
    if (FAILED_UNEXPECTEDLY(hResult))
    {
        hResult = CBandProxy_CreateInstance(IID_PPV_ARG(IBandProxy, &fBandProxy));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        hResult = fBandProxy->SetSite(fSite);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::HasFocusIO()
{
    HRESULT hr = S_FALSE;

    if (fMenuBar)
        hr = IUnknown_HasFocusIO(fMenuBar);
    if (hr != S_FALSE)
        return hr;

    if (fControlsBar)
        hr = IUnknown_HasFocusIO(fControlsBar);
    if (hr != S_FALSE)
        return hr;

    if (fNavigationBar)
        hr = IUnknown_HasFocusIO(fNavigationBar);
    if (hr != S_FALSE)
        return hr;

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::TranslateAcceleratorIO(LPMSG lpMsg)
{
    HRESULT hr = S_FALSE;

    if (fMenuBar)
        hr = IUnknown_TranslateAcceleratorIO(fMenuBar, lpMsg);
    if (hr == S_OK)
        return hr;

    if (fControlsBar)
        hr = IUnknown_TranslateAcceleratorIO(fControlsBar, lpMsg);
    if (hr == S_OK)
        return hr;

    if (fNavigationBar)
        hr = IUnknown_TranslateAcceleratorIO(fNavigationBar, lpMsg);
    if (hr == S_OK)
        return hr;

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::ShowDW(BOOL fShow)
{
    HRESULT                     hResult;

    // show the bar here
    if (fShow)
    {
        hResult = ReserveBorderSpace();
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }

    if (fMenuBar)
    {
        hResult = IUnknown_ShowDW(fMenuBar, fShow);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }

    if (fControlsBar)
    {
        hResult = IUnknown_ShowDW(fControlsBar, fShow);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }
    if (fNavigationBar)
    {
        hResult = IUnknown_ShowDW(fNavigationBar, fShow);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }
    if (fLogoBar)
    {
        hResult = IUnknown_ShowDW(fLogoBar, fShow);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::CloseDW(DWORD dwReserved)
{
    HRESULT                     hResult;

    if (fMenuBar)
    {
        hResult = IUnknown_CloseDW(fMenuBar, dwReserved);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        ReleaseCComPtrExpectZero(fMenuBar);
    }
    if (fControlsBar)
    {
        hResult = IUnknown_CloseDW(fControlsBar, dwReserved);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        ReleaseCComPtrExpectZero(fControlsBar);
    }
    if (fNavigationBar)
    {
        hResult = IUnknown_CloseDW(fNavigationBar, dwReserved);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        ReleaseCComPtrExpectZero(fNavigationBar);
    }
    if (fLogoBar)
    {
        hResult = IUnknown_CloseDW(fLogoBar, dwReserved);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        ReleaseCComPtrExpectZero(fLogoBar);
    }

    SetSite(NULL);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::ResizeBorderDW(LPCRECT prcBorder,
    IUnknown *punkToolbarSite, BOOL fReserved)
{
    RECT neededBorderSpace;
    RECT availableBorderSpace = *prcBorder;

    SendMessage(fMainReBar, RB_SIZETORECT, RBSTR_CHANGERECT, reinterpret_cast<LPARAM>(&availableBorderSpace));

    // RBSTR_CHANGERECT does not seem to set the proper size in the rect.
    // Let's make sure we fetch the actual size properly.
    ::GetWindowRect(fMainReBar, &availableBorderSpace);
    neededBorderSpace.left = 0;
    neededBorderSpace.top = availableBorderSpace.bottom - availableBorderSpace.top;
    if (!fLocked)
        neededBorderSpace.top += 3;
    neededBorderSpace.right = 0;
    neededBorderSpace.bottom = 0;

    CComPtr<IDockingWindowSite> dockingWindowSite;

    HRESULT hResult = fSite->QueryInterface(IID_PPV_ARG(IDockingWindowSite, &dockingWindowSite));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = dockingWindowSite->RequestBorderSpaceDW(static_cast<IDockingWindow *>(this), &neededBorderSpace);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    hResult = dockingWindowSite->SetBorderSpaceDW(static_cast<IDockingWindow *>(this), &neededBorderSpace);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetClassID(CLSID *pClassID)
{
    if (pClassID == NULL)
        return E_POINTER;
    *pClassID = CLSID_InternetToolbar;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::IsDirty()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::Load(IStream *pStm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::Save(IStream *pStm, BOOL fClearDirty)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::InitNew()
{
    CComPtr<IShellMenu>                     menuBar;
    CComPtr<IUnknown>                       logoBar;
    CComPtr<IUnknown>                       toolsBar;
    CComPtr<IUnknown>                       navigationBar;
    HRESULT                                 hResult;

    /* Create and attach the menubar to the rebar */
    hResult = CreateMenuBar(&menuBar);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    AddDockItem(menuBar, ITBBID_MENUBAND, CDockSite::ITF_NOTITLE | CDockSite::ITF_NEWBANDALWAYS | CDockSite::ITF_GRIPPERALWAYS);

    hResult = IUnknown_GetWindow(menuBar, &fMenuBandWindow);
    fMenuBar.Attach(menuBar.Detach());                  // transfer the ref count

    // FIXME: The ros Rebar does not properly support fixed-size items such as the brandband,
    // and it will put them in their own row, sized to take up the whole row.
#if 0
    /* Create and attach the brand/logo to the rebar */
    hResult = CBrandBand_CreateInstance(IID_PPV_ARG(IUnknown, &logoBar));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    AddDockItem(logoBar, ITBBID_BRANDBAND, CDockSite::ITF_NOGRIPPER | CDockSite::ITF_NOTITLE | CDockSite::ITF_FIXEDSIZE);
    fLogoBar.Attach(logoBar.Detach());                  // transfer the ref count
#endif

    /* Create and attach the standard toolbar to the rebar */
    hResult = CToolsBand_CreateInstance(IID_PPV_ARG(IUnknown, &toolsBar));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    AddDockItem(toolsBar, ITBBID_TOOLSBAND, CDockSite::ITF_NOTITLE | CDockSite::ITF_NEWBANDALWAYS | CDockSite::ITF_GRIPPERALWAYS);
    fControlsBar.Attach(toolsBar.Detach());             // transfer the ref count
    hResult = IUnknown_GetWindow(fControlsBar, &fToolbarWindow);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;

    /* Create and attach the address/navigation toolbar to the rebar */
    hResult = CAddressBand_CreateInstance(IID_PPV_ARG(IUnknown, &navigationBar));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    AddDockItem(navigationBar, ITBBID_ADDRESSBAND, CDockSite::ITF_NEWBANDALWAYS | CDockSite::ITF_GRIPPERALWAYS);
    fNavigationBar.Attach(navigationBar.Detach());
    hResult = IUnknown_GetWindow(fNavigationBar, &fNavigationWindow);

    return S_OK;
}

HRESULT CInternetToolbar::IsBandVisible(int BandID)
{
    int index = (int)SendMessage(fMainReBar, RB_IDTOINDEX, BandID, 0);

    REBARBANDINFOW bandInfo = {sizeof(REBARBANDINFOW), RBBIM_STYLE};
    SendMessage(fMainReBar, RB_GETBANDINFOW, index, (LPARAM)&bandInfo);

    return (bandInfo.fStyle & RBBS_HIDDEN) ? S_FALSE : S_OK;
}

HRESULT CInternetToolbar::ToggleBandVisibility(int BandID)
{
    int index = (int)SendMessage(fMainReBar, RB_IDTOINDEX, BandID, 0);

    REBARBANDINFOW bandInfo = {sizeof(REBARBANDINFOW), RBBIM_STYLE};
    SendMessage(fMainReBar, RB_GETBANDINFOW, index, (LPARAM)&bandInfo);

    if (bandInfo.fStyle & RBBS_HIDDEN)
        bandInfo.fStyle &= ~RBBS_HIDDEN;
    else
        bandInfo.fStyle |= RBBS_HIDDEN;

    SendMessage(fMainReBar, RB_SETBANDINFOW, index, (LPARAM)&bandInfo);

    ReserveBorderSpace(0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::QueryStatus(const GUID *pguidCmdGroup,
    ULONG cCmds, OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
    if (IsEqualIID(*pguidCmdGroup, CGID_PrivCITCommands))
    {
        while (cCmds != 0)
        {
            switch (prgCmds->cmdID)
            {
                case ITID_TEXTLABELS:       // Text Labels state
                    prgCmds->cmdf = OLECMDF_SUPPORTED;
                    break;
                case ITID_TOOLBARBANDSHOWN: // toolbar visibility
                    prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    if (IsBandVisible(ITBBID_TOOLSBAND) == S_OK)
                        prgCmds->cmdf |= OLECMDF_LATCHED;
                    break;
                case ITID_ADDRESSBANDSHOWN: // address bar visibility
                    prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    if (IsBandVisible(ITBBID_ADDRESSBAND) == S_OK)
                        prgCmds->cmdf |= OLECMDF_LATCHED;
                    break;
                case ITID_LINKSBANDSHOWN:   // links bar visibility
                    prgCmds->cmdf = 0;
                    break;
                case ITID_MENUBANDSHOWN:    // Menubar band visibility
                    prgCmds->cmdf = OLECMDF_SUPPORTED;
                    if (fMenuBar)
                        prgCmds->cmdf |= OLECMDF_LATCHED;
                    break;
                case ITID_AUTOHIDEENABLED:  // Auto hide enabled/disabled
                    prgCmds->cmdf = 0;
                    break;
                case ITID_CUSTOMIZEENABLED: // customize enabled
                    prgCmds->cmdf = OLECMDF_SUPPORTED;
                    break;
                case ITID_TOOLBARLOCKED:    // lock toolbars
                    prgCmds->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    if (fLocked)
                        prgCmds->cmdf |= OLECMDF_LATCHED;
                    break;
                default:
                    prgCmds->cmdf = 0;
                    break;
            }
            prgCmds++;
            cCmds--;
        }
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (IsEqualIID(*pguidCmdGroup, CGID_PrivCITCommands))
    {
        switch (nCmdID)
        {
            case 1:
                // what do I do here?
                return S_OK;
            case ITID_TEXTLABELS:
                // toggle text labels
                return S_OK;
            case ITID_TOOLBARBANDSHOWN:
                return ToggleBandVisibility(ITBBID_TOOLSBAND);
            case ITID_ADDRESSBANDSHOWN:
                return ToggleBandVisibility(ITBBID_ADDRESSBAND);
            case ITID_LINKSBANDSHOWN:
                // toggle links band visibility
                return S_OK;
            case ITID_CUSTOMIZEENABLED:
                // run customize
                return S_OK;
            case ITID_TOOLBARLOCKED:
                return LockUnlockToolbars(!fLocked);
        }
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetTypeInfoCount(UINT *pctinfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetIDsOfNames(REFIID riid, LPOLESTR *rgszNames, UINT cNames,
    LCID lcid, DISPID *rgDispId)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid,
    WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HRESULT                                 hResult;

    switch(dispIdMember)
    {
        case DISPID_BEFORENAVIGATE:
            hResult = S_OK;
            break;
        case DISPID_DOWNLOADCOMPLETE:
            hResult = S_OK;
            break;
        case DISPID_COMMANDSTATECHANGE:
            if (pDispParams->cArgs != 2)
                return E_INVALIDARG;
            if (V_VT(&pDispParams->rgvarg[0]) != VT_BOOL || V_VT(&pDispParams->rgvarg[1]) != VT_I4)
                return E_INVALIDARG;
            return CommandStateChanged(V_BOOL(&pDispParams->rgvarg[0]) != VARIANT_FALSE,
                V_I4(&pDispParams->rgvarg[1]));
        case DISPID_DOWNLOADBEGIN:
            hResult = S_OK;
            break;
        case DISPID_NAVIGATECOMPLETE2:
            hResult = S_OK;
            break;
        case DISPID_DOCUMENTCOMPLETE:
            hResult = S_OK;
            break;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetCommandTarget(IUnknown *theTarget, GUID *category, long param14)
{
    HRESULT                                 hResult;

    TRACE("SetCommandTarget %p category %s param %d\n", theTarget, wine_dbgstr_guid(category), param14);

    fCommandTarget.Release();
    hResult = theTarget->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &fCommandTarget));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    fCommandCategory = *category;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::Unknown1()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::AddButtons(const GUID *pguidCmdGroup, long buttonCount, TBBUTTON *buttons)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::AddString(const GUID *pguidCmdGroup,
    HINSTANCE param10, LPCTSTR param14, long *param18)
{
    long                                    result;

    result = (long)::SendMessage(fToolbarWindow, TB_ADDSTRINGW,
            reinterpret_cast<WPARAM>(param10), reinterpret_cast<LPARAM>(param14));
    *param18 = result;
    if (result == -1)
        return E_FAIL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetButton(const GUID *pguidCmdGroup, long param10, long param14)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetState(const GUID *pguidCmdGroup, long commandID, long *theState)
{
    if (theState == NULL)
        return E_POINTER;
    // map the command id
    *theState = (long)::SendMessage(fToolbarWindow, TB_GETSTATE, commandID, 0);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetState(const GUID *pguidCmdGroup, long commandID, long theState)
{
    // map the command id
    ::SendMessage(fToolbarWindow, TB_SETSTATE, commandID, MAKELONG(theState, 0));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::AddBitmap(const GUID *pguidCmdGroup, long param10, long buttonCount,
    TBADDBITMAP *lParam, long *newIndex, COLORREF param20)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetBitmapSize(long *paramC)
{
    if (paramC == NULL)
        return E_POINTER;
    *paramC = MAKELONG(24, 24);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SendToolbarMsg(const GUID *pguidCmdGroup, UINT uMsg,
    WPARAM wParam, LPARAM lParam, LRESULT *result)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetImageList(const GUID *pguidCmdGroup, HIMAGELIST param10,
    HIMAGELIST param14, HIMAGELIST param18)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::ModifyButton(const GUID *pguidCmdGroup, long param10, long param14)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::OnChange(LONG lEvent, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetSite(IUnknown *pUnkSite)
{
    CComPtr<IBrowserService>                browserService;
    HWND                                    ownerWindow;
    HWND                                    dockContainer;
    HRESULT                                 hResult;

    if (pUnkSite == NULL)
    {
        hResult = AtlUnadvise(fSite, DIID_DWebBrowserEvents, fAdviseCookie);
        ::DestroyWindow(fMainReBar);
        DestroyWindow();
        fSite.Release();
    }
    else
    {
        // get window handle of owner
        hResult = IUnknown_GetWindow(pUnkSite, &ownerWindow);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        if (ownerWindow == NULL)
            return E_FAIL;

        // create dock container
        fSite = pUnkSite;
        dockContainer = SHCreateWorkerWindowW(0, ownerWindow, 0,
            WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, NULL, 0);
        if (dockContainer == NULL)
            return E_FAIL;
        SubclassWindow(dockContainer);

        // create rebar in dock container
        DWORD style = WS_VISIBLE | WS_BORDER | WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
                      RBS_VARHEIGHT | RBS_BANDBORDERS | RBS_REGISTERDROP | RBS_AUTOSIZE | RBS_DBLCLKTOGGLE |
                      CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_TOP;
        DWORD exStyle = WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_TOOLWINDOW;
        fMainReBar = CreateWindowEx(exStyle, REBARCLASSNAMEW, NULL, style,
                            0, 0, 700, 60, dockContainer, NULL, _AtlBaseModule.GetModuleInstance(), NULL);
        if (fMainReBar == NULL)
            return E_FAIL;

        // take advice to watch events
        hResult = IUnknown_QueryService(pUnkSite, SID_SShellBrowser, IID_PPV_ARG(IBrowserService, &browserService));
        hResult = AtlAdvise(browserService, static_cast<IDispatch *>(this), DIID_DWebBrowserEvents, &fAdviseCookie);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetSite(REFIID riid, void **ppvSite)
{
    if (ppvSite == NULL)
        return E_POINTER;
    if (fSite.p != NULL)
        return fSite->QueryInterface(riid, ppvSite);
    *ppvSite = NULL;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    HRESULT                                 hResult;

    if (IsEqualIID(guidService, IID_IBandSite))
        return this->QueryInterface(riid, ppvObject);
    if (IsEqualIID(guidService, SID_IBandProxy))
    {
        if (fBandProxy.p == NULL)
        {
            hResult = CreateAndInitBandProxy();
            if (FAILED_UNEXPECTEDLY(hResult))
                return hResult;
        }
        return fBandProxy->QueryInterface(riid, ppvObject);
    }
    return IUnknown_QueryService(fSite, guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::OnWinEvent(
    HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    HRESULT                                 hResult;

    if (fMenuBar)
    {
        hResult = IUnknown_RelayWinEvent(fMenuBar, hWnd, uMsg, wParam, lParam, theResult);
        if (hResult != S_FALSE)
            return hResult;
    }

    if (fNavigationBar)
    {
        hResult = IUnknown_RelayWinEvent(fNavigationBar, hWnd, uMsg, wParam, lParam, theResult);
        if (hResult != S_FALSE)
            return hResult;
    }

    if (fLogoBar)
    {
        hResult = IUnknown_RelayWinEvent(fLogoBar, hWnd, uMsg, wParam, lParam, theResult);
        if (hResult != S_FALSE)
            return hResult;
    }

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::IsWindowOwner(HWND hWnd)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::AddBand(IUnknown *punk)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::EnumBands(UINT uBand, DWORD *pdwBandID)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::QueryBand(DWORD dwBandID,
    IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
    if (ppstb == NULL)
        return E_POINTER;
    if (dwBandID == ITBBID_MENUBAND && fMenuBar.p != NULL)
        return fMenuBar->QueryInterface(IID_PPV_ARG(IDeskBand, ppstb));
    //if (dwBandID == ITBBID_BRANDBAND && fLogoBar.p != NULL)
    //    return fLogoBar->QueryInterface(IID_PPV_ARG(IDeskBand, ppstb));
    *ppstb = NULL;
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetBandState(DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::RemoveBand(DWORD dwBandID)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetBandObject(DWORD dwBandID, REFIID riid, void **ppv)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::SetBandSiteInfo(const BANDSITEINFO *pbsinfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CInternetToolbar::GetBandSiteInfo(BANDSITEINFO *pbsinfo)
{
    UNIMPLEMENTED;
    return E_NOTIMPL;
}

LRESULT CInternetToolbar::OnTravelBack(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    CComPtr<IWebBrowser>                    webBrowser;
    HRESULT                                 hResult;

    hResult = IUnknown_QueryService(fSite, SID_SShellBrowser, IID_PPV_ARG(IWebBrowser, &webBrowser));
    if (FAILED_UNEXPECTEDLY(hResult))
        return 0;
    hResult = webBrowser->GoBack();
    return 1;
}

LRESULT CInternetToolbar::OnTravelForward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    CComPtr<IWebBrowser>                    webBrowser;
    HRESULT                                 hResult;

    hResult = IUnknown_QueryService(fSite, SID_SShellBrowser, IID_PPV_ARG(IWebBrowser, &webBrowser));
    if (FAILED_UNEXPECTEDLY(hResult))
        return 0;
    hResult = webBrowser->GoForward();
    return 1;
}

LRESULT CInternetToolbar::OnUpLevel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    IUnknown_Exec(fSite, CGID_ShellBrowser, IDM_GOTO_UPONELEVEL, 0, NULL, NULL);
    return 1;
}

LRESULT CInternetToolbar::OnSearch(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    IUnknown_Exec(fSite, CGID_Explorer, 0x1c, 1, NULL, NULL);
    return 1;
}

LRESULT CInternetToolbar::OnFolders(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    IUnknown_Exec(fSite, CGID_Explorer, 0x23, 0, NULL, NULL);
    return 1;
}

LRESULT CInternetToolbar::OnForwardToCommandTarget(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL &bHandled)
{
    HRESULT                                 hResult;

    if (fCommandTarget.p != NULL)
    {
        hResult = fCommandTarget->Exec(&fCommandCategory, wID, 0, NULL, NULL);
        if (FAILED(hResult))
        {
            ::SendMessageW(::GetParent(m_hWnd), WM_COMMAND, wID, 0);
        }
    }
    return 1;
}

LRESULT CInternetToolbar::OnMenuDropDown(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled)
{
    CComPtr<IBrowserService>                browserService;
    CComPtr<IOleCommandTarget>              commandTarget;
    CComPtr<ITravelLog>                     travelLog;
    NMTOOLBARW                              *notifyInfo;
    RECT                                    bounds;
    HMENU                                   newMenu;
    TPMPARAMS                               params;
    int                                     selectedItem;
    VARIANT                                 parmIn;
    OLECMD                                  commandInfo;
    HRESULT                                 hResult;
    wchar_t                                 templateString[200];

    notifyInfo = (NMTOOLBARW *)pNMHDR;
    if (notifyInfo->hdr.hwndFrom != fToolbarWindow)
    {
        // not from the toolbar, keep looking for a message handler
        bHandled = FALSE;
        return 0;
    }
    SendMessage(fToolbarWindow, TB_GETRECT, notifyInfo->iItem, reinterpret_cast<LPARAM>(&bounds));
    ::MapWindowPoints(fToolbarWindow, NULL, reinterpret_cast<POINT *>(&bounds), 2);
    switch (notifyInfo->iItem)
    {
        case IDM_GOTO_BACK:
            newMenu = CreatePopupMenu();
            hResult = IUnknown_QueryService(fSite, SID_SShellBrowser, IID_PPV_ARG(IBrowserService, &browserService));
            hResult = browserService->GetTravelLog(&travelLog);
            hResult = travelLog->InsertMenuEntries(browserService, newMenu, 0, 1, 9, TLMENUF_BACK);
            commandInfo.cmdID = 0x1d;
            hResult = IUnknown_QueryStatus(browserService, CGID_Explorer, 1, &commandInfo, NULL);
            if ((commandInfo.cmdf & (OLECMDF_ENABLED | OLECMDF_LATCHED)) == OLECMDF_ENABLED &&
                travelLog->CountEntries(browserService) > 1)
            {
                AppendMenuW(newMenu, MF_SEPARATOR, -1, L"");

                if (LoadStringW(_AtlBaseModule.GetResourceInstance(),
                                IDS_HISTORYTEXT, templateString, sizeof(templateString) / sizeof(wchar_t)) == 0)
                    StringCbCopyW(templateString, sizeof(templateString), L"&History\tCtrl+H");

                AppendMenuW(newMenu, MF_STRING /* | MF_OWNERDRAW */, IDM_EXPLORERBAR_HISTORY, templateString);
            }
            params.cbSize = sizeof(params);
            params.rcExclude = bounds;
            selectedItem = TrackPopupMenuEx(newMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
                                    bounds.left, bounds.bottom, m_hWnd, &params);
            if (selectedItem == IDM_EXPLORERBAR_HISTORY)
            {
                V_VT(&parmIn) = VT_I4;
                V_I4(&parmIn) = 1;
                Exec(&CGID_Explorer, 0x1d, 2, &parmIn, NULL);
            }
            else if (selectedItem != 0)
                hResult = travelLog->Travel(browserService, -selectedItem);
            DestroyMenu(newMenu);
            break;
        case IDM_GOTO_FORWARD:
            newMenu = CreatePopupMenu();
            hResult = IUnknown_QueryService(fSite, SID_SShellBrowser, IID_PPV_ARG(IBrowserService, &browserService));
            hResult = browserService->GetTravelLog(&travelLog);
            hResult = travelLog->InsertMenuEntries(browserService, newMenu, 0, 1, 9, TLMENUF_FORE);
            commandInfo.cmdID = 0x1d;
            hResult = IUnknown_QueryStatus(browserService, CGID_Explorer, 1, &commandInfo, NULL);
            if ((commandInfo.cmdf & (OLECMDF_ENABLED | OLECMDF_LATCHED)) == OLECMDF_ENABLED &&
                travelLog->CountEntries(browserService) > 1)
            {
                AppendMenuW(newMenu, MF_SEPARATOR, -1, L"");

                if (LoadStringW(_AtlBaseModule.GetResourceInstance(),
                                IDS_HISTORYTEXT, templateString, sizeof(templateString) / sizeof(wchar_t)) == 0)
                    StringCbCopyW(templateString, sizeof(templateString), L"&History\tCtrl+H");

                AppendMenuW(newMenu, MF_STRING /* | MF_OWNERDRAW */, IDM_EXPLORERBAR_HISTORY, templateString);
            }
            params.cbSize = sizeof(params);
            params.rcExclude = bounds;
            selectedItem = TrackPopupMenuEx(newMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
                                    bounds.left, bounds.bottom, m_hWnd, &params);
            if (selectedItem == IDM_EXPLORERBAR_HISTORY)
            {
                V_VT(&parmIn) = VT_I4;
                V_I4(&parmIn) = 1;
                Exec(&CGID_Explorer, 0x1d, 2, &parmIn, NULL);
            }
            else if (selectedItem != 0)
                hResult = travelLog->Travel(browserService, selectedItem);
            DestroyMenu(newMenu);
            break;
        case gViewsCommandID:
            VARIANT                     inValue;
            CComVariant                 outValue;
            HRESULT                     hResult;

            V_VT(&inValue) = VT_INT_PTR;
            V_INTREF(&inValue) = reinterpret_cast<INT *>(&bounds);

            if (fCommandTarget.p != NULL)
                hResult = fCommandTarget->Exec(&fCommandCategory, FCIDM_SHVIEW_AUTOARRANGE, 1, &inValue, &outValue);
            // pvaOut is VT_I4 with value 0x403
            break;
    }
    return TBDDRET_DEFAULT;
}

LRESULT CInternetToolbar::OnQueryInsert(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled)
{
    return 1;
}

LRESULT CInternetToolbar::OnQueryDelete(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled)
{
    return 1;
}

LRESULT CInternetToolbar::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HMENU                                   contextMenuBar;
    HMENU                                   contextMenu;
    POINT                                   clickLocation;
    int                                     command;
    RBHITTESTINFO                           hitTestInfo;
    REBARBANDINFOW                          rebarBandInfo;
    int                                     bandID;
    BOOL                                    goButtonChecked;

    clickLocation.x = LOWORD(lParam);
    clickLocation.y = HIWORD(lParam);
    hitTestInfo.pt = clickLocation;
    ScreenToClient(&hitTestInfo.pt);
    SendMessage(fMainReBar, RB_HITTEST, 0, (LPARAM)&hitTestInfo);
    if (hitTestInfo.iBand == -1)
        return 0;
    rebarBandInfo.cbSize = sizeof(rebarBandInfo);
    rebarBandInfo.fMask = RBBIM_ID;
    SendMessage(fMainReBar, RB_GETBANDINFOW, hitTestInfo.iBand, (LPARAM)&rebarBandInfo);
    bandID = rebarBandInfo.wID;
    contextMenuBar = LoadMenu(_AtlBaseModule.GetResourceInstance(), MAKEINTRESOURCE(IDM_CABINET_CONTEXTMENU));
    contextMenu = GetSubMenu(contextMenuBar, 0);
    switch (bandID)
    {
        case ITBBID_MENUBAND:   // menu band
            DeleteMenu(contextMenu, IDM_TOOLBARS_CUSTOMIZE, MF_BYCOMMAND);
            DeleteMenu(contextMenu, IDM_TOOLBARS_TEXTLABELS, MF_BYCOMMAND);
            DeleteMenu(contextMenu, IDM_TOOLBARS_GOBUTTON, MF_BYCOMMAND);
            break;
        case ITBBID_BRANDBAND:  // brand band
            DeleteMenu(contextMenu, IDM_TOOLBARS_CUSTOMIZE, MF_BYCOMMAND);
            DeleteMenu(contextMenu, IDM_TOOLBARS_TEXTLABELS, MF_BYCOMMAND);
            DeleteMenu(contextMenu, IDM_TOOLBARS_GOBUTTON, MF_BYCOMMAND);
            break;
        case ITBBID_TOOLSBAND:  // tools band
            DeleteMenu(contextMenu, IDM_TOOLBARS_TEXTLABELS, MF_BYCOMMAND);
            DeleteMenu(contextMenu, IDM_TOOLBARS_GOBUTTON, MF_BYCOMMAND);
            break;
        case ITBBID_ADDRESSBAND:    // navigation band
            DeleteMenu(contextMenu, IDM_TOOLBARS_CUSTOMIZE, MF_BYCOMMAND);
            DeleteMenu(contextMenu, IDM_TOOLBARS_TEXTLABELS, MF_BYCOMMAND);
            break;
        default:
            break;
    }

    SHEnableMenuItem(contextMenu, IDM_TOOLBARS_LINKSBAR, FALSE);

    SHCheckMenuItem(contextMenu, IDM_TOOLBARS_STANDARDBUTTONS, IsBandVisible(ITBBID_TOOLSBAND) == S_OK);
    SHCheckMenuItem(contextMenu, IDM_TOOLBARS_ADDRESSBAR, IsBandVisible(ITBBID_ADDRESSBAND) == S_OK);
    SHCheckMenuItem(contextMenu, IDM_TOOLBARS_LINKSBAR, FALSE);
    SHCheckMenuItem(contextMenu, IDM_TOOLBARS_CUSTOMIZE, FALSE);
    SHCheckMenuItem(contextMenu, IDM_TOOLBARS_LOCKTOOLBARS, fLocked);
    goButtonChecked = SHRegGetBoolUSValueW(L"Software\\Microsoft\\Internet Explorer\\Main", L"ShowGoButton", FALSE, TRUE);
    SHCheckMenuItem(contextMenu, IDM_TOOLBARS_GOBUTTON, goButtonChecked);

    // TODO: use GetSystemMetrics(SM_MENUDROPALIGNMENT) to determine menu alignment
    command = TrackPopupMenu(contextMenu, TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON | TPM_RETURNCMD,
                clickLocation.x, clickLocation.y, 0, m_hWnd, NULL);
    switch (command)
    {
        case IDM_TOOLBARS_STANDARDBUTTONS:  // standard buttons
            ToggleBandVisibility(ITBBID_TOOLSBAND);
            break;
        case IDM_TOOLBARS_ADDRESSBAR:   // address bar
            ToggleBandVisibility(ITBBID_ADDRESSBAND);
            break;
        case IDM_TOOLBARS_LINKSBAR: // links
            break;
        case IDM_TOOLBARS_LOCKTOOLBARS: // lock the toolbars
            LockUnlockToolbars(!fLocked);
            break;
        case IDM_TOOLBARS_CUSTOMIZE:    // customize
            SendMessage(fToolbarWindow, TB_CUSTOMIZE, 0, 0);
            break;
        case IDM_TOOLBARS_GOBUTTON:
            SendMessage(fNavigationWindow, WM_COMMAND, IDM_TOOLBARS_GOBUTTON, 0);
            break;
    }

    DestroyMenu(contextMenuBar);
    return 1;
}

LRESULT CInternetToolbar::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (wParam != SIZE_MINIMIZED)
    {
        ::SetWindowPos(fMainReBar, NULL, 0, 0, LOWORD(lParam), HIWORD(lParam),
            SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
    }
    return 1;
}

LRESULT CInternetToolbar::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if ((short)lParam != HTCLIENT || (HWND)wParam != m_hWnd)
    {
        bHandled = FALSE;
        return 0;
    }
    SetCursor(LoadCursor(NULL, IDC_SIZENS));
    return 1;
}

LRESULT CInternetToolbar::OnTipText(UINT idControl, NMHDR *pNMHDR, BOOL &bHandled)
{
    CComPtr<IBrowserService>                browserService;
    CComPtr<ITravelLog>                     travelLog;
    TOOLTIPTEXTW                            *pTTTW;
    UINT                                    nID;
    HRESULT                                 hResult;
    wchar_t                                 tempString[300];

    pTTTW = reinterpret_cast<TOOLTIPTEXTW *>(pNMHDR);
    if ((pTTTW->uFlags & TTF_IDISHWND) != 0)
        nID = ::GetDlgCtrlID((HWND)pNMHDR->idFrom);
    else
        nID = (UINT)pNMHDR->idFrom;

    if (nID != 0)
    {
        if (nID == (UINT)IDM_GOTO_BACK || nID == (UINT)IDM_GOTO_FORWARD)
        {
            // TODO: Should this call QueryService?
            hResult = fSite->QueryInterface(IID_PPV_ARG(IBrowserService, &browserService));
            hResult = browserService->GetTravelLog(&travelLog);
            hResult = travelLog->GetToolTipText(browserService,
                (nID == (UINT)IDM_GOTO_BACK) ? TLOG_BACK : TLOG_FORE,
                0, tempString, 299);
            if (FAILED_UNEXPECTEDLY(hResult))
            {
                bHandled = FALSE;
                return 0;
            }
        }
        else
            tempString[0] = 0;
        wcsncpy (pTTTW->szText, tempString, sizeof(pTTTW->szText) / sizeof(wchar_t));
        ::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
            SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
        return 0;
    }
    return 0;
}

LRESULT CInternetToolbar::OnCommand(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    LRESULT theResult;
    HRESULT hResult;

    hResult = OnWinEvent((HWND) lParam, uMsg, wParam, lParam, &theResult);

    bHandled = hResult == S_OK;

    return FAILED_UNEXPECTEDLY(hResult) ? 0 : theResult;
}
LRESULT CInternetToolbar::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    NMHDR   *notifyHeader;
    LRESULT theResult;
    HRESULT hResult;

    notifyHeader = reinterpret_cast<NMHDR *>(lParam);

    hResult = OnWinEvent(notifyHeader->hwndFrom, uMsg, wParam, lParam, &theResult);

    bHandled = hResult == S_OK;

    return FAILED_UNEXPECTEDLY(hResult) ? 0 : theResult;
}

LRESULT CInternetToolbar::OnLDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = FALSE;
    if (fLocked)
        return 0;

    if (wParam & MK_CONTROL)
        return 0;

    fSizing = TRUE;

    DWORD msgp = GetMessagePos();

    fStartPosition.x = GET_X_LPARAM(msgp);
    fStartPosition.y = GET_Y_LPARAM(msgp);

    RECT rc;
    GetWindowRect(&rc);

    fStartHeight = rc.bottom - rc.top;

    SetCapture();

    bHandled = TRUE;
    return 0;
}

LRESULT CInternetToolbar::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = FALSE;
    if (!fSizing)
        return 0;

    DWORD msgp = GetMessagePos();

    POINT pt;
    pt.x = GET_X_LPARAM(msgp);
    pt.y = GET_Y_LPARAM(msgp);

    ReserveBorderSpace(fStartHeight - fStartPosition.y + pt.y);

    bHandled = TRUE;
    return 0;
}

LRESULT CInternetToolbar::OnLUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = FALSE;
    if (!fSizing)
        return 0;

    OnMouseMove(uMsg, wParam, lParam, bHandled);

    fSizing = FALSE;

    ReleaseCapture();

    return 0;
}

LRESULT CInternetToolbar::OnWinIniChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HRESULT hr;
    HWND hwndMenu;

    hr = IUnknown_GetWindow(fMenuBar, &hwndMenu);
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    CComPtr<IWinEventHandler> menuWinEventHandler;
    hr = fMenuBar->QueryInterface(IID_PPV_ARG(IWinEventHandler, &menuWinEventHandler));
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    LRESULT lres;
    hr = menuWinEventHandler->OnWinEvent(hwndMenu, uMsg, wParam, lParam, &lres);
    if (FAILED_UNEXPECTEDLY(hr))
        return 0;

    return lres;
}
