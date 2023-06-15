/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Task Manager for ReactOS
 * COPYRIGHT:   Copyright (C) 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright (C) 2005 Klemens Friedl <frik85@reactos.at>
 *              Copyright (C) 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#include <strsafe.h>

HICON TrayIcon_GetProcessorUsageIcon(void)
{
    HICON     hTrayIcon = NULL;
    HDC       hScreenDC = NULL;
    HDC       hDC = NULL;
    HBITMAP   hBitmap = NULL;
    HBITMAP   hOldBitmap = NULL;
    HBITMAP   hBitmapMask = NULL;
    ICONINFO  iconInfo;
    ULONG     ProcessorUsage;
    int       nLinesToDraw;
    HBRUSH    hBitmapBrush = NULL;
    RECT      rc;

    /*
     * Get a handle to the screen DC
     */
    hScreenDC = GetDC(NULL);
    if (!hScreenDC)
        goto done;

    /*
     * Create our own DC from it
     */
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
    hOldBitmap = (HBITMAP) SelectObject(hDC, hBitmap);

    /*
     * Get the cpu usage
     */
    ProcessorUsage = PerfDataGetProcessorUsage();

    /*
     * Calculate how many lines to draw
     * since we have 11 rows of space
     * to draw the cpu usage instead of
     * just having 10.
     */
    nLinesToDraw = (ProcessorUsage + (ProcessorUsage / 10)) / 11;
    rc.left = 3;
    rc.top = 12 - nLinesToDraw;
    rc.right = 13;
    rc.bottom = 13;

    /*
     * Now draw the cpu usage
     */
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

    /*
     * Return the newly created tray icon (if successful)
     */
    return hTrayIcon;
}

BOOL TrayIcon_ShellAddTrayIcon(void)
{
    NOTIFYICONDATAW nid;
    HICON           hIcon = TrayIcon_GetProcessorUsageIcon();
    BOOL            bRetVal;
    WCHAR           szMsg[64];

    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hMainWnd;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_ONTRAYICON;
    nid.hIcon = hIcon;

    LoadStringW(NULL, IDS_MSG_TRAYICONCPUUSAGE, szMsg, _countof(szMsg));
    StringCchPrintfW(nid.szTip, _countof(nid.szTip), szMsg, PerfDataGetProcessorUsage());

    bRetVal = Shell_NotifyIconW(NIM_ADD, &nid);

    if (hIcon)
        DestroyIcon(hIcon);

    return bRetVal;
}

BOOL TrayIcon_ShellRemoveTrayIcon(void)
{
    NOTIFYICONDATAW nid;

    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hMainWnd;
    nid.uID = 0;
    nid.uFlags = 0;
    nid.uCallbackMessage = WM_ONTRAYICON;

    return Shell_NotifyIconW(NIM_DELETE, &nid);
}

BOOL TrayIcon_ShellUpdateTrayIcon(void)
{
    NOTIFYICONDATAW nid;
    HICON           hIcon = TrayIcon_GetProcessorUsageIcon();
    BOOL            bRetVal;
    WCHAR           szTemp[64];

    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATAW);
    nid.hWnd = hMainWnd;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_ONTRAYICON;
    nid.hIcon = hIcon;

    LoadStringW(NULL, IDS_MSG_TRAYICONCPUUSAGE, szTemp, _countof(szTemp));
    StringCchPrintfW(nid.szTip, _countof(nid.szTip), szTemp, PerfDataGetProcessorUsage());

    bRetVal = Shell_NotifyIconW(NIM_MODIFY, &nid);

    if (hIcon)
        DestroyIcon(hIcon);

    return bRetVal;
}
