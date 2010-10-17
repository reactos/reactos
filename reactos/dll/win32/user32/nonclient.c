/*
 * Non-client area window functions
 *
 * Copyright 1994 Alexandre Julliard
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "win.h"
#include "user_private.h"
#include "controls.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nonclient);

#define SC_ABOUTWINE            (SC_SCREENSAVE+1)

  /* Some useful macros */
#define HAS_DLGFRAME(style,exStyle) \
    (((exStyle) & WS_EX_DLGMODALFRAME) || \
     (((style) & WS_DLGFRAME) && !((style) & WS_THICKFRAME)))

#define HAS_THICKFRAME(style,exStyle) \
    (((style) & WS_THICKFRAME) && \
     !(((style) & (WS_DLGFRAME|WS_BORDER)) == WS_DLGFRAME))

#define HAS_THINFRAME(style) \
    (((style) & WS_BORDER) || !((style) & (WS_CHILD | WS_POPUP)))

#define HAS_BIGFRAME(style,exStyle) \
    (((style) & (WS_THICKFRAME | WS_DLGFRAME)) || \
     ((exStyle) & WS_EX_DLGMODALFRAME))

#define HAS_STATICOUTERFRAME(style,exStyle) \
    (((exStyle) & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) == \
     WS_EX_STATICEDGE)

#define HAS_ANYFRAME(style,exStyle) \
    (((style) & (WS_THICKFRAME | WS_DLGFRAME | WS_BORDER)) || \
     ((exStyle) & WS_EX_DLGMODALFRAME) || \
     !((style) & (WS_CHILD | WS_POPUP)))

#define HAS_MENU(hwnd,style)  ((((style) & (WS_CHILD | WS_POPUP)) != WS_CHILD) && GetMenu(hwnd))


/******************************************************************************
 * NC_AdjustRectOuter
 *
 * Computes the size of the "outside" parts of the window based on the
 * parameters of the client area.
 *
 * PARAMS
 *     LPRECT  rect
 *     DWORD  style
 *     BOOL  menu
 *     DWORD  exStyle
 *
 * NOTES
 *     "Outer" parts of a window means the whole window frame, caption and
 *     menu bar. It does not include "inner" parts of the frame like client
 *     edge, static edge or scroll bars.
 *
 *****************************************************************************/

static void
NC_AdjustRectOuter (LPRECT rect, DWORD style, BOOL menu, DWORD exStyle)
{
    int adjust;

    if ((exStyle & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) ==
        WS_EX_STATICEDGE)
    {
        adjust = 1; /* for the outer frame always present */
    }
    else
    {
        adjust = 0;
        if ((exStyle & WS_EX_DLGMODALFRAME) ||
            (style & (WS_THICKFRAME|WS_DLGFRAME))) adjust = 2; /* outer */
    }
    if ((style & WS_THICKFRAME) && !(exStyle & WS_EX_DLGMODALFRAME))
        adjust +=  ( GetSystemMetrics (SM_CXFRAME)
                   - GetSystemMetrics (SM_CXDLGFRAME)); /* The resize border */
    if ((style & (WS_BORDER|WS_DLGFRAME)) ||
        (exStyle & WS_EX_DLGMODALFRAME))
        adjust++; /* The other border */

    InflateRect (rect, adjust, adjust);

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (exStyle & WS_EX_TOOLWINDOW)
            rect->top -= GetSystemMetrics(SM_CYSMCAPTION);
        else
            rect->top -= GetSystemMetrics(SM_CYCAPTION);
    }
    if (menu) rect->top -= GetSystemMetrics(SM_CYMENU);
}


/******************************************************************************
 * NC_AdjustRectInner
 *
 * Computes the size of the "inside" part of the window based on the
 * parameters of the client area.
 *
 * PARAMS
 *     LPRECT   rect
 *     DWORD    style
 *     DWORD    exStyle
 *
 * NOTES
 *     "Inner" part of a window means the window frame inside of the flat
 *     window frame. It includes the client edge, the static edge and the
 *     scroll bars.
 *
 *****************************************************************************/

static void
NC_AdjustRectInner (LPRECT rect, DWORD style, DWORD exStyle)
{
    if (exStyle & WS_EX_CLIENTEDGE)
        InflateRect(rect, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));

    if (style & WS_VSCROLL)
    {
        if((exStyle & WS_EX_LEFTSCROLLBAR) != 0)
            rect->left  -= GetSystemMetrics(SM_CXVSCROLL);
        else
            rect->right += GetSystemMetrics(SM_CXVSCROLL);
    }
    if (style & WS_HSCROLL) rect->bottom += GetSystemMetrics(SM_CYHSCROLL);
}



static HICON NC_IconForWindow( HWND hwnd )
{
    HICON hIcon = 0;
    WND *wndPtr = WIN_GetPtr( hwnd );

    if (wndPtr && wndPtr != WND_OTHER_PROCESS && wndPtr != WND_DESKTOP)
    {
        hIcon = wndPtr->hIconSmall;
        if (!hIcon) hIcon = wndPtr->hIcon;
        WIN_ReleasePtr( wndPtr );
    }
    if (!hIcon) hIcon = (HICON) GetClassLongPtrW( hwnd, GCLP_HICONSM );
    if (!hIcon) hIcon = (HICON) GetClassLongPtrW( hwnd, GCLP_HICON );

    /* If there is no hIcon specified and this is a modal dialog,
     * get the default one.
     */
    if (!hIcon && (GetWindowLongW( hwnd, GWL_STYLE ) & DS_MODALFRAME))
        hIcon = LoadImageW(0, (LPCWSTR)IDI_WINLOGO, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
    return hIcon;
}

/* Draws the bar part(ie the big rectangle) of the caption */
static void NC_DrawCaptionBar (HDC hdc, const RECT *rect, DWORD dwStyle, 
                               BOOL active, BOOL gradient)
{
    if (gradient)
    {
        TRIVERTEX vertices[6];
        DWORD colLeft = 
            GetSysColor (active ? COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION);
        DWORD colRight = 
            GetSysColor (active ? COLOR_GRADIENTACTIVECAPTION 
                                : COLOR_GRADIENTINACTIVECAPTION);
        int v;
        int buttonsAreaSize = GetSystemMetrics(SM_CYCAPTION) - 1;
        static GRADIENT_RECT mesh[] = {{0, 1}, {2, 3}, {4, 5}};
    
        for (v = 0; v < 3; v++)
        {
            vertices[v].Red = GetRValue (colLeft) << 8;
            vertices[v].Green = GetGValue (colLeft) << 8;
            vertices[v].Blue = GetBValue (colLeft) << 8;
            vertices[v].Alpha = 0x8000;
            vertices[v+3].Red = GetRValue (colRight) << 8;
            vertices[v+3].Green = GetGValue (colRight) << 8;
            vertices[v+3].Blue = GetBValue (colRight) << 8;
            vertices[v+3].Alpha = 0x8000;
        }
    
        if ((dwStyle & WS_SYSMENU) 
            && ((dwStyle & WS_MAXIMIZEBOX) || (dwStyle & WS_MINIMIZEBOX)))
            buttonsAreaSize += 2 * (GetSystemMetrics(SM_CXSIZE) + 1);
        
        /* area behind icon; solid filled with left color */
        vertices[0].x = rect->left;
        vertices[0].y = rect->top;
        if (dwStyle & WS_SYSMENU) 
            vertices[1].x = 
                min (rect->left + GetSystemMetrics(SM_CXSMICON), rect->right);
        else
            vertices[1].x = vertices[0].x;
        vertices[1].y = rect->bottom;
        
        /* area behind text; gradient */
        vertices[2].x = vertices[1].x;
        vertices[2].y = rect->top;
        vertices[3].x = max (vertices[2].x, rect->right - buttonsAreaSize);
        vertices[3].y = rect->bottom;
        
        /* area behind buttons; solid filled with right color */
        vertices[4].x = vertices[3].x;
        vertices[4].y = rect->top;
        vertices[5].x = rect->right;
        vertices[5].y = rect->bottom;
        
        GdiGradientFill (hdc, vertices, 6, mesh, 3, GRADIENT_FILL_RECT_H);
    }
    else
        FillRect (hdc, rect, GetSysColorBrush (active ?
                  COLOR_ACTIVECAPTION : COLOR_INACTIVECAPTION));
}

/***********************************************************************
 *		DrawCaption (USER32.@) Draws a caption bar
 *
 * PARAMS
 *     hwnd   [I]
 *     hdc    [I]
 *     lpRect [I]
 *     uFlags [I]
 *
 * RETURNS
 *     Success:
 *     Failure:
 */

BOOL WINAPI
DrawCaption (HWND hwnd, HDC hdc, const RECT *lpRect, UINT uFlags)
{
    return DrawCaptionTempW (hwnd, hdc, lpRect, 0, 0, NULL, uFlags & 0x103F);
}


/***********************************************************************
 *		DrawCaptionTempA (USER32.@)
 */
BOOL WINAPI DrawCaptionTempA (HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont,
                              HICON hIcon, LPCSTR str, UINT uFlags)
{
    LPWSTR strW;
    INT len;
    BOOL ret = FALSE;

    if (!(uFlags & DC_TEXT) || !str)
        return DrawCaptionTempW( hwnd, hdc, rect, hFont, hIcon, NULL, uFlags );

    len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0 );
    if ((strW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, str, -1, strW, len );
        ret = DrawCaptionTempW (hwnd, hdc, rect, hFont, hIcon, strW, uFlags);
        HeapFree( GetProcessHeap (), 0, strW );
    }
    return ret;
}


/***********************************************************************
 *		DrawCaptionTempW (USER32.@)
 */
BOOL WINAPI DrawCaptionTempW (HWND hwnd, HDC hdc, const RECT *rect, HFONT hFont,
                              HICON hIcon, LPCWSTR str, UINT uFlags)
{
    RECT   rc = *rect;

    TRACE("(%p,%p,%p,%p,%p,%s,%08x)\n",
          hwnd, hdc, rect, hFont, hIcon, debugstr_w(str), uFlags);

    /* drawing background */
    if (uFlags & DC_INBUTTON) {
        FillRect (hdc, &rc, GetSysColorBrush (COLOR_3DFACE));

        if (uFlags & DC_ACTIVE) {
            HBRUSH hbr = SelectObject (hdc, SYSCOLOR_55AABrush);
            PatBlt (hdc, rc.left, rc.top,
                      rc.right-rc.left, rc.bottom-rc.top, 0xFA0089);
            SelectObject (hdc, hbr);
        }
    }
    else {
        DWORD style = GetWindowLongW (hwnd, GWL_STYLE);
        NC_DrawCaptionBar (hdc, &rc, style, uFlags & DC_ACTIVE, uFlags & DC_GRADIENT);
    }


    /* drawing icon */
    if ((uFlags & DC_ICON) && !(uFlags & DC_SMALLCAP)) {
        POINT pt;

        pt.x = rc.left + 2;
        pt.y = (rc.bottom + rc.top - GetSystemMetrics(SM_CYSMICON)) / 2;

        if (!hIcon) hIcon = NC_IconForWindow(hwnd);
        DrawIconEx (hdc, pt.x, pt.y, hIcon, GetSystemMetrics(SM_CXSMICON),
                    GetSystemMetrics(SM_CYSMICON), 0, 0, DI_NORMAL);
        rc.left += (rc.bottom - rc.top);
    }

    /* drawing text */
    if (uFlags & DC_TEXT) {
        HFONT hOldFont;

        if (uFlags & DC_INBUTTON)
            SetTextColor (hdc, GetSysColor (COLOR_BTNTEXT));
        else if (uFlags & DC_ACTIVE)
            SetTextColor (hdc, GetSysColor (COLOR_CAPTIONTEXT));
        else
            SetTextColor (hdc, GetSysColor (COLOR_INACTIVECAPTIONTEXT));

        SetBkMode (hdc, TRANSPARENT);

        if (hFont)
            hOldFont = SelectObject (hdc, hFont);
        else {
            NONCLIENTMETRICSW nclm;
            HFONT hNewFont;
            nclm.cbSize = sizeof(NONCLIENTMETRICSW);
            SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
            hNewFont = CreateFontIndirectW ((uFlags & DC_SMALLCAP) ?
                &nclm.lfSmCaptionFont : &nclm.lfCaptionFont);
            hOldFont = SelectObject (hdc, hNewFont);
        }

        if (str)
            DrawTextW (hdc, str, -1, &rc,
                         DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_LEFT);
        else {
            WCHAR szText[128];
            INT nLen;
            nLen = GetWindowTextW (hwnd, szText, 128);
            DrawTextW (hdc, szText, nLen, &rc,
                         DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_LEFT);
        }

        if (hFont)
            SelectObject (hdc, hOldFont);
        else
            DeleteObject (SelectObject (hdc, hOldFont));
    }

    /* drawing focus ??? */
    if (uFlags & 0x2000)
        FIXME("undocumented flag (0x2000)!\n");

    return 0;
}


/***********************************************************************
 *		AdjustWindowRect (USER32.@)
 */
BOOL WINAPI AdjustWindowRect( LPRECT rect, DWORD style, BOOL menu )
{
    return AdjustWindowRectEx( rect, style, menu, 0 );
}


/***********************************************************************
 *		AdjustWindowRectEx (USER32.@)
 */
BOOL WINAPI AdjustWindowRectEx( LPRECT rect, DWORD style, BOOL menu, DWORD exStyle )
{
    if (style & WS_ICONIC) return TRUE;
    style &= ~(WS_HSCROLL | WS_VSCROLL);

    TRACE("(%s) %08x %d %08x\n", wine_dbgstr_rect(rect), style, menu, exStyle );

    NC_AdjustRectOuter( rect, style, menu, exStyle );
    NC_AdjustRectInner( rect, style, exStyle );

    return TRUE;
}


/***********************************************************************
 *           NC_HandleNCCalcSize
 *
 * Handle a WM_NCCALCSIZE message. Called from DefWindowProc().
 */
LRESULT NC_HandleNCCalcSize( HWND hwnd, WPARAM wparam, RECT *winRect )
{
    RECT tmpRect = { 0, 0, 0, 0 };
    LRESULT result = 0;
    LONG cls_style = GetClassLongW(hwnd, GCL_STYLE);
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );
    LONG exStyle = GetWindowLongW( hwnd, GWL_EXSTYLE );

    if (winRect == NULL)
        return 0;

    if (cls_style & CS_VREDRAW) result |= WVR_VREDRAW;
    if (cls_style & CS_HREDRAW) result |= WVR_HREDRAW;

    if (!(style & WS_ICONIC))
    {
        NC_AdjustRectOuter( &tmpRect, style, FALSE, exStyle );

        winRect->left   -= tmpRect.left;
        winRect->top    -= tmpRect.top;
        winRect->right  -= tmpRect.right;
        winRect->bottom -= tmpRect.bottom;

        if (((style & (WS_CHILD | WS_POPUP)) != WS_CHILD) && GetMenu(hwnd))
        {
            TRACE("Calling GetMenuBarHeight with hwnd %p, width %d, at (%d, %d).\n",
                  hwnd, winRect->right - winRect->left, -tmpRect.left, -tmpRect.top );

            winRect->top +=
                MENU_GetMenuBarHeight( hwnd,
                                       winRect->right - winRect->left,
                                       -tmpRect.left, -tmpRect.top );
        }

        if( exStyle & WS_EX_CLIENTEDGE)
            if( winRect->right - winRect->left > 2 * GetSystemMetrics(SM_CXEDGE) &&
                   winRect->bottom - winRect->top > 2 * GetSystemMetrics(SM_CYEDGE))
                InflateRect( winRect, - GetSystemMetrics(SM_CXEDGE),
                        - GetSystemMetrics(SM_CYEDGE));

        if (style & WS_VSCROLL)
            if (winRect->right - winRect->left >= GetSystemMetrics(SM_CXVSCROLL))
            {
                /* rectangle is in screen coords when wparam is false */
                if (!wparam && (exStyle & WS_EX_LAYOUTRTL)) exStyle ^= WS_EX_LEFTSCROLLBAR;

                if((exStyle & WS_EX_LEFTSCROLLBAR) != 0)
                    winRect->left  += GetSystemMetrics(SM_CXVSCROLL);
                else
                    winRect->right -= GetSystemMetrics(SM_CXVSCROLL);
            }

        if (style & WS_HSCROLL)
            if( winRect->bottom - winRect->top > GetSystemMetrics(SM_CYHSCROLL))
                    winRect->bottom -= GetSystemMetrics(SM_CYHSCROLL);

        if (winRect->top > winRect->bottom)
            winRect->bottom = winRect->top;

        if (winRect->left > winRect->right)
            winRect->right = winRect->left;
    }
    return result;
}


/***********************************************************************
 *           NC_GetInsideRect
 *
 * Get the 'inside' rectangle of a window, i.e. the whole window rectangle
 * but without the borders (if any).
 */
static void NC_GetInsideRect( HWND hwnd, enum coords_relative relative, RECT *rect,
                              DWORD style, DWORD ex_style )
{
    WIN_GetRectangles( hwnd, relative, rect, NULL );

    if (style & WS_ICONIC) return;

    /* Remove frame from rectangle */
    if (HAS_THICKFRAME( style, ex_style ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXFRAME), -GetSystemMetrics(SM_CYFRAME) );
    }
    else if (HAS_DLGFRAME( style, ex_style ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXDLGFRAME), -GetSystemMetrics(SM_CYDLGFRAME));
    }
    else if (HAS_THINFRAME( style ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER) );
    }

    /* We have additional border information if the window
     * is a child (but not an MDI child) */
    if ((style & WS_CHILD) && !(ex_style & WS_EX_MDICHILD))
    {
        if (ex_style & WS_EX_CLIENTEDGE)
            InflateRect (rect, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE));
        if (ex_style & WS_EX_STATICEDGE)
            InflateRect (rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
    }
}


/***********************************************************************
 * NC_HandleNCHitTest
 *
 * Handle a WM_NCHITTEST message. Called from DefWindowProc().
 */
LRESULT NC_HandleNCHitTest( HWND hwnd, POINT pt )
{
    RECT rect, rcClient;
    DWORD style, ex_style;

    TRACE("hwnd=%p pt=%d,%d\n", hwnd, pt.x, pt.y );

    WIN_GetRectangles( hwnd, COORDS_SCREEN, &rect, &rcClient );
    if (!PtInRect( &rect, pt )) return HTNOWHERE;

    style = GetWindowLongW( hwnd, GWL_STYLE );
    ex_style = GetWindowLongW( hwnd, GWL_EXSTYLE );
    if (style & WS_MINIMIZE) return HTCAPTION;

    if (PtInRect( &rcClient, pt )) return HTCLIENT;

    /* Check borders */
    if (HAS_THICKFRAME( style, ex_style ))
    {
        InflateRect( &rect, -GetSystemMetrics(SM_CXFRAME), -GetSystemMetrics(SM_CYFRAME) );
        if (!PtInRect( &rect, pt ))
        {
            /* Check top sizing border */
            if (pt.y < rect.top)
            {
                if (pt.x < rect.left+GetSystemMetrics(SM_CXSIZE)) return HTTOPLEFT;
                if (pt.x >= rect.right-GetSystemMetrics(SM_CXSIZE)) return HTTOPRIGHT;
                return HTTOP;
            }
            /* Check bottom sizing border */
            if (pt.y >= rect.bottom)
            {
                if (pt.x < rect.left+GetSystemMetrics(SM_CXSIZE)) return HTBOTTOMLEFT;
                if (pt.x >= rect.right-GetSystemMetrics(SM_CXSIZE)) return HTBOTTOMRIGHT;
                return HTBOTTOM;
            }
            /* Check left sizing border */
            if (pt.x < rect.left)
            {
                if (pt.y < rect.top+GetSystemMetrics(SM_CYSIZE)) return HTTOPLEFT;
                if (pt.y >= rect.bottom-GetSystemMetrics(SM_CYSIZE)) return HTBOTTOMLEFT;
                return HTLEFT;
            }
            /* Check right sizing border */
            if (pt.x >= rect.right)
            {
                if (pt.y < rect.top+GetSystemMetrics(SM_CYSIZE)) return HTTOPRIGHT;
                if (pt.y >= rect.bottom-GetSystemMetrics(SM_CYSIZE)) return HTBOTTOMRIGHT;
                return HTRIGHT;
            }
        }
    }
    else  /* No thick frame */
    {
        if (HAS_DLGFRAME( style, ex_style ))
            InflateRect(&rect, -GetSystemMetrics(SM_CXDLGFRAME), -GetSystemMetrics(SM_CYDLGFRAME));
        else if (HAS_THINFRAME( style ))
            InflateRect(&rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
        if (!PtInRect( &rect, pt )) return HTBORDER;
    }

    /* Check caption */

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (ex_style & WS_EX_TOOLWINDOW)
            rect.top += GetSystemMetrics(SM_CYSMCAPTION) - 1;
        else
            rect.top += GetSystemMetrics(SM_CYCAPTION) - 1;
        if (!PtInRect( &rect, pt ))
        {
            BOOL min_or_max_box = (style & WS_MAXIMIZEBOX) ||
                                  (style & WS_MINIMIZEBOX);
            if (ex_style & WS_EX_LAYOUTRTL)
            {
                /* Check system menu */
                if ((style & WS_SYSMENU) && !(ex_style & WS_EX_TOOLWINDOW) && NC_IconForWindow(hwnd))
                {
                    rect.right -= GetSystemMetrics(SM_CYCAPTION) - 1;
                    if (pt.x > rect.right) return HTSYSMENU;
                }

                /* Check close button */
                if (style & WS_SYSMENU)
                {
                    rect.left += GetSystemMetrics(SM_CYCAPTION);
                    if (pt.x < rect.left) return HTCLOSE;
                }

                /* Check maximize box */
                /* In win95 there is automatically a Maximize button when there is a minimize one*/
                if (min_or_max_box && !(ex_style & WS_EX_TOOLWINDOW))
                {
                    rect.left += GetSystemMetrics(SM_CXSIZE);
                    if (pt.x < rect.left) return HTMAXBUTTON;
                }

                /* Check minimize box */
                if (min_or_max_box && !(ex_style & WS_EX_TOOLWINDOW))
                {
                    rect.left += GetSystemMetrics(SM_CXSIZE);
                    if (pt.x < rect.left) return HTMINBUTTON;
                }
            }
            else
            {
                /* Check system menu */
                if ((style & WS_SYSMENU) && !(ex_style & WS_EX_TOOLWINDOW) && NC_IconForWindow(hwnd))
                {
                    rect.left += GetSystemMetrics(SM_CYCAPTION) - 1;
                    if (pt.x < rect.left) return HTSYSMENU;
                }

                /* Check close button */
                if (style & WS_SYSMENU)
                {
                    rect.right -= GetSystemMetrics(SM_CYCAPTION);
                    if (pt.x > rect.right) return HTCLOSE;
                }

                /* Check maximize box */
                /* In win95 there is automatically a Maximize button when there is a minimize one*/
                if (min_or_max_box && !(ex_style & WS_EX_TOOLWINDOW))
                {
                    rect.right -= GetSystemMetrics(SM_CXSIZE);
                    if (pt.x > rect.right) return HTMAXBUTTON;
                }

                /* Check minimize box */
                if (min_or_max_box && !(ex_style & WS_EX_TOOLWINDOW))
                {
                    rect.right -= GetSystemMetrics(SM_CXSIZE);
                    if (pt.x > rect.right) return HTMINBUTTON;
                }
            }
            return HTCAPTION;
        }
    }

      /* Check menu bar */

    if (HAS_MENU( hwnd, style ) && (pt.y < rcClient.top) &&
        (pt.x >= rcClient.left) && (pt.x < rcClient.right))
        return HTMENU;

      /* Check vertical scroll bar */

    if (ex_style & WS_EX_LAYOUTRTL) ex_style ^= WS_EX_LEFTSCROLLBAR;
    if (style & WS_VSCROLL)
    {
        if((ex_style & WS_EX_LEFTSCROLLBAR) != 0)
            rcClient.left -= GetSystemMetrics(SM_CXVSCROLL);
        else
            rcClient.right += GetSystemMetrics(SM_CXVSCROLL);
        if (PtInRect( &rcClient, pt )) return HTVSCROLL;
    }

      /* Check horizontal scroll bar */

    if (style & WS_HSCROLL)
    {
        rcClient.bottom += GetSystemMetrics(SM_CYHSCROLL);
        if (PtInRect( &rcClient, pt ))
        {
            /* Check size box */
            if ((style & WS_VSCROLL) &&
                ((((ex_style & WS_EX_LEFTSCROLLBAR) != 0) && (pt.x <= rcClient.left + GetSystemMetrics(SM_CXVSCROLL))) ||
                (((ex_style & WS_EX_LEFTSCROLLBAR) == 0) && (pt.x >= rcClient.right - GetSystemMetrics(SM_CXVSCROLL)))))
                return HTSIZE;
            return HTHSCROLL;
        }
    }

    /* Has to return HTNOWHERE if nothing was found
       Could happen when a window has a customized non client area */
    return HTNOWHERE;
}


/******************************************************************************
 *
 *   NC_DrawSysButton
 *
 *   Draws the system icon.
 *
 *****************************************************************************/
BOOL NC_DrawSysButton (HWND hwnd, HDC hdc, BOOL down)
{
    HICON hIcon = NC_IconForWindow( hwnd );

    if (hIcon)
    {
        RECT rect;
        DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
        DWORD ex_style = GetWindowLongW( hwnd, GWL_EXSTYLE );

        NC_GetInsideRect( hwnd, COORDS_WINDOW, &rect, style, ex_style );
        DrawIconEx (hdc, rect.left + 2, rect.top + 1, hIcon,
                    GetSystemMetrics(SM_CXSMICON),
                    GetSystemMetrics(SM_CYSMICON), 0, 0, DI_NORMAL);
    }
    return (hIcon != 0);
}


/******************************************************************************
 *
 *   NC_DrawCloseButton
 *
 *   Draws the close button.
 *
 *   If bGrayed is true, then draw a disabled Close button
 *
 *****************************************************************************/

static void NC_DrawCloseButton (HWND hwnd, HDC hdc, BOOL down, BOOL bGrayed)
{
    RECT rect;
    DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
    DWORD ex_style = GetWindowLongW( hwnd, GWL_EXSTYLE );

    NC_GetInsideRect( hwnd, COORDS_WINDOW, &rect, style, ex_style );

    /* A tool window has a smaller Close button */
    if (ex_style & WS_EX_TOOLWINDOW)
    {
        INT iBmpHeight = 11; /* Windows does not use SM_CXSMSIZE and SM_CYSMSIZE   */
        INT iBmpWidth = 11;  /* it uses 11x11 for  the close button in tool window */
        INT iCaptionHeight = GetSystemMetrics(SM_CYSMCAPTION);

        rect.top = rect.top + (iCaptionHeight - 1 - iBmpHeight) / 2;
        rect.left = rect.right - (iCaptionHeight + 1 + iBmpWidth) / 2;
        rect.bottom = rect.top + iBmpHeight;
        rect.right = rect.left + iBmpWidth;
    }
    else
    {
        rect.left = rect.right - GetSystemMetrics(SM_CXSIZE);
        rect.bottom = rect.top + GetSystemMetrics(SM_CYSIZE) - 2;
        rect.top += 2;
        rect.right -= 2;
    }
    DrawFrameControl( hdc, &rect, DFC_CAPTION,
                      (DFCS_CAPTIONCLOSE |
                       (down ? DFCS_PUSHED : 0) |
                       (bGrayed ? DFCS_INACTIVE : 0)) );
}

/******************************************************************************
 *   NC_DrawMaxButton
 *
 *   Draws the maximize button for windows.
 *   If bGrayed is true, then draw a disabled Maximize button
 */
static void NC_DrawMaxButton(HWND hwnd,HDC hdc,BOOL down, BOOL bGrayed)
{
    RECT rect;
    UINT flags;
    DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
    DWORD ex_style = GetWindowLongW( hwnd, GWL_EXSTYLE );

    /* never draw maximize box when window has WS_EX_TOOLWINDOW style */
    if (ex_style & WS_EX_TOOLWINDOW) return;

    flags = (style & WS_MAXIMIZE) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMAX;

    NC_GetInsideRect( hwnd, COORDS_WINDOW, &rect, style, ex_style );
    if (style & WS_SYSMENU)
        rect.right -= GetSystemMetrics(SM_CXSIZE);
    rect.left = rect.right - GetSystemMetrics(SM_CXSIZE);
    rect.bottom = rect.top + GetSystemMetrics(SM_CYSIZE) - 2;
    rect.top += 2;
    rect.right -= 2;
    if (down) flags |= DFCS_PUSHED;
    if (bGrayed) flags |= DFCS_INACTIVE;
    DrawFrameControl( hdc, &rect, DFC_CAPTION, flags );
}

/******************************************************************************
 *   NC_DrawMinButton
 *
 *   Draws the minimize button for windows.
 *   If bGrayed is true, then draw a disabled Minimize button
 */
static void  NC_DrawMinButton(HWND hwnd,HDC hdc,BOOL down, BOOL bGrayed)
{
    RECT rect;
    UINT flags = DFCS_CAPTIONMIN;
    DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
    DWORD ex_style = GetWindowLongW( hwnd, GWL_EXSTYLE );

    /* never draw minimize box when window has WS_EX_TOOLWINDOW style */
    if (ex_style & WS_EX_TOOLWINDOW) return;

    NC_GetInsideRect( hwnd, COORDS_WINDOW, &rect, style, ex_style );
    if (style & WS_SYSMENU)
        rect.right -= GetSystemMetrics(SM_CXSIZE);
    if (style & (WS_MAXIMIZEBOX|WS_MINIMIZEBOX))
        rect.right -= GetSystemMetrics(SM_CXSIZE) - 2;
    rect.left = rect.right - GetSystemMetrics(SM_CXSIZE);
    rect.bottom = rect.top + GetSystemMetrics(SM_CYSIZE) - 2;
    rect.top += 2;
    rect.right -= 2;
    if (down) flags |= DFCS_PUSHED;
    if (bGrayed) flags |= DFCS_INACTIVE;
    DrawFrameControl( hdc, &rect, DFC_CAPTION, flags );
}

/******************************************************************************
 *
 *   NC_DrawFrame
 *
 *   Draw a window frame inside the given rectangle, and update the rectangle.
 *
 *   Bugs
 *        Many.  First, just what IS a frame in Win95?  Note that the 3D look
 *        on the outer edge is handled by NC_DoNCPaint.  As is the inner
 *        edge.  The inner rectangle just inside the frame is handled by the
 *        Caption code.
 *
 *        In short, for most people, this function should be a nop (unless
 *        you LIKE thick borders in Win95/NT4.0 -- I've been working with
 *        them lately, but just to get this code right).  Even so, it doesn't
 *        appear to be so.  It's being worked on...
 *
 *****************************************************************************/

static void  NC_DrawFrame( HDC  hdc, RECT  *rect, BOOL  active, DWORD style, DWORD exStyle)
{
    INT width, height;

    /* Firstly the "thick" frame */
    if (style & WS_THICKFRAME)
    {
        width = GetSystemMetrics(SM_CXFRAME) - GetSystemMetrics(SM_CXDLGFRAME);
        height = GetSystemMetrics(SM_CYFRAME) - GetSystemMetrics(SM_CYDLGFRAME);

        SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVEBORDER :
                                            COLOR_INACTIVEBORDER) );
        /* Draw frame */
        PatBlt( hdc, rect->left, rect->top,
                  rect->right - rect->left, height, PATCOPY );
        PatBlt( hdc, rect->left, rect->top,
                  width, rect->bottom - rect->top, PATCOPY );
        PatBlt( hdc, rect->left, rect->bottom - 1,
                  rect->right - rect->left, -height, PATCOPY );
        PatBlt( hdc, rect->right - 1, rect->top,
                  -width, rect->bottom - rect->top, PATCOPY );

        InflateRect( rect, -width, -height );
    }

    /* Now the other bit of the frame */
    if ((style & (WS_BORDER|WS_DLGFRAME)) ||
        (exStyle & WS_EX_DLGMODALFRAME))
    {
        width = GetSystemMetrics(SM_CXDLGFRAME) - GetSystemMetrics(SM_CXEDGE);
        height = GetSystemMetrics(SM_CYDLGFRAME) - GetSystemMetrics(SM_CYEDGE);
        /* This should give a value of 1 that should also work for a border */

        SelectObject( hdc, GetSysColorBrush(
                      (exStyle & (WS_EX_DLGMODALFRAME|WS_EX_CLIENTEDGE)) ?
                          COLOR_3DFACE :
                      (exStyle & WS_EX_STATICEDGE) ?
                          COLOR_WINDOWFRAME :
                      (style & (WS_DLGFRAME|WS_THICKFRAME)) ?
                          COLOR_3DFACE :
                      /* else */
                          COLOR_WINDOWFRAME));

        /* Draw frame */
        PatBlt( hdc, rect->left, rect->top,
                  rect->right - rect->left, height, PATCOPY );
        PatBlt( hdc, rect->left, rect->top,
                  width, rect->bottom - rect->top, PATCOPY );
        PatBlt( hdc, rect->left, rect->bottom - 1,
                  rect->right - rect->left, -height, PATCOPY );
        PatBlt( hdc, rect->right - 1, rect->top,
                  -width, rect->bottom - rect->top, PATCOPY );

        InflateRect( rect, -width, -height );
    }
}


/******************************************************************************
 *
 *   NC_DrawCaption
 *
 *   Draw the window caption for windows.
 *   The correct pen for the window frame must be selected in the DC.
 *
 *****************************************************************************/

static void  NC_DrawCaption( HDC  hdc, RECT *rect, HWND hwnd, DWORD  style, 
                             DWORD  exStyle, BOOL active )
{
    RECT  r = *rect;
    WCHAR buffer[256];
    HPEN  hPrevPen;
    HMENU hSysMenu;
    BOOL gradient = FALSE;

    hPrevPen = SelectObject( hdc, SYSCOLOR_GetPen(
                     ((exStyle & (WS_EX_STATICEDGE|WS_EX_CLIENTEDGE|
                                 WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE) ?
                      COLOR_WINDOWFRAME : COLOR_3DFACE) );
    MoveToEx( hdc, r.left, r.bottom - 1, NULL );
    LineTo( hdc, r.right, r.bottom - 1 );
    SelectObject( hdc, hPrevPen );
    r.bottom--;

    SystemParametersInfoW (SPI_GETGRADIENTCAPTIONS, 0, &gradient, 0);
    NC_DrawCaptionBar (hdc, &r, style, active, gradient);

    if ((style & WS_SYSMENU) && !(exStyle & WS_EX_TOOLWINDOW)) {
        if (NC_DrawSysButton (hwnd, hdc, FALSE))
            r.left += GetSystemMetrics(SM_CXSMICON) + 2;
    }

    if (style & WS_SYSMENU)
    {
        UINT state;

        /* Go get the sysmenu */
        hSysMenu = GetSystemMenu(hwnd, FALSE);
        state = GetMenuState(hSysMenu, SC_CLOSE, MF_BYCOMMAND);

        /* Draw a grayed close button if disabled or if SC_CLOSE is not there */
        NC_DrawCloseButton (hwnd, hdc, FALSE,
                            (state & (MF_DISABLED | MF_GRAYED)) || (state == 0xFFFFFFFF));
        r.right -= GetSystemMetrics(SM_CYCAPTION) - 1;

        if ((style & WS_MAXIMIZEBOX) || (style & WS_MINIMIZEBOX))
        {
            /* In win95 the two buttons are always there */
            /* But if the menu item is not in the menu they're disabled*/

            NC_DrawMaxButton( hwnd, hdc, FALSE, (!(style & WS_MAXIMIZEBOX)));
            r.right -= GetSystemMetrics(SM_CXSIZE) + 1;

            NC_DrawMinButton( hwnd, hdc, FALSE,  (!(style & WS_MINIMIZEBOX)));
            r.right -= GetSystemMetrics(SM_CXSIZE) + 1;
        }
    }

    if (GetWindowTextW( hwnd, buffer, sizeof(buffer)/sizeof(WCHAR) ))
    {
        NONCLIENTMETRICSW nclm;
        HFONT hFont, hOldFont;
        nclm.cbSize = sizeof(nclm);
        SystemParametersInfoW (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
        if (exStyle & WS_EX_TOOLWINDOW)
            hFont = CreateFontIndirectW (&nclm.lfSmCaptionFont);
        else
            hFont = CreateFontIndirectW (&nclm.lfCaptionFont);
        hOldFont = SelectObject (hdc, hFont);
        if (active) SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
        else SetTextColor( hdc, GetSysColor( COLOR_INACTIVECAPTIONTEXT ) );
        SetBkMode( hdc, TRANSPARENT );
        r.left += 2;
        DrawTextW( hdc, buffer, -1, &r,
                     DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_LEFT );
        DeleteObject (SelectObject (hdc, hOldFont));
    }
}


/******************************************************************************
 *   NC_DoNCPaint
 *
 *   Paint the non-client area for windows.
 */
static void  NC_DoNCPaint( HWND  hwnd, HRGN  clip, BOOL  suppress_menupaint )
{
    HDC hdc;
    RECT rfuzz, rect, rectClip;
    BOOL active;
    WND *wndPtr;
    DWORD dwStyle, dwExStyle;
    WORD flags;
    HRGN hrgn;
    RECT rectClient;

    if (!(wndPtr = WIN_GetPtr( hwnd )) || wndPtr == WND_OTHER_PROCESS) return;
    dwStyle = wndPtr->dwStyle;
    dwExStyle = wndPtr->dwExStyle;
    flags = wndPtr->flags;
    WIN_ReleasePtr( wndPtr );

    if ( dwStyle & WS_MINIMIZE ||
         !WIN_IsWindowDrawable( hwnd, 0 )) return; /* Nothing to do */

    active  = flags & WIN_NCACTIVATED;

    TRACE("%p %d\n", hwnd, active );

    /* MSDN docs are pretty idiotic here, they say app CAN use clipRgn in
       the call to GetDCEx implying that it is allowed not to use it either.
       However, the suggested GetDCEx(    , DCX_WINDOW | DCX_INTERSECTRGN)
       will cause clipRgn to be deleted after ReleaseDC().
       Now, how is the "system" supposed to tell what happened?
     */

    WIN_GetRectangles( hwnd, COORDS_SCREEN, NULL, &rectClient );
    hrgn = CreateRectRgnIndirect( &rectClient );

    if (clip > (HRGN)1)
    {
        CombineRgn( hrgn, clip, hrgn, RGN_DIFF );
        hdc = GetDCEx( hwnd, hrgn, DCX_USESTYLE | DCX_WINDOW | DCX_INTERSECTRGN );
    }
    else
    {
        hdc = GetDCEx( hwnd, hrgn, DCX_USESTYLE | DCX_WINDOW | DCX_EXCLUDERGN );
    }

    if (!hdc) return;

    WIN_GetRectangles( hwnd, COORDS_WINDOW, &rect, NULL );
    GetClipBox( hdc, &rectClip );

    SelectObject( hdc, SYSCOLOR_GetPen(COLOR_WINDOWFRAME) );

    if (HAS_STATICOUTERFRAME(dwStyle, dwExStyle)) {
        DrawEdge (hdc, &rect, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);
    }
    else if (HAS_BIGFRAME( dwStyle, dwExStyle)) {
        DrawEdge (hdc, &rect, EDGE_RAISED, BF_RECT | BF_ADJUST);
    }

    NC_DrawFrame(hdc, &rect, active, dwStyle, dwExStyle );

    if ((dwStyle & WS_CAPTION) == WS_CAPTION)
    {
        RECT  r = rect;
        if (dwExStyle & WS_EX_TOOLWINDOW) {
            r.bottom = rect.top + GetSystemMetrics(SM_CYSMCAPTION);
            rect.top += GetSystemMetrics(SM_CYSMCAPTION);
        }
        else {
            r.bottom = rect.top + GetSystemMetrics(SM_CYCAPTION);
            rect.top += GetSystemMetrics(SM_CYCAPTION);
        }
        if( IntersectRect( &rfuzz, &r, &rectClip ) )
            NC_DrawCaption(hdc, &r, hwnd, dwStyle, dwExStyle, active);
    }

    if (HAS_MENU( hwnd, dwStyle ))
    {
	RECT r = rect;
	r.bottom = rect.top + GetSystemMetrics(SM_CYMENU);

	TRACE("Calling DrawMenuBar with rect (%s)\n", wine_dbgstr_rect(&r));

	rect.top += MENU_DrawMenuBar( hdc, &r, hwnd, suppress_menupaint ) + 1;
    }

    TRACE("After MenuBar, rect is (%s).\n", wine_dbgstr_rect(&rect));

    if (dwExStyle & WS_EX_CLIENTEDGE)
	DrawEdge (hdc, &rect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);

    /* Draw the scroll-bars */

    if (dwStyle & WS_VSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_VERT, TRUE, TRUE );
    if (dwStyle & WS_HSCROLL)
        SCROLL_DrawScrollBar( hwnd, hdc, SB_HORZ, TRUE, TRUE );

    /* Draw the "size-box" */
    if ((dwStyle & WS_VSCROLL) && (dwStyle & WS_HSCROLL))
    {
        RECT r = rect;
        if((dwExStyle & WS_EX_LEFTSCROLLBAR) != 0)
            r.right = r.left + GetSystemMetrics(SM_CXVSCROLL) + 1;
        else
            r.left = r.right - GetSystemMetrics(SM_CXVSCROLL) + 1;
        r.top  = r.bottom - GetSystemMetrics(SM_CYHSCROLL) + 1;
        FillRect( hdc, &r,  GetSysColorBrush(COLOR_SCROLLBAR) );
    }

    ReleaseDC( hwnd, hdc );
}




/***********************************************************************
 *           NC_HandleNCPaint
 *
 * Handle a WM_NCPAINT message. Called from DefWindowProc().
 */
LRESULT NC_HandleNCPaint( HWND hwnd , HRGN clip)
{
    DWORD dwStyle = GetWindowLongW( hwnd, GWL_STYLE );

    if( dwStyle & WS_VISIBLE )
    {
	if( dwStyle & WS_MINIMIZE )
	    WINPOS_RedrawIconTitle( hwnd );
	else
	    NC_DoNCPaint( hwnd, clip, FALSE );
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleNCActivate
 *
 * Handle a WM_NCACTIVATE message. Called from DefWindowProc().
 */
LRESULT NC_HandleNCActivate( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    WND* wndPtr = WIN_GetPtr( hwnd );

    if (!wndPtr || wndPtr == WND_OTHER_PROCESS) return FALSE;

    /* Lotus Notes draws menu descriptions in the caption of its main
     * window. When it wants to restore original "system" view, it just
     * sends WM_NCACTIVATE message to itself. Any optimizations here in
     * attempt to minimize redrawings lead to a not restored caption.
     */
    if (wParam) wndPtr->flags |= WIN_NCACTIVATED;
    else wndPtr->flags &= ~WIN_NCACTIVATED;
    WIN_ReleasePtr( wndPtr );

    /* This isn't documented but is reproducible in at least XP SP2 and
     * Outlook 2007 depends on it
     */
    if (lParam != -1)
    {
        if (IsIconic(hwnd))
            WINPOS_RedrawIconTitle( hwnd );
        else
            NC_DoNCPaint( hwnd, (HRGN)1, FALSE );
    }

    return TRUE;
}


/***********************************************************************
 *           NC_HandleSetCursor
 *
 * Handle a WM_SETCURSOR message. Called from DefWindowProc().
 */
LRESULT NC_HandleSetCursor( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    hwnd = WIN_GetFullHandle( (HWND)wParam );

    switch((short)LOWORD(lParam))
    {
    case HTERROR:
        {
            WORD msg = HIWORD( lParam );
            if ((msg == WM_LBUTTONDOWN) || (msg == WM_MBUTTONDOWN) ||
                (msg == WM_RBUTTONDOWN) || (msg == WM_XBUTTONDOWN))
                MessageBeep(0);
        }
        break;

    case HTCLIENT:
        {
            HCURSOR hCursor = (HCURSOR)GetClassLongPtrW(hwnd, GCLP_HCURSOR);
            if(hCursor) {
                SetCursor(hCursor);
                return TRUE;
            }
            return FALSE;
        }

    case HTLEFT:
    case HTRIGHT:
        return (LRESULT)SetCursor( LoadCursorA( 0, (LPSTR)IDC_SIZEWE ) );

    case HTTOP:
    case HTBOTTOM:
        return (LRESULT)SetCursor( LoadCursorA( 0, (LPSTR)IDC_SIZENS ) );

    case HTTOPLEFT:
    case HTBOTTOMRIGHT:
        return (LRESULT)SetCursor( LoadCursorA( 0, (LPSTR)IDC_SIZENWSE ) );

    case HTTOPRIGHT:
    case HTBOTTOMLEFT:
        return (LRESULT)SetCursor( LoadCursorA( 0, (LPSTR)IDC_SIZENESW ) );
    }

    /* Default cursor: arrow */
    return (LRESULT)SetCursor( LoadCursorA( 0, (LPSTR)IDC_ARROW ) );
}

/***********************************************************************
 *           NC_GetSysPopupPos
 */
void NC_GetSysPopupPos( HWND hwnd, RECT* rect )
{
    if (IsIconic(hwnd)) GetWindowRect( hwnd, rect );
    else
    {
        DWORD style = GetWindowLongW( hwnd, GWL_STYLE );
        DWORD ex_style = GetWindowLongW( hwnd, GWL_EXSTYLE );

        NC_GetInsideRect( hwnd, COORDS_CLIENT, rect, style, ex_style );
        rect->right = rect->left + GetSystemMetrics(SM_CYCAPTION) - 1;
        rect->bottom = rect->top + GetSystemMetrics(SM_CYCAPTION) - 1;
        MapWindowPoints( hwnd, 0, (POINT *)rect, 2 );
    }
}

/***********************************************************************
 *           NC_TrackMinMaxBox
 *
 * Track a mouse button press on the minimize or maximize box.
 *
 * The big difference between 3.1 and 95 is the disabled button state.
 * In win95 the system button can be disabled, so it can ignore the mouse
 * event.
 *
 */
static void NC_TrackMinMaxBox( HWND hwnd, WORD wParam )
{
    MSG msg;
    HDC hdc = GetWindowDC( hwnd );
    BOOL pressed = TRUE;
    UINT state;
    DWORD wndStyle = GetWindowLongW( hwnd, GWL_STYLE);
    HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);

    void  (*paintButton)(HWND, HDC, BOOL, BOOL);

    if (wParam == HTMINBUTTON)
    {
        /* If the style is not present, do nothing */
        if (!(wndStyle & WS_MINIMIZEBOX))
            return;

        /* Check if the sysmenu item for minimize is there  */
        state = GetMenuState(hSysMenu, SC_MINIMIZE, MF_BYCOMMAND);

        paintButton = NC_DrawMinButton;
    }
    else
    {
        /* If the style is not present, do nothing */
        if (!(wndStyle & WS_MAXIMIZEBOX))
            return;

        /* Check if the sysmenu item for maximize is there  */
        state = GetMenuState(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND);

        paintButton = NC_DrawMaxButton;
    }

    SetCapture( hwnd );

    (*paintButton)( hwnd, hdc, TRUE, FALSE);

    while(1)
    {
        BOOL oldstate = pressed;

        if (!GetMessageW( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST )) break;
        if (CallMsgFilterW( &msg, MSGF_MAX )) continue;

        if(msg.message == WM_LBUTTONUP)
            break;

        if(msg.message != WM_MOUSEMOVE)
            continue;

        pressed = (NC_HandleNCHitTest( hwnd, msg.pt ) == wParam);
        if (pressed != oldstate)
           (*paintButton)( hwnd, hdc, pressed, FALSE);
    }

    if(pressed)
        (*paintButton)(hwnd, hdc, FALSE, FALSE);

    ReleaseCapture();
    ReleaseDC( hwnd, hdc );

    /* If the item minimize or maximize of the sysmenu are not there */
    /* or if the style is not present, do nothing */
    if ((!pressed) || (state == 0xFFFFFFFF))
        return;

    if (wParam == HTMINBUTTON)
        SendMessageW( hwnd, WM_SYSCOMMAND, SC_MINIMIZE, MAKELONG(msg.pt.x,msg.pt.y) );
    else
        SendMessageW( hwnd, WM_SYSCOMMAND,
                      IsZoomed(hwnd) ? SC_RESTORE:SC_MAXIMIZE, MAKELONG(msg.pt.x,msg.pt.y) );
}

/***********************************************************************
 * NC_TrackCloseButton
 *
 * Track a mouse button press on the Win95 close button.
 */
static void NC_TrackCloseButton (HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    MSG msg;
    HDC hdc;
    BOOL pressed = TRUE;
    HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);
    UINT state;

    if(hSysMenu == 0)
        return;

    state = GetMenuState(hSysMenu, SC_CLOSE, MF_BYCOMMAND);

    /* If the item close of the sysmenu is disabled or not there do nothing */
    if((state & MF_DISABLED) || (state & MF_GRAYED) || (state == 0xFFFFFFFF))
        return;

    hdc = GetWindowDC( hwnd );

    SetCapture( hwnd );

    NC_DrawCloseButton (hwnd, hdc, TRUE, FALSE);

    while(1)
    {
        BOOL oldstate = pressed;

        if (!GetMessageW( &msg, 0, WM_MOUSEFIRST, WM_MOUSELAST )) break;
        if (CallMsgFilterW( &msg, MSGF_MAX )) continue;

        if(msg.message == WM_LBUTTONUP)
            break;

        if(msg.message != WM_MOUSEMOVE)
            continue;

        pressed = (NC_HandleNCHitTest( hwnd, msg.pt ) == wParam);
        if (pressed != oldstate)
           NC_DrawCloseButton (hwnd, hdc, pressed, FALSE);
    }

    if(pressed)
        NC_DrawCloseButton (hwnd, hdc, FALSE, FALSE);

    ReleaseCapture();
    ReleaseDC( hwnd, hdc );
    if (!pressed) return;

    SendMessageW( hwnd, WM_SYSCOMMAND, SC_CLOSE, lParam );
}


/***********************************************************************
 *           NC_TrackScrollBar
 *
 * Track a mouse button press on the horizontal or vertical scroll-bar.
 */
static void NC_TrackScrollBar( HWND hwnd, WPARAM wParam, POINT pt )
{
    INT scrollbar;

    if ((wParam & 0xfff0) == SC_HSCROLL)
    {
        if ((wParam & 0x0f) != HTHSCROLL) return;
	scrollbar = SB_HORZ;
    }
    else  /* SC_VSCROLL */
    {
        if ((wParam & 0x0f) != HTVSCROLL) return;
	scrollbar = SB_VERT;
    }
    SCROLL_TrackScrollBar( hwnd, scrollbar, pt );
}


/***********************************************************************
 *           NC_HandleNCLButtonDown
 *
 * Handle a WM_NCLBUTTONDOWN message. Called from DefWindowProc().
 */
LRESULT NC_HandleNCLButtonDown( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    LONG style = GetWindowLongW( hwnd, GWL_STYLE );

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
        {
            HWND top = GetAncestor( hwnd, GA_ROOT );

            if (FOCUS_MouseActivate( top ) || (GetActiveWindow() == top))
                SendMessageW( hwnd, WM_SYSCOMMAND, SC_MOVE + HTCAPTION, lParam );
            break;
        }

    case HTSYSMENU:
         if( style & WS_SYSMENU )
         {
             if( !(style & WS_MINIMIZE) )
             {
                HDC hDC = GetWindowDC(hwnd);
                NC_DrawSysButton( hwnd, hDC, TRUE );
                ReleaseDC( hwnd, hDC );
             }
             SendMessageW( hwnd, WM_SYSCOMMAND, SC_MOUSEMENU + HTSYSMENU, lParam );
         }
         break;

    case HTMENU:
        SendMessageW( hwnd, WM_SYSCOMMAND, SC_MOUSEMENU, lParam );
        break;

    case HTHSCROLL:
        SendMessageW( hwnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lParam );
        break;

    case HTVSCROLL:
        SendMessageW( hwnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lParam );
        break;

    case HTMINBUTTON:
    case HTMAXBUTTON:
        NC_TrackMinMaxBox( hwnd, wParam );
        break;

    case HTCLOSE:
        NC_TrackCloseButton (hwnd, wParam, lParam);
        break;

    case HTLEFT:
    case HTRIGHT:
    case HTTOP:
    case HTTOPLEFT:
    case HTTOPRIGHT:
    case HTBOTTOM:
    case HTBOTTOMLEFT:
    case HTBOTTOMRIGHT:
        /* Old comment:
         * "make sure hittest fits into 0xf and doesn't overlap with HTSYSMENU"
         * This was previously done by setting wParam=SC_SIZE + wParam - 2
         */
        /* But that is not what WinNT does. Instead it sends this. This
         * is easy to differentiate from HTSYSMENU, because HTSYSMENU adds
         * SC_MOUSEMENU into wParam.
         */
        SendMessageW( hwnd, WM_SYSCOMMAND, SC_SIZE + wParam - (HTLEFT-WMSZ_LEFT), lParam);
        break;

    case HTBORDER:
        break;
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleNCLButtonDblClk
 *
 * Handle a WM_NCLBUTTONDBLCLK message. Called from DefWindowProc().
 */
LRESULT NC_HandleNCLButtonDblClk( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    /*
     * if this is an icon, send a restore since we are handling
     * a double click
     */
    if (IsIconic(hwnd))
    {
        SendMessageW( hwnd, WM_SYSCOMMAND, SC_RESTORE, lParam );
        return 0;
    }

    switch(wParam)  /* Hit test */
    {
    case HTCAPTION:
        /* stop processing if WS_MAXIMIZEBOX is missing */
        if (GetWindowLongW( hwnd, GWL_STYLE ) & WS_MAXIMIZEBOX)
            SendMessageW( hwnd, WM_SYSCOMMAND,
                          IsZoomed(hwnd) ? SC_RESTORE : SC_MAXIMIZE, lParam );
        break;

    case HTSYSMENU:
        {
            HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);
            UINT state = GetMenuState(hSysMenu, SC_CLOSE, MF_BYCOMMAND);

            /* If the item close of the sysmenu is disabled or not there do nothing */
            if ((state & (MF_DISABLED | MF_GRAYED)) || (state == 0xFFFFFFFF))
                break;

            SendMessageW( hwnd, WM_SYSCOMMAND, SC_CLOSE, lParam );
            break;
        }

    case HTHSCROLL:
        SendMessageW( hwnd, WM_SYSCOMMAND, SC_HSCROLL + HTHSCROLL, lParam );
        break;

    case HTVSCROLL:
        SendMessageW( hwnd, WM_SYSCOMMAND, SC_VSCROLL + HTVSCROLL, lParam );
        break;
    }
    return 0;
}


/***********************************************************************
 *           NC_HandleSysCommand
 *
 * Handle a WM_SYSCOMMAND message. Called from DefWindowProc().
 */
LRESULT NC_HandleSysCommand( HWND hwnd, WPARAM wParam, LPARAM lParam )
{
    TRACE("hwnd %p WM_SYSCOMMAND %lx %lx\n", hwnd, wParam, lParam );

    if (!IsWindowEnabled( hwnd )) return 0;

    if (HOOK_CallHooks( WH_CBT, HCBT_SYSCOMMAND, wParam, lParam, TRUE ))
        return 0;

    if (!USER_Driver->pSysCommand( hwnd, wParam, lParam ))
        return 0;

    switch (wParam & 0xfff0)
    {
    case SC_SIZE:
    case SC_MOVE:
        WINPOS_SysCommandSizeMove( hwnd, wParam );
        break;

    case SC_MINIMIZE:
        if (hwnd == GetActiveWindow())
            ShowOwnedPopups(hwnd,FALSE);
        ShowWindow( hwnd, SW_MINIMIZE );
        break;

    case SC_MAXIMIZE:
        if (IsIconic(hwnd) && hwnd == GetActiveWindow())
            ShowOwnedPopups(hwnd,TRUE);
        ShowWindow( hwnd, SW_MAXIMIZE );
        break;

    case SC_RESTORE:
        if (IsIconic(hwnd) && hwnd == GetActiveWindow())
            ShowOwnedPopups(hwnd,TRUE);
        ShowWindow( hwnd, SW_RESTORE );
        break;

    case SC_CLOSE:
        return SendMessageW( hwnd, WM_CLOSE, 0, 0 );

    case SC_VSCROLL:
    case SC_HSCROLL:
        {
            POINT pt;
            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            NC_TrackScrollBar( hwnd, wParam, pt );
        }
        break;

    case SC_MOUSEMENU:
        {
            POINT pt;
            pt.x = (short)LOWORD(lParam);
            pt.y = (short)HIWORD(lParam);
            MENU_TrackMouseMenuBar( hwnd, wParam & 0x000F, pt );
        }
        break;

    case SC_KEYMENU:
        MENU_TrackKbdMenuBar( hwnd, wParam, (WCHAR)lParam );
        break;

    case SC_TASKLIST:
        WinExec( "taskman.exe", SW_SHOWNORMAL );
        break;

    case SC_SCREENSAVE:
        if (wParam == SC_ABOUTWINE)
        {
            HMODULE hmodule = LoadLibraryA( "shell32.dll" );
            if (hmodule)
            {
                BOOL (WINAPI *aboutproc)(HWND, LPCSTR, LPCSTR, HICON);

                aboutproc = (void *)GetProcAddress( hmodule, "ShellAboutA" );
                if (aboutproc) aboutproc( hwnd, PACKAGE_STRING, NULL, 0 );
                FreeLibrary( hmodule );
            }
        }
        break;

    case SC_HOTKEY:
    case SC_ARRANGE:
    case SC_NEXTWINDOW:
    case SC_PREVWINDOW:
        FIXME("unimplemented WM_SYSCOMMAND %04lx!\n", wParam);
        break;
    }
    return 0;
}

/***********************************************************************
 *		GetTitleBarInfo (USER32.@)
 * TODO: Handle STATE_SYSTEM_PRESSED
 */
BOOL WINAPI GetTitleBarInfo(HWND hwnd, PTITLEBARINFO tbi) {
    DWORD dwStyle;
    DWORD dwExStyle;

    TRACE("(%p %p)\n", hwnd, tbi);

    if(!tbi) {
        SetLastError(ERROR_NOACCESS);
        return FALSE;
    }

    if(tbi->cbSize != sizeof(TITLEBARINFO)) {
        TRACE("Invalid TITLEBARINFO size: %d\n", tbi->cbSize);
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    dwStyle = GetWindowLongW(hwnd, GWL_STYLE);
    dwExStyle = GetWindowLongW(hwnd, GWL_EXSTYLE);
    NC_GetInsideRect(hwnd, COORDS_SCREEN, &tbi->rcTitleBar, dwStyle, dwExStyle);

    tbi->rcTitleBar.bottom = tbi->rcTitleBar.top;
    if(dwExStyle & WS_EX_TOOLWINDOW)
        tbi->rcTitleBar.bottom += GetSystemMetrics(SM_CYSMCAPTION);
    else {
        tbi->rcTitleBar.bottom += GetSystemMetrics(SM_CYCAPTION);
        tbi->rcTitleBar.left += GetSystemMetrics(SM_CXSIZE);
    }

    ZeroMemory(tbi->rgstate, sizeof(tbi->rgstate));
    /* Does the title bar always have STATE_SYSTEM_FOCUSABLE?
     * Under XP it seems to
     */
    tbi->rgstate[0] = STATE_SYSTEM_FOCUSABLE;
    if(dwStyle & WS_CAPTION) {
        tbi->rgstate[1] = STATE_SYSTEM_INVISIBLE;
        if(dwStyle & WS_SYSMENU) {
            if(!(dwStyle & (WS_MINIMIZEBOX|WS_MAXIMIZEBOX))) {
                tbi->rgstate[2] = STATE_SYSTEM_INVISIBLE;
                tbi->rgstate[3] = STATE_SYSTEM_INVISIBLE;
            }
            else {
                if(!(dwStyle & WS_MINIMIZEBOX))
                    tbi->rgstate[2] = STATE_SYSTEM_UNAVAILABLE;
                if(!(dwStyle & WS_MAXIMIZEBOX))
                    tbi->rgstate[3] = STATE_SYSTEM_UNAVAILABLE;
            }
            if(!(dwExStyle & WS_EX_CONTEXTHELP))
                tbi->rgstate[4] = STATE_SYSTEM_INVISIBLE;
            if(GetClassLongW(hwnd, GCL_STYLE) & CS_NOCLOSE)
                tbi->rgstate[5] = STATE_SYSTEM_UNAVAILABLE;
        }
        else {
            tbi->rgstate[2] = STATE_SYSTEM_INVISIBLE;
            tbi->rgstate[3] = STATE_SYSTEM_INVISIBLE;
            tbi->rgstate[4] = STATE_SYSTEM_INVISIBLE;
            tbi->rgstate[5] = STATE_SYSTEM_INVISIBLE;
        }
    }
    else
        tbi->rgstate[0] |= STATE_SYSTEM_INVISIBLE;
    return TRUE;
}
