/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/imgarea.cpp
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

#include "dialogs.h"
#include "registry.h"

/* FUNCTIONS ********************************************************/

extern void
zoomTo(int newZoom, int mouseX, int mouseY);

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
    w = clientRectImageArea.right * clientRectScrollbox.right / (clientRectImageArea.right * 2);
    h = clientRectImageArea.bottom * clientRectScrollbox.bottom / (clientRectImageArea.bottom * 2);
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
    sizeboxLeftTop.MoveWindow(
               0,
               0, 3, 3, TRUE);
    sizeboxCenterTop.MoveWindow(
               imgXRes * toolsModel.GetZoom() / 2000 + 3 * 3 / 4,
               0, 3, 3, TRUE);
    sizeboxRightTop.MoveWindow(
               imgXRes * toolsModel.GetZoom() / 1000 + 3,
               0, 3, 3, TRUE);
    sizeboxLeftCenter.MoveWindow(
               0,
               imgYRes * toolsModel.GetZoom() / 2000 + 3 * 3 / 4, 3, 3, TRUE);
    sizeboxRightCenter.MoveWindow(
               imgXRes * toolsModel.GetZoom() / 1000 + 3,
               imgYRes * toolsModel.GetZoom() / 2000 + 3 * 3 / 4, 3, 3, TRUE);
    sizeboxLeftBottom.MoveWindow(
               0,
               imgYRes * toolsModel.GetZoom() / 1000 + 3, 3, 3, TRUE);
    sizeboxCenterBottom.MoveWindow(
               imgXRes * toolsModel.GetZoom() / 2000 + 3 * 3 / 4,
               imgYRes * toolsModel.GetZoom() / 1000 + 3, 3, 3, TRUE);
    sizeboxRightBottom.MoveWindow(
               imgXRes * toolsModel.GetZoom() / 1000 + 3,
               imgYRes * toolsModel.GetZoom() / 1000 + 3, 3, 3, TRUE);
    UpdateScrollbox();
    return 0;
}

LRESULT CImgAreaWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DefWindowProc(WM_PAINT, wParam, lParam);
    HDC hdc = imageArea.GetDC();
    StretchBlt(hdc, 0, 0, imgXRes * toolsModel.GetZoom() / 1000, imgYRes * toolsModel.GetZoom() / 1000, hDrawingDC, 0, 0, imgXRes,
               imgYRes, SRCCOPY);
    if (showGrid && (toolsModel.GetZoom() >= 4000))
    {
        HPEN oldPen = (HPEN) SelectObject(hdc, CreatePen(PS_SOLID, 1, 0x00a0a0a0));
        int counter;
        for(counter = 0; counter <= imgYRes; counter++)
        {
            MoveToEx(hdc, 0, counter * toolsModel.GetZoom() / 1000, NULL);
            LineTo(hdc, imgXRes * toolsModel.GetZoom() / 1000, counter * toolsModel.GetZoom() / 1000);
        }
        for(counter = 0; counter <= imgXRes; counter++)
        {
            MoveToEx(hdc, counter * toolsModel.GetZoom() / 1000, 0, NULL);
            LineTo(hdc, counter * toolsModel.GetZoom() / 1000, imgYRes * toolsModel.GetZoom() / 1000);
        }
        DeleteObject(SelectObject(hdc, oldPen));
    }
    imageArea.ReleaseDC(hdc);
    selectionWindow.Invalidate(FALSE);
    miniature.Invalidate(FALSE);
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
    if ((!drawing) || (toolsModel.GetActiveTool() == TOOL_COLOR))
    {
        SetCapture();
        drawing = TRUE;
        startPaintingL(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / toolsModel.GetZoom(), GET_Y_LPARAM(lParam) * 1000 / toolsModel.GetZoom(),
                       paletteModel.GetFgColor(), paletteModel.GetBgColor());
    }
    else
    {
        SendMessage(WM_LBUTTONUP, wParam, lParam);
        undo();
    }
    Invalidate(FALSE);
    if ((toolsModel.GetActiveTool() == TOOL_ZOOM) && (toolsModel.GetZoom() < 8000))
        zoomTo(toolsModel.GetZoom() * 2, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    return 0;
}

LRESULT CImgAreaWindow::OnRButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if ((!drawing) || (toolsModel.GetActiveTool() == TOOL_COLOR))
    {
        SetCapture();
        drawing = TRUE;
        startPaintingR(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / toolsModel.GetZoom(), GET_Y_LPARAM(lParam) * 1000 / toolsModel.GetZoom(),
                       paletteModel.GetFgColor(), paletteModel.GetBgColor());
    }
    else
    {
        SendMessage(WM_RBUTTONUP, wParam, lParam);
        undo();
    }
    Invalidate(FALSE);
    if ((toolsModel.GetActiveTool() == TOOL_ZOOM) && (toolsModel.GetZoom() > 125))
        zoomTo(toolsModel.GetZoom() / 2, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    return 0;
}

LRESULT CImgAreaWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (drawing)
    {
        ReleaseCapture();
        drawing = FALSE;
        endPaintingL(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / toolsModel.GetZoom(), GET_Y_LPARAM(lParam) * 1000 / toolsModel.GetZoom(), paletteModel.GetFgColor(),
                     paletteModel.GetBgColor());
        Invalidate(FALSE);
        if (toolsModel.GetActiveTool() == TOOL_COLOR)
        {
            COLORREF tempColor =
                GetPixel(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / toolsModel.GetZoom(), GET_Y_LPARAM(lParam) * 1000 / toolsModel.GetZoom());
            if (tempColor != CLR_INVALID)
                paletteModel.SetFgColor(tempColor);
            paletteWindow.Invalidate(FALSE);
        }
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "");
    }
    return 0;
}

LRESULT CImgAreaWindow::OnRButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (drawing)
    {
        ReleaseCapture();
        drawing = FALSE;
        endPaintingR(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / toolsModel.GetZoom(), GET_Y_LPARAM(lParam) * 1000 / toolsModel.GetZoom(), paletteModel.GetFgColor(),
                     paletteModel.GetBgColor());
        Invalidate(FALSE);
        if (toolsModel.GetActiveTool() == TOOL_COLOR)
        {
            COLORREF tempColor =
                GetPixel(hDrawingDC, GET_X_LPARAM(lParam) * 1000 / toolsModel.GetZoom(), GET_Y_LPARAM(lParam) * 1000 / toolsModel.GetZoom());
            if (tempColor != CLR_INVALID)
                paletteModel.SetBgColor(tempColor);
            paletteWindow.Invalidate(FALSE);
        }
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "");
    }
    return 0;
}

LRESULT CImgAreaWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LONG xNow = GET_X_LPARAM(lParam) * 1000 / toolsModel.GetZoom();
    LONG yNow = GET_Y_LPARAM(lParam) * 1000 / toolsModel.GetZoom();
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
        tme.hwndTrack = imageArea.m_hWnd;
        tme.dwHoverTime = 0;
        TrackMouseEvent(&tme);

        if (!drawing)
        {
            TCHAR coordStr[100];
            _stprintf(coordStr, _T("%ld, %ld"), xNow, yNow);
            SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) coordStr);
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
            else if (xNow > imgXRes)
                xRel = imgXRes-start.x;
            if (yRel < 0)
                yRel = (yNow < 0) ? -start.y : yRel;
            else if (yNow > imgYRes)
                 yRel = imgYRes-start.y;
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
                TCHAR coordStr[100];
                _stprintf(coordStr, _T("%ld, %ld"), xNow, yNow);
                SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) coordStr);
                break;
            }
        }
        if ((wParam & MK_LBUTTON) != 0)
        {
            whilePaintingL(hDrawingDC, xNow, yNow, paletteModel.GetFgColor(), paletteModel.GetBgColor());
            Invalidate(FALSE);
            if ((toolsModel.GetActiveTool() >= TOOL_TEXT) || (toolsModel.GetActiveTool() == TOOL_RECTSEL) || (toolsModel.GetActiveTool() == TOOL_FREESEL))
            {
                TCHAR sizeStr[100];
                if ((toolsModel.GetActiveTool() >= TOOL_LINE) && (GetAsyncKeyState(VK_SHIFT) < 0))
                    yRel = xRel;
                _stprintf(sizeStr, _T("%ld x %ld"), xRel, yRel);
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) sizeStr);
            }
        }
        if ((wParam & MK_RBUTTON) != 0)
        {
            whilePaintingR(hDrawingDC, xNow, yNow, paletteModel.GetFgColor(), paletteModel.GetBgColor());
            Invalidate(FALSE);
            if (toolsModel.GetActiveTool() >= TOOL_TEXT)
            {
                TCHAR sizeStr[100];
                if ((toolsModel.GetActiveTool() >= TOOL_LINE) && (GetAsyncKeyState(VK_SHIFT) < 0))
                    yRel = xRel;
                _stprintf(sizeStr, _T("%ld x %ld"), xRel, yRel);
                SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) sizeStr);
            }
        }
    }
    return 0;
}

LRESULT CImgAreaWindow::OnMouseLeave(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) _T(""));
    if (toolsModel.GetActiveTool() == TOOL_ZOOM)
        imageArea.Invalidate(FALSE);
    return 0;
}
