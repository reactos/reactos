//===========================================================================
// Image dragging API (definitely private)
//===========================================================================
#include <windows.h>
#include <windowsx.h>

#include "autoscrl.h"
#include "common.h"
#include "debug.h"

#if 0
BOOL DAD_SetDragImage(HIMAGELIST him, POINT FAR* pptOffset);
BOOL DAD_DragEnter(HWND hwndTarget);
BOOL DAD_DragEnterEx(HWND hwndTarget, const POINT ptStart);
BOOL DAD_ShowDragImage(BOOL fShow);
BOOL DAD_DragMove(POINT pt);
BOOL DAD_DragLeave(void);
BOOL DAD_SetDragImageFromListView(HWND hwndLV, POINT ptOffset);
#endif


// -------------- auto scroll stuff --------------

BOOL _AddTimeSample(AUTO_SCROLL_DATA *pad, const POINT *ppt, DWORD dwTime)
{
    pad->pts[pad->iNextSample] = *ppt;
    pad->dwTimes[pad->iNextSample] = dwTime;

    pad->iNextSample++;

    if (pad->iNextSample == ARRAYSIZE(pad->pts))
        pad->bFull = TRUE;

    pad->iNextSample = pad->iNextSample % ARRAYSIZE(pad->pts);

    return pad->bFull;
}

#ifdef DEBUG
// for debugging, verify we have good averages
DWORD g_time = 0;
int g_distance = 0;
#endif

int _CurrentVelocity(AUTO_SCROLL_DATA *pad)
{
    int i, iStart, iNext;
    int dx, dy, distance;
    DWORD time;

    Assert(pad->bFull);

    distance = 0;
    time = 1;	// avoid div by zero

    i = iStart = pad->iNextSample % ARRAYSIZE(pad->pts);

    do {
	iNext = (i + 1) % ARRAYSIZE(pad->pts);

	dx = abs(pad->pts[i].x - pad->pts[iNext].x);
	dy = abs(pad->pts[i].y - pad->pts[iNext].y);
	distance += (dx + dy);
	time += abs(pad->dwTimes[i] - pad->dwTimes[iNext]);

	i = iNext;

    } while (i != iStart);

#ifdef DEBUG
    g_time = time;
    g_distance = distance;
#endif

    // scale this so we don't loose accuracy
    return (distance * 1024) / time;
}



// NOTE: this is duplicated in shell32.dll
//
// checks to see if we are at the end position of a scroll bar
// to avoid scrolling when not needed (avoid flashing)
//
// in:
//      code        SB_VERT or SB_HORZ
//      bDown       FALSE is up or left
//                  TRUE  is down or right

BOOL CanScroll(HWND hwnd, int code, BOOL bDown)
{
    SCROLLINFO si;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(hwnd, code, &si);

    if (bDown)
    {
    	if (si.nPage)
    	    si.nMax -= si.nPage - 1;
    	return si.nPos < si.nMax;
    }
    else
    {
    	return si.nPos > si.nMin;
    }
}

#define DSD_NONE		0x0000
#define DSD_UP			0x0001
#define DSD_DOWN		0x0002
#define DSD_LEFT		0x0004
#define DSD_RIGHT		0x0008

//---------------------------------------------------------------------------
DWORD DAD_DragScrollDirection(HWND hwnd, const POINT *ppt)
{
    RECT rcOuter, rc;
    DWORD dwDSD = DSD_NONE;	// 0
    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

    // BUGBUG: do these as globals
#define g_cxVScroll GetSystemMetrics(SM_CXVSCROLL)
#define g_cyHScroll GetSystemMetrics(SM_CYHSCROLL)
#define g_cxSmIcon  GetSystemMetrics(SM_CXSMICON)
#define g_cySmIcon  GetSystemMetrics(SM_CYSMICON)
#define g_cxIcon    GetSystemMetrics(SM_CXICON)
#define g_cyIcon    GetSystemMetrics(SM_CXICON)

    GetClientRect(hwnd, &rc);

    if (dwStyle & WS_HSCROLL)
	rc.bottom -= g_cyHScroll;

    if (dwStyle & WS_VSCROLL)
	rc.right -= g_cxVScroll;

    // the explorer forwards us drag/drop things outside of our client area
    // so we need to explictly test for that before we do things
    //
    rcOuter = rc;
    InflateRect(&rcOuter, g_cxSmIcon, g_cySmIcon);

    InflateRect(&rc, -g_cxIcon, -g_cyIcon);

    if (!PtInRect(&rc, *ppt) && PtInRect(&rcOuter, *ppt))
    {
    	// Yep - can we scroll?
    	if (dwStyle & WS_HSCROLL)
    	{
    	    if (ppt->x < rc.left)
    	    {
    	    	if (CanScroll(hwnd, SB_HORZ, FALSE))
    	    	    dwDSD |= DSD_LEFT;
    	    }
    	    else if (ppt->x > rc.right)
    	    {
    	    	if (CanScroll(hwnd, SB_HORZ, TRUE))
    	    	    dwDSD |= DSD_RIGHT;
    	    }
    	}
    	if (dwStyle & WS_VSCROLL)
    	{
    	    if (ppt->y < rc.top)
    	    {
    	    	if (CanScroll(hwnd, SB_VERT, FALSE))
    	    	    dwDSD |= DSD_UP;
    	    }
    	    else if (ppt->y > rc.bottom)
    	    {
    	    	if (CanScroll(hwnd, SB_VERT, TRUE))
    	    	    dwDSD |= DSD_DOWN;
    	    }
    	}
    }
    return dwDSD;
}


#define SCROLL_FREQUENCY	(GetDoubleClickTime()/4)	// 1 line scroll every 1/4 second
#define MIN_SCROLL_VELOCITY	20	// scaled mouse velocity

#define DAD_ShowDragImage(f)	// BUGBUG

BOOL DAD_AutoScroll(HWND hwnd, AUTO_SCROLL_DATA *pad, const POINT *pptNow)
{
    // first time we've been called, init our state
    int v;
    DWORD dwTimeNow = GetTickCount();
    DWORD dwDSD = DAD_DragScrollDirection(hwnd, pptNow);

    if (!_AddTimeSample(pad, pptNow, dwTimeNow))
	return dwDSD;

    v = _CurrentVelocity(pad);

    if (v <= MIN_SCROLL_VELOCITY)
    {
        // Nope, do some scrolling.
        if ((dwTimeNow - pad->dwLastScroll) < SCROLL_FREQUENCY)
    	    dwDSD = 0;

	if (dwDSD & (DSD_UP | DSD_DOWN | DSD_LEFT | DSD_RIGHT))
            DAD_ShowDragImage(FALSE);

        if (dwDSD & DSD_UP)
        {
            FORWARD_WM_VSCROLL(hwnd, NULL, SB_LINEUP, 1, SendMessage);
        }
        else if (dwDSD & DSD_DOWN)
        {
            FORWARD_WM_VSCROLL(hwnd, NULL, SB_LINEDOWN, 1, SendMessage);
        }
        if (dwDSD & DSD_LEFT)
        {
            FORWARD_WM_HSCROLL(hwnd, NULL, SB_LINEUP, 1, SendMessage);
        }
        else if (dwDSD & DSD_RIGHT)
        {
            FORWARD_WM_HSCROLL(hwnd, NULL, SB_LINEDOWN, 1, SendMessage);
        }

        DAD_ShowDragImage(TRUE);

	if (dwDSD)
	{
    	    DebugMsg(DM_TRACE, "v=%d", v);
	    pad->dwLastScroll = dwTimeNow;
	}
    }
    return dwDSD;	// bits set if in scroll region
}

