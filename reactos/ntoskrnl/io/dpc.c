/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/io/dpc.c
 * PURPOSE:         io manager dpc functions
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID IoInitializeDpcRequest(PDEVICE_OBJECT DeviceObject, 
			    PIO_DPC_ROUTINE DpcRoutine)
{
   KeInitializeDpc(&DeviceObject->Dpc,(PKDEFERRED_ROUTINE)DpcRoutine,
		   DeviceObject);
}

VOID IoRequestDpc(PDEVICE_OBJECT DeviceObject,
		  PIRP Irp,
		  PVOID Context)
{
   KeInsertQueueDpc(&DeviceObject->Dpc,Irp,Context);
}
