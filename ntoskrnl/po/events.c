/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/po/events.c
 * PURPOSE:         Power Manager
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include <ntoskrnl.h>
//#define NDEBUG
#include <internal/debug.h>

PKWIN32_POWEREVENT_CALLOUT PopEventCallout;
extern PCALLBACK_OBJECT SetSystemTimeCallback;

VOID
NTAPI
PoNotifySystemTimeSet(VOID)
{
    KIRQL OldIrql;

    /* Check if Win32k registered a notification callback */
    if (PopEventCallout)
    {
        /* Raise to dispatch */
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

        /* Notify the callback */
        ExNotifyCallback(SetSystemTimeCallback, NULL, NULL);

        /* Lower IRQL back */
        KeLowerIrql(OldIrql);
    }
}

NTSTATUS
NTAPI
PopAddRemoveSysCapsCallback(
	IN PVOID NotificationStructure,
	IN PVOID Context)
{
	PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE DeviceHandle;
	IO_STATUS_BLOCK IoStatusBlock;
	BOOLEAN Arrival;
	ULONG Caps;
	NTSTATUS Status;

	DPRINT("PopAddRemoveSysCapsCallback(%p %p)\n",
		NotificationStructure, Context);

	Notification = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION)NotificationStructure;
	if (Notification->Version != 1)
		return STATUS_REVISION_MISMATCH;
	if (Notification->Size != sizeof(DEVICE_INTERFACE_CHANGE_NOTIFICATION))
		return STATUS_INVALID_PARAMETER;
	if (RtlCompareMemory(&Notification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL, sizeof(GUID) == sizeof(GUID)))
		Arrival = TRUE;
	else if (RtlCompareMemory(&Notification->Event, &GUID_DEVICE_INTERFACE_REMOVAL, sizeof(GUID) == sizeof(GUID)))
		Arrival = FALSE;
	else
		return STATUS_INVALID_PARAMETER;

	if (Arrival)
	{
		DPRINT("Arrival of %wZ\n", Notification->SymbolicLinkName);

		/* Open device */
		InitializeObjectAttributes(
			&ObjectAttributes,
			Notification->SymbolicLinkName,
			OBJ_KERNEL_HANDLE,
			NULL,
			NULL);
		Status = ZwOpenFile(
			&DeviceHandle,
			FILE_READ_DATA,
			&ObjectAttributes,
			&IoStatusBlock,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			0);
		if (!NT_SUCCESS(Status))
		{
			DPRINT("ZwOpenFile() failed with status 0x%08lx\n", Status);
			return Status;
		}

		/* Send IOCTL_GET_SYS_BUTTON_CAPS to get new caps */
		Status = ZwDeviceIoControlFile(
			DeviceHandle,
			NULL,
			NULL,
			NULL,
			&IoStatusBlock,
			IOCTL_GET_SYS_BUTTON_CAPS,
			NULL,
			0,
			&Caps,
			sizeof(Caps));
		if (!NT_SUCCESS(Status))
		{
			DPRINT("ZwDeviceIoControlFile(IOCTL_GET_SYS_BUTTON_CAPS) failed with status 0x%08lx\n", Status);
			ZwClose(DeviceHandle);
			return Status;
		}
		/* FIXME: What do do with this? */
		DPRINT1("Device capabilities: 0x%lx\n", Caps);

		/* Send IOCTL_GET_SYS_BUTTON_CAPS to get current caps */
		/* FIXME: Set a IO completion routine on it to be able to send a new one */
		DPRINT1("Send a IOCTL_GET_SYS_BUTTON_EVENT\n");
		return ZwClose(DeviceHandle);
	}
	else
	{
		DPRINT1("Removal of a power capable device not implemented\n");
		return STATUS_NOT_IMPLEMENTED;
	}
}
