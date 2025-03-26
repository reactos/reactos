/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window procedure of the main window and all children apart from
 *             hPalWin, hToolSettings and hSelection
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

class CMiniatureWindow : public CWindowImpl<CMiniatureWindow>
{
public:
    DECLARE_WND_CLASS_EX(L"MiniatureWindow", CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
                         COLOR_BTNFACE)

    BEGIN_MSG_MAP(CMiniatureWindow)
        MESSAGE_HANDLER(WM_MOVE, OnMove)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CLOSE, OnClose)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
    END_MSG_MAP()

    CMiniatureWindow();
    virtual ~CMiniatureWindow();

    HWND DoCreate(HWND hwndParent);

protected:
    HBITMAP m_hbmCached; // Cached buffer bitmap

    LRESULT OnMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetMinMaxInfo(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
