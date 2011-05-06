
INT
Test_HwndRoutine_DeregisterShellHookWindow(PTESTINFO pti, HWND hWnd)
{
	TEST(NtUserCallHwnd(hWnd, _HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW) == TRUE);
	return APISTATUS_NORMAL;
}

INT
Test_HwndRoutine_GetWindowContextHelpId (PTESTINFO pti, HWND hWnd)
{
	TEST(NtUserCallHwndParam(hWnd, 0xbadb00b, _HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID) == TRUE);
	TEST(NtUserCallHwnd(hWnd, _HWND_ROUTINE_GETWNDCONTEXTHLPID) == 0xbadb00b);
	return APISTATUS_NORMAL;
}

INT
Test_HwndRoutine_SetMsgBox(PTESTINFO pti, HWND hWnd)
{
	TEST(NtUserCallHwnd(hWnd, 0x49) != FALSE);
	return APISTATUS_NORMAL;
}



INT
Test_NtUserCallHwnd(PTESTINFO pti)
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
	Test_HwndRoutine_DeregisterShellHookWindow(pti, hWnd); /* 0x45 */
	TEST(NtUserCallHwnd(hWnd, 0x46) == FALSE); // DWP_GetEnabledPopup
	Test_HwndRoutine_GetWindowContextHelpId (pti, hWnd); /* 0x47 */
	TEST(NtUserCallHwnd(hWnd, 0x48) == TRUE);
	Test_HwndRoutine_SetMsgBox(pti, hWnd); /* 0x49 */
	TEST(GetLastError() == ERROR_SUCCESS);

	DestroyWindow(hWnd);

	return APISTATUS_NORMAL;
}
