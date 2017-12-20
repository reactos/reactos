/*
	vfdrdwr.c

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver: Read and Write functions

	Copyright (C) 2003-2005 Ken Kato
*/

#include "imports.h"
#include "vfddrv.h"
#include "vfddbg.h"

//
//	IRP_MJ_READ and IRP_MJ_WRITE dispatcher
//	Insert the IRP into the IRP queue list.
//	Actual operation is performed by the device thread
//
#define IO_READ_OFF(p)	(p)->Parameters.Read.ByteOffset.QuadPart
#define IO_READ_LEN(p)	(p)->Parameters.Read.Length

NTSTATUS
NTAPI
VfdReadWrite (
	IN PDEVICE_OBJECT			DeviceObject,
	IN PIRP						Irp)
{
	PDEVICE_EXTENSION			device_extension;
	PIO_STACK_LOCATION			io_stack;
	NTSTATUS					status;

	device_extension = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

	io_stack = IoGetCurrentIrpStackLocation(Irp);

#if DBG
	if (DeviceObject && DeviceObject->DeviceExtension &&
		((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->DeviceName.Buffer) {

		VFDTRACE(VFDINFO, ("[VFD] %-40s %ws\n",
			GetMajorFuncName(io_stack->MajorFunction),
			((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->DeviceName.Buffer));
	}
	else {
		VFDTRACE(VFDINFO, ("[VFD] %-40s %p\n",
			GetMajorFuncName(io_stack->MajorFunction),
			DeviceObject));
	}
#endif	// DBG

#ifdef VFD_PNP

	if (device_extension->DeviceState != VFD_WORKING) {

		// Device is not yet started or being removed, reject any IO request
		// TODO: Queue the IRPs

		VFDTRACE(VFDWARN, ("[VFD] Device not ready\n"));

		status = STATUS_INVALID_DEVICE_STATE;
		goto complete_request;
	}
	else {
		status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);

		if (!NT_SUCCESS(status)) {
			VFDTRACE(0, ("[VFD] Acquire RemoveLock failed: %s\n", GetStatusName(status)));

			goto complete_request;
		}
	}
#endif	// VFD_PNP

/*
	//	Check if volume verification is required

	if ((DeviceObject->Flags & DO_VERIFY_VOLUME) &&
		!(io_stack->Flags & SL_OVERRIDE_VERIFY_VOLUME)) {

		status = STATUS_VERIFY_REQUIRED;
		goto complete_request;
	}
*/

	//	Check if an image is opened

	if (!device_extension->FileHandle &&
		!device_extension->FileBuffer)	{

		status = STATUS_NO_MEDIA_IN_DEVICE;
		goto complete_request;
	}


	// Check if write operation is allowed

	if (io_stack->MajorFunction == IRP_MJ_WRITE &&
		(device_extension->MediaFlags & VFD_FLAG_WRITE_PROTECTED)) {

		status = STATUS_MEDIA_WRITE_PROTECTED;
		goto complete_request;
	}


	// Check for invalid parameters.  It is an error for the starting offset
	// + length to go past the end of the partition, or for the length or
	// offset to not be a proper multiple of the sector size.
	//
	// Others are possible, but we don't check them since we trust the
	// file system and they aren't deadly.

	if ((IO_READ_OFF(io_stack) + IO_READ_LEN(io_stack)) >
		VFD_SECTOR_TO_BYTE(device_extension->Sectors)) {

		VFDTRACE(VFDWARN,
			("[VFD] Offset:%I64u + Length:%u goes past the media size %lu\n",
			IO_READ_OFF(io_stack), IO_READ_LEN(io_stack),
			VFD_SECTOR_TO_BYTE(device_extension->Sectors)));

		status = STATUS_INVALID_PARAMETER;
		goto complete_request;
	}

	if (!VFD_SECTOR_ALIGNED((IO_READ_LEN(io_stack))) ||
		!VFD_SECTOR_ALIGNED((IO_READ_OFF(io_stack)))) {

		VFDTRACE(VFDWARN,
			("[VFD] Invalid Alignment Offset:%I64u Length:%u\n",
			IO_READ_OFF(io_stack), IO_READ_LEN(io_stack)));

		status = STATUS_INVALID_PARAMETER;
		goto complete_request;
	}

	//	If read/write data length is 0, we are done

	if (IO_READ_LEN(io_stack) == 0) {
		status = STATUS_SUCCESS;
		goto complete_request;
	}

	//	It seems that actual read/write operation is going to take place
	//	so mark the IRP as pending, insert the IRP into queue list
	//	then signal the device thread to perform the operation

	IoMarkIrpPending(Irp);

	ExInterlockedInsertTailList(
		&device_extension->ListHead,
		&Irp->Tail.Overlay.ListEntry,
		&device_extension->ListLock);

	KeSetEvent(
		&device_extension->RequestEvent,
		(KPRIORITY) 0,
		FALSE);

	VFDTRACE(VFDINFO,("[VFD] %-40s - STATUS_PENDING\n",
		GetMajorFuncName(io_stack->MajorFunction)));

	return STATUS_PENDING;

complete_request:

	//	complete the request immediately

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	VFDTRACE(VFDWARN,("[VFD] %-40s - %s\n",
		GetMajorFuncName(io_stack->MajorFunction),
		GetStatusName(status)));

	return status;
}

//
//	Substitute for MmGetSystemAddressForMdlSafe
//	for NT 4.0 DDK does not provide its equivqlent
//	originally written by Bruce Engle for filedisk
//
static PVOID
MmGetSystemAddressForMdlPrettySafe(
	IN PMDL						Mdl,
	IN MM_PAGE_PRIORITY			Priority)
{
#if (VER_PRODUCTBUILD >= 2195)
	if (OsMajorVersion >= 5) {
		return MmGetSystemAddressForMdlSafe(Mdl, Priority);
	}
	else {
#endif	// (VER_PRODUCTBUILD >= 2195)
		CSHORT	MdlMappingCanFail;
		PVOID	MappedSystemVa;

		MdlMappingCanFail = (CSHORT)(Mdl->MdlFlags & MDL_MAPPING_CAN_FAIL);

		Mdl->MdlFlags |= MDL_MAPPING_CAN_FAIL;

		MappedSystemVa = MmGetSystemAddressForMdl(Mdl);

		if (!MdlMappingCanFail) {
			Mdl->MdlFlags &= ~MDL_MAPPING_CAN_FAIL;
		}

		return MappedSystemVa;
#if (VER_PRODUCTBUILD >= 2195)
	}
#endif	// (VER_PRODUCTBUILD >= 2195)
}

//
//	Read sectors from image file or RAM disk buffer into read buffer
//
VOID
VfdReadData(
	IN		PDEVICE_EXTENSION	DeviceExtension,
	IN OUT	PIRP				Irp,
	IN		ULONG				Length,
	IN		PLARGE_INTEGER		Offset)
{
	PVOID buf;

	VFDTRACE(VFDINFO,("[VFD] VfdReadData - IN\n"));

	buf = MmGetSystemAddressForMdlPrettySafe(
		Irp->MdlAddress, NormalPagePriority);

	if (!buf) {
		VFDTRACE(0,
			("[VFD] MmGetSystemAddressForMdlPrettySafe\n"));

		Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		return;
	}

	if (DeviceExtension->FileHandle) {

		//	Read from image file
		Irp->IoStatus.Status = ZwReadFile(
			DeviceExtension->FileHandle,
			NULL,
			NULL,
			NULL,
			&Irp->IoStatus,
			buf,
			Length,
			Offset,
			NULL);

		if (NT_SUCCESS(Irp->IoStatus.Status)) {
			Irp->IoStatus.Information = Length;
		}
		else {
			VFDTRACE(0,
				("[VFD] ZwReadFile - %s\n",
				GetStatusName(Irp->IoStatus.Status)));
		}
	}
	else if (DeviceExtension->FileBuffer) {

		//	Copy from RAM disk buffer
		RtlMoveMemory(
			buf,
			DeviceExtension->FileBuffer + Offset->QuadPart,
			Length);

		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = Length;
	}
	else {
		//	no image opened
		Irp->IoStatus.Status = STATUS_NO_MEDIA_IN_DEVICE;
	}

	VFDTRACE(VFDINFO,("[VFD] VfdReadData - %s\n",
		GetStatusName(Irp->IoStatus.Status)));

	return;
}

//
//	Write sectors from write buffer into image file or RAM image buffer
//
VOID
VfdWriteData(
	IN		PDEVICE_EXTENSION	DeviceExtension,
	IN OUT	PIRP				Irp,
	IN		ULONG				Length,
	IN		PLARGE_INTEGER		Offset)
{
	PVOID buf;

	VFDTRACE(VFDINFO,("[VFD] VfdWriteData - IN\n"));

	buf = MmGetSystemAddressForMdlPrettySafe(
		Irp->MdlAddress, NormalPagePriority);

	if (!buf) {
		VFDTRACE(0,
			("[VFD] MmGetSystemAddressForMdlPrettySafe\n"));

		Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
		return;
	}

	if (DeviceExtension->FileHandle) {

		//	Write into image file
		Irp->IoStatus.Status = ZwWriteFile(
			DeviceExtension->FileHandle,
			NULL,
			NULL,
			NULL,
			&Irp->IoStatus,
			buf,
			Length,
			Offset,
			NULL);

		if (NT_SUCCESS(Irp->IoStatus.Status)) {
			Irp->IoStatus.Information = Length;
		}
		else {
			VFDTRACE(0,
				("[VFD] ZwWriteFile - %s\n",
				GetStatusName(Irp->IoStatus.Status)));
		}
	}
	else if (DeviceExtension->FileBuffer) {

		//	Deal with the modify flag
		if (RtlCompareMemory(
			DeviceExtension->FileBuffer + Offset->QuadPart,
			buf, Length) != Length) {
			DeviceExtension->MediaFlags |= VFD_FLAG_DATA_MODIFIED;
		}

		//	Copy into RAM image buffer
		RtlMoveMemory(
			DeviceExtension->FileBuffer + Offset->QuadPart,
			buf, Length);

		Irp->IoStatus.Status = STATUS_SUCCESS;
		Irp->IoStatus.Information = Length;
	}
	else {
		//	no image opened
		Irp->IoStatus.Status = STATUS_NO_MEDIA_IN_DEVICE;
	}

	VFDTRACE(VFDINFO,("[VFD] VfdWriteData - %s\n",
		GetStatusName(Irp->IoStatus.Status)));

	return;
}
