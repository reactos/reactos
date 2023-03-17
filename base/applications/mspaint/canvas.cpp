/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/canvas.cpp
 * PURPOSE:     Providing the canvas window class
 * PROGRAMMERS: Benedikt Freisen
 */

#include "precomp.h"

/* FUNCTIONS ********************************************************/

CCanvasWindow::CCanvasWindow()
    : m_bDragging(FALSE)
    , m_whereDragging(SIZEBOX_NONE)
    , m_bShowSizeBoxes(TRUE)
{
    m_ptOrig.x = m_ptOrig.y = 0;
}

VOID CCanvasWindow::ShowSizeBoxes(BOOL bShow)
{
    m_bShowSizeBoxes = bShow;
    Invalidate(TRUE);
}

BOOL CCanvasWindow::GetHitTestRect(LPRECT prc, SIZEBOX_HITTEST sht, LPCRECT prcBase, BOOL bSetCursor)
{
    RECT rcSizeBox;
    if (!getSizeBoxRect(&rcSizeBox, sht, prcBase, bSetCursor))
    {
        ::SetRectEmpty(prc);
        return FALSE;
    }

    *prc = rcSizeBox;
    return TRUE;
}

SIZEBOX_HITTEST CCanvasWindow::HitTest(POINT pt, BOOL bSetCursor)
{
    RECT rc, rcBase = GetBaseRect();
    for (INT i = SIZEBOX_UPPER_LEFT; i <= SIZEBOX_MAX; ++i)
    {
        SIZEBOX_HITTEST sht = (SIZEBOX_HITTEST)i;
        GetHitTestRect(&rc, sht, &rcBase, bSetCursor);
        if (::PtInRect(&rc, pt))
            return sht;
    }

    if (::PtInRect(&rc, pt))
        return SIZEBOX_CONTENTS;

    return SIZEBOX_NONE;
}

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

    if (imageArea.IsWindow() && hwndFrom != imageArea.m_hWnd)
    {
        INT dx = -GetScrollPos(SB_HORZ), dy = -GetScrollPos(SB_VERT);
        imageArea.MoveWindow(dx + GRIP_SIZE, dy + GRIP_SIZE, sizeZoomed.cx, sizeZoomed.cy, TRUE);
    }
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
    Invalidate(FALSE); // FIXME: Flicker
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
    Invalidate(FALSE); // FIXME: Flicker
    return 0;
}

LRESULT CCanvasWindow::OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    SIZEBOX_HITTEST sht = HitTest(pt, TRUE);

    if (sht == SIZEBOX_NONE)
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

    if (sht == SIZEBOX_CONTENTS)
    {
        // TODO:
        return 0;
    }

    if (m_bShowSizeBoxes)
    {
        m_bDragging = TRUE;
        m_whereDragging = sht;
        m_ptOrig = pt;
        SetCapture();
        SetFocus();
    }

    return 0;
}

LRESULT CCanvasWindow::OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (!m_bDragging || ::GetCapture() != m_hWnd)
        return 0;

    CString strSize;
    INT imgXRes = imageModel.GetWidth(), imgYRes = imageModel.GetHeight();
    INT xRel = UnZoomed(GET_X_LPARAM(lParam) - m_ptOrig.x);
    INT yRel = UnZoomed(GET_Y_LPARAM(lParam) - m_ptOrig.y);

    switch (m_whereDragging)
    {
        case SIZEBOX_UPPER_LEFT:
            strSize.Format(_T("%d x %d"), imgXRes - xRel, imgYRes - yRel);
            break;
        case SIZEBOX_UPPER_CENTER:
            strSize.Format(_T("%d x %d"), imgXRes, imgYRes - yRel);
            break;
        case SIZEBOX_UPPER_RIGHT:
            strSize.Format(_T("%d x %d"), imgXRes + xRel, imgYRes - yRel);
            break;
        case SIZEBOX_MIDDLE_LEFT:
            strSize.Format(_T("%d x %d"), imgXRes - xRel, imgYRes);
            break;
        case SIZEBOX_MIDDLE_RIGHT:
            strSize.Format(_T("%d x %d"), imgXRes + xRel, imgYRes);
            break;
        case SIZEBOX_LOWER_LEFT:
            strSize.Format(_T("%d x %d"), imgXRes - xRel, imgYRes + yRel);
            break;
        case SIZEBOX_LOWER_CENTER:
            strSize.Format(_T("%d x %d"), imgXRes, imgYRes + yRel);
            break;
        case SIZEBOX_LOWER_RIGHT:
            strSize.Format(_T("%d x %d"), imgXRes + xRel, imgYRes + yRel);
            break;
        default:
            break;
    }

    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM) (LPCTSTR) strSize);

    return 0;
}

LRESULT CCanvasWindow::OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ::ReleaseCapture();

    if (!m_bDragging)
        return 0;

    INT imgXRes = imageModel.GetWidth(), imgYRes = imageModel.GetHeight();
    INT xRel = (GET_X_LPARAM(lParam) - m_ptOrig.x) * 1000 / toolsModel.GetZoom();
    INT yRel = (GET_Y_LPARAM(lParam) - m_ptOrig.y) * 1000 / toolsModel.GetZoom();
    switch (m_whereDragging)
    {
        case SIZEBOX_UPPER_LEFT:
            imageModel.Crop(imgXRes - xRel, imgYRes - yRel, xRel, yRel);
            break;
        case SIZEBOX_UPPER_CENTER:
            imageModel.Crop(imgXRes, imgYRes - yRel, 0, yRel);
            break;
        case SIZEBOX_UPPER_RIGHT:
            imageModel.Crop(imgXRes + xRel, imgYRes - yRel, 0, yRel);
            break;
        case SIZEBOX_MIDDLE_LEFT:
            imageModel.Crop(imgXRes - xRel, imgYRes, xRel, 0);
            break;
        case SIZEBOX_MIDDLE_RIGHT:
            imageModel.Crop(imgXRes + xRel, imgYRes, 0, 0);
            break;
        case SIZEBOX_LOWER_LEFT:
            imageModel.Crop(imgXRes - xRel, imgYRes + yRel, xRel, 0);
            break;
        case SIZEBOX_LOWER_CENTER:
            imageModel.Crop(imgXRes, imgYRes + yRel, 0, 0);
            break;
        case SIZEBOX_LOWER_RIGHT:
            imageModel.Crop(imgXRes + xRel, imgYRes + yRel, 0, 0);
            break;
        default:
            break;
    }

    m_bDragging = FALSE;
    m_whereDragging = SIZEBOX_NONE;

    toolsModel.resetTool(); // resets the point-buffer of the polygon and bezier functions
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)_T(""));

    Update(NULL);
    Invalidate(TRUE);

    return 0;
}

LRESULT CCanvasWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    POINT pt;
    ::GetCursorPos(&pt);
    ScreenToClient(&pt);

    if (HitTest(pt, TRUE) == SIZEBOX_NONE)
        bHandled = FALSE;

    return 0;
}

LRESULT CCanvasWindow::OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == VK_ESCAPE && ::GetCapture() == m_hWnd)
    {
        m_bDragging = FALSE;
        m_whereDragging = SIZEBOX_NONE;
        ::ReleaseCapture();
    }

    return 0;
}

LRESULT CCanvasWindow::OnCancelMode(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    m_bDragging = FALSE;
    m_whereDragging = SIZEBOX_NONE;
    Invalidate(TRUE);
    return 0;
}

LRESULT CCanvasWindow::OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return ::SendMessage(GetParent(), nMsg, wParam, lParam);
}

LRESULT CCanvasWindow::OnCaptureChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    mainWindow.SetFocus();
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
    OnDraw(hDC, rcClient, ps.rcPaint);
    EndPaint(&ps);
    return 0;
}

RECT CCanvasWindow::GetBaseRect()
{
    CRect rcBase = { 0, 0, Zoomed(imageModel.GetWidth()), Zoomed(imageModel.GetHeight()) };
    ::InflateRect(&rcBase, GRIP_SIZE, GRIP_SIZE);
    ::OffsetRect(&rcBase, GRIP_SIZE - GetScrollPos(SB_HORZ), GRIP_SIZE - GetScrollPos(SB_VERT));
    return rcBase;
}

VOID CCanvasWindow::OnDraw(HDC hDC, RECT& rcClient, RECT& rcPaint)
{
    HDC hdcMem = ::CreateCompatibleDC(hDC);
    HBITMAP hbm = ::CreateCompatibleBitmap(hDC, rcClient.right, rcClient.bottom);
    HGDIOBJ hbmOld = ::SelectObject(hdcMem, hbm);

    ::FillRect(hdcMem, &rcPaint, (HBRUSH)(COLOR_APPWORKSPACE + 1));

    if (m_bShowSizeBoxes)
    {
        RECT rcBase = GetBaseRect();
        drawSizeBoxes(hdcMem, &rcBase, FALSE, &rcPaint);
    }

    // Transfer the bits
    ::BitBlt(hDC,
             rcPaint.left, rcPaint.top,
             rcPaint.right - rcPaint.left, rcPaint.bottom - rcPaint.top,
             hdcMem, rcPaint.left, rcPaint.top, SRCCOPY);

    ::SelectObject(hdcMem, hbmOld);
    ::DeleteDC(hdcMem);
}
