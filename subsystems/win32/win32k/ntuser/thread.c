/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/thread.c
 * PURPOSE:         User Thread Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtUserGetGUIThreadInfo(DWORD idThread,
                       LPGUITHREADINFO lpgui)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserGetThreadState(DWORD ThreadState)
{
    DPRINT1("NtUserGetThreadState() ThreadState %d\n", ThreadState);

    /* TODO: A big switch-case for all possible threadstate requires */
    return 0;
}

VOID
APIENTRY
NtUserSetThreadState(DWORD Unknown0,
                     DWORD Unknown1)
{
    UNIMPLEMENTED;
    return;
}

DWORD
APIENTRY
NtUserQueryInformationThread(DWORD dwUnknown1,
                             DWORD dwUnknown2,
                             DWORD dwUnknown3,
                             DWORD dwUnknown4,
                             DWORD dwUnknown5)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserSetInformationThread(DWORD dwUnknown1,
                           DWORD dwUnknown2,
                           DWORD dwUnknown3,
                           DWORD dwUnknown4)
{
    UNIMPLEMENTED;
    return 0;
}

VOID
APIENTRY
NtUserSetThreadLayoutHandles(DWORD dwUnknown1,
                             DWORD dwUnknown2)
{
    UNIMPLEMENTED;
	return;
}
