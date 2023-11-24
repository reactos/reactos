/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Providing the canvas window class
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

class CCanvasWindow : public CWindowImpl<CCanvasWindow>
{
public:
    DECLARE_WND_CLASS_EX(L"ReactOSPaintCanvas", CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
                         COLOR_APPWORKSPACE)

    BEGIN_MSG_MAP(CCanvasWindow)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
        MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnButtonDown)
        MESSAGE_HANDLER(WM_RBUTTONDOWN, OnButtonDown)
        MESSAGE_HANDLER(WM_MBUTTONDOWN, OnButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnButtonDblClk)
        MESSAGE_HANDLER(WM_RBUTTONDBLCLK, OnButtonDblClk)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnButtonUp)
        MESSAGE_HANDLER(WM_RBUTTONUP, OnButtonUp)
        MESSAGE_HANDLER(WM_MBUTTONUP, OnButtonUp)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
        MESSAGE_HANDLER(WM_CANCELMODE, OnCancelMode)
        MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
        MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnCtlColorEdit)
        MESSAGE_HANDLER(WM_PALETTEMODELCOLORCHANGED, OnPaletteModelColorChanged)
    END_MSG_MAP()

    CCanvasWindow();
    virtual ~CCanvasWindow();

    BOOL m_drawing;

    VOID OnEndDraw(BOOL bCancel);
    VOID updateScrollRange();
    VOID updateScrollPos(INT x = 0, INT y = 0);

    VOID ImageToCanvas(POINT& pt);
    VOID ImageToCanvas(RECT& rc);
    VOID CanvasToImage(POINT& pt);
    VOID CanvasToImage(RECT& rc);
    VOID GetImageRect(RECT& rc);
    VOID getNewZoomRect(CRect& rcView, INT newZoom, CPoint ptTarget);
    VOID zoomTo(INT newZoom, LONG left = 0, LONG top = 0);

protected:
    HITTEST m_hitCanvasSizeBox;
    POINT m_ptOrig; // The origin of drag start
    HBITMAP m_ahbmCached[2]; // The cached buffer bitmaps
    CRect m_rcResizing; // Resizing rectagle

    HITTEST CanvasHitTest(POINT pt);
    RECT GetBaseRect();
    VOID DoDraw(HDC hDC, RECT& rcClient, RECT& rcPaint);
    VOID OnHVScroll(WPARAM wParam, INT fnBar);

    LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnVScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCancelMode(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCaptureChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtlColorEdit(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaletteModelColorChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    UINT m_nMouseDownMsg = 0;
    LRESULT OnButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
