/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/adapter.c
 * PURPOSE:         DMA handling
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <internal/kernel.h>
#include <internal/linkage.h>
#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS IoAllocateAdapterChannel(PADAPTER_OBJECT AdapterObject,
				  PDEVICE_OBJECT DeviceObject,
				  ULONG NumberOfMapRegisters,
				  PDRIVER_CONTROL ExecutionRoutine,
				  PVOID Context)
{
   UNIMPLEMENTED;
}

BOOLEAN IoFlushAdapterBuffers(PADAPTER_OBJECT AdapterObject,
			      PMDL Mdl,
			      PVOID MapRegisterBase,
			      PVOID CurrentVa,
			      ULONG Length,
			      BOOLEAN WriteToDevice)
{
   UNIMPLEMENTED;
}

PHYSICAL_ADDRESS IoMapTransfer(PADAPTER_OBJECT AdapterObject,
			       PMDL Mdl,
			       PVOID MapRegisterBase,
			       PVOID CurrentVa,
			       PULONG Length,
			       BOOLEAN WriteToDevice)
{
   UNIMPLEMENTED;
}

VOID IoFreeAdapterChannel(PADAPTER_OBJECT AdapterObject)
{
   UNIMPLEMENTED;
}

VOID IoFreeMapRegisters(PADAPTER_OBJECT AdapterObject,
			PVOID MapRegisterBase,
			ULONG NumberOfMapRegisters)
{
   UNIMPLEMENTED;
}
