#include <windows.h>
#include <user32/win.h>

WINBOOL IsWindow(HANDLE hWnd)
{
	return TRUE;
}

WND * WIN_FindWndPtr( HWND hwnd )
{
	return NULL;
}

WND*   WIN_GetDesktop(void)
{
	return NULL;
}

WND **WIN_BuildWinArray( WND *wndPtr, UINT bwaFlags, UINT* pTotal )
{
	return NULL;
}