/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/imgarea.cpp
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

void
updateCanvasAndScrollbars()
{
    selectionWindow.ShowWindow(SW_HIDE);

    int zoomedWidth = Zoomed(imageModel.GetWidth());
    int zoomedHeight = Zoomed(imageModel.GetHeight());
    imageArea.MoveWindow(GRIP_SIZE, GRIP_SIZE, zoomedWidth, zoomedHeight, FALSE);

    scrollboxWindow.Invalidate(TRUE);
    imageArea.Invalidate(FALSE);

    scrollboxWindow.SetScrollPos(SB_HORZ, 0, TRUE);
    scrollboxWindow.SetScrollPos(SB_VERT, 0, TRUE);
}

void CImgAreaWindow::drawZoomFrame(int mouseX, int mouseY)
{
    HDC hdc;
    HPEN oldPen;
    HBRUSH oldBrush;
    LOGBRUSH logbrush;
    int rop;

    RECT clientRectScrollbox;
    RECT clientRectImageArea;
    int x, y, w, h;
    scrollboxWindow.GetClientRect(&clientRectScrollbox);
    GetClientRect(&clientRectImageArea);
    w = clientRectImageArea.right * 2;
    h = clientRectImageArea.bottom * 2;
    if (!w || !h)
    {
        return;
    }
    w = clientRectImageArea.right * clientRectScrollbox.right / w;
    h = clientRectImageArea.bottom * clientRectScrollbox.bottom / h;
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
    int imgXRes = imageModel.GetWidth();
    int imgYRes = imageModel.GetHeight();
    sizeboxLeftTop.MoveWindow(
               0,
               0, GRIP_SIZE, GRIP_SIZE, TRUE);
    sizeboxCenterTop.MoveWindow(
               GRIP_SIZE + (Zoomed(imgXRes) - GRIP_SIZE) / 2,
               0, GRIP_SIZE, GRIP_SIZE, TRUE);
    sizeboxRightTop.MoveWindow(
               GRIP_SIZE + Zoomed(imgXRes),
               0, GRIP_SIZE, GRIP_SIZE, TRUE);
    sizeboxLeftCenter.MoveWindow(
               0,
               GRIP_SIZE + (Zoomed(imgYRes) - GRIP_SIZE) / 2,
               GRIP_SIZE, GRIP_SIZE, TRUE);
    sizeboxRightCenter.MoveWindow(
               GRIP_SIZE + Zoomed(imgXRes),
               GRIP_SIZE + (Zoomed(imgYRes) - GRIP_SIZE) / 2,
               GRIP_SIZE, GRIP_SIZE, TRUE);
    sizeboxLeftBottom.MoveWindow(
               0,
               GRIP_SIZE + Zoomed(imgYRes), GRIP_SIZE, GRIP_SIZE, TRUE);
    sizeboxCenterBottom.MoveWindow(
               GRIP_SIZE + (Zoomed(imgXRes) - GRIP_SIZE) / 2,
               GRIP_SIZE + Zoomed(imgYRes), GRIP_SIZE, GRIP_SIZE, TRUE);
    sizeboxRightBottom.MoveWindow(
               GRIP_SIZE + Zoomed(imgXRes),
               GRIP_SIZE + Zoomed(imgYRes), GRIP_SIZE, GRIP_SIZE, TRUE);
    UpdateScrollbox();
    return 0;
}

LRESULT CImgAreaWindow::OnEraseBkGnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HDC hdc = (HDC)wParam;

    if (toolsModel.GetActiveTool() == TOOL_TEXT && !toolsModel.IsBackgroundTransparent() &&
        textEditWindow.IsWindowVisible())
    {
        // Do clipping
        HWND hChild = textEditWindow;
        RECT rcChild;
        ::GetWindowRect(hChild, &rcChild);
        ::MapWindowPoints(NULL, m_hWnd, (LPPOINT)&rcChild, 2);
        ExcludeClipRect(hdc, rcChild.left, rcChild.top, rcChild.right, rcChild.bottom);
    }

    return DefWindowProc(nMsg, wParam, lParam);
}

LRESULT CImgAreaWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(&ps);
    int imgXRes = imageModel.GetWidth();
    int imgYRes = imageModel.GetHeight();
    StretchBlt(hdc, 0, 0, Zoomed(imgXRes), Zoomed(imgYRes), imageModel.GetDC(), 0, 0, imgXRes,
               imgYRes, SRCCOPY);
    if (showGrid && (toolsModel.GetZoom() >= 4000))
    {
        HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, 1, 0x00a0a0a0));
        int counter;
        for(counter = 0; counter <= imgYRes; counter++)
        {
            MoveToEx(hdc, 0, Zoomed(counter), NULL);
            LineTo(hdc, Zoomed(imgXRes), Zoomed(counter));
        }
        for(counter = 0; counter <= imgXRes; counter++)
        {
            MoveToEx(hdc, Zoomed(counter), 0, NULL);
            LineTo(hdc, Zoomed(counter), Zoomed(imgYRes));
        }
        DeleteObject(SelectObject(hdc, oldPen));
    }
    EndPaint(&ps);
    if (selectionWindow.IsWindowVisible())
        selectionWindow.Invalidate(FALSE);
    if (miniature.IsWindowVisible())
        miniature.Invalidate(FALSE);
    if (textEditWindow.IsWindowVisible())
        textEditWindow.Invalidate(FALSE);
    return 0;
}

LRESULT CImgAreaWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    switch (toolsModel.GetActiveTool())
    {
        case TOOL_FILL:
            SetCursor(hCurFill);
            break;
        case TOOL_COLOR:
            SetCursor(hCurColor);
            break;
        case TOOL_ZOOM:
            SetCursor(hCurZoom);
            break;
        case TOOL_PEN:
            SetCursor(hCurPen);
            break;
        case TOOL_AIRBRUSH:
            SetCursor(hCurAirbrush);
            break;
        default:
            SetCursor(LoadCursor(NULL, IDC_CROSS));
    }
    return 0;
}

LRESULT CImgAreaWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    drawing = TRUE;
    SetCapture();
    INT x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
    toolsModel.OnButtonDown(TRUE, UnZoomed(x), UnZoomed(y), FALSE);
    Invalidate(FALSE);
    return 0;
}

LRESULT CImgAreaWindow::OnLButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    drawing = FALSE;
    ReleaseCapture();
    INT x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
    toolsModel.OnButtonDown(TRUE, UnZoomed(x), UnZoomed(y), TRUE);
    toolsModel.resetTool();
    Invalidate(FALSE);
    return 0;
}

LRESULT CImgAreaWindow::OnRButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    drawing = TRUE;
    SetCapture();
    INT x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
    toolsModel.OnButtonDown(FALSE, UnZoomed(x), UnZoomed(y), FALSE);
    Invalidate(FALSE);
    return 0;
}

LRESULT CImgAreaWindow::OnRButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    drawing = FALSE;
    ReleaseCapture();
    INT x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
    toolsModel.OnButtonDown(FALSE, UnZoomed(x), UnZoomed(y), TRUE);
    toolsModel.resetTool();
    Invalidate(FALSE);
    return 0;
}

LRESULT CImgAreaWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (drawing)
    {
        drawing = FALSE;
        INT x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        toolsModel.OnButtonUp(TRUE, UnZoomed(x), UnZoomed(y));
        Invalidate(FALSE);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "");
    }
    ReleaseCapture();
    return 0;
}

void CImgAreaWindow::cancelDrawing()
{
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
            if (drawing || ToolBase::pointSP != 0)
                cancelDrawing();
        }
    }
    return 0;
}

LRESULT CImgAreaWindow::OnRButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (drawing)
    {
        drawing = FALSE;
        INT x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
        toolsModel.OnButtonUp(FALSE, UnZoomed(x), UnZoomed(y));
        Invalidate(FALSE);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "");
    }
    ReleaseCapture();
    return 0;
}

LRESULT CImgAreaWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LONG xNow = UnZoomed(GET_X_LPARAM(lParam));
    LONG yNow = UnZoomed(GET_Y_LPARAM(lParam));
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
            strCoord.Format(_T("%ld, %ld"), xNow, yNow);
            SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) (LPCTSTR) strCoord);
        }
    }
    if (drawing)
    {
        /* values displayed in statusbar */
        LONG xRel = xNow - start.x;
        LONG yRel = yNow - start.y;
        /* freesel, rectsel and text tools always show numbers limited to fit into image area */
        if ((toolsModel.GetActiveTool() == TOOL_FREESEL) || (toolsModel.GetActiveTool() == TOOL_RECTSEL) || (toolsModel.GetActiveTool() == TOOL_TEXT))
        {
            if (xRel < 0)
                xRel = (xNow < 0) ? -start.x : xRel;
            else if (xNow > imageModel.GetWidth())
                xRel = imageModel.GetWidth() - start.x;
            if (yRel < 0)
                yRel = (yNow < 0) ? -start.y : yRel;
            else if (yNow > imageModel.GetHeight())
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
                strCoord.Format(_T("%ld, %ld"), xNow, yNow);
                SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) (LPCTSTR) strCoord);
                break;
            }
            default:
                break;
        }
        if (wParam & MK_LBUTTON)
        {
            toolsModel.OnMouseMove(TRUE, xNow, yNow);
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
            toolsModel.OnMouseMove(FALSE, xNow, yNow);
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
    updateCanvasAndScrollbars();
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
