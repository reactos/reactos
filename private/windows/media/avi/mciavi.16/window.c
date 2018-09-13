/******************************************************************************

   Copyright (C) Microsoft Corporation 1985-1991. All rights reserved.

   Title:   window.c - Multimedia Systems Media Control Interface
            driver for AVI.

*****************************************************************************/
#include "graphic.h"

#include "avitask.h"	// for TASKIDLE

//#define IDM_CONFIG              0x100
//#define IDM_SKIPFRAMES          0x110
#define IDM_MUTE                0x120
#define IDM_STRETCH             0x130

#ifdef WIN32
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

#ifdef WIN32
/*
 * de-register the class on unloading the dll so that we can
 * successfully re-register the class next time we are loaded.
 * note that nt only unregisters a class when the app exits.
 */
BOOL NEAR PASCAL GraphicWindowFree(void)
{
	return(UnregisterClass(szClassName, ghModule));
}
#endif

DWORD FAR PASCAL GraphicConfig(NPMCIGRAPHIC npMCI, DWORD dwFlags);

#if 0
static void NEAR PASCAL Credits(HWND hwnd);
#endif

long FAR PASCAL _LOADDS GraphicWndProc (HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT		ps;
    NPMCIGRAPHIC	npMCI;
    HMENU               hmenu;
    HDC                 hdc;
    RECT                rc;
    MINMAXINFO FAR *    lpmmi;
    TCHAR                ach[80];

#ifndef WIN32
    WORD ww;

    ww = GetWindowWord (hwnd, 0);
#else
    DWORD ww;
    ww = GetWindowLong (hwnd, 0);
#endif

    if ((ww == 0) && (wMsg != WM_CREATE)) {
	DPF(("null npMCI in windowproc!"));
        return DefWindowProc(hwnd, wMsg, wParam, lParam);
    }

    npMCI = (NPMCIGRAPHIC)ww;


    if (npMCI) {
	EnterCrit(npMCI);
    }

    switch (wMsg)
        {

        case WM_CREATE:

            npMCI = (NPMCIGRAPHIC)(UINT)(DWORD)
			    ((LPCREATESTRUCT)lParam)->lpCreateParams;

	    EnterCrit(npMCI);

#ifndef WIN32
            SetWindowWord (hwnd, 0, (WORD)npMCI);
#else
            SetWindowLong (hwnd, 0, (UINT)npMCI);
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
                LoadString(ghModule, MCIAVI_MENU_CONFIG, ach, sizeof(ach)/sizeof(TCHAR));
                AppendMenu(hmenu, MF_STRING, IDM_CONFIG, ach);
#endif

                LoadString(ghModule, MCIAVI_MENU_STRETCH, ach, sizeof(ach)/sizeof(TCHAR));
                AppendMenu(hmenu, MF_STRING, IDM_STRETCH, ach);

                LoadString(ghModule, MCIAVI_MENU_MUTE, ach, sizeof(ach)/sizeof(TCHAR));
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
		LeaveCrit(npMCI);  // Must not hold while in DefWindowProc
		lParam = DefWindowProc(hwnd, wMsg, wParam, lParam);
                gfEvilSysMenu--;
		return lParam;
		
#ifdef IDM_SKIPFRAMES
	    case IDM_SKIPFRAMES:
		npMCI->dwOptionFlags ^= MCIAVIO_SKIPFRAMES;
                break;
#endif
	    case IDM_STRETCH:
		npMCI->dwOptionFlags ^= MCIAVIO_STRETCHTOWINDOW;
		
		if (!(npMCI->dwOptionFlags & MCIAVIO_STRETCHTOWINDOW)) {
		    SetWindowToDefaultSize(npMCI);
                }
		
		ResetDestRect(npMCI);
                break;

            case IDM_MUTE:
                DeviceMute(npMCI, (npMCI->dwFlags & MCIAVI_PLAYAUDIO) != 0);
		break;

#ifdef IDM_CONFIG
	    case IDM_CONFIG:
		npMCI->wMessageCurrent = MCI_CONFIGURE;
                gfEvil++;
		GraphicConfig(npMCI, 0L);
                gfEvil--;
		npMCI->wMessageCurrent = 0;
                break;
#endif
            }
            break;

        case WM_CLOSE:

            // Hide default window

            DeviceStop(npMCI, MCI_WAIT);
            ShowWindow(hwnd, SW_HIDE);
            LeaveCrit(npMCI);
            return 0L;

        case WM_DESTROY:

            // The window may be destroyed 2 ways.
            //  a. the device is closed. In this case the animation is
            //  freed in DeviceClose which is called from GraphicClose
            //  and the animation ID is NULL by the time this window is
            //  destroyed.
            //  b. the window is closed. In this case, the animation is
            //  not closed and we should set the stage to NULL. A new
            //  default window will be created if needed.

            if (IsTask(npMCI->hTask)) {
                DeviceStop(npMCI, MCI_WAIT);
            }
	    if (npMCI->hwnd == npMCI->hwndDefault)
		npMCI->hwnd = NULL;
	    npMCI->hwndDefault = NULL;
            break;

        case WM_ERASEBKGND:
            hdc = (HDC) wParam;

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

	    /* Hack: if we're in a WAIT state, we won't get
            ** a WM_PAINT, so we need to invalidate the streams here
            */
            GetClipBox(hdc, &rc);
            StreamInvalidate(npMCI, &rc);
	
            LeaveCrit(npMCI);
            return 0L;

        case WM_PAINT:

#ifdef WIN32
	    /*
	     * on NT we have to poll more often to avoid deadlock between
	     * threads (a SetWindowPos call on one thread will cause
	     * the window-creating thread to issue the WM_SIZE message -
	     * synchronously). The side effect of this is that we poll
	     * for messages at times when it is not safe to process all
	     * messages.
             *
             * So unless we know it is safe to paint, we punt...
	     */
	    //if (npMCI->wTaskState != TASKIDLE)
            if ((npMCI->wTaskState != TASKIDLE) && (npMCI->wTaskState != TASKPAUSED))
            {
                npMCI->dwFlags |= MCIAVI_NEEDUPDATE;
                DPF0(("Punting on painting, wTaskState = %x", npMCI->wTaskState));
		break;
	    }
#endif
            hdc = BeginPaint(hwnd, &ps);
	
            GetClientRect(hwnd, &rc);

	    /* If updating fails, paint gray. */	
            if (DeviceUpdate(npMCI, MCI_DGV_UPDATE_PAINT, hdc, &ps.rcPaint)
			== MCIERR_DEVICE_NOT_READY) {
		GetClientRect(hwnd, &rc);
                FillRect(hdc, &rc, GetStockObject(DKGRAY_BRUSH));
	    }
            EndPaint(hwnd, &ps);
            return 0L;
	
	case WM_PALETTECHANGED:

	    // We're not using the default window.  We have no business here.
	    if (npMCI->hwnd != hwnd)
		break;

	    //
	    // someone has realized a palette - so we need to re-realize our
	    // palette (note that this will also cause drawdib to
	    // check for PAL_INDICES vs PAL_COLOURS.
	    //
	    if ((HWND) wParam != hwnd) {
		DeviceRealize(npMCI);
                InvalidateRect(hwnd, NULL, FALSE);
	    }
	    break;
	
	case WM_QUERYNEWPALETTE:

	    // We're not using the default window.  We have no business here.
	    if (npMCI->hwnd != hwnd)
		break;

            LeaveCrit(npMCI);     // tomor -- maybe this should be after?
            return DeviceRealize(npMCI);

        case WM_WINDOWPOSCHANGED:
            CheckWindowMove(npMCI, TRUE);
            break;

#ifdef WM_AVISWP
	case WM_AVISWP:
        {
            long res;
            res =  SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, lParam);
            LeaveCrit(npMCI);
            return(res);
        }
#endif

	case WM_SIZE:
            ResetDestRect(npMCI);
	    break;
	
        case WM_QUERYENDSESSION:
            DeviceStop(npMCI, MCI_WAIT);
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
        case WM_ACTIVATE:
            DeviceSetActive(npMCI, (BOOL)wParam);
            break;

        case WM_AUDIO_ON:
            Assert(npMCI->dwFlags & MCIAVI_PLAYAUDIO);
            Assert(npMCI->dwFlags & MCIAVI_LOSTAUDIO);
            Assert(npMCI->hWave == NULL);

            npMCI->dwFlags &= ~MCIAVI_PLAYAUDIO;
            DeviceMute(npMCI, FALSE);
            break;

        case WM_AUDIO_OFF:
            Assert(npMCI->dwFlags & MCIAVI_PLAYAUDIO);
            Assert(!(npMCI->dwFlags & MCIAVI_LOSTAUDIO));
            Assert(npMCI->hWave != NULL);

            DeviceMute(npMCI, TRUE);

            npMCI->dwFlags |= MCIAVI_LOSTAUDIO;
            npMCI->dwFlags |= MCIAVI_PLAYAUDIO;
            break;

#if 0
	case WM_LBUTTONDOWN:
	    {
		DWORD	dw;
		static DWORD dwLastClick;
		static DWORD dwClicks = 0;
		#define MAX_CLICKS	7
		/*     . = (0,300)  - = (300,1000)  word = (500,1500)	*/
		/*     AVI:   .-    ...-   ..				*/
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
		    DeviceStop(npMCI, MCI_WAIT);
		    Credits(hwnd);
		    dwClicks = 0;
		}
	    }
#endif
        }

   if (npMCI) {
	LeaveCrit(npMCI);
   }

   return DefWindowProc(hwnd, wMsg, wParam, lParam);
}

#if 0
static void NEAR PASCAL Credits(HWND hwnd)
{
	/*  Credits...  */
	RECT		rc;
	RECT		rcUpdate;
	HDC		hdc;
	MSG		msg;
	int		dyLine;
	int		yLine;
	TEXTMETRIC	tm;
	DWORD		dwNextTime;
	long		lScroll;
	DWORD		rgb;
	HANDLE		hResInfo;
	HANDLE		hResData;
	LPSTR		pchSrc, pchDst;
	char		achLine[100];
	int		iEncrypt;

	#define EOFCHAR	'@'		// end of credits file

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
	dwNextTime = GetCurrentTime();	// time to do the next scroll
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
			break;			// exit on key hit

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
				break;		// exit on click
		}

		/* scroll at a fixed no. of vertical pixels per sec. */
		if (dwNextTime > GetCurrentTime())
			continue;
		dwNextTime += 50L;	// millseconds per scroll

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
				break;		// no more lines
			*pchDst = 0;		// null-terminate
			pchSrc++, iEncrypt++;	// skip '\n'
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
#ifdef WIN32
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

void FAR PASCAL SetWindowToDefaultSize(NPMCIGRAPHIC npMCI)
{
    RECT rc;

    if (npMCI->hwnd && npMCI->hwnd == npMCI->hwndDefault) {
        rc = npMCI->rcMovie;

	if (npMCI->dwOptionFlags & MCIAVIO_ZOOMBY2)
	    SetRect(&rc, 0, 0, rc.right*2, rc.bottom*2);

        AdjustWindowRect(&rc, GetWindowLong(npMCI->hwnd, GWL_STYLE), FALSE);

	if (IsIconic(npMCI->hwnd)) {
	    WINDOWPLACEMENT wp;
	    wp.length = sizeof(wp);
	    GetWindowPlacement(npMCI->hwnd, &wp);
	    wp.rcNormalPosition.right = wp.rcNormalPosition.left +
					    (rc.right - rc.left);
	    wp.rcNormalPosition.bottom = wp.rcNormalPosition.top +
					    (rc.bottom - rc.top);
	    SetWindowPlacement(npMCI->hwnd, &wp);
	} else {
	    SetWindowPos(npMCI->hwnd, NULL, 0, 0,
                        rc.right - rc.left, rc.bottom - rc.top,
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
	}
    }
}

void FAR PASCAL ResetDestRect(NPMCIGRAPHIC npMCI)
{
    RECT    rc;

    /* WM_SIZE messages (on NT at least) are sometimes sent
     * during CreateWindow processing (eg if the initial window size
     * is not CW_DEFAULT). Some fields in npMCI are only filled in
     * after CreateWindow has returned. So there is a danger that at this
     * point some fields are not valid.
     */

    if (npMCI->hwnd &&
        npMCI->hwnd == npMCI->hwndDefault &&
        (npMCI->dwOptionFlags & MCIAVIO_STRETCHTOWINDOW)) {
        GetClientRect(npMCI->hwnd, &rc);
    }

    else if (npMCI->streams > 0) {
        rc = npMCI->rcMovie;

        if (npMCI->dwOptionFlags & MCIAVIO_ZOOMBY2) {
            rc.right *= 2;
            rc.bottom *= 2;
        }
    }
    else {
        return;
    }

    if (!IsRectEmpty(&rc))
        DevicePut(npMCI, &rc, MCI_DGV_PUT_DESTINATION);
}

void CheckWindowMove(NPMCIGRAPHIC npMCI, BOOL fForce)
{
#ifdef WIN32
    POINT   dwOrg;
#else
    DWORD   dwOrg;
#endif
    UINT    wRgn;
    HDC     hdc;
    RECT    rc;
    BOOL    f;
    BOOL    fGetDC;

    if (!(npMCI->dwFlags & MCIAVI_WANTMOVE))
        return;

    if (!npMCI->hicDraw || !npMCI->hwnd || npMCI->nVideoStreams == 0)
        return;

    Assert(IsWindow(npMCI->hwnd));
    Assert(npMCI->paStreamInfo);
    Assert(npMCI->nVideoStreams > 0);

    //
    //  when the screen is locked for update by a window move operation
    //  we dont want to turn off the video.
    //
    //  we can tell if the screen is locked by checking a DC to the screen.
    //
    hdc = GetDC(NULL);
    f = GetClipBox(hdc, &rc) == NULLREGION;
    ReleaseDC(NULL, hdc);

    if (f)
    {
        npMCI->wRgnType = (UINT) -1;
        return;
    }

    if (fForce)
        npMCI->wRgnType = (UINT) -1;

    if (fGetDC = (npMCI->hdc == NULL))
        hdc = GetDC (npMCI->hwnd);
    else
        hdc = npMCI->hdc;

    wRgn = GetClipBox(hdc, &rc);
#ifdef WIN32
    GetDCOrgEx(hdc, &dwOrg);
#else
    dwOrg = GetDCOrg(hdc);
#endif

    if (fGetDC)
        ReleaseDC(npMCI->hwnd, hdc);

    if (wRgn == npMCI->wRgnType &&
#ifdef WIN32
        dwOrg.x == npMCI->dwOrg.x &&
        dwOrg.y == npMCI->dwOrg.y &&
#else
        dwOrg == npMCI->dwOrg &&
#endif
        EqualRect(&rc, &npMCI->rcClip))
        return;

    npMCI->wRgnType = wRgn;
    npMCI->dwOrg    = dwOrg;
    npMCI->rcClip   = rc;

    rc = npMCI->psiVideo->rcDest;
    ClientToScreen(npMCI->hwnd, (LPPOINT)&rc);
    ClientToScreen(npMCI->hwnd, (LPPOINT)&rc+1);

    if (wRgn == NULLREGION)
        SetRectEmpty(&rc);

    DPF2(("Sending ICM_DRAW_WINDOW message Rgn=%d, Org=(%d,%d) [%d, %d, %d, %d]\n", wRgn, dwOrg, rc));

    if (ICDrawWindow(npMCI->hicDraw, &rc) != ICERR_OK) {
        DPF2(("Draw device does not want ICM_DRAW_WINDOW messages!\n"));
        npMCI->dwFlags &= ~MCIAVI_WANTMOVE;
    }
}

