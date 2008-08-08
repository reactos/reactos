/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/process.c
 * PURPOSE:         User Process Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtUserNotifyProcessCreate(DWORD dwUnknown1,
                          DWORD dwUnknown2,
                          DWORD dwUnknown3,
                          DWORD dwUnknown4)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserProcessConnect(DWORD dwUnknown1,
                     DWORD dwUnknown2,
                     DWORD dwUnknown3)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetInformationProcess(DWORD dwUnknown1,
                            DWORD dwUnknown2,
                            DWORD dwUnknown3,
                            DWORD dwUnknown4)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserGetGuiResources(HANDLE hProcess,
                      DWORD uiFlags)
{
    UNIMPLEMENTED;
    return 0;
}
