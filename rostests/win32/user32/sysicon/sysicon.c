/*
 *  Copyright 2006 Saveliy Tretiakov
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

 /* This testapp demonstrates WS_SYSMENU + WS_EX_DLGMODALFRAME
  * behavior and shows that DrawCaption does care
  * about WS_EX_DLGMODALFRAME and WS_EX_TOOLWINDOW
  */

#include <windows.h>
#include <stdio.h>

WCHAR WndClass[] = L"sysicon_class";
HICON hIcon = NULL, hIconSm = NULL;

LRESULT CALLBACK WndProc(HWND hWnd,
                         UINT msg,
                         WPARAM wParam,
                         LPARAM lParam)
{
    switch (msg)
    {
        case WM_PAINT:
        {
            HDC hDc;
            PAINTSTRUCT Ps;
            RECT Rect;
            GetClientRect(hWnd, &Rect);

            Rect.left = 10;
            Rect.top = 10;
            Rect.right -= 10;
            Rect.bottom = 25;

            hDc = BeginPaint(hWnd, &Ps);
            SetBkMode(hDc, TRANSPARENT);

            DrawCaption(hWnd, hDc, &Rect, DC_GRADIENT | DC_ACTIVE | DC_TEXT | DC_ICON);

            EndPaint(hWnd, &Ps);

            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int APIENTRY wWinMain(HINSTANCE hInst,
                      HINSTANCE hPrevInstance,
                      LPWSTR lpCmdLine,
                      int nCmdShow)
{
    HWND hWnd1a, hWnd1b, hWnd2a, hWnd2b, hWnd3a, hWnd3b;
    MSG msg;
    WNDCLASSEXW wcx;
    UINT result;

    memset(&wcx, 0, sizeof(wcx));
    wcx.cbSize = sizeof(wcx);
    wcx.lpfnWndProc = (WNDPROC)WndProc;
    wcx.hInstance = hInst;
    wcx.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wcx.lpszClassName = WndClass;

    if (!(result = RegisterClassExW(&wcx)))
        return 1;

    /* Load the user icons */
    hIcon   = (HICON)LoadImageW(hInst, MAKEINTRESOURCEW(100), IMAGE_ICON, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
    hIconSm = (HICON)CopyImage(hIcon, IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_COPYFROMRESOURCE);

    /* WS_EX_DLGMODALFRAME */
    hWnd1a = CreateWindowExW(WS_EX_DLGMODALFRAME,
                             WndClass,
                             L"WS_SYSMENU | WS_EX_DLGMODALFRAME without user icon",
                             WS_CAPTION | WS_SYSMENU,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             400, 100,
                             NULL, 0,
                             hInst, NULL);
    if (!hWnd1a)
        return 1;

    ShowWindow(hWnd1a, SW_SHOW);
    UpdateWindow(hWnd1a);

    /* WS_EX_DLGMODALFRAME */
    hWnd1b = CreateWindowExW(WS_EX_DLGMODALFRAME,
                             WndClass,
                             L"WS_SYSMENU | WS_EX_DLGMODALFRAME with user icon",
                             WS_CAPTION | WS_SYSMENU,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             400, 100,
                             NULL, 0,
                             hInst, NULL);

    if (!hWnd1b)
        return 1;

    ShowWindow(hWnd1b, SW_SHOW);
    UpdateWindow(hWnd1b);
    SendMessageW(hWnd1b, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
    SendMessageW(hWnd1b, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);

    hWnd2a = CreateWindowExW(WS_EX_TOOLWINDOW,
                             WndClass,
                             L"WS_SYSMENU | WS_EX_TOOLWINDOW without user icon",
                             WS_CAPTION | WS_SYSMENU,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             400, 100,
                             NULL, 0,
                             hInst, NULL);
    if (!hWnd2a)
        return 1;

    ShowWindow(hWnd2a, SW_SHOW);
    UpdateWindow(hWnd2a);

    hWnd2b = CreateWindowExW(WS_EX_TOOLWINDOW,
                             WndClass,
                             L"WS_SYSMENU | WS_EX_TOOLWINDOW with user icon",
                             WS_CAPTION | WS_SYSMENU,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             400, 100,
                             NULL, 0,
                             hInst, NULL);
    if (!hWnd2b)
        return 1;

    ShowWindow(hWnd2b, SW_SHOW);
    UpdateWindow(hWnd2b);
    SendMessageW(hWnd2b, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
    SendMessageW(hWnd2b, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);

    hWnd3a = CreateWindowExW(0,
                             WndClass,
                             L"WS_SYSMENU without user icon",
                             WS_CAPTION | WS_SYSMENU,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             400, 100,
                             NULL, 0,
                             hInst, NULL);
    if (!hWnd3a)
        return 1;

    ShowWindow(hWnd3a, SW_SHOW);
    UpdateWindow(hWnd3a);

    hWnd3b = CreateWindowExW(0,
                             WndClass,
                             L"WS_SYSMENU with user icon",
                             WS_CAPTION | WS_SYSMENU,
                             CW_USEDEFAULT, CW_USEDEFAULT,
                             400, 100,
                             NULL, 0,
                             hInst, NULL);
    if (!hWnd3b)
        return 1;

    ShowWindow(hWnd3b, SW_SHOW);
    UpdateWindow(hWnd3b);
    SendMessageW(hWnd3b, WM_SETICON, ICON_SMALL, (LPARAM)hIconSm);
    SendMessageW(hWnd3b, WM_SETICON, ICON_BIG,   (LPARAM)hIcon);

    while(GetMessageW(&msg, NULL, 0, 0 ))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (hIcon)   DestroyIcon(hIcon);
    if (hIconSm) DestroyIcon(hIconSm);

    UnregisterClassW(WndClass, hInst);
    return 0;
}
