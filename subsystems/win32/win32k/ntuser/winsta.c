/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/winsta.c
 * PURPOSE:         Window Station Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HWINSTA
APIENTRY
NtUserCreateWindowStation(PUNICODE_STRING lpszWindowStationName,
                          ACCESS_MASK dwDesiredAccess,
                          LPSECURITY_ATTRIBUTES lpSecurity,
                          DWORD Unknown3,
                          DWORD Unknown4,
                          DWORD Unknown5,
                          DWORD Unknown6)
{
    UNIMPLEMENTED;
    return NULL;
}

HWINSTA
APIENTRY
NtUserOpenWindowStation(PUNICODE_STRING lpszWindowStationName,
                        ACCESS_MASK dwDesiredAccess)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserCloseWindowStation(HWINSTA hWinSta)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserLockWindowStation(HWINSTA hWindowStation)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserUnlockWindowStation(HWINSTA hWindowStation)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserSetWindowStationUser(DWORD Unknown0,
                           DWORD Unknown1,
                           DWORD Unknown2,
                           DWORD Unknown3)
{
    UNIMPLEMENTED;
    return 0;
}

HWINSTA
APIENTRY
NtUserGetProcessWindowStation(VOID)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserSetProcessWindowStation(HWINSTA hWindowStation)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserLockWorkStation(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}
