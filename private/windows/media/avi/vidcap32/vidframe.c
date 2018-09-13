/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   vidframe.c: Frame for the capture window
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

/*
 * Window class that provides a frame for the AVICAP window in the
 * VidCap capture tool. Responsible for positioning within the
 * parent window, handling scrolling and painting a size border if
 * there is room.
 */

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <vfw.h>
#include "vidcap.h"

#include "vidframe.h"

/*
 * pixels to move when asked to scroll one line or page
 */
#define LINE_SCROLL	10
#define PAGE_SCROLL	50

// class name
#define VIDFRAMECLASSNAME   "vidframeClass"


/*
 * standard brushes
 */
static HBRUSH ghbrBackground = NULL, ghbrFace, ghbrHighlight, ghbrShadow;
static BOOL   fhbrBackgroundIsSysObj;


/*
 * create brushes to be used in painting
 */
void
vidframeCreateTools(HWND hwnd)
{

    vidframeSetBrush(hwnd, gBackColour);

    ghbrHighlight  = CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT));
    ghbrShadow  = CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));
    ghbrFace  = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
}

void
vidframeDeleteTools(void)
{
    if (ghbrBackground) {
        if (!fhbrBackgroundIsSysObj) {
            DeleteObject(ghbrBackground);
            ghbrBackground = NULL;
        }
    }

    if (ghbrHighlight) {
        DeleteObject(ghbrHighlight);
        ghbrHighlight = NULL;
    }

    if (ghbrShadow) {
        DeleteObject(ghbrShadow);
        ghbrShadow = NULL;
    }

    if (ghbrFace) {
        DeleteObject(ghbrFace);
        ghbrFace = NULL;
    }
}


/*
 * change the background fill brush to be one of-
 *  IDD_PrefsDefBackground  - windows default background colour
 *  IDD_PrefsLtGrey - light grey
 *  IDD_PrefsDkGrey - dark grey
 *  IDD_PrefsBlack - black
 */
void
vidframeSetBrush(HWND hwnd, int iPref)
{
    if (ghbrBackground != NULL) {
        if (!fhbrBackgroundIsSysObj) {
            DeleteObject(ghbrBackground);
            ghbrBackground = NULL;
        }
    }

    switch(iPref) {
    case IDD_PrefsDefBackground:
        ghbrBackground = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
        fhbrBackgroundIsSysObj = FALSE;
        break;

    case IDD_PrefsLtGrey:
        ghbrBackground = GetStockObject(LTGRAY_BRUSH);
        fhbrBackgroundIsSysObj = TRUE;
        break;

    case IDD_PrefsDkGrey:
        ghbrBackground = GetStockObject(DKGRAY_BRUSH);
        fhbrBackgroundIsSysObj = TRUE;
        break;

    case IDD_PrefsBlack:
        ghbrBackground = GetStockObject(BLACK_BRUSH);
        fhbrBackgroundIsSysObj = TRUE;
        break;

    default:
        return;
    }

    if (hwnd != NULL) {
#ifdef _WIN32
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR) ghbrBackground);
#else
        SetClassWord(hwnd, GCW_HBRBACKGROUND, (WORD) ghbrBackground);
#endif
        InvalidateRect(hwnd, NULL, TRUE);
    }
}




/*
 * layout the window  - decide if we need scrollbars or
 * not, and position the avicap window correctly
 */
void
vidframeLayout(HWND hwnd, HWND hwndCap)
{
    RECT rc;
    RECT rcCap;
    CAPSTATUS cs;
    int cx, cy;
    POINT ptScroll;


    // get the x and y scroll pos so we can reset them
    ptScroll.y = GetScrollPos(hwnd, SB_VERT);
    ptScroll.x = GetScrollPos(hwnd, SB_HORZ);

    GetClientRect(hwnd, &rc);
    if (!capGetStatus(hwndCap, &cs, sizeof(cs))) {
        // no current window? - make it 0 size
        cs.uiImageWidth = 0;
        cs.uiImageHeight = 0;

    }

    SetRect(&rcCap, 0, 0, cs.uiImageWidth, cs.uiImageHeight);

    /*
     * check which scrollbars we need - note that adding and removing
     * scrollbars affects the other dimension - so recheck client rect
     */
    if (RECTWIDTH(rcCap) < RECTWIDTH(rc)) {
        // fits horz.
        SetScrollRange(hwnd, SB_HORZ, 0, 0, TRUE);
    } else {
        // need horz scrollbar
        SetScrollRange(hwnd, SB_HORZ, 0, RECTWIDTH(rcCap) - RECTWIDTH(rc), FALSE);
    }

    // get client size in case shrunk/expanded
    GetClientRect(hwnd, &rc);

    // check vert scrollbar
    if (RECTHEIGHT(rcCap) < RECTHEIGHT(rc)) {
        SetScrollRange(hwnd, SB_VERT, 0, 0, TRUE);
    } else {
        SetScrollRange(hwnd, SB_VERT, 0, RECTHEIGHT(rcCap) - RECTHEIGHT(rc), FALSE);

        // this may have caused the horz scrollbar to be unneeded
        GetClientRect(hwnd, &rc);
        if (RECTWIDTH(rcCap) < RECTWIDTH(rc)) {
            // fits horz.
            SetScrollRange(hwnd, SB_HORZ, 0, 0, TRUE);
        } else {
            // need horz scrollbar
            SetScrollRange(hwnd, SB_HORZ, 0, RECTWIDTH(rcCap) - RECTWIDTH(rc), FALSE);
        }
    }

    /*
     * be sure we don't leave any underwear showing if we have scrolled
     * back or removed the scrollbars
     */
    {
        int cmax, cmin;

        GetScrollRange(hwnd, SB_HORZ, &cmin, &cmax);
        if (ptScroll.x > cmax) {
            ptScroll.x = cmax;
        }
        GetScrollRange(hwnd, SB_VERT, &cmin, &cmax);
        if (ptScroll.y > cmax) {
            ptScroll.y = cmax;
        }
        SetScrollPos(hwnd, SB_HORZ, ptScroll.x, TRUE);
        SetScrollPos(hwnd, SB_VERT, ptScroll.y, TRUE);
        capSetScrollPos(hwndCap, &ptScroll);
    }

    // centre the window if requested and if room
    if(gbCentre) {
        GetClientRect(hwnd, &rc);
        cx = max(0, (RECTWIDTH(rc) - (int) cs.uiImageWidth)/2);
        cy = max(0, (RECTHEIGHT(rc) - (int) cs.uiImageHeight)/2);
        OffsetRect(&rcCap, cx, cy);
    }

    // DWORD align the capture window for optimal codec speed
    // during preview.  
    rc = rcCap;
    MapWindowPoints (hwnd, NULL, (LPPOINT)&rc, 1);
    cx = rc.left - (rc.left & ~3);
    OffsetRect(&rcCap, -cx, 0);

    MoveWindow(hwndCap,
            rcCap.left, rcCap.top,         
            RECTWIDTH(rcCap), RECTHEIGHT(rcCap),
            TRUE);

    InvalidateRect(hwnd, NULL, TRUE);
}

/*
 * paint the vidframe window. The fill colour is always selected as the
 * background brush, so all we need to do here is paint the
 * fancy border around the inner window if room.
 */
void
vidframePaint(HWND hwnd, HWND hwndCap)
{
    POINT ptInner;
    RECT rcCap;
    PAINTSTRUCT ps;
    HDC hdc;
    HBRUSH hbr;
    int cx, cy;

    hdc = BeginPaint(hwnd, &ps);

    /*
     * first calculate the location of the upper left corner
     * of the avicap window in vidframe-window client co-ordinates
     */
    ptInner.x = 0;
    ptInner.y = 0;
    MapWindowPoints(hwndCap, hwnd, &ptInner, 1);

    // width and height of cap window
    GetWindowRect(hwndCap, &rcCap);
    cx = RECTWIDTH(rcCap);
    cy = RECTHEIGHT(rcCap);

    // shadow lines
    hbr = SelectObject(hdc, ghbrShadow);
    PatBlt(hdc, ptInner.x-1, ptInner.y-1, cx + 1, 1, PATCOPY);
    PatBlt(hdc, ptInner.x-1, ptInner.y-1, 1, cy + 1, PATCOPY);
    PatBlt(hdc, ptInner.x + cx + 4, ptInner.y-5, 1, cy+10, PATCOPY);
    PatBlt(hdc, ptInner.x -5, ptInner.y+cy+4, cx+10, 1, PATCOPY);

    // hi-light lines
    SelectObject(hdc, ghbrHighlight);
    PatBlt(hdc, ptInner.x - 5, ptInner.y - 5, 1, cy+9, PATCOPY);
    PatBlt(hdc, ptInner.x - 5, ptInner.y - 5, cx+9, 1, PATCOPY);
    PatBlt(hdc, ptInner.x+cx, ptInner.y-1, 1, cy+2, PATCOPY);
    PatBlt(hdc, ptInner.x-1, ptInner.y+cy, cx, 1, PATCOPY);

    // fill bordered area with button face colour
    SelectObject(hdc, ghbrFace);
    PatBlt(hdc, ptInner.x-4, ptInner.y-4, cx+8, 3, PATCOPY);
    PatBlt(hdc, ptInner.x-4, ptInner.y+cy+1, cx+8, 3, PATCOPY);
    PatBlt(hdc, ptInner.x-4, ptInner.y-1, 3, cy+2, PATCOPY);
    PatBlt(hdc, ptInner.x+cx+1, ptInner.y-1, 3, cy+2, PATCOPY);

    SelectObject(hdc, hbr);

    EndPaint(hwnd, &ps);

}

/*
 * respond to a scrollbar message by moving the current scroll
 * position horizontally
 */
void
vidframeHScroll(HWND hwnd, HWND hwndCap, int code, int pos)
{
    POINT pt;
    int cmax, cmin;

    pt.x = GetScrollPos(hwnd, SB_HORZ);
    pt.y = GetScrollPos(hwnd, SB_VERT);
    GetScrollRange(hwnd, SB_HORZ, &cmin, &cmax);


    switch(code) {
    case SB_LINEUP:
        pt.x -= LINE_SCROLL;
        break;

    case SB_LINEDOWN:
        pt.x += LINE_SCROLL;
        break;

    case SB_PAGEUP:
        pt.x -= PAGE_SCROLL;
        break;

    case SB_PAGEDOWN:
        pt.x += PAGE_SCROLL;
        break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        pt.x = pos;
        break;
    }

    if (pt.x < cmin) {
        pt.x = cmin;
    } else if (pt.x > cmax) {
        pt.x = cmax;
    }
    SetScrollPos(hwnd, SB_HORZ, pt.x, TRUE);
    capSetScrollPos(hwndCap, &pt);

}


/*
 * respond to a scrollbar message by moving the current scroll
 * position vertically
 */
void
vidframeVScroll(HWND hwnd, HWND hwndCap, int code, int pos)
{
    POINT pt;
    int cmax, cmin;

    pt.x = GetScrollPos(hwnd, SB_HORZ);
    pt.y = GetScrollPos(hwnd, SB_VERT);
    GetScrollRange(hwnd, SB_VERT, &cmin, &cmax);


    switch(code) {
    case SB_LINEUP:
        pt.y -= LINE_SCROLL;
        break;

    case SB_LINEDOWN:
        pt.y += LINE_SCROLL;
        break;

    case SB_PAGEUP:
        pt.y -= PAGE_SCROLL;
        break;

    case SB_PAGEDOWN:
        pt.y += PAGE_SCROLL;
        break;

    case SB_THUMBTRACK:
    case SB_THUMBPOSITION:
        pt.y = pos;
        break;
    }

    if (pt.y < cmin) {
        pt.y = cmin;
    } else if (pt.y > cmax) {
        pt.y = cmax;
    }
    SetScrollPos(hwnd, SB_VERT, pt.y, TRUE);
    capSetScrollPos(hwndCap, &pt);
}



LRESULT FAR PASCAL 
vidframeProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch(message) {

    case WM_MOVE:
    case WM_SIZE:
        if (ghWndCap) {
            vidframeLayout(hwnd, ghWndCap);
        }
        break;

    case WM_SYSCOLORCHANGE:
        // re-get brushes - we will be sent a paint message
        vidframeDeleteTools();
        vidframeCreateTools(hwnd);
        return(TRUE);


    case WM_PALETTECHANGED:
    case WM_QUERYNEWPALETTE:
        // allow the avicap window to handle this
        if (ghWndCap) {
            return SendMessage(ghWndCap, message, wParam, lParam) ;
        }

    case WM_PAINT:
        if (ghWndCap) {
            vidframePaint(hwnd, ghWndCap);
        }
        break;

    case WM_HSCROLL:
        if (ghWndCap) {
            vidframeHScroll(hwnd, ghWndCap,
                GET_WM_HSCROLL_CODE(wParam, lParam),
                GET_WM_HSCROLL_POS(wParam, lParam)
                );
        }
        break;

    case WM_VSCROLL:
        if (ghWndCap) {
            vidframeVScroll(hwnd, ghWndCap,
                GET_WM_VSCROLL_CODE(wParam, lParam),
                GET_WM_VSCROLL_POS(wParam, lParam)
                );
        }
        break;

    case WM_DESTROY:
        vidframeDeleteTools();
        break;

    default:
        return(DefWindowProc(hwnd, message, wParam, lParam));

    }
    return(0);
}



/*
 * create a frame window and child capture window at the
 * given location. Initialise the class if this is the
 * first time through.
 *
 * returns the window handle of the frame window
 * (or NULL if failure). returns the window handle of the AVICAP window
 * via phwndCap.
 */
HWND
vidframeCreate(
    HWND hwndParent,
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    int x,
    int y,
    int cx,
    int cy,
    HWND FAR * phwndCap
)
{
    HWND hwnd, hwndCap;
    static BOOL bInitDone = FALSE;

    if (!bInitDone) {
        WNDCLASS wc;

        vidframeCreateTools(NULL);

        if (!hPrevInstance) {
            // If it's the first instance, register the window class
            wc.lpszClassName = VIDFRAMECLASSNAME;
            wc.hInstance     = hInstance;
            wc.lpfnWndProc   = vidframeProc;
            wc.hCursor       = LoadCursor(NULL, IDC_ARROW) ;
            wc.hIcon         = NULL;
            wc.lpszMenuName  = NULL;
            wc.hbrBackground = ghbrBackground;
            wc.style         = CS_HREDRAW | CS_VREDRAW ;
            wc.cbClsExtra    = 0 ;
            wc.cbWndExtra    = 0 ;   

            if(!RegisterClass(&wc)) {
                return(NULL);
            }
        }
        bInitDone = TRUE;
    }

    hwnd = CreateWindowEx(
                gfIsRTL ? WS_EX_LEFTSCROLLBAR | WS_EX_RIGHT | WS_EX_RTLREADING : 0,
                VIDFRAMECLASSNAME,
                NULL,
                WS_CHILD|WS_VISIBLE|WS_HSCROLL|WS_VSCROLL|WS_CLIPCHILDREN,
                x, y, cx, cy,
                hwndParent,
                (HMENU) 0,
                hInstance,
                NULL);

    if (hwnd == NULL) {
        return(NULL);
    }


    /*
     * create an AVICAP window within this window. Leave vidframeLayout
     * to do the layout
     */
    hwndCap = capCreateCaptureWindow(
                    NULL,
                    WS_CHILD | WS_VISIBLE,
                    0, 0, 160, 120,
                    hwnd,               // parent window
                    1                   // child window id
              );


    if (hwndCap == NULL) {
        return(NULL);
    }

    *phwndCap = hwndCap;
    return(hwnd);
}

