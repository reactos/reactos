/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995 Alexandre Julliard
 *
 * FIXME: Do not repaint full nonclient area all the time. Instead, compute 
 *	  intersection with hrgnUpdate (which should be moved from client to 
 *	  window coords as well, lookup 'the pain' comment in the winpos.c).
 */
#include <windows.h>
#include <user32/paint.h>
#include <user32/win.h>
#include <user32/queue.h>
#include <user32/dce.h>
#include <user32/debug.h>


  /* Last CTLCOLOR id */
//#define CTLCOLOR_MAX   CTLCOLOR_STATIC





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
	if(hrgnUpdate > (HRGN)1)
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
	if (wndPtr->hrgnUpdate > (HRGN)1)
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
	if( wndPtr->hrgnUpdate > (HRGN)1 )
	    CombineRgn(hrgn, wndPtr->hrgnUpdate, 0, RGN_COPY);

	/* do ugly coordinate translations in dce.c */

	ret = DCE_ExcludeRgn( hdc, wndPtr, hrgn );
	DeleteObject( hrgn );
	return ret;
    } 
    return GetClipBox( hdc, &rect );
}


