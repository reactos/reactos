/*
 * Static control
 *
 * Copyright  David W. Metcalfe, 1993
 *
 */

#include <windows.h>
#include <user32/win.h>
#include <user32/static.h>
#include <user32/debug.h>
#include <user32/syscolor.h>
#include <user32/nc.h>
#include <user32/defwnd.h>


/* Static Control Styles */




//#define SS_ETCHEDHORZ       0x00000010L
//#define SS_ETCHEDVERT       0x00000011L
//#define SS_ETCHEDFRAME      0x00000012L
#define SS_TYPEMASK         0x0000001FL

//#define SS_NOPREFIX         0x00000080L
//#define SS_NOTIFY           0x00000100L
//#define SS_CENTERIMAGE      0x00000200L
//#define SS_RIGHTJUST        0x00000400L
//#define SS_REALSIZEIMAGE    0x00000800L
//#define SS_SUNKEN           0x00001000L

/* Static Control Messages */

//#define STM_SETICON         0x0170
//#define STM_GETICON         0x0171
//#define STM_SETIMAGE        0x0172
//#define STM_GETIMAGE        0x0173

static void STATIC_PaintTextfn( WND *wndPtr, HDC hdc );
static void STATIC_PaintRectfn( WND *wndPtr, HDC hdc );
static void STATIC_PaintIconfn( WND *wndPtr, HDC hdc );
static void STATIC_PaintBitmapfn( WND *wndPtr, HDC hdc );
static void STATIC_PaintEtchedfn( WND *wndPtr, HDC hdc );

static COLORREF color_windowframe, color_background, color_window;


typedef void (*pfPaint)( WND *, HDC );

static pfPaint staticPaintFunc[SS_TYPEMASK+1] =
{
    STATIC_PaintTextfn,      /* SS_LEFT */
    STATIC_PaintTextfn,      /* SS_CENTER */
    STATIC_PaintTextfn,      /* SS_RIGHT */
    STATIC_PaintIconfn,      /* SS_ICON */
    STATIC_PaintRectfn,      /* SS_BLACKRECT */
    STATIC_PaintRectfn,      /* SS_GRAYRECT */
    STATIC_PaintRectfn,      /* SS_WHITERECT */
    STATIC_PaintRectfn,      /* SS_BLACKFRAME */
    STATIC_PaintRectfn,      /* SS_GRAYFRAME */
    STATIC_PaintRectfn,      /* SS_WHITEFRAME */
    NULL,                    /* Not defined */
    STATIC_PaintTextfn,      /* SS_SIMPLE */
    STATIC_PaintTextfn,      /* SS_LEFTNOWORDWRAP */
    NULL,                    /* SS_OWNERDRAW */
    STATIC_PaintBitmapfn,    /* SS_BITMAP */
    NULL,                    /* SS_ENHMETAFILE */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDHORIZ */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDVERT */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDFRAME */
};

HICON STATIC_LoadIcon(WND *wndPtr,const void *name )
{
	HICON hIcon;
	if ( wndPtr->class->bUnicode ) {
		hIcon = LoadIconW(wndPtr->hInstance,(LPCWSTR)name);
	}			
	else
		hIcon = LoadIconA(wndPtr->hInstance,(LPCSTR)name);

	return hIcon;
}

/***********************************************************************
 *           STATIC_SetIcon
 *
 * Set the icon for an SS_ICON control.
 */
HICON STATIC_SetIcon( WND *wndPtr, HICON hicon )
{
    HICON prevIcon;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;
    ICONINFO iconinfo;
    BITMAP info;

    if ( !GetIconInfo(hicon,	&iconinfo ) )
	return NULL;

    if ( iconinfo.hbmColor )
    	GetObject(iconinfo.hbmColor, sizeof(BITMAP),&info);
    else
	GetObject(iconinfo.hbmMask, sizeof(BITMAP),&info);
   
    if ((wndPtr->dwStyle & SS_TYPEMASK) != SS_ICON) return 0;

    prevIcon = infoPtr->hIcon;
    infoPtr->hIcon = hicon;
    if (hicon)
    {
        SetWindowPos( wndPtr->hwndSelf, 0, 0, 0, info.bmWidth, info.bmHeight,
                        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
    }
    return prevIcon;
}


HBITMAP STATIC_LoadBitmap(WND *wndPtr,const void *name )
{
	HBITMAP hBitmap;
	if ( wndPtr->class->bUnicode ) {
		hBitmap = LoadBitmapW(wndPtr->hInstance,(LPCWSTR)name);
	}			
	else
		hBitmap = LoadBitmapA(wndPtr->hInstance,(LPCSTR)name);
	return hBitmap;
}
/***********************************************************************
 *           STATIC_SetBitmap
 *
 * Set the bitmap for an SS_BITMAP control.
 */
HICON STATIC_SetBitmap( WND *wndPtr, HICON hicon )
{

    HICON prevIcon;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;
    BITMAP info;

    if ( hicon == NULL )
	return NULL;

    GetObject(hicon, sizeof(BITMAP),&info);

    if ((wndPtr->dwStyle & SS_TYPEMASK) != SS_BITMAP) return 0;
   
    prevIcon = infoPtr->hIcon;
    infoPtr->hIcon = hicon;
    if (hicon)
    {
        SetWindowPos( wndPtr->hwndSelf, 0, 0, 0, info.bmWidth, info.bmHeight,
                        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
    }

    return prevIcon;

}




/***********************************************************************
 *           StaticWndProc
 */
LRESULT WINAPI StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                              LPARAM lParam )
{
    LRESULT lResult = 0;
    WND *wndPtr = WIN_FindWndPtr(hWnd);
    LONG style = wndPtr->dwStyle & SS_TYPEMASK;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    switch (uMsg)
    {
    case WM_NCCREATE: {
        CREATESTRUCT *cs = (CREATESTRUCT *)lParam;

	if ((TWEAK_WineLook > WIN31_LOOK) && (wndPtr->dwStyle & SS_SUNKEN))
	    wndPtr->dwExStyle |= WS_EX_STATICEDGE;

        if (style == SS_ICON)
        {
            if (cs->lpszName) {
	    	STATIC_SetIcon( wndPtr,STATIC_LoadIcon(wndPtr,cs->lpszName));
	    }
            return 1;
        }
	if (style == SS_BITMAP)
	{
            if (cs->lpszName) {
                STATIC_SetBitmap( wndPtr,STATIC_LoadBitmap(wndPtr,cs->lpszName));
	    }
            return 1;
	}

	if ( wndPtr->class->bUnicode )
        	return DefWindowProcW( hWnd, uMsg, wParam, lParam );
	else
		return DefWindowProcA( hWnd, uMsg, wParam, lParam );
    }
    case WM_CREATE:
        if (style < 0L || style > SS_TYPEMASK)
        {
            DPRINT( "Unknown style 0x%02lx\n", style );
            lResult = -1L;
            break;
        }
        /* initialise colours */
        color_windowframe  = GetSysColor(COLOR_WINDOWFRAME);
        color_background   = GetSysColor(COLOR_BACKGROUND);
        color_window       = GetSysColor(COLOR_WINDOW);
        break;

    case WM_NCDESTROY:
        if (style == SS_ICON) {
/*
 * FIXME
 *           DestroyIcon( STATIC_SetIcon( wndPtr, 0 ) );
 * 
 * We don't want to do this yet because DestroyIcon is broken. If the icon
 * had already been loaded by the application the last thing we want to do is
 * GlobalFree the handle.
 */
        } else {
		if ( wndPtr->class->bUnicode )
        		lResult = DefWindowProcW( hWnd, uMsg, wParam, lParam );
		else
			lResult = DefWindowProcA( hWnd, uMsg, wParam, lParam );
        
	}
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            BeginPaint( hWnd, &ps );
            if (staticPaintFunc[style])
                (staticPaintFunc[style])( wndPtr, ps.hdc );
            EndPaint( hWnd, &ps );
        }
        break;

    case WM_ENABLE:
        InvalidateRect( hWnd, NULL, FALSE );
        break;

    case WM_SYSCOLORCHANGE:
        color_windowframe  = GetSysColor(COLOR_WINDOWFRAME);
        color_background   = GetSysColor(COLOR_BACKGROUND);
        color_window       = GetSysColor(COLOR_WINDOW);
        InvalidateRect( hWnd, NULL, TRUE );
        break;

    case WM_SETTEXT:
   	if (style == SS_ICON)
        {
            if (lParam) {
		STATIC_SetIcon( wndPtr,STATIC_LoadIcon(wndPtr,(const void *)lParam));		
	    }
            
        }
	else if (style == SS_BITMAP)
	{
            if (lParam) {
                STATIC_SetBitmap( wndPtr,STATIC_LoadBitmap(wndPtr,(const void *)lParam));
	    }
            
	}
      	else {
		DEFWND_SetText( wndPtr, (const void *)lParam );
		
        }
        InvalidateRect( hWnd, NULL, FALSE );
        UpdateWindow( hWnd );
        break;

    case WM_SETFONT:
        if (style == SS_ICON) return 0;
        if (style == SS_BITMAP) return 0;
        infoPtr->hFont = (HFONT)wParam;
        if (LOWORD(lParam))
        {
            InvalidateRect( hWnd, NULL, FALSE );
            UpdateWindow( hWnd );
        }
        break;

    case WM_GETFONT:
        return infoPtr->hFont;

    case WM_NCHITTEST:
        return HTTRANSPARENT;

    case WM_GETDLGCODE:
        return DLGC_STATIC;

    case STM_GETIMAGE:
    case STM_GETICON:
        return infoPtr->hIcon;

    case STM_SETIMAGE:
    	/* FIXME: handle wParam */
        lResult = STATIC_SetBitmap( wndPtr, (HBITMAP)lParam );
        InvalidateRect( hWnd, NULL, FALSE );
        UpdateWindow( hWnd );
	break;


    case STM_SETICON:
        lResult = STATIC_SetIcon( wndPtr, (HICON)wParam );
        InvalidateRect( hWnd, NULL, FALSE );
        UpdateWindow( hWnd );
        break;

    default:
	if ( wndPtr->class->bUnicode )
        	lResult = DefWindowProcW( hWnd, uMsg, wParam, lParam );
	else
		lResult = DefWindowProcA( hWnd, uMsg, wParam, lParam );
        break;
    }
    
    return lResult;
}


static void STATIC_PaintTextfn( WND *wndPtr, HDC hdc )
{
    RECT rc;
    HBRUSH hBrush;
    WORD wFormat;

    LONG style = wndPtr->dwStyle;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    GetClientRect( wndPtr->hwndSelf, &rc);

    switch (style & SS_TYPEMASK)
    {
    case SS_LEFT:
	wFormat = DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_CENTER:
	wFormat = DT_CENTER | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_RIGHT:
	wFormat = DT_RIGHT | DT_EXPANDTABS | DT_WORDBREAK | DT_NOCLIP;
	break;

    case SS_SIMPLE:
	wFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOCLIP;
	break;

    case SS_LEFTNOWORDWRAP:
	wFormat = DT_LEFT | DT_SINGLELINE | DT_EXPANDTABS | DT_VCENTER | DT_NOCLIP;
	break;

    default:
        return;
    }

    if (style & SS_NOPREFIX)
	wFormat |= DT_NOPREFIX;

    if (infoPtr->hFont) SelectObject( hdc, infoPtr->hFont );
    hBrush = MSG_SendMessage( wndPtr->parent, WM_CTLCOLORSTATIC,
                             (WPARAM)hdc,(LPARAM) wndPtr->hwndSelf );
    if (!hBrush) hBrush = GetStockObject(WHITE_BRUSH);
    FillRect( hdc, &rc, hBrush ); 
    if (wndPtr->text)  {
	if ( wndPtr->class->bUnicode ) {
		DrawTextW( hdc, wndPtr->text, -1, &rc, wFormat );
	}
	else {
		DrawTextA( hdc, wndPtr->text, -1, &rc, wFormat );
	}
    }

}	
	

static void STATIC_PaintRectfn( WND *wndPtr, HDC hdc )
{
    RECT rc;
    HBRUSH hBrush;

    GetClientRect( wndPtr->hwndSelf, &rc);
    
    switch (wndPtr->dwStyle & SS_TYPEMASK)
    {
    case SS_BLACKRECT:
	hBrush = CreateSolidBrush(color_windowframe);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_GRAYRECT:
	hBrush = CreateSolidBrush(color_background);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_WHITERECT:
	hBrush = CreateSolidBrush(color_window);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_BLACKFRAME:
	hBrush = CreateSolidBrush(color_windowframe);
        FrameRect( hdc, &rc, hBrush );
	break;
    case SS_GRAYFRAME:
	hBrush = CreateSolidBrush(color_background);
        FrameRect( hdc, &rc, hBrush );
	break;
    case SS_WHITEFRAME:
	hBrush = CreateSolidBrush(color_window);
        FrameRect( hdc, &rc, hBrush );
	break;
    default:
        return;
    }
    DeleteObject( hBrush );
}


static void STATIC_PaintIconfn( WND *wndPtr, HDC hdc )
{
    RECT rc;
    HBRUSH hbrush;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;

    GetClientRect( wndPtr->hwndSelf, &rc );
    hbrush = MSG_SendMessage( wndPtr->parent, WM_CTLCOLORSTATIC, 
		(WPARAM)hdc, (LPARAM)wndPtr->hwndSelf );
    FillRect( hdc, &rc, hbrush );
    if (infoPtr->hIcon) DrawIcon( hdc, rc.left, rc.top, infoPtr->hIcon );
}

static void STATIC_PaintBitmapfn(WND *wndPtr, HDC hdc )
{
    RECT rc;
    HBRUSH hbrush;
    STATICINFO *infoPtr = (STATICINFO *)wndPtr->wExtra;
    HDC hMemDC;
    HBITMAP oldbitmap;

    GetClientRect( wndPtr->hwndSelf, &rc );
    hbrush = MSG_SendMessage( wndPtr->parent, WM_CTLCOLORSTATIC,
                             (WPARAM)hdc,(LPARAM) wndPtr->hwndSelf );
    FillRect( hdc, &rc, hbrush );
    if (infoPtr->hIcon) {
	BITMAP bmp;
	GetObject( infoPtr->hIcon, sizeof(BITMAP),&bmp);
  
        if (!(hMemDC = CreateCompatibleDC( hdc ))) return;

	oldbitmap = SelectObject(hMemDC,infoPtr->hIcon);
	BitBlt(hdc,bmp.bmWidth,bmp.bmHeight,bmp.bmWidth,bmp.bmHeight,hMemDC,0,0,SRCCOPY);
//	BitBlt(hdc,bmp.size.cx,bmp.size.cy,bmp.bmWidth,bmp.bmHeight,hMemDC,0,0,SRCCOPY);
	DeleteDC(hMemDC);
 
    }
}


static void STATIC_PaintEtchedfn( WND *wndPtr, HDC hdc )
{
    RECT rc;
    HBRUSH hbrush;
    HPEN hpen;

    if (TWEAK_WineLook == WIN31_LOOK)
	return;

    GetClientRect( wndPtr->hwndSelf, &rc );
    hbrush = MSG_SendMessage( wndPtr->parent, WM_CTLCOLORSTATIC,
                             (WPARAM)hdc, (LPARAM)wndPtr->hwndSelf );
    FillRect( hdc, &rc, hbrush );

    switch (wndPtr->dwStyle & SS_TYPEMASK)
    {
	case SS_ETCHEDHORZ:
	    hpen = SelectObject (hdc, GetSysColorPen (COLOR_3DSHADOW));
	    MoveToEx (hdc, rc.left, rc.bottom / 2 - 1, NULL);
	    LineTo (hdc, rc.right - 1, rc.bottom / 2 - 1);
	    SelectObject (hdc, GetSysColorPen (COLOR_3DHIGHLIGHT));
	    MoveToEx (hdc, rc.left, rc.bottom / 2, NULL);
	    LineTo (hdc, rc.right, rc.bottom / 2);
	    LineTo (hdc, rc.right, rc.bottom / 2 - 1);
	    SelectObject (hdc, hpen);
	    break;

	case SS_ETCHEDVERT:
	    hpen = SelectObject (hdc, GetSysColorPen (COLOR_3DSHADOW));
	    MoveToEx (hdc, rc.right / 2 - 1, rc.top, NULL);
	    LineTo (hdc, rc.right / 2 - 1, rc.bottom - 1);
	    SelectObject (hdc, GetSysColorPen (COLOR_3DHIGHLIGHT));
	    MoveToEx (hdc, rc.right / 2, rc.top, NULL);
	    LineTo (hdc, rc.right / 2, rc.bottom);
	    LineTo (hdc, rc.right / 2 -1 , rc.bottom);
	    SelectObject (hdc, hpen); 
	    break;

	case SS_ETCHEDFRAME:
	    DrawEdge (hdc, &rc, EDGE_ETCHED, BF_RECT);
	    break;
    }
}

