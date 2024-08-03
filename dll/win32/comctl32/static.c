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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commctrl.h"

#include "wine/debug.h"

#include "comctl32.h"

WINE_DEFAULT_DEBUG_CHANNEL(static);

static void STATIC_PaintOwnerDrawfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintTextfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintRectfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintIconfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintBitmapfn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintEnhMetafn( HWND hwnd, HDC hdc, DWORD style );
static void STATIC_PaintEtchedfn( HWND hwnd, HDC hdc, DWORD style );

/* offsets for GetWindowLong for static private information */
#define HFONT_GWL_OFFSET    0
#define HICON_GWL_OFFSET    (sizeof(HFONT))
#define STATIC_EXTRA_BYTES  (HICON_GWL_OFFSET + sizeof(HICON))

typedef void (*pfPaint)( HWND hwnd, HDC hdc, DWORD style );

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
    STATIC_PaintEtchedfn,    /* SS_ETCHEDHORZ */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDVERT */
    STATIC_PaintEtchedfn,    /* SS_ETCHEDFRAME */
};

static BOOL get_icon_size( HICON handle, SIZE *size )
{
    ICONINFO info;
    BITMAP bmp;
    int ret;

    if (!GetIconInfo(handle, &info))
        return FALSE;

#ifdef __REACTOS__
    ret = GetObjectW(info.hbmMask, sizeof(bmp), &bmp);
#else
    ret = GetObjectW(info.hbmColor, sizeof(bmp), &bmp);
#endif
    if (ret)
    {
        size->cx = bmp.bmWidth;
        size->cy = bmp.bmHeight;
#ifdef __REACTOS__
        /*
            If this structure defines a black and white icon, this bitmask is formatted
            so that the upper half is the icon AND bitmask and the lower half is
            the icon XOR bitmask.
        */
        if (!info.hbmColor)
            size->cy /= 2;
#endif
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

    if ((style & SS_TYPEMASK) != SS_ICON) return 0;
    if (hicon && !get_icon_size( hicon, &size ))
    {
        WARN("hicon != 0, but invalid\n");
        return 0;
    }
    prevIcon = (HICON)SetWindowLongPtrW( hwnd, HICON_GWL_OFFSET, (LONG_PTR)hicon );
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

/***********************************************************************
 *           STATIC_SetBitmap
 *
 * Set the bitmap for an SS_BITMAP control.
 */
static HBITMAP STATIC_SetBitmap( HWND hwnd, HBITMAP hBitmap, DWORD style )
{
    HBITMAP hOldBitmap;

    if ((style & SS_TYPEMASK) != SS_BITMAP) return 0;
    if (hBitmap && GetObjectType(hBitmap) != OBJ_BITMAP)
    {
        WARN("hBitmap != 0, but it's not a bitmap\n");
        return 0;
    }
    hOldBitmap = (HBITMAP)SetWindowLongPtrW( hwnd, HICON_GWL_OFFSET, (LONG_PTR)hBitmap );
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
    if ((style & SS_TYPEMASK) != SS_ENHMETAFILE) return 0;
    if (hEnhMetaFile && GetObjectType(hEnhMetaFile) != OBJ_ENHMETAFILE)
    {
        WARN("hEnhMetaFile != 0, but it's not an enhanced metafile\n");
        return 0;
    }
    return (HENHMETAFILE)SetWindowLongPtrW( hwnd, HICON_GWL_OFFSET, (LONG_PTR)hEnhMetaFile );
}

/***********************************************************************
 *           STATIC_GetImage
 *
 * Gets the bitmap for an SS_BITMAP control, the icon/cursor for an
 * SS_ICON control or the enhanced metafile for an SS_ENHMETAFILE control.
 */
static HANDLE STATIC_GetImage( HWND hwnd, WPARAM wParam, DWORD style )
{
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
    return (HANDLE)GetWindowLongPtrW( hwnd, HICON_GWL_OFFSET );
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
    LONG style = full_style & SS_TYPEMASK;
    RECT rc;

    GetClientRect( hwnd, &rc );
    if (!IsRectEmpty(&rc) && IsWindowVisible(hwnd) && staticPaintFunc[style])
    {
        HDC hdc;
        HRGN hrgn;

        hdc = GetDC( hwnd );
        hrgn = set_control_clipping( hdc, &rc );
        (staticPaintFunc[style])( hwnd, hdc, full_style );
        SelectClipRgn( hdc, hrgn );
        if (hrgn) DeleteObject( hrgn );
        ReleaseDC( hwnd, hdc );
    }
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
    LRESULT lResult = 0;
    LONG full_style = GetWindowLongW( hwnd, GWL_STYLE );
    LONG style = full_style & SS_TYPEMASK;

    if (!IsWindow( hwnd )) return 0;

    switch (uMsg)
    {
    case WM_CREATE:
        if (style < 0L || style > SS_TYPEMASK)
        {
            ERR("Unknown style 0x%02x\n", style );
            return -1;
        }
        break;

    case WM_NCDESTROY:
        if (style == SS_ICON)
        {
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
            GetClientRect( hwnd, &rect );
            if (staticPaintFunc[style])
            {
                HRGN hrgn = set_control_clipping( hdc, &rect );
                (staticPaintFunc[style])( hwnd, hdc, full_style );
                SelectClipRgn( hdc, hrgn );
                if (hrgn) DeleteObject( hrgn );
            }
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

    case WM_NCCREATE:
        {
            CREATESTRUCTW *cs = (CREATESTRUCTW *)lParam;

            if (full_style & SS_SUNKEN)
                SetWindowLongW( hwnd, GWL_EXSTYLE,
                                GetWindowLongW( hwnd, GWL_EXSTYLE ) | WS_EX_STATICEDGE );

            switch (style)
            {
            case SS_ICON:
                {
                    HICON hIcon;

                    hIcon = STATIC_LoadIconW(cs->hInstance, cs->lpszName, full_style);
                    STATIC_SetIcon(hwnd, hIcon, full_style);
                }
                break;
            case SS_BITMAP:
                if ((ULONG_PTR)cs->hInstance >> 16)
                {
                    HBITMAP hBitmap;
                    hBitmap = LoadBitmapW(cs->hInstance, cs->lpszName);
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
            SetWindowLongPtrW( hwnd, HFONT_GWL_OFFSET, wParam );
            if (LOWORD(lParam))
                RedrawWindow( hwnd, NULL, 0, RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW | RDW_ALLCHILDREN );
        }
        break;

    case WM_GETFONT:
        return GetWindowLongPtrW( hwnd, HFONT_GWL_OFFSET );

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
            lResult = (LRESULT)STATIC_SetBitmap( hwnd, (HBITMAP)lParam, full_style );
            break;
        case IMAGE_ENHMETAFILE:
            lResult = (LRESULT)STATIC_SetEnhMetaFile( hwnd, (HENHMETAFILE)lParam, full_style );
            break;
        case IMAGE_ICON:
        case IMAGE_CURSOR:
            lResult = (LRESULT)STATIC_SetIcon( hwnd, (HICON)lParam, full_style );
            break;
        default:
            FIXME("STM_SETIMAGE: Unhandled type %lx\n", wParam);
            break;
        }
        STATIC_TryPaintFcn( hwnd, full_style );
        break;

    case STM_SETICON:
        lResult = (LRESULT)STATIC_SetIcon( hwnd, (HICON)wParam, full_style );
        STATIC_TryPaintFcn( hwnd, full_style );
        break;

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return lResult;
}

static void STATIC_PaintOwnerDrawfn( HWND hwnd, HDC hdc, DWORD style )
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

    font = (HFONT)GetWindowLongPtrW( hwnd, HFONT_GWL_OFFSET );
    if (font) oldFont = SelectObject( hdc, font );
    SendMessageW( GetParent(hwnd), WM_CTLCOLORSTATIC, (WPARAM)hdc, (LPARAM)hwnd );
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

static void STATIC_PaintTextfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;
    HBRUSH hBrush;
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

    if ((hFont = (HFONT)GetWindowLongPtrW( hwnd, HFONT_GWL_OFFSET )))
        hOldFont = SelectObject( hdc, hFont );

    /* SS_SIMPLE controls: WM_CTLCOLORSTATIC is sent, but the returned
                           brush is not used */
    hBrush = STATIC_SendWmCtlColorStatic(hwnd, hdc);

    if ((style & SS_TYPEMASK) != SS_SIMPLE)
    {
        FillRect( hdc, &rc, hBrush );
        if (!IsWindowEnabled(hwnd)) SetTextColor(hdc, GetSysColor(COLOR_GRAYTEXT));
    }

    buf_size = 256;
    if (!(text = HeapAlloc( GetProcessHeap(), 0, buf_size * sizeof(WCHAR) )))
        goto no_TextOut;

    while ((len = InternalGetWindowText( hwnd, text, buf_size )) == buf_size - 1)
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
    else
    {
        UINT flags = DST_COMPLEX;
        if (style & WS_DISABLED)
            flags |= DSS_DISABLED;
        DrawStateW(hdc, hBrush, STATIC_DrawTextCallback,
                   (LPARAM)text, (WPARAM)format,
                   rc.left, rc.top,
                   rc.right - rc.left, rc.bottom - rc.top,
                   flags);
    }

no_TextOut:
    HeapFree( GetProcessHeap(), 0, text );

    if (hFont)
        SelectObject( hdc, hOldFont );
}

static void STATIC_PaintRectfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;
    HBRUSH hBrush;

    GetClientRect( hwnd, &rc);

    /* FIXME: send WM_CTLCOLORSTATIC */
    switch (style & SS_TYPEMASK)
    {
    case SS_BLACKRECT:
        hBrush = CreateSolidBrush(comctl32_color.clr3dDkShadow);
        FillRect( hdc, &rc, hBrush );
        break;
    case SS_GRAYRECT:
        hBrush = CreateSolidBrush(comctl32_color.clr3dShadow);
        FillRect( hdc, &rc, hBrush );
        break;
    case SS_WHITERECT:
        hBrush = CreateSolidBrush(comctl32_color.clr3dHilight);
        FillRect( hdc, &rc, hBrush );
        break;
    case SS_BLACKFRAME:
        hBrush = CreateSolidBrush(comctl32_color.clr3dDkShadow);
        FrameRect( hdc, &rc, hBrush );
        break;
    case SS_GRAYFRAME:
        hBrush = CreateSolidBrush(comctl32_color.clr3dShadow);
        FrameRect( hdc, &rc, hBrush );
        break;
    case SS_WHITEFRAME:
        hBrush = CreateSolidBrush(comctl32_color.clr3dHilight);
        FrameRect( hdc, &rc, hBrush );
        break;
    default:
        return;
    }
    DeleteObject( hBrush );
}


static void STATIC_PaintIconfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc, iconRect;
    HBRUSH hbrush;
    HICON hIcon;
    SIZE size;

    GetClientRect( hwnd, &rc );
    hbrush = STATIC_SendWmCtlColorStatic(hwnd, hdc);
    hIcon = (HICON)GetWindowLongPtrW( hwnd, HICON_GWL_OFFSET );
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

static void STATIC_PaintBitmapfn(HWND hwnd, HDC hdc, DWORD style )
{
    HDC hMemDC;
    HBITMAP hBitmap, oldbitmap;
    HBRUSH hbrush;

    hbrush = STATIC_SendWmCtlColorStatic(hwnd, hdc);

    if ((hBitmap = (HBITMAP)GetWindowLongPtrW( hwnd, HICON_GWL_OFFSET ))
         && (GetObjectType(hBitmap) == OBJ_BITMAP)
         && (hMemDC = CreateCompatibleDC( hdc )))
    {
        BITMAP bm;
        RECT rcClient;
        LOGBRUSH brush;

        GetObjectW(hBitmap, sizeof(bm), &bm);
        oldbitmap = SelectObject(hMemDC, hBitmap);

        /* Set the background color for monochrome bitmaps
           to the color of the background brush */
        if (GetObjectW( hbrush, sizeof(brush), &brush ))
        {
            if (brush.lbStyle == BS_SOLID)
                SetBkColor(hdc, brush.lbColor);
        }
        GetClientRect(hwnd, &rcClient);
        if (style & SS_CENTERIMAGE)
        {
            FillRect( hdc, &rcClient, hbrush );
            rcClient.left = (rcClient.right - rcClient.left)/2 - bm.bmWidth/2;
            rcClient.top = (rcClient.bottom - rcClient.top)/2 - bm.bmHeight/2;
            rcClient.right = rcClient.left + bm.bmWidth;
            rcClient.bottom = rcClient.top + bm.bmHeight;
        }
        StretchBlt(hdc, rcClient.left, rcClient.top, rcClient.right - rcClient.left,
                   rcClient.bottom - rcClient.top, hMemDC,
                   0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);
        SelectObject(hMemDC, oldbitmap);
        DeleteDC(hMemDC);
    }
}

static void STATIC_PaintEnhMetafn(HWND hwnd, HDC hdc, DWORD style )
{
    HENHMETAFILE hEnhMetaFile;
    RECT rc;
    HBRUSH hbrush;

    GetClientRect(hwnd, &rc);
    hbrush = STATIC_SendWmCtlColorStatic(hwnd, hdc);
    FillRect(hdc, &rc, hbrush);
    if ((hEnhMetaFile = (HENHMETAFILE)GetWindowLongPtrW( hwnd, HICON_GWL_OFFSET )))
    {
        /* The control's current font is not selected into the
           device context! */
        if (GetObjectType(hEnhMetaFile) == OBJ_ENHMETAFILE)
            PlayEnhMetaFile(hdc, hEnhMetaFile, &rc);
    }
}

static void STATIC_PaintEtchedfn( HWND hwnd, HDC hdc, DWORD style )
{
    RECT rc;

    /* FIXME: sometimes (not always) sends WM_CTLCOLORSTATIC */
    GetClientRect( hwnd, &rc );
    switch (style & SS_TYPEMASK)
    {
    case SS_ETCHEDHORZ:
        DrawEdge(hdc, &rc, EDGE_ETCHED, BF_TOP | BF_BOTTOM);
        break;
    case SS_ETCHEDVERT:
        DrawEdge(hdc, &rc, EDGE_ETCHED, BF_LEFT | BF_RIGHT);
        break;
    case SS_ETCHEDFRAME:
        DrawEdge(hdc, &rc, EDGE_ETCHED, BF_RECT);
        break;
    }
}

void STATIC_Register(void)
{
    WNDCLASSW wndClass;

    memset(&wndClass, 0, sizeof(wndClass));
    wndClass.style = CS_DBLCLKS | CS_PARENTDC | CS_GLOBALCLASS;
    wndClass.lpfnWndProc = STATIC_WindowProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = STATIC_EXTRA_BYTES;
    wndClass.hCursor = LoadCursorW(0, (LPWSTR)IDC_ARROW);
    wndClass.hbrBackground = NULL;
    wndClass.lpszClassName = WC_STATICW;
    RegisterClassW(&wndClass);
}

#ifdef __REACTOS__
void STATIC_Unregister(void)
{
    UnregisterClassW(WC_STATICW, NULL);
}
#endif
