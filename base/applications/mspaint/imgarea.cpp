/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/imgarea.cpp
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

#include "precomp.h"

CImgAreaWindow imageArea;

/* FUNCTIONS ********************************************************/

LRESULT CImgAreaWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_hCurFill     = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDC_FILL));
    m_hCurColor    = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDC_COLOR));
    m_hCurZoom     = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDC_ZOOM));
    m_hCurPen      = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDC_PEN));
    m_hCurAirbrush = LoadIcon(hProgInstance, MAKEINTRESOURCE(IDC_AIRBRUSH));
    return 0;
}

void CImgAreaWindow::drawZoomFrame(int mouseX, int mouseY)
{
    HDC hdc;
    HPEN oldPen;
    HBRUSH oldBrush;
    LOGBRUSH logbrush;
    int rop;

    RECT clientRectCanvas;
    RECT clientRectImageArea;
    int x, y, w, h;
    canvasWindow.GetClientRect(&clientRectCanvas);
    GetClientRect(&clientRectImageArea);
    w = clientRectImageArea.right * 2;
    h = clientRectImageArea.bottom * 2;
    if (!w || !h)
    {
        return;
    }
    w = clientRectImageArea.right * clientRectCanvas.right / w;
    h = clientRectImageArea.bottom * clientRectCanvas.bottom / h;
    x = max(0, min(clientRectImageArea.right - w, mouseX - w / 2));
    y = max(0, min(clientRectImageArea.bottom - h, mouseY - h / 2));

    hdc = GetDC();
    oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, 0, 0));
    logbrush.lbStyle = BS_HOLLOW;
    oldBrush = (HBRUSH) SelectObject(hdc, CreateBrushIndirect(&logbrush));
    rop = SetROP2(hdc, R2_NOT);
    Rectangle(hdc, x, y, x + w, y + h);
    SetROP2(hdc, rop);
    DeleteObject(SelectObject(hdc, oldBrush));
    DeleteObject(SelectObject(hdc, oldPen));
    ReleaseDC(hdc);
}

LRESULT CImgAreaWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!IsWindow())
        return 0;
    canvasWindow.Update(NULL);
    return 0;
}

LRESULT CImgAreaWindow::OnEraseBkGnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return TRUE; // Don't fill background
}

LRESULT CImgAreaWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rcClient;
    GetClientRect(&rcClient);

    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);

    // We use a memory bitmap to reduce flickering
    HDC hdcMem = ::CreateCompatibleDC(hdc);
    HBITMAP hbm = ::CreateCompatibleBitmap(hdc, rcClient.right, rcClient.bottom);
    HGDIOBJ hbmOld = ::SelectObject(hdcMem, hbm);

    // Draw the image
    SIZE size = { imageModel.GetWidth(), imageModel.GetHeight() };
    StretchBlt(hdcMem, 0, 0, ::Zoomed(size.cx), ::Zoomed(size.cy),
               imageModel.GetDC(), 0, 0, size.cx, size.cy, SRCCOPY);

    // Draw the grid
    if (showGrid && (toolsModel.GetZoom() >= 4000))
    {
        HPEN oldPen = (HPEN) SelectObject(hdcMem, CreatePen(PS_SOLID, 1, 0x00a0a0a0));
        for (int counter = 0; counter <= size.cy; counter++)
        {
            ::MoveToEx(hdcMem, 0, ::Zoomed(counter), NULL);
            ::LineTo(hdcMem, ::Zoomed(size.cx), ::Zoomed(counter));
        }
        for (int counter = 0; counter <= size.cx; counter++)
        {
            ::MoveToEx(hdcMem, ::Zoomed(counter), 0, NULL);
            ::LineTo(hdcMem, ::Zoomed(counter), ::Zoomed(size.cy));
        }
        ::DeleteObject(::SelectObject(hdcMem, oldPen));
    }

    // Draw selection
    if (selectionModel.m_bShow)
    {
        RECT rc = selectionModel.m_rc;
        Zoomed(rc);
        ::InflateRect(&rc, GRIP_SIZE, GRIP_SIZE);
        drawSizeBoxes(hdcMem, &rc, TRUE, &ps.rcPaint);
        ::InflateRect(&rc, -GRIP_SIZE, -GRIP_SIZE);
        selectionModel.DrawSelection(hdcMem, &rc, paletteModel.GetBgColor(),
                                     toolsModel.IsBackgroundTransparent());
    }

    // Transfer bits
    ::BitBlt(hdc, 0, 0, rcClient.right, rcClient.bottom, hdcMem, 0, 0, SRCCOPY);
    ::SelectObject(hdcMem, hbmOld);
    EndPaint(&ps);

    if (miniature.IsWindow())
        miniature.Invalidate(FALSE);
    if (textEditWindow.IsWindow())
        textEditWindow.Invalidate(FALSE);
    return 0;
}

CANVAS_HITTEST CImgAreaWindow::SelectionHitTest(POINT ptZoomed)
{
    if (!selectionModel.m_bShow)
        return HIT_NONE;

    RECT rcSelection = selectionModel.m_rc;
    Zoomed(rcSelection);
    ::InflateRect(&rcSelection, GRIP_SIZE, GRIP_SIZE);

    return getSizeBoxHitTest(ptZoomed, &rcSelection);
}

void CImgAreaWindow::StartSelectionDrag(CANVAS_HITTEST hit, POINT ptUnZoomed)
{
    m_hitSelection = hit;
    selectionModel.m_ptHit = ptUnZoomed;
    selectionModel.TakeOff();

    SetCapture();
    Invalidate(FALSE);
}

void CImgAreaWindow::SelectionDragging(POINT ptUnZoomed)
{
    selectionModel.Dragging(m_hitSelection, ptUnZoomed);
    Invalidate(FALSE);
}

void CImgAreaWindow::EndSelectionDrag(POINT ptUnZoomed)
{
    selectionModel.Dragging(m_hitSelection, ptUnZoomed);
    m_hitSelection = HIT_NONE;
    Invalidate(FALSE);
}

LRESULT CImgAreaWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt;
    ::GetCursorPos(&pt);
    ScreenToClient(&pt);

    CANVAS_HITTEST hit = SelectionHitTest(pt);
    if (hit != HIT_NONE)
    {
        if (!setCursorOnSizeBox(hit))
            ::SetCursor(::LoadCursor(NULL, IDC_SIZEALL));
        return 0;
    }

    UnZoomed(pt);

    switch (toolsModel.GetActiveTool())
    {
        case TOOL_FILL:
            ::SetCursor(m_hCurFill);
            break;
        case TOOL_COLOR:
            ::SetCursor(m_hCurColor);
            break;
        case TOOL_ZOOM:
            ::SetCursor(m_hCurZoom);
            break;
        case TOOL_PEN:
            ::SetCursor(m_hCurPen);
            break;
        case TOOL_AIRBRUSH:
            ::SetCursor(m_hCurAirbrush);
            break;
        default:
            ::SetCursor(::LoadCursor(NULL, IDC_CROSS));
    }
    return 0;
}

LRESULT CImgAreaWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

    CANVAS_HITTEST hit = SelectionHitTest(pt);
    if (hit != HIT_NONE)
    {
        UnZoomed(pt);
        StartSelectionDrag(hit, pt);
        return 0;
    }

    UnZoomed(pt);
    drawing = TRUE;
    SetCapture();
    toolsModel.OnButtonDown(TRUE, pt.x, pt.y, FALSE);
    Invalidate(FALSE);
    return 0;
}

LRESULT CImgAreaWindow::OnLButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    UnZoomed(pt);
    drawing = FALSE;
    ReleaseCapture();
    toolsModel.OnButtonDown(TRUE, pt.x, pt.y, TRUE);
    toolsModel.resetTool();
    Invalidate(FALSE);
    return 0;
}

LRESULT CImgAreaWindow::OnRButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    UnZoomed(pt);
    drawing = TRUE;
    SetCapture();
    toolsModel.OnButtonDown(FALSE, pt.x, pt.y, FALSE);
    Invalidate(FALSE);
    return 0;
}

LRESULT CImgAreaWindow::OnRButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    UnZoomed(pt);
    drawing = FALSE;
    ReleaseCapture();
    toolsModel.OnButtonDown(FALSE, pt.x, pt.y, TRUE);
    toolsModel.resetTool();
    Invalidate(FALSE);
    return 0;
}

LRESULT CImgAreaWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    UnZoomed(pt);
    if (drawing)
    {
        drawing = FALSE;
        toolsModel.OnButtonUp(TRUE, pt.x, pt.y);
        Invalidate(FALSE);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "");
    }
    else if (m_hitSelection != HIT_NONE)
    {
        EndSelectionDrag(pt);
    }
    ReleaseCapture();
    return 0;
}

void CImgAreaWindow::cancelDrawing()
{
    selectionModel.ClearColor();
    selectionModel.ClearMask();
    m_hitSelection = HIT_NONE;
    drawing = FALSE;
    toolsModel.OnCancelDraw();
    Invalidate(FALSE);
}

LRESULT CImgAreaWindow::OnCaptureChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (drawing)
        cancelDrawing();
    return 0;
}

LRESULT CImgAreaWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE)
    {
        if (GetCapture() == m_hWnd)
        {
            ReleaseCapture();
        }
        else
        {
            if (drawing || ToolBase::pointSP != 0 || selectionModel.m_bShow)
                cancelDrawing();
        }
    }
    return 0;
}

LRESULT CImgAreaWindow::OnRButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    UnZoomed(pt);
    if (drawing)
    {
        drawing = FALSE;
        toolsModel.OnButtonUp(FALSE, pt.x, pt.y);
        Invalidate(FALSE);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "");
    }
    else if (m_hitSelection != HIT_NONE)
    {
        EndSelectionDrag(pt);
    }
    ReleaseCapture();
    return 0;
}

LRESULT CImgAreaWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    UnZoomed(pt);

    if (m_hitSelection != HIT_NONE)
    {
        SelectionDragging(pt);
        return 0;
    }

    if ((!drawing) || (toolsModel.GetActiveTool() <= TOOL_AIRBRUSH))
    {
        TRACKMOUSEEVENT tme;

        if (toolsModel.GetActiveTool() == TOOL_ZOOM)
        {
            Invalidate(FALSE);
            UpdateWindow();
            drawZoomFrame(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
        }

        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        tme.dwHoverTime = 0;
        TrackMouseEvent(&tme);

        if (!drawing)
        {
            CString strCoord;
            strCoord.Format(_T("%ld, %ld"), pt.x, pt.y);
            SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) (LPCTSTR) strCoord);
        }
    }
    if (drawing)
    {
        /* values displayed in statusbar */
        LONG xRel = pt.x - start.x;
        LONG yRel = pt.y - start.y;
        /* freesel, rectsel and text tools always show numbers limited to fit into image area */
        if ((toolsModel.GetActiveTool() == TOOL_FREESEL) || (toolsModel.GetActiveTool() == TOOL_RECTSEL) || (toolsModel.GetActiveTool() == TOOL_TEXT))
        {
            if (xRel < 0)
                xRel = (pt.x < 0) ? -start.x : xRel;
            else if (pt.x > imageModel.GetWidth())
                xRel = imageModel.GetWidth() - start.x;
            if (yRel < 0)
                yRel = (pt.y < 0) ? -start.y : yRel;
            else if (pt.y > imageModel.GetHeight())
                yRel = imageModel.GetHeight() - start.y;
        }
        /* rectsel and shape tools always show non-negative numbers when drawing */
        if ((toolsModel.GetActiveTool() == TOOL_RECTSEL) || (toolsModel.GetActiveTool() == TOOL_SHAPE))
        {
            if (xRel < 0)
                xRel = -xRel;
            if (yRel < 0)
                yRel =  -yRel;
        }
        /* while drawing, update cursor coordinates only for tools 3, 7, 8, 9, 14 */
        switch(toolsModel.GetActiveTool())
        {
            case TOOL_RUBBER:
            case TOOL_PEN:
            case TOOL_BRUSH:
            case TOOL_AIRBRUSH:
            case TOOL_SHAPE:
            {
                CString strCoord;
                strCoord.Format(_T("%ld, %ld"), pt.x, pt.y);
                SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) (LPCTSTR) strCoord);
                break;
            }
            default:
                break;
        }
        if (wParam & MK_LBUTTON)
        {
            toolsModel.OnMouseMove(TRUE, pt.x, pt.y);
            Invalidate(FALSE);
            if ((toolsModel.GetActiveTool() >= TOOL_TEXT) || (toolsModel.GetActiveTool() == TOOL_RECTSEL) || (toolsModel.GetActiveTool() == TOOL_FREESEL))
            {
                CString strSize;
                if ((toolsModel.GetActiveTool() >= TOOL_LINE) && (GetAsyncKeyState(VK_SHIFT) < 0))
                    yRel = xRel;
                strSize.Format(_T("%ld x %ld"), xRel, yRel);
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) (LPCTSTR) strSize);
            }
        }
        if (wParam & MK_RBUTTON)
        {
            toolsModel.OnMouseMove(FALSE, pt.x, pt.y);
            Invalidate(FALSE);
            if (toolsModel.GetActiveTool() >= TOOL_TEXT)
            {
                CString strSize;
                if ((toolsModel.GetActiveTool() >= TOOL_LINE) && (GetAsyncKeyState(VK_SHIFT) < 0))
                    yRel = xRel;
                strSize.Format(_T("%ld x %ld"), xRel, yRel);
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) (LPCTSTR) strSize);
            }
        }
    }
    return 0;
}

LRESULT CImgAreaWindow::OnMouseLeave(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) _T(""));
    if (toolsModel.GetActiveTool() == TOOL_ZOOM)
        Invalidate(FALSE);
    return 0;
}

LRESULT CImgAreaWindow::OnImageModelDimensionsChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    canvasWindow.Update(NULL);
    return 0;
}

LRESULT CImgAreaWindow::OnImageModelImageChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Invalidate(FALSE);
    return 0;
}

LRESULT CImgAreaWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return ::SendMessage(GetParent(), nMsg, wParam, lParam);
}

LRESULT CImgAreaWindow::OnCtlColorEdit(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HDC hdc = (HDC)wParam;
    SetBkMode(hdc, TRANSPARENT);
    return (LRESULT)GetStockObject(NULL_BRUSH);
}

void CImgAreaWindow::finishDrawing()
{
    toolsModel.OnFinishDraw();
    drawing = FALSE;
    Invalidate(FALSE);
}
