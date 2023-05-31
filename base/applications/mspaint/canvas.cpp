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
    : m_drawing(FALSE)
    , m_hitSelection(HIT_NONE)
    , m_whereHit(HIT_NONE)
    , m_ptOrig { -1, -1 }
{
    ::SetRectEmpty(&m_rcNew);
}

VOID CCanvasWindow::drawZoomFrame(INT mouseX, INT mouseY)
{
    // FIXME: Draw the border of the area that is to be zoomed in
    CRect rc;
    GetImageRect(rc);
    ImageToCanvas(rc);

    HDC hdc = GetDC();
    DrawXorRect(hdc, &rc);
    ReleaseDC(hdc);
}

RECT CCanvasWindow::GetBaseRect()
{
    CRect rcBase;
    GetImageRect(rcBase);
    ImageToCanvas(rcBase);
    ::InflateRect(&rcBase, GRIP_SIZE, GRIP_SIZE);
    return rcBase;
}

VOID CCanvasWindow::ImageToCanvas(POINT& pt)
{
    pt.x = Zoomed(pt.x);
    pt.y = Zoomed(pt.y);
    pt.x += GRIP_SIZE - GetScrollPos(SB_HORZ);
    pt.y += GRIP_SIZE - GetScrollPos(SB_VERT);
}

VOID CCanvasWindow::ImageToCanvas(RECT& rc)
{
    rc.left = Zoomed(rc.left);
    rc.top = Zoomed(rc.top);
    rc.right = Zoomed(rc.right);
    rc.bottom = Zoomed(rc.bottom);
    ::OffsetRect(&rc, GRIP_SIZE - GetScrollPos(SB_HORZ), GRIP_SIZE - GetScrollPos(SB_VERT));
}

VOID CCanvasWindow::CanvasToImage(POINT& pt, BOOL bZoomed)
{
    pt.x -= GRIP_SIZE - GetScrollPos(SB_HORZ);
    pt.y -= GRIP_SIZE - GetScrollPos(SB_VERT);
    if (bZoomed)
        return;
    pt.x = UnZoomed(pt.x);
    pt.y = UnZoomed(pt.y);
}

VOID CCanvasWindow::CanvasToImage(RECT& rc, BOOL bZoomed)
{
    ::OffsetRect(&rc, GetScrollPos(SB_HORZ) - GRIP_SIZE, GetScrollPos(SB_VERT) - GRIP_SIZE);
    if (bZoomed)
        return;
    rc.left = UnZoomed(rc.left);
    rc.top = UnZoomed(rc.top);
    rc.right = UnZoomed(rc.right);
    rc.bottom = UnZoomed(rc.bottom);
}

VOID CCanvasWindow::GetImageRect(RECT& rc)
{
    ::SetRect(&rc, 0, 0, imageModel.GetWidth(), imageModel.GetHeight());
}

CANVAS_HITTEST CCanvasWindow::CanvasHitTest(POINT pt)
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

    // Draw the sizeboxes if necessary
    RECT rcBase = GetBaseRect();
    if (!selectionModel.m_bShow)
        drawSizeBoxes(hdcMem, &rcBase, FALSE, &rcPaint);

    // Draw the image
    CRect rcImage;
    GetImageRect(rcImage);
    ImageToCanvas(rcImage);
    SIZE sizeImage = { imageModel.GetWidth(), imageModel.GetHeight() };
    StretchBlt(hdcMem, rcImage.left, rcImage.top, rcImage.Width(), rcImage.Height(),
               imageModel.GetDC(), 0, 0, sizeImage.cx, sizeImage.cy, SRCCOPY);

    // Draw the grid
    if (showGrid && toolsModel.GetZoom() >= 4000)
    {
        HPEN oldPen = (HPEN) SelectObject(hdcMem, CreatePen(PS_SOLID, 1, RGB(160, 160, 160)));
        for (INT counter = 0; counter < sizeImage.cy; counter++)
        {
            POINT pt0 = { 0, counter }, pt1 = { sizeImage.cx, counter };
            ImageToCanvas(pt0);
            ImageToCanvas(pt1);
            ::MoveToEx(hdcMem, pt0.x, pt0.y, NULL);
            ::LineTo(hdcMem, pt1.x, pt1.y);
        }
        for (INT counter = 0; counter < sizeImage.cx; counter++)
        {
            POINT pt0 = { counter, 0 }, pt1 = { counter, sizeImage.cy };
            ImageToCanvas(pt0);
            ImageToCanvas(pt1);
            ::MoveToEx(hdcMem, pt0.x, pt0.y, NULL);
            ::LineTo(hdcMem, pt1.x, pt1.y);
        }
        ::DeleteObject(::SelectObject(hdcMem, oldPen));
    }

    // Draw selection
    if (selectionModel.m_bShow)
    {
        RECT rcSelection = selectionModel.m_rc;
        ImageToCanvas(rcSelection);

        ::InflateRect(&rcSelection, GRIP_SIZE, GRIP_SIZE);
        drawSizeBoxes(hdcMem, &rcSelection, TRUE, &rcPaint);
        ::InflateRect(&rcSelection, -GRIP_SIZE, -GRIP_SIZE);

        INT iSaveDC = ::SaveDC(hdcMem);
        ::IntersectClipRect(hdcMem, rcImage.left, rcImage.top, rcImage.right, rcImage.bottom);
        selectionModel.DrawSelection(hdcMem, &rcSelection, paletteModel.GetBgColor(),
                                     toolsModel.IsBackgroundTransparent());
        ::RestoreDC(hdcMem, iSaveDC);
    }

    // Draw new frame if any
    if (m_whereHit != HIT_NONE && !::IsRectEmpty(&m_rcNew))
        DrawXorRect(hdcMem, &m_rcNew);

    // Transfer the bits
    ::BitBlt(hDC,
             rcPaint.left, rcPaint.top,
             rcPaint.right - rcPaint.left, rcPaint.bottom - rcPaint.top,
             hdcMem, rcPaint.left, rcPaint.top, SRCCOPY);

    ::DeleteObject(::SelectObject(hdcMem, hbmOld));
    ::DeleteDC(hdcMem);
}

VOID CCanvasWindow::Update(HWND hwndFrom)
{
    CRect rcClient;
    GetClientRect(&rcClient);

    CSize sizePage(rcClient.right, rcClient.bottom);
    CSize sizeZoomed = { Zoomed(imageModel.GetWidth()), Zoomed(imageModel.GetHeight()) };
    CSize sizeWhole = { sizeZoomed.cx + (GRIP_SIZE * 2), sizeZoomed.cy + (GRIP_SIZE * 2) };

    // show/hide the scrollbars
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

LRESULT CCanvasWindow::OnLRButtonDown(BOOL bLeftButton, UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

    CANVAS_HITTEST hitSelection = SelectionHitTest(pt);
    if (hitSelection != HIT_NONE)
    {
        if (bLeftButton)
        {
            UnZoomed(pt);
            StartSelectionDrag(hitSelection, pt);
        }
        return 0;
    }

    CANVAS_HITTEST hit = CanvasHitTest(pt);
    if (hit == HIT_NONE || hit == HIT_BORDER)
    {
        switch (toolsModel.GetActiveTool())
        {
            case TOOL_BEZIER:
            case TOOL_SHAPE:
                toolsModel.OnCancelDraw();
                canvasWindow.Invalidate();
                break;

            case TOOL_FREESEL:
            case TOOL_RECTSEL:
                toolsModel.OnFinishDraw();
                canvasWindow.Invalidate();
                break;

            default:
                break;
        }

        toolsModel.resetTool(); // resets the point-buffer of the polygon and bezier functions
        return 0;
    }

    CanvasToImage(pt, TRUE);

    if (hit == HIT_INNER)
    {
        m_drawing = TRUE;
        UnZoomed(pt);
        SetCapture();
        toolsModel.OnButtonDown(bLeftButton, pt.x, pt.y, FALSE);
        Invalidate(FALSE);
        return 0;
    }

    if (bLeftButton)
    {
        m_whereHit = hit;
        UnZoomed(pt);
        m_ptOrig = pt;
        SetCapture();
    }
    return 0;
}

LRESULT CCanvasWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return OnLRButtonDown(TRUE, nMsg, wParam, lParam, bHandled);
}

LRESULT CCanvasWindow::OnRButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return OnLRButtonDown(FALSE, nMsg, wParam, lParam, bHandled);
}

LRESULT CCanvasWindow::OnLRButtonDblClk(BOOL bLeftButton, UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    CanvasToImage(pt);

    m_drawing = FALSE;
    ReleaseCapture();

    toolsModel.OnButtonDown(bLeftButton, pt.x, pt.y, TRUE);
    toolsModel.resetTool();
    Invalidate(FALSE);
    return 0;
}

LRESULT CCanvasWindow::OnLButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return OnLRButtonDblClk(TRUE, nMsg, wParam, lParam, bHandled);
}

LRESULT CCanvasWindow::OnRButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return OnLRButtonDblClk(FALSE, nMsg, wParam, lParam, bHandled);
}

LRESULT CCanvasWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    CanvasToImage(pt);

    if (m_hitSelection != HIT_NONE)
    {
        SelectionDragging(pt);
        return 0;
    }

    if (!m_drawing || toolsModel.GetActiveTool() <= TOOL_AIRBRUSH)
    {
        if (toolsModel.GetActiveTool() == TOOL_ZOOM)
        {
            Invalidate(FALSE);
            UpdateWindow();
            CanvasToImage(pt);
            drawZoomFrame(pt.x, pt.y);
        }

        TRACKMOUSEEVENT tme = { sizeof(tme) };
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        tme.dwHoverTime = 0;
        ::TrackMouseEvent(&tme);

        if (!m_drawing)
        {
            CString strCoord;
            strCoord.Format(_T("%ld, %ld"), pt.x, pt.y);
            SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM) (LPCTSTR) strCoord);
        }
    }

    if (m_drawing)
    {
        // values displayed in statusbar
        LONG xRel = pt.x - start.x;
        LONG yRel = pt.y - start.y;

        switch (toolsModel.GetActiveTool())
        {
            // freesel, rectsel and text tools always show numbers limited to fit into image area
            case TOOL_FREESEL:
            case TOOL_RECTSEL:
            case TOOL_TEXT:
                if (xRel < 0)
                    xRel = (pt.x < 0) ? -start.x : xRel;
                else if (pt.x > imageModel.GetWidth())
                    xRel = imageModel.GetWidth() - start.x;
                if (yRel < 0)
                    yRel = (pt.y < 0) ? -start.y : yRel;
                else if (pt.y > imageModel.GetHeight())
                    yRel = imageModel.GetHeight() - start.y;
                break;

            // while drawing, update cursor coordinates only for tools 3, 7, 8, 9, 14
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

        // rectsel and shape tools always show non-negative numbers when drawing
        if (toolsModel.GetActiveTool() == TOOL_RECTSEL || toolsModel.GetActiveTool() == TOOL_SHAPE)
        {
            if (xRel < 0)
                xRel = -xRel;
            if (yRel < 0)
                yRel =  -yRel;
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
        return 0;
    }

    if (m_whereHit == HIT_NONE || ::GetCapture() != m_hWnd)
        return 0;

    // Dragging now... Calculate the new size
    INT cxImage = imageModel.GetWidth(), cyImage = imageModel.GetHeight();
    INT cxDelta = pt.x - m_ptOrig.x;
    INT cyDelta = pt.y - m_ptOrig.y;
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

    CRect rc = { 0, 0, cxImage, cyImage };
    switch (m_whereHit)
    {
        case HIT_UPPER_LEFT:
            ::OffsetRect(&rc, cxDelta, cyDelta);
            break;
        case HIT_UPPER_CENTER:
            ::OffsetRect(&rc, 0, cyDelta);
            break;
        case HIT_UPPER_RIGHT:
            ::OffsetRect(&rc, 0, cyDelta);
            break;
        case HIT_MIDDLE_LEFT:
            ::OffsetRect(&rc, cxDelta, 0);
            break;
        case HIT_LOWER_LEFT:
            ::OffsetRect(&rc, cxDelta, 0);
            break;
        default:
            break;
    }
    ImageToCanvas(rc);
    m_rcNew = rc;
    Invalidate(TRUE);

    return 0;
}

LRESULT CCanvasWindow::OnLRButtonUp(BOOL bLeftButton, UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    CanvasToImage(pt);

    ::ReleaseCapture();

    if (m_drawing)
    {
        m_drawing = FALSE;
        toolsModel.OnButtonUp(bLeftButton, pt.x, pt.y);
        Invalidate(FALSE);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) "");
        return 0;
    }
    else if (m_hitSelection != HIT_NONE && bLeftButton)
    {
        EndSelectionDrag(pt);
        return 0;
    }

    if (m_whereHit == HIT_NONE || !bLeftButton)
        return 0;

    // Resize the image
    INT cxImage = imageModel.GetWidth(), cyImage = imageModel.GetHeight();
    INT cxDelta = pt.x - m_ptOrig.x;
    INT cyDelta = pt.y - m_ptOrig.y;
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
    ::SetRectEmpty(&m_rcNew);

    imageSaved = FALSE;

    m_whereHit = HIT_NONE;
    toolsModel.resetTool(); // resets the point-buffer of the polygon and bezier functions
    Update(NULL);
    Invalidate(TRUE);
    return 0;
}

LRESULT CCanvasWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return OnLRButtonUp(TRUE, nMsg, wParam, lParam, bHandled);
}

LRESULT CCanvasWindow::OnRButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return OnLRButtonUp(FALSE, nMsg, wParam, lParam, bHandled);
}

LRESULT CCanvasWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt;
    ::GetCursorPos(&pt);
    ScreenToClient(&pt);

    CANVAS_HITTEST hitSelection = SelectionHitTest(pt);
    if (hitSelection != HIT_NONE)
    {
        if (!setCursorOnSizeBox(hitSelection))
            ::SetCursor(::LoadCursor(NULL, IDC_SIZEALL));
        return 0;
    }

    CRect rcImage;
    GetImageRect(rcImage);
    ImageToCanvas(rcImage);
    if (::PtInRect(&rcImage, pt))
    {
        switch (toolsModel.GetActiveTool())
        {
            case TOOL_FILL:
                ::SetCursor(::LoadIcon(hProgInstance, MAKEINTRESOURCE(IDC_FILL)));
                break;
            case TOOL_COLOR:
                ::SetCursor(::LoadIcon(hProgInstance, MAKEINTRESOURCE(IDC_COLOR)));
                break;
            case TOOL_ZOOM:
                ::SetCursor(::LoadIcon(hProgInstance, MAKEINTRESOURCE(IDC_ZOOM)));
                break;
            case TOOL_PEN:
                ::SetCursor(::LoadIcon(hProgInstance, MAKEINTRESOURCE(IDC_PEN)));
                break;
            case TOOL_AIRBRUSH:
                ::SetCursor(::LoadIcon(hProgInstance, MAKEINTRESOURCE(IDC_AIRBRUSH)));
                break;
            default:
                ::SetCursor(::LoadCursor(NULL, IDC_CROSS));
        }
        return 0;
    }

    if (selectionModel.m_bShow || !setCursorOnSizeBox(CanvasHitTest(pt)))
        ::SetCursor(::LoadCursor(NULL, IDC_ARROW));

    return 0;
}

LRESULT CCanvasWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE && ::GetCapture() == m_hWnd)
    {
        // Cancel dragging
        ::ReleaseCapture();
        m_whereHit = HIT_NONE;
        ::SetRectEmpty(&m_rcNew);
        Invalidate(TRUE);
    }

    return 0;
}

LRESULT CCanvasWindow::OnCancelMode(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Cancel dragging
    m_whereHit = HIT_NONE;
    ::SetRectEmpty(&m_rcNew);
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

VOID CCanvasWindow::cancelDrawing()
{
    selectionModel.ClearColor();
    selectionModel.ClearMask();
    m_hitSelection = HIT_NONE;
    m_drawing = FALSE;
    toolsModel.OnCancelDraw();
    Invalidate(FALSE);
}

VOID CCanvasWindow::finishDrawing()
{
    toolsModel.OnFinishDraw();
    m_drawing = FALSE;
    Invalidate(FALSE);
}

CANVAS_HITTEST CCanvasWindow::SelectionHitTest(POINT ptZoomed)
{
    if (!selectionModel.m_bShow)
        return HIT_NONE;

    RECT rcSelection = selectionModel.m_rc;
    Zoomed(rcSelection);
    ::OffsetRect(&rcSelection, GRIP_SIZE - GetScrollPos(SB_HORZ), GRIP_SIZE - GetScrollPos(SB_VERT));
    ::InflateRect(&rcSelection, GRIP_SIZE, GRIP_SIZE);

    return getSizeBoxHitTest(ptZoomed, &rcSelection);
}

VOID CCanvasWindow::StartSelectionDrag(CANVAS_HITTEST hit, POINT ptUnZoomed)
{
    m_hitSelection = hit;
    selectionModel.m_ptHit = ptUnZoomed;
    selectionModel.TakeOff();

    SetCapture();
    Invalidate(FALSE);
}

VOID CCanvasWindow::SelectionDragging(POINT ptUnZoomed)
{
    selectionModel.Dragging(m_hitSelection, ptUnZoomed);
    Invalidate(FALSE);
}

VOID CCanvasWindow::EndSelectionDrag(POINT ptUnZoomed)
{
    selectionModel.Dragging(m_hitSelection, ptUnZoomed);
    m_hitSelection = HIT_NONE;
    Invalidate(FALSE);
}

LRESULT CCanvasWindow::OnCtlColorEdit(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetTextColor((HDC)wParam, paletteModel.GetFgColor());
    SetBkMode((HDC)wParam, TRANSPARENT);
    return (LRESULT)GetStockObject(NULL_BRUSH);
}
