/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/canvas.h
 * PURPOSE:     Providing the canvas window class
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

class CCanvasWindow : public CWindowImpl<CCanvasWindow>
{
public:
    DECLARE_WND_CLASS_EX(_T("ReactOSPaintCanvas"), 0, COLOR_APPWORKSPACE)

    BEGIN_MSG_MAP(CCanvasWindow)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
        MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
        MESSAGE_HANDLER(WM_CANCELMODE, OnCancelMode)
        MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
    END_MSG_MAP()

    CCanvasWindow();

    VOID Update(HWND hwndFrom);
    VOID ShowSizeBoxes(BOOL bShow);

protected:
    BOOL m_bDragging;
    SIZEBOX_HITTEST m_whereDragging;
    BOOL m_bShowSizeBoxes;
    POINT m_ptOrig;

    BOOL GetHitTestRect(LPRECT prc, SIZEBOX_HITTEST sht, LPCRECT prcBase, BOOL bSetCursor = FALSE);
    SIZEBOX_HITTEST HitTest(POINT pt, BOOL bSetCursor = FALSE);
    RECT GetBaseRect();
    VOID OnDraw(HDC hDC, RECT& rcClient, RECT& rcPaint);

    LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnVScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCancelMode(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCaptureChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
