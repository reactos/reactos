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
// unconditional move - used to restore position to prev location
// when eg. menu disappears...
#define WM_EVENT_FORCEMOVE (WM_APP + 3)

/////////////////////////////////////////////////////////////////////////////


extern "C" MAGHOOKAPI BOOL WINAPI InstallEventHook (HWND hwndPostTo);
extern "C" MAGHOOKAPI DWORD_PTR WINAPI GetCursorHack();
extern "C" MAGHOOKAPI void WINAPI FakeCursorMove(POINT pt);
extern "C" MAGHOOKAPI void SetZoomRect(  SIZE sz );


// Macros and function prototypes for debugging
#ifdef DEBUG
  #define _DEBUG
#endif

#ifdef _DEBUG
  void FAR CDECL PrintIt(LPSTR strFmt, ...);
  #define DBPRINTF PrintIt
#else
  #define DBPRINTF        1 ? (void)0 : (void)
#endif
	
//////////////////////////////// End of File //////////////////////////////////
