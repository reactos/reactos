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
This class knows how to contain base bar site in a cabinet window.
*/

#include "shellbars.h"

/*
Base bar that contains a vertical or horizontal explorer band. It also
provides resizing abilities.
*/
/*
TODO:
  **Make base bar support resizing -- almost done (need to support integral ?)
    Add context menu for base bar
    Fix base bar to correctly initialize fVertical field
    Fix base bar to correctly reposition its base bar site when resized -- done ?
*/

class CBaseBar :
    public CWindowImpl<CBaseBar, CWindow, CControlWinTraits>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IInputObjectSite,
    public IOleCommandTarget,
    public IServiceProvider,
    public IInputObject,
    public IDeskBar,
    public IDockingWindow,
    public IPersistStream,
    public IPersistStreamInit,
    public IPersistPropertyBag,
    public IObjectWithSite
{
private:
    CComPtr<IUnknown>                       fSite;
    CComPtr<IUnknown>                       fClient;
    HWND                                    fClientWindow;
    bool                                    fVertical;
    bool                                    fVisible;
    int                                     fNeededSize;        // width or height

    // used by resize tracking loop
    bool                                    fTracking;
    POINT                                   fLastLocation;
public:
    CBaseBar();
    ~CBaseBar();
    HRESULT Initialize(BOOL);

public:
    HRESULT ReserveBorderSpace();

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;

    // *** IInputObjectSite specific methods ***
    STDMETHOD(OnFocusChangeIS)(IUnknown *punkObj, BOOL fSetFocus) override;

    // *** IOleCommandTarget specific methods ***
    STDMETHOD(QueryStatus)(const GUID *pguidCmdGroup, ULONG cCmds,
        OLECMD prgCmds[  ], OLECMDTEXT *pCmdText) override;
    STDMETHOD(Exec)(const GUID *pguidCmdGroup, DWORD nCmdID,
        DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut) override;

    // *** IServiceProvider methods ***
    STDMETHOD(QueryService)(REFGUID guidService, REFIID riid, void **ppvObject) override;

    // *** IInputObject methods ***
    // forward the methods to the contained active bar
    STDMETHOD(UIActivateIO)(BOOL fActivate, LPMSG lpMsg) override;
    STDMETHOD(HasFocusIO)() override;
    STDMETHOD(TranslateAcceleratorIO)(LPMSG lpMsg) override;

    // *** IDeskBar methods ***
    STDMETHOD(SetClient)(IUnknown *punkClient) override;
    STDMETHOD(GetClient)(IUnknown **ppunkClient) override;
    STDMETHOD(OnPosRectChangeDB)(LPRECT prc) override;

    // *** IDockingWindow methods ***
    STDMETHOD(ShowDW)(BOOL fShow) override;
    STDMETHOD(CloseDW)(DWORD dwReserved) override;
    STDMETHOD(ResizeBorderDW)(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved) override;

    // *** IObjectWithSite methods ***
    STDMETHOD(SetSite)(IUnknown *pUnkSite) override;
    STDMETHOD(GetSite)(REFIID riid, void **ppvSite) override;

    // *** IPersist methods ***
    STDMETHOD(GetClassID)(CLSID *pClassID) override;

    // *** IPersistStream methods ***
    STDMETHOD(IsDirty)() override;
    STDMETHOD(Load)(IStream *pStm) override;
    STDMETHOD(Save)(IStream *pStm, BOOL fClearDirty) override;
    STDMETHOD(GetSizeMax)(ULARGE_INTEGER *pcbSize) override;

    // *** IPersistStreamInit methods ***
    STDMETHOD(InitNew)() override;

    // *** IPersistPropertyBag methods ***
    STDMETHOD(Load)(IPropertyBag *pPropBag, IErrorLog *pErrorLog) override;
    STDMETHOD(Save)(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties) override;

    // message handlers
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCancelMode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnCaptureChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);

DECLARE_WND_CLASS_EX(_T("BaseBar"), 0, COLOR_3DFACE)

BEGIN_MSG_MAP(CBaseBar)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
    MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_CANCELMODE, OnCancelMode)
    MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
END_MSG_MAP()

BEGIN_COM_MAP(CBaseBar)
    COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDockingWindow)
    COM_INTERFACE_ENTRY_IID(IID_IInputObjectSite, IInputObjectSite)
    COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
    COM_INTERFACE_ENTRY_IID(IID_IServiceProvider, IServiceProvider)
    COM_INTERFACE_ENTRY_IID(IID_IInputObject, IInputObject)
    COM_INTERFACE_ENTRY_IID(IID_IDeskBar, IDeskBar)
    COM_INTERFACE_ENTRY_IID(IID_IDockingWindow, IDockingWindow)
    COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
    COM_INTERFACE_ENTRY2_IID(IID_IPersist, IPersist, IPersistStream)
    COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
    COM_INTERFACE_ENTRY_IID(IID_IPersistStreamInit, IPersistStreamInit)
    COM_INTERFACE_ENTRY_IID(IID_IPersistPropertyBag, IPersistPropertyBag)
END_COM_MAP()
};

CBaseBar::CBaseBar()
{
    fClientWindow = NULL;
    fVertical = true;
    fVisible = false;
    fNeededSize = 200;
    fTracking = false;
}

CBaseBar::~CBaseBar()
{
}

HRESULT CBaseBar::Initialize(BOOL vert)
{
    fVertical = (vert == TRUE);
    return S_OK;
}

HRESULT CBaseBar::ReserveBorderSpace()
{
    CComPtr<IDockingWindowSite>             dockingWindowSite;
    RECT                                    availableBorderSpace;
    RECT                                    neededBorderSpace;
    HRESULT                                 hResult;

    hResult = fSite->QueryInterface(IID_PPV_ARG(IDockingWindowSite, &dockingWindowSite));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    hResult = dockingWindowSite->GetBorderDW(static_cast<IDeskBar *>(this), &availableBorderSpace);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    memset(&neededBorderSpace, 0, sizeof(neededBorderSpace));
    if (fVisible)
    {
        if (fVertical)
            neededBorderSpace.left = fNeededSize + GetSystemMetrics(SM_CXFRAME);
        else
            neededBorderSpace.bottom = fNeededSize + GetSystemMetrics(SM_CXFRAME);
    }
    hResult = dockingWindowSite->SetBorderSpaceDW(static_cast<IDeskBar *>(this), &neededBorderSpace);
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    return S_OK;
}

// current bar size is stored in the registry under
// key=HKCU\Software\Microsoft\Internet Explorer\Explorer Bars
// value=current bar GUID
// result is 8 bytes of binary data, 2 longs. First is the size, second is reserved and will always be 0
/*HRESULT CBaseBar::StopCurrentBar()
{
    CComPtr<IOleCommandTarget>              commandTarget;
    HRESULT                                 hResult;

    if (fCurrentBar.p != NULL)
    {
        hResult = fCurrentBar->QueryInterface(IID_IOleCommandTarget, (void **)&commandTarget);
        hResult = commandTarget->Exec(NULL, 0x17, 0, NULL, NULL);
    }
    // hide the current bar
    memcpy(&fCurrentActiveClass, &GUID_NULL, sizeof(fCurrentActiveClass));
    return S_OK;
}*/

HRESULT STDMETHODCALLTYPE CBaseBar::GetWindow(HWND *lphwnd)
{
    if (lphwnd == NULL)
        return E_POINTER;
    *lphwnd = m_hWnd;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::OnFocusChangeIS (IUnknown *punkObj, BOOL fSetFocus)
{
    return IUnknown_OnFocusChangeIS(fSite, punkObj, fSetFocus);
}

HRESULT STDMETHODCALLTYPE CBaseBar::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
    OLECMD prgCmds[  ], OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (IsEqualIID(*pguidCmdGroup, CGID_Explorer))
    {
        // pass through to the explorer ?
    }
    else if (IsEqualIID(*pguidCmdGroup, IID_IDeskBarClient))
    {
        switch (nCmdID)
        {
            case 0:
            {
                // hide current band
                ShowDW(0);

                // Inform our site that we closed
                VARIANT var;
                V_VT(&var) = VT_UNKNOWN;
                V_UNKNOWN(&var) = static_cast<IDeskBar *>(this);
                IUnknown_Exec(fSite, CGID_Explorer, 0x22, 0, &var, NULL);
                break;
            }
            case 2:
                // switch bands
                break;
            case 3:
                break;
        }
    }
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::QueryService(REFGUID guidService, REFIID riid, void **ppvObject)
{
    CComPtr<IServiceProvider>               serviceProvider;
    HRESULT                                 hResult;

    if (fSite == NULL)
        return E_FAIL;
    hResult = fSite->QueryInterface(IID_PPV_ARG(IServiceProvider, &serviceProvider));
    if (FAILED_UNEXPECTEDLY(hResult))
        return hResult;
    // called for SID_STopLevelBrowser, IID_IBrowserService to find top level browser
    // called for SID_IWebBrowserApp, IID_IConnectionPointContainer
    // connection point called for DIID_DWebBrowserEvents2 to establish connection
    return serviceProvider->QueryService(guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CBaseBar::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    return IUnknown_UIActivateIO(fClient, fActivate, lpMsg);
}

HRESULT STDMETHODCALLTYPE CBaseBar::HasFocusIO()
{
    return IUnknown_HasFocusIO(fClient);
}

HRESULT STDMETHODCALLTYPE CBaseBar::TranslateAcceleratorIO(LPMSG lpMsg)
{
    return IUnknown_TranslateAcceleratorIO(fClient, lpMsg);
}

HRESULT STDMETHODCALLTYPE CBaseBar::SetClient(IUnknown *punkClient)
{
    CComPtr<IOleWindow>                     oleWindow;
    HWND                                    ownerWindow;
    HRESULT                                 hResult;

    /* Clean up old client */
    fClient = NULL;

    if (punkClient)
    {
        hResult = punkClient->QueryInterface(IID_PPV_ARG(IUnknown, &fClient));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        hResult = fSite->QueryInterface(IID_PPV_ARG(IOleWindow, &oleWindow));
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        hResult = oleWindow->GetWindow(&ownerWindow);
        if (FAILED_UNEXPECTEDLY(hResult))
            return hResult;
        Create(ownerWindow, 0, NULL,
            WS_VISIBLE | WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_TOOLWINDOW);
        ReserveBorderSpace();
    }
    else
    {
        DestroyWindow();
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::GetClient(IUnknown **ppunkClient)
{
    if (ppunkClient == NULL)
        return E_POINTER;
    *ppunkClient = fClient;
    if (fClient.p != NULL)
        fClient.p->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::OnPosRectChangeDB(LPRECT prc)
{
    if (prc == NULL)
        return E_POINTER;
    if (fVertical)
        fNeededSize = prc->right - prc->left;
    else
        fNeededSize = prc->bottom - prc->top;
    ReserveBorderSpace();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::ShowDW(BOOL fShow)
{
    fVisible = fShow ? true : false;
    ShowWindow(fShow);
    ReserveBorderSpace();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::CloseDW(DWORD dwReserved)
{
    ShowDW(0);
    // Detach from our client
    SetClient(NULL);
    // Destroy our site
    SetSite(NULL);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::ResizeBorderDW(LPCRECT prcBorder, IUnknown *punkToolbarSite, BOOL fReserved)
{
    ReserveBorderSpace();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::SetSite(IUnknown *pUnkSite)
{
    fSite = pUnkSite;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::GetSite(REFIID riid, void **ppvSite)
{
    if (ppvSite == NULL)
        return E_POINTER;
    *ppvSite = fSite;
    if (fSite.p != NULL)
        fSite.p->AddRef();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CBaseBar::GetClassID(CLSID *pClassID)
{
    if (pClassID == NULL)
        return E_POINTER;
    // TODO: what class to return here?
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::IsDirty()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::Load(IStream *pStm)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::Save(IStream *pStm, BOOL fClearDirty)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    if (pcbSize == NULL)
        return E_POINTER;
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::InitNew()
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::Load(IPropertyBag *pPropBag, IErrorLog *pErrorLog)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CBaseBar::Save(IPropertyBag *pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    return E_NOTIMPL;
}

LRESULT CBaseBar::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    DWORD                                   dwWidth;
    DWORD                                   dwHeight;
    CComPtr<IOleWindow>                     pClient;
    HWND                                    clientHwnd;
    HRESULT                                 hr;

    if (fVisible)
    {
        dwWidth = LOWORD(lParam);
        dwHeight = HIWORD(lParam);

        // substract resizing grips to child's window size
        if (fVertical)
            dwWidth -= GetSystemMetrics(SM_CXFRAME);
        else
            dwHeight -= GetSystemMetrics(SM_CXFRAME);
        hr = fClient->QueryInterface(IID_PPV_ARG(IOleWindow, &pClient));
        if (FAILED_UNEXPECTEDLY(hr))
            return 0;
        hr = pClient->GetWindow(&clientHwnd);
        if (FAILED_UNEXPECTEDLY(hr))
            return 0;
        ::SetWindowPos(clientHwnd, NULL, 0, (fVertical) ? 0 : GetSystemMetrics(SM_CXFRAME), dwWidth, dwHeight, NULL);
        bHandled = TRUE;
    }
    return 0;
}

LRESULT CBaseBar::OnSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    if ((short)lParam != HTCLIENT || (HWND)wParam != m_hWnd)
    {
        bHandled = FALSE;
        return 0;
    }
    if (fVertical)
        SetCursor(LoadCursor(NULL, IDC_SIZEWE));
    else
        SetCursor(LoadCursor(NULL, IDC_SIZENS));
    return 1;
}

LRESULT CBaseBar::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    CComPtr<IWinEventHandler>               winEventHandler;
    LRESULT                                 result;
    HRESULT                                 hResult;

    result = 0;
    if (fClient.p != NULL)
    {
        hResult = fClient->QueryInterface(IID_PPV_ARG(IWinEventHandler, &winEventHandler));
        if (SUCCEEDED(hResult) && winEventHandler.p != NULL)
            hResult = winEventHandler->OnWinEvent(NULL, uMsg, wParam, lParam, &result);
    }
    return result;
}

LRESULT CBaseBar::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    SetCapture();
    fTracking = true;
    fLastLocation.x = (short)LOWORD(lParam);
    fLastLocation.y = (short)HIWORD(lParam);
    return 0;
}

LRESULT CBaseBar::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    ReleaseCapture();
    fTracking = false;
    return 0;
}

LRESULT CBaseBar::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    POINT                                   newLocation;
    int                                     delta;

    if (fTracking)
    {
        newLocation.x = (short)LOWORD(lParam);
        newLocation.y = (short)HIWORD(lParam);
        if (fVertical)
            delta = newLocation.x - fLastLocation.x;
        else
            delta = fLastLocation.y - newLocation.y;
        if (fNeededSize + delta < 0)
            return 0;
        fNeededSize += delta;
        fLastLocation = newLocation;
        ReserveBorderSpace();
    }
    return 0;
}

LRESULT CBaseBar::OnCancelMode(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    fTracking = false;
    return 0;
}

LRESULT CBaseBar::OnCaptureChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    fTracking = false;
    return 0;
}

HRESULT CBaseBar_CreateInstance(REFIID riid, void **ppv, BOOL vertical)
{
    return ShellObjectCreatorInit<CBaseBar, BOOL>(vertical, riid, ppv);
}
