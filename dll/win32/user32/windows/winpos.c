/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/window.c
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
    style = GetWindowLongW( hwnd, GWL_STYLE );
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
STDCALL
WinPosActivateOtherWindow(HWND hwnd)
{
    HWND hwndTo, fg;

    if ((GetWindowLongW( hwnd, GWL_STYLE ) & WS_POPUP) && (hwndTo = GetWindow( hwnd, GW_OWNER )))
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



UINT STDCALL
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
HWND STDCALL
GetActiveWindow(VOID)
{
  return (HWND)NtUserGetThreadState(THREADSTATE_ACTIVEWINDOW);
}

/*
 * @implemented
 */
HWND STDCALL
SetActiveWindow(HWND hWnd)
{
  return(NtUserSetActiveWindow(hWnd));
}

/*
 * @unimplemented
 */
UINT STDCALL
ArrangeIconicWindows(HWND hWnd)
{
  return NtUserCallHwndLock( hWnd, HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS);
}
