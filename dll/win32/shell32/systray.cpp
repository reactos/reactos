/*
 * Copyright 2004 Martin Fuchs
 * Copyright 2018 Hermes Belusca-Maito
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

WINE_DEFAULT_DEBUG_CHANNEL(shell_notify);

/*************************************************************************
 * Shell_NotifyIcon             [SHELL32.296]
 * Shell_NotifyIconA            [SHELL32.297]
 */
BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA pnid)
{
    NOTIFYICONDATAW nidW;
    DWORD cbSize, dwValidFlags;

    /* Initialize and capture the basic data fields */
    ZeroMemory(&nidW, sizeof(nidW));
    nidW.cbSize = sizeof(nidW); // Use a default size for the moment
    nidW.hWnd   = pnid->hWnd;
    nidW.uID    = pnid->uID;
    nidW.uFlags = pnid->uFlags;
    nidW.uCallbackMessage = pnid->uCallbackMessage;
    nidW.hIcon  = pnid->hIcon;

    /* Validate the structure size and the flags */
    cbSize = pnid->cbSize;
    dwValidFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    if (cbSize == sizeof(NOTIFYICONDATAA))
    {
        nidW.cbSize = sizeof(nidW);
        dwValidFlags |= NIF_STATE | NIF_INFO | NIF_GUID /* | NIF_REALTIME | NIF_SHOWTIP */;
    }
    else if (cbSize == NOTIFYICONDATAA_V3_SIZE)
    {
        nidW.cbSize = NOTIFYICONDATAW_V3_SIZE;
        dwValidFlags |= NIF_STATE | NIF_INFO | NIF_GUID;
    }
    else if (cbSize == NOTIFYICONDATAA_V2_SIZE)
    {
        nidW.cbSize = NOTIFYICONDATAW_V2_SIZE;
        dwValidFlags |= NIF_STATE | NIF_INFO;
    }
    else // if cbSize == NOTIFYICONDATAA_V1_SIZE or something else
    {
        if (cbSize != NOTIFYICONDATAA_V1_SIZE)
        {
            WARN("Invalid cbSize (%d) - using only Win95 fields (size=%d)\n",
                cbSize, NOTIFYICONDATAA_V1_SIZE);
            cbSize = NOTIFYICONDATAA_V1_SIZE;
        }
        nidW.cbSize = NOTIFYICONDATAW_V1_SIZE;
    }
    nidW.uFlags &= dwValidFlags;

    /* Capture the other data fields */

    if (nidW.uFlags & NIF_TIP)
    {
        /*
         * Depending on the size of the NOTIFYICONDATA structure
         * we should convert part of, or all the szTip string.
         */
        if (cbSize <= NOTIFYICONDATAA_V1_SIZE)
        {
#define NIDV1_TIP_SIZE_A  (NOTIFYICONDATAA_V1_SIZE - FIELD_OFFSET(NOTIFYICONDATAA, szTip))/sizeof(CHAR)
            MultiByteToWideChar(CP_ACP, 0, pnid->szTip, NIDV1_TIP_SIZE_A,
                                nidW.szTip, _countof(nidW.szTip));
            /* Truncate the string */
            nidW.szTip[NIDV1_TIP_SIZE_A - 1] = 0;
#undef NIDV1_TIP_SIZE_A
        }
        else
        {
            MultiByteToWideChar(CP_ACP, 0, pnid->szTip, -1,
                                nidW.szTip, _countof(nidW.szTip));
        }
    }

    if (cbSize >= NOTIFYICONDATAA_V2_SIZE)
    {
        nidW.dwState     = pnid->dwState;
        nidW.dwStateMask = pnid->dwStateMask;
        nidW.uTimeout    = pnid->uTimeout;
        nidW.dwInfoFlags = pnid->dwInfoFlags;

        if (nidW.uFlags & NIF_INFO)
        {
            MultiByteToWideChar(CP_ACP, 0, pnid->szInfo, -1,
                                nidW.szInfo, _countof(nidW.szInfo));
            MultiByteToWideChar(CP_ACP, 0, pnid->szInfoTitle, -1,
                                nidW.szInfoTitle, _countof(nidW.szInfoTitle));
        }
    }

    if ((cbSize >= NOTIFYICONDATAA_V3_SIZE) && (nidW.uFlags & NIF_GUID))
        nidW.guidItem = pnid->guidItem;

    if (cbSize >= sizeof(NOTIFYICONDATAA))
        nidW.hBalloonIcon = pnid->hBalloonIcon;

    /* Call the unicode function */
    return Shell_NotifyIconW(dwMessage, &nidW);
}

/*************************************************************************
 * Shell_NotifyIconW            [SHELL32.298]
 */
BOOL WINAPI Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW pnid)
{
    BOOL ret = FALSE;
    HWND hShellTrayWnd;
    DWORD cbSize, dwValidFlags;
    TRAYNOTIFYDATAW tnid;
    COPYDATASTRUCT data;

    /* Find a handle to the shell tray window */
    hShellTrayWnd = FindWindowW(L"Shell_TrayWnd", NULL);
    if (!hShellTrayWnd)
        return FALSE; // None found, bail out

    /* Validate the structure size and the flags */
    cbSize = pnid->cbSize;
    dwValidFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    if (cbSize == sizeof(NOTIFYICONDATAW))
    {
        dwValidFlags |= NIF_STATE | NIF_INFO | NIF_GUID /* | NIF_REALTIME | NIF_SHOWTIP */;
    }
    else if (cbSize == NOTIFYICONDATAW_V3_SIZE)
    {
        dwValidFlags |= NIF_STATE | NIF_INFO | NIF_GUID;
    }
    else if (cbSize == NOTIFYICONDATAW_V2_SIZE)
    {
        dwValidFlags |= NIF_STATE | NIF_INFO;
    }
    else // if cbSize == NOTIFYICONDATAW_V1_SIZE or something else
    {
        if (cbSize != NOTIFYICONDATAW_V1_SIZE)
        {
            WARN("Invalid cbSize (%d) - using only Win95 fields (size=%d)\n",
                cbSize, NOTIFYICONDATAW_V1_SIZE);
            cbSize = NOTIFYICONDATAW_V1_SIZE;
        }
    }

    /* Build the data structure */
    ZeroMemory(&tnid, sizeof(tnid));
    tnid.dwSignature = NI_NOTIFY_SIG;
    tnid.dwMessage   = dwMessage;

    /* Copy only the needed data, everything else is zeroed out */
    CopyMemory(&tnid.nid, pnid, cbSize);
    /* Adjust the size (the NOTIFYICONDATA structure is the full-fledged one) and the flags */
    tnid.nid.cbSize = sizeof(tnid.nid);
    tnid.nid.uFlags &= dwValidFlags;

    /* Be sure the szTip member (that could be cut-off) is correctly NULL-terminated */
    if (tnid.nid.uFlags & NIF_TIP)
    {
        if (cbSize <= NOTIFYICONDATAW_V1_SIZE)
        {
#define NIDV1_TIP_SIZE_W  (NOTIFYICONDATAW_V1_SIZE - FIELD_OFFSET(NOTIFYICONDATAW, szTip))/sizeof(WCHAR)
            tnid.nid.szTip[NIDV1_TIP_SIZE_W - 1] = 0;
#undef NIDV1_TIP_SIZE_W
        }
        else
        {
            tnid.nid.szTip[_countof(tnid.nid.szTip) - 1] = 0;
        }
    }

    /* Be sure the info strings are correctly NULL-terminated */
    if (tnid.nid.uFlags & NIF_INFO)
    {
        tnid.nid.szInfo[_countof(tnid.nid.szInfo) - 1] = 0;
        tnid.nid.szInfoTitle[_countof(tnid.nid.szInfoTitle) - 1] = 0;
    }

    /* Send the data */
    data.dwData = 1;
    data.cbData = sizeof(tnid);
    data.lpData = &tnid;
    if (SendMessageW(hShellTrayWnd, WM_COPYDATA, (WPARAM)pnid->hWnd, (LPARAM)&data))
        ret = TRUE;

    return ret;
}
