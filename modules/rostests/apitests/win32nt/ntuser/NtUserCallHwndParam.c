/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserCallHwndParam
 * PROGRAMMERS:
 */

#include <win32nt.h>

void
Test_HwndParamRoutine_SetWindowContextHelpId(HWND hWnd)
{
	TEST(NtUserCallHwndParam(hWnd, 12345, _HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID) == TRUE);
	TEST(NtUserCallHwnd(hWnd, HWND_ROUTINE_GETWNDCONTEXTHLPID) == 12345);
}

START_TEST(NtUserCallHwndParam)
{
    HWND hWnd;

	hWnd = CreateWindowA("BUTTON",
	                     "Test",
	                     BS_PUSHBUTTON | WS_VISIBLE,
	                     0,
	                     0,
	                     50,
	                     30,
	                     NULL,
	                     NULL,
	                     g_hInstance,
	                     0);
	ASSERT(hWnd);

	Test_HwndParamRoutine_SetWindowContextHelpId(hWnd);

	DestroyWindow(hWnd);
}
