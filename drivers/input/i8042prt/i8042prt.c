/*
 * PROJECT:     ReactOS i8042 (ps/2 keyboard-mouse controller) driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/i8042prt/i8042prt.c
 * PURPOSE:     Driver entry function
 * PROGRAMMERS: Copyright Victor Kirhenshtein (sauros@iname.com)
                Copyright Jason Filby (jasonfilby@yahoo.com)
                Copyright Martijn Vernooij (o112w8r02@sneakemail.com)
                Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "i8042prt.h"

#include <debug.h>

/* FUNCTIONS *****************************************************************/

static DRIVER_STARTIO i8042StartIo;
_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
static DRIVER_DISPATCH i8042DeviceControl;
_Dispatch_type_(IRP_MJ_INTERNAL_DEVICE_CONTROL)
static DRIVER_DISPATCH i8042InternalDeviceControl;
_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
static DRIVER_DISPATCH i8042SystemControl;
_Dispatch_type_(IRP_MJ_POWER)
static DRIVER_DISPATCH i8042Power;
DRIVER_INITIALIZE DriverEntry;

NTSTATUS NTAPI
i8042AddDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	PI8042_DRIVER_EXTENSION DriverExtension;
	PFDO_DEVICE_EXTENSION DeviceExtension = NULL;
	PDEVICE_OBJECT Fdo = NULL;
	ULONG DeviceExtensionSize;
	NTSTATUS Status;

	TRACE_(I8042PRT, "i8042AddDevice(%p %p)\n", DriverObject, Pdo);

	DriverExtension = (PI8042_DRIVER_EXTENSION)IoGetDriverObjectExtension(DriverObject, DriverObject);

	if (Pdo == NULL)
	{
		/* We're getting a NULL Pdo at the first call as
		 * we are a legacy driver. Ignore it */
		return STATUS_SUCCESS;
	}

	/* Create new device object. As we don't know if the device would be a keyboard
	 * or a mouse, we have to allocate the biggest device extension. */
	DeviceExtensionSize = MAX(sizeof(I8042_KEYBOARD_EXTENSION), sizeof(I8042_MOUSE_EXTENSION));
	Status = IoCreateDevice(
		DriverObject,
		DeviceExtensionSize,
		NULL,
		Pdo->DeviceType,
		FILE_DEVICE_SECURE_OPEN,
		TRUE,
		&Fdo);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "IoCreateDevice() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	DeviceExtension = (PFDO_DEVICE_EXTENSION)Fdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, DeviceExtensionSize);
	DeviceExtension->Type = Unknown;
	DeviceExtension->Fdo = Fdo;
	DeviceExtension->Pdo = Pdo;
	DeviceExtension->PortDeviceExtension = &DriverExtension->Port;
	Status = IoAttachDeviceToDeviceStackSafe(Fdo, Pdo, &DeviceExtension->LowerDevice);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "IoAttachDeviceToDeviceStackSafe() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	ExInterlockedInsertTailList(
		&DriverExtension->DeviceListHead,
		&DeviceExtension->ListEntry,
		&DriverExtension->DeviceListLock);

	Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
	return STATUS_SUCCESS;

cleanup:
	if (DeviceExtension && DeviceExtension->LowerDevice)
		IoDetachDevice(DeviceExtension->LowerDevice);
	if (Fdo)
		IoDeleteDevice(Fdo);
	return Status;
}

VOID NTAPI
i8042SendHookWorkItem(
	IN PDEVICE_OBJECT DeviceObject,
	IN PVOID Context)
{
	PI8042_HOOK_WORKITEM WorkItemData;
	PFDO_DEVICE_EXTENSION FdoDeviceExtension;
	PPORT_DEVICE_EXTENSION PortDeviceExtension;
	PDEVICE_OBJECT TopOfStack = NULL;
	ULONG IoControlCode;
	PVOID InputBuffer;
	ULONG InputBufferLength;
	IO_STATUS_BLOCK IoStatus;
	KEVENT Event;
	PIRP NewIrp;
	NTSTATUS Status;

	TRACE_(I8042PRT, "i8042SendHookWorkItem(%p %p)\n", DeviceObject, Context);

	WorkItemData = (PI8042_HOOK_WORKITEM)Context;
	FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	PortDeviceExtension = FdoDeviceExtension->PortDeviceExtension;

	switch (FdoDeviceExtension->Type)
	{
		case Keyboard:
		{
			PI8042_KEYBOARD_EXTENSION DeviceExtension;
			DeviceExtension = (PI8042_KEYBOARD_EXTENSION)FdoDeviceExtension;
			IoControlCode = IOCTL_INTERNAL_I8042_HOOK_KEYBOARD;
			InputBuffer = &DeviceExtension->KeyboardHook;
			InputBufferLength = sizeof(INTERNAL_I8042_HOOK_KEYBOARD);
			break;
		}
		case Mouse:
		{
			PI8042_MOUSE_EXTENSION DeviceExtension;
			DeviceExtension = (PI8042_MOUSE_EXTENSION)FdoDeviceExtension;
			IoControlCode = IOCTL_INTERNAL_I8042_HOOK_MOUSE;
			InputBuffer = &DeviceExtension->MouseHook;
			InputBufferLength = sizeof(INTERNAL_I8042_HOOK_MOUSE);
			break;
		}
		default:
		{
			ERR_(I8042PRT, "Unknown FDO type %u\n", FdoDeviceExtension->Type);
			ASSERT(FALSE);
			WorkItemData->Irp->IoStatus.Status = STATUS_INTERNAL_ERROR;
			goto cleanup;
		}
	}

	KeInitializeEvent(&Event, NotificationEvent, FALSE);
	TopOfStack = IoGetAttachedDeviceReference(DeviceObject);

	NewIrp = IoBuildDeviceIoControlRequest(
		IoControlCode,
		TopOfStack,
		InputBuffer,
		InputBufferLength,
		NULL,
		0,
		TRUE,
		&Event,
		&IoStatus);

	if (!NewIrp)
	{
		WARN_(I8042PRT, "IoBuildDeviceIoControlRequest() failed\n");
		WorkItemData->Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanup;
	}

	Status = IoCallDriver(TopOfStack, NewIrp);
	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(
			&Event,
			Executive,
			KernelMode,
			FALSE,
			NULL);
		Status = IoStatus.Status;
	}
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "IoCallDriver() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	if (FdoDeviceExtension->Type == Keyboard)
	{
		PI8042_KEYBOARD_EXTENSION DeviceExtension;

		DeviceExtension = (PI8042_KEYBOARD_EXTENSION)FdoDeviceExtension;
		/* Call the hooked initialization if it exists */
		if (DeviceExtension->KeyboardHook.InitializationRoutine)
		{
			Status = DeviceExtension->KeyboardHook.InitializationRoutine(
				DeviceExtension->KeyboardHook.Context,
				PortDeviceExtension,
				i8042SynchReadPort,
				i8042SynchWritePortKbd,
				FALSE);
			if (!NT_SUCCESS(Status))
			{
				WARN_(I8042PRT, "KeyboardHook.InitializationRoutine() failed with status 0x%08lx\n", Status);
				WorkItemData->Irp->IoStatus.Status = Status;
				goto cleanup;
			}
		}
	}

	WorkItemData->Irp->IoStatus.Status = STATUS_SUCCESS;

cleanup:
	if (TopOfStack != NULL)
		ObDereferenceObject(TopOfStack);
	WorkItemData->Irp->IoStatus.Information = 0;
	IoCompleteRequest(WorkItemData->Irp, IO_NO_INCREMENT);

	IoFreeWorkItem(WorkItemData->WorkItem);
	ExFreePoolWithTag(WorkItemData, I8042PRT_TAG);
}

static VOID NTAPI
i8042StartIo(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PFDO_DEVICE_EXTENSION DeviceExtension;

	DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
	switch (DeviceExtension->Type)
	{
		case Keyboard:
			i8042KbdStartIo(DeviceObject, Irp);
			break;
		default:
			ERR_(I8042PRT, "Unknown FDO type %u\n", DeviceExtension->Type);
			ASSERT(FALSE);
			break;
	}
}

/* Write the current byte of the packet. Returns FALSE in case
 * of problems.
 */
static BOOLEAN
i8042PacketWrite(
	IN PPORT_DEVICE_EXTENSION DeviceExtension)
{
	UCHAR Port = DeviceExtension->PacketPort;

	if (Port)
	{
		if (!i8042Write(DeviceExtension,
		                DeviceExtension->ControlPort,
		                Port))
		{
			/* something is really wrong! */
			WARN_(I8042PRT, "Failed to send packet byte!\n");
			return FALSE;
		}
	}

	return i8042Write(DeviceExtension,
	                  DeviceExtension->DataPort,
	                  DeviceExtension->Packet.Bytes[DeviceExtension->Packet.CurrentByte]);
}

BOOLEAN
i8042PacketIsr(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN UCHAR Output)
{
	if (DeviceExtension->Packet.State == Idle)
		return FALSE;

	switch (Output)
	{
		case KBD_RESEND:
			DeviceExtension->PacketResends++;
			if (DeviceExtension->PacketResends > DeviceExtension->Settings.ResendIterations)
			{
				DeviceExtension->Packet.State = Idle;
				DeviceExtension->PacketComplete = TRUE;
				DeviceExtension->PacketResult = STATUS_IO_TIMEOUT;
				DeviceExtension->PacketResends = 0;
				return TRUE;
			}
			DeviceExtension->Packet.CurrentByte--;
			break;

		case KBD_NACK:
			DeviceExtension->Packet.State = Idle;
			DeviceExtension->PacketComplete = TRUE;
			DeviceExtension->PacketResult = STATUS_UNEXPECTED_IO_ERROR;
			DeviceExtension->PacketResends = 0;
			return TRUE;

		default:
			DeviceExtension->PacketResends = 0;
	}

	if (DeviceExtension->Packet.CurrentByte >= DeviceExtension->Packet.ByteCount)
	{
		DeviceExtension->Packet.State = Idle;
		DeviceExtension->PacketComplete = TRUE;
		DeviceExtension->PacketResult = STATUS_SUCCESS;
		return TRUE;
	}

	if (!i8042PacketWrite(DeviceExtension))
	{
		DeviceExtension->Packet.State = Idle;
		DeviceExtension->PacketComplete = TRUE;
		DeviceExtension->PacketResult = STATUS_IO_TIMEOUT;
		return TRUE;
	}
	DeviceExtension->Packet.CurrentByte++;

	return TRUE;
}

/*
 * This function starts a packet. It must be called with the
 * correct DIRQL.
 */
NTSTATUS
i8042StartPacket(
	IN PPORT_DEVICE_EXTENSION DeviceExtension,
	IN PFDO_DEVICE_EXTENSION FdoDeviceExtension,
	IN PUCHAR Bytes,
	IN ULONG ByteCount,
	IN PIRP Irp)
{
	KIRQL Irql;
	NTSTATUS Status;

	Irql = KeAcquireInterruptSpinLock(DeviceExtension->HighestDIRQLInterrupt);

	if (DeviceExtension->Packet.State != Idle)
	{
		Status = STATUS_DEVICE_BUSY;
		goto done;
	}

	switch (FdoDeviceExtension->Type)
	{
		case Keyboard: DeviceExtension->PacketPort = 0; break;
		case Mouse: DeviceExtension->PacketPort = CTRL_WRITE_MOUSE; break;
		default:
			ERR_(I8042PRT, "Unknown FDO type %u\n", FdoDeviceExtension->Type);
			ASSERT(FALSE);
			Status = STATUS_INTERNAL_ERROR;
			goto done;
	}

	DeviceExtension->Packet.Bytes = Bytes;
	DeviceExtension->Packet.CurrentByte = 0;
	DeviceExtension->Packet.ByteCount = ByteCount;
	DeviceExtension->Packet.State = SendingBytes;
	DeviceExtension->PacketResult = Status = STATUS_PENDING;
	DeviceExtension->CurrentIrp = Irp;
	DeviceExtension->CurrentIrpDevice = FdoDeviceExtension->Fdo;

	if (!i8042PacketWrite(DeviceExtension))
	{
		Status = STATUS_IO_TIMEOUT;
		DeviceExtension->Packet.State = Idle;
		DeviceExtension->PacketResult = STATUS_ABANDONED;
		goto done;
	}

	DeviceExtension->Packet.CurrentByte++;

done:
	KeReleaseInterruptSpinLock(DeviceExtension->HighestDIRQLInterrupt, Irql);

	if (Status != STATUS_PENDING)
	{
		DeviceExtension->CurrentIrp = NULL;
		DeviceExtension->CurrentIrpDevice = NULL;
		Irp->IoStatus.Status = Status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
	}
	return Status;
}

static NTSTATUS NTAPI
i8042DeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PFDO_DEVICE_EXTENSION DeviceExtension;

	TRACE_(I8042PRT, "i8042DeviceControl(%p %p)\n", DeviceObject, Irp);
	DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	switch (DeviceExtension->Type)
	{
		case Keyboard:
			return i8042KbdDeviceControl(DeviceObject, Irp);
		default:
			Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			return STATUS_INVALID_DEVICE_REQUEST;
	}
}

static NTSTATUS NTAPI
i8042InternalDeviceControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PFDO_DEVICE_EXTENSION DeviceExtension;
	ULONG ControlCode;
	NTSTATUS Status;

	TRACE_(I8042PRT, "i8042InternalDeviceControl(%p %p)\n", DeviceObject, Irp);
	DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

	switch (DeviceExtension->Type)
	{
		case Unknown:
		{
			ControlCode = IoGetCurrentIrpStackLocation(Irp)->Parameters.DeviceIoControl.IoControlCode;
			switch (ControlCode)
			{
				case IOCTL_INTERNAL_KEYBOARD_CONNECT:
					Status = i8042KbdInternalDeviceControl(DeviceObject, Irp);
					break;
				case IOCTL_INTERNAL_MOUSE_CONNECT:
					Status = i8042MouInternalDeviceControl(DeviceObject, Irp);
					break;
				default:
					ERR_(I8042PRT, "Unknown IO control code 0x%lx\n", ControlCode);
					ASSERT(FALSE);
					Status = STATUS_INVALID_DEVICE_REQUEST;
					break;
			}
			break;
		}
		case Keyboard:
			Status = i8042KbdInternalDeviceControl(DeviceObject, Irp);
			break;
		case Mouse:
			Status = i8042MouInternalDeviceControl(DeviceObject, Irp);
			break;
		default:
			ERR_(I8042PRT, "Unknown FDO type %u\n", DeviceExtension->Type);
			ASSERT(FALSE);
			Status = STATUS_INTERNAL_ERROR;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
	}

	return Status;
}

static NTSTATUS NTAPI
i8042Power(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	PFDO_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
	PDEVICE_OBJECT LowerDevice = DeviceExtension->LowerDevice;

	PoStartNextPowerIrp(Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver(LowerDevice, Irp);
}

static NTSTATUS NTAPI
i8042SystemControl(
	IN PDEVICE_OBJECT DeviceObject,
	IN PIRP Irp)
{
	return ForwardIrpAndForget(DeviceObject, Irp);
}

NTSTATUS NTAPI
DriverEntry(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	PI8042_DRIVER_EXTENSION DriverExtension;
	NTSTATUS Status;

	Status = IoAllocateDriverObjectExtension(
		DriverObject,
		DriverObject,
		sizeof(I8042_DRIVER_EXTENSION),
		(PVOID*)&DriverExtension);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "IoAllocateDriverObjectExtension() failed with status 0x%08lx\n", Status);
		return Status;
	}
	RtlZeroMemory(DriverExtension, sizeof(I8042_DRIVER_EXTENSION));
	KeInitializeSpinLock(&DriverExtension->Port.SpinLock);
	InitializeListHead(&DriverExtension->DeviceListHead);
	KeInitializeSpinLock(&DriverExtension->DeviceListLock);

	Status = DuplicateUnicodeString(
		RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
		RegistryPath,
		&DriverExtension->RegistryPath);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "DuplicateUnicodeString() failed with status 0x%08lx\n", Status);
		return Status;
	}

	Status = ReadRegistryEntries(&DriverExtension->RegistryPath, &DriverExtension->Port.Settings);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "ReadRegistryEntries() failed with status 0x%08lx\n", Status);
		return Status;
	}

	DriverObject->DriverExtension->AddDevice = i8042AddDevice;
	DriverObject->DriverStartIo = i8042StartIo;

	DriverObject->MajorFunction[IRP_MJ_CREATE]  = i8042Create;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = i8042Cleanup;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]   = i8042Close;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = i8042DeviceControl;
	DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = i8042InternalDeviceControl;
	DriverObject->MajorFunction[IRP_MJ_POWER]   = i8042Power;
	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = i8042SystemControl;
	DriverObject->MajorFunction[IRP_MJ_PNP]     = i8042Pnp;

    i8042InitializeHwHacks();

	return STATUS_SUCCESS;
}
