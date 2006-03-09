/*
 * Static control
 *
 * Copyright  David W. Metcalfe, 1993
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <user32.h>

#ifndef __REACTOS__
WINE_DEFAULT_DEBUG_CHANNEL(static);
#endif

static void STATIC_PaintOwnerDrawfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintTextfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintRectfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintIconfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintBitmapfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintEtchedfn( HWND hwnd, HDC hdc, DWORD style );
static LRESULT WINAPI StaticWndProcA( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
static LRESULT WINAPI StaticWndProcW( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

static COLORREF color_3dshadow, color_3ddkshadow, color_3dhighlight;

/* offsets for GetWindowLong for static private information */
#define HFONT_GWL_OFFSET    0
#define HICON_GWL_OFFSET    (sizeof(HFONT))
#define STATIC_EXTRA_BYTES  (HICON_GWL_OFFSET + sizeof(HICON))

typedef void (*pfPaint)( HWND hwnd, HDC hdc, DWORD style );

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
    NULL,                    /* SS_USERITEM */
    STATIC_PaintTextfn,      /* SS_SIMPLE */
    STATIC_PaintTextfn,      /* SS_LEFTNOWORDWRAP */
    STATIC_PaintOwnerDrawfn, /* SS_OWNERDRAW */
    STATIC_PaintBitmapfn,    /* SS_BITMAP */
    NULL,                    /* SS_ENHMETAFILE */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDHORIZ */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDVERT */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDFRAME */
};


/*********************************************************************
 * static class descriptor
 */
const struct builtin_class_descr STATIC_builtin_class =
{
#ifdef __REACTOS__
    L"Static",            /* name */
    CS_DBLCLKS | CS_PARENTDC, /* style  */
    (WNDPROC) StaticWndProcW,                  /* procW */
    (WNDPROC) StaticWndProcA,                  /* procA */
    STATIC_EXTRA_BYTES,                        /* extra */
    (LPCWSTR) IDC_ARROW,                        /* cursor */ /* FIXME Wine uses IDC_ARROWA */
    0                                          /* brush */
#else
    "Static",            /* name */
    CS_DBLCLKS | CS_PARENTDC, /* style  */
    StaticWndProcA,      /* procA */
    StaticWndProcW,      /* procW */
    STATIC_EXTRA_BYTES,  /* extra */
    IDC_ARROW,           /* cursor */
    0                    /* brush */
#endif
};


/***********************************************************************
 *           STATIC_SetIcon
 *
 * Set the icon for an SS_ICON control.
 */
static HICON STATIC_SetIcon( HWND hwnd, HICON hicon, DWORD style )
{
#ifdef __REACTOS__
    HICON prevIcon;

    if ((style & SS_TYPEMASK) != SS_ICON) return 0;
    prevIcon = (HICON)SetWindowLongA( hwnd, HICON_GWL_OFFSET, (LONG)hicon );
    if (hicon && !(style & SS_CENTERIMAGE))
    {
        ICONINFO info;
        BITMAP bm;

        if (!GetIconInfo(hicon, &info))
        {
            return 0;
        }
        if (!GetObjectW(info.hbmColor, sizeof(BITMAP), &bm))
        {
            return 0;
        }
        SetWindowPos( hwnd, 0, 0, 0, bm.bmWidth, bm.bmHeight,
                      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
    }
    return prevIcon;
#else
    HICON prevIcon;
    CURSORICONINFO *info = hicon?(CURSORICONINFO *) GlobalLock16(HICON_16(hicon)):NULL;

    if ((style & SS_TYPEMASK) != SS_ICON) return 0;
    if (hicon && !info) {
	ERR("huh? hicon!=0, but info=0???\n");
    	return 0;
    }
    prevIcon = (HICON)SetWindowLongA( hwnd, HICON_GWL_OFFSET, (LONG)hicon );
    if (hicon && !(style & SS_CENTERIMAGE))
    {
        SetWindowPos( hwnd, 0, 0, 0, info->nWidth, info->nHeight,
                        SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
        GlobalUnlock16(HICON_16(hicon));
    }
    return prevIcon;
#endif
}

/***********************************************************************
 *           STATIC_SetBitmap
 *
 * Set the bitmap for an SS_BITMAP control.
 */
static HBITMAP STATIC_SetBitmap( HWND hwnd, HBITMAP hBitmap, DWORD style )
{
    HBITMAP hOldBitmap;

    if ((style & SS_TYPEMASK) != SS_BITMAP) return 0;
    if (hBitmap && GetObjectType(hBitmap) != OBJ_BITMAP) {
#ifdef __REACTOS__
	OutputDebugStringA("huh? hBitmap!=0, but not bitmap\n");
#else
	ERR("huh? hBitmap!=0, but not bitmap\n");
#endif
    	return 0;
    }
    hOldBitmap = (HBITMAP)SetWindowLongA( hwnd, HICON_GWL_OFFSET, (LONG)hBitmap );
    if (hBitmap && !(style & SS_CENTERIMAGE))
    {
        BITMAP bm;
        GetObjectW(hBitmap, sizeof(bm), &bm);
        SetWindowPos( hwnd, 0, 0, 0, bm.bmWidth, bm.bmHeight,
		      SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
    }
    return hOldBitmap;
}

/***********************************************************************
 *           STATIC_LoadIconA
 *
 * Load the icon for an SS_ICON control.
 */
static HICON STATIC_LoadIconA( HWND hwnd, LPCSTR name )
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE );
    HICON hicon = LoadIconA( hInstance, name );
    if (!hicon) hicon = LoadIconA( 0, name );
    return hicon;
}

/***********************************************************************
 *           STATIC_LoadIconW
 *
 * Load the icon for an SS_ICON control.
 */
static HICON STATIC_LoadIconW( HWND hwnd, LPCWSTR name )
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE );
    HICON hicon = LoadIconW( hInstance, name );
    if (!hicon) hicon = LoadIconW( 0, name );
    return hicon;
}

/***********************************************************************
 *           STATIC_LoadBitmapA
 *
 * Load the bitmap for an SS_BITMAP control.
 */
static HBITMAP STATIC_LoadBitmapA( HWND hwnd, LPCSTR name )
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE );
    HBITMAP hbitmap = LoadBitmapA( hInstance, name );
    if (!hbitmap)  /* Try OEM icon (FIXME: is this right?) */
        hbitmap = LoadBitmapA( 0, name );
    return hbitmap;
}

/***********************************************************************
 *           STATIC_LoadBitmapW
 *
 * Load the bitmap for an SS_BITMAP control.
 */
static HBITMAP STATIC_LoadBitmapW( HWND hwnd, LPCWSTR name )
{
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongA( hwnd, GWL_HINSTANCE );
    HBITMAP hbitmap = LoadBitmapW( hInstance, name );
    if (!hbitmap)  /* Try OEM icon (FIXME: is this right?) */
        hbitmap = LoadBitmapW( 0, name );
    return hbitmap;
}

/***********************************************************************
 *           STATIC_TryPaintFcn
 *
 * Try to immediately paint the control.
 */
static VOID STATIC_TryPaintFcn(HWND hwnd, LONG full_style)
{
    LONG style = full_style & SS_TYPEMASK;
    RECT rc;

    GetClientRect( hwnd, &rc );
    if (!IsRectEmpty(&rc) && IsWindowVisible(hwnd) && staticPaintFunc[style])
    {
	HDC hdc;
	hdc = GetDC( hwnd );
	(staticPaintFunc[style])( hwnd, hdc, full_style );
	ReleaseDC( hwnd, hdc );
    }
}

static VOID STATIC_InitColours()
{
    color_3ddkshadow  = GetSysColor(COLOR_3DDKSHADOW);
    color_3dshadow    = GetSysColor(COLOR_3DSHADOW);
    color_3dhighlight = GetSysColor(COLOR_3DHIGHLIGHT);
}

/***********************************************************************
 *           StaticWndProc_common
 */
static LRESULT StaticWndProc_common( HWND hwnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam, BOOL unicode )
{
    LRESULT lResult = 0;
    LONG full_style = GetWindowLongA( hwnd, GWL_STYLE );
    LONG style = full_style & SS_TYPEMASK;

    switch (uMsg)
    {
    case WM_CREATE:
        if (style < 0L || style > SS_TYPEMASK)
        {
#ifdef __REACTOS__
            OutputDebugStringA("Unknown style\n");
#else
            ERR("Unknown style 0x%02lx\n", style );
#endif
            return -1;
        }
        STATIC_InitColours();
        break;

    case WM_NCDESTROY:
        if (style == SS_ICON) {
/*
 * FIXME
 *           DestroyIcon32( STATIC_SetIcon( wndPtr, 0 ) );
 *
 * We don't want to do this yet because DestroyIcon32 is broken. If the icon
 * had already been loaded by the application the last thing we want to do is
 * GlobalFree16 the handle.
 */
            break;
        }
        else return unicode ? DefWindowProcW(hwnd, uMsg, wParam, lParam) :
                              DefWindowProcA(hwnd, uMsg, wParam, lParam);

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = wParam ? (HDC)wParam : BeginPaint(hwnd, &ps);
            if (staticPaintFunc[style])
                (staticPaintFunc[style])( hwnd, hdc, full_style );
            if (!wParam) EndPaint(hwnd, &ps);
        }
        break;

    case WM_ENABLE:
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_SYSCOLORCHANGE:
        STATIC_InitColours();
        InvalidateRect(hwnd, NULL, TRUE);
        break;

    case WM_NCCREATE:
	if (full_style & SS_SUNKEN)
            SetWindowLongA( hwnd, GWL_EXSTYLE,
                            GetWindowLongA( hwnd, GWL_EXSTYLE ) | WS_EX_STATICEDGE );

	if(unicode)
	    lParam = (LPARAM)(((LPCREATESTRUCTW)lParam)->lpszName);
	else
	    lParam = (LPARAM)(((LPCREATESTRUCTA)lParam)->lpszName);
	/* fall through */
    case WM_SETTEXT:
	switch (style) {
	case SS_ICON:
	{
	    HICON hIcon;
	    if(unicode)
		hIcon = STATIC_LoadIconW(hwnd, (LPCWSTR)lParam);
	    else
		hIcon = STATIC_LoadIconA(hwnd, (LPCSTR)lParam);
            /* FIXME : should we also return the previous hIcon here ??? */
            STATIC_SetIcon(hwnd, hIcon, full_style);
	    break;
	}
        case SS_BITMAP:
	{
	    HBITMAP hBitmap;
	    if(unicode)
		hBitmap = STATIC_LoadBitmapW(hwnd, (LPCWSTR)lParam);
	    else
		hBitmap = STATIC_LoadBitmapA(hwnd, (LPCSTR)lParam);
            STATIC_SetBitmap(hwnd, hBitmap, full_style);
	    break;
	}
	case SS_LEFT:
	case SS_CENTER:
	case SS_RIGHT:
	case SS_SIMPLE:
	case SS_LEFTNOWORDWRAP:
        {
	    if (HIWORD(lParam))
	    {
		if(unicode)
		    lResult = DefWindowProcW( hwnd, WM_SETTEXT, wParam, lParam );
		else
		    lResult = DefWindowProcA( hwnd, WM_SETTEXT, wParam, lParam );
	    }
	    if (uMsg == WM_SETTEXT)
		STATIC_TryPaintFcn( hwnd, full_style );
	    break;
	}
	default:
	    if (HIWORD(lParam))
	    {
		if(unicode)
		    lResult = DefWindowProcW( hwnd, WM_SETTEXT, wParam, lParam );
		else
		    lResult = DefWindowProcA( hwnd, WM_SETTEXT, wParam, lParam );
	    }
	    if(uMsg == WM_SETTEXT)
		InvalidateRect(hwnd, NULL, TRUE);
	}
        return 1; /* success. FIXME: check text length */

    case WM_SETFONT:
        if ((style == SS_ICON) || (style == SS_BITMAP)) return 0;
        SetWindowLongA( hwnd, HFONT_GWL_OFFSET, wParam );
        if (LOWORD(lParam))
            InvalidateRect( hwnd, NULL, TRUE );
        break;

    case WM_GETFONT:
        return GetWindowLongA( hwnd, HFONT_GWL_OFFSET );

    case WM_NCHITTEST:
        if (full_style & SS_NOTIFY)
           return HTCLIENT;
        else
           return HTTRANSPARENT;

    case WM_GETDLGCODE:
        return DLGC_STATIC;

    case WM_LBUTTONDOWN:
    case WM_NCLBUTTONDOWN:
        if (full_style & SS_NOTIFY)
            SendMessageW( GetParent(hwnd), WM_COMMAND,
                          MAKEWPARAM( GetWindowLongW(hwnd,GWL_ID), STN_CLICKED ), (LPARAM)hwnd);
        return 0;

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
        if (full_style & SS_NOTIFY)
            SendMessageW( GetParent(hwnd), WM_COMMAND,
                          MAKEWPARAM( GetWindowLongW(hwnd,GWL_ID), STN_DBLCLK ), (LPARAM)hwnd);
        return 0;

    case STM_GETIMAGE:
#ifndef __REACTOS__
    case STM_GETICON16:
#endif
    case STM_GETICON:
        return GetWindowLongA( hwnd, HICON_GWL_OFFSET );

    case STM_SETIMAGE:
        switch(wParam) {
	case IMAGE_BITMAP:
	    lResult = (LRESULT)STATIC_SetBitmap( hwnd, (HBITMAP)lParam, full_style );
	    break;
	case IMAGE_ICON:
	    lResult = (LRESULT)STATIC_SetIcon( hwnd, (HICON)lParam, full_style );
	    break;
	default:
#ifndef __REACTOS__
	    FIXME("STM_SETIMAGE: Unhandled type %x\n", wParam);
#endif
	    break;
	}
        InvalidateRect( hwnd, NULL, TRUE );
	break;

#ifndef __REACTOS__
    case STM_SETICON16:
#endif
    case STM_SETICON:
        lResult = (LRESULT)STATIC_SetIcon( hwnd, (HICON)wParam, full_style );
        InvalidateRect( hwnd, NULL, TRUE );
        break;

    default:
        return unicode ? DefWindowProcW(hwnd, uMsg, wParam, lParam) :
                         DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    return lResult;
}

/***********************************************************************
 *           StaticWndProcA
 */
static LRESULT WINAPI StaticWndProcA( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if (!IsWindow( hWnd )) return 0;
    return StaticWndProc_common(hWnd, uMsg, wParam, lParam, FALSE);
}

/***********************************************************************
 *           StaticWndProcW
 */
static LRESULT WINAPI StaticWndProcW( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if (!IsWindow( hWnd )) return 0;
    return StaticWndProc_common(hWnd, uMsg, wParam, lParam, TRUE);
}

static void STATIC_PaintOwnerDrawfn( HWND hwnd, HDC hdc, DWORD style )
{
  DRAWITEMSTRUCT dis;
  LONG id = GetWindowLongA( hwnd, GWL_ID );

  dis.CtlType    = ODT_STATIC;
  dis.CtlID      = id;
  dis.itemID     = 0;
  dis.itemAction = ODA_DRAWENTIRE;
  dis.itemState  = 0;
  dis.hwndItem   = hwnd;
  dis.hDC        = hdc;
  dis.itemData   = 0;
  GetClientRect( hwnd, &dis.rcItem );

  SendMessageW( GetParent(hwnd), WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)hwnd );
  SendMessageW( GetParent(hwnd), WM_DRAWITEM, id, (LPARAM)&dis );
}

static void STATIC_PaintTextfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;
    HBRUSH hBrush;
    HFONT hFont;
    WORD wFormat;
    INT len;
    WCHAR *text;

    GetClientRect( hwnd, &rc);

    switch (style & SS_TYPEMASK)
    {
    case SS_LEFT:
	wFormat = DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK;
	break;

    case SS_CENTER:
	wFormat = DT_CENTER | DT_EXPANDTABS | DT_WORDBREAK;
	break;

    case SS_RIGHT:
	wFormat = DT_RIGHT | DT_EXPANDTABS | DT_WORDBREAK;
	break;

    case SS_SIMPLE:
	wFormat = DT_LEFT | DT_SINGLELINE | DT_VCENTER;
	break;

    case SS_LEFTNOWORDWRAP:
	wFormat = DT_LEFT | DT_EXPANDTABS | DT_VCENTER;
	break;

    default:
        return;
    }

    if (style & SS_NOPREFIX)
	wFormat |= DT_NOPREFIX;
    if (style & SS_CENTERIMAGE)
	wFormat |= DT_VCENTER;

    if ((hFont = (HFONT)GetWindowLongA( hwnd, HFONT_GWL_OFFSET ))) SelectObject( hdc, hFont );

    if ((style & SS_NOPREFIX) || ((style & SS_TYPEMASK) != SS_SIMPLE))
    {
        hBrush = (HBRUSH)SendMessageW( GetParent(hwnd), WM_CTLCOLORSTATIC,
				       (WPARAM)hdc, (LPARAM)hwnd );
        if (!hBrush) /* did the app forget to call defwindowproc ? */
            hBrush = (HBRUSH)DefWindowProcW(GetParent(hwnd), WM_CTLCOLORSTATIC,
					    (WPARAM)hdc, (LPARAM)hwnd);
        FillRect( hdc, &rc, hBrush );
    }
    if (!IsWindowEnabled(hwnd)) SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));

    if (!(len = SendMessageW( hwnd, WM_GETTEXTLENGTH, 0, 0 ))) return;
    if (!(text = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return;
    SendMessageW( hwnd, WM_GETTEXT, len + 1, (LPARAM)text );
    DrawTextW( hdc, text, -1, &rc, wFormat );
    HeapFree( GetProcessHeap(), 0, text );
}

static void STATIC_PaintRectfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;
    HBRUSH hBrush;

    GetClientRect( hwnd, &rc);

    switch (style & SS_TYPEMASK)
    {
    case SS_BLACKRECT:
	hBrush = CreateSolidBrush(color_3ddkshadow);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_GRAYRECT:
	hBrush = CreateSolidBrush(color_3dshadow);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_WHITERECT:
	hBrush = CreateSolidBrush(color_3dhighlight);
        FillRect( hdc, &rc, hBrush );
	break;
    case SS_BLACKFRAME:
	hBrush = CreateSolidBrush(color_3ddkshadow);
        FrameRect( hdc, &rc, hBrush );
	break;
    case SS_GRAYFRAME:
	hBrush = CreateSolidBrush(color_3dshadow);
        FrameRect( hdc, &rc, hBrush );
	break;
    case SS_WHITEFRAME:
	hBrush = CreateSolidBrush(color_3dhighlight);
        FrameRect( hdc, &rc, hBrush );
	break;
    default:
        return;
    }
    DeleteObject( hBrush );
}


static void STATIC_PaintIconfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;
    HBRUSH hbrush;
    HICON hIcon;
    INT x, y;

    GetClientRect( hwnd, &rc );
    hbrush = (HBRUSH)SendMessageW( GetParent(hwnd), WM_CTLCOLORSTATIC,
				   (WPARAM)hdc, (LPARAM)hwnd );
    FillRect( hdc, &rc, hbrush );
    hIcon = (HICON)GetWindowLongPtrW( hwnd, HICON_GWL_OFFSET );
    if (style & SS_CENTERIMAGE)
    {
#ifndef __REACTOS__
        CURSORICONINFO *info = hIcon ? (CURSORICONINFO *)GlobalLock16(HICON_16(hIcon)) : NULL;
        x = (rc.right - rc.left)/2 - (info ? info->nWidth/2 : 0);
        y = (rc.bottom - rc.top)/2 - (info ? info->nHeight/2 : 0);
#else
        ICONINFO info;
        BITMAP bm;

        if (!GetIconInfo(hIcon, &info))
            return;
        if (!GetObjectW(info.hbmColor, sizeof(BITMAP), &bm))
            return;
        x = (rc.right - rc.left)/2 - bm.bmWidth/2;
        y = (rc.bottom - rc.top)/2 - bm.bmHeight/2;
#endif
    }
    else
    {
        x = rc.left;
        y = rc.top;
    }
    if (hIcon)
        DrawIcon( hdc, x, y, hIcon );
}

static void STATIC_PaintBitmapfn(HWND hwnd, HDC hdc, DWORD style )
{
    HDC hMemDC;
    HBITMAP hBitmap, oldbitmap;

    /* message is still sent, even if the returned brush is not used */
    SendMessageW( GetParent(hwnd), WM_CTLCOLORSTATIC,
				   (WPARAM)hdc, (LPARAM)hwnd );

    if ((hBitmap = (HBITMAP)GetWindowLongA( hwnd, HICON_GWL_OFFSET )))
    {
        BITMAP bm;
        INT x, y;

        if(GetObjectType(hBitmap) != OBJ_BITMAP) return;
        if (!(hMemDC = CreateCompatibleDC( hdc ))) return;
	GetObjectW(hBitmap, sizeof(bm), &bm);
	oldbitmap = SelectObject(hMemDC, hBitmap);
        if (style & SS_CENTERIMAGE)
        {
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            x = (rcClient.right - rcClient.left)/2 - bm.bmWidth/2;
            y = (rcClient.bottom - rcClient.top)/2 - bm.bmHeight/2;
        }
        else
        {
            x = 0;
            y = 0;
        }
        BitBlt(hdc, x, y, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0,
	       SRCCOPY);
	SelectObject(hMemDC, oldbitmap);
	DeleteDC(hMemDC);
    }
}


static void STATIC_PaintEtchedfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;

    GetClientRect( hwnd, &rc );
    switch (style & SS_TYPEMASK)
    {
	case SS_ETCHEDHORZ:
	    DrawEdge(hdc,&rc,EDGE_ETCHED,BF_TOP|BF_BOTTOM);
	    break;
	case SS_ETCHEDVERT:
	    DrawEdge(hdc,&rc,EDGE_ETCHED,BF_LEFT|BF_RIGHT);
	    break;
	case SS_ETCHEDFRAME:
	    DrawEdge (hdc, &rc, EDGE_ETCHED, BF_RECT);
	    break;
    }
}
