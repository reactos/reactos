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
	KIRQL Irql;
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
	
	KeAcquireSpinLock(&DeviceExtension->InputBufferLock, &Irql);
	while (Length-- > 0)
	{
		Status = PopCircularBufferEntry(&DeviceExtension->InputBuffer, &ReceivedByte);
		if (!NT_SUCCESS(Status))
			break;
		DPRINT("Serial: read from buffer 0x%x (%c)\n", ReceivedByte, ReceivedByte);
		Buffer[Information++] = ReceivedByte;
	}
	KeReleaseSpinLock(&DeviceExtension->InputBufferLock, Irql);
	if (Length > 0 &&
		!(DeviceExtension->SerialTimeOuts.ReadIntervalTimeout == INFINITE &&
		  DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant == 0 &&
		  DeviceExtension->SerialTimeOuts.ReadTotalTimeoutMultiplier == 0))
	{
		ULONG IntervalTimeout;
		ULONG TotalTimeout;
		BOOLEAN UseIntervalTimeout = FALSE;
		BOOLEAN UseTotalTimeout = FALSE;
		ULONG ThisByteTimeout;
		BOOLEAN IsByteReceived;
		ULONG i;
		/* Extract timeouts informations */
		if (DeviceExtension->SerialTimeOuts.ReadIntervalTimeout == INFINITE &&
			DeviceExtension->SerialTimeOuts.ReadTotalTimeoutMultiplier == INFINITE &&
			DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant > 0 &&
			DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant < INFINITE)
		{
			if (Information > 0)
			{
				/* don't read mode bytes */
				Length = 0;
			}
			else
			{
				/* read only one byte */
				UseTotalTimeout = TRUE;
				TotalTimeout = DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant;
				Length = 1;
			}
		}
		else
		{
			if (DeviceExtension->SerialTimeOuts.ReadIntervalTimeout != 0)
			{
				UseIntervalTimeout = TRUE;
				IntervalTimeout = DeviceExtension->SerialTimeOuts.ReadIntervalTimeout;
			}
			if (DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant != 0 ||
				DeviceExtension->SerialTimeOuts.ReadTotalTimeoutMultiplier != 0)
			{
				UseTotalTimeout = TRUE;
				TotalTimeout = DeviceExtension->SerialTimeOuts.ReadTotalTimeoutConstant +
					DeviceExtension->SerialTimeOuts.ReadTotalTimeoutMultiplier * Length;
			}
		}
		DPRINT("Serial: UseIntervalTimeout = %ws, IntervalTimeout = %lu\n",
			UseIntervalTimeout ? L"YES" : L"NO",
			UseIntervalTimeout ? IntervalTimeout : 0);
		DPRINT("Serial: UseTotalTimeout = %ws, TotalTimeout = %lu\n",
			UseTotalTimeout ? L"YES" : L"NO",
			UseTotalTimeout ? TotalTimeout : 0);
		
		/* FIXME: it should be better to use input buffer instead of
		 * disabling interrupts, and try to directly read for port! */
		
		/* FIXME: NtQueryPerformanceCounter gives a more accurate
		 * timer, but it is not available on all computers. First try
		 * NtQueryPerformanceCounter, and current method if it is not
		 * implemented. */
		
		/* FIXME: remove disabling interrupts */
		WRITE_PORT_UCHAR(SER_IER(ComPortBase), DeviceExtension->IER & ~1);
		while (Length > 0)
		{
			ThisByteTimeout = IntervalTimeout;
			IsByteReceived = FALSE;
			while (TRUE)
			{
				for (i = 0; i < 1000; i++)
				{
#if 1
					if ((READ_PORT_UCHAR(SER_LSR(ComPortBase)) & SR_LSR_DR) != 0)
					{
						ReceivedByte = READ_PORT_UCHAR(ComPortBase);
						DPRINT("Serial: received byte 0x%02x (%c)\n", ReceivedByte, ReceivedByte);
						IsByteReceived = TRUE;
						break;
					}
#else
					KeAcquireSpinLock(&DeviceExtension->InputBufferLock, &Irql);
					if (!IsCircularBufferEmpty(&DeviceExtension->InputBuffer))
					{
						PopCircularBufferEntry(&DeviceExtension->InputBuffer, &ReceivedByte);
						KeReleaseSpinLock(&DeviceExtension->InputBufferLock, Irql);
						DPRINT("Serial: reading byte from buffer 0x%02x (%c)\n", ReceivedByte, ReceivedByte);
						IsByteReceived = TRUE;
						break;
					}
					KeReleaseSpinLock(&DeviceExtension->InputBufferLock, Irql);
#endif
					KeStallExecutionProcessor(1);
				}
				if (IsByteReceived) break;
				if (UseIntervalTimeout)
				{
					if (ThisByteTimeout == 0) break; else ThisByteTimeout--;
				}
				if (UseTotalTimeout)
				{
					if (TotalTimeout == 0) break; else TotalTimeout--;
				}
			}
			if (!IsByteReceived) break;
			Buffer[Information++] = ReceivedByte;
			Length--;
		}
		/* FIXME: remove enabling interrupts */
		WRITE_PORT_UCHAR(SER_IER(ComPortBase), DeviceExtension->IER);
	}
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
