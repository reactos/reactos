/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Serial port driver
 * FILE:            drivers/dd/serial/misc.c
 * PURPOSE:         Miscellaneous operations
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */
/* FIXME: call IoAcquireRemoveLock/IoReleaseRemoveLock around each I/O operation */

#include "serial.h"

#include <debug.h>


NTSTATUS NTAPI
ForwardIrpAndForget(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PDEVICE_OBJECT LowerDevice = ((PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerDevice;

	ASSERT(LowerDevice);

	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(LowerDevice, Irp);
}

VOID NTAPI
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
	ComPortBase = ULongToPtr(DeviceExtension->BaseAddress);

	KeAcquireSpinLock(&DeviceExtension->InputBufferLock, &Irql);
	while (READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_DATA_RECEIVED)
	{
		Byte = READ_PORT_UCHAR(SER_RBR(ComPortBase));
		INFO_(SERIAL, "Byte received on COM%lu: 0x%02x\n",
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

VOID NTAPI
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
	ComPortBase = ULongToPtr(DeviceExtension->BaseAddress);

	KeAcquireSpinLock(&DeviceExtension->OutputBufferLock, &Irql);
	while (!IsCircularBufferEmpty(&DeviceExtension->OutputBuffer)
		&& READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_THR_EMPTY)
	{
		Status = PopCircularBufferEntry(&DeviceExtension->OutputBuffer, &Byte);
		if (!NT_SUCCESS(Status))
			break;
		WRITE_PORT_UCHAR(SER_THR(ComPortBase), Byte);
		INFO_(SERIAL, "Byte sent to COM%lu: 0x%02x\n",
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

VOID NTAPI
SerialCompleteIrp(
	IN PKDPC Dpc,
	IN PVOID pDeviceExtension, // real type PSERIAL_DEVICE_EXTENSION
	IN PVOID pIrp, // real type PIRP
	IN PVOID Unused)
{
	IoCompleteRequest((PIRP)pIrp, IO_NO_INCREMENT);
}

BOOLEAN NTAPI
SerialInterruptService(
	IN PKINTERRUPT Interrupt,
	IN OUT PVOID ServiceContext)
{
	PDEVICE_OBJECT DeviceObject;
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	PUCHAR ComPortBase;
	UCHAR Iir;
	ULONG Events = 0;
	BOOLEAN ret = FALSE;

	/* FIXME: sometimes, produce SERIAL_EV_RXFLAG event */

	DeviceObject = (PDEVICE_OBJECT)ServiceContext;
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	ComPortBase = ULongToPtr(DeviceExtension->BaseAddress);

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
			TRACE_(SERIAL, "SR_IIR_MSR_CHANGE\n");

			MSR = READ_PORT_UCHAR(SER_MSR(ComPortBase));
			if (MSR & SR_MSR_CTS_CHANGED)
			{
				if (MSR & SR_MSR_CTS)
					KeInsertQueueDpc(&DeviceExtension->SendByteDpc, NULL, NULL);
				else
				{
					; /* FIXME: stop transmission */
				}
				Events |= SERIAL_EV_CTS;
			}
			if (MSR & SR_MSR_DSR_CHANGED)
			{
				if (MSR & SR_MSR_DSR)
					KeInsertQueueDpc(&DeviceExtension->ReceivedByteDpc, NULL, NULL);
				else
				{
					; /* FIXME: stop reception */
				}
				Events |= SERIAL_EV_DSR;
			}
			if (MSR & SR_MSR_RI_CHANGED)
			{
				INFO_(SERIAL, "SR_MSR_RI_CHANGED changed: now %d\n", MSR & SI_MSR_RI);
				Events |= SERIAL_EV_RING;
			}
			if (MSR & SR_MSR_DCD_CHANGED)
			{
				INFO_(SERIAL, "SR_MSR_DCD_CHANGED changed: now %d\n", MSR & SR_MSR_DCD);
				Events |= SERIAL_EV_RLSD;
			}
			IER = READ_PORT_UCHAR(SER_IER(ComPortBase));
			WRITE_PORT_UCHAR(SER_IER(ComPortBase), IER | SR_IER_MSR_CHANGE);

			ret = TRUE;
			goto done;
		}
		case SR_IIR_THR_EMPTY:
		{
			TRACE_(SERIAL, "SR_IIR_THR_EMPTY\n");

			KeInsertQueueDpc(&DeviceExtension->SendByteDpc, NULL, NULL);
			Events |= SERIAL_EV_TXEMPTY;

			ret = TRUE;
			goto done;
		}
		case SR_IIR_DATA_RECEIVED:
		{
			ULONG AlreadyReceivedBytes, Limit;
			TRACE_(SERIAL, "SR_IIR_DATA_RECEIVED\n");

			KeInsertQueueDpc(&DeviceExtension->ReceivedByteDpc, NULL, NULL);
			Events |= SERIAL_EV_RXCHAR;

			/* Check if buffer will be 80% full */
			AlreadyReceivedBytes = GetNumberOfElementsInCircularBuffer(
				&DeviceExtension->InputBuffer) * 5;
			Limit = DeviceExtension->InputBuffer.Length * 4;
			if (AlreadyReceivedBytes < Limit && AlreadyReceivedBytes + 1 >= Limit)
			{
				/* Buffer is full at 80% */
				Events |= SERIAL_EV_RX80FULL;
			}

			ret = TRUE;
			goto done;
		}
		case SR_IIR_ERROR:
		{
			UCHAR LSR;
			TRACE_(SERIAL, "SR_IIR_ERROR\n");

			LSR = READ_PORT_UCHAR(SER_LSR(ComPortBase));
			if (LSR & SR_LSR_OVERRUN_ERROR)
			{
				InterlockedIncrement((PLONG)&DeviceExtension->SerialPerfStats.SerialOverrunErrorCount);
				Events |= SERIAL_EV_ERR;
			}
			if (LSR & SR_LSR_PARITY_ERROR)
			{
				InterlockedIncrement((PLONG)&DeviceExtension->SerialPerfStats.ParityErrorCount);
				Events |= SERIAL_EV_ERR;
			}
			if (LSR & SR_LSR_FRAMING_ERROR)
			{
				InterlockedIncrement((PLONG)&DeviceExtension->SerialPerfStats.FrameErrorCount);
				Events |= SERIAL_EV_ERR;
			}
			if (LSR & SR_LSR_BREAK_INT)
			{
				InterlockedIncrement((PLONG)&DeviceExtension->BreakInterruptErrorCount);
				Events |= SERIAL_EV_BREAK;
			}

			ret = TRUE;
			goto done;
		}
	}

done:
	if (!ret)
		return FALSE;
	if (DeviceExtension->WaitOnMaskIrp && (Events & DeviceExtension->WaitMask))
	{
		/* Finish pending IRP */
		PULONG pEvents = (PULONG)DeviceExtension->WaitOnMaskIrp->AssociatedIrp.SystemBuffer;

		DeviceExtension->WaitOnMaskIrp->IoStatus.Status = STATUS_SUCCESS;
		DeviceExtension->WaitOnMaskIrp->IoStatus.Information = sizeof(ULONG);
		*pEvents = Events;
		KeInsertQueueDpc(&DeviceExtension->CompleteIrpDpc, DeviceExtension->WaitOnMaskIrp, NULL);

		/* We are now ready to handle another IRP, even if this one is not completed */
		DeviceExtension->WaitOnMaskIrp = NULL;
		return TRUE;
	}
	return TRUE;
}
