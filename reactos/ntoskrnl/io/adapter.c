/* $Id: adapter.c,v 1.7 2002/09/08 10:23:24 chorns Exp $
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

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* DATA **********************************************************************/

POBJECT_TYPE EXPORTED IoAdapterObjectType = NULL;	/* FIXME */
POBJECT_TYPE EXPORTED IoDeviceHandlerObjectType = NULL;	/* FIXME */
USHORT       EXPORTED IoDeviceHandlerObjectSize = 0;	/* FIXME */

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
