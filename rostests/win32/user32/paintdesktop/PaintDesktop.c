/*
 * PROJECT:         ReactOS Tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            rostests/win32/user32/paintdesktop/PaintDesktop.c
 *
 * PURPOSE:         Demonstrates how the user32!PaintDesktop() API visually works.
 *                  This API paints the desktop inside the given HDC with its
 *                  origin always fixed to the origin of the monitor on which
 *                  the window is present.
 *
 * PROGRAMMER:      Hermes Belusca-Maito
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static HINSTANCE hInst;
static PWSTR szTitle       = L"PaintDesktop";
static PWSTR szWindowClass = L"PAINTDESKTOP";

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(HINSTANCE hInstance,
                      HINSTANCE hPrevInstance,
                      LPWSTR    lpCmdLine,
                      int       nCmdShow)
{
    MSG msg;

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MyRegisterClass(hInstance);

    if (!InitInstance (hInstance, nCmdShow))
        return FALSE;

    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASS wc;

    wc.style          = 0;
    wc.lpfnWndProc    = WndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance      = hInstance;
    wc.hIcon          = LoadIconW(NULL, IDI_WINLOGO);
    wc.hCursor        = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName   = NULL;
    wc.lpszClassName  = szWindowClass;

    return RegisterClass(&wc);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance;

    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
    if (!hWnd)
        return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_MOVE:
            InvalidateRect(hWnd, NULL, TRUE);
            UpdateWindow(hWnd);
            break;

        case WM_ERASEBKGND:
            return (LRESULT)PaintDesktop((HDC)wParam);

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}
