/*
 * Window position related functions.
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *                       1995, 1996 Alex Korobka
 */
#include <windows.h>
#include <string.h>
#include <user32/sysmetr.h>
#include <user32/caret.h>
#include <user32/win.h>
#include <user32/queue.h>
#include <user32/winpos.h>
#include <user32/hook.h>
#include <user32/dce.h>
#include <user32/nc.h>
#include <user32/paint.h>
#include <user32/debug.h>

HWND GetSysModalWindow(void);


extern MESSAGEQUEUE* pActiveQueue;


void STDCALL SwitchToThisWindow( HWND hwnd, WINBOOL restore )
{
    ShowWindow( hwnd, restore ? SW_RESTORE : SW_SHOWMINIMIZED );
}


WINBOOL STDCALL ClientToScreen( HWND hwnd, LPPOINT lppnt )
{
    return MapWindowPoints( hwnd, 0, lppnt, 1 );
   
}


WINBOOL STDCALL ScreenToClient( HWND hwnd, LPPOINT lppnt )
{
    return MapWindowPoints( 0, hwnd, lppnt, 1 );
}


HWND STDCALL WindowFromPoint( POINT pt )
{
    WND *pWnd;

    WINPOS_WindowFromPoint( WIN_GetDesktop(), pt, &pWnd );
    return (HWND)pWnd->hwndSelf;
}



HWND STDCALL ChildWindowFromPoint( HWND hwndParent, POINT pt )
{
    /* pt is in the client coordinates */

    WND* wnd = WIN_FindWndPtr(hwndParent);
    RECT rect;

    if( !wnd ) return 0;

    /* get client rect fast */
    rect.top = rect.left = 0;
    rect.right = wnd->rectClient.right - wnd->rectClient.left;
    rect.bottom = wnd->rectClient.bottom - wnd->rectClient.top;

    if (!PtInRect( &rect, pt )) return 0;

    wnd = wnd->child;
    while ( wnd )
    {
        if (PtInRect( &wnd->rectWindow, pt )) return wnd->hwndSelf;
        wnd = wnd->next;
    }
    return hwndParent;
}




HWND STDCALL ChildWindowFromPointEx( HWND hwndParent, POINT pt,
		UINT uFlags)
{
    /* pt is in the client coordinates */

    WND* wnd = WIN_FindWndPtr(hwndParent);
    RECT rect;

    if( !wnd ) return 0;

    /* get client rect fast */
    rect.top = rect.left = 0;
    rect.right = wnd->rectClient.right - wnd->rectClient.left;
    rect.bottom = wnd->rectClient.bottom - wnd->rectClient.top;

    if (!PtInRect( &rect, pt )) return 0;

    wnd = wnd->child;
    while ( wnd )
    {
        if (PtInRect( &wnd->rectWindow, pt )) {
		if ( (uFlags & CWP_SKIPINVISIBLE) && 
				!(wnd->dwStyle & WS_VISIBLE) )
		        wnd = wnd->next;
		else if ( (uFlags & CWP_SKIPDISABLED) && 
				(wnd->dwStyle & WS_DISABLED) )
		        wnd = wnd->next;
		else if ( (uFlags & CWP_SKIPTRANSPARENT) && 
				(wnd->dwExStyle & WS_EX_TRANSPARENT) )
		        wnd = wnd->next;
		else
			return wnd->hwndSelf;
	}
    }
    return hwndParent;
}


WINBOOL STDCALL MapWindowPoints( HWND hwndFrom, HWND hwndTo,
                               LPPOINT lppt, UINT count )
{
    POINT offset;

    WINPOS_GetWinOffset( hwndFrom, hwndTo, &offset );
    while (count--)
    {
	lppt->x += offset.x;
	lppt->y += offset.y;
        lppt++;
    }
    return TRUE;
}


WINBOOL STDCALL IsIconic(HWND hWnd)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) return FALSE;
    return (wndPtr->dwStyle & WS_MINIMIZE) != 0;
}
 

WINBOOL STDCALL IsZoomed(HWND hWnd)
{
    WND * wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) return FALSE;
    return (wndPtr->dwStyle & WS_MAXIMIZE) != 0;
}


HWND STDCALL GetActiveWindow(void)
{  
    return WINPOS_GetActiveWindow();
}

HWND STDCALL SetActiveWindow(HWND  hWnd )
{
	HWND hPrev;
	hPrev = GetActiveWindow();
	if ( WINPOS_SetActiveWindow( hWnd, TRUE, TRUE) )
		return hPrev;
	return NULL;
}


/***********************************************************************
 *           ShowWindow   (USER32.534)
 */
WINBOOL STDCALL ShowWindow( HWND hwnd, INT cmd ) 
{    
    WND* 	wndPtr = WIN_FindWndPtr( hwnd );
    WINBOOL 	wasVisible = FALSE, showFlag;
    RECT 	newPos = {0, 0, 100, 100};
    int 	swp = 0;

    if (!wndPtr) return FALSE;

#ifdef OPTIMIZATION

    wasVisible = (wndPtr->dwStyle & WS_VISIBLE) != 0;
#endif

    switch(cmd)
    {
        case SW_HIDE:
            if (!wasVisible) return FALSE;
	    swp |= SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | 
		        SWP_NOACTIVATE | SWP_NOZORDER;
	    break;

	case SW_SHOWMINNOACTIVE:
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOWMINIMIZED:
            swp |= SWP_SHOWWINDOW;
            /* fall through */
	case SW_MINIMIZE:
            swp |= SWP_FRAMECHANGED;
            if( !(wndPtr->dwStyle & WS_MINIMIZE) )
		 swp |= WINPOS_MinMaximize( wndPtr, SW_MINIMIZE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_SHOWMAXIMIZED: /* same as SW_MAXIMIZE */
            swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;
            if( !(wndPtr->dwStyle & WS_MAXIMIZE) )
		 swp |= WINPOS_MinMaximize( wndPtr, SW_MAXIMIZE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
            break;

	case SW_SHOWNA:
            swp |= SWP_NOACTIVATE | SWP_NOZORDER;
            /* fall through */
	case SW_SHOW:
	    swp |= SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOMOVE;
	    break;

	case SW_SHOWNOACTIVATE:
            swp |= SWP_NOZORDER;
            if (GetActiveWindow()) swp |= SWP_NOACTIVATE;
            /* fall through */
	case SW_SHOWNORMAL:  /* same as SW_NORMAL: */
	case SW_SHOWDEFAULT: /* FIXME: should have its own handler */
	case SW_RESTORE:
	    swp |= SWP_SHOWWINDOW | SWP_FRAMECHANGED;

            if( wndPtr->dwStyle & (WS_MINIMIZE | WS_MAXIMIZE) )
		 swp |= WINPOS_MinMaximize( wndPtr, SW_RESTORE, &newPos );
            else swp |= SWP_NOSIZE | SWP_NOMOVE;
	    break;
    }

    showFlag = (cmd != SW_HIDE);
    if (showFlag != wasVisible)
    {
        SendMessageW( hwnd, WM_SHOWWINDOW, showFlag, 0 );
        if (!IsWindow( hwnd )) return wasVisible;
    }

    if ((wndPtr->dwStyle & WS_CHILD) &&
        !IsWindowVisible( wndPtr->parent->hwndSelf ) &&
        (swp & (SWP_NOSIZE | SWP_NOMOVE)) == (SWP_NOSIZE | SWP_NOMOVE) )
    {
        /* Don't call SetWindowPos() on invisible child windows */
        if (cmd == SW_HIDE) wndPtr->dwStyle &= ~WS_VISIBLE;
        else wndPtr->dwStyle |= WS_VISIBLE;
    }
    else
    {
        /* We can't activate a child window */
        if (wndPtr->dwStyle & WS_CHILD) swp |= SWP_NOACTIVATE | SWP_NOZORDER;
        SetWindowPos( hwnd, HWND_TOP, 
			newPos.left, newPos.top, newPos.right, newPos.bottom, swp );
        if (!IsWindow( hwnd )) return wasVisible;
	else if( wndPtr->dwStyle & WS_MINIMIZE ) WINPOS_ShowIconTitle( wndPtr, TRUE );
    }

    if (wndPtr->flags & WIN_NEED_SIZE)
    {
        /* should happen only in CreateWindowEx() */
	int wParam = SIZE_RESTORED;

	wndPtr->flags &= ~WIN_NEED_SIZE;
	if (wndPtr->dwStyle & WS_MAXIMIZE) 
		wParam = SIZE_MAXIMIZED;
	else if (wndPtr->dwStyle & WS_MINIMIZE) 
		wParam = SIZE_MINIMIZED;
	SendMessageW( hwnd, WM_SIZE, wParam,
		     MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
			    wndPtr->rectClient.bottom-wndPtr->rectClient.top));
	SendMessageW( hwnd, WM_MOVE, 0,
		   MAKELONG(wndPtr->rectClient.left, wndPtr->rectClient.top) );
    }

    SetWindowPos( hwnd, HWND_TOP, 
			newPos.left, newPos.top, newPos.right, newPos.bottom, swp );

 //   SendMessage(hwnd, WM_NCACTIVATE,TRUE,0);
 //   SendMessage(hwnd, WM_NCPAINT,CreateRectRgn(100,100,100, 100) ,0);

   
    
    return wasVisible;
}


WINBOOL STDCALL SetWindowPos( HWND hWnd, HWND hWndInsertAfter ,
	     int X, int Y,
	     int cx, int cy,
	     UINT flags)
{

    WINDOWPOS   winpos;
    WND *	wndPtr;
    RECT 	newWindowRect, newClientRect, oldWindowRect;
    HRGN	visRgn = 0;
    HWND	tempInsertAfter= 0;
    int 	result = 0;
    UINT	uFlags;
    WINBOOL      resync = FALSE;

 
      /* Check window handle */

    if (hWnd == GetDesktopWindow()) return FALSE;

    if (hWnd == GetActiveWindow() ) flags |= SWP_NOACTIVATE;   /* Already active */


   /* Check dimensions */

    if (cx <= 0) cx = 1;
    if (cy <= 0) cy = 1;

    if (!(wndPtr = WIN_FindWndPtr( hWnd ))) return FALSE;

#if OPTIMIZATION
    if(wndPtr->dwStyle & WS_VISIBLE)
        flags &= ~SWP_SHOWWINDOW;
    else
    {
	uFlags |= SMC_NOPARENTERASE; 
	flags &= ~SWP_HIDEWINDOW;
	if (!(flags & SWP_SHOWWINDOW)) flags |= SWP_NOREDRAW;
    }



      /* Check flags */

    if ((wndPtr->rectWindow.right - wndPtr->rectWindow.left == cx) &&
        (wndPtr->rectWindow.bottom - wndPtr->rectWindow.top == cy))
        flags |= SWP_NOSIZE;    /* Already the right size */
    if ((wndPtr->rectWindow.left == X) && (wndPtr->rectWindow.top == Y))
        flags |= SWP_NOMOVE;    /* Already the right position */

#endif
      /* Check hWndInsertAfter */

    if (!(flags & (SWP_NOZORDER | SWP_NOACTIVATE)))
    {
	  /* Ignore TOPMOST flags when activating a window */
          /* _and_ moving it in Z order. */

	if ((hWndInsertAfter == HWND_TOPMOST) || (hWndInsertAfter == HWND_NOTOPMOST))
	    hWndInsertAfter = HWND_TOP;	
    }
      /* TOPMOST not supported yet */
    if ((hWndInsertAfter == HWND_TOPMOST) || (hWndInsertAfter == HWND_NOTOPMOST)) 
	hWndInsertAfter = HWND_TOP;

      /* hWndInsertAfter must be a sibling of the window */
    if ((hWndInsertAfter != HWND_TOP) && (hWndInsertAfter != HWND_BOTTOM))
       {
	 WND* wnd = WIN_FindWndPtr(hWndInsertAfter);

	 if( wnd ) {
	   if( wnd->parent != wndPtr->parent ) return FALSE;
	   if( wnd->next == wndPtr ) flags |= SWP_NOZORDER;
	 }
       }
    else 
    {
         /* FIXME: the following optimization is no good for "X-ed" windows */
       if (hWndInsertAfter == HWND_TOP && wndPtr->parent)
	   flags |= ( wndPtr->parent->child == wndPtr)? SWP_NOZORDER: 0;
       else /* HWND_BOTTOM */
	   flags |= ( wndPtr->next )? 0: SWP_NOZORDER;
   }
    

      /* Fill the WINDOWPOS structure */

    winpos.hwnd = hWnd;
    winpos.hwndInsertAfter = hWndInsertAfter;
    winpos.x = X;
    winpos.y = Y;
    winpos.cx = cx;
    winpos.cy = cy;
    winpos.flags = flags;
    
      /* Send WM_WINDOWPOSCHANGING message */

    if (!(winpos.flags & SWP_NOSENDCHANGING))
	MSG_SendMessage( wndPtr, WM_WINDOWPOSCHANGING, 0, (LPARAM)&winpos );

      /* Calculate new position and size */

    newWindowRect = wndPtr->rectWindow;
    newClientRect = (wndPtr->dwStyle & WS_MINIMIZE) ? wndPtr->rectWindow
						    : wndPtr->rectClient;

    if (!(winpos.flags & SWP_NOSIZE))
    {
        newWindowRect.right  = newWindowRect.left + winpos.cx;
        newWindowRect.bottom = newWindowRect.top + winpos.cy;
    }
    if (!(winpos.flags & SWP_NOMOVE))
    {
        newWindowRect.left    = winpos.x;
        newWindowRect.top     = winpos.y;
        newWindowRect.right  += winpos.x - wndPtr->rectWindow.left;
        newWindowRect.bottom += winpos.y - wndPtr->rectWindow.top;

	OffsetRect( &newClientRect, winpos.x - wndPtr->rectWindow.left, 
                                      winpos.y - wndPtr->rectWindow.top );
    }

    winpos.flags |= SWP_NOCLIENTMOVE | SWP_NOCLIENTSIZE;

      /* Reposition window in Z order */

    if (!(winpos.flags & SWP_NOZORDER))
    {
	/* reorder owned popups if hwnd is top-level window 
         */
	if( wndPtr->parent == WIN_GetDesktop() )
	    hWndInsertAfter = WINPOS_ReorderOwnedPopups( hWndInsertAfter,
							 wndPtr, winpos.flags );

       // if (((X11DRV_WND_DATA *) wndPtr->pDriverData)->window)
        //{
          //  WIN_UnlinkWindow( winpos.hwnd );
           // WIN_LinkWindow( winpos.hwnd, hWndInsertAfter );
        //}
        else WINPOS_MoveWindowZOrder( winpos.hwnd, hWndInsertAfter );
    }

    if (  !(winpos.flags & SWP_NOREDRAW) && 
	((winpos.flags & (SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED)) 
		      != (SWP_NOMOVE | SWP_NOSIZE)) )
          visRgn = DCE_GetVisRgn(hWnd, DCX_WINDOW | DCX_CLIPSIBLINGS);


      /* Send WM_NCCALCSIZE message to get new client area */
    if( (winpos.flags & (SWP_FRAMECHANGED | SWP_NOSIZE)) != SWP_NOSIZE )
    {
         result = WINPOS_SendNCCalcSize( winpos.hwnd, TRUE, &newWindowRect,
				    &wndPtr->rectWindow, &wndPtr->rectClient,
				    &winpos, &newClientRect );

         /* FIXME: WVR_ALIGNxxx */

         if( newClientRect.left != wndPtr->rectClient.left ||
             newClientRect.top != wndPtr->rectClient.top )
             winpos.flags &= ~SWP_NOCLIENTMOVE;

         if( (newClientRect.right - newClientRect.left !=
              wndPtr->rectClient.right - wndPtr->rectClient.left) ||
	     (newClientRect.bottom - newClientRect.top !=
	      wndPtr->rectClient.bottom - wndPtr->rectClient.top) )
	     winpos.flags &= ~SWP_NOCLIENTSIZE;
    }
    else
      if( !(flags & SWP_NOMOVE) && (newClientRect.left != wndPtr->rectClient.left ||
				    newClientRect.top != wndPtr->rectClient.top) )
	    winpos.flags &= ~SWP_NOCLIENTMOVE;

    /* Update active DCEs 
     * TODO: Optimize conditions that trigger DCE update.
     */

#if 0
    if( (((winpos.flags & SWP_AGG_NOPOSCHANGE) != SWP_AGG_NOPOSCHANGE) && 
					 wndPtr->dwStyle & WS_VISIBLE) || 
	(flags & (SWP_HIDEWINDOW | SWP_SHOWWINDOW)) ) 
    {
        RECT rect;

        UnionRect(&rect, &newWindowRect, &wndPtr->rectWindow);
	DCE_InvalidateDCE(wndPtr, &rect);
    }

    /* change geometry */

    oldWindowRect = wndPtr->rectWindow;
     {
	RECT oldClientRect = wndPtr->rectClient;

        wndPtr->rectWindow = newWindowRect;
        wndPtr->rectClient = newClientRect;

	if( oldClientRect.bottom - oldClientRect.top ==
	    newClientRect.bottom - newClientRect.top ) result &= ~WVR_VREDRAW;

	if( oldClientRect.right - oldClientRect.left ==
	    newClientRect.right - newClientRect.left ) result &= ~WVR_HREDRAW;

        if( !(flags & (SWP_NOREDRAW | SWP_HIDEWINDOW | SWP_SHOWWINDOW)) )
	{
	    uFlags |=  ((winpos.flags & SWP_NOCOPYBITS) || 
			(result >= WVR_HREDRAW && result < WVR_VALIDRECTS)) ? SMC_NOCOPY : 0;
	    uFlags |=  (winpos.flags & SWP_FRAMECHANGED) ? SMC_DRAWFRAME : 0;

	    if( (winpos.flags & SWP_AGG_NOGEOMETRYCHANGE) != SWP_AGG_NOGEOMETRYCHANGE )
		uFlags = WINPOS_SizeMoveClean(wndPtr, visRgn, &oldWindowRect, 
							      &oldClientRect, uFlags);
	    else
	    { 
		/* adjust the frame and do not erase the parent */

		if( winpos.flags & SWP_FRAMECHANGED ) wndPtr->flags |= WIN_NEEDS_NCPAINT;
		if( winpos.flags & SWP_NOZORDER ) uFlags |= SMC_NOPARENTERASE;
	    }
	}
        DeleteObject(visRgn);
    }
#endif
    if (flags & SWP_SHOWWINDOW)
    {
	wndPtr->dwStyle |= WS_VISIBLE;
  
        {
            if (!(flags & SWP_NOREDRAW))
                PAINT_RedrawWindow( winpos.hwnd, NULL, 0,
                                RDW_INVALIDATE | RDW_ALLCHILDREN |
                                RDW_FRAME | RDW_ERASENOW | RDW_ERASE, 0 );
        }
    }
    else if (flags & SWP_HIDEWINDOW)
    {
        wndPtr->dwStyle &= ~WS_VISIBLE;

  
       
         if (!(flags & SWP_NOREDRAW) && wndPtr->parent != NULL ) 
                PAINT_RedrawWindow( wndPtr->parent->hwndSelf, &oldWindowRect,
                                    0, RDW_INVALIDATE | RDW_ALLCHILDREN |
                                       RDW_ERASE | RDW_ERASENOW, 0 );
	 
	  uFlags |= SMC_NOPARENTERASE;
        

        if ((winpos.hwnd == GetFocus()) ||
            IsChild( winpos.hwnd, GetFocus()))
        {
            /* Revert focus to parent */
            SetFocus( GetParent(winpos.hwnd) );
        }
	if (hWnd == CARET_GetHwnd()) DestroyCaret();

	if (winpos.hwnd == GetActiveWindow())
	    WINPOS_ActivateOtherWindow( wndPtr );
    }

      /* Activate the window */

    if (!(flags & SWP_NOACTIVATE))
	    WINPOS_ChangeActiveWindow( winpos.hwnd, FALSE );
    
      /* Repaint the window */

  //  if (((X11DRV_WND_DATA *) wndPtr->pDriverData)->window)
  //      EVENT_Synchronize();  /* Wait for all expose events */

    //if (!GetCapture())
    //    EVENT_DummyMotionNotify(); /* Simulate a mouse event to set the cursor */

    if (wndPtr->parent && !(flags & SWP_DEFERERASE) && !(uFlags & SMC_NOPARENTERASE) )
        PAINT_RedrawWindow( wndPtr->parent->hwndSelf, NULL, 0, RDW_ALLCHILDREN | RDW_ERASENOW, 0 );
    else if(wndPtr->parent && wndPtr->parent == WIN_GetDesktop() && wndPtr->parent->flags & WIN_NEEDS_ERASEBKGND )
	PAINT_RedrawWindow( wndPtr->parent->hwndSelf, NULL, 0, RDW_NOCHILDREN | RDW_ERASENOW, 0 );

      /* And last, send the WM_WINDOWPOSCHANGED message */

    DPRINT("\tstatus flags = %04x\n", winpos.flags & SWP_AGG_STATUSFLAGS);

    if ( resync ||
        (((winpos.flags & SWP_AGG_STATUSFLAGS) != SWP_AGG_NOPOSCHANGE) && 
         !(winpos.flags & SWP_NOSENDCHANGING)) )
    {
        SendMessageA( winpos.hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&winpos );
        //if (resync) EVENT_Synchronize ();
    }

    return TRUE;
}




/***********************************************************************
 *           MoveWindow   (USER32.399)
 */
WINBOOL STDCALL MoveWindow( HWND hwnd, INT x, INT y, INT cx, INT cy,
                            WINBOOL repaint )
{    
    int flags = SWP_NOZORDER | SWP_NOACTIVATE;
    if (!repaint) flags |= SWP_NOREDRAW;
   
    return SetWindowPos( hwnd, 0, x, y, cx, cy, flags );
}

