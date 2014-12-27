/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Input Method Editor and Input Method Manager support
 * FILE:             win32ss/user/ntuser/ime.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

#include <win32k.h>
//DBG_DEFAULT_CHANNEL(UserMisc);


DWORD
APIENTRY
NtUserGetImeHotKey(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserNotifyIMEStatus(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2)
{
   STUB

   return 0;
}


DWORD
APIENTRY
NtUserSetImeHotKey(
   DWORD Unknown0,
   DWORD Unknown1,
   DWORD Unknown2,
   DWORD Unknown3,
   DWORD Unknown4)
{
   STUB

   return 0;
}

DWORD
APIENTRY
NtUserCheckImeHotKey(
    DWORD  VirtualKey,
    LPARAM lParam)
{
    STUB;
    return 0;
}


DWORD
APIENTRY
NtUserDisableThreadIme(
    DWORD dwUnknown1)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserGetAppImeLevel(
    DWORD dwUnknown1)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserGetImeInfoEx(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    STUB;
    return 0;
}


DWORD
APIENTRY
NtUserSetAppImeLevel(
    DWORD dwUnknown1,
    DWORD dwUnknown2)
{
    STUB;
    return 0;
}

DWORD
APIENTRY
NtUserSetImeInfoEx(
    DWORD dwUnknown1)
{
    STUB;
    return 0;
}

DWORD APIENTRY
NtUserSetImeOwnerWindow(DWORD Unknown0,
                        DWORD Unknown1)
{
   STUB

   return 0;
}


/* EOF */
