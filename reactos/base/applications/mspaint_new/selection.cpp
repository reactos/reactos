/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/selection.cpp
 * PURPOSE:     Window procedure of the selection window
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

const LPCTSTR CSelectionWindow::m_lpszCursorLUT[9] = { /* action to mouse cursor lookup table */
    IDC_SIZEALL,

    IDC_SIZENWSE, IDC_SIZENS, IDC_SIZENESW,
    IDC_SIZEWE,               IDC_SIZEWE,
    IDC_SIZENESW, IDC_SIZENS, IDC_SIZENWSE
};

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

int CSelectionWindow::IdentifyCorner(int iXPos, int iYPos, int iWidth, int iHeight)
{
    if (iYPos < 3)
    {
        if (iXPos < 3)
            return ACTION_RESIZE_TOP_LEFT;
        if ((iXPos < iWidth / 2 + 2) && (iXPos >= iWidth / 2 - 1))
            return ACTION_RESIZE_TOP;
        if (iXPos >= iWidth - 3)
            return ACTION_RESIZE_TOP_RIGHT;
    }
    if ((iYPos < iHeight / 2 + 2) && (iYPos >= iHeight / 2 - 1))
    {
        if (iXPos < 3)
            return ACTION_RESIZE_LEFT;
        if (iXPos >= iWidth - 3)
            return ACTION_RESIZE_RIGHT;
    }
    if (iYPos >= iHeight - 3)
    {
        if (iXPos < 3)
            return ACTION_RESIZE_BOTTOM_LEFT;
        if ((iXPos < iWidth / 2 + 2) && (iXPos >= iWidth / 2 - 1))
            return ACTION_RESIZE_BOTTOM;
        if (iXPos >= iWidth - 3)
            return ACTION_RESIZE_BOTTOM_RIGHT;
    }
    return 0;
}

LRESULT CSelectionWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_bMoving)
    {
        HDC hDC = GetDC();
        DefWindowProc(WM_PAINT, wParam, lParam);
        SelectionFrame(hDC, 1, 1, selectionModel.GetDestRectWidth() * toolsModel.GetZoom() / 1000 + 5,
                       selectionModel.GetDestRectHeight() * toolsModel.GetZoom() / 1000 + 5,
                       m_dwSystemSelectionColor);
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
    m_bMoving = FALSE;
    m_iAction = ACTION_MOVE;
    /* update the system selection color */
    m_dwSystemSelectionColor = GetSysColor(COLOR_HIGHLIGHT);
    SendMessage(WM_PAINT, 0, MAKELPARAM(0, 0));
    return 0;
}

LRESULT CSelectionWindow::OnSysColorChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    /* update the system selection color */
    m_dwSystemSelectionColor = GetSysColor(COLOR_HIGHLIGHT);
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
    m_ptPos.x = GET_X_LPARAM(lParam);
    m_ptPos.y = GET_Y_LPARAM(lParam);
    m_ptDelta.x = 0;
    m_ptDelta.y = 0;
    SetCapture();
    if (m_iAction != ACTION_MOVE)
        SetCursor(LoadCursor(NULL, m_lpszCursorLUT[m_iAction]));
    m_bMoving = TRUE;
    scrlClientWindow.InvalidateRect(NULL, TRUE);
    imageArea.SendMessage(WM_PAINT, 0, 0);
    return 0;
}

LRESULT CSelectionWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_bMoving)
    {
        TCHAR sizeStr[100];
        imageModel.ResetToPrevious();
        m_ptFrac.x += GET_X_LPARAM(lParam) - m_ptPos.x;
        m_ptFrac.y += GET_Y_LPARAM(lParam) - m_ptPos.y;
        m_ptDelta.x += m_ptFrac.x * 1000 / toolsModel.GetZoom();
        m_ptDelta.y += m_ptFrac.y * 1000 / toolsModel.GetZoom();
        if (toolsModel.GetZoom() < 1000)
        {
            m_ptFrac.x = 0;
            m_ptFrac.y = 0;
        }
        else
        {
            m_ptFrac.x -= (m_ptFrac.x * 1000 / toolsModel.GetZoom()) * toolsModel.GetZoom() / 1000;
            m_ptFrac.y -= (m_ptFrac.y * 1000 / toolsModel.GetZoom()) * toolsModel.GetZoom() / 1000;
        }
        selectionModel.ModifyDestRect(m_ptDelta, m_iAction);

        _stprintf(sizeStr, _T("%d x %d"), selectionModel.GetDestRectWidth(), selectionModel.GetDestRectHeight());
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) sizeStr);

        if (toolsModel.GetActiveTool() == TOOL_TEXT)
        {
            selectionModel.DrawTextToolText(hDrawingDC, paletteModel.GetFgColor(), paletteModel.GetBgColor(), toolsModel.IsBackgroundTransparent());
        }
        else
        {
            if (m_iAction != ACTION_MOVE)
                selectionModel.DrawSelectionStretched(hDrawingDC);
            else
                selectionModel.DrawSelection(hDrawingDC, paletteModel.GetBgColor(), toolsModel.IsBackgroundTransparent());
        }
        imageArea.InvalidateRect(NULL, FALSE);
        imageArea.SendMessage(WM_PAINT, 0, 0);
        m_ptPos.x = GET_X_LPARAM(lParam);
        m_ptPos.y = GET_Y_LPARAM(lParam);
    }
    else
    {
        int w = selectionModel.GetDestRectWidth() * toolsModel.GetZoom() / 1000 + 6;
        int h = selectionModel.GetDestRectHeight() * toolsModel.GetZoom() / 1000 + 6;
        m_ptPos.x = GET_X_LPARAM(lParam);
        m_ptPos.y = GET_Y_LPARAM(lParam);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) NULL);
        m_iAction = IdentifyCorner(m_ptPos.x, m_ptPos.y, w, h);
        if (m_iAction != ACTION_MOVE)
            SetCursor(LoadCursor(NULL, m_lpszCursorLUT[m_iAction]));
    }
    return 0;
}

LRESULT CSelectionWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_bMoving)
    {
        m_bMoving = FALSE;
        ReleaseCapture();
        if (m_iAction != ACTION_MOVE)
        {
            if (toolsModel.GetActiveTool() == TOOL_TEXT)
            {
                // FIXME: What to do?
            }
            else
            {
                selectionModel.ScaleContentsToFit();
            }
        }
        placeSelWin();
        ShowWindow(SW_HIDE);
        ShowWindow(SW_SHOW);
    }
    return 0;
}

LRESULT CSelectionWindow::OnPaletteModelColorChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (toolsModel.GetActiveTool() == TOOL_TEXT)
        ForceRefreshSelectionContents();
    return 0;
}

LRESULT CSelectionWindow::OnToolsModelSettingsChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (toolsModel.GetActiveTool() == TOOL_FREESEL ||
        toolsModel.GetActiveTool() == TOOL_RECTSEL ||
        toolsModel.GetActiveTool() == TOOL_TEXT)
        ForceRefreshSelectionContents();
    return 0;
}

LRESULT CSelectionWindow::OnSelectionModelRefreshNeeded(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ForceRefreshSelectionContents();
    return 0;
}
