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
	PIO_STACK_LOCATION Stack;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	ULONG Length;
	ULONG Information = 0;
	PUCHAR Buffer;
	PUCHAR ComPortBase;
	UCHAR ReceivedByte;
	NTSTATUS Status = STATUS_SUCCESS;
	
	DPRINT("Serial: IRP_MJ_READ\n");
	
	/* FIXME: pend operation if possible */
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	Length = Stack->Parameters.Read.Length;
	Buffer = SerialGetUserBuffer(Irp);
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;
	
	if (Stack->Parameters.Read.ByteOffset.QuadPart != 0 || Buffer == NULL)
	{
		Status = STATUS_INVALID_PARAMETER;
		goto ByeBye;
	}
	
	if (Length == 0)
	{
		Status = STATUS_SUCCESS;
		goto ByeBye;
	}
	
	Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);
	if (!NT_SUCCESS(Status))
		goto ByeBye;
	
	/* FIXME: lock InputBuffer */
	while (Length-- > 0 && !IsCircularBufferEmpty(&DeviceExtension->InputBuffer))
	{
		Status = PopCircularBufferEntry(&DeviceExtension->InputBuffer, &ReceivedByte);
		if (!NT_SUCCESS(Status))
			break;
		DPRINT("Serial: read from buffer 0x%x (%c)\n", ReceivedByte, ReceivedByte);
		Buffer[Information++] = ReceivedByte;
	}
	if (Length > 0 &&
		!(DeviceExtension->SerialTimeOuts.ReadIntervalTimeout == INFINITE &&
		  DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant == 0 &&
		  DeviceExtension->SerialTimeOuts.ReadTotalTimeoutMultiplier == 0))
	{
		if (DeviceExtension->SerialTimeOuts.ReadIntervalTimeout == 0
			|| DeviceExtension->SerialTimeOuts.ReadTotalTimeoutMultiplier == 0)
		{
			DPRINT("Serial: we must wait for %lu characters!\n", Length);
#if 1
			/* Disable interrupts */
			WRITE_PORT_UCHAR(SER_IER((PUCHAR)DeviceExtension->BaseAddress), DeviceExtension->IER & ~1);
			
			/* Polling code */
			while (Length > 0)
			{
				while ((READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_DR) == 0)
					;
				ReceivedByte = READ_PORT_UCHAR(SER_RBR(ComPortBase));
				Buffer[Information++] = ReceivedByte;
				Length--;
			}
			/* Enable interrupts */
			WRITE_PORT_UCHAR(SER_IER((PUCHAR)DeviceExtension->BaseAddress), DeviceExtension->IER);
#else
			while (Length > 0)
			{
				if (!IsCircularBufferEmpty(&DeviceExtension->InputBuffer))
				{
					Status = PopCircularBufferEntry(&DeviceExtension->InputBuffer, &ReceivedByte);
					if (!NT_SUCCESS(Status))
						break;
					DPRINT1("Serial: read from buffer 0x%x (%c)\n", ReceivedByte, ReceivedByte);
					Buffer[Information++] = ReceivedByte;
					Length--;
				}
			}
#endif
		}
		else
		{
			/* FIXME: use ReadTotalTimeoutMultiplier and ReadTotalTimeoutConstant */
			DPRINT1("Serial: we must wait for %lu characters at maximum within %lu milliseconds! UNIMPLEMENTED\n",
				Length,
				Stack->Parameters.Read.Length * DeviceExtension->SerialTimeOuts.ReadTotalTimeoutMultiplier + DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant);
		}
	}
	/* FIXME: unlock InputBuffer */
	Status = STATUS_SUCCESS;
	
	IoReleaseRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);

ByeBye:
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
	NTSTATUS Status = STATUS_SUCCESS;
	
	DPRINT("Serial: IRP_MJ_WRITE\n");
	
	/* FIXME: pend operation if possible */
	/* FIXME: use write timeouts */
	
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
	
	/* FIXME: lock OutputBuffer */
	if (IsCircularBufferEmpty(&DeviceExtension->OutputBuffer))
	{
		/* Put the maximum amount of data in UART output buffer */
		while (Information < Length)
		{
			/* if UART output buffer is not full, directly write to it */
			if ((READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_TBE) != 0)
			{
				DPRINT("Serial: direct write 0x%02x (%c)\n", Buffer[Information], Buffer[Information]);
				WRITE_PORT_UCHAR(SER_THR(ComPortBase), Buffer[Information]);
				DeviceExtension->SerialPerfStats.TransmittedCount++;
				Information++;
			}
			else
				break;
		}
	}
	/* write remaining bytes into output buffer */
	while (Information < Length)
	{
		Status = PushCircularBufferEntry(&DeviceExtension->OutputBuffer, Buffer[Information]);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("Serial: buffer overrun on COM%lu\n", DeviceExtension->ComPort);
			DeviceExtension->SerialPerfStats.BufferOverrunErrorCount++;
			break;
		}
		DPRINT1("Serial: write to buffer 0x%02x\n", Buffer[Information]);
		Information++;
	}
	/* FIXME: unlock OutputBuffer */
	IoReleaseRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);

ByeBye:
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
