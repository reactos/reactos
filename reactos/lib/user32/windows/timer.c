#include <windows.h>

UINT STDCALL SetTimer( HWND  hWnd,UINT  nIDEvent,
    UINT  uElapse, TIMERPROC  lpTimerFunc 	
   )
{
	return 0;
}

WINBOOL STDCALL KillTimer(HWND  hWnd, UINT  uIDEvent )
{
	return FALSE;
}