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
 * start7.cpp - module plumbing: pane canvases, search box subclass and
 * the public factory functions of the Win7-style Start menu.
 */

#include "../precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(startmenu7);

static const LPCWSTR s_szCanvasPropParent = L"SM7Parent";
static const LPCWSTR s_szCanvasPropPane   = L"SM7Pane";

VOID
SM7FillGradient(HDC hdc, const RECT &rc, COLORREF c1, COLORREF c2)
{
    int cy = rc.bottom - rc.top;
    if (cy <= 0 || rc.right <= rc.left)
        return;

    SelectObject(hdc, GetStockObject(DC_BRUSH));
    for (int y = 0; y < cy; y++)
    {
        COLORREF c = RGB(GetRValue(c1) + (GetRValue(c2) - GetRValue(c1)) * y / cy,
                         GetGValue(c1) + (GetGValue(c2) - GetGValue(c1)) * y / cy,
                         GetBValue(c1) + (GetBValue(c2) - GetBValue(c1)) * y / cy);
        RECT rl = { rc.left, rc.top + y, rc.right, rc.top + y + 1 };
        SetDCBrushColor(hdc, c);
        FillRect(hdc, &rl, (HBRUSH)GetStockObject(DC_BRUSH));
    }
}

/*****************************************************************************
 * Pane canvases: dumb child surfaces that delegate painting and mouse input
 * to their parent menu window.
 */

static LRESULT CALLBACK
SM7CanvasProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndParent = (HWND)GetPropW(hwnd, s_szCanvasPropParent);
    INT_PTR pane = (INT_PTR)GetPropW(hwnd, s_szCanvasPropPane);

    switch (uMsg)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (hdc && hwndParent)
                SendMessageW(hwndParent, UWM_CANVAS_PAINT, (WPARAM)hdc, pane);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        {
            if (hwndParent)
            {
                POINT pt = { SM7_GET_X_LPARAM(lParam), SM7_GET_Y_LPARAM(lParam) };
                ClientToScreen(hwnd, &pt);
                ScreenToClient(hwndParent, &pt);
                SendMessageW(hwndParent, UWM_CANVAS_MOUSE, uMsg, MAKELPARAM(pt.x, pt.y));

                TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
            }
            return 0;
        }

        case WM_MOUSELEAVE:
            if (hwndParent)
                SendMessageW(hwndParent, UWM_CANVAS_LEAVE, 0, pane);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_NCDESTROY:
            RemovePropW(hwnd, s_szCanvasPropParent);
            RemovePropW(hwnd, s_szCanvasPropPane);
            break;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

BOOL
SM7RegisterCanvasClass(HINSTANCE hInstance)
{
    WNDCLASSW wc = { 0 };

    if (GetClassInfoW(hInstance, L"SM7Canvas", &wc))
        return TRUE;

    wc.lpfnWndProc = SM7CanvasProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"SM7Canvas";
    return RegisterClassW(&wc) != 0;
}

/*****************************************************************************
 * Search edit subclass: special keys are forwarded to the menu window.
 */

LRESULT CALLBACK
SM7EditProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndParent = (HWND)GetPropW(hwnd, s_szCanvasPropParent);
    WNDPROC fnOrig = (WNDPROC)::GetWindowLongPtrW(hwnd, GWLP_WNDPROC);

    if (hwndParent && uMsg == WM_KEYDOWN)
    {
        switch (wParam)
        {
            case VK_ESCAPE:
                SendMessageW(hwndParent, WM_KEYDOWN, VK_ESCAPE, 0);
                return 0;

            case VK_RETURN:
                SendMessageW(hwndParent, WM_KEYDOWN, VK_RETURN, 0);
                return 0;

            case VK_TAB:
                SetFocus(hwndParent);
                return 0;

            case VK_UP:
            case VK_DOWN:
            case VK_LEFT:
            case VK_RIGHT:
                SendMessageW(hwndParent, WM_KEYDOWN, wParam, lParam);
                return 0;
        }
    }

    return CallWindowProcW(fnOrig, hwnd, uMsg, wParam, lParam);
}

VOID
CWin7StartMenu::SetSearchProps(HWND hwndSearch)
{
    SetPropW(hwndSearch, s_szCanvasPropParent, m_hWnd);
}

VOID
CWin7StartMenu::ClearSearchProps(HWND hwndSearch)
{
    RemovePropW(hwndSearch, s_szCanvasPropParent);
}

VOID
CWin7StartMenu::AttachPane(HWND hwndPane, INT pane)
{
    SetPropW(hwndPane, s_szCanvasPropParent, m_hWnd);
    SetPropW(hwndPane, s_szCanvasPropPane, (HANDLE)(INT_PTR)pane);
}

/*****************************************************************************
 * Public interface
 */

BOOL UseModernStartMenu(VOID)
{
    SHELLSTATE ss;

    ZeroMemory(&ss, sizeof(ss));
    SHGetSetSettings(&ss, SSF_STARTPANELON, FALSE);
    return ss.fStartPanelOn != 0;
}

HRESULT
CWin7StartMenu_CreateInstance(IN OUT ITrayWindow *Tray, REFIID riid, PVOID *ppv)
{
    return ShellObjectCreatorInit<CWin7StartMenu>(Tray, riid, ppv);
}

IMenuPopup*
CreateWin7StartMenu(IN ITrayWindow *Tray)
{
    IMenuPopup *pPopup = NULL;

    if (SUCCEEDED(CWin7StartMenu_CreateInstance(Tray, IID_PPV_ARG(IMenuPopup, &pPopup))))
        return pPopup;

    ERR("Failed to create the Win7-style start menu\n");
    return NULL;
}
