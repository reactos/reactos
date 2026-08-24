/*
 * ReactOS Explorer
 *
 * Copyright 2026 ReactOS contributors
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * window.cpp - CWin7StartMenu construction, layout, popup positioning,
 * dismissal and the COM/IOleWindow glue used by CTrayWindow.
 */

#include "../precomp.h"

CWin7StartMenu::CWin7StartMenu() :
    m_hwndLeft(NULL),
    m_hwndRight(NULL),
    m_hwndBar(NULL),
    m_hwndSearch(NULL),
    m_fnEditProc(NULL),
    m_HoverPane(PANE_NONE),
    m_HoverIndex(-1),
    m_ActivePane(PANE_NONE),
    m_ActiveIndex(-1),
    m_BarHover(0),
    m_Font(NULL),
    m_FontBold(NULL)
{
    m_szFilter[0] = 0;
}

CWin7StartMenu::~CWin7StartMenu()
{
    UnsubclassSearch();
    if (m_hWnd)
        DestroyWindow();
    ClearItems();
    if (m_Font)
        DeleteObject(m_Font);
    if (m_FontBold)
        DeleteObject(m_FontBold);
}

HRESULT CWin7StartMenu::Initialize(IN ITrayWindow *Tray)
{
    m_Tray = Tray;
    return S_OK;
}

/*****************************************************************************
 * construction
 */

BOOL CWin7StartMenu::CreateFonts()
{
    NONCLIENTMETRICSW ncm = { sizeof(ncm) };

    if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
        return FALSE;

    ncm.lfMessageFont.lfWeight = FW_NORMAL;
    m_Font = CreateFontIndirectW(&ncm.lfMessageFont);
    if (!m_Font)
        return FALSE;

    ncm.lfMessageFont.lfWeight = FW_BOLD;
    m_FontBold = CreateFontIndirectW(&ncm.lfMessageFont);

    return m_FontBold != NULL;
}

BOOL CWin7StartMenu::CreateMenuWindow()
{
    RECT rcTray;

    if (!::GetWindowRect(m_Tray->GetHWND(), &rcTray))
    {
        rcTray.left = rcTray.top = 0;
        rcTray.right = rcTray.bottom = 100;
    }

    DWORD dwStyleEx = SM7WinTraits::GetWndExStyle(0);
    DWORD dwStyle = SM7WinTraits::GetWndStyle(0);

    /* Parent-less popup; ATL Create(parent, rect, name, style, exstyle, id) */
    if (!Create(NULL, rcTray, NULL, dwStyle, dwStyleEx, 0U))
        return FALSE;

    /* Park the hidden window on the correct monitor so that later
       SetForegroundWindow calls behave predictably */
    HMONITOR hmon = MonitorFromWindow(m_Tray->GetHWND(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    if (GetMonitorInfoW(hmon, &mi))
    {
        SetWindowPos(NULL, mi.rcWork.left, mi.rcWork.bottom, 0, 0,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW |
                     SWP_NOZORDER);
    }

    return TRUE;
}

BOOL CWin7StartMenu::CreateChildren()
{
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN;
    HINSTANCE hInstance = _AtlBaseModule.GetModuleInstance();

    if (!SM7RegisterCanvasClass(hInstance))
        return FALSE;

    m_hwndLeft = CreateWindowExW(0, L"SM7Canvas", NULL, dwStyle,
                                 0, 0, 10, 10, m_hWnd,
                                 NULL, hInstance, NULL);
    m_hwndRight = CreateWindowExW(0, L"SM7Canvas", NULL, dwStyle,
                                  0, 0, 10, 10, m_hWnd,
                                  NULL, hInstance, NULL);
    m_hwndBar = CreateWindowExW(0, L"SM7Canvas", NULL, dwStyle,
                                0, 0, 10, 10, m_hWnd,
                                NULL, hInstance, NULL);

    m_hwndSearch = CreateWindowExW(WS_EX_CLIENTEDGE, WC_EDITW, NULL,
                                   WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                                   0, 0, 10, 10, m_hWnd,
                                   NULL, hInstance, NULL);

    if (!m_hwndLeft || !m_hwndRight || !m_hwndBar || !m_hwndSearch)
        return FALSE;

    AttachPane(m_hwndLeft, PANE_LEFT);
    AttachPane(m_hwndRight, PANE_RIGHT);
    AttachPane(m_hwndBar, PANE_BAR);

    SendMessageW(m_hwndSearch, WM_SETFONT, (WPARAM)m_Font, MAKELPARAM(TRUE, 0));

    /* Subclass the search box so special keys reach the menu logic */
    m_fnEditProc = (WNDPROC)::GetWindowLongPtrW(m_hwndSearch, GWLP_WNDPROC);
    ::SetWindowLongPtrW(m_hwndSearch, GWLP_WNDPROC, (LONG_PTR)SM7EditProc);
    SetSearchProps(m_hwndSearch);

    return TRUE;
}

/*****************************************************************************
 * layout
 */

VOID CWin7StartMenu::ComputeLayout()
{
    RECT rc;
    GetClientRect(&rc);

    int L = SM7_BORDER + 1;
    int T = SM7_BORDER + 1;
    int R = rc.right - SM7_BORDER - 1;
    int B = rc.bottom - SM7_BORDER - 1;

    m_rcLeft.left = L;
    m_rcLeft.top = T;
    m_rcLeft.right = L + SM7_LEFT_WIDTH;
    m_rcLeft.bottom = B - SM7_BAR_HEIGHT;

    m_rcRight.left = m_rcLeft.right + 1;
    m_rcRight.top = T;
    m_rcRight.right = R;
    m_rcRight.bottom = B - SM7_BAR_HEIGHT;

    m_rcBar.left = L;
    m_rcBar.top = B - SM7_BAR_HEIGHT + 1;
    m_rcBar.right = R;
    m_rcBar.bottom = B;

    /* the search strip occupies the bottom of the left pane */
    m_rcSearch.left = m_rcLeft.left + 6;
    m_rcSearch.right = m_rcLeft.right - 6;
    m_rcSearch.bottom = m_rcLeft.bottom - 6;
    m_rcSearch.top = m_rcSearch.bottom - SM7_SEARCH_HEIGHT;

    int barH = m_rcBar.bottom - m_rcBar.top;
    m_rcArrow.right = m_rcBar.right - 8;
    m_rcArrow.left = m_rcArrow.right - 26;
    m_rcArrow.top = m_rcBar.top + (barH - 28) / 2;
    m_rcArrow.bottom = m_rcArrow.top + 28;

    m_rcShutdown.right = m_rcArrow.left - 2;
    m_rcShutdown.left = m_rcShutdown.right - 92;
    m_rcShutdown.top = m_rcArrow.top;
    m_rcShutdown.bottom = m_rcArrow.bottom;
}

VOID CWin7StartMenu::ApplyChildrenLayout()
{
    RECT rc = m_rcLeft;
    rc.bottom = m_rcSearch.top - 4;
    ::MoveWindow(m_hwndLeft, rc.left, rc.top, rc.right - rc.left,
                 rc.bottom - rc.top, TRUE);
    ::MoveWindow(m_hwndRight, m_rcRight.left, m_rcRight.top,
                 m_rcRight.right - m_rcRight.left,
                 m_rcRight.bottom - m_rcRight.top, TRUE);
    ::MoveWindow(m_hwndBar, m_rcBar.left, m_rcBar.top,
                 m_rcBar.right - m_rcBar.left,
                 m_rcBar.bottom - m_rcBar.top, TRUE);
    ::MoveWindow(m_hwndSearch, m_rcSearch.left, m_rcSearch.top,
                 m_rcSearch.right - m_rcSearch.left,
                 m_rcSearch.bottom - m_rcSearch.top, TRUE);
}

VOID CWin7StartMenu::ResizeToContent()
{
    HMONITOR hmon = MonitorFromWindow(m_Tray->GetHWND(), MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    int cyMax = 600, cxMax = 800;

    if (GetMonitorInfoW(hmon, &mi))
    {
        cyMax = (mi.rcWork.bottom - mi.rcWork.top) - 80;
        cxMax = (mi.rcWork.right - mi.rcWork.left) - 40;
    }

    int cyLeft = (int)m_LeftItems.GetCount() * SM7_ITEM_HEIGHT + SM7_SEARCH_HEIGHT + 16;
    int cyRight = max((int)m_RightItems.GetCount(), 1) * SM7_RITEM_HEIGHT + 8;

    int cy = max(cyLeft, cyRight) + SM7_BAR_HEIGHT + 2 * SM7_BORDER + 2;
    cy = min(cy, cyMax);
    cy = max(cy, 260);

    int cx = min(SM7_LEFT_WIDTH + SM7_RIGHT_WIDTH + 2 * SM7_BORDER + 3, cxMax);

    ComputeLayout();

    SetWindowRgn(m_hWnd, CreateRoundRectRgn(0, 0, cx + 1, cy + 1,
                                            SM7_RADIUS * 2, SM7_RADIUS * 2), FALSE);
    SetWindowPos(NULL, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
    ApplyChildrenLayout();
}

/*****************************************************************************
 * show / hide
 */

STDMETHODIMP CWin7StartMenu::Popup(POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    if (!m_hWnd)
    {
        if (!CreateFonts() || !CreateMenuWindow() || !CreateChildren())
            return E_FAIL;
    }

    lstrcpynW(m_szFilter, L"", _countof(m_szFilter));
    SetWindowTextW(m_hwndSearch, L"");

    FillLeftItems();
    FillRightItems();
    ResizeToContent();

    RECT rcWin;
    GetWindowRect(&rcWin);
    int cx = rcWin.right - rcWin.left;
    int cy = rcWin.bottom - rcWin.top;

    POINT ptAnchor = { ppt->x, ppt->y };
    HMONITOR hmon = MonitorFromPoint(ptAnchor, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = { sizeof(mi) };
    if (!GetMonitorInfoW(hmon, &mi))
        GetMonitorInfoW(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi);

    int x = ppt->x;
    int y = ppt->y;

    if (dwFlags & MPPF_TOP)
        y = ppt->y - cy;
    else if (dwFlags & MPPF_BOTTOM)
        y = ppt->y;
    if (dwFlags & MPPF_LEFT)
        x = ppt->x - cx;

    if (prcExclude && (dwFlags & MPPF_TOP))
    {
        if (y + cy > prcExclude->top && y < prcExclude->bottom)
            y = prcExclude->top - cy;
    }

    x = max(mi.rcWork.left, min(x, mi.rcWork.right - cx));
    y = max(mi.rcWork.top, min(y, mi.rcWork.bottom - cy));

    m_ActivePane = ItemCount(PANE_LEFT) > 0 ? PANE_LEFT : PANE_NONE;
    m_ActiveIndex = ItemCount(PANE_LEFT) > 0 ? 0 : -1;
    SetHover(PANE_NONE, -1);
    m_BarHover = 0;

    SetWindowPos(HWND_TOPMOST, x, y, 0, 0, SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOSIZE);
    SetForegroundWindow(m_hWnd);
    SetFocus(m_hWnd);

    return S_OK;
}

VOID CWin7StartMenu::Hide()
{
    if (!m_hWnd || !IsWindowVisible())
        return;

    ShowWindow(SW_HIDE);
    if (m_Tray)
        Tray_OnStartMenuDismissed(m_Tray);
}

/*****************************************************************************
 * IOleWindow / IOleCommandTarget / IMenuPopup remainder
 */

STDMETHODIMP CWin7StartMenu::OnSelect(DWORD dwSelectType)
{
    UNREFERENCED_PARAMETER(dwSelectType);
    Hide();
    return S_OK;
}

STDMETHODIMP CWin7StartMenu::SetSubMenu(IMenuPopup *pmp, BOOL fSet)
{
    UNREFERENCED_PARAMETER(pmp);
    if (!fSet)
        Hide();
    return S_OK;
}

STDMETHODIMP CWin7StartMenu::GetWindow(HWND *phwnd)
{
    if (!phwnd)
        return E_POINTER;
    *phwnd = m_hWnd;
    return *phwnd ? S_OK : E_FAIL;
}

STDMETHODIMP CWin7StartMenu::ContextSensitiveHelp(BOOL fEnterMode)
{
    UNREFERENCED_PARAMETER(fEnterMode);
    return E_NOTIMPL;
}

/* This menu is self-contained: it owns no client object and ignores
   position change notifications coming from a menu site. */
STDMETHODIMP CWin7StartMenu::SetClient(IUnknown *punkClient)
{
    UNREFERENCED_PARAMETER(punkClient);
    return E_NOTIMPL;
}

STDMETHODIMP CWin7StartMenu::GetClient(IUnknown **ppunkClient)
{
    if (!ppunkClient)
        return E_POINTER;
    *ppunkClient = NULL;
    return S_FALSE;
}

STDMETHODIMP CWin7StartMenu::OnPosRectChangeDB(LPRECT prc)
{
    UNREFERENCED_PARAMETER(prc);
    return E_NOTIMPL;
}

STDMETHODIMP CWin7StartMenu::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds,
                                         OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    UNREFERENCED_PARAMETER(pguidCmdGroup);
    UNREFERENCED_PARAMETER(cCmds);
    UNREFERENCED_PARAMETER(prgCmds);
    UNREFERENCED_PARAMETER(pCmdText);
    return OLECMDERR_E_NOTSUPPORTED;
}

/* The classic tray asks its menu popup to refresh through IOleCommandTarget.
   This menu rebuilds its contents every time it pops up, so just accept it. */
STDMETHODIMP CWin7StartMenu::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt,
                                  VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    UNREFERENCED_PARAMETER(pguidCmdGroup);
    UNREFERENCED_PARAMETER(nCmdID);
    UNREFERENCED_PARAMETER(nCmdExecOpt);
    UNREFERENCED_PARAMETER(pvaIn);
    UNREFERENCED_PARAMETER(pvaOut);
    return S_OK;
}

/*****************************************************************************
 * create / destroy handlers
 */

LRESULT CWin7StartMenu::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    bHandled = TRUE;
    return 0;
}

LRESULT CWin7StartMenu::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL &bHandled)
{
    UNREFERENCED_PARAMETER(uMsg);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);
    bHandled = TRUE;

    UnsubclassSearch();
    m_hwndLeft = m_hwndRight = m_hwndBar = m_hwndSearch = NULL;
    return 0;
}

VOID CWin7StartMenu::UnsubclassSearch()
{
    if (m_hwndSearch && ::IsWindow(m_hwndSearch) && m_fnEditProc)
    {
        ClearSearchProps(m_hwndSearch);
        ::SetWindowLongPtrW(m_hwndSearch, GWLP_WNDPROC, (LONG_PTR)m_fnEditProc);
    }
    m_hwndSearch = NULL;
    m_fnEditProc = NULL;
}
