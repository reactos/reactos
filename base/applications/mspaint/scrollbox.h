/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/scrollbox.h
 * PURPOSE:     Functionality surrounding the scroll box window class
 * PROGRAMMERS: Benedikt Freisen
 */

#pragma once

class CScrollboxWindow : public CWindowImpl<CScrollboxWindow>
{
public:
    DECLARE_WND_CLASS_EX(_T("Scrollbox"), 0, COLOR_APPWORKSPACE)

    BEGIN_MSG_MAP(CScrollboxWindow)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_HSCROLL, OnHScroll)
        MESSAGE_HANDLER(WM_VSCROLL, OnVScroll)
        MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
        MESSAGE_HANDLER(WM_MOUSEWHEEL, OnMouseWheel)
    END_MSG_MAP()

    LRESULT OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnHScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnVScroll(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnMouseWheel(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
};

void UpdateScrollbox();
