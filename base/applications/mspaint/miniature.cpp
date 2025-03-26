/*
 * PROJECT:    PAINT for ReactOS
 * LICENSE:    LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:    Window procedure of the main window and all children apart from
 *             hPalWin, hToolSettings and hSelection
 * COPYRIGHT:  Copyright 2015 Benedikt Freisen <b.freisen@gmx.net>
 *             Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

CMiniatureWindow miniature;

/* FUNCTIONS ********************************************************/

CMiniatureWindow::CMiniatureWindow()
    : m_hbmCached(NULL)
{
}

CMiniatureWindow::~CMiniatureWindow()
{
    if (m_hbmCached)
        ::DeleteObject(m_hbmCached);
}

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

    WCHAR strTitle[100];
    ::LoadStringW(g_hinstExe, IDS_MINIATURETITLE, strTitle, _countof(strTitle));

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
    return Create(hwndParent, rc, strTitle, style, WS_EX_PALETTEWINDOW);
}

LRESULT CMiniatureWindow::OnMove(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (IsWindowVisible() && !IsIconic() && !IsZoomed())
    {
        CRect rc;
        GetWindowRect(&rc);
        registrySettings.ThumbXPos = rc.left;
        registrySettings.ThumbYPos = rc.top;
    }
    return 0;
}

LRESULT CMiniatureWindow::OnSize(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (IsWindowVisible() && !IsIconic() && !IsZoomed())
    {
        CRect rc;
        GetWindowRect(&rc);
        registrySettings.ThumbWidth = rc.Width();
        registrySettings.ThumbHeight = rc.Height();
    }
    return 0;
}

LRESULT CMiniatureWindow::OnClose(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ShowWindow(SW_HIDE);
    registrySettings.ShowThumbnail = FALSE;
    return 0;
}

LRESULT CMiniatureWindow::OnEraseBkgnd(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return TRUE; /* Avoid flickering */
}

LRESULT CMiniatureWindow::OnPaint(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RECT rc;
    GetClientRect(&rc);

    // Start painting
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(&ps);
    if (!hDC)
        return 0;

    // Use a memory bitmap to reduce flickering
    HDC hdcMem = ::CreateCompatibleDC(hDC);
    m_hbmCached = CachedBufferDIB(m_hbmCached, rc.right, rc.bottom);
    HGDIOBJ hbmOld = ::SelectObject(hdcMem, m_hbmCached);

    // FIXME: Consider aspect ratio

    // Fill the background
    ::FillRect(hdcMem, &rc, (HBRUSH)(COLOR_BTNFACE + 1));

    // Draw the image (hdcMem <-- imageModel)
    int cxImage = imageModel.GetWidth();
    int cyImage = imageModel.GetHeight();
    ::StretchBlt(hdcMem, 0, 0, rc.right, rc.bottom,
                 imageModel.GetDC(), 0, 0, cxImage, cyImage,
                 SRCCOPY);

    // Move the image (hDC <-- hdcMem)
    ::BitBlt(hDC, 0, 0, rc.right, rc.bottom, hdcMem, 0, 0, SRCCOPY);

    // Clean up
    ::SelectObject(hdcMem, hbmOld);
    ::DeleteDC(hdcMem);

    EndPaint(&ps);
    return 0;
}

LRESULT CMiniatureWindow::OnGetMinMaxInfo(UINT nMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Avoid too small
    LPMINMAXINFO pInfo = (LPMINMAXINFO)lParam;
    pInfo->ptMinTrackSize = { 100, 75 };
    return 0;
}
