/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/selection.cpp
 * PURPOSE:     Window procedure of the selection window
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* DEFINES **********************************************************/

#define ACTION_MOVE                 0
#define ACTION_RESIZE_TOP_LEFT      1
#define ACTION_RESIZE_TOP           2
#define ACTION_RESIZE_TOP_RIGHT     3
#define ACTION_RESIZE_LEFT          4
#define ACTION_RESIZE_RIGHT         5
#define ACTION_RESIZE_BOTTOM_LEFT   6
#define ACTION_RESIZE_BOTTOM        7
#define ACTION_RESIZE_BOTTOM_RIGHT  8

/* FUNCTIONS ********************************************************/

LPCTSTR cursors[9] = { /* action to mouse cursor lookup table */
    IDC_SIZEALL,

    IDC_SIZENWSE, IDC_SIZENS, IDC_SIZENESW,
    IDC_SIZEWE,               IDC_SIZEWE,
    IDC_SIZENESW, IDC_SIZENS, IDC_SIZENWSE
};

BOOL moving = FALSE;
int action = ACTION_MOVE;
POINTS pos;
POINTS frac;
POINT delta;
DWORD system_selection_color;

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
    if (selectionWindow.IsWindowVisible())
    {
        selectionWindow.SendMessage(WM_LBUTTONDOWN, 0, MAKELPARAM(0, 0));
        selectionWindow.SendMessage(WM_MOUSEMOVE,   0, MAKELPARAM(0, 0));
        selectionWindow.SendMessage(WM_LBUTTONUP,   0, MAKELPARAM(0, 0));
    }
}

int
identifyCorner(short x, short y, short w, short h)
{
    if (y < 3)
    {
        if (x < 3)
            return ACTION_RESIZE_TOP_LEFT;
        if ((x < w / 2 + 2) && (x >= w / 2 - 1))
            return ACTION_RESIZE_TOP;
        if (x >= w - 3)
            return ACTION_RESIZE_TOP_RIGHT;
    }
    if ((y < h / 2 + 2) && (y >= h / 2 - 1))
    {
        if (x < 3)
            return ACTION_RESIZE_LEFT;
        if (x >= w - 3)
            return ACTION_RESIZE_RIGHT;
    }
    if (y >= h - 3)
    {
        if (x < 3)
            return ACTION_RESIZE_BOTTOM_LEFT;
        if ((x < w / 2 + 2) && (x >= w / 2 - 1))
            return ACTION_RESIZE_BOTTOM;
        if (x >= w - 3)
            return ACTION_RESIZE_BOTTOM_RIGHT;
    }
    return 0;
}

LRESULT CSelectionWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!moving)
    {
        HDC hDC = GetDC();
        DefWindowProc(WM_PAINT, wParam, lParam);
        SelectionFrame(hDC, 1, 1, RECT_WIDTH(rectSel_dest) * toolsModel.GetZoom() / 1000 + 5,
                       RECT_HEIGHT(rectSel_dest) * toolsModel.GetZoom() / 1000 + 5,
                       system_selection_color);
        ReleaseDC(hDC);
    }
    return 0;
}

LRESULT CSelectionWindow::OnEraseBkgnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // do nothing => transparent background
    return 0;
}

LRESULT CSelectionWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /* update the system selection color */
    system_selection_color = GetSysColor(COLOR_HIGHLIGHT);
    SendMessage(WM_PAINT, 0, MAKELPARAM(0, 0));
    return 0;
}

LRESULT CSelectionWindow::OnSysColorChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /* update the system selection color */
    system_selection_color = GetSysColor(COLOR_HIGHLIGHT);
    SendMessage(WM_PAINT, 0, MAKELPARAM(0, 0));
    return 0;
}

LRESULT CSelectionWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetCursor(LoadCursor(NULL, IDC_SIZEALL));
    return 0;
}

LRESULT CSelectionWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    pos.x = GET_X_LPARAM(lParam);
    pos.y = GET_Y_LPARAM(lParam);
    delta.x = 0;
    delta.y = 0;
    SetCapture();
    if (action != ACTION_MOVE)
        SetCursor(LoadCursor(NULL, cursors[action]));
    moving = TRUE;
    scrlClientWindow.InvalidateRect(NULL, TRUE);
    imageArea.SendMessage(WM_PAINT, 0, 0);
    return 0;
}

LRESULT CSelectionWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (moving)
    {
        TCHAR sizeStr[100];
        POINT deltaUsed;
        imageModel.ResetToPrevious();
        frac.x += GET_X_LPARAM(lParam) - pos.x;
        frac.y += GET_Y_LPARAM(lParam) - pos.y;
        delta.x += frac.x * 1000 / toolsModel.GetZoom();
        delta.y += frac.y * 1000 / toolsModel.GetZoom();
        if (toolsModel.GetZoom() < 1000)
        {
            frac.x = 0;
            frac.y = 0;
        }
        else
        {
            frac.x -= (frac.x * 1000 / toolsModel.GetZoom()) * toolsModel.GetZoom() / 1000;
            frac.y -= (frac.y * 1000 / toolsModel.GetZoom()) * toolsModel.GetZoom() / 1000;
        }
        switch (action)
        {
            case ACTION_MOVE:                /* move selection */
                deltaUsed.x = delta.x;
                deltaUsed.y = delta.y;
                OffsetRect(&rectSel_dest, deltaUsed.x, deltaUsed.y);
                break;
            case ACTION_RESIZE_TOP_LEFT:     /* resize at upper left corner */
                deltaUsed.x = min(delta.x, RECT_WIDTH(rectSel_dest) - 1);
                deltaUsed.y = min(delta.y, RECT_HEIGHT(rectSel_dest) - 1);
                rectSel_dest.left += deltaUsed.x;
                rectSel_dest.top  += deltaUsed.y;
                break;
            case ACTION_RESIZE_TOP:          /* resize at top edge */
                deltaUsed.x = delta.x;
                deltaUsed.y = min(delta.y, RECT_HEIGHT(rectSel_dest) - 1);
                rectSel_dest.top += deltaUsed.y;
                break;
            case ACTION_RESIZE_TOP_RIGHT:    /* resize at upper right corner */
                deltaUsed.x = max(delta.x, -(RECT_WIDTH(rectSel_dest) - 1));
                deltaUsed.y = min(delta.y, RECT_HEIGHT(rectSel_dest) - 1);
                rectSel_dest.top   += deltaUsed.y;
                rectSel_dest.right += deltaUsed.x;
                break;
            case ACTION_RESIZE_LEFT:         /* resize at left edge */
                deltaUsed.x = min(delta.x, RECT_WIDTH(rectSel_dest) - 1);
                deltaUsed.y = delta.y;
                rectSel_dest.left += deltaUsed.x;
                break;
            case ACTION_RESIZE_RIGHT:        /* resize at right edge */
                deltaUsed.x = max(delta.x, -(RECT_WIDTH(rectSel_dest) - 1));
                deltaUsed.y = delta.y;
                rectSel_dest.right += deltaUsed.x;
                break;
            case ACTION_RESIZE_BOTTOM_LEFT:  /* resize at lower left corner */
                deltaUsed.x = min(delta.x, RECT_WIDTH(rectSel_dest) - 1);
                deltaUsed.y = max(delta.y, -(RECT_HEIGHT(rectSel_dest) - 1));
                rectSel_dest.left   += deltaUsed.x;
                rectSel_dest.bottom += deltaUsed.y;
                break;
            case ACTION_RESIZE_BOTTOM:       /* resize at bottom edge */
                deltaUsed.x = delta.x;
                deltaUsed.y = max(delta.y, -(RECT_HEIGHT(rectSel_dest) - 1));
                rectSel_dest.bottom += deltaUsed.y;
                break;
            case ACTION_RESIZE_BOTTOM_RIGHT: /* resize at lower right corner */
                deltaUsed.x = max(delta.x, -(RECT_WIDTH(rectSel_dest) - 1));
                deltaUsed.y = max(delta.y, -(RECT_HEIGHT(rectSel_dest) - 1));
                rectSel_dest.right  += deltaUsed.x;
                rectSel_dest.bottom += deltaUsed.y;
                break;
        }
        delta.x -= deltaUsed.x;
        delta.y -= deltaUsed.y;

        _stprintf(sizeStr, _T("%d x %d"), RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest));
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) sizeStr);

        if (toolsModel.GetActiveTool() == TOOL_TEXT)
        {
            Text(hDrawingDC, rectSel_dest.left, rectSel_dest.top, rectSel_dest.right, rectSel_dest.bottom, paletteModel.GetFgColor(), paletteModel.GetBgColor(), textToolText, hfontTextFont, toolsModel.IsBackgroundTransparent());
        }
        else
        {
            if (action != ACTION_MOVE)
                StretchBlt(hDrawingDC, rectSel_dest.left, rectSel_dest.top, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest), hSelDC, 0, 0, GetDIBWidth(hSelBm), GetDIBHeight(hSelBm), SRCCOPY);
            else
            if (toolsModel.IsBackgroundTransparent() == 0)
                MaskBlt(hDrawingDC, rectSel_dest.left, rectSel_dest.top, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest),
                        hSelDC, 0, 0, hSelMask, 0, 0, MAKEROP4(SRCCOPY, SRCAND));
            else
            {
                ColorKeyedMaskBlt(hDrawingDC, rectSel_dest.left, rectSel_dest.top, RECT_WIDTH(rectSel_dest), RECT_HEIGHT(rectSel_dest),
                                  hSelDC, 0, 0, hSelMask, 0, 0, MAKEROP4(SRCCOPY, SRCAND), paletteModel.GetBgColor());
            }
        }
        imageArea.InvalidateRect(NULL, FALSE);
        imageArea.SendMessage(WM_PAINT, 0, 0);
        pos.x = GET_X_LPARAM(lParam);
        pos.y = GET_Y_LPARAM(lParam);
    }
    else
    {
        int w = RECT_WIDTH(rectSel_dest) * toolsModel.GetZoom() / 1000 + 6;
        int h = RECT_HEIGHT(rectSel_dest) * toolsModel.GetZoom() / 1000 + 6;
        pos.x = GET_X_LPARAM(lParam);
        pos.y = GET_Y_LPARAM(lParam);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) NULL);
        action = identifyCorner(pos.x, pos.y, w, h);
        if (action != ACTION_MOVE)
            SetCursor(LoadCursor(NULL, cursors[action]));
    }
    return 0;
}

LRESULT CSelectionWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (moving)
    {
        moving = FALSE;
        ReleaseCapture();
        if (action != ACTION_MOVE)
        {
            if (toolsModel.GetActiveTool() == TOOL_TEXT)
            {
                // FIXME: What to do?
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
        ShowWindow(SW_HIDE);
        ShowWindow(SW_SHOW);
    }
    return 0;
}
