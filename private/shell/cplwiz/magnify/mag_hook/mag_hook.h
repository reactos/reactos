/******************************************************************************
Module name: Mag_Hook.h
Written by:  Jeffrey Richter
Purpose:     Magnify Hook DLL Header file for exported functions and symbols.
******************************************************************************/


#ifndef MAGHOOKAPI 
#define MAGHOOKAPI  __declspec(dllimport)
#endif


/////////////////////////////////////////////////////////////////////////////


#define WM_EVENT_CARETMOVE (WM_APP + 0)
#define WM_EVENT_FOCUSMOVE (WM_APP + 1)
#define WM_EVENT_MOUSEMOVE (WM_APP + 2)


/////////////////////////////////////////////////////////////////////////////


extern "C" MAGHOOKAPI BOOL WINAPI InstallEventHook (HWND hwndPostTo);
extern "C" MAGHOOKAPI DWORD WINAPI GetCursorHack();
extern "C" MAGHOOKAPI void WINAPI FakeCursorMove(POINT pt);

	
//////////////////////////////// End of File //////////////////////////////////
