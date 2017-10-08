/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserRedrawWindow
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtUserRedrawWindow)
{
    HINSTANCE hinst = GetModuleHandle(NULL);
	HWND hWnd;
	RECT rect;

	hWnd = CreateWindowA("BUTTON",
	                     "Test",
	                     BS_PUSHBUTTON | WS_VISIBLE,
	                     0,
	                     0,
	                     50,
	                     30,
	                     NULL,
	                     NULL,
	                     hinst,
	                     0);
	ASSERT(hWnd);

	rect.left = 0;
	rect.top = 0;
	rect.right = 10;
	rect.bottom = 10;

	TEST(NtUserRedrawWindow(hWnd, &rect, NULL, RDW_VALIDATE) == TRUE);

	DestroyWindow(hWnd);

}
