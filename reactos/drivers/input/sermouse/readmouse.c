/*
 * PROJECT:     ReactOS Serial mouse driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/sermouse/fdo.c
 * PURPOSE:     Read mouse moves and send them to mouclass
 * PROGRAMMERS: Copyright Jason Filby (jasonfilby@yahoo.com)
                Copyright Filip Navara (xnavara@volny.cz)
                Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "sermouse.h"

static NTSTATUS
SermouseDeviceIoControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN ULONG CtlCode,
	IN PVOID InputBuffer OPTIONAL,
	IN ULONG InputBufferSize,
	IN OUT PVOID OutputBuffer OPTIONAL,
	IN OUT PULONG OutputBufferSize)
{
	KEVENT Event;
	PIRP Irp;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;

	KeInitializeEvent(&Event, NotificationEvent, FALSE);

	Irp = IoBuildDeviceIoControlRequest(CtlCode,
		DeviceObject,
		InputBuffer,
		InputBufferSize,
		OutputBuffer,
		(OutputBufferSize) ? *OutputBufferSize : 0,
		FALSE,
		&Event,
		&IoStatus);
	if (Irp == NULL)
	{
		WARN_(SERMOUSE, "IoBuildDeviceIoControlRequest() failed\n");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	Status = IoCallDriver(DeviceObject, Irp);

	if (Status == STATUS_PENDING)
	{
		INFO_(SERMOUSE, "Operation pending\n");
		KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
		Status = IoStatus.Status;
	}

	if (OutputBufferSize)
	{
		*OutputBufferSize = (ULONG)IoStatus.Information;
	}

	return Status;
}

VOID NTAPI
SermouseDeviceWorker(
	PVOID Context)
{
	PSERMOUSE_DEVICE_EXTENSION DeviceExtension;
	PDEVICE_OBJECT LowerDevice;
	UCHAR Buffer[PACKET_BUFFER_SIZE];
	PIRP Irp;
	IO_STATUS_BLOCK ioStatus;
	KEVENT event;
	PUCHAR PacketBuffer;
	UCHAR ReceivedByte;
	ULONG Queue;
	PMOUSE_INPUT_DATA Input;
	ULONG ButtonsDifference;
	KIRQL OldIrql;
	ULONG i;
	ULONG Fcr;
	ULONG BaudRate;
	SERIAL_TIMEOUTS Timeouts;
	SERIAL_LINE_CONTROL LCR;
	LARGE_INTEGER Zero;
	NTSTATUS Status;

	TRACE_(SERMOUSE, "SermouseDeviceWorker() called\n");

	DeviceExtension = (PSERMOUSE_DEVICE_EXTENSION)((PDEVICE_OBJECT)Context)->DeviceExtension;
	LowerDevice = DeviceExtension->LowerDevice;
	Zero.QuadPart = 0;
	PacketBuffer = DeviceExtension->PacketBuffer;

	ASSERT(LowerDevice);

	/* Initialize device extension */
	DeviceExtension->ActiveQueue = 0;
	DeviceExtension->PacketBufferPosition = 0;
	DeviceExtension->PreviousButtons = 0;

	/* Initialize serial port */
	Fcr = 0;
	Status = SermouseDeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_FIFO_CONTROL,
		&Fcr, sizeof(Fcr), NULL, NULL);
	if (!NT_SUCCESS(Status)) PsTerminateSystemThread(Status);
	/* Set serial port speed */
	BaudRate = DeviceExtension->AttributesInformation.SampleRate * 8;
	Status = SermouseDeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_BAUD_RATE,
		&BaudRate, sizeof(BaudRate), NULL, NULL);
	if (!NT_SUCCESS(Status)) PsTerminateSystemThread(Status);
	/* Set LCR */
	LCR.WordLength = 7;
	LCR.Parity = NO_PARITY;
	LCR.StopBits = STOP_BIT_1;
	Status = SermouseDeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_LINE_CONTROL,
		&LCR, sizeof(LCR), NULL, NULL);
	if (!NT_SUCCESS(Status)) PsTerminateSystemThread(Status);

	/* Set timeouts */
	Timeouts.ReadTotalTimeoutConstant = Timeouts.ReadTotalTimeoutMultiplier = 0;
	Timeouts.ReadIntervalTimeout = 100;
	Timeouts.WriteTotalTimeoutMultiplier = Timeouts.WriteTotalTimeoutConstant = 0;
	Status = SermouseDeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_TIMEOUTS,
		&Timeouts, sizeof(Timeouts), NULL, NULL);
	if (!NT_SUCCESS(Status)) PsTerminateSystemThread(Status);

	/* main read loop */
	RtlZeroMemory(Buffer, PACKET_BUFFER_SIZE);
	while (TRUE)
	{
		Status = KeWaitForSingleObject(
			&DeviceExtension->StopWorkerThreadEvent,
			Executive,
			KernelMode,
			TRUE,
			&Zero);
		if (Status != STATUS_TIMEOUT)
		{
			/* we need to stop the worker thread */
			KeResetEvent(&DeviceExtension->StopWorkerThreadEvent);
			break;
		}

		KeInitializeEvent(&event, NotificationEvent, FALSE);
		Irp = IoBuildSynchronousFsdRequest(
			IRP_MJ_READ,
			LowerDevice,
			Buffer, PACKET_BUFFER_SIZE,
			&Zero,
			&event,
			&ioStatus);
		if (!Irp)
		{
			/* no memory actually, try later */
			CHECKPOINT;
			KeStallExecutionProcessor(10);
			continue;
		}

		Status = IoCallDriver(LowerDevice, Irp);
		if (Status == STATUS_PENDING)
		{
			KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
			Status = ioStatus.Status;
		}

		if (!NT_SUCCESS(Status))
			continue;

		/* Read all available data and process */
		for (i = 0; i < ioStatus.Information; i++)
		{
			ReceivedByte = Buffer[i];
			INFO_(SERMOUSE, "ReceivedByte 0x%02x\n", ReceivedByte);

			/* Synchronize */
			if ((ReceivedByte & 0x40) == 0x40)
				DeviceExtension->PacketBufferPosition = 0;

			PacketBuffer[DeviceExtension->PacketBufferPosition] = ReceivedByte & 0x7f;
			DeviceExtension->PacketBufferPosition++;

			/* Process packet if complete */
			if (DeviceExtension->PacketBufferPosition >= 3)
			{
				Queue = DeviceExtension->ActiveQueue % 2;

				/* Prevent buffer overflow */
				if (DeviceExtension->InputDataCount[Queue] == 1)
					continue;

				Input = &DeviceExtension->MouseInputData[Queue];

				if (DeviceExtension->PacketBufferPosition == 3)
				{
					/* Retrieve change in x and y from packet */
					Input->LastX = (signed char)(PacketBuffer[1] | ((PacketBuffer[0] & 0x03) << 6));
					Input->LastY = (signed char)(PacketBuffer[2] | ((PacketBuffer[0] & 0x0c) << 4));

					/* Determine the current state of the buttons */
					Input->RawButtons = (DeviceExtension->PreviousButtons & MOUSE_BUTTON_MIDDLE) |
						((UCHAR)(PacketBuffer[0] & LEFT_BUTTON_MASK) >> LEFT_BUTTON_SHIFT) |
						((UCHAR)(PacketBuffer[0] & RIGHT_BUTTON_MASK) >> RIGHT_BUTTON_SHIFT);
				}
				else if (DeviceExtension->PacketBufferPosition == 4)
				{
					DeviceExtension->PacketBufferPosition = 0;
					/* If middle button state changed than report event */
					if (((UCHAR)(PacketBuffer[3] & MIDDLE_BUTTON_MASK) >> MIDDLE_BUTTON_SHIFT) ^
						(DeviceExtension->PreviousButtons & MOUSE_BUTTON_MIDDLE))
					{
						Input->RawButtons ^= MOUSE_BUTTON_MIDDLE;
						Input->LastX = 0;
						Input->LastY = 0;
					}
					else
					{
						continue;
					}
				}

				/* Determine ButtonFlags */
				Input->ButtonFlags = 0;
				ButtonsDifference = DeviceExtension->PreviousButtons ^ Input->RawButtons;

				if (ButtonsDifference != 0)
				{
					if (ButtonsDifference & MOUSE_BUTTON_LEFT
						&& DeviceExtension->AttributesInformation.NumberOfButtons >= 1)
					{
						if (Input->RawButtons & MOUSE_BUTTON_LEFT)
							Input->ButtonFlags |= MOUSE_LEFT_BUTTON_DOWN;
						else
							Input->ButtonFlags |= MOUSE_LEFT_BUTTON_UP;
					}

					if (ButtonsDifference & MOUSE_BUTTON_RIGHT
						&& DeviceExtension->AttributesInformation.NumberOfButtons >= 2)
					{
						if (Input->RawButtons & MOUSE_BUTTON_RIGHT)
							Input->ButtonFlags |= MOUSE_RIGHT_BUTTON_DOWN;
						else
							Input->ButtonFlags |= MOUSE_RIGHT_BUTTON_UP;
					}

					if (ButtonsDifference & MOUSE_BUTTON_MIDDLE
						&& DeviceExtension->AttributesInformation.NumberOfButtons >= 3)
					{
						if (Input->RawButtons & MOUSE_BUTTON_MIDDLE)
							Input->ButtonFlags |= MOUSE_MIDDLE_BUTTON_DOWN;
						else
							Input->ButtonFlags |= MOUSE_MIDDLE_BUTTON_UP;
					}
				}

				/* Send the Input data to the Mouse Class driver */
				DeviceExtension->InputDataCount[Queue]++;

				KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
				InterlockedIncrement((PLONG)&DeviceExtension->ActiveQueue);
				(*(PSERVICE_CALLBACK_ROUTINE)DeviceExtension->ConnectData.ClassService)(
					DeviceExtension->ConnectData.ClassDeviceObject,
					&DeviceExtension->MouseInputData[Queue],
					&DeviceExtension->MouseInputData[Queue] + 1,
					&DeviceExtension->InputDataCount[Queue]);
				KeLowerIrql(OldIrql);
				DeviceExtension->InputDataCount[Queue] = 0;

				/* Copy RawButtons to Previous Buttons for Input */
				DeviceExtension->PreviousButtons = Input->RawButtons;
			}
		}
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}
