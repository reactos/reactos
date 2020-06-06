/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for SHAppBarMessage
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "shelltest.h"
#include <windowsx.h>
#include <shlwapi.h>
#include <stdio.h>

/* Based on https://github.com/katahiromz/AppBarSample */

//#define VERBOSE

#define IDT_AUTOHIDE 1
#define IDT_AUTOUNHIDE 2

#define ID_ACTION 100

#define APPBAR_CALLBACK (WM_USER + 100)

#define LEFT_DOWN() mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
#define LEFT_UP() mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
#define MOVE(x, y) SetCursorPos((x), (y))

static const TCHAR s_szName[] = TEXT("AppBarSample");
static RECT s_rcWorkArea;
static HWND s_hwnd1 = NULL;
static HWND s_hwnd2 = NULL;

#ifdef VERBOSE
static LPCSTR MessageOfAppBar(DWORD dwMessage)
{
    static char s_buf[32];
    switch (dwMessage)
    {
    case ABM_NEW: return "ABM_NEW";
    case ABM_REMOVE: return "ABM_REMOVE";
    case ABM_QUERYPOS: return "ABM_QUERYPOS";
    case ABM_SETPOS: return "ABM_SETPOS";
    case ABM_GETSTATE: return "ABM_GETSTATE";
    case ABM_GETTASKBARPOS: return "ABM_GETTASKBARPOS";
    case ABM_ACTIVATE: return "ABM_ACTIVATE";
    case ABM_GETAUTOHIDEBAR: return "ABM_GETAUTOHIDEBAR";
    case ABM_SETAUTOHIDEBAR: return "ABM_SETAUTOHIDEBAR";
    case ABM_WINDOWPOSCHANGED: return "ABM_WINDOWPOSCHANGED";
    }
    wsprintfA(s_buf, "%lu", dwMessage);
    return s_buf;
}

static UINT WINAPI
SHAppBarMessageWrap(DWORD dwMessage, PAPPBARDATA pData)
{
    trace("SHAppBarMessage entered (dwMessage=%s, rc=(%ld, %ld, %ld, %ld))\n",
          MessageOfAppBar(dwMessage),
          pData->rc.left, pData->rc.top, pData->rc.right, pData->rc.bottom);
    UINT ret = SHAppBarMessage(dwMessage, pData);
    trace("SHAppBarMessage leaved (dwMessage=%s, rc=(%ld, %ld, %ld, %ld))\n",
          MessageOfAppBar(dwMessage),
          pData->rc.left, pData->rc.top, pData->rc.right, pData->rc.bottom);
    return ret;
}
#define SHAppBarMessage SHAppBarMessageWrap

#undef ARRAYSIZE
#define ARRAYSIZE _countof

void appbar_tprintf(const TCHAR *fmt, ...)
{
    TCHAR szText[512];
    va_list va;
    va_start(va, fmt);
    wvsprintf(szText, fmt, va);
#ifdef UNICODE
    printf("%ls", szText);
#else
    printf("%s", szText);
#endif
    va_end(va);
}

#define MSGDUMP_TPRINTF appbar_tprintf
#include "msgdump.h"

#endif  // def VERBOSE

void SlideWindow(HWND hwnd, LPRECT prc)
{
#define SLIDE_HIDE 400
#define SLIDE_SHOW 150
    RECT rcOld, rcNew = *prc;
    GetWindowRect(hwnd, &rcOld);

    BOOL fShow = (rcNew.bottom - rcNew.top > rcOld.bottom - rcOld.top) ||
                 (rcNew.right - rcNew.left > rcOld.right - rcOld.left);

    INT dx = (rcNew.right - rcOld.right) + (rcNew.left - rcOld.left);
    INT dy = (rcNew.bottom - rcOld.bottom) + (rcNew.top - rcOld.top);

    LONG dt = SLIDE_HIDE;
    if (fShow)
    {
        dt = SLIDE_SHOW;
        rcOld = rcNew;
        OffsetRect(&rcOld, -dx, -dy);
        SetWindowPos(hwnd, NULL, rcOld.left, rcOld.top,
                     rcOld.right - rcOld.left, rcOld.bottom - rcOld.top,
                     SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME);
    }

    HANDLE hThread = GetCurrentThread();
    INT priority = GetThreadPriority(hThread);
    SetThreadPriority(hThread, THREAD_PRIORITY_HIGHEST);

    LONG t, t0 = GetTickCount();
    while ((t = GetTickCount()) < t0 + dt)
    {
        INT x = rcOld.left + dx * (t - t0) / dt;
        INT y = rcOld.top + dy * (t - t0) / dt;
        SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

        UpdateWindow(hwnd);
        UpdateWindow(GetDesktopWindow());
    }

    SetThreadPriority(hThread, priority);
    SetWindowPos(hwnd, NULL, rcNew.left, rcNew.top,
                 rcNew.right - rcNew.left, rcNew.bottom - rcNew.top,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_DRAWFRAME);
#undef SLIDE_HIDE
#undef SLIDE_SHOW
}

class Window
{
public:
    Window(INT cx, INT cy, BOOL fAutoHide = FALSE)
        : m_hwnd(NULL)
        , m_fAutoHide(fAutoHide)
        , m_cxWidth(cx)
        , m_cyHeight(cy)
    {
    }

    virtual ~Window()
    {
    }

    static BOOL DoRegisterClass(HINSTANCE hInstance)
    {
        WNDCLASS wc;
        ZeroMemory(&wc, sizeof(wc));
        wc.lpfnWndProc = Window::WindowProc;
        wc.hInstance = hInstance;
        wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
        wc.lpszClassName = s_szName;
        return !!RegisterClass(&wc);
    }

    static HWND DoCreateMainWnd(HINSTANCE hInstance, LPCTSTR pszText, INT cx, INT cy,
                                DWORD style = WS_POPUP | WS_THICKFRAME | WS_CLIPCHILDREN,
                                DWORD exstyle = WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                                BOOL fAutoHide = FALSE)
    {
        Window *this_ = new Window(cx, cy, fAutoHide);
        HWND hwnd = CreateWindowEx(exstyle, s_szName, pszText, style,
                                   CW_USEDEFAULT, CW_USEDEFAULT, 50, 50,
                                   NULL, NULL, hInstance, this_);
        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);
        return hwnd;
    }

    static INT DoMainLoop()
    {
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return (INT)msg.wParam;
    }

    static Window *GetAppbarData(HWND hwnd)
    {
        return (Window *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    virtual LRESULT CALLBACK
    WindowProcDx(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
#ifdef VERBOSE
        MD_msgdump(hwnd, uMsg, wParam, lParam);
#endif
        switch (uMsg)
        {
        HANDLE_MSG(hwnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hwnd, WM_ACTIVATE, OnActivate);
        HANDLE_MSG(hwnd, WM_WINDOWPOSCHANGED, OnWindowPosChanged);
        HANDLE_MSG(hwnd, WM_SIZE, OnSize);
        HANDLE_MSG(hwnd, WM_MOVE, OnMove);
        HANDLE_MSG(hwnd, WM_NCDESTROY, OnNCDestroy);
        HANDLE_MSG(hwnd, WM_TIMER, OnTimer);
        HANDLE_MSG(hwnd, WM_NCHITTEST, OnNCHitTest);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN, OnLButtonDown);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE, OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONUP, OnLButtonUp);
        HANDLE_MSG(hwnd, WM_RBUTTONDOWN, OnRButtonDown);
        HANDLE_MSG(hwnd, WM_KEYDOWN, OnKey);
        HANDLE_MSG(hwnd, WM_PAINT, OnPaint);

        case APPBAR_CALLBACK:
            OnAppBarCallback(hwnd, uMsg, wParam, lParam);
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    static LRESULT CALLBACK
    WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Window *this_ = GetAppbarData(hwnd);
        if (uMsg == WM_CREATE)
        {
            LPCREATESTRUCT pCS = (LPCREATESTRUCT)lParam;
            this_ = (Window *)pCS->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this_);
        }
        if (this_)
            return this_->WindowProcDx(hwnd, uMsg, wParam, lParam);
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

protected:
    HWND m_hwnd;
    BOOL m_fAutoHide;
    BOOL m_fOnTop;
    BOOL m_fHiding;
    UINT m_uSide;
    LONG m_cxWidth;
    LONG m_cyHeight;
    LONG m_cxSave;
    LONG m_cySave;
    BOOL m_fAppBarRegd;
    BOOL m_fMoving;
    BOOL m_bDragged;
    POINT m_ptDragOn;
    RECT m_rcAppBar;
    RECT m_rcDrag;

    void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
        HANDLE hThread;
        switch (id)
        {
        case ID_ACTION:
            PostMessage(s_hwnd2, WM_COMMAND, ID_ACTION + 1, 0);
            break;
        case ID_ACTION + 1:
            hThread = CreateThread(NULL, 0, ActionThreadFunc, this, 0, NULL);
            if (!hThread)
            {
                skip("failed to create thread\n");
                PostMessage(s_hwnd1, WM_CLOSE, 0, 0);
                PostMessage(s_hwnd2, WM_CLOSE, 0, 0);
                return;
            }
            CloseHandle(hThread);
        }
    }

    void OnPaint(HWND hwnd)
    {
        PAINTSTRUCT ps;

        TCHAR szText[64];
        GetWindowText(hwnd, szText, 64);

        RECT rc;
        GetClientRect(hwnd, &rc);

        if (HDC hdc = BeginPaint(hwnd, &ps))
        {
            DrawText(hdc, szText, -1, &rc, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
            EndPaint(hwnd, &ps);
        }
    }

    void OnRButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        m_fAutoHide = !m_fAutoHide;
        AppBar_SetAutoHide(hwnd, m_fAutoHide);
    }

    void OnKey(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags)
    {
        if (vk == VK_ESCAPE)
            DestroyWindow(hwnd);
    }

    void OnAppBarCallback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        static HWND s_hwndZOrder = NULL;

        switch (wParam)
        {
        case ABN_STATECHANGE:
            break;

        case ABN_FULLSCREENAPP:
            if (lParam)
            {
                s_hwndZOrder = GetWindow(hwnd, GW_HWNDPREV);
                SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
            }
            else
            {
                SetWindowPos(hwnd, m_fOnTop ? HWND_TOPMOST : s_hwndZOrder,
                             0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
                s_hwndZOrder = NULL;
            }
            break;

        case ABN_POSCHANGED:
            {
                APPBARDATA abd = { sizeof(abd) };
                abd.hWnd = hwnd;
                AppBar_PosChanged(&abd);
            }
            break;
        }
    }

    BOOL AppBar_Register(HWND hwnd)
    {
        APPBARDATA abd = { sizeof(abd) };
        abd.hWnd = hwnd;
        abd.uCallbackMessage = APPBAR_CALLBACK;

        m_fAppBarRegd = (BOOL)SHAppBarMessage(ABM_NEW, &abd);
        return m_fAppBarRegd;
    }

    BOOL AppBar_UnRegister(HWND hwnd)
    {
        APPBARDATA abd = { sizeof(abd) };
        abd.hWnd = hwnd;

        m_fAppBarRegd = !SHAppBarMessage(ABM_REMOVE, &abd);
        return !m_fAppBarRegd;
    }

    BOOL AppBar_SetAutoHide(HWND hwnd, BOOL fHide)
    {
        if (fHide)
            return AppBar_AutoHide(hwnd);
        else
            return AppBar_NoAutoHide(hwnd);
    }

    BOOL AppBar_AutoHide(HWND hwnd)
    {
        APPBARDATA abd = { sizeof(abd) };
        abd.hWnd = hwnd;
        abd.uEdge = m_uSide;

        HWND hwndAutoHide = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd);
        if (hwndAutoHide)
            return FALSE;

        abd.lParam = TRUE;
        if (!(BOOL)SHAppBarMessage(ABM_SETAUTOHIDEBAR, &abd))
            return FALSE;

        m_fAutoHide = TRUE;
        m_cxSave = m_cxWidth;
        m_cySave = m_cyHeight;

        RECT rc = m_rcAppBar;
        switch (m_uSide)
        {
        case ABE_TOP:
            rc.bottom = rc.top + 2;
            break;
        case ABE_BOTTOM:
            rc.top = rc.bottom - 2;
            break;
        case ABE_LEFT:
            rc.right = rc.left + 2;
            break;
        case ABE_RIGHT:
            rc.left = rc.right - 2;
            break;
        }

        AppBar_QueryPos(hwnd, &rc);
        abd.rc = rc;
        SHAppBarMessage(ABM_SETPOS, &abd);
        rc = abd.rc;

        m_fHiding = TRUE;
        SlideWindow(hwnd, &rc);

        AppBar_SetAutoHideTimer(hwnd);
        return TRUE;
    }

    BOOL AppBar_NoAutoHide(HWND hwnd)
    {
        APPBARDATA abd = { sizeof(abd) };
        abd.hWnd = hwnd;
        abd.uEdge = m_uSide;
        HWND hwndAutoHide = (HWND)SHAppBarMessage(ABM_GETAUTOHIDEBAR, &abd);
        if (hwndAutoHide != hwnd)
            return FALSE;

        abd.lParam = FALSE;
        if (!(BOOL)SHAppBarMessage(ABM_SETAUTOHIDEBAR, &abd))
            return FALSE;

        m_fAutoHide = FALSE;
        m_cxWidth = m_cxSave;
        m_cyHeight = m_cySave;
        AppBar_SetSide(hwnd, m_uSide);
        return TRUE;
    }

    BOOL AppBar_SetSide(HWND hwnd, UINT uSide)
    {
        RECT rc;
        SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

        BOOL fAutoHide = FALSE;
        if (m_fAutoHide)
        {
            fAutoHide = m_fAutoHide;
            SetWindowRedraw(GetDesktopWindow(), FALSE);
            AppBar_SetAutoHide(hwnd, FALSE);
            m_fHiding = FALSE;
        }

        switch (uSide)
        {
        case ABE_TOP:
            rc.bottom = rc.top + m_cyHeight;
            break;
        case ABE_BOTTOM:
            rc.top = rc.bottom - m_cyHeight;
            break;
        case ABE_LEFT:
            rc.right = rc.left + m_cxWidth;
            break;
        case ABE_RIGHT:
            rc.left = rc.right - m_cxWidth;
            break;
        }

        APPBARDATA abd = { sizeof(abd) };
        abd.hWnd = hwnd;
        AppBar_QuerySetPos(uSide, &rc, &abd, TRUE);

        if (fAutoHide)
        {
            AppBar_SetAutoHide(hwnd, TRUE);
            m_fHiding = TRUE;

            SetWindowRedraw(GetDesktopWindow(), TRUE);
            RedrawWindow(GetDesktopWindow(), NULL, NULL,
                         RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
        }

        return TRUE;
    }

    void AppBar_SetAlwaysOnTop(HWND hwnd, BOOL fOnTop)
    {
        SetWindowPos(hwnd, (fOnTop ? HWND_TOPMOST : HWND_NOTOPMOST),
                     0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        m_fOnTop = fOnTop;
    }

    void AppBar_Hide(HWND hwnd)
    {
        if (!m_fAutoHide)
            return;

        RECT rc = m_rcAppBar;
        switch (m_uSide)
        {
        case ABE_TOP:
            rc.bottom = rc.top + 2;
            break;
        case ABE_BOTTOM:
            rc.top = rc.bottom - 2;
            break;
        case ABE_LEFT:
            rc.right = rc.left + 2;
            break;
        case ABE_RIGHT:
            rc.left = rc.right - 2;
            break;
        }

        m_fHiding = TRUE;
        SlideWindow(hwnd, &rc);
    }

    void AppBar_UnHide(HWND hwnd)
    {
        SlideWindow(hwnd, &m_rcAppBar);
        m_fHiding = FALSE;

        AppBar_SetAutoHideTimer(hwnd);
    }

    void AppBar_SetAutoHideTimer(HWND hwnd)
    {
        if (m_fAutoHide)
        {
            SetTimer(hwnd, IDT_AUTOHIDE, 500, NULL);
        }
    }

    void AppBar_SetAutoUnhideTimer(HWND hwnd)
    {
        if (m_fAutoHide && m_fHiding)
        {
            SetTimer(hwnd, IDT_AUTOUNHIDE, 50, NULL);
        }
    }

    void AppBar_Size(HWND hwnd)
    {
        if (m_fAppBarRegd)
        {
            APPBARDATA abd = { sizeof(abd) };
            abd.hWnd = hwnd;

            RECT rc;
            GetWindowRect(hwnd, &rc);
            AppBar_QuerySetPos(m_uSide, &rc, &abd, TRUE);
        }
    }

    void AppBar_QueryPos(HWND hwnd, LPRECT lprc)
    {
        APPBARDATA abd = { sizeof(abd) };
        abd.hWnd = hwnd;
        abd.rc = *lprc;
        abd.uEdge = m_uSide;

        INT cx = 0, cy = 0;
        if (ABE_LEFT == abd.uEdge || ABE_RIGHT == abd.uEdge)
        {
            cx = abd.rc.right - abd.rc.left;
            abd.rc.top = 0;
            abd.rc.bottom = GetSystemMetrics(SM_CYSCREEN);
        }
        else
        {
            cy = abd.rc.bottom - abd.rc.top;
            abd.rc.left = 0;
            abd.rc.right = GetSystemMetrics(SM_CXSCREEN);
        }

        SHAppBarMessage(ABM_QUERYPOS, &abd);

        switch (abd.uEdge)
        {
        case ABE_LEFT:
            abd.rc.right = abd.rc.left + cx;
            break;
        case ABE_RIGHT:
            abd.rc.left = abd.rc.right - cx;
            break;
        case ABE_TOP:
            abd.rc.bottom = abd.rc.top + cy;
            break;
        case ABE_BOTTOM:
            abd.rc.top = abd.rc.bottom - cy;
            break;
        }

        *lprc = abd.rc;
    }

    void AppBar_QuerySetPos(UINT uEdge, LPRECT lprc, PAPPBARDATA pabd, BOOL fMove)
    {
        pabd->rc = *lprc;
        pabd->uEdge = uEdge;
        m_uSide = uEdge;

        AppBar_QueryPos(pabd->hWnd, &pabd->rc);

        SHAppBarMessage(ABM_SETPOS, pabd);

        if (fMove)
        {
            RECT rc = pabd->rc;
            MoveWindow(pabd->hWnd, rc.left, rc.top,
                       rc.right - rc.left, rc.bottom - rc.top, TRUE);
        }

        if (!m_fAutoHide)
        {
            m_rcAppBar = pabd->rc;
        }
    }

    void AppBar_PosChanged(PAPPBARDATA pabd)
    {
        RECT rc;
        SetRect(&rc, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));

        if (m_fAutoHide)
        {
            m_rcAppBar = rc;
            switch (m_uSide)
            {
            case ABE_TOP:
                m_rcAppBar.bottom = m_rcAppBar.top + m_cySave;
                break;
            case ABE_BOTTOM:
                m_rcAppBar.top = m_rcAppBar.bottom - m_cySave;
                break;
            case ABE_LEFT:
                m_rcAppBar.right = m_rcAppBar.left + m_cxSave;
                break;
            case ABE_RIGHT:
                m_rcAppBar.left = m_rcAppBar.right - m_cxSave;
                break;
            }
        }

        RECT rcWindow;
        GetWindowRect(pabd->hWnd, &rcWindow);
        INT cx = rcWindow.right - rcWindow.left;
        INT cy = rcWindow.bottom - rcWindow.top;
        switch (m_uSide)
        {
        case ABE_TOP:
            rc.bottom = rc.top + cy;
            break;
        case ABE_BOTTOM:
            rc.top = rc.bottom - cy;
            break;
        case ABE_LEFT:
            rc.right = rc.left + cx;
            break;
        case ABE_RIGHT:
            rc.left = rc.right - cx;
            break;
        }
        AppBar_QuerySetPos(m_uSide, &rc, pabd, TRUE);
    }

    BOOL OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
    {
        m_hwnd = hwnd;
        m_fOnTop = TRUE;
        m_uSide = ABE_TOP;

        m_fAppBarRegd = FALSE;
        m_fMoving = FALSE;
        m_cxSave = m_cxWidth;
        m_cySave = m_cyHeight;
        m_bDragged = FALSE;

        AppBar_Register(hwnd);
        AppBar_SetSide(hwnd, ABE_TOP);

        return TRUE;
    }

    void OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
    {
        APPBARDATA abd = { sizeof(abd) };
        abd.hWnd = hwnd;
        SHAppBarMessage(ABM_ACTIVATE, &abd);

        switch (state)
        {
            case WA_ACTIVE:
            case WA_CLICKACTIVE:
                AppBar_UnHide(hwnd);
                KillTimer(hwnd, IDT_AUTOHIDE);
                break;

            case WA_INACTIVE:
                AppBar_Hide(hwnd);
                break;
        }
    }

    void OnWindowPosChanged(HWND hwnd, const LPWINDOWPOS lpwpos)
    {
        APPBARDATA abd = { sizeof(abd) };
        abd.hWnd = hwnd;
        SHAppBarMessage(ABM_WINDOWPOSCHANGED, &abd);

        FORWARD_WM_WINDOWPOSCHANGED(hwnd, lpwpos, DefWindowProc);
    }

    void OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
        RECT rcWindow;

        if (m_fMoving || (m_fAutoHide && m_fHiding))
            return;

        if (!m_fHiding)
        {
            if (!m_fAutoHide)
                AppBar_Size(hwnd);

            GetWindowRect(hwnd, &rcWindow);
            m_rcAppBar = rcWindow;

            if (m_uSide == ABE_TOP || m_uSide == ABE_BOTTOM)
            {
                m_cyHeight = m_cySave = rcWindow.bottom - rcWindow.top;
            }
            else
            {
                m_cxWidth = m_cxSave = rcWindow.right - rcWindow.left;
            }
        }

        InvalidateRect(hwnd, NULL, TRUE);
    }

    void OnMove(HWND hwnd, int x, int y)
    {
        if (m_fMoving || m_fAutoHide)
            return;

        if (!m_fHiding)
            AppBar_Size(hwnd);
    }

    void OnNCDestroy(HWND hwnd)
    {
        AppBar_UnRegister(hwnd);

        m_hwnd = NULL;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
        delete this;
    }

    void OnTimer(HWND hwnd, UINT id)
    {
        POINT pt;
        RECT rc;
        HWND hwndActive;

        switch (id)
        {
        case IDT_AUTOHIDE:
            if (m_fAutoHide && !m_fHiding && !m_fMoving)
            {
                GetCursorPos(&pt);
                GetWindowRect(hwnd, &rc);
                hwndActive = GetForegroundWindow();

                if (!PtInRect(&rc, pt) &&
                    hwndActive != hwnd &&
                    hwndActive != NULL &&
                    GetWindowOwner(hwndActive) != hwnd)
                {
                    KillTimer(hwnd, id);
                    AppBar_Hide(hwnd);
                }
            }
            break;

        case IDT_AUTOUNHIDE:
            KillTimer(hwnd, id);

            if (m_fAutoHide && m_fHiding)
            {
                GetCursorPos(&pt);
                GetWindowRect(hwnd, &rc);
                if (PtInRect(&rc, pt))
                {
                    AppBar_UnHide(hwnd);
                }
            }
            break;
        }
    }

    UINT OnNCHitTest(HWND hwnd, int x, int y)
    {
        AppBar_SetAutoUnhideTimer(hwnd);

        UINT uHitTest = FORWARD_WM_NCHITTEST(hwnd, x, y, DefWindowProc);

        if (m_uSide == ABE_TOP && uHitTest == HTBOTTOM)
            return HTBOTTOM;

        if (m_uSide == ABE_BOTTOM && uHitTest == HTTOP)
            return HTTOP;

        if (m_uSide == ABE_LEFT && uHitTest == HTRIGHT)
            return HTRIGHT;

        if (m_uSide == ABE_RIGHT && uHitTest == HTLEFT)
            return HTLEFT;

        return HTCLIENT;
    }

    void OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
    {
        m_fMoving = TRUE;
        m_bDragged = FALSE;
        SetCapture(hwnd);
        GetCursorPos(&m_ptDragOn);
    }

    void OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
    {
        if (!m_fMoving)
            return;

        POINT pt;
        GetCursorPos(&pt);
        if (labs(pt.x - m_ptDragOn.x) > GetSystemMetrics(SM_CXDRAG) ||
            labs(pt.y - m_ptDragOn.y) > GetSystemMetrics(SM_CYDRAG))
        {
            m_bDragged = TRUE;
        }

        INT cxScreen = GetSystemMetrics(SM_CXSCREEN);
        INT cyScreen = GetSystemMetrics(SM_CYSCREEN);

        DWORD dx, dy;
        UINT ix, iy;
        if (pt.x < cxScreen / 2)
        {
            dx = pt.x;
            ix = ABE_LEFT;
        }
        else
        {
            dx = cxScreen - pt.x;
            ix = ABE_RIGHT;
        }

        if (pt.y < cyScreen / 2)
        {
            dy = pt.y;
            iy = ABE_TOP;
        }
        else
        {
            dy = cyScreen - pt.y;
            iy = ABE_BOTTOM;
        }

        if (cxScreen * dy > cyScreen * dx)
        {
            m_rcDrag.top = 0;
            m_rcDrag.bottom = cyScreen;
            if (ix == ABE_LEFT)
            {
                m_uSide = ABE_LEFT;
                m_rcDrag.left = 0;
                m_rcDrag.right = m_rcDrag.left + m_cxWidth;
            }
            else
            {
                m_uSide = ABE_RIGHT;
                m_rcDrag.right = cxScreen;
                m_rcDrag.left = m_rcDrag.right - m_cxWidth;
            }
        }
        else
        {
            m_rcDrag.left = 0;
            m_rcDrag.right = cxScreen;
            if (iy == ABE_TOP)
            {
                m_uSide = ABE_TOP;
                m_rcDrag.top = 0;
                m_rcDrag.bottom = m_rcDrag.top + m_cyHeight;
            }
            else
            {
                m_uSide = ABE_BOTTOM;
                m_rcDrag.bottom = cyScreen;
                m_rcDrag.top = m_rcDrag.bottom - m_cyHeight;
            }
        }

        AppBar_QueryPos(hwnd, &m_rcDrag);

        if (m_bDragged)
        {
            MoveWindow(hwnd, m_rcDrag.left, m_rcDrag.top,
                       m_rcDrag.right - m_rcDrag.left,
                       m_rcDrag.bottom - m_rcDrag.top,
                       TRUE);
        }
    }

    void OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
    {
        if (!m_fMoving)
            return;

        OnMouseMove(hwnd, x, y, keyFlags);

        m_rcAppBar = m_rcDrag;

        ReleaseCapture();

        if (m_fAutoHide)
        {
            switch (m_uSide)
            {
            case ABE_TOP:
                m_rcDrag.bottom = m_rcDrag.top + 2;
                break;
            case ABE_BOTTOM:
                m_rcDrag.top = m_rcDrag.bottom - 2;
                break;
            case ABE_LEFT:
                m_rcDrag.right = m_rcDrag.left + 2;
                break;
            case ABE_RIGHT:
                m_rcDrag.left = m_rcDrag.right - 2;
                break;
            }
        }

        if (m_bDragged)
        {
            if (m_fAutoHide)
            {
                AppBar_AutoHide(hwnd);
            }
            else
            {
                APPBARDATA abd = { sizeof(abd) };
                abd.hWnd = hwnd;
                AppBar_QuerySetPos(m_uSide, &m_rcDrag, &abd, FALSE);
            }
        }

        m_fMoving = FALSE;
    }

public:
    void DoAction()
    {
#define INTERVAL 250
        POINT pt;
        RECT rc1, rc2;
        DWORD dwTID = GetWindowThreadProcessId(s_hwnd1, NULL);

        GetWindowRect(s_hwnd1, &rc1);
        GetWindowRect(s_hwnd2, &rc2);
        ok_long(rc1.left, s_rcWorkArea.left);
        ok_long(rc1.top, s_rcWorkArea.top);
        ok_long(rc1.right, s_rcWorkArea.right);
        ok_long(rc1.bottom, s_rcWorkArea.top + 80);
        ok_long(rc2.left, s_rcWorkArea.left);
        ok_long(rc2.top, s_rcWorkArea.top + 80);
        ok_long(rc2.right, s_rcWorkArea.right);
        ok_long(rc2.bottom, s_rcWorkArea.top + 110);
        PostMessage(s_hwnd1, WM_CLOSE, 0, 0);
        Sleep(INTERVAL);

        GetWindowRect(s_hwnd2, &rc2);
        ok_long(rc2.left, s_rcWorkArea.left);
        ok_long(rc2.top, s_rcWorkArea.top);
        ok_long(rc2.right, s_rcWorkArea.right);
        ok_long(rc2.bottom, s_rcWorkArea.top + 30);
        AppBar_SetSide(s_hwnd2, ABE_LEFT);
        Sleep(INTERVAL);

        GetWindowRect(s_hwnd2, &rc2);
        ok_long(rc2.left, s_rcWorkArea.left);
        ok_long(rc2.top, s_rcWorkArea.top);
        ok_long(rc2.right, s_rcWorkArea.left + 30);
        AppBar_SetSide(s_hwnd2, ABE_TOP);
        Sleep(INTERVAL);

        GetWindowRect(s_hwnd2, &rc2);
        ok_long(rc2.left, s_rcWorkArea.left);
        ok_long(rc2.top, s_rcWorkArea.top);
        ok_long(rc2.right, s_rcWorkArea.right);
        ok_long(rc2.bottom, s_rcWorkArea.top + 30);
        AppBar_SetSide(s_hwnd2, ABE_RIGHT);
        Sleep(INTERVAL);

        GetWindowRect(s_hwnd2, &rc2);
        ok_long(rc2.left, s_rcWorkArea.right - 30);
        ok_long(rc2.top, s_rcWorkArea.top);
        ok_long(rc2.right, s_rcWorkArea.right);
        Sleep(INTERVAL);

        GetWindowRect(s_hwnd2, &rc2);
        pt.x = (rc2.left + rc2.right) / 2;
        pt.y = (rc2.top + rc2.bottom) / 2;
        MOVE(pt.x, pt.y);
        LEFT_DOWN();
        MOVE(pt.x + 64, pt.y + 64);
        Sleep(INTERVAL);

        pt.x = s_rcWorkArea.left + 80;
        pt.y = (s_rcWorkArea.top + s_rcWorkArea.bottom) / 2;
        MOVE(pt.x, pt.y);
        LEFT_UP();
        Sleep(INTERVAL);

        GetWindowRect(s_hwnd2, &rc2);
        ok_long(rc2.left, s_rcWorkArea.left);
        ok_long(rc2.top, s_rcWorkArea.top);
        ok_long(rc2.right, s_rcWorkArea.left + 30);
        Sleep(INTERVAL);

        GetWindowRect(s_hwnd2, &rc2);
        pt.x = (rc2.left + rc2.right) / 2;
        pt.y = (rc2.top + rc2.bottom) / 2;
        MOVE(pt.x, pt.y);
        LEFT_DOWN();
        MOVE(pt.x + 64, pt.y + 64);
        Sleep(INTERVAL);

        pt.x = s_rcWorkArea.right - 80;
        pt.y = (s_rcWorkArea.top + s_rcWorkArea.bottom) / 2;
        MOVE(pt.x, pt.y);
        LEFT_UP();
        Sleep(INTERVAL);

        GetWindowRect(s_hwnd2, &rc2);
        ok_long(rc2.left, s_rcWorkArea.right - 30);
        ok_long(rc2.top, s_rcWorkArea.top);
        ok_long(rc2.right, s_rcWorkArea.right);
        Sleep(INTERVAL);

        PostMessage(s_hwnd2, WM_CLOSE, 0, 0);
        PostMessage(s_hwnd2, WM_QUIT, 0, 0);
        PostThreadMessage(dwTID, WM_QUIT, 0, 0);
#undef INTERVAL
    }

    static DWORD WINAPI ActionThreadFunc(LPVOID args)
    {
        Window *this_ = (Window *)args;
        this_->DoAction();
        return 0;
    }
};

START_TEST(SHAppBarMessage)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    if (!Window::DoRegisterClass(hInstance))
    {
        skip("Window::DoRegisterClass failed\n");
        return;
    }

    SystemParametersInfo(SPI_GETWORKAREA, 0, &s_rcWorkArea, FALSE);

    HWND hwnd1 = Window::DoCreateMainWnd(hInstance, TEXT("Test1"), 80, 80,
                                         WS_POPUP | WS_THICKFRAME | WS_CLIPCHILDREN);
    if (!hwnd1)
    {
        skip("CreateWindowExW failed\n");
        return;
    }

    HWND hwnd2 = Window::DoCreateMainWnd(hInstance, TEXT("Test2"), 30, 30,
                                         WS_POPUP | WS_BORDER | WS_CLIPCHILDREN);
    if (!hwnd2)
    {
        skip("CreateWindowExW failed\n");
        return;
    }

    s_hwnd1 = hwnd1;
    s_hwnd2 = hwnd2;

    PostMessage(hwnd1, WM_COMMAND, ID_ACTION, 0);

    Window::DoMainLoop();
}
