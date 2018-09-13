/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    preview.c

Abstract:

    This module contains the code for console preview window

Author:

    Therese Stowell (thereses) Feb-3-1992 (swiped from Win3.1)

Revision History:

--*/

#include "shellprv.h"
#pragma hdrstop

#include "lnkcon.h"

/* ----- Equates ----- */

LONG CnslAspectScale( LONG n1, LONG n2, LONG m );
void CnslAspectPoint( LPCONSOLEPROP_DATA pcpd, RECT* rectPreview, POINT* pt);


VOID
UpdatePreviewRect( LPCONSOLEPROP_DATA pcpd )

/*++

    Update the global window size and dimensions

--*/

{
    POINT MinSize;
    POINT MaxSize;
    POINT WindowSize;
    PFONT_INFO lpFont;
    HMONITOR hMonitor;
    MONITORINFO mi;

    /*
     * Get the font pointer
     */
    lpFont = &pcpd->FontInfo[pcpd->CurrentFontIndex];

    /*
     * Get the window size
     */
    MinSize.x = (GetSystemMetrics(SM_CXMIN)-pcpd->NonClientSize.x) / lpFont->Size.X;
    MinSize.y = (GetSystemMetrics(SM_CYMIN)-pcpd->NonClientSize.y) / lpFont->Size.Y;
    MaxSize.x = GetSystemMetrics(SM_CXFULLSCREEN) / lpFont->Size.X;
    MaxSize.y = GetSystemMetrics(SM_CYFULLSCREEN) / lpFont->Size.Y;
    WindowSize.x = max(MinSize.x, min(MaxSize.x, pcpd->lpConsole->dwWindowSize.X));
    WindowSize.y = max(MinSize.y, min(MaxSize.y, pcpd->lpConsole->dwWindowSize.Y));

    /*
     * Get the window rectangle, making sure it's at least twice the
     * size of the non-client area.
     */
    pcpd->WindowRect.left = pcpd->lpConsole->dwWindowOrigin.X;
    pcpd->WindowRect.top = pcpd->lpConsole->dwWindowOrigin.Y;
    pcpd->WindowRect.right = WindowSize.x * lpFont->Size.X + pcpd->NonClientSize.x;
    if (pcpd->WindowRect.right < pcpd->NonClientSize.x * 2) {
        pcpd->WindowRect.right = pcpd->NonClientSize.x * 2;
    }
    pcpd->WindowRect.right += pcpd->WindowRect.left;
    pcpd->WindowRect.bottom = WindowSize.y * lpFont->Size.Y + pcpd->NonClientSize.y;
    if (pcpd->WindowRect.bottom < pcpd->NonClientSize.y * 2) {
        pcpd->WindowRect.bottom = pcpd->NonClientSize.y * 2;
    }
    pcpd->WindowRect.bottom += pcpd->WindowRect.top;

    /*
     * Get information about the monitor we're on
     */
    hMonitor = MonitorFromRect(&pcpd->WindowRect, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof(mi);
    GetMonitorInfo(hMonitor, &mi);
    pcpd->xScreen = mi.rcWork.right - mi.rcWork.left;
    pcpd->yScreen = mi.rcWork.bottom - mi.rcWork.top;

    /*
     * Convert window rectangle to monitor relative coordinates
     */
    pcpd->WindowRect.right  -= pcpd->WindowRect.left;
    pcpd->WindowRect.left   -= mi.rcWork.left;
    pcpd->WindowRect.bottom -= pcpd->WindowRect.top;
    pcpd->WindowRect.top    -= mi.rcWork.top;

    /*
     * Update the display flags
     */
    if (WindowSize.x < pcpd->lpConsole->dwScreenBufferSize.X) {
        pcpd->PreviewFlags |= PREVIEW_HSCROLL;
    } else {
        pcpd->PreviewFlags &= ~PREVIEW_HSCROLL;
    }
    if (WindowSize.y < pcpd->lpConsole->dwScreenBufferSize.Y) {
        pcpd->PreviewFlags |= PREVIEW_VSCROLL;
    } else {
        pcpd->PreviewFlags &= ~PREVIEW_VSCROLL;
    }
}


VOID
InvalidatePreviewRect(HWND hWnd, LPCONSOLEPROP_DATA pcpd)

/*++

    Invalidate the area covered by the preview "window"

--*/

{
    RECT rectWin;
    RECT rectPreview;


    /*
     * Get the size of the preview "screen"
     */
    GetClientRect(hWnd, &rectPreview);

    /*
     * Get the dimensions of the preview "window" and scale it to the
     * preview "screen"
     */
    rectWin.left   = pcpd->WindowRect.left;
    rectWin.top    = pcpd->WindowRect.top;
    rectWin.right  = pcpd->WindowRect.left + pcpd->WindowRect.right;
    rectWin.bottom = pcpd->WindowRect.top + pcpd->WindowRect.bottom;
    CnslAspectPoint( pcpd, &rectPreview, (POINT*)&rectWin.left);
    CnslAspectPoint( pcpd, &rectPreview, (POINT*)&rectWin.right);

    /*
     * Invalidate the area covered by the preview "window"
     */
    InvalidateRect(hWnd, &rectWin, FALSE);
}


VOID
PreviewPaint(
    PAINTSTRUCT* pPS,
    HWND hWnd,
    LPCONSOLEPROP_DATA pcpd
    )

/*++

    Paints the font preview.  This is called inside the paint message
    handler for the preview window

--*/

{
    RECT rectWin;
    RECT rectPreview;
    HBRUSH hbrFrame;
    HBRUSH hbrTitle;
    HBRUSH hbrOld;
    HBRUSH hbrClient;
    HBRUSH hbrBorder;
    HBRUSH hbrButton;
    HBRUSH hbrScroll;
    HBRUSH hbrDesktop;
    POINT ptButton;
    POINT ptScroll;
    HDC hDC;
    HBITMAP hBitmap;
    HBITMAP hBitmapOld;
    COLORREF rgbClient;


    /*
     * Get the size of the preview "screen"
     */
    GetClientRect(hWnd, &rectPreview);

    /*
     * Get the dimensions of the preview "window" and scale it to the
     * preview "screen"
     */
    rectWin = pcpd->WindowRect;
    CnslAspectPoint( pcpd, &rectPreview, (POINT*)&rectWin.left);
    CnslAspectPoint( pcpd, &rectPreview, (POINT*)&rectWin.right);

    /*
     * Compute the dimensions of some other window components
     */
    ptButton.x = GetSystemMetrics(SM_CXSIZE);
    ptButton.y = GetSystemMetrics(SM_CYSIZE);
    CnslAspectPoint( pcpd, &rectPreview, &ptButton);
    ptButton.y *= 2;       /* Double the computed size for "looks" */
    ptScroll.x = GetSystemMetrics(SM_CXVSCROLL);
    ptScroll.y = GetSystemMetrics(SM_CYHSCROLL);
    CnslAspectPoint( pcpd, &rectPreview, &ptScroll);

    /*
     * Create the memory device context
     */
    hDC = CreateCompatibleDC(pPS->hdc);
    hBitmap = CreateCompatibleBitmap(pPS->hdc,
                                     rectPreview.right,
                                     rectPreview.bottom);
    hBitmapOld = SelectObject(hDC, hBitmap);

    /*
     * Create the brushes
     */
    hbrBorder  = CreateSolidBrush(GetSysColor(COLOR_ACTIVEBORDER));
    hbrTitle   = CreateSolidBrush(GetSysColor(COLOR_ACTIVECAPTION));
    hbrFrame   = CreateSolidBrush(GetSysColor(COLOR_WINDOWFRAME));
    hbrButton  = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    hbrScroll  = CreateSolidBrush(GetSysColor(COLOR_SCROLLBAR));
    hbrDesktop = CreateSolidBrush(GetSysColor(COLOR_BACKGROUND));
    rgbClient  = GetNearestColor(hDC, ScreenBkColor(pcpd));
    hbrClient  = CreateSolidBrush(rgbClient);

    /*
     * Erase the clipping area
     */
    FillRect(hDC, &(pPS->rcPaint), hbrDesktop);

    /*
     * Fill in the whole window with the client brush
     */
    hbrOld = SelectObject(hDC, hbrClient);
    PatBlt(hDC, rectWin.left, rectWin.top,
           rectWin.right - 1, rectWin.bottom - 1, PATCOPY);

    /*
     * Fill in the caption bar
     */
    SelectObject(hDC, hbrTitle);
    PatBlt(hDC, rectWin.left + 3, rectWin.top + 3,
           rectWin.right - 7, ptButton.y - 2, PATCOPY);

    /*
     * Draw the "buttons"
     */
    SelectObject(hDC, hbrButton);
    PatBlt(hDC, rectWin.left + 3, rectWin.top + 3,
           ptButton.x, ptButton.y - 2, PATCOPY);
    PatBlt(hDC, rectWin.left + rectWin.right - 4 - ptButton.x,
           rectWin.top + 3,
           ptButton.x, ptButton.y - 2, PATCOPY);
    PatBlt(hDC, rectWin.left + rectWin.right - 4 - 2 * ptButton.x - 1,
           rectWin.top + 3,
           ptButton.x, ptButton.y - 2, PATCOPY);
    SelectObject(hDC, hbrFrame);
    PatBlt(hDC, rectWin.left + 3 + ptButton.x, rectWin.top + 3,
           1, ptButton.y - 2, PATCOPY);
    PatBlt(hDC, rectWin.left + rectWin.right - 4 - ptButton.x - 1,
           rectWin.top + 3,
           1, ptButton.y - 2, PATCOPY);
    PatBlt(hDC, rectWin.left + rectWin.right - 4 - 2 * ptButton.x - 2,
           rectWin.top + 3,
           1, ptButton.y - 2, PATCOPY);

    /*
     * Draw the scrollbars
     */
    SelectObject(hDC, hbrScroll);
    if (pcpd->PreviewFlags & PREVIEW_HSCROLL) {
        PatBlt(hDC, rectWin.left + 3,
               rectWin.top + rectWin.bottom - 4 - ptScroll.y,
               rectWin.right - 7, ptScroll.y, PATCOPY);
    }
    if (pcpd->PreviewFlags & PREVIEW_VSCROLL) {
        PatBlt(hDC, rectWin.left + rectWin.right - 4 - ptScroll.x,
               rectWin.top + 1 + ptButton.y + 1,
               ptScroll.x, rectWin.bottom - 6 - ptButton.y, PATCOPY);
        if (pcpd->PreviewFlags & PREVIEW_HSCROLL) {
            SelectObject(hDC, hbrFrame);
            PatBlt(hDC, rectWin.left + rectWin.right - 5 - ptScroll.x,
                   rectWin.top + rectWin.bottom - 4 - ptScroll.y,
                   1, ptScroll.y, PATCOPY);
            PatBlt(hDC, rectWin.left + rectWin.right - 4 - ptScroll.x,
                   rectWin.top + rectWin.bottom - 5 - ptScroll.y,
                   ptScroll.x, 1, PATCOPY);
        }
    }

    /*
     * Draw the interior window frame and caption frame
     */
    SelectObject(hDC, hbrFrame);
    PatBlt(hDC, rectWin.left + 2, rectWin.top + 2,
           1, rectWin.bottom - 5, PATCOPY);
    PatBlt(hDC, rectWin.left + 2, rectWin.top + 2,
           rectWin.right - 5, 1, PATCOPY);
    PatBlt(hDC, rectWin.left + 2, rectWin.top + rectWin.bottom - 4,
           rectWin.right - 5, 1, PATCOPY);
    PatBlt(hDC, rectWin.left + rectWin.right - 4, rectWin.top + 2,
           1, rectWin.bottom - 5, PATCOPY);
    PatBlt(hDC, rectWin.left + 2, rectWin.top + 1 + ptButton.y,
           rectWin.right - 5, 1, PATCOPY);

    /*
     * Draw the border
     */
    SelectObject(hDC, hbrBorder);
    PatBlt(hDC, rectWin.left + 1, rectWin.top + 1,
           1, rectWin.bottom - 3, PATCOPY);
    PatBlt(hDC, rectWin.left + 1, rectWin.top + 1,
           rectWin.right - 3, 1, PATCOPY);
    PatBlt(hDC, rectWin.left + 1, rectWin.top + rectWin.bottom - 3,
           rectWin.right - 3, 1, PATCOPY);
    PatBlt(hDC, rectWin.left + rectWin.right - 3, rectWin.top + 1,
           1, rectWin.bottom - 3, PATCOPY);

    /*
     * Draw the exterior window frame
     */
    SelectObject(hDC, hbrFrame);
    PatBlt(hDC, rectWin.left, rectWin.top,
           1, rectWin.bottom - 1, PATCOPY);
    PatBlt(hDC, rectWin.left, rectWin.top,
           rectWin.right - 1, 1, PATCOPY);
    PatBlt(hDC, rectWin.left, rectWin.top + rectWin.bottom - 2,
           rectWin.right - 1, 1, PATCOPY);
    PatBlt(hDC, rectWin.left + rectWin.right - 2, rectWin.top,
           1, rectWin.bottom - 1, PATCOPY);

    /*
     * Copy the memory device context to the screen device context
     */
    BitBlt(pPS->hdc, 0, 0, rectPreview.right, rectPreview.bottom,
           hDC, 0, 0, SRCCOPY);

    /*
     * Clean up everything
     */
    SelectObject(hDC, hbrOld);
    SelectObject(hDC, hBitmapOld);
    DeleteObject(hbrBorder);
    DeleteObject(hbrFrame);
    DeleteObject(hbrTitle);
    DeleteObject(hbrClient);
    DeleteObject(hbrButton);
    DeleteObject(hbrScroll);
    DeleteObject(hbrDesktop);
    DeleteObject(hBitmap);
    DeleteDC(hDC);
}


#define LPCS_INDEX 0
#define PCPD_INDEX sizeof(PVOID)

LRESULT
PreviewWndProc(
    HWND hWnd,
    UINT wMessage,
    WPARAM wParam,
    LPARAM lParam
    )

/*
 * PreviewWndProc
 *      Handles the preview window
 */

{
    PAINTSTRUCT ps;
    LPCREATESTRUCT lpcs;
    RECT rcWindow;
    LPCONSOLEPROP_DATA pcpd;
    int cx;
    int cy;


    switch (wMessage) {
    case WM_CREATE:
        lpcs = (LPCREATESTRUCT)LocalAlloc( LPTR, SIZEOF( CREATESTRUCT ) );
        if (lpcs)
        {
            CopyMemory( (PVOID)lpcs, (PVOID)lParam, SIZEOF( CREATESTRUCT ) );
            SetWindowLongPtr( hWnd, LPCS_INDEX, (LONG_PTR)lpcs );
        }
        else
            return 0;
        break;

    case CM_PREVIEW_INIT:

        pcpd = (LPCONSOLEPROP_DATA)lParam;
        SetWindowLongPtr( hWnd, PCPD_INDEX, (LONG_PTR)pcpd );

        /*
         * Figure out space used by non-client area
         */
        SetRect(&rcWindow, 0, 0, 50, 50);
        AdjustWindowRect(&rcWindow, WS_OVERLAPPEDWINDOW, FALSE);
        pcpd->NonClientSize.x = rcWindow.right - rcWindow.left - 50;
        pcpd->NonClientSize.y = rcWindow.bottom - rcWindow.top - 50;

        /*
         * Compute the size of the preview "window"
         */
        UpdatePreviewRect( pcpd );

        /*
         * Scale the window so it has the same aspect ratio as the screen
         */
        lpcs = (LPCREATESTRUCT)GetWindowLongPtr( hWnd, LPCS_INDEX );
        cx = lpcs->cx;
        cy = CnslAspectScale( pcpd->yScreen, pcpd->xScreen, cx);
        if (cy > lpcs->cy) {
            cy = lpcs->cy;
            cx = CnslAspectScale(pcpd->xScreen, pcpd->yScreen, cy);
        }
        MoveWindow(hWnd, lpcs->x, lpcs->y, cx, cy, TRUE);
        break;

    case WM_PAINT:
        pcpd = (LPCONSOLEPROP_DATA)GetWindowLongPtr( hWnd, PCPD_INDEX );
        BeginPaint(hWnd, &ps);
        if (pcpd)
            PreviewPaint(&ps, hWnd, pcpd);
        EndPaint(hWnd, &ps);
        break;

    case CM_PREVIEW_UPDATE:
        pcpd = (LPCONSOLEPROP_DATA)GetWindowLongPtr( hWnd, PCPD_INDEX );
        if (pcpd)
        {
            InvalidatePreviewRect(hWnd, pcpd);
            UpdatePreviewRect( pcpd );

            /*
             * Make sure the preview "screen" has the correct aspect ratio
             */
            GetWindowRect(hWnd, &rcWindow);
            cx = rcWindow.right - rcWindow.left;
            cy = CnslAspectScale(pcpd->yScreen, pcpd->xScreen, cx);
            if (cy != rcWindow.bottom - rcWindow.top) {
                SetWindowPos(hWnd, NULL, 0, 0, cx, cy, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
            }
        
            InvalidatePreviewRect(hWnd, pcpd);
        }
        break;

    case WM_DESTROY:
        lpcs = (LPCREATESTRUCT)GetWindowLongPtr( hWnd, LPCS_INDEX );
        if (lpcs)
            LocalFree( lpcs );
        break;

    default:
        return DefWindowProc(hWnd, wMessage, wParam, lParam);
    }
    return 0L;
}


/*  CnslAspectScale
 *      Performs the following calculation in LONG arithmetic to avoid
 *      overflow:
 *          return = n1 * m / n2
 *      This can be used to make an aspect ration calculation where n1/n2
 *      is the aspect ratio and m is a known value.  The return value will
 *      be the value that corresponds to m with the correct apsect ratio.
 */

LONG
CnslAspectScale(
    LONG n1,
    LONG n2,
    LONG m)
{
    LONG Temp;

    Temp = n1 * m + (n2 >> 1);
    return Temp / n2;
}

/*  CnslAspectPoint
 *      Scales a point to be preview-sized instead of screen-sized.
 */

void
CnslAspectPoint(
    LPCONSOLEPROP_DATA pcpd,
    RECT* rectPreview,
    POINT* pt
    )
{
    pt->x = CnslAspectScale(rectPreview->right, pcpd->xScreen, pt->x);
    pt->y = CnslAspectScale(rectPreview->bottom, pcpd->yScreen, pt->y);
}

