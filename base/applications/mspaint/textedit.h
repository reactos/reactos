/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/textedit.h
 * PURPOSE:     Text editor and font chooser for the text tool
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

class CTextEditWindow : public CWindowImpl<CTextEditWindow>
{
public:
    CTextEditWindow();
    ~CTextEditWindow();

    VOID Initialize();
    HWND Create(HWND hwndParent);
    void DoFillBack(HWND hwnd, HDC hDC);
    void DoDraw(HWND hwnd, HDC hDC);
    void InvalidateEdit(LPTSTR pszText);
    void InvalidateEdit2();

    BEGIN_MSG_MAP(CTextEditWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_TOOLSMODELTOOLCHANGED, OnToolsModelToolChanged)
        MESSAGE_HANDLER(WM_TOOLSMODELSETTINGSCHANGED, OnToolsModelSettingsChanged)
        MESSAGE_HANDLER(WM_PALETTEMODELCOLORCHANGED, OnPaletteModelColorChanged)
        MESSAGE_HANDLER(WM_CHAR, OnChar)
        MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
        MESSAGE_HANDLER(WM_KEYUP, OnKeyUp)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkGnd)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_NCPAINT, OnNCPaint)
        MESSAGE_HANDLER(WM_NCCALCSIZE, OnNCCalcSize);
        MESSAGE_HANDLER(WM_NCHITTEST, OnNCHitTest);
        MESSAGE_HANDLER(WM_MOVE, OnMoveSize);
        MESSAGE_HANDLER(WM_SIZE, OnMoveSize);
        MESSAGE_HANDLER(EM_SETSEL, OnSetSel);
    END_MSG_MAP()

    LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelSettingsChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaletteModelColorChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnChar(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnKeyUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkGnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNCPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNCCalcSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNCHitTest(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMoveSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetSel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

protected:
    HFONT m_hFont;
    LOGFONT m_lf;

    SIZE DoCalcRect(HDC hDC, LPTSTR pszText, INT cchText, LPRECT prcParent, LPCTSTR pszOldText);
    INT HitTestGrip(RECT& rc, POINT pt);
    void DrawGrip(HDC hDC, RECT& rc);
};
