
#include <windows.h>
#include <user32/paint.h>

/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *
 * FIXME: Do not repaint full nonclient area all the time. Instead, compute 
 *	  intersection with hrgnUpdate (which should be moved from client to 
 *	  window coords as well, lookup 'the pain' comment in the winpos.c).
 */

  /* Last CTLCOLOR id */
//#define CTLCOLOR_MAX   CTLCOLOR_STATIC

HBRUSH DEFWND_ControlColor( HDC hDC, UINT ctlType );


/***********************************************************************
 *           PAINT_RedrawWindow
 *
 * FIXME: Windows uses WM_SYNCPAINT to cut down the number of intertask
 * SendMessage() calls. This is a comment inside DefWindowProc() source 
 * from 16-bit SDK:
 *
 *   This message avoids lots of inter-app message traffic
 *   by switching to the other task and continuing the
 *   recursion there.
 * 
 * wParam         = flags
 * LOWORD(lParam) = hrgnClip
 * HIWORD(lParam) = hwndSkip  (not used; always NULL)
 *
 * All in all, a prime candidate for a rewrite.
 */
WINBOOL PAINT_RedrawWindow( HWND hwnd, const RECT *rectUpdate,
                           HRGN hrgnUpdate, UINT flags, UINT control )
{
    WINBOOL bIcon;
    HRGN hrgn;
    RECT rectClient;
    WND* wndPtr;
    WND **list, **ppWnd;

    if (!hwnd) hwnd = GetDesktopWindow();
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;

#ifdef OPTIMIZE
    if (!WIN_IsWindowDrawable( wndPtr, !(flags & RDW_FRAME) ) )
        return TRUE;  /* No redraw needed */
#endif

    bIcon = (wndPtr->dwStyle & WS_MINIMIZE && wndPtr->class->hIcon);
 
    GetClientRect( hwnd, &rectClient );

    OffsetRect(&rectClient,5,16);
    
    if (flags & RDW_INVALIDATE)  /* Invalidate */
    {
        int rgnNotEmpty = COMPLEXREGION;

        if (wndPtr->hrgnUpdate > (HRGN)1)  /* Is there already an update region? */
        {
            if ((hrgn = hrgnUpdate) == 0)
                hrgn = CreateRectRgnIndirect( rectUpdate ? rectUpdate :
                                                &rectClient );
            rgnNotEmpty = CombineRgn( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate,
                                        hrgn, RGN_OR );
            if (!hrgnUpdate) DeleteObject( hrgn );
        }
        else  /* No update region yet */
        {
            if (!(wndPtr->flags & WIN_INTERNAL_PAINT))
                QUEUE_IncPaintCount( wndPtr->hmemTaskQ );
            if (hrgnUpdate)
            {
                wndPtr->hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );
                rgnNotEmpty = CombineRgn( wndPtr->hrgnUpdate, hrgnUpdate,
                                            0, RGN_COPY );
            }
            else wndPtr->hrgnUpdate = CreateRectRgnIndirect( rectUpdate ?
                                                    rectUpdate : &rectClient );
        }
	
        if (flags & RDW_FRAME) wndPtr->flags |= WIN_NEEDS_NCPAINT;

        /* restrict update region to client area (FIXME: correct?) */
        if (wndPtr->hrgnUpdate)
        {
            HRGN clientRgn = CreateRectRgnIndirect( &rectClient );
            rgnNotEmpty = CombineRgn( wndPtr->hrgnUpdate, clientRgn, 
                                        wndPtr->hrgnUpdate, RGN_AND );
            DeleteObject( clientRgn );
        }

	/* check for bogus update region */ 
	if ( rgnNotEmpty == NULLREGION )
	   {
	     wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
	     DeleteObject( wndPtr->hrgnUpdate );
	     wndPtr->hrgnUpdate=0;
             if (!(wndPtr->flags & WIN_INTERNAL_PAINT))
                   QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
	   }
	else
             if (flags & RDW_ERASE) wndPtr->flags |= WIN_NEEDS_ERASEBKGND;
	flags |= RDW_FRAME;  /* Force children frame invalidation */
    }
    else if (flags & RDW_VALIDATE)  /* Validate */
    {
          /* We need an update region in order to validate anything */
        if (wndPtr->hrgnUpdate > (HRGN)1)
        {
            if (!hrgnUpdate && !rectUpdate)
            {
                  /* Special case: validate everything */
                DeleteObject( wndPtr->hrgnUpdate );
                wndPtr->hrgnUpdate = 0;
            }
            else
            {
                if ((hrgn = hrgnUpdate) == 0)
                    hrgn = CreateRectRgnIndirect( rectUpdate );
                if (CombineRgn( wndPtr->hrgnUpdate, wndPtr->hrgnUpdate,
                                  hrgn, RGN_DIFF ) == NULLREGION)
                {
                    DeleteObject( wndPtr->hrgnUpdate );
                    wndPtr->hrgnUpdate = 0;
                }
                if (!hrgnUpdate) DeleteObject( hrgn );
            }
            if (!wndPtr->hrgnUpdate)  /* No more update region */
		if (!(wndPtr->flags & WIN_INTERNAL_PAINT))
		    QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
        }
        if (flags & RDW_NOFRAME) wndPtr->flags &= ~WIN_NEEDS_NCPAINT;
	if (flags & RDW_NOERASE) wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
    }

      /* Set/clear internal paint flag */

    if (flags & RDW_INTERNALPAINT)
    {
	if ( wndPtr->hrgnUpdate <= (HRGN)1 && !(wndPtr->flags & WIN_INTERNAL_PAINT))
	    QUEUE_IncPaintCount( wndPtr->hmemTaskQ );
	wndPtr->flags |= WIN_INTERNAL_PAINT;	    
    }
    else if (flags & RDW_NOINTERNALPAINT)
    {
	if ( wndPtr->hrgnUpdate <= (HRGN)1 && (wndPtr->flags & WIN_INTERNAL_PAINT))
	    QUEUE_DecPaintCount( wndPtr->hmemTaskQ );
	wndPtr->flags &= ~WIN_INTERNAL_PAINT;
    }

      /* Erase/update window */

    if (flags & RDW_UPDATENOW)
    {
        if (wndPtr->hrgnUpdate) /* wm_painticon wparam is 1 */
            SendMessageA( hwnd, (bIcon) ? WM_PAINTICON : WM_PAINT, bIcon, 0 );
    }
    else 
//if (flags & RDW_ERASENOW)
    {
        //if (wndPtr->flags & WIN_NEEDS_NCPAINT)
	    WIN_UpdateNCArea( wndPtr, FALSE);

        //if (wndPtr->flags & WIN_NEEDS_ERASEBKGND)
        {
            HDC hdc = GetDCEx( hwnd, wndPtr->hrgnUpdate,
                                   DCX_INTERSECTRGN | DCX_USESTYLE |
                                   DCX_KEEPCLIPRGN | DCX_WINDOWPAINT |
                                   (bIcon ? DCX_WINDOW : 0) );
            if (hdc)
            {
               if (SendMessageA( hwnd, (bIcon) ? WM_ICONERASEBKGND
						: WM_ERASEBKGND,
                                  (WPARAM)hdc, 0 ))
                  wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
               ReleaseDC( hwnd, hdc );
            }
        }
    }

      /* Recursively process children */

    if (!(flags & RDW_NOCHILDREN) &&
        ((flags & RDW_ALLCHILDREN) || !(wndPtr->dwStyle & WS_CLIPCHILDREN)) &&
	!(wndPtr->dwStyle & WS_MINIMIZE) )
    {
        if ( hrgnUpdate || rectUpdate )
	{
	   if (!(hrgn = CreateRectRgn( 0, 0, 0, 0 ))) return TRUE;
	   if( !hrgnUpdate )
           {
	        control |= (RDW_C_DELETEHRGN | RDW_C_USEHRGN);
 	        if( !(hrgnUpdate = CreateRectRgnIndirect( rectUpdate )) )
                {
                    DeleteObject( hrgn );
                    return TRUE;
                }
           }
           if( (list = WIN_BuildWinArray( wndPtr, 0, NULL )) )
	   {
		for (ppWnd = list; *ppWnd; ppWnd++)
		{
		    wndPtr = *ppWnd;
		    if (!IsWindow(wndPtr->hwndSelf)) continue;
		    if (wndPtr->dwStyle & WS_VISIBLE)
		    {
			SetRectRgn( hrgn, 
				wndPtr->rectWindow.left, wndPtr->rectWindow.top, 
				wndPtr->rectWindow.right, wndPtr->rectWindow.bottom );
			if (CombineRgn( hrgn, hrgn, hrgnUpdate, RGN_AND ))
			{
			    OffsetRgn( hrgn, -wndPtr->rectClient.left,
                                        -wndPtr->rectClient.top );
			    PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, hrgn, flags,
                                         RDW_C_USEHRGN );
			}
		    }
		}
		HeapFree( GetProcessHeap(), 0, list );
	   }
	   DeleteObject( hrgn );
	   if (control & RDW_C_DELETEHRGN) DeleteObject( hrgnUpdate );
	}
        else
        {
	    if( (list = WIN_BuildWinArray( wndPtr, 0, NULL )) )
	    {
		for (ppWnd = list; *ppWnd; ppWnd++)
		{
		    wndPtr = *ppWnd;
		    if (IsWindow( wndPtr->hwndSelf ))
			PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, 0, flags, 0 );
		}
	        HeapFree( GetProcessHeap(), 0, list );
	    }
	}

    }
    return TRUE;
}



/***********************************************************************
 *           GetControlBrush   Not A Win32 API
 */
HBRUSH GetControlBrush( HWND hwnd, HDC hdc, UINT ctlType )
{
    WND* wndPtr = WIN_FindWndPtr( hwnd );

    if((ctlType <= CTLCOLOR_MAX) && wndPtr )
    {
	WND* parent;
	if( wndPtr->dwStyle & WS_POPUP ) parent = wndPtr->owner;
	else parent = wndPtr->parent;
	if( !parent ) parent = wndPtr;
	return (HBRUSH)PAINT_GetControlBrush( parent->hwndSelf, hwnd, hdc, ctlType );
    }
    return (HBRUSH)0;
}

/***********************************************************************
 *	     PAINT_GetControlBrush
 */
HBRUSH PAINT_GetControlBrush( HWND hParent, HWND hWnd, HDC hDC, UINT ctlType )
{
    LOGBRUSH LogBrush;
    HBRUSH bkgBrush = (HBRUSH)SendMessageA( hParent, WM_CTLCOLORMSGBOX + ctlType, 
							     (WPARAM)hDC, (LPARAM)hWnd );
    if( !GetObject(bkgBrush,sizeof(LOGBRUSH),&LogBrush) )
	bkgBrush = DEFWND_ControlColor( hDC, ctlType );
    return bkgBrush;
}



