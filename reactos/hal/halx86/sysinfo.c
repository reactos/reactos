/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/sysinfo.c
 * PURPOSE:         Getting system information
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <hal.h>
#include <bus.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
HalpQuerySystemInformation(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
			   IN ULONG BufferSize,
			   IN OUT PVOID Buffer,
			   OUT PULONG ReturnedLength)
{
  ULONG DataLength;
  NTSTATUS Status;

  DPRINT1("HalpQuerySystemInformation() called\n");

  *ReturnedLength = 0;

  DataLength = 0;

  switch(InformationClass)
    {
#if 0
      case HalInstalledBusInformation:
	Status = HalpQueryBusInformation(BufferSize,
					 Buffer,
					 ReturnedLength);
	break;
#endif

      default:
	DataLength = 0;
	Status = STATUS_INVALID_LEVEL;
	break;
    }

  if (DataLength != 0)
    {
      if (DataLength > BufferSize)
	DataLength = BufferSize;

//      RtlCopyMemory();

      *ReturnedLength = DataLength;
    }

  return(Status);
}


#if 0
NTSTATUS
HalpSetSystemInformation(VOID)
{
   UNIMPLEMENTED;
}
#endif

/* EOF */
