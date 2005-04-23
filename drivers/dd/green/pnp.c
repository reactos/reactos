/* $Id:
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS VT100 emulator
 * FILE:            drivers/dd/green/pnp.c
 * PURPOSE:         IRP_MJ_PNP operations
 * 
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.com)
 */

#define NDEBUG
#include "green.h"

NTSTATUS STDCALL
GreenAddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	PDEVICE_OBJECT Fdo = NULL;
	PGREEN_DEVICE_EXTENSION DeviceExtension;
	UNICODE_STRING serialPortName;
	NTSTATUS Status;
	
	DPRINT("Green: AddDevice(DriverObject %p, Pdo %p)\n", DriverObject, Pdo);
	
	/* Create green FDO */
	Status = IoCreateDevice(DriverObject,
		sizeof(GREEN_DEVICE_EXTENSION),
		NULL,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		TRUE,
		&Fdo);
	if (!NT_SUCCESS(Status))
		return Status;
	
	DeviceExtension = (PGREEN_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(GREEN_DEVICE_EXTENSION));
	DeviceExtension->Common.Type = Green;
	
	Status = KeyboardInitialize(DriverObject, &DeviceExtension->Keyboard);
	if (!NT_SUCCESS(Status))
	{
		IoDeleteDevice(Fdo);
		return Status;
	}
	((PKEYBOARD_DEVICE_EXTENSION)DeviceExtension->Keyboard->DeviceExtension)->Green = Fdo;
	
	Status = ScreenInitialize(DriverObject, &DeviceExtension->Screen);
	if (!NT_SUCCESS(Status))
	{
		IoDeleteDevice(DeviceExtension->Keyboard);
		IoDeleteDevice(Fdo);
		return Status;
	}
	((PSCREEN_DEVICE_EXTENSION)DeviceExtension->Screen->DeviceExtension)->Green = Fdo;
	
	/* initialize green Fdo */
	DeviceExtension->LowerDevice = IoAttachDeviceToDeviceStack(Fdo, Pdo);
	DeviceExtension->LineControl.WordLength = 8;
	DeviceExtension->LineControl.Parity = NO_PARITY;
	DeviceExtension->LineControl.StopBits = STOP_BIT_1;
	DeviceExtension->BaudRate = SERIAL_BAUD_38400;
	DeviceExtension->Timeouts.ReadTotalTimeoutConstant = 1; /* not null */
	DeviceExtension->Timeouts.ReadIntervalTimeout = INFINITE;
	DeviceExtension->Timeouts.ReadTotalTimeoutMultiplier = INFINITE;
	DeviceExtension->Timeouts.WriteTotalTimeoutMultiplier = 0; /* FIXME */
	DeviceExtension->Timeouts.WriteTotalTimeoutConstant = 0; /* FIXME */
	
	/* open associated serial port */
	RtlInitUnicodeString(&serialPortName, L"\\Device\\Serial1"); /* FIXME: don't hardcode string */
	Status = ObReferenceObjectByName(
		&serialPortName,
		OBJ_EXCLUSIVE | OBJ_KERNEL_HANDLE,
		NULL,
		(ACCESS_MASK)0,
		IoDeviceObjectType,
		KernelMode,
		NULL,
		(PVOID*)&DeviceExtension->Serial);
	/* FIXME: we never ObDereferenceObject */
	
	Fdo->Flags |= DO_POWER_PAGABLE | DO_BUFFERED_IO;
	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	
	return Status;
}
