/* $Id: uuid.c,v 1.1 2004/11/12 12:04:32 ekohl Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           UUID generator
 * FILE:              kernel/ex/uuid.c
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtAllocateUuids(OUT PULARGE_INTEGER Time,
		OUT PULONG Range,
		OUT PULONG Sequence,
		OUT PUCHAR Seed)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtSetUuidSeed(IN PUCHAR Seed)
{
  UNIMPLEMENTED;
  return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
