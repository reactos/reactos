// --------------------------------------------------------------------------
//
//  Mag_Hook.cpp
//
//  Accessibility Event trapper for magnifier. Uses an out-of-context WinEvent
//  hook (if MSAA is installed) or a mouse hook (if MSAA is not installed) to 
//  tell the app where to magnify.
//
//  Mainly what we want this to do is to watch for focus changes, caret
//  movement, and mouse pointer movement, and then post the appropriate
//  location to the Magnifier app so it can magnify the correct area.
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
#include <math.h>

//  Note on use of message queue:
//
//  mag_hook previously called SetWinEventHook( OUT_OF_CONTEXT ), which
//  would cause event messages to be queued by user, later calling back
//  to NotifyProc. Problem - user's queueing isn't so good - can get big
//  (esp. if magnifier is slow in consuming events), and can cause stress
//  problems for us.
//
//  Workaround - we now implement our own message queue (msgqueue.cpp) - it's
//  a fixed-length circular buffer. Once full, messages are politely dropped
//  (write returns FALSE. Dropped events are not a major problem for magnifier,
//  since during high activity periods that cause the queue to fill, there's
//  likely to be another focus/caret/mouse event which will supercede the
//  dropped message anyway.)
//  We now hook in-context, using callback NotifyProc_redir - this adds a
//  struct with all the event params to the message queue.
//  Back in-context, the queue calls our EvQueueProc callback, which gets
//  the saved struct of event params, and passes it to the original
//  NotifyProc.
//
//  In-context event,
//    NotifyProc_redir called in-context,
//      Writes event params to queue.
//      ---
//      back out of context,
//        Queue callback EvQueueProc called,
//          Gets struct from queue,
//            calls original NotifyProc.
//
//

#include "msgqueue.h"

// 'redirection' notify proc - receives event=s in-context, and
// places them into queue for later out-of-context processing.
void CALLBACK NotifyProc_redir(HWINEVENTHOOK hEvent, DWORD event,
                               HWND hwndMsg, LONG idObject,
						       LONG idChild, DWORD idThread, DWORD dwEventTime);

BOOL TryFindCaret( HWND hWnd, IAccessible * pAcc, VARIANT * pvarChild, RECT * prc );
BOOL IsFocussedItem( HWND hWnd, IAccessible * pAcc, VARIANT varChild );

struct EventInfo
{
	HWINEVENTHOOK hEvent;
	DWORD event;
	HWND hwndMsg;
	LONG idObject;
	LONG idChild;
	DWORD idThread;
	DWORD dwEventTime;
};


// number of events in event queue
#define EV_QUEUE_LEN 128

// Event queue callback - called-back by queue (out-of-context)
void CALLBACK EvQueueProc(  HQUEUECLI hQ,
							DWORD  EventCode,
							void * pMsg,
							DWORD  Len,
							void * pUserData );

// Event and string queues, client and server ends...
HQUEUECLI g_hQEvCli = NULL;
HQUEUESRV g_hQEvSrv = NULL;




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


// Workaround for menus - menus 'steal' focus, and don't hand it back
// - so we have to remember where it was before going into menu mode, so we
// can restore it properly afterwards.
POINT g_ptLastKnownBeforeMenu;
BOOL  g_InMenu = FALSE;
SIZE  g_ZoomSz;

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
DWORD_PTR		g_dwCursorHack = 0;
DWORD_PTR		g_MainExeThreadID =0;
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

            // Create server (write) end of event queue
            g_hQEvSrv = CreateMsgQueueServer( TEXT("MagMsgs"), sizeof( EventInfo ), EV_QUEUE_LEN );
            break;

		case DLL_PROCESS_DETACH:
            // Close server (write) end of event queue
			CloseMsgQueueServer( g_hQEvSrv );
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
BOOL WINAPI InstallEventHookWithOleAcc (HWND hwndPostTo) 
{
	if (hwndPostTo != NULL) 
    {
		g_MainExeThreadID = GetCurrentThreadId();

		if(g_hwndEventPost || g_hEventHook)
			return FALSE; // We already have a hook installed - you can only have one at a time

		// Install the hook
		g_hwndEventPost = hwndPostTo; // Must set this before installing the hook

        // Original out-of-context SetWinEventHook code, commented out, but left
        // here for reference...
		// g_hEventHook = pSetWinEventHook(EVENT_MIN, EVENT_MAX, g_hModEventDll,
		//		NotifyProc, 0, 0,  /*WINEVENT_SKIPOWNPROCESS | */WINEVENT_OUTOFCONTEXT);

        // Create client (read) end of event queue...
		g_hQEvCli = CreateMsgQueueClient( TEXT( "MagMsgs" ),
										sizeof( EventInfo ),
										EV_QUEUE_LEN,
										g_hModEventDll,
										EvQueueProc,
										NULL );

        // Set hook - in-context, and using NotifyProc_redir (which just dumps the events
        // to the queue) as callback...
		// Currently using a OUTOFCONTEXT hook. As the INCONTEXT hook is not going to  
		// work if launched from UM. 
		g_hEventHook = pSetWinEventHook(EVENT_MIN, EVENT_MAX, g_hModEventDll,
			        NotifyProc, 0, 0, WINEVENT_OUTOFCONTEXT);



		if (!g_hEventHook) 
        {
			// Something went wrong - reset g_hwndEventPost to NULL
			g_hwndEventPost = NULL;

            // Close message queue client (read) end...
			if( g_hQEvCli )
			{
				CloseMsgQueueClient( g_hQEvCli );
				g_hQEvCli = NULL;
			}

			return FALSE;
		}
	} 
    else 
    {
        // Close message queue client (read) end...
		if( g_hQEvCli )
		{
			CloseMsgQueueClient( g_hQEvCli );
			g_hQEvCli = NULL;
		}

		// NOTE - We never fail if they are trying to uninstall the hook
		g_hwndEventPost = NULL;
		// Uninstalling the hook
		if (g_hEventHook != NULL) 
        {
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
BOOL WINAPI InstallEventHookWithoutOleAcc (HWND hwndPostTo) 
{
	if (hwndPostTo != NULL) 
    {
		if(g_hwndEventPost || g_hMouseHook)
			return FALSE; // We already have a hook installed - yo u can only have one at a time

		// Install the hook
		g_hwndEventPost = hwndPostTo; // Must set this before installing the hook
		g_hMouseHook = SetWindowsHookEx(WH_MOUSE, MouseProc, g_hModEventDll, 0);
		if (!g_hMouseHook) 
        {
			// Something went wrong - reset g_hwndEventPost to NULL
			g_hwndEventPost = NULL;
			return FALSE;
		}
	} 
    else 
    {
		// NOTE - We never fail if they are trying to uninstall the hook
		g_hwndEventPost = NULL;
		// Uninstalling the hook
		if (g_hMouseHook != NULL) 
        {
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

BOOL WINAPI InstallEventHook (HWND hwndPostTo) 
{
	// We check onl y the first time if Ole Acc is installed.  From then on,
	// we assume it remains constant.
	if(g_bCheckOnlyOnceForOleAcc) 
    {
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

DWORD_PTR WINAPI GetCursorHack()
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
						 LONG idChild, DWORD idThread, DWORD dwmsEventTime) 
{

	WPARAM			wParam;
	LPARAM			lParam;
	// Initialize pac to NULL so that we Release() pointers we've obtained.
	// Otherwise we will continually leak a heck of memory.
	IAccessible *	pacc = NULL;
	RECT			LocationRect, CaretSrch;
	VARIANT 		varChild;
	HRESULT 		hr;
	BOOL			bX, bY;
	//	 char sz[100];

	switch (event) 
    {
	    case EVENT_OBJECT_FOCUS:
            // OutputDebugString(TEXT("FocusEvent"));
			// Here, For some windows for example, C-S Help, The windows are created 
			// hidden and then moved around. So, Call to MSAA delays the process of getting 
			// the window rect. And seems to work OK :a-anilk
			// What happens if we get the info the location from MSAA always ??
		    /*if (!IsWindowVisible(hwndMsg))
            {
                OutputDebugString(TEXT(" - invisible window\r\n"));
                // Next best thing - use the window rect...
                if( ! GetWindowRect(hwndMsg, & LocationRect) )
                {
			        return;
                }
                // Got window rect - fall through and continue processing...
            }
            else*/
            {
		        hr = pAccessibleObjectFromEvent(hwndMsg, idObject, idChild, &pacc, &varChild);
		        if (!SUCCEEDED(hr))
                {
#ifdef DEBUG
                    OutputDebugString(TEXT(" - AOFE failed\r\n"));
#endif
			        return;
                }
		        if (!GetObjectLocation(pacc,&varChild,&LocationRect))
                {
#ifdef DEBUG
                    OutputDebugString(TEXT(" - GetObjectLocation failed\r\n")); 
#endif
			        break;
                }
                // Got object rect - fall through and continue processing...
            }

			// This could be problematic if one window sends all the focus events like in IE
			// Hack would be to ignore the carets that are invisible, But can can other problems
			// for a possibly smaller gain : Donot use for now : a-anilk
			// if ( TryFindCaret( hwndMsg, pacc, &varChild, &CaretSrch ) )
			//	LocationRect = CaretSrch;

			// Ignore bogus focus events
			if ( !IsFocussedItem( hwndMsg, pacc, varChild ) )
				return;

			// Remove bogus all zero events from IE5.0
			if ( (LocationRect.top == 0) && (LocationRect.left == 0) && 
				  (LocationRect.bottom == 0) && (LocationRect.right == 0))
				return;

			int ZoomX;
			int ZoomY;

			// If the focussed object doesnot fit into the zoom area, 
			// Then reset the location of the focussed rectangle
			// BUG: Need to handle RTL languages

			// Does it fit horizontally?
			if( g_ZoomSz.cx <= abs(LocationRect.left - LocationRect.right) )
			{
				// HACK. The X-Dimension doesnot fit in.... So, Right Shift so that the focussed area is 
				// left justified. 
				ZoomX = LocationRect.left + (g_ZoomSz.cx/2);
			}
			else
			{
				// Zoom to center, as usual
				ZoomX = (LocationRect.left + LocationRect.right) / 2;
			}

			// Does it fit vertically?
			if( g_ZoomSz.cy <= abs(LocationRect.top - LocationRect.bottom) )
			{
				// Doesn't fit - zoom to left...
				ZoomY = LocationRect.top + (g_ZoomSz.cy/2);
			}
			else
			{
				// HACK. The Y-Dimension doesnot fit in.... So, Move up so that the focussed area is 
				// at the top. 
				wParam = MAKELONG(LocationRect.left , LocationRect.top + (g_ZoomSz.cy/2));
				ZoomY = (LocationRect.top + LocationRect.bottom) / 2;
			}

			wParam = MAKELONG( ZoomX, ZoomY );
		
            // Only update 'last known non-menu point' for focus while not in menu mode
            if( ! g_InMenu )
            {
                g_ptLastKnownBeforeMenu.x = LOWORD( wParam );
                g_ptLastKnownBeforeMenu.y = HIWORD( wParam );
            }

		    // JMC: TODO: Make sure the top left corner of the object is in the zoom rect
			// BMCK: PostMessage->SendMessage, since we're in-context. (Avoids hogging message queue)
		    // PostMessage(g_hwndEventPost, WM_EVENT_FOCUSMOVE, wParam, lParam);
		    SendMessage(g_hwndEventPost, WM_EVENT_FOCUSMOVE, wParam, 0);
            // OutputDebugString(TEXT(" - success\r\n"));
		    break;
		    
	    case EVENT_OBJECT_LOCATIONCHANGE:
		    switch (idObject) 
            {
		        case OBJID_CARET:
			        hr = pAccessibleObjectFromEvent (hwndMsg,idObject,idChild, &pacc, &varChild);
			        if (!SUCCEEDED(hr))
				        return;
			        if (!GetObjectLocation (pacc,&varChild,&LocationRect))
				        break;
			        
			        // center zoomed area on center of focus rect.
			        wParam = MAKELONG(((LocationRect.left + LocationRect.right) / 2), ((LocationRect.bottom + LocationRect.top) / 2));
			        lParam = dwmsEventTime;

                    // Only update 'last known non-menu point' for caret while not in menu mode
                    if( ! g_InMenu )
                    {
                        g_ptLastKnownBeforeMenu.x = LOWORD( wParam );
                        g_ptLastKnownBeforeMenu.y = HIWORD( wParam );
                    }

			        // BMCK: PostMessage->SendMessage, since we're in-context. (Avoids hogging message queue)
			        // PostMessage(g_hwndEventPost, WM_EVENT_CARETMOVE, wParam, lParam);
			        SendMessage(g_hwndEventPost, WM_EVENT_CARETMOVE, wParam, lParam);
			        break;
			        
		        case OBJID_CURSOR:
			        hr = pAccessibleObjectFromEvent (hwndMsg,idObject,idChild, &pacc, &varChild);
			        if (!SUCCEEDED(hr))
				        return;
			        if (!GetObjectLocation (pacc,&varChild,&LocationRect))
				        break;
			        wParam = MAKELONG(LocationRect.left, LocationRect.top);
			        lParam = dwmsEventTime;

                    // update 'last known non-menu point' for mouse even if in menu mode -
                    // mouse moves can occur while in menu mode, and we do want to remember them.
                    g_ptLastKnownBeforeMenu.x = LOWORD( wParam );
                    g_ptLastKnownBeforeMenu.y = HIWORD( wParam );

			        // BMCK: PostMessage->SendMessage, since we're in-context. (Avoids hogging message queue)
                    // PostMessage(g_hwndEventPost, WM_EVENT_MOUSEMOVE, wParam, lParam);
			        SendMessage(g_hwndEventPost, WM_EVENT_MOUSEMOVE, wParam, lParam);
			        break;
		    }
		    break;

            case EVENT_SYSTEM_MENUSTART:
                g_InMenu = TRUE;
                break;

				// Fix context menu tracking. :a-anilk
			case EVENT_SYSTEM_MENUPOPUPSTART:
				{
					TCHAR buffer[100];

					GetClassName(hwndMsg,buffer,100); 
					// Is it "Start" menu derivative. Then donot do this! 
					if ( lstrcmpi(buffer, TEXT("ToolbarWindow32")) == 0 )
						break;

					hr = pAccessibleObjectFromEvent (hwndMsg,idObject,idChild, &pacc, &varChild);
					if (!SUCCEEDED(hr))
						  return;
					if (!GetObjectLocation (pacc,&varChild,&LocationRect))
						  break;

					wParam = MAKELONG(LocationRect.left, LocationRect.top);
					lParam = dwmsEventTime;
					SendMessage(g_hwndEventPost, WM_EVENT_FORCEMOVE, wParam, lParam);
				}
				break;
	
            case EVENT_SYSTEM_MENUEND:
#ifdef DEBUG
                OutputDebugString (TEXT("Menu End\r\n"));
#endif
                if( g_InMenu )
                {
                    g_InMenu = FALSE;

			        lParam = GetTickCount();
                    wParam = MAKELONG( g_ptLastKnownBeforeMenu.x,
                                       g_ptLastKnownBeforeMenu.y );;

        			// BMCK: PostMessage->SendMessage, since we're in-context. (Avoids hogging message queue)
			        // PostMessage(g_hwndEventPost, WM_EVENT_FORCEMOVE, wParam, lParam);
			        SendMessage(g_hwndEventPost, WM_EVENT_FORCEMOVE, wParam, lParam);
                }
                break;

            case EVENT_SYSTEM_DIALOGEND:
#ifdef DEBUG
                OutputDebugString (TEXT("Dialog End\r\n"));
#endif
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
BOOL GetObjectLocation(IAccessible * pacc, VARIANT* pvarChild, LPRECT lpRect) 
{
	HRESULT hr;
	SetRectEmpty(lpRect);
	
	hr = pacc->accLocation(&lpRect->left, &lpRect->top, &lpRect->right, &lpRect->bottom, *pvarChild);
	
	// the location is not a rect, but a top left, plus a width and height.
	// I want it as a real rect, so I'll convert it.
	lpRect->right  = lpRect->left + lpRect->right;
	lpRect->bottom = lpRect->top  + lpRect->bottom;
	
	if ( hr != S_OK )
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
	// the address space of MAGNIFY.EXE.  To avoid this, we also check that
	// g_bCheckOnlyOnceForOleAcc is TRUE.  If g_bCheckOnlyOnceForOleAcc is TRUE,
	// we are in another processes address space.
	// If we posted ourselves WM_EVENT_MOUSEMOVE while in MAGNIFY.EXE, we got all
	// sorts of weird crashes.
	if((WM_MOUSEMOVE == wParam || WM_NCMOUSEMOVE == wParam) && g_bCheckOnlyOnceForOleAcc)
	{
		g_dwCursorHack = (DWORD_PTR)GetCursor(); // JMC: Hack to get cursor on systems that don't support new GetCursorInfo
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
	g_dwCursorHack = (DWORD_PTR)GetCursor(); // JMC: Hack to get cursor on systems that don't support new GetCursorInfo
	PostMessage(g_hwndEventPost, WM_EVENT_MOUSEMOVE, MAKELONG(pt.x, pt.y), 0);
}




// --------------------------------------------------------------------------
//
// NotifyProc_redir
//
// 'redirection' notify proc - receives events in-context, and
// places them into queue for later out-of-context processing.
//
// --------------------------------------------------------------------------

void CALLBACK
NotifyProc_redir(HWINEVENTHOOK hEvent, DWORD event, HWND hwndMsg, LONG idObject,
    LONG idChild, DWORD idThread, DWORD dwEventTime)
{
	if( idThread == g_MainExeThreadID 
		 && idObject == OBJID_CURSOR
		 && event == EVENT_OBJECT_LOCATIONCHANGE
		 && idChild == 0 )
	{
		// Ignore cursor move events from our own thread - we'll catch these
		// using FakeMouseMove instead.
		// (This gets around a nasty infinite mouse move message bug that
		// we don't yet understand fully...)
		return;
	}

	EventInfo evInfo;

	evInfo.hEvent = hEvent;
	evInfo.event = event;
	evInfo.hwndMsg = hwndMsg;
	evInfo.idObject = idObject;
    evInfo.idChild = idChild;
	evInfo.idThread = idThread;
	evInfo.dwEventTime = dwEventTime;

    WriteMsgQueueServer( g_hQEvSrv, & evInfo, sizeof( evInfo ) );
}



// --------------------------------------------------------------------------
//
// EvQueueProc
//
// Called back by event queue - passes event data on to real/original
// NotifyProc.
//
// --------------------------------------------------------------------------

void CALLBACK EvQueueProc(  HQUEUECLI hQ,
						    DWORD  EventCode,
						    void * pMsg,
						    DWORD  Len,
						    void * pUserData )
{
	// For this version, ignore the fact that messages are dropped.
	// Don't bother flushing stale messages either.
	if( EventCode == MSGQ_MESSAGE )
	{
		EventInfo * pevInfo = (EventInfo *) pMsg;
		NotifyProc( pevInfo->hEvent,
					pevInfo->event,
					pevInfo->hwndMsg,
					pevInfo->idObject,
					pevInfo->idChild,
					pevInfo->idThread,
					pevInfo->dwEventTime );
	}
	else if( EventCode == MSGQ_DROP )
	{
		// Do nothing
	}
}

// --------------------------------------------------------------------------
//
// SetZoomRect: Sets the maximum zoom rectangle.
//
// --------------------------------------------------------------------------

void SetZoomRect( SIZE sz )
{
	g_ZoomSz = sz;
}


BOOL TryFindCaret( HWND hWnd, IAccessible * pAcc, VARIANT * pvarChild, RECT * prc )
{
    // Check that it is the currently active caret...
    GUITHREADINFO gui;
	TCHAR buffer[100];

	GetClassName(hWnd,buffer,100); 

    gui.cbSize = sizeof(GUITHREADINFO);
    if( ! GetGUIThreadInfo( NULL , &gui ) )
    {
        OutputDebugString( TEXT("GetGUIThreadInfo failed") );
        return FALSE;
    }
        
    if( gui.hwndCaret != hWnd )
    {
        return FALSE;
    }

	// Is it toolbar, We cannot determine who had focus!!!
	if ( (lstrcmpi(buffer, TEXT("ToolbarWindow32")) == 0) ||
		(lstrcmpi(buffer, TEXT("Internet Explorer_Server")) == 0))
			MessageBeep(100);
			// return FALSE;

    // Try to get the caret for that window (if one exists)...
    IAccessible * pAccCaret = NULL;
    VARIANT varCaret;
    varCaret.vt = VT_I4;
    varCaret.lVal = CHILDID_SELF;
    if( S_OK != AccessibleObjectFromWindow( hWnd, OBJID_CARET, IID_IAccessible, (void **) & pAccCaret ) )
    {
        OutputDebugString( TEXT("TryFindCaret: AccessibleObjectFromWindow failed") );
        return FALSE;
    }

    // Now get location of the caret... (will fail if caret is invisible)
    HRESULT hr = pAccCaret->accLocation( & prc->left, & prc->top, & prc->right, & prc->bottom, varCaret );
    pAccCaret->Release();

    if( hr != S_OK )
    {
        // Error, or caret is currently invisible.
        return FALSE;
    }

    // Convert accLocation's left/right/width/height to left/right/top/bottom...
    prc->right += prc->left;
    prc->bottom += prc->top;
	

    // All done!
    return TRUE;
}

BOOL IsFocussedItem( HWND hWnd, IAccessible * pAcc, VARIANT varChild )
{
	
	TCHAR buffer[100];

	GetClassName(hWnd,buffer,100); 
	// Is it toolbar, We cannot determine who had focus!!!
	if ( (lstrcmpi(buffer, TEXT("ToolbarWindow32")) == 0) ||
		(lstrcmpi(buffer, TEXT("Internet Explorer_Server")) == 0))
			return TRUE;
	
	VARIANT varState;
	HRESULT hr;
	
	VariantInit(&varState); 

	hr = pAcc->get_accState(varChild, &varState);

	if ( hr == S_OK )
	{
		if ( ! (varState.lVal & STATE_SYSTEM_FOCUSED) )
		return FALSE;
	}

	return TRUE;
}

