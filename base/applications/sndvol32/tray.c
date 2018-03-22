/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Sound Volume Control
 * FILE:        base/applications/sndvol32/tray.c
 * PROGRAMMERS: Eric Kohl <eric.kohl@reactos.org>
 */

#include "sndvol32.h"

static VOID
OnTrayInitDialog(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam)
{
    POINT ptCursor;
    RECT rcWindow;
    RECT rcScreen;
    LONG x, y, cx, cy;

    GetCursorPos(&ptCursor);

    GetWindowRect(hwnd, &rcWindow);

    GetWindowRect(GetDesktopWindow(), &rcScreen);

    cx = rcWindow.right - rcWindow.left;
    cy = rcWindow.bottom - rcWindow.top;

    if (ptCursor.y + cy > rcScreen.bottom)
        y = ptCursor.y - cy;
    else
        y = ptCursor.y;

    if (ptCursor.x + cx > rcScreen.right)
        x = ptCursor.x - cx;
    else
        x = ptCursor.x;

    SetWindowPos(hwnd, HWND_TOPMOST, x, y, 0, 0, SWP_NOSIZE);

    /* Disable the controls for now */
    EnableWindow(GetDlgItem(hwnd, IDC_LINE_SLIDER_VERT), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_LINE_SWITCH), FALSE);
}


INT_PTR
CALLBACK
TrayDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnTrayInitDialog(hwndDlg, wParam, lParam);
            break;

        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE)
                EndDialog(hwndDlg, IDOK);
            break;
    }

    return 0;
}

/* EOF */
