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

/*
 * TrayNotifyWnd
 */

static const WCHAR szTrayNotifyWndClass [] = TEXT("TrayNotifyWnd");

#define TRAY_NOTIFY_WND_SPACING_X   1
#define TRAY_NOTIFY_WND_SPACING_Y   1

class CTrayNotifyWnd :
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public CWindowImpl < CTrayNotifyWnd, CWindow, CControlWinTraits >
{
    HWND hWndNotify;

    CSysPagerWnd * m_pager;
    CTrayClockWnd * m_clock;

    CComPtr<ITrayWindow> TrayWindow;

    HTHEME TrayTheme;
    SIZE szTrayClockMin;
    SIZE szTrayNotify;
    MARGINS ContentMargin;
    BOOL IsHorizontal;

public:
    CTrayNotifyWnd() :
        hWndNotify(NULL),
        m_pager(NULL),
        m_clock(NULL),
        TrayTheme(NULL),
        IsHorizontal(FALSE)
    {
        ZeroMemory(&szTrayClockMin, sizeof(szTrayClockMin));
        ZeroMemory(&szTrayNotify, sizeof(szTrayNotify));
        ZeroMemory(&ContentMargin, sizeof(ContentMargin));
    }
    virtual ~CTrayNotifyWnd() { }

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
        m_clock = new CTrayClockWnd();
        m_clock->_Init(m_hWnd, !g_TaskbarSettings.sr.HideClock);

        m_pager = new CSysPagerWnd();
        m_pager->_Init(m_hWnd, !g_TaskbarSettings.sr.HideClock);

        return TRUE;
    }

    BOOL GetMinimumSize(IN OUT PSIZE pSize)
    {
        SIZE szClock = { 0, 0 };
        SIZE szTray = { 0, 0 };

        if (!g_TaskbarSettings.sr.HideClock)
        {
            if (IsHorizontal)
            {
                szClock.cy = pSize->cy - 2 * TRAY_NOTIFY_WND_SPACING_Y;
                if (szClock.cy <= 0)
                    goto NoClock;
            }
            else
            {
                szClock.cx = pSize->cx - 2 * TRAY_NOTIFY_WND_SPACING_X;
                if (szClock.cx <= 0)
                    goto NoClock;
            }

            m_clock->SendMessage(TCWM_GETMINIMUMSIZE, (WPARAM) IsHorizontal, (LPARAM) &szClock);

            szTrayClockMin = szClock;
        }
        else
        NoClock:
        szTrayClockMin = szClock;

        if (IsHorizontal)
        {
            szTray.cy = pSize->cy - 2 * TRAY_NOTIFY_WND_SPACING_Y;
        }
        else
        {
            szTray.cx = pSize->cx - 2 * TRAY_NOTIFY_WND_SPACING_X;
        }

        m_pager->GetSize(IsHorizontal, &szTray);

        szTrayNotify = szTray;

        if (IsHorizontal)
        {
            pSize->cx = 2 * TRAY_NOTIFY_WND_SPACING_X;

            if (!g_TaskbarSettings.sr.HideClock)
                pSize->cx += TRAY_NOTIFY_WND_SPACING_X + szTrayClockMin.cx;

            pSize->cx += szTray.cx;
        }
        else
        {
            pSize->cy = 2 * TRAY_NOTIFY_WND_SPACING_Y;

            if (!g_TaskbarSettings.sr.HideClock)
                pSize->cy += TRAY_NOTIFY_WND_SPACING_Y + szTrayClockMin.cy;

            pSize->cy += szTray.cy;
        }

        pSize->cy += ContentMargin.cyTopHeight + ContentMargin.cyBottomHeight;
        pSize->cx += ContentMargin.cxLeftWidth + ContentMargin.cxRightWidth;

        return TRUE;
    }

    VOID Size(IN const SIZE *pszClient)
    {
        if (!g_TaskbarSettings.sr.HideClock)
        {
            POINT ptClock;
            SIZE szClock;

            if (IsHorizontal)
            {
                ptClock.x = pszClient->cx - szTrayClockMin.cx - ContentMargin.cxRightWidth;
                ptClock.y = ContentMargin.cyTopHeight;
                szClock.cx = szTrayClockMin.cx;
                szClock.cy = pszClient->cy - ContentMargin.cyTopHeight - ContentMargin.cyBottomHeight;
            }
            else
            {
                ptClock.x = ContentMargin.cxLeftWidth;
                ptClock.y = pszClient->cy - szTrayClockMin.cy;
                szClock.cx = pszClient->cx - ContentMargin.cxLeftWidth - ContentMargin.cxRightWidth;
                szClock.cy = szTrayClockMin.cy;
            }

            m_clock->SetWindowPos(
                NULL,
                ptClock.x,
                ptClock.y,
                szClock.cx,
                szClock.cy,
                SWP_NOZORDER);

            POINT ptPager;

            if (IsHorizontal)
            {
                ptPager.x = ContentMargin.cxLeftWidth;
                ptPager.y = (pszClient->cy - szTrayNotify.cy)/2;
            }
            else
            {
                ptPager.x = (pszClient->cx - szTrayNotify.cx)/2;
                ptPager.y = ContentMargin.cyTopHeight;
            }

            m_pager->SetWindowPos(
                NULL,
                ptPager.x,
                ptPager.y,
                szTrayNotify.cx,
                szTrayNotify.cy,
                SWP_NOZORDER);
        }
    }

    LRESULT DrawBackground(HDC hdc)
    {
        HRESULT res;
        RECT rect;

        GetClientRect(&rect);

        if (TrayTheme)
        {
            if (IsThemeBackgroundPartiallyTransparent(TrayTheme, TNP_BACKGROUND, 0))
            {
                DrawThemeParentBackground(m_hWnd, hdc, &rect);
            }

            res = DrawThemeBackground(TrayTheme, hdc, TNP_BACKGROUND, 0, &rect, 0);
        }

        return res;
    }

    LRESULT OnEraseBackground(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        HDC hdc = (HDC) wParam;

        if (!TrayTheme)
        {
            bHandled = FALSE;
            return 0;
        }

        return DrawBackground(hdc);
    }

    BOOL NotifyIconCmd(WPARAM wParam, LPARAM lParam)
    {
        if (m_pager)
        {
            return m_pager->NotifyIconCmd(wParam, lParam);
        }

        return TRUE;
    }

    BOOL GetClockRect(OUT PRECT rcClock)
    {
        if (!m_clock->IsWindowVisible())
            return FALSE;

        return m_clock->GetWindowRect(rcClock);
    }

    LRESULT OnGetMinimumSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        BOOL Horizontal = (BOOL) wParam;

        if (Horizontal != IsHorizontal)
        {
            IsHorizontal = Horizontal;
            if (IsHorizontal)
                SetWindowTheme(m_hWnd, L"TrayNotifyHoriz", NULL);
            else
                SetWindowTheme(m_hWnd, L"TrayNotifyVert", NULL);
        }

        return (LRESULT) GetMinimumSize((PSIZE) lParam);
    }

    LRESULT OnUpdateTime(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (m_clock != NULL)
        {
            /* Forward the message to the tray clock window procedure */
            return m_clock->OnUpdateTime(uMsg, wParam, lParam, bHandled);
        }
        return FALSE;
    }

    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        SIZE szClient;

        szClient.cx = LOWORD(lParam);
        szClient.cy = HIWORD(lParam);

        Size(&szClient);

        return TRUE;
    }

    LRESULT OnNcHitTest(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        return HTTRANSPARENT;
    }

    LRESULT OnShowClock(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        BOOL PrevHidden = g_TaskbarSettings.sr.HideClock;
        g_TaskbarSettings.sr.HideClock = (wParam == 0);

        if (m_clock != NULL && PrevHidden != g_TaskbarSettings.sr.HideClock)
        {
            m_clock->ShowWindow(g_TaskbarSettings.sr.HideClock ? SW_HIDE : SW_SHOW);
        }

        return (LRESULT) (!PrevHidden);
    }

    LRESULT OnTaskbarSettingsChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        TaskbarSettings* newSettings = (TaskbarSettings*)lParam;
        if (newSettings->bShowSeconds != g_TaskbarSettings.bShowSeconds)
        {
            g_TaskbarSettings.bShowSeconds = newSettings->bShowSeconds;
            /* TODO: Toggle showing seconds */
        }

        if (newSettings->sr.HideClock != g_TaskbarSettings.sr.HideClock)
        {
            g_TaskbarSettings.sr.HideClock = newSettings->sr.HideClock;
            /* TODO: Toggle hiding the clock */
        }

        return 0;
    }

    LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        const NMHDR *nmh = (const NMHDR *) lParam;

        if (nmh->hwndFrom == m_clock->m_hWnd)
        {
            /* Pass down notifications */
            return m_clock->SendMessage(WM_NOTIFY, wParam, lParam);
        }

        return FALSE;
    }

    LRESULT OnSetFont(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (m_clock != NULL)
        {
            m_clock->SendMessageW(WM_SETFONT, wParam, lParam);
        }

        bHandled = FALSE;
        return FALSE;
    }

    LRESULT OnCtxMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        bHandled = TRUE;
        return 0;
    }

    LRESULT OnSettingChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        if (wParam == SPI_SETNONCLIENTMETRICS)
        {
            m_pager->ResizeImagelist();
        }
        return 0;
    }

    DECLARE_WND_CLASS_EX(szTrayNotifyWndClass, CS_DBLCLKS, COLOR_3DFACE)

    BEGIN_MSG_MAP(CTrayNotifyWnd)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_THEMECHANGED, OnThemeChanged)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
        MESSAGE_HANDLER(WM_SETTINGCHANGE, OnSettingChanged)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_NCHITTEST, OnNcHitTest)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_SETFONT, OnSetFont)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnCtxMenu) // FIXME: This handler is not necessary in Windows
        MESSAGE_HANDLER(TNWM_GETMINIMUMSIZE, OnGetMinimumSize)
        MESSAGE_HANDLER(TNWM_UPDATETIME, OnUpdateTime)
        MESSAGE_HANDLER(TNWM_SHOWCLOCK, OnShowClock)
        MESSAGE_HANDLER(TWM_SETTINGSCHANGED, OnTaskbarSettingsChanged)
    END_MSG_MAP()

    HWND _Init(IN OUT ITrayWindow *TrayWindow)
    {
        HWND hWndTrayWindow;

        hWndTrayWindow = TrayWindow->GetHWND();
        if (hWndTrayWindow == NULL)
            return NULL;

        this->TrayWindow = TrayWindow;
        this->hWndNotify = hWndTrayWindow;

        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        return Create(hWndTrayWindow, 0, NULL, dwStyle, WS_EX_STATICEDGE);
    }
};

HWND CreateTrayNotifyWnd(IN OUT ITrayWindow *Tray, CTrayNotifyWnd** ppinstance)
{
    CTrayNotifyWnd * pTrayNotify = new CTrayNotifyWnd();
    // TODO: Destroy after the window is destroyed
    *ppinstance = pTrayNotify;

    return pTrayNotify->_Init(Tray);
}

BOOL
TrayNotify_NotifyIconCmd(CTrayNotifyWnd* pTrayNotify, WPARAM wParam, LPARAM lParam)
{
    return pTrayNotify->NotifyIconCmd(wParam, lParam);
}

BOOL
TrayNotify_GetClockRect(CTrayNotifyWnd* pTrayNotify, OUT PRECT rcClock)
{
    return pTrayNotify->GetClockRect(rcClock);
}
