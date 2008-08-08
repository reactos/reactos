/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/simplecall.c
 * PURPOSE:         Simplecall Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtUserCallHwnd(HWND hWnd,
               DWORD Routine)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserCallHwndLock(HWND hWnd,
                   DWORD Routine)
{
    UNIMPLEMENTED;
    return FALSE;
}

HWND
APIENTRY
NtUserCallHwndOpt(HWND hWnd,
                  DWORD Routine)
{
    UNIMPLEMENTED;
    return NULL;
}

DWORD
APIENTRY
NtUserCallHwndParam(HWND hWnd,
                    DWORD Param,
                    DWORD Routine)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserCallHwndParamLock(HWND hWnd,
                        DWORD Param,
                        DWORD Routine)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserCallNoParam(DWORD Routine)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserCallOneParam(DWORD Param,
                   DWORD Routine)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserCallTwoParam(DWORD Param1,
                   DWORD Param2,
                   DWORD Routine)
{
    UNIMPLEMENTED;
    return 0;
}
