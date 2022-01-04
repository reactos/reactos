/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/sizebox.cpp
 * PURPOSE:     Window procedure of the size boxes
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

static BOOL resizing = FALSE;
static short xOrig;
static short yOrig;

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
        CString strSize;
        short xRel;
        short yRel;
        int imgXRes = imageModel.GetWidth();
        int imgYRes = imageModel.GetHeight();
        xRel = UnZoomed(GET_X_LPARAM(lParam) - xOrig);
        yRel = UnZoomed(GET_Y_LPARAM(lParam) - yOrig);
        if (m_hWnd == sizeboxLeftTop.m_hWnd)
            strSize.Format(_T("%d x %d"), imgXRes - xRel, imgYRes - yRel);
        if (m_hWnd == sizeboxCenterTop.m_hWnd)
            strSize.Format(_T("%d x %d"), imgXRes, imgYRes - yRel);
        if (m_hWnd == sizeboxRightTop.m_hWnd)
            strSize.Format(_T("%d x %d"), imgXRes + xRel, imgYRes - yRel);
        if (m_hWnd == sizeboxLeftCenter.m_hWnd)
            strSize.Format(_T("%d x %d"), imgXRes - xRel, imgYRes);
        if (m_hWnd == sizeboxRightCenter.m_hWnd)
            strSize.Format(_T("%d x %d"), imgXRes + xRel, imgYRes);
        if (m_hWnd == sizeboxLeftBottom.m_hWnd)
            strSize.Format(_T("%d x %d"), imgXRes - xRel, imgYRes + yRel);
        if (m_hWnd == sizeboxCenterBottom.m_hWnd)
            strSize.Format(_T("%d x %d"), imgXRes, imgYRes + yRel);
        if (m_hWnd == sizeboxRightBottom.m_hWnd)
            strSize.Format(_T("%d x %d"), imgXRes + xRel, imgYRes + yRel);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) (LPCTSTR) strSize);
    }
    return 0;
}

LRESULT CSizeboxWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (resizing)
    {
        short xRel;
        short yRel;
        int imgXRes = imageModel.GetWidth();
        int imgYRes = imageModel.GetHeight();
        xRel = (GET_X_LPARAM(lParam) - xOrig) * 1000 / toolsModel.GetZoom();
        yRel = (GET_Y_LPARAM(lParam) - yOrig) * 1000 / toolsModel.GetZoom();
        if (m_hWnd == sizeboxLeftTop)
            imageModel.Crop(imgXRes - xRel, imgYRes - yRel, xRel, yRel);
        if (m_hWnd == sizeboxCenterTop.m_hWnd)
            imageModel.Crop(imgXRes, imgYRes - yRel, 0, yRel);
        if (m_hWnd == sizeboxRightTop.m_hWnd)
            imageModel.Crop(imgXRes + xRel, imgYRes - yRel, 0, yRel);
        if (m_hWnd == sizeboxLeftCenter.m_hWnd)
            imageModel.Crop(imgXRes - xRel, imgYRes, xRel, 0);
        if (m_hWnd == sizeboxRightCenter.m_hWnd)
            imageModel.Crop(imgXRes + xRel, imgYRes, 0, 0);
        if (m_hWnd == sizeboxLeftBottom.m_hWnd)
            imageModel.Crop(imgXRes - xRel, imgYRes + yRel, xRel, 0);
        if (m_hWnd == sizeboxCenterBottom.m_hWnd)
            imageModel.Crop(imgXRes, imgYRes + yRel, 0, 0);
        if (m_hWnd == sizeboxRightBottom.m_hWnd)
            imageModel.Crop(imgXRes + xRel, imgYRes + yRel, 0, 0);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) _T(""));
    }
    resizing = FALSE;
    ReleaseCapture();
    return 0;
}

LRESULT CSizeboxWindow::OnCaptureChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    resizing = FALSE;
    return 0;
}

LRESULT CSizeboxWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
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
