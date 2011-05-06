
INT
Test_NoParamRoutine_CreateMenu(PTESTINFO pti) /* 0 */
{
	HMENU hMenu;

	hMenu = (HMENU)NtUserCallNoParam(_NOPARAM_ROUTINE_CREATEMENU);
	TEST(IsMenu(hMenu) == TRUE);
	DestroyMenu(hMenu);

	return APISTATUS_NORMAL;
}

INT
Test_NoParamRoutine_CreatePopupMenu(PTESTINFO pti) /* 1 */
{
	HMENU hMenu;

	hMenu = (HMENU)NtUserCallNoParam(_NOPARAM_ROUTINE_CREATEMENUPOPUP);
	TEST(IsMenu(hMenu) == TRUE);
	DestroyMenu(hMenu);

	return APISTATUS_NORMAL;
}

INT
Test_NoParamRoutine_DisableProcessWindowsGhosting(PTESTINFO pti) /* 2 */
{
	return APISTATUS_NORMAL;
}

INT
Test_NoParamRoutine_ClearWakeMask(PTESTINFO pti) /* 3 */
{
	return APISTATUS_NORMAL;
}

INT
Test_NoParamRoutine_AllowForegroundActivation(PTESTINFO pti) /* 4 */
{
	return APISTATUS_NORMAL;
}

INT
Test_NoParamRoutine_DestroyCaret(PTESTINFO pti) /* 5 */
{
	return APISTATUS_NORMAL;
}

INT
Test_NoParamRoutine_LoadUserApiHook(PTESTINFO pti) /* 0x1d */
{
	DWORD dwRet;
	dwRet = NtUserCallNoParam(_NOPARAM_ROUTINE_LOADUSERAPIHOOK);

//	TEST(dwRet != 0);
	return APISTATUS_NORMAL;
}


INT
Test_NtUserCallNoParam(PTESTINFO pti)
{
	Test_NoParamRoutine_CreateMenu(pti);
	Test_NoParamRoutine_CreatePopupMenu(pti);
	Test_NoParamRoutine_LoadUserApiHook(pti); /* 0x1d */

	return APISTATUS_NORMAL;
}
