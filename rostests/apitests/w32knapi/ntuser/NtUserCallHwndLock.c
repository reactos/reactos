

HMENU g_hMenu;

INT
Test_HwndLockRoutine_WindowHasShadow (PTESTINFO pti, HWND hWnd) /* 0x53 */
{
//	TEST(NtUserCallHwndLock(hWnd, 0x53) == 0);
	return APISTATUS_NORMAL;
}


INT
Test_HwndLockRoutine_ArrangeIconicWindows(PTESTINFO pti, HWND hWnd) /* 0x54 */
{
//	TEST(NtUserCallHwndLock(hWnd, _HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS) == 0);
	return APISTATUS_NORMAL;
}

INT
Test_HwndLockRoutine_DrawMenuBar(PTESTINFO pti, HWND hWnd) /* 0x55 */
{
	TEST(NtUserCallHwndLock(hWnd, 0x55) == 1);
	return APISTATUS_NORMAL;
}

INT
Test_HwndLockRoutine_CheckImeShowStatusInThread(PTESTINFO pti, HWND hWnd) /* 0x56 */
{
	TEST(NtUserCallHwndLock(hWnd, 0x56) != 0);
	return APISTATUS_NORMAL;
}

INT
Test_HwndLockRoutine_GetSysMenuHandle(PTESTINFO pti, HWND hWnd) /* 0x57 */
{
	NtUserCallHwndLock(hWnd, 0x5c);
//	HMENU hMenu = (HMENU)NtUserCallHwndLock(hWnd, 0x57);
//	TEST(hMenu != 0);
	return APISTATUS_NORMAL;
}

INT
Test_HwndLockRoutine_RedrawFrame(PTESTINFO pti, HWND hWnd) /* 0x58 */
{
//	TEST(NtUserCallHwndLock(hWnd, 0x68) != 0);
	return APISTATUS_NORMAL;
}

INT
Test_HwndLockRoutine_UpdateWindow(PTESTINFO pti, HWND hWnd) /* 0x5e */
{
	TEST(NtUserCallHwndLock(hWnd, 0x5e) == 1);
	return APISTATUS_NORMAL;
}

INT
Test_NtUserCallHwndLock(PTESTINFO pti)
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

	Test_HwndLockRoutine_WindowHasShadow (pti, hWnd); /* 0x53 */
	Test_HwndLockRoutine_ArrangeIconicWindows(pti, hWnd);
	Test_HwndLockRoutine_DrawMenuBar(pti, hWnd);
	Test_HwndLockRoutine_CheckImeShowStatusInThread(pti, hWnd);
	Test_HwndLockRoutine_GetSysMenuHandle(pti, hWnd);
	Test_HwndLockRoutine_RedrawFrame(pti, hWnd);

	Test_HwndLockRoutine_UpdateWindow(pti, hWnd);

	DestroyWindow(hWnd);

    return APISTATUS_NORMAL;
}
