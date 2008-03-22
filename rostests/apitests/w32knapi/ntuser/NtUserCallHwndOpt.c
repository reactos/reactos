

INT
Test_HwndOptRoutine_SetProgmanWindow(PTESTINFO pti) /* 0x4a */
{
//	NtUserCallHwndOpt(hWnd, HWNDOPT_ROUTINE_SETPROGMANWINDOW);
	return APISTATUS_NORMAL;
}

INT
Test_HwndOptRoutine_SetTaskmanWindow (PTESTINFO pti) /* 0x4b */
{
//	NtUserCallHwndOpt(hWnd, HWNDOPT_ROUTINE_SETTASKMANWINDOW);
	return APISTATUS_NORMAL;
}

INT
Test_NtUserCallHwndOpt(PTESTINFO pti)
{

	Test_HwndOptRoutine_SetProgmanWindow(pti);  /* 0x4a */
	Test_HwndOptRoutine_SetTaskmanWindow (pti); /* 0x4b */

    return APISTATUS_NORMAL;
}
