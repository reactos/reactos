/*
 * PROJECT:     ReactOS VT100 emulator
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/dd/green/keyboard.c
 * PURPOSE:     Keyboard part of green management
 * PROGRAMMERS: Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 */

#include "green.h"

#define NDEBUG
#include <debug.h>

static BOOLEAN
TranslateCharToScanCodes(
	IN PUCHAR InputBuffer,
	IN ULONG InputBufferSize,
	OUT KEYBOARD_INPUT_DATA* OutputBuffer,
	OUT PULONG OutputBufferSize,
	OUT PULONG BytesConsumed)
{
	BOOLEAN NormalKey = FALSE;
	USHORT MakeCode;

	if (InputBufferSize == 0)
		return FALSE;

	switch (*InputBuffer)
	{
		case 0x1b: MakeCode = 0x01; NormalKey = TRUE; break; /* ESC */

		case '1': MakeCode = 0x02; NormalKey = TRUE; break;
		case '2': MakeCode = 0x03; NormalKey = TRUE; break;
		case '3': MakeCode = 0x04; NormalKey = TRUE; break;
		case '4': MakeCode = 0x05; NormalKey = TRUE; break;
		case '5': MakeCode = 0x06; NormalKey = TRUE; break;
		case '6': MakeCode = 0x07; NormalKey = TRUE; break;
		case '7': MakeCode = 0x08; NormalKey = TRUE; break;
		case '8': MakeCode = 0x09; NormalKey = TRUE; break;
		case '9': MakeCode = 0x0a; NormalKey = TRUE; break;
		case '0': MakeCode = 0x0b; NormalKey = TRUE; break;
		case '-': MakeCode = 0x0c; NormalKey = TRUE; break;
		case '=': MakeCode = 0x0d; NormalKey = TRUE; break;
		case '\b': MakeCode = 0x0e; NormalKey = TRUE; break;

		case '\t': MakeCode = 0x0f; NormalKey = TRUE; break;
		case 'q': MakeCode = 0x10; NormalKey = TRUE; break;
		case 'w': MakeCode = 0x11; NormalKey = TRUE; break;
		case 'e': MakeCode = 0x12; NormalKey = TRUE; break;
		case 'r': MakeCode = 0x13; NormalKey = TRUE; break;
		case 't': MakeCode = 0x14; NormalKey = TRUE; break;
		case 'y': MakeCode = 0x15; NormalKey = TRUE; break;
		case 'u': MakeCode = 0x16; NormalKey = TRUE; break;
		case 'i': MakeCode = 0x17; NormalKey = TRUE; break;
		case 'o': MakeCode = 0x18; NormalKey = TRUE; break;
		case 'p': MakeCode = 0x19; NormalKey = TRUE; break;
		case '[': MakeCode = 0x1a; NormalKey = TRUE; break;
		case ']': MakeCode = 0x1b; NormalKey = TRUE; break;

		case '\r': MakeCode = 0x1c; NormalKey = TRUE; break;

		case 'a': MakeCode = 0x1e; NormalKey = TRUE; break;
		case 's': MakeCode = 0x1f; NormalKey = TRUE; break;
		case 'd': MakeCode = 0x20; NormalKey = TRUE; break;
		case 'f': MakeCode = 0x21; NormalKey = TRUE; break;
		case 'g': MakeCode = 0x22; NormalKey = TRUE; break;
		case 'h': MakeCode = 0x23; NormalKey = TRUE; break;
		case 'j': MakeCode = 0x24; NormalKey = TRUE; break;
		case 'k': MakeCode = 0x25; NormalKey = TRUE; break;
		case 'l': MakeCode = 0x26; NormalKey = TRUE; break;
		case ';': MakeCode = 0x27; NormalKey = TRUE; break;
		case '\'': MakeCode = 0x28; NormalKey = TRUE; break;

		case '`': MakeCode = 0x29; NormalKey = TRUE; break;

		case '\\': MakeCode = 0x2b; NormalKey = TRUE; break;

		case 'z': MakeCode = 0x2c; NormalKey = TRUE; break;
		case 'x': MakeCode = 0x2d; NormalKey = TRUE; break;
		case 'c': MakeCode = 0x2e; NormalKey = TRUE; break;
		case 'v': MakeCode = 0x2f; NormalKey = TRUE; break;
		case 'b': MakeCode = 0x30; NormalKey = TRUE; break;
		case 'n': MakeCode = 0x31; NormalKey = TRUE; break;
		case 'm': MakeCode = 0x32; NormalKey = TRUE; break;
		case ',': MakeCode = 0x33; NormalKey = TRUE; break;
		case '.': MakeCode = 0x34; NormalKey = TRUE; break;
		case '/': MakeCode = 0x35; NormalKey = TRUE; break;

		case ' ': MakeCode = 0x39; NormalKey = TRUE; break;
	}
	if (NormalKey && *OutputBufferSize >= 2)
	{
		OutputBuffer[0].MakeCode = MakeCode;
		OutputBuffer[0].Flags = KEY_MAKE;
		OutputBuffer[1].MakeCode = MakeCode;
		OutputBuffer[1].Flags = KEY_BREAK;
		*BytesConsumed = 2;
		return TRUE;
	}

	/* Consume strange character by ignoring it */
	DPRINT1("Strange byte received 0x%02x ('%c')\n",
		*InputBuffer, *InputBuffer >= 32 ? *InputBuffer : '.');
	*BytesConsumed = 1;
	return TRUE;
}

NTSTATUS
KeyboardAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	PDEVICE_OBJECT Fdo;
	PKEYBOARD_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;

	DPRINT("KeyboardInitialize() called\n");

	Status = IoCreateDevice(DriverObject,
		sizeof(KEYBOARD_DEVICE_EXTENSION),
		NULL,
		FILE_DEVICE_KEYBOARD,
		FILE_DEVICE_SECURE_OPEN,
		TRUE,
		&Fdo);
	if (!NT_SUCCESS(Status))
		return Status;

	DeviceExtension = (PKEYBOARD_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(KEYBOARD_DEVICE_EXTENSION));
	DeviceExtension->Common.Type = KeyboardFDO;
	DeviceExtension->Common.LowerDevice = IoAttachDeviceToDeviceStack(Fdo, Pdo);
	DeviceExtension->Green = ((PGREEN_DRIVER_EXTENSION)IoGetDriverObjectExtension(DriverObject, DriverObject))->GreenMainDO;
	((PGREEN_DEVICE_EXTENSION)DeviceExtension->Green->DeviceExtension)->KeyboardFdo = Fdo;
	Fdo->Flags |= DO_POWER_PAGABLE | DO_BUFFERED_IO;
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

static VOID NTAPI
KeyboardDpcSendData(
	IN PKDPC Dpc,
	IN PVOID pDeviceExtension, /* real type PKEYBOARD_DEVICE_EXTENSION */
	IN PVOID Unused1,
	IN PVOID Unused2)
{
	PKEYBOARD_DEVICE_EXTENSION DeviceExtension;
	ULONG Queue;
	ULONG InputDataConsumed;

	DeviceExtension = (PKEYBOARD_DEVICE_EXTENSION)pDeviceExtension;

	Queue = DeviceExtension->ActiveQueue % 2;
	InterlockedIncrement((PLONG)&DeviceExtension->ActiveQueue);
	(*(PSERVICE_CALLBACK_ROUTINE)DeviceExtension->ClassInformation.ClassService)(
			DeviceExtension->ClassInformation.ClassDeviceObject,
			DeviceExtension->KeyboardInputData[Queue],
			DeviceExtension->KeyboardInputData[Queue] + DeviceExtension->InputDataCount[Queue],
			&InputDataConsumed);

	DeviceExtension->InputDataCount[Queue] = 0;
}

static VOID NTAPI
KeyboardDeviceWorker(
	PVOID Context)
{
	PDEVICE_OBJECT DeviceObject;
	PKEYBOARD_DEVICE_EXTENSION DeviceExtension;
	PGREEN_DEVICE_EXTENSION GreenDeviceExtension;
	PDEVICE_OBJECT LowerDevice;
	UCHAR Buffer[16]; /* Arbitrary size */
	ULONG BufferSize;
	LARGE_INTEGER Zero;
	PIRP Irp;
	IO_STATUS_BLOCK ioStatus;
	KEVENT event;
	KIRQL OldIrql;
	ULONG i, Queue;
	ULONG SpaceInQueue;
	ULONG BytesConsumed = 0;
	PKEYBOARD_INPUT_DATA Input;
	NTSTATUS Status;

	DPRINT("KeyboardDeviceWorker() called\n");

	DeviceObject = (PDEVICE_OBJECT)Context;
	DeviceExtension = (PKEYBOARD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	GreenDeviceExtension = (PGREEN_DEVICE_EXTENSION)DeviceExtension->Green->DeviceExtension;
	LowerDevice = GreenDeviceExtension->Serial;
	BufferSize = sizeof(Buffer);
	Zero.QuadPart = 0;

	/* Initialize device extension */
	DeviceExtension->ActiveQueue = 0;
	DeviceExtension->InputDataCount[0] = 0;
	DeviceExtension->InputDataCount[1] = 0;
	KeInitializeDpc(&DeviceExtension->KeyboardDpc, KeyboardDpcSendData, DeviceExtension);
	RtlZeroMemory(&DeviceExtension->KeyboardInputData, sizeof(DeviceExtension->KeyboardInputData));

	/* main read loop */
	while (TRUE)
	{
		KeInitializeEvent(&event, NotificationEvent, FALSE);
		Irp = IoBuildSynchronousFsdRequest(
			IRP_MJ_READ,
			LowerDevice,
			Buffer, BufferSize,
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
		i = 0;
		while (i < ioStatus.Information)
		{
			Queue = DeviceExtension->ActiveQueue % 2;

			Input = &DeviceExtension->KeyboardInputData[Queue][DeviceExtension->InputDataCount[Queue]];

			/* Translate current chars to scan codes */
			SpaceInQueue = KEYBOARD_BUFFER_SIZE - DeviceExtension->InputDataCount[Queue];
			if (TranslateCharToScanCodes(
				&Buffer[i],               /* input buffer */
				ioStatus.Information - i, /* input buffer size */
				Input,                    /* output buffer */
				&SpaceInQueue,            /* output buffer size */
				&BytesConsumed))          /* bytes consumed in input buffer */
			{
				DPRINT("Got char 0x%02x (%c)\n", Buffer[i], Buffer[i] >= 32 ? Buffer[i] : ' ');
				DeviceExtension->InputDataCount[Queue] += BytesConsumed;

				/* Send the data to the keyboard class driver */
				KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
				KeInsertQueueDpc(&DeviceExtension->KeyboardDpc, NULL, NULL);
				KeLowerIrql(OldIrql);
				i += BytesConsumed;
			}
			else
			{
				/* TranslateCharToScanCodes failed. Possible reasons:
				 * - not enough bytes in input buffer (escape control code; wait next received bytes)
				 * - not enough room in output buffer (wait for the Dpc to empty it)
				 *
				 * The best way to resolve this is to try later.
				 */
				i++;
			}
		}
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS
KeyboardInternalDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PIO_STACK_LOCATION Stack;
	PKEYBOARD_DEVICE_EXTENSION DeviceExtension;
	NTSTATUS Status;

	Stack = IoGetCurrentIrpStackLocation(Irp);
	Irp->IoStatus.Information = 0;
	DeviceExtension = (PKEYBOARD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	switch (Stack->Parameters.DeviceIoControl.IoControlCode)
	{
		case IOCTL_INTERNAL_KEYBOARD_CONNECT:
		{
			DPRINT("IRP_MJ_INTERNAL_DEVICE_CONTROL / IOCTL_INTERNAL_KEYBOARD_CONNECT\n");
			if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(CONNECT_DATA))
			{
				Status = STATUS_INVALID_PARAMETER;
				break;
			}

			DeviceExtension->ClassInformation =
				*((PCONNECT_DATA)Stack->Parameters.DeviceIoControl.Type3InputBuffer);

			/* Start read loop */
			Status = PsCreateSystemThread(
				&DeviceExtension->WorkerThreadHandle,
				(ACCESS_MASK)0L,
				NULL,
				NULL,
				NULL,
				KeyboardDeviceWorker,
				DeviceObject);
			break;
		}
		default:
		{
			DPRINT("IRP_MJ_INTERNAL_DEVICE_CONTROL / unknown ioctl code 0x%lx\n",
				Stack->Parameters.DeviceIoControl.IoControlCode);
			Status = STATUS_INVALID_DEVICE_REQUEST;
		}
	}

	Irp->IoStatus.Status = Status;
	IoCompleteRequest (Irp, IO_NO_INCREMENT);
	return Status;
}
