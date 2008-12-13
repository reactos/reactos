
INT
Test_OneParamRoutine_BeginDeferWindowPos(PTESTINFO pti) /* 0x1e */
{
	HDWP hWinPosInfo;

	hWinPosInfo = (HDWP)NtUserCallOneParam(5, 0x1e);
	TEST(hWinPosInfo != 0);
	TEST(EndDeferWindowPos(hWinPosInfo) != 0);

	return APISTATUS_NORMAL;
}

INT
Test_OneParamRoutine_WindowFromDC(PTESTINFO pti) /* 0x1f */
{
	HDC hDC = GetDC(NULL);
	HWND hWnd;

	hWnd = (HWND)NtUserCallOneParam((DWORD)hDC, 0x1f);
	TEST(hWnd != 0);
	TEST(IsWindow(hWnd));
	TEST(hWnd == GetDesktopWindow());

	return APISTATUS_NORMAL;
}

INT
Test_OneParamRoutine_MapDesktopObject(PTESTINFO pti) /* 0x30 */
{
	DWORD pObject;
	HWND hWnd;
	HMENU hMenu;

	hWnd = GetDesktopWindow();
	pObject = NtUserCallOneParam((DWORD)hWnd, _ONEPARAM_ROUTINE_MAPDEKTOPOBJECT);
	TEST(pObject > 0);
	TEST(pObject < 0x80000000);

	hMenu = CreateMenu();
	pObject = NtUserCallOneParam((DWORD)hMenu, _ONEPARAM_ROUTINE_MAPDEKTOPOBJECT);
	DestroyMenu(hMenu); 
	TEST(pObject > 0);
	TEST(pObject < 0x80000000);

	return APISTATUS_NORMAL;
}

INT
Test_OneParamRoutine_SwapMouseButtons(PTESTINFO pti) /* 0x42 */
{
	BOOL bInverse;

	NtUserCallOneParam(TRUE, _ONEPARAM_ROUTINE_SWAPMOUSEBUTTON);
	bInverse = (BOOL)NtUserCallOneParam(FALSE, _ONEPARAM_ROUTINE_SWAPMOUSEBUTTON);
	TEST(bInverse == TRUE);
	bInverse = (BOOL)NtUserCallOneParam(FALSE, _ONEPARAM_ROUTINE_SWAPMOUSEBUTTON);
	TEST(bInverse == FALSE);

	// TODO: test other values
	return APISTATUS_NORMAL;
}

INT
Test_NtUserCallOneParam(PTESTINFO pti)
{
	Test_OneParamRoutine_BeginDeferWindowPos(pti); /* 0x1e */
	Test_OneParamRoutine_WindowFromDC(pti); /* 0x1f */
	Test_OneParamRoutine_MapDesktopObject(pti); /* 0x30 */

	Test_OneParamRoutine_SwapMouseButtons(pti); /* 0x42 */

	return APISTATUS_NORMAL;
}
