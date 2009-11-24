/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/lazyrite.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

extern KEVENT MpwCompleteEvent;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
CcWaitForCurrentLazyWriterActivity(VOID)
{
    KeWaitForSingleObject(&MpwCompleteEvent, Executive, KernelMode, FALSE, NULL);
    return STATUS_SUCCESS;
}

/* EOF */
