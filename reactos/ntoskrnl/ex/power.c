/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtSetSystemPowerState(VOID)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtShutdownSystem(IN SHUTDOWN_ACTION Action)
{
   return(ZwShutdownSystem(Action));
}

NTSTATUS STDCALL ZwShutdownSystem(IN SHUTDOWN_ACTION Action)
{
   UNIMPLEMENTED;
}
