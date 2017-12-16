/*
	vfddrv.c

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver: miscellaneous driver functions

	Copyright (C) 2003-2005 Ken Kato
*/

#include "imports.h"
#include "vfddrv.h"
#include "vfddbg.h"

//
//	driver reinitialize routine
//	-- create a drive letter for each device
//
#ifdef __cplusplus
extern "C"
#endif	// __cplusplus
static VOID
NTAPI
VfdReinitialize(
	IN PDRIVER_OBJECT			DriverObject,
	IN PVOID					Context,
	IN ULONG					Count);

//
//	specify code segment
//
#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, VfdReinitialize)
#pragma alloc_text(PAGE, VfdUnloadDriver)
#pragma alloc_text(PAGE, VfdCreateClose)
#pragma alloc_text(PAGE, VfdCopyUnicode)
#pragma alloc_text(PAGE, VfdFreeUnicode)
#endif	// ALLOC_PRAGMA

//
//	operating system version
//
#ifndef __REACTOS__
extern ULONG OsMajorVersion = 0;
extern ULONG OsMinorVersion = 0;
extern ULONG OsBuildNumber	= 0;
#else
ULONG OsMajorVersion = 0;
ULONG OsMinorVersion = 0;
ULONG OsBuildNumber	= 0;
#endif

//
//	Trace level flag
//
#if DBG
#ifndef __REACTOS__
extern ULONG TraceFlags	= (ULONG)-1;
#else
ULONG TraceFlags	= (ULONG)-1;
#endif
#endif	// DBG

//
//	Driver Entry routine
//
NTSTATUS
NTAPI
DriverEntry (
	IN PDRIVER_OBJECT			DriverObject,
	IN PUNICODE_STRING			RegistryPath)
{
	NTSTATUS					status;
	PVFD_DRIVER_EXTENSION		driver_extension;
	ULONG						number_of_devices = VFD_DEFAULT_DEVICES;

	ASSERT(DriverObject);

	//	Get operating system version

	PsGetVersion(&OsMajorVersion, &OsMinorVersion, &OsBuildNumber, NULL);

#ifdef VFD_PNP
#define VFD_PNP_TAG	"(Plug & Play version)"
#else
#define VFD_PNP_TAG
#endif

	VFDTRACE(0, ("[VFD] %s %s" VFD_PNP_TAG "\n",
		VFD_PRODUCT_NAME, VFD_DRIVER_VERSION_STR));

	VFDTRACE(0,
		("[VFD] Running on Windows NT %lu.%lu build %lu\n",
		OsMajorVersion, OsMinorVersion, OsBuildNumber));

	VFDTRACE(0,
		("[VFD] Build Target Environment: %d\n", VER_PRODUCTBUILD));

#ifdef VFD_PNP

	// Create device_extension for the driver object to store driver specific
	// information. Device specific information are stored in device extension
	// for each device object.

	status = IoAllocateDriverObjectExtension(
		DriverObject,
		VFD_DRIVER_EXTENSION_ID,
		sizeof(VFD_DRIVER_EXTENSION),
		&driver_extension);

	if(!NT_SUCCESS(status)) {
		VFDTRACE(0, ("[VFD] IoAllocateDriverObjectExtension - %s\n",
			GetStatusName(status)));
		return status;
	}

#else	// VFD_PNP

	//	Windows NT doesn't have the IoAllocateDriverObjectExtension
	//	function and I think there's little point in making a non-PnP
	//	driver incompatible with Windows NT.

	driver_extension = (PVFD_DRIVER_EXTENSION)ExAllocatePoolWithTag(
		PagedPool, sizeof(VFD_DRIVER_EXTENSION), VFD_POOL_TAG);

	if (!driver_extension) {
		VFDTRACE(0, ("[VFD] failed to allocate the driver extension.\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

#endif	// VFD_PNP

	RtlZeroMemory(driver_extension, sizeof(VFD_DRIVER_EXTENSION));

	//
	// Copy the registry path into the driver extension so we can use it later
	//
	if (VfdCopyUnicode(&(driver_extension->RegistryPath), RegistryPath)) {

		//
		//	Read config values from the registry
		//
		RTL_QUERY_REGISTRY_TABLE params[3];
		ULONG default_devs	= VFD_DEFAULT_DEVICES;
#if DBG
		ULONG default_trace = (ULONG)-1;
#endif

		RtlZeroMemory(params, sizeof(params));

		VFDTRACE(0, ("[VFD] Registry Path: %ws\n",
			driver_extension->RegistryPath.Buffer));

		params[0].Flags			= RTL_QUERY_REGISTRY_DIRECT;
		params[0].Name			= VFD_REG_DEVICE_NUMBER;
		params[0].EntryContext	= &number_of_devices;
		params[0].DefaultType	= REG_DWORD;
		params[0].DefaultData	= &default_devs;
		params[0].DefaultLength	= sizeof(ULONG);

#if DBG
		params[1].Flags			= RTL_QUERY_REGISTRY_DIRECT;
		params[1].Name			= VFD_REG_TRACE_FLAGS;
		params[1].EntryContext	= &TraceFlags;
		params[1].DefaultType	= REG_DWORD;
		params[1].DefaultData	= &default_trace;
		params[1].DefaultLength	= sizeof(ULONG);
#endif	// DBG

		status = RtlQueryRegistryValues(
			RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
			driver_extension->RegistryPath.Buffer,
			params, NULL, NULL);

		if (!NT_SUCCESS(status) ||
			number_of_devices == 0 ||
			number_of_devices > VFD_MAXIMUM_DEVICES) {
			number_of_devices = VFD_DEFAULT_DEVICES;
		}

		VFDTRACE(0,("[VFD] NumberOfDevices = %lu\n", number_of_devices));
		VFDTRACE(0,("[VFD] TraceFlags = 0x%08x\n", TraceFlags));
	}
	else {
		VFDTRACE(0, ("[VFD] failed to allocate the registry path buffer.\n"));
		// this error is not fatal
	}

	//
	//	Create VFD device objects
	//
	do {
#ifdef VFD_PNP
		status = VfdCreateDevice(DriverObject, NULL);
#else	// VFD_PNP
		status = VfdCreateDevice(DriverObject, driver_extension);
#endif	// VFD_PNP

		if (!NT_SUCCESS(status)) {
			break;
		}
	}
	while (driver_extension->NumberOfDevices < number_of_devices);

	if (!driver_extension->NumberOfDevices) {

		//	Failed to create even one device

		VfdFreeUnicode(&(driver_extension->RegistryPath));

		return status;
	}

	//	Setup dispatch table

	DriverObject->MajorFunction[IRP_MJ_CREATE]			= VfdCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]			= VfdCreateClose;
	DriverObject->MajorFunction[IRP_MJ_READ]			= VfdReadWrite;
	DriverObject->MajorFunction[IRP_MJ_WRITE]			= VfdReadWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]	= VfdDeviceControl;

#ifdef VFD_PNP
	DriverObject->MajorFunction[IRP_MJ_PNP]				= VfdPlugAndPlay;
	DriverObject->MajorFunction[IRP_MJ_POWER]			= VfdPowerControl;
	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]	= VfdSystemControl;
	DriverObject->DriverExtension->AddDevice			= VfdAddDevice;
#endif	// VFDPNP

	DriverObject->DriverUnload = VfdUnloadDriver;

	//	Register the driver reinitialize routine to be called
	//	*after* the DriverEntry routine returns

	IoRegisterDriverReinitialization(
		DriverObject, VfdReinitialize, NULL);

	VFDTRACE(VFDINFO,
		("[VFD] driver initialized with %lu devices.\n",
		driver_extension->NumberOfDevices));

	return STATUS_SUCCESS;
}

//
//	Driver unload routine
//	Cleans up the device objects and other resources
//
VOID
NTAPI
VfdUnloadDriver (
	IN PDRIVER_OBJECT			DriverObject)
{
	PDEVICE_OBJECT				device_object;
	PVFD_DRIVER_EXTENSION		driver_extension;

	VFDTRACE(VFDINFO, ("[VFD] VfdUnloadDriver - IN\n"));

	device_object = DriverObject->DeviceObject;

#ifdef VFD_PNP
	driver_extension = IoGetDriverObjectExtension(
		DriverObject, VFD_DRIVER_EXTENSION_ID);
#else
	if (device_object && device_object->DeviceExtension) {
		driver_extension =
			((PDEVICE_EXTENSION)(device_object->DeviceExtension))->DriverExtension;
	}
	else {
		driver_extension = NULL;
	}
#endif	// VFD_PNP

	//
	//	Delete all remaining device objects
	//
	while (device_object) {

		PDEVICE_OBJECT next_device = device_object->NextDevice;

		VfdDeleteDevice(device_object);

		device_object = next_device;
	}

	//
	//	Release the driver extension and the registry path buffer
	//
	if (driver_extension) {

		if (driver_extension->RegistryPath.Buffer) {
			VFDTRACE(0, ("[VFD] Releasing the registry path buffer\n"));
			ExFreePool(driver_extension->RegistryPath.Buffer);
		}

#ifndef VFD_PNP
		//	The system takes care of freeing the driver extension
		//	allocated with IoAllocateDriverObjectExtension in a PnP driver.
		VFDTRACE(0, ("[VFD] Releasing the driver extension\n"));
		ExFreePool(driver_extension);
#endif	// VFD_PNP
	}

	VFDTRACE(VFDINFO, ("[VFD] VfdUnloadDriver - OUT\n"));
}

//
//	IRP_MJ_CREATE and IRP_MJ_CLOSE handler
//	Really nothing to do here...
//
NTSTATUS
NTAPI
VfdCreateClose (
	IN PDEVICE_OBJECT			DeviceObject,
	IN PIRP						Irp)
{
#if DBG
	if (DeviceObject && DeviceObject->DeviceExtension &&
		((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->DeviceName.Buffer) {

		VFDTRACE(VFDINFO, ("[VFD] %-40s %ws\n",
			GetMajorFuncName(IoGetCurrentIrpStackLocation(Irp)->MajorFunction),
			((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->DeviceName.Buffer));
	}
	else {
		VFDTRACE(VFDINFO, ("[VFD] %-40s %p\n",
			GetMajorFuncName(IoGetCurrentIrpStackLocation(Irp)->MajorFunction),
			DeviceObject));
	}
#endif	// DBG

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = FILE_OPENED;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

//
//	Called after the DriverEntry routine has returned
//	(Re)Create a persistent drive letter for each device
//
VOID
NTAPI
VfdReinitialize(
	IN PDRIVER_OBJECT			DriverObject,
	IN PVOID					Context,
	IN ULONG					Count)
{
	PDEVICE_OBJECT				device_object;
	PDEVICE_EXTENSION			device_extension;

	UNREFERENCED_PARAMETER(Context);
	UNREFERENCED_PARAMETER(Count);

	VFDTRACE(VFDINFO, ("[VFD] VfdReinitialize - IN\n"));

	device_object = DriverObject->DeviceObject;

	while (device_object) {
		device_extension = (PDEVICE_EXTENSION)device_object->DeviceExtension;

#ifdef VFD_MOUNT_MANAGER
		if (OsMajorVersion >= 5) {
			//	Windows 2000 / XP
			//	Notify the mount manager of a VFD volume arrival
			VfdMountMgrNotifyVolume(device_extension);

			if (device_extension->DriveLetter) {
				//	Create a drive letter via the mount manager.
				//	The mount manager may have created a drive letter
				//	in response to the volume arrival notification above.
				//	In that case, the following call just fails.
				VfdMountMgrMountPoint(
					device_extension, device_extension->DriveLetter);
				//	ignoring the error for it is not fatal here
			}
		}
		else
#endif	// VFD_MOUNT_MANAGER
		{
			//	Windows NT style drive letter assignment
			//	Simply create a symbolic link here
			if (device_extension->DriveLetter) {
				VfdSetLink(
					device_extension, device_extension->DriveLetter);
				//	ignoring the error for it is not fatal here
			}
		}

		device_object = device_object->NextDevice;
	}

	VFDTRACE(VFDINFO, ("[VFD] VfdReinitialize - OUT\n"));
}

//
//	Device dedicated thread routine
//	Dispatch read, write and device I/O request
//	redirected from the driver dispatch routines
//
VOID
NTAPI
VfdDeviceThread (
	IN PVOID					ThreadContext)
{
	PDEVICE_OBJECT				device_object;
	PDEVICE_EXTENSION			device_extension;
	PLIST_ENTRY					request;
	PIRP						irp;
	PIO_STACK_LOCATION			io_stack;

	ASSERT(ThreadContext != NULL);

	device_object = (PDEVICE_OBJECT)ThreadContext;

	device_extension = (PDEVICE_EXTENSION)device_object->DeviceExtension;

	KeSetPriorityThread(KeGetCurrentThread(), LOW_REALTIME_PRIORITY);

	for (;;) {
		//	wait for the request event to be signalled
		KeWaitForSingleObject(
			&device_extension->RequestEvent,
			Executive,
			KernelMode,
			FALSE,
			NULL);

		//	terminate request ?
		if (device_extension->TerminateThread) {
			VFDTRACE(0, ("[VFD] Exitting the I/O thread\n"));
			PsTerminateSystemThread(STATUS_SUCCESS);
		}

		//	perform requested tasks

		while ((request = ExInterlockedRemoveHeadList(
			&device_extension->ListHead,
			&device_extension->ListLock)) != NULL)
		{
			irp = CONTAINING_RECORD(request, IRP, Tail.Overlay.ListEntry);

			io_stack = IoGetCurrentIrpStackLocation(irp);

			irp->IoStatus.Information = 0;

			switch (io_stack->MajorFunction) {
			case IRP_MJ_READ:
				VfdReadData(device_extension, irp,
					io_stack->Parameters.Read.Length,
					&io_stack->Parameters.Read.ByteOffset);
				break;

			case IRP_MJ_WRITE:
				VfdWriteData(device_extension, irp,
					io_stack->Parameters.Write.Length,
					&io_stack->Parameters.Write.ByteOffset);
				break;

			case IRP_MJ_DEVICE_CONTROL:
				VfdIoCtlThread(device_extension, irp,
					io_stack->Parameters.DeviceIoControl.IoControlCode);
				break;

			default:
				//	This shouldn't happen...
				VFDTRACE(0,
					("[VFD] %s passed to the I/O thread\n",
					GetMajorFuncName(io_stack->MajorFunction)));

				irp->IoStatus.Status = STATUS_DRIVER_INTERNAL_ERROR;
			}

			IoCompleteRequest(irp,
				(CCHAR)(NT_SUCCESS(irp->IoStatus.Status) ?
					IO_DISK_INCREMENT : IO_NO_INCREMENT));

#ifdef VFD_PNP
			IoReleaseRemoveLock(&device_extension->RemoveLock, irp);
#endif	// VFD_PNP
		} // while
	} // for (;;)
}

//
//	Copy a UNICODE_STRING adding a trailing NULL characer
//
PWSTR VfdCopyUnicode(
	PUNICODE_STRING				dst,
	PUNICODE_STRING				src)
{
	RtlZeroMemory(dst, sizeof(UNICODE_STRING));

	dst->MaximumLength =
		(USHORT)(src->MaximumLength + sizeof(UNICODE_NULL));

	dst->Buffer = (PWSTR)ExAllocatePoolWithTag(
		PagedPool, dst->MaximumLength, VFD_POOL_TAG);

	if(dst->Buffer) {
		dst->Length = src->Length;
		RtlZeroMemory(dst->Buffer, dst->MaximumLength);

		if (src->Length) {
			RtlCopyMemory(dst->Buffer, src->Buffer, src->Length);
		}
	}

	return dst->Buffer;
}

//
//	Free a UNICODE_STRING buffer
//
VOID VfdFreeUnicode(
	PUNICODE_STRING				str)
{
	if (str->Buffer) {
		ExFreePool(str->Buffer);
	}
	RtlZeroMemory(str, sizeof(UNICODE_STRING));
}
