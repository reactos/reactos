/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window procedure of the main window and all children apart from
 *             hPalWin, hToolSettings and hSelection
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

#define TOOLBAR_ROWS        8
#define TOOLBAR_COLUMNS     2
#define CXY_TB_BUTTON       25
#define CX_TOOLBAR          (TOOLBAR_COLUMNS * CXY_TB_BUTTON)
#define CY_TOOLBAR          (TOOLBAR_ROWS * CXY_TB_BUTTON)

class CPaintToolBar : public CWindow
{
public:
    BOOL DoCreate(HWND hwndParent);
    static LRESULT CALLBACK ToolBarWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class CToolBox : public CWindowImpl<CToolBox>
{
public:
    DECLARE_WND_CLASS_EX(L"ToolBox", CS_DBLCLKS, COLOR_BTNFACE)

    BEGIN_MSG_MAP(CToolBox)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_SYSCOLORCHANGE, OnSysColorChange)
        MESSAGE_HANDLER(WM_COMMAND, OnCommand)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
        MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
        MESSAGE_HANDLER(WM_TOOLSMODELTOOLCHANGED, OnToolsModelToolChanged)
    END_MSG_MAP()

    BOOL DoCreate(HWND hwndParent);

private:
    CPaintToolBar toolbar;

    LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSysColorChange(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCommand(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonUp(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnToolsModelToolChanged(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
