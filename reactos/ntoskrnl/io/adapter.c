/* $Id$
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
#include <internal/debug.h>

/* DATA **********************************************************************/

POBJECT_TYPE EXPORTED IoAdapterObjectType = NULL;	/* FIXME */
POBJECT_TYPE EXPORTED IoDeviceHandlerObjectType = NULL;	/* FIXME */
ULONG        EXPORTED IoDeviceHandlerObjectSize = 0;    /* FIXME */

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
IoAllocateAdapterChannel (PADAPTER_OBJECT	AdapterObject,
			  PDEVICE_OBJECT	DeviceObject,
			  ULONG		NumberOfMapRegisters,
			  PDRIVER_CONTROL	ExecutionRoutine,
			  PVOID		Context)
{
  DeviceObject->Queue.Wcb.DeviceObject = DeviceObject;
  DeviceObject->Queue.Wcb.DeviceContext = Context;

  return HalAllocateAdapterChannel( AdapterObject,
				    &DeviceObject->Queue.Wcb,
				    NumberOfMapRegisters,
				    ExecutionRoutine);
}


/* NOTE: Missing IoXXXAdapter finctions in HAL.DLL */


/* EOF */
