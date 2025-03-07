/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Show Desktop tray button implementation
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2018-2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *              Copyright 2023 Ethan Rodensky <splitwirez@gmail.com>
 */

#include "precomp.h"
#include <commoncontrols.h>
#include <uxtheme.h>

#define IDI_SHELL32_DESKTOP 35
#define IDI_IMAGERES_DESKTOP 110

#define SHOW_DESKTOP_TIMER_ID 999
#define SHOW_DESKTOP_TIMER_INTERVAL 200

CTrayShowDesktopButton::CTrayShowDesktopButton() :
    m_nClickedTime(0),
    m_inset({2, 2}),
    m_icon(NULL),
    m_highContrastMode(FALSE),
    m_drawWithDedicatedBackground(FALSE),
    m_bHovering(FALSE),
    m_hWndTaskbar(NULL),
    m_bPressed(FALSE),
    m_bHorizontal(FALSE)
{
}

INT CTrayShowDesktopButton::WidthOrHeight() const
{
    if (IsThemeActive() && !m_highContrastMode)
    {
        if (m_drawWithDedicatedBackground)
        {
            if (GetSystemMetrics(SM_TABLETPC))
            {
                //TODO: DPI scaling - return logical-to-physical conversion of 24, not fixed value
                return 24;
            }
            else
                return 15;
        }
        else
        {
            INT CurMargin = m_bHorizontal
                ? (m_ContentMargins.cxLeftWidth + m_ContentMargins.cxRightWidth)
                : (m_ContentMargins.cyTopHeight + m_ContentMargins.cyBottomHeight);
            return max(16 + CurMargin, 18) + 6;
        }
    }
    else
    {
        return max(2 * GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXSMICON),
                   2 * GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYSMICON));
    }
}

HRESULT CTrayShowDesktopButton::DoCreate(HWND hwndParent)
{
    const DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_DEFPUSHBUTTON;
    Create(hwndParent, NULL, NULL, style);

    if (!m_hWnd)
        return E_FAIL;

    // Get desktop icon
    bool bIconRetrievalFailed = ExtractIconExW(L"imageres.dll", -IDI_IMAGERES_DESKTOP, NULL, &m_icon, 1) == UINT_MAX;
    if (bIconRetrievalFailed || !m_icon)
        ExtractIconExW(L"shell32.dll", -IDI_SHELL32_DESKTOP, NULL, &m_icon, 1);

    // Get appropriate size at which to display desktop icon
    m_szIcon.cx = GetSystemMetrics(SM_CXSMICON);
    m_szIcon.cy = GetSystemMetrics(SM_CYSMICON);

    // Create tooltip
    m_tooltip.Create(m_hWnd, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP);

    TOOLINFOW ti = { 0 };
    ti.cbSize = TTTOOLINFOW_V1_SIZE;
    ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
    ti.hwnd = m_hWnd;
    ti.uId = reinterpret_cast<UINT_PTR>(m_hWnd);
    ti.hinst = hExplorerInstance;
    ti.lpszText = MAKEINTRESOURCEW(IDS_TRAYDESKBTN_TOOLTIP);

    m_tooltip.AddTool(&ti);

    // Prep visual style
    EnsureWindowTheme(TRUE);

    // Get HWND of Taskbar
    m_hWndTaskbar = ::GetParent(hwndParent);
    if (!::IsWindow(m_hWndTaskbar))
        return E_FAIL;

    return S_OK;
}

LRESULT CTrayShowDesktopButton::OnClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // The actual action can be delayed as an expected behaviour.
    // But a too late action is an unexpected behaviour.
    LONG nTime0 = m_nClickedTime;
    LONG nTime1 = ::GetMessageTime();
    if (nTime1 - nTime0 >= 600) // Ignore after 0.6 sec
        return 0;

    // Show/Hide Desktop
    ::SendMessage(m_hWndTaskbar, WM_COMMAND, TRAYCMD_TOGGLE_DESKTOP, 0);

    return 0;
}

// This function is called from OnLButtonDown and parent.
VOID CTrayShowDesktopButton::Click()
{
    // The actual action can be delayed as an expected behaviour.
    m_nClickedTime = ::GetMessageTime();
    PostMessage(TSDB_CLICK, 0, 0);
}

LRESULT CTrayShowDesktopButton::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_bPressed = FALSE;
    ReleaseCapture();
    Invalidate(TRUE);

    POINT pt;
    ::GetCursorPos(&pt);
    if (PtInButton(&pt))
        Click(); // Left-click
    return 0;
}

LRESULT CTrayShowDesktopButton::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_bPressed = TRUE;
    SetCapture();
    Invalidate(TRUE);
    return 0;
}

LRESULT CTrayShowDesktopButton::OnSettingChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT ret = OnThemeChanged(uMsg, wParam, lParam, bHandled);
    EnsureWindowTheme(TRUE);
    return ret;
}

LRESULT CTrayShowDesktopButton::OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HIGHCONTRAST hcInfo;
    hcInfo.cbSize = sizeof(hcInfo);
    if (SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(hcInfo), &hcInfo, FALSE))
        m_highContrastMode = (hcInfo.dwFlags & HCF_HIGHCONTRASTON);

    if (m_hTheme)
    {
        ::CloseThemeData(m_hTheme);
        m_hTheme = NULL;
    }
    if (m_hFallbackTheme)
    {
        ::CloseThemeData(m_hFallbackTheme);
        m_hFallbackTheme = NULL;
    }

    EnsureWindowTheme(FALSE);

    Invalidate(TRUE);
    return 0;
}

LRESULT CTrayShowDesktopButton::OnWindowPosChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    EnsureWindowTheme(TRUE);
    return 0;
}

VOID CTrayShowDesktopButton::EnsureWindowTheme(BOOL setTheme)
{
    if (setTheme)
        SetWindowTheme(m_hWnd, m_bHorizontal ? L"ShowDesktop" : L"VerticalShowDesktop", NULL);

    if (::IsWindow(m_hWndTaskbar))
    {
        m_hFallbackTheme = ::OpenThemeData(m_hWndTaskbar, L"Toolbar");
        GetThemeMargins(m_hFallbackTheme, NULL, TP_BUTTON, 0, TMT_CONTENTMARGINS, NULL, &m_ContentMargins);
    }
    else
    {
        m_hFallbackTheme = NULL;
    }

    MARGINS contentMargins;
    if (GetThemeMargins(GetWindowTheme(GetParent().m_hWnd), NULL, TNP_BACKGROUND, 0, TMT_CONTENTMARGINS, NULL, &contentMargins) == S_OK)
    {
        m_inset.cx = max(0, contentMargins.cxRightWidth - 5);
        m_inset.cy = max(0, contentMargins.cyBottomHeight - 5);
    }
    else
    {
        m_inset.cx = 2;
        m_inset.cy = 2;
    }

    m_drawWithDedicatedBackground = FALSE;
    if (IsThemeActive())
    {
        m_hTheme = OpenThemeData(m_hWnd, L"Button");
        if (m_hTheme != NULL)
            m_drawWithDedicatedBackground = !IsThemePartDefined(m_hTheme, BP_PUSHBUTTON, 0);
    }
}

LRESULT CTrayShowDesktopButton::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rc;
    GetClientRect(&rc);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);
    OnDraw(hdc, &rc);
    EndPaint(&ps);

    return 0;
}

LRESULT CTrayShowDesktopButton::OnPrintClient(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if ((lParam & PRF_CHECKVISIBLE) && !IsWindowVisible())
        return 0;

    RECT rc;
    GetClientRect(&rc);

    HDC hdc = (HDC)wParam;
    OnDraw(hdc, &rc);

    return 0;
}

BOOL CTrayShowDesktopButton::PtInButton(LPPOINT ppt) const
{
    if (!ppt || !IsWindow())
        return FALSE;

    RECT rc;
    GetWindowRect(&rc);
    INT cxEdge = ::GetSystemMetrics(SM_CXEDGE), cyEdge = ::GetSystemMetrics(SM_CYEDGE);
    ::InflateRect(&rc, max(cxEdge, 1), max(cyEdge, 1));

    return m_bHorizontal
        ? (ppt->x > rc.left)
        : (ppt->y > rc.top);
}

VOID CTrayShowDesktopButton::StartHovering()
{
    if (m_bHovering)
        return;

    m_bHovering = TRUE;
    Invalidate(TRUE);

    SetTimer(SHOW_DESKTOP_TIMER_ID, SHOW_DESKTOP_TIMER_INTERVAL, NULL);

    ::PostMessage(m_hWndTaskbar, WM_NCPAINT, 0, 0);
}

LRESULT CTrayShowDesktopButton::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    StartHovering();
    return 0;
}

LRESULT CTrayShowDesktopButton::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam != SHOW_DESKTOP_TIMER_ID || !m_bHovering)
        return 0;

    POINT pt;
    ::GetCursorPos(&pt);
    if (!PtInButton(&pt)) // The end of hovering?
    {
        m_bHovering = FALSE;
        KillTimer(SHOW_DESKTOP_TIMER_ID);
        Invalidate(TRUE);

        ::PostMessage(m_hWndTaskbar, WM_NCPAINT, 0, 0);
    }

    return 0;
}

LRESULT CTrayShowDesktopButton::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_hTheme)
    {
        ::CloseThemeData(m_hTheme);
        m_hTheme = NULL;
    }
    if (m_hFallbackTheme)
    {
        ::CloseThemeData(m_hFallbackTheme);
        m_hFallbackTheme = NULL;
    }

    return 0;
}

VOID CTrayShowDesktopButton::OnDraw(HDC hdc, LPRECT prc)
{
    RECT rc = { prc->left, prc->top, prc->right, prc->bottom };
    LPRECT lpRc = &rc;
    HBRUSH hbrBackground = NULL;

    if (m_hTheme)
    {
        HTHEME theme;
        int part = 0;
        int state = 0;
        if (m_drawWithDedicatedBackground)
        {
            theme = m_hTheme;

            if (m_bPressed)
                state = PBS_PRESSED;
            else if (m_bHovering)
                state = PBS_HOT;
            else
                state = PBS_NORMAL;
        }
        else
        {
            part = TP_BUTTON;
            theme = m_hFallbackTheme;

            if (m_bPressed)
                state = TS_PRESSED;
            else if (m_bHovering)
                state = TS_HOT;
            else
                state = TS_NORMAL;

            if (m_bHorizontal)
                rc.right -= m_inset.cx;
            else
                rc.bottom -= m_inset.cy;
        }

        if (::IsThemeBackgroundPartiallyTransparent(theme, part, state))
            ::DrawThemeParentBackground(m_hWnd, hdc, NULL);

        ::DrawThemeBackground(theme, hdc, part, state, lpRc, lpRc);
    }
    else
    {
        hbrBackground = ::GetSysColorBrush(COLOR_3DFACE);
        ::FillRect(hdc, lpRc, hbrBackground);

        if (m_bPressed || m_bHovering)
        {
            UINT edge = m_bPressed ? BDR_SUNKENOUTER : BDR_RAISEDINNER;
            DrawEdge(hdc, lpRc, edge, BF_RECT);
        }
    }

    if (m_highContrastMode || !m_drawWithDedicatedBackground)
    {
        /* Prepare to draw icon */

        // Determine X-position of icon's top-left corner
        int iconX = rc.left;
        iconX += (rc.right - iconX) / 2;
        iconX -= m_szIcon.cx / 2;

        // Determine Y-position of icon's top-left corner
        int iconY = rc.top;
        iconY += (rc.bottom - iconY) / 2;
        iconY -= m_szIcon.cy / 2;

        // Ok now actually draw the icon itself
        if (m_icon)
        {
            DrawIconEx(hdc, iconX, iconY,
                       m_icon, 0, 0,
                       0, hbrBackground, DI_NORMAL);
        }
    }
}
