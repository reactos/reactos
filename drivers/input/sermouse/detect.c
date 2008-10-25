/*
 * PROJECT:     ReactOS Serial mouse driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/sermouse/detect.c
 * PURPOSE:     Detect serial mouse type
 * PROGRAMMERS: Copyright Jason Filby (jasonfilby@yahoo.com)
                Copyright Filip Navara (xnavara@volny.cz)
                Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "sermouse.h"

/* Most of this file is ripped from reactos/drivers/bus/serenum/detect.c */

static NTSTATUS
DeviceIoControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN ULONG CtlCode,
	IN PVOID InputBuffer OPTIONAL,
	IN SIZE_T InputBufferSize,
	IN OUT PVOID OutputBuffer OPTIONAL,
	IN OUT PSIZE_T OutputBufferSize)
{
	KEVENT Event;
	PIRP Irp;
	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status;

	KeInitializeEvent (&Event, NotificationEvent, FALSE);

	Irp = IoBuildDeviceIoControlRequest(CtlCode,
		DeviceObject,
		InputBuffer,
		(ULONG)InputBufferSize,
		OutputBuffer,
		(OutputBufferSize) ? (ULONG)*OutputBufferSize : 0,
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
		*OutputBufferSize = (SIZE_T)IoStatus.Information;
	}

	return Status;
}

static NTSTATUS
ReadBytes(
	IN PDEVICE_OBJECT LowerDevice,
	OUT PUCHAR Buffer,
	IN ULONG BufferSize,
	OUT PULONG_PTR FilledBytes)
{
	PIRP Irp;
	IO_STATUS_BLOCK ioStatus;
	KEVENT event;
	LARGE_INTEGER zero;
	NTSTATUS Status;

	KeInitializeEvent(&event, NotificationEvent, FALSE);
	zero.QuadPart = 0;
	Irp = IoBuildSynchronousFsdRequest(
		IRP_MJ_READ,
		LowerDevice,
		Buffer, BufferSize,
		&zero,
		&event,
		&ioStatus);
	if (!Irp)
		return FALSE;

	Status = IoCallDriver(LowerDevice, Irp);
	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(&event, Suspended, KernelMode, FALSE, NULL);
		Status = ioStatus.Status;
	}
	INFO_(SERMOUSE, "Bytes received: %lu/%lu\n",
		ioStatus.Information, BufferSize);
	*FilledBytes = ioStatus.Information;
	return Status;
}

static NTSTATUS
Wait(
	IN ULONG milliseconds)
{
	KTIMER Timer;
	LARGE_INTEGER DueTime;

	DueTime.QuadPart = milliseconds * -10;
	KeInitializeTimer(&Timer);
	KeSetTimer(&Timer, DueTime, NULL);
	return KeWaitForSingleObject(&Timer, Executive, KernelMode, FALSE, NULL);
}

SERMOUSE_MOUSE_TYPE
SermouseDetectLegacyDevice(
	IN PDEVICE_OBJECT LowerDevice)
{
	HANDLE Handle;
	ULONG Fcr, Mcr;
	ULONG BaudRate;
	ULONG Command;
	SERIAL_TIMEOUTS Timeouts;
	SERIAL_LINE_CONTROL LCR;
	ULONG_PTR i, Count = 0;
	UCHAR Buffer[16];
	SERMOUSE_MOUSE_TYPE MouseType = mtNone;
	NTSTATUS Status;

	TRACE_(SERMOUSE, "SermouseDetectLegacyDevice(LowerDevice %p)\n", LowerDevice);

	RtlZeroMemory(Buffer, sizeof(Buffer));

	/* Open port */
	Status = ObOpenObjectByPointer(
		LowerDevice,
		OBJ_EXCLUSIVE | OBJ_KERNEL_HANDLE,
		NULL,
		0,
		NULL,
		KernelMode,
		&Handle);
	if (!NT_SUCCESS(Status)) return mtNone;

	/* Reset UART */
	TRACE_(SERMOUSE, "Reset UART\n");
	Mcr = 0; /* MCR: DTR/RTS/OUT2 off */
	Status = DeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_MODEM_CONTROL,
		&Mcr, sizeof(Mcr), NULL, NULL);
	if (!NT_SUCCESS(Status)) goto ByeBye;

	/* Set communications parameters */
	TRACE_(SERMOUSE, "Set communications parameters\n");
	/* DLAB off */
	Fcr = 0;
	Status = DeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_FIFO_CONTROL,
		&Fcr, sizeof(Fcr), NULL, NULL);
	if (!NT_SUCCESS(Status)) goto ByeBye;
	/* Set serial port speed */
	BaudRate = 1200;
	Status = DeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_BAUD_RATE,
		&BaudRate, sizeof(BaudRate), NULL, NULL);
	if (!NT_SUCCESS(Status)) goto ByeBye;
	/* Set LCR */
	LCR.WordLength = 7;
	LCR.Parity = NO_PARITY;
	LCR.StopBits = STOP_BITS_2;
	Status = DeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_LINE_CONTROL,
		&LCR, sizeof(LCR), NULL, NULL);
	if (!NT_SUCCESS(Status)) goto ByeBye;

	/* Flush receive buffer */
	TRACE_(SERMOUSE, "Flush receive buffer\n");
	Command = SERIAL_PURGE_RXCLEAR;
	Status = DeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_MODEM_CONTROL,
		&Command, sizeof(Command), NULL, NULL);
	if (!NT_SUCCESS(Status)) goto ByeBye;
	/* Wait 100 ms */
	Wait(100);

	/* Enable DTR/RTS */
	TRACE_(SERMOUSE, "Enable DTR/RTS\n");
	Status = DeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_DTR,
		NULL, 0, NULL, NULL);
	if (!NT_SUCCESS(Status)) goto ByeBye;
	Status = DeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_RTS,
		NULL, 0, NULL, NULL);
	if (!NT_SUCCESS(Status)) goto ByeBye;

	/* Set timeout to 500 microseconds */
	TRACE_(SERMOUSE, "Set timeout to 500 microseconds\n");
	Timeouts.ReadIntervalTimeout = 100;
	Timeouts.ReadTotalTimeoutMultiplier = 0;
	Timeouts.ReadTotalTimeoutConstant = 500;
	Timeouts.WriteTotalTimeoutMultiplier = Timeouts.WriteTotalTimeoutConstant = 0;
	Status = DeviceIoControl(LowerDevice, IOCTL_SERIAL_SET_TIMEOUTS,
		&Timeouts, sizeof(Timeouts), NULL, NULL);
	if (!NT_SUCCESS(Status)) goto ByeBye;

	/* Fill the read buffer */
	TRACE_(SERMOUSE, "Fill the read buffer\n");
	Status = ReadBytes(LowerDevice, Buffer, sizeof(Buffer)/sizeof(Buffer[0]), &Count);
	if (!NT_SUCCESS(Status)) goto ByeBye;

	for (i = 0; i < Count; i++)
	{
		if (Buffer[i] == 'B')
		{
			/* Sign for Microsoft Ballpoint */
			ERR_(SERMOUSE, "Microsoft Ballpoint device detected. THIS DEVICE IS NOT YET SUPPORTED");
			MouseType = mtNone;
			goto ByeBye;
		}
		else if (Buffer[i] == 'M')
		{
			/* Sign for Microsoft Mouse protocol followed by button specifier */
			if (i == sizeof(Buffer) - 1)
			{
				/* Overflow Error */
				goto ByeBye;
			}
			switch (Buffer[i + 1])
			{
				case '3':
					INFO_(SERMOUSE, "Microsoft Mouse with 3-buttons detected\n");
					MouseType = mtLogitech;
				case 'Z':
					INFO_(SERMOUSE, "Microsoft Wheel Mouse detected\n");
					MouseType = mtWheelZ;
				default:
					INFO_(SERMOUSE, "Microsoft Mouse with 2-buttons detected\n");
					MouseType = mtMicrosoft;
			}
			goto ByeBye;
		}
	}

ByeBye:
	/* Close port */
	if (Handle)
		ZwClose(Handle);
	return MouseType;
}
