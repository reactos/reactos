/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: winpos.c,v 1.32 2003/10/17 17:38:38 mf Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsys/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  NtGdid
 */
/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <windows.h>
#include <include/winpos.h>
#include <include/rect.h>
#include <include/callback.h>
#include <include/painting.h>
#include <include/dce.h>
#include <include/vis.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define MINMAX_NOSWP  (0x00010000)

#define SWP_EX_NOCOPY 0x0001
#define SWP_EX_PAINTSELF 0x0002

#define  SWP_AGG_NOGEOMETRYCHANGE \
    (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE)
#define  SWP_AGG_NOPOSCHANGE \
    (SWP_AGG_NOGEOMETRYCHANGE | SWP_NOZORDER)
#define  SWP_AGG_STATUSFLAGS \
    (SWP_AGG_NOPOSCHANGE | SWP_FRAMECHANGED | SWP_HIDEWINDOW | SWP_SHOWWINDOW)

ATOM AtomInternalPos = (ATOM) NULL;

/* FUNCTIONS *****************************************************************/

#define HAS_DLGFRAME(Style, ExStyle) \
       (((ExStyle) & WS_EX_DLGMODALFRAME) || \
        (((Style) & WS_DLGFRAME) && !((Style) & WS_BORDER)))

#define HAS_THICKFRAME(Style, ExStyle) \
       (((Style) & WS_THICKFRAME) && \
        !((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)

VOID FASTCALL
WinPosSetupInternalPos(VOID)
{
  AtomInternalPos = NtAddAtom(L"SysIP", (ATOM*)(PULONG)&AtomInternalPos);
}

BOOL STDCALL
NtUserGetClientOrigin(HWND hWnd, LPPOINT Point)
{
  PWINDOW_OBJECT WindowObject;

  WindowObject = IntGetWindowObject(hWnd);
  if (WindowObject == NULL)
    {
      Point->x = Point->y = 0;
      return(TRUE);
    }
  Point->x = WindowObject->ClientRect.left;
  Point->y = WindowObject->ClientRect.top;
  return(TRUE);
}

BOOL FASTCALL
WinPosActivateOtherWindow(PWINDOW_OBJECT Window)
{
	return FALSE;
}

POINT STATIC FASTCALL
WinPosFindIconPos(HWND hWnd, POINT Pos)
{
  POINT point;
  //FIXME
  return point;
}

HWND STATIC FASTCALL
WinPosNtGdiIconTitle(PWINDOW_OBJECT WindowObject)
{
  return(NULL);
}

BOOL STATIC FASTCALL
WinPosShowIconTitle(PWINDOW_OBJECT WindowObject, BOOL Show)
{
  PINTERNALPOS InternalPos = (PINTERNALPOS)IntGetProp(WindowObject, AtomInternalPos);
  PWINDOW_OBJECT IconWindow;
  NTSTATUS Status;

  if (InternalPos)
    {
      HWND hWnd = InternalPos->IconTitle;

      if (hWnd == NULL)
	{
	  hWnd = WinPosNtGdiIconTitle(WindowObject);
	}
      if (Show)
	{
	  Status = 
	    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->
				       HandleTable,
				       hWnd,
				       otWindow,
				       (PVOID*)&IconWindow);
	  if (NT_SUCCESS(Status))
	    {
	      if (!(IconWindow->Style & WS_VISIBLE))
		{
		  NtUserSendMessage(hWnd, WM_SHOWWINDOW, TRUE, 0);
		  WinPosSetWindowPos(hWnd, 0, 0, 0, 0, 0, SWP_NOSIZE |
				     SWP_NOMOVE | SWP_NOACTIVATE | 
				     SWP_NOZORDER | SWP_SHOWWINDOW);
		}
	      ObmDereferenceObject(IconWindow);
	    }
	}
      else
	{
	  WinPosShowWindow(hWnd, SW_HIDE);
	}
    }
  return(FALSE);
}

PINTERNALPOS STATIC STDCALL
WinPosInitInternalPos(PWINDOW_OBJECT WindowObject, POINT pt, PRECT RestoreRect)
{
  PINTERNALPOS InternalPos = (PINTERNALPOS)IntGetProp(WindowObject, AtomInternalPos);
  if (InternalPos == NULL)
    {
      InternalPos = 
	ExAllocatePool(NonPagedPool, sizeof(INTERNALPOS));
      IntSetProp(WindowObject, AtomInternalPos, InternalPos);
      InternalPos->IconTitle = 0;
      InternalPos->NormalRect = WindowObject->WindowRect;
      InternalPos->IconPos.x = InternalPos->MaxPos.x = 0xFFFFFFFF;
      InternalPos->IconPos.y = InternalPos->MaxPos.y = 0xFFFFFFFF;
    }
  if (WindowObject->Style & WS_MINIMIZE)
    {
      InternalPos->IconPos = pt;
    }
  else if (WindowObject->Style & WS_MAXIMIZE)
    {
      InternalPos->MaxPos = pt;
    }
  else if (RestoreRect != NULL)
    {
      InternalPos->NormalRect = *RestoreRect;
    }
  return(InternalPos);
}

UINT STDCALL
WinPosMinMaximize(PWINDOW_OBJECT WindowObject, UINT ShowFlag, RECT* NewPos)
{
  POINT Size;
  PINTERNALPOS InternalPos;
  UINT SwpFlags = 0;

  Size.x = WindowObject->WindowRect.left;
  Size.y = WindowObject->WindowRect.top;
  InternalPos = WinPosInitInternalPos(WindowObject, Size, 
				      &WindowObject->WindowRect); 

  if (InternalPos)
    {
      if (WindowObject->Style & WS_MINIMIZE)
	{
	  if (!NtUserSendMessage(WindowObject->Self, WM_QUERYOPEN, 0, 0))
	    {
	      return(SWP_NOSIZE | SWP_NOMOVE);
	    }
	  SwpFlags |= SWP_NOCOPYBITS;
	}
      switch (ShowFlag)
	{
	case SW_MINIMIZE:
	  {
	    if (WindowObject->Style & WS_MAXIMIZE)
	      {
		WindowObject->Flags |= WINDOWOBJECT_RESTOREMAX;
		WindowObject->Style &= ~WS_MAXIMIZE;
	      }
	    else
	      {
		WindowObject->Style &= ~WINDOWOBJECT_RESTOREMAX;
	      }
	    WindowObject->Style |= WS_MINIMIZE;
	    InternalPos->IconPos = WinPosFindIconPos(WindowObject,
						     InternalPos->IconPos);
	    NtGdiSetRect(NewPos, InternalPos->IconPos.x, InternalPos->IconPos.y,
			NtUserGetSystemMetrics(SM_CXICON),
			NtUserGetSystemMetrics(SM_CYICON));
	    SwpFlags |= SWP_NOCOPYBITS;
	    break;
	  }

	case SW_MAXIMIZE:
	  {
	    WinPosGetMinMaxInfo(WindowObject, &Size, &InternalPos->MaxPos, 
				NULL, NULL);
	    if (WindowObject->Style & WS_MINIMIZE)
	      {
		WinPosShowIconTitle(WindowObject, FALSE);
		WindowObject->Style &= ~WS_MINIMIZE;
	      }
	    WindowObject->Style |= WS_MINIMIZE;
	    NtGdiSetRect(NewPos, InternalPos->MaxPos.x, InternalPos->MaxPos.y,
			Size.x, Size.y);
	    break;
	  }

	case SW_RESTORE:
	  {
	    if (WindowObject->Style & WS_MINIMIZE)
	      {
		WindowObject->Style &= ~WS_MINIMIZE;
		WinPosShowIconTitle(WindowObject, FALSE);
		if (WindowObject->Flags & WINDOWOBJECT_RESTOREMAX)
		  {
		    WinPosGetMinMaxInfo(WindowObject, &Size,
					&InternalPos->MaxPos, NULL, NULL);
		    WindowObject->Style |= WS_MAXIMIZE;
		    NtGdiSetRect(NewPos, InternalPos->MaxPos.x,
				InternalPos->MaxPos.y, Size.x, Size.y);
		    break;
		  }
	      }
	    else
	      {
		if (!(WindowObject->Style & WS_MAXIMIZE))
		  {
		    return(-1);
		  }
		else
		  {
		    WindowObject->Style &= ~WS_MAXIMIZE;
		  }	      
		*NewPos = InternalPos->NormalRect;
		NewPos->right -= NewPos->left;
		NewPos->bottom -= NewPos->top;
		break;
	      }
	  }
	}
    }
  else
    {
      SwpFlags |= SWP_NOSIZE | SWP_NOMOVE;
    }
  return(SwpFlags);
}

UINT STDCALL
WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
		    POINT* MinTrack, POINT* MaxTrack)
{
  MINMAXINFO MinMax;
  INT XInc, YInc;
  INTERNALPOS* Pos;

  /* Get default values. */
  MinMax.ptMaxSize.x = NtUserGetSystemMetrics(SM_CXSCREEN);
  MinMax.ptMaxSize.y = NtUserGetSystemMetrics(SM_CYSCREEN);
  MinMax.ptMinTrackSize.x = NtUserGetSystemMetrics(SM_CXMINTRACK);
  MinMax.ptMinTrackSize.y = NtUserGetSystemMetrics(SM_CYMINTRACK);
  MinMax.ptMaxTrackSize.x = NtUserGetSystemMetrics(SM_CXSCREEN);
  MinMax.ptMaxTrackSize.y = NtUserGetSystemMetrics(SM_CYSCREEN);

  if (HAS_DLGFRAME(Window->Style, Window->ExStyle))
    {
      XInc = NtUserGetSystemMetrics(SM_CXDLGFRAME);
      YInc = NtUserGetSystemMetrics(SM_CYDLGFRAME);
    }
  else
    {
      XInc = YInc = 0;
      if (HAS_THICKFRAME(Window->Style, Window->ExStyle))
	{
	  XInc += NtUserGetSystemMetrics(SM_CXFRAME);
	  YInc += NtUserGetSystemMetrics(SM_CYFRAME);
	}
      if (Window->Style & WS_BORDER)
	{
	  XInc += NtUserGetSystemMetrics(SM_CXBORDER);
	  YInc += NtUserGetSystemMetrics(SM_CYBORDER);
	}
    }
  MinMax.ptMaxSize.x += 2 * XInc;
  MinMax.ptMaxSize.y += 2 * YInc;

  Pos = (PINTERNALPOS)IntGetProp(Window, AtomInternalPos);
  if (Pos != NULL)
    {
      MinMax.ptMaxPosition = Pos->MaxPos;
    }
  else
    {
      MinMax.ptMaxPosition.x -= XInc;
      MinMax.ptMaxPosition.y -= YInc;
    }

  IntSendMessage(Window->Self, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax, TRUE);

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

BOOL STATIC FASTCALL
WinPosChangeActiveWindow(HWND hWnd, BOOL MouseMsg)
{
  PWINDOW_OBJECT WindowObject;

  WindowObject = IntGetWindowObject(hWnd);
  if (WindowObject == NULL)
    {
      return FALSE;
    }

  NtUserSendMessage(hWnd,
    WM_ACTIVATE,
	  MAKELONG(MouseMsg ? WA_CLICKACTIVE : WA_CLICKACTIVE,
      (WindowObject->Style & WS_MINIMIZE) ? 1 : 0),
      (LPARAM)IntGetDesktopWindow());  /* FIXME: Previous active window */

  IntReleaseWindowObject(WindowObject);

  return TRUE;
}

LONG STATIC STDCALL
WinPosDoNCCALCSize(PWINDOW_OBJECT Window, PWINDOWPOS WinPos,
		   RECT* WindowRect, RECT* ClientRect)
{
  UINT wvrFlags = 0;

  /* Send WM_NCCALCSIZE message to get new client area */
  if ((WinPos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE)
    {
      NCCALCSIZE_PARAMS params;
      WINDOWPOS winposCopy;

      params.rgrc[0] = *WindowRect;
      params.rgrc[1] = Window->WindowRect;
      params.rgrc[2] = Window->ClientRect;
      if (0 != (Window->Style & WS_CHILD))
	{
	  NtGdiOffsetRect(&(params.rgrc[0]), - Window->Parent->ClientRect.left,
	                      - Window->Parent->ClientRect.top);
	  NtGdiOffsetRect(&(params.rgrc[1]), - Window->Parent->ClientRect.left,
	                      - Window->Parent->ClientRect.top);
	  NtGdiOffsetRect(&(params.rgrc[2]), - Window->Parent->ClientRect.left,
	                      - Window->Parent->ClientRect.top);
	}
      params.lppos = &winposCopy;
      winposCopy = *WinPos;

      wvrFlags = IntSendNCCALCSIZEMessage(Window->Self, TRUE, NULL, &params);

      /* If the application send back garbage, ignore it */
      if (params.rgrc[0].left <= params.rgrc[0].right &&
          params.rgrc[0].top <= params.rgrc[0].bottom)
	{
          *ClientRect = params.rgrc[0];
	  if (Window->Style & WS_CHILD)
	    {
	      NtGdiOffsetRect(ClientRect, Window->Parent->ClientRect.left,
	                      Window->Parent->ClientRect.top);
	    }
	}

       /* FIXME: WVR_ALIGNxxx */

      if (ClientRect->left != Window->ClientRect.left ||
          ClientRect->top != Window->ClientRect.top)
	{
          WinPos->flags &= ~SWP_NOCLIENTMOVE;
	}

      if ((ClientRect->right - ClientRect->left !=
           Window->ClientRect.right - Window->ClientRect.left) ||
          (ClientRect->bottom - ClientRect->top !=
           Window->ClientRect.bottom - Window->ClientRect.top))
	{
          WinPos->flags &= ~SWP_NOCLIENTSIZE;
	}
    }
  else
    {
      if (! (WinPos->flags & SWP_NOMOVE)
          && (ClientRect->left != Window->ClientRect.left ||
              ClientRect->top != Window->ClientRect.top))
	{
          WinPos->flags &= ~SWP_NOCLIENTMOVE;
	}
    }

  return wvrFlags;
}

BOOL STDCALL
WinPosDoWinPosChanging(PWINDOW_OBJECT WindowObject,
		       PWINDOWPOS WinPos,
		       PRECT WindowRect,
		       PRECT ClientRect)
{
  INT X, Y;

  if (!(WinPos->flags & SWP_NOSENDCHANGING))
    {
      IntSendWINDOWPOSCHANGINGMessage(WindowObject->Self, WinPos);
    }
  
  *WindowRect = WindowObject->WindowRect;
  *ClientRect = 
    (WindowObject->Style & WS_MINIMIZE) ? WindowObject->WindowRect :
    WindowObject->ClientRect;

  if (!(WinPos->flags & SWP_NOSIZE))
    {
      WindowRect->right = WindowRect->left + WinPos->cx;
      WindowRect->bottom = WindowRect->top + WinPos->cy;
    }

  if (!(WinPos->flags & SWP_NOMOVE))
    {
      X = WinPos->x;
      Y = WinPos->y;
      if (0 != (WindowObject->Style & WS_CHILD))
	{
	  X += WindowObject->Parent->ClientRect.left;
	  Y += WindowObject->Parent->ClientRect.top;
	}
      WindowRect->left = X;
      WindowRect->top = Y;
      WindowRect->right += X - WindowObject->WindowRect.left;
      WindowRect->bottom += Y - WindowObject->WindowRect.top;
      NtGdiOffsetRect(ClientRect,
        X - WindowObject->WindowRect.left,
        Y - WindowObject->WindowRect.top);
    }

  WinPos->flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;

  return TRUE;
}

/***********************************************************************
 *	     WinPosInternalMoveWindow
 *
 * Update WindowRect and ClientRect of Window and all of its children
 * We keep both WindowRect and ClientRect in screen coordinates internally
 */
static VOID
WinPosInternalMoveWindow(PWINDOW_OBJECT Window, INT MoveX, INT MoveY)
{
  PWINDOW_OBJECT Child;

  Window->WindowRect.left += MoveX;
  Window->WindowRect.right += MoveX;
  Window->WindowRect.top += MoveY;
  Window->WindowRect.bottom += MoveY;

  Window->ClientRect.left += MoveX;
  Window->ClientRect.right += MoveX;
  Window->ClientRect.top += MoveY;
  Window->ClientRect.bottom += MoveY;

  ExAcquireFastMutexUnsafe(&Window->ChildrenListLock);
  Child = Window->FirstChild;
  while (Child)
    {
      WinPosInternalMoveWindow(Child, MoveX, MoveY);
      Child = Child->NextSibling;
    }
  ExReleaseFastMutexUnsafe(&Window->ChildrenListLock);
}


/* x and y are always screen relative */
BOOLEAN STDCALL
WinPosSetWindowPos(HWND Wnd, HWND WndInsertAfter, INT x, INT y, INT cx,
		   INT cy, UINT flags)
{
  PWINDOW_OBJECT Window;
  NTSTATUS Status;
  WINDOWPOS WinPos;
  RECT NewWindowRect;
  RECT NewClientRect;
  HRGN VisBefore = NULL;
  HRGN VisAfter = NULL;
  HRGN DirtyRgn = NULL;
  HRGN ExposedRgn = NULL;
  HRGN CopyRgn = NULL;
  ULONG WvrFlags = 0;
  RECT OldWindowRect, OldClientRect;
  UINT FlagsEx = 0;
  int RgnType;
  HDC Dc;
  RECT CopyRect;
  RECT TempRect;

  /* FIXME: Get current active window from active queue. */

  /* Check if the window is for a desktop. */
  if (Wnd == PsGetWin32Thread()->Desktop->DesktopWindow)
    {
      return(FALSE);
    }

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       Wnd,
			       otWindow,
			       (PVOID*)&Window);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }
  
  /* Fix up the flags. */
  if (Window->Style & WS_VISIBLE)
    {
      flags &= ~SWP_SHOWWINDOW;
    }
  else 
    {
      if (!(flags & SWP_SHOWWINDOW))
	{
	  flags |= SWP_NOREDRAW;
	}
      flags &= ~SWP_HIDEWINDOW;
    }

  cx = max(cx, 0);
  cy = max(cy, 0);

  if ((Window->WindowRect.right - Window->WindowRect.left) == cx &&
      (Window->WindowRect.bottom - Window->WindowRect.top) == cy)
    {
      flags |= SWP_NOSIZE;
    }
  if (Window->WindowRect.left == x && Window->WindowRect.top == y)
    {
      flags |= SWP_NOMOVE;
    }
  if (Window->Style & WIN_NCACTIVATED)
    {
      flags |= SWP_NOACTIVATE;
    }
  else if ((Window->Style & (WS_POPUP | WS_CHILD)) != WS_CHILD)
    {
      if (!(flags & SWP_NOACTIVATE))
	{
	  flags &= ~SWP_NOZORDER;
	  WndInsertAfter = HWND_TOP;
	}
    }

  if (WndInsertAfter == HWND_TOPMOST || WndInsertAfter == HWND_NOTOPMOST)
    {
      WndInsertAfter = HWND_TOP;
    }

  if (WndInsertAfter != HWND_TOP && WndInsertAfter != HWND_BOTTOM)
    {
      /* FIXME: Find the window to insert after. */
    }

  WinPos.hwnd = Wnd;
  WinPos.hwndInsertAfter = WndInsertAfter;
  WinPos.x = x;
  WinPos.y = y;
  WinPos.cx = cx;
  WinPos.cy = cy;
  WinPos.flags = flags;
  if (0 != (Window->Style & WS_CHILD))
    {
      WinPos.x -= Window->Parent->ClientRect.left;
      WinPos.y -= Window->Parent->ClientRect.top;
    }

  WinPosDoWinPosChanging(Window, &WinPos, &NewWindowRect, &NewClientRect);

  if ((WinPos.flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) !=
      SWP_NOZORDER)
    {
      /* FIXME: SWP_DoOwnedPopups. */
    }
  
  /* FIXME: Adjust flags based on WndInsertAfter */

  /* Compute the visible region before the window position is changed */
  if ((!(WinPos.flags & (SWP_NOREDRAW | SWP_SHOWWINDOW)) &&
       WinPos.flags & (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | 
		       SWP_HIDEWINDOW | SWP_FRAMECHANGED)) != 
      (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER))
    {
      if (Window->Style & WS_CLIPCHILDREN)
	{
	  VisBefore = VIS_ComputeVisibleRegion(PsGetWin32Thread()->Desktop,
	                                       Window, FALSE, FALSE, TRUE);
	}
      else
	{
	  VisBefore = VIS_ComputeVisibleRegion(PsGetWin32Thread()->Desktop,
	                                       Window, FALSE, FALSE, FALSE);
	}
      if (NULLREGION == UnsafeIntGetRgnBox(VisBefore, &TempRect))
	{
	  NtGdiDeleteObject(VisBefore);
	  VisBefore = NULL;
	}
    }

  WvrFlags = WinPosDoNCCALCSize(Window, &WinPos, &NewWindowRect,
				&NewClientRect);

  /* FIXME: Relink windows. (also take into account shell window in hwndShellWindow) */

  /* FIXME: Reset active DCEs */

  OldWindowRect = Window->WindowRect;
  OldClientRect = Window->ClientRect;

  /* FIXME: Check for redrawing the whole client rect. */

  if (! (WinPos.flags & SWP_NOMOVE))
    {
      WinPosInternalMoveWindow(Window,
                               NewWindowRect.left - OldWindowRect.left,
                               NewWindowRect.top - OldWindowRect.top);
    }
  Window->WindowRect = NewWindowRect;
  Window->ClientRect = NewClientRect;

  if (WinPos.flags & SWP_SHOWWINDOW)
    {
      Window->Style |= WS_VISIBLE;
      FlagsEx |= SWP_EX_PAINTSELF;
    }
  else if (WinPos.flags & SWP_HIDEWINDOW)
    {
      Window->Style &= ~WS_VISIBLE;
    }

  if (!(WinPos.flags & SWP_NOREDRAW))
    {
      /* Determine the new visible region */
      if (Window->Style & WS_CLIPCHILDREN)
	{
	  VisAfter = VIS_ComputeVisibleRegion(PsGetWin32Thread()->Desktop,
	                                      Window, FALSE, FALSE, TRUE);
	}
      else
	{
	  VisAfter = VIS_ComputeVisibleRegion(PsGetWin32Thread()->Desktop,
	                                      Window, FALSE, FALSE, FALSE);
	}
      if (NULLREGION == UnsafeIntGetRgnBox(VisAfter, &TempRect))
	{
	  NtGdiDeleteObject(VisAfter);
	  VisAfter = NULL;
	}

      /* Determine which pixels can be copied from the old window position
         to the new. Those pixels must be visible in both the old and new
         position. Also, check the class style to see if the windows of this
         class need to be completely repainted on (horizontal/vertical) size
         change */
      if (NULL != VisBefore && NULL != VisAfter && ! (WinPos.flags & SWP_NOCOPYBITS)
          && ((WinPos.flags & SWP_NOSIZE)
              || ! (Window->Class->style & (CS_HREDRAW | CS_VREDRAW))))
	{
	  CopyRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
	  RgnType = NtGdiCombineRgn(CopyRgn, VisAfter, VisBefore, RGN_AND);

	  /* If this is (also) a window resize, the whole nonclient area
             needs to be repainted. So we limit the copy to the client area,
	     'cause there is no use in copying it (would possibly cause
	     "flashing" too). However, if the copy region is already empty,
	     we don't have to crop (can't take anything away from an empty
	     region...) */
	  if (! (WinPos.flags & SWP_NOSIZE)
	      && ERROR != RgnType && NULLREGION != RgnType)
	    {
	      RECT ORect = OldClientRect;
	      RECT NRect = NewClientRect;
	      NtGdiOffsetRect(&ORect, - OldWindowRect.left, - OldWindowRect.top);
	      NtGdiOffsetRect(&NRect, - NewWindowRect.left, - NewWindowRect.top);
	      NtGdiIntersectRect(&CopyRect, &ORect, &NRect);
	      REGION_CropRgn(CopyRgn, CopyRgn, &CopyRect, NULL);
	    }

	  /* No use in copying bits which are in the update region. */
	  if ((HRGN) 1 == Window->UpdateRegion)
	    {
	      /* The whole window is in the update region. No use
	         copying anything, so set the copy region empty */
	      NtGdiSetRectRgn(CopyRgn, 0, 0, 0, 0);
	    }
	  else if (1 < (DWORD) Window->UpdateRegion)
	    {
	      NtGdiCombineRgn(CopyRgn, CopyRgn, Window->UpdateRegion, RGN_DIFF);
	    }
		  

	  /* Now, get the bounding box of the copy region. If it's empty
	     there's nothing to copy. Also, it's no use copying bits onto
	     themselves */
	  UnsafeIntGetRgnBox(CopyRgn, &CopyRect);
	  if (NtGdiIsEmptyRect(&CopyRect))
	    {
	      /* Nothing to copy, clean up */
	      NtGdiDeleteObject(CopyRgn);
	      CopyRgn = NULL;
	    }
	  else if (OldWindowRect.left != NewWindowRect.left
	           || OldWindowRect.top != NewWindowRect.top)
	    {
	      /* Small trick here: there is no function to bitblt a region. So
                 we set the region as the clipping region, take the bounding box
	         of the region and bitblt that. Since nothing outside the clipping
	         region is copied, this has the effect of bitblt'ing the region.

	         Since NtUserGetDCEx takes ownership of the clip region, we need
	         to create a copy of CopyRgn and pass that. We need CopyRgn later */
	      HRGN ClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
	      NtGdiCombineRgn(ClipRgn, CopyRgn, NULL, RGN_COPY);
	      Dc = NtUserGetDCEx(Wnd, ClipRgn, DCX_WINDOW | DCX_CACHE |
			    DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_CLIPSIBLINGS );
	      NtGdiBitBlt(Dc, CopyRect.left, CopyRect.top, CopyRect.right - CopyRect.left,
	                  CopyRect.bottom - CopyRect.top, Dc,
	                  CopyRect.left + (OldWindowRect.left - NewWindowRect.left),
	                  CopyRect.top + (OldWindowRect.top - NewWindowRect.top), SRCCOPY);
	      NtUserReleaseDC(Wnd, Dc);
	    }
	}
      else
	{
	  CopyRgn = NULL;
	}

      /* We need to redraw what wasn't visible before */
      if (NULL != VisAfter)
	{
	  if (NULL != CopyRgn)
	    {
	      DirtyRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
	      RgnType = NtGdiCombineRgn(DirtyRgn, VisAfter, CopyRgn, RGN_DIFF);
	      if (ERROR != RgnType && NULLREGION != RgnType)
		{
		  PaintRedrawWindow(Window, NULL, DirtyRgn,
		                    RDW_ERASE | RDW_FRAME | RDW_INVALIDATE |
		                    RDW_ALLCHILDREN | RDW_ERASENOW, 
		                    RDW_EX_XYWINDOW | RDW_EX_USEHRGN);
		}
	      NtGdiDeleteObject(DirtyRgn);
	    }
	  else
	    {
	      PaintRedrawWindow(Window, NULL, NULL,
	                        RDW_ERASE | RDW_FRAME | RDW_INVALIDATE |
	                        RDW_ALLCHILDREN | RDW_ERASENOW, 
	                        RDW_EX_XYWINDOW | RDW_EX_USEHRGN);
	    }
	}

      if (NULL != CopyRgn)
	{
	  NtGdiDeleteObject(CopyRgn);
	}
    }

  /* Expose what was covered before but not covered anymore */
  if (NULL != VisBefore)
    {
      ExposedRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
      NtGdiCombineRgn(ExposedRgn, VisBefore, NULL, RGN_COPY);
      NtGdiOffsetRgn(ExposedRgn, OldWindowRect.left - NewWindowRect.left,
                     OldWindowRect.top - NewWindowRect.top);
      if (NULL != VisAfter)
	{
	  RgnType = NtGdiCombineRgn(ExposedRgn, ExposedRgn, VisAfter, RGN_DIFF);
	}
      else
	{
	  RgnType = SIMPLEREGION;
	}
      if (ERROR != RgnType && NULLREGION != RgnType)
        {
          VIS_WindowLayoutChanged(PsGetWin32Thread()->Desktop, Window, ExposedRgn);
        }
      NtGdiDeleteObject(ExposedRgn);

      NtGdiDeleteObject(VisBefore);
    }

  if (NULL != VisAfter)
    {
      NtGdiDeleteObject(VisAfter);
    }

  /* FIXME: Hide or show the claret */

  if (!(flags & SWP_NOACTIVATE))
    {
      WinPosChangeActiveWindow(WinPos.hwnd, FALSE);
    }

  /* FIXME: Check some conditions before doing this. */
  IntSendWINDOWPOSCHANGEDMessage(WinPos.hwnd, &WinPos);

  ObmDereferenceObject(Window);
  return(TRUE);
}

LRESULT STDCALL
WinPosGetNonClientSize(HWND Wnd, RECT* WindowRect, RECT* ClientRect)
{
  *ClientRect = *WindowRect;
  return(IntSendNCCALCSIZEMessage(Wnd, FALSE, ClientRect, NULL));
}

BOOLEAN FASTCALL
WinPosShowWindow(HWND Wnd, INT Cmd)
{
  BOOLEAN WasVisible;
  PWINDOW_OBJECT Window;
  NTSTATUS Status;
  UINT Swp = 0;
  RECT NewPos;
  BOOLEAN ShowFlag;
  HRGN VisibleRgn;

  Status = 
    ObmReferenceObjectByHandle(PsGetWin32Process()->WindowStation->HandleTable,
			       Wnd,
			       otWindow,
			       (PVOID*)&Window);
  if (!NT_SUCCESS(Status))
    {
      return(FALSE);
    }
  
  WasVisible = (Window->Style & WS_VISIBLE) != 0;

  switch (Cmd)
    {
    case SW_HIDE:
      {
	if (!WasVisible)
	  {
	    ObmDereferenceObject(Window);
	    return(FALSE);
	  }
	Swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE |
	  SWP_NOZORDER;
	break;
      }

    case SW_SHOWMINNOACTIVE:
      Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
      /* Fall through. */
    case SW_SHOWMINIMIZED:
      Swp |= SWP_SHOWWINDOW;
      /* Fall through. */
    case SW_MINIMIZE:
      {
	Swp |= SWP_FRAMECHANGED;
	if (!(Window->Style & WS_MINIMIZE))
	  {
	    Swp |= WinPosMinMaximize(Window, SW_MINIMIZE, &NewPos);
	  }
	else
	  {
	    Swp |= SWP_NOSIZE | SWP_NOMOVE;
	  }
	break;
      }

    case SW_SHOWMAXIMIZED:
      {
	Swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
	if (!(Window->Style & WS_MAXIMIZE))
	  {
	    Swp |= WinPosMinMaximize(Window, SW_MAXIMIZE, &NewPos);
	  }
	else
	  {
	    Swp |= SWP_NOSIZE | SWP_NOMOVE;
	  }
	break;
      }

    case SW_SHOWNA:
      Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
      /* Fall through. */
    case SW_SHOW:
      Swp |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
      /* Don't activate the topmost window. */
      break;

    case SW_SHOWNOACTIVATE:
      Swp |= SWP_NOZORDER;
      /* Fall through. */
    case SW_SHOWNORMAL:
    case SW_SHOWDEFAULT:
    case SW_RESTORE:
      Swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
      if (Window->Style & (WS_MINIMIZE | WS_MAXIMIZE))
	{
	  Swp |= WinPosMinMaximize(Window, SW_RESTORE, &NewPos);	 
	}
      else
	{
	  Swp |= SWP_NOSIZE | SWP_NOMOVE;
	}
      break;
    }

  ShowFlag = (Cmd != SW_HIDE);
  if (ShowFlag != WasVisible)
    {
      NtUserSendMessage(Wnd, WM_SHOWWINDOW, ShowFlag, 0);
      /* 
       * FIXME: Need to check the window wasn't destroyed during the 
       * window procedure. 
       */
    }

  if (Window->Style & WS_CHILD &&
      !IntIsWindowVisible(Window->Parent->Self) &&
      (Swp & (SWP_NOSIZE | SWP_NOMOVE)) == (SWP_NOSIZE | SWP_NOMOVE))
    {
      if (Cmd == SW_HIDE)
	{
	  VisibleRgn = VIS_ComputeVisibleRegion(PsGetWin32Thread()->Desktop, Window,
	                                        FALSE, FALSE, FALSE);
	  Window->Style &= ~WS_VISIBLE;
	  VIS_WindowLayoutChanged(PsGetWin32Thread()->Desktop, Window, VisibleRgn);
	  NtGdiDeleteObject(VisibleRgn);
	}
      else
	{
	  Window->Style |= WS_VISIBLE;
	}
    }
  else
    {
      if (Window->Style & WS_CHILD &&
	  !(Window->ExStyle & WS_EX_MDICHILD))
	{
	  Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
	}
      if (!(Swp & MINMAX_NOSWP))
	{
	  WinPosSetWindowPos(Wnd, HWND_TOP, NewPos.left, NewPos.top,
			     NewPos.right, NewPos.bottom, LOWORD(Swp));
	  if (Cmd == SW_HIDE)
	    {
	      /* Hide the window. */
	      if (Wnd == IntGetActiveWindow())
		{
		  WinPosActivateOtherWindow(Window);
		}
	      /* Revert focus to parent. */
	      if (Wnd == IntGetFocusWindow() ||
		  IntIsChildWindow(Wnd, IntGetFocusWindow()))
		{
		  IntSetFocusWindow(Window->Parent->Self);
		}
	    }
	}
      /* FIXME: Check for window destruction. */
      /* Show title for minimized windows. */
      if (Window->Style & WS_MINIMIZE)
	{
	  WinPosShowIconTitle(Window, TRUE);
	}
    }

  if (Window->Flags & WINDOWOBJECT_NEED_SIZE)
    {
      WPARAM wParam = SIZE_RESTORED;

      Window->Flags &= ~WINDOWOBJECT_NEED_SIZE;
      if (Window->Style & WS_MAXIMIZE)
	{
	  wParam = SIZE_MAXIMIZED;
	}
      else if (Window->Style & WS_MINIMIZE)
	{
	  wParam = SIZE_MINIMIZED;
	}

      NtUserSendMessage(Wnd, WM_SIZE, wParam,
			MAKELONG(Window->ClientRect.right - 
				 Window->ClientRect.left,
				 Window->ClientRect.bottom -
				 Window->ClientRect.top));
      NtUserSendMessage(Wnd, WM_MOVE, 0,
			MAKELONG(Window->ClientRect.left,
				 Window->ClientRect.top));
    }

  /* Activate the window if activation is not requested and the window is not minimized */
  if (!(Swp & (SWP_NOACTIVATE | SWP_HIDEWINDOW)) && !(Window->Style & WS_MINIMIZE))
    {
      WinPosChangeActiveWindow(Wnd, FALSE);
    }

  ObmDereferenceObject(Window);
  return(WasVisible);
}

BOOL STATIC FASTCALL
WinPosPtInWindow(PWINDOW_OBJECT Window, POINT Point)
{
  return(Point.x >= Window->WindowRect.left &&
	 Point.x < Window->WindowRect.right &&
	 Point.y >= Window->WindowRect.top &&
	 Point.y < Window->WindowRect.bottom);
}

USHORT STATIC STDCALL
WinPosSearchChildren(PWINDOW_OBJECT ScopeWin, POINT Point,
		     PWINDOW_OBJECT* Window)
{
  PWINDOW_OBJECT Current;

  ExAcquireFastMutexUnsafe(&ScopeWin->ChildrenListLock);
  Current = ScopeWin->FirstChild;
  while (Current)
    {
      if (Current->Style & WS_VISIBLE &&
	  ((!(Current->Style & WS_DISABLED)) ||
	   (Current->Style & (WS_CHILD | WS_POPUP)) != WS_CHILD) &&
	  WinPosPtInWindow(Current, Point))
	{
	  *Window = Current;
	  if (Current->Style & WS_DISABLED)
	    {
		  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
	      return(HTERROR);
	    }
	  if (Current->Style & WS_MINIMIZE)
	    {
		  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
	      return(HTCAPTION);
	    }
	  if (Point.x >= Current->ClientRect.left &&
	      Point.x < Current->ClientRect.right &&
	      Point.y >= Current->ClientRect.top &&
	      Point.y < Current->ClientRect.bottom)
	    {

		  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
	      return(WinPosSearchChildren(Current, Point, Window));
	    }

	  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
	  return(0);
	}
      Current = Current->NextSibling;
    }
		  
  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
  return(0);
}

USHORT STDCALL
WinPosWindowFromPoint(PWINDOW_OBJECT ScopeWin, POINT WinPoint, 
		      PWINDOW_OBJECT* Window)
{
  HWND DesktopWindowHandle;
  PWINDOW_OBJECT DesktopWindow;
  POINT Point = WinPoint;
  USHORT HitTest;

  *Window = NULL;

  if (ScopeWin->Style & WS_DISABLED)
    {
      return(HTERROR);
    }

  /* Translate the point to the space of the scope window. */
  DesktopWindowHandle = IntGetDesktopWindow();
  DesktopWindow = IntGetWindowObject(DesktopWindowHandle);
  Point.x += ScopeWin->ClientRect.left - DesktopWindow->ClientRect.left;
  Point.y += ScopeWin->ClientRect.top - DesktopWindow->ClientRect.top;
  IntReleaseWindowObject(DesktopWindow);

  HitTest = WinPosSearchChildren(ScopeWin, Point, Window);
  if (HitTest != 0)
    {
      return(HitTest);
    }

  if ((*Window) == NULL)
    {
      return(HTNOWHERE);
    }
  if ((*Window)->MessageQueue == PsGetWin32Thread()->MessageQueue)
    {
      HitTest = IntSendMessage((*Window)->Self, WM_NCHITTEST, 0,
				MAKELONG(Point.x, Point.y), FALSE);
      /* FIXME: Check for HTTRANSPARENT here. */
    }
  else
    {
      HitTest = HTCLIENT;
    }

  return(HitTest);
}

BOOL
WinPosSetActiveWindow(PWINDOW_OBJECT Window, BOOL Mouse, BOOL ChangeFocus)
{
  PUSER_MESSAGE_QUEUE ActiveQueue;
  HWND PrevActive;

  ActiveQueue = IntGetFocusMessageQueue();
  if (ActiveQueue != NULL)
    {
      PrevActive = ActiveQueue->ActiveWindow;
    }
  else
    {
      PrevActive = NULL;
    }

  if (Window->Self == IntGetActiveDesktop() || Window->Self == PrevActive)
    {
      return(FALSE);
    }
  if (PrevActive != NULL)
    {
      PWINDOW_OBJECT PrevActiveWindow = IntGetWindowObject(PrevActive);
      WORD Iconised = HIWORD(PrevActiveWindow->Style & WS_MINIMIZE);
      if (!IntSendMessage(PrevActive, WM_NCACTIVATE, FALSE, 0, TRUE))
	{
	  /* FIXME: Check if the new window is system modal. */
	  return(FALSE);
	}
      IntSendMessage(PrevActive, 
		      WM_ACTIVATE, 
		      MAKEWPARAM(WA_INACTIVE, Iconised), 
		      (LPARAM)Window->Self,
		      TRUE);
      /* FIXME: Check if anything changed while processing the message. */
      IntReleaseWindowObject(PrevActiveWindow);
    }

  if (Window != NULL)
    {
      Window->MessageQueue->ActiveWindow = Window->Self;
    }
  else if (ActiveQueue != NULL)
    {
      ActiveQueue->ActiveWindow = NULL;
    }
  /* FIXME:  Unset this flag for inactive windows */
  //if ((Window->Style) & WS_CHILD) Window->Flags |= WIN_NCACTIVATED;

  /* FIXME: Send palette messages. */

  /* FIXME: Redraw icon title of previously active window. */

  /* FIXME: Bring the window to the top. */  

  /* FIXME: Send WM_ACTIVATEAPP */
  
  IntSetFocusMessageQueue(Window->MessageQueue);

  /* FIXME: Send activate messages. */

  /* FIXME: Change focus. */

  /* FIXME: Redraw new window icon title. */

  return(TRUE);
}

HWND STDCALL
NtUserGetActiveWindow(VOID)
{
  PUSER_MESSAGE_QUEUE ActiveQueue;

  ActiveQueue = IntGetFocusMessageQueue();
  if (ActiveQueue == NULL)
    {
      return(NULL);
    }
  return(ActiveQueue->ActiveWindow);
}

HWND STDCALL
NtUserSetActiveWindow(HWND hWnd)
{
  PWINDOW_OBJECT Window;
  PUSER_MESSAGE_QUEUE ThreadQueue;
  HWND Prev;

  Window = IntGetWindowObject(hWnd);
  if (Window == NULL || (Window->Style & (WS_DISABLED | WS_CHILD)))
    {
      IntReleaseWindowObject(Window);
      return(0);
    }
  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
  if (Window->MessageQueue != ThreadQueue)
    {
      IntReleaseWindowObject(Window);
      return(0);
    }
  Prev = Window->MessageQueue->ActiveWindow;
  WinPosSetActiveWindow(Window, FALSE, FALSE);
  IntReleaseWindowObject(Window);
  return(Prev);
}


/* EOF */
