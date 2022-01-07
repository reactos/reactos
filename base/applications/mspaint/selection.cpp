/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/selection.cpp
 * PURPOSE:     Window procedure of the selection window
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
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
        imageModel.ResetToPrevious();
        imageModel.DrawSelectionBackground();
        selectionModel.DrawSelection(imageModel.GetDC(), paletteModel.GetBgColor(), toolsModel.IsBackgroundTransparent());
    }
}

int CSelectionWindow::IdentifyCorner(int iXPos, int iYPos, int iWidth, int iHeight)
{
    POINT pt = { iXPos, iYPos };
    HWND hwndChild = ChildWindowFromPointEx(pt, CWP_SKIPINVISIBLE | CWP_SKIPDISABLED);
    if (hwndChild == sizeboxLeftTop)
        return ACTION_RESIZE_TOP_LEFT;
    if (hwndChild == sizeboxCenterTop)
        return ACTION_RESIZE_TOP;
    if (hwndChild == sizeboxRightTop)
        return ACTION_RESIZE_TOP_RIGHT;
    if (hwndChild == sizeboxRightCenter)
        return ACTION_RESIZE_RIGHT;
    if (hwndChild == sizeboxLeftCenter)
        return ACTION_RESIZE_LEFT;
    if (hwndChild == sizeboxCenterBottom)
        return ACTION_RESIZE_BOTTOM;
    if (hwndChild == sizeboxRightBottom)
        return ACTION_RESIZE_BOTTOM_RIGHT;
    if (hwndChild == sizeboxLeftBottom)
        return ACTION_RESIZE_BOTTOM_LEFT;
    return 0;
}

LRESULT CSelectionWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_bMoving)
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(&ps);
        SelectionFrame(hDC, 1, 1, Zoomed(selectionModel.GetDestRectWidth()) + 5,
                       Zoomed(selectionModel.GetDestRectHeight()) + 5,
                       GetSysColor(COLOR_HIGHLIGHT));
        EndPaint(&ps);
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
    Invalidate();
    return 0;
}

LRESULT CSelectionWindow::OnSysColorChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Invalidate();
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
    m_ptDelta.x = m_ptDelta.y = 0;
    SetCapture();
    if (m_iAction != ACTION_MOVE)
        SetCursor(LoadCursor(NULL, m_lpszCursorLUT[m_iAction]));
    m_bMoving = TRUE;
    scrlClientWindow.InvalidateRect(NULL, FALSE);
    scrlClientWindow.SendMessage(WM_PAINT, 0, 0);
    imageArea.Invalidate(FALSE);
    imageArea.SendMessage(WM_PAINT, 0, 0);
    return 0;
}

LRESULT CSelectionWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_bMoving)
    {
        imageModel.ResetToPrevious();
        imageModel.DrawSelectionBackground();

        m_ptFrac.x += GET_X_LPARAM(lParam) - m_ptPos.x;
        m_ptFrac.y += GET_Y_LPARAM(lParam) - m_ptPos.y;
        m_ptDelta.x += UnZoomed(m_ptFrac.x);
        m_ptDelta.y += UnZoomed(m_ptFrac.y);
        if (toolsModel.GetZoom() < 1000)
        {
            m_ptFrac.x = m_ptFrac.y = 0;
        }
        else
        {
            m_ptFrac.x -= Zoomed(UnZoomed(m_ptFrac.x));
            m_ptFrac.y -= Zoomed(UnZoomed(m_ptFrac.y));
        }
        selectionModel.ModifyDestRect(m_ptDelta, m_iAction);

        CString strSize;
        strSize.Format(_T("%ld x %ld"), selectionModel.GetDestRectWidth(), selectionModel.GetDestRectHeight());
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) (LPCTSTR) strSize);

        if (m_iAction != ACTION_MOVE)
            selectionModel.DrawSelectionStretched(imageModel.GetDC());
        else
            selectionModel.DrawSelection(imageModel.GetDC(), paletteModel.GetBgColor(), toolsModel.IsBackgroundTransparent());

        imageArea.InvalidateRect(NULL, FALSE);
        imageArea.SendMessage(WM_PAINT, 0, 0);
        m_ptPos.x = GET_X_LPARAM(lParam);
        m_ptPos.y = GET_Y_LPARAM(lParam);
    }
    else
    {
        int w = Zoomed(selectionModel.GetDestRectWidth()) + 2 * GRIP_SIZE;
        int h = Zoomed(selectionModel.GetDestRectHeight()) + 2 * GRIP_SIZE;
        m_ptPos.x = GET_X_LPARAM(lParam);
        m_ptPos.y = GET_Y_LPARAM(lParam);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) NULL);
        m_iAction = IdentifyCorner(m_ptPos.x, m_ptPos.y, w, h);
        if (m_iAction != ACTION_MOVE)
            SetCursor(LoadCursor(NULL, m_lpszCursorLUT[m_iAction]));
    }
    return 0;
}

LRESULT CSelectionWindow::OnMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_bMoved = TRUE;
    return 0;
}

LRESULT CSelectionWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_bMoving)
    {
        m_bMoving = FALSE;
        ReleaseCapture();
        if (m_iAction != ACTION_MOVE && toolsModel.GetActiveTool() != TOOL_TEXT)
        {
            imageModel.Undo();
            imageModel.DrawSelectionBackground();
            selectionModel.ScaleContentsToFit();
            imageModel.CopyPrevious();
        }
        placeSelWin();
    }
    return 0;
}

LRESULT CSelectionWindow::OnCaptureChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_bMoving)
    {
        m_bMoving = FALSE;
        if (m_iAction == ACTION_MOVE)
        {
            if (toolsModel.GetActiveTool() == TOOL_RECTSEL)
            {
                imageArea.cancelDrawing();
            }
            else
            {
                placeSelWin();
            }
        }
        else
        {
            m_iAction = ACTION_MOVE;
        }
        ShowWindow(SW_HIDE);
    }
    return 0;
}

LRESULT CSelectionWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE)
    {
        if (GetCapture() == m_hWnd)
        {
            ReleaseCapture();
        }
    }
    return 0;
}

LRESULT CSelectionWindow::OnPaletteModelColorChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CSelectionWindow::OnToolsModelSettingsChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CSelectionWindow::OnSelectionModelRefreshNeeded(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CSelectionWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return ::SendMessage(GetParent(), nMsg, wParam, lParam);
}
