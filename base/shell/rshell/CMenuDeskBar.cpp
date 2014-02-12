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

WINE_DEFAULT_DEBUG_CHANNEL(CMenuDeskBar);

#define WRAP_LOG 1

class CMenuDeskBar :
    public CComCoClass<CMenuDeskBar>,
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
    CMenuDeskBar();
    ~CMenuDeskBar();

private:
#if WRAP_LOG
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
#else
#endif

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


    DECLARE_NOT_AGGREGATABLE(CMenuDeskBar)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMenuDeskBar)
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
HRESULT CMenuDeskBar_Constructor(REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    CMenuDeskBar * deskbar = new CComObject<CMenuDeskBar>();

    if (!deskbar)
        return E_OUTOFMEMORY;

    HRESULT hr = deskbar->QueryInterface(riid, ppv);

    if (FAILED(hr))
        deskbar->Release();

    return hr;
}

CMenuDeskBar::CMenuDeskBar()
{
    HRESULT hr;
    WrapLogOpen();

    hr = CoCreateInstance(CLSID_MenuDeskBar, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IMenuPopup, &m_IMenuPopup));
    hr = m_IMenuPopup->QueryInterface(IID_PPV_ARG(IUnknown, &m_IUnknown));

    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IOleCommandTarget, &m_IOleCommandTarget));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IServiceProvider, &m_IServiceProvider));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IDeskBar, &m_IDeskBar));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IOleWindow, &m_IOleWindow));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IInputObjectSite, &m_IInputObjectSite));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IInputObject, &m_IInputObject));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IObjectWithSite, &m_IObjectWithSite));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IBanneredBar, &m_IBanneredBar));
    hr = m_IUnknown->QueryInterface(IID_PPV_ARG(IInitializeObject, &m_IInitializeObject));
}

CMenuDeskBar::~CMenuDeskBar()
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
HRESULT STDMETHODCALLTYPE CMenuDeskBar::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    WrapLogEnter("CMenuDeskBar<%p>::Popup(POINTL *ppt=%p, RECTL *prcExclude=%p, MP_POPUPFLAGS dwFlags=%08x)\n", this, ppt, prcExclude, dwFlags);
    HRESULT hr = m_IMenuPopup->Popup(ppt, prcExclude, dwFlags);
    WrapLogExit("CMenuDeskBar::Popup() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnSelect(DWORD dwSelectType)
{
    WrapLogEnter("CMenuDeskBar<%p>::OnSelect(DWORD dwSelectType=%08x)\n", this, dwSelectType);
    HRESULT hr = m_IMenuPopup->OnSelect(dwSelectType);
    WrapLogExit("CMenuDeskBar::OnSelect() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    WrapLogEnter("CMenuDeskBar<%p>::SetSubMenu(IMenuPopup *pmp=%p, BOOL fSet=%d)\n", this, pmp, fSet);
    HRESULT hr = m_IMenuPopup->SetSubMenu(pmp, fSet);
    WrapLogExit("CMenuDeskBar::SetSubMenu() = %08x\n", hr);
    return hr;
}

// *** IOleWindow methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetWindow(HWND *phwnd)
{
    WrapLogEnter("CMenuDeskBar<%p>::GetWindow(HWND *phwnd=%p)\n", this, phwnd);
    HRESULT hr = m_IOleWindow->GetWindow(phwnd);
    if (phwnd) WrapLogMsg("*phwnd=%p\n", *phwnd);
    WrapLogExit("CMenuDeskBar::GetWindow() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::ContextSensitiveHelp(BOOL fEnterMode)
{
    WrapLogEnter("CMenuDeskBar<%p>::ContextSensitiveHelp(BOOL fEnterMode=%d)\n", this, fEnterMode);
    HRESULT hr = m_IOleWindow->ContextSensitiveHelp(fEnterMode);
    WrapLogExit("CMenuDeskBar::ContextSensitiveHelp() = %08x\n", hr);
    return hr;
}

// *** IObjectWithSite methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSite(IUnknown *pUnkSite)
{
    WrapLogEnter("CMenuDeskBar<%p>::SetSite(IUnknown *pUnkSite=%p)\n", this, pUnkSite);
    HRESULT hr = m_IObjectWithSite->SetSite(pUnkSite);
    WrapLogExit("CMenuDeskBar::SetSite() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetSite(REFIID riid, PVOID *ppvSite)
{
    WrapLogEnter("CMenuDeskBar<%p>::GetSite(REFIID riid=%s, PVOID *ppvSite=%p)\n", this, Wrap(riid), ppvSite);
    HRESULT hr = m_IObjectWithSite->GetSite(riid, ppvSite);
    if (ppvSite) WrapLogMsg("*ppvSite=%p\n", *ppvSite);
    WrapLogExit("CMenuDeskBar::GetSite() = %08x\n", hr);
    return hr;
}

// *** IBanneredBar methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetIconSize(DWORD iIcon)
{
    WrapLogEnter("CMenuDeskBar<%p>::SetIconSize(DWORD iIcon=%d)\n", this, iIcon);
    HRESULT hr = m_IBanneredBar->SetIconSize(iIcon);
    WrapLogExit("CMenuDeskBar::SetIconSize() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetIconSize(DWORD* piIcon)
{
    WrapLogEnter("CMenuDeskBar<%p>::GetIconSize(DWORD* piIcon=%p)\n", this, piIcon);
    HRESULT hr = m_IBanneredBar->GetIconSize(piIcon);
    if (piIcon) WrapLogMsg("*piIcon=%d\n", *piIcon);
    WrapLogExit("CMenuDeskBar::GetIconSize() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetBitmap(HBITMAP hBitmap)
{
    WrapLogEnter("CMenuDeskBar<%p>::SetBitmap(HBITMAP hBitmap=%p)\n", this, hBitmap);
    HRESULT hr = m_IBanneredBar->SetBitmap(hBitmap);
    WrapLogExit("CMenuDeskBar::SetBitmap() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetBitmap(HBITMAP* phBitmap)
{
    WrapLogEnter("CMenuDeskBar<%p>::GetBitmap(HBITMAP* phBitmap=%p)\n", this, phBitmap);
    HRESULT hr = m_IBanneredBar->GetBitmap(phBitmap);
    if (phBitmap) WrapLogMsg("*phBitmap=%p\n", *phBitmap);
    WrapLogExit("CMenuDeskBar::GetBitmap() = %08x\n", hr);
    return hr;
}


// *** IInitializeObject methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBar::Initialize(THIS)
{
    WrapLogEnter("CMenuDeskBar<%p>::Initialize()\n", this);
    HRESULT hr = m_IInitializeObject->Initialize();
    WrapLogExit("CMenuDeskBar::Initialize() = %08x\n", hr);
    return hr;
}

// *** IOleCommandTarget methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    WrapLogEnter("CMenuDeskBar<%p>::QueryStatus(const GUID *pguidCmdGroup=%p, ULONG cCmds=%u, prgCmds=%p, pCmdText=%p)\n", this, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    HRESULT hr = m_IOleCommandTarget->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    if (pguidCmdGroup) WrapLogMsg("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    WrapLogExit("CMenuDeskBar::QueryStatus() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    //bool b;

    WrapLogEnter("CMenuDeskBar<%p>::Exec(const GUID *pguidCmdGroup=%p, DWORD nCmdID=%d, DWORD nCmdexecopt=%d, VARIANT *pvaIn=%p, VARIANT *pvaOut=%p)\n", this, pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);

    //if (pguidCmdGroup && IsEqualGUID(*pguidCmdGroup, CLSID_MenuBand))
    //{
    //    if (nCmdID == 19) // popup
    //    {
    //        b = true;
    //    }
    //}


    if (pguidCmdGroup) WrapLogMsg("*pguidCmdGroup=%s\n", Wrap(*pguidCmdGroup));
    HRESULT hr = m_IOleCommandTarget->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvaIn, pvaOut);
    WrapLogExit("CMenuDeskBar::Exec() = %08x\n", hr);
    return hr;
}

// *** IServiceProvider methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    WrapLogEnter("CMenuDeskBar<%p>::QueryService(REFGUID guidService=%s, REFIID riid=%s, void **ppvObject=%p)\n", this, Wrap(guidService), Wrap(riid), ppvObject);

    //if (IsEqualIID(guidService, SID_SMenuBandChild))
    //{
    //    WrapLogMsg("SID is SID_SMenuBandChild. Using QueryInterface of self instead of wrapped object.\n");
    //    HRESULT hr = this->QueryInterface(riid, ppvObject);
    //    if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
    //    WrapLogExit("CMenuDeskBar::QueryService() = %08x\n", hr);
    //    return hr;
    //}
    //else
    {
        WrapLogMsg("SID not identified.\n");
    }
    HRESULT hr = m_IServiceProvider->QueryService(guidService, riid, ppvObject);
    if (ppvObject) WrapLogMsg("*ppvObject=%p\n", *ppvObject);
    WrapLogExit("CMenuDeskBar::QueryService() = %08x\n", hr);
    return hr;
}

// *** IInputObjectSite methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnFocusChangeIS(LPUNKNOWN lpUnknown, BOOL bFocus)
{
    WrapLogEnter("CMenuDeskBar<%p>::OnFocusChangeIS(LPUNKNOWN lpUnknown=%p, BOOL bFocus=%d)\n", this, lpUnknown, bFocus);
    HRESULT hr = m_IInputObjectSite->OnFocusChangeIS(lpUnknown, bFocus);
    WrapLogExit("CMenuDeskBar::OnFocusChangeIS() = %08x\n", hr);
    return hr;
}

// *** IInputObject methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBar::UIActivateIO(BOOL bActivating, LPMSG lpMsg)
{
    WrapLogEnter("CMenuDeskBar<%p>::UIActivateIO(BOOL bActivating=%d, LPMSG lpMsg=%p)\n", this, bActivating, lpMsg);
    HRESULT hr = m_IInputObject->UIActivateIO(bActivating, lpMsg);
    WrapLogExit("CMenuDeskBar::UIActivateIO() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::HasFocusIO(THIS)
{
    WrapLogEnter("CMenuDeskBar<%p>::HasFocusIO()\n", this);
    HRESULT hr = m_IInputObject->HasFocusIO();
    WrapLogExit("CMenuDeskBar::HasFocusIO() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::TranslateAcceleratorIO(LPMSG lpMsg)
{
    WrapLogEnter("CMenuDeskBar<%p>::TranslateAcceleratorIO(LPMSG lpMsg=%p)\n", this, lpMsg);
    if (lpMsg) WrapLogMsg("*lpMsg=%s\n", Wrap(*lpMsg));
    HRESULT hr = m_IInputObject->TranslateAcceleratorIO(lpMsg);
    WrapLogExit("CMenuDeskBar::TranslateAcceleratorIO() = %08x\n", hr);
    return hr;
}

// *** IDeskBar methods ***
HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetClient(IUnknown *punkClient)
{
    WrapLogEnter("CMenuDeskBar<%p>::SetClient(IUnknown *punkClient=%p)\n", this, punkClient);
    HRESULT hr = m_IDeskBar->SetClient(punkClient);
    WrapLogExit("CMenuDeskBar::SetClient() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetClient(IUnknown **ppunkClient)
{
    WrapLogEnter("CMenuDeskBar<%p>::GetClient(IUnknown **ppunkClient=%p)\n", this, ppunkClient);
    HRESULT hr = m_IDeskBar->GetClient(ppunkClient);
    if (ppunkClient) WrapLogMsg("*ppunkClient=%p\n", *ppunkClient);
    WrapLogExit("CMenuDeskBar::GetClient() = %08x\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnPosRectChangeDB(LPRECT prc)
{
    WrapLogEnter("CMenuDeskBar<%p>::OnPosRectChangeDB(RECT *prc=%p)\n", this, prc);
    HRESULT hr = m_IDeskBar->OnPosRectChangeDB(prc);
    if (prc) WrapLogMsg("*prc=%s\n", Wrap(*prc));
    WrapLogExit("CMenuDeskBar::OnPosRectChangeDB() = %08x\n", hr);
    return hr;
}
