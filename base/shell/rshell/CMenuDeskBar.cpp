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
#include <atlwin.h>
#include <shlwapi_undoc.h>

WINE_DEFAULT_DEBUG_CHANNEL(CMenuDeskBar);

const static GUID CGID_MenuDeskBar = { 0x5C9F0A12, 0x959E, 0x11D0, { 0xA3, 0xA4, 0x00, 0xA0, 0xC9, 0x08, 0x26, 0x36 } };

typedef CWinTraits<
    WS_POPUP | WS_DLGFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
    WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_RIGHTSCROLLBAR | WS_EX_PALETTEWINDOW
> CMenuWinTraits;

class CMenuDeskBar :
    public CWindowImpl<CMenuDeskBar, CWindow, CMenuWinTraits>,
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
    CComPtr<IUnknown>   m_Site;
    CComPtr<IUnknown>   m_Client;
    CComPtr<IMenuPopup> m_SubMenuParent;

    HWND m_ClientWindow;

    DWORD m_IconSize;
    HBITMAP m_Banner;

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

    DECLARE_WND_CLASS_EX(_T("BaseBar"), CS_SAVEBITS | CS_DROPSHADOW, COLOR_3DFACE)

    BEGIN_MSG_MAP(CMenuDeskBar)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_WINDOWPOSCHANGED, OnWindowPosChanged)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
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

private:

    // message handlers
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
    LRESULT OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled);
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

CMenuDeskBar::CMenuDeskBar() :
    m_Client(NULL),
    m_Banner(NULL)
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
    CComPtr<IInputObjectSite> ios;

    HRESULT hr = m_Client->QueryInterface(IID_PPV_ARG(IInputObjectSite, &ios));
    if (FAILED(hr))
        return hr;

    return ios->OnFocusChangeIS(punkObj, fSetFocus);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
    OLECMD prgCmds [], OLECMDTEXT *pCmdText)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Exec(const GUID *pguidCmdGroup, DWORD nCmdID,
    DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
{
    if (IsEqualIID(*pguidCmdGroup, CGID_MenuDeskBar))
    {
        switch (nCmdID)
        {
        case 2: // refresh
            return S_OK;
        case 3: // load complete
            return S_OK;
        case 4: // set font metrics
            return S_OK;
        }
    }
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
    if (m_Site == NULL)
        return E_FAIL;

    if (IsEqualGUID(guidService, SID_SMenuPopup) ||
        IsEqualGUID(guidService, SID_SMenuBandParent) ||
        IsEqualGUID(guidService, SID_STopLevelBrowser))
    {
        return this->QueryInterface(riid, ppvObject);
    }

    return IUnknown_QueryService(m_Site, guidService, riid, ppvObject);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{
    return IUnknown_UIActivateIO(m_Client, fActivate, lpMsg);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::HasFocusIO()
{
    CComPtr<IInputObject> io;

    HRESULT hr = m_Client->QueryInterface(IID_PPV_ARG(IInputObject, &io));
    if (FAILED(hr))
        return hr;

    return io->HasFocusIO();
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::TranslateAcceleratorIO(LPMSG lpMsg)
{
    CComPtr<IInputObject> io;

    HRESULT hr = m_Client->QueryInterface(IID_PPV_ARG(IInputObject, &io));
    if (FAILED(hr))
        return hr;

    return io->TranslateAcceleratorIO(lpMsg);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetClient(IUnknown *punkClient)
{
    CComPtr<IDeskBarClient> pDeskBandClient;
    HRESULT hr;

    m_Client.Release();

    if (punkClient == NULL)
        return S_OK;

    if (m_hWnd == NULL)
    {
        Create(NULL);
    }

    hr = punkClient->QueryInterface(IID_PPV_ARG(IUnknown, &m_Client));
    if (FAILED(hr))
        return hr;

    hr = m_Client->QueryInterface(IID_PPV_ARG(IDeskBarClient, &pDeskBandClient));
    if (FAILED(hr))
        return hr;

    hr = pDeskBandClient->SetDeskBarSite(static_cast<IDeskBar*>(this));
    if (FAILED(hr))
        return hr;

    return IUnknown_GetWindow(m_Client, &m_ClientWindow);
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetClient(IUnknown **ppunkClient)
{
    if (ppunkClient == NULL)
        return E_POINTER;

    if (!m_Client)
        return E_FAIL;

    return m_Client->QueryInterface(IID_PPV_ARG(IUnknown, ppunkClient));
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::OnPosRectChangeDB(LPRECT prc)
{
    if (prc == NULL)
        return E_POINTER;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSite(IUnknown *pUnkSite)
{
    m_Site = pUnkSite;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::GetSite(REFIID riid, void **ppvSite)
{
    if (m_Site == NULL)
        return E_FAIL;

    return m_Site->QueryInterface(riid, ppvSite);
}

LRESULT CMenuDeskBar::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    HRESULT hr;

    if (m_Client)
    {
        RECT rc;

        GetClientRect(&rc);

        if (m_Banner != NULL)
        {
            BITMAP bm;
            ::GetObject(m_Banner, sizeof(bm), &bm);
            rc.left += bm.bmWidth;
        }

        ::SetWindowPos(m_ClientWindow, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, 0);
    }

    return 0;
}

LRESULT CMenuDeskBar::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    CComPtr<IWinEventHandler>               winEventHandler;
    LRESULT                                 result;
    HRESULT                                 hr;

    result = 0;
    if (m_Client.p != NULL)
    {
        hr = m_Client->QueryInterface(IID_PPV_ARG(IWinEventHandler, &winEventHandler));
        if (SUCCEEDED(hr) && winEventHandler.p != NULL)
            hr = winEventHandler->OnWinEvent(NULL, uMsg, wParam, lParam, &result);
    }
    return result;
}

LRESULT CMenuDeskBar::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    return 0;
}

LRESULT CMenuDeskBar::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    bHandled = FALSE;

    if (m_Banner && !m_IconSize)
    {
        BITMAP bm;
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(&ps);

        HDC hdcMem = ::CreateCompatibleDC(hdc);
        HGDIOBJ hbmOld = ::SelectObject(hdcMem, m_Banner);

        ::GetObject(m_Banner, sizeof(bm), &bm);

        RECT rc;
        if (!GetClientRect(&rc))
            WARN("GetClientRect failed\n");

        const int bx = bm.bmWidth;
        const int by = bm.bmHeight;
        const int cy = rc.bottom;

        TRACE("Painting banner: %d by %d\n", bm.bmWidth, bm.bmHeight);

        if (!::StretchBlt(hdc, 0, 0, bx, cy - by, hdcMem, 0, 0, bx, 1, SRCCOPY))
            WARN("StretchBlt failed\n");

        if (!::BitBlt(hdc, 0, cy - by, bx, by, hdcMem, 0, 0, SRCCOPY))
            WARN("BitBlt failed\n");

        ::SelectObject(hdcMem, hbmOld);
        ::DeleteDC(hdcMem);

        EndPaint(&ps);
    }

    return TRUE;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    HRESULT hr;
    CComPtr<IOleCommandTarget> oct;
    CComPtr<IInputObject> io;
    CComPtr<IDeskBand> band;
    CComPtr<IDeskBarClient> dbc;

    if (m_hWnd == NULL)
        return E_FAIL;

    hr = IUnknown_QueryService(m_Client, SID_SMenuBandChild, IID_PPV_ARG(IOleCommandTarget, &oct));
    if (FAILED(hr))
        return hr;

    hr = m_Client->QueryInterface(IID_PPV_ARG(IDeskBarClient, &dbc));
    if (FAILED(hr))
        return hr;

    hr = dbc->SetModeDBC(1);
    // Allow it to fail with E_NOTIMPL.

    // No clue about the arg, using anything != 0
    hr = dbc->UIActivateDBC(TRUE);
    if (FAILED(hr))
        return hr;

    RECT rc = { 0 };
    hr = dbc->GetSize(0, &rc);
    if (FAILED(hr))
        return hr;

    // Unknown meaning
    const int CMD = 19;
    const int CMD_EXEC_OPT = 0;

    hr = IUnknown_QueryServiceExec(m_Client, SID_SMenuBandChild, &CLSID_MenuBand, CMD, CMD_EXEC_OPT, NULL, NULL);
    if (FAILED(hr))
        return hr;

    ::AdjustWindowRect(&rc, ::GetWindowLong(m_hWnd, GWL_STYLE), FALSE);
    rc.right -= rc.left;
    rc.bottom -= rc.top;

    if (m_Banner != NULL)
    {
        BITMAP bm;
        ::GetObject(m_Banner, sizeof(bm), &bm);
        rc.right += bm.bmWidth;
    }

    int x = ppt->x;
    int y = ppt->y - rc.bottom;
    int cx = rc.right;
    int cy = rc.bottom;

    if (y < 0)
    {
        y = 0;
    }

    // if (y+cy > work area height) cy = work area height - y

    this->SetWindowPos(HWND_TOPMOST, x, y, cx, cy, SWP_SHOWWINDOW);

    // HACK: The bar needs to be notified of the size AFTER it is shown.
    // Quick & dirty way of getting it done.
    BOOL bHandled;
    OnSize(WM_SIZE, 0, 0, bHandled);

    UIActivateIO(TRUE, NULL);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetIconSize(THIS_ DWORD iIcon)
{
    HRESULT hr;
    m_IconSize = iIcon;

    // Unknown meaning (set flags? set icon size?)
    const int CMD = 16;
    const int CMD_EXEC_OPT = iIcon ? 0 : 2; // seems to work

    hr = IUnknown_QueryServiceExec(m_Client, SID_SMenuBandChild, &CLSID_MenuBand, CMD, CMD_EXEC_OPT, NULL, NULL);
    if (FAILED(hr))
        return hr;

    BOOL bHandled;
    OnSize(WM_SIZE, 0, 0, bHandled);

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

    BOOL bHandled;
    OnSize(WM_SIZE, 0, 0, bHandled);

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
    CComPtr<IDeskBarClient> dbc;
    HRESULT hr;
    
    bool bubbleUp = false;
    bool cancel = false;

    switch (dwSelectType)
    {
    case MPOS_FULLCANCEL:
    case MPOS_EXECUTE:
        bubbleUp = true;
        cancel = true;
        break;
    case MPOS_CANCELLEVEL:
        cancel = true;
        break;
    case MPOS_SELECTLEFT:
    case MPOS_SELECTRIGHT:
        // if unhandled, spread upwards?
        bubbleUp = true;
        break;
    case MPOS_CHILDTRACKING:
        break;
    }

    if (cancel)
    {
        hr = m_Client->QueryInterface(IID_PPV_ARG(IDeskBarClient, &dbc));
        if (FAILED(hr))
            return hr;

        hr = dbc->UIActivateDBC(FALSE);
        if (FAILED(hr))
            return hr;

        SetWindowPos(m_hWnd, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);

        UIActivateIO(FALSE, NULL);
    }

    //if (bubbleUp && m_Site)
    //{
    //    CComPtr<IMenuPopup> pmp;
    //    HRESULT hr = IUnknown_QueryService(m_Site, SID_SMenuPopup, IID_PPV_ARG(IMenuPopup, &pmp));
    //    if (FAILED(hr))
    //        return hr;
    //    pmp->OnSelect(dwSelectType);
    //}

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CMenuDeskBar::SetSubMenu(
    IMenuPopup *pmp,
    BOOL fSet)
{
    if (fSet)
    {
        m_SubMenuParent = pmp;
    }
    else
    {
        if (m_SubMenuParent)
        {
            if (SHIsSameObject(pmp, m_SubMenuParent))
            {
                m_SubMenuParent = NULL;
            }
        }
    }
    return S_OK;
}


HRESULT STDMETHODCALLTYPE CMenuDeskBar::Initialize(THIS)
{
    return S_OK;
}
