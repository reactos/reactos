/* $Id $
 *
 * COPYRIGHT:  See COPYING in the top directory
 * PROJECT:    ReactOS kernel v0.0.2
 * FILE:       ntoskrnl/ke/error.c
 * PURPOSE:    Error reason setting/getting
 * PROGRAMMER: David Welch
 * UPDATE HISTORY:
 *             16/4/98: Created
 */

/* INCLUDE *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS ***************************************************************/

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
}

/* EOF */
