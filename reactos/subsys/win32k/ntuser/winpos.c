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
/* $Id: winpos.c,v 1.120.2.4 2004/09/14 01:00:44 weiden Exp $
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

BOOL INTERNAL_CALL
IntGetClientOrigin(PWINDOW_OBJECT Window, LPPOINT Point)
{
  if(Window == NULL)
  {
    Window = IntGetDesktopWindow();
  }
  
  if(Window == NULL)
  {
     Point->x = Point->y = 0;
     return FALSE;
  }
  
  Point->x = Window->ClientRect.left;
  Point->y = Window->ClientRect.top;
  
  return TRUE;
}

/*******************************************************************
 *         WinPosActivateOtherWindow
 *
 *  Activates window other than pWnd.
 */
VOID INTERNAL_CALL
WinPosActivateOtherWindow(PWINDOW_OBJECT Window)
{
  PWINDOW_OBJECT Wnd;
  int TryTopmost;
  
  if (!Window || IntIsDesktopWindow(Window))
  {
    IntSetActiveMessageQueue(NULL);
    return;
  }
  
  Wnd = Window;
  for(;;)
  {
    PWINDOW_OBJECT Child;
    
    if(!(Wnd = IntGetParent(Wnd)))
    {
      IntSetActiveMessageQueue(NULL);
      return;
    }
    
    for(TryTopmost = 0; TryTopmost <= 1; TryTopmost++)
    {
      for(Child = Wnd->FirstChild; Child != NULL; Child = Child->NextSibling)
      {
        if(Child == Window)
        {
          /* skip own window */
	  continue;
        }
        
	if(((! TryTopmost && (0 == (Child->ExStyle & WS_EX_TOPMOST)))
            || (TryTopmost && (0 != (Child->ExStyle & WS_EX_TOPMOST))))
           && IntSetForegroundWindow(Child))
            {
              return;
            }
      }
    }
  }
}

VOID STATIC INTERNAL_CALL
WinPosFindIconPos(PWINDOW_OBJECT Window, POINT *Pos)
{
  /* FIXME */
}

PINTERNALPOS INTERNAL_CALL
WinPosInitInternalPos(PWINDOW_OBJECT WindowObject, POINT *pt, PRECT RestoreRect)
{
  PWINDOW_OBJECT Parent;
  INT XInc, YInc;
  
  if (WindowObject->InternalPos == NULL)
    {
      RECT WorkArea;
      PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop; /* Or rather get it from the window? */
      
      Parent = IntGetParent(WindowObject);
      if(Parent)
      {
        if(IntIsDesktopWindow(Parent))
          IntGetDesktopWorkArea(Desktop, &WorkArea);
        else
          WorkArea = Parent->ClientRect;
      }
      else
        IntGetDesktopWorkArea(Desktop, &WorkArea);
      
      WindowObject->InternalPos = ExAllocatePoolWithTag(PagedPool, sizeof(INTERNALPOS), TAG_WININTLIST);
      if(!WindowObject->InternalPos)
      {
        DPRINT1("Failed to allocate INTERNALPOS structure for window 0x%x\n", WindowObject->Handle);
        return NULL;
      }
      WindowObject->InternalPos->NormalRect = WindowObject->WindowRect;
      IntGetWindowBorderMeasures(WindowObject, &XInc, &YInc);
      WindowObject->InternalPos->MaxPos.x = WorkArea.left - XInc;
      WindowObject->InternalPos->MaxPos.y = WorkArea.top - YInc;
      WindowObject->InternalPos->IconPos.x = WorkArea.left;
      WindowObject->InternalPos->IconPos.y = WorkArea.bottom - NtUserGetSystemMetrics(SM_CYMINIMIZED);
    }
  if (WindowObject->Style & WS_MINIMIZE)
    {
      WindowObject->InternalPos->IconPos = *pt;
    }
  else if (WindowObject->Style & WS_MAXIMIZE)
    {
      WindowObject->InternalPos->MaxPos = *pt;
    }
  else if (RestoreRect != NULL)
    {
      WindowObject->InternalPos->NormalRect = *RestoreRect;
    }
  return WindowObject->InternalPos;
}

UINT INTERNAL_CALL
WinPosMinMaximize(PWINDOW_OBJECT WindowObject, UINT ShowFlag, RECT* NewPos)
{
  POINT Size;
  PINTERNALPOS InternalPos;
  UINT SwpFlags = 0;

  Size.x = WindowObject->WindowRect.left;
  Size.y = WindowObject->WindowRect.top;
  InternalPos = WinPosInitInternalPos(WindowObject, &Size, 
				      &WindowObject->WindowRect); 

  if (InternalPos)
    {
      if (WindowObject->Style & WS_MINIMIZE)
	{
	  if (!IntSendMessage(WindowObject, WM_QUERYOPEN, 0, 0))
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
		WindowObject->Flags &= ~WINDOWOBJECT_RESTOREMAX;
	      }
	    IntRedrawWindow(WindowObject, NULL, 0, RDW_VALIDATE | RDW_NOERASE |
	       RDW_NOINTERNALPAINT);
	    WindowObject->Style |= WS_MINIMIZE;
	    WinPosFindIconPos(WindowObject, &InternalPos->IconPos);
	    NtGdiSetRect(NewPos, InternalPos->IconPos.x, InternalPos->IconPos.y,
			NtUserGetSystemMetrics(SM_CXMINIMIZED),
			NtUserGetSystemMetrics(SM_CYMINIMIZED));
	    SwpFlags |= SWP_NOCOPYBITS;
	    break;
	  }

	case SW_MAXIMIZE:
	  {
	    WinPosGetMinMaxInfo(WindowObject, &Size, &InternalPos->MaxPos, 
				NULL, NULL);
	    DPRINT("Maximize: %d,%d %dx%d\n",
	       InternalPos->MaxPos.x, InternalPos->MaxPos.y, Size.x, Size.y);
	    if (WindowObject->Style & WS_MINIMIZE)
	      {
		WindowObject->Style &= ~WS_MINIMIZE;
	      }
	    WindowObject->Style |= WS_MAXIMIZE;
	    NtGdiSetRect(NewPos, InternalPos->MaxPos.x, InternalPos->MaxPos.y,
			Size.x, Size.y);
	    break;
	  }

	case SW_RESTORE:
	  {
	    if (WindowObject->Style & WS_MINIMIZE)
	      {
		WindowObject->Style &= ~WS_MINIMIZE;
		if (WindowObject->Flags & WINDOWOBJECT_RESTOREMAX)
		  {
		    WinPosGetMinMaxInfo(WindowObject, &Size,
					&InternalPos->MaxPos, NULL, NULL);
		    WindowObject->Style |= WS_MAXIMIZE;
		    NtGdiSetRect(NewPos, InternalPos->MaxPos.x,
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
		if (!(WindowObject->Style & WS_MAXIMIZE))
		  {
		    return 0;
		  }
		WindowObject->Style &= ~WS_MAXIMIZE;
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

VOID INTERNAL_CALL
WinPosFillMinMaxInfoStruct(PWINDOW_OBJECT Window, MINMAXINFO *Info)
{
  INT XInc, YInc;
  RECT WorkArea;
  PDESKTOP_OBJECT Desktop = PsGetWin32Thread()->Desktop; /* Or rather get it from the window? */
  
  IntGetDesktopWorkArea(Desktop, &WorkArea);
  
  /* Get default values. */
  Info->ptMaxSize.x = WorkArea.right - WorkArea.left;
  Info->ptMaxSize.y = WorkArea.bottom - WorkArea.top;
  Info->ptMinTrackSize.x = NtUserGetSystemMetrics(SM_CXMINTRACK);
  Info->ptMinTrackSize.y = NtUserGetSystemMetrics(SM_CYMINTRACK);
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

UINT INTERNAL_CALL
WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
		    POINT* MinTrack, POINT* MaxTrack)
{
  MINMAXINFO MinMax;
  
  WinPosFillMinMaxInfoStruct(Window, &MinMax);
  
  IntSendMessage(Window, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax);

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

STATIC VOID INTERNAL_CALL
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

VOID STATIC INTERNAL_CALL
InternalWindowPosToWinPosStructure(PWINDOWPOS WinPos, PINTERNAL_WINDOWPOS WindowPos)
{
  /* convert the PWINDOW_OBJECTs to HWNDs or HWND_* constants */
  WinPos->hwnd = (HWNDValidateWindowObject(WindowPos->Window) ? WindowPos->Window->Handle : WindowPos->Window);
  WinPos->hwndInsertAfter = (HWNDValidateWindowObject(WindowPos->InsertAfter) ? WindowPos->InsertAfter->Handle : WindowPos->InsertAfter);
  WinPos->x = WindowPos->x;
  WinPos->y = WindowPos->y;
  WinPos->cx = WindowPos->cx;
  WinPos->cy = WindowPos->cy;
  WinPos->flags = WindowPos->flags;
}

VOID STATIC INTERNAL_CALL
WinPosToInternalWindowPosStructure(PINTERNAL_WINDOWPOS WindowPos, PWINDOWPOS WinPos)
{
  /* convert the HWNDs to PWINDOW_OBJECTs or WINDOW_* constants */
  if(HWNDValidateWindowObject(WinPos->hwnd))
  {
    WindowPos->Window = IntGetUserObject(WINDOW, WinPos->hwnd);
  }
  else
  {
    WindowPos->Window = (PWINDOW_OBJECT)WinPos->hwnd;
  }
  if(HWNDValidateWindowObject(WinPos->hwndInsertAfter))
  {
    WindowPos->InsertAfter = IntGetUserObject(WINDOW, WinPos->hwndInsertAfter);
  }
  else
  {
    WindowPos->InsertAfter = (PWINDOW_OBJECT)WinPos->hwndInsertAfter;
  }
  WindowPos->x = WinPos->x;
  WindowPos->y = WinPos->y;
  WindowPos->cx = WinPos->cx;
  WindowPos->cy = WinPos->cy;
  WindowPos->flags = WinPos->flags;
}

LONG STATIC INTERNAL_CALL
WinPosDoNCCALCSize(PWINDOW_OBJECT Window, PINTERNAL_WINDOWPOS WinPos,
		   RECT* WindowRect, RECT* ClientRect)
{
  PWINDOW_OBJECT Parent;
  UINT wvrFlags = 0;

  ASSERT(Window);

  /* Send WM_NCCALCSIZE message to get new client area */
  if ((WinPos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE)
    {
      NCCALCSIZE_PARAMS params;
      WINDOWPOS winposCopy;

      params.rgrc[0] = *WindowRect;
      params.rgrc[1] = Window->WindowRect;
      params.rgrc[2] = Window->ClientRect;
      Parent = IntGetParent(Window);
      if (0 != (Window->Style & WS_CHILD) && Parent != NULL)
	{
	  NtGdiOffsetRect(&(params.rgrc[0]), - Parent->ClientRect.left,
	                      - Parent->ClientRect.top);
	  NtGdiOffsetRect(&(params.rgrc[1]), - Parent->ClientRect.left,
	                      - Parent->ClientRect.top);
	  NtGdiOffsetRect(&(params.rgrc[2]), - Parent->ClientRect.left,
	                      - Parent->ClientRect.top);
	}
      params.lppos = &winposCopy;
      
      InternalWindowPosToWinPosStructure(&winposCopy, WinPos);

      wvrFlags = IntSendMessage(Window, WM_NCCALCSIZE, TRUE, (LPARAM) &params);

      /* If the application send back garbage, ignore it */
      if (params.rgrc[0].left <= params.rgrc[0].right &&
          params.rgrc[0].top <= params.rgrc[0].bottom)
	{
          *ClientRect = params.rgrc[0];
	  if ((Window->Style & WS_CHILD) && Parent)
	    {
	      NtGdiOffsetRect(ClientRect, Parent->ClientRect.left,
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

BOOL INTERNAL_CALL
WinPosDoWinPosChanging(PWINDOW_OBJECT WindowObject,
		       PINTERNAL_WINDOWPOS WinPos,
		       PRECT WindowRect,
		       PRECT ClientRect)
{
  INT X, Y;

  if (!(WinPos->flags & SWP_NOSENDCHANGING))
    {
      WINDOWPOS winposCopy;
      InternalWindowPosToWinPosStructure(&winposCopy, WinPos);
      IntSendMessage(WindowObject, WM_WINDOWPOSCHANGING, 0, (LPARAM) &winposCopy);
      WinPosToInternalWindowPosStructure(WinPos, &winposCopy);
    }

  *WindowRect = WindowObject->WindowRect;
  *ClientRect = WindowObject->ClientRect;

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
      
      if ((0 != (WindowObject->Style & WS_CHILD)) && (Parent = IntGetParent(WindowObject)))
	{
	  X += Parent->ClientRect.left;
	  Y += Parent->ClientRect.top;
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

/*
 * Fix Z order taking into account owned popups -
 * basically we need to maintain them above the window that owns them
 */
PWINDOW_OBJECT INTERNAL_CALL
WinPosDoOwnedPopups(PWINDOW_OBJECT Window, PWINDOW_OBJECT InsertAfter)
{
   LONG Style;
   PWINDOW_OBJECT DesktopWindow, Current;
   BOOL Searched = FALSE;

   ASSERT(Window);
   
   Style = Window->Style;

   if ((Style & WS_POPUP) && Window->Owner)
   {
      /* Make sure this popup stays above the owner */
      PWINDOW_OBJECT LocalPrev = WINDOW_TOPMOST;

      if (InsertAfter != WINDOW_TOPMOST)
      {
	 DesktopWindow = IntGetDesktopWindow();
         
         for(Current = DesktopWindow->FirstChild; Current != NULL;
             Current = Current->NextSibling)
         {
           Searched = TRUE;
           
	   if(Current == Window->Owner)
           {
             break;
           }
           if(WINDOW_TOP == InsertAfter)
           {
             if (0 == (Current->ExStyle & WS_EX_TOPMOST))
             {
               break;
             }
           }
           if(Current != Window)
           {
             LocalPrev = Current;
           }
           if(LocalPrev == InsertAfter)
           {
             break;
           }
         }
      }
   }
   else if (Style & WS_CHILD)
   {
      return InsertAfter;
   }

   if (!Searched)
   {
      for(Current = DesktopWindow->FirstChild; Current != NULL;
          Current = Current->NextSibling)
      {
        if(Current == Window)
        {
          break;
        }
        
        if(Current->Style & WS_POPUP && Current->Owner == Window)
        {
          WinPosSetWindowPos(Current, InsertAfter, 0, 0, 0, 0,
                             SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
	  InsertAfter = Current;
        }
      }
   }

   return InsertAfter;
}

/***********************************************************************
 *	     WinPosInternalMoveWindow
 *
 * Update WindowRect and ClientRect of Window and all of its children
 * We keep both WindowRect and ClientRect in screen coordinates internally
 */
VOID STATIC INTERNAL_CALL
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

BOOL INTERNAL_CALL
WinPosFixupFlags(PWINDOW_OBJECT Window, INTERNAL_WINDOWPOS *WinPos)
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

   if (WinPos->Window == IntGetForegroundWindow())
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
         WinPos->InsertAfter = (0 != (Window->ExStyle & WS_EX_TOPMOST) ?
                                WINDOW_TOPMOST : WINDOW_TOP);
         return TRUE;
      }
   }

   /* Check hwndInsertAfter */
   if (!(WinPos->flags & SWP_NOZORDER))
   {
      /* Fix sign extension */
      if (WinPos->InsertAfter == (PWINDOW_OBJECT)0xffff)
      {
         WinPos->InsertAfter = WINDOW_TOPMOST;
      }
      else if (WinPos->InsertAfter == (PWINDOW_OBJECT)0xfffe)
      {
         WinPos->InsertAfter = WINDOW_NOTOPMOST;
      }

      if (WinPos->InsertAfter == WINDOW_NOTOPMOST)
      {
         WinPos->InsertAfter = WINDOW_TOP;
      }
      else if (WINDOW_TOP == WinPos->InsertAfter
               && 0 != (Window->ExStyle & WS_EX_TOPMOST))
      {
         /* Keep it topmost when it's already topmost */
         WinPos->InsertAfter = WINDOW_TOPMOST;
      }

      /* hwndInsertAfter must be a sibling of the window */
      if (WINDOW_TOPMOST != WinPos->InsertAfter
          && WINDOW_TOP != WinPos->InsertAfter
          && WINDOW_NOTOPMOST != WinPos->InsertAfter
          && WINDOW_BOTTOM != WinPos->InsertAfter)
      {
         /* WARNING!!! The handle values in WinPos are NOT handles, they're PWINDOW_OBJECTs! */
         
	 if ((WinPos->InsertAfter ? IntGetParent(WinPos->InsertAfter) : NULL) != IntGetParent(Window->Parent))
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
            
            /* WARNING!!! The handle values in WinPos are NOT handles, they're PWINDOW_OBJECTs! */
            
            if ((WinPos->Window == WinPos->InsertAfter) ||
                (WinPos->Window == (WinPos->InsertAfter ? (WinPos->InsertAfter)->NextSibling : NULL)))
            {
               WinPos->flags |= SWP_NOZORDER;
            }
         }
      }
   }

   return TRUE;
}

/* x and y are always screen relative */
BOOLEAN INTERNAL_CALL
WinPosSetWindowPos(PWINDOW_OBJECT Window, PWINDOW_OBJECT InsertAfter, INT x, INT y, INT cx,
		   INT cy, UINT flags)
{
   INTERNAL_WINDOWPOS WinPos;
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

   ASSERT(Window);

   /*
    * Only allow CSRSS to mess with the desktop window
    */
   if (Window == IntGetDesktopWindow() &&
       Window->MessageQueue->Thread->ThreadsProcess != PsGetCurrentProcess())
   {
      return FALSE;
   }

   WinPos.Window = Window;
   WinPos.InsertAfter = InsertAfter;
   WinPos.x = x;
   WinPos.y = y;
   WinPos.cx = cx;
   WinPos.cy = cy;
   WinPos.flags = flags;

   WinPosDoWinPosChanging(Window, &WinPos, &NewWindowRect, &NewClientRect);

   /* Fix up the flags. */

   /* WARNING!!! The handle values in the WINDOWPOS are whether HWND_* constants
                 of PWINDOW_OBJECTs! They're not Handles! */
   
   if (!WinPosFixupFlags(Window, &WinPos))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   /* Does the window still exist? */
   if (!IntIsWindow(Window))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   if ((WinPos.flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) !=
       SWP_NOZORDER && (HWNDValidateWindowObject(WinPos.Window) ? WinPos.Window->Parent == IntGetDesktopWindow() : FALSE))
   {
      /* WARNING!!! The handle values in WinPos are NOT handles, they're PWINDOW_OBJECTs! */
      WinPos.InsertAfter = WinPosDoOwnedPopups(WinPos.Window, WinPos.InsertAfter);
   }

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

   WvrFlags = WinPosDoNCCALCSize(Window, &WinPos, &NewWindowRect, &NewClientRect);

   /* Relink windows. (also take into account shell window in hwndShellWindow) */
   if (!(WinPos.flags & SWP_NOZORDER) && WinPos.Window != IntGetShellWindow())
   {
      PWINDOW_OBJECT ParentWindow;
      PWINDOW_OBJECT Sibling;
      PWINDOW_OBJECT InsertAfterWindow;

      if ((ParentWindow = IntGetParent(Window)))
      {
         if (WINDOW_TOPMOST == WinPos.InsertAfter)
         {
            InsertAfterWindow = NULL;
         }
         else if (WINDOW_TOP == WinPos.InsertAfter
                  || WINDOW_NOTOPMOST == WinPos.InsertAfter)
         {
            InsertAfterWindow = NULL;
            
            Sibling = ParentWindow->FirstChild;
            while (NULL != Sibling && 0 != (Sibling->ExStyle & WS_EX_TOPMOST))
            {
               InsertAfterWindow = Sibling;
               Sibling = Sibling->NextSibling;
            }
         }
         else if (WinPos.InsertAfter == WINDOW_BOTTOM)
         {
            InsertAfterWindow = NULL;
            
	    if(ParentWindow->LastChild)
            {
               InsertAfterWindow = ParentWindow->LastChild;
            }
         }
         else
            InsertAfterWindow = (HWNDValidateWindowObject(WinPos.InsertAfter) ? WinPos.InsertAfter : NULL);
         /* Do nothing if hwndInsertAfter is HWND_BOTTOM and Window is already
            the last window */
         if (InsertAfterWindow != Window)
         {
             IntUnlinkWindow(Window);
             IntLinkWindow(Window, ParentWindow, InsertAfterWindow);
         }
         
         if ((WINDOW_TOPMOST == WinPos.InsertAfter)
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
      IntRedrawWindow(Window, NULL, 0, RDW_VALIDATE | RDW_NOFRAME |
                      RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ALLCHILDREN);
      Window->Style &= ~WS_VISIBLE;
   }
   else if (WinPos.flags & SWP_SHOWWINDOW)
   {
      Window->Style |= WS_VISIBLE;
   }

   DceResetActiveDCEs(Window,
                      NewWindowRect.left - OldWindowRect.left,
                      NewWindowRect.top - OldWindowRect.top);

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
         NtGdiOffsetRect(&ORect, - OldWindowRect.left, - OldWindowRect.top);
         NtGdiOffsetRect(&NRect, - NewWindowRect.left, - NewWindowRect.top);
         NtGdiIntersectRect(&CopyRect, &ORect, &NRect);
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
      VisRgn = NULL;
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
         Dc = IntGetDCEx(Window, ClipRgn, DCX_WINDOW | DCX_CACHE |
                         DCX_INTERSECTRGN | DCX_CLIPSIBLINGS);
         NtGdiBitBlt(Dc,
            CopyRect.left, CopyRect.top, CopyRect.right - CopyRect.left,
            CopyRect.bottom - CopyRect.top, Dc,
            CopyRect.left + (OldWindowRect.left - NewWindowRect.left),
            CopyRect.top + (OldWindowRect.top - NewWindowRect.top), SRCCOPY);
         IntReleaseDC(Window, Dc);
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
            Window->WindowRect.top - Window->ClientRect.top);DbgPrint("WS10a\n");
         IntRedrawWindow(Window, NULL, DirtyRgn,
            RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);DbgPrint("WS10b\n");
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
         VIS_WindowLayoutChanged(Window, ExposedRgn);
      }
      NtGdiDeleteObject(ExposedRgn);
      NtGdiDeleteObject(VisBefore);
   }

   if (VisAfter != NULL)
   {
      NtGdiDeleteObject(VisAfter);
   }

   if (!(WinPos.flags & SWP_NOREDRAW))
   {
      IntRedrawWindow(Window, NULL, 0, RDW_ALLCHILDREN | RDW_ERASENOW);
   }

   if (!(WinPos.flags & SWP_NOACTIVATE))
   {
      if ((Window->Style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
      {
         IntSendMessage(Window, WM_CHILDACTIVATE, 0, 0);
      }
      else
      {
         IntSetForegroundWindow(Window);
      }
   }

   if ((WinPos.flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE &&
        HWNDValidateWindowObject(WinPos.Window))
   {
      WINDOWPOS wp;
      InternalWindowPosToWinPosStructure(&wp, &WinPos);
      IntSendMessage(WinPos.Window, WM_WINDOWPOSCHANGED, 0, (LPARAM) &wp);
      WinPosToInternalWindowPosStructure(&WinPos, &wp);
   }

   return TRUE;
}

LRESULT INTERNAL_CALL
WinPosGetNonClientSize(PWINDOW_OBJECT Window, RECT* WindowRect, RECT* ClientRect)
{
  LRESULT Result;

  *ClientRect = *WindowRect;
  Result = IntSendMessage(Window, WM_NCCALCSIZE, FALSE, (LPARAM) ClientRect);

  /* FIXME - return if the window doesn't exist anymore */

  FixClientRect(ClientRect, WindowRect);

  return Result;
}

BOOLEAN INTERNAL_CALL
WinPosShowWindow(PWINDOW_OBJECT Window, INT Cmd)
{
  BOOLEAN WasVisible;
  UINT Swp = 0;
  RECT NewPos;
  BOOLEAN ShowFlag;
//  HRGN VisibleRgn;

  ASSERT(Window);
  
  WasVisible = (Window->Style & WS_VISIBLE) != 0;

  switch (Cmd)
    {
    case SW_HIDE:
      {
	if (!WasVisible)
	  {
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
	Swp |= SWP_FRAMECHANGED | SWP_NOACTIVATE;
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
      IntSendMessage(Window, WM_SHOWWINDOW, ShowFlag, 0);
      /* 
       * FIXME: Need to check the window wasn't destroyed during the 
       * window procedure. 
       */
    }

  /* We can't activate a child window */
  if ((Window->Style & WS_CHILD) &&
      !(Window->ExStyle & WS_EX_MDICHILD))
    {
      Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
    }

  WinPosSetWindowPos(Window, (0 != (Window->ExStyle & WS_EX_TOPMOST))
                                   ? WINDOW_TOPMOST : WINDOW_TOP,
                     NewPos.left, NewPos.top, NewPos.right, NewPos.bottom, LOWORD(Swp));

  if (Cmd == SW_HIDE)
    {
      /* FIXME: This will cause the window to be activated irrespective
       * of whether it is owned by the same thread. Has to be done
       * asynchronously.
       */

      if (Window == IntGetActiveWindow())
        {
          WinPosActivateOtherWindow(Window);
        }

      /* Revert focus to parent */
      if (Window == IntGetThreadFocusWindow() ||
          IntIsChildWindow(Window, IntGetThreadFocusWindow()))
        {
          IntSetFocus(Window->Parent);
        }
    }

  /* FIXME: Check for window destruction. */

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

      IntSendMessage(Window, WM_SIZE, wParam,
                     MAKELONG(Window->ClientRect.right - 
                              Window->ClientRect.left,
                              Window->ClientRect.bottom -
                              Window->ClientRect.top));
      IntSendMessage(Window, WM_MOVE, 0,
                     MAKELONG(Window->ClientRect.left,
                              Window->ClientRect.top));
    }

  /* Activate the window if activation is not requested and the window is not minimized */
/*
  if (!(Swp & (SWP_NOACTIVATE | SWP_HIDEWINDOW)) && !(Window->Style & WS_MINIMIZE))
    {
      WinPosChangeActiveWindow(Wnd, FALSE);
    }
*/

  return(WasVisible);
}

STATIC VOID INTERNAL_CALL
WinPosSearchChildren(
   PWINDOW_OBJECT ScopeWin, PUSER_MESSAGE_QUEUE OnlyHitTests, POINT *Point,
   PWINDOW_OBJECT* Window, USHORT *HitTest)
{
   PWINDOW_OBJECT Current;

   for(Current = ScopeWin->FirstChild; Current != NULL; Current = Current->NextSibling)
   {
     if (!(Current->Style & WS_VISIBLE) ||
         (Current->Style & (WS_POPUP | WS_CHILD | WS_DISABLED)) == (WS_CHILD | WS_DISABLED) ||
         !IntPtInWindow(Current, Point->x, Point->y))
     {
        continue;
     }

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
        *HitTest = IntSendMessage(Current, WM_NCHITTEST, 0,
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
        WinPosSearchChildren(Current, OnlyHitTests, Point, Window, HitTest);
     }        

     break;
   }
}

USHORT INTERNAL_CALL
WinPosWindowFromPoint(PWINDOW_OBJECT ScopeWin, PUSER_MESSAGE_QUEUE OnlyHitTests, POINT *WinPoint, 
		      PWINDOW_OBJECT* Window)
{
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
  DesktopWindow = IntGetDesktopWindow();
  if(DesktopWindow != ScopeWin)
  {
    Point.x += ScopeWin->ClientRect.left - DesktopWindow->ClientRect.left;
    Point.y += ScopeWin->ClientRect.top - DesktopWindow->ClientRect.top;
  }
  
  HitTest = HTNOWHERE;
  
  WinPosSearchChildren(ScopeWin, OnlyHitTests, &Point, Window, &HitTest);

  return ((*Window) ? HitTest : HTNOWHERE);
}


