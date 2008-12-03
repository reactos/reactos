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

VOID FASTCALL
co_IntPaintWindows(PWINDOW_OBJECT Window, ULONG Flags, BOOL Recurse);

BOOL FASTCALL
IntValidateParent(PWINDOW_OBJECT Child, HRGN hValidateRgn, BOOL Recurse);

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
IntGetClientOrigin(PWINDOW_OBJECT Window OPTIONAL, LPPOINT Point)
{
   Window = Window ? Window : UserGetWindowObject(IntGetDesktopWindow());
   if (Window == NULL)
   {
      Point->x = Point->y = 0;
      return FALSE;
   }
   Point->x = Window->Wnd->ClientRect.left;
   Point->y = Window->Wnd->ClientRect.top;

   return TRUE;
}




BOOL FASTCALL
UserGetClientOrigin(PWINDOW_OBJECT Window, LPPOINT Point)
{
   BOOL Ret;
   POINT pt;
   NTSTATUS Status;

   if(!Point)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   Ret = IntGetClientOrigin(Window, &pt);

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



/*******************************************************************
 *         can_activate_window
 *
 * Check if we can activate the specified window.
 */
static
BOOL FASTCALL can_activate_window( PWINDOW_OBJECT Wnd OPTIONAL)
{
    LONG style;

    if (!Wnd) return FALSE;
	if (!Wnd->Wnd) return FALSE;
    style = Wnd->Wnd->Style;
    if (!(style & WS_VISIBLE) &&
        Wnd->OwnerThread->ThreadsProcess != CsrProcess) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    return !(style & WS_DISABLED);
}


/*******************************************************************
 *         WinPosActivateOtherWindow
 *
 *  Activates window other than pWnd.
 */
VOID FASTCALL
co_WinPosActivateOtherWindow(PWINDOW_OBJECT Window)
{
   PWINDOW_OBJECT WndTo = NULL;
   HWND Fg;
   USER_REFERENCE_ENTRY Ref;
   PWINDOW Wnd;

   ASSERT_REFS_CO(Window);

   Wnd = Window->Wnd;

   if (IntIsDesktopWindow(Window))
   {
      IntSetFocusMessageQueue(NULL);
      return;
   }

   /* If this is popup window, try to activate the owner first. */
   if ((Wnd->Style & WS_POPUP) && (WndTo = IntGetOwner(Window)))
   {
      WndTo = UserGetAncestor( WndTo, GA_ROOT );
      if (can_activate_window(WndTo)) goto done;
   }

   /* Pick a next top-level window. */
   /* FIXME: Search for non-tooltip windows first. */
   WndTo = Window;
   for (;;)
   {
      if (!(WndTo = WndTo->NextSibling)) break;
      if (can_activate_window( WndTo )) break;
   }

done:

   if (WndTo) UserRefObjectCo(WndTo, &Ref);

   Fg = UserGetForegroundWindow();
   if ((!Fg || Window->hSelf == Fg) && WndTo)//fixme: ok if WndTo is NULL??
   {
      /* fixme: wine can pass WndTo=NULL to co_IntSetForegroundWindow. hmm */
      if (co_IntSetForegroundWindow(WndTo))
      {
         UserDerefObjectCo(WndTo);
         return;
      }
   }

   if (!co_IntSetActiveWindow(WndTo))  /* ok for WndTo to be NULL here */
      co_IntSetActiveWindow(0);

   if (WndTo) UserDerefObjectCo(WndTo);
}


UINT
FASTCALL
co_WinPosArrangeIconicWindows(PWINDOW_OBJECT parent)
{
   RECT rectParent;
   INT i, x, y, xspacing, yspacing;
   HWND *List = IntWinListChildren(parent);

   ASSERT_REFS_CO(parent);

   IntGetClientRect( parent, &rectParent );
   x = rectParent.left;
   y = rectParent.bottom;

   xspacing = UserGetSystemMetrics(SM_CXMINSPACING);
   yspacing = UserGetSystemMetrics(SM_CYMINSPACING);

   DPRINT("X:%d Y:%d XS:%d YS:%d\n",x,y,xspacing,yspacing);

   for( i = 0; List[i]; i++)
   {
      PWINDOW_OBJECT WndChild;
      PWINDOW ChildWnd;

      if (!(WndChild = UserGetWindowObject(List[i])))
         continue;

      ChildWnd = WndChild->Wnd;

      if((ChildWnd->Style & WS_MINIMIZE) != 0 )
      {
         USER_REFERENCE_ENTRY Ref;
         UserRefObjectCo(WndChild, &Ref);

         co_WinPosSetWindowPos(WndChild, 0, x + UserGetSystemMetrics(SM_CXBORDER),
                               y - yspacing - UserGetSystemMetrics(SM_CYBORDER)
                               , 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );

         UserDerefObjectCo(WndChild);

         if (x <= rectParent.right - xspacing)
            x += xspacing;
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


static VOID FASTCALL
WinPosFindIconPos(PWINDOW_OBJECT Window, POINT *Pos)
{
   /* FIXME */
}

VOID FASTCALL
WinPosInitInternalPos(PWINDOW_OBJECT Window, POINT *pt, PRECT RestoreRect)
{
    PWINDOW_OBJECT Parent;
    UINT XInc, YInc;
    PWINDOW Wnd = Window->Wnd;

   if (!Wnd->InternalPosInitialized)
   {
      RECT WorkArea;
      PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
      PDESKTOP Desktop = pti->Desktop; /* Or rather get it from the window? */

      Parent = Window->Parent;
      if(Parent)
      {
         if(IntIsDesktopWindow(Parent))
            IntGetDesktopWorkArea(Desktop, &WorkArea);
         else
            WorkArea = Parent->Wnd->ClientRect;
      }
      else
         IntGetDesktopWorkArea(Desktop, &WorkArea);

      Wnd->InternalPos.NormalRect = Window->Wnd->WindowRect;
      IntGetWindowBorderMeasures(Window, &XInc, &YInc);
      Wnd->InternalPos.MaxPos.x = WorkArea.left - XInc;
      Wnd->InternalPos.MaxPos.y = WorkArea.top - YInc;
      Wnd->InternalPos.IconPos.x = WorkArea.left;
      Wnd->InternalPos.IconPos.y = WorkArea.bottom - UserGetSystemMetrics(SM_CYMINIMIZED);

      Wnd->InternalPosInitialized = TRUE;
   }
   if (Wnd->Style & WS_MINIMIZE)
   {
      Wnd->InternalPos.IconPos = *pt;
   }
   else if (Wnd->Style & WS_MAXIMIZE)
   {
      Wnd->InternalPos.MaxPos = *pt;
   }
   else if (RestoreRect != NULL)
   {
      Wnd->InternalPos.NormalRect = *RestoreRect;
   }
}

UINT FASTCALL
co_WinPosMinMaximize(PWINDOW_OBJECT Window, UINT ShowFlag, RECT* NewPos)
{
   POINT Size;
   UINT SwpFlags = 0;
   PWINDOW Wnd;

   ASSERT_REFS_CO(Window);
   Wnd = Window->Wnd;

   Size.x = Wnd->WindowRect.left;
   Size.y = Wnd->WindowRect.top;
   WinPosInitInternalPos(Window, &Size, &Wnd->WindowRect);

      if (Wnd->Style & WS_MINIMIZE)
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
               if (Wnd->Style & WS_MAXIMIZE)
               {
                  Window->Flags |= WINDOWOBJECT_RESTOREMAX;
                  Wnd->Style &= ~WS_MAXIMIZE;
               }
               else
               {
                  Window->Flags &= ~WINDOWOBJECT_RESTOREMAX;
               }
               co_UserRedrawWindow(Window, NULL, 0, RDW_VALIDATE | RDW_NOERASE |
                                   RDW_NOINTERNALPAINT);
               Wnd->Style |= WS_MINIMIZE;
               WinPosFindIconPos(Window, &Wnd->InternalPos.IconPos);
               IntGdiSetRect(NewPos, Wnd->InternalPos.IconPos.x, Wnd->InternalPos.IconPos.y,
                             UserGetSystemMetrics(SM_CXMINIMIZED),
                             UserGetSystemMetrics(SM_CYMINIMIZED));
               SwpFlags |= SWP_NOCOPYBITS;
               break;
            }

         case SW_MAXIMIZE:
            {
               co_WinPosGetMinMaxInfo(Window, &Size, &Wnd->InternalPos.MaxPos,
                                      NULL, NULL);
               DPRINT("Maximize: %d,%d %dx%d\n",
                      Wnd->InternalPos.MaxPos.x, Wnd->InternalPos.MaxPos.y, Size.x, Size.y);
               if (Wnd->Style & WS_MINIMIZE)
               {
                  Wnd->Style &= ~WS_MINIMIZE;
               }
               Wnd->Style |= WS_MAXIMIZE;
               IntGdiSetRect(NewPos, Wnd->InternalPos.MaxPos.x, Wnd->InternalPos.MaxPos.y,
                             Size.x, Size.y);
               break;
            }

         case SW_RESTORE:
            {
               if (Wnd->Style & WS_MINIMIZE)
               {
                  Wnd->Style &= ~WS_MINIMIZE;
                  if (Window->Flags & WINDOWOBJECT_RESTOREMAX)
                  {
                     co_WinPosGetMinMaxInfo(Window, &Size,
                                            &Wnd->InternalPos.MaxPos, NULL, NULL);
                     Wnd->Style |= WS_MAXIMIZE;
                     IntGdiSetRect(NewPos, Wnd->InternalPos.MaxPos.x,
                                   Wnd->InternalPos.MaxPos.y, Size.x, Size.y);
                     break;
                  }
                  else
                  {
                     *NewPos = Wnd->InternalPos.NormalRect;
                     NewPos->right -= NewPos->left;
                     NewPos->bottom -= NewPos->top;
                     break;
                  }
               }
               else
               {
                  if (!(Wnd->Style & WS_MAXIMIZE))
                  {
                     return 0;
                  }
                  Wnd->Style &= ~WS_MAXIMIZE;
                  *NewPos = Wnd->InternalPos.NormalRect;
                  NewPos->right -= NewPos->left;
                  NewPos->bottom -= NewPos->top;
                  break;
               }
            }
      }

   return(SwpFlags);
}

static
VOID FASTCALL
WinPosFillMinMaxInfoStruct(PWINDOW_OBJECT Window, MINMAXINFO *Info)
{
   UINT XInc, YInc;
   RECT WorkArea;
   PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
   PDESKTOP Desktop = pti->Desktop; /* Or rather get it from the window? */

   IntGetDesktopWorkArea(Desktop, &WorkArea);

   /* Get default values. */
   Info->ptMinTrackSize.x = UserGetSystemMetrics(SM_CXMINTRACK);
   Info->ptMinTrackSize.y = UserGetSystemMetrics(SM_CYMINTRACK);

   IntGetWindowBorderMeasures(Window, &XInc, &YInc);
   Info->ptMaxSize.x = WorkArea.right - WorkArea.left + 2 * XInc;
   Info->ptMaxSize.y = WorkArea.bottom - WorkArea.top + 2 * YInc;
   Info->ptMaxTrackSize.x = Info->ptMaxSize.x;
   Info->ptMaxTrackSize.y = Info->ptMaxSize.y;

   if (Window->Wnd->InternalPosInitialized)
   {
      Info->ptMaxPosition = Window->Wnd->InternalPos.MaxPos;
   }
   else
   {
      Info->ptMaxPosition.x = WorkArea.left - XInc;
      Info->ptMaxPosition.y = WorkArea.top - YInc;
   }
}

UINT FASTCALL
co_WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
                       POINT* MinTrack, POINT* MaxTrack)
{
   MINMAXINFO MinMax;

   ASSERT_REFS_CO(Window);

   WinPosFillMinMaxInfoStruct(Window, &MinMax);

   co_IntSendMessage(Window->hSelf, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax);

   MinMax.ptMaxTrackSize.x = max(MinMax.ptMaxTrackSize.x,
                                 MinMax.ptMinTrackSize.x);
   MinMax.ptMaxTrackSize.y = max(MinMax.ptMaxTrackSize.y,
                                 MinMax.ptMinTrackSize.y);

   if (MaxSize)
      *MaxSize = MinMax.ptMaxSize;
   if (MaxPos)
      *MaxPos = MinMax.ptMaxPosition;
   if (MinTrack)
      *MinTrack = MinMax.ptMinTrackSize;
   if (MaxTrack)
      *MaxTrack = MinMax.ptMaxTrackSize;

   return 0; //FIXME: what does it return?
}

static
VOID FASTCALL
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

static
LONG FASTCALL
co_WinPosDoNCCALCSize(PWINDOW_OBJECT Window, PWINDOWPOS WinPos,
                      RECT* WindowRect, RECT* ClientRect)
{
   PWINDOW_OBJECT Parent;
   UINT wvrFlags = 0;
   PWINDOW Wnd;

   ASSERT_REFS_CO(Window);
   Wnd = Window->Wnd;

   /* Send WM_NCCALCSIZE message to get new client area */
   if ((WinPos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE)
   {
      NCCALCSIZE_PARAMS params;
      WINDOWPOS winposCopy;

      params.rgrc[0] = *WindowRect;
      params.rgrc[1] = Window->Wnd->WindowRect;
      params.rgrc[2] = Window->Wnd->ClientRect;
      Parent = Window->Parent;
      if (0 != (Wnd->Style & WS_CHILD) && Parent)
      {
         IntGdiOffsetRect(&(params.rgrc[0]), - Parent->Wnd->ClientRect.left,
                          - Parent->Wnd->ClientRect.top);
         IntGdiOffsetRect(&(params.rgrc[1]), - Parent->Wnd->ClientRect.left,
                          - Parent->Wnd->ClientRect.top);
         IntGdiOffsetRect(&(params.rgrc[2]), - Parent->Wnd->ClientRect.left,
                          - Parent->Wnd->ClientRect.top);
      }
      params.lppos = &winposCopy;
      winposCopy = *WinPos;

      wvrFlags = co_IntSendMessage(Window->hSelf, WM_NCCALCSIZE, TRUE, (LPARAM) &params);

      /* If the application send back garbage, ignore it */
      if (params.rgrc[0].left <= params.rgrc[0].right &&
          params.rgrc[0].top <= params.rgrc[0].bottom)
      {
         *ClientRect = params.rgrc[0];
         if ((Wnd->Style & WS_CHILD) && Parent)
         {
            IntGdiOffsetRect(ClientRect, Parent->Wnd->ClientRect.left,
                             Parent->Wnd->ClientRect.top);
         }
         FixClientRect(ClientRect, WindowRect);
      }

      /* FIXME: WVR_ALIGNxxx */

      if (ClientRect->left != Wnd->ClientRect.left ||
          ClientRect->top != Wnd->ClientRect.top)
      {
         WinPos->flags &= ~SWP_NOCLIENTMOVE;
      }

      if ((ClientRect->right - ClientRect->left !=
            Wnd->ClientRect.right - Wnd->ClientRect.left) ||
            (ClientRect->bottom - ClientRect->top !=
             Wnd->ClientRect.bottom - Wnd->ClientRect.top))
      {
         WinPos->flags &= ~SWP_NOCLIENTSIZE;
      }
   }
   else
   {
      if (! (WinPos->flags & SWP_NOMOVE)
            && (ClientRect->left != Wnd->ClientRect.left ||
                ClientRect->top != Wnd->ClientRect.top))
      {
         WinPos->flags &= ~SWP_NOCLIENTMOVE;
      }
   }

   return wvrFlags;
}

static
BOOL FASTCALL
co_WinPosDoWinPosChanging(PWINDOW_OBJECT Window,
                          PWINDOWPOS WinPos,
                          PRECT WindowRect,
                          PRECT ClientRect)
{
   INT X, Y;
   PWINDOW Wnd;

   ASSERT_REFS_CO(Window);
   Wnd = Window->Wnd;

   if (!(WinPos->flags & SWP_NOSENDCHANGING))
   {
      co_IntPostOrSendMessage(Window->hSelf, WM_WINDOWPOSCHANGING, 0, (LPARAM) WinPos);
   }

   *WindowRect = Wnd->WindowRect;
   *ClientRect = Wnd->ClientRect;

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
      if ((0 != (Wnd->Style & WS_CHILD)) && Parent)
      {
         X += Parent->Wnd->ClientRect.left;
         Y += Parent->Wnd->ClientRect.top;
      }

      WindowRect->left = X;
      WindowRect->top = Y;
      WindowRect->right += X - Wnd->WindowRect.left;
      WindowRect->bottom += Y - Wnd->WindowRect.top;
      IntGdiOffsetRect(ClientRect,
                       X - Wnd->WindowRect.left,
                       Y - Wnd->WindowRect.top);
   }

   WinPos->flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;

   return TRUE;
}

/*
 * Fix Z order taking into account owned popups -
 * basically we need to maintain them above the window that owns them
 */
static
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
         DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
         List = IntWinListChildren(DesktopWindow);

         if (List != NULL)
         {
            for (i = 0; List[i]; i++)
            {
               if (List[i] == Owner)
                  break;
               if (HWND_TOP == hWndInsertAfter)
               {
                  ChildObject = UserGetWindowObject(List[i]);
                  if (NULL != ChildObject)
                  {
                     if (0 == (ChildObject->Wnd->ExStyle & WS_EX_TOPMOST))
                     {
                        break;
                     }
                  }
               }
               if (List[i] != hWnd)
                  hWndLocalPrev = List[i];
               if (hWndLocalPrev == hWndInsertAfter)
                  break;
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
      DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
      List = IntWinListChildren(DesktopWindow);
   }
   if (List != NULL)
   {
      for (i = 0; List[i]; i++)
      {
         PWINDOW_OBJECT Wnd;

         if (List[i] == hWnd)
            break;

         if (!(Wnd = UserGetWindowObject(List[i])))
            continue;

         if ((Wnd->Wnd->Style & WS_POPUP) &&
               UserGetWindow(List[i], GW_OWNER) == hWnd)
         {
            USER_REFERENCE_ENTRY Ref;
            UserRefObjectCo(Wnd, &Ref);

            co_WinPosSetWindowPos(Wnd, hWndInsertAfter, 0, 0, 0, 0,
                                  SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

            UserDerefObjectCo(Wnd);

            hWndInsertAfter = List[i];
         }
      }
      ExFreePool(List);
   }

   return hWndInsertAfter;
}

/***********************************************************************
 *      WinPosInternalMoveWindow
 *
 * Update WindowRect and ClientRect of Window and all of its children
 * We keep both WindowRect and ClientRect in screen coordinates internally
 */
static
VOID FASTCALL
WinPosInternalMoveWindow(PWINDOW_OBJECT Window, INT MoveX, INT MoveY)
{
   PWINDOW_OBJECT Child;

   Window->Wnd->WindowRect.left += MoveX;
   Window->Wnd->WindowRect.right += MoveX;
   Window->Wnd->WindowRect.top += MoveY;
   Window->Wnd->WindowRect.bottom += MoveY;

   Window->Wnd->ClientRect.left += MoveX;
   Window->Wnd->ClientRect.right += MoveX;
   Window->Wnd->ClientRect.top += MoveY;
   Window->Wnd->ClientRect.bottom += MoveY;

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
static
BOOL FASTCALL
WinPosFixupFlags(WINDOWPOS *WinPos, PWINDOW_OBJECT Window)
{
   PWINDOW Wnd = Window->Wnd;
   if (Wnd->Style & WS_VISIBLE)
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
   if (Wnd->WindowRect.right - Wnd->WindowRect.left == WinPos->cx &&
       Wnd->WindowRect.bottom - Wnd->WindowRect.top == WinPos->cy)
   {
      WinPos->flags |= SWP_NOSIZE;
   }

   /* Check for right position */
   if (Wnd->WindowRect.left == WinPos->x &&
       Wnd->WindowRect.top == WinPos->y)
   {
      WinPos->flags |= SWP_NOMOVE;
   }

   if (WinPos->hwnd == UserGetForegroundWindow())
   {
      WinPos->flags |= SWP_NOACTIVATE;   /* Already active */
   }
   else
      if ((Wnd->Style & (WS_POPUP | WS_CHILD)) != WS_CHILD)
      {
         /* Bring to the top when activating */
         if (!(WinPos->flags & SWP_NOACTIVATE))
         {
            WinPos->flags &= ~SWP_NOZORDER;
            WinPos->hwndInsertAfter = (0 != (Wnd->ExStyle & WS_EX_TOPMOST) ?
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
               && 0 != (Wnd->ExStyle & WS_EX_TOPMOST))
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
         PWINDOW_OBJECT InsAfterWnd, Parent = Window->Parent;

         InsAfterWnd = UserGetWindowObject(WinPos->hwndInsertAfter);

         if (InsAfterWnd && UserGetAncestor(InsAfterWnd, GA_PARENT) != Parent)
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
co_WinPosSetWindowPos(
   PWINDOW_OBJECT Window,
   HWND WndInsertAfter,
   INT x,
   INT y,
   INT cx,
   INT cy,
   UINT flags
)
{
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

   ASSERT_REFS_CO(Window);

   /* FIXME: Get current active window from active queue. */

   /*
    * Only allow CSRSS to mess with the desktop window
    */
   if (Window->hSelf == IntGetDesktopWindow() &&
         Window->OwnerThread->ThreadsProcess != PsGetCurrentProcess())
   {
      return FALSE;
   }

   WinPos.hwnd = Window->hSelf;
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
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   /* Does the window still exist? */
   if (!IntIsWindow(WinPos.hwnd))
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   if ((WinPos.flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) !=
         SWP_NOZORDER &&
//         UserGetAncestor(WinPos.hwnd, GA_PARENT) == IntGetDesktopWindow())
//faxme: is WinPos.hwnd constant?? (WinPos.hwnd = Window->hSelf above)
         UserGetAncestor(Window, GA_PARENT)->hSelf == IntGetDesktopWindow())
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

         if (VisBefore != NULL && (VisRgn = (PROSRGNDATA)REGION_LockRgn(VisBefore)) &&
               REGION_GetRgnBox(VisRgn, &TempRect) == NULLREGION)
         {
            REGION_UnlockRgn(VisRgn);
            NtGdiDeleteObject(VisBefore);
            VisBefore = NULL;
         }
         else if(VisRgn)
         {
            REGION_UnlockRgn(VisRgn);
            NtGdiOffsetRgn(VisBefore, -Window->Wnd->WindowRect.left, -Window->Wnd->WindowRect.top);
         }
      }
   }

   WvrFlags = co_WinPosDoNCCALCSize(Window, &WinPos, &NewWindowRect, &NewClientRect);

    //DPRINT1("co_WinPosDoNCCALCSize");

   /* Relink windows. (also take into account shell window in hwndShellWindow) */
   if (!(WinPos.flags & SWP_NOZORDER) && WinPos.hwnd != UserGetShellWindow())
   {
      PWINDOW_OBJECT ParentWindow;
      PWINDOW_OBJECT Sibling;
      PWINDOW_OBJECT InsertAfterWindow;

      if ((ParentWindow = Window->Parent))
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
            while (NULL != Sibling && 0 != (Sibling->Wnd->ExStyle & WS_EX_TOPMOST))
            {
               InsertAfterWindow = Sibling;
               Sibling = Sibling->NextSibling;
            }
            if (NULL != InsertAfterWindow)
            {
               UserReferenceObject(InsertAfterWindow);
            }
         }
         else if (WinPos.hwndInsertAfter == HWND_BOTTOM)
         {
            if(ParentWindow->LastChild)
            {
               UserReferenceObject(ParentWindow->LastChild);
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
            UserDereferenceObject(InsertAfterWindow);
         if ((HWND_TOPMOST == WinPos.hwndInsertAfter)
               || (0 != (Window->Wnd->ExStyle & WS_EX_TOPMOST)
                   && NULL != Window->PrevSibling
                   && 0 != (Window->PrevSibling->Wnd->ExStyle & WS_EX_TOPMOST))
               || (NULL != Window->NextSibling
                   && 0 != (Window->NextSibling->Wnd->ExStyle & WS_EX_TOPMOST)))
         {
            Window->Wnd->ExStyle |= WS_EX_TOPMOST;
         }
         else
         {
            Window->Wnd->ExStyle &= ~ WS_EX_TOPMOST;
         }

      }
   }

   OldWindowRect = Window->Wnd->WindowRect;
   OldClientRect = Window->Wnd->ClientRect;

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

   Window->Wnd->WindowRect = NewWindowRect;
   Window->Wnd->ClientRect = NewClientRect;

   if (!(WinPos.flags & SWP_SHOWWINDOW) && (WinPos.flags & SWP_HIDEWINDOW))
   {
      /* Clear the update region */
      co_UserRedrawWindow(Window, NULL, 0, RDW_VALIDATE | RDW_NOFRAME |
                          RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ALLCHILDREN);
      if ((Window->Wnd->Style & WS_VISIBLE) &&
          Window->Parent == UserGetDesktopWindow())
      {
         co_IntShellHookNotify(HSHELL_WINDOWDESTROYED, (LPARAM)Window->hSelf);
      }
      Window->Wnd->Style &= ~WS_VISIBLE;
   }
   else if (WinPos.flags & SWP_SHOWWINDOW)
   {
      if (!(Window->Wnd->Style & WS_VISIBLE) &&
          Window->Parent == UserGetDesktopWindow())
      {
         co_IntShellHookNotify(HSHELL_WINDOWCREATED, (LPARAM)Window->hSelf);
      }
      Window->Wnd->Style |= WS_VISIBLE;
   }

   if (Window->UpdateRegion != NULL && Window->UpdateRegion != (HRGN)1)
   {
      NtGdiOffsetRgn(Window->UpdateRegion,
                     NewWindowRect.left - OldWindowRect.left,
                     NewWindowRect.top - OldWindowRect.top);
   }

   DceResetActiveDCEs(Window);

   if (!(WinPos.flags & SWP_NOREDRAW))
   {
      /* Determine the new visible region */
      VisAfter = VIS_ComputeVisibleRegion(Window, FALSE, FALSE, TRUE);
      VisRgn = NULL;

      if (VisAfter != NULL && (VisRgn = (PROSRGNDATA)REGION_LockRgn(VisAfter)) &&
            REGION_GetRgnBox(VisRgn, &TempRect) == NULLREGION)
      {
         REGION_UnlockRgn(VisRgn);
         NtGdiDeleteObject(VisAfter);
         VisAfter = NULL;
      }
      else if(VisRgn)
      {
         REGION_UnlockRgn(VisRgn);
         NtGdiOffsetRgn(VisAfter, -Window->Wnd->WindowRect.left, -Window->Wnd->WindowRect.top);
      }

      /*
       * Determine which pixels can be copied from the old window position
       * to the new. Those pixels must be visible in both the old and new
       * position. Also, check the class style to see if the windows of this
       * class need to be completely repainted on (horizontal/vertical) size
       * change.
       */
      if (VisBefore != NULL && VisAfter != NULL && !(WinPos.flags & SWP_NOCOPYBITS) &&
          ((WinPos.flags & SWP_NOSIZE) || !(WvrFlags & WVR_REDRAW)) &&
          !(Window->Wnd->ExStyle & WS_EX_TRANSPARENT))
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
            PROSRGNDATA pCopyRgn;
            RECT ORect = OldClientRect;
            RECT NRect = NewClientRect;
            IntGdiOffsetRect(&ORect, - OldWindowRect.left, - OldWindowRect.top);
            IntGdiOffsetRect(&NRect, - NewWindowRect.left, - NewWindowRect.top);
            IntGdiIntersectRect(&CopyRect, &ORect, &NRect);
            pCopyRgn = REGION_LockRgn(CopyRgn);
            REGION_CropAndOffsetRegion(pCopyRgn, pCopyRgn, &CopyRect, NULL);
            REGION_UnlockRgn(pCopyRgn);
         }

         /* No use in copying bits which are in the update region. */
         if (Window->UpdateRegion != NULL)
         {
            NtGdiOffsetRgn(CopyRgn, NewWindowRect.left, NewWindowRect.top);
            NtGdiCombineRgn(CopyRgn, CopyRgn, Window->UpdateRegion, RGN_DIFF);
            NtGdiOffsetRgn(CopyRgn, -NewWindowRect.left, -NewWindowRect.top);
         }

         /*
          * Now, get the bounding box of the copy region. If it's empty
          * there's nothing to copy. Also, it's no use copying bits onto
          * themselves.
          */
         if ((VisRgn = (PROSRGNDATA)REGION_LockRgn(CopyRgn)) &&
               REGION_GetRgnBox(VisRgn, &CopyRect) == NULLREGION)
         {
            /* Nothing to copy, clean up */
            REGION_UnlockRgn(VisRgn);
            NtGdiDeleteObject(CopyRgn);
            CopyRgn = NULL;
         }
         else if (OldWindowRect.left != NewWindowRect.left ||
                  OldWindowRect.top != NewWindowRect.top)
         {
            if(VisRgn)
            {
               REGION_UnlockRgn(VisRgn);
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
            NtGdiOffsetRgn(CopyRgn, NewWindowRect.left, NewWindowRect.top);
            Dc = UserGetDCEx(Window, CopyRgn, DCX_WINDOW | DCX_CACHE |
                             DCX_INTERSECTRGN | DCX_CLIPSIBLINGS |
                             DCX_KEEPCLIPRGN);
            NtGdiBitBlt(Dc,
                        CopyRect.left, CopyRect.top, CopyRect.right - CopyRect.left,
                        CopyRect.bottom - CopyRect.top, Dc,
                        CopyRect.left + (OldWindowRect.left - NewWindowRect.left),
                        CopyRect.top + (OldWindowRect.top - NewWindowRect.top), SRCCOPY, 0, 0);
            UserReleaseDC(Window, Dc, FALSE);
            IntValidateParent(Window, CopyRgn, FALSE);
            NtGdiOffsetRgn(CopyRgn, -NewWindowRect.left, -NewWindowRect.top);
         }
         else if(VisRgn)
         {
            REGION_UnlockRgn(VisRgn);
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
                      /* old code
            NtGdiOffsetRgn(DirtyRgn, Window->WindowRect.left, Window->WindowRect.top);
            IntInvalidateWindows(Window, DirtyRgn,
               RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
         }
         NtGdiDeleteObject(DirtyRgn);
         */

            PWINDOW_OBJECT Parent = Window->Parent;

            NtGdiOffsetRgn(DirtyRgn,
                           Window->Wnd->WindowRect.left,
                           Window->Wnd->WindowRect.top);
            if ((Window->Wnd->Style & WS_CHILD) &&
                (Parent) &&
                !(Parent->Wnd->Style & WS_CLIPCHILDREN))
            {
               IntInvalidateWindows(Parent, DirtyRgn,
                  RDW_ERASE | RDW_INVALIDATE);
               co_IntPaintWindows(Parent, RDW_ERASENOW, FALSE);
            }
            else
            {
                IntInvalidateWindows(Window, DirtyRgn,
                RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
            }
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
         if ((Window->Wnd->Style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
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
      co_IntPostOrSendMessage(WinPos.hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM) &WinPos);

   return TRUE;
}

LRESULT FASTCALL
co_WinPosGetNonClientSize(PWINDOW_OBJECT Window, RECT* WindowRect, RECT* ClientRect)
{
   LRESULT Result;

   ASSERT_REFS_CO(Window);

   *ClientRect = *WindowRect;
   Result = co_IntSendMessage(Window->hSelf, WM_NCCALCSIZE, FALSE, (LPARAM) ClientRect);

   FixClientRect(ClientRect, WindowRect);

   return Result;
}

BOOLEAN FASTCALL
co_WinPosShowWindow(PWINDOW_OBJECT Window, INT Cmd)
{
   BOOLEAN WasVisible;
   UINT Swp = 0;
   RECT NewPos;
   BOOLEAN ShowFlag;
   //  HRGN VisibleRgn;
   PWINDOW Wnd;

   ASSERT_REFS_CO(Window);
   Wnd = Window->Wnd;

   WasVisible = (Wnd->Style & WS_VISIBLE) != 0;

   switch (Cmd)
   {
      case SW_HIDE:
         {
            if (!WasVisible)
            {
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
            Swp |= SWP_NOACTIVATE;
            if (!(Wnd->Style & WS_MINIMIZE))
            {
               Swp |= co_WinPosMinMaximize(Window, SW_MINIMIZE, &NewPos) |
                      SWP_FRAMECHANGED;
            }
            else
            {
               Swp |= SWP_NOSIZE | SWP_NOMOVE;
               if (! WasVisible)
               {
                  Swp |= SWP_FRAMECHANGED;
               }
            }
            break;
         }

      case SW_SHOWMAXIMIZED:
         {
            Swp |= SWP_SHOWWINDOW;
            if (!(Wnd->Style & WS_MAXIMIZE))
            {
               Swp |= co_WinPosMinMaximize(Window, SW_MAXIMIZE, &NewPos) |
                      SWP_FRAMECHANGED;
            }
            else
            {
               Swp |= SWP_NOSIZE | SWP_NOMOVE;
               if (! WasVisible)
               {
                  Swp |= SWP_FRAMECHANGED;
               }
            }
            break;
         }

      case SW_SHOWNA:
         Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
         /* Fall through. */
      case SW_SHOW:
            if (WasVisible) return(TRUE); // Nothing to do!
         Swp |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
         /* Don't activate the topmost window. */
         break;

      case SW_SHOWNOACTIVATE:
         //Swp |= SWP_NOZORDER;
         Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
         /* Fall through. */
      case SW_SHOWNORMAL:
      case SW_SHOWDEFAULT:
      case SW_RESTORE:
         Swp |= SWP_SHOWWINDOW;
         if (Wnd->Style & (WS_MINIMIZE | WS_MAXIMIZE))
         {
            Swp |= co_WinPosMinMaximize(Window, SW_RESTORE, &NewPos) |
                   SWP_FRAMECHANGED;
         }
         else
         {
            Swp |= SWP_NOSIZE | SWP_NOMOVE;
            if (! WasVisible)
            {
               Swp |= SWP_FRAMECHANGED;
            }
         }
         break;
   }

   ShowFlag = (Cmd != SW_HIDE);

   if (ShowFlag != WasVisible)
   {
      co_IntSendMessage(Window->hSelf, WM_SHOWWINDOW, ShowFlag, 0);
   }

   /* We can't activate a child window */
   if ((Wnd->Style & WS_CHILD) &&
       !(Wnd->ExStyle & WS_EX_MDICHILD))
   {
      Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
   }

   co_WinPosSetWindowPos(Window, 0 != (Wnd->ExStyle & WS_EX_TOPMOST)
                         ? HWND_TOPMOST : HWND_TOP,
                         NewPos.left, NewPos.top, NewPos.right, NewPos.bottom, LOWORD(Swp));

   if (Cmd == SW_HIDE)
   {
      PWINDOW_OBJECT ThreadFocusWindow;

      /* FIXME: This will cause the window to be activated irrespective
       * of whether it is owned by the same thread. Has to be done
       * asynchronously.
       */

      if (Window->hSelf == UserGetActiveWindow())
      {
         co_WinPosActivateOtherWindow(Window);
      }


      //temphack
      ThreadFocusWindow = UserGetWindowObject(IntGetThreadFocusWindow());

      /* Revert focus to parent */
      if (ThreadFocusWindow && (Window == ThreadFocusWindow ||
            IntIsChildWindow(Window, ThreadFocusWindow)))
      {
         //faxme: as long as we have ref on Window, we also, indirectly, have ref on parent...
         co_UserSetFocus(Window->Parent);
      }
   }

   /* FIXME: Check for window destruction. */

   if ((Window->Flags & WINDOWOBJECT_NEED_SIZE) &&
       !(Window->Status & WINDOWSTATUS_DESTROYING))
   {
      WPARAM wParam = SIZE_RESTORED;

      Window->Flags &= ~WINDOWOBJECT_NEED_SIZE;
      if (Wnd->Style & WS_MAXIMIZE)
      {
         wParam = SIZE_MAXIMIZED;
      }
      else if (Wnd->Style & WS_MINIMIZE)
      {
         wParam = SIZE_MINIMIZED;
      }

      co_IntSendMessage(Window->hSelf, WM_SIZE, wParam,
                        MAKELONG(Wnd->ClientRect.right -
                                 Wnd->ClientRect.left,
                                 Wnd->ClientRect.bottom -
                                 Wnd->ClientRect.top));
      co_IntSendMessage(Window->hSelf, WM_MOVE, 0,
                        MAKELONG(Wnd->ClientRect.left,
                                 Wnd->ClientRect.top));
      IntEngWindowChanged(Window, WOC_RGN_CLIENT);

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


#if 0

/* find child of 'parent' that contains the given point (in parent-relative coords) */
PWINDOW_OBJECT child_window_from_point(PWINDOW_OBJECT parent, int x, int y )
{
    PWINDOW_OBJECT Wnd;// = parent->FirstChild;

//    LIST_FOR_EACH_ENTRY( Wnd, &parent->children, struct window, entry )
    for (Wnd = parent->FirstChild; Wnd; Wnd = Wnd->NextSibling)
    {
        if (!IntPtInWindow( Wnd, x, y )) continue;  /* skip it */

        /* if window is minimized or disabled, return at once */
        if (Wnd->Style & (WS_MINIMIZE|WS_DISABLED)) return Wnd;

        /* if point is not in client area, return at once */
        if (x < Wnd->ClientRect.left || x >= Wnd->ClientRect.right ||
            y < Wnd->ClientRect.top || y >= Wnd->ClientRect.bottom)
            return Wnd;

        return child_window_from_point( Wnd, x - Wnd->ClientRect.left, y - Wnd->ClientRect.top );
    }
    return parent;  /* not found any child */
}
#endif

/* wine server: child_window_from_point

Caller must dereference the "returned" Window
*/
static
VOID FASTCALL
co_WinPosSearchChildren(
   PWINDOW_OBJECT ScopeWin,
   PUSER_MESSAGE_QUEUE OnlyHitTests,
   POINT *Point,
   PWINDOW_OBJECT* Window,
   USHORT *HitTest
   )
{
   PWINDOW_OBJECT Current;
   PWINDOW CurrentWnd;
   HWND *List, *phWnd;
   USER_REFERENCE_ENTRY Ref;

   ASSERT_REFS_CO(ScopeWin);

   if ((List = IntWinListChildren(ScopeWin)))
   {
      for (phWnd = List; *phWnd; ++phWnd)
      {
         if (!(Current = UserGetWindowObject(*phWnd)))
            continue;
         CurrentWnd = Current->Wnd;

         if (!(CurrentWnd->Style & WS_VISIBLE))
         {
            continue;
         }

         if ((CurrentWnd->Style & (WS_POPUP | WS_CHILD | WS_DISABLED)) ==
               (WS_CHILD | WS_DISABLED))
         {
            continue;
         }

         if (!IntPtInWindow(Current, Point->x, Point->y))
         {
             continue;
         }

         if (*Window) UserDereferenceObject(*Window);
         *Window = Current;
         UserReferenceObject(*Window);

         if (CurrentWnd->Style & WS_MINIMIZE)
         {
            *HitTest = HTCAPTION;
            break;
         }

         if (CurrentWnd->Style & WS_DISABLED)
         {
            *HitTest = HTERROR;
            break;
         }

         UserRefObjectCo(Current, &Ref);

         if (OnlyHitTests && (Current->MessageQueue == OnlyHitTests))
         {
            *HitTest = co_IntSendMessage(Current->hSelf, WM_NCHITTEST, 0,
                                         MAKELONG(Point->x, Point->y));
            if ((*HitTest) == (USHORT)HTTRANSPARENT)
            {
               UserDerefObjectCo(Current);
               continue;
            }
         }
         else
            *HitTest = HTCLIENT;

         if (Point->x >= CurrentWnd->ClientRect.left &&
               Point->x < CurrentWnd->ClientRect.right &&
               Point->y >= CurrentWnd->ClientRect.top &&
               Point->y < CurrentWnd->ClientRect.bottom)
         {
            co_WinPosSearchChildren(Current, OnlyHitTests, Point, Window, HitTest);
         }

         UserDerefObjectCo(Current);

         break;
      }
      ExFreePool(List);
   }
}

/* wine: WINPOS_WindowFromPoint */
USHORT FASTCALL
co_WinPosWindowFromPoint(PWINDOW_OBJECT ScopeWin, PUSER_MESSAGE_QUEUE OnlyHitTests, POINT *WinPoint,
                         PWINDOW_OBJECT* Window)
{
   HWND DesktopWindowHandle;
   PWINDOW_OBJECT DesktopWindow;
   POINT Point = *WinPoint;
   USHORT HitTest;

   ASSERT_REFS_CO(ScopeWin);

   *Window = NULL;

   if(!ScopeWin)
   {
      DPRINT1("WinPosWindowFromPoint(): ScopeWin == NULL!\n");
      return(HTERROR);
   }

   if (ScopeWin->Wnd->Style & WS_DISABLED)
   {
      return(HTERROR);
   }

   /* Translate the point to the space of the scope window. */
   DesktopWindowHandle = IntGetDesktopWindow();
   if((DesktopWindowHandle != ScopeWin->hSelf) &&
         (DesktopWindow = UserGetWindowObject(DesktopWindowHandle)))
   {
      Point.x += ScopeWin->Wnd->ClientRect.left - DesktopWindow->Wnd->ClientRect.left;
      Point.y += ScopeWin->Wnd->ClientRect.top - DesktopWindow->Wnd->ClientRect.top;
   }

   HitTest = HTNOWHERE;

   co_WinPosSearchChildren(ScopeWin, OnlyHitTests, &Point, Window, &HitTest);

   return ((*Window) ? HitTest : HTNOWHERE);
}

BOOL
APIENTRY
NtUserGetMinMaxInfo(
   HWND hWnd,
   MINMAXINFO *MinMaxInfo,
   BOOL SendMessage)
{
   POINT Size;
   PWINDOW_OBJECT Window = NULL;
   PWINDOW Wnd;
   MINMAXINFO SafeMinMax;
   NTSTATUS Status;
   BOOL ret;
   USER_REFERENCE_ENTRY Ref;

   DPRINT("Enter NtUserGetMinMaxInfo\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      ret = FALSE;
      goto cleanup;
   }

   UserRefObjectCo(Window, &Ref);
   Wnd = Window->Wnd;

   Size.x = Window->Wnd->WindowRect.left;
   Size.y = Window->Wnd->WindowRect.top;
   WinPosInitInternalPos(Window, &Size,
                         &Wnd->WindowRect);

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
      SetLastNtError(Status);
      ret = FALSE;
      goto cleanup;
   }

   ret = TRUE;

cleanup:
   if (Window) UserDerefObjectCo(Window);

   DPRINT("Leave NtUserGetMinMaxInfo, ret=%i\n", ret);
   UserLeave();
   return ret;
}

/* EOF */
