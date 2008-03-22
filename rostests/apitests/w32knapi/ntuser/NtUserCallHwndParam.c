
INT
Test_HwndParamRoutine_SetWindowContextHelpId (PTESTINFO pti, HWND hWnd)
{
	TEST(NtUserCallHwndParam(hWnd, 12345, _HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID) == TRUE);
	TEST(NtUserCallHwnd(hWnd, _HWND_ROUTINE_GETWNDCONTEXTHLPID) == 12345);
	return APISTATUS_NORMAL;
}


INT
Test_NtUserCallHwndParam(PTESTINFO pti)
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

	Test_HwndParamRoutine_SetWindowContextHelpId (pti, hWnd);

	DestroyWindow(hWnd);

    return APISTATUS_NORMAL;
}
