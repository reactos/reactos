/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/dd/serial/create.c
 * PURPOSE:         Serial IRP_MJ_READ/IRP_MJ_WRITE operations
 * 
 * PROGRAMMERS:     Hervé Poussineau (poussine@freesurf.fr)
 */
/* FIXME: call IoAcquireRemoveLock/IoReleaseRemoveLock around each I/O operation */

#define NDEBUG
#include "serial.h"

static PVOID
SerialGetUserBuffer(IN PIRP Irp)
{
   ASSERT(Irp);

   if (Irp->MdlAddress)
      return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
   else
   	/* FIXME: try buffer */
      return Irp->UserBuffer;
}

NTSTATUS STDCALL
SerialRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	ULONG Information = 0;
	NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
	
	DPRINT1("Serial: IRP_MJ_READ unimplemented\n");
	
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS STDCALL
SerialWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	ULONG Length;
	ULONG Information = 0;
	PUCHAR Buffer;
	PUCHAR ComPortBase;
	ULONG i;
	NTSTATUS Status = STATUS_SUCCESS;
	
	DPRINT("Serial: IRP_MJ_WRITE\n");
	
	/* FIXME: pend operation if possible */
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	Length = Stack->Parameters.Write.Length;
	Buffer = SerialGetUserBuffer(Irp);
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;
	
	if (Stack->Parameters.Write.ByteOffset.QuadPart != 0 || Buffer == NULL)
	{
		Status = STATUS_INVALID_PARAMETER;
		goto ByeBye;
	}
	
	Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);
	if (!NT_SUCCESS(Status))
		goto ByeBye;
	
	for (i = 0; i < Length; i++)
	{
		/* verify if output buffer is not full */
		while ((READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_TBE) == 0)
			;
		WRITE_PORT_UCHAR(SER_THR(ComPortBase), Buffer[i]);
		DeviceExtension->SerialPerfStats.TransmittedCount++;
	}
	IoReleaseRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);
	
	Information = Length;

ByeBye:
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
