/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "controls.h"

static ATOM g_Atom = 0;

static
LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT Result = FALSE;
    PPAGE_HOST PageHost;
    if (msg == WM_CREATE)
    {
        PageHost = (PPAGE_HOST)((LPCREATESTRUCT)lParam)->lpCreateParams;
        if (PageHost == NULL)
            return FALSE;
        PageHost->Wnd = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)PageHost);
    }
    else
    {
        PageHost = (PPAGE_HOST)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (PageHost)
        Result = CallPageProc(PageHost, msg, wParam, lParam);
    if (Result == FALSE)
        Result = DefWindowProc(hwnd, msg, wParam, lParam);
    return Result;
}

static inline
ATOM
RegisterPageHost(HINSTANCE hInst)
{
    if (g_Atom)
        return g_Atom;
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = (WNDPROC)WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"PageHost";
    wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    g_Atom = RegisterClassW(&wc);
    return g_Atom;
}

PPAGE_HOST
CreatePageHost(HINSTANCE hInst,
               HWND Parent,
               int x, int y, int cx, int cy,
               PPAGE PageData)
{
    PPAGE_HOST PageHost;

    if (!RegisterPageHost(hInst)) return NULL;

    PageHost = (PPAGE_HOST)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PAGE_HOST));
    if (!PageHost) return NULL;

    PageHost->WndParent = Parent;
    PageHost->PageData = PageData;
    PageHost->Wnd = CreateWindowExW(0, L"PageHost", NULL, WS_CHILD,
                                    x, y, cx, cy,
                                    Parent, NULL, hInst, PageHost);
    if (!PageHost->Wnd)
    {
        HeapFree(GetProcessHeap(), 0, PageHost);
        return NULL;
    }

    return PageHost;
}

LRESULT
CallPageProc(PPAGE_HOST PageHost, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (!PageHost || !PageHost->PageData || !PageHost->PageData->PageProc)
        return FALSE;
    return PageHost->PageData->PageProc(PageHost, msg, wParam, lParam);
}

void
ShowPage(PPAGE_HOST PageHost)
{
    ShowWindow(PageHost->Wnd, SW_SHOW);
}

void
HidePage(PPAGE_HOST PageHost)
{
    ShowWindow(PageHost->Wnd, SW_HIDE);
}

void
DestroyPageHost(PPAGE_HOST PageHost)
{
    if (!PageHost) return;
    DestroyWindow(PageHost->Wnd);
    HeapFree(GetProcessHeap(), 0, PageHost);
}
