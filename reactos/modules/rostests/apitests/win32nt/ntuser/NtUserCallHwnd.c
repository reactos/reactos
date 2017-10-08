/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserCallHwnd
 * PROGRAMMERS:
 */

#include <win32nt.h>

void
Test_HwndRoutine_DeregisterShellHookWindow(HWND hWnd)
{
	TEST(NtUserCallHwnd(hWnd, _HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW) == TRUE);

}

void
Test_HwndRoutine_GetWindowContextHelpId (HWND hWnd)
{
	TEST(NtUserCallHwndParam(hWnd, 0xbadb00b, _HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID) == TRUE);
	TEST(NtUserCallHwnd(hWnd, _HWND_ROUTINE_GETWNDCONTEXTHLPID) == 0xbadb00b);

}

void
Test_HwndRoutine_SetMsgBox(HWND hWnd)
{
	TEST(NtUserCallHwnd(hWnd, 0x49) != FALSE);

}


START_TEST(NtUserCallHwnd)
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

	SetLastError(ERROR_SUCCESS);
	TEST(NtUserCallHwnd(hWnd, 0x44) == FALSE);
	Test_HwndRoutine_DeregisterShellHookWindow(hWnd); /* 0x45 */
	TEST(NtUserCallHwnd(hWnd, 0x46) == FALSE); // DWP_GetEnabledPopup
	Test_HwndRoutine_GetWindowContextHelpId (hWnd); /* 0x47 */
	TEST(NtUserCallHwnd(hWnd, 0x48) == TRUE);
	Test_HwndRoutine_SetMsgBox(hWnd); /* 0x49 */
	TEST(GetLastError() == ERROR_SUCCESS);

	DestroyWindow(hWnd);
}
