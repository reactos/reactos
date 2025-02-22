/*
 * PROJECT:     ReactOS Task Manager
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tray Icon.
 * COPYRIGHT:   Copyright 1999-2001 Brian Palmer <brianp@reactos.org>
 *              Copyright 2005 Klemens Friedl <frik85@reactos.at>
 */

#include "precomp.h"

static HICON
TrayIcon_GetProcessorUsageIcon(
    _In_ ULONG CpuUsage)
{
    HICON    hTrayIcon = NULL;
    HDC      hScreenDC = NULL;
    HDC      hDC = NULL;
    HBITMAP  hBitmap = NULL;
    HBITMAP  hOldBitmap = NULL;
    HBITMAP  hBitmapMask = NULL;
    ICONINFO iconInfo;
    int      nLinesToDraw;
    HBRUSH   hBitmapBrush = NULL;
    RECT     rc;

    /* Get a handle to the screen DC */
    hScreenDC = GetDC(NULL);
    if (!hScreenDC)
        goto done;

    /* Create our own DC from it */
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

static BOOL
TrayIcon_Update(
    _In_ DWORD dwMessage)
{
    static WCHAR szMsg[64] = L"";

    NOTIFYICONDATAW nid;
    ULONG CpuUsage;
    HICON hIcon = NULL;
    BOOL  bRetVal;

    if (!*szMsg)
        LoadStringW(hInst, IDS_MSG_TRAYICONCPUUSAGE, szMsg, _countof(szMsg));

    ZeroMemory(&nid, sizeof(nid));

    CpuUsage = PerfDataGetProcessorUsage();
    hIcon = TrayIcon_GetProcessorUsageIcon(CpuUsage);

    nid.cbSize = sizeof(nid);
    nid.hWnd = hMainWnd;
    nid.uID = 0;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_ONTRAYICON;
    nid.hIcon = hIcon;

    StringCchPrintfW(nid.szTip, _countof(nid.szTip), szMsg, CpuUsage);

    bRetVal = Shell_NotifyIconW(dwMessage, &nid);

    if (hIcon)
        DestroyIcon(hIcon);

    return bRetVal;
}

BOOL TrayIcon_AddIcon(VOID)
{
    return TrayIcon_Update(NIM_ADD);
}

BOOL TrayIcon_RemoveIcon(VOID)
{
    NOTIFYICONDATAW nid;

    ZeroMemory(&nid, sizeof(nid));

    nid.cbSize = sizeof(nid);
    nid.hWnd = hMainWnd;
    nid.uID = 0;
    nid.uFlags = 0;
    nid.uCallbackMessage = WM_ONTRAYICON;

    return Shell_NotifyIconW(NIM_DELETE, &nid);
}

BOOL TrayIcon_UpdateIcon(VOID)
{
    return TrayIcon_Update(NIM_MODIFY);
}
