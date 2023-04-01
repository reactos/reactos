/*
 * PROJECT:     PAINT for ReactOS
 * LICENSE:     LGPL
 * FILE:        base/applications/mspaint/miniature.cpp
 * PURPOSE:     Window procedure of the main window and all children apart from
 *              hPalWin, hToolSettings and hSelection
 * PROGRAMMERS: Benedikt Freisen
 */

#include "precomp.h"

CMiniatureWindow miniature;

/* FUNCTIONS ********************************************************/

HWND CMiniatureWindow::DoCreate(HWND hwndParent)
{
    if (m_hWnd)
        return m_hWnd;

    RECT rc =
    {
        (LONG)registrySettings.ThumbXPos, (LONG)registrySettings.ThumbYPos,
        (LONG)(registrySettings.ThumbXPos + registrySettings.ThumbWidth),
        (LONG)(registrySettings.ThumbYPos + registrySettings.ThumbHeight)
    };

    TCHAR strTitle[100];
    ::LoadString(hProgInstance, IDS_MINIATURETITLE, strTitle, _countof(strTitle));

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
    return Create(hwndParent, rc, strTitle, style, WS_EX_PALETTEWINDOW);
}

LRESULT CMiniatureWindow::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ShowWindow(SW_HIDE);
    registrySettings.ShowThumbnail = FALSE;
    return 0;
}

LRESULT CMiniatureWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rc;
    GetClientRect(&rc);

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(&ps);
    StretchBlt(hDC, 0, 0, rc.right, rc.bottom,
               imageModel.GetDC(), 0, 0, imageModel.GetWidth(), imageModel.GetHeight(),
               SRCCOPY);
    EndPaint(&ps);
    return 0;
}
