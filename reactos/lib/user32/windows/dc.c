/*
 * USER DCE functions
 *
 * Copyright 1993 Alexandre Julliard
 *	     1996,1997 Alex Korobka
 *
 *
 * Note: Visible regions of CS_OWNDC/CS_CLASSDC window DCs 
 * have to be updated dynamically. 
 * 
 * Internal DCX flags:
 *
 * DCX_DCEEMPTY    - dce is uninitialized
 * DCX_DCEBUSY     - dce is in use
 * DCX_DCEDIRTY    - ReleaseDC() should wipe instead of caching
 * DCX_KEEPCLIPRGN - ReleaseDC() should not delete the clipping region
 * DCX_WINDOWPAINT - BeginPaint() is in effect
 */

/* GetDCEx flags */

#define DCX_USESTYLE         0x00010000



#include <windows.h>
#include <user32/win.h>
#include <user32/dce.h>
#include <user32/sysmetr.h>
#include <user32/debug.h>


extern DCE *firstDCE;

INT SelectVisRgn(HDC hdc,HRGN hrgn);
INT  ExcludeVisRect(HDC hDC,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect);
INT  RestoreVisRgn(HDC hdc);


HDC STDCALL GetDC( HWND  hWnd 	)
{
    if (!hWnd)
        return GetDCEx( GetDesktopWindow(), 0, DCX_CACHE | DCX_WINDOW );
    return GetDCEx( hWnd, 0, DCX_USESTYLE );
}

/***********************************************************************
 *           GetDCEx   
 *
 * Unimplemented flags: DCX_LOCKWINDOWUPDATE
 *
 * FIXME: Full support for hrgnClip == 1 (alias for entire window).
 */
HDC WINAPI GetDCEx( HWND hwnd, HRGN hrgnClip, DWORD flags )
{
    HRGN 	hrgnVisible = 0;
    HDC 	hdc = 0;
    DCE * 	dce;
    //HDC 	dc;
    WND * 	wndPtr;
    DWORD 	dcxFlags = 0;
    BOOL	bUpdateVisRgn = TRUE;
    BOOL	bUpdateClipOrigin = FALSE;

    DPRINT("hwnd %04x, hrgnClip %04x, flags %08x\n", 
				hwnd, hrgnClip, (unsigned)flags);
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;

    /* fixup flags */

    if (!(wndPtr->class->style & (CS_OWNDC | CS_CLASSDC))) flags |= DCX_CACHE;

    if (flags & DCX_USESTYLE)
    {
	flags &= ~( DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

        if( wndPtr->dwStyle & WS_CLIPSIBLINGS )
            flags |= DCX_CLIPSIBLINGS;

	if ( !(flags & DCX_WINDOW) )
	{
            if (wndPtr->class->style & CS_PARENTDC) flags |= DCX_PARENTCLIP;

	    if (wndPtr->dwStyle & WS_CLIPCHILDREN &&
                     !(wndPtr->dwStyle & WS_MINIMIZE) ) flags |= DCX_CLIPCHILDREN;
	}
	else flags |= DCX_CACHE;
    }

    if( flags & DCX_NOCLIPCHILDREN )
    {
        flags |= DCX_CACHE;
        flags &= ~(DCX_PARENTCLIP | DCX_CLIPCHILDREN);
    }

    if (flags & DCX_WINDOW) 
	flags = (flags & ~DCX_CLIPCHILDREN) | DCX_CACHE;

    if (!(wndPtr->dwStyle & WS_CHILD) || !wndPtr->parent ) 
	flags &= ~DCX_PARENTCLIP;
    else if( flags & DCX_PARENTCLIP )
    {
	flags |= DCX_CACHE;
	if( !(flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) )
	    if( (wndPtr->dwStyle & WS_VISIBLE) && (wndPtr->parent->dwStyle & WS_VISIBLE) )
	    {
		flags &= ~DCX_CLIPCHILDREN;
		if( wndPtr->parent->dwStyle & WS_CLIPSIBLINGS )
		    flags |= DCX_CLIPSIBLINGS;
	    }
    }

    /* find a suitable DCE */

    dcxFlags = flags & (DCX_PARENTCLIP | DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | 
		        DCX_CACHE | DCX_WINDOW);

    if (flags & DCX_CACHE)
    {
	DCE*	dceEmpty;
	DCE*	dceUnused;

	dceEmpty = dceUnused = NULL;

	/* Strategy: First, we attempt to find a non-empty but unused DCE with
	 * compatible flags. Next, we look for an empty entry. If the cache is
	 * full we have to purge one of the unused entries.
	 */

	for (dce = firstDCE; (dce); dce = dce->next)
	{
	    if ((dce->DCXflags & (DCX_CACHE | DCX_DCEBUSY)) == DCX_CACHE )
	    {
		dceUnused = dce;

		if (dce->DCXflags & DCX_DCEEMPTY)
		    dceEmpty = dce;
		else
		if ((dce->hwndCurrent == hwnd) &&
		   ((dce->DCXflags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
				      DCX_CACHE | DCX_WINDOW | DCX_PARENTCLIP)) == dcxFlags))
		{
		    DPRINT("\tfound valid %08x dce [%04x], flags %08x\n", 
					(unsigned)dce, hwnd, (unsigned)dcxFlags );
		    bUpdateVisRgn = FALSE; 
		    bUpdateClipOrigin = TRUE;
		    break;
		}
	    }
	}
	if (!dce) dce = (dceEmpty) ? dceEmpty : dceUnused;
    }
    else 
    {
        dce = (wndPtr->class->style & CS_OWNDC) ? wndPtr->dce : wndPtr->class->dce;
	if( dce->hwndCurrent == hwnd )
	{
	    DPRINT("\tskipping hVisRgn update\n");
	    bUpdateVisRgn = FALSE; /* updated automatically, via DCHook() */

	    if( (dce->DCXflags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) &&
		(flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) )
	    {
		/* This is likely to be a nested BeginPaint(). */

		if( dce->hClipRgn != hrgnClip )
		{
		    DPRINT("fixme new hrgnClip[%04x] smashes the previous[%04x]\n",
			  hrgnClip, dce->hClipRgn );
		    DCE_DeleteClipRgn( dce );
		}
		else 
		    RestoreVisRgn(dce->hDC);
	    }
	}
    }
    if (!dce) return 0;

    dce->hwndCurrent = hwnd;
    dce->hClipRgn = 0;
    dce->DCXflags = dcxFlags | (flags & DCX_WINDOWPAINT) | DCX_DCEBUSY;
    hdc = dce->hDC;
    
    //if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    //bUpdateVisRgn = bUpdateVisRgn || (dc->w.flags & DC_DIRTY);

    /* recompute visible region */

    //wndPtr->pDriver->pSetDrawable( wndPtr, dc, flags, bUpdateClipOrigin );
    if( bUpdateVisRgn )
    {
	DPRINT("updating visrgn for %08x dce, hwnd [%04x]\n", (unsigned)dce, hwnd);

	if (flags & DCX_PARENTCLIP)
        {
            WND *parentPtr = wndPtr->parent;

	    if( wndPtr->dwStyle & WS_VISIBLE && !(parentPtr->dwStyle & WS_MINIMIZE) )
	    {
		if( parentPtr->dwStyle & WS_CLIPSIBLINGS ) 
		    dcxFlags = DCX_CLIPSIBLINGS | (flags & ~(DCX_CLIPCHILDREN | DCX_WINDOW));
		else
		    dcxFlags = flags & ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW);

                hrgnVisible = DCE_GetVisRgn( parentPtr->hwndSelf, dcxFlags );
		if( flags & DCX_WINDOW )
		    OffsetRgn( hrgnVisible, -wndPtr->rectWindow.left,
					      -wndPtr->rectWindow.top );
		else
		    OffsetRgn( hrgnVisible, -wndPtr->rectClient.left,
					      -wndPtr->rectClient.top );
                DCE_OffsetVisRgn( hdc, hrgnVisible );
	    }
	    else
		hrgnVisible = CreateRectRgn( 0, 0, 0, 0 );
        }
        else 
	    if ((hwnd == GetDesktopWindow())) {
                 hrgnVisible = CreateRectRgn( 0, 0, SYSMETRICS_CXSCREEN,
						      SYSMETRICS_CYSCREEN );
//	(rootWindow == DefaultRootWindow(display))
	    }
	    else 
            {
                hrgnVisible = DCE_GetVisRgn( hwnd, flags );
                DCE_OffsetVisRgn( hdc, hrgnVisible );
            }

	//dc->w.flags &= ~DC_DIRTY;
	dce->DCXflags &= ~DCX_DCEDIRTY;
	SelectVisRgn( hdc, hrgnVisible );
    }
    else
	DPRINT("no visrgn update %08x dce, hwnd [%04x]\n", (unsigned)dce, hwnd);

    /* apply additional region operation (if any) */

    if( flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN) )
    {
	if( !hrgnVisible ) hrgnVisible = CreateRectRgn( 0, 0, 0, 0 );

	dce->DCXflags |= flags & (DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_EXCLUDERGN);
	dce->hClipRgn = hrgnClip;

	DPRINT( "\tsaved VisRgn, clipRgn = %04x\n", hrgnClip);

	SaveVisRgn( hdc );
        CombineRgn( hrgnVisible, hrgnClip, 0, RGN_COPY );
        DCE_OffsetVisRgn( hdc, hrgnVisible );
        CombineRgn( hrgnVisible, InquireVisRgn( hdc ), hrgnVisible,
                      (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );
	SelectVisRgn( hdc, hrgnVisible );
    }

    if( hrgnVisible ) DeleteObject( hrgnVisible );

    DPRINT( "(%04x,%04x,0x%lx): returning %04x\n", 
	       hwnd, hrgnClip, flags, hdc);
    return hdc;
}

int STDCALL ReleaseDC(HWND  hWnd,HDC  hDC )
{
	return 0;
}

HDC GetWindowDC(HWND  hWnd )
{
    if (!hWnd) hWnd = GetDesktopWindow();
    return GetDCEx( hWnd, 0, DCX_USESTYLE | DCX_WINDOW );
}


/***********************************************************************
 *           BeginPaint    (USER.10)
 */
HDC
STDCALL
BeginPaint(
	   HWND hWnd,
	   LPPAINTSTRUCT lpPaint)
{
    WINBOOL bIcon;
    HRGN hrgnUpdate;
    WND *wndPtr = WIN_FindWndPtr( hWnd );
    if (!wndPtr) return 0;

    bIcon = (wndPtr->dwStyle & WS_MINIMIZE && wndPtr->class->hIcon);

    wndPtr->flags &= ~WIN_NEEDS_BEGINPAINT;

    if (wndPtr->flags & WIN_NEEDS_NCPAINT) WIN_UpdateNCArea( wndPtr, TRUE );

    if (((hrgnUpdate = wndPtr->hrgnUpdate) != 0) ||
        (wndPtr->flags & WIN_INTERNAL_PAINT))
        QUEUE_DecPaintCount( wndPtr->hmemTaskQ );

    wndPtr->hrgnUpdate = 0;
    wndPtr->flags &= ~WIN_INTERNAL_PAINT;

    HideCaret( hWnd );

    DPRINT("hrgnUpdate = %04x, \n", hrgnUpdate);

    /* When bIcon is TRUE hrgnUpdate is automatically in window coordinates
     * (because rectClient == rectWindow for WS_MINIMIZE windows).
     */

    if (wndPtr->class->style & CS_PARENTDC)
    {
        /* Don't clip the output to the update region for CS_PARENTDC window */
	if(hrgnUpdate > 1)
	    DeleteObject(hrgnUpdate);
        lpPaint->hdc = GetDCEx( hWnd, 0, DCX_WINDOWPAINT | DCX_USESTYLE |
                              (bIcon ? DCX_WINDOW : 0) );
    }
    else
    {
        lpPaint->hdc = GetDCEx(hWnd, hrgnUpdate, DCX_INTERSECTRGN |
                             DCX_WINDOWPAINT | DCX_USESTYLE |
                             (bIcon ? DCX_WINDOW : 0) );
    }

    DPRINT("hdc = %04x\n", lpPaint->hdc);

    if (!lpPaint->hdc)
    {
        //WARN(win, "GetDCEx() failed in BeginPaint(), hWnd=%04x\n", hWnd);
        return 0;
    }

    GetClipBox( lpPaint->hdc, &lpPaint->rcPaint );

	DPRINT("box = (%i,%i - %i,%i)\n", lpPaint->rcPaint.left, lpPaint->rcPaint.top,
		    lpPaint->rcPaint.right, lpPaint->rcPaint.bottom );

    if (wndPtr->flags & WIN_NEEDS_ERASEBKGND)
    {
        wndPtr->flags &= ~WIN_NEEDS_ERASEBKGND;
        lpPaint->fErase = !SendMessageA(hWnd, (bIcon) ? WM_ICONERASEBKGND
                                                   : WM_ERASEBKGND,
                                     (WPARAM)lpPaint->hdc, 0 );
    }
    else lpPaint->fErase = TRUE;

    return lpPaint->hdc;
 
}


WINBOOL STDCALL EndPaint( HWND hWnd, const PAINTSTRUCT *lpPaint )
{
    ReleaseDC( hWnd, lpPaint->hdc );
    ShowCaret( hWnd );
    return TRUE;
}

/***********************************************************************
 *           RedrawWindow    (USER32.426)
 */
WINBOOL STDCALL RedrawWindow( HWND hwnd, const RECT *rectUpdate,
                              HRGN hrgnUpdate, UINT flags )
{
    return PAINT_RedrawWindow( hwnd, rectUpdate, hrgnUpdate, flags, 0 );
}

/***********************************************************************
 *           UpdateWindow   (USER32.567)
 */
WINBOOL
STDCALL
UpdateWindow(
	     HWND hWnd)
{
    return PAINT_RedrawWindow( hWnd, NULL, 0, RDW_UPDATENOW | RDW_NOCHILDREN, 0 );
}

/***********************************************************************
 *           InvalidateRgn   (USER32.9)
 */
WINBOOL
STDCALL
InvalidateRgn(
	      HWND hWnd,
	      HRGN hRgn,
	      WINBOOL bErase)
{
    return PAINT_RedrawWindow(hWnd, NULL, hRgn, RDW_INVALIDATE | (bErase ? RDW_ERASE : 0), 0 );
}

/***********************************************************************
 *           InvalidateRect   (USER32.8)
 */
WINBOOL
STDCALL
InvalidateRect(
	       HWND hWnd ,
	       CONST RECT *lpRect,
	       WINBOOL bErase)
{
    return PAINT_RedrawWindow( hWnd, lpRect, 0, 
			RDW_INVALIDATE | (bErase ? RDW_ERASE : 0), 0 );
}

/***********************************************************************
 *           ValidateRgn   (USER32.572)
 */
WINBOOL
STDCALL
ValidateRgn(
	    HWND hWnd,
	    HRGN hRgn)
{
    return PAINT_RedrawWindow( hWnd, NULL, hRgn, RDW_VALIDATE | RDW_NOCHILDREN, 0 );
}

/***********************************************************************
 *           ValidateRect   (USER32.571)
 */
WINBOOL
STDCALL
ValidateRect(
	     HWND hWnd ,
	     CONST RECT *lpRect)
{
    return PAINT_RedrawWindow( hWnd, lpRect, 0, RDW_VALIDATE | RDW_NOCHILDREN, 0 );
}

/***********************************************************************
 *           GetUpdateRect   (USER32.297)
 */
WINBOOL STDCALL GetUpdateRect( HWND hwnd, LPRECT rect, WINBOOL erase )
{
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return FALSE;

    if (rect)
    {
	if (wndPtr->hrgnUpdate > 1)
	{
	    HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
	    if (GetUpdateRgn( hwnd, hrgn, erase ) == ERROR) return FALSE;
	    GetRgnBox( hrgn, rect );
	    DeleteObject( hrgn );
	}
	else SetRectEmpty( rect );
    }
    return ((UINT)wndPtr->hrgnUpdate > 1);
}

/***********************************************************************
 *           GetUpdateRgn   (USER32.298)
 */
INT STDCALL GetUpdateRgn( HWND hwnd, HRGN hrgn, WINBOOL erase )
{
    INT retval;
    WND * wndPtr = WIN_FindWndPtr( hwnd );
    if (!wndPtr) return ERROR;

    if ((UINT)wndPtr->hrgnUpdate <= 1)
    {
        SetRectRgn( hrgn, 0, 0, 0, 0 );
        return NULLREGION;
    }
    retval = CombineRgn( hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY );
    if (erase) RedrawWindow( hwnd, NULL, 0, RDW_ERASENOW | RDW_NOCHILDREN );
    return retval;
}

/***********************************************************************
 *           ExcludeUpdateRgn   (USER32.195)
 */
INT STDCALL ExcludeUpdateRgn( HDC hdc, HWND hwnd )
{
    RECT rect;
    WND * wndPtr;

    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return ERROR;

    if (wndPtr->hrgnUpdate)
    {
	INT ret;
	HRGN hrgn = CreateRectRgn(wndPtr->rectWindow.left - wndPtr->rectClient.left,
				      wndPtr->rectWindow.top - wndPtr->rectClient.top,
				      wndPtr->rectClient.right - wndPtr->rectClient.left,
				      wndPtr->rectClient.bottom - wndPtr->rectClient.top);
	if( wndPtr->hrgnUpdate > 1 )
	    CombineRgn(hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY);

	/* do ugly coordinate translations in dce.c */

	ret = DCE_ExcludeRgn( hdc, wndPtr, hrgn );
	DeleteObject( hrgn );
	return ret;
    } 
    return GetClipBox( hdc, &rect );
}



WINBOOL PAINT_RedrawWindow( HWND hwnd, const RECT *rectUpdate,
                           HRGN hrgnUpdate, UINT flags, UINT control )
{
}