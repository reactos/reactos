/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/plugplay.c
 * PURPOSE:         Mysterious nt4 support for plug-and-play
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtPlugPlayControl (DWORD Unknown1,
                   DWORD Unknown2,
                   DWORD Unknown3)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS
STDCALL
NtGetPlugPlayEvent (ULONG Reserved1,
                    ULONG Reserved2,
                    PVOID Buffer,
                    ULONG BufferLength)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
}
