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
    public CComCoClass<CMenuBandWrap>,
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
    CMenuBandWrap();
    ~CMenuBandWrap();

private:
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
HRESULT CMenuBand_Wrapper(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    CMenuBandWrap * site = new CComObject<CMenuBandWrap>();

    if (!site)
        return E_OUTOFMEMORY;

    HRESULT hr = site->QueryInterface(riid, ppv);

    if (FAILED(hr))
        site->Release();

    return hr;
}

CMenuBandWrap::CMenuBandWrap()
{
    WrapLogOpen();

    CoCreateInstance(CLSID_MenuBand, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellMenu, &m_IShellMenu));
    m_IShellMenu->QueryInterface(IID_PPV_ARG(IUnknown, &m_IUnknown));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IDeskBand, &m_IDeskBand));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IDockingWindow, &m_IDockingWindow));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IOleWindow, &m_IOleWindow));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IObjectWithSite, &m_IObjectWithSite));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IInputObject, &m_IInputObject));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IPersistStream, &m_IPersistStream));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IPersist, &m_IPersist));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &m_IOleCommandTarget));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IServiceProvider, &m_IServiceProvider));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IMenuPopup, &m_IMenuPopup));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IDeskBar, &m_IDeskBar));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IMenuBand, &m_IMenuBand));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IShellMenu2, &m_IShellMenu2));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IWinEventHandler, &m_IWinEventHandler));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IShellMenuAcc, &m_IShellMenuAcc));
}

CMenuBandWrap::~CMenuBandWrap()
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
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetSubMenu(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetSubMenu()\n", this);
    HRESULT hr = m_IShellMenu2->GetSubMenu();
    WrapLogExit("CMenuBandWrap::GetSubMenu() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetToolbar(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetToolbar()\n", this);
    HRESULT hr = m_IShellMenu2->SetToolbar();
    WrapLogExit("CMenuBandWrap::SetToolbar() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetMinWidth(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetMinWidth()\n", this);
    HRESULT hr = m_IShellMenu2->SetMinWidth();
    WrapLogExit("CMenuBandWrap::SetMinWidth() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetNoBorder(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetNoBorder()\n", this);
    HRESULT hr = m_IShellMenu2->SetNoBorder();
    WrapLogExit("CMenuBandWrap::SetNoBorder() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetTheme(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetTheme()\n", this);
    HRESULT hr = m_IShellMenu2->SetTheme();
    WrapLogExit("CMenuBandWrap::SetTheme() = %08x\n", hr);
    return hr;
}


// *** IShellMenuAcc methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetTop(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetTop()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetTop();
    WrapLogExit("CMenuBandWrap::GetTop() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetBottom(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetTracked(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetParentSite(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetState(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::DoDefaultAction(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::IsEmpty(THIS)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBottom()\n", this);
    HRESULT hr = m_IShellMenuAcc->GetBottom();
    WrapLogExit("CMenuBandWrap::GetBottom() = %08x\n", hr);
    return hr;
}

// *** IDeskBand methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO *pdbi)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetBandInfo(DWORD dwBandID=%d, DWORD dwViewMode=%d, DESKBANDINFO *pdbi=%p)\n", this, dwBandID, dwViewMode, pdbi);
    HRESULT hr = m_IDeskBand->GetBandInfo(dwBandID, dwViewMode, pdbi);
    WrapLogExit("CMenuBandWrap::GetBandInfo() = %08x\n", hr);
    return hr;
}

// *** IDockingWindow methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::ShowDW(BOOL fShow)
{
    WrapLogEnter("CMenuBandWrap<%p>::ShowDW(BOOL fShow=%d)\n", this, fShow);
    HRESULT hr = m_IDockingWindow->ShowDW(fShow);
    WrapLogExit("CMenuBandWrap::ShowDW() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::CloseDW(DWORD dwReserved)
{
    WrapLogEnter("CMenuBandWrap<%p>::CloseDW(DWORD dwReserved=%d)\n", this, dwReserved);
    HRESULT hr = m_IDockingWindow->CloseDW(dwReserved);
    WrapLogExit("CMenuBandWrap::CloseDW() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    WrapLogEnter("CMenuBandWrap<%p>::ResizeBorderDW(LPCRECT prcBorder=%p, IUnknown *punkToolbarSite=%p, BOOL fReserved=%d)\n", this, prcBorder, punkToolbarSite, fReserved);
    if (prcBorder) WrapLogMsg("*prcBorder=%s\n", Wrap(*prcBorder));
    HRESULT hr = m_IDockingWindow->ResizeBorderDW(prcBorder, punkToolbarSite, fReserved);
    if (prcBorder) WrapLogMsg("*prcBorder=%s\n", Wrap(*prcBorder));
    WrapLogExit("CMenuBandWrap::ResizeBorderDW() = %08x\n", hr);
    return hr;
}

// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetWindow(HWND *phwnd)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetWindow(HWND *phwnd=%p)\n", this, phwnd);
    HRESULT hr = m_IOleWindow->GetWindow(phwnd);
    if (phwnd) WrapLogMsg("*phwnd=%p\n", *phwnd);
    WrapLogExit("CMenuBandWrap::GetWindow() = %08x\n", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBandWrap::ContextSensitiveHelp(BOOL fEnterMode)
{
    WrapLogEnter("CMenuBandWrap<%p>::ContextSensitiveHelp(BOOL fEnterMode=%d)\n", this, fEnterMode);
    HRESULT hr = m_IOleWindow->ContextSensitiveHelp(fEnterMode);
    WrapLogExit("CMenuBandWrap::ContextSensitiveHelp() = %08x\n", hr);
    return hr;
}

// *** IWinEventHandler methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
{
    //WrapLogEnter("CMenuBandWrap<%p>::OnWinEvent(HWND hWnd=%p, UINT uMsg=%u, WPARAM wParam=%08x, LPARAM lParam=%08x, LRESULT *theResult=%p)\n", this, hWnd, uMsg, wParam, lParam, theResult);
    HRESULT hr = m_IWinEventHandler->OnWinEvent(hWnd, uMsg, wParam, lParam, theResult);
    //WrapLogExit("CMenuBandWrap::OnWinEvent() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::IsWindowOwner(HWND hWnd)
{
    //WrapLogEnter("CMenuBandWrap<%p>::IsWindowOwner(HWND hWnd=%08x)\n", this, hWnd);
    HRESULT hr = m_IWinEventHandler->IsWindowOwner(hWnd);
    //WrapLogExit("CMenuBandWrap::IsWindowOwner() = %08x\n", hr);
    return hr;
}

// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetSite(IUnknown *pUnkSite)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetSite(IUnknown *pUnkSite=%p)\n", this, pUnkSite);
    HRESULT hr = m_IObjectWithSite->SetSite(pUnkSite);
    WrapLogExit("CMenuBandWrap::SetSite() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetSite(REFIID riid, PVOID *ppvSite)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetSite(REFIID riid=%s, PVOID *ppvSite=%p)\n", this, Wrap(riid), ppvSite);
    HRESULT hr = m_IObjectWithSite->GetSite(riid, ppvSite);
    if (ppvSite) WrapLogMsg("*ppvSite=%p\n", *ppvSite);
    WrapLogExit("CMenuBandWrap::GetSite() = %08x\n", hr);
    return hr;
}

// *** IInputObject methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    WrapLogEnter("CMenuBandWrap<%p>::UIActivateIO(BOOL fActivate=%d, LPMSG lpMsg=%p)\n", this, fActivate, lpMsg);
    HRESULT hr = m_IInputObject->UIActivateIO(fActivate, lpMsg);
    WrapLogExit("CMenuBandWrap::UIActivateIO() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::HasFocusIO()
{
    WrapLogEnter("CMenuBandWrap<%p>::HasFocusIO()\n", this);
    HRESULT hr = m_IInputObject->HasFocusIO();
    WrapLogExit("CMenuBandWrap::HasFocusIO() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::TranslateAcceleratorIO(LPMSG lpMsg)
{
    WrapLogEnter("CMenuBandWrap<%p>::TranslateAcceleratorIO(LPMSG lpMsg=%p)\n", this, lpMsg);
    if (lpMsg) WrapLogMsg("*lpMsg=%s\n", Wrap(*lpMsg));
    HRESULT hr = m_IInputObject->TranslateAcceleratorIO(lpMsg);
    WrapLogExit("CMenuBandWrap::TranslateAcceleratorIO() = %08x\n", hr);
    return hr;
}

// *** IPersistStream methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::IsDirty()
{
    WrapLogEnter("CMenuBandWrap<%p>::IsDirty()\n", this);
    HRESULT hr = m_IPersistStream->IsDirty();
    WrapLogExit("CMenuBandWrap::IsDirty() = %08x\n", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBandWrap::Load(IStream *pStm)
{
    WrapLogEnter("CMenuBandWrap<%p>::Load(IStream *pStm=%p)\n", this, pStm);
    HRESULT hr = m_IPersistStream->Load(pStm);
    WrapLogExit("CMenuBandWrap::Load() = %08x\n", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBandWrap::Save(IStream *pStm, BOOL fClearDirty)
{
    WrapLogEnter("CMenuBandWrap<%p>::Save(IStream *pStm=%p, BOOL fClearDirty=%d)\n", this, pStm, fClearDirty);
    HRESULT hr = m_IPersistStream->Save(pStm, fClearDirty);
    WrapLogExit("CMenuBandWrap::Save() = %08x\n", hr);
    return hr;
}
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetSizeMax(ULARGE_INTEGER *pcbSize=%p)\n", this, pcbSize);
    HRESULT hr = m_IPersistStream->GetSizeMax(pcbSize);
    WrapLogExit("CMenuBandWrap::GetSizeMax() = %08x\n", hr);
    return hr;
}

// *** IPersist methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetClassID(CLSID *pClassID)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetClassID(CLSID *pClassID=%p)\n", this, pClassID);
    HRESULT hr = m_IPersist->GetClassID(pClassID);
    if (pClassID) WrapLogMsg("*pClassID=%s\n", Wrap(*pClassID));
    WrapLogExit("CMenuBandWrap::GetClassID() = %08x\n", hr);
    return hr;
}

// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    WrapLogEnter("CMenuBandWrap<%p>::QueryStatus(const GUID *pguidCmdGroup=%p, ULONG cCmds=%u, prgCmds=%p, pCmdText=%p)\n", this, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    HRESULT hr = m_IOleCommandTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    if (pguidCmdGroup) WrapLogMsg("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    WrapLogExit("CMenuBandWrap::QueryStatus() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    WrapLogEnter("CMenuBandWrap<%p>::Exec(const GUID *pguidCmdGroup=%p, DWORD nCmdID=%d, DWORD nCmdexecopt=%d, VARIANT *pvaIn=%p, VARIANT *pvaOut=%p)\n", this, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    if (pguidCmdGroup) WrapLogMsg("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    HRESULT hr = m_IOleCommandTarget->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    WrapLogExit("CMenuBandWrap::Exec() = %08x\n", hr);
    return hr;
}

// *** IServiceProvider methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    WrapLogEnter("CMenuBandWrap<%p>::QueryService(REFGUID guidService=%s, REFIID riid=%s, void **ppvObject=%p)\n", this, Wrap(guidService), Wrap(riid), ppvObject);

    if (IsEqualIID(guidService, SID_SMenuBandChild))
    {
        WrapLogMsg("SID is SID_SMenuBandChild. Using QueryInterface of self instead of wrapped object.\n");
        HRESULT hr = this->QueryInterface(riid, ppvObject);
        if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
        if (SUCCEEDED(hr))
        {
            WrapLogExit("CMenuBandWrap::QueryService() = %08x\n", hr);
            return hr;
        }
        else
        {
            WrapLogMsg("QueryInterface on wrapper failed. Handing over to innter object.\n");
        }
    }
    else if (IsEqualIID(guidService, SID_SMenuBandBottom))
    {
        WrapLogMsg("SID is SID_SMenuBandBottom. Using QueryInterface of self instead of wrapped object.\n");
        HRESULT hr = this->QueryInterface(riid, ppvObject);
        if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
        if (SUCCEEDED(hr))
        {
            WrapLogExit("CMenuBandWrap::QueryService() = %08x\n", hr);
            return hr;
        }
        else
        {
            WrapLogMsg("QueryInterface on wrapper failed. Handing over to innter object.\n");
        }
    }
    else if (IsEqualIID(guidService, SID_SMenuBandBottomSelected))
    {
        WrapLogMsg("SID is SID_SMenuBandBottomSelected. Using QueryInterface of self instead of wrapped object.\n");
        HRESULT hr = this->QueryInterface(riid, ppvObject);
        if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
        if (SUCCEEDED(hr))
        {
            WrapLogExit("CMenuBandWrap::QueryService() = %08x\n", hr);
            return hr;
        }
        else
        {
            WrapLogMsg("QueryInterface on wrapper failed. Handing over to innter object.\n");
        }
    }
    else
    {
        WrapLogMsg("SID not identified.\n");
    }
    HRESULT hr = m_IServiceProvider->QueryService(guidService, riid, ppvObject);
    if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
    WrapLogExit("CMenuBandWrap::QueryService() = %08x\n", hr);
    return hr;
}


// *** IMenuPopup methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::Popup(POINTL *ppt=%p, RECTL *prcExclude=%p, MP_POPUPFLAGS dwFlags=%08x)\n", this, ppt, prcExclude, dwFlags);
    HRESULT hr = m_IMenuPopup->Popup(ppt, prcExclude, dwFlags);
    WrapLogExit("CMenuBandWrap::Popup() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::OnSelect(DWORD dwSelectType)
{
    WrapLogEnter("CMenuBandWrap<%p>::OnSelect(DWORD dwSelectType=%08x)\n", this, dwSelectType);
    HRESULT hr = m_IMenuPopup->OnSelect(dwSelectType);
    WrapLogExit("CMenuBandWrap::OnSelect() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetSubMenu(IMenuPopup *pmp=%p, BOOL fSet=%d)\n", this, pmp, fSet);
    HRESULT hr = m_IMenuPopup->SetSubMenu(pmp, fSet);
    WrapLogExit("CMenuBandWrap::SetSubMenu() = %08x\n", hr);
    return hr;
}


// *** IDeskBar methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetClient(IUnknown *punkClient)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetClient(IUnknown *punkClient=%p)\n", this, punkClient);
    HRESULT hr = m_IDeskBar->SetClient(punkClient);
    WrapLogExit("CMenuBandWrap::SetClient() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetClient(IUnknown **ppunkClient)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetClient(IUnknown **ppunkClient=%p)\n", this, ppunkClient);
    HRESULT hr = m_IDeskBar->GetClient(ppunkClient);
    if (ppunkClient) WrapLogMsg("*ppunkClient=%p\n", *ppunkClient);
    WrapLogExit("CMenuBandWrap::GetClient() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::OnPosRectChangeDB(RECT *prc)
{
    WrapLogEnter("CMenuBandWrap<%p>::OnPosRectChangeDB(RECT *prc=%p)\n", this, prc);
    HRESULT hr = m_IDeskBar->OnPosRectChangeDB(prc);
    if (prc) WrapLogMsg("*prc=%s\n", Wrap(*prc));
    WrapLogExit("CMenuBandWrap::OnPosRectChangeDB() = %08x\n", hr);
    return hr;
}


// *** IMenuBand methods ***
HRESULT STDMETHODCALLTYPE CMenuBandWrap::IsMenuMessage(MSG *pmsg)
{
    //WrapLogEnter("CMenuBandWrap<%p>::IsMenuMessage(MSG *pmsg=%p)\n", this, pmsg);
    HRESULT hr = m_IMenuBand->IsMenuMessage(pmsg);
    //WrapLogExit("CMenuBandWrap::IsMenuMessage() = %08x\n", hr);
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
    WrapLogExit("CMenuBandWrap::Initialize() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetMenuInfo(IShellMenuCallback **ppsmc, UINT *puId, UINT *puIdAncestor, DWORD *pdwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetMenuInfo(IShellMenuCallback **ppsmc=%p, UINT *puId=%p, UINT *puIdAncestor=%p, DWORD *pdwFlags=%p)\n", this, ppsmc, puId, puIdAncestor, pdwFlags);
    HRESULT hr = m_IShellMenu->GetMenuInfo(ppsmc, puId, puIdAncestor, pdwFlags);
    if (ppsmc) WrapLogMsg("*ppsmc=%p\n", *ppsmc);
    if (puId) WrapLogMsg("*puId=%u\n", *puId);
    if (puIdAncestor) WrapLogMsg("*puIdAncestor=%u\n", *puIdAncestor);
    if (pdwFlags) WrapLogMsg("*pdwFlags=%08x\n", *pdwFlags);
    WrapLogExit("CMenuBandWrap::GetMenuInfo() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetShellFolder(IShellFolder *psf, LPCITEMIDLIST pidlFolder, HKEY hKey, DWORD dwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetShellFolder(IShellFolder *psf=%p, LPCITEMIDLIST pidlFolder=%p, HKEY hKey=%p, DWORD dwFlags=%08x)\n", this, psf, pidlFolder, hKey, dwFlags);
    HRESULT hr = m_IShellMenu->SetShellFolder(psf, pidlFolder, hKey, dwFlags);
    WrapLogExit("CMenuBandWrap::SetShellFolder() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetShellFolder(DWORD *pdwFlags, LPITEMIDLIST *ppidl, REFIID riid, void **ppv)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetShellFolder(DWORD *pdwFlags=%p, LPITEMIDLIST *ppidl=%p, REFIID riid=%s, void **ppv=%p)\n", this, pdwFlags, ppidl, Wrap(riid), ppv);
    HRESULT hr = m_IShellMenu->GetShellFolder(pdwFlags, ppidl, riid, ppv);
    if (pdwFlags) WrapLogMsg("*pdwFlags=%08x\n", *pdwFlags);
    if (ppidl) WrapLogMsg("*ppidl=%p\n", *ppidl);
    if (ppv) WrapLogMsg("*ppv=%p\n", *ppv);
    WrapLogExit("CMenuBandWrap::GetShellFolder() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetMenu(HMENU hmenu, HWND hwnd, DWORD dwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetMenu(HMENU hmenu=%p, HWND hwnd=%p, DWORD dwFlags=%08x)\n", this, hmenu, hwnd, dwFlags);
    HRESULT hr = m_IShellMenu->SetMenu(hmenu, hwnd, dwFlags);
    WrapLogExit("CMenuBandWrap::SetMenu() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetMenu(HMENU *phmenu, HWND *phwnd, DWORD *pdwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetMenu(HMENU *phmenu=%p, HWND *phwnd=%p, DWORD *pdwFlags=%p)\n", this, phmenu, phwnd, pdwFlags);
    HRESULT hr = m_IShellMenu->GetMenu(phmenu, phwnd, pdwFlags);
    if (phmenu) WrapLogMsg("*phmenu=%p\n", *phmenu);
    if (phwnd) WrapLogMsg("*phwnd=%p\n", *phwnd);
    if (pdwFlags) WrapLogMsg("*pdwFlags=%08x\n", *pdwFlags);
    WrapLogExit("CMenuBandWrap::GetMenu() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::InvalidateItem(LPSMDATA psmd, DWORD dwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::InvalidateItem(LPSMDATA psmd=%p, DWORD dwFlags=%08x)\n", this, psmd, dwFlags);
    HRESULT hr = m_IShellMenu->InvalidateItem(psmd, dwFlags);
    WrapLogExit("CMenuBandWrap::InvalidateItem() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::GetState(LPSMDATA psmd)
{
    WrapLogEnter("CMenuBandWrap<%p>::GetState(LPSMDATA psmd=%p)\n", this, psmd);
    HRESULT hr = m_IShellMenu->GetState(psmd);
    WrapLogExit("CMenuBandWrap::GetState() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuBandWrap::SetMenuToolbar(IUnknown *punk, DWORD dwFlags)
{
    WrapLogEnter("CMenuBandWrap<%p>::SetMenuToolbar(IUnknown *punk=%p, DWORD dwFlags=%08x)\n", this, punk, dwFlags);
    HRESULT hr = m_IShellMenu->SetMenuToolbar(punk, dwFlags);
    WrapLogExit("CMenuBandWrap::SetMenuToolbar() = %08x\n", hr);
    return hr;
}
