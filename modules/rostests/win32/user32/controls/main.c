/*
 * PROJECT:     ReactOS Tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     User32 Control Gallery
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh <madamin@pm.me>
 */

#include "controls.h"

static PAGE g_Pages[] =
{
    { PAGE_BUTTONS,     L"Buttons",         ButtonPageProc },
    { PAGE_COMBO,       L"Combo Boxes",     ComboPageProc },
    { PAGE_EDIT,        L"Edit Controls",   EditPageProc },
    { PAGE_ICONTITLE,   L"Icon Title",      IconTitlePageProc },
    { PAGE_LISTBOX,     L"List Boxes",      ListBoxPageProc },
    { PAGE_MDI,         L"MDI",             MdiPageProc },
    { PAGE_SCROLLBAR,   L"Scroll Bars",     ScrollBarPageProc },
    { PAGE_STATIC,      L"Static",          StaticPageProc },
};

static
LRESULT CALLBACK
WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
        {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
            PNAVIGATOR Navigator = CreateNavigationHost(hInst, hwnd, g_Pages, _countof(g_Pages));
            if (Navigator == NULL)
            {
                PostQuitMessage(-1);
                return -1;
            }
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)Navigator);
            break;
        }

        case WM_COMMAND:
        {
            PNAVIGATOR Navigator = (PNAVIGATOR)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (LOWORD(wParam) == IDC_NAVIGATION_LIST && HIWORD(wParam) == LBN_SELCHANGE)
            {
                int Sel = SendMessageW(Navigator->WndNavigationList, LB_GETCURSEL, 0, 0);
                if (Sel != LB_ERR)
                {
                    NavigateTo(Navigator, (PAGE_ID)Sel);
                }
            }
            break;
        }

        case WM_SIZE:
        {
            PNAVIGATOR Navigator = (PNAVIGATOR)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            UpdateSize(Navigator);
            break;
        }

        case WM_DESTROY:
        {
            PNAVIGATOR Navigator = (PNAVIGATOR)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            DestroyNavigationHost(Navigator);
            PostQuitMessage(0);
            break;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

INT
WINAPI
wWinMain(HINSTANCE hInst,
         HINSTANCE hPrev,
         LPWSTR Cmd,
         int iCmd)
{
    WNDCLASS wc = {0};
    HWND hwnd;
    MSG msg;

    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"User32Gallery";
    RegisterClass(&wc);

    hwnd = CreateWindowEx(
        0,
        wc.lpszClassName,
        L"USER32 Control Gallery",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        640,
        400,
        NULL,
        NULL,
        hInst,
        NULL);
    ShowWindow(hwnd, iCmd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
