/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 * Copyright 2018 Ged Murphy <gedmurphy@reactos.org>
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

#include "precomp.h"
#include <commoncontrols.h>

static const WCHAR szTrayNotifyWndClass[] = L"TrayNotifyWnd";

#define TRAY_NOTIFY_WND_SPACING_X   1
#define TRAY_NOTIFY_WND_SPACING_Y   1
#define CLOCK_TEXT_HACK             4

/*
 * TrayNotifyWnd
 */

class CTrayNotifyWnd :
    public CComCoClass<CTrayNotifyWnd>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CTrayNotifyWnd, CWindow, CControlWinTraits >,
    public IOleWindow
{
    CComPtr<IUnknown> m_clock;
    CTrayShowDesktopButton m_ShowDesktopButton;
    CComPtr<IUnknown> m_pager;

    HWND m_hwndClock;
    HWND m_hwndShowDesktop;
    HWND m_hwndPager;

    HTHEME TrayTheme;
    SIZE trayClockMinSize;
    SIZE trayShowDesktopSize;
    SIZE trayNotifySize;
    MARGINS ContentMargin;
    BOOL IsHorizontal;

public:
    CTrayNotifyWnd() :
        m_hwndClock(NULL),
        m_hwndPager(NULL),
        TrayTheme(NULL),
        IsHorizontal(FALSE)
    {
        ZeroMemory(&trayClockMinSize, sizeof(trayClockMinSize));
        ZeroMemory(&trayShowDesktopSize, sizeof(trayShowDesktopSize));
        ZeroMemory(&trayNotifySize, sizeof(trayNotifySize));
        ZeroMemory(&ContentMargin, sizeof(ContentMargin));
    }
    ~CTrayNotifyWnd() { }

    LRESULT OnThemeChanged()
    {
        if (TrayTheme)
            CloseThemeData(TrayTheme);

        if (IsThemeActive())
            TrayTheme = OpenThemeData(m_hWnd, L"TrayNotify");
        else
            TrayTheme = NULL;

        if (TrayTheme)
        {
            SetWindowExStyle(m_hWnd, WS_EX_STATICEDGE, 0);

            GetThemeMargins(TrayTheme,
                NULL,
                TNP_BACKGROUND,
                0,
                TMT_CONTENTMARGINS,
                NULL,
                &ContentMargin);
        }
        else
        {
            SetWindowExStyle(m_hWnd, WS_EX_STATICEDGE, WS_EX_STATICEDGE);

            ContentMargin.cxLeftWidth = 2;
            ContentMargin.cxRightWidth = 2;
            ContentMargin.cyTopHeight = 2;
            ContentMargin.cyBottomHeight = 2;
        }

        return TRUE;
    }

    LRESULT OnThemeChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return OnThemeChanged();
    }

    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HRESULT hr;

        hr = CTrayClockWnd_CreateInstance(m_hWnd, IID_PPV_ARG(IUnknown, &m_clock));
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        hr = IUnknown_GetWindow(m_clock, &m_hwndClock);
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        hr = CSysPagerWnd_CreateInstance(m_hWnd, IID_PPV_ARG(IUnknown, &m_pager));
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        hr = IUnknown_GetWindow(m_pager, &m_hwndPager);
        if (FAILED_UNEXPECTEDLY(hr))
            return FALSE;

        /* Create the 'Show Desktop' button */
        m_ShowDesktopButton.DoCreate(m_hWnd);
        m_hwndShowDesktop = m_ShowDesktopButton.m_hWnd;

        return TRUE;
    }

    BOOL GetMinimumSize(IN OUT PSIZE pSize)
    {
        SIZE clockSize = { 0, 0 };
        SIZE traySize = { 0, 0 };
        SIZE showDesktopSize = { 0, 0 };

        if (!g_TaskbarSettings.sr.HideClock)
        {
            if (IsHorizontal)
            {
                clockSize.cy = pSize->cy;
                if (clockSize.cy <= 0)
                    goto NoClock;
            }
            else
            {
                clockSize.cx = pSize->cx;
                if (clockSize.cx <= 0)
                    goto NoClock;
            }

            ::SendMessage(m_hwndClock, TNWM_GETMINIMUMSIZE, (WPARAM) IsHorizontal, (LPARAM) &clockSize);

            trayClockMinSize = clockSize;
        }
        else
        NoClock:
        trayClockMinSize = clockSize;

        if (IsHorizontal)
        {
            traySize.cy = pSize->cy - 2 * TRAY_NOTIFY_WND_SPACING_Y;
        }
        else
        {
            traySize.cx = pSize->cx - 2 * TRAY_NOTIFY_WND_SPACING_X;
        }

        ::SendMessage(m_hwndPager, TNWM_GETMINIMUMSIZE, (WPARAM) IsHorizontal, (LPARAM) &traySize);

        trayNotifySize = traySize;

        INT showDesktopButtonExtent = 0;
        if (g_TaskbarSettings.bShowDesktopButton)
        {
            showDesktopButtonExtent = m_ShowDesktopButton.WidthOrHeight();
            if (IsHorizontal)
            {
                showDesktopSize.cx = showDesktopButtonExtent;
                showDesktopSize.cy = pSize->cy;
            }
            else
            {
                showDesktopSize.cx = pSize->cx;
                showDesktopSize.cy = showDesktopButtonExtent;
            }
        }
        trayShowDesktopSize = showDesktopSize;

        if (IsHorizontal)
        {
            pSize->cx = 2 * TRAY_NOTIFY_WND_SPACING_X;

            if (!g_TaskbarSettings.sr.HideClock)
                pSize->cx += TRAY_NOTIFY_WND_SPACING_X + trayClockMinSize.cx;

            if (g_TaskbarSettings.bShowDesktopButton)
                pSize->cx += showDesktopButtonExtent;

            pSize->cx += traySize.cx;
            pSize->cx += ContentMargin.cxLeftWidth + ContentMargin.cxRightWidth;
        }
        else
        {
            pSize->cy = 2 * TRAY_NOTIFY_WND_SPACING_Y;

            if (!g_TaskbarSettings.sr.HideClock)
                pSize->cy += TRAY_NOTIFY_WND_SPACING_Y + trayClockMinSize.cy;

            if (g_TaskbarSettings.bShowDesktopButton)
                pSize->cy += showDesktopButtonExtent;

            pSize->cy += traySize.cy;
            pSize->cy += ContentMargin.cyTopHeight + ContentMargin.cyBottomHeight;
        }

        return TRUE;
    }

    VOID Size(IN OUT SIZE *pszClient)
    {
        RECT rcClient = {0, 0, pszClient->cx, pszClient->cy};
        AlignControls(&rcClient);
        pszClient->cx = rcClient.right - rcClient.left;
        pszClient->cy = rcClient.bottom - rcClient.top;
    }

    VOID AlignControls(IN CONST PRECT prcClient OPTIONAL)
    {
        RECT rcClient;
        if (prcClient != NULL)
            rcClient = *prcClient;
        else
            GetClientRect(&rcClient);

        rcClient.left += ContentMargin.cxLeftWidth;
        rcClient.top += ContentMargin.cyTopHeight;
        rcClient.right -= ContentMargin.cxRightWidth;
        rcClient.bottom -= ContentMargin.cyBottomHeight;

        CONST UINT swpFlags = SWP_DRAWFRAME | SWP_NOCOPYBITS | SWP_NOZORDER;

        if (g_TaskbarSettings.bShowDesktopButton)
        {
            POINT ptShowDesktop =
            {
                rcClient.left,
                rcClient.top
            };
            SIZE showDesktopSize =
            {
                rcClient.right - rcClient.left,
                rcClient.bottom - rcClient.top
            };

            INT cxyShowDesktop = m_ShowDesktopButton.WidthOrHeight();
            if (IsHorizontal)
            {
                if (!TrayTheme)
                {
                    ptShowDesktop.y -= ContentMargin.cyTopHeight;
                    showDesktopSize.cy += ContentMargin.cyTopHeight + ContentMargin.cyBottomHeight;
                }

                rcClient.right -= (cxyShowDesktop - ContentMargin.cxRightWidth);

                ptShowDesktop.x = rcClient.right;
                showDesktopSize.cx = cxyShowDesktop;

                // HACK: Clock has layout problems - remove this once addressed.
                rcClient.right -= CLOCK_TEXT_HACK;
            }
            else
            {
                if (!TrayTheme)
                {
                    ptShowDesktop.x -= ContentMargin.cxLeftWidth;
                    showDesktopSize.cx += ContentMargin.cxLeftWidth + ContentMargin.cxRightWidth;
                }

                rcClient.bottom -= (cxyShowDesktop - ContentMargin.cyBottomHeight);

                ptShowDesktop.y = rcClient.bottom;
                showDesktopSize.cy = cxyShowDesktop;

                // HACK: Clock has layout problems - remove this once addressed.
                rcClient.bottom -= CLOCK_TEXT_HACK;
            }

            /* Resize and reposition the button */
            ::SetWindowPos(m_hwndShowDesktop,
                NULL,
                ptShowDesktop.x,
                ptShowDesktop.y,
                showDesktopSize.cx,
                showDesktopSize.cy,
                swpFlags);
        }

        if (!g_TaskbarSettings.sr.HideClock)
        {
            POINT ptClock = { rcClient.left, rcClient.top };
            SIZE clockSize = { rcClient.right - rcClient.left, rcClient.bottom - rcClient.top };

            if (IsHorizontal)
            {
                rcClient.right -= trayClockMinSize.cx;

                ptClock.x = rcClient.right;
                clockSize.cx = trayClockMinSize.cx;
            }
            else
            {
                rcClient.bottom -= trayClockMinSize.cy;

                ptClock.y = rcClient.bottom;
                clockSize.cy = trayClockMinSize.cy;
            }

            ::SetWindowPos(m_hwndClock,
                NULL,
                ptClock.x,
                ptClock.y,
                clockSize.cx,
                clockSize.cy,
                swpFlags);
        }

        POINT ptPager;
        if (IsHorizontal)
        {
            ptPager.x = ContentMargin.cxLeftWidth;
            ptPager.y = ((rcClient.bottom - rcClient.top) - trayNotifySize.cy) / 2;
            if (g_TaskbarSettings.UseCompactTrayIcons())
                ptPager.y += ContentMargin.cyTopHeight;
        }
        else
        {
            ptPager.x = ((rcClient.right - rcClient.left) - trayNotifySize.cx) / 2;
            if (g_TaskbarSettings.UseCompactTrayIcons())
                ptPager.x += ContentMargin.cxLeftWidth;
            ptPager.y = ContentMargin.cyTopHeight;
        }

        ::SetWindowPos(m_hwndPager,
            NULL,
            ptPager.x,
            ptPager.y,
            trayNotifySize.cx,
            trayNotifySize.cy,
            swpFlags);

        if (prcClient != NULL)
        {
            prcClient->left = rcClient.left - ContentMargin.cxLeftWidth;
            prcClient->top = rcClient.top - ContentMargin.cyTopHeight;
            prcClient->right = rcClient.right + ContentMargin.cxRightWidth;
            prcClient->bottom = rcClient.bottom + ContentMargin.cyBottomHeight;
        }
    }

    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HDC hdc = (HDC) wParam;

        if (!TrayTheme)
        {
            bHandled = FALSE;
            return 0;
        }

        RECT rect;
        GetClientRect(&rect);
        if (IsThemeBackgroundPartiallyTransparent(TrayTheme, TNP_BACKGROUND, 0))
            DrawThemeParentBackground(m_hWnd, hdc, &rect);

        DrawThemeBackground(TrayTheme, hdc, TNP_BACKGROUND, 0, &rect, 0);

        return TRUE;
    }

    LRESULT OnGetMinimumSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        BOOL Horizontal = (BOOL) wParam;

        if (Horizontal != IsHorizontal)
            IsHorizontal = Horizontal;

        SetWindowTheme(m_hWnd,
                       IsHorizontal ? L"TrayNotifyHoriz" : L"TrayNotifyVert",
                       NULL);
        m_ShowDesktopButton.m_bHorizontal = Horizontal;

        return (LRESULT)GetMinimumSize((PSIZE)lParam);
    }

    LRESULT OnGetShowDesktopButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (wParam == NULL)
            return 0;

        CTrayShowDesktopButton** ptr = (CTrayShowDesktopButton**)wParam;
        if (!m_ShowDesktopButton)
        {
            *ptr = NULL;
            return 0;
        }

        *ptr = &m_ShowDesktopButton;
        bHandled = TRUE;
        return 0;
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SIZE clientSize;

        clientSize.cx = LOWORD(lParam);
        clientSize.cy = HIWORD(lParam);

        Size(&clientSize);

        return TRUE;
    }

    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);

        if (m_ShowDesktopButton && m_ShowDesktopButton.PtInButton(&pt))
            return HTCLIENT;

        return HTTRANSPARENT;
    }

    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        POINT pt;
        ::GetCursorPos(&pt);

        if (m_ShowDesktopButton && m_ShowDesktopButton.PtInButton(&pt))
            m_ShowDesktopButton.StartHovering();

        return TRUE;
    }

    LRESULT OnCtxMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        bHandled = TRUE;

        if (reinterpret_cast<HWND>(wParam) == m_hwndClock)
            return GetParent().SendMessage(uMsg, wParam, lParam);
        else
            return 0;
    }

    LRESULT OnClockMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return SendMessageW(m_hwndClock, uMsg, wParam, lParam);
    }

    LRESULT OnTaskbarSettingsChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        TaskbarSettings* newSettings = (TaskbarSettings*)lParam;

        /* Toggle show desktop button */
        if (newSettings->bShowDesktopButton != g_TaskbarSettings.bShowDesktopButton)
        {
            g_TaskbarSettings.bShowDesktopButton = newSettings->bShowDesktopButton;
            ::ShowWindow(m_hwndShowDesktop, g_TaskbarSettings.bShowDesktopButton ? SW_SHOW : SW_HIDE);

            /* Ask the parent to resize */
            NMHDR nmh = {m_hWnd, 0, NTNWM_REALIGN};
            SendMessage(WM_NOTIFY, 0, (LPARAM) &nmh);
        }

        return OnClockMessage(uMsg, wParam, lParam, bHandled);
    }

    LRESULT OnPagerMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return SendMessageW(m_hwndPager, uMsg, wParam, lParam);
    }

    LRESULT OnRealign(INT uCode, LPNMHDR hdr, BOOL& bHandled)
    {
        hdr->hwndFrom = m_hWnd;
        return GetParent().SendMessage(WM_NOTIFY, 0, (LPARAM)hdr);
    }

    // *** IOleWindow methods ***

    STDMETHODIMP
    GetWindow(HWND* phwnd) override
    {
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;
        return S_OK;
    }

    STDMETHODIMP
    ContextSensitiveHelp(BOOL fEnterMode) override
    {
        return E_NOTIMPL;
    }

    HRESULT Initialize(IN HWND hwndParent)
    {
        const DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        Create(hwndParent, 0, NULL, dwStyle, WS_EX_STATICEDGE);
        return m_hWnd ? S_OK : E_FAIL;
    }

    DECLARE_NOT_AGGREGATABLE(CTrayNotifyWnd)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CTrayNotifyWnd)
        COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleWindow)
    END_COM_MAP()

    DECLARE_WND_CLASS_EX(szTrayNotifyWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTrayNotifyWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_NCMOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenu)
        MESSAGE_HANDLER(WM_NCLBUTTONDBLCLK, OnClockMessage)
        MESSAGE_HANDLER(WM_SETFONT, OnClockMessage)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnPagerMessage)
        MESSAGE_HANDLER(WM_COPYDATA, OnPagerMessage)
        MESSAGE_HANDLER(TWM_SETTINGSCHANGED, OnTaskbarSettingsChanged)
        NOTIFY_CODE_HANDLER(NTNWM_REALIGN, OnRealign)
        MESSAGE_HANDLER(TNWM_GETMINIMUMSIZE, OnGetMinimumSize)
        MESSAGE_HANDLER(TNWM_GETSHOWDESKTOPBUTTON, OnGetShowDesktopButton)
    END_MSG_MAP()
};

HRESULT CTrayNotifyWnd_CreateInstance(HWND hwndParent, REFIID riid, void **ppv)
{
    return ShellObjectCreatorInit<CTrayNotifyWnd>(hwndParent, riid, ppv);
}
