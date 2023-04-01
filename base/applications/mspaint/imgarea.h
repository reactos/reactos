/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/imgarea.h
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 *              Katayama Hirofumi MZ
 */

#pragma once

class CImgAreaWindow : public CWindowImpl<CImgAreaWindow>
{
public:
    CImgAreaWindow()
        : drawing(FALSE)
        , m_hitSelection(HIT_NONE)
    {
    }

    BOOL drawing;
    CANVAS_HITTEST m_hitSelection;

    void cancelDrawing();
    void finishDrawing();

    DECLARE_WND_CLASS_EX(_T("ImgAreaWindow"), CS_DBLCLKS, COLOR_BTNFACE)

    BEGIN_MSG_MAP(CImgAreaWindow)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkGnd)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDblClk)
        MESSAGE_HANDLER(WM_RBUTTONDOWN, OnRButtonDown)
        MESSAGE_HANDLER(WM_RBUTTONDBLCLK, OnRButtonDblClk)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
        MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
        MESSAGE_HANDLER(WM_IMAGEMODELDIMENSIONSCHANGED, OnImageModelDimensionsChanged)
        MESSAGE_HANDLER(WM_IMAGEMODELIMAGECHANGED, OnImageModelImageChanged)
        MESSAGE_HANDLER(WM_CAPTURECHANGED, OnCaptureChanged)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_CTLCOLOREDIT, OnCtlColorEdit)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
    END_MSG_MAP()

private:
    HCURSOR m_hCurFill;
    HCURSOR m_hCurColor;
    HCURSOR m_hCurZoom;
    HCURSOR m_hCurPen;
    HCURSOR m_hCurAirbrush;

    LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkGnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRButtonDblClk(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnRButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseLeave(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnImageModelDimensionsChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnImageModelImageChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCaptureChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCtlColorEdit(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    void drawZoomFrame(int mouseX, int mouseY);
    CANVAS_HITTEST SelectionHitTest(POINT ptZoomed);
    void StartSelectionDrag(CANVAS_HITTEST hit, POINT ptUnZoomed);
    void SelectionDragging(POINT ptUnZoomed);
    void EndSelectionDrag(POINT ptUnZoomed);
};
