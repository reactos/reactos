/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/base/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
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
