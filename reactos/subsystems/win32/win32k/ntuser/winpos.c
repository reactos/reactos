/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Windows
 * FILE:             subsystems/win32/win32k/ntuser/window.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserWinpos);

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
IntGetClientOrigin(PWND Window OPTIONAL, LPPOINT Point)
{
   Window = Window ? Window : UserGetWindowObject(IntGetDesktopWindow());
   if (Window == NULL)
   {
      Point->x = Point->y = 0;
      return FALSE;
   }
   Point->x = Window->rcClient.left;
   Point->y = Window->rcClient.top;

   return TRUE;
}


/*******************************************************************
 *         can_activate_window
 *
 * Check if we can activate the specified window.
 */
static
BOOL FASTCALL can_activate_window( PWND Wnd OPTIONAL)
{
    LONG style;

    if (!Wnd) return FALSE;

    style = Wnd->style;
    if (!(style & WS_VISIBLE) &&
        Wnd->head.pti->pEThread->ThreadsProcess != CsrProcess) return FALSE;
    if ((style & WS_MINIMIZE) &&
        Wnd->head.pti->pEThread->ThreadsProcess != CsrProcess) return FALSE;
    if ((style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
    return TRUE;
    /* FIXME: This window could be disable  because the child that closed
              was a popup. */
    //return !(style & WS_DISABLED);
}


/*******************************************************************
 *         WinPosActivateOtherWindow
 *
 *  Activates window other than pWnd.
 */
VOID FASTCALL
co_WinPosActivateOtherWindow(PWND Wnd)
{
   PWND WndTo = NULL;
   HWND Fg;
   USER_REFERENCE_ENTRY Ref;

   ASSERT_REFS_CO(Wnd);

   if (IntIsDesktopWindow(Wnd))
   {
      IntSetFocusMessageQueue(NULL);
      return;
   }

   /* If this is popup window, try to activate the owner first. */
   if ((Wnd->style & WS_POPUP) && (WndTo = Wnd->spwndOwner))
   {
      WndTo = UserGetAncestor( WndTo, GA_ROOT );
      if (can_activate_window(WndTo)) goto done;
   }

   /* Pick a next top-level window. */
   /* FIXME: Search for non-tooltip windows first. */
   WndTo = Wnd;
   for (;;)
   {
      if (!(WndTo = WndTo->spwndNext)) break;
      if (can_activate_window( WndTo )) break;
   }

done:

   if (WndTo) UserRefObjectCo(WndTo, &Ref);

   Fg = UserGetForegroundWindow();
   if ((!Fg || Wnd->head.h == Fg) && WndTo)//fixme: ok if WndTo is NULL??
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
co_WinPosArrangeIconicWindows(PWND parent)
{
   RECTL rectParent;
   INT i, x, y, xspacing, yspacing;
   HWND *List = IntWinListChildren(parent);

   ASSERT_REFS_CO(parent);

   /* Check if we found any children */
   if(List == NULL)
   {
       return 0;
   }

   IntGetClientRect( parent, &rectParent );
   x = rectParent.left;
   y = rectParent.bottom;

   xspacing = UserGetSystemMetrics(SM_CXICONSPACING);
   yspacing = UserGetSystemMetrics(SM_CYICONSPACING);

   TRACE("X:%d Y:%d XS:%d YS:%d\n",x,y,xspacing,yspacing);

   for( i = 0; List[i]; i++)
   {
      PWND Child;

      if (!(Child = UserGetWindowObject(List[i])))
         continue;

      if((Child->style & WS_MINIMIZE) != 0 )
      {
         USER_REFERENCE_ENTRY Ref;
         UserRefObjectCo(Child, &Ref);

         co_WinPosSetWindowPos(Child, 0, x + (xspacing - UserGetSystemMetrics(SM_CXICON)) / 2,
                               y - yspacing - UserGetSystemMetrics(SM_CYICON) / 2
                               , 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE );

         UserDerefObjectCo(Child);

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
WinPosFindIconPos(PWND Window, POINT *Pos)
{
   ERR("WinPosFindIconPos FIXME!\n");
}

VOID FASTCALL
WinPosInitInternalPos(PWND Wnd, POINT *pt, RECTL *RestoreRect)
{
    PWND Parent;
    UINT XInc, YInc;

   if (!Wnd->InternalPosInitialized)
   {
      RECTL WorkArea;

      Parent = Wnd->spwndParent;
      if(Parent)
      {
         if(IntIsDesktopWindow(Parent))
             UserSystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0);
         else
            WorkArea = Parent->rcClient;
      }
      else
         UserSystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0);

      Wnd->InternalPos.NormalRect = Wnd->rcWindow;
      IntGetWindowBorderMeasures(Wnd, &XInc, &YInc);
      Wnd->InternalPos.MaxPos.x = WorkArea.left - XInc;
      Wnd->InternalPos.MaxPos.y = WorkArea.top - YInc;
      Wnd->InternalPos.IconPos.x = WorkArea.left;
      Wnd->InternalPos.IconPos.y = WorkArea.bottom - UserGetSystemMetrics(SM_CYMINIMIZED);

      Wnd->InternalPosInitialized = TRUE;
   }
   if (Wnd->style & WS_MINIMIZE)
   {
      Wnd->InternalPos.IconPos = *pt;
   }
   else if (Wnd->style & WS_MAXIMIZE)
   {
      Wnd->InternalPos.MaxPos = *pt;
   }
   else if (RestoreRect != NULL)
   {
      Wnd->InternalPos.NormalRect = *RestoreRect;
   }
}

UINT FASTCALL
co_WinPosMinMaximize(PWND Wnd, UINT ShowFlag, RECT* NewPos)
{
   POINT Size;
   UINT SwpFlags = 0;

   ASSERT_REFS_CO(Wnd);

   Size.x = Wnd->rcWindow.left;
   Size.y = Wnd->rcWindow.top;
   WinPosInitInternalPos(Wnd, &Size, &Wnd->rcWindow);

   if (co_HOOK_CallHooks( WH_CBT, HCBT_MINMAX, (WPARAM)Wnd->head.h, ShowFlag))
   {
      ERR("WinPosMinMaximize WH_CBT Call Hook return!\n");
      return SWP_NOSIZE | SWP_NOMOVE;
   }
      if (Wnd->style & WS_MINIMIZE)
      {
         if (!co_IntSendMessageNoWait(Wnd->head.h, WM_QUERYOPEN, 0, 0))
         {
            return(SWP_NOSIZE | SWP_NOMOVE);
         }
         SwpFlags |= SWP_NOCOPYBITS;
      }
      switch (ShowFlag)
      {
         case SW_MINIMIZE:
            {
               if (Wnd->style & WS_MAXIMIZE)
               {
                  Wnd->state2 |= WNDS2_MAXIMIZEBUTTONDOWN;
                  Wnd->style &= ~WS_MAXIMIZE;
               }
               else
               {
                  Wnd->state2 &= ~WNDS2_MAXIMIZEBUTTONDOWN;
               }
               co_UserRedrawWindow(Wnd, NULL, 0, RDW_VALIDATE | RDW_NOERASE |
                                   RDW_NOINTERNALPAINT);
               Wnd->style |= WS_MINIMIZE;
               WinPosFindIconPos(Wnd, &Wnd->InternalPos.IconPos);
               RECTL_vSetRect(NewPos, Wnd->InternalPos.IconPos.x, Wnd->InternalPos.IconPos.y,
                             UserGetSystemMetrics(SM_CXMINIMIZED),
                             UserGetSystemMetrics(SM_CYMINIMIZED));
               SwpFlags |= SWP_NOCOPYBITS;
               break;
            }

         case SW_MAXIMIZE:
            {
               co_WinPosGetMinMaxInfo(Wnd, &Size, &Wnd->InternalPos.MaxPos,
                                      NULL, NULL);
               TRACE("Maximize: %d,%d %dx%d\n",
                      Wnd->InternalPos.MaxPos.x, Wnd->InternalPos.MaxPos.y, Size.x, Size.y);
               if (Wnd->style & WS_MINIMIZE)
               {
                  Wnd->style &= ~WS_MINIMIZE;
               }
               Wnd->style |= WS_MAXIMIZE;
               RECTL_vSetRect(NewPos, Wnd->InternalPos.MaxPos.x, Wnd->InternalPos.MaxPos.y,
                             Size.x, Size.y);
               break;
            }

         case SW_RESTORE:
            {
               if (Wnd->style & WS_MINIMIZE)
               {
                  Wnd->style &= ~WS_MINIMIZE;
                  if (Wnd->state2 & WNDS2_MAXIMIZEBUTTONDOWN)
                  {
                     co_WinPosGetMinMaxInfo(Wnd, &Size,
                                            &Wnd->InternalPos.MaxPos, NULL, NULL);
                     Wnd->style |= WS_MAXIMIZE;
                     RECTL_vSetRect(NewPos, Wnd->InternalPos.MaxPos.x,
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
                  if (!(Wnd->style & WS_MAXIMIZE))
                  {
                     return 0;
                  }
                  Wnd->style &= ~WS_MAXIMIZE;
                  *NewPos = Wnd->InternalPos.NormalRect;
                  NewPos->right -= NewPos->left;
                  NewPos->bottom -= NewPos->top;
                  break;
               }
            }
      }
   return(SwpFlags);
}

BOOL
UserHasWindowEdge(DWORD Style, DWORD ExStyle)
{
   if (Style & WS_MINIMIZE)
      return TRUE;
   if (ExStyle & WS_EX_DLGMODALFRAME)
      return TRUE;
   if (ExStyle & WS_EX_STATICEDGE)
      return FALSE;
   if (Style & WS_THICKFRAME)
      return TRUE;
   Style &= WS_CAPTION;
   if (Style == WS_DLGFRAME || Style == WS_CAPTION)
      return TRUE;
   return FALSE;
}

VOID
UserGetWindowBorders(DWORD Style, DWORD ExStyle, SIZE *Size, BOOL WithClient)
{
   DWORD Border = 0;

   if (UserHasWindowEdge(Style, ExStyle))
      Border += 2;
   else if (ExStyle & WS_EX_STATICEDGE)
      Border += 1;
   if ((ExStyle & WS_EX_CLIENTEDGE) && WithClient)
      Border += 2;
   if (Style & WS_CAPTION || ExStyle & WS_EX_DLGMODALFRAME)
      Border ++;
   Size->cx = Size->cy = Border;
   if ((Style & WS_THICKFRAME) && !(Style & WS_MINIMIZE))
   {
      Size->cx += UserGetSystemMetrics(SM_CXFRAME) - UserGetSystemMetrics(SM_CXDLGFRAME);
      Size->cy += UserGetSystemMetrics(SM_CYFRAME) - UserGetSystemMetrics(SM_CYDLGFRAME);
   }
   Size->cx *= UserGetSystemMetrics(SM_CXBORDER);
   Size->cy *= UserGetSystemMetrics(SM_CYBORDER);
}

BOOL WINAPI
UserAdjustWindowRectEx(LPRECT lpRect,
                       DWORD dwStyle,
                       BOOL bMenu,
                       DWORD dwExStyle)
{
   SIZE BorderSize;

   if (bMenu)
   {
      lpRect->top -= UserGetSystemMetrics(SM_CYMENU);
   }
   if ((dwStyle & WS_CAPTION) == WS_CAPTION)
   {
      if (dwExStyle & WS_EX_TOOLWINDOW)
         lpRect->top -= UserGetSystemMetrics(SM_CYSMCAPTION);
      else
         lpRect->top -= UserGetSystemMetrics(SM_CYCAPTION);
   }
   UserGetWindowBorders(dwStyle, dwExStyle, &BorderSize, TRUE);
   RECTL_vInflateRect(
      lpRect,
      BorderSize.cx,
      BorderSize.cy);

   return TRUE;
}

UINT FASTCALL
co_WinPosGetMinMaxInfo(PWND Window, POINT* MaxSize, POINT* MaxPos,
                       POINT* MinTrack, POINT* MaxTrack)
{
   MINMAXINFO MinMax;
   PMONITOR monitor;
    INT xinc, yinc;
    LONG style = Window->style;
    LONG adjustedStyle;
    LONG exstyle = Window->ExStyle;
    RECT rc;

    ASSERT_REFS_CO(Window);

    /* Compute default values */

    rc = Window->rcWindow;
    MinMax.ptReserved.x = rc.left;
    MinMax.ptReserved.y = rc.top;

    if ((style & WS_CAPTION) == WS_CAPTION)
        adjustedStyle = style & ~WS_BORDER; /* WS_CAPTION = WS_DLGFRAME | WS_BORDER */
    else
        adjustedStyle = style;

    if(Window->spwndParent)
        IntGetClientRect(Window->spwndParent, &rc);
    UserAdjustWindowRectEx(&rc, adjustedStyle, ((style & WS_POPUP) && Window->IDMenu), exstyle);

    xinc = -rc.left;
    yinc = -rc.top;

    MinMax.ptMaxSize.x = rc.right - rc.left;
    MinMax.ptMaxSize.y = rc.bottom - rc.top;
    if (style & (WS_DLGFRAME | WS_BORDER))
    {
        MinMax.ptMinTrackSize.x = UserGetSystemMetrics(SM_CXMINTRACK);
        MinMax.ptMinTrackSize.y = UserGetSystemMetrics(SM_CYMINTRACK);
    }
    else
    {
        MinMax.ptMinTrackSize.x = 2 * xinc;
        MinMax.ptMinTrackSize.y = 2 * yinc;
    }
    MinMax.ptMaxTrackSize.x = UserGetSystemMetrics(SM_CXMAXTRACK);
    MinMax.ptMaxTrackSize.y = UserGetSystemMetrics(SM_CYMAXTRACK);
    MinMax.ptMaxPosition.x = -xinc;
    MinMax.ptMaxPosition.y = -yinc;

    //if (!EMPTYPOINT(win->max_pos)) MinMax.ptMaxPosition = win->max_pos;

   co_IntSendMessage(Window->head.h, WM_GETMINMAXINFO, 0, (LPARAM)&MinMax);

    /* if the app didn't change the values, adapt them for the current monitor */
    if ((monitor = IntGetPrimaryMonitor()))
    {
        RECT rc_work;

        rc_work = monitor->rcMonitor;

        if (style & WS_MAXIMIZEBOX)
        {
            if ((style & WS_CAPTION) == WS_CAPTION || !(style & (WS_CHILD | WS_POPUP)))
                rc_work = monitor->rcWork;
        }

        if (MinMax.ptMaxSize.x == UserGetSystemMetrics(SM_CXSCREEN) + 2 * xinc &&
            MinMax.ptMaxSize.y == UserGetSystemMetrics(SM_CYSCREEN) + 2 * yinc)
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
FixClientRect(PRECTL ClientRect, PRECTL WindowRect)
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
co_WinPosDoNCCALCSize(PWND Window, PWINDOWPOS WinPos,
                      RECT* WindowRect, RECT* ClientRect)
{
   PWND Parent;
   UINT wvrFlags = 0;

   ASSERT_REFS_CO(Window);

   /* Send WM_NCCALCSIZE message to get new client area */
   if ((WinPos->flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE)
   {
      NCCALCSIZE_PARAMS params;
      WINDOWPOS winposCopy;

      params.rgrc[0] = *WindowRect;
      params.rgrc[1] = Window->rcWindow;
      params.rgrc[2] = Window->rcClient;
      Parent = Window->spwndParent;
      if (0 != (Window->style & WS_CHILD) && Parent)
      {
         RECTL_vOffsetRect(&(params.rgrc[0]), - Parent->rcClient.left,
                          - Parent->rcClient.top);
         RECTL_vOffsetRect(&(params.rgrc[1]), - Parent->rcClient.left,
                          - Parent->rcClient.top);
         RECTL_vOffsetRect(&(params.rgrc[2]), - Parent->rcClient.left,
                          - Parent->rcClient.top);
      }
      params.lppos = &winposCopy;
      winposCopy = *WinPos;

      wvrFlags = co_IntSendMessage(Window->head.h, WM_NCCALCSIZE, TRUE, (LPARAM) &params);

      /* If the application send back garbage, ignore it */
      if (params.rgrc[0].left <= params.rgrc[0].right &&
          params.rgrc[0].top <= params.rgrc[0].bottom)
      {
         *ClientRect = params.rgrc[0];
         if ((Window->style & WS_CHILD) && Parent)
         {
            RECTL_vOffsetRect(ClientRect, Parent->rcClient.left,
                             Parent->rcClient.top);
         }
         FixClientRect(ClientRect, WindowRect);
      }

      /* FIXME: WVR_ALIGNxxx */

      if (ClientRect->left != Window->rcClient.left ||
          ClientRect->top != Window->rcClient.top)
      {
         WinPos->flags &= ~SWP_NOCLIENTMOVE;
      }

      if ((ClientRect->right - ClientRect->left !=
            Window->rcClient.right - Window->rcClient.left) ||
            (ClientRect->bottom - ClientRect->top !=
             Window->rcClient.bottom - Window->rcClient.top))
      {
         WinPos->flags &= ~SWP_NOCLIENTSIZE;
      }
   }
   else
   {
      if (! (WinPos->flags & SWP_NOMOVE)
            && (ClientRect->left != Window->rcClient.left ||
                ClientRect->top != Window->rcClient.top))
      {
         WinPos->flags &= ~SWP_NOCLIENTMOVE;
      }
   }

   return wvrFlags;
}

static
BOOL FASTCALL
co_WinPosDoWinPosChanging(PWND Window,
                          PWINDOWPOS WinPos,
                          PRECTL WindowRect,
                          PRECTL ClientRect)
{
   INT X, Y;

   ASSERT_REFS_CO(Window);

   if (!(WinPos->flags & SWP_NOSENDCHANGING))
   {
      co_IntSendMessageNoWait(Window->head.h, WM_WINDOWPOSCHANGING, 0, (LPARAM) WinPos);
   }

   *WindowRect = Window->rcWindow;
   *ClientRect = Window->rcClient;

   if (!(WinPos->flags & SWP_NOSIZE))
   {
      WindowRect->right = WindowRect->left + WinPos->cx;
      WindowRect->bottom = WindowRect->top + WinPos->cy;
   }

   if (!(WinPos->flags & SWP_NOMOVE))
   {
      PWND Parent;
      X = WinPos->x;
      Y = WinPos->y;
      Parent = Window->spwndParent;
      if ((0 != (Window->style & WS_CHILD)) && Parent)
      {
         X += Parent->rcClient.left;
         Y += Parent->rcClient.top;
      }

      WindowRect->left = X;
      WindowRect->top = Y;
      WindowRect->right += X - Window->rcWindow.left;
      WindowRect->bottom += Y - Window->rcWindow.top;
      RECTL_vOffsetRect(ClientRect,
                       X - Window->rcWindow.left,
                       Y - Window->rcWindow.top);
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
WinPosDoOwnedPopups(PWND Window, HWND hWndInsertAfter)
{
   HWND *List = NULL;
   HWND Owner;
   LONG Style;
   PWND DesktopWindow, ChildObject;
   int i;

   Owner = Window->spwndOwner ? Window->spwndOwner->head.h : NULL;
   Style = Window->style;

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
                     if (0 == (ChildObject->ExStyle & WS_EX_TOPMOST))
                     {
                        break;
                     }
                  }
               }
               if (List[i] != Window->head.h)
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
         PWND Wnd;

         if (List[i] == Window->head.h)
            break;

         if (!(Wnd = UserGetWindowObject(List[i])))
            continue;

         if (Wnd->style & WS_POPUP && Wnd->spwndOwner == Window)
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
WinPosInternalMoveWindow(PWND Window, INT MoveX, INT MoveY)
{
   PWND Child;

   ASSERT(Window != Window->spwndChild);

   Window->rcWindow.left += MoveX;
   Window->rcWindow.right += MoveX;
   Window->rcWindow.top += MoveY;
   Window->rcWindow.bottom += MoveY;

   Window->rcClient.left += MoveX;
   Window->rcClient.right += MoveX;
   Window->rcClient.top += MoveY;
   Window->rcClient.bottom += MoveY;

   for(Child = Window->spwndChild; Child; Child = Child->spwndNext)
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
WinPosFixupFlags(WINDOWPOS *WinPos, PWND Wnd)
{
   if (Wnd->style & WS_VISIBLE)
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
   if (Wnd->rcWindow.right - Wnd->rcWindow.left == WinPos->cx &&
       Wnd->rcWindow.bottom - Wnd->rcWindow.top == WinPos->cy)
   {
      WinPos->flags |= SWP_NOSIZE;
   }

   /* Check for right position */
   if (Wnd->rcWindow.left == WinPos->x &&
       Wnd->rcWindow.top == WinPos->y)
   {
      WinPos->flags |= SWP_NOMOVE;
   }

   if (WinPos->hwnd == UserGetForegroundWindow())
   {
      WinPos->flags |= SWP_NOACTIVATE;   /* Already active */
   }
   else
      if ((Wnd->style & (WS_POPUP | WS_CHILD)) != WS_CHILD)
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
         PWND InsAfterWnd;

         InsAfterWnd = UserGetWindowObject(WinPos->hwndInsertAfter);
         if(!InsAfterWnd)
         {
             return TRUE;
         }

         if (InsAfterWnd->spwndParent != Wnd->spwndParent)
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
                ((InsAfterWnd->spwndNext) && (WinPos->hwnd == InsAfterWnd->spwndNext->head.h)))
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
   PWND Window,
   HWND WndInsertAfter,
   INT x,
   INT y,
   INT cx,
   INT cy,
   UINT flags
)
{
   WINDOWPOS WinPos;
   RECTL NewWindowRect;
   RECTL NewClientRect;
   PROSRGNDATA VisRgn;
   HRGN VisBefore = NULL;
   HRGN VisAfter = NULL;
   HRGN DirtyRgn = NULL;
   HRGN ExposedRgn = NULL;
   HRGN CopyRgn = NULL;
   ULONG WvrFlags = 0;
   RECTL OldWindowRect, OldClientRect;
   int RgnType;
   HDC Dc;
   RECTL CopyRect;
   PWND Ancestor;
   BOOL bPointerInWindow;

   ASSERT_REFS_CO(Window);

   /* FIXME: Get current active window from active queue. */
   /*
    * Only allow CSRSS to mess with the desktop window
    */

   if ( Window->head.h == IntGetDesktopWindow() &&
        Window->head.pti->pEThread->ThreadsProcess != PsGetCurrentProcess())
   {
      return FALSE;
   }
   bPointerInWindow = IntPtInWindow(Window, gpsi->ptCursor.x, gpsi->ptCursor.y);

   WinPos.hwnd = Window->head.h;
   WinPos.hwndInsertAfter = WndInsertAfter;
   WinPos.x = x;
   WinPos.y = y;
   WinPos.cx = cx;
   WinPos.cy = cy;
   WinPos.flags = flags;

   co_WinPosDoWinPosChanging(Window, &WinPos, &NewWindowRect, &NewClientRect);

   /* Does the window still exist? */
   if (!IntIsWindow(WinPos.hwnd))
   {
      EngSetLastError(ERROR_INVALID_WINDOW_HANDLE);
      return FALSE;
   }

   /* Fix up the flags. */
   if (!WinPosFixupFlags(&WinPos, Window))
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }

   Ancestor = UserGetAncestor(Window, GA_PARENT);
   if ( (WinPos.flags & (SWP_NOZORDER | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) !=
         SWP_NOZORDER &&
         Ancestor && Ancestor->head.h == IntGetDesktopWindow() )
   {
      WinPos.hwndInsertAfter = WinPosDoOwnedPopups(Window, WinPos.hwndInsertAfter);
   }

   if (!(WinPos.flags & SWP_NOREDRAW))
   {
      /* Compute the visible region before the window position is changed */
      if (!(WinPos.flags & SWP_SHOWWINDOW) &&
           (WinPos.flags & (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                             SWP_HIDEWINDOW | SWP_FRAMECHANGED)) !=
            (SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER))
      {
         VisBefore = VIS_ComputeVisibleRegion(Window, FALSE, FALSE,
                                              (Window->style & WS_CLIPSIBLINGS) ? TRUE : FALSE);
         VisRgn = NULL;

         if ( VisBefore != NULL &&
             (VisRgn = (PROSRGNDATA)RGNOBJAPI_Lock(VisBefore, NULL)) &&
              REGION_Complexity(VisRgn) == NULLREGION )
         {
            RGNOBJAPI_Unlock(VisRgn);
            GreDeleteObject(VisBefore);
            VisBefore = NULL;
         }
         else if(VisRgn)
         {
            RGNOBJAPI_Unlock(VisRgn);
            NtGdiOffsetRgn(VisBefore, -Window->rcWindow.left, -Window->rcWindow.top);
         }
      }
   }

   WvrFlags = co_WinPosDoNCCALCSize(Window, &WinPos, &NewWindowRect, &NewClientRect);

    TRACE("co_WinPosDoNCCALCSize returned %d\n", WvrFlags);

   /* Relink windows. (also take into account shell window in hwndShellWindow) */
   if (!(WinPos.flags & SWP_NOZORDER) && WinPos.hwnd != UserGetShellWindow())
   {
      IntLinkHwnd(Window, WndInsertAfter);
   }

   OldWindowRect = Window->rcWindow;
   OldClientRect = Window->rcClient;

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
       NewClientRect.top  != OldClientRect.top)
   {
      WinPosInternalMoveWindow(Window,
                               NewClientRect.left - OldClientRect.left,
                               NewClientRect.top - OldClientRect.top);
   }

   Window->rcWindow = NewWindowRect;
   Window->rcClient = NewClientRect;

   if (WinPos.flags & SWP_HIDEWINDOW)
   {
      /* Clear the update region */
      co_UserRedrawWindow( Window,
                           NULL,
                           0,
                           RDW_VALIDATE | RDW_NOFRAME | RDW_NOERASE | RDW_NOINTERNALPAINT | RDW_ALLCHILDREN);

      if (Window->spwndParent == UserGetDesktopWindow())
         co_IntShellHookNotify(HSHELL_WINDOWDESTROYED, (LPARAM)Window->head.h);

      Window->style &= ~WS_VISIBLE;
   }
   else if (WinPos.flags & SWP_SHOWWINDOW)
   {
      if (Window->spwndParent == UserGetDesktopWindow())
         co_IntShellHookNotify(HSHELL_WINDOWCREATED, (LPARAM)Window->head.h);

      Window->style |= WS_VISIBLE;
   }

   if (Window->hrgnUpdate != NULL && Window->hrgnUpdate != HRGN_WINDOW)
   {
      NtGdiOffsetRgn(Window->hrgnUpdate,
                     NewWindowRect.left - OldWindowRect.left,
                     NewWindowRect.top - OldWindowRect.top);
   }

   DceResetActiveDCEs(Window);

   if (!(WinPos.flags & SWP_NOREDRAW))
   {
      /* Determine the new visible region */
      VisAfter = VIS_ComputeVisibleRegion(Window, FALSE, FALSE,
                                          (Window->style & WS_CLIPSIBLINGS) ? TRUE : FALSE);
      VisRgn = NULL;

      if ( VisAfter != NULL &&
          (VisRgn = (PROSRGNDATA)RGNOBJAPI_Lock(VisAfter, NULL)) &&
           REGION_Complexity(VisRgn) == NULLREGION )
      {
         RGNOBJAPI_Unlock(VisRgn);
         GreDeleteObject(VisAfter);
         VisAfter = NULL;
      }
      else if(VisRgn)
      {
         RGNOBJAPI_Unlock(VisRgn);
         NtGdiOffsetRgn(VisAfter, -Window->rcWindow.left, -Window->rcWindow.top);
      }

      /*
       * Determine which pixels can be copied from the old window position
       * to the new. Those pixels must be visible in both the old and new
       * position. Also, check the class style to see if the windows of this
       * class need to be completely repainted on (horizontal/vertical) size
       * change.
       */
      if ( VisBefore != NULL &&
           VisAfter != NULL &&
          !(WinPos.flags & SWP_NOCOPYBITS) &&
          ((WinPos.flags & SWP_NOSIZE) || !(WvrFlags & WVR_REDRAW)) &&
          !(Window->ExStyle & WS_EX_TRANSPARENT) )
      {
         CopyRgn = IntSysCreateRectRgn(0, 0, 0, 0);
         RgnType = NtGdiCombineRgn(CopyRgn, VisAfter, VisBefore, RGN_AND);

         /*
          * If this is (also) a window resize, the whole nonclient area
          * needs to be repainted. So we limit the copy to the client area,
          * 'cause there is no use in copying it (would possibly cause
          * "flashing" too). However, if the copy region is already empty,
          * we don't have to crop (can't take anything away from an empty
          * region...)
          */
         if (!(WinPos.flags & SWP_NOSIZE) &&
               RgnType != ERROR &&
               RgnType != NULLREGION )
         {
            PROSRGNDATA pCopyRgn;
            RECTL ORect = OldClientRect;
            RECTL NRect = NewClientRect;
            RECTL_vOffsetRect(&ORect, - OldWindowRect.left, - OldWindowRect.top);
            RECTL_vOffsetRect(&NRect, - NewWindowRect.left, - NewWindowRect.top);
            RECTL_bIntersectRect(&CopyRect, &ORect, &NRect);
            pCopyRgn = RGNOBJAPI_Lock(CopyRgn, NULL);
            REGION_CropAndOffsetRegion(pCopyRgn, pCopyRgn, &CopyRect, NULL);
            RGNOBJAPI_Unlock(pCopyRgn);
         }

         /* No use in copying bits which are in the update region. */
         if (Window->hrgnUpdate != NULL)
         {
            NtGdiOffsetRgn(CopyRgn, NewWindowRect.left, NewWindowRect.top);
            NtGdiCombineRgn(CopyRgn, CopyRgn, Window->hrgnUpdate, RGN_DIFF);
            NtGdiOffsetRgn(CopyRgn, -NewWindowRect.left, -NewWindowRect.top);
         }

         /*
          * Now, get the bounding box of the copy region. If it's empty
          * there's nothing to copy. Also, it's no use copying bits onto
          * themselves.
          */
         if ( (VisRgn = (PROSRGNDATA)RGNOBJAPI_Lock(CopyRgn, NULL)) &&
               REGION_GetRgnBox(VisRgn, &CopyRect) == NULLREGION)
         {
            /* Nothing to copy, clean up */
            RGNOBJAPI_Unlock(VisRgn);
            GreDeleteObject(CopyRgn);
            CopyRgn = NULL;
         }
         else if (OldWindowRect.left != NewWindowRect.left ||
                  OldWindowRect.top != NewWindowRect.top)
         {
            if(VisRgn)
            {
               RGNOBJAPI_Unlock(VisRgn);
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
            Dc = UserGetDCEx( Window,
                              CopyRgn,
                              DCX_WINDOW|DCX_CACHE|DCX_INTERSECTRGN|DCX_CLIPSIBLINGS|DCX_KEEPCLIPRGN);
            NtGdiBitBlt( Dc,
                         CopyRect.left, CopyRect.top,
                         CopyRect.right - CopyRect.left,
                         CopyRect.bottom - CopyRect.top,
                         Dc,
                         CopyRect.left + (OldWindowRect.left - NewWindowRect.left),
                         CopyRect.top + (OldWindowRect.top - NewWindowRect.top),
                         SRCCOPY,
                         0,
                         0);

            UserReleaseDC(Window, Dc, FALSE);
            IntValidateParent(Window, CopyRgn, FALSE);
            NtGdiOffsetRgn(CopyRgn, -NewWindowRect.left, -NewWindowRect.top);
         }
         else if(VisRgn)
         {
            RGNOBJAPI_Unlock(VisRgn);
         }
      }
      else
      {
         CopyRgn = NULL;
      }

      /* We need to redraw what wasn't visible before */
      if (VisAfter != NULL)
      {
         DirtyRgn = IntSysCreateRectRgn(0, 0, 0, 0);
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
            NtGdiOffsetRgn(DirtyRgn, Window->rcWindow.left, Window->rcWindow.top);
            IntInvalidateWindows( Window,
                                  DirtyRgn,
               RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
         }
         GreDeleteObject(DirtyRgn);
         */

            PWND Parent = Window->spwndParent;

            NtGdiOffsetRgn( DirtyRgn,
                            Window->rcWindow.left,
                            Window->rcWindow.top);
            if ( (Window->style & WS_CHILD) &&
                 (Parent) &&
                !(Parent->style & WS_CLIPCHILDREN))
            {
               IntInvalidateWindows( Parent,
                                     DirtyRgn,
                                     RDW_ERASE | RDW_INVALIDATE);
               co_IntPaintWindows(Parent, RDW_ERASENOW, FALSE);
            }
            else
            {
                IntInvalidateWindows( Window,
                                      DirtyRgn,
                    RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
            }
         }
         GreDeleteObject(DirtyRgn);
      }

      if (CopyRgn != NULL)
      {
         GreDeleteObject(CopyRgn);
      }

      /* Expose what was covered before but not covered anymore */
      if (VisBefore != NULL)
      {
         ExposedRgn = IntSysCreateRectRgn(0, 0, 0, 0);
         RgnType = NtGdiCombineRgn(ExposedRgn, VisBefore, NULL, RGN_COPY);
         NtGdiOffsetRgn( ExposedRgn,
                         OldWindowRect.left - NewWindowRect.left,
                         OldWindowRect.top  - NewWindowRect.top);

         if (VisAfter != NULL)
            RgnType = NtGdiCombineRgn(ExposedRgn, ExposedRgn, VisAfter, RGN_DIFF);

         if (RgnType != ERROR && RgnType != NULLREGION)
         {
            co_VIS_WindowLayoutChanged(Window, ExposedRgn);
         }
         GreDeleteObject(ExposedRgn);
         GreDeleteObject(VisBefore);
      }

      if (VisAfter != NULL)
      {
         GreDeleteObject(VisAfter);
      }

      if (!(WinPos.flags & SWP_NOACTIVATE))
      {
         if ((Window->style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
         {
            co_IntSendMessageNoWait(WinPos.hwnd, WM_CHILDACTIVATE, 0, 0);
         }
         else
         {
            co_IntSetForegroundWindow(Window);
         }
      }
   }

   if ((WinPos.flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE)
   {
      /* WM_WINDOWPOSCHANGED is sent even if SWP_NOSENDCHANGING is set
         and always contains final window position.
       */
      WinPos.x = NewWindowRect.left;
      WinPos.y = NewWindowRect.top;
      WinPos.cx = NewWindowRect.right - NewWindowRect.left;
      WinPos.cy = NewWindowRect.bottom - NewWindowRect.top;
      co_IntSendMessageNoWait(WinPos.hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM) &WinPos);
   }

   if ( WinPos.flags & SWP_FRAMECHANGED  || WinPos.flags & SWP_STATECHANGED ||
      !(WinPos.flags & SWP_NOCLIENTSIZE) || !(WinPos.flags & SWP_NOCLIENTMOVE) )
   {
      PWND pWnd = UserGetWindowObject(WinPos.hwnd);
      if (pWnd)
         IntNotifyWinEvent(EVENT_OBJECT_LOCATIONCHANGE, pWnd, OBJID_WINDOW, CHILDID_SELF, WEF_SETBYWNDPTI);
   }

   if(bPointerInWindow != IntPtInWindow(Window, gpsi->ptCursor.x, gpsi->ptCursor.y))
   {
      /* Generate mouse move message */
      MSG msg;
      msg.message = WM_MOUSEMOVE;
      msg.wParam = IntGetSysCursorInfo()->ButtonsDown;
      msg.lParam = MAKELPARAM(gpsi->ptCursor.x, gpsi->ptCursor.y);
      msg.pt = gpsi->ptCursor;
      co_MsqInsertMouseMessage(&msg, 0, 0, TRUE);
   }

   return TRUE;
}

LRESULT FASTCALL
co_WinPosGetNonClientSize(PWND Window, RECT* WindowRect, RECT* ClientRect)
{
   LRESULT Result;

   ASSERT_REFS_CO(Window);

   *ClientRect = *WindowRect;
   Result = co_IntSendMessageNoWait(Window->head.h, WM_NCCALCSIZE, FALSE, (LPARAM) ClientRect);

   FixClientRect(ClientRect, WindowRect);

   return Result;
}

void FASTCALL
co_WinPosSendSizeMove(PWND Wnd)
{
    WPARAM wParam = SIZE_RESTORED;

    Wnd->state &= ~WNDS_SENDSIZEMOVEMSGS;
    if (Wnd->style & WS_MAXIMIZE)
    {
        wParam = SIZE_MAXIMIZED;
    }
    else if (Wnd->style & WS_MINIMIZE)
    {
        wParam = SIZE_MINIMIZED;
    }

    co_IntSendMessageNoWait(Wnd->head.h, WM_SIZE, wParam,
                        MAKELONG(Wnd->rcClient.right -
                                 Wnd->rcClient.left,
                                 Wnd->rcClient.bottom -
                                 Wnd->rcClient.top));
    co_IntSendMessageNoWait(Wnd->head.h, WM_MOVE, 0,
                        MAKELONG(Wnd->rcClient.left,
                                 Wnd->rcClient.top));
    IntEngWindowChanged(Wnd, WOC_RGN_CLIENT);
}

BOOLEAN FASTCALL
co_WinPosShowWindow(PWND Wnd, INT Cmd)
{
   BOOLEAN WasVisible;
   UINT Swp = 0;
   RECTL NewPos;
   BOOLEAN ShowFlag;
   //  HRGN VisibleRgn;

   ASSERT_REFS_CO(Wnd);

   WasVisible = (Wnd->style & WS_VISIBLE) != 0;

   switch (Cmd)
   {
      case SW_HIDE:
         {
            if (!WasVisible)
            {
               return(FALSE);
            }
            Swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE;
            if (Wnd->head.h != UserGetActiveWindow())
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
            if (!(Wnd->style & WS_MINIMIZE))
            {
               Swp |= co_WinPosMinMaximize(Wnd, SW_MINIMIZE, &NewPos) |
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
            if (!(Wnd->style & WS_MAXIMIZE))
            {
               Swp |= co_WinPosMinMaximize(Wnd, SW_MAXIMIZE, &NewPos) |
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
         if (Wnd->style & (WS_MINIMIZE | WS_MAXIMIZE))
         {
            Swp |= co_WinPosMinMaximize(Wnd, SW_RESTORE, &NewPos) |
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
      co_IntSendMessageNoWait(Wnd->head.h, WM_SHOWWINDOW, ShowFlag, 0);
   }

   /* We can't activate a child window */
   if ((Wnd->style & WS_CHILD) &&
       !(Wnd->ExStyle & WS_EX_MDICHILD))
   {
      Swp |= SWP_NOACTIVATE | SWP_NOZORDER;
   }

   co_WinPosSetWindowPos(Wnd, 0 != (Wnd->ExStyle & WS_EX_TOPMOST)
                         ? HWND_TOPMOST : HWND_TOP,
                         NewPos.left, NewPos.top, NewPos.right, NewPos.bottom, LOWORD(Swp));

   if ((Cmd == SW_HIDE) || (Cmd == SW_MINIMIZE))
   {
      PWND ThreadFocusWindow;

      /* FIXME: This will cause the window to be activated irrespective
       * of whether it is owned by the same thread. Has to be done
       * asynchronously.
       */

      if (Wnd->head.h == UserGetActiveWindow())
      {
         co_WinPosActivateOtherWindow(Wnd);
      }


      //temphack
      ThreadFocusWindow = UserGetWindowObject(IntGetThreadFocusWindow());

      /* Revert focus to parent */
      if (ThreadFocusWindow && (Wnd == ThreadFocusWindow ||
            IntIsChildWindow(Wnd, ThreadFocusWindow)))
      {
         //faxme: as long as we have ref on Window, we also, indirectly, have ref on parent...
         co_UserSetFocus(Wnd->spwndParent);
      }
   }

   /* FIXME: Check for window destruction. */

   if ((Wnd->state & WNDS_SENDSIZEMOVEMSGS) &&
       !(Wnd->state2 & WNDS2_INDESTROY))
   {
        co_WinPosSendSizeMove(Wnd);
   }

   /* Activate the window if activation is not requested and the window is not minimized */
   /*
     if (!(Swp & (SWP_NOACTIVATE | SWP_HIDEWINDOW)) && !(Window->style & WS_MINIMIZE))
       {
         WinPosChangeActiveWindow(Wnd, FALSE);
       }
   */
   return(WasVisible);
}

static
PWND FASTCALL
co_WinPosSearchChildren(
   PWND ScopeWin,
   POINT *Point,
   USHORT *HitTest
   )
{
    PWND pwndChild;
    HWND *List, *phWnd;

    if (!(ScopeWin->style & WS_VISIBLE))
    {
        return NULL;
    }

    if ((ScopeWin->style & WS_DISABLED))
    {
        return NULL;
    }

    if (!IntPtInWindow(ScopeWin, Point->x, Point->y))
    {
        return NULL;
    }

    UserReferenceObject(ScopeWin);

    if (Point->x - ScopeWin->rcClient.left < ScopeWin->rcClient.right &&
        Point->y - ScopeWin->rcClient.top < ScopeWin->rcClient.bottom )
    {
        List = IntWinListChildren(ScopeWin);
        if(List)
        {
            for (phWnd = List; *phWnd; ++phWnd)
            {
                if (!(pwndChild = UserGetWindowObject(*phWnd)))
                {
                    continue;
                }

                pwndChild = co_WinPosSearchChildren(pwndChild, Point, HitTest);

                if(pwndChild != NULL)
                {
                    /* We found a window. Don't send any more WM_NCHITTEST messages */
                    ExFreePool(List);
                    UserDereferenceObject(ScopeWin);
                    return pwndChild;
                }
            }
            ExFreePool(List);
        }
    }

    *HitTest = co_IntSendMessage(ScopeWin->head.h, WM_NCHITTEST, 0,
                                 MAKELONG(Point->x, Point->y));
    if ((*HitTest) == (USHORT)HTTRANSPARENT)
    {
         UserDereferenceObject(ScopeWin);
         return NULL;
    }

    return ScopeWin;
}

PWND FASTCALL
co_WinPosWindowFromPoint(PWND ScopeWin, POINT *WinPoint, USHORT* HitTest)
{
   PWND Window;
   POINT Point = *WinPoint;
   USER_REFERENCE_ENTRY Ref;

   if( ScopeWin == NULL )
   {
       ScopeWin = UserGetDesktopWindow();
       if(ScopeWin == NULL)
           return NULL;
   }

   *HitTest = HTNOWHERE;

   ASSERT_REFS_CO(ScopeWin);
   UserRefObjectCo(ScopeWin, &Ref);

   Window = co_WinPosSearchChildren(ScopeWin, &Point, HitTest);

   UserDerefObjectCo(ScopeWin);
   if(Window)
       ASSERT_REFS_CO(Window);
   ASSERT_REFS_CO(ScopeWin);

   return Window;
}

HDWP
FASTCALL
IntDeferWindowPos( HDWP hdwp,
                   HWND hwnd,
                   HWND hwndAfter,
                   INT x,
                   INT y,
                   INT cx,
                   INT cy,
                   UINT flags )
{
    PSMWP pDWP;
    int i;
    HDWP retvalue = hdwp;

    TRACE("hdwp %p, hwnd %p, after %p, %d,%d (%dx%d), flags %08x\n",
          hdwp, hwnd, hwndAfter, x, y, cx, cy, flags);

    if (flags & ~(SWP_NOSIZE | SWP_NOMOVE |
                  SWP_NOZORDER | SWP_NOREDRAW |
                  SWP_NOACTIVATE | SWP_NOCOPYBITS |
                  SWP_NOOWNERZORDER|SWP_SHOWWINDOW |
                  SWP_HIDEWINDOW | SWP_FRAMECHANGED))
    {
       EngSetLastError(ERROR_INVALID_PARAMETER);
       return NULL;
    }

    if (!(pDWP = (PSMWP)UserGetObject(gHandleTable, hdwp, otSMWP)))
    {
       EngSetLastError(ERROR_INVALID_DWP_HANDLE);
       return NULL;
    }

    for (i = 0; i < pDWP->ccvr; i++)
    {
        if (pDWP->acvr[i].pos.hwnd == hwnd)
        {
              /* Merge with the other changes */
            if (!(flags & SWP_NOZORDER))
            {
                pDWP->acvr[i].pos.hwndInsertAfter = hwndAfter;
            }
            if (!(flags & SWP_NOMOVE))
            {
                pDWP->acvr[i].pos.x = x;
                pDWP->acvr[i].pos.y = y;
            }
            if (!(flags & SWP_NOSIZE))
            {
                pDWP->acvr[i].pos.cx = cx;
                pDWP->acvr[i].pos.cy = cy;
            }
            pDWP->acvr[i].pos.flags &= flags | ~(SWP_NOSIZE | SWP_NOMOVE |
                                               SWP_NOZORDER | SWP_NOREDRAW |
                                               SWP_NOACTIVATE | SWP_NOCOPYBITS|
                                               SWP_NOOWNERZORDER);
            pDWP->acvr[i].pos.flags |= flags & (SWP_SHOWWINDOW | SWP_HIDEWINDOW |
                                              SWP_FRAMECHANGED);
            goto END;
        }
    }
    if (pDWP->ccvr >= pDWP->ccvrAlloc)
    {
        PCVR newpos = ExAllocatePoolWithTag(PagedPool, pDWP->ccvrAlloc * 2 * sizeof(CVR), USERTAG_SWP);
        if (!newpos)
        {
            retvalue = NULL;
            goto END;
        }
        RtlZeroMemory(newpos, pDWP->ccvrAlloc * 2 * sizeof(CVR));
        RtlCopyMemory(newpos, pDWP->acvr, pDWP->ccvrAlloc * sizeof(CVR));
        ExFreePoolWithTag(pDWP->acvr, USERTAG_SWP);
        pDWP->ccvrAlloc *= 2;
        pDWP->acvr = newpos;
    }
    pDWP->acvr[pDWP->ccvr].pos.hwnd = hwnd;
    pDWP->acvr[pDWP->ccvr].pos.hwndInsertAfter = hwndAfter;
    pDWP->acvr[pDWP->ccvr].pos.x = x;
    pDWP->acvr[pDWP->ccvr].pos.y = y;
    pDWP->acvr[pDWP->ccvr].pos.cx = cx;
    pDWP->acvr[pDWP->ccvr].pos.cy = cy;
    pDWP->acvr[pDWP->ccvr].pos.flags = flags;
    pDWP->acvr[pDWP->ccvr].hrgnClip = NULL;
    pDWP->acvr[pDWP->ccvr].hrgnInterMonitor = NULL;
    pDWP->ccvr++;
END:
    return retvalue;
}

BOOL FASTCALL IntEndDeferWindowPosEx( HDWP hdwp )
{
    PSMWP pDWP;
    PCVR winpos;
    BOOL res = TRUE;
    int i;

    TRACE("%p\n", hdwp);

    if (!(pDWP = (PSMWP)UserGetObject(gHandleTable, hdwp, otSMWP)))
    {
       EngSetLastError(ERROR_INVALID_DWP_HANDLE);
       return FALSE;
    }

    for (i = 0, winpos = pDWP->acvr; res && i < pDWP->ccvr; i++, winpos++)
    {
        PWND pwnd;
        USER_REFERENCE_ENTRY Ref;

        TRACE("hwnd %p, after %p, %d,%d (%dx%d), flags %08x\n",
               winpos->pos.hwnd, winpos->pos.hwndInsertAfter, winpos->pos.x, winpos->pos.y,
               winpos->pos.cx, winpos->pos.cy, winpos->pos.flags);
        
        pwnd = UserGetWindowObject(winpos->pos.hwnd);
        if(!pwnd)
            continue;

        UserRefObjectCo(pwnd, &Ref);

        res = co_WinPosSetWindowPos( pwnd,
                                     winpos->pos.hwndInsertAfter,
                                     winpos->pos.x,
                                     winpos->pos.y,
                                     winpos->pos.cx,
                                     winpos->pos.cy,
                                     winpos->pos.flags);

        UserDerefObjectCo(pwnd);
    }
    ExFreePoolWithTag(pDWP->acvr, USERTAG_SWP);
    UserDereferenceObject(pDWP);
    UserDeleteObject(hdwp, otSMWP);
    return res;
}

/*
 * @implemented
 */
HWND APIENTRY
NtUserChildWindowFromPointEx(HWND hwndParent,
                             LONG x,
                             LONG y,
                             UINT uiFlags)
{
   PWND Parent;
   POINTL Pt;
   HWND Ret;
   HWND *List, *phWnd;

   if(!(Parent = UserGetWindowObject(hwndParent)))
   {
      return NULL;
   }

   Pt.x = x;
   Pt.y = y;

   if(Parent->head.h != IntGetDesktopWindow())
   {
      Pt.x += Parent->rcClient.left;
      Pt.y += Parent->rcClient.top;
   }

   if(!IntPtInWindow(Parent, Pt.x, Pt.y))
   {
      return NULL;
   }

   Ret = Parent->head.h;
   if((List = IntWinListChildren(Parent)))
   {
      for(phWnd = List; *phWnd; phWnd++)
      {
         PWND Child;
         if((Child = UserGetWindowObject(*phWnd)))
         {
            if(!(Child->style & WS_VISIBLE) && (uiFlags & CWP_SKIPINVISIBLE))
            {
               continue;
            }
            if((Child->style & WS_DISABLED) && (uiFlags & CWP_SKIPDISABLED))
            {
               continue;
            }
            if((Child->ExStyle & WS_EX_TRANSPARENT) && (uiFlags & CWP_SKIPTRANSPARENT))
            {
               continue;
            }
            if(IntPtInWindow(Child, Pt.x, Pt.y))
            {
               Ret = Child->head.h;
               break;
            }
         }
      }
      ExFreePool(List);
   }

   return Ret;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserEndDeferWindowPosEx(HDWP WinPosInfo,
                          DWORD Unknown1)
{
   BOOL Ret;
   TRACE("Enter NtUserEndDeferWindowPosEx\n");
   UserEnterExclusive();
   Ret = IntEndDeferWindowPosEx(WinPosInfo);
   TRACE("Leave NtUserEndDeferWindowPosEx, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

/*
 * @implemented
 */
HDWP APIENTRY
NtUserDeferWindowPos(HDWP WinPosInfo,
                     HWND Wnd,
                     HWND WndInsertAfter,
                     int x,
                     int y,
                     int cx,
                     int cy,
                     UINT Flags)
{
   PWND pWnd, pWndIA;
   HDWP Ret = NULL;
   UINT Tmp = ~(SWP_ASYNCWINDOWPOS|SWP_DEFERERASE|SWP_NOSENDCHANGING|SWP_NOREPOSITION|
                SWP_NOCOPYBITS|SWP_HIDEWINDOW|SWP_SHOWWINDOW|SWP_FRAMECHANGED|
                SWP_NOACTIVATE|SWP_NOREDRAW|SWP_NOZORDER|SWP_NOMOVE|SWP_NOSIZE);

   TRACE("Enter NtUserDeferWindowPos\n");
   UserEnterExclusive();

   if ( Flags & Tmp )
   {
      EngSetLastError(ERROR_INVALID_FLAGS);
      goto Exit;
   }

   pWnd = UserGetWindowObject(Wnd);
   if ( !pWnd ||                          // FIXME:
         pWnd == IntGetDesktopWindow() || // pWnd->fnid == FNID_DESKTOP
         pWnd == IntGetMessageWindow() )  // pWnd->fnid == FNID_MESSAGEWND
   {
      goto Exit;
   }

   if ( WndInsertAfter &&
        WndInsertAfter != HWND_BOTTOM &&
        WndInsertAfter != HWND_TOPMOST &&
        WndInsertAfter != HWND_NOTOPMOST )
   {
      pWndIA = UserGetWindowObject(WndInsertAfter);
      if ( !pWndIA ||
            pWndIA == IntGetDesktopWindow() ||
            pWndIA == IntGetMessageWindow() )
      {
         goto Exit;
      }
   }

   Ret = IntDeferWindowPos(WinPosInfo, Wnd, WndInsertAfter, x, y, cx, cy, Flags);

Exit:
   TRACE("Leave NtUserDeferWindowPos, ret=%i\n", Ret);
   UserLeave();
   return Ret;
}

DWORD
APIENTRY
NtUserMinMaximize(
    HWND hWnd,
    UINT cmd, // Wine SW_ commands
    BOOL Hide)
{
  RECTL NewPos;
  UINT SwFlags;
  PWND pWnd;

  TRACE("Enter NtUserMinMaximize\n");
  UserEnterExclusive();

  pWnd = UserGetWindowObject(hWnd);
  if ( !pWnd ||                          // FIXME:
        pWnd == IntGetDesktopWindow() || // pWnd->fnid == FNID_DESKTOP
        pWnd == IntGetMessageWindow() )  // pWnd->fnid == FNID_MESSAGEWND
  {
     goto Exit;
  }

  if ( cmd > SW_MAX || pWnd->state2 & WNDS2_INDESTROY)
  {
     EngSetLastError(ERROR_INVALID_PARAMETER);
     goto Exit;
  }

  co_WinPosMinMaximize(pWnd, cmd, &NewPos);

  SwFlags = Hide ? SWP_NOACTIVATE|SWP_NOZORDER|SWP_FRAMECHANGED : SWP_NOZORDER|SWP_FRAMECHANGED;

  co_WinPosSetWindowPos( pWnd,
                         NULL,
                         NewPos.left,
                         NewPos.top,
                         NewPos.right,
                         NewPos.bottom,
                         SwFlags);

  co_WinPosShowWindow(pWnd, cmd);

Exit:
  TRACE("Leave NtUserMinMaximize\n");
  UserLeave();
  return 0; // Always NULL?
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserMoveWindow(
   HWND hWnd,
   int X,
   int Y,
   int nWidth,
   int nHeight,
   BOOL bRepaint)
{
   return NtUserSetWindowPos(hWnd, 0, X, Y, nWidth, nHeight,
                             (bRepaint ? SWP_NOZORDER | SWP_NOACTIVATE :
                              SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOREDRAW));
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetWindowPos(
   HWND hWnd,
   HWND hWndInsertAfter,
   int X,
   int Y,
   int cx,
   int cy,
   UINT uFlags)
{
   DECLARE_RETURN(BOOL);
   PWND Window, pWndIA;
   BOOL ret;
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserSetWindowPos\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)) || // FIXME:
         Window == IntGetDesktopWindow() ||     // pWnd->fnid == FNID_DESKTOP
         Window == IntGetMessageWindow() )      // pWnd->fnid == FNID_MESSAGEWND
   {
      RETURN(FALSE);
   }

   if ( hWndInsertAfter &&
        hWndInsertAfter != HWND_BOTTOM &&
        hWndInsertAfter != HWND_TOPMOST &&
        hWndInsertAfter != HWND_NOTOPMOST )
   {
      pWndIA = UserGetWindowObject(hWndInsertAfter);
      if ( !pWndIA ||
            pWndIA == IntGetDesktopWindow() ||
            pWndIA == IntGetMessageWindow() )
      {
         RETURN(FALSE);
      }
   }

   /* First make sure that coordinates are valid for WM_WINDOWPOSCHANGING */
   if (!(uFlags & SWP_NOMOVE))
   {
      if (X < -32768) X = -32768;
      else if (X > 32767) X = 32767;
      if (Y < -32768) Y = -32768;
      else if (Y > 32767) Y = 32767;
   }
   if (!(uFlags & SWP_NOSIZE))
   {
      if (cx < 0) cx = 0;
      else if (cx > 32767) cx = 32767;
      if (cy < 0) cy = 0;
      else if (cy > 32767) cy = 32767;
   }

   UserRefObjectCo(Window, &Ref);
   ret = co_WinPosSetWindowPos(Window, hWndInsertAfter, X, Y, cx, cy, uFlags);
   UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   TRACE("Leave NtUserSetWindowPos, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
INT APIENTRY
NtUserSetWindowRgn(
   HWND hWnd,
   HRGN hRgn,
   BOOL bRedraw)
{
   HRGN hrgnCopy;
   PWND Window;
   INT flags = (SWP_NOCLIENTSIZE|SWP_NOCLIENTMOVE|SWP_NOACTIVATE|SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOMOVE);
   BOOLEAN Ret = FALSE;
   DECLARE_RETURN(INT);

   TRACE("Enter NtUserSetWindowRgn\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)) || // FIXME:
         Window == IntGetDesktopWindow() ||     // pWnd->fnid == FNID_DESKTOP
         Window == IntGetMessageWindow() )      // pWnd->fnid == FNID_MESSAGEWND
   {
      RETURN( 0);
   }

   if (hRgn) // The region will be deleted in user32.
   {
      if (GreIsHandleValid(hRgn))
      {
         hrgnCopy = IntSysCreateRectRgn(0, 0, 0, 0);

         NtGdiCombineRgn(hrgnCopy, hRgn, 0, RGN_COPY);
      }
      else
         RETURN( 0);
   }
   else
   {
      hrgnCopy = NULL;
   }

   /* Delete the region passed by the caller */
   GreDeleteObject(hRgn);

   if (Window->hrgnClip)
   {
      /* Delete no longer needed region handle */
      IntGdiSetRegionOwner(Window->hrgnClip, GDI_OBJ_HMGR_POWNED);
      GreDeleteObject(Window->hrgnClip);
   }

   if (hrgnCopy)
   {
      /* Set public ownership */
      IntGdiSetRegionOwner(hrgnCopy, GDI_OBJ_HMGR_PUBLIC);
   }
   Window->hrgnClip = hrgnCopy;

   Ret = co_WinPosSetWindowPos(Window, HWND_TOP, 0, 0, 0, 0, bRedraw ? flags : (flags|SWP_NOREDRAW) );

   RETURN( (INT)Ret);

CLEANUP:
   TRACE("Leave NtUserSetWindowRgn, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserSetWindowPlacement(HWND hWnd,
                         WINDOWPLACEMENT *lpwndpl)
{
   PWND Wnd;
   WINDOWPLACEMENT Safepl;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserSetWindowPlacement\n");
   UserEnterExclusive();

   if (!(Wnd = UserGetWindowObject(hWnd)) || // FIXME:
         Wnd == IntGetDesktopWindow() ||     // pWnd->fnid == FNID_DESKTOP
         Wnd == IntGetMessageWindow() )      // pWnd->fnid == FNID_MESSAGEWND
   {
      RETURN( FALSE);
   }

   _SEH2_TRY
   {
      ProbeForRead(lpwndpl, sizeof(WINDOWPLACEMENT), 1);
      RtlCopyMemory(&Safepl, lpwndpl, sizeof(WINDOWPLACEMENT));
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      SetLastNtError(_SEH2_GetExceptionCode());
      _SEH2_YIELD(RETURN( FALSE));
   }
   _SEH2_END

   if(Safepl.length != sizeof(WINDOWPLACEMENT))
   {
      RETURN( FALSE);
   }

   UserRefObjectCo(Wnd, &Ref);

   if ((Wnd->style & (WS_MAXIMIZE | WS_MINIMIZE)) == 0)
   {
      co_WinPosSetWindowPos(Wnd, NULL,
                            Safepl.rcNormalPosition.left, Safepl.rcNormalPosition.top,
                            Safepl.rcNormalPosition.right - Safepl.rcNormalPosition.left,
                            Safepl.rcNormalPosition.bottom - Safepl.rcNormalPosition.top,
                            SWP_NOZORDER | SWP_NOACTIVATE);
   }

   /* FIXME - change window status */
   co_WinPosShowWindow(Wnd, Safepl.showCmd);

   Wnd->InternalPosInitialized = TRUE;
   Wnd->InternalPos.NormalRect = Safepl.rcNormalPosition;
   Wnd->InternalPos.IconPos = Safepl.ptMinPosition;
   Wnd->InternalPos.MaxPos = Safepl.ptMaxPosition;

   UserDerefObjectCo(Wnd);
   RETURN(TRUE);

CLEANUP:
   TRACE("Leave NtUserSetWindowPlacement, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @unimplemented
 */
BOOL APIENTRY
NtUserShowWindowAsync(HWND hWnd, LONG nCmdShow)
{
#if 0
   STUB
   return 0;
#else
   return NtUserShowWindow(hWnd, nCmdShow);
#endif
}

/*
 * @implemented
 */
BOOL APIENTRY
NtUserShowWindow(HWND hWnd, LONG nCmdShow)
{
   PWND Window;
   BOOL ret;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserShowWindow\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)) || // FIXME:
         Window == IntGetDesktopWindow() ||     // pWnd->fnid == FNID_DESKTOP
         Window == IntGetMessageWindow() )      // pWnd->fnid == FNID_MESSAGEWND
   {
      RETURN(FALSE);
   }

   if ( nCmdShow > SW_MAX || Window->state2 & WNDS2_INDESTROY)
   {
      EngSetLastError(ERROR_INVALID_PARAMETER);
      RETURN(FALSE);
   }

   UserRefObjectCo(Window, &Ref);
   ret = co_WinPosShowWindow(Window, nCmdShow);
   UserDerefObjectCo(Window);

   RETURN(ret);

CLEANUP:
   TRACE("Leave NtUserShowWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

//// Ugly NtUser API ////
BOOL
APIENTRY
NtUserGetMinMaxInfo(
   HWND hWnd,
   MINMAXINFO *MinMaxInfo,
   BOOL SendMessage)
{
   POINT Size;
   PWND Window = NULL;
   MINMAXINFO SafeMinMax;
   NTSTATUS Status;
   BOOL ret;
   USER_REFERENCE_ENTRY Ref;

   TRACE("Enter NtUserGetMinMaxInfo\n");
   UserEnterExclusive();

   if(!(Window = UserGetWindowObject(hWnd)))
   {
      ret = FALSE;
      goto cleanup;
   }

   UserRefObjectCo(Window, &Ref);

   Size.x = Window->rcWindow.left;
   Size.y = Window->rcWindow.top;
   WinPosInitInternalPos(Window, &Size,
                         &Window->rcWindow);

   co_WinPosGetMinMaxInfo(Window, &SafeMinMax.ptMaxSize, &SafeMinMax.ptMaxPosition,
                          &SafeMinMax.ptMinTrackSize, &SafeMinMax.ptMaxTrackSize);

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

   TRACE("Leave NtUserGetMinMaxInfo, ret=%i\n", ret);
   UserLeave();
   return ret;
}

/* EOF */
