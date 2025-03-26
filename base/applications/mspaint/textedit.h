/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Text editor and font chooser for the text tool
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

#define CX_MINTEXTEDIT 100
#define CY_MINTEXTEDIT 24

class CTextEditWindow : public CWindowImpl<CTextEditWindow>
{
public:
    CTextEditWindow();

    HWND Create(HWND hwndParent);
    void DoFillBack(HWND hwnd, HDC hDC);
    void FixEditPos(LPCWSTR pszOldText);
    void InvalidateEditRect();
    void UpdateFont();
    BOOL GetEditRect(LPRECT prc) const;
    void ValidateEditRect(LPCRECT prc OPTIONAL);
    HFONT GetFont() const { return m_hFont; }

    BEGIN_MSG_MAP(CTextEditWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_TOOLSMODELTOOLCHANGED, OnToolsModelToolChanged)
        MESSAGE_HANDLER(WM_TOOLSMODELSETTINGSCHANGED, OnToolsModelSettingsChanged)
        MESSAGE_HANDLER(WM_TOOLSMODELZOOMCHANGED, OnToolsModelZoomChanged)
        MESSAGE_HANDLER(WM_PALETTEMODELCOLORCHANGED, OnPaletteModelColorChanged)
        MESSAGE_HANDLER(WM_MOVING, OnMoving)
        MESSAGE_HANDLER(WM_SIZING, OnSizing)
        MESSAGE_HANDLER(WM_CHAR, OnChar)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkGnd)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_NCPAINT, OnNCPaint)
        MESSAGE_HANDLER(WM_NCCALCSIZE, OnNCCalcSize);
        MESSAGE_HANDLER(WM_NCHITTEST, OnNCHitTest);
        MESSAGE_HANDLER(WM_SETCURSOR, OnSetCursor);
        MESSAGE_HANDLER(WM_MOVE, OnMove);
        MESSAGE_HANDLER(WM_SIZE, OnSize);
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown);
        MESSAGE_HANDLER(EM_SETSEL, OnSetSel);
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel);
        MESSAGE_HANDLER(WM_CUT, OnCut);
        MESSAGE_HANDLER(WM_PASTE, OnPaste);
        MESSAGE_HANDLER(WM_CLEAR, OnClear);
    END_MSG_MAP()

    LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelSettingsChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelZoomChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaletteModelColorChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMoving(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSizing(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnChar(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkGnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNCPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNCCalcSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNCHitTest(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetSel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCut(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaste(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClear(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    HWND m_hwndParent;
    HFONT m_hFont;
    HFONT m_hFontZoomed;
    RECT m_rc;

    INT DoHitTest(RECT& rc, POINT pt);
    void DrawGrip(HDC hDC, RECT& rc);
};
