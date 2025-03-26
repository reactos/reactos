/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window for fullscreen view
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 */

#include "precomp.h"

CFullscreenWindow fullscreenWindow;

/* FUNCTIONS ********************************************************/

HWND CFullscreenWindow::DoCreate()
{
    if (m_hWnd)
        return m_hWnd;

    RECT rc = {0, 0, 0, 0}; // Rely on SW_SHOWMAXIMIZED
    return Create(HWND_DESKTOP, rc, NULL, WS_POPUPWINDOW, WS_EX_TOPMOST);
}

LRESULT CFullscreenWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SendMessage(WM_SETICON, ICON_BIG, (LPARAM)::LoadIconW(g_hinstExe, MAKEINTRESOURCEW(IDI_APPICON)));
    SendMessage(WM_SETICON, ICON_SMALL, (LPARAM)::LoadIconW(g_hinstExe, MAKEINTRESOURCEW(IDI_APPICON)));
    return 0;
}

LRESULT CFullscreenWindow::OnCloseOrKeyDownOrLButtonDown(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    mainWindow.ShowWindow(SW_SHOW);
    ShowWindow(SW_HIDE);
    return 0;
}

LRESULT CFullscreenWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(&ps);
    RECT rcWnd;
    GetWindowRect(&rcWnd);
    INT cxDest = imageModel.GetWidth();
    INT cyDest = imageModel.GetHeight();
    INT xDest = (rcWnd.right - rcWnd.left - cxDest) / 2;
    INT yDest = (rcWnd.bottom - rcWnd.top - cyDest) / 2;
    ::BitBlt(hDC, xDest, yDest, cxDest, cyDest, imageModel.GetDC(), 0, 0, SRCCOPY);
    EndPaint(&ps);
    return 0;
}

LRESULT CFullscreenWindow::OnGetText(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // return caption of the main window, instead
    return mainWindow.SendMessage(nMsg, wParam, lParam);
}
