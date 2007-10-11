/*
 * Systray handling
 *
 * Copyright 1999 Kai Morich	<kai.morich@bigfoot.de>
 * Copyright 2004 Mike Hearn, for CodeWeavers
 * Copyright 2005 Robert Shearman
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winuser.h"
#include "shellapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(systray);

static const WCHAR classname[] = /* Shell_TrayWnd */ {'S','h','e','l','l','_','T','r','a','y','W','n','d','\0'};

/*************************************************************************
 * Shell_NotifyIcon			[SHELL32.296]
 * Shell_NotifyIconA			[SHELL32.297]
 */
BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA pnid)
{
    NOTIFYICONDATAW nidW;
    INT cbSize;

    /* Validate the cbSize as Windows XP does */
    if (pnid->cbSize != NOTIFYICONDATAA_V1_SIZE &&
        pnid->cbSize != NOTIFYICONDATAA_V2_SIZE &&
        pnid->cbSize != NOTIFYICONDATAA_V3_SIZE &&
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
        MultiByteToWideChar(CP_ACP, 0, pnid->szTip, -1, nidW.szTip, sizeof(nidW.szTip)/sizeof(WCHAR));

    if (cbSize >= NOTIFYICONDATAA_V2_SIZE)
    {
        nidW.dwState      = pnid->dwState;
        nidW.dwStateMask  = pnid->dwStateMask;

        /* szInfo, szInfoTitle */
        if (pnid->uFlags & NIF_INFO)
        {
            MultiByteToWideChar(CP_ACP, 0, pnid->szInfo, -1,  nidW.szInfo, sizeof(nidW.szInfo)/sizeof(WCHAR));
            MultiByteToWideChar(CP_ACP, 0, pnid->szInfoTitle, -1, nidW.szInfoTitle, sizeof(nidW.szInfoTitle)/sizeof(WCHAR));
        }

        nidW.u.uTimeout = pnid->u.uTimeout;
        nidW.dwInfoFlags = pnid->dwInfoFlags;
    }
    
    if (cbSize >= NOTIFYICONDATAA_V3_SIZE)
        nidW.guidItem = pnid->guidItem;

    if (cbSize >= sizeof(NOTIFYICONDATAA))
        nidW.hBalloonIcon = pnid->hBalloonIcon;
    return Shell_NotifyIconW(dwMessage, &nidW);
}

/*************************************************************************
 * Shell_NotifyIconW			[SHELL32.298]
 */
BOOL WINAPI Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW nid)
{
    HWND tray;
    COPYDATASTRUCT cds;
    char *buffer = NULL;

    TRACE("dwMessage = %d, nid->cbSize=%d\n", dwMessage, nid->cbSize);

    /* Validate the cbSize so that WM_COPYDATA doesn't crash the application */
    if (nid->cbSize != NOTIFYICONDATAW_V1_SIZE &&
        nid->cbSize != NOTIFYICONDATAW_V2_SIZE &&
        nid->cbSize != NOTIFYICONDATAW_V3_SIZE &&
        nid->cbSize != sizeof(NOTIFYICONDATAW))
    {
        NOTIFYICONDATAW newNid;

        WARN("Invalid cbSize (%d) - using only Win95 fields (size=%d)\n",
            nid->cbSize, NOTIFYICONDATAW_V1_SIZE);
        CopyMemory(&newNid, nid, NOTIFYICONDATAW_V1_SIZE);
        newNid.cbSize = NOTIFYICONDATAW_V1_SIZE;
        return Shell_NotifyIconW(dwMessage, &newNid);
    }

    tray = FindWindowExW(0, NULL, classname, NULL);
    if (!tray) return FALSE;

    cds.dwData = dwMessage;

    /* FIXME: if statement only needed because we don't support interprocess
     * icon handles */
    if (nid->uFlags & NIF_ICON)
    {
        ICONINFO iconinfo;
        BITMAP bmMask;
        BITMAP bmColour;
        LONG cbMaskBits;
        LONG cbColourBits;

        if (!GetIconInfo(nid->hIcon, &iconinfo))
            goto noicon;

        if (!GetObjectW(iconinfo.hbmMask, sizeof(bmMask), &bmMask) ||
            !GetObjectW(iconinfo.hbmColor, sizeof(bmColour), &bmColour))
        {
            DeleteObject(iconinfo.hbmMask);
            DeleteObject(iconinfo.hbmColor);
            goto noicon;
        }

        cbMaskBits = (bmMask.bmPlanes * bmMask.bmWidth * bmMask.bmHeight * bmMask.bmBitsPixel) / 8;
        cbColourBits = (bmColour.bmPlanes * bmColour.bmWidth * bmColour.bmHeight * bmColour.bmBitsPixel) / 8;
        cds.cbData = nid->cbSize + 2*sizeof(BITMAP) + cbMaskBits + cbColourBits;
        buffer = HeapAlloc(GetProcessHeap(), 0, cds.cbData);
        if (!buffer)
        {
            DeleteObject(iconinfo.hbmMask);
            DeleteObject(iconinfo.hbmColor);
            return FALSE;
        }
        cds.lpData = buffer;

        memcpy(buffer, nid, nid->cbSize);
        buffer += nid->cbSize;
        memcpy(buffer, &bmMask, sizeof(bmMask));
        buffer += sizeof(bmMask);
        memcpy(buffer, &bmColour, sizeof(bmColour));
        buffer += sizeof(bmColour);
        GetBitmapBits(iconinfo.hbmMask, cbMaskBits, buffer);
        buffer += cbMaskBits;
        GetBitmapBits(iconinfo.hbmColor, cbColourBits, buffer);
        buffer += cbColourBits;

        DeleteObject(iconinfo.hbmMask);
        DeleteObject(iconinfo.hbmColor);
    }
    else
    {
noicon:
        cds.cbData = nid->cbSize;
        cds.lpData = nid;
    }

    SendMessageW(tray, WM_COPYDATA, (WPARAM)nid->hWnd, (LPARAM)&cds);

    /* FIXME: if statement only needed because we don't support interprocess
     * icon handles */
    HeapFree(GetProcessHeap(), 0, buffer);

    return TRUE;
}
