/* $Id:$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/error.c
 * PURPOSE:         Error reason setting/getting
 *
 * PROGRAMMERS:     David Welch
 */

/* INCLUDE *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* FUNCTIONS ***************************************************************/

/*
 * @unimplemented
 */
VOID
STDCALL
KiCoprocessorError(
    VOID
)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
VOID
STDCALL
KiUnexpectedInterrupt(
    VOID
)
{
	UNIMPLEMENTED;
}

NTSTATUS STDCALL 
NtRaiseHardError(IN NTSTATUS Status,
		 ULONG Unknown2,
		 ULONG Unknown3,
		 ULONG Unknown4,
		 ULONG Unknown5,
		 ULONG Unknown6)
{
  DPRINT1("Hard error %x\n", Status);
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
NtSetDefaultHardErrorPort(IN HANDLE PortHandle)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
}

/* EOF */
