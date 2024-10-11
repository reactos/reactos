/*
 * Progress control
 *
 * Copyright 1997, 2002 Dimitrie O. Paun
 * Copyright 1998, 1999 Eric Kohl
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
 * TODO:
 *
 * Styles:
 *    -- PBS_SMOOTHREVERSE
 *
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "commctrl.h"
#include "comctl32.h"
#include "uxtheme.h"
#include "vssym32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(progress);

typedef struct
{
    HWND      Self;         /* The window handle for this control */
    INT       CurVal;       /* Current progress value */
    INT       MinVal;       /* Minimum progress value */
    INT       MaxVal;       /* Maximum progress value */
    INT       Step;         /* Step to use on PMB_STEPIT */
    INT       MarqueePos;   /* Marquee animation position */
    BOOL      Marquee;      /* Whether the marquee animation is enabled */
    COLORREF  ColorBar;     /* Bar color */
    COLORREF  ColorBk;      /* Background color */
    HFONT     Font;         /* Handle to font (not unused) */
} PROGRESS_INFO;

/* Control configuration constants */

#define LED_GAP           2
#define MARQUEE_LEDS      5
#define ID_MARQUEE_TIMER  1
#define DEFAULT_MARQUEE_PERIOD 30

/* Helper to obtain size of a progress bar chunk ("led"). */
static inline int get_led_size ( const PROGRESS_INFO *infoPtr, LONG style,
                                 const RECT* rect )
{
    HTHEME theme = GetWindowTheme (infoPtr->Self);
    if (theme)
    {
        int chunkSize;
        if (SUCCEEDED( GetThemeInt( theme, 0, 0, TMT_PROGRESSCHUNKSIZE, &chunkSize )))
            return chunkSize;
    }

    if (style & PBS_VERTICAL)
        return MulDiv (rect->right - rect->left, 2, 3);
    else
        return MulDiv (rect->bottom - rect->top, 2, 3);
}

/* Helper to obtain gap between progress bar chunks */
static inline int get_led_gap ( const PROGRESS_INFO *infoPtr )
{
    HTHEME theme = GetWindowTheme (infoPtr->Self);
    if (theme)
    {
        int spaceSize;
        if (SUCCEEDED( GetThemeInt( theme, 0, 0, TMT_PROGRESSSPACESIZE, &spaceSize )))
            return spaceSize;
    }

    return LED_GAP;
}

/* Get client rect. Takes into account that theming needs no adjustment. */
static inline void get_client_rect (HWND hwnd, RECT* rect)
{
    HTHEME theme = GetWindowTheme (hwnd);
    GetClientRect (hwnd, rect);
    if (!theme)
        InflateRect(rect, -1, -1);
    else
    {
        DWORD dwStyle = GetWindowLongW (hwnd, GWL_STYLE);
        int part = (dwStyle & PBS_VERTICAL) ? PP_BARVERT : PP_BAR;
        GetThemeBackgroundContentRect (theme, 0, part, 0, rect, rect);
    }
}

/* Compute the extend of the bar */
static inline int get_bar_size( LONG style, const RECT* rect )
{
    if (style & PBS_VERTICAL)
        return rect->bottom - rect->top;
    else
        return rect->right - rect->left;
}

/* Compute the pixel position of a progress value */
static inline int get_bar_position( const PROGRESS_INFO *infoPtr, LONG style,
                                    const RECT* rect, INT value )
{
    return MulDiv (value - infoPtr->MinVal, get_bar_size (style, rect),
                      infoPtr->MaxVal - infoPtr->MinVal);
}

/***********************************************************************
 * PROGRESS_Invalidate
 *
 * Don't be too clever about invalidating the progress bar.
 * InstallShield depends on this simple behaviour.
 */
static void PROGRESS_Invalidate( const PROGRESS_INFO *infoPtr, INT old, INT new )
{
    InvalidateRect( infoPtr->Self, NULL, old > new );
}

/* Information for a progress bar drawing helper */
typedef struct tagProgressDrawInfo
{
    HDC hdc;
    RECT rect;
    HBRUSH hbrBar;
    HBRUSH hbrBk;
    int ledW, ledGap;
    HTHEME theme;
    RECT bgRect;
} ProgressDrawInfo;

typedef void (*ProgressDrawProc)(const ProgressDrawInfo* di, int start, int end);

/* draw solid horizontal bar from 'start' to 'end' */
static void draw_solid_bar_H (const ProgressDrawInfo* di, int start, int end)
{
    RECT r;
    SetRect(&r, di->rect.left + start, di->rect.top, di->rect.left + end, di->rect.bottom);
    FillRect (di->hdc, &r, di->hbrBar);
}

/* draw solid horizontal background from 'start' to 'end' */
static void draw_solid_bkg_H (const ProgressDrawInfo* di, int start, int end)
{
    RECT r;
    SetRect(&r, di->rect.left + start, di->rect.top, di->rect.left + end, di->rect.bottom);
    FillRect (di->hdc, &r, di->hbrBk);
}

/* draw solid vertical bar from 'start' to 'end' */
static void draw_solid_bar_V (const ProgressDrawInfo* di, int start, int end)
{
    RECT r;
    SetRect(&r, di->rect.left, di->rect.bottom - end, di->rect.right, di->rect.bottom - start);
    FillRect (di->hdc, &r, di->hbrBar);
}

/* draw solid vertical background from 'start' to 'end' */
static void draw_solid_bkg_V (const ProgressDrawInfo* di, int start, int end)
{
    RECT r;
    SetRect(&r, di->rect.left, di->rect.bottom - end, di->rect.right, di->rect.bottom - start);
    FillRect (di->hdc, &r, di->hbrBk);
}

/* draw chunky horizontal bar from 'start' to 'end' */
static void draw_chunk_bar_H (const ProgressDrawInfo* di, int start, int end)
{
    RECT r;
    int right = di->rect.left + end;
    r.left = di->rect.left + start;
    r.top = di->rect.top;
    r.bottom = di->rect.bottom;
    while (r.left < right)
    {
        r.right = min (r.left + di->ledW, right);
        FillRect (di->hdc, &r, di->hbrBar);
        r.left = r.right;
        r.right = min (r.left + di->ledGap, right);
        FillRect (di->hdc, &r, di->hbrBk);
        r.left = r.right;
    }
}

/* draw chunky vertical bar from 'start' to 'end' */
static void draw_chunk_bar_V (const ProgressDrawInfo* di, int start, int end)
{
    RECT r;
    int top = di->rect.bottom - end;
    r.left = di->rect.left;
    r.right = di->rect.right;
    r.bottom = di->rect.bottom - start;
    while (r.bottom > top)
    {
        r.top = max (r.bottom - di->ledW, top);
        FillRect (di->hdc, &r, di->hbrBar);
        r.bottom = r.top;
        r.top = max (r.bottom - di->ledGap, top);
        FillRect (di->hdc, &r, di->hbrBk);
        r.bottom = r.top;
    }
}

/* drawing functions for "classic" style */
static const ProgressDrawProc drawProcClassic[8] = {
  /* Smooth */
    /* Horizontal */
    draw_solid_bar_H, draw_solid_bkg_H,
    /* Vertical */
    draw_solid_bar_V, draw_solid_bkg_V,
  /* Chunky */
    /* Horizontal */
    draw_chunk_bar_H, draw_solid_bkg_H,
    /* Vertical */
    draw_chunk_bar_V, draw_solid_bkg_V,
};

/* draw themed horizontal bar from 'start' to 'end' */
static void draw_theme_bar_H (const ProgressDrawInfo* di, int start, int end)
{
    RECT r;
    r.left = di->rect.left + start;
    r.top = di->rect.top;
    r.bottom = di->rect.bottom;
    r.right = di->rect.left + end;
    DrawThemeBackground (di->theme, di->hdc, PP_CHUNK, 0, &r, NULL);
}

/* draw themed vertical bar from 'start' to 'end' */
static void draw_theme_bar_V (const ProgressDrawInfo* di, int start, int end)
{
    RECT r;
    r.left = di->rect.left;
    r.right = di->rect.right;
    r.bottom = di->rect.bottom - start;
    r.top = di->rect.bottom - end;
    DrawThemeBackground (di->theme, di->hdc, PP_CHUNKVERT, 0, &r, NULL);
}

/* draw themed horizontal background from 'start' to 'end' */
static void draw_theme_bkg_H (const ProgressDrawInfo* di, int start, int end)
{
    RECT bgrect, r;

    SetRect(&r, di->rect.left + start, di->rect.top, di->rect.left + end, di->rect.bottom);
    bgrect = di->bgRect;
    OffsetRect(&bgrect, -bgrect.left, -bgrect.top);

    DrawThemeBackground (di->theme, di->hdc, PP_BAR, 0, &bgrect, &r);
}

/* draw themed vertical background from 'start' to 'end' */
static void draw_theme_bkg_V (const ProgressDrawInfo* di, int start, int end)
{
    RECT bgrect, r;

    SetRect(&r, di->rect.left, di->rect.bottom - end, di->rect.right, di->rect.bottom - start);
    bgrect = di->bgRect;
    OffsetRect(&bgrect, -bgrect.left, -bgrect.top);

    DrawThemeBackground (di->theme, di->hdc, PP_BARVERT, 0, &bgrect, &r);
}

/* drawing functions for themed style */
static const ProgressDrawProc drawProcThemed[8] = {
  /* Smooth */
    /* Horizontal */
    draw_theme_bar_H, draw_theme_bkg_H,
    /* Vertical */
    draw_theme_bar_V, draw_theme_bkg_V,
  /* Chunky */
    /* Horizontal */
    draw_theme_bar_H, draw_theme_bkg_H,
    /* Vertical */
    draw_theme_bar_V, draw_theme_bkg_V,
};

/***********************************************************************
 * PROGRESS_Draw
 * Draws the progress bar.
 */
static LRESULT PROGRESS_Draw (PROGRESS_INFO *infoPtr, HDC hdc)
{
    int barSize;
    DWORD dwStyle;
    BOOL barSmooth;
    const ProgressDrawProc* drawProcs;
    ProgressDrawInfo pdi;

    TRACE("(infoPtr=%p, hdc=%p)\n", infoPtr, hdc);

    pdi.hdc = hdc;
    pdi.theme = GetWindowTheme (infoPtr->Self);

    /* get the required bar brush */
    if (infoPtr->ColorBar == CLR_DEFAULT)
        pdi.hbrBar = GetSysColorBrush(COLOR_HIGHLIGHT);
    else
        pdi.hbrBar = CreateSolidBrush (infoPtr->ColorBar);

    if (infoPtr->ColorBk == CLR_DEFAULT)
        pdi.hbrBk = GetSysColorBrush(COLOR_3DFACE);
    else
        pdi.hbrBk = CreateSolidBrush(infoPtr->ColorBk);

    /* get the window style */
    dwStyle = GetWindowLongW (infoPtr->Self, GWL_STYLE);

    /* get client rectangle */
    GetClientRect (infoPtr->Self, &pdi.rect);
    if (!pdi.theme) {
        FrameRect( hdc, &pdi.rect, pdi.hbrBk );
        InflateRect(&pdi.rect, -1, -1);
    }
    else
    {
        RECT cntRect;
        int part = (dwStyle & PBS_VERTICAL) ? PP_BARVERT : PP_BAR;
        
        GetThemeBackgroundContentRect (pdi.theme, hdc, part, 0, &pdi.rect, 
            &cntRect);
        
        /* Exclude content rect - content background will be drawn later */
        ExcludeClipRect (hdc, cntRect.left, cntRect.top, 
            cntRect.right, cntRect.bottom);
        if (IsThemeBackgroundPartiallyTransparent (pdi.theme, part, 0))
            DrawThemeParentBackground (infoPtr->Self, hdc, NULL);
        DrawThemeBackground (pdi.theme, hdc, part, 0, &pdi.rect, NULL);
        SelectClipRgn (hdc, NULL);
        pdi.rect = cntRect;
    }

    /* compute some drawing parameters */
    barSmooth = (dwStyle & PBS_SMOOTH) && !pdi.theme;
    drawProcs = &((pdi.theme ? drawProcThemed : drawProcClassic)[(barSmooth ? 0 : 4)
        + ((dwStyle & PBS_VERTICAL) ? 2 : 0)]);
    barSize = get_bar_size( dwStyle, &pdi.rect );
    if (pdi.theme)
    {
        GetWindowRect( infoPtr->Self, &pdi.bgRect );
        MapWindowPoints( infoPtr->Self, 0, (POINT*)&pdi.bgRect, 2 );
    }

    if (!barSmooth)
        pdi.ledW = get_led_size( infoPtr, dwStyle, &pdi.rect);
    pdi.ledGap = get_led_gap( infoPtr );

    if (dwStyle & PBS_MARQUEE)
    {
        const int ledW = !barSmooth ? (pdi.ledW + pdi.ledGap) : 1;
        const int leds = (barSize + ledW - 1) / ledW;
        const int ledMEnd = infoPtr->MarqueePos + MARQUEE_LEDS;

        if (ledMEnd > leds)
        {
            /* case 1: the marquee bar extends over the end and wraps around to 
             * the start */
            const int gapStart = max((ledMEnd - leds) * ledW, 0);
            const int gapEnd = min(infoPtr->MarqueePos * ledW, barSize);

            drawProcs[0]( &pdi, 0, gapStart);
            drawProcs[1]( &pdi, gapStart, gapEnd);
            drawProcs[0]( &pdi, gapEnd, barSize);
        }
        else
        {
            /* case 2: the marquee bar is between start and end */
            const int barStart = infoPtr->MarqueePos * ledW;
            const int barEnd = min (ledMEnd * ledW, barSize);

            drawProcs[1]( &pdi, 0, barStart);
            drawProcs[0]( &pdi, barStart, barEnd);
            drawProcs[1]( &pdi, barEnd, barSize);
        }
    }
    else
    {
        int barEnd = get_bar_position( infoPtr, dwStyle, &pdi.rect,
            infoPtr->CurVal);
        if (!barSmooth)
        {
            const int ledW = pdi.ledW + pdi.ledGap;
            barEnd = min (((barEnd + ledW - 1) / ledW) * ledW, barSize);
        }
        drawProcs[0]( &pdi, 0, barEnd);
        drawProcs[1]( &pdi, barEnd, barSize);
    }

    /* delete bar brush */
    if (infoPtr->ColorBar != CLR_DEFAULT) DeleteObject (pdi.hbrBar);
    if (infoPtr->ColorBk != CLR_DEFAULT) DeleteObject (pdi.hbrBk);

    return 0;
}

/***********************************************************************
 * PROGRESS_Paint
 * Draw the progress bar. The background need not be erased.
 * If dc!=0, it draws on it
 */
static LRESULT PROGRESS_Paint (PROGRESS_INFO *infoPtr, HDC hdc)
{
    PAINTSTRUCT ps;
    if (hdc) return PROGRESS_Draw (infoPtr, hdc);
    hdc = BeginPaint (infoPtr->Self, &ps);
    PROGRESS_Draw (infoPtr, hdc);
    EndPaint (infoPtr->Self, &ps);
    return 0;
}


/***********************************************************************
 * Advance marquee progress by one step.
 */
static void PROGRESS_UpdateMarquee (PROGRESS_INFO *infoPtr)
{
    LONG style = GetWindowLongW (infoPtr->Self, GWL_STYLE);
    RECT rect;
    int ledWidth, leds;
    HTHEME theme = GetWindowTheme (infoPtr->Self);
    BOOL smooth = (style & PBS_SMOOTH) && !theme;

    get_client_rect (infoPtr->Self, &rect);

    if (smooth)
        ledWidth = 1;
    else
        ledWidth = get_led_size( infoPtr, style, &rect ) + get_led_gap( infoPtr );

    leds = (get_bar_size( style, &rect ) + ledWidth - 1) /
        ledWidth;

    /* increment the marquee progress */
    if (++infoPtr->MarqueePos >= leds)
        infoPtr->MarqueePos = 0;

    InvalidateRect(infoPtr->Self, &rect, TRUE);
    UpdateWindow(infoPtr->Self);
}


/***********************************************************************
 *           PROGRESS_CoercePos
 * Makes sure the current position (CurVal) is within bounds.
 */
static void PROGRESS_CoercePos(PROGRESS_INFO *infoPtr)
{
    if(infoPtr->CurVal < infoPtr->MinVal)
        infoPtr->CurVal = infoPtr->MinVal;
    if(infoPtr->CurVal > infoPtr->MaxVal)
        infoPtr->CurVal = infoPtr->MaxVal;
}


/***********************************************************************
 *           PROGRESS_SetFont
 * Set new Font for progress bar
 */
static HFONT PROGRESS_SetFont (PROGRESS_INFO *infoPtr, HFONT hFont, BOOL bRedraw)
{
    HFONT hOldFont = infoPtr->Font;
    infoPtr->Font = hFont;
    /* Since infoPtr->Font is not used, there is no need for repaint */
    return hOldFont;
}

static DWORD PROGRESS_SetRange (PROGRESS_INFO *infoPtr, int low, int high)
{
    DWORD res = MAKELONG(LOWORD(infoPtr->MinVal), LOWORD(infoPtr->MaxVal));

    /* if nothing changes, simply return */
    if(infoPtr->MinVal == low && infoPtr->MaxVal == high) return res;

    infoPtr->MinVal = low;
    infoPtr->MaxVal = high;
    PROGRESS_CoercePos(infoPtr);
    InvalidateRect(infoPtr->Self, NULL, TRUE);
    return res;
}

static UINT PROGRESS_SetPos (PROGRESS_INFO *infoPtr, INT pos)
{
    DWORD style = GetWindowLongW(infoPtr->Self, GWL_STYLE);

    if (style & PBS_MARQUEE)
    {
        PROGRESS_UpdateMarquee(infoPtr);
        return 1;
    }
    else
    {
        UINT oldVal;
        oldVal = infoPtr->CurVal;
        if (oldVal != pos) {
	    infoPtr->CurVal = pos;
	    PROGRESS_CoercePos(infoPtr);
	    TRACE("PBM_SETPOS: current pos changed from %d to %d\n", oldVal, infoPtr->CurVal);
            PROGRESS_Invalidate( infoPtr, oldVal, infoPtr->CurVal );
            UpdateWindow( infoPtr->Self );
        }
        return oldVal;
    }
}

/***********************************************************************
 *           ProgressWindowProc
 */
static LRESULT WINAPI ProgressWindowProc(HWND hwnd, UINT message,
                                         WPARAM wParam, LPARAM lParam)
{
    PROGRESS_INFO *infoPtr;
    static const WCHAR themeClass[] = L"Progress";
    HTHEME theme;

    TRACE("hwnd %p, msg %04x, wparam %Ix, lParam %Ix\n", hwnd, message, wParam, lParam);

    infoPtr = (PROGRESS_INFO *)GetWindowLongPtrW(hwnd, 0);

    if (!infoPtr && message != WM_CREATE)
        return DefWindowProcW( hwnd, message, wParam, lParam );

    switch(message) {
    case WM_CREATE:
    {
	DWORD dwExStyle = GetWindowLongW (hwnd, GWL_EXSTYLE);

        theme = OpenThemeData (hwnd, themeClass);

	dwExStyle &= ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE);
	if (!theme) dwExStyle |= WS_EX_STATICEDGE;
        SetWindowLongW (hwnd, GWL_EXSTYLE, dwExStyle);
	/* Force recalculation of a non-client area */
	SetWindowPos(hwnd, 0, 0, 0, 0, 0,
	    SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

        /* allocate memory for info struct */
        infoPtr = Alloc(sizeof(*infoPtr));
        if (!infoPtr) return -1;
        SetWindowLongPtrW (hwnd, 0, (DWORD_PTR)infoPtr);

        /* initialize the info struct */
        infoPtr->Self = hwnd;
        infoPtr->MinVal = 0;
        infoPtr->MaxVal = 100;
        infoPtr->CurVal = 0;
        infoPtr->Step = 10;
        infoPtr->MarqueePos = 0;
        infoPtr->Marquee = FALSE;
        infoPtr->ColorBar = CLR_DEFAULT;
        infoPtr->ColorBk = CLR_DEFAULT;
        infoPtr->Font = 0;

        TRACE("Progress Ctrl creation, hwnd=%p\n", hwnd);
        return 0;
    }

    case WM_DESTROY:
        TRACE("Progress Ctrl destruction, hwnd=%p\n", hwnd);
        Free (infoPtr);
        SetWindowLongPtrW(hwnd, 0, 0);
        theme = GetWindowTheme (hwnd);
        CloseThemeData (theme);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_GETFONT:
        return (LRESULT)infoPtr->Font;

    case WM_SETFONT:
        return (LRESULT)PROGRESS_SetFont(infoPtr, (HFONT)wParam, (BOOL)lParam);

    case WM_PRINTCLIENT:
    case WM_PAINT:
        return PROGRESS_Paint (infoPtr, (HDC)wParam);

    case WM_TIMER:
        if (wParam == ID_MARQUEE_TIMER)
            PROGRESS_UpdateMarquee (infoPtr);
        return 0;

    case WM_THEMECHANGED:
    {
        DWORD dwExStyle = GetWindowLongW (hwnd, GWL_EXSTYLE);
        
        theme = GetWindowTheme (hwnd);
        CloseThemeData (theme);
        theme = OpenThemeData (hwnd, themeClass);
        
        /* WS_EX_STATICEDGE disappears when the control is themed */
        if (theme)
            dwExStyle &= ~WS_EX_STATICEDGE;
        else
            dwExStyle |= WS_EX_STATICEDGE;
        SetWindowLongW (hwnd, GWL_EXSTYLE, dwExStyle);
        
        InvalidateRect (hwnd, NULL, TRUE);
        return 0;
    }

    case PBM_DELTAPOS:
    {
	INT oldVal;
        oldVal = infoPtr->CurVal;
        if(wParam != 0) {
	    infoPtr->CurVal += (INT)wParam;
	    PROGRESS_CoercePos (infoPtr);
	    TRACE("PBM_DELTAPOS: current pos changed from %d to %d\n", oldVal, infoPtr->CurVal);
            PROGRESS_Invalidate( infoPtr, oldVal, infoPtr->CurVal );
            UpdateWindow( infoPtr->Self );
        }
        return oldVal;
    }

    case PBM_SETPOS:
        return PROGRESS_SetPos(infoPtr, wParam);

    case PBM_SETRANGE:
        return PROGRESS_SetRange (infoPtr, (int)LOWORD(lParam), (int)HIWORD(lParam));

    case PBM_SETSTEP:
    {
	INT oldStep;
        oldStep = infoPtr->Step;
        infoPtr->Step = (INT)wParam;
        return oldStep;
    }

    case PBM_GETSTEP:
        return infoPtr->Step;

    case PBM_STEPIT:
    {
        int oldVal = infoPtr->CurVal;

        if (infoPtr->MinVal != infoPtr->MaxVal)
        {
            infoPtr->CurVal += infoPtr->Step;
            if (infoPtr->CurVal > infoPtr->MaxVal)
                infoPtr->CurVal = (infoPtr->CurVal - infoPtr->MinVal) % (infoPtr->MaxVal - infoPtr->MinVal) + infoPtr->MinVal;
            if (infoPtr->CurVal < infoPtr->MinVal)
                infoPtr->CurVal = (infoPtr->CurVal - infoPtr->MinVal) % (infoPtr->MaxVal - infoPtr->MinVal) + infoPtr->MaxVal;

            if (oldVal != infoPtr->CurVal)
            {
                TRACE("PBM_STEPIT: current pos changed from %d to %d\n", oldVal, infoPtr->CurVal);
                PROGRESS_Invalidate( infoPtr, oldVal, infoPtr->CurVal );
                UpdateWindow( infoPtr->Self );
            }
        }

        return oldVal;
    }

    case PBM_SETRANGE32:
        return PROGRESS_SetRange (infoPtr, (int)wParam, (int)lParam);

    case PBM_GETRANGE:
        if (lParam) {
            ((PPBRANGE)lParam)->iLow = infoPtr->MinVal;
            ((PPBRANGE)lParam)->iHigh = infoPtr->MaxVal;
        }
        return wParam ? infoPtr->MinVal : infoPtr->MaxVal;

    case PBM_GETPOS:
        return infoPtr->CurVal;

    case PBM_SETBARCOLOR:
    {
        COLORREF clr = infoPtr->ColorBar;

        infoPtr->ColorBar = (COLORREF)lParam;
	InvalidateRect(hwnd, NULL, TRUE);
        return clr;
    }

    case PBM_GETBARCOLOR:
	return infoPtr->ColorBar;

    case PBM_SETBKCOLOR:
    {
        COLORREF clr = infoPtr->ColorBk;

        infoPtr->ColorBk = (COLORREF)lParam;
	InvalidateRect(hwnd, NULL, TRUE);
        return clr;
    }

    case PBM_GETBKCOLOR:
	return infoPtr->ColorBk;

    case PBM_SETSTATE:
        if(wParam != PBST_NORMAL)
            FIXME("state %Ix not yet handled\n", wParam);
        return PBST_NORMAL;

    case PBM_GETSTATE:
        return PBST_NORMAL;

    case PBM_SETMARQUEE:
	if(wParam != 0)
        {
            UINT period = lParam ? (UINT)lParam : DEFAULT_MARQUEE_PERIOD;
            infoPtr->Marquee = TRUE;
            SetTimer(infoPtr->Self, ID_MARQUEE_TIMER, period, NULL);
        }
        else
        {
            infoPtr->Marquee = FALSE;
            KillTimer(infoPtr->Self, ID_MARQUEE_TIMER);
        }
	return infoPtr->Marquee;

    default:
        if ((message >= WM_USER) && (message < WM_APP) && !COMCTL32_IsReflectedMessage(message))
            ERR("unknown msg %04x, wp %Ix, lp %Ix\n", message, wParam, lParam );
        return DefWindowProcW( hwnd, message, wParam, lParam );
    }
}


/***********************************************************************
 * PROGRESS_Register [Internal]
 *
 * Registers the progress bar window class.
 */
void PROGRESS_Register (void)
{
    WNDCLASSW wndClass;

    ZeroMemory (&wndClass, sizeof(wndClass));
    wndClass.style         = CS_GLOBALCLASS | CS_VREDRAW | CS_HREDRAW;
    wndClass.lpfnWndProc   = ProgressWindowProc;
    wndClass.cbClsExtra    = 0;
    wndClass.cbWndExtra    = sizeof (PROGRESS_INFO *);
    wndClass.hCursor       = LoadCursorW (0, (LPWSTR)IDC_ARROW);
    wndClass.lpszClassName = PROGRESS_CLASSW;

    RegisterClassW (&wndClass);
}


/***********************************************************************
 * PROGRESS_Unregister [Internal]
 *
 * Unregisters the progress bar window class.
 */
void PROGRESS_Unregister (void)
{
    UnregisterClassW (PROGRESS_CLASSW, NULL);
}
