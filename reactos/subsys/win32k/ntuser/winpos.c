/* $Id: winpos.c,v 1.2 2002/07/04 19:56:37 dwelch Exp $
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

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define MINMAX_NOSWP  (0x00010000)

#define SWP_EX_PAINTSELF 0x0002

/* FUNCTIONS *****************************************************************/

POINT STATIC
WinPosFindIconPos(HWND hWnd, POINT Pos)
{
}

HWND STATIC
WinPosCreateIconTitle(PWINDOW_OBJECT WindowObject)
{
  return(NULL);
}

BOOL STATIC
WinPosShowIconTitle(PWINDOW_OBJECT WindowObject, BOOL Show)
{
  PINTERNALPOS InternalPos = WindowObject->InternalPos;
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

PINTERNALPOS STATIC
WinPosInitInternalPos(PWINDOW_OBJECT WindowObject, POINT pt, PRECT RestoreRect)
{
  if (WindowObject->InternalPos == NULL)
    {
      WindowObject->InternalPos = 
	ExAllocatePool(NonPagedPool, sizeof(INTERNALPOS));
      WindowObject->InternalPos->IconTitle = 0;
      WindowObject->InternalPos->NormalRect = WindowObject->WindowRect;
      WindowObject->InternalPos->IconPos.x = 
	WindowObject->InternalPos->MaxPos.x = 0xFFFFFFFF;
      WindowObject->InternalPos->IconPos.y = 
	WindowObject->InternalPos->MaxPos.y = 0xFFFFFFFF;
    }
  if (WindowObject->Style & WS_MINIMIZE)
    {
      WindowObject->InternalPos->IconPos = pt;
    }
  else if (WindowObject->Style & WS_MAXIMIZE)
    {
      WindowObject->InternalPos->MaxPos = pt;
    }
  else if (RestoreRect != NULL)
    {
      WindowObject->InternalPos->NormalRect = *RestoreRect;
    }
  return(WindowObject->InternalPos);
}

UINT
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

UINT
WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
		    POINT* MinTrack, POINT* MaxTrack)
{
}

BOOL STATIC
WinPosChangeActiveWindow(HWND Wnd, BOOL MouseMsg)
{
}

LONG STATIC
WinPosDoNCCALCSize(PWINDOW_OBJECT Window, PWINDOWPOS WinPos,
		   RECT* WindowRect, RECT* ClientRect)
{
}

BOOL
WinPosDoWinPosChanging(PWINDOW_OBJECT WindowObject,
		       PWINDOWPOS WinPos,
		       PRECT WindowRect,
		       PRECT ClientRect)
{
  if (!(WinPos->flags & SWP_NOSENDCHANGING))
    {
      NtUserSendMessage(WindowObject->Self, WM_WINDOWPOSCHANGING, 0,
			(LPARAM)WinPos);
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

      W32kOffsetRect(ClientRect, WinPos->x - WindowObject->WindowRect.left,
		     WinPos->y - WindowObject->WindowRect.top);
    }

  WinPos->flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;
  return(TRUE);
}

BOOLEAN
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

  /* FIXME: Check if the window is for a desktop. */

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
      VisRgn = 1;
    }
  else
    {
      /* FIXME: Move the window bits */
    }

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
				(VisRgn == 1) ? 0 : VisRgn,
				RDW_ERASE | RDW_FRAME | RDW_INVALIDATE |
				RDW_ALLCHILDREN, 
				RDW_EX_XYWINDOW | RDW_EX_USEHRGN);
	    }
	  else
	    {
	      PaintRedrawWindow(Window->Self, NULL,
				(VisRgn == 1) ? 0 : VisRgn,
				RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN,
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
  NtUserSendMessage(WinPos.hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&WinPos);

  ObmDereferenceObject(Window);
  return(TRUE);
}

LRESULT
WinPosGetNonClientSize(HWND Wnd, RECT* WindowRect, RECT* ClientRect)
{
}

BOOLEAN
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
      /* !IsWindowVisible(WindowObject->Parent->Self) && */
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
	    }
	}
      /* FIXME: Check for window destruction. */
      /* FIXME: Show title for minimized windows. */
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
  ObmDereferenceObject(Window);
  return(WasVisible);
}
