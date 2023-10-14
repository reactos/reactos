/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Providing the canvas window class
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#include "precomp.h"

CCanvasWindow canvasWindow;

/* FUNCTIONS ********************************************************/

CCanvasWindow::CCanvasWindow()
    : m_drawing(FALSE)
    , m_hitSelection(HIT_NONE)
    , m_hitCanvasSizeBox(HIT_NONE)
    , m_ptOrig { -1, -1 }
{
    m_ahbmCached[0] = m_ahbmCached[1] = NULL;
    ::SetRectEmpty(&m_rcResizing);
}

CCanvasWindow::~CCanvasWindow()
{
    if (m_ahbmCached[0])
        ::DeleteObject(m_ahbmCached[0]);
    if (m_ahbmCached[1])
        ::DeleteObject(m_ahbmCached[1]);
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

HITTEST CCanvasWindow::CanvasHitTest(POINT pt)
{
    if (selectionModel.m_bShow || ::IsWindowVisible(textEditWindow))
        return HIT_INNER;
    RECT rcBase = GetBaseRect();
    return getSizeBoxHitTest(pt, &rcBase);
}

VOID CCanvasWindow::DoDraw(HDC hDC, RECT& rcClient, RECT& rcPaint)
{
    // Calculate the intersection on the canvas to reduce bits transfer
    CRect rcIntersectCanvas;
    rcIntersectCanvas.IntersectRect(&rcClient, &rcPaint);

    // We use a memory bitmap to reduce flickering
    HDC hdcMem0 = ::CreateCompatibleDC(hDC);
    m_ahbmCached[0] = CachedBufferDIB(m_ahbmCached[0], rcClient.right, rcClient.bottom);
    HGDIOBJ hbm0Old = ::SelectObject(hdcMem0, m_ahbmCached[0]);

    // Fill the background on hdcMem0
    ::FillRect(hdcMem0, &rcIntersectCanvas, (HBRUSH)(COLOR_APPWORKSPACE + 1));

    // Draw the sizeboxes if necessary
    RECT rcBase = GetBaseRect();
    if (!selectionModel.m_bShow && !::IsWindowVisible(textEditWindow))
        drawSizeBoxes(hdcMem0, &rcBase, FALSE, &rcIntersectCanvas);

    // Calculate image size
    CRect rcImage;
    GetImageRect(rcImage);
    SIZE sizeImage = { imageModel.GetWidth(), imageModel.GetHeight() };

    // Calculate the intersection on the image to reduce bits transfer
    CRect rcIntersectImage = rcIntersectCanvas;
    CanvasToImage(rcIntersectImage);

    // hdcMem1 <-- imageModel
    HDC hdcMem1 = ::CreateCompatibleDC(hDC);
    m_ahbmCached[1] = CachedBufferDIB(m_ahbmCached[1], sizeImage.cx, sizeImage.cy);
    HGDIOBJ hbm1Old = ::SelectObject(hdcMem1, m_ahbmCached[1]);
    ::BitBlt(hdcMem1, rcIntersectImage.left, rcIntersectImage.top,
                      rcIntersectImage.Width(), rcIntersectImage.Height(),
             imageModel.GetDC(), rcIntersectImage.left, rcIntersectImage.top, SRCCOPY);

    // Draw overlay #1 on hdcMem1
    toolsModel.OnDrawOverlayOnImage(hdcMem1);

    // Transfer the bits with stretch (hdcMem0 <-- hdcMem1)
    ImageToCanvas(rcImage);
    ::StretchBlt(hdcMem0, rcImage.left, rcImage.top, rcImage.Width(), rcImage.Height(),
                 hdcMem1, 0, 0, sizeImage.cx, sizeImage.cy, SRCCOPY);

    // Clean up hdcMem1
    ::SelectObject(hdcMem1, hbm1Old);
    ::DeleteDC(hdcMem1);

    // Draw the grid on hdcMem0
    if (g_showGrid && toolsModel.GetZoom() >= 4000)
    {
        HPEN oldPen = (HPEN) ::SelectObject(hdcMem0, ::CreatePen(PS_SOLID, 1, RGB(160, 160, 160)));
        for (INT counter = 0; counter < sizeImage.cy; counter++)
        {
            POINT pt0 = { 0, counter }, pt1 = { sizeImage.cx, counter };
            ImageToCanvas(pt0);
            ImageToCanvas(pt1);
            ::MoveToEx(hdcMem0, pt0.x, pt0.y, NULL);
            ::LineTo(hdcMem0, pt1.x, pt1.y);
        }
        for (INT counter = 0; counter < sizeImage.cx; counter++)
        {
            POINT pt0 = { counter, 0 }, pt1 = { counter, sizeImage.cy };
            ImageToCanvas(pt0);
            ImageToCanvas(pt1);
            ::MoveToEx(hdcMem0, pt0.x, pt0.y, NULL);
            ::LineTo(hdcMem0, pt1.x, pt1.y);
        }
        ::DeleteObject(::SelectObject(hdcMem0, oldPen));
    }

    // Draw overlay #2 on hdcMem0
    toolsModel.OnDrawOverlayOnCanvas(hdcMem0);

    // Draw new frame on hdcMem0 if any
    if (m_hitCanvasSizeBox != HIT_NONE && !::IsRectEmpty(&m_rcResizing))
        DrawXorRect(hdcMem0, &m_rcResizing);

    // Transfer the bits (hDC <-- hdcMem0)
    ::BitBlt(hDC,
             rcIntersectCanvas.left, rcIntersectCanvas.top,
             rcIntersectCanvas.Width(), rcIntersectCanvas.Height(),
             hdcMem0, rcIntersectCanvas.left, rcIntersectCanvas.top, SRCCOPY);

    // Clean up hdcMem0
    ::SelectObject(hdcMem0, hbm0Old);
    ::DeleteDC(hdcMem0);
}

VOID CCanvasWindow::updateScrollInfo()
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

VOID CCanvasWindow::resetScrollPos()
{
    SetScrollPos(SB_HORZ, 0);
    SetScrollPos(SB_VERT, 0);
}

LRESULT CCanvasWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_hWnd)
        updateScrollInfo();

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
    updateScrollInfo();
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

    HITTEST hitSelection = SelectionHitTest(pt);
    if (hitSelection != HIT_NONE)
    {
        selectionModel.m_nSelectionBrush = 0; // Selection Brush is OFF
        if (bLeftButton)
        {
            CanvasToImage(pt);
            if (::GetKeyState(VK_CONTROL) < 0) // Ctrl+Click is Selection Clone
            {
                imageModel.SelectionClone();
            }
            else if (::GetKeyState(VK_SHIFT) < 0) // Shift+Dragging is Selection Brush
            {
                selectionModel.m_nSelectionBrush = 1; // Selection Brush is ON
            }
            StartSelectionDrag(hitSelection, pt);
        }
        else
        {
            canvasWindow.ClientToScreen(&pt);
            mainWindow.TrackPopupMenu(pt, 0);
        }
        return 0;
    }

    HITTEST hit = CanvasHitTest(pt);
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
        m_hitCanvasSizeBox = hit;
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

    if (toolsModel.GetActiveTool() == TOOL_ZOOM)
        Invalidate();

    if (m_hitSelection != HIT_NONE)
    {
        SelectionDragging(pt);
        return 0;
    }

    if (!m_drawing || toolsModel.GetActiveTool() <= TOOL_AIRBRUSH)
    {
        TRACKMOUSEEVENT tme = { sizeof(tme) };
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        tme.dwHoverTime = 0;
        ::TrackMouseEvent(&tme);

        if (!m_drawing)
        {
            RECT rcImage;
            GetImageRect(rcImage);

            CString strCoord;
            if (::PtInRect(&rcImage, pt))
                strCoord.Format(_T("%ld, %ld"), pt.x, pt.y);
            ::SendMessage(g_hStatusBar, SB_SETTEXT, 1, (LPARAM) (LPCTSTR) strCoord);
        }
    }

    if (m_drawing)
    {
        // values displayed in statusbar
        LONG xRel = pt.x - g_ptStart.x;
        LONG yRel = pt.y - g_ptStart.y;

        switch (toolsModel.GetActiveTool())
        {
            // freesel, rectsel and text tools always show numbers limited to fit into image area
            case TOOL_FREESEL:
            case TOOL_RECTSEL:
            case TOOL_TEXT:
                if (xRel < 0)
                    xRel = (pt.x < 0) ? -g_ptStart.x : xRel;
                else if (pt.x > imageModel.GetWidth())
                    xRel = imageModel.GetWidth() - g_ptStart.x;
                if (yRel < 0)
                    yRel = (pt.y < 0) ? -g_ptStart.y : yRel;
                else if (pt.y > imageModel.GetHeight())
                    yRel = imageModel.GetHeight() - g_ptStart.y;
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
                ::SendMessage(g_hStatusBar, SB_SETTEXT, 1, (LPARAM) (LPCTSTR) strCoord);
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
            if ((toolsModel.GetActiveTool() >= TOOL_TEXT) || toolsModel.IsSelection())
            {
                CString strSize;
                if ((toolsModel.GetActiveTool() >= TOOL_LINE) && (GetAsyncKeyState(VK_SHIFT) < 0))
                    yRel = xRel;
                strSize.Format(_T("%ld x %ld"), xRel, yRel);
                ::SendMessage(g_hStatusBar, SB_SETTEXT, 2, (LPARAM) (LPCTSTR) strSize);
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
                ::SendMessage(g_hStatusBar, SB_SETTEXT, 2, (LPARAM) (LPCTSTR) strSize);
            }
        }
        return 0;
    }

    if (m_hitCanvasSizeBox == HIT_NONE || ::GetCapture() != m_hWnd)
        return 0;

    // Dragging now... Calculate the new size
    INT cxImage = imageModel.GetWidth(), cyImage = imageModel.GetHeight();
    INT cxDelta = pt.x - m_ptOrig.x;
    INT cyDelta = pt.y - m_ptOrig.y;
    switch (m_hitCanvasSizeBox)
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
    ::SendMessage(g_hStatusBar, SB_SETTEXT, 2, (LPARAM) (LPCTSTR) strSize);

    // Dragging now... Fix the position...
    CRect rcResizing = { 0, 0, cxImage, cyImage };
    switch (m_hitCanvasSizeBox)
    {
        case HIT_UPPER_LEFT:
            ::OffsetRect(&rcResizing, cxDelta, cyDelta);
            break;
        case HIT_UPPER_CENTER:
            ::OffsetRect(&rcResizing, 0, cyDelta);
            break;
        case HIT_UPPER_RIGHT:
            ::OffsetRect(&rcResizing, 0, cyDelta);
            break;
        case HIT_MIDDLE_LEFT:
            ::OffsetRect(&rcResizing, cxDelta, 0);
            break;
        case HIT_LOWER_LEFT:
            ::OffsetRect(&rcResizing, cxDelta, 0);
            break;
        default:
            break;
    }
    ImageToCanvas(rcResizing);
    m_rcResizing = rcResizing;
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
        ::SendMessage(g_hStatusBar, SB_SETTEXT, 2, (LPARAM)_T(""));
        return 0;
    }
    else if (m_hitSelection != HIT_NONE && bLeftButton)
    {
        EndSelectionDrag(pt);
        return 0;
    }

    if (m_hitCanvasSizeBox == HIT_NONE || !bLeftButton)
        return 0;

    // Resize the image
    INT cxImage = imageModel.GetWidth(), cyImage = imageModel.GetHeight();
    INT cxDelta = pt.x - m_ptOrig.x;
    INT cyDelta = pt.y - m_ptOrig.y;
    switch (m_hitCanvasSizeBox)
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
    ::SetRectEmpty(&m_rcResizing);

    g_imageSaved = FALSE;

    m_hitCanvasSizeBox = HIT_NONE;
    toolsModel.resetTool(); // resets the point-buffer of the polygon and bezier functions
    updateScrollInfo();
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
    if (CWaitCursor::IsWaiting())
    {
        bHandled = FALSE;
        return 0;
    }

    POINT pt;
    ::GetCursorPos(&pt);
    ScreenToClient(&pt);

    CRect rcClient;
    GetClientRect(&rcClient);

    if (!::PtInRect(&rcClient, pt))
    {
        bHandled = FALSE;
        return 0;
    }

    HITTEST hitSelection = SelectionHitTest(pt);
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
                ::SetCursor(::LoadIcon(g_hinstExe, MAKEINTRESOURCE(IDC_FILL)));
                break;
            case TOOL_COLOR:
                ::SetCursor(::LoadIcon(g_hinstExe, MAKEINTRESOURCE(IDC_COLOR)));
                break;
            case TOOL_ZOOM:
                ::SetCursor(::LoadIcon(g_hinstExe, MAKEINTRESOURCE(IDC_ZOOM)));
                break;
            case TOOL_PEN:
                ::SetCursor(::LoadIcon(g_hinstExe, MAKEINTRESOURCE(IDC_PEN)));
                break;
            case TOOL_AIRBRUSH:
                ::SetCursor(::LoadIcon(g_hinstExe, MAKEINTRESOURCE(IDC_AIRBRUSH)));
                break;
            default:
                ::SetCursor(::LoadCursor(NULL, IDC_CROSS));
        }
        return 0;
    }

    if (selectionModel.m_bShow || !setCursorOnSizeBox(CanvasHitTest(pt)))
        bHandled = FALSE;

    return 0;
}

LRESULT CCanvasWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE && ::GetCapture() == m_hWnd)
    {
        // Cancel dragging
        ::ReleaseCapture();
        m_hitCanvasSizeBox = HIT_NONE;
        ::SetRectEmpty(&m_rcResizing);
        Invalidate(TRUE);
    }

    return 0;
}

LRESULT CCanvasWindow::OnCancelMode(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Cancel dragging
    m_hitCanvasSizeBox = HIT_NONE;
    ::SetRectEmpty(&m_rcResizing);
    Invalidate(TRUE);
    return 0;
}

LRESULT CCanvasWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return ::SendMessage(GetParent(), nMsg, wParam, lParam);
}

LRESULT CCanvasWindow::OnCaptureChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ::SendMessage(g_hStatusBar, SB_SETTEXT, 2, (LPARAM)_T(""));
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
    selectionModel.ClearColorImage();
    selectionModel.ClearMaskImage();
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

HITTEST CCanvasWindow::SelectionHitTest(POINT ptImage)
{
    if (!selectionModel.m_bShow)
        return HIT_NONE;

    RECT rcSelection = selectionModel.m_rc;
    Zoomed(rcSelection);
    ::OffsetRect(&rcSelection, GRIP_SIZE - GetScrollPos(SB_HORZ), GRIP_SIZE - GetScrollPos(SB_VERT));
    ::InflateRect(&rcSelection, GRIP_SIZE, GRIP_SIZE);

    return getSizeBoxHitTest(ptImage, &rcSelection);
}

VOID CCanvasWindow::StartSelectionDrag(HITTEST hit, POINT ptImage)
{
    m_hitSelection = hit;
    selectionModel.m_ptHit = ptImage;
    selectionModel.TakeOff();

    SetCapture();
    Invalidate(FALSE);
}

VOID CCanvasWindow::SelectionDragging(POINT ptImage)
{
    if (selectionModel.m_nSelectionBrush)
    {
        imageModel.SelectionClone(selectionModel.m_nSelectionBrush == 1);
        selectionModel.m_nSelectionBrush = 2; // Selection Brush is ON and drawn
    }

    selectionModel.Dragging(m_hitSelection, ptImage);
    Invalidate(FALSE);
}

VOID CCanvasWindow::EndSelectionDrag(POINT ptImage)
{
    selectionModel.Dragging(m_hitSelection, ptImage);
    m_hitSelection = HIT_NONE;
    Invalidate(FALSE);
}

VOID CCanvasWindow::MoveSelection(INT xDelta, INT yDelta)
{
    if (!selectionModel.m_bShow)
        return;

    selectionModel.TakeOff();
    ::OffsetRect(&selectionModel.m_rc, xDelta, yDelta);
    Invalidate(FALSE);
}

LRESULT CCanvasWindow::OnCtlColorEdit(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetTextColor((HDC)wParam, paletteModel.GetFgColor());
    SetBkMode((HDC)wParam, TRANSPARENT);
    return (LRESULT)GetStockObject(NULL_BRUSH);
}

LRESULT CCanvasWindow::OnPaletteModelColorChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    imageModel.NotifyImageChanged();
    return 0;
}
