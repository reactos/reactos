/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Miscellaneous User functions
 * FILE:             win32ss/user/ntuser/nonclient.c
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

#define HASSIZEGRIP(Style, ExStyle, ParentStyle, WindowRect, ParentClientRect) \
            ((!(Style & WS_CHILD) && (Style & WS_THICKFRAME) && !(Style & WS_MAXIMIZE))  || \
             ((Style & WS_CHILD) && (ParentStyle & WS_THICKFRAME) && !(ParentStyle & WS_MAXIMIZE) && \
             (WindowRect.right - WindowRect.left == ParentClientRect.right) && \
             (WindowRect.bottom - WindowRect.top == ParentClientRect.bottom)))


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
void FASTCALL // Previously known as "UserGetInsideRectNC"
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
    if (UserHasThickFrameStyle(Style, ExStyle))
    {
        RECTL_vInflateRect(rect, -UserGetSystemMetrics(SM_CXFRAME), -UserGetSystemMetrics(SM_CYFRAME));
    }
    else if (UserHasDlgFrameStyle(Style, ExStyle))
    {
        RECTL_vInflateRect(rect, -UserGetSystemMetrics(SM_CXDLGFRAME), -UserGetSystemMetrics(SM_CYDLGFRAME));
        /* FIXME: this isn't in NC_AdjustRect? why not? */
        if (ExStyle & WS_EX_DLGMODALFRAME)
            RECTL_vInflateRect( rect, -1, 0 );
    }
    else if (UserHasThinFrameStyle(Style, ExStyle))
    {
        RECTL_vInflateRect(rect, -UserGetSystemMetrics(SM_CXBORDER), -UserGetSystemMetrics(SM_CYBORDER));
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

/***********************************************************************
 *           NC_GetSysPopupPos
 */
void FASTCALL
NC_GetSysPopupPos(PWND Wnd, RECT *Rect)
{
  RECT WindowRect;

  if ((Wnd->style & WS_MINIMIZE) != 0)
  {
      IntGetWindowRect(Wnd, Rect);
  }
  else
    {
      NC_GetInsideRect(Wnd, Rect);
      IntGetWindowRect(Wnd, &WindowRect);
      RECTL_vOffsetRect(Rect, WindowRect.left, WindowRect.top);
      if (Wnd->style & WS_CHILD)
      {
          IntClientToScreen(IntGetParent(Wnd), (POINT *) Rect);
      }
      Rect->right = Rect->left + UserGetSystemMetrics(SM_CYCAPTION) - 1;
      Rect->bottom = Rect->top + UserGetSystemMetrics(SM_CYCAPTION) - 1;
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
              break;
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
                    HRGN hrgnNew;
                    HRGN hrgnOrig = GreCreateRectRgnIndirect(&pwnd->rcWindow);

                    if (pwnd->hrgnClip != NULL)
                       NtGdiCombineRgn(hrgnOrig, hrgnOrig, pwnd->hrgnClip, RGN_AND);

                    //// This causes the mdi child window to jump up when it is moved.
                    //IntMapWindowPoints( 0, pWndParent, (POINT *)&rect, 2 );
		    co_WinPosSetWindowPos( pwnd,
		                           0,
		                           newRect.left,
		                           newRect.top,
				           newRect.right - newRect.left,
				           newRect.bottom - newRect.top,
				          ( hittest == HTCAPTION ) ? SWP_NOSIZE : 0 );

                    hrgnNew = GreCreateRectRgnIndirect(&pwnd->rcWindow);
                    if (pwnd->hrgnClip != NULL)
                       NtGdiCombineRgn(hrgnNew, hrgnNew, pwnd->hrgnClip, RGN_AND);

                    if (hrgnNew)
                    {
                       if (hrgnOrig)
                          NtGdiCombineRgn(hrgnOrig, hrgnOrig, hrgnNew, RGN_DIFF);
                    }
                    else
                    {
                       if (hrgnOrig)
                       {
                          GreDeleteObject(hrgnOrig);
                          hrgnOrig = 0;
                       }
                    }

                    // Update all the windows after the move or size, including this window.
                    UpdateThreadWindows(UserGetDesktopWindow()->spwndChild, pti, hrgnOrig);

                    if (hrgnOrig) GreDeleteObject(hrgnOrig);
                    if (hrgnNew) GreDeleteObject(hrgnNew);
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
   {
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
}

PCURICON_OBJECT FASTCALL NC_IconForWindow( PWND pWnd )
{
    PCURICON_OBJECT pIcon = NULL;
    HICON hIcon;

   hIcon = UserGetProp(pWnd, gpsi->atomIconSmProp, TRUE);
   if (!hIcon) hIcon = UserGetProp(pWnd, gpsi->atomIconProp, TRUE);

   if (!hIcon && pWnd->pcls->spicnSm)
       return pWnd->pcls->spicnSm;
   if (!hIcon && pWnd->pcls->spicn)
       return pWnd->pcls->spicn;

   // WARNING: Wine code has this test completely wrong. The following is how
   // Windows behaves for windows having the WS_EX_DLGMODALFRAME style set:
   // it does not use the default icon! And it does not check for DS_MODALFRAME.
   if (!hIcon && !(pWnd->ExStyle & WS_EX_DLGMODALFRAME))
   {
      if (!hIcon) hIcon = gpsi->hIconSmWindows; // Both are IDI_WINLOGO Small
      if (!hIcon) hIcon = gpsi->hIconWindows;   // Reg size.
   }
   if (hIcon)
   {
       pIcon = (PCURICON_OBJECT)UserGetObjectNoErr(gHandleTable,
                                                   hIcon,
                                                   TYPE_CURSOR);
   }
   return pIcon;
}

BOOL
UserDrawSysMenuButton(PWND pWnd, HDC hDC, LPRECT Rect, BOOL Down)
{
   PCURICON_OBJECT WindowIcon;
   BOOL Ret = FALSE;

   if ((WindowIcon = NC_IconForWindow(pWnd)))
   {
      UserReferenceObject(WindowIcon);

      Ret = UserDrawIconEx(hDC,
                           Rect->left + 2,
                           Rect->top + 2,
                           WindowIcon,
                           UserGetSystemMetrics(SM_CXSMICON),
                           UserGetSystemMetrics(SM_CYSMICON),
                           0, NULL, DI_NORMAL);

      UserDereferenceObject(WindowIcon);
   }
   return Ret;
}

BOOL
IntIsScrollBarVisible(PWND pWnd, INT hBar)
{
  SCROLLBARINFO sbi;
  sbi.cbSize = sizeof(SCROLLBARINFO);

  if(!co_IntGetScrollBarInfo(pWnd, hBar, &sbi))
    return FALSE;

  return !(sbi.rgstate[0] & STATE_SYSTEM_OFFSCREEN);
}

/*
 * FIXME:
 * - Cache bitmaps, then just bitblt instead of calling DFC() (and
 *   wasting precious CPU cycles) every time
 * - Center the buttons vertically in the rect
 */
VOID
UserDrawCaptionButton(PWND pWnd, LPRECT Rect, DWORD Style, DWORD ExStyle, HDC hDC, BOOL bDown, ULONG Type)
{
   RECT TempRect;

   if (!(Style & WS_SYSMENU))
   {
      return;
   }

   TempRect = *Rect;

   switch (Type)
   {
      case DFCS_CAPTIONMIN:
      {
         if (ExStyle & WS_EX_TOOLWINDOW)
            return; /* ToolWindows don't have min/max buttons */

         if (Style & WS_SYSMENU)
             TempRect.right -= UserGetSystemMetrics(SM_CXSIZE) + 1;

         if (Style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX))
             TempRect.right -= UserGetSystemMetrics(SM_CXSIZE) - 2;

         TempRect.left = TempRect.right - UserGetSystemMetrics(SM_CXSIZE) + 1;
         TempRect.bottom = TempRect.top + UserGetSystemMetrics(SM_CYSIZE) - 2;
         TempRect.top += 2;
         TempRect.right -= 1;

         DrawFrameControl(hDC, &TempRect, DFC_CAPTION,
                          ((Style & WS_MINIMIZE) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMIN) |
                          (bDown ? DFCS_PUSHED : 0) |
                          ((Style & WS_MINIMIZEBOX) ? 0 : DFCS_INACTIVE));
         break;
      }
      case DFCS_CAPTIONMAX:
      {
         if (ExStyle & WS_EX_TOOLWINDOW)
             return; /* ToolWindows don't have min/max buttons */

         if (Style & WS_SYSMENU)
             TempRect.right -= UserGetSystemMetrics(SM_CXSIZE) + 1;

         TempRect.left = TempRect.right - UserGetSystemMetrics(SM_CXSIZE) + 1;
         TempRect.bottom = TempRect.top + UserGetSystemMetrics(SM_CYSIZE) - 2;
         TempRect.top += 2;
         TempRect.right -= 1;

         DrawFrameControl(hDC, &TempRect, DFC_CAPTION,
                          ((Style & WS_MAXIMIZE) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMAX) |
                          (bDown ? DFCS_PUSHED : 0) |
                          ((Style & WS_MAXIMIZEBOX) ? 0 : DFCS_INACTIVE));
         break;
      }
      case DFCS_CAPTIONCLOSE:
      {
          PMENU pSysMenu = IntGetSystemMenu(pWnd, FALSE);
          UINT MenuState = IntGetMenuState(UserHMGetHandle(pSysMenu), SC_CLOSE, MF_BYCOMMAND); /* in case of error MenuState==0xFFFFFFFF */

         /* A tool window has a smaller Close button */
         if (ExStyle & WS_EX_TOOLWINDOW)
         {
            TempRect.left = TempRect.right - UserGetSystemMetrics(SM_CXSMSIZE);
            TempRect.bottom = TempRect.top + UserGetSystemMetrics(SM_CYSMSIZE) - 2;
         }
         else
         {
            TempRect.left = TempRect.right - UserGetSystemMetrics(SM_CXSIZE);
            TempRect.bottom = TempRect.top + UserGetSystemMetrics(SM_CYSIZE) - 2;
         }
         TempRect.top += 2;
         TempRect.right -= 2;

         DrawFrameControl(hDC, &TempRect, DFC_CAPTION,
                          (DFCS_CAPTIONCLOSE | (bDown ? DFCS_PUSHED : 0) |
                          ((!(MenuState & (MF_GRAYED|MF_DISABLED)) && !(pWnd->pcls->style & CS_NOCLOSE)) ? 0 : DFCS_INACTIVE)));
         break;
      }
   }
}

VOID
UserDrawCaptionButtonWnd(PWND pWnd, HDC hDC, BOOL bDown, ULONG Type)
{
   RECT WindowRect;
   SIZE WindowBorder;

   IntGetWindowRect(pWnd, &WindowRect);

   WindowRect.right -= WindowRect.left;
   WindowRect.bottom -= WindowRect.top;
   WindowRect.left = WindowRect.top = 0;

   UserGetWindowBorders(pWnd->style, pWnd->ExStyle, &WindowBorder, FALSE);

   RECTL_vInflateRect(&WindowRect, -WindowBorder.cx, -WindowBorder.cy);

   UserDrawCaptionButton(pWnd, &WindowRect, pWnd->style, pWnd->ExStyle, hDC, bDown, Type);
}

VOID
NC_DrawFrame( HDC hDC, RECT *CurrentRect, BOOL Active, DWORD Style, DWORD ExStyle)
{
   /* Firstly the "thick" frame */
   if ((Style & WS_THICKFRAME) && !(Style & WS_MINIMIZE))
   {
      LONG Width =
         (UserGetSystemMetrics(SM_CXFRAME) - UserGetSystemMetrics(SM_CXDLGFRAME)) *
         UserGetSystemMetrics(SM_CXBORDER);

      LONG Height =
         (UserGetSystemMetrics(SM_CYFRAME) - UserGetSystemMetrics(SM_CYDLGFRAME)) *
         UserGetSystemMetrics(SM_CYBORDER);

      NtGdiSelectBrush(hDC, IntGetSysColorBrush(Active ? COLOR_ACTIVEBORDER : COLOR_INACTIVEBORDER));

      /* Draw frame */
      NtGdiPatBlt(hDC, CurrentRect->left, CurrentRect->top, CurrentRect->right - CurrentRect->left, Height, PATCOPY);
      NtGdiPatBlt(hDC, CurrentRect->left, CurrentRect->top, Width, CurrentRect->bottom - CurrentRect->top, PATCOPY);
      NtGdiPatBlt(hDC, CurrentRect->left, CurrentRect->bottom - 1, CurrentRect->right - CurrentRect->left, -Height, PATCOPY);
      NtGdiPatBlt(hDC, CurrentRect->right - 1, CurrentRect->top, -Width, CurrentRect->bottom - CurrentRect->top, PATCOPY);

      RECTL_vInflateRect(CurrentRect, -Width, -Height);
   }

   /* Now the other bit of the frame */
   if (Style & (WS_DLGFRAME | WS_BORDER) || ExStyle & WS_EX_DLGMODALFRAME)
   {
      DWORD Width = UserGetSystemMetrics(SM_CXBORDER);
      DWORD Height = UserGetSystemMetrics(SM_CYBORDER);

      NtGdiSelectBrush(hDC, IntGetSysColorBrush(
         (ExStyle & (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE)) ? COLOR_3DFACE :
         (ExStyle & WS_EX_STATICEDGE) ? COLOR_WINDOWFRAME :
         (Style & (WS_DLGFRAME | WS_THICKFRAME)) ? COLOR_3DFACE :
         COLOR_WINDOWFRAME));

      /* Draw frame */
      NtGdiPatBlt(hDC, CurrentRect->left, CurrentRect->top, CurrentRect->right - CurrentRect->left, Height, PATCOPY);
      NtGdiPatBlt(hDC, CurrentRect->left, CurrentRect->top, Width, CurrentRect->bottom - CurrentRect->top, PATCOPY);
      NtGdiPatBlt(hDC, CurrentRect->left, CurrentRect->bottom - 1, CurrentRect->right - CurrentRect->left, -Height, PATCOPY);
      NtGdiPatBlt(hDC, CurrentRect->right - 1, CurrentRect->top, -Width, CurrentRect->bottom - CurrentRect->top, PATCOPY);

      RECTL_vInflateRect(CurrentRect, -Width, -Height);
   }
}

VOID UserDrawCaptionBar(
   PWND pWnd,
   HDC hDC,
   INT Flags)
{
   DWORD Style, ExStyle;
   RECT WindowRect, CurrentRect, TempRect;
   HPEN PreviousPen;
   BOOL Gradient = FALSE;
   PCURICON_OBJECT pIcon = NULL;

   if (!(Flags & DC_NOVISIBLE) && !IntIsWindowVisible(pWnd)) return;

   TRACE("UserDrawCaptionBar: pWnd %p, hDc %p, Flags 0x%x.\n", pWnd, hDC, Flags);

   Style = pWnd->style;
   ExStyle = pWnd->ExStyle;

   IntGetWindowRect(pWnd, &WindowRect);

   CurrentRect.top = CurrentRect.left = 0;
   CurrentRect.right = WindowRect.right - WindowRect.left;
   CurrentRect.bottom = WindowRect.bottom - WindowRect.top;

   /* Draw outer edge */
   if (UserHasWindowEdge(Style, ExStyle))
   {
      DrawEdge(hDC, &CurrentRect, EDGE_RAISED, BF_RECT | BF_ADJUST);
   }
   else if (ExStyle & WS_EX_STATICEDGE)
   {
#if 0
      DrawEdge(hDC, &CurrentRect, BDR_SUNKENINNER, BF_RECT | BF_ADJUST | BF_FLAT);
#else
      NtGdiSelectBrush(hDC, IntGetSysColorBrush(COLOR_BTNSHADOW));
      NtGdiPatBlt(hDC, CurrentRect.left, CurrentRect.top, CurrentRect.right - CurrentRect.left, 1, PATCOPY);
      NtGdiPatBlt(hDC, CurrentRect.left, CurrentRect.top, 1, CurrentRect.bottom - CurrentRect.top, PATCOPY);

      NtGdiSelectBrush(hDC, IntGetSysColorBrush(COLOR_BTNHIGHLIGHT));
      NtGdiPatBlt(hDC, CurrentRect.left, CurrentRect.bottom - 1, CurrentRect.right - CurrentRect.left, 1, PATCOPY);
      NtGdiPatBlt(hDC, CurrentRect.right - 1, CurrentRect.top, 1, CurrentRect.bottom - CurrentRect.top, PATCOPY);

      RECTL_vInflateRect(&CurrentRect, -1, -1);
#endif
   }

   if (Flags & DC_FRAME) NC_DrawFrame(hDC, &CurrentRect, (Flags & DC_ACTIVE), Style, ExStyle);

   /* Draw caption */
   if ((Style & WS_CAPTION) == WS_CAPTION)
   {
      TempRect = CurrentRect;

      Flags |= DC_TEXT|DC_BUTTONS; // Icon will be checked if not already set.

      if (UserSystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, &Gradient, 0) && Gradient)
      {
         Flags |= DC_GRADIENT;
      }

      if (ExStyle & WS_EX_TOOLWINDOW)
      {
         Flags |= DC_SMALLCAP;
         TempRect.bottom = TempRect.top + UserGetSystemMetrics(SM_CYSMCAPTION) - 1;
         CurrentRect.top += UserGetSystemMetrics(SM_CYSMCAPTION);
      }
      else
      {
         TempRect.bottom = TempRect.top + UserGetSystemMetrics(SM_CYCAPTION) - 1;
         CurrentRect.top += UserGetSystemMetrics(SM_CYCAPTION);
      }

      if (!(Flags & DC_ICON)               &&
          !(Flags & DC_SMALLCAP)           &&
           (Style & WS_SYSMENU)            &&
          !(ExStyle & WS_EX_TOOLWINDOW) )
      {
         pIcon = NC_IconForWindow(pWnd); // Force redraw of caption with icon if DC_ICON not flaged....
      }
      UserDrawCaption(pWnd, hDC, &TempRect, NULL, pIcon ? UserHMGetHandle(pIcon) : NULL, NULL, Flags);

      /* Draw buttons */
      if (Style & WS_SYSMENU)
      {
         UserDrawCaptionButton(pWnd, &TempRect, Style, ExStyle, hDC, FALSE, DFCS_CAPTIONCLOSE);
         if ((Style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX)) && !(ExStyle & WS_EX_TOOLWINDOW))
         {
            UserDrawCaptionButton(pWnd, &TempRect, Style, ExStyle, hDC, FALSE, DFCS_CAPTIONMIN);
            UserDrawCaptionButton(pWnd, &TempRect, Style, ExStyle, hDC, FALSE, DFCS_CAPTIONMAX);
         }
      }

      if (!(Style & WS_MINIMIZE))
      {
         /* Line under caption */
         PreviousPen = NtGdiSelectPen(hDC, NtGdiGetStockObject(DC_PEN));

         IntSetDCPenColor( hDC, IntGetSysColor(((ExStyle & (WS_EX_STATICEDGE|WS_EX_CLIENTEDGE|WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE) ?
                                             COLOR_WINDOWFRAME : COLOR_3DFACE));

         GreMoveTo(hDC, TempRect.left, TempRect.bottom, NULL);

         NtGdiLineTo(hDC, TempRect.right, TempRect.bottom);

         NtGdiSelectPen(hDC, PreviousPen);
      }
   }

   if (!(Style & WS_MINIMIZE))
   {
      /* Draw menu bar */
      if (pWnd->state & WNDS_HASMENU && pWnd->IDMenu) // Should be pWnd->spmenu
      {
          PMENU menu = UserGetMenuObject(UlongToHandle(pWnd->IDMenu)); // FIXME!
          TempRect = CurrentRect;
          TempRect.bottom = TempRect.top + menu->cyMenu; // Should be pWnd->spmenu->cyMenu;
          CurrentRect.top += MENU_DrawMenuBar(hDC, &TempRect, pWnd, FALSE);
      }

      if (ExStyle & WS_EX_CLIENTEDGE)
      {
          DrawEdge(hDC, &CurrentRect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
      }
   }
}

// Note from Wine:
/* MSDN docs are pretty idiotic here, they say app CAN use clipRgn in
   the call to GetDCEx implying that it is allowed not to use it either.
   However, the suggested GetDCEx(    , DCX_WINDOW | DCX_INTERSECTRGN)
   will cause clipRgn to be deleted after ReleaseDC().
   Now, how is the "system" supposed to tell what happened?
 */
/*
 * FIXME:
 * - Drawing of WS_BORDER after scrollbars
 * - Correct drawing of size-box
 */
LRESULT
NC_DoNCPaint(PWND pWnd, HDC hDC, INT Flags)
{
   DWORD Style, ExStyle;
   PWND Parent;
   RECT WindowRect, CurrentRect, TempRect;
   BOOL Active = FALSE;

   if (!IntIsWindowVisible(pWnd) ||
       (pWnd->state & WNDS_NONCPAINT && !(pWnd->state & WNDS_FORCEMENUDRAW)) ||
        IntEqualRect(&pWnd->rcWindow, &pWnd->rcClient) )
      return 0;

   Style = pWnd->style;

   TRACE("DefWndNCPaint: pWnd %p, hDc %p, Active %s.\n", pWnd, hDC, Flags & DC_ACTIVE ? "TRUE" : "FALSE");

   Parent = IntGetParent(pWnd);
   ExStyle = pWnd->ExStyle;

   if (Flags == -1) // NC paint mode.
   {
      if (ExStyle & WS_EX_MDICHILD)
      {
         Active = IntIsChildWindow(gpqForeground->spwndActive, pWnd);

         if (Active)
            Active = (UserHMGetHandle(pWnd) == (HWND)co_IntSendMessage(UserHMGetHandle(Parent), WM_MDIGETACTIVE, 0, 0));
      }
      else
      {
         Active = (gpqForeground == pWnd->head.pti->MessageQueue);
      }
      Flags = DC_NC; // Redraw everything!
   }
   else
      Flags |= DC_NC;


   IntGetWindowRect(pWnd, &WindowRect);

   CurrentRect.top = CurrentRect.left = 0;
   CurrentRect.right = WindowRect.right - WindowRect.left;
   CurrentRect.bottom = WindowRect.bottom - WindowRect.top;

   /* Draw outer edge */
   if (UserHasWindowEdge(pWnd->style, pWnd->ExStyle))
   {
      DrawEdge(hDC, &CurrentRect, EDGE_RAISED, BF_RECT | BF_ADJUST);
   }
   else if (pWnd->ExStyle & WS_EX_STATICEDGE)
   {
#if 0
      DrawEdge(hDC, &CurrentRect, BDR_SUNKENINNER, BF_RECT | BF_ADJUST | BF_FLAT);
#else
      NtGdiSelectBrush(hDC, IntGetSysColorBrush(COLOR_BTNSHADOW));
      NtGdiPatBlt(hDC, CurrentRect.left, CurrentRect.top, CurrentRect.right - CurrentRect.left, 1, PATCOPY);
      NtGdiPatBlt(hDC, CurrentRect.left, CurrentRect.top, 1, CurrentRect.bottom - CurrentRect.top, PATCOPY);

      NtGdiSelectBrush(hDC, IntGetSysColorBrush(COLOR_BTNHIGHLIGHT));
      NtGdiPatBlt(hDC, CurrentRect.left, CurrentRect.bottom - 1, CurrentRect.right - CurrentRect.left, 1, PATCOPY);
      NtGdiPatBlt(hDC, CurrentRect.right - 1, CurrentRect.top, 1, CurrentRect.bottom - CurrentRect.top, PATCOPY);

      RECTL_vInflateRect(&CurrentRect, -1, -1);
#endif
   }

   if (Flags & DC_FRAME) NC_DrawFrame(hDC, &CurrentRect, Active ? Active : (Flags & DC_ACTIVE), Style, ExStyle);

   /* Draw caption */
   if ((Style & WS_CAPTION) == WS_CAPTION)
   {
      HPEN PreviousPen;
      BOOL Gradient = FALSE;

      if (Flags & DC_REDRAWHUNGWND)
      {
         Flags &= ~DC_REDRAWHUNGWND;
         Flags |= DC_NOSENDMSG;
      }

      if (UserSystemParametersInfo(SPI_GETGRADIENTCAPTIONS, 0, &Gradient, 0) && Gradient)
      {
         Flags |= DC_GRADIENT;
      }

      if (Active)
      {
         if (pWnd->state & WNDS_ACTIVEFRAME)
            Flags |= DC_ACTIVE;
         else
         {
            ERR("Wnd is active and not set active!\n");
         }
      }

      TempRect = CurrentRect;

      if (ExStyle & WS_EX_TOOLWINDOW)
      {
         Flags |= DC_SMALLCAP;
         TempRect.bottom = TempRect.top + UserGetSystemMetrics(SM_CYSMCAPTION) - 1;
         CurrentRect.top += UserGetSystemMetrics(SM_CYSMCAPTION);
      }
      else
      {
         TempRect.bottom = TempRect.top + UserGetSystemMetrics(SM_CYCAPTION) - 1;
         CurrentRect.top += UserGetSystemMetrics(SM_CYCAPTION);
      }

      UserDrawCaption(pWnd, hDC, &TempRect, NULL, NULL, NULL, Flags);

      /* Draw buttons */
      if (Style & WS_SYSMENU)
      {
         UserDrawCaptionButton(pWnd, &TempRect, Style, ExStyle, hDC, FALSE, DFCS_CAPTIONCLOSE);
         if ((Style & (WS_MAXIMIZEBOX | WS_MINIMIZEBOX)) && !(ExStyle & WS_EX_TOOLWINDOW))
         {
            UserDrawCaptionButton(pWnd, &TempRect, Style, ExStyle, hDC, FALSE, DFCS_CAPTIONMIN);
            UserDrawCaptionButton(pWnd, &TempRect, Style, ExStyle, hDC, FALSE, DFCS_CAPTIONMAX);
         }
      }
      if (!(Style & WS_MINIMIZE))
      {
        /* Line under caption */
         PreviousPen = NtGdiSelectPen(hDC, NtGdiGetStockObject(DC_PEN));

         IntSetDCPenColor( hDC, IntGetSysColor(
                         ((ExStyle & (WS_EX_STATICEDGE | WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE) ?
                          COLOR_WINDOWFRAME : COLOR_3DFACE));

         GreMoveTo(hDC, TempRect.left, TempRect.bottom, NULL);

         NtGdiLineTo(hDC, TempRect.right, TempRect.bottom);

         NtGdiSelectPen(hDC, PreviousPen);
      }
   }

   if (!(Style & WS_MINIMIZE))
   {
     /* Draw menu bar */
     if (pWnd->state & WNDS_HASMENU && pWnd->IDMenu) // Should be pWnd->spmenu
     {
         if (!(Flags & DC_NOSENDMSG))
         {
             PMENU menu = UserGetMenuObject(UlongToHandle(pWnd->IDMenu)); // FIXME!
             TempRect = CurrentRect;
             TempRect.bottom = TempRect.top + menu->cyMenu; // Should be pWnd->spmenu->cyMenu;
             CurrentRect.top += MENU_DrawMenuBar(hDC, &TempRect, pWnd, FALSE);
         }
     }

     if (ExStyle & WS_EX_CLIENTEDGE)
     {
         DrawEdge(hDC, &CurrentRect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);
     }

     /* Draw the scrollbars */
     if ((Style & WS_VSCROLL) && (Style & WS_HSCROLL) &&
         IntIsScrollBarVisible(pWnd, OBJID_VSCROLL) && IntIsScrollBarVisible(pWnd, OBJID_HSCROLL))
     {
        RECT ParentClientRect;

        TempRect = CurrentRect;

        if (ExStyle & WS_EX_LEFTSCROLLBAR)
           TempRect.right = TempRect.left + UserGetSystemMetrics(SM_CXVSCROLL);
        else
           TempRect.left = TempRect.right - UserGetSystemMetrics(SM_CXVSCROLL);

        TempRect.top = TempRect.bottom - UserGetSystemMetrics(SM_CYHSCROLL);

        FillRect(hDC, &TempRect, IntGetSysColorBrush(COLOR_BTNFACE));

        if (Parent)
           IntGetClientRect(Parent, &ParentClientRect);

        if (HASSIZEGRIP(Style, ExStyle, Parent->style, WindowRect, ParentClientRect))
        {
           DrawFrameControl(hDC, &TempRect, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
        }

        IntDrawScrollBar(pWnd, hDC, SB_VERT);
        IntDrawScrollBar(pWnd, hDC, SB_HORZ);
     }
     else
     {
        if (Style & WS_VSCROLL && IntIsScrollBarVisible(pWnd, OBJID_VSCROLL))
        {
           IntDrawScrollBar(pWnd, hDC, SB_VERT);
        }
        else if (Style & WS_HSCROLL && IntIsScrollBarVisible(pWnd, OBJID_HSCROLL))
        {
           IntDrawScrollBar(pWnd, hDC, SB_HORZ);
        }
     }
   }
   return 0; // For WM_NCPAINT message, return 0.
}

LRESULT NC_HandleNCCalcSize( PWND Wnd, WPARAM wparam, RECTL *Rect, BOOL Suspended )
{
   LRESULT Result = 0;
   SIZE WindowBorders;
   RECT OrigRect;
   LONG Style = Wnd->style;
   LONG  exStyle = Wnd->ExStyle; 

   if (Rect == NULL)
   {
      return Result;
   }
   OrigRect = *Rect;

   Wnd->state &= ~WNDS_HASCAPTION;

   if (wparam)
   {
      if (Wnd->pcls->style & CS_VREDRAW)
      {
         Result |= WVR_VREDRAW;
      }
      if (Wnd->pcls->style & CS_HREDRAW)
      {
         Result |= WVR_HREDRAW;
      }
      Result |= WVR_VALIDRECTS;
   }

   if (!(Wnd->style & WS_MINIMIZE))
   {
      if (UserHasWindowEdge(Wnd->style, Wnd->ExStyle))
      {
         UserGetWindowBorders(Wnd->style, Wnd->ExStyle, &WindowBorders, FALSE);
         RECTL_vInflateRect(Rect, -WindowBorders.cx, -WindowBorders.cy);
      }
      else if ((Wnd->ExStyle & WS_EX_STATICEDGE) || (Wnd->style & WS_BORDER))
      {
         RECTL_vInflateRect(Rect, -1, -1);
      }

      if ((Wnd->style & WS_CAPTION) == WS_CAPTION)
      {
         Wnd->state |= WNDS_HASCAPTION;

         if (Wnd->ExStyle & WS_EX_TOOLWINDOW)
            Rect->top += UserGetSystemMetrics(SM_CYSMCAPTION);
         else
            Rect->top += UserGetSystemMetrics(SM_CYCAPTION);
      }

      if (HAS_MENU(Wnd, Style))
      {
         HDC hDC = UserGetDCEx(Wnd, 0, DCX_USESTYLE | DCX_WINDOW);

         Wnd->state |= WNDS_HASMENU;

         if (hDC)
         {
           RECT CliRect = *Rect;
           CliRect.bottom -= OrigRect.top;
           CliRect.right -= OrigRect.left;
           CliRect.left -= OrigRect.left;
           CliRect.top -= OrigRect.top;
           if (!Suspended) Rect->top += MENU_DrawMenuBar(hDC, &CliRect, Wnd, TRUE);
           UserReleaseDC(Wnd, hDC, FALSE);
         }
      }

      if (Wnd->ExStyle & WS_EX_CLIENTEDGE)
      {
         RECTL_vInflateRect(Rect, -2 * UserGetSystemMetrics(SM_CXBORDER), -2 * UserGetSystemMetrics(SM_CYBORDER));
      }

      if (Style & WS_VSCROLL)
      {
         if (Rect->right - Rect->left >= UserGetSystemMetrics(SM_CXVSCROLL))
         {
             Wnd->state |= WNDS_HASVERTICALSCROOLLBAR;

             /* rectangle is in screen coords when wparam is false */
             if (!wparam && (exStyle & WS_EX_LAYOUTRTL)) exStyle ^= WS_EX_LEFTSCROLLBAR;

             if((exStyle & WS_EX_LEFTSCROLLBAR) != 0)
                 Rect->left  += UserGetSystemMetrics(SM_CXVSCROLL);
             else
                 Rect->right -= UserGetSystemMetrics(SM_CXVSCROLL);
          }
      }

      if (Style & WS_HSCROLL)
      {
         if( Rect->bottom - Rect->top > UserGetSystemMetrics(SM_CYHSCROLL))
         {
              Wnd->state |= WNDS_HASHORIZONTALSCROLLBAR;

              Rect->bottom -= UserGetSystemMetrics(SM_CYHSCROLL);
         }
      }

      if (Rect->top > Rect->bottom)
         Rect->bottom = Rect->top;

      if (Rect->left > Rect->right)
         Rect->right = Rect->left;
   }
   else
   {
      Rect->right = Rect->left;
      Rect->bottom = Rect->top;
   }

   return Result;
}

static
INT NC_DoNCActive(PWND Wnd)
{
   INT Ret = 0;

   if ( IntGetSysColor(COLOR_CAPTIONTEXT) != IntGetSysColor(COLOR_INACTIVECAPTIONTEXT) ||
        IntGetSysColor(COLOR_ACTIVECAPTION) != IntGetSysColor(COLOR_INACTIVECAPTION) )
       Ret = DC_CAPTION;

   if (!(Wnd->style & WS_MINIMIZED) && UserHasThickFrameStyle(Wnd->style, Wnd->ExStyle))
   {
      //if (IntGetSysColor(COLOR_ACTIVEBORDER) != IntGetSysColor(COLOR_INACTIVEBORDER)) // Why are these the same?
      {
          Ret = DC_FRAME;
      }
   }
   return Ret;
}

LRESULT NC_HandleNCActivate( PWND Wnd, WPARAM wParam, LPARAM lParam )
{
   INT Flags;
  /* Lotus Notes draws menu descriptions in the caption of its main
   * window. When it wants to restore original "system" view, it just
   * sends WM_NCACTIVATE message to itself. Any optimizations here in
   * attempt to minimize redrawings lead to a not restored caption.
   */
   if (wParam & DC_ACTIVE)
   {
      Wnd->state |= WNDS_ACTIVEFRAME|WNDS_HASCAPTION;
      wParam = DC_CAPTION|DC_ACTIVE;
   }
   else
   {
      Wnd->state &= ~WNDS_ACTIVEFRAME;
      wParam = DC_CAPTION;
   }

   if ((Wnd->state & WNDS_NONCPAINT) || !(Wnd->style & WS_VISIBLE))
      return TRUE;

   /* This isn't documented but is reproducible in at least XP SP2 and
    * Outlook 2007 depends on it
    */
   // MSDN:
   // If this parameter is set to -1, DefWindowProc does not repaint the
   // nonclient area to reflect the state change.
   if ( lParam != -1 &&
        ( Flags = NC_DoNCActive(Wnd)) != 0 )
   {
      HDC hDC;
      HRGN hRgnTemp = NULL, hRgn = (HRGN)lParam;

      if (GreIsHandleValid(hRgn))
      {
         hRgnTemp = NtGdiCreateRectRgn(0, 0, 0, 0);
         if (NtGdiCombineRgn(hRgnTemp, hRgn, 0, RGN_COPY) == ERROR)
         {
            GreDeleteObject(hRgnTemp);
            hRgnTemp = NULL;
         }
      }

      if ((hDC = UserGetDCEx(Wnd, hRgnTemp, DCX_WINDOW|DCX_USESTYLE)))
      {
         NC_DoNCPaint(Wnd, hDC, wParam | Flags); // Redraw MENUs.
         UserReleaseDC(Wnd, hDC, FALSE);
      }
      else
         GreDeleteObject(hRgnTemp);
   }

   return TRUE;
}

VOID
NC_DoButton(PWND pWnd, WPARAM wParam, LPARAM lParam)
{
   MSG Msg;
   HDC WindowDC;
   BOOL Pressed = TRUE, OldState;
   WPARAM SCMsg;
   PMENU SysMenu;
   ULONG ButtonType;
   DWORD Style;
   UINT MenuState;

   Style = pWnd->style;
   switch (wParam)
   {
      case HTCLOSE:
         SysMenu = IntGetSystemMenu(pWnd, FALSE);
         MenuState = IntGetMenuState(UserHMGetHandle(SysMenu), SC_CLOSE, MF_BYCOMMAND); /* in case of error MenuState==0xFFFFFFFF */
         if (!(Style & WS_SYSMENU) || (MenuState & (MF_GRAYED|MF_DISABLED)) || (pWnd->style & CS_NOCLOSE))
            return;
         ButtonType = DFCS_CAPTIONCLOSE;
         SCMsg = SC_CLOSE;
         break;
      case HTMINBUTTON:
         if (!(Style & WS_MINIMIZEBOX))
            return;
         ButtonType = DFCS_CAPTIONMIN;
         SCMsg = ((Style & WS_MINIMIZE) ? SC_RESTORE : SC_MINIMIZE);
         break;
      case HTMAXBUTTON:
         if (!(Style & WS_MAXIMIZEBOX))
            return;
         ButtonType = DFCS_CAPTIONMAX;
         SCMsg = ((Style & WS_MAXIMIZE) ? SC_RESTORE : SC_MAXIMIZE);
         break;

      default:
         ASSERT(FALSE);
         return;
   }

   /*
    * FIXME: Not sure where to do this, but we must flush the pending
    * window updates when someone clicks on the close button and at
    * the same time the window is overlapped with another one. This
    * looks like a good place for now...
    */
   co_IntUpdateWindows(pWnd, RDW_ALLCHILDREN, FALSE);

   WindowDC = UserGetWindowDC(pWnd);
   UserDrawCaptionButtonWnd(pWnd, WindowDC, TRUE, ButtonType);

   co_UserSetCapture(UserHMGetHandle(pWnd));

   for (;;)
   {
      if (!co_IntGetPeekMessage(&Msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE, TRUE)) break;
      if (IntCallMsgFilter( &Msg, MSGF_MAX )) continue;

      if (Msg.message == WM_LBUTTONUP)
         break;

      if (Msg.message != WM_MOUSEMOVE)
         continue;

      OldState = Pressed;
      Pressed = (GetNCHitEx(pWnd, Msg.pt) == wParam);
      if (Pressed != OldState)
         UserDrawCaptionButtonWnd(pWnd, WindowDC, Pressed, ButtonType);
   }

   if (Pressed)
      UserDrawCaptionButtonWnd(pWnd, WindowDC, FALSE, ButtonType);
   IntReleaseCapture();
   UserReleaseDC(pWnd, WindowDC, FALSE);
   if (Pressed)
      co_IntSendMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SCMsg, SCMsg == SC_CLOSE ? lParam : MAKELONG(Msg.pt.x,Msg.pt.y));
}


LRESULT
NC_HandleNCLButtonDown(PWND pWnd, WPARAM wParam, LPARAM lParam)
{
    switch (wParam)
    {
        case HTCAPTION:
        {
            PWND TopWnd = pWnd, parent;
            while(1)
            {
                if ((TopWnd->style & (WS_POPUP|WS_CHILD)) != WS_CHILD)
                    break;
                parent = UserGetAncestor( TopWnd, GA_PARENT );
                if (!parent || UserIsDesktopWindow(parent)) break;
                TopWnd = parent;
            }

            if ( co_IntSetForegroundWindowMouse(TopWnd) ||
                 //NtUserCallHwndLock(hTopWnd, HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOWMOUSE) ||
                 UserGetActiveWindow() == UserHMGetHandle(TopWnd))
            {
               co_IntSendMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SC_MOVE + HTCAPTION, lParam);
            }
            break;
        }
        case HTSYSMENU:
        {
          LONG style = pWnd->style;
          if (style & WS_SYSMENU)
          {
              if(!(style & WS_MINIMIZE) )
              {
                RECT rect;
                HDC hDC = UserGetWindowDC(pWnd);
                NC_GetInsideRect(pWnd, &rect);
                UserDrawSysMenuButton(pWnd, hDC, &rect, TRUE);
                UserReleaseDC( pWnd, hDC, FALSE );
              }
	      co_IntSendMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SC_MOUSEMENU + HTSYSMENU, lParam);
	  }
	  break;
        }
        case HTMENU:
        {
            co_IntSendMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SC_MOUSEMENU + HTMENU, lParam);
            break;
        }
        case HTHSCROLL:
        {
            co_IntSendMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lParam);
            break;
        }
        case HTVSCROLL:
        {
            co_IntSendMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lParam);
            break;
        }
        case HTMINBUTTON:
        case HTMAXBUTTON:
        case HTCLOSE:
        {
          NC_DoButton(pWnd, wParam, lParam);
          break;
        }
        case HTLEFT:
        case HTRIGHT:
        case HTTOP:
        case HTBOTTOM:
        case HTTOPLEFT:
        case HTTOPRIGHT:
        case HTBOTTOMLEFT:
        case HTBOTTOMRIGHT:
        {
           /* Old comment:
            * "make sure hittest fits into 0xf and doesn't overlap with HTSYSMENU"
            * This was previously done by setting wParam=SC_SIZE + wParam - 2
            */
           /* But that is not what WinNT does. Instead it sends this. This
            * is easy to differentiate from HTSYSMENU, because HTSYSMENU adds
            * SC_MOUSEMENU into wParam.
            */
            co_IntSendMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SC_SIZE + wParam - (HTLEFT - WMSZ_LEFT), lParam);
            break;
        }
        case HTBORDER:
            break;
    }
    return(0);
}


LRESULT
NC_HandleNCLButtonDblClk(PWND pWnd, WPARAM wParam, LPARAM lParam)
{
  ULONG Style;

  Style = pWnd->style;
  switch(wParam)
  {
    case HTCAPTION:
    {
      /* Maximize/Restore the window */
      if((Style & WS_CAPTION) == WS_CAPTION && (Style & WS_MAXIMIZEBOX))
      {
        co_IntSendMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, ((Style & (WS_MINIMIZE | WS_MAXIMIZE)) ? SC_RESTORE : SC_MAXIMIZE), 0);
      }
      break;
    }
    case HTSYSMENU:
    {
      PMENU SysMenu = IntGetSystemMenu(pWnd, FALSE);
      UINT state = IntGetMenuState(UserHMGetHandle(SysMenu), SC_CLOSE, MF_BYCOMMAND);
                  
      /* If the close item of the sysmenu is disabled or not present do nothing */
      if ((state & (MF_DISABLED | MF_GRAYED)) || (state == 0xFFFFFFFF))
          break;

      co_IntSendMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SC_CLOSE, lParam);
      break;
    }
    default:
      return NC_HandleNCLButtonDown(pWnd, wParam, lParam);
  }
  return(0);
}

/***********************************************************************
 *           NC_HandleNCRButtonDown
 *
 * Handle a WM_NCRBUTTONDOWN message. Called from DefWindowProc().
 */
LRESULT NC_HandleNCRButtonDown( PWND pwnd, WPARAM wParam, LPARAM lParam ) 
{
  MSG msg;
  INT hittest = wParam;

  switch (hittest)
  {
  case HTCAPTION:
  case HTSYSMENU:
      if (!IntGetSystemMenu( pwnd, FALSE )) break;

      co_UserSetCapture( UserHMGetHandle(pwnd) );
      for (;;)
      {
          if (!co_IntGetPeekMessage(&msg, 0, WM_MOUSEFIRST, WM_MOUSELAST, PM_REMOVE, TRUE)) break;
          if (IntCallMsgFilter( &msg, MSGF_MAX )) continue;
          if (msg.message == WM_RBUTTONUP)
          {
             hittest = GetNCHitEx( pwnd, msg.pt );
             break;
          }
          if (UserHMGetHandle(pwnd) != IntGetCapture()) return 0;
      }
      IntReleaseCapture();
      if (hittest == HTCAPTION || hittest == HTSYSMENU || hittest == HTHSCROLL || hittest == HTVSCROLL)
      {
         TRACE("Msg pt %x and Msg.lParam %x and lParam %x\n",MAKELONG(msg.pt.x,msg.pt.y),msg.lParam,lParam);
         co_IntSendMessage( UserHMGetHandle(pwnd), WM_CONTEXTMENU, (WPARAM)UserHMGetHandle(pwnd), MAKELONG(msg.pt.x,msg.pt.y));
      }
      break;
  }
  return 0;
}


#if 0 // Old version, kept there for reference, which is also used
      // almost unmodified in uxtheme.dll (in nonclient.c)
/*
 * FIXME:
 * - Check the scrollbar handling
 */
LRESULT
DefWndNCHitTest(HWND hWnd, POINT Point)
{
   RECT WindowRect, ClientRect, OrigWndRect;
   POINT ClientPoint;
   SIZE WindowBorders;
   DWORD Style = GetWindowLongPtrW(hWnd, GWL_STYLE);
   DWORD ExStyle = GetWindowLongPtrW(hWnd, GWL_EXSTYLE);

   GetWindowRect(hWnd, &WindowRect);
   if (!PtInRect(&WindowRect, Point))
   {
      return HTNOWHERE;
   }
   OrigWndRect = WindowRect;

   if (UserHasWindowEdge(Style, ExStyle))
   {
      LONG XSize, YSize;

      UserGetWindowBorders(Style, ExStyle, &WindowBorders, FALSE);
      InflateRect(&WindowRect, -WindowBorders.cx, -WindowBorders.cy);
      XSize = GetSystemMetrics(SM_CXSIZE) * GetSystemMetrics(SM_CXBORDER);
      YSize = GetSystemMetrics(SM_CYSIZE) * GetSystemMetrics(SM_CYBORDER);
      if (!PtInRect(&WindowRect, Point))
      {
         BOOL ThickFrame;

         ThickFrame = (Style & WS_THICKFRAME);
         if (Point.y < WindowRect.top)
         {
            if(Style & WS_MINIMIZE)
              return HTCAPTION;
            if(!ThickFrame)
              return HTBORDER;
            if (Point.x < (WindowRect.left + XSize))
               return HTTOPLEFT;
            if (Point.x >= (WindowRect.right - XSize))
               return HTTOPRIGHT;
            return HTTOP;
         }
         if (Point.y >= WindowRect.bottom)
         {
            if(Style & WS_MINIMIZE)
              return HTCAPTION;
            if(!ThickFrame)
              return HTBORDER;
            if (Point.x < (WindowRect.left + XSize))
               return HTBOTTOMLEFT;
            if (Point.x >= (WindowRect.right - XSize))
               return HTBOTTOMRIGHT;
            return HTBOTTOM;
         }
         if (Point.x < WindowRect.left)
         {
            if(Style & WS_MINIMIZE)
              return HTCAPTION;
            if(!ThickFrame)
              return HTBORDER;
            if (Point.y < (WindowRect.top + YSize))
               return HTTOPLEFT;
            if (Point.y >= (WindowRect.bottom - YSize))
               return HTBOTTOMLEFT;
            return HTLEFT;
         }
         if (Point.x >= WindowRect.right)
         {
            if(Style & WS_MINIMIZE)
              return HTCAPTION;
            if(!ThickFrame)
              return HTBORDER;
            if (Point.y < (WindowRect.top + YSize))
               return HTTOPRIGHT;
            if (Point.y >= (WindowRect.bottom - YSize))
               return HTBOTTOMRIGHT;
            return HTRIGHT;
         }
      }
   }
   else
   {
      if (ExStyle & WS_EX_STATICEDGE)
         InflateRect(&WindowRect,
            -GetSystemMetrics(SM_CXBORDER),
            -GetSystemMetrics(SM_CYBORDER));
      if (!PtInRect(&WindowRect, Point))
         return HTBORDER;
   }

   if ((Style & WS_CAPTION) == WS_CAPTION)
   {
      if (ExStyle & WS_EX_TOOLWINDOW)
         WindowRect.top += GetSystemMetrics(SM_CYSMCAPTION);
      else
         WindowRect.top += GetSystemMetrics(SM_CYCAPTION);
      if (!PtInRect(&WindowRect, Point))
      {
         if (Style & WS_SYSMENU)
         {
            if (ExStyle & WS_EX_TOOLWINDOW)
            {
               WindowRect.right -= GetSystemMetrics(SM_CXSMSIZE);
            }
            else
            {
               // if(!(ExStyle & WS_EX_DLGMODALFRAME))
               // FIXME: The real test should check whether there is
               // an icon for the system window, and if so, do the
               // rect.left increase.
               // See dll/win32/uxtheme/nonclient.c!DefWndNCHitTest
               // and win32ss/user/ntuser/nonclient.c!GetNCHitEx which does
               // the test better.
                  WindowRect.left += GetSystemMetrics(SM_CXSIZE);
               WindowRect.right -= GetSystemMetrics(SM_CXSIZE);
            }
         }
         if (Point.x < WindowRect.left)
            return HTSYSMENU;
         if (WindowRect.right <= Point.x)
            return HTCLOSE;
         if (Style & WS_MAXIMIZEBOX || Style & WS_MINIMIZEBOX)
            WindowRect.right -= GetSystemMetrics(SM_CXSIZE);
         if (Point.x >= WindowRect.right)
            return HTMAXBUTTON;
         if (Style & WS_MINIMIZEBOX)
            WindowRect.right -= GetSystemMetrics(SM_CXSIZE);
         if (Point.x >= WindowRect.right)
            return HTMINBUTTON;
         return HTCAPTION;
      }
   }

   if(!(Style & WS_MINIMIZE))
   {
     ClientPoint = Point;
     ScreenToClient(hWnd, &ClientPoint);
     GetClientRect(hWnd, &ClientRect);

     if (PtInRect(&ClientRect, ClientPoint))
     {
        return HTCLIENT;
     }

     if (GetMenu(hWnd) && !(Style & WS_CHILD))
     {
        if (Point.x > 0 && Point.x < WindowRect.right && ClientPoint.y < 0)
           return HTMENU;
     }

     if (ExStyle & WS_EX_CLIENTEDGE)
     {
        InflateRect(&WindowRect, -2 * GetSystemMetrics(SM_CXBORDER),
           -2 * GetSystemMetrics(SM_CYBORDER));
     }

     if ((Style & WS_VSCROLL) && (Style & WS_HSCROLL) &&
         (WindowRect.bottom - WindowRect.top) > GetSystemMetrics(SM_CYHSCROLL))
     {
        RECT ParentRect, TempRect = WindowRect, TempRect2 = WindowRect;
        HWND Parent = GetParent(hWnd);

        TempRect.bottom -= GetSystemMetrics(SM_CYHSCROLL);
        if ((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
           TempRect.right = TempRect.left + GetSystemMetrics(SM_CXVSCROLL);
        else
           TempRect.left = TempRect.right - GetSystemMetrics(SM_CXVSCROLL);
        if (PtInRect(&TempRect, Point))
           return HTVSCROLL;

        TempRect2.top = TempRect2.bottom - GetSystemMetrics(SM_CYHSCROLL);
        if ((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
           TempRect2.left += GetSystemMetrics(SM_CXVSCROLL);
        else
           TempRect2.right -= GetSystemMetrics(SM_CXVSCROLL);
        if (PtInRect(&TempRect2, Point))
           return HTHSCROLL;

        TempRect.top = TempRect2.top;
        TempRect.bottom = TempRect2.bottom;
        if(Parent)
          GetClientRect(Parent, &ParentRect);
        if (PtInRect(&TempRect, Point) && HASSIZEGRIP(Style, ExStyle,
            GetWindowLongPtrW(Parent, GWL_STYLE), OrigWndRect, ParentRect))
        {
           if ((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
              return HTBOTTOMLEFT;
           else
              return HTBOTTOMRIGHT;
        }
     }
     else
     {
        if (Style & WS_VSCROLL)
        {
           RECT TempRect = WindowRect;

           if ((ExStyle & WS_EX_LEFTSCROLLBAR) != 0)
              TempRect.right = TempRect.left + GetSystemMetrics(SM_CXVSCROLL);
           else
              TempRect.left = TempRect.right - GetSystemMetrics(SM_CXVSCROLL);
           if (PtInRect(&TempRect, Point))
              return HTVSCROLL;
        } else
        if (Style & WS_HSCROLL)
        {
           RECT TempRect = WindowRect;
           TempRect.top = TempRect.bottom - GetSystemMetrics(SM_CYHSCROLL);
           if (PtInRect(&TempRect, Point))
              return HTHSCROLL;
        }
     }
   }

   return HTNOWHERE;
}
#endif

DWORD FASTCALL
GetNCHitEx(PWND pWnd, POINT pt)
{
   RECT rcWindow, rcClient;
   DWORD Style, ExStyle;

   if (!pWnd) return HTNOWHERE;

   if (UserIsDesktopWindow(pWnd))
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
        else if (HAS_CLIENTFRAME( Style, ExStyle ))
            RECTL_vInflateRect(&rcWindow, -UserGetSystemMetrics(SM_CXEDGE), -UserGetSystemMetrics(SM_CYEDGE));
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
