/* $Id: adapter.c,v 1.6 2002/09/07 15:12:52 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/adapter.c
 * PURPOSE:         DMA handling
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* DATA **********************************************************************/

POBJECT_TYPE IoAdapterObjectType = NULL;	/* FIXME */
POBJECT_TYPE IoDeviceHandlerObjectType = NULL;	/* FIXME */
ULONG        IoDeviceHandlerObjectSize = 0;	/* FIXME */

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL
IoAllocateAdapterChannel (PADAPTER_OBJECT	AdapterObject,
			  PDEVICE_OBJECT	DeviceObject,
			  ULONG		NumberOfMapRegisters,
			  PDRIVER_CONTROL	ExecutionRoutine,
			  PVOID		Context)
{
  return HalAllocateAdapterChannel( AdapterObject,
				    DeviceObject,
				    NumberOfMapRegisters,
				    ExecutionRoutine,
				    Context );
}


/* NOTE: Missing IoXXXAdapter finctions in HAL.DLL */


/* EOF */
