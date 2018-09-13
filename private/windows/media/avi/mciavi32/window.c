/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   Title:   window.c - Multimedia Systems Media Control Interface
	    driver for AVI.

*****************************************************************************/
#include "graphic.h"

#include "avitask.h"    // for TASKIDLE

//#define IDM_CONFIG              0x100
//#define IDM_SKIPFRAMES          0x110
#define IDM_MUTE                0x120
#define IDM_STRETCH             0x130

#ifdef _WIN32
// Use a different class name on 32 bit systems to ease the 16/32
// coexistence problem.  (We might want both classes defined at once.)
TCHAR szClassName[] = TEXT("AVIWnd32");
#else
char szClassName[] = "AVIWnd";
#endif


DWORD NEAR PASCAL GraphicStop (NPMCIGRAPHIC npMCI, DWORD dwFlags);
DWORD NEAR PASCAL GraphicPause (NPMCIGRAPHIC npMCI, DWORD dwFlags);
DWORD NEAR PASCAL GraphicPlay (NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_ANIM_PLAY_PARMS lpPlay );
DWORD NEAR PASCAL GraphicSeek (NPMCIGRAPHIC npMCI, DWORD dwFlags,
    LPMCI_SEEK_PARMS lpSeek);

BOOL NEAR PASCAL GraphicWindowInit (void)
{
    WNDCLASS cls;

    // define the class of window we want to register

    cls.lpszClassName = szClassName;
    cls.style = CS_GLOBALCLASS | CS_OWNDC;
    cls.hCursor = LoadCursor (NULL, IDC_ARROW);
    cls.hIcon = NULL;
    cls.lpszMenuName = NULL;
////cls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    cls.hbrBackground = GetStockObject(BLACK_BRUSH);
    cls.hInstance = ghModule;
    cls.lpfnWndProc = GraphicWndProc;
    cls.cbClsExtra = 0;
    cls.cbWndExtra = sizeof (NPMCIGRAPHIC);

    return RegisterClass (&cls);
}

#ifdef _WIN32
/*
 * de-register the class on unloading the dll so that we can
 * successfully re-register the class next time we are loaded.
 * note that nt only unregisters a class when the app exits.
 */
BOOL NEAR PASCAL GraphicWindowFree(void)
{
	return(UnregisterClass(szClassName, ghModule));
}


/**************************************************************************
***************************************************************************/

//-----------------------------------------------------------------------
// this is the winproc thread's main function.
// we create the window and signal the event once initialization
// is complete. We then block either processing messages or waiting for
// hEventWinProcDie. When this is set, we clean up and exit

void aviWinProcTask(DWORD_PTR dwInst)
{
    NPMCIGRAPHIC npMCI = (NPMCIGRAPHIC) dwInst;
    HWND hWnd;
    MSG msg;

    // Create the default window - the caller may
    // supply style and parent window.

#ifdef DEBUG
    if (npMCI->hwndParent) {
	if ( !IsWindow(npMCI->hwndParent)) {
	    //DebugBreak();
	    // This should have been trapped before getting here
	}
    }
#endif

    if (npMCI->dwStyle != (DWORD) -1) {
	// CW_USEDEFAULT can't be used with popups or children so
	// if the user provides the style, default to full-screen.

	hWnd =

	// Note:  The CreateWindow/Ex call is written this way as on Win32
	// CreateWindow is a MACRO, and hence the call must be contained
	// within the preprocessor block.
	CreateWindowEx(
#ifdef BIDI
	    WS_EX_BIDI_SCROLL |  WS_EX_BIDI_MENU |WS_EX_BIDI_NOICON,
#else
	    0,
#endif
	    szClassName,
	    FileName(npMCI->szFilename),
	    npMCI->dwStyle,
	    0, 0,
	    GetSystemMetrics (SM_CXSCREEN),
	    GetSystemMetrics (SM_CYSCREEN),
	    npMCI->hwndParent,
	    NULL, ghModule, (LPTSTR)npMCI);
    } else {
	npMCI->dwStyle = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX |
		  WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	hWnd =
	CreateWindowEx(
#ifdef BIDI
	    WS_EX_BIDI_SCROLL |  WS_EX_BIDI_MENU |WS_EX_BIDI_NOICON,
#else
	    0,
#endif
	    szClassName,
	    FileName(npMCI->szFilename),
	    npMCI->dwStyle,
	    CW_USEDEFAULT, 0,
	    CW_USEDEFAULT, 0,
	    npMCI->hwndParent,
	    NULL, ghModule, (LPTSTR)npMCI);
    }

    npMCI->hwndDefault = hWnd;
    npMCI->hwndPlayback = hWnd;


    if (!hWnd) {
	// fail to start up - just exit this function and the caller
	// will detect thread exit
	DPF(("CreateWindow failed, LastError=%d\n", GetLastError()));
	npMCI->dwReturn = MCIERR_CREATEWINDOW;
	return;
    }

    // window created ok - signal worker thread to continue
    SetEvent(npMCI->hEventWinProcOK);

    while (GetMessage(&msg, NULL, 0, 0)) {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
    }

    // WM_QUIT received - exit now
    GdiFlush();
    return;

}

//
// Pass request bit(s) to worker thread.
// Use WinProcRequestEx when the WinCrit critical section is already held
//   and WINPROC_ACTIVE/INACTIVE is not being used
//
// Use WinProcRequest when the WinCrit critical section is required
//

void INLINE
WinProcRequestEx(NPMCIGRAPHIC npMCI, DWORD dwFlag)
{
    DPF2(("WinProcRequestEx... request %8x", dwFlag));
    npMCI->dwWinProcRequests |= dwFlag;
    SetEvent(npMCI->heWinProcRequest);
    DPF2(("!...Ex request %8x done\n", dwFlag));
}

void
WinProcRequest(NPMCIGRAPHIC npMCI, DWORD dwFlag)
{
    DPF2(("WinProcRequest... request %8x", dwFlag));
    EnterWinCrit(npMCI);
    DPF2(("!... request %8x ...", dwFlag));

    // If we are being made active or inactive, then ensure that only
    // one of the WINPROC_ACTIVE/WINPROC_INACTIVE bits is switched on.
    if (dwFlag & (WINPROC_ACTIVE | WINPROC_INACTIVE)) {
	npMCI->dwWinProcRequests &= ~(WINPROC_ACTIVE | WINPROC_INACTIVE);
    }
    npMCI->dwWinProcRequests |= dwFlag;

    SetEvent(npMCI->heWinProcRequest);

    DPF2(("!... request %8x done\n", dwFlag));
    LeaveWinCrit(npMCI);
}


#endif



DWORD FAR PASCAL GraphicConfig(NPMCIGRAPHIC npMCI, DWORD dwFlags);
#if 0
static void NEAR PASCAL Credits(HWND hwnd);
#endif

//
// Window proc is always run on the winproc thread. We hold
// a critical section during all paint/palette code to protect us
// against interaction with the worker thread. We do not hold this
// critical section for the whole winproc.
//
LRESULT FAR PASCAL _LOADDS GraphicWndProc (HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT         ps;
    NPMCIGRAPHIC        npMCI;
    HMENU               hmenu;
    HDC                 hdc;
    RECT                rc;
    MINMAXINFO FAR *    lpmmi;
    TCHAR               ach[80];

#ifndef _WIN32
    npMCI = (NPMCIGRAPHIC)GetWindowWord (hwnd, 0);
#else
    npMCI = (NPMCIGRAPHIC)GetWindowLongPtr (hwnd, 0);

    if (npMCI) {
	if (IsBadReadPtr(npMCI, sizeof(MCIGRAPHIC))
	  || !IsTask(npMCI->hTask)) {
	    DPF2(("WndProc called with msg %04x after task npMCI=%08x is dead\n", wMsg, npMCI));

	    // if the device is in some sort of shutdown state, only the
	    // messages below are safe to process. Note: not processing
	    // AVIM_DESTROY can lead to deadlock.
	    switch (wMsg) {
		case AVIM_DESTROY:
		    DestroyWindow(hwnd);
		    return 1L;
		case WM_DESTROY:
		    PostQuitMessage(0);
		    break;

		default:
		   return DefWindowProc(hwnd, wMsg, wParam, lParam);
	    }
	}
	Assert(wMsg != WM_CREATE);
    } else {

	// npMCI is NULL - only WM_CREATE can safely be processed

	// I think we can also safely process AVIM_DESTROY and that
	// will avoid a few potential hangs...
	if ((wMsg != WM_CREATE) && (wMsg != AVIM_DESTROY)) {
	    return DefWindowProc(hwnd, wMsg, wParam, lParam);
	}
    }
#endif

    switch (wMsg)
	{

	case WM_CREATE:

	    npMCI = (NPMCIGRAPHIC)(UINT_PTR)(DWORD_PTR)
			    ((LPCREATESTRUCT)lParam)->lpCreateParams;

#ifdef _WIN32
	    SetWindowLongPtr (hwnd, 0, (UINT_PTR)npMCI);
#else
	    SetWindowWord (hwnd, 0, (WORD)npMCI);
#endif
	
	    hmenu = GetSystemMenu(hwnd, 0);
	
	    if (hmenu) {
		/* Our system menu is too long--get rid of extra stuff. */
//              DeleteMenu(hmenu, SC_RESTORE, MF_BYCOMMAND);
//              DeleteMenu(hmenu, SC_MINIMIZE, MF_BYCOMMAND);
		DeleteMenu(hmenu, SC_MAXIMIZE, MF_BYCOMMAND);
		DeleteMenu(hmenu, SC_TASKLIST, MF_BYCOMMAND);

		/* Add additional menu items to the end of the system menu */
//              AppendMenu(hmenu, MF_SEPARATOR, 0, 0L);

#ifdef IDM_CONFIG
		LoadString(ghModule, MCIAVI_MENU_CONFIG, ach, NUMELMS(ach));
		AppendMenu(hmenu, MF_STRING, IDM_CONFIG, ach);
#endif

		LoadString(ghModule, MCIAVI_MENU_STRETCH, ach, NUMELMS(ach));
		AppendMenu(hmenu, MF_STRING, IDM_STRETCH, ach);

		LoadString(ghModule, MCIAVI_MENU_MUTE, ach, NUMELMS(ach));
		AppendMenu(hmenu, MF_STRING, IDM_MUTE, ach);
	    }
	
	    break;
	
	case WM_INITMENU:
	
	    hmenu = GetSystemMenu(hwnd, 0);
	
	    if (hmenu) {
#ifdef IDM_SKIPFRAMES
		CheckMenuItem(hmenu, IDM_SKIPFRAMES, MF_BYCOMMAND |
			    ((npMCI->dwOptionFlags & MCIAVIO_SKIPFRAMES) ?
					    MF_CHECKED : MF_UNCHECKED));
#endif
		CheckMenuItem(hmenu, IDM_STRETCH, MF_BYCOMMAND |
			    ((npMCI->dwOptionFlags & MCIAVIO_STRETCHTOWINDOW) ?
					    MF_CHECKED : MF_UNCHECKED));

#ifdef IDM_CONFIG
		/* If in configure box, disable menu item. */
		EnableMenuItem(hmenu, IDM_CONFIG, MF_BYCOMMAND |
			    (npMCI->wMessageCurrent == 0 ?
						MF_ENABLED : MF_GRAYED));
#endif
					
		/* If in stupid mode, disable stretch menu item. */
		EnableMenuItem(hmenu, IDM_STRETCH, MF_BYCOMMAND |
			    ((!(npMCI->dwOptionFlags & MCIAVIO_STUPIDMODE)) ?
					    MF_ENABLED : MF_GRAYED));
					
		EnableMenuItem(hmenu, IDM_MUTE, MF_BYCOMMAND |
			    (npMCI->nAudioStreams ?
					    MF_ENABLED : MF_GRAYED));

		CheckMenuItem(hmenu, IDM_MUTE, MF_BYCOMMAND |
			    (!(npMCI->dwFlags & MCIAVI_PLAYAUDIO) ?
					    MF_CHECKED : MF_UNCHECKED));
	    }
	    break;
	
	case WM_SYSCOMMAND:
	    switch (wParam & 0xfff0) {
	    case SC_KEYMENU:
	    case SC_MOUSEMENU:
		gfEvilSysMenu++;
		lParam = DefWindowProc(hwnd, wMsg, wParam, lParam);
		gfEvilSysMenu--;
		return lParam;
		
#ifdef IDM_SKIPFRAMES
	    case IDM_SKIPFRAMES:
		EnterWinCrit(npMCI);
		npMCI->dwOptionFlags ^= MCIAVIO_SKIPFRAMES;
		LeaveWinCrit(npMCI);
		break;
#endif
	    case IDM_STRETCH:
		EnterWinCrit(npMCI);

		npMCI->dwOptionFlags ^= MCIAVIO_STRETCHTOWINDOW;
		
		if (!(npMCI->dwOptionFlags & MCIAVIO_STRETCHTOWINDOW)) {
		    SetWindowToDefaultSize(npMCI, FALSE);
		}
		
		Winproc_DestRect(npMCI, FALSE);
		LeaveWinCrit(npMCI);
		break;

	    case IDM_MUTE:
		// just set request flag in npMCI
		WinProcRequest(npMCI, WINPROC_MUTE);
		break;

#ifdef IDM_CONFIG
	    case IDM_CONFIG:
		npMCI->wMessageCurrent = MCI_CONFIGURE;
		gfEvil++;
		dwOptions = npMCI->dwOptions;
		f = ConfigDialog(NULL, npMCI);
		if (f) {
	#ifdef DEBUG
		    //
		    // in DEBUG always reset the dest rect because the user may
		    // have played with the DEBUG DrawDib options and we will
		    // need to call DrawDibBegin() again.
		    //
		    if (TRUE)
	#else
		    if ((npMCI->dwOptionFlags & (MCIAVIO_STUPIDMODE|MCIAVIO_ZOOMBY2
						 |MCIAVIO_WINDOWSIZEMASK))
				!= (dwOptions & (MCIAVIO_STUPIDMODE|MCIAVIO_ZOOMBY2
						 |MCIAVIO_WINDOWSIZEMASK)) )
	#endif
		    {
	
			npMCI->lFrameDrawn =
				(- (LONG) npMCI->wEarlyRecords) - 1;
			//EnterWinCrit(npMCI);
			SetWindowToDefaultSize(npMCI, FALSE);
			Winproc_DestRect(npMCI, FALSE);
			//LeaveWinCrit(npMCI);
		    }
		} else {
		    npMCI->dwOptionFlags = dwOptions;
		}
		

		GraphicConfig(npMCI, 0L);
		gfEvil--;
		npMCI->wMessageCurrent = 0;
		break;
#endif
	    }
	    break;

	case WM_CLOSE:

	    // Hide default window

	    WinProcRequest(npMCI, WINPROC_STOP);
	    ShowWindow(hwnd, SW_HIDE);
	    return 0L;

// this stuff is here because of some bizarre inter-thread rules in nt
	case AVIM_DESTROY:
	    DestroyWindow(hwnd);
	    return 1L;

	case AVIM_SHOWSTAGE:
	    {
		BOOL bIsVis;

		// activate if not visible. force activation if requested
		// (if palette changes)
		if (wParam) {
		    bIsVis = FALSE;
		} else {
		    bIsVis = IsWindowVisible(hwnd);
		}

		SetWindowPos(npMCI->hwndPlayback, HWND_TOP, 0, 0, 0, 0,
		    SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW |
		    (bIsVis ? SWP_NOACTIVATE : 0));

		if (!bIsVis) {
		    SetForegroundWindow(hwnd);
		}
	    }
	    return 1L;



	case WM_DESTROY:

	    // The window may be destroyed 2 ways.
	    //  a. the device is closed. In this case the animation is
	    //  freed in DeviceClose which is called from GraphicClose
	    //  and the animation ID is NULL by the time this window is
	    //  destroyed.
	    //  b. the window is closed. In this case, the animation is
	    //  not closed and we should set the stage to NULL. A new
	    //  default window will be created if needed.

	    EnterWinCrit(npMCI);
	    if (IsTask(npMCI->hTask)) {
		WinProcRequestEx(npMCI, WINPROC_STOP);
	    }

	    if (npMCI->hwndPlayback == npMCI->hwndDefault)
		npMCI->hwndPlayback = NULL;
	    npMCI->hwndDefault = NULL;

	    LeaveWinCrit(npMCI);

	    // winproc thread can now exit
	    PostQuitMessage(0);
	    break;

	case WM_ERASEBKGND:

	    hdc = (HDC) wParam;

	    // We should not need any critical section as the system
	    // has given us a HDC as a message parameter.
	    Assert(hdc && (hdc != npMCI->hdc));
	    //EnterWinCrit(npMCI);

	    if (!(npMCI->dwFlags & MCIAVI_SHOWVIDEO)) {
		FillRect(hdc, &npMCI->rcDest, GetStockObject(GRAY_BRUSH));
	    }
		
	    SaveDC(hdc);

	    ExcludeClipRect(hdc,
		npMCI->rcDest.left, npMCI->rcDest.top,
		npMCI->rcDest.right, npMCI->rcDest.bottom);

	    GetClientRect(hwnd, &rc);
	    FillRect(hdc, &rc, GetStockObject(BLACK_BRUSH));

	    RestoreDC(hdc, -1);

#if 0
	    /* Hack: if we're in a WAIT state, we won't get
	    ** a WM_PAINT, so we need to invalidate the streams here
	    */
	    GetClipBox(hdc, &rc);
	    StreamInvalidate(npMCI, &rc);
#endif

	    //LeaveWinCrit(npMCI);

	    return 0L;

	case WM_PAINT:


	    // we always do this even if we leave it to the
	    // worker to paint, since otherwise we'll loop endlessly
	    // processing WM_PAINT
	    hdc = BeginPaint(hwnd, &ps);

	    rc = ps.rcPaint;

	    // if the worker thread is cueing, seeking or playing we don't
	    // actually want to paint (in case it is doing rle delta paints).
	    // now we've got the window critsec, it is safe to check
	    // the task state
	    if ((npMCI->wTaskState == TASKPLAYING) ||
		(npMCI->wTaskState == TASKCUEING)) {
		    npMCI->dwFlags |= MCIAVI_NEEDUPDATE;
	    } else {

		StreamInvalidate(npMCI, &rc);
		// we don't need to pass rc in as we are clipped to this
		// already and all they do is clip to it.

		// don't paint if there is a 'pf' since we only ever
		// reference this on the worker thread (OLE threading rules)
		if (npMCI->pf) {
		    WinProcRequest(npMCI, WINPROC_UPDATE);
		} else {

		    // note: TryStreamUpdate will get the HDC critical section
		    if (!TryStreamUpdate(npMCI, MCI_DGV_UPDATE_PAINT, hdc, NULL)) {

			// requires full play - ask worker thread to do this

			// to paint the frame at this point, we would have
			// to call mciaviPlayFile. This can only be safely called
			// on the worker thread. To avoid potential deadlock,
			// this paint has to be done asynchronously from us, and
			// also we lose the update rect information - the
			// worker will paint the entire destination rect.

#if 0 // looks bad - don't bother
			// paint gray first, in case the worker thread is busy
			// or it fails
			GetClientRect(hwnd, &rc);
			FillRect(hdc, &rc, GetStockObject(DKGRAY_BRUSH));
#endif          
			WinProcRequest(npMCI, WINPROC_UPDATE);

		    } else {
			// painted successfully on this thread

			// if we are playing and so have an hdc then we need to
			// reprepare the orig dc after switching back to it
			if (npMCI->wTaskState == TASKPAUSED) {
			    EnterHDCCrit(npMCI);
			    // if a new command has arrived the state may have changed
			    // underneath us.  If so, the dc may have been released.  We
			    // only want to call PrepareDC if npMCI->hdc is non null.
			    if(npMCI->hdc) PrepareDC(npMCI);
			    LeaveHDCCrit(npMCI);  // remember to release the critical section
			}
		    }
		}
	    }

	    EndPaint(hwnd, &ps);
	    return 0L;         // we do NOT call DefWindowProc
	
	case WM_PALETTECHANGED:

	    // We're not using the default window.  We have no business here.
	    if (npMCI->hwndPlayback != hwnd)
		break;

	    //
	    // someone has realized a palette - so we need to re-realize our
	    // palette (note that this will also cause drawdib to
	    // check for PAL_INDICES vs PAL_COLOURS.
	    //
	    if ((HWND) wParam != hwnd) {
		WinProcRequest(npMCI, WINPROC_REALIZE);

		// invalidate is done in InternalRealize
		//InvalidateRect(hwnd, NULL, FALSE);
	    }
	    break;
	
	case WM_QUERYNEWPALETTE:
	    {
#if 0
		LONG lRet;
#endif
		// We're not using the default window.  We have no business here.
		if (npMCI->hwndPlayback != hwnd)
		    break;

#if 0
    // not true = internalrealize has always returned 0, so why do this
    // on this thread?
		EnterHDCCrit(npMCI);

		// need to do this on winproc thread to get
		// correct return value.
		lRet = InternalRealize(npMCI);
		LeaveHDCCrit(npMCI);
		return(lRet);
#else
		WinProcRequest(npMCI, WINPROC_REALIZE);
		return 0;
#endif
	    }

	case WM_WINDOWPOSCHANGED:
	    //EnterWinCrit(npMCI);
	    //CheckWindowMove grabs the critical section when it needs it
	    CheckWindowMove(npMCI, TRUE);
	    //LeaveWinCrit(npMCI);
	    break;

	case WM_SIZE:
	    EnterWinCrit(npMCI);
	    Winproc_DestRect(npMCI, FALSE);
	    LeaveWinCrit(npMCI);
	    break;
	
	case WM_QUERYENDSESSION:

	    WinProcRequest(npMCI, WINPROC_STOP);

	    break;

	case WM_ENDSESSION:
	    if (wParam)  {
		DestroyWindow(hwnd); // we may not be able to destroy window?
	    }
	    break;

	case WM_GETMINMAXINFO:
	    lpmmi = (MINMAXINFO FAR *)(lParam);

	    lpmmi->ptMinTrackSize.x = GetSystemMetrics(SM_CXSIZE) * 2;
	    break;

	case WM_NCACTIVATE:
	    WinProcRequest(npMCI, wParam?WINPROC_ACTIVE : WINPROC_INACTIVE);
	    break;

#if 0
We should not need both ACTIVATE and NCACTIVATE - use the one that
arrives first (to give us a little more time and reduce the start up
latency)
	case WM_ACTIVATE:
	    WinProcRequest(npMCI, wParam?WINPROC_ACTIVE : WINPROC_INACTIVE);
	    break;
#endif

#ifdef REMOTESTEAL
	case WM_AUDIO_ON:     // Someone has released the wave device
	  {
	    extern HWND hwndLostAudio;
	    //Assert(npMCI->dwFlags & MCIAVI_PLAYAUDIO);
	    //Assert(npMCI->dwFlags & MCIAVI_LOSTAUDIO);
	    //Assert(npMCI->hWave == NULL);
	    // Timing might be such that these assertions are invalid

	    // If we are not playing then we might be able to forward
	    // the message to another... OR, tell the one giving up the
	    // audio that it can be reclaimed because we no longer have
	    // a use for it.
	    if (npMCI->wTaskState != TASKPLAYING) {
		if (!hwndLostAudio) {
		    hwndLostAudio = (HWND)wParam;
		}
		if (hwndLostAudio) {
		    DPF2(("Forwarding WM_AUDIO_ON message to %x...\n", hwndLostAudio));
		    if (IsWindow(hwndLostAudio)) {
			PostMessage(hwndLostAudio, WM_AUDIO_ON, 0, 0);
		    }
		    hwndLostAudio = 0;  // Prevent further messages
		}
	    } else {
		// we are playing
		// Save the window handle of the window releasing the sound
		DPF2(("Setting hwndLostAudio to %x (was %x)\n", wParam, hwndLostAudio));
		hwndLostAudio = (HWND)wParam;
		// if hwndLostAudio==0 then we do not return it to anyone

		WinProcRequest(npMCI, WINPROC_SOUND);
	    }
	    return 0;
	    break;
	  }

	case WM_AUDIO_OFF:    // Someone wants our wave device
	  {
	    extern HWND hwndWantAudio;
	    //Assert(npMCI->dwFlags & MCIAVI_PLAYAUDIO);
	    //Assert(!(npMCI->dwFlags & MCIAVI_LOSTAUDIO));
	    //Assert(npMCI->hWave != NULL);
	    // Timing might be such that these assertions are invalid

	    SetNTFlags(npMCI, NTF_AUDIO_OFF);
	    // Save the window handle of the window which wants sound
	    DPF2(("WM_AUDIO_OFF... hwndWantAudio set to %x (was %x)\n", wParam, hwndWantAudio));
	    hwndWantAudio = (HWND)wParam;
	    // if hwndWantAudio==0 then we do not release the wave device

	    if (IsWindow(hwndWantAudio)) {
		WinProcRequest(npMCI, WINPROC_SILENT);
	    } else {
		DPF(("WM_AUDIO_OFF... but the target window is invalid\n"));
	    }
	    ResetNTFlags(npMCI, NTF_AUDIO_OFF);
	    return 0;
	    break;
	  }
#endif // REMOTESTEAL

#if 0
	case WM_LBUTTONDOWN:
	    {
		DWORD   dw;
		static DWORD dwLastClick;
		static DWORD dwClicks = 0;
		#define MAX_CLICKS      7
		/*     . = (0,300)  - = (300,1000)  word = (500,1500)   */
		/*     AVI:   .-    ...-   ..                           */
		static DWORD adwClickHigh[MAX_CLICKS] =
		    {  300, 1500,  300,  300,  300, 1500,  300 };
		static DWORD adwClickLow[MAX_CLICKS] =
		    {    0,  500,    0,    0,    0,  500,    0 };
		
		dw = timeGetTime();
		if (((dw - dwLastClick) > adwClickLow[dwClicks]) &&
			((dw - dwLastClick) <= adwClickHigh[dwClicks]))
		    dwClicks++;
		else
		    dwClicks = 0;

		dwLastClick = dw;

		if (dwClicks == MAX_CLICKS) {
		    WinProcRequest(npMCI, WINPROC_STOP);
		    Credits(hwnd);
		    dwClicks = 0;
		}
	    }
#endif
	} // switch(wMsg)

    return DefWindowProc(hwnd, wMsg, wParam, lParam);
}

#if 0
static void NEAR PASCAL Credits(HWND hwnd)
{
	/*  Credits...  */
	RECT            rc;
	RECT            rcUpdate;
	HDC             hdc;
	MSG             msg;
	int             dyLine;
	int             yLine;
	TEXTMETRIC      tm;
	DWORD           dwNextTime;
	long            lScroll;
	DWORD           rgb;
	HANDLE          hResInfo;
	HANDLE          hResData;
	LPSTR           pchSrc, pchDst;
	char            achLine[100];
	int             iEncrypt;

	#define EOFCHAR '@'             // end of credits file

	/* load the credits */
	if ((hResInfo = FindResource(ghModule, TEXT("MMS"), TEXT("MMSCR"))) == NULL)
		return;
	if ((hResData = LoadResource(ghModule, hResInfo)) == NULL)
		return;
	if ((pchSrc = LockResource(hResData)) == NULL)
		return;

	/* we want to get all mouse and keyboard events, to make
	 * sure we stop the animation when the user clicks or
	 * hits a key
	 */
	SetFocus(hwnd);
	SetCapture(hwnd);

	/* Scroll the credits up, one pixel at a time.  pchSrc
	 * points to the encrypted data; achLine contains a decrypted
	 * line (null-terminated).  dyLine is the height of each
	 * line (constant), and yLine is between 0 and dyLine,
	 * indicating how many pixels of the line have been scrolled
	 * in vertically from the bottom
	 */
	hdc = GetDC(hwnd);
	SelectObject(hdc, GetStockObject(ANSI_VAR_FONT));
	GetClientRect(hwnd, &rc);
	SetTextAlign(hdc, TA_CENTER);
	SetBkColor(hdc, RGB(0, 0, 0));
	SetRect(&rcUpdate, 0, rc.bottom - 1, rc.right, rc.bottom);
	GetTextMetrics(hdc, &tm);
	if ((dyLine = tm.tmHeight + tm.tmExternalLeading) == 0)
		dyLine = 1;
	yLine = dyLine;
	dwNextTime = GetCurrentTime();  // time to do the next scroll
	lScroll = 0;
	iEncrypt = 0;
	while (TRUE) {
		/* If the user clicks the mouse or hits a key, exit.
		 * However, ignore WM_LBUTTONUP because they will have
		 * to let go of the mouse after clicking the icon.
		 * Also, ignore mouse move messages.
		 */
		if (PeekMessage(&msg, hwnd, WM_KEYFIRST, WM_KEYLAST,
				PM_NOREMOVE | PM_NOYIELD))
			break;                  // exit on key hit

		if (PeekMessage(&msg, hwnd, WM_MOUSEFIRST, WM_MOUSELAST,
				PM_NOREMOVE | PM_NOYIELD)) {
			if ((msg.message == WM_MOUSEMOVE) ||
			    (msg.message == WM_LBUTTONUP)) {
				/* remove and ignore message */
				PeekMessage(&msg, hwnd, msg.message,
					msg.message,
					PM_REMOVE | PM_NOYIELD);
			}
			else
				break;          // exit on click
		}

		/* scroll at a fixed no. of vertical pixels per sec. */
		if (dwNextTime > GetCurrentTime())
			continue;
		dwNextTime += 50L;      // millseconds per scroll

		if (yLine == dyLine) {
			/* decrypt a line and copy to achLine */
			pchDst = achLine;
			while (TRUE) {
				*pchDst = (char) (*pchSrc++ ^
					(128 | (iEncrypt++ & 127)));
				if ((*pchDst == '\r') ||
				    (*pchDst == EOFCHAR))
					break;
				pchDst++;
			}

			if (*pchDst == EOFCHAR)
				break;          // no more lines
			*pchDst = 0;            // null-terminate
			pchSrc++, iEncrypt++;   // skip '\n'
			yLine = 0;
		}

		/* scroll screen up one pixel */
		BitBlt(hdc, 0, 0, rcUpdate.right, rcUpdate.top,
			hdc, 0, 1, SRCCOPY);

		/* vary the text colors through a "rainbow" */
		switch ((int) (lScroll++ / 4) % 5/*num-of-cases*/) {
		case 0: rgb = RGB(255,   0,   0); break;
		case 1: rgb = RGB(255, 255,   0); break;
		case 2: rgb = RGB(  0, 255,   0); break;
		case 3: rgb = RGB(  0, 255, 255); break;
		case 4: rgb = RGB(255,   0, 255); break;
		}
		SetTextColor(hdc, rgb);

		/* fill in the bottom pixel */
		SaveDC(hdc);
		yLine++;
		IntersectClipRect(hdc, rcUpdate.left, rcUpdate.top,
			rcUpdate.right, rcUpdate.bottom);
#ifdef _WIN32
		ExtTextOutA(hdc, rc.right / 2, rc.bottom - yLine,
			ETO_OPAQUE, &rcUpdate,
			achLine, lstrlenA(achLine), NULL);
#else
		ExtTextOut(hdc, rc.right / 2, rc.bottom - yLine,
			ETO_OPAQUE, &rcUpdate,
			achLine, lstrlen(achLine), NULL);
#endif
		RestoreDC(hdc, -1);
	}

	ReleaseDC(hwnd, hdc);
	ReleaseCapture();
	UnlockResource(hResData);
	FreeResource(hResData);
	InvalidateRect(hwnd, NULL, TRUE);
}
#endif



//
// Obey the registry default sizing of Zoom by 2 and Fixed screen %.
// Takes a Rect and either zooms it by 2 or replaces it with a constant size
// or leaves it alone.
//
void FAR PASCAL AlterRectUsingDefaults(NPMCIGRAPHIC npMCI, LPRECT lprc)
{
	if (npMCI->dwOptionFlags & MCIAVIO_ZOOMBY2) {
	    SetRect(lprc, 0, 0, lprc->right*2, lprc->bottom*2);
	} else {
		if (npMCI->dwOptionFlags & MCIAVIO_WINDOWSIZEMASK) {
			lprc->right = GetSystemMetrics (SM_CXSCREEN);
		lprc->bottom = GetSystemMetrics (SM_CYSCREEN);
			
			switch(npMCI->dwOptionFlags & MCIAVIO_WINDOWSIZEMASK)
			{
			case MCIAVIO_1QSCREENSIZE:
				SetRect(lprc, 0, 0, lprc->right/4, lprc->bottom/4);
				break;
			
			case MCIAVIO_2QSCREENSIZE:
				SetRect(lprc, 0, 0, lprc->right/2, lprc->bottom/2);
				break;
		    
			case MCIAVIO_3QSCREENSIZE:
				SetRect(lprc, 0, 0, lprc->right*3/4, lprc->bottom*3/4);
				break;
		    
			case MCIAVIO_MAXWINDOWSIZE:
				SetRect(lprc, 0, 0, lprc->right, lprc->bottom);
				break;
			}
		}
	}
}


// Set the size of the default window to be the rcMovie size (default
// destination size).  Keep in mind a top level window might grow off screen,
// so adjust the position so that it remains on screen.
void FAR PASCAL SetWindowToDefaultSize(NPMCIGRAPHIC npMCI, BOOL fUseDefaultSizing)
{
    RECT                        rc, rcW;
    int                         xScreen, yScreen, x, y;
    WINDOWPLACEMENT wp;

    wp.length = sizeof(wp);

    if (npMCI->hwndPlayback && npMCI->hwndPlayback == npMCI->hwndDefault) {

		// Get the size of the movie, maybe alter it if the configure options
		// tell us to play zoomed or fullscreen or something, and adjust for
		// non-client area.
		//
	
		rc = npMCI->rcMovie;
		if (fUseDefaultSizing)
			AlterRectUsingDefaults(npMCI, &rc);
	AdjustWindowRect(&rc, GetWindowLong(npMCI->hwndPlayback, GWL_STYLE), FALSE);

		
		// For top-level windows, get the position where the playback window is
		// (or will be) on the screen.  Make it fit on the screen if possible.
		// Dorking with the position of the default window if it's a
		// child window is a bad idea.  First of all, SetWindowPos is going to
		// position it relative to its parent, and these calculations figure
		// out where we want it in screen coordinates.  Second of all, trying
		// to move a child so that it's on screen when the parent could be
		// offscreen itself or hiding the child window is just asking for
		// trouble.
		//
		
		if (!(GetWindowLong(npMCI->hwndPlayback, GWL_STYLE) & WS_CHILD)) {
			if (IsIconic(npMCI->hwndPlayback)) {
				GetWindowPlacement(npMCI->hwndPlayback, &wp);
				rcW = wp.rcNormalPosition;
			} else {
				GetWindowRect(npMCI->hwndPlayback, &rcW);
			}

			rcW.right = rcW.left + rc.right - rc.left;
			rcW.bottom = rcW.top + rc.bottom - rc.top;
			xScreen = GetSystemMetrics(SM_CXSCREEN);
	    yScreen = GetSystemMetrics(SM_CYSCREEN);
			
			if (rcW.right > xScreen) {
				x = min(rcW.left, rcW.right - xScreen);
				rcW.left -= x;
				rcW.right -= x;
			}

			if (rcW.bottom > yScreen) {
				y = min(rcW.top, rcW.bottom - yScreen);
				rcW.top -= y;
				rcW.bottom -= y;
			}

			if (IsIconic(npMCI->hwndPlayback)) {
				wp.rcNormalPosition = rcW;
				SetWindowPlacement(npMCI->hwndPlayback, &wp);
			} else {
				SetWindowPos(npMCI->hwndPlayback, NULL, rcW.left, rcW.top,
						rcW.right - rcW.left, rcW.bottom - rcW.top,
						    SWP_NOZORDER | SWP_NOACTIVATE);
			}

			// For a child window, we don't move it, we just size it.
			//
		} else {
			if (IsIconic(npMCI->hwndPlayback)) {
				GetWindowPlacement(npMCI->hwndPlayback, &wp);
				wp.rcNormalPosition.right = wp.rcNormalPosition.left +
											(rc.right - rc.left);
				wp.rcNormalPosition.bottom = wp.rcNormalPosition.top +
											(rc.bottom - rc.top);
				SetWindowPlacement(npMCI->hwndPlayback, &wp);
			} else {
				SetWindowPos(npMCI->hwndPlayback, NULL, 0, 0,
							rc.right - rc.left, rc.bottom - rc.top,
							SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
			}
		}
    }
}

void FAR PASCAL Winproc_DestRect(NPMCIGRAPHIC npMCI, BOOL fUseDefaultSizing)
{
    RECT    rc;

    /* WM_SIZE messages (on NT at least) are sometimes sent
     * during CreateWindow processing (eg if the initial window size
     * is not CW_DEFAULT). Some fields in npMCI are only filled in
     * after CreateWindow has returned. So there is a danger that at this
     * point some fields are not valid.
     */

    if (npMCI->hwndPlayback &&
	npMCI->hwndPlayback == npMCI->hwndDefault &&
	(npMCI->dwOptionFlags & MCIAVIO_STRETCHTOWINDOW)) {
	GetClientRect(npMCI->hwndPlayback, &rc);
    }

    // Only allow ZOOMBY2 and fixed % defaults for our default playback window
    else if (npMCI->streams > 0 && npMCI->hwndPlayback == npMCI->hwndDefault) { 
		rc = npMCI->rcMovie;

		// Note: irrelevant on winproc thread
		//if (fUseDefaultSizing)
		//      AlterRectUsingDefaults(npMCI, &rc);

    }
    else {
	return;
    }

    if (!IsRectEmpty(&rc)) {
		WinCritCheckIn(npMCI);
		WinProcRequestEx(npMCI, WINPROC_RESETDEST);  // faster form... we have the critical section
    }
}

#ifdef _WIN32
    #define DWORG POINT
    #define GETDCORG(hdc, dwOrg)  GetDCOrgEx(hdc, &dwOrg)
#else
    #define DWORG DWORD
    #define GETDCORG(hdc, dwOrg)  dwOrg = GetDCOrg(hdc)
#endif

void CheckWindowMove(NPMCIGRAPHIC npMCI, BOOL fForce)
{
    DWORG   dwOrg;
    UINT    wRgn;
    HDC     hdc;
    RECT    rc;
    BOOL    fNull;
    BOOL    fGetDC;

    if (!(npMCI->dwFlags & MCIAVI_WANTMOVE))
	return;

    if (!npMCI->hicDraw || !npMCI->hwndPlayback || npMCI->nVideoStreams == 0)
	return;

    Assert(IsWindow(npMCI->hwndPlayback));
    Assert(npMCI->paStreamInfo);
    Assert(npMCI->nVideoStreams > 0);

    //
    //  when the screen is locked for update by a window move operation
    //  we dont want to turn off the video.
    //
    //  we can tell if the screen is locked by checking a DC to the screen.
    //
    hdc = GetDC(NULL);
    fNull = GetClipBox(hdc, &rc);
    ReleaseDC(NULL, hdc);

    if (NULLREGION == fNull)
    {
	npMCI->wRgnType = (UINT) -1;
	return;
    }

    if (fForce)
	npMCI->wRgnType = (UINT) -1;


    // sync worker thread/winproc thread interaction
    EnterHDCCrit(npMCI);

    if (fGetDC = (npMCI->hdc == NULL)) {
	hdc = GetDC (npMCI->hwndPlayback);
    } else {
	hdc = npMCI->hdc;
    }

    wRgn = GetClipBox(hdc, &rc);

    GETDCORG(hdc, dwOrg);

    if (fGetDC)
	ReleaseDC(npMCI->hwndPlayback, hdc);

    if (wRgn == npMCI->wRgnType &&
#ifdef _WIN32
	dwOrg.x == npMCI->dwOrg.x &&
	dwOrg.y == npMCI->dwOrg.y &&
#else
	dwOrg == npMCI->dwOrg &&
#endif
	EqualRect(&rc, &npMCI->rcClip)) {

	LeaveHDCCrit(npMCI);
	return;
    }

    npMCI->wRgnType = wRgn;
    npMCI->dwOrg    = dwOrg;
    npMCI->rcClip   = rc;

    rc = npMCI->psiVideo->rcDest;
    ClientToScreen(npMCI->hwndPlayback, (LPPOINT)&rc);
    ClientToScreen(npMCI->hwndPlayback, (LPPOINT)&rc+1);

    if (wRgn == NULLREGION)
	SetRectEmpty(&rc);

    DPF2(("Sending ICM_DRAW_WINDOW message Rgn=%d, Org=(%d,%d) [%d, %d, %d, %d]\n", wRgn, dwOrg, rc));

    if (ICDrawWindow(npMCI->hicDraw, &rc) != ICERR_OK) {
	DPF2(("Draw device does not want ICM_DRAW_WINDOW messages!\n"));
	npMCI->dwFlags &= ~MCIAVI_WANTMOVE;
    }
    LeaveHDCCrit(npMCI);
}
