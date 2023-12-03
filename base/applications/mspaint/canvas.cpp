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
    , m_hitCanvasSizeBox(HIT_NONE)
    , m_ptOrig { -1, -1 }
{
    m_ahbmCached[0] = m_ahbmCached[1] = NULL;
    m_rcResizing.SetRectEmpty();
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
    rcBase.InflateRect(GRIP_SIZE, GRIP_SIZE);
    return rcBase;
}

VOID CCanvasWindow::ImageToCanvas(POINT& pt)
{
    Zoomed(pt);
    pt.x += GRIP_SIZE - GetScrollPos(SB_HORZ);
    pt.y += GRIP_SIZE - GetScrollPos(SB_VERT);
}

VOID CCanvasWindow::ImageToCanvas(RECT& rc)
{
    Zoomed(rc);
    ::OffsetRect(&rc, GRIP_SIZE - GetScrollPos(SB_HORZ), GRIP_SIZE - GetScrollPos(SB_VERT));
}

VOID CCanvasWindow::CanvasToImage(POINT& pt)
{
    pt.x -= GRIP_SIZE - GetScrollPos(SB_HORZ);
    pt.y -= GRIP_SIZE - GetScrollPos(SB_VERT);
    UnZoomed(pt);
}

VOID CCanvasWindow::CanvasToImage(RECT& rc)
{
    ::OffsetRect(&rc, GetScrollPos(SB_HORZ) - GRIP_SIZE, GetScrollPos(SB_VERT) - GRIP_SIZE);
    UnZoomed(rc);
}

VOID CCanvasWindow::GetImageRect(RECT& rc)
{
    rc = { 0, 0, imageModel.GetWidth(), imageModel.GetHeight() };
}

HITTEST CCanvasWindow::CanvasHitTest(POINT pt)
{
    if (selectionModel.m_bShow || ::IsWindowVisible(textEditWindow))
        return HIT_INNER;
    RECT rcBase = GetBaseRect();
    return getSizeBoxHitTest(pt, &rcBase);
}

VOID CCanvasWindow::getNewZoomRect(CRect& rcView, INT newZoom, CPoint ptTarget)
{
    CRect rcImage;
    GetImageRect(rcImage);
    ImageToCanvas(rcImage);

    // Calculate the zoom rectangle
    INT oldZoom = toolsModel.GetZoom();
    GetClientRect(rcView);
    LONG cxView = rcView.right * oldZoom / newZoom, cyView = rcView.bottom * oldZoom / newZoom;
    rcView.SetRect(ptTarget.x - cxView / 2, ptTarget.y - cyView / 2,
                   ptTarget.x + cxView / 2, ptTarget.y + cyView / 2);

    // Shift the rectangle if necessary
    INT dx = 0, dy = 0;
    if (rcView.left < rcImage.left)
        dx = rcImage.left - rcView.left;
    else if (rcImage.right < rcView.right)
        dx = rcImage.right - rcView.right;
    if (rcView.top < rcImage.top)
        dy = rcImage.top - rcView.top;
    else if (rcImage.bottom < rcView.bottom)
        dy = rcImage.bottom - rcView.bottom;
    rcView.OffsetRect(dx, dy);

    rcView.IntersectRect(&rcView, &rcImage);
}

VOID CCanvasWindow::zoomTo(INT newZoom, LONG left, LONG top)
{
    POINT pt = { left, top };
    CanvasToImage(pt);

    toolsModel.SetZoom(newZoom);
    ImageToCanvas(pt);
    pt.x += GetScrollPos(SB_HORZ);
    pt.y += GetScrollPos(SB_VERT);

    updateScrollRange();
    updateScrollPos(pt.x, pt.y);
    Invalidate(TRUE);
}

VOID CCanvasWindow::DoDraw(HDC hDC, RECT& rcClient, RECT& rcPaint)
{
    // This is the target area we have to draw on
    CRect rcCanvasDraw;
    rcCanvasDraw.IntersectRect(&rcClient, &rcPaint);

    // We use a memory bitmap to reduce flickering
    HDC hdcMem0 = ::CreateCompatibleDC(hDC);
    m_ahbmCached[0] = CachedBufferDIB(m_ahbmCached[0], rcClient.right, rcClient.bottom);
    HGDIOBJ hbm0Old = ::SelectObject(hdcMem0, m_ahbmCached[0]);

    // Fill the background on hdcMem0
    ::FillRect(hdcMem0, &rcCanvasDraw, (HBRUSH)(COLOR_APPWORKSPACE + 1));

    // Draw the sizeboxes if necessary
    RECT rcBase = GetBaseRect();
    if (!selectionModel.m_bShow && !::IsWindowVisible(textEditWindow))
        drawSizeBoxes(hdcMem0, &rcBase, FALSE, &rcCanvasDraw);

    // Calculate image size
    CRect rcImage;
    GetImageRect(rcImage);
    SIZE sizeImage = { imageModel.GetWidth(), imageModel.GetHeight() };

    // Calculate the target area on the image
    CRect rcImageDraw = rcCanvasDraw;
    CanvasToImage(rcImageDraw);
    rcImageDraw.IntersectRect(&rcImageDraw, &rcImage);

    // Consider rounding down by zooming
    rcImageDraw.right += 1;
    rcImageDraw.bottom += 1;

    // hdcMem1 <-- imageModel
    HDC hdcMem1 = ::CreateCompatibleDC(hDC);
    m_ahbmCached[1] = CachedBufferDIB(m_ahbmCached[1], sizeImage.cx, sizeImage.cy);
    HGDIOBJ hbm1Old = ::SelectObject(hdcMem1, m_ahbmCached[1]);
    ::BitBlt(hdcMem1, rcImageDraw.left, rcImageDraw.top, rcImageDraw.Width(), rcImageDraw.Height(),
             imageModel.GetDC(), rcImageDraw.left, rcImageDraw.top, SRCCOPY);

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
    if (m_hitCanvasSizeBox != HIT_NONE && !m_rcResizing.IsRectEmpty())
        DrawXorRect(hdcMem0, &m_rcResizing);

    // Transfer the bits (hDC <-- hdcMem0)
    ::BitBlt(hDC, rcCanvasDraw.left, rcCanvasDraw.top, rcCanvasDraw.Width(), rcCanvasDraw.Height(),
             hdcMem0, rcCanvasDraw.left, rcCanvasDraw.top, SRCCOPY);

    // Clean up hdcMem0
    ::SelectObject(hdcMem0, hbm0Old);
    ::DeleteDC(hdcMem0);
}

VOID CCanvasWindow::updateScrollRange()
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

VOID CCanvasWindow::updateScrollPos(INT x, INT y)
{
    SetScrollPos(SB_HORZ, x);
    SetScrollPos(SB_VERT, y);
}

LRESULT CCanvasWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (m_hWnd)
        updateScrollRange();

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
            si.nPos = (SHORT)HIWORD(wParam);
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
    si.nPos = max(min(si.nPos, si.nMax), si.nMin);
    SetScrollInfo(fnBar, &si);
    Invalidate();
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

LRESULT CCanvasWindow::OnButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

    m_nMouseDownMsg = nMsg;
    BOOL bLeftButton = (nMsg == WM_LBUTTONDOWN);

    if (nMsg == WM_MBUTTONDOWN)
    {
        m_ptOrig = pt;
        SetCapture();
        ::SetCursor(::LoadCursorW(g_hinstExe, MAKEINTRESOURCEW(IDC_HANDDRAG)));
        return 0;
    }

    HITTEST hitSelection = selectionModel.hitTest(pt);
    if (hitSelection != HIT_NONE)
    {
        m_drawing = TRUE;
        CanvasToImage(pt);
        SetCapture();
        toolsModel.OnButtonDown(bLeftButton, pt.x, pt.y, FALSE);
        Invalidate();
        return 0;
    }

    HITTEST hit = CanvasHitTest(pt);
    if (hit == HIT_NONE || hit == HIT_BORDER)
    {
        switch (toolsModel.GetActiveTool())
        {
            case TOOL_BEZIER:
            case TOOL_SHAPE:
                toolsModel.OnEndDraw(TRUE);
                Invalidate();
                break;

            case TOOL_FREESEL:
            case TOOL_RECTSEL:
                toolsModel.OnEndDraw(FALSE);
                Invalidate();
                break;

            default:
                break;
        }

        toolsModel.resetTool(); // resets the point-buffer of the polygon and bezier functions
        return 0;
    }

    CanvasToImage(pt);

    if (hit == HIT_INNER)
    {
        m_drawing = TRUE;
        SetCapture();
        toolsModel.OnButtonDown(bLeftButton, pt.x, pt.y, FALSE);
        Invalidate();
        return 0;
    }

    if (bLeftButton)
    {
        m_hitCanvasSizeBox = hit;
        m_ptOrig = pt;
        SetCapture();
    }

    return 0;
}

LRESULT CCanvasWindow::OnButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    CanvasToImage(pt);

    m_drawing = FALSE;
    ::ReleaseCapture();
    m_nMouseDownMsg = 0;

    toolsModel.OnButtonDown(nMsg == WM_LBUTTONDBLCLK, pt.x, pt.y, TRUE);
    toolsModel.resetTool();
    Invalidate();
    return 0;
}

LRESULT CCanvasWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

    if (m_nMouseDownMsg == WM_MBUTTONDOWN)
    {
        INT x = GetScrollPos(SB_HORZ) - (pt.x - m_ptOrig.x);
        INT y = GetScrollPos(SB_VERT) - (pt.y - m_ptOrig.y);
        SendMessage(WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, x), 0);
        SendMessage(WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, y), 0);
        m_ptOrig = pt;
        return 0;
    }

    CanvasToImage(pt);

    if (toolsModel.GetActiveTool() == TOOL_ZOOM)
        Invalidate();

    if (!m_drawing || toolsModel.GetActiveTool() <= TOOL_AIRBRUSH)
    {
        TRACKMOUSEEVENT tme = { sizeof(tme) };
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        tme.dwHoverTime = 0;
        ::TrackMouseEvent(&tme);

        if (!m_drawing)
        {
            CRect rcImage;
            GetImageRect(rcImage);

            CStringW strCoord;
            if (rcImage.PtInRect(pt))
                strCoord.Format(L"%ld, %ld", pt.x, pt.y);
            ::SendMessageW(g_hStatusBar, SB_SETTEXT, 1, (LPARAM)(LPCWSTR)strCoord);
        }
    }

    if (m_drawing || toolsModel.IsSelection())
    {
        toolsModel.DrawWithMouseTool(pt, wParam);
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
    CStringW strSize;
    strSize.Format(L"%d x %d", cxImage, cyImage);
    ::SendMessageW(g_hStatusBar, SB_SETTEXT, 2, (LPARAM)(LPCWSTR)strSize);

    // Dragging now... Fix the position...
    CRect rcResizing = { 0, 0, cxImage, cyImage };
    switch (m_hitCanvasSizeBox)
    {
        case HIT_UPPER_LEFT:
            rcResizing.OffsetRect(cxDelta, cyDelta);
            break;
        case HIT_UPPER_CENTER:
            rcResizing.OffsetRect(0, cyDelta);
            break;
        case HIT_UPPER_RIGHT:
            rcResizing.OffsetRect(0, cyDelta);
            break;
        case HIT_MIDDLE_LEFT:
            rcResizing.OffsetRect(cxDelta, 0);
            break;
        case HIT_LOWER_LEFT:
            rcResizing.OffsetRect(cxDelta, 0);
            break;
        default:
            break;
    }
    ImageToCanvas(rcResizing);
    m_rcResizing = rcResizing;
    Invalidate(TRUE);

    return 0;
}

LRESULT CCanvasWindow::OnButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    CanvasToImage(pt);

    ::ReleaseCapture();

    BOOL bLeftButton = (m_nMouseDownMsg == WM_LBUTTONDOWN);
    m_nMouseDownMsg = 0;

    if (m_drawing)
    {
        m_drawing = FALSE;
        toolsModel.OnButtonUp(bLeftButton, pt.x, pt.y);
        Invalidate(FALSE);
        ::SendMessageW(g_hStatusBar, SB_SETTEXT, 2, (LPARAM)L"");
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
    m_rcResizing.SetRectEmpty();

    g_imageSaved = FALSE;

    m_hitCanvasSizeBox = HIT_NONE;
    toolsModel.resetTool(); // resets the point-buffer of the polygon and bezier functions
    updateScrollRange();
    Invalidate(TRUE);
    return 0;
}

LRESULT CCanvasWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (CWaitCursor::IsWaiting())
    {
        bHandled = FALSE;
        return 0;
    }

    if (m_nMouseDownMsg == WM_MBUTTONDOWN)
    {
        ::SetCursor(::LoadCursorW(g_hinstExe, MAKEINTRESOURCEW(IDC_HANDDRAG)));
        return 0;
    }

    POINT pt;
    ::GetCursorPos(&pt);
    ScreenToClient(&pt);

    CRect rcClient;
    GetClientRect(&rcClient);

    if (!rcClient.PtInRect(pt))
    {
        bHandled = FALSE;
        return 0;
    }

    HITTEST hitSelection = selectionModel.hitTest(pt);
    if (hitSelection != HIT_NONE)
    {
        if (!setCursorOnSizeBox(hitSelection))
            ::SetCursor(::LoadCursorW(NULL, (LPCWSTR)IDC_SIZEALL));
        return 0;
    }

    CRect rcImage;
    GetImageRect(rcImage);
    ImageToCanvas(rcImage);

    if (rcImage.PtInRect(pt))
    {
        switch (toolsModel.GetActiveTool())
        {
            case TOOL_FILL:
                ::SetCursor(::LoadCursorW(g_hinstExe, MAKEINTRESOURCEW(IDC_FILL)));
                break;
            case TOOL_COLOR:
                ::SetCursor(::LoadCursorW(g_hinstExe, MAKEINTRESOURCEW(IDC_COLOR)));
                break;
            case TOOL_ZOOM:
                ::SetCursor(::LoadCursorW(g_hinstExe, MAKEINTRESOURCEW(IDC_ZOOM)));
                break;
            case TOOL_PEN:
                ::SetCursor(::LoadCursorW(g_hinstExe, MAKEINTRESOURCEW(IDC_PEN)));
                break;
            case TOOL_AIRBRUSH:
                ::SetCursor(::LoadCursorW(g_hinstExe, MAKEINTRESOURCEW(IDC_AIRBRUSH)));
                break;
            default:
                ::SetCursor(::LoadCursorW(NULL, (LPCWSTR)IDC_CROSS));
        }
        return 0;
    }

    if (selectionModel.m_bShow || !setCursorOnSizeBox(CanvasHitTest(pt)))
        bHandled = FALSE;

    return 0;
}

LRESULT CCanvasWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE)
    {
        OnEndDraw(TRUE);
        ::ReleaseCapture();
        m_nMouseDownMsg = 0;
        m_hitCanvasSizeBox = HIT_NONE;
        m_rcResizing.SetRectEmpty();
        Invalidate(TRUE);
    }

    return 0;
}

LRESULT CCanvasWindow::OnCancelMode(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Cancel dragging
    m_hitCanvasSizeBox = HIT_NONE;
    m_rcResizing.SetRectEmpty();
    Invalidate(TRUE);
    return 0;
}

LRESULT CCanvasWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return ::SendMessageW(GetParent(), nMsg, wParam, lParam);
}

LRESULT CCanvasWindow::OnCaptureChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ::SendMessageW(g_hStatusBar, SB_SETTEXT, 2, (LPARAM)L"");
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

VOID CCanvasWindow::OnEndDraw(BOOL bCancel)
{
    m_drawing = FALSE;
    toolsModel.OnEndDraw(bCancel);
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
