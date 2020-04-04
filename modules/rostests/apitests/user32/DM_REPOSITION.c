/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Test for DM_REPOSITION
 * COPYRIGHT:   Copyright 2019 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <windowsx.h>

static BOOL OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    RECT rc, rc2, rcWork;
    INT cx, cy, nBitsPixel;
    HMONITOR hMon;
    MONITORINFO mi = { sizeof(mi) };
    HDC hdc;

    /* get monitor info */
    hMon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    ok(hMon != NULL, "hMon is NULL\n");
    ok_int(GetMonitorInfoW(hMon, &mi), TRUE);
    rcWork = mi.rcWork;

    /* get size */
    GetWindowRect(hwnd, &rc);
    cx = rc.right - rc.left;
    cy = rc.bottom - rc.top;

    /* move */
    ok_int(SetWindowPos(hwnd, NULL, rcWork.left - 80, rcWork.top - 80, 0, 0,
                        SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER |
                        SWP_NOZORDER), TRUE);
    ok_int(GetWindowRect(hwnd, &rc), TRUE);
    ok_long(rc.left, rcWork.left - 80);
    ok_long(rc.top, rcWork.top - 80);
    ok_long(rc.right, rc.left + cx);
    ok_long(rc.bottom, rc.top + cy);

    /* reposition */
    ok_int(SendMessageW(hwnd, DM_REPOSITION, 0, 0), 0);
    ok_int(GetWindowRect(hwnd, &rc), TRUE);
    ok_long(rc.left, rcWork.left);
    ok_long(rc.top, rcWork.top);
    ok_long(rc.right, rc.left + cx);
    ok_long(rc.bottom, rc.top + cy);

    /* move */
    ok_int(SetWindowPos(hwnd, NULL,
                        rcWork.right - cx + 80, rcWork.bottom - cy + 80, 0, 0,
                        SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER |
                        SWP_NOZORDER), TRUE);
    ok_int(GetWindowRect(hwnd, &rc), TRUE);
    ok_long(rc.left, rcWork.right - cx + 80);
    ok_long(rc.top, rcWork.bottom - cy + 80);
    ok_long(rc.right, rc.left + cx);
    ok_long(rc.bottom, rc.top + cy);

    /* reposition */
    ok_int(SendMessageW(hwnd, DM_REPOSITION, 0, 0), 0);
    ok_int(GetWindowRect(hwnd, &rc), TRUE);
    ok_long(rc.left, rcWork.right - cx);
    ok_long(rc.top, rcWork.bottom - cy - 4);
    ok_long(rc.right, rc.left + cx);
    ok_long(rc.bottom, rc.top + cy);

    /* minimize */
    ShowWindow(hwnd, SW_MINIMIZE);
    ok_int(GetWindowRect(hwnd, &rc), TRUE);

    /* reposition */
    ok_int(SendMessageW(hwnd, DM_REPOSITION, 0, 0), 0);
    ok_int(GetWindowRect(hwnd, &rc2), TRUE);
    ok_int(EqualRect(&rc, &rc2), TRUE);

    /* restore */
    ShowWindow(hwnd, SW_RESTORE);

    /* move */
    ok_int(SetWindowPos(hwnd, NULL,
                        rcWork.right - cx + 80, rcWork.bottom - cy + 80, 0, 0,
                        SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOOWNERZORDER |
                        SWP_NOZORDER), TRUE);
    ok_int(GetWindowRect(hwnd, &rc), TRUE);
    ok_long(rc.left, rcWork.right - cx + 80);
    ok_long(rc.top, rcWork.bottom - cy + 80);
    ok_long(rc.right, rc.left + cx);
    ok_long(rc.bottom, rc.top + cy);

    /* WM_DISPLAYCHANGE */
    hdc = GetWindowDC(hwnd);
    nBitsPixel = GetDeviceCaps(hdc, BITSPIXEL);
    ReleaseDC(hwnd, hdc);
    SendMessageW(hwnd, WM_DISPLAYCHANGE, nBitsPixel,
                 MAKELONG(GetSystemMetrics(SM_CXSCREEN),
                          GetSystemMetrics(SM_CYSCREEN)));
    ok_int(GetWindowRect(hwnd, &rc), TRUE);
    ok_long(rc.left, rcWork.right - cx + 80);
    ok_long(rc.top, rcWork.bottom - cy + 80);
    ok_long(rc.right, rc.left + cx);
    ok_long(rc.bottom, rc.top + cy);

    /* quit */
    PostMessage(hwnd, WM_COMMAND, IDOK, 0);
    return TRUE;
}

static void OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwnd, id);
            break;
    }
}

static INT_PTR CALLBACK
DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);
    }
    return 0;
}

START_TEST(DM_REPOSITION)
{
    HMODULE hMod = GetModuleHandle(NULL);
    ok(hMod != NULL, "\n");
    DialogBox(hMod, TEXT("REPOSITION"), NULL, DialogProc);
}
