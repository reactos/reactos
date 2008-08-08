/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/timer.c
 * PURPOSE:         Timer Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtUserKillTimer(HWND hWnd,
                UINT_PTR uIDEvent)
{
    UNIMPLEMENTED;
    return FALSE;
}

UINT_PTR
APIENTRY
NtUserSetSystemTimer(HWND hWnd,
                     UINT_PTR nIDEvent,
                     UINT uElapse,
                     TIMERPROC lpTimerFunc)
{
    UNIMPLEMENTED;
    return 0;
}

UINT_PTR
APIENTRY
NtUserSetTimer(HWND hWnd,
               UINT_PTR nIDEvent,
               UINT uElapse,
               TIMERPROC lpTimerFunc)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserValidateTimerCallback(DWORD dwUnknown1,
                            DWORD dwUnknown2,
                            DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}
