/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/paint/selection.c
 * PURPOSE:     Window procedure of the selection window
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

LPCTSTR cursors[9] = { IDC_SIZEALL, IDC_SIZENWSE, IDC_SIZENS, IDC_SIZENESW,
    IDC_SIZEWE, IDC_SIZEWE, IDC_SIZENESW, IDC_SIZENS, IDC_SIZENWSE
};

BOOL moving = FALSE;
int action = 0;
POINTS pos;
POINTS frac;

BOOL
ColorKeyedMaskBlt(HDC hdcDest, int nXDest, int nYDest, int nWidth, int nHeight, HDC hdcSrc, int nXSrc, int nYSrc, HBITMAP hbmMask, int xMask, int yMask, DWORD dwRop, COLORREF keyColor)
{
    HDC hTempDC;
    HDC hTempDC2;
    HBITMAP hTempBm;
    HBRUSH hTempBrush;
    HBITMAP hTempMask;
    
    hTempDC = CreateCompatibleDC(hdcSrc);
    hTempDC2 = CreateCompatibleDC(hdcSrc);
    hTempBm = CreateCompatibleBitmap(hTempDC, nWidth, nHeight);
    SelectObject(hTempDC, hTempBm);
    hTempBrush = CreateSolidBrush(keyColor);
    SelectObject(hTempDC, hTempBrush);
    BitBlt(hTempDC, 0, 0, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, SRCCOPY);
    PatBlt(hTempDC, 0, 0, nWidth, nHeight, PATINVERT);
    hTempMask = CreateBitmap(nWidth, nHeight, 1, 1, NULL);
    SelectObject(hTempDC2, hTempMask);
    BitBlt(hTempDC2, 0, 0, nWidth, nHeight, hTempDC, 0, 0, SRCCOPY);
    SelectObject(hTempDC, hbmMask);
    BitBlt(hTempDC2, 0, 0, nWidth, nHeight, hTempDC, xMask, yMask, SRCAND);
    MaskBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, hTempMask, xMask, yMask, dwRop);
    DeleteDC(hTempDC);
    DeleteDC(hTempDC2);
    DeleteObject(hTempBm);
    DeleteObject(hTempBrush);
    DeleteObject(hTempMask);
    return TRUE;
}

void
ForceRefreshSelectionContents()
{
    if (IsWindowVisible(hSelection))
    {
        SendMessage(hSelection, WM_LBUTTONDOWN, 0, MAKELPARAM(0, 0));
        SendMessage(hSelection, WM_MOUSEMOVE, 0, MAKELPARAM(0, 0));
        SendMessage(hSelection, WM_LBUTTONUP, 0, MAKELPARAM(0, 0));
    }
}

int
identifyCorner(short x, short y, short w, short h)
{
    if (y < 3)
    {
        if (x < 3)
            return 1;
        if ((x < w / 2 + 2) && (x >= w / 2 - 1))
            return 2;
        if (x >= w - 3)
            return 3;
    }
    if ((y < h / 2 + 2) && (y >= h / 2 - 1))
    {
        if (x < 3)
            return 4;
        if (x >= w - 3)
            return 5;
    }
    if (y >= h - 3)
    {
        if (x < 3)
            return 6;
        if ((x < w / 2 + 2) && (x >= w / 2 - 1))
            return 7;
        if (x >= w - 3)
            return 8;
    }
    return 0;
}

LRESULT CALLBACK
SelectionWinProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
        {
            if (!moving)
            {
                HDC hDC = GetDC(hwnd);
                DefWindowProc(hwnd, message, wParam, lParam);
                SelectionFrame(hDC, 1, 1, RECT_WIDTH(rectSel_dest) * zoom / 1000 + 5,
                               RECT_HEIGHT(rectSel_dest) * zoom / 1000 + 5);
                ReleaseDC(hwnd, hDC);
            }
            break;
        }
        case WM_LBUTTONDOWN:
            pos.x = GET_X_LPARAM(lParam);
            pos.y = GET_Y_LPARAM(lParam);
            SetCapture(hwnd);
            if (action != 0)
                SetCursor(LoadCursor(NULL, cursors[action]));
            moving = TRUE;
            break;
        case WM_MOUSEMOVE:
            if (moving)
            {
                TCHAR sizeStr[100];
                POINT delta;
                resetToU1();
                frac.x += GET_X_LPARAM(lParam) - pos.x;
                frac.y += GET_Y_LPARAM(lParam) - pos.y;
                if (zoom < 1000)
                {
                    delta.x = frac.x * 1000 / zoom;
                    frac.x = 0;
                    delta.y = frac.y * 1000 / zoom;
                    frac.y = 0;
                }
                else
                {
                    delta.x = frac.x * 1000 / zoom;
                    frac.x -= (frac.x * 1000 / zoom) * zoom / 1000;
                    delta.y = frac.y * 1000 / zoom;
                    frac.y -= (frac.y * 1000 / zoom) * zoom / 1000;
                }
                switch (action)
                {
                    /* TODO: make sure that the selection is at least 1 pixel wide and high */
                    case 0: /* move selection */
                        OffsetRect(&rectSel_dest, delta.x, delta.y);
                        break;
                    case 1: /* resize at upper left corner */
                        rectSel_dest.left += delta.x;
                        rectSel_dest.top  += delta.y;
                        break;
                    case 2: /* resize at top edge */
                        rectSel_dest.top += delta.y;
                        break;
                    case 3: /* resize at upper right corner */
                        rectSel_dest.top   += delta.y;
                        rectSel_dest.right += delta.x;
                        break;
                    case 4: /* resize at left edge */
                        rectSel_dest.left += delta.x;
                        break;
                    case 5: /* resize at right edge */
                        rectSel_dest.right += delta.x;
                        break;
                    case 6: /* resize at lower left corner */
                        rectSel_dest.left   += delta.x;
                        rectSel_dest.bottom += delta.y;
                        break;
                    case 7: /* resize at bottom edge */
                        rectSel_dest.bottom += delta.y;
                        break;
                    case 8: /* resize at lower right corner */
                        rectSel_dest.right  += delta.x;
                        rectSel_dest.bottom += delta.y;
                        break;
                }

                _stprintf(sizeStr, _T("%d x %d"), RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest));
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) sizeStr);

                if (activeTool == 10) /* text tool */
                {
                    Text(hDrawingDC, rectSel_dest.left, rectSel_dest.top, rectSel_dest.right, rectSel_dest.bottom, fgColor, bgColor, textToolText, hfontTextFont, transpBg);
                }
                else
                {
                    if (action != 0)
                        StretchBlt(hDrawingDC, rectSel_dest.left, rectSel_dest.top, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), hSelDC, 0, 0, GetDIBWidth(hSelBm), GetDIBHeight(hSelBm), SRCCOPY);
                    else
                    if (transpBg == 0)
                        MaskBlt(hDrawingDC, rectSel_dest.left, rectSel_dest.top, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest),
                                hSelDC, 0, 0, hSelMask, 0, 0, MAKEROP4(SRCCOPY, SRCAND));
                    else
                    {
                        ColorKeyedMaskBlt(hDrawingDC, rectSel_dest.left, rectSel_dest.top, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest),
                                          hSelDC, 0, 0, hSelMask, 0, 0, MAKEROP4(SRCCOPY, SRCAND), bgColor);
                    }
                }
                InvalidateRect(hImageArea, NULL, FALSE);
                pos.x = GET_X_LPARAM(lParam);
                pos.y = GET_Y_LPARAM(lParam);
                //SendMessage(hwnd, WM_PAINT, 0, 0);
            }
            else
            {
                int w = RECT_WIDTH(rectSel_dest) * zoom / 1000 + 6;
                int h = RECT_HEIGHT(rectSel_dest) * zoom / 1000 + 6;
                pos.x = GET_X_LPARAM(lParam);
                pos.y = GET_Y_LPARAM(lParam);
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) NULL);
                action = identifyCorner(pos.x, pos.y, w, h);
                if (action != 0)
                    SetCursor(LoadCursor(NULL, cursors[action]));
            }
            break;
        case WM_LBUTTONUP:
            if (moving)
            {
                moving = FALSE;
                ReleaseCapture();
                if (action != 0)
                {
                    if (activeTool == 10) /* text tool */
                    {
                        
                    }
                    else
                    {
                        HDC hTempDC;
                        HBITMAP hTempBm;
                        hTempDC = CreateCompatibleDC(hSelDC);
                        hTempBm = CreateDIBWithProperties(RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest));
                        SelectObject(hTempDC, hTempBm);
                        SelectObject(hSelDC, hSelBm);
                        StretchBlt(hTempDC, 0, 0, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), hSelDC, 0, 0,
                                   GetDIBWidth(hSelBm), GetDIBHeight(hSelBm), SRCCOPY);
                        DeleteObject(hSelBm);
                        hSelBm = hTempBm;
                        hTempBm = CreateBitmap(RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), 1, 1, NULL);
                        SelectObject(hTempDC, hTempBm);
                        SelectObject(hSelDC, hSelMask);
                        StretchBlt(hTempDC, 0, 0, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), hSelDC, 0, 0,
                                   GetDIBWidth(hSelMask), GetDIBHeight(hSelMask), SRCCOPY);
                        DeleteObject(hSelMask);
                        hSelMask = hTempBm;
                        SelectObject(hSelDC, hSelBm);
                        DeleteDC(hTempDC);
                    }
                }
                placeSelWin();
                ShowWindow(hSelection, SW_HIDE);
                ShowWindow(hSelection, SW_SHOW);
            }
            break;
        default:
            return DefWindowProc(hwnd, message, wParam, lParam);
    }

    return 0;
}
