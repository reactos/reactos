/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for TrackMouseEvent
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "precomp.h"

HWND hWnd1, hWnd2, hWnd3;
HHOOK hMouseHookLL, hMouseHook;
int ignore_timer = 0, ignore_mouse = 0, ignore_mousell = 0;

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
        RECORD_MESSAGE(iwnd, message, SENT, 0,0);
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

static LRESULT CALLBACK MouseLLHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret;
    RECORD_MESSAGE(0, WH_MOUSE_LL, HOOK, wParam, 0);
    ret = CallNextHookEx(hMouseHookLL, nCode, wParam, lParam);
    if(ignore_mousell)
        return TRUE;
    return ret;
}

static LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    MOUSEHOOKSTRUCT *hs = (MOUSEHOOKSTRUCT*) lParam;
    LRESULT ret;
    RECORD_MESSAGE(get_iwnd(hs->hwnd), WH_MOUSE, HOOK, wParam, hs->wHitTestCode);
    ret = CallNextHookEx(hMouseHook, nCode, wParam, lParam);
    if(ignore_mouse)
        return TRUE;
    return ret;
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
            {
                RECORD_MESSAGE(iwnd, msg.message, POST,msg.wParam,0);
                if(ignore_timer)
                    continue;
            }
            else if(!(msg.message > WM_USER || !iwnd || IsDWmMsg(msg.message) || IseKeyMsg(msg.message)))
                RECORD_MESSAGE(iwnd, msg.message, POST,0,0);
        }
        DispatchMessageA( &msg );
    }
}

#define FLUSH_MESSAGES(expected, notexpected) \
    { EXPECT_QUEUE_STATUS(expected, notexpected);\
      FlushMessages();\
    }

static void create_test_windows()
{
    hMouseHookLL = SetWindowsHookExW(WH_MOUSE_LL, MouseLLHookProc, GetModuleHandleW( NULL ), 0);
    hMouseHook = SetWindowsHookExW(WH_MOUSE, MouseHookProc, GetModuleHandleW( NULL ), GetCurrentThreadId());
    ok(hMouseHook!=NULL,"failed to set hook\n");
    ok(hMouseHookLL!=NULL,"failed to set hook\n");

    RegisterSimpleClass(TmeTestProc, L"testClass");

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

static void destroy_test_window()
{
    DestroyWindow(hWnd1);
    UnregisterClassW(L"testClass", 0);
    UnhookWindowsHookEx(hMouseHookLL);
    UnhookWindowsHookEx(hMouseHook);
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

#define MOVE_CURSOR(x,y) mouse_event(MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE ,                           \
                                     x*(65535/GetSystemMetrics(SM_CXVIRTUALSCREEN)),                     \
                                     y*(65535/GetSystemMetrics(SM_CYVIRTUALSCREEN)) , 0,0);

/* the mouse moves over hwnd2 */
MSG_ENTRY mousemove2_chain[]={{0, WH_MOUSE_LL, HOOK, WM_MOUSEMOVE},
                              {2, WM_NCHITTEST},
                              {2, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                              {2, WM_SETCURSOR},
                              {1, WM_SETCURSOR},
                              {2, WM_MOUSEMOVE, POST},
                               {0,0}};

/* the mouse hovers hwnd2 */
MSG_ENTRY mousehover2_chain[]={{0, WH_MOUSE_LL, HOOK, WM_MOUSEMOVE},
                               {2, WM_NCHITTEST},
                               {2, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                               {2, WM_SETCURSOR},
                               {1, WM_SETCURSOR},
                               {2, WM_MOUSEMOVE, POST},
                               {2, WM_SYSTIMER, POST, ID_TME_TIMER},
                               {2, WM_MOUSEHOVER, POST},
                                {0,0}};

/* the mouse leaves hwnd2 and moves to hwnd1 */
MSG_ENTRY mouseleave2to1_chain[]={{0, WH_MOUSE_LL, HOOK, WM_MOUSEMOVE},
                                  {1, WM_NCHITTEST},
                                  {1, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                                  {1, WM_SETCURSOR},
                                  {1, WM_MOUSEMOVE, POST},
                                  {2, WM_MOUSELEAVE, POST},
                                   {0,0}};

/* the mouse leaves hwnd2 and moves to hwnd3 */
MSG_ENTRY mouseleave2to3_chain[]={{0, WH_MOUSE_LL, HOOK, WM_MOUSEMOVE},
                                  {3, WM_NCHITTEST},
                                  {3, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                                  {3, WM_SETCURSOR},
                                  {1, WM_SETCURSOR},
                                  {3, WM_MOUSEMOVE, POST},
                                  {2, WM_MOUSELEAVE, POST},
                                   {0,0}};

/* the mouse hovers hwnd3 */
MSG_ENTRY mousehover3_chain[]={{0, WH_MOUSE_LL, HOOK, WM_MOUSEMOVE},
                               {3, WM_NCHITTEST},
                               {3, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                               {3, WM_SETCURSOR},
                               {1, WM_SETCURSOR},
                               {3, WM_MOUSEMOVE, POST},
                               {3, WM_SYSTIMER, POST, ID_TME_TIMER},
                               {3, WM_MOUSEHOVER, POST},
                                {0,0}};

/* the mouse hovers hwnd3 without moving */
MSG_ENTRY mousehover3_nomove_chain[]={{3, WM_SYSTIMER, POST, ID_TME_TIMER},
                                      {3, WM_MOUSEHOVER, POST},
                                      {0,0}};

/* the mouse hovers hwnd3 and the timer is not dispatched */
MSG_ENTRY mousehover3_droptimer_chain[]={{0, WH_MOUSE_LL, HOOK, WM_MOUSEMOVE},
                                         {3, WM_NCHITTEST},
                                         {3, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                                         {3, WM_SETCURSOR},
                                         {1, WM_SETCURSOR},
                                         {3, WM_MOUSEMOVE, POST},
                                         {3, WM_SYSTIMER, POST, ID_TME_TIMER},
                                         {0,0}};

/* the mouse hovers hwnd3 and mouse message is dropped by WH_MOUSE */
MSG_ENTRY mousehover3_dropmouse_chain[]={{0, WH_MOUSE_LL, HOOK, WM_MOUSEMOVE},
                                         {3, WM_NCHITTEST},
                                         {3, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                                         {3, WM_SYSTIMER, POST, ID_TME_TIMER},
                                         {3, WM_MOUSEHOVER, POST},
                                         {0,0}};

/* the mouse hovers hwnd3 and mouse message is dropped by WH_MOUSE_LL */
MSG_ENTRY mousehover3_dropmousell_chain[]={{0, WH_MOUSE_LL, HOOK, WM_MOUSEMOVE},
                                         {3, WM_SYSTIMER, POST, ID_TME_TIMER},
                                         {3, WM_MOUSEHOVER, POST},
                                         {0,0}};

/* the mouse leaves hwnd3 and moves to hwnd2 and mouse message is dropped by WH_MOUSE */
MSG_ENTRY mouseleave3to2_dropmouse_chain[]={{0, WH_MOUSE_LL, HOOK, WM_MOUSEMOVE},
                                            {2, WM_NCHITTEST},
                                            {2, WH_MOUSE,HOOK, WM_MOUSEMOVE, HTCLIENT},
                                            {0,0}};

/* the mouse leaves hwnd3 and moves to hwnd2 and mouse message is dropped by WH_MOUSE_LL */
MSG_ENTRY mouseleave3to2_dropmousell_chain[]={{0, WH_MOUSE_LL, HOOK, WM_MOUSEMOVE},
                                             {0,0}};

/* after WH_MOUSE drops WM_MOUSEMOVE, WM_MOUSELEAVE is still in the queue */
MSG_ENTRY mouseleave3_remainging_chain[]={{3, WM_MOUSELEAVE, POST},
                                          {0,0}};

void Test_TrackMouseEvent()
{
    MOVE_CURSOR(0,0);
    create_test_windows();
    FlushMessages();
    EMPTY_CACHE();

    /* the mouse moves over hwnd2 */
    MOVE_CURSOR(220,220);
    FlushMessages();
    COMPARE_CACHE(mousemove2_chain);
    EXPECT_TME_FLAGS(hWnd2, 0);

    /* Begin tracking mouse events for hWnd2 */
    TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
    EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* the mouse hovers hwnd2 */
    MOVE_CURSOR(221, 221);
    Sleep(100);
    EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);
    FLUSH_MESSAGES(QS_TIMER|QS_MOUSEMOVE, 0);
    EXPECT_TME_FLAGS(hWnd2, TME_LEAVE);
    COMPARE_CACHE(mousehover2_chain);

    /* the mouse leaves hwnd2 and moves to hwnd1 */
    MOVE_CURSOR(150, 150);
    EXPECT_TME_FLAGS(hWnd2, TME_LEAVE);
    FLUSH_MESSAGES(QS_MOUSEMOVE,QS_TIMER );
    EXPECT_TME_FLAGS(hWnd2, 0);
    COMPARE_CACHE(mouseleave2to1_chain);

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* the mouse moves over hwnd2 */
    MOVE_CURSOR(220,220);
    FLUSH_MESSAGES(QS_MOUSEMOVE, QS_TIMER);
    COMPARE_CACHE(mousemove2_chain);
    EXPECT_TME_FLAGS(hWnd2, 0);

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* Begin tracking mouse events for hWnd2 */
    TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
    TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
    TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
    TmeStartTracking(hWnd2, TME_HOVER|TME_LEAVE);
    FLUSH_MESSAGES(0, QS_TIMER|QS_MOUSEMOVE);
    COMPARE_CACHE(empty_chain);
    EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);

    /* the mouse moves from hwnd2 to the intersection of hwnd2 and hwnd3 */
    MOVE_CURSOR(300,300);
    FLUSH_MESSAGES(QS_MOUSEMOVE, QS_TIMER);
    EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);
    COMPARE_CACHE(mousemove2_chain);

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* the mouse moves from hwnd2 to hwnd3 */
    MOVE_CURSOR(400,400);
    FLUSH_MESSAGES(QS_MOUSEMOVE, QS_TIMER);
    EXPECT_TME_FLAGS(hWnd2, 0);
    COMPARE_CACHE(mouseleave2to3_chain);

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* Begin tracking mouse events for hWnd3 */
    TmeStartTracking(hWnd3, TME_HOVER|TME_LEAVE);
    FLUSH_MESSAGES(0, QS_TIMER|QS_MOUSEMOVE);
    COMPARE_CACHE(empty_chain);
    EXPECT_TME_FLAGS(hWnd2, TME_HOVER|TME_LEAVE);

    /* the mouse hovers hwnd3 */
    MOVE_CURSOR(401,401);
    Sleep(100);
    EXPECT_TME_FLAGS(hWnd3, TME_HOVER|TME_LEAVE);
    FLUSH_MESSAGES(QS_TIMER|QS_MOUSEMOVE, 0);
    EXPECT_TME_FLAGS(hWnd3, TME_LEAVE);
    COMPARE_CACHE(mousehover3_chain);

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* Begin tracking mouse events for hWnd3 */
    TmeStartTracking(hWnd3, TME_HOVER );
    FLUSH_MESSAGES(0, QS_TIMER|QS_MOUSEMOVE);
    COMPARE_CACHE(empty_chain);
    EXPECT_TME_FLAGS(hWnd3, TME_HOVER|TME_LEAVE);

    /* make sure that the timer will fire before the mouse moves */
    Sleep(100);
    FlushMessages();
    COMPARE_CACHE(mousehover3_nomove_chain);
    EXPECT_TME_FLAGS(hWnd3, TME_LEAVE);

    Sleep(100);
    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* the mouse hovers hwnd3 and the timer is not dispatched*/
    TmeStartTracking(hWnd3, TME_HOVER );
    ignore_timer = TRUE;
    MOVE_CURSOR(400,400);
    Sleep(100);
    EXPECT_TME_FLAGS(hWnd3, TME_HOVER|TME_LEAVE);
    FLUSH_MESSAGES(QS_TIMER|QS_MOUSEMOVE, 0);     /* the loop drops WM_SYSTIMER */
    EXPECT_TME_FLAGS(hWnd3, TME_HOVER|TME_LEAVE); /* TME_HOVER is still active  */
    COMPARE_CACHE(mousehover3_droptimer_chain);   /* we get no WM_MOUSEHOVER    */
    ignore_timer = FALSE;

    /* the mouse hovers hwnd3 and mouse message is dropped by WH_MOUSE_LL */
    ignore_mousell = TRUE;
    MOVE_CURSOR(402,402);
    Sleep(100);
    EXPECT_TME_FLAGS(hWnd3, TME_HOVER|TME_LEAVE);
    FLUSH_MESSAGES(QS_TIMER, QS_MOUSEMOVE);         /* WH_MOUSE_LL drops WM_MOUSEMOVE */
    EXPECT_TME_FLAGS(hWnd3, TME_LEAVE);
    COMPARE_CACHE(mousehover3_dropmousell_chain);   /* we get WM_MOUSEHOVER normaly */
    ignore_mousell = FALSE;

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* the mouse hovers hwnd3 and mouse message is dropped by WH_MOUSE */
    ignore_mouse = TRUE;
    TmeStartTracking(hWnd3, TME_HOVER );
    MOVE_CURSOR(401,401);
    Sleep(100);
    EXPECT_TME_FLAGS(hWnd3, TME_HOVER|TME_LEAVE);
    FLUSH_MESSAGES(QS_TIMER|QS_MOUSEMOVE, 0);     /* WH_MOUSE drops WM_MOUSEMOVE */
    EXPECT_TME_FLAGS(hWnd3, TME_LEAVE);
    COMPARE_CACHE(mousehover3_dropmouse_chain);   /* we get WM_MOUSEHOVER normaly */
    ignore_mouse = FALSE;

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* the mouse leaves hwnd3 and moves to hwnd2 and mouse message is dropped by WH_MOUSE_LL */
    ignore_mousell = TRUE;
    TmeStartTracking(hWnd3, TME_LEAVE );
    MOVE_CURSOR(220,220);
    FLUSH_MESSAGES(0, QS_MOUSEMOVE|QS_TIMER);         /* WH_MOUSE drops WM_MOUSEMOVE */
    EXPECT_TME_FLAGS(hWnd3, TME_LEAVE);               /* all flags are removed */
    COMPARE_CACHE(mouseleave3to2_dropmousell_chain);  /* we get no WM_MOUSELEAVE */
    ignore_mousell = FALSE;

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    /* the mouse leaves hwnd3 and moves to hwnd2 and mouse message is dropped by WH_MOUSE */
    ignore_mouse = TRUE;
    MOVE_CURSOR(220,220);
    FLUSH_MESSAGES(QS_MOUSEMOVE, QS_TIMER);         /* WH_MOUSE drops WM_MOUSEMOVE */
    EXPECT_TME_FLAGS(hWnd3, 0);                     /* all flags are removed */
    COMPARE_CACHE(mouseleave3to2_dropmouse_chain);  /* we get no WM_MOUSELEAVE */
    ignore_mouse = FALSE;

    /* after WH_MOUSE drops WM_MOUSEMOVE, WM_MOUSELEAVE is still in the queue */
    FLUSH_MESSAGES(QS_POSTMESSAGE, QS_TIMER|QS_MOUSEMOVE);
    COMPARE_CACHE(mouseleave3_remainging_chain);

    FlushMessages();
    COMPARE_CACHE(empty_chain);

    destroy_test_window();
}

START_TEST(TrackMouseEvent)
{
    Test_TrackMouseEvent();
}
