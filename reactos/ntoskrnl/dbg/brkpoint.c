/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/brkpoints.c
 * PURPOSE:         Handles breakpoints
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtSystemDebugControl(VOID)
{
   UNIMPLEMENTED;
}

VOID STDCALL DbgBreakPoint(VOID)
{
   __asm__("int $3\n\t");
}

VOID STDCALL DbgBreakPointWithStatus(ULONG Status)
{
   __asm__("mov %0, %%eax\n\t"
           "int $3\n\t"
           ::"m"(Status));
}
