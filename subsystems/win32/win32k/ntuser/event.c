/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/event.c
 * PURPOSE:         Event Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtUserEvent(DWORD Unknown0)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
APIENTRY
NtUserNotifyWinEvent(DWORD Event,
                     HWND hWnd,
                     LONG idObject,
                     LONG idChild)
{
    UNIMPLEMENTED;
}

DWORD
APIENTRY
NtUserWaitForMsgAndEvent(DWORD Unknown0)
{
    UNIMPLEMENTED;
    return 0;
}
