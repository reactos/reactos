/* $Id: winpos.c,v 1.8 2003/08/07 04:03:24 royce Exp $
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

#include <windows.h>
#include <user32.h>
#include <window.h>
#include <user32/callback.h>
#include <user32/regcontrol.h>
#include <user32/wininternal.h>
#include <window.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

BOOL
WinPosShowIconTitle(HWND hWnd, BOOL bShow)
{
    PINTERNALPOS lpPos = UserGetInternalPos(hWnd);
  
    if( lpPos)
    {
	HWND hWnd = lpPos->IconTitle;

	if( !hWnd )
	  lpPos->IconTitle = hWnd = NULL; /*ICONTITLE_Create( pWnd );*/
	if( bShow )
	{
	  ULONG Style = GetWindowLongW(hWnd, GWL_STYLE);
	  if( !(Style & WS_VISIBLE) )
	    {
	      SendMessageA( hWnd, WM_SHOWWINDOW, TRUE, 0 );
	      SetWindowPos( hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE |
			    SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW );
	    }
	}
	else ShowWindow( hWnd, SW_HIDE );
    }
    return FALSE;
}

UINT STDCALL
WinPosGetMinMaxInfo(HWND hWnd, POINT* MaxSize, POINT* MaxPos,
		  POINT* MinTrack, POINT* MaxTrack)
{
  MINMAXINFO MinMax;
  INT XInc, YInc;
  INTERNALPOS* Pos;
  ULONG Style = GetWindowLongW(hWnd, GWL_STYLE);
  ULONG ExStyle = GetWindowLongW(hWnd, GWL_EXSTYLE);

  /* Get default values. */
  MinMax.ptMaxSize.x = GetSystemMetrics(SM_CXSCREEN);
  MinMax.ptMaxSize.y = GetSystemMetrics(SM_CYSCREEN);
  MinMax.ptMinTrackSize.x = GetSystemMetrics(SM_CXMINTRACK);
  MinMax.ptMinTrackSize.y = GetSystemMetrics(SM_CYMINTRACK);
  MinMax.ptMaxTrackSize.x = GetSystemMetrics(SM_CXSCREEN);
  MinMax.ptMaxTrackSize.y = GetSystemMetrics(SM_CYSCREEN);

  if (UserHasDlgFrameStyle(Style, ExStyle))
    {
      XInc = GetSystemMetrics(SM_CXDLGFRAME);
      YInc = GetSystemMetrics(SM_CYDLGFRAME);
    }
  else
    {
      XInc = YInc = 0;
      if (UserHasThickFrameStyle(Style, ExStyle))
	{
	  XInc += GetSystemMetrics(SM_CXFRAME);
	  YInc += GetSystemMetrics(SM_CYFRAME);
	}
      if (Style & WS_BORDER)
	{
	  XInc += GetSystemMetrics(SM_CXBORDER);
	  YInc += GetSystemMetrics(SM_CYBORDER);
	}
    }
  MinMax.ptMaxSize.x += 2 * XInc;
  MinMax.ptMaxSize.y += 2 * YInc;

  Pos = UserGetInternalPos(hWnd);
  if (Pos != NULL)
    {
      MinMax.ptMaxPosition = Pos->MaxPos;
    }
  else
    {
      MinMax.ptMaxPosition.x -= XInc;
      MinMax.ptMaxPosition.y -= YInc;
    }

  SendMessageW(hWnd, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax);

  MinMax.ptMaxTrackSize.x = max(MinMax.ptMaxTrackSize.x,
				MinMax.ptMinTrackSize.x);
  MinMax.ptMaxTrackSize.y = max(MinMax.ptMaxTrackSize.y,
				MinMax.ptMinTrackSize.y);

  if (MaxSize) *MaxSize = MinMax.ptMaxSize;
  if (MaxPos) *MaxPos = MinMax.ptMaxPosition;
  if (MinTrack) *MinTrack = MinMax.ptMinTrackSize;
  if (MaxTrack) *MaxTrack = MinMax.ptMaxTrackSize;

  return 0; //FIXME: what does it return?
}


/*
 * @implemented
 */
HWND STDCALL
GetActiveWindow(VOID)
{
  return(NtUserGetActiveWindow());
}

/*
 * @implemented
 */
HWND STDCALL
SetActiveWindow(HWND hWnd)
{
  return(NtUserSetActiveWindow(hWnd));
}

