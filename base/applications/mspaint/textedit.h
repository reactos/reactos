/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/textedit.h
 * PURPOSE:     Text editor and font chooser for the text tool
 * PROGRAMMERS: Benedikt Freisen
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
    void FixEditPos(LPCTSTR pszOldText);
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
    END_MSG_MAP()

    LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelSettingsChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelZoomChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaletteModelColorChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
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

protected:
    HWND m_hwndParent;
    HFONT m_hFont;
    HFONT m_hFontZoomed;
    LONG m_nAppIsMovingOrSizing;
    RECT m_rc;

    INT DoHitTest(RECT& rc, POINT pt);
    void DrawGrip(HDC hDC, RECT& rc);
    void Reposition();
};
