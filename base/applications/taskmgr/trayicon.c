/*
 * PROJECT:   ReactOS Task Manager
 * LICENSE:   LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * COPYRIGHT: 1999-2001 Brian Palmer <brianp@reactos.org>
 *            2005 Klemens Friedl <frik85@reactos.at>
 */

#include "precomp.h"

HICON TrayIcon_GetProcessorUsageIcon(void)
{
    HICON    hTrayIcon = NULL;
    HDC      hScreenDC = NULL;
    HDC      hDC = NULL;
    HBITMAP  hBitmap = NULL;
    HBITMAP  hOldBitmap = NULL;
    HBITMAP  hBitmapMask = NULL;
    ICONINFO iconInfo;
    ULONG    CpuUsage;
    int      nLinesToDraw;
    HBRUSH   hBitmapBrush = NULL;
    RECT     rc;

    hScreenDC = GetDC(NULL);
    if (!hScreenDC)
        goto done;

    hDC = CreateCompatibleDC(hScreenDC);
    if (!hDC)
        goto done;

    /*
     * Load the bitmaps
     */
    hBitmap = LoadBitmapW(hInst, MAKEINTRESOURCEW(IDB_TRAYICON));
    hBitmapMask = LoadBitmapW(hInst, MAKEINTRESOURCEW(IDB_TRAYMASK));
    if (!hBitmap || !hBitmapMask)
        goto done;

    hBitmapBrush = CreateSolidBrush(RGB(0, 255, 0));
    if (!hBitmapBrush)
        goto done;

    /*
     * Select the bitmap into our device context
     * so we can draw on it.
     */
    hOldBitmap = SelectObject(hDC, hBitmap);

    CpuUsage = PerfDataGetProcessorUsage();

    /*
     * Calculate how many lines to draw
     * since we have 11 rows of space
     * to draw the cpu usage instead of
     * just having 10.
     */
    nLinesToDraw = (CpuUsage + (CpuUsage / 10)) / 11;
    rc.left = 3;
    rc.top = 12 - nLinesToDraw;
    rc.right = 13;
    rc.bottom = 13;

    /* Now draw the cpu usage */
    if (nLinesToDraw)
        FillRect(hDC, &rc, hBitmapBrush);

    /*
     * Now that we are done drawing put the
     * old bitmap back.
     */
    hBitmap = SelectObject(hDC, hOldBitmap);
    hOldBitmap = NULL;

    iconInfo.fIcon = TRUE;
    iconInfo.hbmMask = hBitmapMask;
    iconInfo.hbmColor = hBitmap;

    hTrayIcon = CreateIconIndirect(&iconInfo);

done:
    /*
     * Cleanup
     */
    if (hScreenDC)
        ReleaseDC(NULL, hScreenDC);
    if (hDC)
        DeleteDC(hDC);
    if (hBitmapBrush)
        DeleteObject(hBitmapBrush);
    if (hBitmap)
        DeleteObject(hBitmap);
    if (hBitmapMask)
        DeleteObject(hBitmapMask);

    /* Return the newly created tray icon (if successful) */
    return hTrayIcon;
}

BOOL TrayIcon_AddIcon(void)
{
    NOTIFYICONDATAW nid;
    HICON hIcon = NULL;
    BOOL  bRetVal;
    WCHAR szMsg[64];

    memset(&nid, 0, sizeof(nid));

    hIcon = TrayIcon_GetProcessorUsageIcon();

    nid.cbSize = sizeof(nid);
    nid.hWnd = hMainWnd;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_ONTRAYICON;
    nid.hIcon = hIcon;

    LoadStringW(GetModuleHandleW(NULL), IDS_MSG_TRAYICONCPUUSAGE, szMsg, _countof(szMsg));
    wsprintfW(nid.szTip, szMsg, PerfDataGetProcessorUsage());

    bRetVal = Shell_NotifyIconW(NIM_ADD, &nid);

    if (hIcon)
        DestroyIcon(hIcon);

    return bRetVal;
}

BOOL TrayIcon_RemoveIcon(void)
{
    NOTIFYICONDATAW nid;

    memset(&nid, 0, sizeof(nid));

    nid.cbSize = sizeof(nid);
    nid.hWnd = hMainWnd;
    nid.uID = 0;
    nid.uFlags = 0;
    nid.uCallbackMessage = WM_ONTRAYICON;

    return Shell_NotifyIconW(NIM_DELETE, &nid);
}

BOOL TrayIcon_UpdateIcon(void)
{
    NOTIFYICONDATAW nid;
    HICON           hIcon = NULL;
    BOOL            bRetVal;
    WCHAR           szMsg[64];

    memset(&nid, 0, sizeof(nid));

    hIcon = TrayIcon_GetProcessorUsageIcon();

    nid.cbSize = sizeof(nid);
    nid.hWnd = hMainWnd;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_ONTRAYICON;
    nid.hIcon = hIcon;
    LoadStringW(hInst, IDS_MSG_TRAYICONCPUUSAGE, szMsg, _countof(szMsg));
    wsprintfW(nid.szTip, szMsg, PerfDataGetProcessorUsage());

    bRetVal = Shell_NotifyIconW(NIM_MODIFY, &nid);

    if (hIcon)
        DestroyIcon(hIcon);

    return bRetVal;
}
