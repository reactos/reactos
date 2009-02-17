/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/cache/lazyrite.c
 * PURPOSE:         Logging and configuration routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
//#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KEVENT CcpLazyWriteEvent;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
CcWaitForCurrentLazyWriterActivity(VOID)
{
    KeWaitForSingleObject(&CcpLazyWriteEvent, Executive, KernelMode, FALSE, NULL);
    return STATUS_SUCCESS;
}

/* EOF */
