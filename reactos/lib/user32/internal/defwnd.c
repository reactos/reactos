/*
 * Default window procedure
 *
 * Copyright 1993, 1996 Alexandre Julliard
 *	     1995 Alex Korobka
 */

#include <stdlib.h>
#include <windows.h>
#include <user32/win.h>
#include <user32/nc.h>
#include <user32/heapdup.h>
#include <user32/winpos.h>
#include <user32/dce.h>
#include <user32/sysmetr.h>
#include <user32/paint.h>
#include <user32/defwnd.h>
#include <user32/debug.h>


void  FillWindow( HWND hwndParent, HWND hwnd, HDC hdc, HBRUSH hbrush );

#define WM_CTLCOLOR             0x0019
#define WM_ISACTIVEICON         0x0035
#define WM_DROPOBJECT	    	0x022A
#define WM_QUERYDROPOBJECT  	0x022B

#define DRAG_FILE		0x454C4946

  /* Last COLOR id */
#define COLOR_MAX   COLOR_BTNHIGHLIGHT

  /* bits in the dwKeyData */
#define KEYDATA_ALT 		0x2000
#define KEYDATA_PREVSTATE	0x4000

static short iF10Key = 0;
static short iMenuSysKey = 0;

/***********************************************************************
 *           DEFWND_HandleWindowPosChanged
 *
 * Handle the WM_WINDOWPOSCHANGED message.
 */
void DEFWND_HandleWindowPosChanged( WND *wndPtr, UINT flags )
{
    WPARAM wp = SIZE_RESTORED;

    if (!(flags & SWP_NOCLIENTMOVE))
        SendMessage( wndPtr->hwndSelf, WM_MOVE, 0,
                    MAKELONG(wndPtr->rectClient.left, wndPtr->rectClient.top));
    if (!(flags & SWP_NOCLIENTSIZE))
    {
        if (wndPtr->dwStyle & WS_MAXIMIZE) wp = SIZE_MAXIMIZED;
        else if (wndPtr->dwStyle & WS_MINIMIZE) wp = SIZE_MINIMIZED;

        SendMessage( wndPtr->hwndSelf, WM_SIZE, wp, 
                     MAKELONG(wndPtr->rectClient.right-wndPtr->rectClient.left,
                            wndPtr->rectClient.bottom-wndPtr->rectClient.top));
    }
}


/***********************************************************************
 *           DEFWND_SetText
 *
 * Set the window text.
 */

void DEFWND_SetText( WND *wndPtr,const void *text )
{
 	if ( wndPtr->class->bUnicode )
		DEFWND_SetTextW( wndPtr, (LPCWSTR) text );
	else
		DEFWND_SetTextA( wndPtr, (LPCSTR) text );
	   
}

void DEFWND_SetTextA( WND *wndPtr, LPCSTR text )
{
    if (!text) text = "";
    if (wndPtr->text) HeapFree( GetProcessHeap(), 0, wndPtr->text );
    wndPtr->text = (void *)HEAP_strdupA( GetProcessHeap(), 0, text );   
    NC_HandleNCPaint( wndPtr->hwndSelf , (HRGN)1 );  /* Repaint caption */
}


void DEFWND_SetTextW( WND *wndPtr, LPCWSTR text )
{
    if (!text) text = L"";
    if (wndPtr->text) HeapFree( GetProcessHeap(), 0, wndPtr->text );
    wndPtr->text = (void *)HEAP_strdupW( GetProcessHeap(), 0, text );  
    NC_HandleNCPaint( wndPtr->hwndSelf , (HRGN)1 );  /* Repaint caption */  

}
/***********************************************************************
 *           DEFWND_ControlColor
 *
 * Default colors for control painting.
 */
HBRUSH DEFWND_ControlColor( HDC hDC, UINT ctlType )
{
    if( ctlType == CTLCOLOR_SCROLLBAR)
    {
	HBRUSH hb = GetSysColorBrush(COLOR_SCROLLBAR);
	SetBkColor( hDC, RGB(255, 255, 255) );
	SetTextColor( hDC, RGB(0, 0, 0) );
	UnrealizeObject( hb );
        return hb;
    }

    SetTextColor( hDC, GetSysColor(COLOR_WINDOWTEXT));

    if (TWEAK_WineLook > WIN31_LOOK) {
	if ((ctlType == CTLCOLOR_EDIT) || (ctlType == CTLCOLOR_LISTBOX))
	    SetBkColor( hDC, GetSysColor(COLOR_WINDOW) );
	else {
	    SetBkColor( hDC, GetSysColor(COLOR_3DFACE) );
	    return GetSysColorBrush(COLOR_3DFACE);
	}
    }
    else
	SetBkColor( hDC, GetSysColor(COLOR_WINDOW) );
    return GetSysColorBrush(COLOR_WINDOW);
}


/***********************************************************************
 *           DEFWND_SetRedraw
 */
void DEFWND_SetRedraw( WND* wndPtr, WPARAM wParam )
{
    WINBOOL bVisible = wndPtr->dwStyle & WS_VISIBLE;

    DPRINT("%04x %i\n", (UINT)wndPtr->hwndSelf, (wParam!=0) );

    if( wParam )
    {
	if( !bVisible )
	{
	    //wndPtr->dwStyle |= WS_VISIBLE;
	    DCE_InvalidateDCE( wndPtr, &wndPtr->rectWindow );
	}
    }
    else if( bVisible )
    {
	if( wndPtr->dwStyle & WS_MINIMIZE ) wParam = RDW_VALIDATE;
	else wParam = RDW_ALLCHILDREN | RDW_VALIDATE;

	PAINT_RedrawWindow( wndPtr->hwndSelf, NULL, 0, wParam, 0 );
	DCE_InvalidateDCE( wndPtr, &wndPtr->rectWindow );
//	wndPtr->dwStyle &= ~WS_VISIBLE;
    }
}

/***********************************************************************
 *           DEFWND_DefWinProc
 *
 * Default window procedure for messages that are the same in Win and Win.
 */
LRESULT DEFWND_DefWinProc( WND *wndPtr, UINT msg, WPARAM wParam,
                                  LPARAM lParam )
{
    POINT pt;
    switch(msg)
    {
    case WM_NCPAINT:
	return NC_HandleNCPaint( wndPtr->hwndSelf, (HRGN)wParam );

    case WM_NCHITTEST:
	pt.x = LOWORD(lParam);  
        pt.y = HIWORD(lParam);  
        return NC_HandleNCHitTest( wndPtr->hwndSelf, pt);

    case WM_NCLBUTTONDOWN:
	return NC_HandleNCLButtonDown( wndPtr, wParam, lParam );

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
	return NC_HandleNCLButtonDblClk( wndPtr, wParam, lParam );

    case WM_RBUTTONDOWN:
    case WM_NCRBUTTONDOWN:
        if ((wndPtr->flags & WIN_ISWIN) || (TWEAK_WineLook > WIN31_LOOK))
        {
	    ClientToScreen(wndPtr->hwndSelf, (LPPOINT)&lParam);
            MSG_SendMessage( wndPtr, WM_CONTEXTMENU,
			    (WPARAM)wndPtr->hwndSelf,(LPARAM) lParam);
        }
        break;

    case WM_CONTEXTMENU:
	if( wndPtr->dwStyle & WS_CHILD )
	    SendMessageA( wndPtr->parent->hwndSelf, msg, wParam, lParam );
	else
	  if (wndPtr->hSysMenu)
	  { /*
	    TrackPopupMenu(wndPtr->hSysMenu,TPM_LEFTALIGN | TPM_RETURNCMD,LOWORD(lParam),HIWORD(lParam),0,wndPtr->hwndSelf,NULL);
	    DestroyMenu(wndPtr->hSysMenu);
	    */
	    DPRINT("Fixme Display default popup menu\n");
	  /* Track system popup if click was in the caption area. */
	  }
	break;

    case WM_NCACTIVATE:
	return NC_HandleNCActivate( wndPtr, wParam );

    case WM_NCDESTROY:
	if (wndPtr->text) HeapFree( GetProcessHeap(), 0, wndPtr->text );
	wndPtr->text = NULL;
	if (wndPtr->pVScroll) HeapFree( GetProcessHeap(), 0, wndPtr->pVScroll );
	if (wndPtr->pHScroll) HeapFree( GetProcessHeap(), 0, wndPtr->pHScroll );
        wndPtr->pVScroll = wndPtr->pHScroll = NULL;
	return 0;

    case WM_PAINTICON:
    case WM_PAINT:
	{
	    PAINTSTRUCT ps;
	    HDC hdc = BeginPaint( wndPtr->hwndSelf, &ps );
	    if( hdc ) 
	    {
	      if( (wndPtr->dwStyle & WS_MINIMIZE) && wndPtr->class->hIcon )
	      {
	        int x = (wndPtr->rectWindow.right - wndPtr->rectWindow.left -
			SYSMETRICS_CXICON)/2;
	        int y = (wndPtr->rectWindow.bottom - wndPtr->rectWindow.top -
			SYSMETRICS_CYICON)/2;
		DPRINT("Painting class icon: vis rect=(%i,%i - %i,%i)\n",
		ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right, ps.rcPaint.bottom );
	        DrawIcon( hdc, x, y, wndPtr->class->hIcon );
	      }
	      EndPaint( wndPtr->hwndSelf, &ps );
	    }
	    return 0;
	}

    case WM_SETREDRAW:
	DEFWND_SetRedraw( wndPtr, wParam );
        return 0;

    case WM_CLOSE:
	DestroyWindow( wndPtr->hwndSelf );
	return 0;

    case WM_MOUSEACTIVATE:
	if (wndPtr->dwStyle & WS_CHILD)
	{
	    LONG ret = SendMessage( wndPtr->parent->hwndSelf,
                                      WM_MOUSEACTIVATE, wParam, lParam );
	    if (ret) return ret;
	}

	/* Caption clicks are handled by the NC_HandleNCLButtonDown() */ 
	return (LOWORD(lParam) == HTCAPTION) ? MA_NOACTIVATE : MA_ACTIVATE;

    case WM_ACTIVATE:
	if (LOWORD(wParam) != WA_INACTIVE) 
		SetWindowPos(wndPtr->hwndSelf, HWND_TOP, 0, 0, 0, 0,
			 SWP_NOMOVE | SWP_NOSIZE);
	break;

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
	{
	    if (!wndPtr->class->hbrBackground) return 0;

	    if (wndPtr->class->hbrBackground <= (HBRUSH)(COLOR_MAX+1))
            {
                HBRUSH hbrush = CreateSolidBrush( 
                       GetSysColor(((DWORD)wndPtr->class->hbrBackground)-1));
                 FillWindow( GetParent(wndPtr->hwndSelf), wndPtr->hwndSelf,
                             (HDC)wParam, hbrush);
                 DeleteObject( hbrush );
            }
            else FillWindow( GetParent(wndPtr->hwndSelf), wndPtr->hwndSelf,
                             (HDC)wParam, wndPtr->class->hbrBackground );
	    return 1;
	}

    case WM_GETDLGCODE:
	return 0;

    case WM_CTLCOLORMSGBOX:
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX:
    case WM_CTLCOLORBTN:
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLORSCROLLBAR:
	return (LRESULT)DEFWND_ControlColor( (HDC)wParam, msg - WM_CTLCOLORMSGBOX );

    case WM_CTLCOLOR:
	return (LRESULT)DEFWND_ControlColor( (HDC)wParam, HIWORD(lParam) );
	
    case WM_GETTEXTLENGTH:
        if (wndPtr->text && wndPtr->class->bUnicode == TRUE) 
		return (LRESULT)lstrlenW(wndPtr->text);
	else if (wndPtr->text && wndPtr->class->bUnicode == FALSE) 
		return (LRESULT)lstrlenA(wndPtr->text);
        return 0;

    case WM_SETCURSOR:
	if (wndPtr->dwStyle & WS_CHILD)
	    if (SendMessage(wndPtr->parent->hwndSelf, WM_SETCURSOR,
                            wParam, lParam))
		return TRUE;
	return NC_HandleSetCursor( wndPtr->hwndSelf, wParam, lParam );

    case WM_SYSCOMMAND:
	pt.x = LOWORD(lParam);  
        pt.y = HIWORD(lParam);  
        return NC_HandleSysCommand( wndPtr->hwndSelf, wParam,pt);

    case WM_KEYDOWN:
	if(wParam == VK_F10) iF10Key = VK_F10;
	break;

    case WM_SYSKEYDOWN:
	if( HIWORD(lParam) & KEYDATA_ALT )
	{
	    /* if( HIWORD(lParam) & ~KEYDATA_PREVSTATE ) */
	      if( wParam == VK_MENU && !iMenuSysKey )
		iMenuSysKey = 1;
	      else
		iMenuSysKey = 0;
	    
	    iF10Key = 0;

	    if( wParam == VK_F4 )	/* try to close the window */
	    {
		HWND hWnd = WIN_GetTopParent( wndPtr->hwndSelf );
		wndPtr = WIN_FindWndPtr( hWnd );
		if( wndPtr && !(wndPtr->class->style & CS_NOCLOSE) )
		    PostMessage( hWnd, WM_SYSCOMMAND, SC_CLOSE, 0 );
	    }
	} 
	else if( wParam == VK_F10 )
	        iF10Key = 1;
	     else
	        if( wParam == VK_ESCAPE && (GetKeyState(VK_SHIFT) & 0x8000))
		    SendMessage( wndPtr->hwndSelf, WM_SYSCOMMAND,
                                  (WPARAM)SC_KEYMENU, (LPARAM)VK_SPACE);
	break;

    case WM_KEYUP:
    case WM_SYSKEYUP:
	/* Press and release F10 or ALT */
	if (((wParam == VK_MENU) && iMenuSysKey) ||
            ((wParam == VK_F10) && iF10Key))
	      SendMessage( WIN_GetTopParent(wndPtr->hwndSelf),
                             WM_SYSCOMMAND, SC_KEYMENU, 0L );
	iMenuSysKey = iF10Key = 0;
        break;

    case WM_SYSCHAR:
	iMenuSysKey = 0;
	if (wParam == VK_RETURN && (wndPtr->dwStyle & WS_MINIMIZE))
        {
	    PostMessage( wndPtr->hwndSelf, WM_SYSCOMMAND,
                           (WPARAM)SC_RESTORE, 0L ); 
	    break;
        } 
	if ((HIWORD(lParam) & KEYDATA_ALT) && wParam)
        {
	    if (wParam == VK_TAB || wParam == VK_ESCAPE) break;
	    if (wParam == VK_SPACE && (wndPtr->dwStyle & WS_CHILD))
                SendMessage( wndPtr->parent->hwndSelf, msg, wParam, lParam );
	    else
                SendMessage( wndPtr->hwndSelf, WM_SYSCOMMAND,
                               (WPARAM)SC_KEYMENU, (LPARAM)(DWORD)wParam );
        } 
	else /* check for Ctrl-Esc */
            if (wParam != VK_ESCAPE) MessageBeep(0);
	break;

    case WM_SHOWWINDOW:
        if (!lParam) return 0; /* sent from ShowWindow */
        if (!(wndPtr->dwStyle & WS_POPUP) || !wndPtr->owner) return 0;
        if ((wndPtr->dwStyle & WS_VISIBLE) && wParam) return 0;
	else if (!(wndPtr->dwStyle & WS_VISIBLE) && !wParam) return 0;
        ShowWindow( wndPtr->hwndSelf, wParam ? SW_SHOWNOACTIVATE : SW_HIDE );
	break; 

    case WM_CANCELMODE:
	//if (wndPtr->parent == WIN_GetDesktop()) 
		//EndMenu();
	if (GetCapture() == wndPtr->hwndSelf) ReleaseCapture();
	break;

    case WM_VKEYTOITEM:
    case WM_CHARTOITEM:
	return -1;

    case WM_DROPOBJECT:
	return DRAG_FILE;  

    case WM_QUERYDROPOBJECT:
	if (wndPtr->dwExStyle & WS_EX_ACCEPTFILES) return 1;
	break;

    case WM_QUERYDRAGICON:
        {
#if 0
            HICON hIcon=0;
            UINT len;

            if( (hIcon=wndPtr->class->hCursor) ) return (LRESULT)hIcon;
            for(len=1; len<64; len++)
                if((hIcon=LoadIcon(wndPtr->hInstance,MAKEINTRESOURCE(len))))
                    return (LRESULT)hIcon;
            return (LRESULT)LoadIcon(0,IDI_APPLICATION);
        
#endif
	}
	return 0;
        break;

    case WM_ISACTIVEICON:
	return ((wndPtr->flags & WIN_NCACTIVATED) != 0);

    case WM_QUERYOPEN:
    case WM_QUERYENDSESSION:
	return 1;
    }
    return 0;
}




