/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/mouse.c
 * PURPOSE:         Mouse Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtUserDragDetect(HWND hWnd,
                 POINT pt)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserDragObject(HWND hwnd1,
                 HWND hwnd2,
                 UINT u1,
                 DWORD dw1,
                 HCURSOR hc1)
{
    UNIMPLEMENTED;
    return 0;
}

UINT
APIENTRY
NtUserGetDoubleClickTime(VOID)
{
    UNIMPLEMENTED;
   return 0;
}

INT
APIENTRY
NtUserGetMouseMovePointsEx(UINT cbSize,
                           LPMOUSEMOVEPOINT lppt,
                           LPMOUSEMOVEPOINT lpptBuf,
                           INT nBufPoints,
                           DWORD resolution)
{
    UNIMPLEMENTED;
    return 0;
}

HWND
APIENTRY
NtUserSetCapture(HWND hWnd)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserTrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack)
{
    UNIMPLEMENTED;
    return FALSE;
}
