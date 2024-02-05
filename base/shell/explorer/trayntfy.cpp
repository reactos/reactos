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
public:
    CTrayShowDesktopButton m_ShowDesktopButton;
private:
    CComPtr<IUnknown> m_pager;

    HWND m_hwndClock;
    HWND m_hwndShowDesktop;
    HWND m_hwndPager;

    HTHEME TrayTheme;
    SIZE szTrayClockMin;
    SIZE szTrayShowDesktop;
    SIZE szTrayNotify;
    MARGINS ContentMargin;
    BOOL IsHorizontal;

public:
    CTrayNotifyWnd() :
        m_ShowDesktopButton(),
        m_hwndClock(NULL),
        m_hwndPager(NULL),
        TrayTheme(NULL),
        IsHorizontal(FALSE)
    {
        ZeroMemory(&szTrayClockMin, sizeof(szTrayClockMin));
        ZeroMemory(&szTrayShowDesktop, sizeof(szTrayShowDesktop));
        ZeroMemory(&szTrayNotify, sizeof(szTrayNotify));
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
        SIZE szClock = { 0, 0 };
        SIZE szTray = { 0, 0 };
        SIZE szShowDesktop = { 0, 0 };

        if (!g_TaskbarSettings.sr.HideClock)
        {
            if (IsHorizontal)
            {
                szClock.cy = pSize->cy;
                if (szClock.cy <= 0)
                    goto NoClock;
            }
            else
            {
                szClock.cx = pSize->cx;
                if (szClock.cx <= 0)
                    goto NoClock;
            }

            ::SendMessage(m_hwndClock, TNWM_GETMINIMUMSIZE, (WPARAM) IsHorizontal, (LPARAM) &szClock);

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

        ::SendMessage(m_hwndPager, TNWM_GETMINIMUMSIZE, (WPARAM) IsHorizontal, (LPARAM) &szTray);

        szTrayNotify = szTray;

        INT showDesktopButtonExtent = 0;
        if (g_TaskbarSettings.bShowDesktopButton)
        {
            showDesktopButtonExtent = m_ShowDesktopButton.WidthOrHeight();
            if (IsHorizontal)
            {
                szShowDesktop.cx = showDesktopButtonExtent;
                szShowDesktop.cy = pSize->cy;
            }
            else
            {
                szShowDesktop.cx = pSize->cx;
                szShowDesktop.cy = showDesktopButtonExtent;
            }
        }
        szTrayShowDesktop = szShowDesktop;

        if (IsHorizontal)
        {
            pSize->cx = 2 * TRAY_NOTIFY_WND_SPACING_X;

            if (!g_TaskbarSettings.sr.HideClock)
                pSize->cx += TRAY_NOTIFY_WND_SPACING_X + szTrayClockMin.cx;
            
            if (g_TaskbarSettings.bShowDesktopButton)
                pSize->cx += showDesktopButtonExtent;

            pSize->cx += szTray.cx;
            pSize->cx += ContentMargin.cxLeftWidth + ContentMargin.cxRightWidth;
        }
        else
        {
            pSize->cy = 2 * TRAY_NOTIFY_WND_SPACING_Y;

            if (!g_TaskbarSettings.sr.HideClock)
                pSize->cy += TRAY_NOTIFY_WND_SPACING_Y + szTrayClockMin.cy;
            
            if (g_TaskbarSettings.bShowDesktopButton)
                pSize->cy += showDesktopButtonExtent;

            pSize->cy += szTray.cy;
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

    VOID AlignControls(IN PRECT prcClient OPTIONAL)
    {
        RECT rcClient;
        if (prcClient != NULL)
        {
            rcClient.left = prcClient->left;
            rcClient.top = prcClient->top;
            rcClient.right = prcClient->right;
            rcClient.bottom = prcClient->bottom;
        }
        else
        {
            if (!GetClientRect(&rcClient))
            {
                ERR("Could not get client rect lastErr=%d\n", GetLastError());
                return;
            }
        }

        rcClient.left += ContentMargin.cxLeftWidth;
        rcClient.top += ContentMargin.cyTopHeight;
        rcClient.right -= ContentMargin.cxRightWidth;
        rcClient.bottom -= ContentMargin.cyBottomHeight;

        UINT swpFlags = SWP_DRAWFRAME | SWP_NOCOPYBITS | SWP_NOZORDER;

        if (g_TaskbarSettings.bShowDesktopButton)
        {
            POINT ptShowDesktop =
            {
                rcClient.left,
                rcClient.top
            };
            SIZE szShowDesktop =
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
                    szShowDesktop.cy += ContentMargin.cyTopHeight + ContentMargin.cyBottomHeight;
                }

                rcClient.right -= (cxyShowDesktop - ContentMargin.cxRightWidth);

                ptShowDesktop.x = rcClient.right;
                szShowDesktop.cx = cxyShowDesktop;

                rcClient.right -= 5;
            }
            else
            {
                if (!TrayTheme)
                {
                    ptShowDesktop.x -= ContentMargin.cxLeftWidth;
                    szShowDesktop.cx += ContentMargin.cxLeftWidth + ContentMargin.cxRightWidth;
                }

                rcClient.bottom -= (cxyShowDesktop - ContentMargin.cyBottomHeight);

                ptShowDesktop.y = rcClient.bottom;
                szShowDesktop.cy = cxyShowDesktop;

                rcClient.bottom -= 5;
            }

            /* Resize and reposition the button */
            ::SetWindowPos(m_hwndShowDesktop,
                NULL,
                ptShowDesktop.x,
                ptShowDesktop.y,
                szShowDesktop.cx,
                szShowDesktop.cy,
                swpFlags
            );
        }

        if (!g_TaskbarSettings.sr.HideClock)
        {
            POINT ptClock =
            {
                rcClient.left,
                rcClient.top
            };
            SIZE szClock =
            {
                rcClient.right - rcClient.left,
                rcClient.bottom - rcClient.top
            };

            if (IsHorizontal)
            {
                rcClient.right -= szTrayClockMin.cx;

                ptClock.x = rcClient.right;
                szClock.cx = szTrayClockMin.cx;
            }
            else
            {
                rcClient.bottom -= szTrayClockMin.cy;

                ptClock.y = rcClient.bottom;
                szClock.cy = szTrayClockMin.cy;
            }

            ::SetWindowPos(m_hwndClock,
                NULL,
                ptClock.x,
                ptClock.y,
                szClock.cx,
                szClock.cy,
                swpFlags);
        }

        POINT ptPager;

        if (IsHorizontal)
        {
            ptPager.x = ContentMargin.cxLeftWidth;
            ptPager.y = ((rcClient.bottom - rcClient.top) - szTrayNotify.cy)/2;
        }
        else
        {
            ptPager.x = ((rcClient.right - rcClient.left) - szTrayNotify.cx)/2;
            ptPager.y = ContentMargin.cyTopHeight;
        }

        ::SetWindowPos(m_hwndPager,
            NULL,
            ptPager.x,
            ptPager.y,
            szTrayNotify.cx,
            szTrayNotify.cy,
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
        m_ShowDesktopButton.IsHorizontal = Horizontal;

        return (LRESULT) GetMinimumSize((PSIZE) lParam);
    }

    LRESULT OnGetShowDesktopButton(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        CTrayShowDesktopButton** ptr = (CTrayShowDesktopButton**)wParam;
        if (!m_ShowDesktopButton)
        {
            *ptr = 0;
            return 0;
        }

        *ptr = &m_ShowDesktopButton;
        bHandled = TRUE;
        return 0;
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

    HRESULT WINAPI GetWindow(HWND* phwnd)
    {
        if (!phwnd)
            return E_INVALIDARG;
        *phwnd = m_hWnd;
        return S_OK;
    }

    HRESULT WINAPI ContextSensitiveHelp(BOOL fEnterMode)
    {
        return E_NOTIMPL;
    }

    HRESULT Initialize(IN HWND hwndParent)
    {
        DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
        Create(hwndParent, 0, NULL, dwStyle, WS_EX_STATICEDGE);
        if (!m_hWnd)
            return E_FAIL;
        return S_OK;
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
