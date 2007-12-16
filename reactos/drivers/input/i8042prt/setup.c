/*
 * PROJECT:     ReactOS i8042 (ps/2 keyboard-mouse controller) driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/i8042prt/setup.c
 * PURPOSE:     Create a legacy PDO during ReactOS installation
 * PROGRAMMERS: Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 */

/* NOTE:
 * All this file is a big hack and should be removed one day...
 */

/* INCLUDES ******************************************************************/

#include "i8042prt.h"

/* GLOBALS *******************************************************************/

#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_CONTROL_PORT 0x64
#define KEYBOARD_IRQ          1

/* FUNCTIONS *****************************************************************/

BOOLEAN
IsFirstStageSetup(
	VOID)
{
	UNICODE_STRING PathU = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SYSTEM\\Setup");
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE hSetupKey = (HANDLE)NULL;
	NTSTATUS Status;
	BOOLEAN ret = TRUE;

	InitializeObjectAttributes(&ObjectAttributes, &PathU, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwOpenKey(&hSetupKey, KEY_QUERY_VALUE, &ObjectAttributes);

	if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
		ret = TRUE;
	else
		ret = FALSE;

	if (hSetupKey != (HANDLE)NULL)
		ZwClose(hSetupKey);
	INFO_(I8042PRT, "IsFirstStageSetup() returns %s\n", ret ? "YES" : "NO");
	return ret;
}

static VOID NTAPI
SendStartDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PVOID Context,
	IN ULONG Count)
{
	PDEVICE_OBJECT Pdo;
	PCM_RESOURCE_LIST AllocatedResources = NULL;
	PCM_RESOURCE_LIST AllocatedResourcesTranslated = NULL;
	PDEVICE_OBJECT TopDeviceObject = NULL;
	KEVENT Event;
	IO_STATUS_BLOCK IoStatusBlock;
	PIRP Irp;
	PIO_STACK_LOCATION Stack;
	ULONG ResourceListSize;
	NTSTATUS Status;

	Pdo = (PDEVICE_OBJECT)Context;
	TRACE_(I8042PRT, "SendStartDevice(%p)\n", Pdo);

	/* Create default resource list */
	ResourceListSize = sizeof(CM_RESOURCE_LIST) + 3 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
	AllocatedResources = ExAllocatePoolWithTag(PagedPool, ResourceListSize, I8042PRT_TAG);
	if (!AllocatedResources)
	{
		WARN_(I8042PRT, "ExAllocatePoolWithTag() failed\n");
		Status = STATUS_NO_MEMORY;
		goto cleanup;
	}
	AllocatedResources->Count = 1;
	AllocatedResources->List[0].PartialResourceList.Version = 1;
	AllocatedResources->List[0].PartialResourceList.Revision = 1;
	AllocatedResources->List[0].PartialResourceList.Count = 3;
	/* Data port */
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[0].Type = CmResourceTypePort;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[0].ShareDisposition = CmResourceShareDeviceExclusive;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[0].Flags = 0; /* FIXME */
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[0].u.Port.Start.u.HighPart = 0;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[0].u.Port.Start.u.LowPart = KEYBOARD_DATA_PORT;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[0].u.Port.Length = 1;
	/* Control port */
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[1].Type = CmResourceTypePort;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[1].ShareDisposition = CmResourceShareDeviceExclusive;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[1].Flags = 0; /* FIXME */
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[1].u.Port.Start.u.HighPart = 0;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[1].u.Port.Start.u.LowPart = KEYBOARD_CONTROL_PORT;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[1].u.Port.Length = 1;
	/* Interrupt */
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[2].Type = CmResourceTypeInterrupt;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[2].ShareDisposition = CmResourceShareDeviceExclusive;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[2].Flags = CM_RESOURCE_INTERRUPT_LATCHED;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[2].u.Interrupt.Level = KEYBOARD_IRQ;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[2].u.Interrupt.Vector = 0;
	AllocatedResources->List[0].PartialResourceList.PartialDescriptors[2].u.Interrupt.Affinity = (KAFFINITY)-1;

	/* Create default resource list translated */
	AllocatedResourcesTranslated = ExAllocatePoolWithTag(PagedPool, ResourceListSize, I8042PRT_TAG);
	if (!AllocatedResourcesTranslated)
	{
		WARN_(I8042PRT, "ExAllocatePoolWithTag() failed\n");
		Status = STATUS_NO_MEMORY;
		goto cleanup;
	}
	RtlCopyMemory(AllocatedResourcesTranslated, AllocatedResources, ResourceListSize);
	AllocatedResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[2].u.Interrupt.Vector = HalGetInterruptVector(
			Internal, 0,
			AllocatedResources->List[0].PartialResourceList.PartialDescriptors[2].u.Interrupt.Level,
			AllocatedResources->List[0].PartialResourceList.PartialDescriptors[2].u.Interrupt.Vector,
			(PKIRQL)&AllocatedResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[2].u.Interrupt.Level,
			&AllocatedResourcesTranslated->List[0].PartialResourceList.PartialDescriptors[2].u.Interrupt.Affinity);

	/* Send IRP_MN_START_DEVICE */
	TopDeviceObject = IoGetAttachedDeviceReference(Pdo);
	KeInitializeEvent(
		&Event,
		NotificationEvent,
		FALSE);
	Irp = IoBuildSynchronousFsdRequest(
		IRP_MJ_PNP,
		TopDeviceObject,
		NULL,
		0,
		NULL,
		&Event,
		&IoStatusBlock);
	Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
	Irp->IoStatus.Information = 0;
	Stack = IoGetNextIrpStackLocation(Irp);
	Stack->MinorFunction = IRP_MN_START_DEVICE;
	Stack->Parameters.StartDevice.AllocatedResources = AllocatedResources;
	Stack->Parameters.StartDevice.AllocatedResourcesTranslated = AllocatedResourcesTranslated;
	Status = IoCallDriver(TopDeviceObject, Irp);
	if (Status == STATUS_PENDING)
	{
		KeWaitForSingleObject(
			&Event,
			Executive,
			KernelMode,
			FALSE,
			NULL);
		Status = IoStatusBlock.Status;
	}
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "IoCallDriver() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

cleanup:
	if (TopDeviceObject)
		ObDereferenceObject(TopDeviceObject);
	if (AllocatedResources)
		ExFreePoolWithTag(AllocatedResources, I8042PRT_TAG);
	if (AllocatedResourcesTranslated)
		ExFreePoolWithTag(AllocatedResourcesTranslated, I8042PRT_TAG);
}

static NTSTATUS
AddRegistryEntry(
	IN PCWSTR PortTypeName,
	IN PUNICODE_STRING DeviceName,
	IN PCWSTR RegistryPath)
{
	UNICODE_STRING PathU = RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\HARDWARE\\DEVICEMAP");
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE hDeviceMapKey = (HANDLE)-1;
	HANDLE hPortKey = (HANDLE)-1;
	UNICODE_STRING PortTypeNameU;
	NTSTATUS Status;

	InitializeObjectAttributes(&ObjectAttributes, &PathU, OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE, NULL, NULL);
	Status = ZwOpenKey(&hDeviceMapKey, 0, &ObjectAttributes);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "ZwOpenKey() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	RtlInitUnicodeString(&PortTypeNameU, PortTypeName);
	InitializeObjectAttributes(&ObjectAttributes, &PortTypeNameU, OBJ_KERNEL_HANDLE, hDeviceMapKey, NULL);
	Status = ZwCreateKey(&hPortKey, KEY_SET_VALUE, &ObjectAttributes, 0, NULL, REG_OPTION_VOLATILE, NULL);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "ZwCreateKey() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	Status = ZwSetValueKey(hPortKey, DeviceName, 0, REG_SZ, (PVOID)RegistryPath, wcslen(RegistryPath) * sizeof(WCHAR) + sizeof(UNICODE_NULL));
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "ZwSetValueKey() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	Status = STATUS_SUCCESS;

cleanup:
	if (hDeviceMapKey != (HANDLE)-1)
		ZwClose(hDeviceMapKey);
	if (hPortKey != (HANDLE)-1)
		ZwClose(hPortKey);
	return Status;
}

NTSTATUS
i8042AddLegacyKeyboard(
	IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	UNICODE_STRING KeyboardName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardPort8042");
	PI8042_DEVICE_TYPE DeviceExtension = NULL;
	PDEVICE_OBJECT Pdo = NULL;
	NTSTATUS Status;

	TRACE_(I8042PRT, "i8042AddLegacyKeyboard()\n");

	/* Create a named PDO */
	Status = IoCreateDevice(
		DriverObject,
		sizeof(I8042_DEVICE_TYPE),
		&KeyboardName,
		FILE_DEVICE_8042_PORT,
		FILE_DEVICE_SECURE_OPEN,
		TRUE,
		&Pdo);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "IoCreateDevice() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	/* Initialize device extension */
	DeviceExtension = (PI8042_DEVICE_TYPE)Pdo->DeviceExtension;
	RtlZeroMemory(DeviceExtension, sizeof(I8042_DEVICE_TYPE));
	*DeviceExtension = PhysicalDeviceObject;
	Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

	/* Add FDO at the top of the PDO */
	Status = i8042AddDevice(DriverObject, Pdo);
	if (!NT_SUCCESS(Status))
	{
		WARN_(I8042PRT, "i8042AddDevice() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

	/* We will send the IRP_MN_START_DEVICE later, once kbdclass is loaded */
	AddRegistryEntry(L"KeyboardPort", &KeyboardName, RegistryPath->Buffer);
	IoRegisterBootDriverReinitialization(
		DriverObject,
		SendStartDevice,
		Pdo);

	Status = STATUS_SUCCESS;
	/* Yes, completly forget the Pdo pointer, as we will never
	 * have to unload this driver during first stage setup.
	 */

cleanup:
	if (!NT_SUCCESS(Status))
	{
		if (Pdo)
			IoDeleteDevice(Pdo);
	}
	return Status;
}
