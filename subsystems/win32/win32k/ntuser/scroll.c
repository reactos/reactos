/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/scroll.c
 * PURPOSE:         Scrollbar and Scrolling Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtUserEnableScrollBar(HWND hWnd,
                      UINT wSBflags,
                      UINT wArrows)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserGetScrollBarInfo(HWND hWnd,
                       LONG idObject,
                       PSCROLLBARINFO psbi)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserScrollDC(HDC hDC,
               INT dx,
               INT dy,
               CONST RECT *lprcScroll,
               CONST RECT *lprcClip ,
               HRGN hrgnUpdate,
               LPRECT lprcUpdate)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserScrollWindowEx(HWND hWnd,
                     INT dx,
                     INT dy,
                     const RECT *rect,
                     const RECT *clipRect,
                     HRGN hrgnUpdate,
                     LPRECT rcUpdate,
                     UINT flags)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetScrollInfo(HWND hwnd,
                    INT fnBar,
                    LPCSCROLLINFO lpsi,
                    BOOL bRedraw)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserShowScrollBar(HWND hWnd,
                    INT wBar,
                    DWORD bShow)
{
    UNIMPLEMENTED;
    return 0;
}
