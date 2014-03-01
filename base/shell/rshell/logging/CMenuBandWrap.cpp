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

WINE_DEFAULT_DEBUG_CHANNEL(CMenuBandWrap);

class CMenuBandWrap :
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
    CMenuBandWrap() {}
    ~CMenuBandWrap();

    HRESULT InitWrap(IShellMenu * shellMenu);

private:
    CComPtr<IDeskBand         > m_IDeskBand;
    CComPtr<IDockingWindow    > m_IDockingWindow;
    CComPtr<IOleWindow        > m_IOleWindow;
    CComPtr<IObjectWithSite   > m_IObjectWithSite;
    CComPtr<IInputObject      > m_IInputObject;
    CComPtr<IPersistStream    > m_IPersistStream;
    CComPtr<IPersist          > m_IPersist;
    CComPtr<IOleCommandTarget > m_IOleCommandTarget;
    CComPtr<IServiceProvider  > m_IServiceProvider;
    CComPtr<IMenuPopup        > m_IMenuPopup;
    CComPtr<IDeskBar          > m_IDeskBar;
    CComPtr<IMenuBand         > m_IMenuBand;
    CComPtr<IShellMenu2       > m_IShellMenu2;
    CComPtr<IShellMenu        > m_IShellMenu;
    CComPtr<IWinEventHandler  > m_IWinEventHandler;
    CComPtr<IShellMenuAcc     > m_IShellMenuAcc;

    IUnknown * m_site;
    
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

    DECLARE_NOT_AGGREGATABLE(CMenuBandWrap)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMenuBandWrap)
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
};

extern "C"
HRESULT WINAPI CMenuBand_Wrapper(IShellMenu * shellMenu, REFIID riid, LPVOID *ppv)
{
    HRESULT hr;

    *ppv = NULL;

    CMenuBandWrap * site = new CComObject<CMenuBandWrap>();

    if (!site)
        return E_OUTOFMEMORY;

    hr = site->InitWrap(shellMenu);
    if (FAILED(hr))
    {
        site->Release();
        return hr;
    }

    hr = site->QueryInterface(riid, ppv);

    if (FAILED(hr))
        site->Release();

    return hr;
}

HRESULT CMenuBandWrap::InitWrap(IShellMenu * shellMenu)
{
    HRESULT hr;

    WrapLogOpen();

    m_IShellMenu = shellMenu;

    hr = shellMenu->QueryInterface(IID_PPV_ARG(IDeskBand, &m_IDeskBand));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IDockingWindow, &m_IDockingWindow));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IOleWindow, &m_IOleWindow));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IObjectWithSite, &m_IObjectWithSite));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IInputObject, &m_IInputObject));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IPersistStream, &m_IPersistStream));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IPersist, &m_IPersist));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &m_IOleCommandTarget));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IServiceProvider, &m_IServiceProvider));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IMenuPopup, &m_IMenuPopup));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IDeskBar, &m_IDeskBar));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IMenuBand, &m_IMenuBand));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IShellMenu2, &m_IShellMenu2));
    if (FAILED(hr)) return hr;
    hr = shellMenu->QueryInterface(IID_PPV_ARG(IWinEventHandler, &m_IWinEventHandler));
    if (FAILED(hr)) return hr;
    //hr = shellMenu->QueryInterface(IID_PPV_ARG(IShellMenuAcc, &m_IShellMenuAcc));
    m_IShellMenuAcc = NULL;
    return hr;
}

CMenuBandWrap::~CMenuBandWrap()
{
    WrapLogClose();
}


// *** IShellMenu2 methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetSubMenu(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetSubMenu()\n", this);
    HRESULT hr = m_IShellMenu2->GetSubMenu();
    WrapLogExit("CMenuBandWrap::GetSubMenu()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetToolbar(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetToolbar()\n", this);
    HRESULT hr = m_IShellMenu2->SetToolbar();
    WrapLogExit("CMenuBandWrap::SetToolbar()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetMinWidth(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetMinWidth()\n", this);
    HRESULT hr = m_IShellMenu2->SetMinWidth();
    WrapLogExit("CMenuBandWrap::SetMinWidth()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetNoBorder(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetNoBorder()\n", this);
    HRESULT hr = m_IShellMenu2->SetNoBorder();
    WrapLogExit("CMenuBandWrap::SetNoBorder()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetTheme(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetTheme()\n", this);
    HRESULT hr = m_IShellMenu2->SetTheme();
    WrapLogExit("CMenuBandWrap::SetTheme()", hr);
    return hr;
}


// *** IShellMenuAcc methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetTop(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetTop()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetTop();
    WrapLogExit("CMenuBandWrap::GetTop()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetBottom(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetTracked(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetParentSite(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetState(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::DoDefaultAction(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::IsEmpty(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom()", hr);
    return hr;
}

// *** IDeskBand methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBandInfo(DWORD dwBandID=%d, DWORD dwViewMode=%d, DESKBANDINFO *pdbi=%p)\n", this, dwBandID, dwViewMode, pdbi);
    HRESULT hr = m_IDeskBand->GetBandInfo(dwBandID, dwViewMode, pdbi);
    WrapLogExit("CMenuBandWrap::GetBandInfo()", hr);
    return hr;
}

// *** IDockingWindow methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::ShowDW(BOOL fShow)
{
    WrapLogEnter("CMenuBandWrap<%p>::ShowDW(BOOL fShow=%d)\n", this, fShow);
    HRESULT hr = m_IDockingWindow->ShowDW(fShow);
    WrapLogExit("CMenuBandWrap::ShowDW()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::CloseDW(DWORD dwReserved)
{
    WrapLogEnter("CMenuBandWrap<%p>::CloseDW(DWORD dwReserved=%d)\n", this, dwReserved);
    HRESULT hr = m_IDockingWindow->CloseDW(dwReserved);
    WrapLogExit("CMenuBandWrap::CloseDW()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    WrapLogEnter("CMenuBandWrap<%p>::ResizeBorderDW(LPCRECT prcBorder=%p, IUnknown *punkToolbarSite=%p, BOOL fReserved=%d)\n", this, prcBorder, punkToolbarSite, fReserved);
    if (prcBorder) WrapLogPre("*prcBorder=%s\n", Wrap(*prcBorder));
    HRESULT hr = m_IDockingWindow->ResizeBorderDW(prcBorder, punkToolbarSite, fReserved);
    if (prcBorder) WrapLogPost("*prcBorder=%s\n", Wrap(*prcBorder));
    WrapLogExit("CMenuBandWrap::ResizeBorderDW()", hr);
    return hr;
}

// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetWindow(HWND *phwnd)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetWindow(HWND *phwnd=%p)\n", this, phwnd);
    HRESULT hr = m_IOleWindow->GetWindow(phwnd);
    if (phwnd) WrapLogPost("*phwnd=%p\n", *phwnd);
    WrapLogExit("CMenuBandWrap::GetWindow()", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBandWrap::ContextSensitiveHelp(BOOL fEnterMode)
{
    WrapLogEnter("CMenuBandWrap<%p>::ContextSensitiveHelp(BOOL fEnterMode=%d)\n", this, fEnterMode);
    HRESULT hr = m_IOleWindow->ContextSensitiveHelp(fEnterMode);
    WrapLogExit("CMenuBandWrap::ContextSensitiveHelp()", hr);
    return hr;
}

// *** IWinEventHandler methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    //WrapLogEnter("CMenuBandWrap<%p>::OnWinEvent(HWND hWnd=%p, UINT uMsg=%u, WPARAM wParam=%08x, LPARAM lParam=%08x, LRESULT *theResult=%p)\n", this, hWnd, uMsg, wParam, lParam, theResult);
    HRESULT hr = m_IWinEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
    //WrapLogExit("CMenuBandWrap::OnWinEvent()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::IsWindowOwner(HWND hWnd)
{
    //WrapLogEnter("CMenuBandWrap<%p>::IsWindowOwner(HWND hWnd=%08x)\n", this, hWnd);
    HRESULT hr = m_IWinEventHandler->IsWindowOwner(hWnd);
    //WrapLogExit("CMenuBandWrap::IsWindowOwner()", hr);
    return hr;
}

// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetSite(IUnknown *pUnkSite)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetSite(IUnknown *pUnkSite=%p)\n", this, pUnkSite);
    HRESULT hr = m_IObjectWithSite->SetSite(pUnkSite);
    WrapLogExit("CMenuBandWrap::SetSite()", hr);
    m_site = pUnkSite;
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetSite(REFIID riid, PVOID *ppvSite)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetSite(REFIID riid=%s, PVOID *ppvSite=%p)\n", this, Wrap(riid), ppvSite);
    HRESULT hr = m_IObjectWithSite->GetSite(riid, ppvSite);
    if (ppvSite) WrapLogPost("*ppvSite=%p\n", *ppvSite);
    WrapLogExit("CMenuBandWrap::GetSite()", hr);
    return hr;
}

// *** IInputObject methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    WrapLogEnter("CMenuBandWrap<%p>::UIActivateIO(BOOL fActivate=%d, LPMSG lpMsg=%p)\n", this, fActivate, lpMsg);
    HRESULT hr = m_IInputObject->UIActivateIO(fActivate, lpMsg);
    WrapLogExit("CMenuBandWrap::UIActivateIO()", hr);

    HRESULT hrt;
    CComPtr<IMenuPopup> pmp;

    hrt = IUnknown_QueryService(m_site, SID_SMenuPopup, IID_PPV_ARG(IMenuPopup, &pmp));
    if (FAILED(hrt))
        return hr;

    hrt = pmp->SetSubMenu(this, fActivate);

    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::HasFocusIO()
{
    WrapLogEnter("CMenuBandWrap<%p>::HasFocusIO()\n", this);
    HRESULT hr = m_IInputObject->HasFocusIO();
    WrapLogExit("CMenuBandWrap::HasFocusIO()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::TranslateAcceleratorIO(LPMSG lpMsg)
{
    WrapLogEnter("CMenuBandWrap<%p>::TranslateAcceleratorIO(LPMSG lpMsg=%p)\n", this, lpMsg);
    if (lpMsg) WrapLogPre("*lpMsg=%s\n", Wrap(*lpMsg));
    HRESULT hr = m_IInputObject->TranslateAcceleratorIO(lpMsg);
    WrapLogExit("CMenuBandWrap::TranslateAcceleratorIO()", hr);
    return hr;
}

// *** IPersistStream methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::IsDirty()
{
    WrapLogEnter("CMenuBandWrap<%p>::IsDirty()\n", this);
    HRESULT hr = m_IPersistStream->IsDirty();
    WrapLogExit("CMenuBandWrap::IsDirty()", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBandWrap::Load(IStream *pStm)
{
    WrapLogEnter("CMenuBandWrap<%p>::Load(IStream *pStm=%p)\n", this, pStm);
    HRESULT hr = m_IPersistStream->Load(pStm);
    WrapLogExit("CMenuBandWrap::Load()", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBandWrap::Save(IStream *pStm, BOOL fClearDirty)
{
    WrapLogEnter("CMenuBandWrap<%p>::Save(IStream *pStm=%p, BOOL fClearDirty=%d)\n", this, pStm, fClearDirty);
    HRESULT hr = m_IPersistStream->Save(pStm, fClearDirty);
    WrapLogExit("CMenuBandWrap::Save()", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetSizeMax(ULARGE_INTEGER *pcbSize=%p)\n", this, pcbSize);
    HRESULT hr = m_IPersistStream->GetSizeMax(pcbSize);
    WrapLogExit("CMenuBandWrap::GetSizeMax()", hr);
    return hr;
}

// *** IPersist methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetClassID(CLSID *pClassID)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetClassID(CLSID *pClassID=%p)\n", this, pClassID);
    HRESULT hr = m_IPersist->GetClassID(pClassID);
    if (pClassID) WrapLogPost("*pClassID=%s\n", Wrap(*pClassID));
    WrapLogExit("CMenuBandWrap::GetClassID()", hr);
    return hr;
}

// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    WrapLogEnter("CMenuBandWrap<%p>::QueryStatus(const GUID *pguidCmdGroup=%p, ULONG cCmds=%u, prgCmds=%p, pCmdText=%p)\n", this, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    HRESULT hr = m_IOleCommandTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    if (pguidCmdGroup) WrapLogPost("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    WrapLogExit("CMenuBandWrap::QueryStatus()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    WrapLogEnter("CMenuBandWrap<%p>::Exec(const GUID *pguidCmdGroup=%p, DWORD nCmdID=%d, DWORD nCmdexecopt=%d, VARIANT *pvaIn=%p, VARIANT *pvaOut=%p)\n", this, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    if (pguidCmdGroup) WrapLogPre("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    HRESULT hr = m_IOleCommandTarget->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    WrapLogExit("CMenuBandWrap::Exec()", hr);
    return hr;
}

// *** IServiceProvider methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    WrapLogEnter("CMenuBandWrap<%p>::QueryService(REFGUID guidService=%s, REFIID riid=%s, void **ppvObject=%p)\n", this, Wrap(guidService), Wrap(riid), ppvObject);

    if (IsEqualIID(guidService, SID_SMenuBandChild))
    {
        WrapLogPre("SID is SID_SMenuBandChild. Using QueryInterface of self instead of wrapped object.\n");
        HRESULT hr = this->QueryInterface(riid, ppvObject);
        if (SUCCEEDED(hr))
        {
            if (ppvObject) WrapLogPost("*ppvObject=%p\n", *ppvObject);
            WrapLogExit("CMenuBandWrap::QueryService()", hr);
            return hr;
        }
        else
        {
            WrapLogPre("QueryInterface on wrapper failed. Handing over to innter object.\n");
        }
    }
    else if (IsEqualIID(guidService, SID_SMenuBandBottom))
    {
        WrapLogPre("SID is SID_SMenuBandBottom. Using QueryInterface of self instead of wrapped object.\n");
        HRESULT hr = this->QueryInterface(riid, ppvObject);
        if (SUCCEEDED(hr))
        {
            if (ppvObject) WrapLogPost("*ppvObject=%p\n", *ppvObject);
            WrapLogExit("CMenuBandWrap::QueryService()", hr);
            return hr;
        }
        else
        {
            WrapLogPre("QueryInterface on wrapper failed. Handing over to innter object.\n");
        }
    }
    else if (IsEqualIID(guidService, SID_SMenuBandBottomSelected))
    {
        WrapLogPre("SID is SID_SMenuBandBottomSelected. Using QueryInterface of self instead of wrapped object.\n");
        HRESULT hr = this->QueryInterface(riid, ppvObject);
        if (SUCCEEDED(hr))
        {
            if (ppvObject) WrapLogPost("*ppvObject=%p\n", *ppvObject);
            WrapLogExit("CMenuBandWrap::QueryService()", hr);
            return hr;
        }
        else
        {
            WrapLogPre("QueryInterface on wrapper failed. Handing over to innter object.\n");
        }
    }
    else
    {
        WrapLogPre("SID not identified.\n");
    }
    HRESULT hr = m_IServiceProvider->QueryService(guidService, riid, ppvObject);
    if (ppvObject) WrapLogPost("*ppvObject=%p\n", *ppvObject);
    WrapLogExit("CMenuBandWrap::QueryService()", hr);
    return hr;
}


// *** IMenuPopup methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::Popup(POINTL *ppt=%p, RECTL *prcExclude=%p, MP_POPUPFLAGS dwFlags=%08x)\n", this, ppt, prcExclude, dwFlags);
    HRESULT hr = m_IMenuPopup->Popup(ppt, prcExclude, dwFlags);
    WrapLogExit("CMenuBandWrap::Popup()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::OnSelect(DWORD dwSelectType)
{
    WrapLogEnter("CMenuBandWrap<%p>::OnSelect(DWORD dwSelectType=%08x)\n", this, dwSelectType);
    HRESULT hr = m_IMenuPopup->OnSelect(dwSelectType);
    WrapLogExit("CMenuBandWrap::OnSelect()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetSubMenu(IMenuPopup *pmp=%p, BOOL fSet=%d)\n", this, pmp, fSet);
    HRESULT hr = m_IMenuPopup->SetSubMenu(pmp, fSet);
    WrapLogExit("CMenuBandWrap::SetSubMenu()", hr);
    return hr;
}


// *** IDeskBar methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetClient(IUnknown *punkClient)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetClient(IUnknown *punkClient=%p)\n", this, punkClient);
    HRESULT hr = m_IDeskBar->SetClient(punkClient);
    WrapLogExit("CMenuBandWrap::SetClient()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetClient(IUnknown **ppunkClient)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetClient(IUnknown **ppunkClient=%p)\n", this, ppunkClient);
    HRESULT hr = m_IDeskBar->GetClient(ppunkClient);
    if (ppunkClient) WrapLogPost("*ppunkClient=%p\n", *ppunkClient);
    WrapLogExit("CMenuBandWrap::GetClient()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::OnPosRectChangeDB(RECT *prc)
{
    WrapLogEnter("CMenuBandWrap<%p>::OnPosRectChangeDB(RECT *prc=%p)\n", this, prc);
    HRESULT hr = m_IDeskBar->OnPosRectChangeDB(prc);
    if (prc) WrapLogPost("*prc=%s\n", Wrap(*prc));
    WrapLogExit("CMenuBandWrap::OnPosRectChangeDB()", hr);
    return hr;
}


// *** IMenuBand methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::IsMenuMessage(MSG *pmsg)
{
    //WrapLogEnter("CMenuBandWrap<%p>::IsMenuMessage(MSG *pmsg=%p)\n", this, pmsg);
    HRESULT hr = m_IMenuBand->IsMenuMessage(pmsg);
    //WrapLogExit("CMenuBandWrap::IsMenuMessage()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::TranslateMenuMessage(MSG *pmsg, LRESULT *plRet)
{
    //WrapLogEnter("CMenuBandWrap<%p>::TranslateMenuMessage(MSG *pmsg=%p, LRESULT *plRet=%p)\n", this, pmsg, plRet);
    HRESULT hr = m_IMenuBand->TranslateMenuMessage(pmsg, plRet);
    //WrapLogExit("CMenuBandWrap::TranslateMenuMessage(*plRet=%d) = %08x\n", *plRet, hr);
    return hr;
}

// *** IShellMenu methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::Initialize(IShellMenuCallback *psmc, UINT uId, UINT uIdAncestor, DWORD dwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::Initialize(IShellMenuCallback *psmc=%p, UINT uId=%u, UINT uIdAncestor=%u, DWORD dwFlags=%08x)\n", this, psmc, uId, uIdAncestor, dwFlags);
    HRESULT hr = m_IShellMenu->Initialize(psmc, uId, uIdAncestor, dwFlags);
    WrapLogExit("CMenuBandWrap::Initialize()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetMenuInfo(IShellMenuCallback **ppsmc, UINT *puId, UINT *puIdAncestor, DWORD *pdwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetMenuInfo(IShellMenuCallback **ppsmc=%p, UINT *puId=%p, UINT *puIdAncestor=%p, DWORD *pdwFlags=%p)\n", this, ppsmc, puId, puIdAncestor, pdwFlags);
    HRESULT hr = m_IShellMenu->GetMenuInfo(ppsmc, puId, puIdAncestor, pdwFlags);
    if (ppsmc) WrapLogPost("*ppsmc=%p\n", *ppsmc);
    if (puId) WrapLogPost("*puId=%u\n", *puId);
    if (puIdAncestor) WrapLogPost("*puIdAncestor=%u\n", *puIdAncestor);
    if (pdwFlags) WrapLogPost("*pdwFlags=%08x\n", *pdwFlags);
    WrapLogExit("CMenuBandWrap::GetMenuInfo()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetShellFolder(IShellFolder *psf=%p, LPCITEMIDLIST pidlFolder=%p, HKEY hKey=%p, DWORD dwFlags=%08x)\n", this, psf, pidlFolder, hKey, dwFlags);
    HRESULT hr = m_IShellMenu->SetShellFolder(psf, pidlFolder, hKey, dwFlags);
    WrapLogExit("CMenuBandWrap::SetShellFolder()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetShellFolder(DWORD *pdwFlags=%p, LPITEMIDLIST *ppidl=%p, REFIID riid=%s, void **ppv=%p)\n", this, pdwFlags, ppidl, Wrap(riid), ppv);
    HRESULT hr = m_IShellMenu->GetShellFolder(pdwFlags, ppidl, riid, ppv);
    if (pdwFlags) WrapLogPost("*pdwFlags=%08x\n", *pdwFlags);
    if (ppidl) WrapLogPost("*ppidl=%p\n", *ppidl);
    if (ppv) WrapLogPost("*ppv=%p\n", *ppv);
    WrapLogExit("CMenuBandWrap::GetShellFolder()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetMenu(HMENU hmenu=%p, HWND hwnd=%p, DWORD dwFlags=%08x)\n", this, hmenu, hwnd, dwFlags);
    HRESULT hr = m_IShellMenu->SetMenu(hmenu, hwnd, dwFlags);
    WrapLogExit("CMenuBandWrap::SetMenu()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetMenu(HMENU *phmenu=%p, HWND *phwnd=%p, DWORD *pdwFlags=%p)\n", this, phmenu, phwnd, pdwFlags);
    HRESULT hr = m_IShellMenu->GetMenu(phmenu, phwnd, pdwFlags);
    if (phmenu) WrapLogPost("*phmenu=%p\n", *phmenu);
    if (phwnd) WrapLogPost("*phwnd=%p\n", *phwnd);
    if (pdwFlags) WrapLogPost("*pdwFlags=%08x\n", *pdwFlags);
    WrapLogExit("CMenuBandWrap::GetMenu()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::InvalidateItem(LPSMDATA psmd=%p, DWORD dwFlags=%08x)\n", this, psmd, dwFlags);
    HRESULT hr = m_IShellMenu->InvalidateItem(psmd, dwFlags);
    WrapLogExit("CMenuBandWrap::InvalidateItem()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetState(LPSMDATA psmd)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetState(LPSMDATA psmd=%p)\n", this, psmd);
    HRESULT hr = m_IShellMenu->GetState(psmd);
    WrapLogExit("CMenuBandWrap::GetState()", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetMenuToolbar(IUnknown *punk, DWORD dwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetMenuToolbar(IUnknown *punk=%p, DWORD dwFlags=%08x)\n", this, punk, dwFlags);
    HRESULT hr = m_IShellMenu->SetMenuToolbar(punk, dwFlags);
    WrapLogExit("CMenuBandWrap::SetMenuToolbar()", hr);
    return hr;
}
