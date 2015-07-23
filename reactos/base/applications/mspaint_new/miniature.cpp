/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint_new/miniature.cpp
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 */

/* INCLUDES *********************************************************/

#include "precomp.h"

/* FUNCTIONS ********************************************************/

LRESULT CMiniatureWindow::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    miniature.ShowWindow(SW_HIDE);
    showMiniature = FALSE;
    return 0;
}

LRESULT CMiniatureWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DefWindowProc(WM_PAINT, wParam, lParam);
    RECT mclient;
    HDC hdc;
    miniature.GetClientRect(&mclient);
    hdc = miniature.GetDC();
    StretchBlt(hdc, 0, 0, mclient.right, mclient.bottom, imageModel.GetDC(), 0, 0, imageModel.GetWidth(), imageModel.GetHeight(), SRCCOPY);
    miniature.ReleaseDC(hdc);
    return 0;
}

LRESULT CMiniatureWindow::OnSetCursor(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return 0;
}
