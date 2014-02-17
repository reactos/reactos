/*
* Shell Menu Band
*
* Copyright 2014 David Quintana
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
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
*/
#include "precomp.h"
#include "wraplog.h"
#include <windowsx.h>

WINE_DEFAULT_DEBUG_CHANNEL(CMenuBand);

#define WRAP_LOG 0

#define TBSTYLE_EX_VERTICAL 4

#define TIMERID_HOTTRACK 1
#define SUBCLASS_ID_MENUBAND 1

extern "C" BOOL WINAPI Shell_GetImageLists(HIMAGELIST * lpBigList, HIMAGELIST * lpSmallList);

class CMenuBand;

class CMenuToolbarBase
{
public:
    CMenuToolbarBase(CMenuBand *menuBand);
    virtual ~CMenuToolbarBase() {}

    HRESULT CreateToolbar(HWND hwndParent, DWORD dwFlags);
    HRESULT GetWindow(HWND *phwnd);
    HRESULT ShowWindow(BOOL fShow);
    HRESULT Close();

    BOOL IsWindowOwner(HWND hwnd) { return m_hwnd && m_hwnd == hwnd; }

    virtual HRESULT FillToolbar() = 0;
    virtual HRESULT PopupItem(UINT uItem) = 0;
    virtual HRESULT HasSubMenu(UINT uItem) = 0;
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult) = 0;

    HRESULT OnHotItemChange(const NMTBHOTITEM * hot);

    static LRESULT CALLBACK s_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
protected:

    static const UINT WM_USER_SHOWPOPUPMENU = WM_USER + 1;

    LRESULT CALLBACK SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    CMenuBand *m_menuBand;
    HWND m_hwnd;
    DWORD m_dwMenuFlags;
    UINT m_hotItem;
    WNDPROC m_SubclassOld;
};

class CMenuStaticToolbar : public CMenuToolbarBase
{
public:
    CMenuStaticToolbar(CMenuBand *menuBand);
    virtual ~CMenuStaticToolbar() {}

    HRESULT SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);
    HRESULT GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags);

    virtual HRESULT FillToolbar();
    virtual HRESULT PopupItem(UINT uItem);
    virtual HRESULT HasSubMenu(UINT uItem);
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);
private:
    HMENU m_hmenu;
};

class CMenuSFToolbar : public CMenuToolbarBase
{
public:
    CMenuSFToolbar(CMenuBand *menuBand);
    virtual ~CMenuSFToolbar();

    HRESULT SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags);
    HRESULT GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv);

    virtual HRESULT FillToolbar();
    virtual HRESULT PopupItem(UINT uItem);
    virtual HRESULT HasSubMenu(UINT uItem);
    virtual HRESULT OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult);


private:

    IShellFolder * m_shellFolder;
    LPCITEMIDLIST m_idList;
    HKEY m_hKey;
};

class CMenuBand :
    public CComCoClass<CMenuBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IDeskBand,
    public IObjectWithSite,
    public IInputObject,
    public IPersistStream,
    public IOleCommandTarget,
    public IServiceProvider,
    public IMenuPopup,
    public IMenuBand,
    public IShellMenu2,
    public IWinEventHandler,
    public IShellMenuAcc
{
public:
    CMenuBand();
    ~CMenuBand();

private:
#if WRAP_LOG
    IUnknown          * m_IUnknown;
    IDeskBand         * m_IDeskBand;
    IDockingWindow    * m_IDockingWindow;
    IOleWindow        * m_IOleWindow;
    IObjectWithSite   * m_IObjectWithSite;
    IInputObject      * m_IInputObject;
    IPersistStream    * m_IPersistStream;
    IPersist          * m_IPersist;
    IOleCommandTarget * m_IOleCommandTarget;
    IServiceProvider  * m_IServiceProvider;
    IMenuPopup        * m_IMenuPopup;
    IDeskBar          * m_IDeskBar;
    IMenuBand         * m_IMenuBand;
    IShellMenu2       * m_IShellMenu2;
    IShellMenu        * m_IShellMenu;
    IWinEventHandler  * m_IWinEventHandler;
    IShellMenuAcc     * m_IShellMenuAcc;
#else
    IOleWindow *m_site;
    IShellMenuCallback *m_psmc;

    CMenuStaticToolbar *m_staticToolbar;
    CMenuSFToolbar *m_SFToolbar;

    UINT m_uId;
    UINT m_uIdAncestor;
    DWORD m_dwFlags;
    PVOID m_UserData;
    HMENU m_hmenu;
#endif

    BOOL m_useBigIcons;

public:

    // *** IDeskBand methods ***
    virtual HRESULT STDMETHODCALLTYPE GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi);

    // *** IDockingWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE ShowDW(BOOL fShow);
    virtual HRESULT STDMETHODCALLTYPE CloseDW(DWORD dwReserved);
    virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, PVOID *ppvSite);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL fActivate, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO();
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IPersistStream methods ***
    virtual HRESULT STDMETHODCALLTYPE IsDirty();
    virtual HRESULT STDMETHODCALLTYPE Load(IStream *pStm);
    virtual HRESULT STDMETHODCALLTYPE Save(IStream *pStm, BOOL fClearDirty);
    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(ULARGE_INTEGER *pcbSize);

    // *** IPersist methods ***
    virtual HRESULT STDMETHODCALLTYPE GetClassID(CLSID *pClassID);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IMenuPopup methods ***
    virtual HRESULT STDMETHODCALLTYPE Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags);
    virtual HRESULT STDMETHODCALLTYPE OnSelect(DWORD dwSelectType);
    virtual HRESULT STDMETHODCALLTYPE SetSubMenu(IMenuPopup *pmp, BOOL fSet);

    // *** IDeskBar methods ***
    virtual HRESULT STDMETHODCALLTYPE SetClient(IUnknown *punkClient);
    virtual HRESULT STDMETHODCALLTYPE GetClient(IUnknown **ppunkClient);
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(RECT *prc);

    // *** IMenuBand methods ***
    virtual HRESULT STDMETHODCALLTYPE IsMenuMessage(MSG *pmsg);
    virtual HRESULT STDMETHODCALLTYPE TranslateMenuMessage(MSG *pmsg, LRESULT *plRet);

    // *** IShellMenu methods ***
    virtual HRESULT STDMETHODCALLTYPE Initialize(IShellMenuCallback *psmc, UINT uId, UINT uIdAncestor, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetMenuInfo(IShellMenuCallback **ppsmc, UINT *puId, UINT *puIdAncestor, DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv);
    virtual HRESULT STDMETHODCALLTYPE SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags);
    virtual HRESULT STDMETHODCALLTYPE InvalidateItem(LPSMDATA psmd, DWORD dwFlags);
    virtual HRESULT STDMETHODCALLTYPE GetState(LPSMDATA psmd);
    virtual HRESULT STDMETHODCALLTYPE SetMenuToolbar(IUnknown *punk, DWORD dwFlags);

    // *** IWinEventHandler methods ***
    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult);
    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd);

    // *** IShellMenu2 methods ***
    virtual HRESULT STDMETHODCALLTYPE GetSubMenu(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetToolbar(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetMinWidth(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetNoBorder(THIS);
    virtual HRESULT STDMETHODCALLTYPE SetTheme(THIS);

    // *** IShellMenuAcc methods ***
    virtual HRESULT STDMETHODCALLTYPE GetTop(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetBottom(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetTracked(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetParentSite(THIS);
    virtual HRESULT STDMETHODCALLTYPE GetState(THIS);
    virtual HRESULT STDMETHODCALLTYPE DoDefaultAction(THIS);
    virtual HRESULT STDMETHODCALLTYPE IsEmpty(THIS);

    HRESULT CallCBWithId(UINT Id, UINT uMsg, WPARAM wParam, LPARAM lParam);

    BOOL UseBigIcons() {
        return m_useBigIcons;
    }

    DECLARE_NOT_AGGREGATABLE(CMenuBand)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMenuBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBar, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenu, IShellMenu)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDockingWindow, IDockingWindow)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IMenuBand, IMenuBand)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenu2, IShellMenu2)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IShellMenuAcc, IShellMenuAcc)
    END_COM_MAP()

private:
    HRESULT _CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

extern "C"
HRESULT CMenuBand_Constructor(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    CMenuBand * site = new CComObject<CMenuBand>();

    if (!site)
        return E_OUTOFMEMORY;

    HRESULT hr = site->QueryInterface(riid, ppv);

    if (FAILED(hr))
        site->Release();

    return hr;
}


#if WRAP_LOG
CMenuBand::CMenuBand()
{
    HRESULT hr;
    WrapLogOpen();

    hr = CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellMenu, &m_IShellMenu));
    hr = m_IShellMenu->QueryInterface(IID_PPV_ARG(IUnknown, &m_IUnknown));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IDeskBand, &m_IDeskBand));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IDockingWindow, &m_IDockingWindow));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IOleWindow, &m_IOleWindow));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IObjectWithSite, &m_IObjectWithSite));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IInputObject, &m_IInputObject));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IPersistStream, &m_IPersistStream));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IPersist, &m_IPersist));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &m_IOleCommandTarget));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IServiceProvider, &m_IServiceProvider));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IMenuPopup, &m_IMenuPopup));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IDeskBar, &m_IDeskBar));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IMenuBand, &m_IMenuBand));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IShellMenu2, &m_IShellMenu2));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IWinEventHandler, &m_IWinEventHandler));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IShellMenuAcc, &m_IShellMenuAcc));
}

CMenuBand::~CMenuBand()
{
    m_IUnknown->Release();
    m_IDeskBand->Release();
    m_IDockingWindow->Release();
    m_IOleWindow->Release();
    m_IObjectWithSite->Release();
    m_IInputObject->Release();
    m_IPersistStream->Release();
    m_IPersist->Release();
    m_IOleCommandTarget->Release();
    m_IServiceProvider->Release();
    m_IMenuPopup->Release();
    m_IDeskBar->Release();
    m_IMenuBand->Release();
    m_IShellMenu2->Release();
    m_IShellMenu->Release();
    m_IWinEventHandler->Release();
    m_IShellMenuAcc->Release();
    WrapLogClose();
}


// *** IShellMenu2 methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::GetSubMenu(THIS)
{
    WrapLogEnter("CMenuBand<%p>::GetSubMenu()\n", this);
    HRESULT hr = m_IShellMenu2->GetSubMenu();
    WrapLogExit("CMenuBand::GetSubMenu() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetToolbar(THIS)
{
    WrapLogEnter("CMenuBand<%p>::SetToolbar()\n", this);
    HRESULT hr = m_IShellMenu2->SetToolbar();
    WrapLogExit("CMenuBand::SetToolbar() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetMinWidth(THIS)
{
    WrapLogEnter("CMenuBand<%p>::SetMinWidth()\n", this);
    HRESULT hr = m_IShellMenu2->SetMinWidth();
    WrapLogExit("CMenuBand::SetMinWidth() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetNoBorder(THIS)
{
    WrapLogEnter("CMenuBand<%p>::SetNoBorder()\n", this);
    HRESULT hr = m_IShellMenu2->SetNoBorder();
    WrapLogExit("CMenuBand::SetNoBorder() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetTheme(THIS)
{
    WrapLogEnter("CMenuBand<%p>::SetTheme()\n", this);
    HRESULT hr = m_IShellMenu2->SetTheme();
    WrapLogExit("CMenuBand::SetTheme() = %08x\n", hr);
    return hr;
}


// *** IShellMenuAcc methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::GetTop(THIS)
{
    WrapLogEnter("CMenuBand<%p>::GetTop()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetTop();
    WrapLogExit("CMenuBand::GetTop() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetBottom(THIS)
{
    WrapLogEnter("CMenuBand<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBand::GetBottom() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetTracked(THIS)
{
    WrapLogEnter("CMenuBand<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBand::GetBottom() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetParentSite(THIS)
{
    WrapLogEnter("CMenuBand<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBand::GetBottom() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetState(THIS)
{
    WrapLogEnter("CMenuBand<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBand::GetBottom() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::DoDefaultAction(THIS)
{
    WrapLogEnter("CMenuBand<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBand::GetBottom() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsEmpty(THIS)
{
    WrapLogEnter("CMenuBand<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBand::GetBottom() = %08x\n", hr);
    return hr;
}

// *** IDeskBand methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    WrapLogEnter("CMenuBand<%p>::GetBandInfo(DWORD dwBandID=%d, DWORD dwViewMode=%d, DESKBANDINFO *pdbi=%p)\n", this, dwBandID, dwViewMode, pdbi);
    HRESULT hr = m_IDeskBand->GetBandInfo(dwBandID, dwViewMode, pdbi);
    WrapLogExit("CMenuBand::GetBandInfo() = %08x\n", hr);
    return hr;
}

// *** IDockingWindow methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::ShowDW(BOOL fShow)
{
    WrapLogEnter("CMenuBand<%p>::ShowDW(BOOL fShow=%d)\n", this, fShow);
    HRESULT hr = m_IDockingWindow->ShowDW(fShow);
    WrapLogExit("CMenuBand::ShowDW() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::CloseDW(DWORD dwReserved)
{
    WrapLogEnter("CMenuBand<%p>::CloseDW(DWORD dwReserved=%d)\n", this, dwReserved);
    HRESULT hr = m_IDockingWindow->CloseDW(dwReserved);
    WrapLogExit("CMenuBand::CloseDW() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    WrapLogEnter("CMenuBand<%p>::ResizeBorderDW(LPCRECT prcBorder=%p, IUnknown *punkToolbarSite=%p, BOOL fReserved=%d)\n", this, prcBorder, punkToolbarSite, fReserved);
    if (prcBorder) WrapLogMsg("*prcBorder=%s\n", Wrap(*prcBorder));
    HRESULT hr = m_IDockingWindow->ResizeBorderDW(prcBorder, punkToolbarSite, fReserved);
    if (prcBorder) WrapLogMsg("*prcBorder=%s\n", Wrap(*prcBorder));
    WrapLogExit("CMenuBand::ResizeBorderDW() = %08x\n", hr);
    return hr;
}

// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::GetWindow(HWND *phwnd)
{
    WrapLogEnter("CMenuBand<%p>::GetWindow(HWND *phwnd=%p)\n", this, phwnd);
    HRESULT hr = m_IOleWindow->GetWindow(phwnd);
    if (phwnd) WrapLogMsg("*phwnd=%p\n", *phwnd);
    WrapLogExit("CMenuBand::GetWindow() = %08x\n", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    WrapLogEnter("CMenuBand<%p>::ContextSensitiveHelp(BOOL fEnterMode=%d)\n", this, fEnterMode);
    HRESULT hr = m_IOleWindow->ContextSensitiveHelp(fEnterMode);
    WrapLogExit("CMenuBand::ContextSensitiveHelp() = %08x\n", hr);
    return hr;
}

// *** IWinEventHandler methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    //WrapLogEnter("CMenuBand<%p>::OnWinEvent(HWND hWnd=%p, UINT uMsg=%u, WPARAM wParam=%08x, LPARAM lParam=%08x, LRESULT *theResult=%p)\n", this, hWnd, uMsg, wParam, lParam, theResult);
    HRESULT hr = m_IWinEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
    //WrapLogExit("CMenuBand::OnWinEvent() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsWindowOwner(HWND hWnd)
{
    //WrapLogEnter("CMenuBand<%p>::IsWindowOwner(HWND hWnd=%08x)\n", this, hWnd);
    HRESULT hr = m_IWinEventHandler->IsWindowOwner(hWnd);
    //WrapLogExit("CMenuBand::IsWindowOwner() = %08x\n", hr);
    return hr;
}

// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::SetSite(IUnknown *pUnkSite)
{
    WrapLogEnter("CMenuBand<%p>::SetSite(IUnknown *pUnkSite=%p)\n", this, pUnkSite);
    HRESULT hr = m_IObjectWithSite->SetSite(pUnkSite);
    WrapLogExit("CMenuBand::SetSite() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetSite(REFIID riid, PVOID *ppvSite)
{
    WrapLogEnter("CMenuBand<%p>::GetSite(REFIID riid=%s, PVOID *ppvSite=%p)\n", this, Wrap(riid), ppvSite);
    HRESULT hr = m_IObjectWithSite->GetSite(riid, ppvSite);
    if (ppvSite) WrapLogMsg("*ppvSite=%p\n", *ppvSite);
    WrapLogExit("CMenuBand::GetSite() = %08x\n", hr);
    return hr;
}

// *** IInputObject methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    WrapLogEnter("CMenuBand<%p>::UIActivateIO(BOOL fActivate=%d, LPMSG lpMsg=%p)\n", this, fActivate, lpMsg);
    HRESULT hr = m_IInputObject->UIActivateIO(fActivate, lpMsg);
    WrapLogExit("CMenuBand::UIActivateIO() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::HasFocusIO()
{
    WrapLogEnter("CMenuBand<%p>::HasFocusIO()\n", this);
    HRESULT hr = m_IInputObject->HasFocusIO();
    WrapLogExit("CMenuBand::HasFocusIO() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    WrapLogEnter("CMenuBand<%p>::TranslateAcceleratorIO(LPMSG lpMsg=%p)\n", this, lpMsg);
    if (lpMsg) WrapLogMsg("*lpMsg=%s\n", Wrap(*lpMsg));
    HRESULT hr = m_IInputObject->TranslateAcceleratorIO(lpMsg);
    WrapLogExit("CMenuBand::TranslateAcceleratorIO() = %08x\n", hr);
    return hr;
}

// *** IPersistStream methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::IsDirty()
{
    WrapLogEnter("CMenuBand<%p>::IsDirty()\n", this);
    HRESULT hr = m_IPersistStream->IsDirty();
    WrapLogExit("CMenuBand::IsDirty() = %08x\n", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBand::Load(IStream *pStm)
{
    WrapLogEnter("CMenuBand<%p>::Load(IStream *pStm=%p)\n", this, pStm);
    HRESULT hr = m_IPersistStream->Load(pStm);
    WrapLogExit("CMenuBand::Load() = %08x\n", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBand::Save(IStream *pStm, BOOL fClearDirty)
{
    WrapLogEnter("CMenuBand<%p>::Save(IStream *pStm=%p, BOOL fClearDirty=%d)\n", this, pStm, fClearDirty);
    HRESULT hr = m_IPersistStream->Save(pStm, fClearDirty);
    WrapLogExit("CMenuBand::Save() = %08x\n", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    WrapLogEnter("CMenuBand<%p>::GetSizeMax(ULARGE_INTEGER *pcbSize=%p)\n", this, pcbSize);
    HRESULT hr = m_IPersistStream->GetSizeMax(pcbSize);
    WrapLogExit("CMenuBand::GetSizeMax() = %08x\n", hr);
    return hr;
}

// *** IPersist methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::GetClassID(CLSID *pClassID)
{
    WrapLogEnter("CMenuBand<%p>::GetClassID(CLSID *pClassID=%p)\n", this, pClassID);
    HRESULT hr = m_IPersist->GetClassID(pClassID);
    if (pClassID) WrapLogMsg("*pClassID=%s\n", Wrap(*pClassID));
    WrapLogExit("CMenuBand::GetClassID() = %08x\n", hr);
    return hr;
}

// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    WrapLogEnter("CMenuBand<%p>::QueryStatus(const GUID *pguidCmdGroup=%p, ULONG cCmds=%u, prgCmds=%p, pCmdText=%p)\n", this, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    HRESULT hr = m_IOleCommandTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    if (pguidCmdGroup) WrapLogMsg("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    WrapLogExit("CMenuBand::QueryStatus() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    bool b;

    WrapLogEnter("CMenuBand<%p>::Exec(const GUID *pguidCmdGroup=%p, DWORD nCmdID=%d, DWORD nCmdexecopt=%d, VARIANT *pvaIn=%p, VARIANT *pvaOut=%p)\n", this, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);

    if (pguidCmdGroup && IsEqualGUID(*pguidCmdGroup, CLSID_MenuBand))
    {
        if (nCmdID == 19) // popup
        {
            b = true;
        }
    }


    if (pguidCmdGroup) WrapLogMsg("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    HRESULT hr = m_IOleCommandTarget->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    WrapLogExit("CMenuBand::Exec() = %08x\n", hr);
    return hr;
}

// *** IServiceProvider methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    WrapLogEnter("CMenuBand<%p>::QueryService(REFGUID guidService=%s, REFIID riid=%s, void **ppvObject=%p)\n", this, Wrap(guidService), Wrap(riid), ppvObject);

    if (IsEqualIID(guidService, SID_SMenuBandChild))
    {
        WrapLogMsg("SID is SID_SMenuBandChild. Using QueryInterface of self instead of wrapped object.\n");
        HRESULT hr = this->QueryInterface(riid, ppvObject);
        if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
        WrapLogExit("CMenuBand::QueryService() = %08x\n", hr);
        return hr;
    }
    else
    {
        WrapLogMsg("SID not identified.\n");
    }
    HRESULT hr = m_IServiceProvider->QueryService(guidService, riid, ppvObject);
    if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
    WrapLogExit("CMenuBand::QueryService() = %08x\n", hr);
    return hr;
}


// *** IMenuPopup methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    WrapLogEnter("CMenuBand<%p>::Popup(POINTL *ppt=%p, RECTL *prcExclude=%p, MP_POPUPFLAGS dwFlags=%08x)\n", this, ppt, prcExclude, dwFlags);
    HRESULT hr = m_IMenuPopup->Popup(ppt, prcExclude, dwFlags);
    WrapLogExit("CMenuBand::Popup() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnSelect(DWORD dwSelectType)
{
    WrapLogEnter("CMenuBand<%p>::OnSelect(DWORD dwSelectType=%08x)\n", this, dwSelectType);
    HRESULT hr = m_IMenuPopup->OnSelect(dwSelectType);
    WrapLogExit("CMenuBand::OnSelect() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    WrapLogEnter("CMenuBand<%p>::SetSubMenu(IMenuPopup *pmp=%p, BOOL fSet=%d)\n", this, pmp, fSet);
    HRESULT hr = m_IMenuPopup->SetSubMenu(pmp, fSet);
    WrapLogExit("CMenuBand::SetSubMenu() = %08x\n", hr);
    return hr;
}


// *** IDeskBar methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::SetClient(IUnknown *punkClient)
{
    WrapLogEnter("CMenuBand<%p>::SetClient(IUnknown *punkClient=%p)\n", this, punkClient);
    HRESULT hr = m_IDeskBar->SetClient(punkClient);
    WrapLogExit("CMenuBand::SetClient() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetClient(IUnknown **ppunkClient)
{
    WrapLogEnter("CMenuBand<%p>::GetClient(IUnknown **ppunkClient=%p)\n", this, ppunkClient);
    HRESULT hr = m_IDeskBar->GetClient(ppunkClient);
    if (ppunkClient) WrapLogMsg("*ppunkClient=%p\n", *ppunkClient);
    WrapLogExit("CMenuBand::GetClient() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnPosRectChangeDB(RECT *prc)
{
    WrapLogEnter("CMenuBand<%p>::OnPosRectChangeDB(RECT *prc=%p)\n", this, prc);
    HRESULT hr = m_IDeskBar->OnPosRectChangeDB(prc);
    if (prc) WrapLogMsg("*prc=%s\n", Wrap(*prc));
    WrapLogExit("CMenuBand::OnPosRectChangeDB() = %08x\n", hr);
    return hr;
}


// *** IMenuBand methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::IsMenuMessage(MSG *pmsg)
{
    //WrapLogEnter("CMenuBand<%p>::IsMenuMessage(MSG *pmsg=%p)\n", this, pmsg);
    HRESULT hr = m_IMenuBand->IsMenuMessage(pmsg);
    //WrapLogExit("CMenuBand::IsMenuMessage() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateMenuMessage(MSG *pmsg, LRESULT *plRet)
{
    //WrapLogEnter("CMenuBand<%p>::TranslateMenuMessage(MSG *pmsg=%p, LRESULT *plRet=%p)\n", this, pmsg, plRet);
    HRESULT hr = m_IMenuBand->TranslateMenuMessage(pmsg, plRet);
    //WrapLogExit("CMenuBand::TranslateMenuMessage(*plRet=%d) = %08x\n", *plRet, hr);
    return hr;
}

// *** IShellMenu methods ***
HRESULT STDMETHODCALLTYPE CMenuBand::Initialize(IShellMenuCallback *psmc, UINT uId, UINT uIdAncestor, DWORD dwFlags)
{
    WrapLogEnter("CMenuBand<%p>::Initialize(IShellMenuCallback *psmc=%p, UINT uId=%u, UINT uIdAncestor=%u, DWORD dwFlags=%08x)\n", this, psmc, uId, uIdAncestor, dwFlags);
    HRESULT hr = m_IShellMenu->Initialize(psmc, uId, uIdAncestor, dwFlags);
    WrapLogExit("CMenuBand::Initialize() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetMenuInfo(IShellMenuCallback **ppsmc, UINT *puId, UINT *puIdAncestor, DWORD *pdwFlags)
{
    WrapLogEnter("CMenuBand<%p>::GetMenuInfo(IShellMenuCallback **ppsmc=%p, UINT *puId=%p, UINT *puIdAncestor=%p, DWORD *pdwFlags=%p)\n", this, ppsmc, puId, puIdAncestor, pdwFlags);
    HRESULT hr = m_IShellMenu->GetMenuInfo(ppsmc, puId, puIdAncestor, pdwFlags);
    if (ppsmc) WrapLogMsg("*ppsmc=%p\n", *ppsmc);
    if (puId) WrapLogMsg("*puId=%u\n", *puId);
    if (puIdAncestor) WrapLogMsg("*puIdAncestor=%u\n", *puIdAncestor);
    if (pdwFlags) WrapLogMsg("*pdwFlags=%08x\n", *pdwFlags);
    WrapLogExit("CMenuBand::GetMenuInfo() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    WrapLogEnter("CMenuBand<%p>::SetShellFolder(IShellFolder *psf=%p, LPCITEMIDLIST pidlFolder=%p, HKEY hKey=%p, DWORD dwFlags=%08x)\n", this, psf, pidlFolder, hKey, dwFlags);
    HRESULT hr = m_IShellMenu->SetShellFolder(psf, pidlFolder, hKey, dwFlags);
    WrapLogExit("CMenuBand::SetShellFolder() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    WrapLogEnter("CMenuBand<%p>::GetShellFolder(DWORD *pdwFlags=%p, LPITEMIDLIST *ppidl=%p, REFIID riid=%s, void **ppv=%p)\n", this, pdwFlags, ppidl, Wrap(riid), ppv);
    HRESULT hr = m_IShellMenu->GetShellFolder(pdwFlags, ppidl, riid, ppv);
    if (pdwFlags) WrapLogMsg("*pdwFlags=%08x\n", *pdwFlags);
    if (ppidl) WrapLogMsg("*ppidl=%p\n", *ppidl);
    if (ppv) WrapLogMsg("*ppv=%p\n", *ppv);
    WrapLogExit("CMenuBand::GetShellFolder() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags)
{
    WrapLogEnter("CMenuBand<%p>::SetMenu(HMENU hmenu=%p, HWND hwnd=%p, DWORD dwFlags=%08x)\n", this, hmenu, hwnd, dwFlags);
    HRESULT hr = m_IShellMenu->SetMenu(hmenu, hwnd, dwFlags);
    WrapLogExit("CMenuBand::SetMenu() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags)
{
    WrapLogEnter("CMenuBand<%p>::GetMenu(HMENU *phmenu=%p, HWND *phwnd=%p, DWORD *pdwFlags=%p)\n", this, phmenu, phwnd, pdwFlags);
    HRESULT hr = m_IShellMenu->GetMenu(phmenu, phwnd, pdwFlags);
    if (phmenu) WrapLogMsg("*phmenu=%p\n", *phmenu);
    if (phwnd) WrapLogMsg("*phwnd=%p\n", *phwnd);
    if (pdwFlags) WrapLogMsg("*pdwFlags=%08x\n", *pdwFlags);
    WrapLogExit("CMenuBand::GetMenu() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{
    WrapLogEnter("CMenuBand<%p>::InvalidateItem(LPSMDATA psmd=%p, DWORD dwFlags=%08x)\n", this, psmd, dwFlags);
    HRESULT hr = m_IShellMenu->InvalidateItem(psmd, dwFlags);
    WrapLogExit("CMenuBand::InvalidateItem() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetState(LPSMDATA psmd)
{
    WrapLogEnter("CMenuBand<%p>::GetState(LPSMDATA psmd=%p)\n", this, psmd);
    HRESULT hr = m_IShellMenu->GetState(psmd);
    WrapLogExit("CMenuBand::GetState() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetMenuToolbar(IUnknown *punk, DWORD dwFlags)
{
    WrapLogEnter("CMenuBand<%p>::SetMenuToolbar(IUnknown *punk=%p, DWORD dwFlags=%08x)\n", this, punk, dwFlags);
    HRESULT hr = m_IShellMenu->SetMenuToolbar(punk, dwFlags);
    WrapLogExit("CMenuBand::SetMenuToolbar() = %08x\n", hr);
    return hr;
}
#else

CMenuToolbarBase::CMenuToolbarBase(CMenuBand *menuBand) :
    m_menuBand(menuBand),
    m_hwnd(NULL),
    m_dwMenuFlags(0)
{
}

HRESULT CMenuToolbarBase::ShowWindow(BOOL fShow)
{
    ::ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);

    HIMAGELIST ilBig, ilSmall;
    Shell_GetImageLists(&ilBig, &ilSmall);

    if (m_menuBand->UseBigIcons())
    {
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, (LPARAM) ilBig);
    }
    else
    {
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, (LPARAM) ilSmall);
    }

    return S_OK;
}

HRESULT CMenuToolbarBase::Close()
{
    DestroyWindow(m_hwnd);
    m_hwnd = NULL;
    return S_OK;
}

HRESULT CMenuToolbarBase::CreateToolbar(HWND hwndParent, DWORD dwFlags)
{
    LONG tbStyles = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN |
        TBSTYLE_TOOLTIPS | TBSTYLE_TRANSPARENT | TBSTYLE_REGISTERDROP | TBSTYLE_LIST | TBSTYLE_FLAT |
        CCS_NODIVIDER | CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_TOP;
    LONG tbExStyles = TBSTYLE_EX_DOUBLEBUFFER;

    if (dwFlags & SMINIT_VERTICAL)
    {
        tbStyles |= CCS_VERT;
        tbExStyles |= TBSTYLE_EX_VERTICAL;
    }

    RECT rc;

    if (!::GetClientRect(hwndParent, &rc) || (rc.left == rc.right) || (rc.top == rc.bottom))
    {
        rc.left = 0;
        rc.top = 0;
        rc.right = 1;
        rc.bottom = 1;
    }

    HWND hwndToolbar = CreateWindowEx(
        tbExStyles, TOOLBARCLASSNAMEW, NULL,
        tbStyles, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        hwndParent, NULL, _AtlBaseModule.GetModuleInstance(), 0);

    if (hwndToolbar == NULL)
        return E_FAIL;

    ::SetParent(hwndToolbar, hwndParent);

    m_hwnd = hwndToolbar;

    /* Identify the version of the used Common Controls DLL by sending the size of the TBBUTTON structure */
    SendMessageW(m_hwnd, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    HIMAGELIST ilBig, ilSmall;
    Shell_GetImageLists(&ilBig, &ilSmall);

    //if (dwFlags & SMINIT_TOPLEVEL)
    //{
    //    /* Hide the placeholders for the button images */
    //    SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, 0);
    //}
    //else
    if (m_menuBand->UseBigIcons())
    {
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, (LPARAM) ilBig);
    }
    else
    {
        SendMessageW(m_hwnd, TB_SETIMAGELIST, 0, (LPARAM) ilSmall);
    }

    SetWindowLongPtr(m_hwnd, GWLP_USERDATA, (LONG_PTR)this);
    m_SubclassOld = (WNDPROC) SetWindowLongPtr(m_hwnd, GWLP_WNDPROC, (LONG_PTR) CMenuToolbarBase::s_SubclassProc);

    return S_OK;
}

HRESULT CMenuToolbarBase::GetWindow(HWND *phwnd)
{
    if (!phwnd)
        return E_FAIL;

    *phwnd = m_hwnd;

    return S_OK;
}

LRESULT CALLBACK CMenuToolbarBase::s_SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CMenuToolbarBase * pthis = (CMenuToolbarBase *) GetWindowLongPtr(hWnd, GWLP_USERDATA);
    return pthis->SubclassProc(hWnd, uMsg, wParam, lParam);
}

LRESULT CMenuToolbarBase::SubclassProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TIMER:
        if (wParam == TIMERID_HOTTRACK)
        {
            PopupItem(m_hotItem);
            KillTimer(hWnd, TIMERID_HOTTRACK);
        }
    }

    return m_SubclassOld(hWnd, uMsg, wParam, lParam);
}

HRESULT CMenuToolbarBase::OnHotItemChange(const NMTBHOTITEM * hot)
{
    if (hot->dwFlags & HICF_LEAVING)
    {
        KillTimer(m_hwnd, TIMERID_HOTTRACK);
    }
    else if (m_hotItem != hot->idNew)
    {
        if (HasSubMenu(hot->idNew) == S_OK)
        {
            DWORD elapsed;
            SystemParametersInfo(SPI_GETMENUSHOWDELAY, 0, &elapsed, 0);

            m_hotItem = hot->idNew;

            SetTimer(m_hwnd, TIMERID_HOTTRACK, elapsed, NULL);
        }
    }

    return S_OK;
}

BOOL
AllocAndGetMenuString(HMENU hMenu, UINT ItemIDByPosition, WCHAR** String)
{
    int Length;

    Length = GetMenuStringW(hMenu, ItemIDByPosition, NULL, 0, MF_BYPOSITION);

    if (!Length)
        return FALSE;

    /* Also allocate space for the terminating NULL character */
    ++Length;
    *String = (PWSTR) HeapAlloc(GetProcessHeap(), 0, Length * sizeof(WCHAR));

    GetMenuStringW(hMenu, ItemIDByPosition, *String, Length, MF_BYPOSITION);

    return TRUE;
}

CMenuStaticToolbar::CMenuStaticToolbar(CMenuBand *menuBand) :
    CMenuToolbarBase(menuBand),
    m_hmenu(NULL)
{
}

HRESULT  CMenuStaticToolbar::GetMenu(
    HMENU *phmenu,
    HWND *phwnd,
    DWORD *pdwFlags)
{
    *phmenu = m_hmenu;
    *phwnd = NULL;
    *pdwFlags = m_dwMenuFlags;

    return S_OK;
}

HRESULT  CMenuStaticToolbar::SetMenu(
    HMENU hmenu,
    HWND hwnd,
    DWORD dwFlags)
{
    m_hmenu = hmenu;
    m_dwMenuFlags = dwFlags;

    return S_OK;
}

HRESULT CMenuStaticToolbar::FillToolbar()
{
    int i;
    int ic = GetMenuItemCount(m_hmenu);

    for (i = 0; i < ic; i++)
    {
        MENUITEMINFOW info;
        TBBUTTON tbb = { 0 };
        PWSTR MenuString = NULL;

        tbb.fsState = TBSTATE_ENABLED;
        tbb.fsStyle = 0;

        info.cbSize = sizeof(info);
        info.fMask = MIIM_FTYPE | MIIM_ID;

        GetMenuItemInfoW(m_hmenu, i, TRUE, &info);

        if (info.fType == MFT_STRING)
        {
            if (!AllocAndGetMenuString(m_hmenu, i, &MenuString))
                return E_OUTOFMEMORY;
            if (::GetSubMenu(m_hmenu, i) != NULL)
                tbb.fsStyle |= BTNS_WHOLEDROPDOWN;
            tbb.iString = (INT_PTR) MenuString;
            tbb.idCommand = info.wID;

            SMINFO sminfo;
            if (info.wID >= 0 && SUCCEEDED(m_menuBand->CallCBWithId(info.wID, SMC_GETINFO, 0, (LPARAM) &sminfo)))
            {
                tbb.iBitmap = sminfo.iIcon;
            }
        }
        else
        {
            tbb.fsStyle |= BTNS_SEP;
        }

        SendMessageW(m_hwnd, TB_ADDBUTTONS, 1, (LPARAM) (LPTBBUTTON) &tbb);

        if (MenuString)
            HeapFree(GetProcessHeap(), 0, MenuString);
    }

    return S_OK;
}

HRESULT CMenuStaticToolbar::OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    return m_menuBand->CallCBWithId(wParam, SMC_EXEC, 0, 0);
}

HRESULT CMenuStaticToolbar::PopupItem(UINT uItem)
{
    RECT rc;
    TBBUTTONINFO info = { 0 };
    info.cbSize = sizeof(TBBUTTONINFO);
    info.dwMask = 0;
    int index = SendMessage(m_hwnd, TB_GETBUTTONINFO, uItem, (LPARAM) &info);
    if (index < 0)
        return E_FAIL;
    if (!SendMessage(m_hwnd, TB_GETITEMRECT, index, (LPARAM) &rc))
        return E_FAIL;

    POINT a = { rc.left, rc.top };
    POINT b = { rc.right, rc.bottom };

    ClientToScreen(m_hwnd, &a);
    ClientToScreen(m_hwnd, &b);

    POINTL pt = { b.x, b.y };
    RECTL rcl = { a.x, a.y, b.x, b.y }; // maybe-TODO: fetch client area of deskbar?

    CComPtr<IShellMenu> shellMenu;
    HRESULT hr = m_menuBand->CallCBWithId(uItem, SMC_GETOBJECT, (WPARAM) &IID_IShellMenu, (LPARAM) &shellMenu);
    if (FAILED(hr))
        return hr;

    CComPtr<IMenuPopup> popup;
    hr = CSubMenu_Constructor(shellMenu, IID_PPV_ARG(IMenuPopup, &popup));
    if (FAILED(hr))
        return hr;

    popup->Popup(&pt, &rcl, MPPF_TOP | MPPF_RIGHT);

    return S_OK;
}

HRESULT CMenuStaticToolbar::HasSubMenu(UINT uItem)
{
    TBBUTTONINFO info = { 0 };
    info.cbSize = sizeof(TBBUTTONINFO);
    info.dwMask = 0;
    int index = SendMessage(m_hwnd, TB_GETBUTTONINFO, uItem, (LPARAM) &info);
    if (index < 0)
        return E_FAIL;
    return ::GetSubMenu(m_hmenu, index) ? S_OK : S_FALSE;
}

CMenuSFToolbar::CMenuSFToolbar(CMenuBand * menuBand) :
    CMenuToolbarBase(menuBand),
    m_shellFolder(NULL)
{
}

CMenuSFToolbar::~CMenuSFToolbar()
{
}

HRESULT CMenuSFToolbar::FillToolbar()
{
    HRESULT hr;
    TBBUTTON tbb = { 0 };
    int i = 0;
    PWSTR MenuString;

    tbb.fsState = TBSTATE_ENABLED;
    tbb.fsStyle = 0;

    IEnumIDList * eidl;
    m_shellFolder->EnumObjects(m_hwnd, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, &eidl);

    LPITEMIDLIST item = (LPITEMIDLIST) CoTaskMemAlloc(sizeof(ITEMIDLIST));
    ULONG fetched;
    while ((hr = eidl->Next(1, &item, &fetched)) == S_OK)
    {
        INT index = 0;
        INT indexOpen = 0;

        IShellItem *psi;
        SHCreateShellItem(NULL, m_shellFolder, item, &psi);

        hr = psi->GetDisplayName(SIGDN_NORMALDISPLAY, &MenuString);
        if (FAILED(hr))
            return hr;

        index = SHMapPIDLToSystemImageListIndex(m_shellFolder, item, &indexOpen);

        tbb.idCommand = i++;
        tbb.iString = (INT_PTR) MenuString;
        tbb.iBitmap = index;

        SendMessageW(m_hwnd, TB_ADDBUTTONS, 1, (LPARAM) (LPTBBUTTON) &tbb);
        HeapFree(GetProcessHeap(), 0, MenuString);

    }
    CoTaskMemFree(item);

    return hr;
}

HRESULT CMenuSFToolbar::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    m_shellFolder = psf;
    m_idList = pidlFolder;
    m_hKey = hKey;
    m_dwMenuFlags = dwFlags;
    return S_OK;
}

HRESULT CMenuSFToolbar::GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    HRESULT hr;

    hr = m_shellFolder->QueryInterface(riid, ppv);
    if (FAILED(hr))
        return hr;

    if (pdwFlags)
        *pdwFlags = m_dwMenuFlags;

    if (ppidl)
    {
        LPITEMIDLIST pidl = NULL;

        if (m_idList)
        {
            pidl = ILClone(m_idList);
            if (!pidl)
            {
                (*(IUnknown**) ppv)->Release();
                return E_FAIL;
            }
        }

        *ppidl = pidl;
    }

    return hr;
}
HRESULT CMenuSFToolbar::OnCommand(WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    // TODO: return m_menuBand->CallCBWithPidl(GetPidlFromId(wParam), SMC_SFEXEC, 0, 0);
    return S_OK;
}

HRESULT CMenuSFToolbar::PopupItem(UINT uItem)
{
    return S_OK;
}

HRESULT CMenuSFToolbar::HasSubMenu(UINT uItem)
{
    return S_FALSE; // GetSubMenu(m_hmenu, uItem) ? S_OK : S_FALSE;
}

CMenuBand::CMenuBand() :
    m_site(NULL),
    m_psmc(NULL),
    m_staticToolbar(NULL),
    m_SFToolbar(NULL),
    m_useBigIcons(FALSE)
{
}

CMenuBand::~CMenuBand()
{
    if (m_site)
        m_site->Release();

    if (m_psmc)
        m_psmc->Release();

    if (m_staticToolbar)
        delete m_staticToolbar;

    if (m_SFToolbar)
        delete m_SFToolbar;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::Initialize(
    IShellMenuCallback *psmc,
    UINT uId,
    UINT uIdAncestor,
    DWORD dwFlags)
{
    if (m_psmc)
        m_psmc->Release();

    m_psmc = psmc;
    m_uId = uId;
    m_uIdAncestor = uIdAncestor;
    m_dwFlags = dwFlags;

    if (m_psmc)
    {
        m_psmc->AddRef();

        _CallCB(SMC_CREATE, 0, (LPARAM) &m_UserData);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetMenuInfo(
    IShellMenuCallback **ppsmc,
    UINT *puId,
    UINT *puIdAncestor,
    DWORD *pdwFlags)
{
    if (!pdwFlags) // maybe?
        return E_INVALIDARG;

    if (ppsmc)
        *ppsmc = m_psmc;

    if (puId)
        *puId = m_uId;

    if (puIdAncestor)
        *puIdAncestor = m_uIdAncestor;

    *pdwFlags = m_dwFlags;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::SetMenu(
    HMENU hmenu,
    HWND hwnd,
    DWORD dwFlags)
{
    if (m_staticToolbar == NULL)
    {
        m_staticToolbar = new CMenuStaticToolbar(this);
    }
    m_hmenu = hmenu;

    HRESULT hResult = m_staticToolbar->SetMenu(hmenu, hwnd, dwFlags);
    if (FAILED(hResult))
        return hResult;

    if (m_site)
    {
        HWND hwndParent;

        hResult = m_site->GetWindow(&hwndParent);
        if (FAILED(hResult))
            return hResult;

        hResult = m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hResult))
            return hResult;

        hResult = m_staticToolbar->FillToolbar();
    }

    return hResult;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetMenu(
    HMENU *phmenu,
    HWND *phwnd,
    DWORD *pdwFlags)
{
    if (m_staticToolbar == NULL)
        return E_FAIL;

    return m_staticToolbar->GetMenu(phmenu, phwnd, pdwFlags);
}

HRESULT STDMETHODCALLTYPE  CMenuBand::SetSite(IUnknown *pUnkSite)
{
    HWND                    hwndParent;
    HRESULT                 hResult;

    if (m_site != NULL)
        m_site->Release();

    if (pUnkSite == NULL)
        return S_OK;

    hwndParent = NULL;
    hResult = pUnkSite->QueryInterface(IID_PPV_ARG(IOleWindow, &m_site));
    if (SUCCEEDED(hResult))
    {
        m_site->GetWindow(&hwndParent);
        m_site->Release();
    }
    if (!::IsWindow(hwndParent))
        return E_FAIL;

    if (m_staticToolbar != NULL)
    {
        hResult = m_staticToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hResult))
            return hResult;

        hResult = m_staticToolbar->FillToolbar();
    }

    if (m_SFToolbar != NULL)
    {
        hResult = m_SFToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hResult))
            return hResult;

        hResult = m_SFToolbar->FillToolbar();
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetSite(REFIID riid, PVOID *ppvSite)
{
    if (m_site == NULL)
        return E_FAIL;

    return m_site->QueryInterface(riid, ppvSite);
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetWindow(
    HWND *phwnd)
{
    if (m_SFToolbar != NULL)
        return m_SFToolbar->GetWindow(phwnd);

    if (m_staticToolbar != NULL)
        return m_staticToolbar->GetWindow(phwnd);

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnPosRectChangeDB(RECT *prc)
{
    SIZE sizeStaticX = { 0 };
    SIZE sizeShlFldX = { 0 };
    SIZE sizeStaticY = { 0 };
    SIZE sizeShlFldY = { 0 };
    HWND hwndStatic = NULL;
    HWND hwndShlFld = NULL;
    HRESULT hResult = S_OK;

    if (m_staticToolbar != NULL)
        hResult = m_staticToolbar->GetWindow(&hwndStatic);
    if (FAILED(hResult))
        return hResult;

    if (m_SFToolbar != NULL)
        hResult = m_SFToolbar->GetWindow(&hwndShlFld);
    if (FAILED(hResult))
        return hResult;

    if (hwndStatic == NULL && hwndShlFld == NULL)
        return E_FAIL;

    if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, TRUE, (LPARAM) &sizeStaticY);
    if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, TRUE, (LPARAM) &sizeShlFldY);
    if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, FALSE, (LPARAM) &sizeStaticX);
    if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, FALSE, (LPARAM) &sizeShlFldX);

    int sy = max(prc->bottom - prc->top, sizeStaticY.cy + sizeShlFldY.cy);

    if (hwndShlFld)
    {
        SetWindowPos(hwndShlFld, NULL,
            prc->left,
            prc->top,
            prc->right - prc->left,
            sizeShlFldY.cy,
            0);
        DWORD btnSize = SendMessage(hwndShlFld, TB_GETBUTTONSIZE, 0, 0);
        SendMessage(hwndShlFld, TB_SETBUTTONSIZE, 0, MAKELPARAM(prc->right - prc->left, HIWORD(btnSize)));
    }
    if (hwndStatic)
    {
        SetWindowPos(hwndStatic, hwndShlFld,
            prc->left,
            prc->top + sizeShlFldY.cy,
            prc->right - prc->left,
            sy - sizeShlFldY.cy,
            0);
        DWORD btnSize = SendMessage(hwndStatic, TB_GETBUTTONSIZE, 0, 0);
        SendMessage(hwndStatic, TB_SETBUTTONSIZE, 0, MAKELPARAM(prc->right - prc->left, HIWORD(btnSize)));
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  CMenuBand::GetBandInfo(
    DWORD dwBandID,
    DWORD dwViewMode,
    DESKBANDINFO *pdbi)
{
    HWND hwndStatic = NULL;
    HWND hwndShlFld = NULL;
    HRESULT hResult = S_OK;

    if (m_staticToolbar != NULL)
        hResult = m_staticToolbar->GetWindow(&hwndStatic);
    if (FAILED(hResult))
        return hResult;

    if (m_SFToolbar != NULL)
        hResult = m_SFToolbar->GetWindow(&hwndShlFld);
    if (FAILED(hResult))
        return hResult;

    if (hwndStatic == NULL && hwndShlFld == NULL)
        return E_FAIL;

    // HACK (?)
    if (pdbi->dwMask == 0)
    {
        pdbi->dwMask = DBIM_MINSIZE | DBIM_MAXSIZE | DBIM_INTEGRAL | DBIM_ACTUAL | DBIM_TITLE | DBIM_MODEFLAGS | DBIM_BKCOLOR;
    }

    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        SIZE sizeStatic = { 0 };
        SIZE sizeShlFld = { 0 };

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, TRUE, (LPARAM) &sizeStatic);
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, TRUE, (LPARAM) &sizeShlFld);

        pdbi->ptMinSize.x = 0;
        pdbi->ptMinSize.y = sizeStatic.cy + sizeShlFld.cy;
    }
    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        SIZE sizeStatic = { 0 };
        SIZE sizeShlFld = { 0 };

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETMAXSIZE, 0, (LPARAM) &sizeStatic);
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETMAXSIZE, 0, (LPARAM) &sizeShlFld);

        pdbi->ptMaxSize.x = max(sizeStatic.cx, sizeShlFld.cx); // ignored
        pdbi->ptMaxSize.y = sizeStatic.cy + sizeShlFld.cy;
    }
    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        pdbi->ptIntegral.x = 0;
        pdbi->ptIntegral.y = 0;
    }
    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        SIZE sizeStatic = { 0 };
        SIZE sizeShlFld = { 0 };

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, FALSE, (LPARAM) &sizeStatic);
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, FALSE, (LPARAM) &sizeShlFld);
        pdbi->ptActual.x = max(sizeStatic.cx, sizeShlFld.cx);

        if (hwndStatic) SendMessageW(hwndStatic, TB_GETIDEALSIZE, TRUE, (LPARAM) &sizeStatic);
        if (hwndShlFld) SendMessageW(hwndShlFld, TB_GETIDEALSIZE, TRUE, (LPARAM) &sizeShlFld);
        pdbi->ptActual.y = sizeStatic.cy + sizeShlFld.cy;
    }
    if (pdbi->dwMask & DBIM_TITLE)
        wcscpy(pdbi->wszTitle, L"");
    if (pdbi->dwMask & DBIM_MODEFLAGS)
        pdbi->dwModeFlags = DBIMF_UNDELETEABLE;
    if (pdbi->dwMask & DBIM_BKCOLOR)
        pdbi->crBkgnd = 0;
    return S_OK;
}

/* IDockingWindow */
HRESULT STDMETHODCALLTYPE  CMenuBand::ShowDW(BOOL fShow)
{
    HRESULT hr = S_OK;

    if (m_staticToolbar != NULL)
        hr = m_staticToolbar->ShowWindow(fShow);
    if (FAILED(hr))
        return hr;
    if (m_SFToolbar != NULL)
        hr = m_SFToolbar->ShowWindow(fShow);
    if (FAILED(hr))
        return hr;

    if (fShow)
        return _CallCB(SMC_INITMENU, 0, 0);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::CloseDW(DWORD dwReserved)
{
    ShowDW(FALSE);

    if (m_staticToolbar != NULL)
        return m_staticToolbar->Close();

    if (m_SFToolbar != NULL)
        return m_SFToolbar->Close();

    return S_OK;
}
HRESULT STDMETHODCALLTYPE CMenuBand::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::HasFocusIO()
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateAcceleratorIO(LPMSG lpMsg)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsDirty()
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Load(IStream *pStm)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Save(IStream *pStm, BOOL fClearDirty)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetClassID(CLSID *pClassID)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (!pguidCmdGroup)
        return E_FAIL;

    if (IsEqualGUID(*pguidCmdGroup, CLSID_MenuBand))
    {
        if (nCmdID == 16) // set (big) icon size
        {
            this->m_useBigIcons = TRUE;
            return S_OK;
        }
        else if (nCmdID == 19) // popup-related
        {
            return S_FALSE;
        }
    }

    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    if (IsEqualIID(guidService, SID_SMenuBandChild))
        return this->QueryInterface(riid, ppvObject);
    WARN("Unknown service requested %s\n", wine_dbgstr_guid(&guidService));
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnSelect(DWORD dwSelectType)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetClient(IUnknown *punkClient)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetClient(IUnknown **ppunkClient)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsMenuMessage(MSG *pmsg)
{
    //UNIMPLEMENTED;
    //return S_OK;
    return S_FALSE;
    //return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::TranslateMenuMessage(MSG *pmsg, LRESULT *plRet)
{
    //UNIMPLEMENTED;
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    if (m_SFToolbar == NULL)
    {
        m_SFToolbar = new CMenuSFToolbar(this);
    }

    HRESULT hResult = m_SFToolbar->SetShellFolder(psf, pidlFolder, hKey, dwFlags);
    if (FAILED(hResult))
        return hResult;

    if (m_site)
    {
        HWND hwndParent;

        hResult = m_site->GetWindow(&hwndParent);
        if (FAILED(hResult))
            return hResult;

        hResult = m_SFToolbar->CreateToolbar(hwndParent, m_dwFlags);
        if (FAILED(hResult))
            return hResult;

        hResult = m_SFToolbar->FillToolbar();
    }

    return hResult;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    if (m_SFToolbar)
        return m_SFToolbar->GetShellFolder(pdwFlags, ppidl, riid, ppv);
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CMenuBand::InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetState(LPSMDATA psmd)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetMenuToolbar(IUnknown *punk, DWORD dwFlags)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    *theResult = 0;
    switch (uMsg)
    {
    case WM_COMMAND:

        if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd))
        {
            return m_staticToolbar->OnCommand(wParam, lParam, theResult);
        }

        if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd))
        {
            return m_SFToolbar->OnCommand(wParam, lParam, theResult);
        }

        return S_OK;

    case WM_NOTIFY:
        NMHDR * hdr = (LPNMHDR) lParam;
        NMTBCUSTOMDRAW * cdraw;
        NMTBHOTITEM * hot;
        switch (hdr->code)
        {
        case TBN_HOTITEMCHANGE:
            hot = (NMTBHOTITEM*) hdr;

            if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd))
            {
                return m_staticToolbar->OnHotItemChange(hot);
            }

            if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd))
            {
                return m_SFToolbar->OnHotItemChange(hot);
            }

            return S_OK;

        case NM_CUSTOMDRAW:
            cdraw = (LPNMTBCUSTOMDRAW) hdr;
            switch (cdraw->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                *theResult = CDRF_NOTIFYITEMDRAW;
                return S_OK;

            case CDDS_ITEMPREPAINT:

                cdraw->clrBtnFace = GetSysColor(COLOR_MENU);
                cdraw->clrBtnHighlight = GetSysColor(COLOR_MENUHILIGHT);

                cdraw->clrText = GetSysColor(COLOR_MENUTEXT);
                cdraw->clrTextHighlight = GetSysColor(COLOR_HIGHLIGHTTEXT);
                cdraw->clrHighlightHotTrack = GetSysColor(COLOR_HIGHLIGHTTEXT);

                RECT rc = cdraw->nmcd.rc;
                HDC hdc = cdraw->nmcd.hdc;

                HBRUSH bgBrush = GetSysColorBrush(COLOR_MENU);
                HBRUSH hotBrush = GetSysColorBrush(COLOR_MENUHILIGHT);

                switch (cdraw->nmcd.uItemState)
                {
                case CDIS_HOT:
                case CDIS_FOCUS:
                    FillRect(hdc, &rc, hotBrush);
                    break;
                default:
                    FillRect(hdc, &rc, bgBrush);
                    break;
                }

                *theResult = TBCDRF_NOBACKGROUND | TBCDRF_NOEDGES | TBCDRF_NOETCHEDEFFECT | TBCDRF_HILITEHOTTRACK | TBCDRF_NOOFFSET;
                return S_OK;
            }
            return S_OK;
        }
        return S_OK;
    }

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsWindowOwner(HWND hWnd)
{
    if (m_staticToolbar && m_staticToolbar->IsWindowOwner(hWnd))
        return S_OK;

    if (m_SFToolbar && m_SFToolbar->IsWindowOwner(hWnd))
        return S_OK;

    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetSubMenu(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetToolbar(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetMinWidth(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetNoBorder(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::SetTheme(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetTop(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetBottom(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetTracked(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetParentSite(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::GetState(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::DoDefaultAction(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuBand::IsEmpty(THIS)
{
    UNIMPLEMENTED;
    return S_OK;
}

HRESULT CMenuBand::CallCBWithId(UINT Id, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!m_psmc)
        return S_FALSE;

    HWND hwnd;
    GetWindow(&hwnd);

    SMDATA smData = { 0 };
    smData.punk = (IShellMenu2*)this;
    smData.uId = Id;
    smData.uIdParent = m_uId;
    smData.uIdAncestor = m_uIdAncestor;
    smData.hwnd = hwnd;
    if (m_staticToolbar)
    {
        smData.hmenu = m_hmenu;
    }
    smData.pvUserData = NULL;
    if (m_SFToolbar)
        m_SFToolbar->GetShellFolder(NULL, &smData.pidlFolder, IID_PPV_ARG(IShellFolder, &smData.psf));
    HRESULT hr = m_psmc->CallbackSM(&smData, uMsg, wParam, lParam);
    ILFree(smData.pidlFolder);
    if (smData.psf)
        smData.psf->Release();
    return hr;
}

HRESULT CMenuBand::_CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (!m_psmc)
        return S_FALSE;

    HWND hwnd;
    GetWindow(&hwnd);

    SMDATA smData = { 0 };
    smData.punk = (IShellMenu2*)this;
    smData.uIdParent = m_uId;
    smData.uIdAncestor = m_uIdAncestor;
    smData.hwnd = hwnd;
    if (m_staticToolbar)
    {
        smData.hmenu = m_hmenu;
    }
    smData.pvUserData = NULL;
    if (m_SFToolbar)
        m_SFToolbar->GetShellFolder(NULL, &smData.pidlFolder, IID_PPV_ARG(IShellFolder, &smData.psf));
    HRESULT hr = m_psmc->CallbackSM(&smData, uMsg, wParam, lParam);
    ILFree(smData.pidlFolder);
    if (smData.psf)
        smData.psf->Release();
    return hr;
}
#endif