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
  return NtUserCallHwndLock( hWnd, HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS);
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
    PWND FromWnd, ToWnd;
    POINT Delta;
    UINT i;

    FromWnd = ValidateHwndOrDesk(hWndFrom);
    if (!FromWnd)
        return 0;

    ToWnd = ValidateHwndOrDesk(hWndTo);
    if (!ToWnd)
        return 0;

    Delta.x = FromWnd->rcClient.left - ToWnd->rcClient.left;
    Delta.y = FromWnd->rcClient.top - ToWnd->rcClient.top;

    for (i = 0; i != cPoints; i++)
    {
        lpPoints[i].x += Delta.x;
        lpPoints[i].y += Delta.y;
    }

    return MAKELONG(LOWORD(Delta.x), LOWORD(Delta.y));
}


/*
 * @implemented
 */
BOOL WINAPI
ScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
    PWND Wnd, DesktopWnd;

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return FALSE;

    DesktopWnd = GetThreadDesktopWnd();

    lpPoint->x += DesktopWnd->rcClient.left - Wnd->rcClient.left;
    lpPoint->y += DesktopWnd->rcClient.top - Wnd->rcClient.top;

    return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
ClientToScreen(HWND hWnd, LPPOINT lpPoint)
{
    PWND Wnd, DesktopWnd;

    Wnd = ValidateHwnd(hWnd);
    if (!Wnd)
        return FALSE;

    DesktopWnd = GetThreadDesktopWnd();

    lpPoint->x += Wnd->rcClient.left - DesktopWnd->rcClient.left;
    lpPoint->y += Wnd->rcClient.top - DesktopWnd->rcClient.top;

    return TRUE;
}

