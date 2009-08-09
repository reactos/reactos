/*
 * PROJECT:     ReactOS Keyboard class driver
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/input/kbdclass/setup.c
 * PURPOSE:     Create a legacy PDO during ReactOS installation
 * PROGRAMMERS: Copyright 2006-2007 Hervé Poussineau (hpoussin@reactos.org)
 */

/* NOTE:
 * All this file is a big hack and should be removed one day...
 */

/* INCLUDES ******************************************************************/

#include "kbdclass.h"

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
	INFO_(CLASS_NAME, "IsFirstStageSetup() returns %s\n", ret ? "YES" : "NO");
	return ret;
}

VOID NTAPI
Send8042StartDevice(
	IN PDRIVER_OBJECT DriverObject,
	IN PDEVICE_OBJECT Pdo)
{
	PCM_RESOURCE_LIST AllocatedResources = NULL;
	PCM_RESOURCE_LIST AllocatedResourcesTranslated = NULL;
	PDEVICE_OBJECT TopDeviceObject = NULL;
	KEVENT Event;
	IO_STATUS_BLOCK IoStatusBlock;
	PIRP Irp;
	PIO_STACK_LOCATION Stack;
	ULONG ResourceListSize;
	NTSTATUS Status;

	TRACE_(CLASS_NAME, "SendStartDevice(%p)\n", Pdo);

	/* Create default resource list */
	ResourceListSize = sizeof(CM_RESOURCE_LIST) + 3 * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);
	AllocatedResources = ExAllocatePoolWithTag(PagedPool, ResourceListSize, CLASS_TAG);
	if (!AllocatedResources)
	{
		WARN_(CLASS_NAME, "ExAllocatePoolWithTag() failed\n");
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
	AllocatedResourcesTranslated = ExAllocatePoolWithTag(PagedPool, ResourceListSize, CLASS_TAG);
	if (!AllocatedResourcesTranslated)
	{
		WARN_(CLASS_NAME, "ExAllocatePoolWithTag() failed\n");
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
		WARN_(CLASS_NAME, "IoCallDriver() failed with status 0x%08lx\n", Status);
		goto cleanup;
	}

cleanup:
	if (TopDeviceObject)
		ObDereferenceObject(TopDeviceObject);
	if (AllocatedResources)
		ExFreePoolWithTag(AllocatedResources, CLASS_TAG);
	if (AllocatedResourcesTranslated)
		ExFreePoolWithTag(AllocatedResourcesTranslated, CLASS_TAG);
}
