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
/* $Id$
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

#include <w32k.h>

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

/* FUNCTIONS *****************************************************************/

BOOL FASTCALL
IntGetClientOrigin(HWND hWnd, LPPOINT Point)
{
  PWINDOW_OBJECT Window;

  Window = IntGetWindowObject((hWnd ? hWnd : IntGetDesktopWindow()));
  if (Window == NULL)
    {
      Point->x = Point->y = 0;
      return FALSE;
    }
  Point->x = Window->ClientRect.left;
  Point->y = Window->ClientRect.top;

  IntReleaseWindowObject(Window);
  return TRUE;
}




BOOL FASTCALL
UserGetClientOrigin(HWND hWnd, LPPOINT Point)
{
  BOOL Ret;
  POINT pt;
  NTSTATUS Status;

  if(!Point)
  {
    SetLastWin32Error(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  Ret = IntGetClientOrigin(hWnd, &pt);

  if(!Ret)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return FALSE;
  }

  Status = MmCopyToCaller(Point, &pt, sizeof(POINT));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return FALSE;
  }

  return Ret;
}



BOOL STDCALL
NtUserGetClientOrigin(HWND hWnd, LPPOINT Point)
{
   DECLARE_RETURN(BOOL);  

   DPRINT("Enter NtUserGetClientOrigin\n");
   UserEnterShared();
   
   RETURN(UserGetClientOrigin(hWnd, Point));
   
CLEANUP:
   DPRINT("Leave NtUserGetClientOrigin, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*******************************************************************
 *         WinPosActivateOtherWindow
 *
 *  Activates window other than pWnd.
 */
VOID FASTCALL
co_WinPosActivateOtherWindow(PWINDOW_OBJECT Window)
{
  PWINDOW_OBJECT Wnd, Old;
  HWND Fg;

  if (!Window || IntIsDesktopWindow(Window))
  {
    IntSetFocusMessageQueue(NULL);
    return;
  }

  /* If this is popup window, try to activate the owner first. */
  if ((Window->Style & WS_POPUP) && (Wnd = IntGetOwner(Window)))
  {
    for(;;)
    {
      Old = Wnd;
      Wnd = IntGetParentObject(Wnd);
      if(IntIsDesktopWindow(Wnd))
      {
        IntReleaseWindowObject(Wnd);
        Wnd = Old;
        break;
      }
      IntReleaseWindowObject(Old);
    }

    if ((Wnd->Style & (WS_DISABLED | WS_VISIBLE)) == WS_VISIBLE &&
        (Wnd->Style & (WS_POPUP | WS_CHILD)) != WS_CHILD)
      goto done;

    IntReleaseWindowObject(Wnd);
  }

  /* Pick a next top-level window. */
  /* FIXME: Search for non-tooltip windows first. */
  Wnd = Window;
  while (Wnd != NULL)
  {
    Old = Wnd;
    if (Old->NextSibling == NULL)
    {
      Wnd = NULL;
      if (Old != Window)
        IntReleaseWindowObject(Old);
      break;
    }
    Wnd = IntGetWindowObject(Old->NextSibling->hSelf);
    if (Old != Window)
      IntReleaseWindowObject(Old);
    if ((Wnd->Style & (WS_DISABLED | WS_VISIBLE)) == WS_VISIBLE &&
        (Wnd->Style & (WS_POPUP | WS_CHILD)) != WS_CHILD)
      break;
  }

done:
  Fg = UserGetForegroundWindow();
  if (Wnd && (!Fg || Window->hSelf == Fg))
  {
    if (co_IntSetForegroundWindow(Wnd))
    {
      IntReleaseWindowObject(Wnd);
      return;
    }
  }
  if (!co_IntSetActiveWindow(Wnd))
    co_IntSetActiveWindow(0);
  if (Wnd)
    IntReleaseWindowObject(Wnd);
}


UINT
FASTCALL
co_WinPosArrangeIconicWindows(PWINDOW_OBJECT parent)
{
    RECT rectParent;
    HWND hwndChild;
    INT i, x, y, xspacing, yspacing;
    HWND *List = IntWinListChildren(parent);
    
    IntGetClientRect( parent, &rectParent );
    x = rectParent.left;
    y = rectParent.bottom;
    
    xspacing = UserGetSystemMetrics(SM_CXMINSPACING);
    yspacing = UserGetSystemMetrics(SM_CYMINSPACING);

    DPRINT("X:%d Y:%d XS:%d YS:%d\n",x,y,xspacing,yspacing);

    for( i = 0; List[i]; i++)
    {
       hwndChild = List[i];
        
      if((UserGetWindowLong( hwndChild, GWL_STYLE, FALSE) & WS_MINIMIZE) != 0 )
      {
         co_WinPosSetWindowPos( hwndChild, 0, x + UserGetSystemMetrics(SM_CXBORDER),
                      y - yspacing - UserGetSystemMetrics(SM_CYBORDER)
                     , 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );
         if (x <= rectParent.right - xspacing) x += xspacing;
         else
         {
            x = rectParent.left;
            y -= yspacing;
         }
      }
    }
    ExFreePool(List);
    return yspacing;
}


VOID STATIC FASTCALL
WinPosFindIconPos(PWINDOW_OBJECT Window, POINT *Pos)
{
  /* FIXME */
}

PINTERNALPOS FASTCALL
WinPosInitInternalPos(PWINDOW_OBJECT Window, POINT *pt, PRECT RestoreRect)
{
  PWINDOW_OBJECT Parent;
  UINT XInc, YInc;

  if (Window->InternalPos == NULL)
    {
      RECT WorkArea;
      PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop; /* Or rather get it from the window? */

      Parent = Window->Parent;
      if(Parent)
      {
        if(IntIsDesktopWindow(Parent))
          IntGetDesktopWorkArea(Desktop, &WorkArea);
        else
          WorkArea = Parent->ClientRect;
      }
      else
        IntGetDesktopWorkArea(Desktop, &WorkArea);

      Window->InternalPos = ExAllocatePoolWithTag(PagedPool, sizeof(INTERNALPOS), TAG_WININTLIST);
      if(!Window->InternalPos)
      {
        DPRINT1("Failed to allocate INTERNALPOS structure for window 0x%x\n", Window->hSelf);
        return NULL;
      }
      Window->InternalPos->NormalRect = Window->WindowRect;
      IntGetWindowBorderMeasures(Window, &XInc, &YInc);
      Window->InternalPos->MaxPos.x = WorkArea.left - XInc;
      Window->InternalPos->MaxPos.y = WorkArea.top - YInc;
      Window->InternalPos->IconPos.x = WorkArea.left;
      Window->InternalPos->IconPos.y = WorkArea.bottom - UserGetSystemMetrics(SM_CYMINIMIZED);
    }
  if (Window->Style & WS_MINIMIZE)
    {
      Window->InternalPos->IconPos = *pt;
    }
  else if (Window->Style & WS_MAXIMIZE)
    {
      Window->InternalPos->MaxPos = *pt;
    }
  else if (RestoreRect != NULL)
    {
      Window->InternalPos->NormalRect = *RestoreRect;
    }
  return(Window->InternalPos);
}

UINT FASTCALL
co_WinPosMinMaximize(PWINDOW_OBJECT Window, UINT ShowFlag, RECT* NewPos)
{
  POINT Size;
  PINTERNALPOS InternalPos;
  UINT SwpFlags = 0;

  Size.x = Window->WindowRect.left;
  Size.y = Window->WindowRect.top;
  InternalPos = WinPosInitInternalPos(Window, &Size, &Window->WindowRect);

  if (InternalPos)
    {
      if (Window->Style & WS_MINIMIZE)
	{
     if (!co_IntSendMessage(Window->hSelf, WM_QUERYOPEN, 0, 0))
	    {
	      return(SWP_NOSIZE | SWP_NOMOVE);
	    }
	  SwpFlags |= SWP_NOCOPYBITS;
	}
      switch (ShowFlag)
	{
	case SW_MINIMIZE:
	  {
	    if (Window->Style & WS_MAXIMIZE)
	      {
		Window->Flags |= WINDOWOBJECT_RESTOREMAX;
		Window->Style &= ~WS_MAXIMIZE;
	      }
	    else
	      {
		Window->Flags &= ~WINDOWOBJECT_RESTOREMAX;
	      }
       co_UserRedrawWindow(Window, NULL, 0, RDW_VALIDATE | RDW_NOERASE |
	       RDW_NOINTERNALPAINT);
	    Window->Style |= WS_MINIMIZE;
	    WinPosFindIconPos(Window, &InternalPos->IconPos);
	    IntGdiSetRect(NewPos, InternalPos->IconPos.x, InternalPos->IconPos.y,
			  UserGetSystemMetrics(SM_CXMINIMIZED),
			  UserGetSystemMetrics(SM_CYMINIMIZED));
	    SwpFlags |= SWP_NOCOPYBITS;
	    break;
	  }

	case SW_MAXIMIZE:
	  {
       co_WinPosGetMinMaxInfo(Window, &Size, &InternalPos->MaxPos,
				NULL, NULL);
	    DPRINT("Maximize: %d,%d %dx%d\n",
	       InternalPos->MaxPos.x, InternalPos->MaxPos.y, Size.x, Size.y);
	    if (Window->Style & WS_MINIMIZE)
	      {
		Window->Style &= ~WS_MINIMIZE;
	      }
	    Window->Style |= WS_MAXIMIZE;
	    IntGdiSetRect(NewPos, InternalPos->MaxPos.x, InternalPos->MaxPos.y,
			  Size.x, Size.y);
	    break;
	  }

	case SW_RESTORE:
	  {
	    if (Window->Style & WS_MINIMIZE)
	      {
		Window->Style &= ~WS_MINIMIZE;
		if (Window->Flags & WINDOWOBJECT_RESTOREMAX)
		  {
          co_WinPosGetMinMaxInfo(Window, &Size,
					&InternalPos->MaxPos, NULL, NULL);
		    Window->Style |= WS_MAXIMIZE;
		    IntGdiSetRect(NewPos, InternalPos->MaxPos.x,
				  InternalPos->MaxPos.y, Size.x, Size.y);
		    break;
		  }
		else
		  {
		    *NewPos = InternalPos->NormalRect;
		    NewPos->right -= NewPos->left;
		    NewPos->bottom -= NewPos->top;
		    break;
		  }
	      }
	    else
	      {
		if (!(Window->Style & WS_MAXIMIZE))
		  {
		    return 0;
		  }
		Window->Style &= ~WS_MAXIMIZE;
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

VOID FASTCALL
WinPosFillMinMaxInfoStruct(PWINDOW_OBJECT Window, MINMAXINFO *Info)
{
  UINT XInc, YInc;
  RECT WorkArea;
  PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop; /* Or rather get it from the window? */

  IntGetDesktopWorkArea(Desktop, &WorkArea);

  /* Get default values. */
  Info->ptMaxSize.x = WorkArea.right - WorkArea.left;
  Info->ptMaxSize.y = WorkArea.bottom - WorkArea.top;
  Info->ptMinTrackSize.x = UserGetSystemMetrics(SM_CXMINTRACK);
  Info->ptMinTrackSize.y = UserGetSystemMetrics(SM_CYMINTRACK);
  Info->ptMaxTrackSize.x = Info->ptMaxSize.x;
  Info->ptMaxTrackSize.y = Info->ptMaxSize.y;

  IntGetWindowBorderMeasures(Window, &XInc, &YInc);
  Info->ptMaxSize.x += 2 * XInc;
  Info->ptMaxSize.y += 2 * YInc;

  if (Window->InternalPos != NULL)
    {
      Info->ptMaxPosition = Window->InternalPos->MaxPos;
    }
  else
    {
      Info->ptMaxPosition.x -= WorkArea.left + XInc;
      Info->ptMaxPosition.y -= WorkArea.top + YInc;
    }
}

UINT FASTCALL
co_WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
		    POINT* MinTrack, POINT* MaxTrack)
{
  MINMAXINFO MinMax;

  WinPosFillMinMaxInfoStruct(Window, &MinMax);

  co_IntSendMessage(Window->hSelf, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax);

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

STATIC VOID FASTCALL
FixClientRect(PRECT ClientRect, PRECT WindowRect)
{
  if (ClientRect->left < WindowRect->left)
    {
      ClientRect->left = WindowRect->left;
    }
  else if (WindowRect->right < ClientRect->left)
    {
      ClientRect->left = WindowRect->right;
    }
  if (ClientRect->right < WindowRect->left)
    {
      ClientRect->right = WindowRect->left;
    }
  else if (WindowRect->right < ClientRect->right)
    {
      ClientRect->right = WindowRect->right;
    }
  if (ClientRect->top < WindowRect->top)
    {
      ClientRect->top = WindowRect->top;
    }
  else if (WindowRect->bottom < ClientRect->top)
    {
      ClientRect->top = WindowRect->bottom;
    }
  if (ClientRect->bottom < WindowRect->top)
    {
      ClientRect->bottom = WindowRect->top;
    }
  else if (WindowRect->bottom < ClientRect->bottom)
    {
      ClientRect->bottom = WindowRect->bottom;
    }
}

LONG STATIC FASTCALL
co_WinPosDoNCCALCSize(PWINDOW_OBJECT Window, PWINDOWPOS WinPos,
		   RECT* WindowRect, RECT* ClientRect)
{
  PWINDOW_OBJECT Parent;
  UINT wvrFlags = 0;

  /* Send WM_NCCALCSIZE message to get new client area */
  if ((WinPos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE)
    {
      NCCALCSIZE_PARAMS params;
      WINDOWPOS winposCopy;

      params.rgrc[0] = *WindowRect;
      params.rgrc[1] = Window->WindowRect;
      params.rgrc[2] = Window->ClientRect;
      Parent = Window->Parent;
      if (0 != (Window->Style & WS_CHILD) && Parent)
	{
	  IntGdiOffsetRect(&(params.rgrc[0]), - Parent->ClientRect.left,
	                      - Parent->ClientRect.top);
	  IntGdiOffsetRect(&(params.rgrc[1]), - Parent->ClientRect.left,
	                      - Parent->ClientRect.top);
	  IntGdiOffsetRect(&(params.rgrc[2]), - Parent->ClientRect.left,
	                      - Parent->ClientRect.top);
	}
      params.lppos = &winposCopy;
      winposCopy = *WinPos;

      wvrFlags = co_IntSendMessage(Window->hSelf, WM_NCCALCSIZE, TRUE, (LPARAM) &params);

      /* If the application send back garbage, ignore it */
      if (params.rgrc[0].left <= params.rgrc[0].right &&
          params.rgrc[0].top <= params.rgrc[0].bottom)
	{
          *ClientRect = params.rgrc[0];
	  if ((Window->Style & WS_CHILD) && Parent)
	    {
	      IntGdiOffsetRect(ClientRect, Parent->ClientRect.left,
	                      Parent->ClientRect.top);
	    }
          FixClientRect(ClientRect, WindowRect);
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

BOOL FASTCALL
co_WinPosDoWinPosChanging(PWINDOW_OBJECT Window,
		       PWINDOWPOS WinPos,
		       PRECT WindowRect,
		       PRECT ClientRect)
{
  INT X, Y;

  if (!(WinPos->flags & SWP_NOSENDCHANGING))
    {
      co_IntSendMessage(Window->hSelf, WM_WINDOWPOSCHANGING, 0, (LPARAM) WinPos);
    }

  *WindowRect = Window->WindowRect;
  *ClientRect = Window->ClientRect;

  if (!(WinPos->flags & SWP_NOSIZE))
    {
      WindowRect->right = WindowRect->left + WinPos->cx;
      WindowRect->bottom = WindowRect->top + WinPos->cy;
    }

  if (!(WinPos->flags & SWP_NOMOVE))
    {
      PWINDOW_OBJECT Parent;
      X = WinPos->x;
      Y = WinPos->y;
      Parent = Window->Parent;
      if ((0 != (Window->Style & WS_CHILD)) && Parent)
	{
	  X += Parent->ClientRect.left;
	  Y += Parent->ClientRect.top;
	}

      WindowRect->left = X;
      WindowRect->top = Y;
      WindowRect->right += X - Window->WindowRect.left;
      WindowRect->bottom += Y - Window->WindowRect.top;
      IntGdiOffsetRect(ClientRect,
        X - Window->WindowRect.left,
        Y - Window->WindowRect.top);
    }

  WinPos->flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;

  return TRUE;
}

/*
 * Fix Z order taking into account owned popups -
 * basically we need to maintain them above the window that owns them
 */
HWND FASTCALL
WinPosDoOwnedPopups(HWND hWnd, HWND hWndInsertAfter)
{
   HWND *List = NULL;
   HWND Owner = UserGetWindow(hWnd, GW_OWNER);
   LONG Style = UserGetWindowLong(hWnd, GWL_STYLE, FALSE);
   PWINDOW_OBJECT DesktopWindow, ChildObject;
   int i;

   if ((Style & WS_POPUP) && Owner)
   {
      /* Make sure this popup stays above the owner */
      HWND hWndLocalPrev = HWND_TOPMOST;

      if (hWndInsertAfter != HWND_TOPMOST)
      {
         DesktopWindow = IntGetWindowObject(IntGetDesktopWindow());
         List = IntWinListChildren(DesktopWindow);
         IntReleaseWindowObject(DesktopWindow);
         if (List != NULL)
         {
            for (i = 0; List[i]; i++)
            {
               if (List[i] == Owner) break;
               if (HWND_TOP == hWndInsertAfter)
               {
                 ChildObject = IntGetWindowObject(List[i]);
                 if (NULL != ChildObject)
                 {
                   if (0 == (ChildObject->ExStyle & WS_EX_TOPMOST))
                   {
                     IntReleaseWindowObject(ChildObject);
                     break;
                   }
                   IntReleaseWindowObject(ChildObject);
                 }
               }
               if (List[i] != hWnd) hWndLocalPrev = List[i];
               if (hWndLocalPrev == hWndInsertAfter) break;
            }
            hWndInsertAfter = hWndLocalPrev;
         }
      }
   }
   else if (Style & WS_CHILD)
   {
      return hWndInsertAfter;
   }

   if (!List)
   {
      DesktopWindow = IntGetWindowObject(IntGetDesktopWindow());
      List = IntWinListChildren(DesktopWindow);
      IntReleaseWindowObject(DesktopWindow);
   }
   if (List != NULL)
   {
      for (i = 0; List[i]; i++)
      {
         if (List[i] == hWnd)
            break;
         if ((UserGetWindowLong(List[i], GWL_STYLE, FALSE) & WS_POPUP) &&
             UserGetWindow(List[i], GW_OWNER) == hWnd)
         {
            co_WinPosSetWindowPos(List[i], hWndInsertAfter, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
            hWndInsertAfter = List[i];
         }
      }
      ExFreePool(List);
   }

   return hWndInsertAfter;
}

/***********************************************************************
 *	     WinPosInternalMoveWindow
 *
 * Update WindowRect and ClientRect of Window and all of its children
 * We keep both WindowRect and ClientRect in screen coordinates internally
 */
VOID STATIC FASTCALL
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

   for(Child = Window->FirstChild; Child; Child = Child->NextSibling)
   {
     WinPosInternalMoveWindow(Child, MoveX, MoveY);
   }
}

/*
 * WinPosFixupSWPFlags
 *
 * Fix redundant flags and values in the WINDOWPOS structure.
 */

BOOL FASTCALL
WinPosFixupFlags(WINDOWPOS *WinPos, PWINDOW_OBJECT Window)
{
   if (Window->Style & WS_VISIBLE)
   {
      WinPos->flags &= ~SWP_SHOWWINDOW;
   }
   else
   {
      WinPos->flags &= ~SWP_HIDEWINDOW;
      if (!(WinPos->flags & SWP_SHOWWINDOW))
         WinPos->flags |= SWP_NOREDRAW;
   }

   WinPos->cx = max(WinPos->cx, 0);
   WinPos->cy = max(WinPos->cy, 0);

   /* Check for right size */
   if (Window->WindowRect.right - Window->WindowRect.left == WinPos->cx &&
       Window->WindowRect.bottom - Window->WindowRect.top == WinPos->cy)
   {
      WinPos->flags |= SWP_NOSIZE;
   }

   /* Check for right position */
   if (Window->WindowRect.left == WinPos->x &&
       Window->WindowRect.top == WinPos->y)
   {
      WinPos->flags |= SWP_NOMOVE;
   }

   if (WinPos->hwnd == UserGetForegroundWindow())
   {
      WinPos->flags |= SWP_NOACTIVATE;   /* Already active */
   }
   else
   if ((Window->Style & (WS_POPUP | WS_CHILD)) != WS_CHILD)
   {
      /* Bring to the top when activating */
      if (!(WinPos->flags & SWP_NOACTIVATE))
      {
         WinPos->flags &= ~SWP_NOZORDER;
         WinPos->hwndInsertAfter = (0 != (Window->ExStyle & WS_EX_TOPMOST) ?
                                    HWND_TOPMOST : HWND_TOP);
         return TRUE;
      }
   }

   /* Check hwndInsertAfter */
   if (!(WinPos->flags & SWP_NOZORDER))
   {
      /* Fix sign extension */
      if (WinPos->hwndInsertAfter == (HWND)0xffff)
      {
         WinPos->hwndInsertAfter = HWND_TOPMOST;
      }
      else if (WinPos->hwndInsertAfter == (HWND)0xfffe)
      {
         WinPos->hwndInsertAfter = HWND_NOTOPMOST;
      }

      if (WinPos->hwndInsertAfter == HWND_NOTOPMOST)
      {
         WinPos->hwndInsertAfter = HWND_TOP;
      }
      else if (HWND_TOP == WinPos->hwndInsertAfter
               && 0 != (Window->ExStyle & WS_EX_TOPMOST))
      {
         /* Keep it topmost when it's already topmost */
         WinPos->hwndInsertAfter = HWND_TOPMOST;
      }

      /* hwndInsertAfter must be a sibling of the window */
      if (HWND_TOPMOST != WinPos->hwndInsertAfter
          && HWND_TOP != WinPos->hwndInsertAfter
          && HWND_NOTOPMOST != WinPos->hwndInsertAfter
          && HWND_BOTTOM != WinPos->hwndInsertAfter)
      {
         PWINDOW_OBJECT Parent = Window->Parent;
         if (UserGetAncestor(WinPos->hwndInsertAfter, GA_PARENT) !=
             (Parent ? Parent->hSelf : NULL))
         {
            return FALSE;
         }
         else
         {
            /*
             * We don't need to change the Z order of hwnd if it's already
             * inserted after hwndInsertAfter or when inserting hwnd after
             * itself.
             */
            if ((WinPos->hwnd == WinPos->hwndInsertAfter) ||
                (WinPos->hwnd == UserGetWindow(WinPos->hwndInsertAfter, GW_HWNDNEXT)))
            {
               WinPos->flags |= SWP_NOZORDER;
            }
         }
      }
   }

   return TRUE;
}

/* x and y are always screen relative */
BOOLEAN FASTCALL
co_WinPosSetWindowPos(HWND Wnd, HWND WndInsertAfter, INT x, INT y, INT cx,
		   INT cy, UINT flags)
{
   PWINDOW_OBJECT Window;
   WINDOWPOS WinPos;
   RECT NewWindowRect;
   RECT NewClientRect;
   PROSRGNDATA VisRgn;
   HRGN VisBefore = NULL;
   HRGN VisAfter = NULL;
   HRGN DirtyRgn = NULL;
   HRGN ExposedRgn = NULL;
   HRGN CopyRgn = NULL;
   ULONG WvrFlags = 0;
   RECT OldWindowRect, OldClientRect;
   int RgnType;
   HDC Dc;
   RECT CopyRect;
   RECT TempRect;

   /* FIXME: Get current active window from active queue. */

   Window = IntGetWindowObject(Wnd);
   if (!Window)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   /*
    * Only allow CSRSS to mess with the desktop window
    */
   if (Wnd == IntGetDesktopWindow() &&
       Window->OwnerThread->ThreadsProcess != PsGetCurrentProcess())
   {
      IntReleaseWindowObject(Window);
      return FALSE;
   }

   WinPos.hwnd = Wnd;
   WinPos.hwndInsertAfter = WndInsertAfter;
   WinPos.x = x;
   WinPos.y = y;
   WinPos.cx = cx;
   WinPos.cy = cy;
   WinPos.flags = flags;

   co_WinPosDoWinPosChanging(Window, &WinPos, &NewWindowRect, &NewClientRect);

   /* Fix up the flags. */
   if (!WinPosFixupFlags(&WinPos, Window))
   {
      IntReleaseWindowObject(Window);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   /* Does the window still exist? */
   if (!IntIsWindow(WinPos.hwnd))
   {
      IntReleaseWindowObject(Window);
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   if ((WinPos.flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) !=
       SWP_NOZORDER &&
       UserGetAncestor(WinPos.hwnd, GA_PARENT) == IntGetDesktopWindow())
   {
      WinPos.hwndInsertAfter = WinPosDoOwnedPopups(WinPos.hwnd, WinPos.hwndInsertAfter);
   }

   if (!(WinPos.flags & SWP_NOREDRAW))
   {
      /* Compute the visible region before the window position is changed */
      if (!(WinPos.flags & (SWP_NOREDRAW | SWP_SHOWWINDOW)) &&
          (WinPos.flags & (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                           SWP_HIDEWINDOW | SWP_FRAMECHANGED)) !=
          (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER))
      {
         VisBefore = VIS_ComputeVisibleRegion(Window, FALSE, FALSE, TRUE);
         VisRgn = NULL;

         if (VisBefore != NULL && (VisRgn = (PROSRGNDATA)RGNDATA_LockRgn(VisBefore)) &&
             UnsafeIntGetRgnBox(VisRgn, &TempRect) == NULLREGION)
         {
            RGNDATA_UnlockRgn(VisRgn);
            NtGdiDeleteObject(VisBefore);
            VisBefore = NULL;
         }
         else if(VisRgn)
         {
            RGNDATA_UnlockRgn(VisRgn);
         }
      }
   }

   WvrFlags = co_WinPosDoNCCALCSize(Window, &WinPos, &NewWindowRect, &NewClientRect);

   /* Relink windows. (also take into account shell window in hwndShellWindow) */
   if (!(WinPos.flags & SWP_NOZORDER) && WinPos.hwnd != UserGetShellWindow())
   {
      PWINDOW_OBJECT ParentWindow;
      PWINDOW_OBJECT Sibling;
      PWINDOW_OBJECT InsertAfterWindow;

      if ((ParentWindow = IntGetParentObject(Window)))
      {
         if (HWND_TOPMOST == WinPos.hwndInsertAfter)
         {
            InsertAfterWindow = NULL;
         }
         else if (HWND_TOP == WinPos.hwndInsertAfter
                  || HWND_NOTOPMOST == WinPos.hwndInsertAfter)
         {
            InsertAfterWindow = NULL;
            Sibling = ParentWindow->FirstChild;
            while (NULL != Sibling && 0 != (Sibling->ExStyle & WS_EX_TOPMOST))
            {
               InsertAfterWindow = Sibling;
               Sibling = Sibling->NextSibling;
            }
            if (NULL != InsertAfterWindow)
            {
              IntReferenceWindowObject(InsertAfterWindow);
            }
         }
         else if (WinPos.hwndInsertAfter == HWND_BOTTOM)
         {
            if(ParentWindow->LastChild)
            {
               IntReferenceWindowObject(ParentWindow->LastChild);
               InsertAfterWindow = ParentWindow->LastChild;
            }
            else
              InsertAfterWindow = NULL;
         }
         else
            InsertAfterWindow = IntGetWindowObject(WinPos.hwndInsertAfter);
         /* Do nothing if hwndInsertAfter is HWND_BOTTOM and Window is already
            the last window */
         if (InsertAfterWindow != Window)
         {
             IntUnlinkWindow(Window);
             IntLinkWindow(Window, ParentWindow, InsertAfterWindow);
         }
         if (InsertAfterWindow != NULL)
            IntReleaseWindowObject(InsertAfterWindow);
         if ((HWND_TOPMOST == WinPos.hwndInsertAfter)
             || (0 != (Window->ExStyle & WS_EX_TOPMOST)
                 && NULL != Window->PrevSibling
                 && 0 != (Window->PrevSibling->ExStyle & WS_EX_TOPMOST))
             || (NULL != Window->NextSibling
                 && 0 != (Window->NextSibling->ExStyle & WS_EX_TOPMOST)))
         {
            Window->ExStyle |= WS_EX_TOPMOST;
         }
         else
         {
            Window->ExStyle &= ~ WS_EX_TOPMOST;
         }

         IntReleaseWindowObject(ParentWindow);
      }
   }

   OldWindowRect = Window->WindowRect;
   OldClientRect = Window->ClientRect;

   if (OldClientRect.bottom - OldClientRect.top ==
       NewClientRect.bottom - NewClientRect.top)
   {
      WvrFlags &= ~WVR_VREDRAW;
   }

   if (OldClientRect.right - OldClientRect.left ==
       NewClientRect.right - NewClientRect.left)
   {
      WvrFlags &= ~WVR_HREDRAW;
   }

   /* FIXME: Actually do something with WVR_VALIDRECTS */

   if (NewClientRect.left != OldClientRect.left ||
       NewClientRect.top != OldClientRect.top)
   {
      WinPosInternalMoveWindow(Window,
                               NewClientRect.left - OldClientRect.left,
                               NewClientRect.top - OldClientRect.top);
   }

   Window->WindowRect = NewWindowRect;
   Window->ClientRect = NewClientRect;

   if (!(WinPos.flags & SWP_SHOWWINDOW) && (WinPos.flags & SWP_HIDEWINDOW))
   {
      /* Clear the update region */
      co_UserRedrawWindow(Window, NULL, 0, RDW_VALIDATE | RDW_NOFRAME |
                      RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ALLCHILDREN);
      Window->Style &= ~WS_VISIBLE;
   }
   else if (WinPos.flags & SWP_SHOWWINDOW)
   {
      Window->Style |= WS_VISIBLE;
   }

   DceResetActiveDCEs(Window);

   if (!(WinPos.flags & SWP_NOREDRAW))
   {
      /* Determine the new visible region */
      VisAfter = VIS_ComputeVisibleRegion(Window, FALSE, FALSE, TRUE);
      VisRgn = NULL;

      if (VisAfter != NULL && (VisRgn = (PROSRGNDATA)RGNDATA_LockRgn(VisAfter)) &&
          UnsafeIntGetRgnBox(VisRgn, &TempRect) == NULLREGION)
      {
         RGNDATA_UnlockRgn(VisRgn);
         NtGdiDeleteObject(VisAfter);
         VisAfter = NULL;
      }
      else if(VisRgn)
      {
         RGNDATA_UnlockRgn(VisRgn);
      }

      /*
       * Determine which pixels can be copied from the old window position
       * to the new. Those pixels must be visible in both the old and new
       * position. Also, check the class style to see if the windows of this
       * class need to be completely repainted on (horizontal/vertical) size
       * change.
       */
      if (VisBefore != NULL && VisAfter != NULL && !(WinPos.flags & SWP_NOCOPYBITS) &&
          ((WinPos.flags & SWP_NOSIZE) || !(WvrFlags & WVR_REDRAW)))
      {
         CopyRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
         RgnType = NtGdiCombineRgn(CopyRgn, VisAfter, VisBefore, RGN_AND);

         /*
          * If this is (also) a window resize, the whole nonclient area
          * needs to be repainted. So we limit the copy to the client area,
          * 'cause there is no use in copying it (would possibly cause
          * "flashing" too). However, if the copy region is already empty,
          * we don't have to crop (can't take anything away from an empty
          * region...)
          */
         if (!(WinPos.flags & SWP_NOSIZE) && RgnType != ERROR &&
             RgnType != NULLREGION)
         {
            RECT ORect = OldClientRect;
            RECT NRect = NewClientRect;
            IntGdiOffsetRect(&ORect, - OldWindowRect.left, - OldWindowRect.top);
            IntGdiOffsetRect(&NRect, - NewWindowRect.left, - NewWindowRect.top);
            IntGdiIntersectRect(&CopyRect, &ORect, &NRect);
            REGION_CropRgn(CopyRgn, CopyRgn, &CopyRect, NULL);
         }

         /* No use in copying bits which are in the update region. */
         if (Window->UpdateRegion != NULL)
         {
            NtGdiCombineRgn(CopyRgn, CopyRgn, Window->UpdateRegion, RGN_DIFF);
         }
         if (Window->NCUpdateRegion != NULL)
         {
            NtGdiCombineRgn(CopyRgn, CopyRgn, Window->NCUpdateRegion, RGN_DIFF);
         }

         /*
          * Now, get the bounding box of the copy region. If it's empty
          * there's nothing to copy. Also, it's no use copying bits onto
          * themselves.
          */
         if ((VisRgn = (PROSRGNDATA)RGNDATA_LockRgn(CopyRgn)) &&
             UnsafeIntGetRgnBox(VisRgn, &CopyRect) == NULLREGION)
         {
            /* Nothing to copy, clean up */
            RGNDATA_UnlockRgn(VisRgn);
            NtGdiDeleteObject(CopyRgn);
            CopyRgn = NULL;
         }
         else if (OldWindowRect.left != NewWindowRect.left ||
                  OldWindowRect.top != NewWindowRect.top)
         {
            if(VisRgn)
            {
               RGNDATA_UnlockRgn(VisRgn);
            }
            /*
             * Small trick here: there is no function to bitblt a region. So
             * we set the region as the clipping region, take the bounding box
             * of the region and bitblt that. Since nothing outside the clipping
             * region is copied, this has the effect of bitblt'ing the region.
             *
             * Since NtUserGetDCEx takes ownership of the clip region, we need
             * to create a copy of CopyRgn and pass that. We need CopyRgn later
             */
            HRGN ClipRgn = NtGdiCreateRectRgn(0, 0, 0, 0);

            NtGdiCombineRgn(ClipRgn, CopyRgn, NULL, RGN_COPY);
            Dc = UserGetDCEx(Window, ClipRgn, DCX_WINDOW | DCX_CACHE |
               DCX_INTERSECTRGN | DCX_CLIPSIBLINGS);
            NtGdiBitBlt(Dc,
               CopyRect.left, CopyRect.top, CopyRect.right - CopyRect.left,
               CopyRect.bottom - CopyRect.top, Dc,
               CopyRect.left + (OldWindowRect.left - NewWindowRect.left),
               CopyRect.top + (OldWindowRect.top - NewWindowRect.top), SRCCOPY);
            UserReleaseDC(Window, Dc);
            IntValidateParent(Window, CopyRgn);
         }
         else if(VisRgn)
         {
            RGNDATA_UnlockRgn(VisRgn);
         }
      }
      else
      {
         CopyRgn = NULL;
      }

      /* We need to redraw what wasn't visible before */
      if (VisAfter != NULL)
      {
         DirtyRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
         if (CopyRgn != NULL)
         {
            RgnType = NtGdiCombineRgn(DirtyRgn, VisAfter, CopyRgn, RGN_DIFF);
         }
         else
         {
            RgnType = NtGdiCombineRgn(DirtyRgn, VisAfter, 0, RGN_COPY);
         }
         if (RgnType != ERROR && RgnType != NULLREGION)
         {
            NtGdiOffsetRgn(DirtyRgn,
               Window->WindowRect.left - Window->ClientRect.left,
               Window->WindowRect.top - Window->ClientRect.top);
            co_UserRedrawWindow(Window, NULL, DirtyRgn,
               RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
         }
         NtGdiDeleteObject(DirtyRgn);
      }

      if (CopyRgn != NULL)
      {
         NtGdiDeleteObject(CopyRgn);
      }

      /* Expose what was covered before but not covered anymore */
      if (VisBefore != NULL)
      {
         ExposedRgn = NtGdiCreateRectRgn(0, 0, 0, 0);
         NtGdiCombineRgn(ExposedRgn, VisBefore, NULL, RGN_COPY);
         NtGdiOffsetRgn(ExposedRgn, OldWindowRect.left - NewWindowRect.left,
                        OldWindowRect.top - NewWindowRect.top);
         if (VisAfter != NULL)
            RgnType = NtGdiCombineRgn(ExposedRgn, ExposedRgn, VisAfter, RGN_DIFF);
         else
            RgnType = SIMPLEREGION;

         if (RgnType != ERROR && RgnType != NULLREGION)
         {
            co_VIS_WindowLayoutChanged(Window, ExposedRgn);
         }
         NtGdiDeleteObject(ExposedRgn);
         NtGdiDeleteObject(VisBefore);
      }

      if (VisAfter != NULL)
      {
         NtGdiDeleteObject(VisAfter);
      }

      if (!(WinPos.flags & SWP_NOACTIVATE))
      {
         if ((Window->Style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
         {
            co_IntSendMessage(WinPos.hwnd, WM_CHILDACTIVATE, 0, 0);
         }
         else
         {
            co_IntSetForegroundWindow(Window);
         }
      }
   }

   if ((WinPos.flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE)
      co_IntSendMessage(WinPos.hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM) &WinPos);

   IntReleaseWindowObject(Window);

   return TRUE;
}

LRESULT FASTCALL
co_WinPosGetNonClientSize(HWND Wnd, RECT* WindowRect, RECT* ClientRect)
{
  LRESULT Result;

  *ClientRect = *WindowRect;
  Result = co_IntSendMessage(Wnd, WM_NCCALCSIZE, FALSE, (LPARAM) ClientRect);

  FixClientRect(ClientRect, WindowRect);

  return Result;
}

BOOLEAN FASTCALL
co_WinPosShowWindow(HWND Wnd, INT Cmd)
{
  BOOLEAN WasVisible;
  PWINDOW_OBJECT Window;
  NTSTATUS Status;
  UINT Swp = 0;
  RECT NewPos;
  BOOLEAN ShowFlag;
//  HRGN VisibleRgn;

  Status =
    ObmReferenceObjectByHandle(gHandleTable,
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
	Swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE;
	if (Window->hSelf != UserGetActiveWindow())
	    Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
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
	Swp |= SWP_FRAMECHANGED | SWP_NOACTIVATE;
	if (!(Window->Style & WS_MINIMIZE))
	  {
	    Swp |= co_WinPosMinMaximize(Window, SW_MINIMIZE, &NewPos);
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
       Swp |= co_WinPosMinMaximize(Window, SW_MAXIMIZE, &NewPos);
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
     Swp |= co_WinPosMinMaximize(Window, SW_RESTORE, &NewPos);
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
      co_IntSendMessage(Wnd, WM_SHOWWINDOW, ShowFlag, 0);
      /*
       * FIXME: Need to check the window wasn't destroyed during the
       * window procedure.
       */
      if (!(Window->Parent))
        {
          co_IntShellHookNotify(HSHELL_WINDOWCREATED, (LPARAM)Wnd);
        }
    }

  /* We can't activate a child window */
  if ((Window->Style & WS_CHILD) &&
      !(Window->ExStyle & WS_EX_MDICHILD))
    {
      Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
    }

  co_WinPosSetWindowPos(Window->hSelf, 0 != (Window->ExStyle & WS_EX_TOPMOST)
                                   ? HWND_TOPMOST : HWND_TOP,
                     NewPos.left, NewPos.top, NewPos.right, NewPos.bottom, LOWORD(Swp));

  if (Cmd == SW_HIDE)
    {
      /* FIXME: This will cause the window to be activated irrespective
       * of whether it is owned by the same thread. Has to be done
       * asynchronously.
       */

      if (Window->hSelf == UserGetActiveWindow())
        {
          co_WinPosActivateOtherWindow(Window);
        }

      /* Revert focus to parent */
      if (Wnd == IntGetThreadFocusWindow() ||
          IntIsChildWindow(Wnd, IntGetThreadFocusWindow()))
        {
          UserSetFocus(Window->Parent->hSelf);
        }

      if (!(Window->Parent))
        {
          co_IntShellHookNotify(HSHELL_WINDOWDESTROYED, (LPARAM)Wnd);
        }
    }

  /* FIXME: Check for window destruction. */

  if ((Window->Flags & WINDOWOBJECT_NEED_SIZE) &&
      !(Window->Status & WINDOWSTATUS_DESTROYING))
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

      co_IntSendMessage(Wnd, WM_SIZE, wParam,
                     MAKELONG(Window->ClientRect.right -
                              Window->ClientRect.left,
                              Window->ClientRect.bottom -
                              Window->ClientRect.top));
      co_IntSendMessage(Wnd, WM_MOVE, 0,
                     MAKELONG(Window->ClientRect.left,
                              Window->ClientRect.top));
      IntEngWindowChanged(Window, WOC_RGN_CLIENT);
    }

  /* Activate the window if activation is not requested and the window is not minimized */
/*
  if (!(Swp & (SWP_NOACTIVATE | SWP_HIDEWINDOW)) && !(Window->Style & WS_MINIMIZE))
    {
      WinPosChangeActiveWindow(Wnd, FALSE);
    }
*/

  ObmDereferenceObject(Window);
  return(WasVisible);
}

STATIC VOID FASTCALL
co_WinPosSearchChildren(
   PWINDOW_OBJECT ScopeWin, PUSER_MESSAGE_QUEUE OnlyHitTests, POINT *Point,
   PWINDOW_OBJECT* Window, USHORT *HitTest)
{
   PWINDOW_OBJECT Current;
   HWND *List, *phWnd;

   if ((List = IntWinListChildren(ScopeWin)))
   {
      for (phWnd = List; *phWnd; ++phWnd)
      {
         if (!(Current = IntGetWindowObject(*phWnd)))
            continue;

         if (!(Current->Style & WS_VISIBLE))
         {
            IntReleaseWindowObject(Current);
	    continue;
         }

         if ((Current->Style & (WS_POPUP | WS_CHILD | WS_DISABLED)) ==
             (WS_CHILD | WS_DISABLED))
         {
            IntReleaseWindowObject(Current);
            continue;
         }

         if (!IntPtInWindow(Current, Point->x, Point->y))
         {
            IntReleaseWindowObject(Current);
            continue;
         }

         if (*Window)
	    IntReleaseWindowObject(*Window);
         *Window = Current;

         if (Current->Style & WS_MINIMIZE)
         {
            *HitTest = HTCAPTION;
            break;
         }

         if (Current->Style & WS_DISABLED)
         {
            *HitTest = HTERROR;
            break;
         }

         if (OnlyHitTests && (Current->MessageQueue == OnlyHitTests))
         {
            *HitTest = co_IntSendMessage(Current->hSelf, WM_NCHITTEST, 0,
                                      MAKELONG(Point->x, Point->y));
            if ((*HitTest) == (USHORT)HTTRANSPARENT)
               continue;
         }
         else
            *HitTest = HTCLIENT;

         if (Point->x >= Current->ClientRect.left &&
             Point->x < Current->ClientRect.right &&
             Point->y >= Current->ClientRect.top &&
             Point->y < Current->ClientRect.bottom)
         {
            co_WinPosSearchChildren(Current, OnlyHitTests, Point, Window, HitTest);
         }

         break;
      }
      ExFreePool(List);
   }
}

USHORT FASTCALL
co_WinPosWindowFromPoint(PWINDOW_OBJECT ScopeWin, PUSER_MESSAGE_QUEUE OnlyHitTests, POINT *WinPoint,
		      PWINDOW_OBJECT* Window)
{
  HWND DesktopWindowHandle;
  PWINDOW_OBJECT DesktopWindow;
  POINT Point = *WinPoint;
  USHORT HitTest;

  *Window = NULL;

  if(!ScopeWin)
  {
    DPRINT1("WinPosWindowFromPoint(): ScopeWin == NULL!\n");
    return(HTERROR);
  }

  if (ScopeWin->Style & WS_DISABLED)
    {
      return(HTERROR);
    }

  /* Translate the point to the space of the scope window. */
  DesktopWindowHandle = IntGetDesktopWindow();
  if((DesktopWindowHandle != ScopeWin->hSelf) &&
     (DesktopWindow = IntGetWindowObject(DesktopWindowHandle)))
  {
    Point.x += ScopeWin->ClientRect.left - DesktopWindow->ClientRect.left;
    Point.y += ScopeWin->ClientRect.top - DesktopWindow->ClientRect.top;
    IntReleaseWindowObject(DesktopWindow);
  }

  HitTest = HTNOWHERE;

  co_WinPosSearchChildren(ScopeWin, OnlyHitTests, &Point, Window, &HitTest);

  return ((*Window) ? HitTest : HTNOWHERE);
}

BOOL
STDCALL
NtUserGetMinMaxInfo(
  HWND hwnd,
  MINMAXINFO *MinMaxInfo,
  BOOL SendMessage)
{
  POINT Size;
  PINTERNALPOS InternalPos;
  PWINDOW_OBJECT Window;
  MINMAXINFO SafeMinMax;
  NTSTATUS Status;
  DECLARE_RETURN(BOOL);

  DPRINT("Enter NtUserGetMinMaxInfo\n");
  UserEnterExclusive();

  Window = IntGetWindowObject(hwnd);
  if(!Window)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    RETURN( FALSE);
  }

  Size.x = Window->WindowRect.left;
  Size.y = Window->WindowRect.top;
  InternalPos = WinPosInitInternalPos(Window, &Size,
				      &Window->WindowRect);
  if(InternalPos)
  {
    if(SendMessage)
    {
      co_WinPosGetMinMaxInfo(Window, &SafeMinMax.ptMaxSize, &SafeMinMax.ptMaxPosition,
                          &SafeMinMax.ptMinTrackSize, &SafeMinMax.ptMaxTrackSize);
    }
    else
    {
      WinPosFillMinMaxInfoStruct(Window, &SafeMinMax);
    }
    Status = MmCopyToCaller(MinMaxInfo, &SafeMinMax, sizeof(MINMAXINFO));
    if(!NT_SUCCESS(Status))
    {
      IntReleaseWindowObject(Window);
      SetLastNtError(Status);
      RETURN( FALSE);
    }
    IntReleaseWindowObject(Window);
    RETURN( TRUE);
  }

  IntReleaseWindowObject(Window);
  RETURN( FALSE);
  
CLEANUP:
  DPRINT("Leave NtUserGetMinMaxInfo, ret=%i\n",_ret_);
  UserLeave();
  END_CLEANUP;
}

/* EOF */
