/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Serial port driver
 * FILE:            drivers/dd/serial/create.c
 * PURPOSE:         Serial IRP_MJ_READ/IRP_MJ_WRITE operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "serial.h"

static IO_WORKITEM_ROUTINE SerialReadWorkItem;

static PVOID
SerialGetUserBuffer(IN PIRP Irp)
{
	ASSERT(Irp);

	if (Irp->MdlAddress)
		return Irp->MdlAddress;
	else
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
	UCHAR ReceivedByte;
	KTIMER TotalTimeoutTimer;
	KIRQL Irql;
	ULONG ObjectCount;
	PVOID ObjectsArray[2];
	ULONG_PTR Information = 0;
	NTSTATUS Status;

	ASSERT(DeviceObject);
	ASSERT(WorkItemData);

	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ComPortBase = ULongToPtr(DeviceExtension->BaseAddress);
	Length = IoGetCurrentIrpStackLocation(Irp)->Parameters.Read.Length;
	Buffer = SerialGetUserBuffer(Irp);

	INFO_(SERIAL, "UseIntervalTimeout = %s, IntervalTimeout = %lu\n",
		WorkItemData->UseIntervalTimeout ? "YES" : "NO",
		WorkItemData->UseIntervalTimeout ? WorkItemData->IntervalTimeout.QuadPart : 0);
	INFO_(SERIAL, "UseTotalTimeout = %s\n",
		WorkItemData->UseTotalTimeout ? "YES" : "NO");

	ObjectCount = 1;
	ObjectsArray[0] = &DeviceExtension->InputBufferNotEmpty;
	if (WorkItemData->UseTotalTimeout)
	{
		KeInitializeTimer(&TotalTimeoutTimer);
		KeSetTimer(&TotalTimeoutTimer, WorkItemData->TotalTimeoutTime, NULL);
		ObjectsArray[ObjectCount] = &TotalTimeoutTimer;
		ObjectCount++;
	}

	/* while buffer is not fully filled */
	while (Length > 0)
	{
		/* read already received bytes from buffer */
		KeAcquireSpinLock(&DeviceExtension->InputBufferLock, &Irql);
		while (!IsCircularBufferEmpty(&DeviceExtension->InputBuffer)
			&& Length > 0)
		{
			PopCircularBufferEntry(&DeviceExtension->InputBuffer, &ReceivedByte);
			INFO_(SERIAL, "Reading byte from buffer: 0x%02x\n", ReceivedByte);

			Buffer[Information++] = ReceivedByte;
			Length--;
		}
		KeClearEvent(&DeviceExtension->InputBufferNotEmpty);
		KeReleaseSpinLock(&DeviceExtension->InputBufferLock, Irql);

		if (WorkItemData->DontWait
			&& !(WorkItemData->ReadAtLeastOneByte && Information == 0))
		{
			INFO_(SERIAL, "Buffer empty. Don't wait more bytes\n");
			break;
		}

		Status = KeWaitForMultipleObjects(
			ObjectCount,
			ObjectsArray,
			WaitAny,
			Executive,
			KernelMode,
			FALSE,
			(WorkItemData->UseIntervalTimeout && Information > 0) ? &WorkItemData->IntervalTimeout : NULL,
			NULL);

		if (Status == STATUS_TIMEOUT /* interval timeout */
			|| Status == STATUS_WAIT_1) /* total timeout */
		{
			TRACE_(SERIAL, "Timeout when reading bytes. Status = 0x%08lx\n", Status);
			break;
		}
	}

	/* stop total timeout timer */
	if (WorkItemData->UseTotalTimeout)
		KeCancelTimer(&TotalTimeoutTimer);

	Irp->IoStatus.Information = Information;
	if (Information == 0)
		Irp->IoStatus.Status = STATUS_TIMEOUT;
	else
		Irp->IoStatus.Status = STATUS_SUCCESS;
}

static VOID NTAPI
SerialReadWorkItem(
	IN PDEVICE_OBJECT DeviceObject,
	IN PVOID pWorkItemData /* real type PWORKITEM_DATA */)
{
	PWORKITEM_DATA WorkItemData;
	PIRP Irp;

	TRACE_(SERIAL, "SerialReadWorkItem() called\n");

	WorkItemData = (PWORKITEM_DATA)pWorkItemData;
	Irp = WorkItemData->Irp;

	ReadBytes(DeviceObject, Irp, WorkItemData);

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	IoFreeWorkItem(WorkItemData->IoWorkItem);
	ExFreePoolWithTag(pWorkItemData, SERIAL_TAG);
}

NTSTATUS NTAPI
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

	TRACE_(SERIAL, "IRP_MJ_READ\n");

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
				WorkItemData->IntervalTimeout.QuadPart = DeviceExtension->SerialTimeOuts.ReadIntervalTimeout;
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
		WorkItemData->IoWorkItem = WorkItem;
		IoMarkIrpPending(Irp);
		IoQueueWorkItem(WorkItem, SerialReadWorkItem, DelayedWorkQueue, WorkItemData);
		return STATUS_PENDING;
	}

	/* Insufficient resources, we can't pend the Irp */
	INFO_(SERIAL, "Insufficient resources\n");
	Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, ULongToPtr(DeviceExtension->ComPort));
	if (!NT_SUCCESS(Status))
	{
		ExFreePoolWithTag(WorkItemData, SERIAL_TAG);
		goto ByeBye;
	}
	ReadBytes(DeviceObject, Irp, WorkItemData);
	Status = Irp->IoStatus.Status;

	IoReleaseRemoveLock(&DeviceExtension->RemoveLock, ULongToPtr(DeviceExtension->ComPort));

ByeBye:
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

NTSTATUS NTAPI
SerialWrite(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	ULONG Length;
	ULONG_PTR Information = 0;
	PUCHAR Buffer;
	PUCHAR ComPortBase;
	KIRQL Irql;
	NTSTATUS Status = STATUS_SUCCESS;

	TRACE_(SERIAL, "IRP_MJ_WRITE\n");

	/* FIXME: pend operation if possible */
	/* FIXME: use write timeouts */

	Stack = IoGetCurrentIrpStackLocation(Irp);
	Length = Stack->Parameters.Write.Length;
	Buffer = SerialGetUserBuffer(Irp);
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ComPortBase = ULongToPtr(DeviceExtension->BaseAddress);

	if (Stack->Parameters.Write.ByteOffset.QuadPart != 0 || Buffer == NULL)
	{
		Status = STATUS_INVALID_PARAMETER;
		goto ByeBye;
	}

	Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, ULongToPtr(DeviceExtension->ComPort));
	if (!NT_SUCCESS(Status))
		goto ByeBye;

	/* push  bytes into output buffer */
	KeAcquireSpinLock(&DeviceExtension->OutputBufferLock, &Irql);
	while (Information < Length)
	{
		Status = PushCircularBufferEntry(&DeviceExtension->OutputBuffer, Buffer[Information]);
		if (!NT_SUCCESS(Status))
		{
			if (Status == STATUS_BUFFER_TOO_SMALL)
			{
				KeReleaseSpinLock(&DeviceExtension->OutputBufferLock, Irql);
				SerialSendByte(NULL, DeviceExtension, NULL, NULL);
				KeAcquireSpinLock(&DeviceExtension->OutputBufferLock, &Irql);
				continue;
			}
			else
			{
				WARN_(SERIAL, "Buffer overrun on COM%lu\n", DeviceExtension->ComPort);
				DeviceExtension->SerialPerfStats.BufferOverrunErrorCount++;
				break;
			}
		}
		Information++;
	}
	KeReleaseSpinLock(&DeviceExtension->OutputBufferLock, Irql);
	IoReleaseRemoveLock(&DeviceExtension->RemoveLock, ULongToPtr(DeviceExtension->ComPort));

	/* send bytes */
	SerialSendByte(NULL, DeviceExtension, NULL, NULL);

ByeBye:
	Irp->IoStatus.Information = Information;
	Irp->IoStatus.Status = Status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}
