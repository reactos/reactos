/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/sizebox.cpp
 * PURPOSE:     Window procedure of the size boxes
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

BOOL resizing = FALSE;
short xOrig;
short yOrig;

LRESULT CSizeboxWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if ((m_hWnd == sizeboxLeftTop.m_hWnd) || (m_hWnd == sizeboxRightBottom.m_hWnd))
        SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
    if ((m_hWnd == sizeboxLeftBottom.m_hWnd) || (m_hWnd == sizeboxRightTop.m_hWnd))
        SetCursor(LoadCursor(NULL, IDC_SIZENESW));
    if ((m_hWnd == sizeboxLeftCenter.m_hWnd) || (m_hWnd == sizeboxRightCenter.m_hWnd))
        SetCursor(LoadCursor(NULL, IDC_SIZEWE));
    if ((m_hWnd == sizeboxCenterTop.m_hWnd) || (m_hWnd == sizeboxCenterBottom.m_hWnd))
        SetCursor(LoadCursor(NULL, IDC_SIZENS));
    return 0;
}

LRESULT CSizeboxWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    resizing = TRUE;
    xOrig = GET_X_LPARAM(lParam);
    yOrig = GET_Y_LPARAM(lParam);
    SetCapture();
    return 0;
}

LRESULT CSizeboxWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (resizing)
    {
        TCHAR sizeStr[100];
        short xRel;
        short yRel;
        xRel = (GET_X_LPARAM(lParam) - xOrig) * 1000 / toolsModel.GetZoom();
        yRel = (GET_Y_LPARAM(lParam) - yOrig) * 1000 / toolsModel.GetZoom();
        if (m_hWnd == sizeboxLeftTop.m_hWnd)
            _stprintf(sizeStr, _T("%d x %d"), imgXRes - xRel, imgYRes - yRel);
        if (m_hWnd == sizeboxCenterTop.m_hWnd)
            _stprintf(sizeStr, _T("%d x %d"), imgXRes, imgYRes - yRel);
        if (m_hWnd == sizeboxRightTop.m_hWnd)
            _stprintf(sizeStr, _T("%d x %d"), imgXRes + xRel, imgYRes - yRel);
        if (m_hWnd == sizeboxLeftCenter.m_hWnd)
            _stprintf(sizeStr, _T("%d x %d"), imgXRes - xRel, imgYRes);
        if (m_hWnd == sizeboxRightCenter.m_hWnd)
            _stprintf(sizeStr, _T("%d x %d"), imgXRes + xRel, imgYRes);
        if (m_hWnd == sizeboxLeftBottom.m_hWnd)
            _stprintf(sizeStr, _T("%d x %d"), imgXRes - xRel, imgYRes + yRel);
        if (m_hWnd == sizeboxCenterBottom.m_hWnd)
            _stprintf(sizeStr, _T("%d x %d"), imgXRes, imgYRes + yRel);
        if (m_hWnd == sizeboxRightBottom.m_hWnd)
            _stprintf(sizeStr, _T("%d x %d"), imgXRes + xRel, imgYRes + yRel);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) sizeStr);
    }
    return 0;
}

LRESULT CSizeboxWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (resizing)
    {
        short xRel;
        short yRel;
        ReleaseCapture();
        resizing = FALSE;
        xRel = (GET_X_LPARAM(lParam) - xOrig) * 1000 / toolsModel.GetZoom();
        yRel = (GET_Y_LPARAM(lParam) - yOrig) * 1000 / toolsModel.GetZoom();
        if (m_hWnd == sizeboxLeftTop)
            cropReversible(imgXRes - xRel, imgYRes - yRel, xRel, yRel);
        if (m_hWnd == sizeboxCenterTop.m_hWnd)
            cropReversible(imgXRes, imgYRes - yRel, 0, yRel);
        if (m_hWnd == sizeboxRightTop.m_hWnd)
            cropReversible(imgXRes + xRel, imgYRes - yRel, 0, yRel);
        if (m_hWnd == sizeboxLeftCenter.m_hWnd)
            cropReversible(imgXRes - xRel, imgYRes, xRel, 0);
        if (m_hWnd == sizeboxRightCenter.m_hWnd)
            cropReversible(imgXRes + xRel, imgYRes, 0, 0);
        if (m_hWnd == sizeboxLeftBottom.m_hWnd)
            cropReversible(imgXRes - xRel, imgYRes + yRel, xRel, 0);
        if (m_hWnd == sizeboxCenterBottom.m_hWnd)
            cropReversible(imgXRes, imgYRes + yRel, 0, 0);
        if (m_hWnd == sizeboxRightBottom.m_hWnd)
            cropReversible(imgXRes + xRel, imgYRes + yRel, 0, 0);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) _T(""));
    }
    return 0;
}
