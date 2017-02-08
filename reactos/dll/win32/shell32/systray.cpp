/*
 * Copyright 2004 Martin Fuchs
 *
 * Pass on icon notification messages to the systray implementation
 * in the currently running shell.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

/* copy data structure for tray notifications */
typedef struct TrayNotifyCDS_Dummy {
    DWORD    cookie;
    DWORD    notify_code;
    DWORD    nicon_data[1];    // placeholder for NOTIFYICONDATA structure
} TrayNotifyCDS_Dummy;

/* The only difference between Shell_NotifyIconA and Shell_NotifyIconW is the call to SendMessageA/W. */
static BOOL SHELL_NotifyIcon(DWORD dwMessage, void* pnid, HWND nid_hwnd, DWORD nid_size, BOOL unicode)
{
    HWND hwnd;
    COPYDATASTRUCT data;

    BOOL ret = FALSE;
    int len = FIELD_OFFSET(TrayNotifyCDS_Dummy, nicon_data) + nid_size;

    TrayNotifyCDS_Dummy* pnotify_data = (TrayNotifyCDS_Dummy*) alloca(len);

    pnotify_data->cookie = 1;
    pnotify_data->notify_code = dwMessage;
    memcpy(&pnotify_data->nicon_data, pnid, nid_size);

    data.dwData = 1;
    data.cbData = len;
    data.lpData = pnotify_data;

    for(hwnd = 0; (hwnd = FindWindowExW(0, hwnd, L"Shell_TrayWnd", NULL)); )
        if ((unicode ? SendMessageW : SendMessageA)(hwnd, WM_COPYDATA, (WPARAM)nid_hwnd, (LPARAM)&data))
            ret = TRUE;

    return ret;
}


/*************************************************************************
 * Shell_NotifyIcon            [SHELL32.296]
 * Shell_NotifyIconA            [SHELL32.297]
 */
BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA pnid)
{
    NOTIFYICONDATAW nidW;
    DWORD cbSize;

    /* Validate the cbSize as Windows XP does */
    if (pnid->cbSize != NOTIFYICONDATAA_V1_SIZE &&
        pnid->cbSize != NOTIFYICONDATAA_V2_SIZE &&
        pnid->cbSize != sizeof(NOTIFYICONDATAA))
    {
        WARN("Invalid cbSize (%d) - using only Win95 fields (size=%d)\n",
            pnid->cbSize, NOTIFYICONDATAA_V1_SIZE);
        cbSize = NOTIFYICONDATAA_V1_SIZE;
    }
    else
        cbSize = pnid->cbSize;

    ZeroMemory(&nidW, sizeof(nidW));
    nidW.cbSize = sizeof(nidW);
    nidW.hWnd   = pnid->hWnd;
    nidW.uID    = pnid->uID;
    nidW.uFlags = pnid->uFlags;
    nidW.uCallbackMessage = pnid->uCallbackMessage;
    nidW.hIcon  = pnid->hIcon;

    /* szTip */
    if (pnid->uFlags & NIF_TIP)
        MultiByteToWideChar(CP_ACP, 0, pnid->szTip, -1, nidW.szTip, _countof(nidW.szTip));

    if (cbSize >= NOTIFYICONDATAA_V2_SIZE)
    {
        nidW.dwState      = pnid->dwState;
        nidW.dwStateMask  = pnid->dwStateMask;

        /* szInfo, szInfoTitle */
        if (pnid->uFlags & NIF_INFO)
        {
            MultiByteToWideChar(CP_ACP, 0, pnid->szInfo, -1,  nidW.szInfo, _countof(nidW.szInfo));
            MultiByteToWideChar(CP_ACP, 0, pnid->szInfoTitle, -1, nidW.szInfoTitle, _countof(nidW.szInfoTitle));
        }

        nidW.uTimeout = pnid->uTimeout;
        nidW.dwInfoFlags = pnid->dwInfoFlags;
    }

    if (cbSize >= sizeof(NOTIFYICONDATAA))
        nidW.hBalloonIcon = pnid->hBalloonIcon;
    return Shell_NotifyIconW(dwMessage, &nidW);
}

/*************************************************************************
 * Shell_NotifyIconW            [SHELL32.298]
 */
BOOL WINAPI Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW pnid)
{
    DWORD cbSize;

    /* Validate the cbSize so that WM_COPYDATA doesn't crash the application */
    if (pnid->cbSize != NOTIFYICONDATAW_V1_SIZE &&
        pnid->cbSize != NOTIFYICONDATAW_V2_SIZE &&
        pnid->cbSize != sizeof(NOTIFYICONDATAW))
    {
        WARN("Invalid cbSize (%d) - using only Win95 fields (size=%d)\n",
            pnid->cbSize, NOTIFYICONDATAW_V1_SIZE);
        cbSize = NOTIFYICONDATAA_V1_SIZE;
    }
    else
        cbSize = pnid->cbSize;

    return SHELL_NotifyIcon(dwMessage, pnid, pnid->hWnd, cbSize, TRUE);
}
