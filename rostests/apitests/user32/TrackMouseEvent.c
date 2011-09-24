/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for TrackMouseEvent
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>
#include "helper.h"
#include <undocuser.h>

HWND hWnd1, hWnd2, hWnd3;
HHOOK hMouseHookLL, hMouseHook;

static int get_iwnd(HWND hWnd)
{
    if(hWnd == hWnd1) return 1;
    else if(hWnd == hWnd2) return 2;
    else if(hWnd == hWnd3) return 3;
    else return 0;
}

LRESULT CALLBACK TmeTestProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int iwnd = get_iwnd(hWnd);

    if(message == WM_PAINT)
    {
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        Rectangle(ps.hdc,  ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom);
        EndPaint(hWnd, &ps);
    }

    if(message > WM_USER || !iwnd || IsDWmMsg(message) || IseKeyMsg(message))
        return DefWindowProc(hWnd, message, wParam, lParam);

    switch(message)
    {
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY :
    case WM_GETICON :
    case WM_GETTEXT:
        break;
    case WM_SYSTIMER:
        ok(0, "Got unexpected WM_SYSTIMER in the winproc. wParam=%d\n", wParam);
        break;
    default:
        record_message(iwnd, message, SENT, 0,0);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static LRESULT CALLBACK MouseLLHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    record_message(0, WH_MOUSE_LL, HOOK, wParam, 0);
    return CallNextHookEx(hMouseHookLL, nCode, wParam, lParam);
}

static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    MOUSEHOOKSTRUCT *hs = (MOUSEHOOKSTRUCT*) lParam;
    record_message(get_iwnd(hs->hwnd), WH_MOUSE, HOOK, wParam, hs->wHitTestCode);
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

static void FlushMessages()
{
    MSG msg;

    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE ))
    {
        int iwnd = get_iwnd(msg.hwnd);
        if(iwnd)
        {
            if(msg.message == WM_SYSTIMER)
                record_message(iwnd, msg.message, POST,msg.wParam,0);
            else if(!(msg.message > WM_USER || !iwnd || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
                record_message(iwnd, msg.message, POST,0,0);
        }
        DispatchMessageA( &msg );
    }
}

static void create_test_windows()
{
    WNDCLASSEXW wcex;

    hMouseHookLL = SetWindowsHookExW(WH_MOUSE_LL, MouseLLHookProc, GetModuleHandleW( NULL ), 0);
    hMouseHook = SetWindowsHookExW(WH_MOUSE, MouseHookProc, GetModuleHandleW( NULL ), GetCurrentThreadId());
    ok(hMouseHook!=NULL,"failed to set hook\n");
    ok(hMouseHookLL!=NULL,"failed to set hook\n");
    
    RegisterSimpleClass(TmeTestProc, L"testClass");
    RegisterClassExW(&wcex);

    hWnd1 = CreateWindowW(L"testClass", L"test", WS_OVERLAPPEDWINDOW,
                         100, 100, 500, 500, NULL, NULL, 0, NULL);
    hWnd2 = CreateWindowW(L"testClass", L"test", WS_CHILD,
                         50, 50, 200, 200, hWnd1, NULL, 0, NULL);
    hWnd3 = CreateWindowW(L"testClass", L"test", WS_CHILD,
                         150, 150, 200, 200, hWnd1, NULL, 0, NULL);

    ShowWindow(hWnd1, SW_SHOW);
    UpdateWindow(hWnd1);
    ShowWindow(hWnd2, SW_SHOW);
    UpdateWindow(hWnd2);
    ShowWindow(hWnd3, SW_SHOWNORMAL);
    UpdateWindow(hWnd3);
    //SetWindowPos (hWnd3, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOREDRAW);
}

static void TmeStartTracking(HWND hwnd, DWORD Flags)
{
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(tme);
    tme.dwFlags = Flags;
    tme.hwndTrack  = hwnd;
    tme.dwHoverTime = 1;
    if(!TrackMouseEvent(&tme))
    {
        trace("failed!\n");
    }
}

DWORD TmeQuery(HWND hwnd)
{
    TRACKMOUSEEVENT tme;
    tme.cbSize = sizeof(tme);
    tme.dwFlags = TME_QUERY|TME_HOVER|TME_LEAVE;
    tme.hwndTrack  = hwnd;
    TrackMouseEvent(&tme);
    return tme.dwFlags;
}

#define EXPECT_TME_FLAGS(hWnd, expected)                                                                 \
    { DWORD flags = TmeQuery(hWnd);                                                                      \
      ok(flags == (expected),"wrong tme flags. expected %li, and got %li\n", (DWORD)(expected), flags);  \
    }

MSG_ENTRY empty_chain[]= {{0,0}};

/* the mouse moves over hwnd2 */
MSG_ENTRY mousemove2_chain[]={{2, WM_NCHITTEST},
                              {2, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                              {2, WM_SETCURSOR},
                              {1, WM_SETCURSOR},
                              {2, WM_MOUSEMOVE, POST},
                               {0,0}};

/* the mouse hovers hwnd2 */
MSG_ENTRY mousehover2_chain[]={{2, WM_NCHITTEST},
                               {2, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                               {2, WM_SETCURSOR},
                               {1, WM_SETCURSOR},
                               {2, WM_MOUSEMOVE, POST},
                               {2, WM_SYSTIMER, POST, ID_TME_TIMER},
                               {2, WM_MOUSEHOVER, POST},
                                {0,0}};

/* the mouse leaves hwnd2 and moves to hwnd1 */
MSG_ENTRY mouseleave2to1_chain[]={{1, WM_NCHITTEST},
                                  {1, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                                  {1, WM_SETCURSOR},
                                  {1, WM_MOUSEMOVE, POST},
                                  {2, WM_MOUSELEAVE, POST},
                                   {0,0}};    

/* the mouse leaves hwnd2 and moves to hwnd3 */
MSG_ENTRY mouseleave2to3_chain[]={{3, WM_NCHITTEST},
                                  {3, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                                  {3, WM_SETCURSOR},
                                  {1, WM_SETCURSOR},
                                  {3, WM_MOUSEMOVE, POST},
                                  {2, WM_MOUSELEAVE, POST},
                                   {0,0}};

void Test_TrackMouseEvent()
{
    SetCursorPos(0,0);
    create_test_windows();
    FlushMessages();
    empty_message_cache();

    /* the mouse moves over hwnd2 */
    SetCursorPos(220,220);
    FlushMessages();
    COMPARE_CACHE(mousemove2_chain);
    EXPECT_TME_FLAGS(hWnd2, 0);
    TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
    EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* the mouse hovers hwnd2 */
    SetCursorPos(221,221);
    Sleep(100);
    EXPECT_QUEUE_STATUS(QS_TIMER|QS_MOUSEMOVE, 0);
    EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);
    FlushMessages();
    EXPECT_TME_FLAGS(hWnd2, TME_LEAVE);
    COMPARE_CACHE(mousehover2_chain);

    /* the mouse leaves hwnd2 and moves to hwnd1 */
    SetCursorPos(150,150);
    EXPECT_QUEUE_STATUS(QS_MOUSEMOVE,QS_TIMER );
    EXPECT_TME_FLAGS(hWnd2, TME_LEAVE);
    FlushMessages();
    EXPECT_TME_FLAGS(hWnd2, 0);
    COMPARE_CACHE(mouseleave2to1_chain);

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* the mouse moves over hwnd2 */
    SetCursorPos(220,220);
    EXPECT_QUEUE_STATUS(QS_MOUSEMOVE, QS_TIMER);
    FlushMessages();
    COMPARE_CACHE(mousemove2_chain);
    EXPECT_TME_FLAGS(hWnd2, 0);
    COMPARE_CACHE(empty_chain);
    TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
    TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
    TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
    TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
    EXPECT_QUEUE_STATUS(0, QS_TIMER|QS_MOUSEMOVE);
    FlushMessages();
    COMPARE_CACHE(empty_chain);
    EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* the mouse moves from hwnd2 to the intersection of hwnd2 and hwnd3 */
    SetCursorPos(300,300);
    EXPECT_QUEUE_STATUS(QS_MOUSEMOVE, QS_TIMER);
    FlushMessages();
    EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);
    COMPARE_CACHE(mousemove2_chain);

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* the mouse moves from hwnd2 to hwnd3 */
    SetCursorPos(400,400);
    EXPECT_QUEUE_STATUS(QS_MOUSEMOVE, QS_TIMER);
    FlushMessages();
    EXPECT_TME_FLAGS(hWnd2, 0);
    COMPARE_CACHE(mouseleave2to3_chain);

    FlushMessages();
    COMPARE_CACHE(empty_chain);
}

START_TEST(TrackMouseEvent)
{
    Test_TrackMouseEvent();
}
