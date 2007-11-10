

INT
Test_NtUserScrollDC(PTESTINFO pti)
{
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
	                     g_hInstance,
	                     0);
	ASSERT(hWnd);
	RedrawWindow(hWnd, &rect, NULL, RDW_UPDATENOW);

	hDC = GetDC(hWnd);
	ASSERT(hDC);

	hRgn = CreateRectRgn(0,0,10,10);

	rcScroll.left = 0;
	rcScroll.top = 25;
	rcScroll.right = 100;
	rcScroll.bottom = 40;

	rcClip.left = 0;
	rcClip.top = 35;
	rcClip.right = 70;
	rcClip.bottom = 1000;

	Result = NtUserScrollDC(hDC, 10, 20, &rcScroll, &rcClip, hRgn, &rcUpdate);

	TEST(Result == TRUE);
	TEST(rcUpdate.left == 0);
	TEST(rcUpdate.top == 35);
	TEST(rcUpdate.right == 70);
	TEST(rcUpdate.bottom == 55);

	hTmpRgn = CreateRectRgn(10,45,70,55);
	Result = CombineRgn(hRgn, hRgn, hTmpRgn, RGN_XOR);
	TEST(Result == SIMPLEREGION);

	SetRectRgn(hTmpRgn,0,35,70,40);
	Result = CombineRgn(hRgn, hRgn, hTmpRgn, RGN_XOR);
	TEST(Result == NULLREGION);

	DeleteObject(hTmpRgn);

	/* TODO: Test with another window in front */

	ReleaseDC(hWnd, hDC);
	DestroyWindow(hWnd);
	DeleteObject(hRgn);

	return APISTATUS_NORMAL;
}

