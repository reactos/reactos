/*
	vfdmnt.c

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver mount manager functions

	Copyright (C) 2003-2005 Ken Kato
*/

#ifndef VFD_MOUNT_MANAGER
/*
	Not in working order for the time being
	so DO NOT define VFD_MOUNT_MANAGER macro
	unless you know exactly what you are doing...
*/
#if !defined(__REACTOS__) || defined(_MSC_VER)
//	suppress empty compile unit warning
#pragma warning (disable: 4206)
#pragma message ("Mount Manager support feature is disabled.")
#endif

#else	//	VFD_MOUNT_MANAGER
/*
	The flow of the drive letter assignment via the Mount Manager
	during the VFD driver start up

	1)	IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION	VFD -> MM
		notifies the mount manager of VFD devices.

	2)	IOCTL_MOUNTDEV_QUERY_DEVICE_NAME			VFD <- MM
		device name (\Device\Floppy<x>)				VFD -> MM

	3)	IOCTL_MOUNTDEV_QUERY_UNIQUE_ID				VFD <- MM
		device unique ID (\??\VirtualFD<x>)			VFD -> MM

	4)	IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME	VFD <- MM
		drive letter link (\DosDevices\<x>:)		VFD -> MM

	5)	The mount manager creates the drive letter link

	6)	IOCTL_MOUNTDEV_LINK_CREATED					VFD <- MM
		The driver stores the created drive letter

	The flow of the drive letter operation with IOCTL_VFD_SET_LINK

	1)	IOCTL_MOUNTMGR_CREATE_POINT or
		IOCTL_MOUNTMGR_DELETE_POINTS				VFD -> MM

	2)	The mount manager creates/deletes the drive letter link

	3)	IOCTL_MOUNTDEV_LINK_CREATED or
		IOCTL_MOUNTDEV_LINK_DELETED					VFD <- MM
		The driver stores the created/deleted drive letter
*/

#include "imports.h"
#include "vfddrv.h"
#include "vfddbg.h"

//
//	Call the mount manager with an IO control IRP
//
static NTSTATUS
VfdMountMgrSendRequest(
	ULONG			ControlCode,
	PVOID			InputBuffer,
	ULONG			InputLength,
	PVOID			OutputBuffer,
	ULONG			OutputLength);

#ifdef ALLOC_PRAGMA
//#pragma alloc_text(PAGE, VfdRegisterMountManager)
#pragma alloc_text(PAGE, VfdMountMgrNotifyVolume)
#pragma alloc_text(PAGE, VfdMountMgrMountPoint)
#pragma alloc_text(PAGE, VfdMountMgrSendRequest)
#pragma alloc_text(PAGE, VfdMountDevUniqueId)
#pragma alloc_text(PAGE, VfdMountDevDeviceName)
#pragma alloc_text(PAGE, VfdMountDevSuggestedLink)
#pragma alloc_text(PAGE, VfdMountDevLinkModified)
#endif	// ALLOC_PRAGMA

/*
#include <initguid.h>
#include <mountmgr.h>
//
//	register a device to the mount manager interface
//	does not work...
//
NTSTATUS
VfdRegisterMountManager(
	PDEVICE_EXTENSION		DeviceExtension)
{
	NTSTATUS		status = STATUS_SUCCESS;
	UNICODE_STRING	interface;
	UNICODE_STRING	interface2;

	VFDTRACE(VFDINFO,
		("[VFD] Registering %ws to the Mount Manager Interface\n",
		DeviceExtension->DeviceName.Buffer));

	RtlInitUnicodeString(&interface, NULL);

	status = IoRegisterDeviceInterface(
		DeviceExtension->DeviceObject,
		(LPGUID)&MOUNTDEV_MOUNTED_DEVICE_GUID,
		NULL,
		&interface);

	if (!NT_SUCCESS(status)) {
		VFDTRACE(0,
			("[VFD] IoRegisterDeviceInterface - %s\n",
			GetStatusName(status)));
		return status;
	}

	status = IoSetDeviceInterfaceState(&interface, TRUE);

	if (NT_SUCCESS(status)) {
		if (VfdCopyUnicode(&interface2, &interface)) {
			VFDTRACE(VFDINFO,
				("[VFD] Interface: %ws\n", interface2.Buffer));
		}
		else {
			VFDTRACE(0,
				("[VFD] Failed to allocate an interface name buffer\n"));
			status = STATUS_INSUFFICIENT_RESOURCES;
		}
	}
	else {
		VFDTRACE(0,
			("[VFD] IoSetDeviceInterfaceState - %s\n",
			GetStatusName(status)));
	}

	RtlFreeUnicodeString(&interface);
	VfdFreeUnicode(&interface2);

	return status;
}
*/

//
//	informs the Mount Manager of a new VFD device
//
NTSTATUS
VfdMountMgrNotifyVolume(
	PDEVICE_EXTENSION		DeviceExtension)
{
	PMOUNTMGR_TARGET_NAME	target_name;
	USHORT					target_name_buf[MAXIMUM_FILENAME_LENGTH];
	NTSTATUS				status;

	VFDTRACE(VFDINFO,
		("[VFD] VfdMountMgrNotifyVolume - %ws\n",
		DeviceExtension->DeviceName.Buffer));

	target_name = (PMOUNTMGR_TARGET_NAME)target_name_buf;

	target_name->DeviceNameLength =
		DeviceExtension->DeviceName.Length;

	RtlCopyMemory(
		target_name->DeviceName,
		DeviceExtension->DeviceName.Buffer,
		DeviceExtension->DeviceName.Length);

	status = VfdMountMgrSendRequest(
		IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION,
		target_name,
		sizeof(target_name->DeviceNameLength) + target_name->DeviceNameLength,
		NULL,
		0);

	VFDTRACE(VFDINFO,
		("[VFD] VfdMountMgrNotifyVolume - %s\n",
		GetStatusName(status)));

	return status;
}

//
//	Create / remove a drive letter via the Mount Manager
//
NTSTATUS
VfdMountMgrMountPoint(
	PDEVICE_EXTENSION	DeviceExtension,
	CHAR				DriveLetter)
{
	ULONG				alloc_size;
	UNICODE_STRING		link_name;
	WCHAR				link_buf[20];
	NTSTATUS			status;

	VFDTRACE(VFDINFO, ("[VFD] VfdMountMgrMountPoint - IN\n"));

	//	convert lower case into upper case

	if (DriveLetter >= 'a' && DriveLetter <= 'z') {
		DriveLetter -= ('a' - 'A');
	}

	if (DriveLetter >= 'A' && DriveLetter <= 'Z') {

		//	Create a new drive letter

		PMOUNTMGR_CREATE_POINT_INPUT	create;

		swprintf(link_buf, L"\\DosDevices\\%wc:", DriveLetter);

		RtlInitUnicodeString(&link_name, link_buf);

		VFDTRACE(VFDINFO,
			("[VFD] Creating a link: %ws => %ws\n",
			link_buf, DeviceExtension->DeviceName.Buffer));

		//	allocate buffer for MOUNTMGR_CREATE_POINT_INPUT

		alloc_size = sizeof(MOUNTMGR_CREATE_POINT_INPUT) +
			link_name.Length + DeviceExtension->DeviceName.Length;

		create = (PMOUNTMGR_CREATE_POINT_INPUT)ExAllocatePoolWithTag(
			NonPagedPool, alloc_size, VFD_POOL_TAG);

		if (!create) {
			VFDTRACE(0, ("[VFD] Failed to allocate mount point input\n"));
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		//	set the symbolic link name

		create->SymbolicLinkNameOffset	= sizeof(MOUNTMGR_CREATE_POINT_INPUT);
		create->SymbolicLinkNameLength	= link_name.Length;

		RtlCopyMemory(
			(PCHAR)create + create->SymbolicLinkNameOffset,
			link_name.Buffer,
			link_name.Length);

		//	set the target device name

		create->DeviceNameOffset		= (USHORT)
			(create->SymbolicLinkNameOffset + create->SymbolicLinkNameLength);
		create->DeviceNameLength		= DeviceExtension->DeviceName.Length;

		RtlCopyMemory(
			(PCHAR)create + create->DeviceNameOffset,
			DeviceExtension->DeviceName.Buffer,
			DeviceExtension->DeviceName.Length);

		//	call the mount manager with the IO control request

		status = VfdMountMgrSendRequest(
			IOCTL_MOUNTMGR_CREATE_POINT,
			create, alloc_size, NULL, 0);

		ExFreePool(create);

		//	no need to set the new drive letter into the
		//	DeviceExtension because the mount manager will issue an
		//	IOCTL_MOUNTDEV_LINK_CREATED and it will be processed then
	}
	else if (DriveLetter == 0) {

		//	Delete the existing drive letter

		PMOUNTMGR_MOUNT_POINT	mount;
		PMOUNTMGR_MOUNT_POINTS	points;
		UNICODE_STRING	unique_id;
		WCHAR			unique_buf[20];

		swprintf(link_buf, L"\\DosDevices\\%wc:",
			DeviceExtension->DriveLetter);

		VFDTRACE(VFDINFO,
			("[VFD] Deleting link: %ws\n", link_buf));

		RtlInitUnicodeString(&link_name, link_buf);

		swprintf(unique_buf, L"\\??\\" VFD_DEVICE_BASENAME L"%lu",
			DeviceExtension->DeviceNumber);

		RtlInitUnicodeString(&unique_id, unique_buf);

		//	allocate buffer for MOUNTMGR_MOUNT_POINT

		alloc_size = sizeof(MOUNTMGR_MOUNT_POINT) +
			link_name.Length +
			unique_id.Length +
			DeviceExtension->DeviceName.Length;

		mount = (PMOUNTMGR_MOUNT_POINT)ExAllocatePoolWithTag(
			NonPagedPool, alloc_size, VFD_POOL_TAG);

		if (!mount) {
			VFDTRACE(0, ("[VFD] Failed to allocate mount point input\n"));
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		RtlZeroMemory(mount, alloc_size + sizeof(WCHAR));

		//	set the symbolic link name

		mount->SymbolicLinkNameOffset	= sizeof(MOUNTMGR_MOUNT_POINT);
		mount->SymbolicLinkNameLength	= link_name.Length;

		RtlCopyMemory(
			(PCHAR)mount + mount->SymbolicLinkNameOffset,
			link_name.Buffer, link_name.Length);

		//	set the unique id

		mount->UniqueIdOffset =
			mount->SymbolicLinkNameOffset +
			mount->SymbolicLinkNameLength;
		mount->UniqueIdLength = unique_id.Length;

		RtlCopyMemory(
			(PCHAR)mount + mount->UniqueIdOffset,
			unique_id.Buffer, unique_id.Length);

		//	set the target device name

		mount->DeviceNameOffset =
			mount->UniqueIdOffset +
			mount->UniqueIdLength;
		mount->DeviceNameLength =
			DeviceExtension->DeviceName.Length;

		RtlCopyMemory(
			(PCHAR)mount + mount->DeviceNameOffset,
			DeviceExtension->DeviceName.Buffer,
			DeviceExtension->DeviceName.Length);

		//	prepare the output buffer

		points = (PMOUNTMGR_MOUNT_POINTS)ExAllocatePoolWithTag(
			NonPagedPool, alloc_size * 2, VFD_POOL_TAG);

		status = VfdMountMgrSendRequest(
			IOCTL_MOUNTMGR_DELETE_POINTS,
			mount, alloc_size, points, alloc_size * 2);

		ExFreePool(mount);
		ExFreePool(points);

		if (status == STATUS_OBJECT_NAME_NOT_FOUND) {
			//	the drive letter did not exist in the first place
			DeviceExtension->DriveLetter = 0;
		}

		//	no need to clear the drive letter in the
		//	DeviceExtension because the mount manager will issue an
		//	IOCTL_MOUNTDEV_LINK_DELETED and it will be processed then
	}
	else {
		return STATUS_INVALID_PARAMETER;
	}

	VFDTRACE(VFDINFO, ("[VFD] VfdMountMgrMountPoint - %s\n",
		GetStatusName(status)));

	return status;
}

//
//	send a request to the Mount Manager
//
NTSTATUS
VfdMountMgrSendRequest(
	ULONG			ControlCode,
	PVOID			InputBuffer,
	ULONG			InputLength,
	PVOID			OutputBuffer,
	ULONG			OutputLength)
{
	NTSTATUS		status = STATUS_SUCCESS;
	UNICODE_STRING	mntmgr_name;
	PDEVICE_OBJECT	mntmgr_dev;
	PFILE_OBJECT	mntmgr_file;
	IO_STATUS_BLOCK	io_status;
	KEVENT			event;
	PIRP			irp;

	//	Obtain a pointer to the Mount Manager device object

	RtlInitUnicodeString(
		&mntmgr_name,
		MOUNTMGR_DEVICE_NAME);

	status = IoGetDeviceObjectPointer(
		&mntmgr_name,
		FILE_READ_ATTRIBUTES,
		&mntmgr_file,
		&mntmgr_dev);

	if (!NT_SUCCESS(status)) {
		VFDTRACE(VFDWARN,
			("[VFD] IoGetDeviceObjectPointer - %s\n", GetStatusName(status)));
		return status;
	}

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	//	Create an IRP request block

	irp = IoBuildDeviceIoControlRequest(
		ControlCode,
		mntmgr_dev,
		InputBuffer,
		InputLength,
		OutputBuffer,
		OutputLength,
		FALSE,
		&event,
		&io_status);

	if (!irp) {
		VFDTRACE(VFDWARN,
			("[VFD] IoBuildDeviceIoControlRequest\n"));
		ObDereferenceObject(mntmgr_file);
		return STATUS_DRIVER_INTERNAL_ERROR;
	}

	//	Call the mount manager

	status = IoCallDriver(mntmgr_dev, irp);

	if (!NT_SUCCESS(status)) {
		VFDTRACE(VFDWARN,
			("[VFD] IoCallDriver - %s\n", GetStatusName(status)));
	}

	if (status == STATUS_PENDING) {

		//	Wait for the operation to complete

		KeWaitForSingleObject(
			&event, Executive, KernelMode, FALSE, NULL);

		status = io_status.Status;

		if (!NT_SUCCESS(status)) {
			VFDTRACE(VFDWARN,
				("[VFD] IoCallDriver - %s\n", GetStatusName(status)));
		}
	}

	ObDereferenceObject(mntmgr_file);

	return status;
}

//
//	IOCTL_MOUNTDEV_QUERY_UNIQUE_ID
//	-- use the device interface link (\??\VirtualFD<x>) as the unique ID
//
NTSTATUS
VfdMountDevUniqueId(
	PDEVICE_EXTENSION	DeviceExtension,
	PMOUNTDEV_UNIQUE_ID	UniqueId,
	ULONG				OutputLength,
	PIO_STATUS_BLOCK	IoStatus)
{
	WCHAR				buf[20];
	UNICODE_STRING		unicode;

	if (OutputLength < sizeof(MOUNTDEV_UNIQUE_ID)) {
		return STATUS_INVALID_PARAMETER;
	}

	swprintf(buf,
		L"\\??\\" VFD_DEVICE_BASENAME L"%lu",
		DeviceExtension->DeviceNumber);

	RtlInitUnicodeString(&unicode, buf);

	UniqueId->UniqueIdLength = unicode.Length;

	if (OutputLength <
		sizeof(UniqueId->UniqueIdLength) + UniqueId->UniqueIdLength) {

		IoStatus->Information = sizeof(MOUNTDEV_UNIQUE_ID);
		return STATUS_BUFFER_OVERFLOW;
	}

	RtlCopyMemory(
		UniqueId->UniqueId, buf, unicode.Length);

	IoStatus->Information =
		sizeof(UniqueId->UniqueIdLength) + UniqueId->UniqueIdLength;

	return STATUS_SUCCESS;
}

//
//	IOCTL_MOUNTDEV_QUERY_DEVICE_NAME
//	Returns the device name of the target device (\Device\Floppy<n>)
//
NTSTATUS
VfdMountDevDeviceName(
	PDEVICE_EXTENSION	DeviceExtension,
	PMOUNTDEV_NAME		DeviceName,
	ULONG				OutputLength,
	PIO_STATUS_BLOCK	IoStatus)
{
	if (OutputLength < sizeof(MOUNTDEV_NAME)) {
		return STATUS_INVALID_PARAMETER;
	}

	DeviceName->NameLength = DeviceExtension->DeviceName.Length;

	if (OutputLength <
		sizeof(DeviceName->NameLength) + DeviceName->NameLength) {

		IoStatus->Information = sizeof(MOUNTDEV_NAME);
		return STATUS_BUFFER_OVERFLOW;
	}

	RtlCopyMemory(
		DeviceName->Name,
		DeviceExtension->DeviceName.Buffer,
		DeviceName->NameLength);

	IoStatus->Information =
		sizeof(DeviceName->NameLength) + DeviceName->NameLength;

	return STATUS_SUCCESS;
}

//
//	IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME
//	Returns the drive letter link which we want the mount manager
//	to create.	This request is issued in response to the volume
//	arrival notification, and the mount manager will create the
//	symbolic link.
//
NTSTATUS
VfdMountDevSuggestedLink(
	PDEVICE_EXTENSION	DeviceExtension,
	PMOUNTDEV_SUGGESTED_LINK_NAME	LinkName,
	ULONG				OutputLength,
	PIO_STATUS_BLOCK	IoStatus)
{
	WCHAR				buf[20];
	UNICODE_STRING		unicode;

	if (OutputLength < sizeof(MOUNTDEV_SUGGESTED_LINK_NAME)) {
		return STATUS_INVALID_PARAMETER;
	}

	LinkName->UseOnlyIfThereAreNoOtherLinks = TRUE;

	if (!DeviceExtension->DriveLetter) {

		//	No persistent drive letter stored in the registry
		VFDTRACE(VFDINFO, ("[VFD] suggested link : none\n"));

		LinkName->NameLength  = 0;

		IoStatus->Information = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
		return STATUS_SUCCESS;
	}

	//	A persistent drive letter exists

	swprintf(buf, L"\\DosDevices\\%wc:",
		DeviceExtension->DriveLetter);

	VFDTRACE(VFDINFO, ("[VFD] suggested link : %ws\n", buf));

	RtlInitUnicodeString(&unicode, buf);

	LinkName->NameLength = unicode.Length;

	if (OutputLength <
		sizeof(MOUNTDEV_SUGGESTED_LINK_NAME) +
		LinkName->NameLength - sizeof(WCHAR)) {

		IoStatus->Information = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
		return STATUS_BUFFER_OVERFLOW;
	}

	RtlCopyMemory(LinkName->Name, buf, unicode.Length);

	IoStatus->Information =
		sizeof(MOUNTDEV_SUGGESTED_LINK_NAME) +
		LinkName->NameLength - sizeof(WCHAR);

	return STATUS_SUCCESS;
}

//
//	IOCTL_MOUNTDEV_LINK_CREATED / IOCTL_MOUNTDEV_LINK_DELETED
//	Issued after the mount manager created/deleted a symbolic link
//	If the link is a drive letter, store the new value into the
//	registry as the new drive letter
//
NTSTATUS
VfdMountDevLinkModified(
	PDEVICE_EXTENSION	DeviceExtension,
	PMOUNTDEV_NAME		LinkName,
	ULONG				InputLength,
	ULONG				ControlCode)
{
	if (InputLength < sizeof(MOUNTDEV_NAME)) {
		return STATUS_INVALID_PARAMETER;
	}

	if (InputLength < sizeof(MOUNTDEV_NAME) +
		LinkName->NameLength - sizeof(WCHAR)) {

		return STATUS_INVALID_PARAMETER;
	}

#if DBG
	{	//	Print the reported link name
		PWSTR buf = ExAllocatePoolWithTag(
			PagedPool, LinkName->NameLength + sizeof(WCHAR), VFD_POOL_TAG);

		if (buf) {
			RtlZeroMemory(buf, LinkName->NameLength + sizeof(WCHAR));
			RtlCopyMemory(buf, LinkName->Name, LinkName->NameLength);
			VFDTRACE(VFDINFO, ("[VFD] %ws\n", buf));
			ExFreePool(buf);
		}
	}
#endif	// DBG

	if (LinkName->NameLength == 28		&&
		LinkName->Name[0]	== L'\\'	&&
		LinkName->Name[1]	== L'D'		&&
		LinkName->Name[2]	== L'o'		&&
		LinkName->Name[3]	== L's'		&&
		LinkName->Name[4]	== L'D'		&&
		LinkName->Name[5]	== L'e'		&&
		LinkName->Name[6]	== L'v'		&&
		LinkName->Name[7]	== L'i'		&&
		LinkName->Name[8]	== L'c'		&&
		LinkName->Name[9]	== L'e'		&&
		LinkName->Name[10]	== L's'		&&
		LinkName->Name[11]	== L'\\'	&&
		LinkName->Name[12]	>= L'A'		&&
		LinkName->Name[12]	<= L'Z'		&&
		LinkName->Name[13]	== L':') {

		//	The link is a drive letter

		if (ControlCode == IOCTL_MOUNTDEV_LINK_CREATED) {
			//	link is created - store the new drive letter
			DeviceExtension->DriveLetter = (CHAR)LinkName->Name[12];
		}
		else {
			//	link is deleted - clear the drive letter
			DeviceExtension->DriveLetter = 0;
		}

		//	Store the value into the registry

		VfdStoreLink(DeviceExtension);
	}

	return STATUS_SUCCESS;
}

#endif	//	VFD_MOUNT_MANAGER
