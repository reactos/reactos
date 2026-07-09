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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 *
 * Notes:
 *   - Controls with SS_SIMPLE but without SS_NOPREFIX:
 *     The text should not be changed. Windows doesn't clear the
 *     client rectangle, so the new text must be larger than the old one.
 *   - The SS_RIGHTJUST style is currently not implemented by Windows
 *     (or it does something different than documented).
 *
 * TODO:
 *   - Animated cursors
 */

#include <user32.h>
#include <wine-compat.h>

WINE_DEFAULT_DEBUG_CHANNEL(static);

static void STATIC_PaintOwnerDrawfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintTextfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintRectfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintIconfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintBitmapfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintEnhMetafn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintEtchedfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );

static COLORREF color_3dshadow, color_3ddkshadow, color_3dhighlight;

/* offsets for GetWindowLong for static private information */
#define HFONT_GWL_OFFSET    0
#define HICON_GWL_OFFSET    (sizeof(HFONT))
#ifdef __REACTOS__
#define UISTATE_GWL_OFFSET (HICON_GWL_OFFSET+sizeof(HICON)) // ReactOS: keep in sync with STATIC_UISTATE_GWL_OFFSET
#define STATIC_EXTRA_BYTES  (UISTATE_GWL_OFFSET + sizeof(LONG))
#endif

typedef void (*pfPaint)( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );

static const pfPaint staticPaintFunc[SS_TYPEMASK+1] =
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
    STATIC_PaintEnhMetafn,   /* SS_ENHMETAFILE */
    NULL,                    /* SS_ETCHEDHORZ */
    NULL,                    /* SS_ETCHEDVERT */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDFRAME */
};


#ifdef __REACTOS__
/*********************************************************************
 * static class descriptor
 */
static const WCHAR staticW[] = {'S','t','a','t','i','c',0};
const struct builtin_class_descr STATIC_builtin_class =
{
    staticW,             /* name */
    CS_DBLCLKS | CS_PARENTDC, /* style  */
    StaticWndProcA,      /* procA */
    StaticWndProcW,      /* procW */
    STATIC_EXTRA_BYTES,  /* extra */
    IDC_ARROW,           /* cursor */
    0                    /* brush */
};
#endif

static HFONT get_control_font( HWND hwnd )
{
    return (HFONT)NtUserGetPrivateData( hwnd, HFONT_GWL_OFFSET, sizeof(HFONT) );
}

static HFONT set_control_font( HWND hwnd, HFONT font )
{
    return (HFONT)NtUserSetPrivateData( hwnd, HFONT_GWL_OFFSET, sizeof(font), (LONG_PTR)font );
}

static HANDLE get_control_icon( HWND hwnd )
{
    return (HFONT)NtUserGetPrivateData( hwnd, HICON_GWL_OFFSET, sizeof(HICON) );
}

static HANDLE set_control_icon( HWND hwnd, HANDLE icon )
{
    return (HANDLE)NtUserSetPrivateData( hwnd, HICON_GWL_OFFSET, sizeof(icon), (LONG_PTR)icon );
}

/***********************************************************************
 *           STATIC_SetIcon
 *
 * Set the icon for an SS_ICON control.
 */
static HICON STATIC_SetIcon( HWND hwnd, HICON hicon, DWORD style )
{
    HICON prevIcon;
    SIZE size;

    if (hicon && !get_icon_size( hicon, &size ))
    {
        WARN("hicon != 0, but invalid\n");
        return 0;
    }
    prevIcon = set_control_icon( hwnd, hicon );
    if (hicon && !(style & SS_CENTERIMAGE) && !(style & SS_REALSIZECONTROL))
    {
        /* Windows currently doesn't implement SS_RIGHTJUST */
        /*
        if ((style & SS_RIGHTJUST) != 0)
        {
            RECT wr;
            GetWindowRect(hwnd, &wr);
            SetWindowPos( hwnd, 0, wr.right - info->nWidth, wr.bottom - info->nHeight,
                          info->nWidth, info->nHeight, SWP_NOACTIVATE | SWP_NOZORDER );
        }
        else */
        {
            NtUserSetWindowPos( hwnd, 0, 0, 0, size.cx, size.cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
        }
    }
    return prevIcon;
}

/***********************************************************************
 *           STATIC_SetBitmap
 *
 * Set the bitmap for an SS_BITMAP control.
 */
static HBITMAP STATIC_SetBitmap( HWND hwnd, HBITMAP hBitmap, DWORD style )
{
    HBITMAP hOldBitmap;

    if (hBitmap && GetObjectType(hBitmap) != OBJ_BITMAP) {
        WARN("hBitmap != 0, but it's not a bitmap\n");
        return 0;
    }
    hOldBitmap = set_control_icon( hwnd, hBitmap );
    if (hBitmap && !(style & SS_CENTERIMAGE) && !(style & SS_REALSIZECONTROL))
    {
        BITMAP bm;
        GetObjectW(hBitmap, sizeof(bm), &bm);
        /* Windows currently doesn't implement SS_RIGHTJUST */
        /*
        if ((style & SS_RIGHTJUST) != 0)
        {
            RECT wr;
            GetWindowRect(hwnd, &wr);
            SetWindowPos( hwnd, 0, wr.right - bm.bmWidth, wr.bottom - bm.bmHeight,
                          bm.bmWidth, bm.bmHeight, SWP_NOACTIVATE | SWP_NOZORDER );
        }
        else */
        {
            NtUserSetWindowPos( hwnd, 0, 0, 0, bm.bmWidth, bm.bmHeight,
                                SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
        }

    }
    return hOldBitmap;
}

/***********************************************************************
 *           STATIC_SetEnhMetaFile
 *
 * Set the enhanced metafile for an SS_ENHMETAFILE control.
 */
static HENHMETAFILE STATIC_SetEnhMetaFile( HWND hwnd, HENHMETAFILE hEnhMetaFile, DWORD style )
{
    if (hEnhMetaFile && GetObjectType(hEnhMetaFile) != OBJ_ENHMETAFILE) {
        WARN("hEnhMetaFile != 0, but it's not an enhanced metafile\n");
        return 0;
    }
    return set_control_icon( hwnd, hEnhMetaFile );
}

/***********************************************************************
 *           STATIC_GetImage
 *
 * Gets the bitmap for an SS_BITMAP control, the icon/cursor for an
 * SS_ICON control or the enhanced metafile for an SS_ENHMETAFILE control.
 */
static HANDLE STATIC_GetImage( HWND hwnd, WPARAM wParam, DWORD style )
{
    switch(style & SS_TYPEMASK)
    {
        case SS_ICON:
            if ((wParam != IMAGE_ICON) &&
                (wParam != IMAGE_CURSOR)) return NULL;
            break;
        case SS_BITMAP:
            if (wParam != IMAGE_BITMAP) return NULL;
            break;
        case SS_ENHMETAFILE:
            if (wParam != IMAGE_ENHMETAFILE) return NULL;
            break;
        default:
            return NULL;
    }
    return get_control_icon( hwnd );
}

/***********************************************************************
 *           STATIC_LoadIconA
 *
 * Load the icon for an SS_ICON control.
 */
static HICON STATIC_LoadIconA( HINSTANCE hInstance, LPCSTR name, DWORD style )
{
    HICON hicon = 0;

    if (hInstance && ((ULONG_PTR)hInstance >> 16))
    {
        if ((style & SS_REALSIZEIMAGE) != 0)
            hicon = LoadImageA(hInstance, name, IMAGE_ICON, 0, 0, LR_SHARED);
        else
        {
            hicon = LoadIconA( hInstance, name );
            if (!hicon) hicon = LoadCursorA( hInstance, name );
        }
    }
    if (!hicon) hicon = LoadIconA( 0, name );
    /* Windows doesn't try to load a standard cursor,
       probably because most IDs for standard cursors conflict
       with the IDs for standard icons anyway */
    return hicon;
}
/***********************************************************************
 *           STATIC_SendWmCtlColorStatic
 *
 * Sends WM_CTLCOLORSTATIC message and returns brush to be used for painting.
 */
static HBRUSH STATIC_SendWmCtlColorStatic(HWND hwnd, HDC hdc)
{
#ifdef __REACTOS__
    return GetControlBrush( hwnd, hdc, WM_CTLCOLORSTATIC);
#else
    HBRUSH hBrush;
    HWND parent = GetParent(hwnd);

    if (!parent) parent = hwnd;
    hBrush = (HBRUSH) SendMessageW( parent, WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)hwnd );
    if (!hBrush) /* did the app forget to call DefWindowProc ? */
        hBrush = (HBRUSH)DefWindowProcW( parent, WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)hwnd);
    return hBrush;
#endif
}

/***********************************************************************
 *           STATIC_LoadIconW
 *
 * Load the icon for an SS_ICON control.
 */
static HICON STATIC_LoadIconW( HINSTANCE hInstance, LPCWSTR name, DWORD style )
{
    HICON hicon = 0;

    if (hInstance && ((ULONG_PTR)hInstance >> 16))
    {
        if ((style & SS_REALSIZEIMAGE) != 0)
            hicon = LoadImageW(hInstance, name, IMAGE_ICON, 0, 0, LR_SHARED);
        else
        {
            hicon = LoadIconW( hInstance, name );
            if (!hicon) hicon = LoadCursorW( hInstance, name );
        }
    }
    if (!hicon) hicon = LoadIconW( 0, name );
    /* Windows doesn't try to load a standard cursor,
       probably because most IDs for standard cursors conflict
       with the IDs for standard icons anyway */
    return hicon;
}

/***********************************************************************
 *           STATIC_TryPaintFcn
 *
 * Try to immediately paint the control.
 */
static VOID STATIC_TryPaintFcn(HWND hwnd, LONG full_style)
{
    if (IsWindowVisible(hwnd))
    {
        RECT rc;
        HDC hdc;
        HRGN hrgn;
        HBRUSH hbrush;
        LONG style = full_style & SS_TYPEMASK;

        GetClientRect( hwnd, &rc );
        hdc = NtUserGetDC( hwnd );
        hrgn = set_control_clipping( hdc, &rc );
        hbrush = STATIC_SendWmCtlColorStatic( hwnd, hdc );
        if (staticPaintFunc[style])
            (staticPaintFunc[style])( hwnd, hdc, hbrush, full_style );
        SelectClipRgn( hdc, hrgn );
        if (hrgn) DeleteObject( hrgn );
        NtUserReleaseDC( hwnd, hdc );
    }
}

static VOID STATIC_InitColours(void)
{
    color_3ddkshadow  = GetSysColor(COLOR_3DDKSHADOW);
    color_3dshadow    = GetSysColor(COLOR_3DSHADOW);
    color_3dhighlight = GetSysColor(COLOR_3DHIGHLIGHT);
}

/***********************************************************************
 *           hasTextStyle
 *
 * Tests if the control displays text.
 */
static BOOL hasTextStyle( DWORD style )
{
    switch(style & SS_TYPEMASK)
    {
        case SS_SIMPLE:
        case SS_LEFT:
        case SS_LEFTNOWORDWRAP:
        case SS_CENTER:
        case SS_RIGHT:
        case SS_OWNERDRAW:
            return TRUE;
    }

    return FALSE;
}

/***********************************************************************
 *           StaticWndProc_common
 */
LRESULT WINAPI StaticWndProc_common( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL unicode ) // ReactOS
{
    LRESULT lResult = 0;
    LONG full_style = GetWindowLongW( hwnd, GWL_STYLE );
    LONG style = full_style & SS_TYPEMASK;

    if (!IsWindow( hwnd )) return 0;
    NtUserSetWindowFNID( hwnd, MAKE_FNID(NTUSER_WNDPROC_STATIC) );

    switch (uMsg)
    {
    case WM_CREATE:
        if (style < 0L || style > SS_TYPEMASK)
        {
            ERR("Unknown style 0x%02lx\n", style );
            return -1;
        }
        STATIC_update_uistate(hwnd, unicode); // ReactOS r30727
        STATIC_InitColours();
        break;

    case WM_NCDESTROY:
#ifdef __REACTOS__
        NtUserSetWindowFNID(hwnd, FNID_DESTROY);
#endif
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

    case WM_ERASEBKGND:
        /* do all painting in WM_PAINT like Windows does */
        return 1;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT rect;
            HDC hdc = wParam ? (HDC)wParam : NtUserBeginPaint( hwnd, &ps );
            HRGN hrgn;
            HBRUSH hbrush;

            GetClientRect( hwnd, &rect );
            hrgn = set_control_clipping( hdc, &rect );
            hbrush = STATIC_SendWmCtlColorStatic( hwnd, hdc );
            if (staticPaintFunc[style])
                (staticPaintFunc[style])( hwnd, hdc, hbrush, full_style );
            SelectClipRgn( hdc, hrgn );
            if (hrgn) DeleteObject( hrgn );
            if (!wParam) NtUserEndPaint( hwnd, &ps );
        }
        break;

    case WM_ENABLE:
        STATIC_TryPaintFcn( hwnd, full_style );
        if (full_style & SS_NOTIFY) {
            if (wParam) {
                SendMessageW( GetParent(hwnd), WM_COMMAND,
                              MAKEWPARAM( GetWindowLongPtrW(hwnd,GWLP_ID), STN_ENABLE ), (LPARAM)hwnd);
            }
            else {
                SendMessageW( GetParent(hwnd), WM_COMMAND,
                              MAKEWPARAM( GetWindowLongPtrW(hwnd,GWLP_ID), STN_DISABLE ), (LPARAM)hwnd);
            }
        }
        break;

    case WM_SYSCOLORCHANGE:
        STATIC_InitColours();
        STATIC_TryPaintFcn( hwnd, full_style );
        break;

    case WM_NCCREATE:
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;

            if (full_style & SS_SUNKEN || style == SS_ETCHEDHORZ || style == SS_ETCHEDVERT)
                SetWindowLongW( hwnd, GWL_EXSTYLE,
                                GetWindowLongW( hwnd, GWL_EXSTYLE ) | WS_EX_STATICEDGE );

            if (style == SS_ETCHEDHORZ || style == SS_ETCHEDVERT)
            {
                RECT rc;
                GetClientRect(hwnd, &rc);
                if (style == SS_ETCHEDHORZ)
                    rc.bottom = rc.top;
                else
                    rc.right = rc.left;
                AdjustWindowRectEx(&rc, full_style, FALSE, GetWindowLongW(hwnd, GWL_EXSTYLE));
                NtUserSetWindowPos( hwnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top,
                                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
            }

            switch (style) {
            case SS_ICON:
                {
                    const WCHAR *name = cs->lpszName;
                    HICON hIcon;

                    if (!unicode)
                    {
                        const char *nameA = (const char *)name;
                        if (nameA && nameA[0] == '\xff')
                            name = MAKEINTRESOURCEW(MAKEWORD(nameA[1], nameA[2]));
                    }
                    else if (name && name[0] == 0xffff)
                    {
                        name = MAKEINTRESOURCEW(name[1]);
                    }

                    if (unicode || IS_INTRESOURCE(name))
                       hIcon = STATIC_LoadIconW(cs->hInstance, name, full_style);
                    else
                       hIcon = STATIC_LoadIconA(cs->hInstance, (const char *)name, full_style);
                    STATIC_SetIcon(hwnd, hIcon, full_style);
                }
                break;
            case SS_BITMAP:
                if ((ULONG_PTR)cs->hInstance >> 16)
                {
                    const WCHAR *name = cs->lpszName;
                    HBITMAP hBitmap;

                    if (!unicode)
                    {
                        const char *nameA = (const char *)name;
                        if (nameA && nameA[0] == '\xff')
                            name = MAKEINTRESOURCEW(MAKEWORD(nameA[1], nameA[2]));
                    }
                    else if (name && name[0] == 0xffff)
                    {
                        name = MAKEINTRESOURCEW(name[1]);
                    }

                    if (unicode || IS_INTRESOURCE(name))
                        hBitmap = LoadBitmapW(cs->hInstance, name);
                    else
                        hBitmap = LoadBitmapA(cs->hInstance, (const char *)name);
                    STATIC_SetBitmap(hwnd, hBitmap, full_style);
                }
                break;
            }
            /* SS_ENHMETAFILE: Despite what MSDN says, Windows does not load
               the enhanced metafile that was specified as the window text. */
        }
        return unicode ? DefWindowProcW(hwnd, uMsg, wParam, lParam) :
                         DefWindowProcA(hwnd, uMsg, wParam, lParam);

    case WM_SETTEXT:
        if (hasTextStyle( full_style ))
        {
            if (unicode)
                lResult = DefWindowProcW( hwnd, uMsg, wParam, lParam );
            else
                lResult = DefWindowProcA( hwnd, uMsg, wParam, lParam );
            STATIC_TryPaintFcn( hwnd, full_style );
        }
        break;

    case WM_SETFONT:
        if (hasTextStyle( full_style ))
        {
            set_control_font( hwnd, (HFONT)wParam );
            if (LOWORD(lParam))
                NtUserRedrawWindow( hwnd, NULL, 0, RDW_INVALIDATE | RDW_ERASE |
                                    RDW_UPDATENOW | RDW_ALLCHILDREN );
        }
        break;

    case WM_GETFONT:
        return (LRESULT)get_control_font( hwnd );

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
                          MAKEWPARAM( GetWindowLongPtrW(hwnd,GWLP_ID), STN_CLICKED ), (LPARAM)hwnd);
        return 0;

    case WM_LBUTTONDBLCLK:
    case WM_NCLBUTTONDBLCLK:
        if (full_style & SS_NOTIFY)
            SendMessageW( GetParent(hwnd), WM_COMMAND,
                          MAKEWPARAM( GetWindowLongPtrW(hwnd,GWLP_ID), STN_DBLCLK ), (LPARAM)hwnd);
        return 0;

    case STM_GETIMAGE:
        return (LRESULT)STATIC_GetImage( hwnd, wParam, full_style );

    case STM_GETICON:
        return (LRESULT)STATIC_GetImage( hwnd, IMAGE_ICON, full_style );

    case STM_SETIMAGE:
        switch(wParam) {
	case IMAGE_BITMAP:
	    if (style != SS_BITMAP) return 0; // ReactOS r43158
            if (style != SS_BITMAP) return 0;
	    lResult = (LRESULT)STATIC_SetBitmap( hwnd, (HBITMAP)lParam, full_style );
	    break;
	case IMAGE_ENHMETAFILE:
	    if (style != SS_ENHMETAFILE) return 0; // ReactOS r43158
            if (style != SS_ENHMETAFILE) return 0;
	    lResult = (LRESULT)STATIC_SetEnhMetaFile( hwnd, (HENHMETAFILE)lParam, full_style );
	    break;
	case IMAGE_ICON:
	case IMAGE_CURSOR:
	    if (style != SS_ICON) return 0; // ReactOS r43158
            if (style != SS_ICON) return 0;
	    lResult = (LRESULT)STATIC_SetIcon( hwnd, (HICON)lParam, full_style );
	    break;
	default:
	    FIXME("STM_SETIMAGE: Unhandled type %Ix\n", wParam);
	    break;
	}
        STATIC_TryPaintFcn( hwnd, full_style );
	break;

    case STM_SETICON:
        if (style != SS_ICON) return 0;
        lResult = (LRESULT)STATIC_SetIcon( hwnd, (HICON)wParam, full_style );
        STATIC_TryPaintFcn( hwnd, full_style );
        break;

#ifdef __REACTOS__
    case WM_UPDATEUISTATE:
        if (unicode)
            DefWindowProcW(hwnd, uMsg, wParam, lParam);
        else
            DefWindowProcA(hwnd, uMsg, wParam, lParam);

        if (STATIC_update_uistate(hwnd, unicode) && hasTextStyle( full_style ))
        {
            RedrawWindow( hwnd, NULL, 0, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
        }
        break;
#endif

    default:
        return unicode ? DefWindowProcW(hwnd, uMsg, wParam, lParam) :
                         DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
    return lResult;
}

/***********************************************************************
 *           StaticWndProcA
 */
LRESULT WINAPI StaticWndProcA( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if (!IsWindow( hWnd )) return 0;
    return StaticWndProc_common(hWnd, uMsg, wParam, lParam, FALSE);
}

/***********************************************************************
 *           StaticWndProcW
 */
LRESULT WINAPI StaticWndProcW( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    if (!IsWindow( hWnd )) return 0;
    return StaticWndProc_common(hWnd, uMsg, wParam, lParam, TRUE);
}

static void STATIC_PaintOwnerDrawfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style )
{
  DRAWITEMSTRUCT dis;
  HFONT font, oldFont = NULL;
  UINT id = (UINT)GetWindowLongPtrW( hwnd, GWLP_ID );

  dis.CtlType    = ODT_STATIC;
  dis.CtlID      = id;
  dis.itemID     = 0;
  dis.itemAction = ODA_DRAWENTIRE;
  dis.itemState  = IsWindowEnabled(hwnd) ? 0 : ODS_DISABLED;
  dis.hwndItem   = hwnd;
  dis.hDC        = hdc;
  dis.itemData   = 0;
  GetClientRect( hwnd, &dis.rcItem );

  font = get_control_font( hwnd );
  if (font) oldFont = SelectObject( hdc, font );
  SendMessageW( GetParent(hwnd), WM_DRAWITEM, id, (LPARAM)&dis );
  if (font) SelectObject( hdc, oldFont );
}

static BOOL CALLBACK STATIC_DrawTextCallback(HDC hdc, LPARAM lp, WPARAM wp, int cx, int cy)
{
    RECT rc;

    SetRect(&rc, 0, 0, cx, cy);
    DrawTextW(hdc, (LPCWSTR)lp, -1, &rc, (UINT)wp);
    return TRUE;
}

static void STATIC_PaintTextfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style )
{
    RECT rc;
    HFONT hFont, hOldFont = NULL;
    UINT format;
    INT len, buf_size;
    WCHAR *text;

    GetClientRect( hwnd, &rc);

    switch (style & SS_TYPEMASK)
    {
    case SS_LEFT:
	format = DT_LEFT | DT_EXPANDTABS | DT_WORDBREAK;
	break;

    case SS_CENTER:
	format = DT_CENTER | DT_EXPANDTABS | DT_WORDBREAK;
	break;

    case SS_RIGHT:
	format = DT_RIGHT | DT_EXPANDTABS | DT_WORDBREAK;
	break;

    case SS_SIMPLE:
        format = DT_LEFT | DT_SINGLELINE;
	break;

    case SS_LEFTNOWORDWRAP:
        format = DT_LEFT | DT_EXPANDTABS;
	break;

    default:
        return;
    }

    if (GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_RIGHT)
        format = DT_RIGHT | (format & ~(DT_LEFT | DT_CENTER));

    if (style & SS_NOPREFIX)
        format |= DT_NOPREFIX;
#ifdef __REACTOS__
    else if (GetWindowLongW(hwnd, UISTATE_GWL_OFFSET) & UISF_HIDEACCEL) // ReactOS r30727
        format |= DT_HIDEPREFIX;
#endif

    if ((style & SS_TYPEMASK) != SS_SIMPLE)
    {
        if (style & SS_CENTERIMAGE)
            format |= DT_SINGLELINE | DT_VCENTER;
        if (style & SS_EDITCONTROL)
            format |= DT_EDITCONTROL;
        if (style & SS_ENDELLIPSIS)
            format |= DT_SINGLELINE | DT_END_ELLIPSIS;
        if (style & SS_PATHELLIPSIS)
            format |= DT_SINGLELINE | DT_PATH_ELLIPSIS;
        if (style & SS_WORDELLIPSIS)
            format |= DT_SINGLELINE | DT_WORD_ELLIPSIS;
    }

    if ((hFont = get_control_font( hwnd )))
        hOldFont = SelectObject( hdc, hFont );

    if ((style & SS_TYPEMASK) != SS_SIMPLE)
    {
        FillRect( hdc, &rc, hbrush );
        if (!IsWindowEnabled(hwnd)) SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
    }

    buf_size = 256;
    if (!(text = HeapAlloc( GetProcessHeap(), 0, buf_size * sizeof(WCHAR) )))
        goto no_TextOut;

    while ((len = NtUserInternalGetWindowText( hwnd, text, buf_size )) == buf_size - 1)
    {
        buf_size *= 2;
        if (!(text = HeapReAlloc( GetProcessHeap(), 0, text, buf_size * sizeof(WCHAR) )))
            goto no_TextOut;
    }

    if (!len) goto no_TextOut;

    if (((style & SS_TYPEMASK) == SS_SIMPLE) && (style & SS_NOPREFIX))
    {
        /* Windows uses the faster ExtTextOut() to draw the text and
           to paint the whole client rectangle with the text background
           color. Reference: "Static Controls" by Kyle Marsh, 1992 */
        ExtTextOutW( hdc, rc.left, rc.top, ETO_CLIPPED | ETO_OPAQUE,
                     &rc, text, len, NULL );
    }
#ifdef __REACTOS__
    else
    {
        UINT flags = DST_COMPLEX;
        if (style & WS_DISABLED)
            flags |= DSS_DISABLED;
        DrawStateW(hdc, hbrush, STATIC_DrawTextCallback,
                   (LPARAM)text, (WPARAM)format,
                   rc.left, rc.top,
                   rc.right - rc.left, rc.bottom - rc.top,
                   flags);
    }
#else
    else
    {
        DrawTextW( hdc, text, -1, &rc, format );
    }
#endif

no_TextOut:
    HeapFree( GetProcessHeap(), 0, text );

    if (hFont)
        SelectObject( hdc, hOldFont );
}

static void STATIC_PaintRectfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style )
{
    RECT rc;

    GetClientRect( hwnd, &rc);

    /* FIXME: send WM_CTLCOLORSTATIC */
    switch (style & SS_TYPEMASK)
    {
    case SS_BLACKRECT:
	hbrush = CreateSolidBrush(color_3ddkshadow);
        FillRect( hdc, &rc, hbrush );
	break;
    case SS_GRAYRECT:
	hbrush = CreateSolidBrush(color_3dshadow);
        FillRect( hdc, &rc, hbrush );
	break;
    case SS_WHITERECT:
	hbrush = CreateSolidBrush(color_3dhighlight);
        FillRect( hdc, &rc, hbrush );
	break;
    case SS_BLACKFRAME:
	hbrush = CreateSolidBrush(color_3ddkshadow);
        FrameRect( hdc, &rc, hbrush );
	break;
    case SS_GRAYFRAME:
	hbrush = CreateSolidBrush(color_3dshadow);
        FrameRect( hdc, &rc, hbrush );
	break;
    case SS_WHITEFRAME:
	hbrush = CreateSolidBrush(color_3dhighlight);
        FrameRect( hdc, &rc, hbrush );
	break;
    default:
        return;
    }
    DeleteObject( hbrush );
}


static void STATIC_PaintIconfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style )
{
    RECT rc, iconRect;
    HICON hIcon;
    SIZE size;

    GetClientRect( hwnd, &rc );
    hIcon = get_control_icon( hwnd );
    if (!hIcon || !get_icon_size( hIcon, &size ))
    {
        FillRect(hdc, &rc, hbrush);
    }
    else
    {
        if (style & SS_CENTERIMAGE)
        {
            iconRect.left = (rc.right - rc.left) / 2 - size.cx / 2;
            iconRect.top = (rc.bottom - rc.top) / 2 - size.cy / 2;
            iconRect.right = iconRect.left + size.cx;
            iconRect.bottom = iconRect.top + size.cy;
        }
        else
            iconRect = rc;
        FillRect( hdc, &rc, hbrush );
        NtUserDrawIconEx( hdc, iconRect.left, iconRect.top, hIcon, iconRect.right - iconRect.left,
                          iconRect.bottom - iconRect.top, 0, NULL, DI_NORMAL );
    }
}

static void STATIC_PaintBitmapfn(HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style )
{
    HDC hMemDC;
    HBITMAP hBitmap, oldbitmap;

    if ((hBitmap = get_control_icon( hwnd ))
         && (GetObjectType(hBitmap) == OBJ_BITMAP)
         && (hMemDC = CreateCompatibleDC( hdc )))
    {
        BITMAP bm;
        RECT rcClient;

        GetObjectW(hBitmap, sizeof(bm), &bm);
        oldbitmap = SelectObject(hMemDC, hBitmap);

        GetClientRect(hwnd, &rcClient);
        if (style & SS_CENTERIMAGE)
        {
            hbrush = CreateSolidBrush(GetPixel(hMemDC, 0, 0));

            FillRect(hdc, &rcClient, hbrush);

            rcClient.left = (rcClient.right - rcClient.left)/2 - bm.bmWidth/2;
            rcClient.top = (rcClient.bottom - rcClient.top)/2 - bm.bmHeight/2;
            rcClient.right = rcClient.left + bm.bmWidth;
            rcClient.bottom = rcClient.top + bm.bmHeight;

            DeleteObject(hbrush);
        }
        StretchBlt(hdc, rcClient.left, rcClient.top, rcClient.right - rcClient.left,
                   rcClient.bottom - rcClient.top, hMemDC,
                   0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
        SelectObject(hMemDC, oldbitmap);
        DeleteDC(hMemDC);
    }
}


static void STATIC_PaintEnhMetafn(HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style )
{
    HENHMETAFILE hEnhMetaFile;
    RECT rc;

    GetClientRect(hwnd, &rc);
    FillRect(hdc, &rc, hbrush);
    if ((hEnhMetaFile = get_control_icon( hwnd )))
    {
        /* The control's current font is not selected into the
           device context! */
        if (GetObjectType(hEnhMetaFile) == OBJ_ENHMETAFILE)
            PlayEnhMetaFile(hdc, hEnhMetaFile, &rc);
    }
}


static void STATIC_PaintEtchedfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style )
{
    RECT rc;

    GetClientRect( hwnd, &rc );
    DrawEdge (hdc, &rc, EDGE_ETCHED, BF_RECT);
}
