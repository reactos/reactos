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
/* $Id: winpos.c,v 1.13 2003/07/10 00:24:04 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsys/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
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

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define MINMAX_NOSWP  (0x00010000)

#define SWP_EX_PAINTSELF 0x0002

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

  WindowObject = W32kGetWindowObject(hWnd);
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
WinPosCreateIconTitle(PWINDOW_OBJECT WindowObject)
{
  return(NULL);
}

BOOL STATIC FASTCALL
WinPosShowIconTitle(PWINDOW_OBJECT WindowObject, BOOL Show)
{
  PINTERNALPOS InternalPos = NtUserGetProp(WindowObject->Self,
					   AtomInternalPos);
  PWINDOW_OBJECT IconWindow;
  NTSTATUS Status;

  if (InternalPos)
    {
      HWND hWnd = InternalPos->IconTitle;

      if (hWnd == NULL)
	{
	  hWnd = WinPosCreateIconTitle(WindowObject);
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
  PINTERNALPOS InternalPos = NtUserGetProp(WindowObject->Self, 
					   AtomInternalPos);
  if (InternalPos == NULL)
    {
      InternalPos = 
	ExAllocatePool(NonPagedPool, sizeof(INTERNALPOS));
      NtUserSetProp(WindowObject->Self, AtomInternalPos, InternalPos);
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
	    W32kSetRect(NewPos, InternalPos->IconPos.x, InternalPos->IconPos.y,
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
	    W32kSetRect(NewPos, InternalPos->MaxPos.x, InternalPos->MaxPos.y,
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
		    W32kSetRect(NewPos, InternalPos->MaxPos.x,
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

  Pos = NtUserGetProp(Window->Self, AtomInternalPos);
  if (Pos != NULL)
    {
      MinMax.ptMaxPosition = Pos->MaxPos;
    }
  else
    {
      MinMax.ptMaxPosition.x -= XInc;
      MinMax.ptMaxPosition.y -= YInc;
    }

  W32kSendMessage(Window->Self, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax, TRUE);

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

  WindowObject = W32kGetWindowObject(hWnd);
  if (WindowObject == NULL)
    {
      return FALSE;
    }

  NtUserSendMessage(hWnd,
    WM_ACTIVATE,
	  MAKELONG(MouseMsg ? WA_CLICKACTIVE : WA_CLICKACTIVE,
      (WindowObject->Style & WS_MINIMIZE) ? 1 : 0),
    W32kGetDesktopWindow());  /* FIXME: Previous active window */

  W32kReleaseWindowObject(WindowObject);

  return TRUE;
}

LONG STATIC STDCALL
WinPosDoNCCALCSize(PWINDOW_OBJECT Window, PWINDOWPOS WinPos,
		   RECT* WindowRect, RECT* ClientRect)
{
  return 0; //FIXME
}

BOOL STDCALL
WinPosDoWinPosChanging(PWINDOW_OBJECT WindowObject,
		       PWINDOWPOS WinPos,
		       PRECT WindowRect,
		       PRECT ClientRect)
{
  if (!(WinPos->flags & SWP_NOSENDCHANGING))
    {
      W32kSendWINDOWPOSCHANGINGMessage(WindowObject->Self, WinPos);
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
      WindowRect->left = WinPos->x;
      WindowRect->top = WinPos->y;
      WindowRect->right += WinPos->x - WindowObject->WindowRect.left;
      WindowRect->bottom += WinPos->y - WindowObject->WindowRect.top;
    }

  if (!(WinPos->flags & SWP_NOSIZE) || !(WinPos->flags & SWP_NOMOVE))
    {
      WinPosGetNonClientSize(WindowObject->Self, WindowRect, ClientRect);
    }

  WinPos->flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;
  return(TRUE);
}

BOOLEAN STDCALL
WinPosSetWindowPos(HWND Wnd, HWND WndInsertAfter, INT x, INT y, INT cx,
		   INT cy, UINT flags)
{
  PWINDOW_OBJECT Window;
  NTSTATUS Status;
  WINDOWPOS WinPos;
  RECT NewWindowRect;
  RECT NewClientRect;
  HRGN VisRgn = NULL;
  ULONG WvrFlags = 0;

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
  if (FALSE /* FIXME: Check if the window if already active. */)
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

  WinPosDoWinPosChanging(Window, &WinPos, &NewWindowRect, &NewClientRect);

  if ((WinPos.flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) !=
      SWP_NOZORDER)
    {
      /* FIXME: SWP_DoOwnedPopups. */
    }
  
  /* FIXME: Adjust flags based on WndInsertAfter */

  if ((!(WinPos.flags & (SWP_NOREDRAW | SWP_SHOWWINDOW)) &&
       WinPos.flags & (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | 
		       SWP_HIDEWINDOW | SWP_FRAMECHANGED)) != 
      (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER))
    {
      if (Window->Style & WS_CLIPCHILDREN)
	{
	  VisRgn = DceGetVisRgn(Wnd, DCX_WINDOW | DCX_CLIPSIBLINGS, 0, 0);
	}
      else
	{
	  VisRgn = DceGetVisRgn(Wnd, DCX_WINDOW, 0, 0);
	}
    }

  WvrFlags = WinPosDoNCCALCSize(Window, &WinPos, &NewWindowRect,
				&NewClientRect);

  /* FIXME: Relink windows. */

  /* FIXME: Reset active DCEs */

  /* FIXME: Check for redrawing the whole client rect. */

  if (WinPos.flags & SWP_SHOWWINDOW)
    {
      Window->Style |= WS_VISIBLE;
      flags |= SWP_EX_PAINTSELF;
      VisRgn = (HRGN) 1;
    }
  else
    {
      /* FIXME: Move the window bits */
    }

  Window->WindowRect = NewWindowRect;
  Window->ClientRect = NewClientRect;

  if (WinPos.flags & SWP_HIDEWINDOW)
    {
      Window->Style &= ~WS_VISIBLE;
    }

  /* FIXME: Hide or show the claret */

  if (VisRgn)
    {
      if (!(WinPos.flags & SWP_NOREDRAW))
	{
	  if (flags & SWP_EX_PAINTSELF)
	    {
	      PaintRedrawWindow(Window->Self, NULL,
				(VisRgn == (HRGN) 1) ? 0 : VisRgn,
				RDW_ERASE | RDW_FRAME | RDW_INVALIDATE |
				RDW_ALLCHILDREN | RDW_ERASENOW, 
				RDW_EX_XYWINDOW | RDW_EX_USEHRGN);
	    }
	  else
	    {
	      PaintRedrawWindow(Window->Self, NULL,
				(VisRgn == (HRGN) 1) ? 0 : VisRgn,
				RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN |
	                        RDW_ERASENOW,
				RDW_EX_USEHRGN);
	    }
	  /* FIXME: Redraw the window parent. */
	}
      /* FIXME: Delete VisRgn */
    }

  if (!(flags & SWP_NOACTIVATE))
    {
      WinPosChangeActiveWindow(WinPos.hwnd, FALSE);
    }

  /* FIXME: Check some conditions before doing this. */
  W32kSendWINDOWPOSCHANGEDMessage(Window->Self, &WinPos);

  ObmDereferenceObject(Window);
  return(TRUE);
}

LRESULT STDCALL
WinPosGetNonClientSize(HWND Wnd, RECT* WindowRect, RECT* ClientRect)
{
  *ClientRect = *WindowRect;
  return(W32kSendNCCALCSIZEMessage(Wnd, FALSE, ClientRect, NULL));
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
      !W32kIsWindowVisible(Window->Parent->Self) &&
      (Swp & (SWP_NOSIZE | SWP_NOMOVE)) == (SWP_NOSIZE | SWP_NOMOVE))
    {
      if (Cmd == SW_HIDE)
	{
	  Window->Style &= ~WS_VISIBLE;
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
	      if (Wnd == W32kGetActiveWindow())
		{
		  WinPosActivateOtherWindow(Window);
		}
	      /* Revert focus to parent. */
	      if (Wnd == W32kGetFocusWindow() ||
		  W32kIsChildWindow(Wnd, W32kGetFocusWindow()))
		{
		  W32kSetFocusWindow(Window->Parent->Self);
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
  PLIST_ENTRY CurrentEntry;
  PWINDOW_OBJECT Current;


  ExAcquireFastMutexUnsafe(&ScopeWin->ChildrenListLock);
  CurrentEntry = ScopeWin->ChildrenListHead.Flink;
  while (CurrentEntry != &ScopeWin->ChildrenListHead)
    {
      Current = 
	CONTAINING_RECORD(CurrentEntry, WINDOW_OBJECT, SiblingListEntry);

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
	      Point.x -= Current->ClientRect.left;
	      Point.y -= Current->ClientRect.top;

		  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
	      return(WinPosSearchChildren(Current, Point, Window));
	    }

	  ExReleaseFastMutexUnsafe(&ScopeWin->ChildrenListLock);
	  return(0);
	}
      CurrentEntry = CurrentEntry->Flink;
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
  DesktopWindowHandle = W32kGetDesktopWindow();
  DesktopWindow = W32kGetWindowObject(DesktopWindowHandle);
  Point.x += ScopeWin->ClientRect.left - DesktopWindow->ClientRect.left;
  Point.y += ScopeWin->ClientRect.top - DesktopWindow->ClientRect.top;
  W32kReleaseWindowObject(DesktopWindow);

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
      HitTest = W32kSendMessage((*Window)->Self, WM_NCHITTEST, 0,
				MAKELONG(Point.x, Point.y), FALSE);
      /* FIXME: Check for HTTRANSPARENT here. */
    }
  else
    {
      HitTest = HTCLIENT;
    }

  return(HitTest);
}
/* EOF */
