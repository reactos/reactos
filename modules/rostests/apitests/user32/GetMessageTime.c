/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetMessageTime and GetTickCount
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
#include "precomp.h"

#define TIMER_ID 999
#define TIMER_INTERVAL 500  /* 500 milliseconds */

#define MOUSE_LOCATION_X(x) ((DWORD)(x) * 0xFFFF / GetSystemMetrics(SM_CXSCREEN))
#define MOUSE_LOCATION_Y(y) ((DWORD)(y) * 0xFFFF / GetSystemMetrics(SM_CYSCREEN))

#define WIN_X   50
#define WIN_Y   50
#define WIN_CX  100
#define WIN_CY  100

static INT s_nCount = 0;
static LONG s_nMsgTime = 0;

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static BOOL s_bReach_WM_MOUSEMOVE;
    static BOOL s_bReach_WM_LBUTTONDOWN;
    static BOOL s_bReach_WM_LBUTTONUP;
    switch (uMsg)
    {
        case WM_CREATE:
            s_nCount = 0;
            s_nMsgTime = GetMessageTime();
            SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);
            s_bReach_WM_MOUSEMOVE = FALSE;
            s_bReach_WM_LBUTTONDOWN = FALSE;
            s_bReach_WM_LBUTTONUP = FALSE;
            break;
        case WM_TIMER:
            if (s_nCount == 5)
            {
                KillTimer(hwnd, TIMER_ID);
                DestroyWindow(hwnd);
                break;
            }
            if (s_nCount != 0)
            {
                ok(GetMessageTime() - s_nMsgTime >= TIMER_INTERVAL / 2,
                   "GetMessageTime() is wrong, compared to previous one\n");
                ok(GetTickCount() - (DWORD)GetMessageTime() < TIMER_INTERVAL / 2,
                   "GetMessageTime() is wrong, compared to GetTickCount()\n");
            }
            s_nMsgTime = GetMessageTime();
            ok(s_nMsgTime != 0, "message time was zero.\n");
            s_nCount++;
            if (s_nCount == 5)
            {
                mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
                            MOUSE_LOCATION_X(WIN_X + WIN_CX / 2),
                            MOUSE_LOCATION_Y(WIN_Y + WIN_CY / 2),
                            0, 0);
                mouse_event(MOUSEEVENTF_MOVE, 1, 1, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
                mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
            }
            break;
        case WM_MOUSEMOVE:
            trace("WM_MOUSEMOVE\n");
            ok_int(s_nCount, 5);
            ok(GetMessageTime() - s_nMsgTime < TIMER_INTERVAL,
               "GetMessageTime() is wrong, compared to previous one\n");
            ok(GetTickCount() - (DWORD)GetMessageTime() < TIMER_INTERVAL / 2,
               "GetMessageTime() is wrong, compared to GetTickCount()\n");
            s_bReach_WM_MOUSEMOVE = TRUE;
            break;
        case WM_LBUTTONDOWN:
            trace("WM_LBUTTONDOWN\n");
            ok_int(s_nCount, 5);
            ok(GetMessageTime() - s_nMsgTime < TIMER_INTERVAL,
               "GetMessageTime() is wrong, compared to previous one\n");
            ok(GetTickCount() - (DWORD)GetMessageTime() < TIMER_INTERVAL / 2,
               "GetMessageTime() is wrong, compared to GetTickCount()\n");
            s_bReach_WM_LBUTTONDOWN = TRUE;
            break;
        case WM_LBUTTONUP:
            trace("WM_LBUTTONUP\n");
            ok_int(s_nCount, 5);
            ok(GetMessageTime() - s_nMsgTime < TIMER_INTERVAL,
               "GetMessageTime() is wrong, compared to previous one\n");
            ok(GetTickCount() - (DWORD)GetMessageTime() < TIMER_INTERVAL / 2,
               "GetMessageTime() is wrong, compared to GetTickCount()\n");
            s_bReach_WM_LBUTTONUP = TRUE;
            break;
        case WM_DESTROY:
            ok_int(s_bReach_WM_MOUSEMOVE, TRUE);
            ok_int(s_bReach_WM_LBUTTONDOWN, TRUE);
            ok_int(s_bReach_WM_LBUTTONUP, TRUE);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

START_TEST(GetMessageTime)
{
    static const WCHAR s_szName[] = L"MessageTimeTestWindow";
    WNDCLASSW wc;
    ATOM atom;
    HWND hwnd;
    MSG msg;
    BOOL bRet;

    mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE,
                MOUSE_LOCATION_X(1), MOUSE_LOCATION_Y(1),
                0, 0);

    ZeroMemory(&wc, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName = s_szName;
    atom = RegisterClassW(&wc);
    ok(atom != 0, "RegisterClassW\n");

    hwnd = CreateWindowW(s_szName, s_szName,
                         WS_OVERLAPPEDWINDOW,
                         WIN_X, WIN_Y, WIN_CX, WIN_CY,
                         NULL, NULL, GetModuleHandleW(NULL), NULL);
    ok(hwnd != NULL, "CreateWindowW\n");

    if (hwnd)
    {
        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);

        while (GetMessageW(&msg, NULL, 0, 0))
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    else
    {
        skip("hwnd was NULL.\n");
    }

    bRet = UnregisterClassW(s_szName, GetModuleHandleW(NULL));
    ok_int(bRet, 1);

    ok_int(s_nCount, 5);
    ok(s_nMsgTime != 0, "message time was zero.\n");
}
