/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps_new/splitter.cpp
 * PURPOSE:         SplitterBar functions
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

HWND hVSplitter = NULL;
HWND hHSplitter = NULL;

static int HSplitterPos = 0;

int
GetHSplitterPos(VOID)
{
    return HSplitterPos;
}

VOID
SetHSplitterPos(int Pos)
{
    HSplitterPos = Pos;
}

/* Callback for horizontal splitter bar */
LRESULT CALLBACK
HSplitterWindowProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_CREATE:
        {
            SetHSplitterPos(GetWindowHeight(hListView));
        }
        break;

        case WM_LBUTTONDOWN:
        {
            SetCapture(hwnd);
        }
        break;

        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
            if (GetCapture() == hwnd)
            {
                ReleaseCapture();
            }
        break;

        case WM_MOUSEMOVE:
            if (GetCapture() == hwnd)
            {
                int Width = GetClientWindowWidth(hMainWnd) - GetWindowWidth(hTreeView) - SPLIT_WIDTH;
                int NewPos;
                HDWP hdwp;
                POINT Point;

                GetCursorPos(&Point);
                ScreenToClient(hMainWnd, &Point);

                NewPos = Point.y;

                if ((GetClientWindowHeight(hMainWnd) - GetWindowHeight(hStatusBar) - SPLIT_WIDTH) < NewPos)
                    break;

                if ((GetWindowHeight(hToolBar) + SPLIT_WIDTH) > NewPos)
                    break;

                SetHSplitterPos(NewPos);

                hdwp = BeginDeferWindowPos(3);

                /* Size HSplitBar */
                DeferWindowPos(hdwp,
                               hHSplitter,
                               0,
                               GetWindowWidth(hTreeView) + SPLIT_WIDTH,
                               Point.y,
                               Width,
                               SPLIT_WIDTH,
                               SWP_NOZORDER|SWP_NOACTIVATE);

                /* Size ListView */
                DeferWindowPos(hdwp,
                               hListView,
                               0,
                               GetWindowWidth(hTreeView) + SPLIT_WIDTH,
                               GetWindowHeight(hToolBar),
                               Width,
                               Point.y - GetWindowHeight(hToolBar),
                               SWP_NOZORDER|SWP_NOACTIVATE);

                /* Size RichEdit */
                DeferWindowPos(hdwp,
                               hRichEdit,
                               0,
                               GetWindowWidth(hTreeView) + SPLIT_WIDTH,
                               Point.y + SPLIT_WIDTH,
                               Width,
                               GetClientWindowHeight(hMainWnd) - (Point.y + SPLIT_WIDTH + GetWindowHeight(hStatusBar)),
                               SWP_NOZORDER|SWP_NOACTIVATE);

                EndDeferWindowPos(hdwp);
            }
        break;
    }

    return DefWindowProc(hwnd, Msg, wParam, lParam);
}

/* Create horizontal splitter bar */
BOOL
CreateHSplitBar(HWND hwnd)
{
    WCHAR szWindowClass[] = L"HSplitterWindowClass";
    WNDCLASSEXW WndClass = {0};

    WndClass.cbSize        = sizeof(WNDCLASSEXW);
    WndClass.lpszClassName = szWindowClass;
    WndClass.lpfnWndProc   = HSplitterWindowProc;
    WndClass.hInstance     = hInst;
    WndClass.style         = CS_HREDRAW | CS_VREDRAW;
    WndClass.hCursor       = LoadCursor(0, IDC_SIZENS);
    WndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    if (RegisterClassExW(&WndClass) == (ATOM) 0)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    hHSplitter = CreateWindowExW(WS_EX_TRANSPARENT,
                                 szWindowClass,
                                 NULL,
                                 WS_CHILD | WS_VISIBLE,
                                 205, 180, 465, SPLIT_WIDTH,
                                 hwnd,
                                 NULL,
                                 hInst,
                                 NULL);


    if (hHSplitter == NULL)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    ShowWindow(hHSplitter, SW_SHOW);
    UpdateWindow(hHSplitter);

    return TRUE;
}

/* Callback for vertical splitter bar */
LRESULT CALLBACK
VSplitterWindowProc(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
        case WM_LBUTTONDOWN:
            SetCapture(hwnd);
        break;

        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
            if (GetCapture() == hwnd)
            {
                ReleaseCapture();
            }
        break;

        case WM_MOUSEMOVE:
            if (GetCapture() == hwnd)
            {
                HDWP hdwp;
                POINT Point;

                GetCursorPos(&Point);
                ScreenToClient(hMainWnd, &Point);

                if ((GetClientWindowWidth(hMainWnd) - SPLIT_WIDTH) < Point.x)
                    break;

                if (SPLIT_WIDTH > Point.x)
                    break;

                hdwp = BeginDeferWindowPos(5);

                /* Size VSplitBar */
                DeferWindowPos(hdwp,
                               hwnd,
                               0,
                               Point.x,
                               GetWindowHeight(hToolBar),
                               SPLIT_WIDTH,
                               GetClientWindowHeight(hMainWnd) - GetWindowHeight(hToolBar) - GetWindowHeight(hStatusBar),
                               SWP_NOZORDER|SWP_NOACTIVATE);

                /* Size TreeView */
                DeferWindowPos(hdwp,
                               hTreeView,
                               0,
                               0,
                               GetWindowHeight(hToolBar),
                               Point.x,
                               GetClientWindowHeight(hMainWnd) - GetWindowHeight(hToolBar) - GetWindowHeight(hStatusBar),
                               SWP_NOZORDER|SWP_NOACTIVATE);

                /* Size ListView */
                DeferWindowPos(hdwp,
                               hListView,
                               0,
                               Point.x + SPLIT_WIDTH,
                               GetWindowHeight(hToolBar),
                               GetClientWindowWidth(hMainWnd) - (Point.x + SPLIT_WIDTH),
                               GetHSplitterPos() - GetWindowHeight(hToolBar),
                               SWP_NOZORDER|SWP_NOACTIVATE);

                DeferWindowPos(hdwp,
                               hRichEdit,
                               0,
                               Point.x + SPLIT_WIDTH,
                               GetHSplitterPos() + SPLIT_WIDTH,
                               GetClientWindowWidth(hMainWnd) - (Point.x + SPLIT_WIDTH),
                               GetClientWindowHeight(hMainWnd) - (GetHSplitterPos() + SPLIT_WIDTH + GetWindowHeight(hStatusBar)),
                               SWP_NOZORDER|SWP_NOACTIVATE);

                DeferWindowPos(hdwp,
                               hHSplitter,
                               0,
                               Point.x + SPLIT_WIDTH,
                               GetHSplitterPos(),
                               GetClientWindowWidth(hMainWnd) - (Point.x + SPLIT_WIDTH),
                               SPLIT_WIDTH,
                               SWP_NOZORDER|SWP_NOACTIVATE);

                EndDeferWindowPos(hdwp);
            }
        break;
    }

    return DefWindowProc(hwnd, Msg, wParam, lParam);
}

/* Create vertical splitter bar */
BOOL
CreateVSplitBar(HWND hwnd)
{
    WCHAR szWindowClass[] = L"VSplitterWindowClass";
    WNDCLASSEXW WndClass = {0};

    WndClass.cbSize        = sizeof(WNDCLASSEXW);
    WndClass.lpszClassName = szWindowClass;
    WndClass.lpfnWndProc   = VSplitterWindowProc;
    WndClass.hInstance     = hInst;
    WndClass.style         = CS_HREDRAW | CS_VREDRAW;
    WndClass.hCursor       = LoadCursor(0, IDC_SIZEWE);
    WndClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);

    if (RegisterClassExW(&WndClass) == (ATOM) 0)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    hVSplitter = CreateWindowExW(WS_EX_TRANSPARENT,
                                 szWindowClass,
                                 NULL,
                                 WS_CHILD | WS_VISIBLE,
                                 201, 28, SPLIT_WIDTH, 350,
                                 hwnd,
                                 NULL,
                                 hInst,
                                 NULL);


    if (!hVSplitter)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    ShowWindow(hVSplitter, SW_SHOW);
    UpdateWindow(hVSplitter);

    return TRUE;
}
