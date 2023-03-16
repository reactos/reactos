/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/canvas.cpp
 * PURPOSE:     Providing the canvas window class
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"
#include <atltypes.h>

/* FUNCTIONS ********************************************************/

VOID CCanvasWindow::Update(HWND hwndFrom)
{
    CRect tempRect;
    GetClientRect(&tempRect);
    CSize sizeScrollBox(tempRect.Width(), tempRect.Height());

    CSize sizeZoomed = { Zoomed(imageModel.GetWidth()), Zoomed(imageModel.GetHeight()) };
    CSize sizeWhole = { sizeZoomed.cx + (GRIP_SIZE * 2), sizeZoomed.cy + (GRIP_SIZE * 2) };

    /* show/hide the scrollbars */
    ShowScrollBar(SB_HORZ, sizeScrollBox.cx < sizeWhole.cx);
    ShowScrollBar(SB_VERT, sizeScrollBox.cy < sizeWhole.cy);

    if (sizeScrollBox.cx < sizeWhole.cx || sizeScrollBox.cy < sizeWhole.cy)
    {
        GetClientRect(&tempRect);
        sizeScrollBox = CSize(tempRect.Width(), tempRect.Height());
    }

    SCROLLINFO si = { sizeof(si), SIF_PAGE | SIF_RANGE };
    si.nMin   = 0;

    si.nMax   = sizeWhole.cx;
    si.nPage  = sizeScrollBox.cx;
    SetScrollInfo(SB_HORZ, &si);

    si.nMax   = sizeWhole.cy;
    si.nPage  = sizeScrollBox.cy;
    SetScrollInfo(SB_VERT, &si);

    INT dx = -GetScrollPos(SB_HORZ);
    INT dy = -GetScrollPos(SB_VERT);

    if (sizeboxLeftTop.IsWindow())
    {
        sizeboxLeftTop.MoveWindow(dx, dy, GRIP_SIZE, GRIP_SIZE, TRUE);
        sizeboxCenterTop.MoveWindow(dx + GRIP_SIZE + (sizeZoomed.cx - GRIP_SIZE) / 2,
                                    dy,
                                    GRIP_SIZE, GRIP_SIZE, TRUE);
        sizeboxRightTop.MoveWindow(dx + GRIP_SIZE + sizeZoomed.cx,
                                   dy,
                                   GRIP_SIZE, GRIP_SIZE, TRUE);
        sizeboxLeftCenter.MoveWindow(dx,
                                     dy + GRIP_SIZE + (sizeZoomed.cy - GRIP_SIZE) / 2,
                                     GRIP_SIZE, GRIP_SIZE, TRUE);
        sizeboxRightCenter.MoveWindow(dx + GRIP_SIZE + sizeZoomed.cx,
                                      dy + GRIP_SIZE + (sizeZoomed.cy - GRIP_SIZE) / 2,
                                      GRIP_SIZE, GRIP_SIZE, TRUE);
        sizeboxLeftBottom.MoveWindow(dx,
                                     dy + GRIP_SIZE + sizeZoomed.cy,
                                     GRIP_SIZE, GRIP_SIZE, TRUE);
        sizeboxCenterBottom.MoveWindow(dx + GRIP_SIZE + (sizeZoomed.cx - GRIP_SIZE) / 2,
                                       dy + GRIP_SIZE + sizeZoomed.cy,
                                       GRIP_SIZE, GRIP_SIZE, TRUE);
        sizeboxRightBottom.MoveWindow(dx + GRIP_SIZE + sizeZoomed.cx,
                                      dy + GRIP_SIZE + sizeZoomed.cy,
                                      GRIP_SIZE, GRIP_SIZE, TRUE);
    }

    if (imageArea.IsWindow() && hwndFrom != imageArea.m_hWnd)
        imageArea.MoveWindow(dx + GRIP_SIZE, dy + GRIP_SIZE, sizeZoomed.cx, sizeZoomed.cy, TRUE);
}

LRESULT CCanvasWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_hWnd)
    {
        Update(m_hWnd);
    }
    return 0;
}

LRESULT CCanvasWindow::OnHScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(SB_HORZ, &si);
    switch (LOWORD(wParam))
    {
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            si.nPos = HIWORD(wParam);
            break;
        case SB_LINELEFT:
            si.nPos -= 5;
            break;
        case SB_LINERIGHT:
            si.nPos += 5;
            break;
        case SB_PAGELEFT:
            si.nPos -= si.nPage;
            break;
        case SB_PAGERIGHT:
            si.nPos += si.nPage;
            break;
    }
    SetScrollInfo(SB_HORZ, &si);
    Update(m_hWnd);
    return 0;
}

LRESULT CCanvasWindow::OnVScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(SB_VERT, &si);
    switch (LOWORD(wParam))
    {
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            si.nPos = HIWORD(wParam);
            break;
        case SB_LINEUP:
            si.nPos -= 5;
            break;
        case SB_LINEDOWN:
            si.nPos += 5;
            break;
        case SB_PAGEUP:
            si.nPos -= si.nPage;
            break;
        case SB_PAGEDOWN:
            si.nPos += si.nPage;
            break;
    }
    SetScrollInfo(SB_VERT, &si);
    Update(m_hWnd);
    return 0;
}

LRESULT CCanvasWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_BEZIER:
        case TOOL_SHAPE:
            if (ToolBase::pointSP != 0)
            {
                toolsModel.OnCancelDraw();
                imageArea.Invalidate();
            }
            break;

        default:
            break;
    }

    toolsModel.resetTool();  // resets the point-buffer of the polygon and bezier functions
    return 0;
}

LRESULT CCanvasWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return ::SendMessage(GetParent(), nMsg, wParam, lParam);
}
