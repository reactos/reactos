/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/dd/serial/misc.c
 * PURPOSE:         Misceallenous operations
 * 
 * PROGRAMMERS:     Hervé Poussineau (poussine@freesurf.fr)
 */

#define NDEBUG
#include "serial.h"

NTSTATUS
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
	
	DPRINT1("Serial: Maybe our interrupt?\n"); /* FIXME */
	
	Iir = READ_PORT_UCHAR(SER_IIR(ComPortBase));
#if 0
	if (Iir == 0xff)
		return TRUE;
	CHECKPOINT1;
	Iir &= SR_IIR_ID_MASK;
	if (!(Iir & SR_IIR_SELF)) { return FALSE; }
	CHECKPOINT1;
#else
	Iir &= SR_IIR_ID_MASK;
	Iir |= SR_IIR_SELF;
#endif
	DPRINT1("Serial: Iir = 0x%x\n", Iir);
	
	switch (Iir)
	{
		case SR_IIR_MSR_CHANGE:
		{
			DPRINT1("Serial: SR_IIR_MSR_CHANGE\n");
			DeviceExtension->MSR = READ_PORT_UCHAR(SER_MSR(ComPortBase));
			/* FIXME: what to do? */
			//return KeInsertQueueDpc (&Self->MsrChangeDpc, Self, 0);
			//return TRUE;
			break;
		}
		case SR_IIR_THR_EMPTY:
		{
			DPRINT1("Serial: SR_IIR_THR_EMPTY\n");
			break;
			/*if (!Self->WaitingSendBytes.Empty() &&
    			(READ_PORT_UCHAR( Self->Port + UART_LSR ) & LSR_THR_EMPTY) )
    			WRITE_PORT_UCHAR( Self->Port + UART_THR, 
		      Self->WaitingSendBytes.PopFront() );
			return KeInsertQueueDpc( &Self->TransmitDpc, Self, 0 );*/
		}
		case SR_IIR_DATA_RECEIVED:
		{
			DPRINT1("Serial: SR_IIR_DATA_RECEIVED\n");
			if (READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_DR)
			{
				DPRINT1("Serial: Byte received: 0x%x\n", READ_PORT_UCHAR(SER_RBR(ComPortBase)));
				DeviceExtension->SerialPerfStats.ReceivedCount++;
				return TRUE;
			}
			break;
			/*if( READ_PORT_UCHAR( Self->Port + UART_LSR ) & LSR_DATA_RECEIVED ) 
    			Self->WaitingReadBytes.PushBack
				( READ_PORT_UCHAR( Self->Port + UART_RDR ) );
			return KeInsertQueueDpc( &Self->DataInDpc, Self, 0 );*/
		}
		case SR_IIR_ERROR:
		{
			DPRINT1("Serial: SR_IIR_ERROR\n");
			break;
			/*Error = READ_PORT_UCHAR( Self->Port + UART_LSR );
			if( Error & LSR_OVERRUN )
			    Self->WaitingReadBytes.PushBack( SerialFifo::OVERRUN );
			if( Error & LSR_PARITY_ERROR )
			    Self->WaitingReadBytes.PushBack( SerialFifo::PARITY );
			if( Error & LSR_FRAMING_ERROR )
			    Self->WaitingReadBytes.PushBack( SerialFifo::FRAMING );
			if( Error & LSR_BREAK )
			    Self->WaitingReadBytes.PushBack( SerialFifo::BREAK );
			if( Error & LSR_TIMEOUT )
			    Self->WaitingReadBytes.PushBack( SerialFifo::TIMEOUT );
			return KeInsertQueueDpc( &Self->DataInDpc, Self, 0 );*/
		}
	}
	return FALSE;
#if 0
		InterruptId = READ_PORT_UCHAR(SER_IIR(ComPortBase)) & SR_IIR_IID;
		DPRINT1("Serial: Interrupt catched: id = %x\n", InterruptId);
		/* FIXME: sometimes, update DeviceExtension->IER */
		/* FIXME: sometimes, update DeviceExtension->MCR */
		/* FIXME: sometimes, update DeviceExtension->MSR */
		switch (InterruptId)
		{
			case 3 << 1:
			{
				/* line status changed */
				DPRINT("Serial: Line status changed\n");
				break;
			}
			case 2 << 1:
			{
				/* data available */
				UCHAR ReceivedByte = READ_PORT_UCHAR(ComPortBase);
				DPRINT("Serial: Data available\n");
				DPRINT1("Serial: received %d\n", ReceivedByte);
				//Buffer[Information++] = ReceivedByte;
			}
			case 1 << 1:
			{
				/* transmit register empty */
				DPRINT("Serial: Transmit register empty\n");
			}
			case 0 << 1:
			{
				/* modem status change */
				UCHAR ReceivedByte = READ_PORT_UCHAR(SER_MSR(ComPortBase));
				DPRINT("Serial: Modem status change\n");
				DPRINT1("Serial: new status = 0x%02x\n", ReceivedByte);
			}
		}
		return TRUE;
	}
	else
		return FALSE;
#endif
}
