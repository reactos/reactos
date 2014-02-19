/*
* Shell Menu Desk Bar
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

class CMenuDeskBarWrap :
    public CComCoClass<CMenuDeskBarWrap>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IOleCommandTarget,
    public IServiceProvider,
    public IInputObjectSite,
    public IInputObject,
    public IMenuPopup,
    public IObjectWithSite,
    public IBanneredBar,
    public IInitializeObject
{
public:
    CMenuDeskBarWrap();
    ~CMenuDeskBarWrap();

private:
    IUnknown * m_IUnknown;
    IMenuPopup * m_IMenuPopup;
    IOleCommandTarget * m_IOleCommandTarget;
    IServiceProvider * m_IServiceProvider;
    IDeskBar * m_IDeskBar;
    IOleWindow * m_IOleWindow;
    IInputObjectSite * m_IInputObjectSite;
    IInputObject * m_IInputObject;
    IObjectWithSite * m_IObjectWithSite;
    IBanneredBar * m_IBanneredBar;
    IInitializeObject * m_IInitializeObject;

public:
    // *** IMenuPopup methods ***
    virtual HRESULT STDMETHODCALLTYPE Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags);
    virtual HRESULT STDMETHODCALLTYPE OnSelect(DWORD dwSelectType);
    virtual HRESULT STDMETHODCALLTYPE SetSubMenu(IMenuPopup *pmp, BOOL fSet);

    // *** IOleWindow methods ***
    virtual HRESULT STDMETHODCALLTYPE GetWindow(HWND *phwnd);
    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL fEnterMode);

    // *** IObjectWithSite methods ***
    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite);
    virtual HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, PVOID *ppvSite);

    // *** IBanneredBar methods ***
    virtual HRESULT STDMETHODCALLTYPE SetIconSize(DWORD iIcon);
    virtual HRESULT STDMETHODCALLTYPE GetIconSize(DWORD* piIcon);
    virtual HRESULT STDMETHODCALLTYPE SetBitmap(HBITMAP hBitmap);
    virtual HRESULT STDMETHODCALLTYPE GetBitmap(HBITMAP* phBitmap);

    // *** IInitializeObject methods ***
    virtual HRESULT STDMETHODCALLTYPE Initialize(THIS);

    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText);
    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut);

    // *** IServiceProvider methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryService(REFGUID guidService, REFIID riid, void **ppvObject);

    // *** IInputObjectSite methods ***
    virtual HRESULT STDMETHODCALLTYPE OnFocusChangeIS(LPUNKNOWN lpUnknown, BOOL bFocus);

    // *** IInputObject methods ***
    virtual HRESULT STDMETHODCALLTYPE UIActivateIO(BOOL bActivating, LPMSG lpMsg);
    virtual HRESULT STDMETHODCALLTYPE HasFocusIO(THIS);
    virtual HRESULT STDMETHODCALLTYPE TranslateAcceleratorIO(LPMSG lpMsg);

    // *** IDeskBar methods ***
    virtual HRESULT STDMETHODCALLTYPE SetClient(IUnknown *punkClient);
    virtual HRESULT STDMETHODCALLTYPE GetClient(IUnknown **ppunkClient);
    virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(LPRECT prc);

    DECLARE_NOT_AGGREGATABLE(CMenuDeskBarWrap)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMenuDeskBarWrap)
        COM_INTERFACE_ENTRY_IID(IID_IMenuPopup, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
        COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
        COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
        COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBar, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IMenuPopup)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IBanneredBar, IBanneredBar)
        COM_INTERFACE_ENTRY_IID(IID_IInitializeObject, IInitializeObject)
    END_COM_MAP()
};

extern "C"
HRESULT CMenuDeskBar_Wrapper(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    CMenuDeskBarWrap * deskbar = new CComObject<CMenuDeskBarWrap>();

    if (!deskbar)
        return E_OUTOFMEMORY;

    HRESULT hr = deskbar->QueryInterface(riid, ppv);

    if (FAILED(hr))
        deskbar->Release();

    return hr;
}

CMenuDeskBarWrap::CMenuDeskBarWrap()
{
    WrapLogOpen();

    CoCreateInstance(CLSID_MenuDeskBar, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMenuPopup, &m_IMenuPopup));
    m_IMenuPopup->QueryInterface(IID_PPV_ARG(IUnknown, &m_IUnknown));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &m_IOleCommandTarget));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IServiceProvider, &m_IServiceProvider));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IDeskBar, &m_IDeskBar));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IOleWindow, &m_IOleWindow));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IInputObjectSite, &m_IInputObjectSite));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IInputObject, &m_IInputObject));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IObjectWithSite, &m_IObjectWithSite));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IBanneredBar, &m_IBanneredBar));
    m_IUnknown->QueryInterface(IID_PPV_ARG(IInitializeObject, &m_IInitializeObject));
}

CMenuDeskBarWrap::~CMenuDeskBarWrap()
{
    m_IUnknown->Release();
    m_IMenuPopup->Release();
    m_IOleCommandTarget->Release();
    m_IServiceProvider->Release();
    m_IDeskBar->Release();
    m_IOleWindow->Release();
    m_IInputObjectSite->Release();
    m_IInputObject->Release();
    m_IObjectWithSite->Release();
    m_IBanneredBar->Release();
    m_IInitializeObject->Release();

    WrapLogClose();
}

// *** IMenuPopup methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::Popup(POINTL *ppt=%p, RECTL *prcExclude=%p, MP_POPUPFLAGS dwFlags=%08x)\n", this, ppt, prcExclude, dwFlags);
    HRESULT hr = m_IMenuPopup->Popup(ppt, prcExclude, dwFlags);
    WrapLogExit("CMenuDeskBarWrap::Popup() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::OnSelect(DWORD dwSelectType)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::OnSelect(DWORD dwSelectType=%08x)\n", this, dwSelectType);
    HRESULT hr = m_IMenuPopup->OnSelect(dwSelectType);
    WrapLogExit("CMenuDeskBarWrap::OnSelect() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::SetSubMenu(IMenuPopup *pmp=%p, BOOL fSet=%d)\n", this, pmp, fSet);
    HRESULT hr = m_IMenuPopup->SetSubMenu(pmp, fSet);
    WrapLogExit("CMenuDeskBarWrap::SetSubMenu() = %08x\n", hr);
    return hr;
}

// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::GetWindow(HWND *phwnd)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::GetWindow(HWND *phwnd=%p)\n", this, phwnd);
    HRESULT hr = m_IOleWindow->GetWindow(phwnd);
    if (phwnd) WrapLogMsg("*phwnd=%p\n", *phwnd);
    WrapLogExit("CMenuDeskBarWrap::GetWindow() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::ContextSensitiveHelp(BOOL fEnterMode)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::ContextSensitiveHelp(BOOL fEnterMode=%d)\n", this, fEnterMode);
    HRESULT hr = m_IOleWindow->ContextSensitiveHelp(fEnterMode);
    WrapLogExit("CMenuDeskBarWrap::ContextSensitiveHelp() = %08x\n", hr);
    return hr;
}

// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::SetSite(IUnknown *pUnkSite)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::SetSite(IUnknown *pUnkSite=%p)\n", this, pUnkSite);
    HRESULT hr = m_IObjectWithSite->SetSite(pUnkSite);
    WrapLogExit("CMenuDeskBarWrap::SetSite() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::GetSite(REFIID riid, PVOID *ppvSite)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::GetSite(REFIID riid=%s, PVOID *ppvSite=%p)\n", this, Wrap(riid), ppvSite);
    HRESULT hr = m_IObjectWithSite->GetSite(riid, ppvSite);
    if (ppvSite) WrapLogMsg("*ppvSite=%p\n", *ppvSite);
    WrapLogExit("CMenuDeskBarWrap::GetSite() = %08x\n", hr);
    return hr;
}

// *** IBanneredBar methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::SetIconSize(DWORD iIcon)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::SetIconSize(DWORD iIcon=%d)\n", this, iIcon);
    HRESULT hr = m_IBanneredBar->SetIconSize(iIcon);
    WrapLogExit("CMenuDeskBarWrap::SetIconSize() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::GetIconSize(DWORD* piIcon)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::GetIconSize(DWORD* piIcon=%p)\n", this, piIcon);
    HRESULT hr = m_IBanneredBar->GetIconSize(piIcon);
    if (piIcon) WrapLogMsg("*piIcon=%d\n", *piIcon);
    WrapLogExit("CMenuDeskBarWrap::GetIconSize() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::SetBitmap(HBITMAP hBitmap)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::SetBitmap(HBITMAP hBitmap=%p)\n", this, hBitmap);
    HRESULT hr = m_IBanneredBar->SetBitmap(hBitmap);
    WrapLogExit("CMenuDeskBarWrap::SetBitmap() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::GetBitmap(HBITMAP* phBitmap)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::GetBitmap(HBITMAP* phBitmap=%p)\n", this, phBitmap);
    HRESULT hr = m_IBanneredBar->GetBitmap(phBitmap);
    if (phBitmap) WrapLogMsg("*phBitmap=%p\n", *phBitmap);
    WrapLogExit("CMenuDeskBarWrap::GetBitmap() = %08x\n", hr);
    return hr;
}


// *** IInitializeObject methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::Initialize(THIS)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::Initialize()\n", this);
    HRESULT hr = m_IInitializeObject->Initialize();
    WrapLogExit("CMenuDeskBarWrap::Initialize() = %08x\n", hr);
    return hr;
}

// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::QueryStatus(const GUID *pguidCmdGroup=%p, ULONG cCmds=%u, prgCmds=%p, pCmdText=%p)\n", this, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    HRESULT hr = m_IOleCommandTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    if (pguidCmdGroup) WrapLogMsg("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    WrapLogExit("CMenuDeskBarWrap::QueryStatus() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::Exec(const GUID *pguidCmdGroup=%p, DWORD nCmdID=%d, DWORD nCmdexecopt=%d, VARIANT *pvaIn=%p, VARIANT *pvaOut=%p)\n", this, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    if (pguidCmdGroup) WrapLogMsg("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    HRESULT hr = m_IOleCommandTarget->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    WrapLogExit("CMenuDeskBarWrap::Exec() = %08x\n", hr);
    return hr;
}

// *** IServiceProvider methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::QueryService(REFGUID guidService=%s, REFIID riid=%s, void **ppvObject=%p)\n", this, Wrap(guidService), Wrap(riid), ppvObject);

    if (IsEqualIID(guidService, SID_SMenuPopup))
    {
        WrapLogMsg("SID is SID_SMenuPopup. Using QueryInterface of self instead of wrapped object.\n");
        HRESULT hr = this->QueryInterface(riid, ppvObject);
        if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
        if (SUCCEEDED(hr))
        {
            WrapLogExit("CMenuDeskBarWrap::QueryService() = %08x\n", hr);
            return hr;
        }
        else
        {
            WrapLogMsg("QueryInterface on wrapper failed. Handing over to innter object.\n");
        }
    }
    else if (IsEqualIID(guidService, SID_SMenuBandParent))
    {
        WrapLogMsg("SID is SID_SMenuBandParent. Using QueryInterface of self instead of wrapped object.\n");
        HRESULT hr = this->QueryInterface(riid, ppvObject);
        if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
        if (SUCCEEDED(hr))
        {
            WrapLogExit("CMenuDeskBarWrap::QueryService() = %08x\n", hr);
            return hr;
        }
        else
        {
            WrapLogMsg("QueryInterface on wrapper failed. Handing over to innter object.\n");
        }
    }
    else if (IsEqualIID(guidService, SID_STopLevelBrowser))
    {
        WrapLogMsg("SID is SID_STopLevelBrowser. Using QueryInterface of self instead of wrapped object.\n");
        HRESULT hr = this->QueryInterface(riid, ppvObject);
        if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
        if (SUCCEEDED(hr))
        {
            WrapLogExit("CMenuDeskBarWrap::QueryService() = %08x\n", hr);
            return hr;
        }
        else
        {
            WrapLogMsg("QueryInterface on wrapper failed. Handing over to innter object.\n");
        }
    }
    else
    {
        WrapLogMsg("SID not identified. Calling wrapped object's QueryService.\n");
    }
    HRESULT hr = m_IServiceProvider->QueryService(guidService, riid, ppvObject);
    if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
    WrapLogExit("CMenuDeskBarWrap::QueryService() = %08x\n", hr);
    return hr;
}

// *** IInputObjectSite methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::OnFocusChangeIS(LPUNKNOWN lpUnknown, BOOL bFocus)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::OnFocusChangeIS(LPUNKNOWN lpUnknown=%p, BOOL bFocus=%d)\n", this, lpUnknown, bFocus);
    HRESULT hr = m_IInputObjectSite->OnFocusChangeIS(lpUnknown, bFocus);
    WrapLogExit("CMenuDeskBarWrap::OnFocusChangeIS() = %08x\n", hr);
    return hr;
}

// *** IInputObject methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::UIActivateIO(BOOL bActivating, LPMSG lpMsg)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::UIActivateIO(BOOL bActivating=%d, LPMSG lpMsg=%p)\n", this, bActivating, lpMsg);
    HRESULT hr = m_IInputObject->UIActivateIO(bActivating, lpMsg);
    WrapLogExit("CMenuDeskBarWrap::UIActivateIO() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::HasFocusIO(THIS)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::HasFocusIO()\n", this);
    HRESULT hr = m_IInputObject->HasFocusIO();
    WrapLogExit("CMenuDeskBarWrap::HasFocusIO() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::TranslateAcceleratorIO(LPMSG lpMsg)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::TranslateAcceleratorIO(LPMSG lpMsg=%p)\n", this, lpMsg);
    if (lpMsg) WrapLogMsg("*lpMsg=%s\n", Wrap(*lpMsg));
    HRESULT hr = m_IInputObject->TranslateAcceleratorIO(lpMsg);
    WrapLogExit("CMenuDeskBarWrap::TranslateAcceleratorIO() = %08x\n", hr);
    return hr;
}

// *** IDeskBar methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::SetClient(IUnknown *punkClient)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::SetClient(IUnknown *punkClient=%p)\n", this, punkClient);
    HRESULT hr = m_IDeskBar->SetClient(punkClient);
    WrapLogExit("CMenuDeskBarWrap::SetClient() = %08x\n", hr);

    CComPtr<IDeskBarClient> dbc;
    punkClient->QueryInterface(IID_PPV_ARG(IDeskBarClient, &dbc));
    dbc->SetDeskBarSite(static_cast<IDeskBar*>(this));

    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::GetClient(IUnknown **ppunkClient)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::GetClient(IUnknown **ppunkClient=%p)\n", this, ppunkClient);
    HRESULT hr = m_IDeskBar->GetClient(ppunkClient);
    if (ppunkClient) WrapLogMsg("*ppunkClient=%p\n", *ppunkClient);
    WrapLogExit("CMenuDeskBarWrap::GetClient() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBarWrap::OnPosRectChangeDB(LPRECT prc)
{
    WrapLogEnter("CMenuDeskBarWrap<%p>::OnPosRectChangeDB(RECT *prc=%p)\n", this, prc);
    HRESULT hr = m_IDeskBar->OnPosRectChangeDB(prc);
    if (prc) WrapLogMsg("*prc=%s\n", Wrap(*prc));
    WrapLogExit("CMenuDeskBarWrap::OnPosRectChangeDB() = %08x\n", hr);
    return hr;
}
