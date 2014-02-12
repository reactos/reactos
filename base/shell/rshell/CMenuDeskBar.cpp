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
#include <atlwin.h>

WINE_DEFAULT_DEBUG_CHANNEL(CMenuDeskBar);

#define WRAP_LOG 1

typedef CWinTraits<
    WS_POPUP | WS_DLGFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
    WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_PALETTEWINDOW
> CMenuWinTraits;

class CMenuDeskBar :
#if !WRAP_LOG
    public CWindowImpl<CMenuDeskBar, CWindow, CMenuWinTraits>,
#endif
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

    CComPtr<IUnknown>                       m_Site;
    CComPtr<IUnknown>                       m_Client;
    HWND                                    m_ClientWindow;
    bool                                    m_Vertical;
    bool                                    m_Visible;
    int                                     m_NeededSize;        // width or height

    DWORD m_IconSize;
    HBITMAP m_Banner;

    // used by resize tracking loop
    bool                                    m_Tracking;
    POINT                                   m_LastLocation;
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

    // message handlers
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCancelMode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCaptureChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

    DECLARE_NOT_AGGREGATABLE(CMenuDeskBar)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

#if !WRAP_LOG
    DECLARE_WND_CLASS_EX(_T("BaseBar"), 0, COLOR_3DFACE)
#endif

    BEGIN_MSG_MAP(CMenuDeskBar)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_CANCELMODE, OnCancelMode)
        MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
        MESSAGE_HANDLER(WM_ACTIVATE, OnActivate)
/*        MESSAGE_HANDLER(WM_PAINT, OnPaint) */
    END_MSG_MAP()

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

#if WRAP_LOG
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
#else

CMenuDeskBar::CMenuDeskBar() :
    m_ClientWindow(NULL),
    m_Vertical(true),
    m_Visible(false),
    m_NeededSize(200),
    m_Tracking(false)
{
}

CMenuDeskBar::~CMenuDeskBar()
{
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnFocusChangeIS(IUnknown *punkObj, BOOL fSetFocus)
{
    // forward to owner
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
    OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (IsEqualIID(*pguidCmdGroup, CGID_Explorer))
    {
    }
    else if (IsEqualIID(*pguidCmdGroup, IID_IDeskBarClient))
    {
        switch (nCmdID)
        {
        case 0:
            // hide current band
            break;
        case 2:
            break;
        case 3:
            break;
        }
    }
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    CComPtr<IServiceProvider>               serviceProvider;
    HRESULT                                 hResult;

    if (m_Site == NULL)
        return E_FAIL;
    hResult = m_Site->QueryInterface(IID_IServiceProvider, reinterpret_cast<void **>(&serviceProvider));
    if (FAILED(hResult))
        return hResult;
    // called for SID_STopLevelBrowser, IID_IBrowserService to find top level browser
    // called for SID_IWebBrowserApp, IID_IConnectionPointContainer
    // connection point called for DIID_DWebBrowserEvents2 to establish connection
    return serviceProvider->QueryService(guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    // forward to contained bar
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::HasFocusIO()
{
    // forward to contained bar
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::TranslateAcceleratorIO(LPMSG lpMsg)
{
    // forward to contained bar
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetClient(IUnknown *punkClient)
{
    CComPtr<IDeskBarClient>                 pDeskBandClient;
    HRESULT                                 hResult;

    if (m_hWnd == NULL)
    {
        HWND ownerWindow = NULL;
        if (m_Site)
        {
            IOleWindow * oleWindow;

            hResult = m_Site->QueryInterface(IID_IOleWindow, reinterpret_cast<void **>(&oleWindow));
            if (SUCCEEDED(hResult))
                hResult = oleWindow->GetWindow(&ownerWindow);

            if (!::IsWindow(ownerWindow))
                return E_FAIL;
        }

        Create(ownerWindow);
    }

    if (punkClient == NULL)
        m_Client.Release();
    else
    {
        hResult = punkClient->QueryInterface(IID_IUnknown, reinterpret_cast<void **>(&m_Client));
        if (FAILED(hResult))
            return hResult;

        hResult = m_Client->QueryInterface(IID_IDeskBarClient, (VOID**) &pDeskBandClient);
        if (FAILED(hResult))
            return hResult;

        hResult = pDeskBandClient->SetDeskBarSite(static_cast<IDeskBar*>(this));
        if (FAILED(hResult))
            return hResult;

    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetClient(IUnknown **ppunkClient)
{
    if (ppunkClient == NULL)
        return E_POINTER;
    *ppunkClient = m_Client;
    if (m_Client.p != NULL)
        m_Client.p->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnPosRectChangeDB(LPRECT prc)
{
    if (prc == NULL)
        return E_POINTER;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSite(IUnknown *pUnkSite)
{
    return S_OK;
    CComPtr<IServiceProvider>               serviceProvider;
    CComPtr<IProfferService>                profferService;
    HRESULT                                 hResult;
    CComPtr<IOleWindow>                     oleWindow;
    HWND                                    ownerWindow;


    m_Site.Release();
    if (pUnkSite == NULL)
    {
        return S_OK;
    }

    if (m_hWnd == NULL)
    {
        // get window handle of parent
        hResult = pUnkSite->QueryInterface(IID_ITrayPriv, reinterpret_cast<void **>(&m_Site));
        if (FAILED(hResult))
            return hResult;

        hResult = pUnkSite->QueryInterface(IID_IOleWindow, reinterpret_cast<void **>(&oleWindow));
        if (SUCCEEDED(hResult))
            hResult = oleWindow->GetWindow(&ownerWindow);

        if (!::IsWindow(ownerWindow))
            return E_FAIL;

        Create(ownerWindow);
    }
    else
    {
        //Set Owner ?
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetSite(REFIID riid, void **ppvSite)
{
    if (ppvSite == NULL)
        return E_POINTER;
    *ppvSite = m_Site;
    if (m_Site.p != NULL)
        m_Site.p->AddRef();
    return S_OK;
}

LRESULT CMenuDeskBar::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HRESULT hr;

    if (m_Client)
    {
        CComPtr<IOleWindow> pOw;
        hr = m_Client->QueryInterface(IID_PPV_ARG(IOleWindow, &pOw));
        if (FAILED(hr))
        {
            ERR("IUnknown_QueryInterface pBs failed: %x\n", hr);
            return 0;
        }

        HWND clientWnd;
        pOw->GetWindow(&clientWnd);

        RECT rc;

        GetClientRect(&rc);

        if (m_Banner != NULL)
        {
            SIZE sz;
            ::GetBitmapDimensionEx(m_Banner, &sz);
            rc.left += sz.cx;
        }
        
        ::SetWindowPos(clientWnd, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
    }

    return 0;
}

LRESULT CMenuDeskBar::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    CComPtr<IWinEventHandler>               winEventHandler;
    LRESULT                                 result;
    HRESULT                                 hResult;

    result = 0;
    if (m_Client.p != NULL)
    {
        hResult = m_Client->QueryInterface(IID_IWinEventHandler, reinterpret_cast<void **>(&winEventHandler));
        if (SUCCEEDED(hResult) && winEventHandler.p != NULL)
            hResult = winEventHandler->OnWinEvent(NULL, uMsg, wParam, lParam, &result);
    }
    return result;
}

LRESULT CMenuDeskBar::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    SetCapture();
    m_Tracking = true;
    m_LastLocation.x = LOWORD(lParam);
    m_LastLocation.y = HIWORD(lParam);
    return 0;
}

LRESULT CMenuDeskBar::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    ReleaseCapture();
    m_Tracking = false;
    return 0;
}

LRESULT CMenuDeskBar::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    POINT                                   newLocation;
    //int                                     delta;

    if (m_Tracking)
    {
        newLocation.x = (short) LOWORD(lParam);
        newLocation.y = (short) HIWORD(lParam);
        m_LastLocation = newLocation;
    }
    return 0;
}

LRESULT CMenuDeskBar::OnCancelMode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    m_Tracking = false;
    return 0;
}

LRESULT CMenuDeskBar::OnCaptureChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    m_Tracking = false;
    return 0;
}

LRESULT CMenuDeskBar::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    return 0;
}

LRESULT CMenuDeskBar::OnActivate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if (LOWORD(wParam) == WA_INACTIVE)
    {
        //DestroyWindow();
        //ShowWindow(SW_HIDE);
    }
    return 0;
}

LRESULT CMenuDeskBar::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    TRACE("OnPaint\n");

    if (m_Banner && !m_IconSize)
    {
        BITMAP bm;
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(&ps);

        HDC hdcMem = ::CreateCompatibleDC(hdc);
        HGDIOBJ hbmOld = ::SelectObject(hdcMem, m_Banner);

        ::GetObject(m_Banner, sizeof(bm), &bm);

        TRACE("Painting banner: %d by %d\n", bm.bmWidth, bm.bmHeight);

        if (!::BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY))
            WARN("BitBlt failed\n");

        ::SelectObject(hdcMem, hbmOld);
        ::DeleteDC(hdcMem);

        EndPaint(&ps);
    }

    return 0;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    //ENTER >> CMenuDeskBar<03901CC0>::Popup(POINTL *ppt=00B3F428, RECTL *prcExclude=00B3F418, MP_POPUPFLAGS dwFlags=80000000)
    //  ENTER >> CMenuBand<00D2CCF8>::QueryService

    //  ENTER >> CMenuBand<00D2CCF8>::ShowDW(BOOL fShow=1)
    //    ENTER >> CMenuDeskBar<03901CC0>::GetSite(REFIID riid={4622AD10-FF23-11D0-8D34-00A0C90F2719}, PVOID *ppvSite=03901D4C)
    //      -- *ppvSite=00BDEA90
    //    EXIT <<< CMenuDeskBar::GetSite() = 00000000
    //    ENTER >> CMenuBand<00D2CCF8>::SetMenu(HMENU hmenu=593F0A07, HWND hwnd=00000000, DWORD dwFlags=20000000)
    //    EXIT <<< CMenuBand::SetMenu() = 00000000
    //  EXIT <<< CMenuBand::ShowDW() = 00000000
    //  ENTER >> CMenuBand<00D2CCF8>::GetBandInfo(DWORD dwBandID=0, DWORD dwViewMode=0, DESKBANDINFO *pdbi=00B3F0F0)
    //  EXIT <<< CMenuBand::GetBandInfo() = 00000000
    //  ENTER >> CMenuBand<00D2CCF8>::QueryService(REFGUID guidService={ED9CC020-08B9-11D1-9823-00C04FD91972}, REFIID riid={B722BCCB-4E68-101B-A2BC-00AA00404770}, void **ppvObject=00B3F304)
    //    -- SID is SID_SMenuBandChild. Using QueryInterface of self instead of wrapped object.
    //    -- *ppvObject=00D2CD08
    //  EXIT <<< CMenuBand::QueryService() = 00000000
    //  ENTER >> CMenuBand<00D2CCF8>::Exec(const GUID *pguidCmdGroup=76BAE1FC, DWORD nCmdID=19, DWORD nCmdexecopt=0, VARIANT *pvaIn=00000000, VARIANT *pvaOut=00000000)
    //    -- *pguidCmdGroup={5B4DAE26-B807-11D0-9815-00C04FD91972}
    //  EXIT <<< CMenuBand::Exec() = 00000001
    //  ENTER >> CMenuBand<00D2CCF8>::OnPosRectChangeDB(RECT *prc=00B3E630)
    //    -- *prc={L: 0, T: 0, R: 218, B: 305}
    //  EXIT <<< CMenuBand::OnPosRectChangeDB() = 00000000
    //  ENTER >> CMenuBand<00D2CCF8>::UIActivateIO(BOOL fActivate=1, LPMSG lpMsg=00000000)
    //  EXIT <<< CMenuBand::UIActivateIO() = 00000001
    //EXIT <<< CMenuDeskBar::Popup() = 00000000
    HRESULT hr;
    IServiceProvider * sp;
    IOleCommandTarget * oct;
    IInputObject * io;

    hr = m_Client->QueryInterface(IID_PPV_ARG(IServiceProvider, &sp));
    if (FAILED(hr))
        return hr;

    hr = m_Client->QueryInterface(IID_PPV_ARG(IInputObject, &io));
    if (FAILED(hr))
        return hr;

    hr = sp->QueryService(SID_SMenuBandChild, IID_PPV_ARG(IOleCommandTarget, &oct));
    if (FAILED(hr))
    {
        sp->Release();
        return hr;
    }

    // Unknown meaning
    const int CMD = 19;
    const int CMD_EXEC_OPT = 0;

    hr = oct->Exec(&CLSID_MenuBand, CMD, CMD_EXEC_OPT, NULL, NULL);

    oct->Release();
    sp->Release();
    return hr;

    // FIXME: everything!
    const int hackWidth = 200;
    const int hackHeight = 400;
    RECT r = { ppt->x, ppt->y - hackHeight, ppt->x + hackWidth, ppt->y };
    if (this->m_hWnd == NULL)
    {
        this->Create(NULL, &r);
    }
    this->SetWindowPos(NULL, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_SHOWWINDOW);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetIconSize(THIS_ DWORD iIcon)
{
    HRESULT hr;
    IServiceProvider * sp;
    IOleCommandTarget * oct;

    m_IconSize = iIcon;

    hr = m_Client->QueryInterface(IID_PPV_ARG(IServiceProvider, &sp));
    if (FAILED(hr))
        return hr;

    hr = sp->QueryService(SID_SMenuBandChild, IID_PPV_ARG(IOleCommandTarget, &oct));
    if (FAILED(hr))
    {
        sp->Release();
        return hr;
    }

    // Unknown meaning
    const int CMD = 16;
    const int CMD_EXEC_OPT = 2;

    hr = oct->Exec(&CLSID_MenuBand, CMD, CMD_EXEC_OPT, NULL, NULL);

    oct->Release();
    sp->Release();
    return hr;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetIconSize(THIS_ DWORD* piIcon)
{
    if (piIcon)
        *piIcon = m_IconSize;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetBitmap(THIS_ HBITMAP hBitmap)
{
    m_Banner = hBitmap;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetBitmap(THIS_ HBITMAP* phBitmap)
{
    if (phBitmap)
        *phBitmap = m_Banner;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnSelect(
    DWORD dwSelectType)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSubMenu(
    IMenuPopup *pmp,
    BOOL fSet)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Initialize(THIS)
{
    return S_OK;
}

#endif