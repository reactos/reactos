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
	
	return Irp->AssociatedIrp.SystemBuffer;
}

static VOID
ReadBytes(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	PWORKITEM_DATA WorkItemData)
{
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	PUCHAR ComPortBase;
	ULONG Length;
	PUCHAR Buffer;
	ULONG Information = 0;
	LARGE_INTEGER SystemTime, ByteTimeoutTime;
	UCHAR ReceivedByte;
	BOOLEAN IsByteReceived;
	//KIRQL Irql;
	
	DPRINT("Serial: ReadBytes() called\n");
	
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;
	Length = IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length;
	Buffer = SerialGetUserBuffer(Irp);
	
	/* FIXME: remove disabling interrupts */
	WRITE_PORT_UCHAR(SER_IER(ComPortBase), DeviceExtension->IER & ~1);
	while (Length > 0)
	{
		/* Calculate dead line to receive the next byte */
		KeQuerySystemTime(&SystemTime);
		ByteTimeoutTime.QuadPart = SystemTime.QuadPart +
			WorkItemData->IntervalTimeout * 10000;
		
		IsByteReceived = FALSE;
		while (TRUE)
		{
#if 1
			if ((READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_DR) != 0)
			{
				ReceivedByte = READ_PORT_UCHAR(ComPortBase);
				DPRINT("Serial: received byte 0x%02x (%c)\n", ReceivedByte, ReceivedByte);
				IsByteReceived = TRUE;
				break;
			}
			else if (WorkItemData->DontWait &&
			         !(WorkItemData->ReadAtLeastOneByte && Information == 0))
			{
				DPRINT("Serial: read buffer empty\n");
				break;
			}
#else
			KeAcquireSpinLock(&DeviceExtension->InputBufferLock, &Irql);
			if (!IsCircularBufferEmpty(&DeviceExtension->InputBuffer))
			{
				CHECKPOINT1;
				PopCircularBufferEntry(&DeviceExtension->InputBuffer, &ReceivedByte);
				KeReleaseSpinLock(&DeviceExtension->InputBufferLock, Irql);
				DPRINT("Serial: reading byte from buffer 0x%02x (%c)\n", ReceivedByte, ReceivedByte);
				IsByteReceived = TRUE;
				break;
			}
			else if (WorkItemData->DontWait &&
			         !(WorkItemData->ReadAtLeastOneByte && Information == 0))
			{
				DPRINT("Serial: read buffer empty\n");
				break;
			}
			KeReleaseSpinLock(&DeviceExtension->InputBufferLock, Irql);
#endif
			if (IsByteReceived) break;
			KeQuerySystemTime(&SystemTime);
			if (WorkItemData->UseIntervalTimeout && Information > 0)
			{
				if (SystemTime.QuadPart >= ByteTimeoutTime.QuadPart)
					break;
			}
			if (WorkItemData->UseTotalTimeout)
			{
				if (SystemTime.QuadPart >= WorkItemData->TotalTimeoutTime.QuadPart)
					break;
			}
		}
		if (!IsByteReceived) break;
		Buffer[Information++] = ReceivedByte;
		Length--;
	}
	/* FIXME: remove enabling interrupts */
	WRITE_PORT_UCHAR(SER_IER(ComPortBase), DeviceExtension->IER);
	
	Irp->IoStatus.Information = Information;
	if (Information == 0)
		Irp->IoStatus.Status = STATUS_TIMEOUT;
	else
		Irp->IoStatus.Status = STATUS_SUCCESS;
}

static VOID STDCALL
SerialReadWorkItem(
	IN PDEVICE_OBJECT DeviceObject,
	IN PVOID pWorkItemData /* real type PWORKITEM_DATA */)
{
	PWORKITEM_DATA WorkItemData;
	PIRP Irp;
	
	DPRINT("Serial: SerialReadWorkItem() called\n");
	
	WorkItemData = (PWORKITEM_DATA)pWorkItemData;
	Irp = WorkItemData->Irp;
	
	ReadBytes(DeviceObject, Irp, WorkItemData);
	ExFreePoolWithTag(pWorkItemData, SERIAL_TAG);
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS STDCALL
SerialRead(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	ULONG Length;
	PUCHAR Buffer;
	PWORKITEM_DATA WorkItemData;
	PIO_WORKITEM WorkItem;
	NTSTATUS Status;
	
	DPRINT("Serial: IRP_MJ_READ\n");
	
	/* FIXME: pend operation if possible */
	
	Stack = IoGetCurrentIrpStackLocation(Irp);
	Length = Stack->Parameters.Read.Length;
	Buffer = SerialGetUserBuffer(Irp);
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	
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
	
	/* Allocate memory for parameters */
	WorkItemData = ExAllocatePoolWithTag(PagedPool, sizeof(WORKITEM_DATA), SERIAL_TAG);
	if (!WorkItemData)
	{
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto ByeBye;
	}
	RtlZeroMemory(WorkItemData, sizeof(WORKITEM_DATA));
	WorkItemData->Irp = Irp;
	
	/* Calculate time outs */
	if (DeviceExtension->SerialTimeOuts.ReadIntervalTimeout == INFINITE &&
		 DeviceExtension->SerialTimeOuts.ReadTotalTimeoutMultiplier == INFINITE &&
		 DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant > 0 &&
		 DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant < INFINITE)
	{
		/* read at least one byte, and at most bytes already received */
		WorkItemData->DontWait = TRUE;
		WorkItemData->ReadAtLeastOneByte = TRUE;
	}
	else if (DeviceExtension->SerialTimeOuts.ReadIntervalTimeout == INFINITE &&
	         DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant == 0 &&
	         DeviceExtension->SerialTimeOuts.ReadTotalTimeoutMultiplier == 0)
	{
		/* read only bytes that are already in buffer */
		WorkItemData->DontWait = TRUE;
	}
	else
	{
		/* use timeouts */
		if (DeviceExtension->SerialTimeOuts.ReadIntervalTimeout != 0)
			{
				WorkItemData->UseIntervalTimeout = TRUE;
				WorkItemData->IntervalTimeout = DeviceExtension->SerialTimeOuts.ReadIntervalTimeout;
		}
		if (DeviceExtension->SerialTimeOuts.ReadTotalTimeoutMultiplier != 0 ||
			 DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant != 0)
		{
			ULONG TotalTimeout;
			LARGE_INTEGER SystemTime;
			
			WorkItemData->UseTotalTimeout = TRUE;
			TotalTimeout = DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant +
				DeviceExtension->SerialTimeOuts.ReadTotalTimeoutMultiplier * Length;
			KeQuerySystemTime(&SystemTime);
			WorkItemData->TotalTimeoutTime.QuadPart = SystemTime.QuadPart +
				TotalTimeout * 10000;
		}
	}
	
	/* Pend IRP */
	WorkItem = IoAllocateWorkItem(DeviceObject);
	if (WorkItem)
	{
		IoQueueWorkItem(WorkItem, SerialReadWorkItem, DelayedWorkQueue, WorkItemData);
		IoMarkIrpPending(Irp);
		return STATUS_PENDING;
	}
	
	/* insufficient resources, we can't pend the Irp */
	CHECKPOINT;
	Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);
	if (!NT_SUCCESS(Status))
	{
		ExFreePoolWithTag(WorkItemData, SERIAL_TAG);
		goto ByeBye;
	}
	ReadBytes(DeviceObject, Irp, WorkItemData);
	Status = Irp->IoStatus.Status;
	
	IoReleaseRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);

ByeBye:
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
	KIRQL Irql;
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
	
	KeAcquireSpinLock(&DeviceExtension->OutputBufferLock, &Irql);
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
	KeReleaseSpinLock(&DeviceExtension->OutputBufferLock, Irql);
	IoReleaseRemoveLock(&DeviceExtension->RemoveLock, (PVOID)DeviceExtension->ComPort);

ByeBye:
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
