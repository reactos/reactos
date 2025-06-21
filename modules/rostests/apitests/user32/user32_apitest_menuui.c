/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Tests for Menu UI
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#define CLASSNAME L"user32_apitest_menuui"

#define MENUID_100 100
#define MENUID_101 101
#define MENUID_200 200
#define MENUID_201 201

static HMENU
CreateMyMenuBarMenu(VOID)
{
    HMENU hMenu = CreateMenu();
    HMENU hSubMenu = CreatePopupMenu();
    InsertMenuW(hSubMenu, -1, MF_BYPOSITION | MF_STRING, MENUID_200, L"Item 200");
    InsertMenuW(hSubMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    InsertMenuW(hSubMenu, -1, MF_BYPOSITION | MF_STRING, MENUID_201, L"Item 201");
    InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hSubMenu, L"&File");
    return hMenu;
}

static HMENU
CreateMyPopupMenu(BOOL bShift)
{
    HMENU hMenu = CreatePopupMenu();
    if (bShift)
    {
        HMENU hSubMenu = CreateMyPopupMenu(FALSE);
        InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, MENUID_100, L"Item 100");
        InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hSubMenu, L"Sub Menu");
    }
    else
    {
        InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, MENUID_100, L"Item 100");
        InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
        InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, MENUID_101, L"Item 101");
    }
    return hMenu;
}

static VOID
OnContextMenu(HWND hwnd, HMENU hMenu, UINT uFlags)
{
    POINT pt;
    GetCursorPos(&pt);

    SetForegroundWindow(hwnd);

    INT nID = TrackPopupMenuEx(hMenu, uFlags | TPM_RETURNCMD, pt.x, pt.y, hwnd, NULL);
    if (nID)
        PostMessageW(hwnd, WM_COMMAND, nID, 0);
}

static VOID
OnCommand(HWND hwnd, UINT nID)
{
    SetPropW(hwnd, L"Hit", UlongToHandle(nID));
}

static
LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            return 0;
        case WM_RBUTTONDOWN:
        {
            HMENU hMenu = CreateMyPopupMenu(GetKeyState(VK_SHIFT) < 0);
            SetPropW(hwnd, L"Hit", NULL);
            OnContextMenu(hwnd, hMenu, 0);
            DestroyMenu(hMenu);
            break;
        }
        case WM_INITMENU:
            SetPropW(hwnd, L"Hit", NULL);
            break;
        case WM_COMMAND:
            OnCommand(hwnd, LOWORD(wParam));
            break;
        case WM_DESTROY:
            SetPropW(hwnd, L"Hit", NULL);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

INT WINAPI
wWinMain(HINSTANCE   hInstance,
         HINSTANCE   hPrevInstance,
         LPWSTR      lpCmdLine,
         INT         nCmdShow)
{
    WNDCLASSW wc = { CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, WindowProc };
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASSNAME;
    if (!RegisterClassW(&wc))
        return 1;

    LPCWSTR pszText = (lpCmdLine && lpCmdLine[0]) ? lpCmdLine : L"user32_apitest_menuui";

    HMENU hMenu = CreateMyMenuBarMenu();
    HWND hwnd = CreateWindowW(CLASSNAME, pszText, WS_OVERLAPPEDWINDOW,
                              CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
                              NULL, hMenu, hInstance, NULL);
    if (!hwnd)
    {
        DestroyMenu(hMenu);
        return 1;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DestroyMenu(hMenu);
    return (INT)msg.wParam;
}
