/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/usrfuncs.c
 * PURPOSE:         User functions
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
NTAPI
NtUserGetThreadState(DWORD ThreadState)
{
    DPRINT1("NtUserGetThreadState() ThreadState %d\n", ThreadState);

    /* TODO: A big switch-case for all possible threadstate requires */
    return 0;
}

