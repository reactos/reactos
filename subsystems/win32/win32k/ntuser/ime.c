/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/ime.c
 * PURPOSE:         Input Method Editor Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtUserCheckImeHotKey(DWORD dwUnknown1,
                     DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserDisableThreadIme(DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
APIENTRY
NtUserNotifyIMEStatus(DWORD Unknown0,
                      DWORD Unknown1,
                      DWORD Unknown2)
{
    UNIMPLEMENTED;
    return;
}

DWORD
APIENTRY
NtUserGetAppImeLevel(DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetAppImeLevel(DWORD dwUnknown1,
                     DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}


DWORD
APIENTRY
NtUserGetImeHotKey(DWORD Unknown0,
                   DWORD Unknown1,
                   DWORD Unknown2,
                   DWORD Unknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetImeHotKey(DWORD Unknown0,
                   DWORD Unknown1,
                   DWORD Unknown2,
                   DWORD Unknown3,
                   DWORD Unknown4)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetImeInfoEx(DWORD dwUnknown1,
                   DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetImeInfoEx(DWORD dwUnknown1)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetImeOwnerWindow(DWORD Unknown0,
                        DWORD Unknown1)
{
    UNIMPLEMENTED;
    return 0;
}
