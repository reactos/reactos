/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <debug.h>

/* DEFINES ******************************************************************/

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

/* FUNCTIONS *****************************************************************/

/* Get the 'inside' rectangle of a window, i.e. the whole window rectangle
 * but without the borders (if any).
 * The rectangle is in window coordinates (for drawing with GetWindowDC()).
 */
void NC_GetInsideRect( HWND hwnd, RECT *rect )
{
    DWORD Style, ExStyle;

    GetWindowRect(hwnd, rect);
    Style = GetWindowLong(hwnd, GWL_STYLE);
    ExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);

    rect->top    = rect->left = 0;
    rect->right  = rect->right - rect->left;
    rect->bottom = rect->bottom - rect->top;

/*    if (wndPtr->dwStyle & WS_ICONIC) goto END; */

    /* Remove frame from rectangle */
    if (HAS_THICKFRAME(Style, ExStyle ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXFRAME), -GetSystemMetrics(SM_CYFRAME) );
    }
    else if (HAS_DLGFRAME(Style, ExStyle ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXDLGFRAME), -GetSystemMetrics(SM_CYDLGFRAME));
    }
    else if (HAS_THINFRAME( Style ))
    {
        InflateRect( rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER) );
    }

    /* We have additional border information if the window
     * is a child (but not an MDI child) */
    if ( (Style & WS_CHILD)  &&
         ( (ExStyle & WS_EX_MDICHILD) == 0 ) )
    {
        if (ExStyle & WS_EX_CLIENTEDGE)
            InflateRect (rect, -GetSystemMetrics(SM_CXEDGE), -GetSystemMetrics(SM_CYEDGE));
        if (ExStyle & WS_EX_STATICEDGE)
            InflateRect (rect, -GetSystemMetrics(SM_CXBORDER), -GetSystemMetrics(SM_CYBORDER));
    }

END:
    return;
}

/*  Draws the Win95 system icon. */
BOOL
NC_DrawSysButton95 (HWND hwnd, HDC hdc, BOOL down)
{
    HICON hIcon = NC_IconForWindow( hwnd );

    if (hIcon)
    {
        RECT rect;
        NC_GetInsideRect( hwnd, &rect );
        DrawIconEx (hdc, rect.left + 2, rect.top + 2, hIcon,
                    GetSystemMetrics(SM_CXSMICON),
                    GetSystemMetrics(SM_CYSMICON), 0, 0, DI_NORMAL);
    }
    return (hIcon != 0);
}

/* Draws the Win95 close button. */
static void NC_DrawCloseButton95 (HWND hwnd, HDC hdc, BOOL down, BOOL bGrayed)
{
    RECT rect;

    NC_GetInsideRect( hwnd, &rect );

    /* A tool window has a smaller Close button */
    if (GetWindowLongA( hwnd, GWL_EXSTYLE ) & WS_EX_TOOLWINDOW)
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
        rect.left = rect.right - GetSystemMetrics(SM_CXSIZE) - 1;
        rect.bottom = rect.top + GetSystemMetrics(SM_CYSIZE) - 1;
        rect.top += 2;
        rect.right -= 2;
    }
    DrawFrameControl( hdc, &rect, DFC_CAPTION,
                      (DFCS_CAPTIONCLOSE |
                       (down ? DFCS_PUSHED : 0) |
                       (bGrayed ? DFCS_INACTIVE : 0)) );
}

/*  Draws the maximize button for Win95 style windows. */
static void NC_DrawMaxButton95(HWND hwnd,HDC hdc,BOOL down, BOOL bGrayed)
{
    RECT rect;
    UINT flags = IsZoomed(hwnd) ? DFCS_CAPTIONRESTORE : DFCS_CAPTIONMAX;

    NC_GetInsideRect( hwnd, &rect );
    if (GetWindowLongA( hwnd, GWL_STYLE) & WS_SYSMENU)
        rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
    rect.left = rect.right - GetSystemMetrics(SM_CXSIZE);
    rect.bottom = rect.top + GetSystemMetrics(SM_CYSIZE) - 1;
    rect.top += 2;
    rect.right -= 2;
    if (down) flags |= DFCS_PUSHED;
    if (bGrayed) flags |= DFCS_INACTIVE;
    DrawFrameControl( hdc, &rect, DFC_CAPTION, flags );
}

/*  Draws the minimize button for Win95 style windows. */
static void  NC_DrawMinButton95(HWND hwnd,HDC hdc,BOOL down, BOOL bGrayed)
{
    RECT rect;
    UINT flags = DFCS_CAPTIONMIN;
    DWORD style = GetWindowLongA( hwnd, GWL_STYLE );

    NC_GetInsideRect( hwnd, &rect );
    if (style & WS_SYSMENU)
        rect.right -= GetSystemMetrics(SM_CXSIZE) + 1;
    if (style & (WS_MAXIMIZEBOX|WS_MINIMIZEBOX))
        rect.right -= GetSystemMetrics(SM_CXSIZE) - 2;
    rect.left = rect.right - GetSystemMetrics(SM_CXSIZE);
    rect.bottom = rect.top + GetSystemMetrics(SM_CYSIZE) - 1;
    rect.top += 2;
    rect.right -= 2;
    if (down) flags |= DFCS_PUSHED;
    if (bGrayed) flags |= DFCS_INACTIVE;
    DrawFrameControl( hdc, &rect, DFC_CAPTION, flags );
}

/* Draw a window frame inside the given rectangle, and update the rectangle. */
static void  NC_DrawFrame95(
    HDC  hdc,
    RECT  *rect,
    BOOL  active,
    DWORD style,
    DWORD exStyle)
{
    INT width, height;

    /* Firstly the "thick" frame */
    if (style & WS_THICKFRAME)
    {
	width = GetSystemMetrics(SM_CXFRAME) - GetSystemMetrics(SM_CXDLGFRAME);
	height = GetSystemMetrics(SM_CYFRAME) - GetSystemMetrics(SM_CYDLGFRAME);

/*        SelectObject( hdc, GetSysColorBrush(active ? COLOR_ACTIVEBORDER :
		      COLOR_INACTIVEBORDER) ); */
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

/*        SelectObject( hdc, GetSysColorBrush(
                      (exStyle & (WS_EX_DLGMODALFRAME|WS_EX_CLIENTEDGE)) ?
                          COLOR_3DFACE :
                      (exStyle & WS_EX_STATICEDGE) ?
                          COLOR_WINDOWFRAME :
                      (style & (WS_DLGFRAME|WS_THICKFRAME)) ?
                          COLOR_3DFACE :
                            // else
                          COLOR_WINDOWFRAME)); */

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

static HICON NC_IconForWindow( HWND hwnd )
{
    HICON hIcon = (HICON) GetClassLongA( hwnd, GCL_HICONSM );
    if (!hIcon) hIcon = (HICON) GetClassLongA( hwnd, GCL_HICON );

    /* If there is no hIcon specified and this is a modal dialog,
     * get the default one.
     */
/*    if (!hIcon && (GetWindowLongA( hwnd, GWL_STYLE ) & DS_MODALFRAME))
        hIcon = LoadImageA(0, IDI_WINLOGOA, IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR); */
    return hIcon;
}

/* Draw the window caption for Win95 style windows. The correct pen for the window frame must be selected in the DC. */
static void  NC_DrawCaption95(
    HDC  hdc,
    RECT *rect,
    HWND hwnd,
    DWORD  style,
    DWORD  exStyle,
    BOOL active )
{
    RECT  r = *rect;
    char    buffer[256];
    HPEN  hPrevPen;
    HMENU hSysMenu;

/*    hPrevPen = SelectObject( hdc, SYSCOLOR_GetPen(
                     ((exStyle & (WS_EX_STATICEDGE|WS_EX_CLIENTEDGE|
                                 WS_EX_DLGMODALFRAME)) == WS_EX_STATICEDGE) ?
                      COLOR_WINDOWFRAME : COLOR_3DFACE) ); */
    MoveToEx( hdc, r.left, r.bottom - 1, NULL );
    LineTo( hdc, r.right, r.bottom - 1 );
    SelectObject( hdc, hPrevPen );
    r.bottom--;

/*    FillRect( hdc, &r, GetSysColorBrush(active ? COLOR_ACTIVECAPTION :
					    COLOR_INACTIVECAPTION) ); */

    if ((style & WS_SYSMENU) && !(exStyle & WS_EX_TOOLWINDOW)) {
	if (NC_DrawSysButton95 (hwnd, hdc, FALSE))
	    r.left += GetSystemMetrics(SM_CYCAPTION) - 1;
    }

    if (style & WS_SYSMENU)
    {
	UINT state;

	/* Go get the sysmenu */
	hSysMenu = GetSystemMenu(hwnd, FALSE);
	state = GetMenuState(hSysMenu, SC_CLOSE, MF_BYCOMMAND);

	/* Draw a grayed close button if disabled and a normal one if SC_CLOSE is not there */
	NC_DrawCloseButton95 (hwnd, hdc, FALSE,
			      ((((state & MF_DISABLED) || (state & MF_GRAYED))) && (state != 0xFFFFFFFF)));
	r.right -= GetSystemMetrics(SM_CYCAPTION) - 1;

	if ((style & WS_MAXIMIZEBOX) || (style & WS_MINIMIZEBOX))
	{
	    /* In win95 the two buttons are always there */
	    /* But if the menu item is not in the menu they're disabled*/

	    NC_DrawMaxButton95( hwnd, hdc, FALSE, (!(style & WS_MAXIMIZEBOX)));
	    r.right -= GetSystemMetrics(SM_CXSIZE) + 1;

	    NC_DrawMinButton95( hwnd, hdc, FALSE,  (!(style & WS_MINIMIZEBOX)));
	    r.right -= GetSystemMetrics(SM_CXSIZE) + 1;
	}
    }

    if (GetWindowTextA( hwnd, buffer, sizeof(buffer) )) {
	NONCLIENTMETRICS nclm;
	HFONT hFont, hOldFont;
	nclm.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfoA (SPI_GETNONCLIENTMETRICS, 0, &nclm, 0);
	if (exStyle & WS_EX_TOOLWINDOW)
	    hFont = CreateFontIndirectA (&nclm.lfSmCaptionFont);
	else
	    hFont = CreateFontIndirectA (&nclm.lfCaptionFont);
	hOldFont = SelectObject (hdc, hFont);
/*	if (active) SetTextColor( hdc, GetSysColor( COLOR_CAPTIONTEXT ) );
	else SetTextColor( hdc, GetSysColor( COLOR_INACTIVECAPTIONTEXT ) ); */
	SetBkMode( hdc, TRANSPARENT );
	r.left += 2;
	DrawTextA( hdc, buffer, -1, &r,
		     DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_LEFT );
	DeleteObject (SelectObject (hdc, hOldFont));
    }
}

static void  NC_DoNCPaint95(
    HWND  hwnd,
    HRGN  clip,
    BOOL  suppress_menupaint)
{
    HDC hdc;
    RECT rfuzz, rect, rectClip;
    BOOL active;
    DWORD Style, ExStyle;
    WORD Flags;
    RECT ClientRect, WindowRect;
    int has_menu;

    /* FIXME: Determine has_menu */
    Style = GetWindowLong(hwnd, GWL_STYLE);
    ExStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    GetWindowRect(hwnd, &WindowRect);
    GetClientRect(hwnd, &ClientRect);

/* Flags = wndPtr->flags; */

/*    if (Style & WS_MINIMIZE ||
         !WIN_IsWindowDrawable(hwnd, 0)) return; */ /* Nothing to do */

    active  = TRUE /* Flags & WIN_NCACTIVATED */;

    DPRINT("%04x %d\n", hwnd, active);

    /* MSDN docs are pretty idiotic here, they say app CAN use clipRgn in
       the call to GetDCEx implying that it is allowed not to use it either.
       However, the suggested GetDCEx(   , DCX_WINDOW | DCX_INTERSECTRGN)
       will cause clipRgn to be deleted after ReleaseDC().
       Now, how is the "system" supposed to tell what happened?
     */

    if (!(hdc = GetDCEx(hwnd, (clip > 1) ? clip : 0, DCX_USESTYLE | DCX_WINDOW |
			      ((clip > 1) ?(DCX_INTERSECTRGN | DCX_KEEPCLIPRGN) : 0)))) return;


/*    if (ExcludeVisRect(hdc, ClientRect.left-WindowRect.left,
		       ClientRect.top-WindowRect.top,
		       ClientRect.right-WindowRect.left,
		       ClientRect.bottom-WindowRect.top)
	== NULLREGION)
    {
	ReleaseDC(hwnd, hdc);
	return;
    } */

    rect.top = rect.left = 0;
    rect.right  = WindowRect.right - WindowRect.left;
    rect.bottom = WindowRect.bottom - WindowRect.top;

    if(clip > 1)
	GetRgnBox(clip, &rectClip);
    else
    {
	clip = 0;
	rectClip = rect;
    }

/*    SelectObject(hdc, SYSCOLOR_GetPen(COLOR_WINDOWFRAME)); */

/*    if (HAS_STATICOUTERFRAME(Style, ExStyle)) {
        DrawEdge (hdc, &rect, BDR_SUNKENOUTER, BF_RECT | BF_ADJUST);
    }
    else if (HAS_BIGFRAME(Style, ExStyle)) {
        DrawEdge (hdc, &rect, EDGE_RAISED, BF_RECT | BF_ADJUST);
    } */

    NC_DrawFrame95(hdc, &rect, active, Style, ExStyle);
/*
    if ((Style & WS_CAPTION) == WS_CAPTION)
    {
        RECT  r = rect;
        if (ExStyle & WS_EX_TOOLWINDOW) {
            r.bottom = rect.top + GetSystemMetrics(SM_CYSMCAPTION);
            rect.top += GetSystemMetrics(SM_CYSMCAPTION);
        }
        else {
            r.bottom = rect.top + GetSystemMetrics(SM_CYCAPTION);
            rect.top += GetSystemMetrics(SM_CYCAPTION);
        }
        if(!clip || IntersectRect(&rfuzz, &r, &rectClip))
            NC_DrawCaption95 (hdc, &r, hwnd, Style,
                              ExStyle, active);
    }
*/
/*    if (has_menu)
    {
	RECT r = rect;
	r.bottom = rect.top + GetSystemMetrics(SM_CYMENU);

	DPRINT("Calling DrawMenuBar with rect (%d, %d)-(%d, %d)\n",
              r.left, r.top, r.right, r.bottom);

	rect.top += MENU_DrawMenuBar(hdc, &r, hwnd, suppress_menupaint) + 1;
    }
*/
    DPRINT("After MenuBar, rect is (%d, %d)-(%d, %d).\n",
          rect.left, rect.top, rect.right, rect.bottom);

    if (ExStyle & WS_EX_CLIENTEDGE)
	DrawEdge (hdc, &rect, EDGE_SUNKEN, BF_RECT | BF_ADJUST);

    /* Draw the scroll-bars */
/*
    if (Style & WS_VSCROLL)
        SCROLL_DrawScrollBar(hwnd, hdc, SB_VERT, TRUE, TRUE);
    if (Style & WS_HSCROLL)
        SCROLL_DrawScrollBar(hwnd, hdc, SB_HORZ, TRUE, TRUE);
*/
    /* Draw the "size-box" */
    if ((Style & WS_VSCROLL) && (Style & WS_HSCROLL))
    {
        RECT r = rect;
        r.left = r.right - GetSystemMetrics(SM_CXVSCROLL) + 1;
        r.top  = r.bottom - GetSystemMetrics(SM_CYHSCROLL) + 1;
/*        FillRect(hdc, &r,  GetSysColorBrush(COLOR_SCROLLBAR)); */
    }

    ReleaseDC(hwnd, hdc);
}

/* Handle a WM_NCPAINT message. Called from DefWindowProc(). */
LONG NC_HandleNCPaint(HWND hwnd , HRGN clip)
{
    DWORD Style = GetWindowLongW(hwnd, GWL_STYLE);

    if(Style & WS_VISIBLE)
    {
/*	if(Style & WS_MINIMIZE)
	    WINPOS_RedrawIconTitle(hwnd); */
/*	else if (TWEAK_WineLook == WIN31_LOOK)
	    NC_DoNCPaint(hwnd, clip, FALSE); */
/*	else */
	    NC_DoNCPaint95(hwnd, clip, FALSE);
    }
    return 0;
}
