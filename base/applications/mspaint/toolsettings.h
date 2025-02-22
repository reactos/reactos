/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window procedure of the tool settings window
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

class CToolSettingsWindow : public CWindowImpl<CToolSettingsWindow>
{
public:
    DECLARE_WND_CLASS_EX(L"ToolSettings", CS_DBLCLKS, COLOR_BTNFACE)

    BEGIN_MSG_MAP(CToolSettingsWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
        MESSAGE_HANDLER(WM_TOOLSMODELTOOLCHANGED, OnToolsModelToolChanged)
        MESSAGE_HANDLER(WM_TOOLSMODELSETTINGSCHANGED, OnToolsModelSettingsChanged)
        MESSAGE_HANDLER(WM_TOOLSMODELZOOMCHANGED, OnToolsModelZoomChanged)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()

    BOOL DoCreate(HWND hwndParent);

private:
    CWindow trackbarZoom;
    HICON m_hNontranspIcon;
    HICON m_hTranspIcon;

    VOID drawTrans(HDC hdc, LPCRECT prc);
    VOID drawRubber(HDC hdc, LPCRECT prc);
    VOID drawBrush(HDC hdc, LPCRECT prc);
    VOID drawLine(HDC hdc, LPCRECT prc);
    VOID drawBox(HDC hdc, LPCRECT prc);
    VOID drawAirBrush(HDC hdc, LPCRECT prc);
    VOID calculateTwoBoxes(CRect& rect1, CRect& rect2);

    LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnVScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnNotify(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelSettingsChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelZoomChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
