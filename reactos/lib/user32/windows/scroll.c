/*
 * Scroll windows and DCs
 *
 * Copyright  David W. Metcalfe, 1993
 *	      Alex Korobka       1995,1996
 *
 *
 */

#include <stdlib.h>
#include <windows.h>
#include <user32/class.h>
#include <user32/win.h>
#include <user32/dce.h>
#include <user32/sysmetr.h>
#include <user32/caret.h>
#include <user32/debug.h>



/*************************************************************************
 *             ScrollWindow   (USER.450)
 *
 * FIXME: verify clipping region calculations
 */
WINBOOL STDCALL ScrollWindow( HWND hwnd, INT dx, INT dy,
                              const RECT *rect, const RECT *clipRect )
{
    HDC  	hdc;
    HRGN 	hrgnUpdate,hrgnClip;
    RECT 	rc, cliprc;
    HWND 	hCaretWnd = CARET_GetHwnd();
    WND*	wndScroll = WIN_FindWndPtr( hwnd );


    if ( !wndScroll || !WIN_IsWindowDrawable( wndScroll, TRUE ) ) return TRUE;

    if ( !rect ) /* do not clip children */
       {
	  GetClientRect(hwnd, &rc);
	  hrgnClip = CreateRectRgnIndirect( &rc );

          if ((hCaretWnd == hwnd) || IsChild(hwnd,hCaretWnd))
              HideCaret(hCaretWnd);
          else hCaretWnd = 0;
 
	  hdc = GetDCEx(hwnd, hrgnClip, DCX_CACHE | DCX_CLIPSIBLINGS);
          DeleteObject( hrgnClip );
       }
    else	/* clip children */
       {
	  CopyRect(&rc, rect);

          if (hCaretWnd == hwnd) HideCaret(hCaretWnd);
          else hCaretWnd = 0;

	  hdc = GetDCEx( hwnd, 0, DCX_CACHE | DCX_USESTYLE );
       }

    if (clipRect == NULL)
	GetClientRect(hwnd, &cliprc);
    else
	CopyRect(&cliprc, clipRect);

    hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );
    ScrollDC( hdc, dx, dy, &rc, &cliprc, hrgnUpdate, NULL );
    ReleaseDC(hwnd, hdc);

    if( !rect )		/* move child windows and update region */
    { 
      WND*	wndPtr;

      if( wndScroll->hrgnUpdate > 1 )
	OffsetRgn( wndScroll->hrgnUpdate, dx, dy );

      for (wndPtr = wndScroll->child; wndPtr; wndPtr = wndPtr->next)
        SetWindowPos(wndPtr->hwndSelf, 0, wndPtr->rectWindow.left + dx,
                       wndPtr->rectWindow.top  + dy, 0,0, SWP_NOZORDER |
                       SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
                       SWP_DEFERERASE );
    }

    PAINT_RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_ALLCHILDREN |
                 RDW_INVALIDATE | RDW_ERASE | RDW_ERASENOW, RDW_C_USEHRGN );

    DeleteObject( hrgnUpdate );
    if( hCaretWnd ) 
    {
	POINT	pt;
	GetCaretPos(&pt);
	pt.x += dx; pt.y += dy;
	SetCaretPos(pt.x, pt.y);
	ShowCaret(hCaretWnd);
    }
    return TRUE;
}





/*************************************************************************
 *             ScrollDC   (USER.449)
 * 
 * Both 'rc' and 'prLClip' are in logical units but update info is 
 * returned in device coordinates.
 */
WINBOOL STDCALL ScrollDC( HDC hdc, INT dx, INT dy, const RECT *rc,
                          const RECT *prLClip, HRGN hrgnUpdate,
                          LPRECT rcUpdate )
{


    RECT rClip;
    POINT src, dest;
    INT  ldx, ldy;
    SIZE vportExt;
    SIZE wndExt;
    POINT DCOrg;

    if (!hdc ) return FALSE;
  
    GetViewportExtEx( hdc,&vportExt);
    GetWindowExtEx(hdc, &wndExt);
    GetDCOrgEx(hdc,&DCOrg);

    /* compute device clipping region */

    if ( rc )
	rClip = *rc;
    else /* maybe we should just return FALSE? */
	GetClipBox( hdc, &rClip );

    if (prLClip)
	IntersectRect(&rClip,&rClip,prLClip);

    if( rClip.left >= rClip.right || rClip.top >= rClip.bottom )
    {
	return FALSE;
    }
   

   /// IntersectVisRect( hdc, rClip.left, rClip.top, 
   //                        rClip.right, rClip.bottom ); 


    /* translate coordinates */


    ldx = dx * wndExt.cx / vportExt.cx;
    ldy = dy * wndExt.cy / vportExt.cy;

    if (dx > 0)
	dest.x = (src.x = rClip.left) + ldx;
    else
	src.x = (dest.x = rClip.left) - ldx;

    if (dy > 0)
	dest.y = (src.y = rClip.top) + ldy;
    else
	src.y = (dest.y = rClip.top) - ldy;

    /* copy bits */

    if( rClip.right - rClip.left > ldx &&
	rClip.bottom - rClip.top > ldy )
    {
	ldx = rClip.right - rClip.left - ldx;
	ldy = rClip.bottom - rClip.top - ldy;

	if (!BitBlt( hdc, dest.x, dest.y, ldx, ldy,
		       hdc, src.x, src.y, SRCCOPY))
	{
	    return FALSE;
	}
    }

    /* restore clipping region */

    SelectClipRgn( hdc,NULL );


    /* compute update areas */

    //&& dc->w.hVisRgn
    if ( (hrgnUpdate || rcUpdate)  )
    {
	HRGN hrgn = (hrgnUpdate) ? hrgnUpdate : CreateRectRgn( 0,0,0,0 );
        HRGN hrgnClip;

        //LPtoDP( hdc, (LPPOINT)&rClip, 2 );
        OffsetRect( &rClip, DCOrg.x, DCOrg.y );
        hrgnClip = CreateRectRgnIndirect( &rClip );
        
        //CombineRgn( hrgn, dc->w.hVisRgn, hrgnClip, RGN_AND );
        OffsetRgn( hrgn, dx, dy );
        //CombineRgn( hrgn, dc->w.hVisRgn, hrgn, RGN_DIFF );
        CombineRgn( hrgn, hrgn, hrgnClip, RGN_AND );
        OffsetRgn( hrgn, -DCOrg.x, -DCOrg.y );

        if( rcUpdate ) GetRgnBox( hrgnUpdate, rcUpdate );

	if (!hrgnUpdate) DeleteObject( hrgn );
        DeleteObject( hrgnClip );     
    }

    return TRUE;
}





/*************************************************************************
 *             ScrollWindowEx   (USER.451)
 *
 * NOTE: Use this function instead of ScrollWindow
 */
INT STDCALL ScrollWindowEx( HWND hwnd, INT dx, INT dy,
                               const RECT *rect, const RECT *clipRect,
                               HRGN hrgnUpdate, LPRECT rcUpdate,
                               UINT flags )
{
    INT  retVal = NULLREGION;
    WINBOOL bCaret = FALSE, bOwnRgn = TRUE;
    RECT rc, cliprc;
    WND*   wnd = WIN_FindWndPtr( hwnd );

    if( !wnd || !WIN_IsWindowDrawable( wnd, TRUE )) return ERROR;

    if (rect == NULL) GetClientRect(hwnd, &rc);
    else rc = *rect;

    if (clipRect) IntersectRect(&cliprc,&rc,clipRect);
    else cliprc = rc;

    if (!IsRectEmpty(&cliprc) && (dx || dy))
    {

	HDC	hDC;
	WINBOOL  bUpdate = (rcUpdate || hrgnUpdate || flags & (SW_INVALIDATE | SW_ERASE));
	HRGN  hrgnClip = CreateRectRgnIndirect(&cliprc);


	rc = cliprc;
	bCaret = SCROLL_FixCaret(hwnd, &rc, flags);

	if( hrgnUpdate ) bOwnRgn = FALSE;
        else if( bUpdate ) hrgnUpdate = CreateRectRgn( 0, 0, 0, 0 );

	hDC = GetDCEx( hwnd, hrgnClip, DCX_CACHE | DCX_USESTYLE | 
		       ((flags & SW_SCROLLCHILDREN) ? DCX_NOCLIPCHILDREN : 0) );
	if( hDC != NULL)
	{

#if 0
	    if( dc->w.hVisRgn && bUpdate )
	    {
                OffsetRgn( hrgnClip, dc->w.DCOrgX, dc->w.DCOrgY );
		CombineRgn( hrgnUpdate, dc->w.hVisRgn, hrgnClip, RGN_AND );
		OffsetRgn( hrgnUpdate, dx, dy );
		CombineRgn( hrgnUpdate, dc->w.hVisRgn, hrgnUpdate, RGN_DIFF );
		CombineRgn( hrgnUpdate, hrgnUpdate, hrgnClip, RGN_AND );
                OffsetRgn( hrgnUpdate, -dc->w.DCOrgX, -dc->w.DCOrgY );

		if( rcUpdate ) GetRgnBox( hrgnUpdate, rcUpdate );
	    }
#endif
	    ReleaseDC(hwnd, hDC);

	}

	if( wnd->hrgnUpdate > 1 )
	{
	    if( rect || clipRect )
	    {
		if( (CombineRgn( hrgnClip, hrgnClip, 
				   wnd->hrgnUpdate, RGN_AND ) != NULLREGION) )
		{
		    CombineRgn( wnd->hrgnUpdate, wnd->hrgnUpdate, hrgnClip, RGN_DIFF );
		    OffsetRgn( hrgnClip, dx, dy );
		    CombineRgn( wnd->hrgnUpdate, wnd->hrgnUpdate, hrgnClip, RGN_OR );
		}
	    }
	    else  
		OffsetRgn( wnd->hrgnUpdate, dx, dy );
	}

	if( flags & SW_SCROLLCHILDREN )
	{
	    RECT	r;
	    WND* 	w;
	    for( w = wnd->child; w; w = w->next )
	    {
	
	         if( !clipRect || IntersectRect(&w->rectWindow, &r, &cliprc) )
		     SetWindowPos(w->hwndSelf, 0, w->rectWindow.left + dx,
				    w->rectWindow.top  + dy, 0,0, SWP_NOZORDER |
				    SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOREDRAW |
				    SWP_DEFERERASE );
	    }
	}

	if( flags & (SW_INVALIDATE | SW_ERASE) )
	    PAINT_RedrawWindow( hwnd, NULL, hrgnUpdate, RDW_INVALIDATE | RDW_ERASE |
		((flags & SW_ERASE) ? RDW_ERASENOW : 0) | ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0 ), 0 );

	if( bCaret )
	{
	    SetCaretPos( rc.left + dx, rc.top + dy );
	    ShowCaret(0);
	}

	if( bOwnRgn && hrgnUpdate ) DeleteObject( hrgnUpdate );
	DeleteObject( hrgnClip );
    }
    return retVal;
}


