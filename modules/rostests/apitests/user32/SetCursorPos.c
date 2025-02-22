/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetCursorPos
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "precomp.h"

static HHOOK hMouseHookLL, hMouseHook;

struct _test_info
{
    int ll_hook_called;
    int hook_called;
    int mouse_move_called;
};

static struct _test_info info[] =
{
    {0,0,0}, /* SetCursorPos without a window */
    {1,1,0}, /* mouse_event without a window */
    {0,1,1}, /* SetCursorPos with a window */
    {1,1,1}, /* mouse_event with a window */
    {0,1,1}, /* multiple SetCursorPos with a window with coalescing */
    {0,2,2}, /* multiple SetCursorPos with a window without coalescing */
    {2,1,1}, /* multiple mouse_event with a window with coalescing */
    {2,2,2}, /* multiple mouse_event with a window without coalescing */
};

static struct _test_info results[8];
static int test_no = 0;


LRESULT CALLBACK MouseLLHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    results[test_no].ll_hook_called++;
    return CallNextHookEx(hMouseHookLL, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    results[test_no].hook_called++;
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

static LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    if(msg == WM_MOUSEMOVE)
        results[test_no].mouse_move_called++;

    return DefWindowProcA( hWnd, msg, wParam, lParam );
}

static HWND CreateTestWindow()
{
    MSG msg;
    WNDCLASSA  wclass;
    HANDLE hInstance = GetModuleHandleA( NULL );
    HWND hWndTest;

    wclass.lpszClassName = "MouseInputTestClass";
    wclass.style         = CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc   = WndProc;
    wclass.hInstance     = hInstance;
    wclass.hIcon         = LoadIconA( 0, IDI_APPLICATION );
    wclass.hCursor       = LoadCursorA( NULL, IDC_ARROW );
    wclass.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
    wclass.lpszMenuName = 0;
    wclass.cbClsExtra    = 0;
    wclass.cbWndExtra    = 0;
    RegisterClassA( &wclass );
    /* create the test window that will receive the keystrokes */
    hWndTest = CreateWindowA( wclass.lpszClassName, "MouseInputTestTest",
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 100, 100,
                              NULL, NULL, hInstance, NULL);
    assert( hWndTest );
    ShowWindow( hWndTest, SW_SHOWMAXIMIZED);
    SetWindowPos( hWndTest, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );
    SetForegroundWindow( hWndTest );
    UpdateWindow( hWndTest);
    SetFocus(hWndTest);

    /* flush pending messages */
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    return hWndTest;
}

void Test_SetCursorPos()
{
    HWND hwnd;
    MSG msg;
    int i;

    hMouseHookLL = SetWindowsHookEx(WH_MOUSE_LL, MouseLLHookProc, GetModuleHandleA( NULL ), 0);
    hMouseHook = SetWindowsHookExW(WH_MOUSE, MouseHookProc, GetModuleHandleW( NULL ), GetCurrentThreadId());
    ok(hMouseHook!=NULL,"failed to set hook\n");
    ok(hMouseHookLL!=NULL,"failed to set hook\n");

    test_no = 0;
    SetCursorPos(1,1);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    test_no = 1;
    mouse_event(MOUSEEVENTF_MOVE, 2,2, 0,0);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    hwnd = CreateTestWindow();
    SetCapture(hwnd);

    test_no = 2;
    SetCursorPos(50,50);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    test_no = 3;
    mouse_event(MOUSEEVENTF_MOVE, 100,100, 0,0);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    test_no = 4;
    SetCursorPos(50,50);
    SetCursorPos(60,60);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    test_no = 5;
    SetCursorPos(50,50);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
    SetCursorPos(60,60);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    test_no = 6;
    mouse_event(MOUSEEVENTF_MOVE, 50,50, 0,0);
    mouse_event(MOUSEEVENTF_MOVE, 60,60, 0,0);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    test_no = 7;
    mouse_event(MOUSEEVENTF_MOVE, 50,50, 0,0);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );
    mouse_event(MOUSEEVENTF_MOVE, 60,60, 0,0);
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

    for(i = 0; i< 8; i++)
    {
#define TEST(s,x,y) ok(y == x, "%d: %s called %d times instead of %d\n",i,s, y,x);
        TEST("WH_MOUSE_LL", info[i].ll_hook_called, results[i].ll_hook_called);
        /* WH_MOUSE results vary greatly among windows versions */
        //TEST("WH_MOUSE", info[i].hook_called, results[i].hook_called);
        //TEST("WM_MOUSEMOVE", info[i].mouse_move_called, results[i].mouse_move_called);
    }

    SetCapture(NULL);
    DestroyWindow(hwnd);

    UnhookWindowsHookEx (hMouseHook);
    UnhookWindowsHookEx (hMouseHookLL);

}

void Test_DesktopAccess()
{
    HDESK hDesk, hDeskInitial;
    POINT curPoint, initialPoint;
    BOOL ret;

    hDeskInitial = GetThreadDesktop(GetCurrentThreadId());
    ok(hDeskInitial != NULL, "Failed to retrieve the initial desktop\n");

    ret = GetCursorPos(&initialPoint);
    ok(ret == TRUE, "GetCursorPos should succed\n");

    hDesk = CreateDesktopW(L"testDesktop", NULL, NULL, 0, 0x01ff, NULL);
    ok(hDesk != 0, "Failed to create a new desktop\n");
    SetThreadDesktop(hDesk);
    ok(GetThreadDesktop(GetCurrentThreadId()) == hDesk, "SetThreadDesktop had no effect\n");

    SetLastError(0xdeadbeef);

    ret = GetCursorPos(&curPoint);
    ok(ret == FALSE, "GetCursorPos should fail\n");

    ok(GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == 0xdeadbeef,
       "Expected ERROR_ACCESS_DENIED or 0xdeadbeef, got 0x%lx\n", GetLastError());
    SetLastError(0xdeadbeef);

    ret = SetCursorPos(2,2);
    //ok(ret == FALSE, "SetCursorPos should fail\n"); // FIXME: fails on WHS testbot

    ok(GetLastError() == 0xdeadbeef, "Wrong last error, got 0x%lx\n", GetLastError());

    ret = GetCursorPos(&curPoint);
    ok(ret == FALSE, "GetCursorPos should fail\n");

    SetThreadDesktop(hDeskInitial);

    ret = GetCursorPos(&curPoint);
    ok(ret == TRUE, "GetCursorPos should succed\n");
    //ok(curPoint.x ==  initialPoint.x && curPoint.y ==  initialPoint.y, "Mouse position changed\n");
}

START_TEST(SetCursorPos)
{
    Test_DesktopAccess();
    Test_SetCursorPos();
}
