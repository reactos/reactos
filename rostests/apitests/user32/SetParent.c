/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SetParent
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#include <winuser.h>
#include "helper.h"

static HWND hWndList[5 + 1];
static const int hWndCount = sizeof(hWndList) / sizeof(hWndList[0]) - 1;
static DWORD dwThreadId;

#define ok_hwnd(val, exp) do                                                                    \
    {                                                                                           \
        HWND _val = (val), _exp = (exp);                                                        \
        int _ival = get_iwnd(_val, FALSE);                                                      \
        int _iexp = get_iwnd(_exp, FALSE);                                                      \
        ok(_val == _exp, #val " = %p (%d), expected %p (%d)\n", _val, _ival, _exp, _iexp);      \
    } while (0)

static
int
get_iwnd(
    _In_ HWND hWnd,
    _In_ BOOL set)
{
    int i;
    if (!hWnd)
        return 0;
    for (i = 1; i <= hWndCount; i++)
    {
        if (hWndList[i] == hWnd)
            return i;
    }
    if (!set)
        return 0;
    for (i = 1; i <= hWndCount; i++)
    {
        if (hWndList[i] == NULL)
        {
            hWndList[i] = hWnd;
            return i;
        }
    }
    ok(0, "Too many windows!\n");
    return 0;
}

static
BOOL
CALLBACK
EnumProc(
    _In_ HWND hWnd,
    _In_ LPARAM lParam)
{
    HWND hLookingFor = (HWND)lParam;
    return hWnd != hLookingFor;
}

static
LRESULT
CALLBACK
WndProc(
    _In_ HWND hWnd,
    _In_ UINT message,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    HWND hTest;
    int iwnd = get_iwnd(hWnd, TRUE);

    ok(GetCurrentThreadId() == dwThreadId, "Thread 0x%lx instead of 0x%lx\n", GetCurrentThreadId(), dwThreadId);
    if (message > WM_USER || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProcW(hWnd, message, wParam, lParam);

    RECOND_MESSAGE(iwnd, message, SENT, wParam, lParam);

    switch(message)
    {
    case WM_DESTROY:
        if (GetParent(hWnd))
        {
            /* child window */
            ok(EnumThreadWindows(dwThreadId, EnumProc, (LPARAM)hWnd), "Child window %p (%d) enumerated\n", hWnd, iwnd);
            ok(!EnumChildWindows(GetParent(hWnd), EnumProc, (LPARAM)hWnd), "Child window %p (%d) not enumerated\n", hWnd, iwnd);
            ok(!EnumThreadWindows(dwThreadId, EnumProc, (LPARAM)GetParent(hWnd)), "Parent window of %p (%d) not enumerated\n", hWnd, iwnd);
        }
        else
        {
            /* top-level window */
            ok(!EnumThreadWindows(dwThreadId, EnumProc, (LPARAM)hWnd), "Window %p (%d) not enumerated in WM_DESTROY\n", hWnd, iwnd);
        }
        if (hWnd == hWndList[3])
        {
            hTest = SetParent(hWndList[4], hWndList[2]);
            ok_hwnd(hTest, hWndList[1]);
            hTest = SetParent(hWndList[5], hWndList[1]);
            ok_hwnd(hTest, hWndList[2]);

            ok_hwnd(GetParent(hWndList[1]), NULL);
            ok_hwnd(GetParent(hWndList[2]), NULL);
            ok_hwnd(GetParent(hWndList[3]), hWndList[1]);
            ok_hwnd(GetParent(hWndList[4]), hWndList[2]);
            ok_hwnd(GetParent(hWndList[5]), hWndList[1]);
        }
        break;
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}


START_TEST(SetParent)
{
    HWND hWnd;
    MSG msg;

    SetCursorPos(0,0);

    dwThreadId = GetCurrentThreadId();
    hWndList[0] = INVALID_HANDLE_VALUE;
    RegisterSimpleClass(WndProc, L"CreateTest");

    hWnd = CreateWindowExW(0, L"CreateTest", NULL, 0,  10, 10, 20, 20,  NULL, NULL, 0, NULL);
    ok(hWnd != NULL, "CreateWindow failed\n");
    ok(hWnd == hWndList[1], "Got %p, expected %p\n", hWnd, hWndList[1]);

    hWnd = CreateWindowExW(0, L"CreateTest", NULL, 0,  40, 10, 20, 20,  NULL, NULL, 0, NULL);
    ok(hWnd != NULL, "CreateWindow failed\n");
    ok(hWnd == hWndList[2], "Got %p, expected %p\n", hWnd, hWndList[2]);

    hWnd = CreateWindowExW(0, L"CreateTest", NULL, WS_CHILD,  60, 10, 20, 20, hWndList[1], NULL, 0, NULL);
    ok(hWnd != NULL, "CreateWindow failed\n");
    ok(hWnd == hWndList[3], "Got %p, expected %p\n", hWnd, hWndList[3]);

    hWnd = CreateWindowExW(0, L"CreateTest", NULL, WS_CHILD,  80, 10, 20, 20, hWndList[1], NULL, 0, NULL);
    ok(hWnd != NULL, "CreateWindow failed\n");
    ok(hWnd == hWndList[4], "Got %p, expected %p\n", hWnd, hWndList[4]);

    hWnd = CreateWindowExW(0, L"CreateTest", NULL, WS_CHILD,  60, 10, 20, 20, hWndList[2], NULL, 0, NULL);
    ok(hWnd != NULL, "CreateWindow failed\n");
    ok(hWnd == hWndList[5], "Got %p, expected %p\n", hWnd, hWndList[5]);

trace("\n");
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
    {
        int iwnd = get_iwnd(msg.hwnd, FALSE);
        if(!(msg.message > WM_USER || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
            RECOND_MESSAGE(iwnd, msg.message, POST, 0, 0);
        DispatchMessageA( &msg );
    }
trace("\n");
    TRACE_CACHE();
trace("\n");

    ok_hwnd(GetParent(hWndList[1]), NULL);
    ok_hwnd(GetParent(hWndList[2]), NULL);
    ok_hwnd(GetParent(hWndList[3]), hWndList[1]);
    ok_hwnd(GetParent(hWndList[4]), hWndList[1]);
    ok_hwnd(GetParent(hWndList[5]), hWndList[2]);

    DestroyWindow(hWndList[1]);
    ok(!IsWindow(hWndList[1]), "\n");
    ok( IsWindow(hWndList[2]), "\n");
    ok(!IsWindow(hWndList[3]), "\n");
    ok( IsWindow(hWndList[4]), "\n");
    ok(!IsWindow(hWndList[5]), "\n");

    ok_hwnd(GetParent(hWndList[1]), NULL);
    ok_hwnd(GetParent(hWndList[2]), NULL);
    ok_hwnd(GetParent(hWndList[3]), NULL);
    ok_hwnd(GetParent(hWndList[4]), hWndList[2]);
    ok_hwnd(GetParent(hWndList[5]), NULL);

trace("\n");
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
    {
        int iwnd = get_iwnd(msg.hwnd, FALSE);
        if(!(msg.message > WM_USER || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
            RECOND_MESSAGE(iwnd, msg.message, POST, 0, 0);
        DispatchMessageA( &msg );
    }
trace("\n");
    TRACE_CACHE();
 }
