/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window for fullscreen view
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#pragma once

class CFullscreenWindow : public CWindowImpl<CFullscreenWindow>
{
public:
    DECLARE_WND_CLASS_EX(L"FullscreenWindow", CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
                         COLOR_BACKGROUND)

    BEGIN_MSG_MAP(CFullscreenWindow)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_CLOSE, OnCloseOrKeyDownOrLButtonDown)
        MESSAGE_HANDLER(WM_KEYDOWN, OnCloseOrKeyDownOrLButtonDown)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnCloseOrKeyDownOrLButtonDown)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_GETTEXT, OnGetText)
    END_MSG_MAP()

    HWND DoCreate();

private:
    LRESULT OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCloseOrKeyDownOrLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnGetText(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};
