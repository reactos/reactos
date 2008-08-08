/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/desktop.c
 * PURPOSE:         Desktop Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HDESK
APIENTRY
NtUserOpenDesktop(PUNICODE_STRING lpszDesktopName,
                  DWORD dwFlags,
                  ACCESS_MASK dwDesiredAccess)
{
    UNIMPLEMENTED;
    return NULL;
}

HDESK
APIENTRY
NtUserOpenInputDesktop(DWORD dwFlags,
                       BOOL fInherit,
                       ACCESS_MASK dwDesiredAccess)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserCloseDesktop(HDESK hDesktop)
{
    UNIMPLEMENTED;
    return FALSE;
}

HDESK
APIENTRY
NtUserCreateDesktop(PUNICODE_STRING lpszDesktopName,
                    DWORD dwFlags,
                    ACCESS_MASK dwDesiredAccess,
                    LPSECURITY_ATTRIBUTES lpSecurity,
                    HWINSTA hWindowStation)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserPaintDesktop(HDC hDC)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserResolveDesktop(DWORD dwUnknown1,
                     DWORD dwUnknown2,
                     DWORD dwUnknown3,
                     DWORD dwUnknown4)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserResolveDesktopForWOW(DWORD Unknown0)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserSwitchDesktop(HDESK hDesktop)
{
    UNIMPLEMENTED;
    return FALSE;
}

HDESK
APIENTRY
NtUserGetThreadDesktop(DWORD dwThreadId,
                       DWORD Unknown1)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserSetThreadDesktop(HDESK hDesktop)
{
    UNIMPLEMENTED;
    return FALSE;
}
