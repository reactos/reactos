/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserCallNoParam
 * PROGRAMMERS:
 */

#include <win32nt.h>

void
Test_NoParamRoutine_CreateMenu(void) /* 0 */
{
	HMENU hMenu;

	hMenu = (HMENU)NtUserCallNoParam(_NOPARAM_ROUTINE_CREATEMENU);
	TEST(IsMenu(hMenu) == TRUE);
	DestroyMenu(hMenu);

}

void
Test_NoParamRoutine_CreatePopupMenu(void) /* 1 */
{
	HMENU hMenu;

	hMenu = (HMENU)NtUserCallNoParam(_NOPARAM_ROUTINE_CREATEMENUPOPUP);
	TEST(IsMenu(hMenu) == TRUE);
	DestroyMenu(hMenu);

}

void
Test_NoParamRoutine_DisableProcessWindowsGhosting(void) /* 2 */
{

}

void
Test_NoParamRoutine_ClearWakeMask(void) /* 3 */
{

}

void
Test_NoParamRoutine_AllowForegroundActivation(void) /* 4 */
{

}

void
Test_NoParamRoutine_DestroyCaret(void) /* 5 */
{

}

void
Test_NoParamRoutine_LoadUserApiHook(void) /* 0x1d */
{
	//DWORD dwRet;
	/* dwRet = */NtUserCallNoParam(_NOPARAM_ROUTINE_LOADUSERAPIHOOK);

//	TEST(dwRet != 0);

}


START_TEST(NtUserCallNoParam)
{
	Test_NoParamRoutine_CreateMenu();
	Test_NoParamRoutine_CreatePopupMenu();
	Test_NoParamRoutine_LoadUserApiHook(); /* 0x1d */

	return APISTATUS_NORMAL;
}
