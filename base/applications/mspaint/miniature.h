/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/miniature.h
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

class CMiniatureWindow : public CWindowImpl<CMiniatureWindow>
{
public:
    DECLARE_WND_CLASS_EX(_T("MiniatureWindow"), CS_DBLCLKS, COLOR_BTNFACE)

    BEGIN_MSG_MAP(CMiniatureWindow)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
    END_MSG_MAP()

    HWND DoCreate(HWND hwndParent);

private:
    LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
