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

/* FUNCTIONS *****************************************************************/

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

/*
 * @unimplemented
 */
UINT STDCALL
ArrangeIconicWindows(HWND hWnd)
{
  return NtUserCallHwndLock( hWnd, HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS);
}
