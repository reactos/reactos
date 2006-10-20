/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/dbgk/dbgkutil.c
 * PURPOSE:         User-Mode Debugging Support, Internal Debug Functions.
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
DbgkCreateThread(PVOID StartAddress)
{
    /* FIXME */
}

VOID
NTAPI
DbgkExitProcess(IN NTSTATUS ExitStatus)
{
    /* FIXME */
}

VOID
NTAPI
DbgkExitThread(IN NTSTATUS ExitStatus)
{
    /* FIXME */
}

VOID
NTAPI
DbgkpSuspendProcess(VOID)
{

}

VOID
NTAPI
DbgkpResumeProcess(VOID)
{

}

/* EOF */
