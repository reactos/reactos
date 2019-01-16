/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for message times
 * PROGRAMMERS:     Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#define TIMER_ID 999
#define TIMER_INTERVAL  500

static INT s_nCount = 0;
static LONG s_nMsgTime = 0;

static LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
            s_nCount = 0;
            s_nMsgTime = GetMessageTime();
            SetTimer(hwnd, TIMER_ID, TIMER_INTERVAL, NULL);
            break;
        case WM_TIMER:
            if (s_nCount != 0)
            {
                ok(GetMessageTime() - s_nMsgTime >= TIMER_INTERVAL / 2,
                   "message time is wrong\n");
            }
            s_nMsgTime = GetMessageTime();
            ok(s_nMsgTime != 0, "message time was zero.\n");
            s_nCount++;
            if (s_nCount >= 5)
            {
                KillTimer(hwnd, TIMER_ID);
                DestroyWindow(hwnd);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

START_TEST(MessageTime)
{
    static const WCHAR s_szName[] = L"MessageTimeTestWindow";
    WNDCLASSW wc;
    ATOM atom;
    HWND hwnd;
    MSG msg;
    BOOL bRet;

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
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0,
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
