//----------------------------------------------------------
//
// BUGBUG: make sure this stuff really works with the DWORD
//	   ranges
//
//----------------------------------------------------------

#include "ctlspriv.h"
#include "tracki.h"
#ifndef _WIN32
#include "muldiv32.h"
#else
#define MulDiv32	MulDiv
#endif

#define THUMBSLOP  2
#define TICKHEIGHT 2

#define ABS(X)  (X >= 0) ? X : -X
#define BOUND(x,low,high)   max(min(x, high),low)
#define LONG2POINT(l, pt)  ((pt).x = (SHORT)LOWORD(l), (pt).y = (SHORT)HIWORD(l))

TCHAR aszTrackbarClassName[] = TRACKBAR_CLASS;

LPARAM FAR CALLBACK _loadds TrackBarWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

//
//  convert a logical scroll-bar position to a physical pixel position
//
static int NEAR PASCAL TBLogToPhys(PTrackBar tb, DWORD dwPos)
{
    if (tb->lLogMax == tb->lLogMin)
	return tb->rc.left;

    return (UINT)MulDiv32(dwPos - tb->lLogMin, tb->iSizePhys - 1,
	tb->lLogMax - tb->lLogMin) + tb->rc.left;
}

static LONG NEAR PASCAL TBPhysToLog(PTrackBar tb, int iPos)
{
    if (tb->iSizePhys <= 1)
	return tb->lLogMin;

    if (iPos <= tb->rc.left)
	return tb->lLogMin;

    if (iPos >= tb->rc.right)
	return tb->lLogMax;

    return MulDiv32(iPos - tb->rc.left, tb->lLogMax - tb->lLogMin,
		    tb->iSizePhys - 1) + tb->lLogMin;
}



/*
	Initialize the trackbar code.
*/

BOOL FAR PASCAL InitTrackBar(HINSTANCE hInstance)
{
    WNDCLASS wc;

    if (!GetClassInfo(hInstance, aszTrackbarClassName, &wc)) {
	// See if we must register a window class
	
	wc.lpszClassName = aszTrackbarClassName;
	wc.lpfnWndProc = (WNDPROC)TrackBarWndProc;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon = NULL;
	wc.lpszMenuName = NULL;
	wc.hbrBackground = (HBRUSH)(NULL);
	wc.hInstance = hInstance;
	wc.style = CS_GLOBALCLASS | CS_DBLCLKS;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = EXTRA_TB_BYTES;
	
	if (!RegisterClass(&wc))
		return FALSE;
    }
    return TRUE;
}


static void NEAR PASCAL PatRect(HDC hdc,int x,int y,int dx,int dy)
{
    RECT    rc;

    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}

static void NEAR PASCAL DrawTic(PTrackBar tb, int x, int yTic)
{
    SetBkColor(tb->hdc, GetSysColor(COLOR_BTNTEXT));
    PatRect(tb->hdc,(x),yTic,1,TICKHEIGHT);
    SetBkColor(tb->hdc, GetSysColor(COLOR_BTNHIGHLIGHT));
    PatRect(tb->hdc,(x)+1,yTic,1,TICKHEIGHT);
}

/* DrawTics() */
/* There is always a tick at the beginning and end of the bar, but you can */
/* add some more of your own with a TBM_SETTIC message.  This draws them.  */
/* They are kept in an array whose handle is a window word.  The first     */
/* element is the number of extra ticks, and then the positions.           */

static void NEAR PASCAL DrawTics(PTrackBar tb)
{
    PDWORD pTics;
    int    iPos;
    int    yTic;
    int    i;

    yTic = tb->rc.bottom + THUMBSLOP + 1;

// !!! Not for MCIWnd
//    DrawTic(tb, tb->rc.left, yTic);               // first
//    DrawTic(tb, tb->rc.right-1, yTic);            // last

    // those inbetween
    pTics = tb->pTics;
    if (pTics) {
	for (i = 0; i < tb->nTics; ++i) {
	    iPos = TBLogToPhys(tb,pTics[i]);
	    DrawTic(tb, iPos, yTic);
	}
    }

    // draw the selection range (triangles)

    if ((tb->Flags & TBF_SELECTION) &&
        (tb->lSelStart <= tb->lSelEnd) && (tb->lSelEnd >= tb->lLogMin)) {

	SetBkColor(tb->hdc, GetSysColor(COLOR_BTNTEXT));

	iPos = TBLogToPhys(tb,tb->lSelStart);

	for (i=0; i < TICKHEIGHT; i++)
	    PatRect(tb->hdc,iPos-i,yTic+i,1,TICKHEIGHT-i);

	iPos = TBLogToPhys(tb,tb->lSelEnd);

	for (i=0; i < TICKHEIGHT; i++)
	    PatRect(tb->hdc,iPos+i,yTic+i,1,TICKHEIGHT-i);
    }

// !!! Not for MCIWnd
//    line across the bottom
//    SetBkColor(tb->hdc, GetSysColor(COLOR_BTNTEXT));
//    PatRect(tb->hdc, tb->rc.left, yTic+TICKHEIGHT,tb->iSizePhys,1);
//
//    SetBkColor(tb->hdc, GetSysColor(COLOR_BTNHIGHLIGHT));
//    PatRect(tb->hdc, tb->rc.left, yTic+TICKHEIGHT+1,tb->iSizePhys,1);

}

/* This draws the track bar itself */

static void NEAR PASCAL DrawChannel(PTrackBar tb)
{
    HBRUSH hbrTemp;

    // draw the frame around the window
    SetBkColor(tb->hdc, GetSysColor(COLOR_WINDOWFRAME));

    PatRect(tb->hdc, tb->rc.left, tb->rc.top,      tb->iSizePhys, 1);
    PatRect(tb->hdc, tb->rc.left, tb->rc.bottom-2, tb->iSizePhys, 1);
    PatRect(tb->hdc, tb->rc.left, tb->rc.top,      1, tb->rc.bottom-tb->rc.top-1);
    PatRect(tb->hdc, tb->rc.right-1, tb->rc.top, 1, tb->rc.bottom-tb->rc.top-1);

    SetBkColor(tb->hdc, GetSysColor(COLOR_BTNHIGHLIGHT));
    PatRect(tb->hdc, tb->rc.left, tb->rc.bottom-1, tb->iSizePhys, 1);

    SetBkColor(tb->hdc, GetSysColor(COLOR_BTNSHADOW));
    PatRect(tb->hdc, tb->rc.left+1, tb->rc.top + 1, tb->iSizePhys-2,1);

    // draw the background in dither gray
    hbrTemp = SelectObject(tb->hdc, hbrDither);
    if (hbrTemp) {
        PatBlt(tb->hdc, tb->rc.left+1, tb->rc.top + 2,
            tb->iSizePhys-2, tb->rc.bottom-tb->rc.top-4, PATCOPY);
        SelectObject(tb->hdc, hbrTemp);
    }

    // now highlight the selection range
    if ((tb->Flags & TBF_SELECTION) &&
        (tb->lSelStart <= tb->lSelEnd) && (tb->lSelEnd > tb->lLogMin)) {
	int iStart, iEnd;

	iStart = TBLogToPhys(tb,tb->lSelStart);
	iEnd   = TBLogToPhys(tb,tb->lSelEnd);

        SetBkColor(tb->hdc, GetSysColor(COLOR_BTNTEXT));
	PatRect(tb->hdc, iStart,tb->rc.top+1,1,tb->rc.bottom-tb->rc.top-2);
	PatRect(tb->hdc, iEnd,  tb->rc.top+1,1,tb->rc.bottom-tb->rc.top-2);

	if (iStart + 2 <= iEnd) {
            SetBkColor(tb->hdc, GetSysColor(COLOR_BTNHIGHLIGHT));
	    PatRect(tb->hdc, iStart+1, tb->rc.top+1, iEnd-iStart-1, tb->rc.bottom-tb->rc.top-3);
	}
    }
}

static void NEAR PASCAL MoveThumb(PTrackBar tb, LONG lPos)
{
	InvalidateRect(tb->hwnd, &tb->Thumb, TRUE);

	tb->lLogPos  = BOUND(lPos,tb->lLogMin,tb->lLogMax);

	tb->Thumb.left   = TBLogToPhys(tb, tb->lLogPos) - tb->wThumbWidth/2;
	tb->Thumb.right  = tb->Thumb.left + tb->wThumbWidth;
	tb->Thumb.top    = tb->rc.top - THUMBSLOP;
	tb->Thumb.bottom = tb->rc.bottom + THUMBSLOP;

	InvalidateRect(tb->hwnd, &tb->Thumb, TRUE);
	UpdateWindow(tb->hwnd);
}


static void NEAR PASCAL DrawThumb(PTrackBar tb)
{
	HBITMAP hbmT;
	HDC     hdcT;
	int     x;

	hdcT = CreateCompatibleDC(tb->hdc);

	if( (tb->Cmd == TB_THUMBTRACK) || !IsWindowEnabled(tb->hwnd) )
	    x = tb->wThumbWidth;
	else
	    x = 0;

	hbmT = SelectObject(hdcT, hbmThumb);
        if (hbmT) {
	    BitBlt(tb->hdc,tb->Thumb.left, tb->rc.top-THUMBSLOP,
		tb->wThumbWidth, tb->wThumbHeight, hdcT, x + 2*tb->wThumbWidth, 0, SRCAND);
	    BitBlt(tb->hdc,tb->Thumb.left, tb->rc.top-THUMBSLOP,
		tb->wThumbWidth, tb->wThumbHeight, hdcT, x, 0, SRCPAINT);
        }

	SelectObject(hdcT, hbmT);
	DeleteDC(hdcT);
}

/* SetTBCaretPos() */
/* Make the caret flash in the middle of the thumb when it has the focus */

static void NEAR PASCAL SetTBCaretPos(PTrackBar tb)
{
	// We only get the caret if we have the focus.
	if (tb->hwnd == GetFocus())
		SetCaretPos(tb->Thumb.left + tb->wThumbWidth / 2,
			tb->Thumb.top + THUMBSLOP);
}

static void NEAR PASCAL DoAutoTics(PTrackBar tb)
{
    LONG NEAR *pl;
    LONG l;

    if (!(GetWindowLong(tb->hwnd, GWL_STYLE) & TBS_AUTOTICKS))
        return;

    tb->nTics = (int)(tb->lLogMax - tb->lLogMin - 1);

    // If our length is zero, we'll blow!
    if (tb->nTics <= 0) {
	tb ->nTics = 0;
	return;
    }

    if (tb->pTics)
        LocalFree((HANDLE)tb->pTics);

    tb->pTics = (DWORD NEAR *)LocalAlloc(LPTR, sizeof(DWORD) * tb->nTics);
    if (!tb->pTics) {
        tb->nTics = 0;
        return;
    }

    for (pl = (LONG NEAR *)tb->pTics, l = tb->lLogMin + 1; l < tb->lLogMax; l++)
        *pl++ = l;
}


LPARAM FAR CALLBACK _loadds TrackBarWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PTrackBar       tb;
	PAINTSTRUCT     ps;
	BITMAP          bm;
	HANDLE          h;
	HDC		hdc;
	HBRUSH		hbr;
	RECT		rc;

	tb = TrackBarLock(hwnd);

	switch (message) {
		case WM_CREATE:
			if (!CreateDitherBrush(FALSE))
				return -1;
			// Get us our window structure.
			TrackBarCreate(hwnd);
			tb = TrackBarLock(hwnd);

			tb->hwnd = hwnd;
			tb->Cmd = (UINT)-1;

			/* load the 2 thumb bitmaps (pressed and released) */
			CreateThumb(FALSE);

			GetObject(hbmThumb, sizeof(bm), &bm);

			// bitmap has up and down thumb and up&down masks
			tb->wThumbWidth  = (UINT)(bm.bmWidth/4);
			tb->wThumbHeight = (UINT)bm.bmHeight;
                        // all lLog fields are zero inited by
                        // the LocalAlloc();

			// fall through to WM_SIZE

		case WM_SIZE:
			GetClientRect(hwnd, &tb->rc);

			tb->rc.bottom  = tb->rc.top + tb->wThumbHeight - THUMBSLOP;
			tb->rc.top    += THUMBSLOP;
			tb->rc.left   += tb->wThumbWidth/2;
			tb->rc.right  -= tb->wThumbWidth/2;

			// Figure out how much room we have to move the thumb in
			//!!! -2
			tb->iSizePhys = tb->rc.right - tb->rc.left;
	
			// Elevator isn't there if there's no room.
			if (tb->iSizePhys == 0) {
				// Lost our thumb.
				tb->Flags |= TBF_NOTHUMB;
				tb->iSizePhys = 1;
			} else {
				// Ah. We have a thumb.
				tb->Flags &= ~TBF_NOTHUMB;
			}
			InvalidateRect(hwnd, NULL, TRUE);
			MoveThumb(tb, tb->lLogPos);
			break;
			
		case WM_DESTROY:
			TrackBarDestroy(hwnd);
			FreeDitherBrush();
			DestroyThumb();
			break;
	
		case WM_SETFOCUS:
			// We gots the focus. We need a caret.
	
			CreateCaret(hwnd, (HBITMAP)1,
				3, tb->wThumbHeight - 2 * THUMBSLOP);
			SetTBCaretPos(tb);
			ShowCaret(hwnd);
			break;
	
		case WM_KILLFOCUS:
			DestroyCaret();
			break;

		case WM_ERASEBKGND:
			hdc = (HDC) wParam;
#ifdef _WIN32
			hbr = (HBRUSH)(UINT)SendMessage(GetParent(hwnd),
				WM_CTLCOLORSTATIC, (WPARAM) hdc, (LPARAM)hwnd);
#else
			hbr = (HBRUSH)(UINT)SendMessage(GetParent(hwnd),
				WM_CTLCOLOR, (WPARAM) hdc,
				MAKELONG(hwnd, CTLCOLOR_STATIC);
#endif

			if (hbr) {
			    GetClientRect(hwnd, &rc);
			    FillRect(hdc, &rc, hbr);
			    return(FALSE);
			}
			return(TRUE);
			break;

		case WM_ENABLE:
			InvalidateRect(hwnd, NULL, FALSE);
			break;

		case WM_PAINT:
			if (wParam == 0)
			    tb->hdc = BeginPaint(hwnd, &ps);
			else
			    tb->hdc = (HDC)wParam;

			// Update the dither brush if necessary.
			CheckSysColors();

			DrawTics(tb);
			DrawThumb(tb);
			ExcludeClipRect(tb->hdc, tb->Thumb.left, tb->Thumb.top,
			    tb->Thumb.right, tb->Thumb.bottom);
			DrawChannel(tb);
			SetTBCaretPos(tb);
	
			if (wParam == 0)
				EndPaint(hwnd, &ps);

			tb->hdc = NULL;
			break;

		case WM_GETDLGCODE:
			return DLGC_WANTARROWS;
			break;
	
		case WM_LBUTTONDOWN:
			/* Give ourselves focus */
			// !!! MCIWnd wants to keep focus
			// SetFocus(hwnd);
			TBTrackInit(tb, lParam);
			break;
	
		case WM_LBUTTONUP:
			// We're through doing whatever we were doing with the
			// button down.
			TBTrackEnd(tb, lParam);
			break;
	
		case WM_TIMER:
			// The only way we get a timer message is if we're
			// autotracking.
			lParam = GetMessagePos();
			ScreenToClient(tb->hwnd, (LPPOINT)&lParam);

			// fall through to WM_MOUSEMOVE
	
		case WM_MOUSEMOVE:
			// We only care that the mouse is moving if we're
			// tracking the bloody thing.
			if (tb->Cmd != (UINT)-1) {
				if (GetCapture() != tb->hwnd) {
				    TBTrackEnd(tb, lParam);
				    return 0L;
				}
				TBTrack(tb, lParam);
			}
			return 0L;
			
		case WM_KEYUP:
			// If key was any of the keyboard accelerators, send end
			// track message when user up clicks on keyboard
			switch (wParam) {
				case VK_HOME:
				case VK_END:
				case VK_PRIOR:
				case VK_NEXT:
				case VK_LEFT:
				case VK_UP:
				case VK_RIGHT:
				case VK_DOWN:
					DoTrack(tb, TB_ENDTRACK, 0);
					break;
				default:
					break;
			}
			break;
	
		case WM_KEYDOWN:
			switch (wParam) {
				case VK_HOME:
					wParam = TB_TOP;
					goto KeyTrack;
	
				case VK_END:
					wParam = TB_BOTTOM;
					goto KeyTrack;
	
				case VK_PRIOR:
					wParam = TB_PAGEUP;
					goto KeyTrack;
	
				case VK_NEXT:
					wParam = TB_PAGEDOWN;
					goto KeyTrack;
	
				case VK_LEFT:
				case VK_UP:
					wParam = TB_LINEUP;
					goto KeyTrack;
	
				case VK_RIGHT:
				case VK_DOWN:
					wParam = TB_LINEDOWN;
KeyTrack:
					DoTrack(tb, wParam, 0);
					break;
	
				default:
					break;
			}
			break;

		case TBM_GETPOS:
			return tb->lLogPos;
	
		case TBM_GETSELSTART:
			return tb->lSelStart;

		case TBM_GETSELEND:
			return tb->lSelEnd;

		case TBM_GETRANGEMIN:
			return tb->lLogMin;

		case TBM_GETRANGEMAX:
			return tb->lLogMax;
	
		case TBM_GETPTICS:
			return (LONG)(LPVOID)tb->pTics;
	
		case TBM_CLEARSEL:
                        tb->Flags &= ~TBF_SELECTION;
			tb->lSelStart = -1;
			tb->lSelEnd   = -1;
			goto RedrawTB;

		case TBM_CLEARTICS:
			if (tb->pTics)
			    LocalFree((HLOCAL)tb->pTics);

			tb->nTics = 0;
			tb->pTics = NULL;
			goto RedrawTB;

		case TBM_GETTIC:

			if (tb->pTics == NULL || (int)wParam >= tb->nTics)
			    return -1L;

			return tb->pTics[wParam];

		case TBM_GETTICPOS:

			if (tb->pTics == NULL || (int)wParam >= tb->nTics)
			    return -1L;

			return TBLogToPhys(tb,tb->pTics[wParam]);

		case TBM_GETNUMTICS:
			return tb->nTics;

		case TBM_SETTIC:
			/* not a valid position */
			if (lParam < 0)
			    break;

			if (tb->pTics)
				h = LocalReAlloc((HLOCAL)tb->pTics,
				    sizeof(DWORD) * (UINT)(tb->nTics + 1),
				    LMEM_MOVEABLE | LMEM_ZEROINIT);
			else
				h = LocalAlloc(LPTR, sizeof(DWORD));

			if (h)
				tb->pTics = (PDWORD)h;
			else
				return (LONG)FALSE;

			tb->pTics[tb->nTics++] = (DWORD)lParam;

			InvalidateRect(hwnd, NULL, TRUE);
			return (LONG)TRUE;
			break;
	
		case TBM_SETPOS:
			/* Only redraw if it will physically move */
			if (wParam && TBLogToPhys(tb, lParam) !=
						TBLogToPhys(tb, tb->lLogPos))
			    MoveThumb(tb, lParam);
			else
			    tb->lLogPos = BOUND(lParam,tb->lLogMin,tb->lLogMax);
			break;

		case TBM_SETSEL:
                        tb->Flags |= TBF_SELECTION;
			tb->lSelStart = LOWORD(lParam);
			tb->lSelEnd   = HIWORD(lParam);
			if (tb->lSelEnd < tb->lSelStart)
			    tb->lSelEnd = tb->lSelStart;
			goto RedrawTB;
	
		case TBM_SETSELSTART:
                        tb->Flags |= TBF_SELECTION;
			tb->lSelStart = lParam;
			if (tb->lSelEnd < tb->lSelStart || tb->lSelEnd == -1)
			    tb->lSelEnd = tb->lSelStart;
			goto RedrawTB;
	
		case TBM_SETSELEND:
                        tb->Flags |= TBF_SELECTION;
			tb->lSelEnd   = lParam;
			if (tb->lSelStart > tb->lSelEnd || tb->lSelStart == -1)
			    tb->lSelStart = tb->lSelEnd;
			goto RedrawTB;
	
		case TBM_SETRANGE:
			tb->lLogMin = LOWORD(lParam);
			tb->lLogMax = HIWORD(lParam);
                        DoAutoTics(tb);
			goto RedrawTB;

		case TBM_SETRANGEMIN:
			tb->lLogMin = (DWORD)lParam;
			goto RedrawTB;

		case TBM_SETRANGEMAX:
			tb->lLogMax = (DWORD)lParam;
	RedrawTB:
			tb->lLogPos = BOUND(tb->lLogPos, tb->lLogMin,tb->lLogMax);
			/* Only redraw if flag says so */
			if (wParam) {
			    InvalidateRect(hwnd, NULL, TRUE);
			    MoveThumb(tb, tb->lLogPos);
			}
			break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

/* DoTrack() */

static void NEAR PASCAL DoTrack(PTrackBar tb, int cmd, DWORD dwPos)
{
	// note: we only send back a WORD worth of the position.
#ifdef _WIN32
	SendMessage(GetParent(tb->hwnd), WM_HSCROLL,
	    MAKELONG(cmd, LOWORD(dwPos)), (LPARAM)tb->hwnd);
#else
	SendMessage(GetParent(tb->hwnd), WM_HSCROLL, cmd,
	    MAKELONG(LOWORD(dwPos), tb->hwnd));
#endif
}

/* WTrackType() */

static WORD NEAR PASCAL WTrackType(PTrackBar tb, LONG lParam)
{
	POINT pt;
#ifdef _WIN32
	LONG2POINT(lParam, pt);
#else
	pt = MAKEPOINT(lParam);
#endif

	if (tb->Flags & TBF_NOTHUMB)            // If no thumb, just leave.
	    return 0;

	if (PtInRect(&tb->Thumb, pt))
	    return TB_THUMBTRACK;

	if (!PtInRect(&tb->rc, pt))
	    return 0;

	if (pt.x >= tb->Thumb.left)
	    return TB_PAGEDOWN;
	else
	    return TB_PAGEUP;
}

/* TBTrackInit() */

static void NEAR PASCAL TBTrackInit(PTrackBar tb, LONG lParam)
{
	UINT wCmd;

	if (tb->Flags & TBF_NOTHUMB)         // No thumb:  just leave.
	    return;

        wCmd = WTrackType(tb, lParam);
	if (!wCmd)
	    return;

	HideCaret(tb->hwnd);
	SetCapture(tb->hwnd);

	tb->Cmd = wCmd;
	tb->dwDragPos = (DWORD)-1;

	// Set up for auto-track (if needed).
	if (wCmd != TB_THUMBTRACK) {
		// Set our timer up
		tb->Timer = (UINT)SetTimer(tb->hwnd, TIMER_ID, REPEATTIME, NULL);
	}

	TBTrack(tb, lParam);
}

/* EndTrack() */

static void near PASCAL TBTrackEnd(PTrackBar tb, long lParam)
{
	lParam = lParam; // Just reference this variable

// 	If we lose mouse capture we need to call this
//	if (GetCapture() != tb->hwnd)
//	    return;

	// Let the mouse go.
	ReleaseCapture();

	// Decide how we're ending this thing.
	if (tb->Cmd == TB_THUMBTRACK)
		DoTrack(tb, TB_THUMBPOSITION, tb->dwDragPos);

	if (tb->Timer)
		KillTimer(tb->hwnd, TIMER_ID);

	tb->Timer = 0;

	// Always send TB_ENDTRACK message.
	DoTrack(tb, TB_ENDTRACK, 0);

	// Give the caret back.
	ShowCaret(tb->hwnd);

	// Nothing going on.
	tb->Cmd = (UINT)-1;

	MoveThumb(tb, tb->lLogPos);
}

static void NEAR PASCAL TBTrack(PTrackBar tb, LONG lParam)
{
	DWORD dwPos;

	// See if we're tracking the thumb
	if (tb->Cmd == TB_THUMBTRACK) {
		dwPos = TBPhysToLog(tb, LOWORD(lParam));

		// Tentative position changed -- notify the guy.
		if (dwPos != tb->dwDragPos) {
			tb->dwDragPos = dwPos;
			MoveThumb(tb, dwPos);
			DoTrack(tb, TB_THUMBTRACK, dwPos);
		}
	}
	else {
		if (tb->Cmd != WTrackType(tb, lParam))
		    return;

		DoTrack(tb, tb->Cmd, 0);
	}
}
