/*
 * Caret functions
 *
 * Copyright 1993 David Metcalfe
 * Copyright 1996 Frans van Dorsselaer
 * Copyright 2001 Eric Pouech
 * Copyright 2002 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winternl.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(caret);

typedef struct
{
    HBITMAP  hBmp;
    UINT     timeout;
} CARET;

static CARET Caret = { 0, 500 };

#define TIMERID 0xffff  /* system timer id for the caret */


/*****************************************************************
 *               CARET_DisplayCaret
 */
static void CARET_DisplayCaret( HWND hwnd, const RECT *r )
{
    HDC hdc;
    HDC hCompDC;

    /* do not use DCX_CACHE here, for x,y,width,height are in logical units */
    if (!(hdc = GetDCEx( hwnd, 0, DCX_USESTYLE /*| DCX_CACHE*/ ))) return;
    hCompDC = CreateCompatibleDC(hdc);
    if (hCompDC)
    {
	HBITMAP	hPrevBmp;

	hPrevBmp = SelectObject(hCompDC, Caret.hBmp);
	BitBlt(hdc, r->left, r->top, r->right-r->left, r->bottom-r->top, hCompDC, 0, 0, SRCINVERT);
	SelectObject(hCompDC, hPrevBmp);
	DeleteDC(hCompDC);
    }
    ReleaseDC( hwnd, hdc );
}


/*****************************************************************
 *               CARET_Callback
 */
static void CALLBACK CARET_Callback( HWND hwnd, UINT msg, UINT_PTR id, DWORD ctime)
{
    BOOL ret;
    RECT r;
    int hidden = 0;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_STATE;
        req->handle = wine_server_user_handle( hwnd );
        req->x      = 0;
        req->y      = 0;
        req->hide   = 0;
        req->state  = -1;  /* toggle current state */
        if ((ret = !wine_server_call( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && !hidden) CARET_DisplayCaret( hwnd, &r );
}


/*****************************************************************
 *		CreateCaret (USER32.@)
 */
BOOL WINAPI CreateCaret( HWND hwnd, HBITMAP bitmap, INT width, INT height )
{
    BOOL ret;
    RECT r;
    int old_state = 0;
    int hidden = 0;
    HBITMAP hBmp = 0;
    HWND prev = 0;

    TRACE("hwnd=%p\n", hwnd);

    if (!hwnd) return FALSE;

    if (bitmap && (bitmap != (HBITMAP)1))
    {
        BITMAP bmp;
        if (!GetObjectA( bitmap, sizeof(bmp), &bmp )) return FALSE;
        width = bmp.bmWidth;
        height = bmp.bmHeight;
	bmp.bmBits = NULL;
	hBmp = CreateBitmapIndirect(&bmp);
	if (hBmp)
	{
	    /* copy the bitmap */
	    LPBYTE buf = HeapAlloc(GetProcessHeap(), 0, bmp.bmWidthBytes * bmp.bmHeight);
	    GetBitmapBits(bitmap, bmp.bmWidthBytes * bmp.bmHeight, buf);
	    SetBitmapBits(hBmp, bmp.bmWidthBytes * bmp.bmHeight, buf);
	    HeapFree(GetProcessHeap(), 0, buf);
	}
    }
    else
    {
	HDC hdc;

        if (!width) width = GetSystemMetrics(SM_CXBORDER);
        if (!height) height = GetSystemMetrics(SM_CYBORDER);

	/* create the uniform bitmap on the fly */
	hdc = GetDC(hwnd);
	if (hdc)
	{
	    HDC hMemDC = CreateCompatibleDC(hdc);
	    if (hMemDC)
	    {
		if ((hBmp = CreateCompatibleBitmap(hMemDC, width, height )))
		{
		    HBITMAP hPrevBmp = SelectObject(hMemDC, hBmp);
                    SetRect( &r, 0, 0, width, height );
		    FillRect(hMemDC, &r, bitmap ? GetStockObject(GRAY_BRUSH) : GetStockObject(WHITE_BRUSH));
		    SelectObject(hMemDC, hPrevBmp);
		}
		DeleteDC(hMemDC);
	    }
	    ReleaseDC(hwnd, hdc);
	}
    }
    if (!hBmp) return FALSE;

    SERVER_START_REQ( set_caret_window )
    {
        req->handle = wine_server_user_handle( hwnd );
        req->width  = width;
        req->height = height;
        if ((ret = !wine_server_call_err( req )))
        {
            prev      = wine_server_ptr_handle( reply->previous );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;
    if (!ret) return FALSE;

    if (prev && !hidden)  /* hide the previous one */
    {
        /* FIXME: won't work if prev belongs to a different process */
        KillSystemTimer( prev, TIMERID );
        if (old_state) CARET_DisplayCaret( prev, &r );
    }

    if (Caret.hBmp) DeleteObject( Caret.hBmp );
    Caret.hBmp = hBmp;
    Caret.timeout = GetProfileIntA( "windows", "CursorBlinkRate", 500 );
    return TRUE;
}


/*****************************************************************
 *		DestroyCaret (USER32.@)
 */
BOOL WINAPI DestroyCaret(void)
{
    BOOL ret;
    HWND prev = 0;
    RECT r;
    int old_state = 0;
    int hidden = 0;

    SERVER_START_REQ( set_caret_window )
    {
        req->handle = 0;
        req->width  = 0;
        req->height = 0;
        if ((ret = !wine_server_call_err( req )))
        {
            prev      = wine_server_ptr_handle( reply->previous );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && prev && !hidden)
    {
        /* FIXME: won't work if prev belongs to a different process */
        KillSystemTimer( prev, TIMERID );
        if (old_state) CARET_DisplayCaret( prev, &r );
    }
    if (Caret.hBmp) DeleteObject( Caret.hBmp );
    Caret.hBmp = 0;
    return ret;
}


/*****************************************************************
 *		SetCaretPos (USER32.@)
 */
BOOL WINAPI SetCaretPos( INT x, INT y )
{
    BOOL ret;
    HWND hwnd = 0;
    RECT r;
    int old_state = 0;
    int hidden = 0;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_POS|SET_CARET_STATE;
        req->handle = 0;
        req->x      = x;
        req->y      = y;
        req->hide   = 0;
        req->state  = 1;
        if ((ret = !wine_server_call_err( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;
    if (ret && !hidden && (x != r.left || y != r.top))
    {
        if (old_state) CARET_DisplayCaret( hwnd, &r );
        r.right += x - r.left;
        r.bottom += y - r.top;
        r.left = x;
        r.top = y;
        CARET_DisplayCaret( hwnd, &r );
        SetSystemTimer( hwnd, TIMERID, Caret.timeout, CARET_Callback );
    }
    return ret;
}


/*****************************************************************
 *		HideCaret (USER32.@)
 */
BOOL WINAPI HideCaret( HWND hwnd )
{
    BOOL ret;
    RECT r;
    int old_state = 0;
    int hidden = 0;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_HIDE|SET_CARET_STATE;
        req->handle = wine_server_user_handle( hwnd );
        req->x      = 0;
        req->y      = 0;
        req->hide   = 1;
        req->state  = 0;
        if ((ret = !wine_server_call_err( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            old_state = reply->old_state;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && !hidden)
    {
        if (old_state) CARET_DisplayCaret( hwnd, &r );
        KillSystemTimer( hwnd, TIMERID );
    }
    return ret;
}


/*****************************************************************
 *		ShowCaret (USER32.@)
 */
BOOL WINAPI ShowCaret( HWND hwnd )
{
    BOOL ret;
    RECT r;
    int hidden = 0;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = SET_CARET_HIDE|SET_CARET_STATE;
        req->handle = wine_server_user_handle( hwnd );
        req->x      = 0;
        req->y      = 0;
        req->hide   = -1;
        req->state  = 1;
        if ((ret = !wine_server_call_err( req )))
        {
            hwnd      = wine_server_ptr_handle( reply->full_handle );
            r.left    = reply->old_rect.left;
            r.top     = reply->old_rect.top;
            r.right   = reply->old_rect.right;
            r.bottom  = reply->old_rect.bottom;
            hidden    = reply->old_hide;
        }
    }
    SERVER_END_REQ;

    if (ret && (hidden == 1))  /* hidden was 1 so it's now 0 */
    {
        CARET_DisplayCaret( hwnd, &r );
        SetSystemTimer( hwnd, TIMERID, Caret.timeout, CARET_Callback );
    }
    return ret;
}


/*****************************************************************
 *		GetCaretPos (USER32.@)
 */
BOOL WINAPI GetCaretPos( LPPOINT pt )
{
    BOOL ret;

    SERVER_START_REQ( set_caret_info )
    {
        req->flags  = 0;  /* don't set anything */
        req->handle = 0;
        req->x      = 0;
        req->y      = 0;
        req->hide   = 0;
        req->state  = 0;
        if ((ret = !wine_server_call_err( req )))
        {
            pt->x = reply->old_rect.left;
            pt->y = reply->old_rect.top;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/*****************************************************************
 *		SetCaretBlinkTime (USER32.@)
 */
BOOL WINAPI SetCaretBlinkTime( UINT msecs )
{
    TRACE("msecs=%d\n", msecs);

    Caret.timeout = msecs;
/*    if (Caret.hwnd) CARET_SetTimer(); FIXME */
    return TRUE;
}


/*****************************************************************
 *		GetCaretBlinkTime (USER32.@)
 */
UINT WINAPI GetCaretBlinkTime(void)
{
    return Caret.timeout;
}
