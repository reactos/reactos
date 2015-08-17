/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/fullscreen.cpp
 * PURPOSE:     Window for fullscreen view
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

LRESULT CFullscreenWindow::OnCreate(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SendMessage(WM_SETICON, ICON_BIG, (LPARAM) LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON)));
    SendMessage(WM_SETICON, ICON_SMALL, (LPARAM) LoadIcon(hProgInstance, MAKEINTRESOURCE(IDI_APPICON)));
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
    BitBlt(hDC, xDest, yDest, cxDest, cyDest, imageModel.GetDC(), 0, 0, SRCCOPY);
    EndPaint(&ps);
    return 0;
}

LRESULT CFullscreenWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Invalidate(TRUE);
    return 0;
}

LRESULT CFullscreenWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    bHandled = FALSE;
    return 0;
}

LRESULT CFullscreenWindow::OnGetText(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // return caption of the main window, instead
    return mainWindow.SendMessage(nMsg, wParam, lParam);
}
