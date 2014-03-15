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

void mirror_rect( const RECT *window_rect, RECT *rect )
{
    int width = window_rect->right - window_rect->left;
    int tmp = rect->left;
    rect->left = width - rect->right;
    rect->right = width - tmp;
}

/* FUNCTIONS *****************************************************************/

#define EMPTYPOINT(pt) ((pt).x == -1 && (pt).y == -1)

UINT WINAPI
WinPosGetMinMaxInfo(HWND hwnd, POINT* maxSize, POINT* maxPos,
		  POINT* minTrack, POINT* maxTrack)
{
    MINMAXINFO MinMax;
    HMONITOR monitor;
    INT xinc, yinc;
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    LONG adjustedStyle;
    LONG exstyle = GetWindowLongW( hwnd, GWL_EXSTYLE );
    RECT rc;
    WND *win;

    /* Compute default values */

    GetWindowRect(hwnd, &rc);
    MinMax.ptReserved.x = rc.left;
    MinMax.ptReserved.y = rc.top;

    if ((style & WS_CAPTION) == WS_CAPTION)
        adjustedStyle = style & ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
    else
        adjustedStyle = style;

    GetClientRect(GetAncestor(hwnd,GA_PARENT), &rc);
    AdjustWindowRectEx(&rc, adjustedStyle, ((style & WS_POPUP) && GetMenu(hwnd)), exstyle);

    xinc = -rc.left;
    yinc = -rc.top;

    MinMax.ptMaxSize.x = rc.right - rc.left;
    MinMax.ptMaxSize.y = rc.bottom - rc.top;
    if (style & (WS_DLGFRAME | WS_BORDER))
    {
        MinMax.ptMinTrackSize.x = GetSystemMetrics(SM_CXMINTRACK);
        MinMax.ptMinTrackSize.y = GetSystemMetrics(SM_CYMINTRACK);
    }
    else
    {
        MinMax.ptMinTrackSize.x = 2 * xinc;
        MinMax.ptMinTrackSize.y = 2 * yinc;
    }
    MinMax.ptMaxTrackSize.x = GetSystemMetrics(SM_CXMAXTRACK);
    MinMax.ptMaxTrackSize.y = GetSystemMetrics(SM_CYMAXTRACK);
    MinMax.ptMaxPosition.x = -xinc;
    MinMax.ptMaxPosition.y = -yinc;

    if ((win = ValidateHwnd( hwnd )) )//&& win != WND_DESKTOP && win != WND_OTHER_PROCESS)
    {
        if (!EMPTYPOINT(win->InternalPos.MaxPos)) MinMax.ptMaxPosition = win->InternalPos.MaxPos;
    }

    SendMessageW( hwnd, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax );

    /* if the app didn't change the values, adapt them for the current monitor */

    if ((monitor = MonitorFromWindow( hwnd, MONITOR_DEFAULTTOPRIMARY )))
    {
        RECT rc_work;
        MONITORINFO mon_info;

        mon_info.cbSize = sizeof(mon_info);
        GetMonitorInfoW( monitor, &mon_info );

        rc_work = mon_info.rcMonitor;

        if (style & WS_MAXIMIZEBOX)
        {
            if ((style & WS_CAPTION) == WS_CAPTION || !(style & (WS_CHILD | WS_POPUP)))
                rc_work = mon_info.rcWork;
        }

        if (MinMax.ptMaxSize.x == GetSystemMetrics(SM_CXSCREEN) + 2 * xinc &&
            MinMax.ptMaxSize.y == GetSystemMetrics(SM_CYSCREEN) + 2 * yinc)
        {
            MinMax.ptMaxSize.x = (rc_work.right - rc_work.left) + 2 * xinc;
            MinMax.ptMaxSize.y = (rc_work.bottom - rc_work.top) + 2 * yinc;
        }
        if (MinMax.ptMaxPosition.x == -xinc && MinMax.ptMaxPosition.y == -yinc)
        {
            MinMax.ptMaxPosition.x = rc_work.left - xinc;
            MinMax.ptMaxPosition.y = rc_work.top - yinc;
        }
    }

      /* Some sanity checks */

    TRACE("%d %d / %d %d / %d %d / %d %d\n",
                      MinMax.ptMaxSize.x, MinMax.ptMaxSize.y,
                      MinMax.ptMaxPosition.x, MinMax.ptMaxPosition.y,
                      MinMax.ptMaxTrackSize.x, MinMax.ptMaxTrackSize.y,
                      MinMax.ptMinTrackSize.x, MinMax.ptMinTrackSize.y);
    MinMax.ptMaxTrackSize.x = max( MinMax.ptMaxTrackSize.x,
                                   MinMax.ptMinTrackSize.x );
    MinMax.ptMaxTrackSize.y = max( MinMax.ptMaxTrackSize.y,
                                   MinMax.ptMinTrackSize.y );

    if (maxSize) *maxSize = MinMax.ptMaxSize;
    if (maxPos) *maxPos = MinMax.ptMaxPosition;
    if (minTrack) *minTrack = MinMax.ptMinTrackSize;
    if (maxTrack) *maxTrack = MinMax.ptMaxTrackSize;

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
    int Change = 1;

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

    if (FromWnd && hWndFrom != GetDesktopWindow()) // FromWnd->fnid != FNID_DESKTOP)
    {
       if (FromWnd->ExStyle & WS_EX_LAYOUTRTL)
       {
          mirror_from = TRUE;
          Change = -Change;
          Delta.x = -FromWnd->rcClient.right;
       }
       else
          Delta.x = FromWnd->rcClient.left;
       Delta.y = FromWnd->rcClient.top;
    }

    if (ToWnd && hWndTo != GetDesktopWindow()) // ToWnd->fnid != FNID_DESKTOP)
    {
       if (ToWnd->ExStyle & WS_EX_LAYOUTRTL)
       {
          mirror_to = TRUE;
          Change = -Change;
          Delta.x += Change * ToWnd->rcClient.right;
       }
       else
          Delta.x -= Change * ToWnd->rcClient.left;
       Delta.y -= ToWnd->rcClient.top;
    }

    for (i = 0; i != cPoints; i++)
    {
        lpPoints[i].x += Delta.x;
        lpPoints[i].x *= Change;
        lpPoints[i].y += Delta.y;
    }

    if ((mirror_from || mirror_to) && cPoints == 2)  /* special case for rectangle */
    {
       int tmp = min(lpPoints[0].x, lpPoints[1].x);
       lpPoints[1].x = max(lpPoints[0].x, lpPoints[1].x);
       lpPoints[0].x = tmp;
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

    if (hWnd != GetDesktopWindow()) // Wnd->fnid != FNID_DESKTOP )
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

    if ( hWnd != GetDesktopWindow()) // Wnd->fnid != FNID_DESKTOP )
    {
       if (Wnd->ExStyle & WS_EX_LAYOUTRTL)
          lpPoint->x = Wnd->rcClient.right - lpPoint->x;
       else
          lpPoint->x += Wnd->rcClient.left;
       lpPoint->y += Wnd->rcClient.top;
    }
    return TRUE;
}
