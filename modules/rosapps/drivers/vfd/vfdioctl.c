/*
	vfdioctl.c

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver: I/O control request handling

	Copyright (C) 2003-2005 Ken Kato
*/

#include "imports.h"
#include "vfddrv.h"
#include "vfddbg.h"

/*
#include <initguid.h>
DEFINE_GUID(VFD_GUID, 0x4563b3d8L, 0x936a, 0x4692,
	0xb6, 0x0c, 0x16, 0xd3, 0xb2, 0x57, 0xbb, 0xf2);
*/

#define IO_INPUTLEN(p)	(p)->Parameters.DeviceIoControl.InputBufferLength
#define IO_OUTPUTLEN(p)	(p)->Parameters.DeviceIoControl.OutputBufferLength
#define IO_CTRLCODE(p)	(p)->Parameters.DeviceIoControl.IoControlCode

//
//	IOCTL commands handler
//
NTSTATUS
NTAPI
VfdDeviceControl (
	IN PDEVICE_OBJECT			DeviceObject,
	IN PIRP						Irp)
{
	PDEVICE_EXTENSION			device_extension;
	PIO_STACK_LOCATION			io_stack;
	NTSTATUS					status;

	device_extension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
	io_stack = IoGetCurrentIrpStackLocation(Irp);

	Irp->IoStatus.Information = 0;

	VFDTRACE(VFDINFO, ("[VFD] %-40s %ws\n",
		GetIoControlName(IO_CTRLCODE(io_stack)),
		device_extension->DeviceName.Buffer));

#ifdef VFD_PNP
	status = IoAcquireRemoveLock(&device_extension->RemoveLock, Irp);

	if (!NT_SUCCESS(status)) {
		VFDTRACE(0,
			("Acquire RemoveLock failed %s\n", NtStatusToStr(status)));

		Irp->IoStatus.Status = status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return status;
	}
#endif	// VFD_PNP

/*
	//	Check if volume verification is required

	if ((DeviceObject->Flags & DO_VERIFY_VOLUME) &&
		!(io_stack->Flags & SL_OVERRIDE_VERIFY_VOLUME)) {

		VFDTRACE(VFDWARN,
			("[VFD] %-40s - %s\n",
				GetIoControlName(IO_CTRLCODE(io_stack)),
				GetStatusName(STATUS_VERIFY_REQUIRED)));

		Irp->IoStatus.Status = STATUS_VERIFY_REQUIRED;

		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return STATUS_VERIFY_REQUIRED;
	}
*/

	switch (IO_CTRLCODE(io_stack)) {
	case IOCTL_VFD_OPEN_IMAGE:
		//	Open an image file or create an empty RAM disk.
		//	Only a few checks are done here.
		//	Actual operation is done in device thread

		status = VfdOpenCheck(
			device_extension,
			(PVFD_IMAGE_INFO)Irp->AssociatedIrp.SystemBuffer,
			IO_INPUTLEN(io_stack));

		if (!NT_SUCCESS(status)) {
			break;
		}

		// Pass the task to the device thread
		status = STATUS_PENDING;
		break;

	case IOCTL_VFD_CLOSE_IMAGE:
	case IOCTL_DISK_EJECT_MEDIA:
	case IOCTL_STORAGE_EJECT_MEDIA:
		//	Close the current image file or delete the RAM disk
		//	Only status check is done here.
		//	Actual operation is done in device thread.

		if (!device_extension->FileHandle &&
			!device_extension->FileBuffer) {
			status = STATUS_NO_MEDIA_IN_DEVICE;
			break;
		}

		// Pass the task to the device thread
		status = STATUS_PENDING;
		break;

	case IOCTL_VFD_QUERY_IMAGE:
		//	Returns current image file information

		status = VfdQueryImage(
			device_extension,
			(PVFD_IMAGE_INFO)Irp->AssociatedIrp.SystemBuffer,
			IO_OUTPUTLEN(io_stack),
			&Irp->IoStatus.Information);

		break;

	case IOCTL_VFD_SET_LINK:
		// Create / remove a persistent drive letter
		// and store it in the registry

		if (IO_INPUTLEN(io_stack) < sizeof(CHAR)) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}

#ifdef VFD_MOUNT_MANAGER
		if (OsMajorVersion >= 5) {
			//	Windows 2000/XP
			//	Create a drive letter via the mount manager

			status = VfdMountMgrMountPoint(device_extension,
				*(PCHAR)Irp->AssociatedIrp.SystemBuffer);

			//	The new drive letter will be stored in the device extension
			//	and the registry when IOCTL_MOUNTDEV_LINK_CREATED or
			//	IOCTL_MOUNTDEV_LINK_DELETED is issued from the mount manager.
		}
		else
#else	//	VFD_MOUNT_MANAGER
		{
			//	Windows NT style drive letter assignment
			//	Simply create a symbolic link and store the new value

			status = VfdSetLink(device_extension,
				*(PCHAR)Irp->AssociatedIrp.SystemBuffer);

			if (NT_SUCCESS(status)) {
				//	Store the new drive letter into the registry
				status = VfdStoreLink(device_extension);
			}
		}
#endif	//	VFD_MOUNT_MANAGER
		break;

	case IOCTL_VFD_QUERY_LINK:
		//	Return the current persistent drive letter

		if (IO_OUTPUTLEN(io_stack) < sizeof(CHAR)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		*(PCHAR)Irp->AssociatedIrp.SystemBuffer =
			device_extension->DriveLetter;

		Irp->IoStatus.Information = sizeof(CHAR);
		status = STATUS_SUCCESS;
		break;

	case IOCTL_VFD_SET_PROTECT:
		//	Set media protect flag

		if (!device_extension->FileHandle &&
			!device_extension->FileBuffer) {
			status = STATUS_NO_MEDIA_IN_DEVICE;
			break;
		}

		device_extension->MediaFlags |= VFD_FLAG_WRITE_PROTECTED;
		status = STATUS_SUCCESS;
		break;

	case IOCTL_VFD_CLEAR_PROTECT:
		//	Clear media protect flag

		if (!device_extension->FileHandle &&
			!device_extension->FileBuffer) {
			status = STATUS_NO_MEDIA_IN_DEVICE;
			break;
		}

		device_extension->MediaFlags &= ~VFD_FLAG_WRITE_PROTECTED;
		status = STATUS_SUCCESS;
		break;

	case IOCTL_VFD_RESET_MODIFY:
		//	Reset the data modify flag

		if (!device_extension->FileHandle &&
			!device_extension->FileBuffer) {
			status = STATUS_NO_MEDIA_IN_DEVICE;
			break;
		}

		device_extension->MediaFlags &= ~VFD_FLAG_DATA_MODIFIED;
		status = STATUS_SUCCESS;
		break;

	case IOCTL_VFD_QUERY_NUMBER:
		//	Return VFD device number (\??\VirtualFD<n>)

		if (IO_OUTPUTLEN(io_stack) < sizeof(ULONG)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		*(PULONG)Irp->AssociatedIrp.SystemBuffer=
			device_extension->DeviceNumber;

		Irp->IoStatus.Information = sizeof(ULONG);
		status = STATUS_SUCCESS;
		break;

	case IOCTL_VFD_QUERY_NAME:
		//	Return VFD device name (\Device\Floppy<n>)
		//	counted unicode string (not null terminated)

		if (IO_OUTPUTLEN(io_stack) < sizeof(USHORT)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		{
			PUSHORT p = (PUSHORT)Irp->AssociatedIrp.SystemBuffer;

			*p = device_extension->DeviceName.Length;

			if (IO_OUTPUTLEN(io_stack) < sizeof(USHORT) + *p) {

				Irp->IoStatus.Information = sizeof(USHORT);
				status = STATUS_BUFFER_OVERFLOW;
				break;
			}

			RtlCopyMemory(p + 1, device_extension->DeviceName.Buffer, *p);

			Irp->IoStatus.Information = sizeof(USHORT) + *p;
		}

		status = STATUS_SUCCESS;
		break;

	case IOCTL_VFD_QUERY_VERSION:
		//	Return the VFD driver version

		if (IO_OUTPUTLEN(io_stack) < sizeof(ULONG)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		*(PULONG)Irp->AssociatedIrp.SystemBuffer =
			(VFD_DRIVER_MAJOR << 16) | VFD_DRIVER_MINOR | VFD_DEBUG_FLAG;

		Irp->IoStatus.Information = sizeof(ULONG);
		status = STATUS_SUCCESS;
		break;

	//
	//	standard disk and storage I/O control requests
	//

	case IOCTL_DISK_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY:
	case IOCTL_STORAGE_CHECK_VERIFY2:

		if (IO_OUTPUTLEN(io_stack) >= sizeof(ULONG)) {

			*(PULONG)Irp->AssociatedIrp.SystemBuffer =
				device_extension->MediaChangeCount;

			Irp->IoStatus.Information = sizeof(ULONG);
		}

		status = STATUS_SUCCESS;
		break;

	case IOCTL_DISK_FORMAT_TRACKS:
	case IOCTL_DISK_FORMAT_TRACKS_EX:
		//	Only parameter checks are performed here
		//	Actual operation is done by the device thread

		status = VfdFormatCheck(
			device_extension,
			(PFORMAT_PARAMETERS)Irp->AssociatedIrp.SystemBuffer,
			IO_INPUTLEN(io_stack),
			IO_CTRLCODE(io_stack));

		if (!NT_SUCCESS(status)) {
			break;
		}

		// Pass the task to the device thread
		status = STATUS_PENDING;
		break;

	case IOCTL_DISK_GET_DRIVE_GEOMETRY:
		//	Returns the geometry of current media

		if (!device_extension->FileHandle &&
			!device_extension->FileBuffer) {
			status = STATUS_NO_MEDIA_IN_DEVICE;
			break;
		}
		//	fall through

	case IOCTL_DISK_GET_MEDIA_TYPES:
	case IOCTL_STORAGE_GET_MEDIA_TYPES:
		//	Return *the last mounted* disk geometry, although xxx_GET_MEDIA_TYPES
		//	commands are supposed to return all supported media types.
		//	This makes the matter much simpler...;-)
		//	If no image has been mounted yet, 1.44MB media is assumed.

		if (IO_OUTPUTLEN(io_stack) < sizeof(DISK_GEOMETRY)) {
			return STATUS_BUFFER_TOO_SMALL;
		}

		//	Copy appropriate DISK_GEOMETRY into the output buffer

		if (device_extension->Geometry) {
			RtlCopyMemory(
				Irp->AssociatedIrp.SystemBuffer,
				device_extension->Geometry,
				sizeof(DISK_GEOMETRY));
		}
		else {
			//	default = 3.5" 1.44 MB media
			RtlCopyMemory(
				Irp->AssociatedIrp.SystemBuffer,
				&geom_tbl[VFD_MEDIA_F3_1P4],
				sizeof(DISK_GEOMETRY));
		}
		Irp->IoStatus.Information = sizeof(DISK_GEOMETRY);

		status = STATUS_SUCCESS;
		break;

	case IOCTL_DISK_GET_LENGTH_INFO:
		//	Return disk length information
		//	(Windows XP requires this request to be handled)

		if (!device_extension->FileHandle &&
			!device_extension->FileBuffer) {
			status = STATUS_NO_MEDIA_IN_DEVICE;
			break;
		}

		if (IO_OUTPUTLEN(io_stack) < sizeof(GET_LENGTH_INFORMATION)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		((PGET_LENGTH_INFORMATION)Irp->AssociatedIrp.SystemBuffer)->Length.QuadPart =
			VFD_SECTOR_TO_BYTE(device_extension->Sectors);

		Irp->IoStatus.Information = sizeof(GET_LENGTH_INFORMATION);

		status = STATUS_SUCCESS;
		break;

	case IOCTL_DISK_IS_WRITABLE:
		//	Checks if current media is writable

		if (!device_extension->FileHandle &&
			!device_extension->FileBuffer) {
			status = STATUS_NO_MEDIA_IN_DEVICE;
		}
		else if (device_extension->MediaFlags & VFD_FLAG_WRITE_PROTECTED) {
			status = STATUS_MEDIA_WRITE_PROTECTED;
		}
		else {
			status = STATUS_SUCCESS;
		}
		break;

/*
	case IOCTL_DISK_MEDIA_REMOVAL:
	case IOCTL_STORAGE_MEDIA_REMOVAL:
		//	Since removal lock is irrelevant for virtual disks,
		//	there's really nothing to do here...

		status = STATUS_SUCCESS;
		break;

	case IOCTL_STORAGE_GET_HOTPLUG_INFO:
		{
			PSTORAGE_HOTPLUG_INFO hotplug;

			if (IO_OUTPUTLEN(io_stack) < sizeof(STORAGE_HOTPLUG_INFO)) {
				status = STATUS_BUFFER_TOO_SMALL;
				break;
			}

			hotplug = (PSTORAGE_HOTPLUG_INFO)Irp->AssociatedIrp.SystemBuffer;

			RtlZeroMemory(hotplug, sizeof(STORAGE_HOTPLUG_INFO));

			hotplug->Size = sizeof(STORAGE_HOTPLUG_INFO);
			hotplug->MediaRemovable = 1;

			Irp->IoStatus.Information = sizeof(STORAGE_HOTPLUG_INFO);
			status = STATUS_SUCCESS;
		}
		break;
*/

#ifdef VFD_MOUNT_MANAGER
	//
	//	IO control requests received from the mount manager
	//	(on Windows 2000 / XP)
	//

	case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
		//	Returns a unique ID for the target device
		status = VfdMountDevUniqueId(
			device_extension,
			Irp->AssociatedIrp.SystemBuffer,
			IO_OUTPUTLEN(io_stack),
			&Irp->IoStatus);
		break;

//	case IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY:

	case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
		//	Returns the device name of the target device
		status = VfdMountDevDeviceName(
			device_extension,
			Irp->AssociatedIrp.SystemBuffer,
			IO_OUTPUTLEN(io_stack),
			&Irp->IoStatus);
		break;

	case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
		//	Returns the drive letter link which we want the mount manager
		//	to create.	This request is issued in response to the volume
		//	arrival notification, and the mount manager will create the
		//	symbolic link.
		status = VfdMountDevSuggestedLink(
			device_extension,
			Irp->AssociatedIrp.SystemBuffer,
			IO_OUTPUTLEN(io_stack),
			&Irp->IoStatus);
		break;

	case IOCTL_MOUNTDEV_LINK_CREATED:
	case IOCTL_MOUNTDEV_LINK_DELETED:
		//	Issued after the mount manager created/deleted a symbolic link
		status = VfdMountDevLinkModified(
			device_extension,
			Irp->AssociatedIrp.SystemBuffer,
			IO_INPUTLEN(io_stack),
			IO_CTRLCODE(io_stack));
		break;

/*
	case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
		{
			PMOUNTDEV_STABLE_GUID guid;

			if (IO_OUTPUTLEN(io_stack) < sizeof(MOUNTDEV_STABLE_GUID)) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}

			guid = Irp->AssociatedIrp.SystemBuffer;

			RtlCopyMemory(
				&guid->StableGuid, &VFD_GUID, sizeof(GUID));

			Irp->IoStatus.Information = sizeof(guid);
			status = STATUS_SUCCESS;
		}
		break;
*/
#endif	//	VFD_MOUNT_MANAGER

	default:
		//	Unknown IOCTL request
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

#if DBG
	if ((NT_SUCCESS(status) && (TraceFlags & VFDINFO) == VFDINFO) ||
		(TraceFlags & VFDWARN) == VFDWARN) {
		VFDTRACE(0,("[VFD] %-40s - %s\n",
			GetIoControlName(IO_CTRLCODE(io_stack)),
			GetStatusName(status)));
	}
#endif

	if (status == STATUS_PENDING) {
		//	Let the device thread perform the operation

		IoMarkIrpPending(Irp);

		ExInterlockedInsertTailList(
			&device_extension->ListHead,
			&Irp->Tail.Overlay.ListEntry,
			&device_extension->ListLock);

		KeSetEvent(
			&device_extension->RequestEvent,
			(KPRIORITY) 0,
			FALSE);
	}
	else {
		//	complete the operation

		Irp->IoStatus.Status = status;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

#ifdef VFD_PNP
		IoReleaseRemoveLock(&device_extension->RemoveLock, Irp);
#endif	// VFD_PNP
	}

	return status;
}

//
//	Handle IO control requests in the device thread
//
VOID
VfdIoCtlThread(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	PIRP					Irp,
	IN	ULONG					ControlCode)
{
	switch (ControlCode) {
	case IOCTL_VFD_OPEN_IMAGE:
		//	open the file from the caller's security context
		//	-- this allows this driver to open network files
		if (DeviceExtension->SecurityContext) {
			SeImpersonateClient(DeviceExtension->SecurityContext, NULL);
		}

		Irp->IoStatus.Status = VfdOpenImage(DeviceExtension,
			(PVFD_IMAGE_INFO)Irp->AssociatedIrp.SystemBuffer);

		PsRevertToSelf();
		break;

	case IOCTL_VFD_CLOSE_IMAGE:
	case IOCTL_DISK_EJECT_MEDIA:
	case IOCTL_STORAGE_EJECT_MEDIA:
		VfdCloseImage(DeviceExtension);
		Irp->IoStatus.Status = STATUS_SUCCESS;
		break;

	case IOCTL_DISK_FORMAT_TRACKS:
	case IOCTL_DISK_FORMAT_TRACKS_EX:
		Irp->IoStatus.Status = VfdFormatTrack(DeviceExtension,
			(PFORMAT_PARAMETERS)Irp->AssociatedIrp.SystemBuffer);
		break;

	default:
		//	This shouldn't happen...
		VFDTRACE(0,
			("[VFD] %s passed to the device thread\n",
			GetIoControlName(ControlCode)));

		Irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
	}

#if DBG
	if ((NT_SUCCESS(Irp->IoStatus.Status) && (TraceFlags & VFDINFO) == VFDINFO) ||
		(TraceFlags & VFDWARN) == VFDWARN) {
		VFDTRACE(0,("[VFD] %-40s - %s\n",
			GetIoControlName(ControlCode),
			GetStatusName(Irp->IoStatus.Status)));
	}
#endif
}
