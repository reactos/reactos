/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/canvas.cpp
 * PURPOSE:     Providing the canvas window class
 * PROGRAMMERS: Benedikt Freisen
 */

#include "precomp.h"

CCanvasWindow canvasWindow;

/* FUNCTIONS ********************************************************/

CCanvasWindow::CCanvasWindow()
    : m_whereHit(HIT_NONE)
    , m_ptOrig { 0, 0 }
{
}

RECT CCanvasWindow::GetBaseRect()
{
    CRect rcBase = { 0, 0, Zoomed(imageModel.GetWidth()), Zoomed(imageModel.GetHeight()) };
    ::InflateRect(&rcBase, GRIP_SIZE, GRIP_SIZE);
    ::OffsetRect(&rcBase, GRIP_SIZE - GetScrollPos(SB_HORZ), GRIP_SIZE - GetScrollPos(SB_VERT));
    return rcBase;
}

CANVAS_HITTEST CCanvasWindow::HitTest(POINT pt)
{
    RECT rcBase = GetBaseRect();
    return getSizeBoxHitTest(pt, &rcBase);
}

VOID CCanvasWindow::DoDraw(HDC hDC, RECT& rcClient, RECT& rcPaint)
{
    // We use a memory bitmap to reduce flickering
    HDC hdcMem = ::CreateCompatibleDC(hDC);
    HBITMAP hbm = ::CreateCompatibleBitmap(hDC, rcClient.right, rcClient.bottom);
    HGDIOBJ hbmOld = ::SelectObject(hdcMem, hbm);

    // Fill the background
    ::FillRect(hdcMem, &rcPaint, (HBRUSH)(COLOR_APPWORKSPACE + 1));

    // Draw the sizeboxes
    RECT rcBase = GetBaseRect();
    drawSizeBoxes(hdcMem, &rcBase, FALSE, &rcPaint);

    // Transfer the bits
    ::BitBlt(hDC,
             rcPaint.left, rcPaint.top,
             rcPaint.right - rcPaint.left, rcPaint.bottom - rcPaint.top,
             hdcMem, rcPaint.left, rcPaint.top, SRCCOPY);

    ::SelectObject(hdcMem, hbmOld);
    ::DeleteDC(hdcMem);
}

VOID CCanvasWindow::Update(HWND hwndFrom)
{
    CRect rcClient;
    GetClientRect(&rcClient);

    CSize sizePage(rcClient.right, rcClient.bottom);
    CSize sizeZoomed = { Zoomed(imageModel.GetWidth()), Zoomed(imageModel.GetHeight()) };
    CSize sizeWhole = { sizeZoomed.cx + (GRIP_SIZE * 2), sizeZoomed.cy + (GRIP_SIZE * 2) };

    /* show/hide the scrollbars */
    ShowScrollBar(SB_HORZ, sizePage.cx < sizeWhole.cx);
    ShowScrollBar(SB_VERT, sizePage.cy < sizeWhole.cy);

    if (sizePage.cx < sizeWhole.cx || sizePage.cy < sizeWhole.cy)
    {
        GetClientRect(&rcClient); // Scrollbars might change, get client rectangle again
        sizePage = CSize(rcClient.right, rcClient.bottom);
    }

    SCROLLINFO si = { sizeof(si), SIF_PAGE | SIF_RANGE };
    si.nMin   = 0;

    si.nMax   = sizeWhole.cx;
    si.nPage  = sizePage.cx;
    SetScrollInfo(SB_HORZ, &si);

    si.nMax   = sizeWhole.cy;
    si.nPage  = sizePage.cy;
    SetScrollInfo(SB_VERT, &si);

    if (imageArea.IsWindow() && hwndFrom != imageArea.m_hWnd)
    {
        INT dx = -GetScrollPos(SB_HORZ), dy = -GetScrollPos(SB_VERT);
        imageArea.MoveWindow(dx + GRIP_SIZE, dy + GRIP_SIZE, sizeZoomed.cx, sizeZoomed.cy, TRUE);
    }
}

LRESULT CCanvasWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_hWnd)
        Update(m_hWnd);

    return 0;
}

VOID CCanvasWindow::OnHVScroll(WPARAM wParam, INT fnBar)
{
    SCROLLINFO si;
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(fnBar, &si);
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
    SetScrollInfo(fnBar, &si);
    Update(m_hWnd);
    Invalidate(FALSE); // FIXME: Flicker
}

LRESULT CCanvasWindow::OnHScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    OnHVScroll(wParam, SB_HORZ);
    return 0;
}

LRESULT CCanvasWindow::OnVScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    OnHVScroll(wParam, SB_VERT);
    return 0;
}

LRESULT CCanvasWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    CANVAS_HITTEST hit = HitTest(pt);

    if (hit == HIT_NONE || hit == HIT_BORDER)
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

            case TOOL_FREESEL:
            case TOOL_RECTSEL:
                toolsModel.OnFinishDraw();
                imageArea.Invalidate();
                break;

            default:
                break;
        }

        toolsModel.resetTool();  // resets the point-buffer of the polygon and bezier functions
        return 0;
    }

    if (hit == HIT_INNER)
    {
        // TODO: In the future, we handle the events of the window-less image area.
        return 0;
    }

    // Start dragging
    m_whereHit = hit;
    m_ptOrig = pt;
    SetCapture();
    return 0;
}

LRESULT CCanvasWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_whereHit == HIT_NONE || ::GetCapture() != m_hWnd)
        return 0;

    // Dragging now... Calculate the new size
    INT cxImage = imageModel.GetWidth(), cyImage = imageModel.GetHeight();
    INT cxDelta = UnZoomed(GET_X_LPARAM(lParam) - m_ptOrig.x);
    INT cyDelta = UnZoomed(GET_Y_LPARAM(lParam) - m_ptOrig.y);
    switch (m_whereHit)
    {
        case HIT_UPPER_LEFT:
            cxImage -= cxDelta;
            cyImage -= cyDelta;
            break;
        case HIT_UPPER_CENTER:
            cyImage -= cyDelta;
            break;
        case HIT_UPPER_RIGHT:
            cxImage += cxDelta;
            cyImage -= cyDelta;
            break;
        case HIT_MIDDLE_LEFT:
            cxImage -= cxDelta;
            break;
        case HIT_MIDDLE_RIGHT:
            cxImage += cxDelta;
            break;
        case HIT_LOWER_LEFT:
            cxImage -= cxDelta;
            cyImage += cyDelta;
            break;
        case HIT_LOWER_CENTER:
            cyImage += cyDelta;
            break;
        case HIT_LOWER_RIGHT:
            cxImage += cxDelta;
            cyImage += cyDelta;
            break;
        default:
            return 0;
    }

    // Limit bitmap size
    cxImage = max(1, cxImage);
    cyImage = max(1, cyImage);
    cxImage = min(MAXWORD, cxImage);
    cyImage = min(MAXWORD, cyImage);

    // Display new size
    CString strSize;
    strSize.Format(_T("%d x %d"), cxImage, cyImage);
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) (LPCTSTR) strSize);

    return 0;
}

LRESULT CCanvasWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ::ReleaseCapture();

    if (m_whereHit == HIT_NONE)
        return 0;

    // Resize the image
    INT cxImage = imageModel.GetWidth(), cyImage = imageModel.GetHeight();
    INT cxDelta = UnZoomed(GET_X_LPARAM(lParam) - m_ptOrig.x);
    INT cyDelta = UnZoomed(GET_Y_LPARAM(lParam) - m_ptOrig.y);
    switch (m_whereHit)
    {
        case HIT_UPPER_LEFT:
            imageModel.Crop(cxImage - cxDelta, cyImage - cyDelta, cxDelta, cyDelta);
            break;
        case HIT_UPPER_CENTER:
            imageModel.Crop(cxImage, cyImage - cyDelta, 0, cyDelta);
            break;
        case HIT_UPPER_RIGHT:
            imageModel.Crop(cxImage + cxDelta, cyImage - cyDelta, 0, cyDelta);
            break;
        case HIT_MIDDLE_LEFT:
            imageModel.Crop(cxImage - cxDelta, cyImage, cxDelta, 0);
            break;
        case HIT_MIDDLE_RIGHT:
            imageModel.Crop(cxImage + cxDelta, cyImage, 0, 0);
            break;
        case HIT_LOWER_LEFT:
            imageModel.Crop(cxImage - cxDelta, cyImage + cyDelta, cxDelta, 0);
            break;
        case HIT_LOWER_CENTER:
            imageModel.Crop(cxImage, cyImage + cyDelta, 0, 0);
            break;
        case HIT_LOWER_RIGHT:
            imageModel.Crop(cxImage + cxDelta, cyImage + cyDelta, 0, 0);
            break;
        default:
            break;
    }

    // Finish dragging
    m_whereHit = HIT_NONE;
    toolsModel.resetTool();  // resets the point-buffer of the polygon and bezier functions
    Update(NULL);
    Invalidate(TRUE);

    return 0;
}

LRESULT CCanvasWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt;
    ::GetCursorPos(&pt);
    ScreenToClient(&pt);

    if (!setCursorOnSizeBox(HitTest(pt)))
        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));

    return 0;
}

LRESULT CCanvasWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE && ::GetCapture() == m_hWnd)
    {
        // Cancel dragging
        m_whereHit = HIT_NONE;
        ::ReleaseCapture();
    }

    return 0;
}

LRESULT CCanvasWindow::OnCancelMode(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Cancel dragging
    m_whereHit = HIT_NONE;
    Invalidate(TRUE);
    return 0;
}

LRESULT CCanvasWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return ::SendMessage(GetParent(), nMsg, wParam, lParam);
}

LRESULT CCanvasWindow::OnCaptureChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)_T(""));
    return 0;
}

LRESULT CCanvasWindow::OnEraseBkgnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return TRUE; // do nothing => transparent background
}

LRESULT CCanvasWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rcClient;
    GetClientRect(&rcClient);

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(&ps);
    DoDraw(hDC, rcClient, ps.rcPaint);
    EndPaint(&ps);
    return 0;
}
