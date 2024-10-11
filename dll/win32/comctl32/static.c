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

#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"
#include "uxtheme.h"

#include "wine/debug.h"

#include "comctl32.h"

WINE_DEFAULT_DEBUG_CHANNEL(static);

static void STATIC_PaintOwnerDrawfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintTextfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintRectfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintIconfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintBitmapfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintEnhMetafn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );
static void STATIC_PaintEtchedfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style );

struct static_extra_info
{
    HFONT hfont;
    union
    {
        HICON hicon;
        HBITMAP hbitmap;
        HENHMETAFILE hemf;
    } image;
    BOOL image_has_alpha;
};

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

static struct static_extra_info *get_extra_ptr( HWND hwnd, BOOL force )
{
    struct static_extra_info *extra = (struct static_extra_info *)GetWindowLongPtrW( hwnd, 0 );
    if (!extra && force)
    {
        extra = Alloc( sizeof(*extra) );
        if (extra)
            SetWindowLongPtrW( hwnd, 0, (ULONG_PTR)extra );
    }
    return extra;
}

static BOOL get_icon_size( HICON handle, SIZE *size )
{
    ICONINFO info;
    BITMAP bmp;
    int ret;

    if (!GetIconInfo(handle, &info))
        return FALSE;

    ret = GetObjectW(info.hbmColor, sizeof(bmp), &bmp);
    if (ret)
    {
        size->cx = bmp.bmWidth;
        size->cy = bmp.bmHeight;
    }

    DeleteObject(info.hbmMask);
    DeleteObject(info.hbmColor);

    return !!ret;
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
    struct static_extra_info *extra;

    if (hicon && !get_icon_size( hicon, &size ))
    {
        WARN("hicon != 0, but invalid\n");
        return 0;
    }

    extra = get_extra_ptr( hwnd, TRUE );
    if (!extra) return 0;

    prevIcon = extra->image.hicon;
    extra->image.hicon = hicon;
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
            SetWindowPos( hwnd, 0, 0, 0, size.cx, size.cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER );
        }
    }
    return prevIcon;
}

static HBITMAP create_alpha_bitmap( HBITMAP hbitmap )
{
    BITMAP bm;
    HBITMAP alpha;
    BITMAPINFO info;
    HDC hdc;
    void *bits;
    DWORD i;
    BYTE *ptr;
    BOOL has_alpha = FALSE;

    GetObjectW( hbitmap, sizeof(bm), &bm );
    if (bm.bmBitsPixel != 32) return 0;

    if (!(hdc = CreateCompatibleDC( 0 ))) return 0;

    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = bm.bmWidth;
    info.bmiHeader.biHeight = -bm.bmHeight;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = bm.bmWidth * bm.bmHeight * 4;
    info.bmiHeader.biXPelsPerMeter = 0;
    info.bmiHeader.biYPelsPerMeter = 0;
    info.bmiHeader.biClrUsed = 0;
    info.bmiHeader.biClrImportant = 0;
    if ((alpha = CreateDIBSection( hdc, &info, DIB_RGB_COLORS, &bits, NULL, 0 )))
    {
        GetDIBits( hdc, hbitmap, 0, bm.bmHeight, bits, &info, DIB_RGB_COLORS );

        for (i = 0, ptr = bits; i < bm.bmWidth * bm.bmHeight; i++, ptr += 4)
            if ((has_alpha = (ptr[3] != 0))) break;

        if (!has_alpha)
        {
            DeleteObject( alpha );
            alpha = 0;
        }
        else
        {
            /* pre-multiply by alpha */
            for (i = 0, ptr = bits; i < bm.bmWidth * bm.bmHeight; i++, ptr += 4)
            {
                unsigned int alpha = ptr[3];
                ptr[0] = (ptr[0] * alpha + 127) / 255;
                ptr[1] = (ptr[1] * alpha + 127) / 255;
                ptr[2] = (ptr[2] * alpha + 127) / 255;
            }
        }
    }

    DeleteDC( hdc );

    return alpha;
}

/***********************************************************************
 *           STATIC_SetBitmap
 *
 * Set the bitmap for an SS_BITMAP control.
 */
static HBITMAP STATIC_SetBitmap( HWND hwnd, HBITMAP hBitmap, DWORD style )
{
    HBITMAP hOldBitmap, alpha;
    struct static_extra_info *extra;

    if (hBitmap && GetObjectType(hBitmap) != OBJ_BITMAP)
    {
        WARN("hBitmap != 0, but it's not a bitmap\n");
        return 0;
    }

    extra = get_extra_ptr( hwnd, TRUE );
    if (!extra) return 0;

    hOldBitmap = extra->image.hbitmap;
    extra->image.hbitmap = hBitmap;
    extra->image_has_alpha = FALSE;

    if (hBitmap)
    {
        alpha = create_alpha_bitmap( hBitmap );
        if (alpha)
        {
            extra->image.hbitmap = alpha;
            extra->image_has_alpha = TRUE;
        }
    }

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
            SetWindowPos( hwnd, 0, 0, 0, bm.bmWidth, bm.bmHeight,
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
    HENHMETAFILE old_hemf;
    struct static_extra_info *extra;

    if (hEnhMetaFile && GetObjectType(hEnhMetaFile) != OBJ_ENHMETAFILE)
    {
        WARN("hEnhMetaFile != 0, but it's not an enhanced metafile\n");
        return 0;
    }

    extra = get_extra_ptr( hwnd, TRUE );
    if (!extra) return 0;

    old_hemf = extra->image.hemf;
    extra->image.hemf = hEnhMetaFile;

    return old_hemf;
}

/***********************************************************************
 *           STATIC_GetImage
 *
 * Gets the bitmap for an SS_BITMAP control, the icon/cursor for an
 * SS_ICON control or the enhanced metafile for an SS_ENHMETAFILE control.
 */
static HANDLE STATIC_GetImage( HWND hwnd, WPARAM wParam, DWORD style )
{
    struct static_extra_info *extra;

    switch (style & SS_TYPEMASK)
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

    extra = get_extra_ptr( hwnd, FALSE );
    return extra ? extra->image.hbitmap : 0;
}

static void STATIC_SetFont( HWND hwnd, HFONT hfont )
{
    struct static_extra_info *extra = get_extra_ptr( hwnd, TRUE );
    if (extra)
        extra->hfont = hfont;
}

static HFONT STATIC_GetFont( HWND hwnd )
{
    struct static_extra_info *extra = get_extra_ptr( hwnd, FALSE );
    return extra ? extra->hfont : 0;
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

static HBRUSH STATIC_SendWmCtlColorStatic(HWND hwnd, HDC hdc)
{
    HBRUSH hBrush;
    HWND parent = GetParent(hwnd);

    if (!parent) parent = hwnd;
    hBrush = (HBRUSH) SendMessageW( parent, WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)hwnd );
    if (!hBrush) /* did the app forget to call DefWindowProc ? */
    {
        /* FIXME: DefWindowProc should return different colors if a
                  manifest is present */
        hBrush = (HBRUSH)DefWindowProcW( parent, WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)hwnd);
    }
    return hBrush;
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
        hdc = GetDC( hwnd );
        hrgn = set_control_clipping( hdc, &rc );
        hbrush = STATIC_SendWmCtlColorStatic( hwnd, hdc );
        if (staticPaintFunc[style])
            (staticPaintFunc[style])( hwnd, hdc, hbrush, full_style );
        SelectClipRgn( hdc, hrgn );
        if (hrgn) DeleteObject( hrgn );
        ReleaseDC( hwnd, hdc );
    }
}

/***********************************************************************
 *           hasTextStyle
 *
 * Tests if the control displays text.
 */
static BOOL hasTextStyle( DWORD style )
{
    switch (style & SS_TYPEMASK)
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

static LRESULT CALLBACK STATIC_WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    const WCHAR *window_name;
    LRESULT lResult = 0;
    LONG full_style = GetWindowLongW( hwnd, GWL_STYLE );
    LONG style = full_style & SS_TYPEMASK;

    if (!IsWindow( hwnd )) return 0;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        HWND parent;

        if (style < 0L || style > SS_TYPEMASK)
        {
            ERR("Unknown style %#lx\n", style );
            return -1;
        }

        parent = GetParent( hwnd );
        if (parent)
            EnableThemeDialogTexture( parent, ETDT_ENABLE );
        break;
    }

    case WM_NCDESTROY:
        if (style == SS_ICON)
        {
            struct static_extra_info *extra = get_extra_ptr( hwnd, FALSE );
            if (extra)
            {
                if (extra->image_has_alpha)
                    DeleteObject( extra->image.hbitmap );
                Free( extra );
            }
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
        else
            return DefWindowProcW(hwnd, uMsg, wParam, lParam);

    case WM_ERASEBKGND:
        /* do all painting in WM_PAINT like Windows does */
        return 1;

    case WM_PRINTCLIENT:
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            RECT rect;
            HDC hdc = wParam ? (HDC)wParam : BeginPaint(hwnd, &ps);
            HRGN hrgn;
            HBRUSH hbrush;

            GetClientRect( hwnd, &rect );
            hrgn = set_control_clipping( hdc, &rect );
            hbrush = STATIC_SendWmCtlColorStatic( hwnd, hdc );
            if (staticPaintFunc[style])
                (staticPaintFunc[style])( hwnd, hdc, hbrush, full_style );
            SelectClipRgn( hdc, hrgn );
            if (hrgn) DeleteObject( hrgn );
            if (!wParam) EndPaint(hwnd, &ps);
        }
        break;

    case WM_ENABLE:
        STATIC_TryPaintFcn( hwnd, full_style );
        if (full_style & SS_NOTIFY)
        {
            if (wParam)
                SendMessageW( GetParent(hwnd), WM_COMMAND,
                              MAKEWPARAM( GetWindowLongPtrW(hwnd,GWLP_ID), STN_ENABLE ), (LPARAM)hwnd);
            else
                SendMessageW( GetParent(hwnd), WM_COMMAND,
                              MAKEWPARAM( GetWindowLongPtrW(hwnd,GWLP_ID), STN_DISABLE ), (LPARAM)hwnd);
        }
        break;

    case WM_SYSCOLORCHANGE:
        COMCTL32_RefreshSysColors();
        STATIC_TryPaintFcn( hwnd, full_style );
        break;

    case WM_THEMECHANGED:
        InvalidateRect( hwnd, 0, TRUE );
        break;

    case WM_NCCREATE:
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;

            if (full_style & SS_SUNKEN || style == SS_ETCHEDHORZ || style == SS_ETCHEDVERT)
                SetWindowLongW( hwnd, GWL_EXSTYLE,
                                GetWindowLongW( hwnd, GWL_EXSTYLE ) | WS_EX_STATICEDGE );

            if (style == SS_ETCHEDHORZ || style == SS_ETCHEDVERT) {
                RECT rc;
                GetClientRect(hwnd, &rc);
                if (style == SS_ETCHEDHORZ)
                    rc.bottom = rc.top;
                else
                    rc.right = rc.left;
                AdjustWindowRectEx(&rc, full_style, FALSE, GetWindowLongW(hwnd, GWL_EXSTYLE));
                SetWindowPos(hwnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
            }

            if (cs->lpszName && cs->lpszName[0] == 0xffff)
                window_name = MAKEINTRESOURCEW(cs->lpszName[1]);
            else
                window_name = cs->lpszName;

            switch (style)
            {
            case SS_ICON:
                {
                    HICON hIcon;

                    hIcon = STATIC_LoadIconW(cs->hInstance, window_name, full_style);
                    STATIC_SetIcon(hwnd, hIcon, full_style);
                }
                break;
            case SS_BITMAP:
                if ((ULONG_PTR)cs->hInstance >> 16)
                {
                    HBITMAP hBitmap;
                    hBitmap = LoadBitmapW(cs->hInstance, window_name);
                    STATIC_SetBitmap(hwnd, hBitmap, full_style);
                }
                break;
            }
            /* SS_ENHMETAFILE: Despite what MSDN says, Windows does not load
               the enhanced metafile that was specified as the window text. */
        }
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);

    case WM_SETTEXT:
        if (hasTextStyle( full_style ))
        {
            lResult = DefWindowProcW( hwnd, uMsg, wParam, lParam );
            STATIC_TryPaintFcn( hwnd, full_style );
        }
        break;

    case WM_SETFONT:
        if (hasTextStyle( full_style ))
        {
            STATIC_SetFont( hwnd, (HFONT)wParam );
            if (LOWORD(lParam))
                RedrawWindow( hwnd, NULL, 0, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
        }
        break;

    case WM_GETFONT:
        return (LRESULT)STATIC_GetFont( hwnd );

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
        switch (wParam)
        {
        case IMAGE_BITMAP:
            if (style != SS_BITMAP) return 0;
            lResult = (LRESULT)STATIC_SetBitmap( hwnd, (HBITMAP)lParam, full_style );
            break;
        case IMAGE_ENHMETAFILE:
            if (style != SS_ENHMETAFILE) return 0;
            lResult = (LRESULT)STATIC_SetEnhMetaFile( hwnd, (HENHMETAFILE)lParam, full_style );
            break;
        case IMAGE_ICON:
        case IMAGE_CURSOR:
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

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return lResult;
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

    font = STATIC_GetFont( hwnd );
    if (font) oldFont = SelectObject( hdc, font );
    SendMessageW( GetParent(hwnd), WM_DRAWITEM, id, (LPARAM)&dis );
    if (font) SelectObject( hdc, oldFont );
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

    if ((hFont = STATIC_GetFont( hwnd )))
        hOldFont = SelectObject( hdc, hFont );

    if ((style & SS_TYPEMASK) != SS_SIMPLE)
    {
        FillRect( hdc, &rc, hbrush );
        if (!IsWindowEnabled(hwnd)) SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
    }

    buf_size = 256;
    if (!(text = Alloc( buf_size * sizeof(WCHAR) )))
        goto no_TextOut;

    while ((len = InternalGetWindowText( hwnd, text, buf_size )) == buf_size - 1)
    {
        buf_size *= 2;
        if (!(text = ReAlloc( text, buf_size * sizeof(WCHAR) )))
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
    else
    {
        DrawTextW( hdc, text, -1, &rc, format );
    }

no_TextOut:
    Free( text );

    if (hFont)
        SelectObject( hdc, hOldFont );
}

static void STATIC_PaintRectfn( HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style )
{
    RECT rc;

    GetClientRect( hwnd, &rc);

    switch (style & SS_TYPEMASK)
    {
    case SS_BLACKRECT:
        hbrush = CreateSolidBrush(comctl32_color.clr3dDkShadow);
        FillRect( hdc, &rc, hbrush );
        break;
    case SS_GRAYRECT:
        hbrush = CreateSolidBrush(comctl32_color.clr3dShadow);
        FillRect( hdc, &rc, hbrush );
        break;
    case SS_WHITERECT:
        hbrush = CreateSolidBrush(comctl32_color.clr3dHilight);
        FillRect( hdc, &rc, hbrush );
        break;
    case SS_BLACKFRAME:
        hbrush = CreateSolidBrush(comctl32_color.clr3dDkShadow);
        FrameRect( hdc, &rc, hbrush );
        break;
    case SS_GRAYFRAME:
        hbrush = CreateSolidBrush(comctl32_color.clr3dShadow);
        FrameRect( hdc, &rc, hbrush );
        break;
    case SS_WHITEFRAME:
        hbrush = CreateSolidBrush(comctl32_color.clr3dHilight);
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
    hIcon = STATIC_GetImage( hwnd, IMAGE_ICON, style );
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
        DrawIconEx( hdc, iconRect.left, iconRect.top, hIcon, iconRect.right - iconRect.left,
                    iconRect.bottom - iconRect.top, 0, NULL, DI_NORMAL );
    }
}

static void STATIC_PaintBitmapfn(HWND hwnd, HDC hdc, HBRUSH hbrush, DWORD style )
{
    HDC hMemDC;
    HBITMAP hBitmap, oldbitmap;
    RECT rcClient;

    GetClientRect( hwnd, &rcClient );
    FillRect( hdc, &rcClient, hbrush );

    if ((hBitmap = STATIC_GetImage( hwnd, IMAGE_BITMAP, style ))
         && (GetObjectType(hBitmap) == OBJ_BITMAP)
         && (hMemDC = CreateCompatibleDC( hdc )))
    {
        BITMAP bm;
        LOGBRUSH brush;
        BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
        struct static_extra_info *extra = get_extra_ptr( hwnd, FALSE );

        GetObjectW(hBitmap, sizeof(bm), &bm);
        oldbitmap = SelectObject(hMemDC, hBitmap);

        /* Set the background color for monochrome bitmaps
           to the color of the background brush */
        if (GetObjectW( hbrush, sizeof(brush), &brush ))
        {
            if (brush.lbStyle == BS_SOLID)
                SetBkColor(hdc, brush.lbColor);
        }
        if (style & SS_CENTERIMAGE)
        {
            rcClient.left = (rcClient.right - rcClient.left)/2 - bm.bmWidth/2;
            rcClient.top = (rcClient.bottom - rcClient.top)/2 - bm.bmHeight/2;
            rcClient.right = rcClient.left + bm.bmWidth;
            rcClient.bottom = rcClient.top + bm.bmHeight;
        }

        if (extra->image_has_alpha)
            GdiAlphaBlend(hdc, rcClient.left, rcClient.top, rcClient.right - rcClient.left,
                   rcClient.bottom - rcClient.top, hMemDC,
                   0, 0, bm.bmWidth, bm.bmHeight, blend);
        else
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
    if ((hEnhMetaFile = STATIC_GetImage( hwnd, IMAGE_ENHMETAFILE, style )))
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
    DrawEdge(hdc, &rc, EDGE_ETCHED, BF_RECT);
}

void STATIC_Register(void)
{
    WNDCLASSW wndClass;

    memset(&wndClass, 0, sizeof(wndClass));
    wndClass.style = CS_DBLCLKS | CS_PARENTDC | CS_GLOBALCLASS;
    wndClass.lpfnWndProc = STATIC_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = sizeof(struct static_extra_info *);
    wndClass.hCursor = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = NULL;
    wndClass.lpszClassName = WC_STATICW;
    RegisterClassW(&wndClass);
}
