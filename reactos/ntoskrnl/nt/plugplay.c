/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/plugplay.c
 * PURPOSE:         Mysterious nt4 support for plug-and-play
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtGetPlugPlayEvent(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtPlugPlayControl(VOID)
{
   UNIMPLEMENTED;
}
