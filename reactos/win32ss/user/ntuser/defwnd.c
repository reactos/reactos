/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Miscellaneous User functions
 * FILE:             subsystems/win32/win32k/ntuser/defwnd.c
 * PROGRAMER:
 */

#include <win32k.h>
#include <windowsx.h>

DBG_DEFAULT_CHANNEL(UserDefwnd);

#define UserHasDlgFrameStyle(Style, ExStyle)                                   \
 (((ExStyle) & WS_EX_DLGMODALFRAME) ||                                         \
  (((Style) & WS_DLGFRAME) && (!((Style) & WS_THICKFRAME))))

#define UserHasThickFrameStyle(Style, ExStyle)                                 \
  (((Style) & WS_THICKFRAME) &&                                                \
   (!(((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)))

#define UserHasThinFrameStyle(Style, ExStyle)                                  \
  (((Style) & WS_BORDER) || (!((Style) & (WS_CHILD | WS_POPUP))))

#define ON_LEFT_BORDER(hit) \
 (((hit) == HTLEFT) || ((hit) == HTTOPLEFT) || ((hit) == HTBOTTOMLEFT))
#define ON_RIGHT_BORDER(hit) \
 (((hit) == HTRIGHT) || ((hit) == HTTOPRIGHT) || ((hit) == HTBOTTOMRIGHT))
#define ON_TOP_BORDER(hit) \
 (((hit) == HTTOP) || ((hit) == HTTOPLEFT) || ((hit) == HTTOPRIGHT))
#define ON_BOTTOM_BORDER(hit) \
 (((hit) == HTBOTTOM) || ((hit) == HTBOTTOMLEFT) || ((hit) == HTBOTTOMRIGHT))

HBRUSH FASTCALL
DefWndControlColor(HDC hDC, UINT ctlType)
{
  if (ctlType == CTLCOLOR_SCROLLBAR)
  {
      HBRUSH hb = IntGetSysColorBrush(COLOR_SCROLLBAR);
      COLORREF bk = IntGetSysColor(COLOR_3DHILIGHT);
      IntGdiSetTextColor(hDC, IntGetSysColor(COLOR_3DFACE));
      IntGdiSetBkColor(hDC, bk);

      /* if COLOR_WINDOW happens to be the same as COLOR_3DHILIGHT
       * we better use 0x55aa bitmap brush to make scrollbar's background
       * look different from the window background.
       */
      if ( bk == IntGetSysColor(COLOR_WINDOW))
          return gpsi->hbrGray;

      NtGdiUnrealizeObject( hb );
      return hb;
  }

  IntGdiSetTextColor(hDC, IntGetSysColor(COLOR_WINDOWTEXT));

  if ((ctlType == CTLCOLOR_EDIT) || (ctlType == CTLCOLOR_LISTBOX))
  {
      IntGdiSetBkColor(hDC, IntGetSysColor(COLOR_WINDOW));
  }
  else
  {
      IntGdiSetBkColor(hDC, IntGetSysColor(COLOR_3DFACE));
      return IntGetSysColorBrush(COLOR_3DFACE);
  }

  return IntGetSysColorBrush(COLOR_WINDOW);
}

LRESULT FASTCALL
DefWndHandleWindowPosChanging(PWND pWnd, WINDOWPOS* Pos)
{
    POINT maxTrack, minTrack;
    LONG style = pWnd->style;

    if (Pos->flags & SWP_NOSIZE) return 0;
    if ((style & WS_THICKFRAME) || ((style & (WS_POPUP | WS_CHILD)) == 0))
    {
        co_WinPosGetMinMaxInfo(pWnd, NULL, NULL, &minTrack, &maxTrack);
        Pos->cx = min(Pos->cx, maxTrack.x);
        Pos->cy = min(Pos->cy, maxTrack.y);
        if (!(style & WS_MINIMIZE))
        {
            if (Pos->cx < minTrack.x) Pos->cx = minTrack.x;
            if (Pos->cy < minTrack.y) Pos->cy = minTrack.y;
        }
    }
    else
    {
        Pos->cx = max(Pos->cx, 0);
        Pos->cy = max(Pos->cy, 0);
    }
    return 0;
}

LRESULT FASTCALL
DefWndHandleWindowPosChanged(PWND pWnd, WINDOWPOS* Pos)
{
   RECT Rect;
   LONG style = pWnd->style;

   IntGetClientRect(pWnd, &Rect);
   IntMapWindowPoints(pWnd, (style & WS_CHILD ? IntGetParent(pWnd) : NULL), (LPPOINT) &Rect, 2);

   if (!(Pos->flags & SWP_NOCLIENTMOVE))
   {
      co_IntSendMessage(UserHMGetHandle(pWnd), WM_MOVE, 0, MAKELONG(Rect.left, Rect.top));
   }

   if (!(Pos->flags & SWP_NOCLIENTSIZE) || (Pos->flags & SWP_STATECHANGED))
   {
      if (style & WS_MINIMIZE) co_IntSendMessage(UserHMGetHandle(pWnd), WM_SIZE, SIZE_MINIMIZED, 0 );
      else
      {
         WPARAM wp = (style & WS_MAXIMIZE) ? SIZE_MAXIMIZED : SIZE_RESTORED;
         co_IntSendMessage(UserHMGetHandle(pWnd), WM_SIZE, wp, MAKELONG(Rect.right - Rect.left, Rect.bottom - Rect.top));
      }
   }
   return 0;
}

VOID FASTCALL
UserDrawWindowFrame(HDC hdc,
                    RECTL *rect,
		    ULONG width,
		    ULONG height)
{
   HBRUSH hbrush = NtGdiSelectBrush( hdc, gpsi->hbrGray );
   NtGdiPatBlt( hdc, rect->left, rect->top, rect->right - rect->left - width, height, PATINVERT );
   NtGdiPatBlt( hdc, rect->left, rect->top + height, width, rect->bottom - rect->top - height, PATINVERT );
   NtGdiPatBlt( hdc, rect->left + width, rect->bottom - 1, rect->right - rect->left - width, -(LONG)height, PATINVERT );
   NtGdiPatBlt( hdc, rect->right - 1, rect->top, -(LONG)width, rect->bottom - rect->top - height, PATINVERT );
   NtGdiSelectBrush( hdc, hbrush );
}

VOID FASTCALL
UserDrawMovingFrame(HDC hdc,
                    RECTL *rect,
                    BOOL thickframe)
{
   if (thickframe) UserDrawWindowFrame(hdc, rect, UserGetSystemMetrics(SM_CXFRAME), UserGetSystemMetrics(SM_CYFRAME));
   else UserDrawWindowFrame(hdc, rect, 1, 1);
}

/***********************************************************************
 *           NC_GetInsideRect
 *
 * Get the 'inside' rectangle of a window, i.e. the whole window rectangle
 * but without the borders (if any).
 */
void FASTCALL
NC_GetInsideRect(PWND Wnd, RECT *rect)
{
    ULONG Style;
    ULONG ExStyle;

    Style = Wnd->style;
    ExStyle = Wnd->ExStyle;

    rect->top    = rect->left = 0;
    rect->right  = Wnd->rcWindow.right - Wnd->rcWindow.left;
    rect->bottom = Wnd->rcWindow.bottom - Wnd->rcWindow.top;

    if (Style & WS_ICONIC) return;

    /* Remove frame from rectangle */
    if (UserHasThickFrameStyle(Style, ExStyle ))
    {
        RECTL_vInflateRect(rect, -UserGetSystemMetrics(SM_CXFRAME), -UserGetSystemMetrics(SM_CYFRAME));
    }
    else
    {
        if (UserHasDlgFrameStyle(Style, ExStyle ))
        {
            RECTL_vInflateRect(rect, -UserGetSystemMetrics(SM_CXDLGFRAME), -UserGetSystemMetrics(SM_CYDLGFRAME));
            /* FIXME: this isn't in NC_AdjustRect? why not? */
            if (ExStyle & WS_EX_DLGMODALFRAME)
	            RECTL_vInflateRect( rect, -1, 0 );
        }
        else
        {
            if (UserHasThinFrameStyle(Style, ExStyle))
            {
                RECTL_vInflateRect(rect, -UserGetSystemMetrics(SM_CXBORDER), -UserGetSystemMetrics(SM_CYBORDER));
            }
        }
    }
    /* We have additional border information if the window
     * is a child (but not an MDI child) */
    if ((Style & WS_CHILD) && !(ExStyle & WS_EX_MDICHILD))
    {
       if (ExStyle & WS_EX_CLIENTEDGE)
          RECTL_vInflateRect (rect, -UserGetSystemMetrics(SM_CXEDGE), -UserGetSystemMetrics(SM_CYEDGE));
       if (ExStyle & WS_EX_STATICEDGE)
          RECTL_vInflateRect (rect, -UserGetSystemMetrics(SM_CXBORDER), -UserGetSystemMetrics(SM_CYBORDER));
    }
}

LONG FASTCALL
DefWndStartSizeMove(PWND Wnd, WPARAM wParam, POINT *capturePoint)
{
   LONG hittest = 0;
   POINT pt;
   MSG msg;
   RECT rectWindow;
   ULONG Style = Wnd->style;
   PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

   rectWindow = Wnd->rcWindow;

   if ((wParam & 0xfff0) == SC_MOVE)
   {
       /* Move pointer at the center of the caption */
       RECT rect = rectWindow;
       /* Note: to be exactly centered we should take the different types
        * of border into account, but it shouldn't make more than a few pixels
        * of difference so let's not bother with that */
       if (Style & WS_SYSMENU)
          rect.left += UserGetSystemMetrics(SM_CXSIZE) + 1;
       if (Style & WS_MINIMIZEBOX)
	  rect.right -= UserGetSystemMetrics(SM_CXSIZE) + 1;
       if (Style & WS_MAXIMIZEBOX)
          rect.right -= UserGetSystemMetrics(SM_CXSIZE) + 1;
       pt.x = (rect.right + rect.left) / 2;
       pt.y = rect.top + UserGetSystemMetrics(SM_CYSIZE)/2;
       hittest = HTCAPTION;
       *capturePoint = pt;
   }
   else  /* SC_SIZE */
   {
       pt.x = pt.y = 0;
       while (!hittest)
       {
          if (!co_IntGetPeekMessage(&msg, 0, 0, 0, PM_REMOVE, TRUE)) return 0;
          if (IntCallMsgFilter( &msg, MSGF_SIZE )) continue;

	  switch(msg.message)
	    {
	    case WM_MOUSEMOVE:
	      //// Clamp the mouse position to the window rectangle when starting a window resize.
              pt.x = min( max( msg.pt.x, rectWindow.left ), rectWindow.right - 1 );
              pt.y = min( max( msg.pt.y, rectWindow.top ), rectWindow.bottom - 1 );
	      hittest = GetNCHitEx(Wnd, pt);
	      if ((hittest < HTLEFT) || (hittest > HTBOTTOMRIGHT)) hittest = 0;
	      break;

	    case WM_LBUTTONUP:
	      return 0;

	    case WM_KEYDOWN:
	      switch (msg.wParam)
		{
		case VK_UP:
		  hittest = HTTOP;
		  pt.x = (rectWindow.left+rectWindow.right)/2;
		  pt.y =  rectWindow.top + UserGetSystemMetrics(SM_CYFRAME) / 2;
		  break;
		case VK_DOWN:
		  hittest = HTBOTTOM;
		  pt.x = (rectWindow.left+rectWindow.right)/2;
		  pt.y =  rectWindow.bottom - UserGetSystemMetrics(SM_CYFRAME) / 2;
		  break;
		case VK_LEFT:
		  hittest = HTLEFT;
		  pt.x =  rectWindow.left + UserGetSystemMetrics(SM_CXFRAME) / 2;
		  pt.y = (rectWindow.top+rectWindow.bottom)/2;
		  break;
		case VK_RIGHT:
		  hittest = HTRIGHT;
		  pt.x =  rectWindow.right - UserGetSystemMetrics(SM_CXFRAME) / 2;
		  pt.y = (rectWindow.top+rectWindow.bottom)/2;
		  break;
		case VK_RETURN:
		case VK_ESCAPE:
		  return 0;
		}
            default:
              IntTranslateKbdMessage( &msg, 0 );
              pti->TIF_flags |= TIF_MOVESIZETRACKING;
              IntDispatchMessage( &msg );
              pti->TIF_flags |= TIF_MOVESIZETRACKING;
              break;
	    }
       }
       *capturePoint = pt;
    }
    UserSetCursorPos(pt.x, pt.y, 0, 0, FALSE);
    co_IntSendMessage(UserHMGetHandle(Wnd), WM_SETCURSOR, (WPARAM)UserHMGetHandle(Wnd), MAKELONG(hittest, WM_MOUSEMOVE));
    return hittest;
}

//
//  System Command Size and Move
//
//  Perform SC_MOVE and SC_SIZE commands.
//
VOID FASTCALL
DefWndDoSizeMove(PWND pwnd, WORD wParam)
{
   MSG msg;
   RECT sizingRect, mouseRect, origRect, unmodRect;
   HDC hdc;
   LONG hittest = (LONG)(wParam & 0x0f);
   PCURICON_OBJECT DragCursor = NULL, OldCursor = NULL;
   POINT minTrack, maxTrack;
   POINT capturePoint, pt;
   ULONG Style, ExStyle;
   BOOL thickframe;
   BOOL iconic;
   BOOL moved = FALSE;
   BOOL DragFullWindows = FALSE;
   PWND pWndParent = NULL;
   WPARAM syscommand = (wParam & 0xfff0);
   PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
   //PMONITOR mon = 0; Don't port sync from wine!!! This breaks explorer task bar sizing!!
   //                  The task bar can grow in size and can not reduce due to the change
   //                  in the work area.

   Style = pwnd->style;
   ExStyle = pwnd->ExStyle;
   iconic = (Style & WS_MINIMIZE) != 0;

   if ((Style & WS_MAXIMIZE) || !IntIsWindowVisible(pwnd)) return;

   thickframe = UserHasThickFrameStyle(Style, ExStyle) && !iconic;

   //
   // Show window contents while dragging the window, get flag from registry data.
   //
   UserSystemParametersInfo(SPI_GETDRAGFULLWINDOWS, 0, &DragFullWindows, 0);

   pt.x = pti->ptLast.x;
   pt.y = pti->ptLast.y;
   capturePoint = pt;
   UserClipCursor( NULL );

   TRACE("pwnd %p command %04lx, hittest %d, pos %d,%d\n",
          pwnd, syscommand, hittest, pt.x, pt.y);

   if (syscommand == SC_MOVE)
   {
      if (!hittest) hittest = DefWndStartSizeMove(pwnd, wParam, &capturePoint);
      if (!hittest) return;
   }
   else  /* SC_SIZE */
   {
      if (!thickframe) return;
      if (hittest && (syscommand != SC_MOUSEMENU))
      {
          hittest += (HTLEFT - WMSZ_LEFT);
      }
      else
      {
          co_UserSetCapture(UserHMGetHandle(pwnd));
          hittest = DefWndStartSizeMove(pwnd, wParam, &capturePoint);
	  if (!hittest)
          {
              IntReleaseCapture();
              return;
          }
      }
   }

   /* Get min/max info */

   co_WinPosGetMinMaxInfo(pwnd, NULL, NULL, &minTrack, &maxTrack);
   sizingRect = pwnd->rcWindow;
   origRect = sizingRect;
   if (Style & WS_CHILD)
   {
      pWndParent = IntGetParent(pwnd);
      IntGetClientRect( pWndParent, &mouseRect );
      IntMapWindowPoints( pWndParent, 0, (LPPOINT)&mouseRect, 2 );
      IntMapWindowPoints( 0, pWndParent, (LPPOINT)&sizingRect, 2 );
      unmodRect = sizingRect;
   }
   else
   {
      if (!(ExStyle & WS_EX_TOPMOST))
      {
        UserSystemParametersInfo(SPI_GETWORKAREA, 0, &mouseRect, 0);
      }
      else
      {
        RECTL_vSetRect(&mouseRect, 0, 0, UserGetSystemMetrics(SM_CXSCREEN), UserGetSystemMetrics(SM_CYSCREEN));
      }
      unmodRect = sizingRect;
   }

   if (ON_LEFT_BORDER(hittest))
   {
      mouseRect.left  = max( mouseRect.left, sizingRect.right-maxTrack.x+capturePoint.x-sizingRect.left );
      mouseRect.right = min( mouseRect.right, sizingRect.right-minTrack.x+capturePoint.x-sizingRect.left );
   }
   else if (ON_RIGHT_BORDER(hittest))
   {
      mouseRect.left  = max( mouseRect.left, sizingRect.left+minTrack.x+capturePoint.x-sizingRect.right );
      mouseRect.right = min( mouseRect.right, sizingRect.left+maxTrack.x+capturePoint.x-sizingRect.right );
   }
   if (ON_TOP_BORDER(hittest))
   {
      mouseRect.top    = max( mouseRect.top, sizingRect.bottom-maxTrack.y+capturePoint.y-sizingRect.top );
      mouseRect.bottom = min( mouseRect.bottom,sizingRect.bottom-minTrack.y+capturePoint.y-sizingRect.top);
   }
   else if (ON_BOTTOM_BORDER(hittest))
   {
      mouseRect.top    = max( mouseRect.top, sizingRect.top+minTrack.y+capturePoint.y-sizingRect.bottom );
      mouseRect.bottom = min( mouseRect.bottom, sizingRect.top+maxTrack.y+capturePoint.y-sizingRect.bottom );
   }

   hdc = UserGetDCEx( pWndParent, 0, DCX_CACHE );
   if (iconic)
   {
       DragCursor = pwnd->pcls->spicn;
       if (DragCursor)
       {
           UserReferenceObject(DragCursor);
       }
       else
       {
           HCURSOR CursorHandle = (HCURSOR)co_IntSendMessage( UserHMGetHandle(pwnd), WM_QUERYDRAGICON, 0, 0 );
           if (CursorHandle)
           {
               DragCursor = UserGetCurIconObject(CursorHandle);
           }
           else
           {
               iconic = FALSE;
           }
       }
   }

   /* repaint the window before moving it around */
   co_UserRedrawWindow( pwnd, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN);

   IntNotifyWinEvent( EVENT_SYSTEM_MOVESIZESTART, pwnd, OBJID_WINDOW, CHILDID_SELF, 0);

   co_IntSendMessage( UserHMGetHandle(pwnd), WM_ENTERSIZEMOVE, 0, 0 );

   MsqSetStateWindow(pti, MSQ_STATE_MOVESIZE, UserHMGetHandle(pwnd));

   if (IntGetCapture() != UserHMGetHandle(pwnd)) co_UserSetCapture( UserHMGetHandle(pwnd) );

   pwnd->head.pti->TIF_flags |= TIF_MOVESIZETRACKING;

   for(;;)
   {
      int dx = 0, dy = 0;

      if (!co_IntGetPeekMessage(&msg, 0, 0, 0, PM_REMOVE, TRUE)) break;
      if (IntCallMsgFilter( &msg, MSGF_SIZE )) continue;

      /* Exit on button-up, Return, or Esc */
      if ((msg.message == WM_LBUTTONUP) ||
	  ((msg.message == WM_KEYDOWN) &&
	   ((msg.wParam == VK_RETURN) || (msg.wParam == VK_ESCAPE)))) break;

      if ((msg.message != WM_KEYDOWN) && (msg.message != WM_MOUSEMOVE))
      {
         IntTranslateKbdMessage( &msg , 0 );
         IntDispatchMessage( &msg );
         continue;  /* We are not interested in other messages */
      }

      pt = msg.pt;

      if (msg.message == WM_KEYDOWN) switch(msg.wParam)
      {
	case VK_UP:    pt.y -= 8; break;
	case VK_DOWN:  pt.y += 8; break;
	case VK_LEFT:  pt.x -= 8; break;
	case VK_RIGHT: pt.x += 8; break;
      }

      pt.x = max( pt.x, mouseRect.left );
      pt.x = min( pt.x, mouseRect.right - 1 );
      pt.y = max( pt.y, mouseRect.top );
      pt.y = min( pt.y, mouseRect.bottom - 1 );

      dx = pt.x - capturePoint.x;
      dy = pt.y - capturePoint.y;

      if (dx || dy)
      {
	  if ( !moved )
	  {
	      moved = TRUE;
          if ( iconic ) /* ok, no system popup tracking */
          {
              OldCursor = UserSetCursor(DragCursor, FALSE);
              UserShowCursor( TRUE );
          }
          else if(!DragFullWindows)
             UserDrawMovingFrame( hdc, &sizingRect, thickframe );
	  }

	  if (msg.message == WM_KEYDOWN) UserSetCursorPos(pt.x, pt.y, 0, 0, FALSE);
	  else
	  {
	      RECT newRect = unmodRect;

	      if (!iconic && !DragFullWindows) UserDrawMovingFrame( hdc, &sizingRect, thickframe );
	      if (hittest == HTCAPTION) RECTL_vOffsetRect( &newRect, dx, dy );
	      if (ON_LEFT_BORDER(hittest)) newRect.left += dx;
	      else if (ON_RIGHT_BORDER(hittest)) newRect.right += dx;
	      if (ON_TOP_BORDER(hittest)) newRect.top += dy;
	      else if (ON_BOTTOM_BORDER(hittest)) newRect.bottom += dy;
	      capturePoint = pt;

              //
              //  Save the new position to the unmodified rectangle. This allows explorer task bar
              //  sizing. Explorer will forces back the position unless a certain amount of sizing
              //  has occurred.
              //
              unmodRect = newRect;

	      /* determine the hit location */
              if (syscommand == SC_SIZE)
              {
                  WPARAM wpSizingHit = 0;

                  if (hittest >= HTLEFT && hittest <= HTBOTTOMRIGHT)
                      wpSizingHit = WMSZ_LEFT + (hittest - HTLEFT);
                  co_IntSendMessage( UserHMGetHandle(pwnd), WM_SIZING, wpSizingHit, (LPARAM)&newRect );
              }
              else
                  co_IntSendMessage( UserHMGetHandle(pwnd), WM_MOVING, 0, (LPARAM)&newRect );

	      if (!iconic)
              {
                 if (!DragFullWindows)
                     UserDrawMovingFrame( hdc, &newRect, thickframe );
                 else
                 {  // Moving the whole window now!
                    PWND pwndTemp;
                    //// This causes the mdi child window to jump up when it is moved.
                    //IntMapWindowPoints( 0, pWndParent, (POINT *)&rect, 2 );
		    co_WinPosSetWindowPos( pwnd,
		                           0,
		                           newRect.left,
		                           newRect.top,
				           newRect.right - newRect.left,
				           newRect.bottom - newRect.top,
				          ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );

                    // Update all the windows after the move or size, including this window.
                    for ( pwndTemp = pwnd->head.rpdesk->pDeskInfo->spwnd->spwndChild;
                          pwndTemp;
                          pwndTemp = pwndTemp->spwndNext )
                    {
                       RECTL rect;
                       // Only the windows that overlap will be redrawn.
                       if (RECTL_bIntersectRect( &rect, &pwnd->rcWindow, &pwndTemp->rcWindow ))
                       {
                          co_UserRedrawWindow( pwndTemp, NULL, NULL, RDW_UPDATENOW | RDW_ALLCHILDREN);
                       }
                    }
                 }
              }
              sizingRect = newRect;
	  }
      }
   }

   pwnd->head.pti->TIF_flags &= ~TIF_MOVESIZETRACKING;

   IntReleaseCapture();

   if ( iconic )
   {
      if ( moved ) /* restore cursors, show icon title later on */
      {
          UserShowCursor( FALSE );
          OldCursor = UserSetCursor(OldCursor, FALSE);
      }

      /* It could be that the cursor was already changed while we were proceeding,
       * so we must unreference whatever cursor was current at the time we restored the old one.
       * Maybe it is DragCursor, but maybe it is another one and DragCursor got already freed.
       */
      if (OldCursor) UserDereferenceObject(OldCursor);
   }
   else if ( moved && !DragFullWindows )
      UserDrawMovingFrame( hdc, &sizingRect, thickframe );

   UserReleaseDC(NULL, hdc, FALSE);

   //// This causes the mdi child window to jump up when it is moved.
   //if (pWndParent) IntMapWindowPoints( 0, pWndParent, (POINT *)&sizingRect, 2 );

   if (co_HOOK_CallHooks(WH_CBT, HCBT_MOVESIZE, (WPARAM)UserHMGetHandle(pwnd), (LPARAM)&sizingRect))
   {
      ERR("DoSizeMove : WH_CBT Call Hook return!\n");
      moved = FALSE;
   }

   IntNotifyWinEvent( EVENT_SYSTEM_MOVESIZEEND, pwnd, OBJID_WINDOW, CHILDID_SELF, 0);

   MsqSetStateWindow(pti, MSQ_STATE_MOVESIZE, NULL);

   co_IntSendMessage( UserHMGetHandle(pwnd), WM_EXITSIZEMOVE, 0, 0 );
   //// wine mdi hack
   co_IntSendMessage( UserHMGetHandle(pwnd), WM_SETVISIBLE, !!(pwnd->style & WS_MINIMIZE), 0L);
   ////
   /* window moved or resized */
   if (moved)
   {
      /* if the moving/resizing isn't canceled call SetWindowPos
       * with the new position or the new size of the window
       */
      if (!((msg.message == WM_KEYDOWN) && (msg.wParam == VK_ESCAPE)) )
      {
	  /* NOTE: SWP_NOACTIVATE prevents document window activation in Word 6 */
	  if (!DragFullWindows || iconic )
	  {
	    co_WinPosSetWindowPos( pwnd,
	                           0,
	                           sizingRect.left,
	                           sizingRect.top,
			           sizingRect.right - sizingRect.left,
			           sizingRect.bottom - sizingRect.top,
			          ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
          }
      }
      else
      { /* restore previous size/position */
	if ( DragFullWindows )
	{
	  co_WinPosSetWindowPos( pwnd,
	                         0,
	                         origRect.left,
	                         origRect.top,
			         origRect.right - origRect.left,
			         origRect.bottom - origRect.top,
			        ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );
        }
      }
   }

   if ( IntIsWindow(UserHMGetHandle(pwnd)) )
     if ( iconic )
     {
	/* Single click brings up the system menu when iconized */
	if ( !moved )
        {
	    if( Style & WS_SYSMENU )
	      co_IntSendMessage( UserHMGetHandle(pwnd), WM_SYSCOMMAND, SC_MOUSEMENU + HTSYSMENU, MAKELONG(pt.x,pt.y));
        }
     }
}

//
// Handle a WM_SYSCOMMAND message. Called from DefWindowProc().
//
LRESULT FASTCALL
DefWndHandleSysCommand(PWND pWnd, WPARAM wParam, LPARAM lParam)
{
   LRESULT lResult = 0;
   BOOL Hook = FALSE;

   if (ISITHOOKED(WH_CBT) || (pWnd->head.rpdesk->pDeskInfo->fsHooks & HOOKID_TO_FLAG(WH_CBT)))
   {
      Hook = TRUE;
      lResult = co_HOOK_CallHooks(WH_CBT, HCBT_SYSCOMMAND, wParam, lParam);

      if (lResult) return lResult;
   }

   switch (wParam & 0xfff0)
   {
      case SC_MOVE:
      case SC_SIZE:
        DefWndDoSizeMove(pWnd, wParam);
        break;

      case SC_MINIMIZE:
        if (UserHMGetHandle(pWnd) == UserGetActiveWindow())
            IntShowOwnedPopups(pWnd,FALSE); // This is done in ShowWindow! Need to retest!
        co_WinPosShowWindow( pWnd, SW_MINIMIZE );
        break;

      case SC_MAXIMIZE:
        if (((pWnd->style & WS_MINIMIZE) != 0) && UserHMGetHandle(pWnd) == UserGetActiveWindow())
            IntShowOwnedPopups(pWnd,TRUE);
        co_WinPosShowWindow( pWnd, SW_MAXIMIZE );
        break;

      case SC_RESTORE:
        if (((pWnd->style & WS_MINIMIZE) != 0) && UserHMGetHandle(pWnd) == UserGetActiveWindow())
            IntShowOwnedPopups(pWnd,TRUE);
        co_WinPosShowWindow( pWnd, SW_RESTORE );
        break;

      case SC_CLOSE:
        return co_IntSendMessage(UserHMGetHandle(pWnd), WM_CLOSE, 0, 0);

      case SC_SCREENSAVE:
        ERR("Screensaver Called!\n");
        UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_START_SCREENSAVE, 0); // always lParam 0 == not Secure
        break;

      case SC_HOTKEY:
        {
           USER_REFERENCE_ENTRY Ref;

           pWnd = ValidateHwndNoErr((HWND)lParam);
           if (pWnd)
           {
              if (pWnd->spwndLastActive)
              {
                 pWnd = pWnd->spwndLastActive;
              }
              UserRefObjectCo(pWnd, &Ref);
              co_IntSetForegroundWindow(pWnd);
              UserDerefObjectCo(pWnd);
              if (pWnd->style & WS_MINIMIZE)
              {
                 UserPostMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SC_RESTORE, 0);
              }
           }
        }
        break;


      default:
   // We do not support anything else here so we should return normal even when sending a hook.
        return 0;
   }

   return(Hook ? 1 : 0); // Don't call us again from user space.
}

PWND FASTCALL
co_IntFindChildWindowToOwner(PWND Root, PWND Owner)
{
   PWND Ret;
   PWND Child, OwnerWnd;

   for(Child = Root->spwndChild; Child; Child = Child->spwndNext)
   {
      OwnerWnd = Child->spwndOwner;
      if(!OwnerWnd)
         continue;

      if (!(Child->style & WS_POPUP) ||
          !(Child->style & WS_VISIBLE) ||
          /* Fixes CMD pop up properties window from having foreground. */
           Owner->head.pti->MessageQueue != Child->head.pti->MessageQueue)
         continue;

      if(OwnerWnd == Owner)
      {
         Ret = Child;
         return Ret;
      }
   }
   return NULL;
}

LRESULT
DefWndHandleSetCursor(PWND pWnd, WPARAM wParam, LPARAM lParam)
{
   PWND pwndPopUP = NULL;
   WORD Msg = HIWORD(lParam);

   /* Not for child windows. */
   if (UserHMGetHandle(pWnd) != (HWND)wParam)
   {
      return FALSE;
   }

   switch((short)LOWORD(lParam))
   {
      case HTERROR:
      {
         //// This is the real fix for CORE-6129! This was a "Code Whole".
         USER_REFERENCE_ENTRY Ref;

         if (Msg == WM_LBUTTONDOWN)
         {
            // Find a pop up window to bring active.
            pwndPopUP = co_IntFindChildWindowToOwner(UserGetDesktopWindow(), pWnd);
            if (pwndPopUP)
            {
               // Not a child pop up from desktop.
               if ( pwndPopUP != UserGetDesktopWindow()->spwndChild )
               {
                  // Get original active window.
                  PWND pwndOrigActive = gpqForeground->spwndActive;

                  co_WinPosSetWindowPos(pWnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

                  UserRefObjectCo(pwndPopUP, &Ref);
                  //UserSetActiveWindow(pwndPopUP);
                  co_IntSetForegroundWindow(pwndPopUP); // HACK
                  UserDerefObjectCo(pwndPopUP);

                  // If the change was made, break out.
                  if (pwndOrigActive != gpqForeground->spwndActive)
                     break;
               }
            }
         }
         ////
	 if (Msg == WM_LBUTTONDOWN || Msg == WM_MBUTTONDOWN ||
	     Msg == WM_RBUTTONDOWN || Msg == WM_XBUTTONDOWN)
	 {
             if (pwndPopUP)
             {
                 FLASHWINFO fwi =
                    {sizeof(FLASHWINFO),
                     UserHMGetHandle(pwndPopUP),
                     FLASHW_ALL,
                     gspv.dwForegroundFlashCount,
                     (gpsi->dtCaretBlink >> 3)};

                 // Now shake that window!
                 IntFlashWindowEx(pwndPopUP, &fwi);
             }
	     UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_MESSAGE_BEEP, 0);
	 }
	 break;
      }

      case HTCLIENT:
      {
         if (pWnd->pcls->spcur)
         {
            UserSetCursor(pWnd->pcls->spcur, FALSE);
	 }
	 return FALSE;
      }

      case HTLEFT:
      case HTRIGHT:
      {
         if (pWnd->style & WS_MAXIMIZE)
         {
            break;
         }
         UserSetCursor(SYSTEMCUR(SIZEWE), FALSE);
         return TRUE;
      }

      case HTTOP:
      case HTBOTTOM:
      {
         if (pWnd->style & WS_MAXIMIZE)
         {
            break;
         }
         UserSetCursor(SYSTEMCUR(SIZENS), FALSE);
         return TRUE;
       }

       case HTTOPLEFT:
       case HTBOTTOMRIGHT:
       {
         if (pWnd->style & WS_MAXIMIZE)
         {
            break;
         }
         UserSetCursor(SYSTEMCUR(SIZENWSE), FALSE);
         return TRUE;
       }

       case HTBOTTOMLEFT:
       case HTTOPRIGHT:
       {
         if (pWnd->style & WS_MAXIMIZE)
         {
            break;
         }
         UserSetCursor(SYSTEMCUR(SIZENESW), FALSE);
         return TRUE;
       }
   }
   UserSetCursor(SYSTEMCUR(ARROW), FALSE);
   return FALSE;
}

VOID FASTCALL DefWndPrint( PWND pwnd, HDC hdc, ULONG uFlags)
{
  /*
   * Visibility flag.
   */
  if ( (uFlags & PRF_CHECKVISIBLE) &&
       !IntIsWindowVisible(pwnd) )
      return;

  /*
   * Unimplemented flags.
   */
  if ( (uFlags & PRF_CHILDREN) ||
       (uFlags & PRF_OWNED)    ||
       (uFlags & PRF_NONCLIENT) )
  {
    FIXME("WM_PRINT message with unsupported flags\n");
  }

  /*
   * Background
   */
  if ( uFlags & PRF_ERASEBKGND)
    co_IntSendMessage(UserHMGetHandle(pwnd), WM_ERASEBKGND, (WPARAM)hdc, 0);

  /*
   * Client area
   */
  if ( uFlags & PRF_CLIENT)
    co_IntSendMessage(UserHMGetHandle(pwnd), WM_PRINTCLIENT, (WPARAM)hdc, uFlags);
}


/*
   Win32k counterpart of User DefWindowProc
 */
LRESULT FASTCALL
IntDefWindowProc(
   PWND Wnd,
   UINT Msg,
   WPARAM wParam,
   LPARAM lParam,
   BOOL Ansi)
{
   LRESULT lResult = 0;
   USER_REFERENCE_ENTRY Ref;

   if (Msg > WM_USER) return 0;

   switch (Msg)
   {
      case WM_SYSCOMMAND:
      {
         ERR("hwnd %p WM_SYSCOMMAND %lx %lx\n", Wnd->head.h, wParam, lParam );
         lResult = DefWndHandleSysCommand(Wnd, wParam, lParam);
         break;
      }
      case WM_SHOWWINDOW:
      {
         if ((Wnd->style & WS_VISIBLE) && wParam) break;
         if (!(Wnd->style & WS_VISIBLE) && !wParam) break;
         if (!Wnd->spwndOwner) break;
         if (LOWORD(lParam))
         {
            if (wParam)
            {
               if (!(Wnd->state & WNDS_HIDDENPOPUP)) break;
               Wnd->state &= ~WNDS_HIDDENPOPUP;
            }
            else
                Wnd->state |= WNDS_HIDDENPOPUP;

            co_WinPosShowWindow(Wnd, wParam ? SW_SHOWNOACTIVATE : SW_HIDE);
         }
      }
      break;

      case WM_CLIENTSHUTDOWN:
         return IntClientShutdown(Wnd, wParam, lParam);

      case WM_APPCOMMAND:
         ERR("WM_APPCOMMAND\n");
         if ( (Wnd->style & (WS_POPUP|WS_CHILD)) != WS_CHILD &&
               Wnd != co_GetDesktopWindow(Wnd) )
         {
            if (!co_HOOK_CallHooks(WH_SHELL, HSHELL_APPCOMMAND, wParam, lParam))
               co_IntShellHookNotify(HSHELL_APPCOMMAND, wParam, lParam);
            break;
         }
         UserRefObjectCo(Wnd->spwndParent, &Ref);
         lResult = co_IntSendMessage(UserHMGetHandle(Wnd->spwndParent), WM_APPCOMMAND, wParam, lParam);
         UserDerefObjectCo(Wnd->spwndParent);
         break;

      case WM_CLOSE:
         co_UserDestroyWindow(Wnd);
         break;

      case WM_CTLCOLORMSGBOX:
      case WM_CTLCOLOREDIT:
      case WM_CTLCOLORLISTBOX:
      case WM_CTLCOLORBTN:
      case WM_CTLCOLORDLG:
      case WM_CTLCOLORSTATIC:
      case WM_CTLCOLORSCROLLBAR:
           return (LRESULT) DefWndControlColor((HDC)wParam, Msg - WM_CTLCOLORMSGBOX);

      case WM_CTLCOLOR:
           return (LRESULT) DefWndControlColor((HDC)wParam, HIWORD(lParam));

      case WM_SETCURSOR:
      {
         if (Wnd->style & WS_CHILD)
         {
             /* with the exception of the border around a resizable wnd,
              * give the parent first chance to set the cursor */
             if (LOWORD(lParam) < HTLEFT || LOWORD(lParam) > HTBOTTOMRIGHT)
             {
                 PWND parent = Wnd->spwndParent;//IntGetParent( Wnd );
                 if (parent != UserGetDesktopWindow() &&
                     co_IntSendMessage( UserHMGetHandle(parent), WM_SETCURSOR, wParam, lParam))
                    return TRUE;
             }
         }
         return DefWndHandleSetCursor(Wnd, wParam, lParam);
      }

      case WM_ACTIVATE:
       /* The default action in Windows is to set the keyboard focus to
        * the window, if it's being activated and not minimized */
         if (LOWORD(wParam) != WA_INACTIVE &&
              !(Wnd->style & WS_MINIMIZE))
         {
            //ERR("WM_ACTIVATE %p\n",hWnd);
            co_UserSetFocus(Wnd);
         }
         break;

      case WM_MOUSEWHEEL:
         if (Wnd->style & WS_CHILD)
         {
            HWND hwndParent;
            PWND pwndParent = IntGetParent(Wnd);
            hwndParent = pwndParent ? UserHMGetHandle(pwndParent) : NULL;
            return co_IntSendMessage( hwndParent, WM_MOUSEWHEEL, wParam, lParam);
         }
         break;

      case WM_ERASEBKGND:
      case WM_ICONERASEBKGND:
      {
         RECT Rect;
         HBRUSH hBrush = Wnd->pcls->hbrBackground;
         if (!hBrush) return 0;
         if (hBrush <= (HBRUSH)COLOR_MENUBAR)
         {
            hBrush = IntGetSysColorBrush((INT)hBrush);
         }
         if (Wnd->pcls->style & CS_PARENTDC)
         {
            /* can't use GetClipBox with a parent DC or we fill the whole parent */
            IntGetClientRect(Wnd, &Rect);
            GreDPtoLP((HDC)wParam, (LPPOINT)&Rect, 2);
         }
         else
         {
            GdiGetClipBox((HDC)wParam, &Rect);
         }
         FillRect((HDC)wParam, &Rect, hBrush);
         return (1);
      }

      case WM_GETHOTKEY:
         //ERR("WM_GETHOTKEY\n");
         return DefWndGetHotKey(Wnd);
      case WM_SETHOTKEY:
         //ERR("WM_SETHOTKEY\n");
         return DefWndSetHotKey(Wnd, wParam);

      case WM_NCHITTEST:
      {
         POINT Point;
         Point.x = GET_X_LPARAM(lParam);
         Point.y = GET_Y_LPARAM(lParam);
         return GetNCHitEx(Wnd, Point);
      }

      case WM_PRINT:
      {
         DefWndPrint(Wnd, (HDC)wParam, lParam);
         return (0);
      }

      case WM_PAINTICON:
      case WM_PAINT:
      {
         PAINTSTRUCT Ps;
         HDC hDC;

         /* If already in Paint and Client area is not empty just return. */
         if (Wnd->state2 & WNDS2_STARTPAINT && !RECTL_bIsEmptyRect(&Wnd->rcClient))
         {
            ERR("In Paint and Client area is not empty!\n");
            return 0;
         }

         hDC = IntBeginPaint(Wnd, &Ps);
         if (hDC)
         {
             if (((Wnd->style & WS_MINIMIZE) != 0) && (Wnd->pcls->spicn))
             {
                 RECT ClientRect;
                 INT x, y;

                 ERR("Doing Paint and Client area is empty!\n");
                 IntGetClientRect(Wnd, &ClientRect);
                 x = (ClientRect.right - ClientRect.left - UserGetSystemMetrics(SM_CXICON)) / 2;
                 y = (ClientRect.bottom - ClientRect.top - UserGetSystemMetrics(SM_CYICON)) / 2;
                 UserDrawIconEx(hDC, x, y, Wnd->pcls->spicn, 0, 0, 0, 0, DI_NORMAL | DI_COMPAT | DI_DEFAULTSIZE);
             }

             IntEndPaint(Wnd, &Ps);
         }
         return (0);
      }

      case WM_SYNCPAINT:
      {
         PREGION Rgn;
         Wnd->state &= ~WNDS_SYNCPAINTPENDING;
         ERR("WM_SYNCPAINT\n");
         Rgn = IntSysCreateRectpRgn(0, 0, 0, 0);
         if (Rgn)
         {
             if (co_UserGetUpdateRgn(Wnd, Rgn, FALSE) != NULLREGION)
             {
                if (!wParam)
                    wParam = (RDW_ERASENOW | RDW_ERASE | RDW_FRAME | RDW_ALLCHILDREN);
                co_UserRedrawWindow(Wnd, NULL, Rgn, wParam);
             }
             REGION_Delete(Rgn);
         }
         return 0;
      }

      case WM_SETREDRAW:
          ERR("WM_SETREDRAW\n");
          if (wParam)
          {
             if (!(Wnd->style & WS_VISIBLE))
             {
                IntSetStyle( Wnd, WS_VISIBLE, 0 );
                Wnd->state |= WNDS_SENDNCPAINT;
             }
          }
          else
          {
             if (Wnd->style & WS_VISIBLE)
             {
                co_UserRedrawWindow( Wnd, NULL, NULL, RDW_ALLCHILDREN | RDW_VALIDATE );
                IntSetStyle( Wnd, 0, WS_VISIBLE );
             }
          }
          return 0;

      case WM_WINDOWPOSCHANGING:
      {
          return (DefWndHandleWindowPosChanging(Wnd, (WINDOWPOS*)lParam));
      }

      case WM_WINDOWPOSCHANGED:
      {
          return (DefWndHandleWindowPosChanged(Wnd, (WINDOWPOS*)lParam));
      }

      /* ReactOS only. */
      case WM_CBT:
      {
         switch (wParam)
         {
            case HCBT_MOVESIZE:
            {
               RECTL rt;

               if (lParam)
               {
                  _SEH2_TRY
                  {
                      ProbeForRead((PVOID)lParam,
                                   sizeof(RECT),
                                   1);

                      RtlCopyMemory(&rt,
                                    (PVOID)lParam,
                                    sizeof(RECT));
                  }
                  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                  {
                      lResult = 1;
                  }
                  _SEH2_END;
               }
               if (!lResult)
                  lResult = co_HOOK_CallHooks(WH_CBT, HCBT_MOVESIZE, (WPARAM)Wnd->head.h, lParam ? (LPARAM)&rt : 0);
           }
            break;
         }
         break;
      }
      break;
   }
   return lResult;
}

PCURICON_OBJECT FASTCALL NC_IconForWindow( PWND pWnd )
{
    PCURICON_OBJECT pIcon = NULL;
    HICON hIcon;

   //FIXME: Some callers use this function as if it returns a boolean saying "this window has an icon".
   //FIXME: Hence we must return a pointer with no reference count.
   //FIXME: This is bad and we should feel bad.
   //FIXME: Stop whining over wine code.

   hIcon = UserGetProp(pWnd, gpsi->atomIconSmProp);
   if (!hIcon) hIcon = UserGetProp(pWnd, gpsi->atomIconProp);

   if (!hIcon && pWnd->pcls->spicnSm)
       return pWnd->pcls->spicnSm;
   if (!hIcon && pWnd->pcls->spicn)
       return pWnd->pcls->spicn;

   if (!hIcon && (pWnd->style & DS_MODALFRAME))
   {
      if (!hIcon) hIcon = gpsi->hIconSmWindows; // Both are IDI_WINLOGO Small
      if (!hIcon) hIcon = gpsi->hIconWindows;   // Reg size.
   }
   if (hIcon)
   {
       pIcon = UserGetCurIconObject(hIcon);
       if (pIcon)
       {
           UserDereferenceObject(pIcon);
       }
   }
   return pIcon;
}

DWORD FASTCALL
GetNCHitEx(PWND pWnd, POINT pt)
{
   RECT rcWindow, rcClient;
   DWORD Style, ExStyle;

   if (!pWnd) return HTNOWHERE;

   if (pWnd == UserGetDesktopWindow()) // pWnd->fnid == FNID_DESKTOP)
   {
      rcClient.left = rcClient.top = rcWindow.left = rcWindow.top = 0;
      rcWindow.right  = UserGetSystemMetrics(SM_CXSCREEN);
      rcWindow.bottom = UserGetSystemMetrics(SM_CYSCREEN);
      rcClient.right  = UserGetSystemMetrics(SM_CXSCREEN);
      rcClient.bottom = UserGetSystemMetrics(SM_CYSCREEN);
   }
   else
   {
      rcClient = pWnd->rcClient;
      rcWindow = pWnd->rcWindow;
   }

   if (!RECTL_bPointInRect(&rcWindow, pt.x, pt.y)) return HTNOWHERE;

   Style = pWnd->style;
   ExStyle = pWnd->ExStyle;

   if (Style & WS_MINIMIZE) return HTCAPTION;

   if (RECTL_bPointInRect( &rcClient,  pt.x, pt.y )) return HTCLIENT;

   /* Check borders */
   if (HAS_THICKFRAME( Style, ExStyle ))
   {
      RECTL_vInflateRect(&rcWindow, -UserGetSystemMetrics(SM_CXFRAME), -UserGetSystemMetrics(SM_CYFRAME) );
      if (!RECTL_bPointInRect(&rcWindow, pt.x, pt.y ))
      {
            /* Check top sizing border */
            if (pt.y < rcWindow.top)
            {
                if (pt.x < rcWindow.left+UserGetSystemMetrics(SM_CXSIZE)) return HTTOPLEFT;
                if (pt.x >= rcWindow.right-UserGetSystemMetrics(SM_CXSIZE)) return HTTOPRIGHT;
                return HTTOP;
            }
            /* Check bottom sizing border */
            if (pt.y >= rcWindow.bottom)
            {
                if (pt.x < rcWindow.left+UserGetSystemMetrics(SM_CXSIZE)) return HTBOTTOMLEFT;
                if (pt.x >= rcWindow.right-UserGetSystemMetrics(SM_CXSIZE)) return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            /* Check left sizing border */
            if (pt.x < rcWindow.left)
            {
                if (pt.y < rcWindow.top+UserGetSystemMetrics(SM_CYSIZE)) return HTTOPLEFT;
                if (pt.y >= rcWindow.bottom-UserGetSystemMetrics(SM_CYSIZE)) return HTBOTTOMLEFT;
                return HTLEFT;
            }
            /* Check right sizing border */
            if (pt.x >= rcWindow.right)
            {
                if (pt.y < rcWindow.top+UserGetSystemMetrics(SM_CYSIZE)) return HTTOPRIGHT;
                if (pt.y >= rcWindow.bottom-UserGetSystemMetrics(SM_CYSIZE)) return HTBOTTOMRIGHT;
                return HTRIGHT;
            }
        }
    }
    else  /* No thick frame */
    {
        if (HAS_DLGFRAME( Style, ExStyle ))
            RECTL_vInflateRect(&rcWindow, -UserGetSystemMetrics(SM_CXDLGFRAME), -UserGetSystemMetrics(SM_CYDLGFRAME));
        else if (HAS_THINFRAME( Style, ExStyle ))
            RECTL_vInflateRect(&rcWindow, -UserGetSystemMetrics(SM_CXBORDER), -UserGetSystemMetrics(SM_CYBORDER));
        if (!RECTL_bPointInRect( &rcWindow, pt.x, pt.y  )) return HTBORDER;
    }

    /* Check caption */

    if ((Style & WS_CAPTION) == WS_CAPTION)
    {
        if (ExStyle & WS_EX_TOOLWINDOW)
            rcWindow.top += UserGetSystemMetrics(SM_CYSMCAPTION) - 1;
        else
            rcWindow.top += UserGetSystemMetrics(SM_CYCAPTION) - 1;
        if (!RECTL_bPointInRect( &rcWindow, pt.x, pt.y ))
        {
            BOOL min_or_max_box = (Style & WS_SYSMENU) && (Style & (WS_MINIMIZEBOX|WS_MAXIMIZEBOX));
            if (ExStyle & WS_EX_LAYOUTRTL)
            {
                /* Check system menu */
                if ((Style & WS_SYSMENU) && !(ExStyle & WS_EX_TOOLWINDOW) && NC_IconForWindow(pWnd))
                {
                    rcWindow.right -= UserGetSystemMetrics(SM_CYCAPTION) - 1;
                    if (pt.x > rcWindow.right) return HTSYSMENU;
                }

                /* Check close button */
                if (Style & WS_SYSMENU)
                {
                    rcWindow.left += UserGetSystemMetrics(SM_CYCAPTION);
                    if (pt.x < rcWindow.left) return HTCLOSE;
                }

                /* Check maximize box */
                /* In Win95 there is automatically a Maximize button when there is a minimize one */
                if (min_or_max_box && !(ExStyle & WS_EX_TOOLWINDOW))
                {
                    rcWindow.left += UserGetSystemMetrics(SM_CXSIZE);
                    if (pt.x < rcWindow.left) return HTMAXBUTTON;
                }

                /* Check minimize box */
                if (min_or_max_box && !(ExStyle & WS_EX_TOOLWINDOW))
                {
                    rcWindow.left += UserGetSystemMetrics(SM_CXSIZE);
                    if (pt.x < rcWindow.left) return HTMINBUTTON;
                }
            }
            else
            {
                /* Check system menu */
                if ((Style & WS_SYSMENU) && !(ExStyle & WS_EX_TOOLWINDOW) && NC_IconForWindow(pWnd))
                {
                    rcWindow.left += UserGetSystemMetrics(SM_CYCAPTION) - 1;
                    if (pt.x < rcWindow.left) return HTSYSMENU;
                }

                /* Check close button */
                if (Style & WS_SYSMENU)
                {
                    rcWindow.right -= UserGetSystemMetrics(SM_CYCAPTION);
                    if (pt.x > rcWindow.right) return HTCLOSE;
                }

                /* Check maximize box */
                /* In Win95 there is automatically a Maximize button when there is a minimize one */
                if (min_or_max_box && !(ExStyle & WS_EX_TOOLWINDOW))
                {
                    rcWindow.right -= UserGetSystemMetrics(SM_CXSIZE);
                    if (pt.x > rcWindow.right) return HTMAXBUTTON;
                }

                /* Check minimize box */
                if (min_or_max_box && !(ExStyle & WS_EX_TOOLWINDOW))
                {
                    rcWindow.right -= UserGetSystemMetrics(SM_CXSIZE);
                    if (pt.x > rcWindow.right) return HTMINBUTTON;
                }
            }
            return HTCAPTION;
        }
    }

      /* Check menu bar */

    if (HAS_MENU( pWnd, Style ) && (pt.y < rcClient.top) &&
        (pt.x >= rcClient.left) && (pt.x < rcClient.right))
        return HTMENU;

      /* Check vertical scroll bar */

    if (ExStyle & WS_EX_LAYOUTRTL) ExStyle ^= WS_EX_LEFTSCROLLBAR;
    if (Style & WS_VSCROLL)
    {
        if((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
            rcClient.left -= UserGetSystemMetrics(SM_CXVSCROLL);
        else
            rcClient.right += UserGetSystemMetrics(SM_CXVSCROLL);
        if (RECTL_bPointInRect( &rcClient, pt.x, pt.y )) return HTVSCROLL;
    }

      /* Check horizontal scroll bar */

    if (Style & WS_HSCROLL)
    {
        rcClient.bottom += UserGetSystemMetrics(SM_CYHSCROLL);
        if (RECTL_bPointInRect( &rcClient, pt.x, pt.y ))
        {
            /* Check size box */
            if ((Style & WS_VSCROLL) &&
                ((((ExStyle & WS_EX_LEFTSCROLLBAR) != 0) && (pt.x <= rcClient.left + UserGetSystemMetrics(SM_CXVSCROLL))) ||
                (((ExStyle & WS_EX_LEFTSCROLLBAR) == 0) && (pt.x >= rcClient.right - UserGetSystemMetrics(SM_CXVSCROLL)))))
                return HTSIZE;
            return HTHSCROLL;
        }
    }

    /* Has to return HTNOWHERE if nothing was found
       Could happen when a window has a customized non client area */
    return HTNOWHERE;
}

/* EOF */
