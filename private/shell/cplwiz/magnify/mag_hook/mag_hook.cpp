// --------------------------------------------------------------------------
//
//  Mag_Hook.cpp
//
//  Accessibility Event trapper for ScreenX. Uses an in-context WinEvent
//  hook to tell the applet where to magnify.
//
//  Mainly what we want this to do is to watch for focus changes, caret
//  movement, and mouse pointer movement, and then post the appropriate
//  location to the ScreenX applet so it can magnify the correct area.
//
// --------------------------------------------------------------------------
#define STRICT

#include <windows.h>
#include <windowsx.h>

// When building with VC5, we need winable.h since the active
// accessibility structures are not in VC5's winuser.h.  winable.h can
// be found in the active accessibility SDK
#ifdef VC5_BUILD___NOT_NT_BUILD_ENVIRONMENT
#include <winable.h>
#else
// The Active Accessibility SDK used WINABLEAPI for the functions.  When
// the functions were moved to winuser.h, WINABLEAPI was replaced with WINUSERAPI.
#define WINABLEAPI WINUSERAPI
#endif

#include <ole2.h>
#include <oleacc.h>
#define MAGHOOKAPI  __declspec(dllexport)
#include "Mag_Hook.h"


// --------------------------------------------------------------------------
//
// Definitions so we don't have to statically link to OLEACC.DLL
// 
// We need the following three functions that were in the Active Accessibility SDK
//
// STDAPI AccessibleObjectFromEvent(HWND hwnd, DWORD dwId, DWORD dwChildId, IAccessible** ppacc, VARIANT* pvarChild);
// WINABLEAPI HWINEVENTHOOK WINAPI SetWinEventHook(DWORD eventMin, DWORD eventMax, HMODULE hmodWinEventProc, WINEVENTPROC lpfnWinEventProc, DWORD idProcess, DWORD idThread, DWORD dwFlags);
// WINABLEAPI BOOL WINAPI UnhookWinEvent(HWINEVENTHOOK hEvent);
//
// --------------------------------------------------------------------------
typedef HRESULT (_stdcall *_tagAccessibleObjectFromEvent)(HWND hwnd, DWORD dwId, DWORD dwChildId, IAccessible** ppacc, VARIANT* pvarChild);
typedef WINABLEAPI HWINEVENTHOOK (WINAPI *_tagSetWinEventHook)(DWORD eventMin, DWORD eventMax, HMODULE hmodWinEventProc, WINEVENTPROC lpfnWinEventProc, DWORD idProcess, DWORD idThread, DWORD dwFlags);
typedef WINABLEAPI BOOL (WINAPI *_tagUnhookWinEvent)(HWINEVENTHOOK hEvent);

_tagAccessibleObjectFromEvent pAccessibleObjectFromEvent = NULL;
_tagSetWinEventHook pSetWinEventHook = NULL;
_tagUnhookWinEvent pUnhookWinEvent = NULL;


// --------------------------------------------------------------------------
//
//  GetAcctiveAccessibleFunctions()
//
//  This function attempts to load the active accessibility functions we need
//  from OLEACC.DLL and USER32.DLL
//
//  If the functions are availible, this returns TRUE
//
// --------------------------------------------------------------------------
BOOL GetAcctiveAccessibleFunctions()
{
	// JMC: HACK: Until OLEACC Works correctly
	return FALSE;

	HMODULE hOleAcc = NULL;
	HMODULE hUser;
	if(!(hOleAcc = LoadLibrary(__TEXT("oleacc.dll"))))
		return FALSE;
	if(!(pAccessibleObjectFromEvent = (_tagAccessibleObjectFromEvent)GetProcAddress(hOleAcc, "AccessibleObjectFromEvent")))
		return FALSE;
	if(!(hUser = GetModuleHandle(__TEXT("user32.dll"))))
		return FALSE;
	if(!(pSetWinEventHook = (_tagSetWinEventHook)GetProcAddress(hUser, "SetWinEventHook")))
		return FALSE;
	if(!(pUnhookWinEvent = (_tagUnhookWinEvent)GetProcAddress(hUser, "UnhookWinEvent")))
		return FALSE;
	return TRUE;
};


// --------------------------------------------------------------------------
//
// Per-process Variables
//
// --------------------------------------------------------------------------
HMODULE     g_hModEventDll;


// --------------------------------------------------------------------------
//
// Shared Variables
//
// --------------------------------------------------------------------------
#pragma data_seg("Shared")
HWINEVENTHOOK	g_hEventHook = NULL;
HHOOK			g_hMouseHook = NULL;
HWND			g_hwndEventPost = NULL;
DWORD			g_dwCursorHack = 0;
#pragma data_seg()
#pragma comment(linker, "/Section:Shared,RWS")


// --------------------------------------------------------------------------
//
// Functions Prototypes (Foward References of callback functions
//
// --------------------------------------------------------------------------
void CALLBACK NotifyProc(HWINEVENTHOOK hEvent, DWORD event, HWND hwndMsg, LONG idObject,
						LONG idChild, DWORD idThread, DWORD dwEventTime);

LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam);


// --------------------------------------------------------------------------
//
//  DllMain()
//
// --------------------------------------------------------------------------
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID fImpLoad)
{
    switch (dwReason) {
		case DLL_PROCESS_ATTACH:
			g_hModEventDll = hInst;
            break;
    }

    return(TRUE);
}

// --------------------------------------------------------------------------
//
//  InstallEventHookWithOleAcc
//
//  This installs the WinEvent hook if hwndPostTo is not null, or removes the
//  hook if the parameter is null. Does no checking for a valid window handle.
//
//  If successful, this returns TRUE.
//
// --------------------------------------------------------------------------
BOOL WINAPI InstallEventHookWithOleAcc (HWND hwndPostTo) {
	if (hwndPostTo != NULL) {
		if(g_hwndEventPost || g_hEventHook)
			return FALSE; // We already have a hook installed - you can only have one at a time

		// Install the hook
		g_hwndEventPost = hwndPostTo; // Must set this before installing the hook
		g_hEventHook = pSetWinEventHook(EVENT_MIN, EVENT_MAX, g_hModEventDll,
				NotifyProc, 0, 0,  WINEVENT_SKIPOWNPROCESS | WINEVENT_OUTOFCONTEXT/*WINEVENT_INCONTEXT*/);
		if (!g_hEventHook) {
			// Something went wrong - reset g_hwndEventPost to NULL
			g_hwndEventPost = NULL;
			return FALSE;
		}
	} else {
		// NOTE - We never fail if they are trying to uninstall the hook
		g_hwndEventPost = NULL;
		// Uninstalling the hook
		if (g_hEventHook != NULL) {
			pUnhookWinEvent(g_hEventHook);
			g_hEventHook = NULL;
		}
	}
	return TRUE;
}

// --------------------------------------------------------------------------
//
//  InstallEventHookWithoutOleAcc
//
//  This installs a mouse hook so we have some functionality when oleacc is
//  not installed
//
//  If successful, this returns TRUE.
//
// --------------------------------------------------------------------------
BOOL WINAPI InstallEventHookWithoutOleAcc (HWND hwndPostTo) {
	if (hwndPostTo != NULL) {
		if(g_hwndEventPost || g_hMouseHook)
			return FALSE; // We already have a hook installed - you can only have one at a time

		// Install the hook
		g_hwndEventPost = hwndPostTo; // Must set this before installing the hook
		g_hMouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, g_hModEventDll, 0);
		if (!g_hMouseHook) {
			// Something went wrong - reset g_hwndEventPost to NULL
			g_hwndEventPost = NULL;
			return FALSE;
		}
	} else {
		// NOTE - We never fail if they are trying to uninstall the hook
		g_hwndEventPost = NULL;
		// Uninstalling the hook
		if (g_hMouseHook != NULL) {
			UnhookWindowsHookEx(g_hMouseHook);
			g_hMouseHook = NULL;
		}
	}
	return TRUE;
}


// --------------------------------------------------------------------------
//
//  InstallEventHook
//
//  This function checks to see if Ole Accessibility is installed, and if so
//  uses the WinEvent hook.  Otherwise, it uses a mouse hook.
//
//  If successful, this returns TRUE.
//
// --------------------------------------------------------------------------

BOOL g_bOleAccInstalled = FALSE;
BOOL g_bCheckOnlyOnceForOleAcc = TRUE;

BOOL WINAPI InstallEventHook (HWND hwndPostTo) {
	// We check only the first time if Ole Acc is installed.  From then on,
	// we assume it remains constant.
	if(g_bCheckOnlyOnceForOleAcc) {
		g_bCheckOnlyOnceForOleAcc = FALSE;
		g_bOleAccInstalled = GetAcctiveAccessibleFunctions();
	}

	if(g_bOleAccInstalled)
		return InstallEventHookWithOleAcc(hwndPostTo);
	else
		return InstallEventHookWithoutOleAcc(hwndPostTo);
}

// --------------------------------------------------------------------------
//
//  GetCursorHack()
//
//  This function returns the last known user cursor handle.
//  
// --------------------------------------------------------------------------

DWORD WINAPI GetCursorHack()
{
	return g_dwCursorHack;
}

// --------------------------------------------------------------------------
//
//  NotifyProc()
//
//	This is the callback function for the WinEvent Hook we install. This
//	gets called whenever there is an event to process. The only things we
//	care about are focus changes and mouse/caret movement. The way we handle
//  the events is to post a message to the client (ScreenX) telling it
//	where the focus/mouse/caret is right now. It can then decide where it 
//	should be magnifying.
//
//	Parameters:
//		hEvent			A handle specific to this call back
//		event			The event being sent
//		hwnd			Window handle of the window generating the event or 
//						NULL if no window is associated with the event.
//		idObject		The object identifier or OBJID_WINDOW.
//		idChild			The child ID of the element triggering the event, 
//						or CHILDID_SELF if the event is for the object itself.
//		dwThreadId		The thread ID of the thread generating the event.  
//						Informational only.
//		dwmsEventTime	The time of the event in milliseconds.
// --------------------------------------------------------------------------
/* Forward ref */ BOOL GetObjectLocation(IAccessible * pacc, VARIANT* pvarChild, LPRECT lpRect);
void CALLBACK NotifyProc(HWINEVENTHOOK hEvent, DWORD event, HWND hwndMsg, LONG idObject,
						 LONG idChild, DWORD idThread, DWORD dwmsEventTime) {

	WPARAM			wParam;
	LPARAM			lParam;
	// Initialize pac to NULL so that we Release() pointers we've obtained.
	// Otherwise we will continually leak a heck of memory.
	IAccessible *	pacc = NULL;
	RECT			LocationRect;
	VARIANT 		varChild;
	HRESULT 		hr;
	//	 char sz[100];

	switch (event) {
#if 0
	case EVENT_OBJECT_HIDE:
		switch (idObject) {
		case OBJID_CARET:
			OutputDebugString("Caret hidden\r\n"); break;
		case OBJID_CURSOR:
			OutputDebugString("Cursor hidden\r\n"); break;
		}
		break;
		
	case EVENT_OBJECT_SHOW:
		switch (idObject) {
		case OBJID_CARET:
			OutputDebugString("Caret shown\r\n"); break;
		case OBJID_CURSOR:
			OutputDebugString("Cursor shown\r\n"); break;
		}
		break;
#endif
	case EVENT_OBJECT_FOCUS:
		if (!IsWindowVisible(hwndMsg))
			return;
		hr = pAccessibleObjectFromEvent(hwndMsg, idObject, idChild, &pacc, &varChild);
		if (!SUCCEEDED(hr))
			return;
		if (!GetObjectLocation(pacc,&varChild,&LocationRect))
			break;

		// center zoomed area on center of focus rect.
		wParam = MAKELONG(((LocationRect.left + LocationRect.right) / 2), ((LocationRect.bottom + LocationRect.top) / 2));
		lParam = dwmsEventTime;

		// JMC: TODO: Make sure the top left corner of the object is in the zoom rect
		PostMessage(g_hwndEventPost, WM_EVENT_FOCUSMOVE, wParam, lParam);
		break;
		
	case EVENT_OBJECT_LOCATIONCHANGE:
		switch (idObject) {
		case OBJID_CARET:
			hr = pAccessibleObjectFromEvent (hwndMsg,idObject,idChild, &pacc, &varChild);
			if (!SUCCEEDED(hr))
				return;
			if (!GetObjectLocation (pacc,&varChild,&LocationRect))
				break;
			
			// center zoomed area on center of focus rect.
			wParam = MAKELONG(((LocationRect.left + LocationRect.right) / 2), ((LocationRect.bottom + LocationRect.top) / 2));
			lParam = dwmsEventTime;
			PostMessage(g_hwndEventPost, WM_EVENT_CARETMOVE, wParam, lParam);
			break;
			
		case OBJID_CURSOR:
			hr = pAccessibleObjectFromEvent (hwndMsg,idObject,idChild, &pacc, &varChild);
			if (!SUCCEEDED(hr))
				return;
			if (!GetObjectLocation (pacc,&varChild,&LocationRect))
				break;
			wParam = MAKELONG(LocationRect.left, LocationRect.top);
			lParam = dwmsEventTime;
			PostMessage(g_hwndEventPost, WM_EVENT_MOUSEMOVE, wParam, lParam);
			break;
		}
		break;
	}
	if (pacc)
		pacc->Release();
}

// --------------------------------------------------------------------------
//
//	GetObjectLocation()
//
//	This fills in a RECT that has the location of the Accessible object
//	specified by pacc and idChild. The coordinates returned are screen
//	coordinates.
//
// --------------------------------------------------------------------------
BOOL GetObjectLocation(IAccessible * pacc, VARIANT* pvarChild, LPRECT lpRect) {
	HRESULT hr;
	SetRectEmpty(lpRect);
	
	hr = pacc->accLocation(&lpRect->left, &lpRect->top, &lpRect->right, &lpRect->bottom, *pvarChild);
	
	// the location is not a rect, but a top left, plus a width and height.
	// I want it as a real rect, so I'll convert it.
	lpRect->right  = lpRect->left + lpRect->right;
	lpRect->bottom = lpRect->top  + lpRect->bottom;
	
	if (!SUCCEEDED(hr))
		return(FALSE);
	return(TRUE);
}



// --------------------------------------------------------------------------
//
//  MouseProc()
//
//	This is the callback function for the Mouse Hook we install.
//
//	Parameters:
// --------------------------------------------------------------------------
LRESULT CALLBACK MouseProc(int code, WPARAM wParam, LPARAM lParam)
{
	// For WM_MOUSEMOVE and WM_NCMOUSEMOVE messages, we post the main window
	// WM_EVENT_MOUSEMOVE messages.  We don't want to do this if we are in
	// the address space of MAGNIFY.EXE.  To avoid this, we also check taht
	// g_bCheckOnlyOnceForOleAcc is TRUE.  If g_bCheckOnlyOnceForOleAcc is TRUE,
	// we are in another processes address space.
	// If we posted ourselves WM_EVENT_MOUSEMOVE while in MAGNIFY.EXE, we got all
	// sorts of weird crashes.
	if((WM_MOUSEMOVE == wParam || WM_NCMOUSEMOVE == wParam) && g_bCheckOnlyOnceForOleAcc)
	{
		g_dwCursorHack = (DWORD)GetCursor(); // JMC: Hack to get cursor on systems that don't support new GetCursorInfo
		MOUSEHOOKSTRUCT *pmhs = (MOUSEHOOKSTRUCT *)lParam;
		PostMessage(g_hwndEventPost, WM_EVENT_MOUSEMOVE, MAKELONG(pmhs->pt.x, pmhs->pt.y), 0);
	}

	return CallNextHookEx(g_hMouseHook, code, wParam, lParam);
}

// --------------------------------------------------------------------------
//
// FakeCursorMove
//
// This function is called to 'fake' the cursor moving.  It is used by the
// magnifier app when a MouseProc is used.  We run into problems when
// posting ourselves messages from the MouseProc of our own process.  To
// avoid these problems, MouseProc() does not post a WM_EVENT_MOUSEMOUVE
// if we are in the address space of MAGNIFY.EXE.  Instead, MAGNIFY.EXE
// is responsible for calling FakeCursorMove() whenever the mouse moves over
// a window of its own process. (NOTE: This is really easy to accomplish in
// MFC.  We just call FakeCursorMove() from PreTranslateMessage() - see
// MagBar.cpp and MagDlg.cpp
//
// --------------------------------------------------------------------------

void WINAPI FakeCursorMove(POINT pt)
{
	g_dwCursorHack = (DWORD)GetCursor(); // JMC: Hack to get cursor on systems that don't support new GetCursorInfo
	PostMessage(g_hwndEventPost, WM_EVENT_MOUSEMOVE, MAKELONG(pt.x, pt.y), 0);
}
