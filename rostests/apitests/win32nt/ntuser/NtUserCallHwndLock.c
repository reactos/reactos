/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserCallHwndLock
 * PROGRAMMERS:
 */

#include <win32nt.h>




HMENU g_hMenu;

void
Test_HwndLockRoutine_WindowHasShadow(HWND hWnd) /* 0x53 */
{
//	TEST(NtUserCallHwndLock(hWnd, 0x53) == 0);

}


void
Test_HwndLockRoutine_ArrangeIconicWindows(HWND hWnd) /* 0x54 */
{
//	TEST(NtUserCallHwndLock(hWnd, _HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS) == 0);

}

void
Test_HwndLockRoutine_DrawMenuBar(HWND hWnd) /* 0x55 */
{
	TEST(NtUserCallHwndLock(hWnd, 0x55) == 1);
	return APISTATUS_NORMAL;
}

void
Test_HwndLockRoutine_CheckImeShowStatusInThread(HWND hWnd) /* 0x56 */
{
	TEST(NtUserCallHwndLock(hWnd, 0x56) != 0);

}

void
Test_HwndLockRoutine_GetSysMenuHandle(HWND hWnd) /* 0x57 */
{
	NtUserCallHwndLock(hWnd, 0x5c);
//	HMENU hMenu = (HMENU)NtUserCallHwndLock(hWnd, 0x57);
//	TEST(hMenu != 0);

}

void
Test_HwndLockRoutine_RedrawFrame(HWND hWnd) /* 0x58 */
{
//	TEST(NtUserCallHwndLock(hWnd, 0x68) != 0);

}

void
Test_HwndLockRoutine_UpdateWindow(HWND hWnd) /* 0x5e */
{
	TEST(NtUserCallHwndLock(hWnd, 0x5e) == 1);

}

START_TEST(NtUserCallHwndLock)
{
	HWND hWnd;
	g_hMenu = CreateMenu();

	hWnd = CreateWindowA("BUTTON",
	                     "Test",
	                     BS_PUSHBUTTON | WS_VISIBLE,
	                     0,
	                     0,
	                     50,
	                     30,
	                     NULL,
	                     g_hMenu,
	                     g_hInstance,
	                     0);
	ASSERT(hWnd);

	SetLastError(ERROR_SUCCESS);
	TEST(NtUserCallHwndLock((HWND)-22, 999) == 0);
	TEST(GetLastError() == ERROR_INVALID_WINDOW_HANDLE);

	SetLastError(ERROR_SUCCESS);
	TEST(NtUserCallHwndLock((HWND)0, 0x55) == 0);
	TEST(GetLastError() == ERROR_INVALID_WINDOW_HANDLE);

	SetLastError(ERROR_SUCCESS);
	TEST(NtUserCallHwndLock(hWnd, 999) == 0);
	TEST(GetLastError() == ERROR_SUCCESS);

	Test_HwndLockRoutine_WindowHasShadow (hWnd); /* 0x53 */
	Test_HwndLockRoutine_ArrangeIconicWindows(hWnd);
	Test_HwndLockRoutine_DrawMenuBar(hWnd);
	Test_HwndLockRoutine_CheckImeShowStatusInThread(hWnd);
	Test_HwndLockRoutine_GetSysMenuHandle(hWnd);
	Test_HwndLockRoutine_RedrawFrame(hWnd);

	Test_HwndLockRoutine_UpdateWindow(hWnd);

	DestroyWindow(hWnd);

}
