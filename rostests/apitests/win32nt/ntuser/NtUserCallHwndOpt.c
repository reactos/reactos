/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for NtUserCallHwndOpt
 * PROGRAMMERS:
 */

#include <win32nt.h>

void
Test_HwndOptRoutine_SetProgmanWindow(void) /* 0x4a */
{
//	NtUserCallHwndOpt(hWnd, HWNDOPT_ROUTINE_SETPROGMANWINDOW);
}

void
Test_HwndOptRoutine_SetTaskmanWindow (void) /* 0x4b */
{
//	NtUserCallHwndOpt(hWnd, HWNDOPT_ROUTINE_SETTASKMANWINDOW);
}

START_TEST(NtUserCallHwndOpt)
{

	Test_HwndOptRoutine_SetProgmanWindow(pti);  /* 0x4a */
	Test_HwndOptRoutine_SetTaskmanWindow (pti); /* 0x4b */
}
