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
    WND *wndPtr;
    if (!hWnd)
        hWnd= GetDesktopWindow();
    if (!(wndPtr = WIN_FindWndPtr( hWnd ))) return 0;

    return wndPtr->dce->hDC;
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

#if 0
    if (!(wndPtr->class->style & (CS_OWNDC | CS_CLASSDC))) 
   	flags |= DCX_CACHE;


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

#endif
    
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

#if 0
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
		if ((dce->hwndCurrent == hwnd) 
		   && ((dce->DCXflags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
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
	if (wndPtr->class->style & CS_OWNDC) 
        	dce = wndPtr->dce;
	else if ( wndPtr->class->style & CS_CLASSDC)
		dce = wndPtr->class->dce;
	else
		return 0;

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

#endif
    dce = wndPtr->dce;

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


