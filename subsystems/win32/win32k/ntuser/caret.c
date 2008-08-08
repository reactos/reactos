/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/caret.c
 * PURPOSE:         Caret Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtUserCreateCaret(HWND hWnd,
                  HBITMAP hBitmap,
                  INT nWidth,
                  INT nHeight)
{
    UNIMPLEMENTED;
    return FALSE;
}

UINT
APIENTRY
NtUserGetCaretBlinkTime(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserGetCaretPos(LPPOINT lpPoint)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserHideCaret(HWND hWnd)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserShowCaret(HWND hWnd)
{
    UNIMPLEMENTED;
    return FALSE;
}
