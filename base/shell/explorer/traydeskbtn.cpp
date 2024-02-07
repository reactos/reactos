/*
 * PROJECT:     ReactOS Explorer
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Show Desktop tray button implementation
 * COPYRIGHT:   Copyright 2006-2007 Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2018-2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "CTrayShowDesktopButton.h"
#include <limits.h>

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
    m_bPressed(FALSE),
    IsHorizontal(FALSE)
{
}

BOOL
CTrayShowDesktopButton::GetTaskbar(OUT HWND* taskbarWnd)
{
    *taskbarWnd = NULL;
    auto parent = GetParent();
    if (!parent)
        return FALSE;

    parent = parent.GetParent();
    if (!parent)
        return FALSE;

    HWND wnd = parent.m_hWnd;
    *taskbarWnd = wnd;
    return wnd != NULL;
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
            return max(16 + (IsHorizontal
                ? (m_ContentMargins.cxLeftWidth + m_ContentMargins.cxRightWidth)
                : (m_ContentMargins.cyTopHeight + m_ContentMargins.cyBottomHeight)
            ), 18) + 6;
        }
    }
    else
    {
        return max(
            2 * GetSystemMetrics(SM_CXBORDER) + GetSystemMetrics(SM_CXSMICON),
            2 * GetSystemMetrics(SM_CYBORDER) + GetSystemMetrics(SM_CYSMICON)
        );
    }
}

HRESULT CTrayShowDesktopButton::DoCreate(HWND hwndParent)
{
    DWORD style = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_DEFPUSHBUTTON;
    Create(hwndParent, NULL, NULL, style);

    if (!m_hWnd)
        return E_FAIL;

    bool bIconRetrievalFailed = ExtractIconExW(L"imageres.dll", -IDI_IMAGERES_DESKTOP, NULL, &m_icon, 1) == UINT_MAX;
    if (bIconRetrievalFailed || !m_icon)
        ExtractIconExW(L"shell32.dll", -IDI_SHELL32_DESKTOP, NULL, &m_icon, 1);

    EnsureWindowTheme(TRUE);

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
    HWND taskbarWnd;
    if (GetTaskbar(&taskbarWnd))
        ::SendMessage(taskbarWnd, WM_COMMAND, TRAYCMD_TOGGLE_DESKTOP, 0);
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
        SetWindowTheme(m_hWnd, IsHorizontal ? L"ShowDesktop" : L"VerticalShowDesktop", NULL);

    HWND taskbarWnd;
    if (GetTaskbar(&taskbarWnd))
    {
        m_hFallbackTheme = ::OpenThemeData(taskbarWnd, L"Toolbar");
        GetThemeMargins(m_hFallbackTheme, NULL, TP_BUTTON, 0, TMT_CONTENTMARGINS, NULL, &m_ContentMargins);
    }
    else
        m_hFallbackTheme = NULL;

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

BOOL CTrayShowDesktopButton::PtInButton(LPPOINT ppt)
{
    if (!ppt || !IsWindow())
        return FALSE;

    RECT rc;
    GetWindowRect(&rc);
    INT cxEdge = ::GetSystemMetrics(SM_CXEDGE), cyEdge = ::GetSystemMetrics(SM_CYEDGE);
    ::InflateRect(&rc, max(cxEdge, 1), max(cyEdge, 1));
    return IsHorizontal
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

    HWND taskbarWnd;
    if (GetTaskbar(&taskbarWnd))
        ::PostMessage(taskbarWnd, WM_NCPAINT, 0, 0);
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

        HWND taskbarWnd;
        if (GetTaskbar(&taskbarWnd))
            ::PostMessage(taskbarWnd, WM_NCPAINT, 0, 0);
    }

    return 0;
}

LRESULT CTrayShowDesktopButton::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_hTheme)
    {
        CloseThemeData(m_hTheme);
        m_hTheme = NULL;
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

            if (IsHorizontal)
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

        int iconSize = 16;
        // Used for icon centering further down
        int iconHalfSize = iconSize / 2;

        // Determine X-position of icon's top-left corner
        int iconX = rc.left;
        int width = (rc.right - iconX);
        iconX += (width / 2);
        iconX -= iconHalfSize;

        // Determine Y-position of icon's top-left corner
        int iconY = rc.top;
        int height = (rc.bottom - iconY);
        iconY += (height / 2);
        iconY -= iconHalfSize;

        // Ok now actually draw the icon itself
        if (m_icon)
        {
            DrawIconEx(hdc
                , iconX
                , iconY
                , m_icon
                , iconSize
                , iconSize
                , 0
                , hbrBackground
                , DI_NORMAL
            );
        }
        else // Fallback for if icon isn't available or something idk lol
        {
            RECT rcIconFallback = { iconX, iconY, iconX + iconSize, iconY + iconSize };
            HBRUSH hbrFallback;

            // Border - #808080
            hbrFallback = CreateSolidBrush(RGB(0x80, 0x80, 0x80));
            ::FillRect(hdc, &rcIconFallback, hbrFallback);
            ::DeleteObject(hbrFallback);

            // Background - #3A6EA5
            ::InflateRect(&rcIconFallback, -1, -1);
            hbrFallback = CreateSolidBrush(RGB(0x3A, 0x6E, 0xA5));
            ::FillRect(hdc, &rcIconFallback, hbrFallback);
            ::DeleteObject(hbrFallback);

            // Line to represent taskbar - #D4D0C8
            rcIconFallback.top = rcIconFallback.bottom - 2;
            hbrFallback = CreateSolidBrush(RGB(0xD4, 0xD0, 0xC8));
            ::FillRect(hdc, &rcIconFallback, hbrFallback);
            ::DeleteObject(hbrFallback);
        }
    }
}
