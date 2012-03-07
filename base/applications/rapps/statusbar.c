/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/statusbar.c
 * PURPOSE:         StatusBar functions
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

HWND hStatusBar;

BOOL
CreateStatusBar(HWND hwnd)
{
    hStatusBar = CreateWindowExW(0,
                                 STATUSCLASSNAMEW,
                                 NULL,
                                 WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP,
                                 0, 0, 0, 0,
                                 hwnd,
                                 (HMENU)IDC_STATUSBAR,
                                 hInst,
                                 NULL);

    if (!hStatusBar)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    return TRUE;
}

VOID
SetStatusBarText(LPCWSTR lpszText)
{
    if (hStatusBar)
    {
        SendMessageW(hStatusBar, SB_SETTEXT, SBT_NOBORDERS, (LPARAM)lpszText);
    }
}
