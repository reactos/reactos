/* $Id:
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/dd/serial/misc.c
 * PURPOSE:         Misceallenous operations
 *
 * PROGRAMMERS:     Hervé Poussineau (poussine@freesurf.fr)
 */
/* FIXME: call IoAcquireRemoveLock/IoReleaseRemoveLock around each I/O operation */

#define NDEBUG
#include "serial.h"

NTSTATUS STDCALL
ForwardIrpAndWaitCompletion(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp,
	IN PVOID Context)
{
	if (Irp->PendingReturned)
		KeSetEvent((PKEVENT)Context, IO_NO_INCREMENT, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
ForwardIrpAndWait(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PDEVICE_OBJECT LowerDevice = ((PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;
	KEVENT Event;
	NTSTATUS Status;

	ASSERT(LowerDevice);

	KeInitializeEvent(&Event, NotificationEvent, FALSE);
	IoCopyCurrentIrpStackLocationToNext(Irp);

	DPRINT("Serial: Calling lower device %p [%wZ]\n", LowerDevice, &LowerDevice->DriverObject->DriverName);
	IoSetCompletionRoutine(Irp, ForwardIrpAndWaitCompletion, &Event, TRUE, TRUE, TRUE);

	Status = IoCallDriver(LowerDevice, Irp);
	if (Status == STATUS_PENDING)
	{
		Status = KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
		if (NT_SUCCESS(Status))
			Status = Irp->IoStatus.Status;
	}

	return Status;
}

NTSTATUS STDCALL
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PDEVICE_OBJECT LowerDevice = ((PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;

	ASSERT(LowerDevice);

	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(LowerDevice, Irp);
}

VOID STDCALL
SerialReceiveByte(
	IN PKDPC Dpc,
	IN PVOID pDeviceExtension, // real type PSERIAL_DEVICE_EXTENSION
	IN PVOID Unused1,
	IN PVOID Unused2)
{
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	PUCHAR ComPortBase;
	UCHAR Byte;
	KIRQL Irql;
	UCHAR IER;
	NTSTATUS Status;

	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)pDeviceExtension;
	ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;

	KeAcquireSpinLock(&DeviceExtension->InputBufferLock, &Irql);
	while (READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_DATA_RECEIVED)
	{
		Byte = READ_PORT_UCHAR(SER_RBR(ComPortBase));
		DPRINT("Serial: Byte received on COM%lu: 0x%02x\n",
			DeviceExtension->ComPort, Byte);
		Status = PushCircularBufferEntry(&DeviceExtension->InputBuffer, Byte);
		if (NT_SUCCESS(Status))
			DeviceExtension->SerialPerfStats.ReceivedCount++;
		else
			DeviceExtension->SerialPerfStats.BufferOverrunErrorCount++;
	}
	KeSetEvent(&DeviceExtension->InputBufferNotEmpty, 0, FALSE);
	KeReleaseSpinLock(&DeviceExtension->InputBufferLock, Irql);

	/* allow new interrupts */
	IER = READ_PORT_UCHAR(SER_IER(ComPortBase));
	WRITE_PORT_UCHAR(SER_IER(ComPortBase), IER | SR_IER_DATA_RECEIVED);
}

VOID STDCALL
SerialSendByte(
	IN PKDPC Dpc,
	IN PVOID pDeviceExtension, // real type PSERIAL_DEVICE_EXTENSION
	IN PVOID Unused1,
	IN PVOID Unused2)
{
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	PUCHAR ComPortBase;
	UCHAR Byte;
	KIRQL Irql;
	UCHAR IER;
	NTSTATUS Status;

	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)pDeviceExtension;
	ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;

	KeAcquireSpinLock(&DeviceExtension->OutputBufferLock, &Irql);
	while (!IsCircularBufferEmpty(&DeviceExtension->OutputBuffer)
		&& READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_THR_EMPTY)
	{
		Status = PopCircularBufferEntry(&DeviceExtension->OutputBuffer, &Byte);
		if (!NT_SUCCESS(Status))
			break;
		WRITE_PORT_UCHAR(SER_THR(ComPortBase), Byte);
		DPRINT("Serial: Byte sent to COM%lu: 0x%02x\n",
			DeviceExtension->ComPort, Byte);
		DeviceExtension->SerialPerfStats.TransmittedCount++;
	}
	if (!IsCircularBufferEmpty(&DeviceExtension->OutputBuffer))
	{
		/* allow new interrupts */
		IER = READ_PORT_UCHAR(SER_IER(ComPortBase));
		WRITE_PORT_UCHAR(SER_IER(ComPortBase), IER | SR_IER_THR_EMPTY);
	}
	KeReleaseSpinLock(&DeviceExtension->OutputBufferLock, Irql);
}

BOOLEAN STDCALL
SerialInterruptService(
	IN PKINTERRUPT Interrupt,
	IN OUT PVOID ServiceContext)
{
	PDEVICE_OBJECT DeviceObject;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	PUCHAR ComPortBase;
	UCHAR Iir;

	DeviceObject = (PDEVICE_OBJECT)ServiceContext;
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;

	Iir = READ_PORT_UCHAR(SER_IIR(ComPortBase));
	if (Iir == 0xff)
		return TRUE;
	Iir &= SR_IIR_ID_MASK;
	if ((Iir & SR_IIR_SELF) != 0) { return FALSE; }

	switch (Iir)
	{
		case SR_IIR_MSR_CHANGE:
		{
			UCHAR MSR, IER;
			DPRINT("Serial: SR_IIR_MSR_CHANGE\n");

			MSR = READ_PORT_UCHAR(SER_MSR(ComPortBase));
			if (MSR & SR_MSR_CTS_CHANGED)
			{
				if (MSR & SR_MSR_CTS)
					KeInsertQueueDpc(&DeviceExtension->SendByteDpc, NULL, NULL);
				else
					; /* FIXME: stop transmission */
			}
			if (MSR & SR_MSR_DSR_CHANGED)
			{
				if (MSR & SR_MSR_DSR)
					KeInsertQueueDpc(&DeviceExtension->ReceivedByteDpc, NULL, NULL);
				else
					; /* FIXME: stop reception */
			}
			IER = READ_PORT_UCHAR(SER_IER(ComPortBase));
			WRITE_PORT_UCHAR(SER_IER(ComPortBase), IER | SR_IER_MSR_CHANGE);
			return TRUE;
		}
		case SR_IIR_THR_EMPTY:
		{
			DPRINT("Serial: SR_IIR_THR_EMPTY\n");

			KeInsertQueueDpc(&DeviceExtension->SendByteDpc, NULL, NULL);
			return TRUE;
		}
		case SR_IIR_DATA_RECEIVED:
		{
			DPRINT("Serial: SR_IIR_DATA_RECEIVED\n");

			KeInsertQueueDpc(&DeviceExtension->ReceivedByteDpc, NULL, NULL);
			return TRUE;
		}
		case SR_IIR_ERROR:
		{
			UCHAR LSR;
			DPRINT("Serial: SR_IIR_ERROR\n");

			LSR = READ_PORT_UCHAR(SER_LSR(ComPortBase));
			if (LSR & SR_LSR_OVERRUN_ERROR)
				InterlockedIncrement((PLONG)&DeviceExtension->SerialPerfStats.SerialOverrunErrorCount);
			if (LSR & SR_LSR_PARITY_ERROR)
				InterlockedIncrement((PLONG)&DeviceExtension->SerialPerfStats.ParityErrorCount);
			if (LSR & SR_LSR_FRAMING_ERROR)
				InterlockedIncrement((PLONG)&DeviceExtension->SerialPerfStats.FrameErrorCount);
			if (LSR & SR_LSR_BREAK_INT)
				InterlockedIncrement((PLONG)&DeviceExtension->BreakInterruptErrorCount);

			return TRUE;
		}
	}
	return FALSE;
}
