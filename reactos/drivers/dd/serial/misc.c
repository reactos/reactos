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
	
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver(LowerDevice, Irp);
}

VOID STDCALL
SerialReceiveByte(
	IN PKDPC Dpc,
	IN PVOID pDeviceExtension, // real type PSERIAL_DEVICE_EXTENSION
	IN PVOID pByte,            // real type UCHAR
	IN PVOID Unused)
{
	PSERIAL_DEVICE_EXTENSION DeviceExtension;
	UCHAR Byte;
	KIRQL Irql;
	NTSTATUS Status;
	
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)pDeviceExtension;
	Byte = (UCHAR)(ULONG_PTR)pByte;
	DPRINT1("Serial: received byte on COM%lu: 0x%02x (%c)\n",
		DeviceExtension->ComPort, Byte, Byte);
	
	KeAcquireSpinLock(&DeviceExtension->InputBufferLock, &Irql);
	Status = PushCircularBufferEntry(&DeviceExtension->InputBuffer, Byte);
	if (!NT_SUCCESS(Status))
	{
		/* FIXME: count buffer overflow */
		return;
	}
	DPRINT1("Serial: push to buffer done\n");
	KeReleaseSpinLock(&DeviceExtension->InputBufferLock, Irql);
	InterlockedIncrement(&DeviceExtension->SerialPerfStats.ReceivedCount);
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
	NTSTATUS Status;
	
	DeviceExtension = (PSERIAL_DEVICE_EXTENSION)pDeviceExtension;
	ComPortBase = (PUCHAR)DeviceExtension->BaseAddress;
	
	KeAcquireSpinLock(&DeviceExtension->OutputBufferLock, &Irql);
	while (!IsCircularBufferEmpty(&DeviceExtension->OutputBuffer)
		&& READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_TBE)
	{
		Status = PopCircularBufferEntry(&DeviceExtension->OutputBuffer, &Byte);
		if (!NT_SUCCESS(Status))
			break;
		WRITE_PORT_UCHAR(SER_THR(ComPortBase), Byte);
		DeviceExtension->SerialPerfStats.TransmittedCount++;
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
	UCHAR Byte;
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
	
	/* FIXME: sometimes, update DeviceExtension->IER */
	/* FIXME: sometimes, update DeviceExtension->MCR */
	switch (Iir)
	{
		case SR_IIR_MSR_CHANGE:
		{
			DPRINT1("Serial: SR_IIR_MSR_CHANGE\n");
			DeviceExtension->MSR = READ_PORT_UCHAR(SER_MSR(ComPortBase));
			/* FIXME: what to do? */
			return TRUE;
		}
		case SR_IIR_THR_EMPTY:
		{
			DPRINT("Serial: SR_IIR_THR_EMPTY\n");
			return KeInsertQueueDpc(&DeviceExtension->SendByteDpc, NULL, NULL);
		}
		case SR_IIR_DATA_RECEIVED:
		{
			DPRINT1("Serial: SR_IIR_DATA_RECEIVED\n");
			while (READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_DR)
			{
				Byte = READ_PORT_UCHAR(SER_RBR(ComPortBase));
				DPRINT1("Serial: Byte received: 0x%02x (%c)\n", Byte, Byte);
				if (!KeInsertQueueDpc(&DeviceExtension->ReceivedByteDpc, (PVOID)(ULONG_PTR)Byte, NULL))
					break;
			}
			return TRUE;
		}
		case SR_IIR_ERROR:
		{
			/* FIXME: what to do? */
			DPRINT1("Serial: SR_IIR_ERROR\n");
			break;
			/*Error = READ_PORT_UCHAR( Self->Port + UART_LSR );
			if( Error & LSR_OVERRUN )
			    Self->WaitingReadBytes.PushBack( SerialFifo::OVERRUN );
			    DeviceExtension->SerialPerfStats.SerialOverrunErrorCount++;
			if( Error & LSR_PARITY_ERROR )
			    Self->WaitingReadBytes.PushBack( SerialFifo::PARITY );
			    DeviceExtension->SerialPerfStats.ParityErrorCount++;
			if( Error & LSR_FRAMING_ERROR )
			    Self->WaitingReadBytes.PushBack( SerialFifo::FRAMING );
			    DeviceExtension->SerialPerfStats.FrameErrorCount++;
			if( Error & LSR_BREAK )
			    Self->WaitingReadBytes.PushBack( SerialFifo::BREAK );
			if( Error & LSR_TIMEOUT )
			    Self->WaitingReadBytes.PushBack( SerialFifo::TIMEOUT );
			return KeInsertQueueDpc( &Self->DataInDpc, Self, 0 );*/
		}
	}
	return FALSE;
}
