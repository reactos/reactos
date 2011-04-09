/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for SetCursorPos
 * PROGRAMMERS:     Giannis Adamopoulos
 */

#include <stdio.h>
#include <wine/test.h>
#include <windows.h>
#include <assert.h>

HHOOK hMouseHook;
int hook_calls = 0;

LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    hook_calls++;
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

void Test_SetCursorPos()
{
	hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, GetModuleHandleA( NULL ), 0);
	ok(hMouseHook!=NULL,"failed to set hook\n");
	
	/* try SetCursorPos */
	SetCursorPos(0,0);
	ok (hook_calls == 0, "hooks shouldn't be called\n");
		
	/* try mouse_event with MOUSEEVENTF_MOVE*/
	mouse_event(MOUSEEVENTF_MOVE, 100,100, 0,0);
    ok (hook_calls == 1, "hooks should have been called\n");
	
	UnhookWindowsHookEx (hMouseHook);	
}

START_TEST(SetCursorPos)
{
    Test_SetCursorPos();
}