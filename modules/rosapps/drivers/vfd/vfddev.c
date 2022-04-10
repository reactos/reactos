/*
	vfddev.c

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver: device create/delete functions

	Copyright (C) 2003-2005 Ken Kato
*/

#include "imports.h"
#include "vfddrv.h"
#include "vfddbg.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VfdCreateDevice)
#pragma alloc_text(PAGE, VfdDeleteDevice)
#endif	// ALLOC_PRAGMA

//
//	Create a VFD device object
//
NTSTATUS
VfdCreateDevice(
	IN PDRIVER_OBJECT		DriverObject,
	OUT PVOID				Parameter)
{
	NTSTATUS				status;
	ULONG					physical_num;

	UNICODE_STRING			unicode_name;
	WCHAR					name_buffer[40];

	PVFD_DRIVER_EXTENSION	driver_extension	= NULL;
	PDEVICE_OBJECT			device_object		= NULL;
	PDEVICE_EXTENSION		device_extension	= NULL;
	HANDLE					thread_handle		= NULL;

	VFDTRACE(VFDINFO | VFDDEV, ("[VFD] VfdCreateDevice - IN\n"));

#ifdef VFD_PNP

	//	Get the driver device_extension for the driver object
	driver_extension = IoGetDriverObjectExtension(
		DriverObject, VFD_DRIVER_EXTENSION_ID);

#else	// VFD_PNP

	//	The driver device_extension is passed as the Parameter
	driver_extension = (PVFD_DRIVER_EXTENSION)Parameter;

#endif	// VFD_PNP

	if (driver_extension == NULL) {
		VFDTRACE(VFDERR, ("[VFD] Failed to get the driver extension\n"));
		return STATUS_DRIVER_INTERNAL_ERROR;
	}

	//
	// Create a device object
	// \Device\Floppy<n>
	//
	physical_num = 0;

	do {
#ifndef __REACTOS__
		name_buffer[sizeof(name_buffer) - 1] = UNICODE_NULL;

		_snwprintf(name_buffer, sizeof(name_buffer) - 1,
			L"\\Device\\Floppy%lu", physical_num);
#else
		name_buffer[ARRAYSIZE(name_buffer) - 1] = UNICODE_NULL;

		_snwprintf(name_buffer, ARRAYSIZE(name_buffer) - 1,
			L"\\Device\\Floppy%lu", physical_num);
#endif

		RtlInitUnicodeString(&unicode_name, name_buffer);

		status = IoCreateDevice(
			DriverObject,
			sizeof(DEVICE_EXTENSION),
			&unicode_name,
			FILE_DEVICE_DISK,
			FILE_REMOVABLE_MEDIA | FILE_FLOPPY_DISKETTE | FILE_DEVICE_SECURE_OPEN,
			FALSE,
			&device_object);

		if (status != STATUS_OBJECT_NAME_EXISTS &&
			status != STATUS_OBJECT_NAME_COLLISION) {
			break;
		}
	}
	while (++physical_num < 100);

	if (!NT_SUCCESS(status)) {
		VFDTRACE(VFDERR,
			("[VFD] IoCreateDevice() %s\n",
			GetStatusName(status)));
		return status;
	}

	IoGetConfigurationInformation()->FloppyCount++;

	VFDTRACE(VFDINFO | VFDDEV,
		("[VFD] Created a device object %ws\n", name_buffer));

	//
	//	Initialize the device object / device extension
	//

	device_object->Flags |= DO_DIRECT_IO;

	device_extension = (PDEVICE_EXTENSION)device_object->DeviceExtension;

	RtlZeroMemory(device_extension, sizeof(DEVICE_EXTENSION));

	//	Store the back pointer to the device object

	device_extension->DeviceObject = device_object;

	//	Store the logical device number

	device_extension->DeviceNumber = driver_extension->NumberOfDevices;

	//	Store the device name

	if (!VfdCopyUnicode(&(device_extension->DeviceName), &unicode_name)) {
		VFDTRACE(VFDERR,
			("[VFD] Failed to allocate device name buffer\n"));
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanup;
	}

	//	set the default disk geometry (3.5" 1.44M)

	device_extension->Geometry = &geom_tbl[0];

	//	Create the interface link (\??\VirtualFD<n>)

#ifndef __REACTOS__
	name_buffer[sizeof(name_buffer) - 1] = UNICODE_NULL;

	_snwprintf(name_buffer, sizeof(name_buffer) - 1,
		L"\\??\\" VFD_DEVICE_BASENAME L"%lu",
		device_extension->DeviceNumber);
#else
	name_buffer[ARRAYSIZE(name_buffer) - 1] = UNICODE_NULL;

	_snwprintf(name_buffer, ARRAYSIZE(name_buffer) - 1,
		L"\\??\\" VFD_DEVICE_BASENAME L"%lu",
		device_extension->DeviceNumber);
#endif

	RtlInitUnicodeString(&unicode_name, name_buffer);

	status = IoCreateSymbolicLink(
		&unicode_name, &device_extension->DeviceName);

	if (!NT_SUCCESS(status)) {
		VFDTRACE(VFDERR,
			("[VFD] IoCreateSymbolicLink(%ws) %s\n",
			name_buffer, GetStatusName(status)));
		goto cleanup;
	}

	VFDTRACE(VFDINFO|VFDDEV,
		("[VFD] Created a symbolic link %ws\n", name_buffer));

	//	Prepare the IRP queue list for the device thread

	InitializeListHead(&device_extension->ListHead);

	KeInitializeSpinLock(&device_extension->ListLock);

	KeInitializeEvent(
		&device_extension->RequestEvent,
		SynchronizationEvent,
		FALSE);

	//	Create the device thread

	device_extension->TerminateThread = FALSE;

	status = PsCreateSystemThread(
		&thread_handle,
		(ACCESS_MASK) 0L,
		NULL,
		NULL,
		NULL,
		VfdDeviceThread,
		device_object);

	if (!NT_SUCCESS(status)) {
		VFDTRACE(VFDERR,
			("[VFD] PsCreateSystemThread() %s\n",
			GetStatusName(status)));
		goto cleanup;
	}

	//	get a reference pointer to the thread

	status = ObReferenceObjectByHandle(
		thread_handle,
		THREAD_ALL_ACCESS,
		NULL,
		KernelMode,
		&device_extension->ThreadPointer,
		NULL);

	ZwClose(thread_handle);

	if (!NT_SUCCESS(status)) {
		VFDTRACE(VFDERR,
			("[VFD] ObReferenceObjectByHandle() %s\n",
			GetStatusName(status)));
		goto cleanup;
	}

	//
	//	Load the persistent drive letter from the registry
	//
	if (driver_extension->RegistryPath.Buffer) {
		VfdLoadLink(device_extension,
			driver_extension->RegistryPath.Buffer);
		//	error is not fatal here
	}

	//	increment the number of devices in the driver extension

	driver_extension->NumberOfDevices++;

	if (DriverObject->DriverUnload) {
		//	not called from the DriverEntry routine
		device_object->Flags &= ~DO_DEVICE_INITIALIZING;
	}

#ifdef VFD_PNP
	if (Parameter) {
		//	return the device object pointer
		*(PDEVICE_OBJECT *)Parameter = device_object;
	}
#else	// VFD_PNP
	device_extension->DriverExtension = driver_extension;
#endif	// VFD_PNP

	VFDTRACE(VFDINFO | VFDDEV, ("[VFD] VfdCreateDevice - OK\n"));

	return STATUS_SUCCESS;

cleanup:
	//
	//	Something went wrong at one point
	//	Delete all resources that might be created in this function
	//
	if (thread_handle) {

		//	terminate the device thread
		device_extension->TerminateThread = TRUE;

		KeSetEvent(
			&device_extension->RequestEvent,
			(KPRIORITY) 0,
			FALSE);

		if (device_extension->ThreadPointer) {
			ObDereferenceObject(device_extension->ThreadPointer);
		}
	}

	VFDTRACE(VFDINFO|VFDDEV,
	("[VFD] Deleting symbolic link %ws\n", name_buffer));

	IoDeleteSymbolicLink(&unicode_name);

	if (device_extension->DeviceName.Buffer) {
		VFDTRACE(VFDINFO|VFDDEV, ("[VFD] Deleting device %ws\n",
			device_extension->DeviceName.Buffer));

		ExFreePool(device_extension->DeviceName.Buffer);
	}

	IoDeleteDevice(device_object);
	IoGetConfigurationInformation()->FloppyCount--;

	VFDTRACE(VFDINFO|VFDDEV,
		("[VFD] VfdCreateDevice - %s\n",
		GetStatusName(status)));

	return status;
}

//
//	delete a VFD device object
//
VOID
VfdDeleteDevice(
	IN PDEVICE_OBJECT			DeviceObject)
{
	PDEVICE_EXTENSION			device_extension;
	PVFD_DRIVER_EXTENSION		driver_extension;
	UNICODE_STRING				unicode_name;
	WCHAR						name_buffer[40];

	VFDTRACE(VFDINFO|VFDDEV, ("[VFD] VfdDeleteDevice - IN\n"));

	device_extension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

	//
	//	decrement the number of device in the driver extension
	//
#ifdef VFD_PNP
	driver_extension = IoGetDriverObjectExtension(
		DeviceObject->DriverObject, VFD_DRIVER_EXTENSION_ID);
#else	// VFD_PNP
	driver_extension = device_extension->DriverExtension;
#endif	// VFD_PNP

	if (driver_extension) {
		driver_extension->NumberOfDevices--;
	}

	//
	//	cleanup the device object
	//

	//	Terminate the device thread

	device_extension->TerminateThread = TRUE;

	KeSetEvent(
		&device_extension->RequestEvent,
		(KPRIORITY) 0,
		FALSE);

	KeWaitForSingleObject(
		device_extension->ThreadPointer,
		Executive,
		KernelMode,
		FALSE,
		NULL);

	ObDereferenceObject(
		device_extension->ThreadPointer);

	//	Delete security context object

	if (device_extension->SecurityContext) {
		SeDeleteClientSecurity(device_extension->SecurityContext);
		ExFreePool(device_extension->SecurityContext);
	}

	//	Close the image file or free the image buffer

	if (device_extension->FileHandle) {
		ZwClose(device_extension->FileHandle);
	}

	if (device_extension->FileBuffer) {
		ExFreePool(device_extension->FileBuffer);
	}

	//	Release the image path buffer

	if (device_extension->FileName.Buffer) {
		ExFreePool(device_extension->FileName.Buffer);
	}

	//	Remove the interface symbolic link

#ifndef __REACTOS__
	name_buffer[sizeof(name_buffer) - 1] = UNICODE_NULL;

	_snwprintf(name_buffer, sizeof(name_buffer) - 1,
		L"\\??\\" VFD_DEVICE_BASENAME L"%lu",
		device_extension->DeviceNumber);
#else
	name_buffer[ARRAYSIZE(name_buffer) - 1] = UNICODE_NULL;

	_snwprintf(name_buffer, ARRAYSIZE(name_buffer) - 1,
		L"\\??\\" VFD_DEVICE_BASENAME L"%lu",
		device_extension->DeviceNumber);
#endif

	RtlInitUnicodeString(&unicode_name, name_buffer);

	VFDTRACE(VFDINFO|VFDDEV,
		("[VFD] Deleting link %ws\n", name_buffer));

	IoDeleteSymbolicLink(&unicode_name);

	//	Remove the persistent drive letter

	if (device_extension->DriveLetter) {
#ifdef VFD_MOUNT_MANAGER
		if (OsMajorVersion >= 5) {
			//	Request the mount manager to remove the drive letter.
			//	This will cause the mount manager to update its database
			//	and it won't arbitrarily assign the drive letter the next
			//	time the driver starts.
			VfdMountMgrMountPoint(device_extension, 0);
		}
		else
#endif	// VFD_MOUNT_MANAGER
		{
			//	Windows NT style drive letter handling
			//	Simply remove the symbolic link
			VfdSetLink(device_extension, 0);
		}
	}

	//	Release the device name buffer

	if (device_extension->DeviceName.Buffer) {
		VFDTRACE(VFDINFO|VFDDEV,
			("[VFD] Deleting device %ws\n",
			device_extension->DeviceName.Buffer));

		ExFreePool(device_extension->DeviceName.Buffer);
	}

	//	Delete the device object

	IoDeleteDevice(DeviceObject);
	IoGetConfigurationInformation()->FloppyCount--;

	VFDTRACE(VFDINFO|VFDDEV,
		("[VFD] VfdDeleteDevice - OUT\n"));

	return;
}
