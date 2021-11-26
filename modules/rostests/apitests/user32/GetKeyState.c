/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for GetKeyState
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include "precomp.h"

HHOOK hKbdHook, hKbdLLHook;

LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam,  LPARAM lParam)
{
    BOOL pressed = !(lParam & (1<<31));
	BOOL altPressed = lParam & (1<<29);

    if(pressed)
	{
	    ok(altPressed,"\n");
	    ok((GetKeyState(VK_MENU) & 0x8000), "Alt should not be pressed\n");\
		ok((GetKeyState(VK_LMENU) & 0x8000), "Left alt should not be pressed\n");\
	}
	else
	{
	    ok(!altPressed,"\n");
	    ok(!(GetKeyState(VK_MENU) & 0x8000), "Alt should be pressed\n");
	    ok(!(GetKeyState(VK_LMENU) & 0x8000), "Left alt should be pressed\n");
	}

    return CallNextHookEx(hKbdHook, code, wParam, lParam);
}

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    PKBDLLHOOKSTRUCT pLLHook = (PKBDLLHOOKSTRUCT)lParam;

    if(wParam == WM_SYSKEYDOWN)
	{
	    ok(pLLHook->flags & LLKHF_ALTDOWN,"Didn't get LLKHF_ALTDOWN flag\n");
		ok((GetAsyncKeyState (VK_MENU) & 0x8000), "Alt should not be pressed in global kbd status\n");
	    ok(!(GetKeyState(VK_MENU) & 0x8000), "Alt should not be pressed in queue state\n");
		ok(!(GetAsyncKeyState (VK_LMENU) & 0x8000), "Left alt should not be pressed in global kbd status\n");
	    ok(!(GetKeyState(VK_LMENU) & 0x8000), "Left alt should not be pressed in queue state\n");
	}
	else if(wParam == WM_SYSKEYUP)
	{
	    ok(!(pLLHook->flags & LLKHF_ALTDOWN),"got LLKHF_ALTDOWN flag\n");
		ok(!(GetAsyncKeyState (VK_MENU) & 0x8000), "Alt should not be pressed in global kbd status\n");
	    ok((GetKeyState(VK_MENU) & 0x8000), "Alt should be pressed in queue state\n");
		ok(!(GetAsyncKeyState (VK_LMENU) & 0x8000), "Left alt should not be pressed in global kbd status\n");
	    ok((GetKeyState(VK_LMENU) & 0x8000), "Left alt should be pressed in queue state\n");
	}

	return CallNextHookEx(hKbdLLHook, nCode, wParam, lParam);
}

static LRESULT CALLBACK WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    if(msg == WM_SYSKEYDOWN)
	{
	    ok(wParam == VK_MENU, "Got wrong wParam in WM_SYSKEYDOWN (%d instead of %d)\n", wParam, VK_MENU );
	}
    return DefWindowProcA( hWnd, msg, wParam, lParam );
}

static HWND CreateTestWindow()
{
    MSG msg;
    WNDCLASSA  wclass;
    HANDLE hInstance = GetModuleHandleA( NULL );
    HWND hWndTest;

    wclass.lpszClassName = "InputSysKeyTestClass";
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
    hWndTest = CreateWindowA( wclass.lpszClassName, "InputSysKeyTest",
                              WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, 100, 100,
                              NULL, NULL, hInstance, NULL);
    assert( hWndTest );
    ShowWindow( hWndTest, SW_SHOW);
    SetWindowPos( hWndTest, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE );
    SetForegroundWindow( hWndTest );
    UpdateWindow( hWndTest);

    /* flush pending messages */
    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

	return hWndTest;
}

void Test_GetKeyState()
{
	HWND hwnd;
    MSG msg;

	hwnd = CreateTestWindow();

	hKbdHook = SetWindowsHookEx(WH_KEYBOARD, KeyboardProc, GetModuleHandleA( NULL ), 0);
	hKbdLLHook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandleA( NULL ), 0);

	ok(hKbdHook != NULL, "\n");
	ok(hKbdLLHook != NULL, "\n");

	keybd_event(VK_LMENU, 0, 0,0);

	while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

	keybd_event(VK_LMENU, 0, KEYEVENTF_KEYUP,0);

	//fixme this hangs the test
    //while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE|PM_NOYIELD )) DispatchMessageA( &msg );

	DestroyWindow(hwnd);

    while (PeekMessage( &msg, 0, 0, 0, PM_REMOVE )) DispatchMessageA( &msg );

	UnhookWindowsHookEx (hKbdHook);
	UnhookWindowsHookEx (hKbdLLHook);
}

START_TEST(GetKeyState)
{
    Test_GetKeyState();
}
