/* $Id: adapter.c,v 1.3 2000/05/09 16:13:49 ekohl Exp $
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

NTSTATUS
STDCALL
IoAllocateAdapterChannel (
	PADAPTER_OBJECT	AdapterObject,
	PDEVICE_OBJECT	DeviceObject,
	ULONG		NumberOfMapRegisters,
	PDRIVER_CONTROL	ExecutionRoutine,
	PVOID		Context
	)
{
	UNIMPLEMENTED;
}


/* NOTE: Missing IoXXXAdapter finctions in HAL.DLL */


/* EOF */
