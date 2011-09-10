/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            dll/win32/user32/windows/winpos.c
 * PURPOSE:         Window management
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* FUNCTIONS *****************************************************************/

/*******************************************************************
 *         can_activate_window
 *
 * Check if we can activate the specified window.
 */
static BOOL can_activate_window( HWND hwnd )
{
    LONG style;

    if (!hwnd) return FALSE;
    style = GetWindowLongPtrW( hwnd, GWL_STYLE );
    if (!(style & WS_VISIBLE)) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    return !(style & WS_DISABLED);
}


/*******************************************************************
 *         WINPOS_ActivateOtherWindow
 *
 *  Activates window other than pWnd.
 */
void
WINAPI
WinPosActivateOtherWindow(HWND hwnd)
{
    HWND hwndTo, fg;

    if ((GetWindowLongPtrW( hwnd, GWL_STYLE ) & WS_POPUP) && (hwndTo = GetWindow( hwnd, GW_OWNER )))
    {
        hwndTo = GetAncestor( hwndTo, GA_ROOT );
        if (can_activate_window( hwndTo )) goto done;
    }

    hwndTo = hwnd;
    for (;;)
    {
        if (!(hwndTo = GetWindow( hwndTo, GW_HWNDNEXT ))) break;
        if (can_activate_window( hwndTo )) break;
    }

 done:
    fg = GetForegroundWindow();
    TRACE("win = %p fg = %p\n", hwndTo, fg);
    if (!fg || (hwnd == fg))
    {
        if (SetForegroundWindow( hwndTo )) return;
    }
    if (!SetActiveWindow( hwndTo )) SetActiveWindow(0);
}



UINT WINAPI
WinPosGetMinMaxInfo(HWND hWnd, POINT* MaxSize, POINT* MaxPos,
		  POINT* MinTrack, POINT* MaxTrack)
{
  MINMAXINFO MinMax;

  if(NtUserGetMinMaxInfo(hWnd, &MinMax, TRUE))
  {
    MinMax.ptMaxTrackSize.x = max(MinMax.ptMaxTrackSize.x,
				  MinMax.ptMinTrackSize.x);
    MinMax.ptMaxTrackSize.y = max(MinMax.ptMaxTrackSize.y,
				  MinMax.ptMinTrackSize.y);

    if (MaxSize) *MaxSize = MinMax.ptMaxSize;
    if (MaxPos) *MaxPos = MinMax.ptMaxPosition;
    if (MinTrack) *MinTrack = MinMax.ptMinTrackSize;
    if (MaxTrack) *MaxTrack = MinMax.ptMaxTrackSize;
  }
  return 0; //FIXME: what does it return?
}


/*
 * @implemented
 */
HWND WINAPI
GetActiveWindow(VOID)
{
  return (HWND)NtUserGetThreadState(THREADSTATE_ACTIVEWINDOW);
}


/*
 * @unimplemented
 */
UINT WINAPI
ArrangeIconicWindows(HWND hWnd)
{
  return NtUserxArrangeIconicWindows( hWnd );
}

/*
 * @implemented
 */
HWND WINAPI
WindowFromPoint(POINT Point)
{
    //TODO: Determine what the actual parameters to
    // NtUserWindowFromPoint are.
    return NtUserWindowFromPoint(Point.x, Point.y);
}


/*
 * @implemented
 */
int WINAPI
MapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints)
{
    PWND FromWnd = NULL, ToWnd = NULL;
    BOOL mirror_from, mirror_to;
    POINT Delta;
    UINT i;

    if (hWndFrom)
    {
       FromWnd = ValidateHwnd(hWndFrom);
       if (!FromWnd)
           return 0;
    }
    if (hWndTo)
    {
       ToWnd = ValidateHwnd(hWndTo);
       if (!ToWnd)
           return 0;
    }

    /* Note: Desktop Top and Left is always 0! */
    Delta.x = Delta.y = 0;
    mirror_from = mirror_to = FALSE;

    if (FromWnd && FromWnd->fnid != FNID_DESKTOP)
    {
       if (FromWnd->ExStyle & WS_EX_LAYOUTRTL)
       {
          mirror_from = TRUE;
          Delta.x = FromWnd->rcClient.right - FromWnd->rcClient.left;
       }
       else
          Delta.x = FromWnd->rcClient.left;
       Delta.y = FromWnd->rcClient.top;
    }

    if (ToWnd && ToWnd->fnid != FNID_DESKTOP)
    {
       if (ToWnd->ExStyle & WS_EX_LAYOUTRTL)
       {
          mirror_to = TRUE;
          Delta.x -= ToWnd->rcClient.right - ToWnd->rcClient.left;
       }
       else
          Delta.x -= ToWnd->rcClient.left;
       Delta.y -= ToWnd->rcClient.top;
    }

    if (mirror_from) Delta.x = -Delta.x;

    for (i = 0; i != cPoints; i++)
    {
        lpPoints[i].x += Delta.x;
        lpPoints[i].y += Delta.y;
        if (mirror_from || mirror_to) lpPoints[i].x = -lpPoints[i].x;
    }

    if ((mirror_from || mirror_to) && cPoints == 2)  /* special case for rectangle */
    {
       int tmp = lpPoints[0].x;
       lpPoints[0].x = lpPoints[1].x;
       lpPoints[1].x = tmp;
    }

    return MAKELONG(LOWORD(Delta.x), LOWORD(Delta.y));
}


/*
 * @implemented
 */
BOOL WINAPI
ScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
    PWND Wnd;
    /* Note: Desktop Top and Left is always 0! */
    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return FALSE;

    if (Wnd->fnid != FNID_DESKTOP)
    {
       if (Wnd->ExStyle & WS_EX_LAYOUTRTL)
          lpPoint->x = Wnd->rcClient.right - lpPoint->x;
       else
          lpPoint->x -= Wnd->rcClient.left;
       lpPoint->y -= Wnd->rcClient.top;
    }
    return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
ClientToScreen(HWND hWnd, LPPOINT lpPoint)
{
    PWND Wnd;
    /* Note: Desktop Top and Left is always 0! */
    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return FALSE;

    if (Wnd->fnid != FNID_DESKTOP)
    {
       if (Wnd->ExStyle & WS_EX_LAYOUTRTL)
          lpPoint->x = Wnd->rcClient.right - lpPoint->x;
       else
          lpPoint->x += Wnd->rcClient.left;
       lpPoint->y += Wnd->rcClient.top;
    }
    return TRUE;
}

