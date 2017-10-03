/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserScrollDC
 * PROGRAMMERS:
 */

#include <win32nt.h>

START_TEST(NtUserScrollDC)
{
    HINSTANCE hinst = GetModuleHandle(NULL);
	HWND hWnd;
	HDC hDC;
	HRGN hRgn, hTmpRgn;
	RECT rcScroll, rcClip, rcUpdate;
	RECT rect = {0,0,100,100};
	INT Result;

	hWnd = CreateWindowA("BUTTON",
	                     "Test",
	                     BS_PUSHBUTTON | WS_VISIBLE,
	                     0,
	                     0,
	                     50,
	                     100,
	                     NULL,
	                     NULL,
	                     hinst,
	                     0);
	ASSERT(hWnd);
	RedrawWindow(hWnd, &rect, NULL, RDW_UPDATENOW);

	hDC = GetDC(hWnd);
	ASSERT(hDC);

	hRgn = CreateRectRgn(0,0,10,10);


	/* Test inverted clip rect */
	rcScroll.left = 0;
	rcScroll.top = 25;
	rcScroll.right = 100;
	rcScroll.bottom = 40;
	rcClip.left = 0;
	rcClip.top = 35;
	rcClip.right = -70;
	rcClip.bottom = -1000;
	SetLastError(ERROR_SUCCESS);
	Result = NtUserScrollDC(hDC, 10, 20, &rcScroll, &rcClip, hRgn, &rcUpdate);
	RTEST(Result == 1);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Test inverted scroll rect */
	rcScroll.left = 0;
	rcScroll.top = 25;
	rcScroll.right = -100;
	rcScroll.bottom = -40;
	rcClip.left = 0;
	rcClip.top = 35;
	rcClip.right = 70;
	rcClip.bottom = 1000;
	SetLastError(ERROR_SUCCESS);
	Result = NtUserScrollDC(hDC, 10, 20, &rcScroll, &rcClip, hRgn, &rcUpdate);
	RTEST(Result == 1);
	RTEST(GetLastError() == ERROR_SUCCESS);

	rcScroll.left = 0;
	rcScroll.top = 25;
	rcScroll.right = 100;
	rcScroll.bottom = 40;

	/* Test invalid update region */
	SetLastError(ERROR_SUCCESS);
	Result = NtUserScrollDC(hDC, 10, 20, &rcScroll, &rcClip, (HRGN)0x123456, &rcUpdate);
	RTEST(Result == 0);
	TEST(GetLastError() == ERROR_INVALID_HANDLE);

	/* Test invalid dc */
	SetLastError(ERROR_SUCCESS);
	Result = NtUserScrollDC((HDC)0x123456, 10, 20, &rcScroll, &rcClip, hRgn, &rcUpdate);
	RTEST(Result == 0);
	RTEST(GetLastError() == ERROR_SUCCESS);

	/* Test invalid update rect */
	SetLastError(ERROR_SUCCESS);
	Result = NtUserScrollDC(hDC, 10, 20, &rcScroll, &rcClip, hRgn, (PVOID)0x80001000);
	RTEST(Result == 0);
	RTEST(GetLastError() == ERROR_NOACCESS);

	Result = NtUserScrollDC(hDC, 10, 20, &rcScroll, &rcClip, hRgn, &rcUpdate);

	RTEST(Result == TRUE);
	RTEST(rcUpdate.left == 0);
	RTEST(rcUpdate.top == 35);
	RTEST(rcUpdate.right == 70);
	RTEST(rcUpdate.bottom == 55);

	hTmpRgn = CreateRectRgn(10,45,70,55);
	Result = CombineRgn(hRgn, hRgn, hTmpRgn, RGN_XOR);
	RTEST(Result == SIMPLEREGION);

	SetRectRgn(hTmpRgn,0,35,70,40);
	Result = CombineRgn(hRgn, hRgn, hTmpRgn, RGN_XOR);
	RTEST(Result == NULLREGION);

	DeleteObject(hTmpRgn);

	/* TODO: Test with another window in front */
	/* TODO: Test with different viewport extension */

	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);
	DeleteObject(hRgn);

}

