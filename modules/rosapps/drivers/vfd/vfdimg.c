/*
	vfdimg.c

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver: Image handling functions

	Copyright (C) 2003-2005 Ken Kato
*/

#include "imports.h"
#include "vfddrv.h"
#include "vfddbg.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VfdOpenCheck)
#pragma alloc_text(PAGE, VfdOpenImage)
#pragma alloc_text(PAGE, VfdCloseImage)
#pragma alloc_text(PAGE, VfdQueryImage)
#endif	// ALLOC_PRAGMA

//
//	Check IOCTL_VFD_OPEN_IMAGE input parameters
//
NTSTATUS
VfdOpenCheck(
	PDEVICE_EXTENSION			DeviceExtension,
	PVFD_IMAGE_INFO				ImageInfo,
	ULONG						InputLength)
{
	//	Check media status

	if (DeviceExtension->FileHandle ||
		DeviceExtension->FileBuffer) {

		VFDTRACE(VFDWARN, ("[VFD] image already opened.\n"));

		return STATUS_DEVICE_BUSY;
	}

	//	Check input parameter length

	if (InputLength < sizeof(VFD_IMAGE_INFO) ||
		InputLength < sizeof(VFD_IMAGE_INFO) + ImageInfo->NameLength)
	{
		return STATUS_INVALID_PARAMETER;
	}

	//	Check input parameters

	if (ImageInfo->MediaType == VFD_MEDIA_NONE ||
		ImageInfo->MediaType >= VFD_MEDIA_MAX) {

		VFDTRACE(VFDWARN, ("[VFD] invalid MediaType - %u.\n",
			ImageInfo->MediaType));

		return STATUS_INVALID_PARAMETER;
	}

	if (ImageInfo->DiskType == VFD_DISKTYPE_FILE &&
		ImageInfo->NameLength == 0) {

		VFDTRACE(VFDWARN,
			("[VFD] File name required for VFD_DISKTYPE_FILE.\n"));

		return STATUS_INVALID_PARAMETER;
	}

	//	create a security context to match the calling process' context
	//	the driver thread uses this context to impersonate the client
	//	to open the specified image file

//	if (ImageInfo->DiskType == VFD_DISKTYPE_FILE)
	{
		SECURITY_QUALITY_OF_SERVICE sqos;

		if (DeviceExtension->SecurityContext != NULL) {
			SeDeleteClientSecurity(DeviceExtension->SecurityContext);
		}
		else {
			DeviceExtension->SecurityContext =
				(PSECURITY_CLIENT_CONTEXT)ExAllocatePoolWithTag(
				NonPagedPool, sizeof(SECURITY_CLIENT_CONTEXT), VFD_POOL_TAG);
		}

		RtlZeroMemory(&sqos, sizeof(SECURITY_QUALITY_OF_SERVICE));

		sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
		sqos.ImpersonationLevel		= SecurityImpersonation;
		sqos.ContextTrackingMode	= SECURITY_STATIC_TRACKING;
		sqos.EffectiveOnly			= FALSE;

		SeCreateClientSecurity(
			PsGetCurrentThread(), &sqos, FALSE,
			DeviceExtension->SecurityContext);
	}

	return STATUS_SUCCESS;
}

//
//	Open a virtual floppy image file or create an empty ram disk
//
NTSTATUS
VfdOpenImage (
	IN PDEVICE_EXTENSION		DeviceExtension,
	IN PVFD_IMAGE_INFO			ImageInfo)
{
	IO_STATUS_BLOCK				io_status;
	NTSTATUS					status	= STATUS_SUCCESS;
	const DISK_GEOMETRY			*geometry;
	ULONG						sectors;
	ULONG						alignment;

	VFDTRACE(0, ("[VFD] VfdOpenImage - IN\n"));

	//
	//	Store file name in the device extension
	//
	if (ImageInfo->NameLength) {

		if (ImageInfo->NameLength + 1 >
			DeviceExtension->FileName.MaximumLength) {

			//	expand the filename buffer

			if (DeviceExtension->FileName.Buffer) {
				ExFreePool(DeviceExtension->FileName.Buffer);
				RtlZeroMemory(
					&DeviceExtension->FileName,
					sizeof(ANSI_STRING));
			}

			DeviceExtension->FileName.Buffer = (PCHAR)ExAllocatePoolWithTag(
				NonPagedPool, ImageInfo->NameLength + 1, VFD_POOL_TAG);

			if (!DeviceExtension->FileName.Buffer) {
				VFDTRACE(0, ("[VFD] Can't allocate memory for image path\n"));
				return STATUS_INSUFFICIENT_RESOURCES;
			}

			DeviceExtension->FileName.MaximumLength
				= (USHORT)(ImageInfo->NameLength + 1);

			RtlZeroMemory(
				DeviceExtension->FileName.Buffer,
				DeviceExtension->FileName.MaximumLength);
		}

		if (DeviceExtension->FileName.Buffer) {
			RtlCopyMemory(
				DeviceExtension->FileName.Buffer,
				ImageInfo->FileName,
				ImageInfo->NameLength);

			DeviceExtension->FileName.Buffer[ImageInfo->NameLength] = '\0';
		}
	}

	DeviceExtension->FileName.Length = ImageInfo->NameLength;

	//
	//	Get DISK_GEOMETRY and calculate the media capacity
	//	-- validity of the ImageInfo->MediaType value is assured in
	//	the VfdOpenCheck function
	//
	geometry = &geom_tbl[ImageInfo->MediaType];

	sectors =
		geometry->Cylinders.LowPart *
		geometry->TracksPerCylinder *
		geometry->SectorsPerTrack;

	if (ImageInfo->ImageSize != 0 &&
		ImageInfo->ImageSize < VFD_SECTOR_TO_BYTE(sectors)) {

			VFDTRACE(0, ("[VFD] Image is smaller than the media\n"));
			return STATUS_INVALID_PARAMETER;
	}

	//
	//	Prepare a virtual media according to the ImageInfo
	//
	if (ImageInfo->DiskType == VFD_DISKTYPE_FILE) {
		//
		//	open an existing image file
		//
		HANDLE						file_handle;
		OBJECT_ATTRIBUTES			attributes;
		UNICODE_STRING				unicode_name;
		FILE_STANDARD_INFORMATION	file_standard;
		FILE_BASIC_INFORMATION		file_basic;
		FILE_ALIGNMENT_INFORMATION	file_alignment;
		PFILE_OBJECT				file_object;
		BOOLEAN						network_drive;

		//	convert the filename into a unicode string

		status = RtlAnsiStringToUnicodeString(
			&unicode_name, &DeviceExtension->FileName, TRUE);

		if (!NT_SUCCESS(status)) {
			VFDTRACE(0, ("[VFD] Failed to convert filename to UNICODE\n"));
			return status;
		}

		VFDTRACE(VFDINFO,
			("[VFD] Opening %s\n", DeviceExtension->FileName.Buffer));

		//	prepare an object attribute to open

		InitializeObjectAttributes(
			&attributes,
			&unicode_name,
			OBJ_CASE_INSENSITIVE,
			NULL,
			NULL);

		//	open the target file

		status = ZwCreateFile(
			&file_handle,
			GENERIC_READ | GENERIC_WRITE,
			&attributes,
			&io_status,
			NULL,
			FILE_ATTRIBUTE_NORMAL,
			0,
			FILE_OPEN,
			FILE_NON_DIRECTORY_FILE |
			FILE_RANDOM_ACCESS |
			FILE_NO_INTERMEDIATE_BUFFERING |
			FILE_SYNCHRONOUS_IO_NONALERT,
			NULL,
			0);

		RtlFreeUnicodeString(&unicode_name);

		if (!NT_SUCCESS(status)) {
			VFDTRACE(0, ("[VFD] ZwCreateFile - %s\n",
				GetStatusName(status)));
			return status;
		}

		//	Check the file size

		status = ZwQueryInformationFile(
			file_handle,
			&io_status,
			&file_standard,
			sizeof(FILE_STANDARD_INFORMATION),
			FileStandardInformation);

		if (!NT_SUCCESS(status)) {
			VFDTRACE(0,
				("[VFD] ZwQueryInformationFile - FILE_STANDARD_INFORMATION\n"));

			ZwClose(file_handle);
			goto exit_func;
		}

		//	Actual file size can be larger than the media capacity

		if (file_standard.EndOfFile.QuadPart < VFD_SECTOR_TO_BYTE(sectors)) {

			VFDTRACE(0, ("[VFD] file is smaller than the media.\n"));

			status = STATUS_INVALID_PARAMETER;

			ZwClose(file_handle);
			goto exit_func;
		}

		DeviceExtension->ImageSize = file_standard.EndOfFile.LowPart;

		// Find out whether the file is on a local disk or a network drive

		network_drive = FALSE;

		status = ObReferenceObjectByHandle(
			file_handle,
			GENERIC_READ,
			NULL,
			KernelMode,
#ifndef __REACTOS__
			&file_object,
#else
			(PVOID *)&file_object,
#endif
			NULL);

		if (NT_SUCCESS(status)) {
			if (file_object && file_object->DeviceObject) {
				VFDTRACE(VFDINFO, ("[VFD] Device type is 0x%08x\n",
					file_object->DeviceObject->DeviceType));

				if (file_object->DeviceObject->DeviceType
					== FILE_DEVICE_NETWORK_FILE_SYSTEM) {
					network_drive = TRUE;
				}

				// how about these types ?
				// FILE_DEVICE_NETWORK
				// FILE_DEVICE_NETWORK_BROWSER
				// FILE_DEVICE_NETWORK_REDIRECTOR
			}
			else {
				VFDTRACE(VFDWARN, ("[VFD Cannot decide the device type\n"));
			}
			ObDereferenceObject(file_object);
		}
		else {
			VFDTRACE(0, ("[VFD] ObReferenceObjectByHandle - %s\n",
				GetStatusName(status)));
		}

		if (!network_drive) {
			// The NT cache manager can deadlock if a filesystem that is using
			// the cache manager is used in a virtual disk that stores its file
			// on a file systemthat is also using the cache manager, this is
			// why we open the file with FILE_NO_INTERMEDIATE_BUFFERING above,
			// however if the file is compressed or encrypted NT will not honor
			// this request and cache it anyway since it need to store the
			// decompressed/unencrypted data somewhere, therefor we put an
			// extra check here and don't alow disk images to be compressed/
			// encrypted.

			status = ZwQueryInformationFile(
				file_handle,
				&io_status,
				&file_basic,
				sizeof(FILE_BASIC_INFORMATION),
				FileBasicInformation);

			if (!NT_SUCCESS(status)) {
				VFDTRACE(0,
					("[VFD] ZwQueryInformationFile - FILE_BASIC_INFORMATION\n"));

				ZwClose(file_handle);
				goto exit_func;
			}

			if (file_basic.FileAttributes
				& (FILE_ATTRIBUTE_COMPRESSED | FILE_ATTRIBUTE_ENCRYPTED))
			{
				VFDTRACE(0,
					("[VFD] Image file is compressed and/or encrypted\n"));

				status =  STATUS_ACCESS_DENIED;

				ZwClose(file_handle);
				goto exit_func;
			}
		}

		//	Retrieve the file alignment requirement

		status = ZwQueryInformationFile(
			file_handle,
			&io_status,
			&file_alignment,
			sizeof(FILE_ALIGNMENT_INFORMATION),
			FileAlignmentInformation);

		if (!NT_SUCCESS(status)) {
			VFDTRACE(0,
				("[VFD] ZwQueryInformationFile - FILE_ALIGNMENT_INFORMATION\n"));

			ZwClose(file_handle);
			goto exit_func;
		}

		DeviceExtension->FileHandle = file_handle;

		alignment = file_alignment.AlignmentRequirement;

		VFDTRACE(0, ("[VFD] Opened an image file\n"));
	}
	else {
		//
		//	Create an empty RAM disk
		//
		DeviceExtension->FileBuffer = (PUCHAR)ExAllocatePoolWithTag(
			NonPagedPool,
			VFD_SECTOR_TO_BYTE(sectors),
			VFD_POOL_TAG);

		if (!DeviceExtension->FileBuffer) {
			VFDTRACE(0, ("[VFD] Can't allocate memory for RAM disk\n"));
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		RtlZeroMemory(
			DeviceExtension->FileBuffer,
			VFD_SECTOR_TO_BYTE(sectors));

		if (ImageInfo->ImageSize) {
			DeviceExtension->ImageSize = ImageInfo->ImageSize;
		}
		else {
			DeviceExtension->ImageSize = VFD_SECTOR_TO_BYTE(sectors);
		}

		alignment = FILE_WORD_ALIGNMENT;

		VFDTRACE(0, ("[VFD] Created an empty RAM disk\n"));
	}

	DeviceExtension->MediaChangeCount++;

	DeviceExtension->MediaType	= ImageInfo->MediaType;
	DeviceExtension->MediaFlags	= ImageInfo->MediaFlags;
	DeviceExtension->FileType	= ImageInfo->FileType;
	DeviceExtension->Geometry	= geometry;
	DeviceExtension->Sectors	= sectors;

	VFDTRACE(0, ("[VFD] Media:%d Flag:0x%02x Size:%lu Capacity:%lu\n",
		DeviceExtension->MediaType,
		DeviceExtension->MediaFlags,
		DeviceExtension->ImageSize,
		DeviceExtension->Sectors));

	DeviceExtension->DeviceObject->AlignmentRequirement
		= alignment;

exit_func:
	VFDTRACE(0, ("[VFD] VfdOpenImage - %s\n", GetStatusName(status)));

	return status;
}

//
//	Close the current image
//
VOID
VfdCloseImage (
	IN PDEVICE_EXTENSION		DeviceExtension)
{
	VFDTRACE(0, ("[VFD] VfdCloseImage - IN\n"));

	ASSERT(DeviceExtension);

	DeviceExtension->MediaType			= VFD_MEDIA_NONE;
	DeviceExtension->MediaFlags			= 0;
	DeviceExtension->FileType			= 0;
	DeviceExtension->ImageSize			= 0;
	DeviceExtension->FileName.Length	= 0;
	DeviceExtension->Sectors			= 0;

	if (DeviceExtension->FileHandle) {
		ZwClose(DeviceExtension->FileHandle);
		DeviceExtension->FileHandle = NULL;
	}

	if (DeviceExtension->FileBuffer) {
		ExFreePool(DeviceExtension->FileBuffer);
		DeviceExtension->FileBuffer = NULL;
	}

	VFDTRACE(0, ("[VFD] VfdCloseImage - OUT\n"));
}

//
//	Return information about the current image
//
NTSTATUS
VfdQueryImage(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	OUT	PVFD_IMAGE_INFO			ImageInfo,
	IN	ULONG					BufferLength,
#ifndef __REACTOS__
	OUT	PULONG					ReturnLength)
#else
	OUT	PSIZE_T					ReturnLength)
#endif
{
	//	Check output buffer length

	if (BufferLength < sizeof(VFD_IMAGE_INFO)) {
		return STATUS_BUFFER_TOO_SMALL;
	}

	RtlZeroMemory(ImageInfo, BufferLength);

	//	Store fixed length image information

	ImageInfo->MediaType	= DeviceExtension->MediaType;

	if (DeviceExtension->MediaType == VFD_MEDIA_NONE) {
		*ReturnLength = sizeof(VFD_IMAGE_INFO);
		return STATUS_SUCCESS;
	}

	if (DeviceExtension->FileBuffer) {
		ImageInfo->DiskType = VFD_DISKTYPE_RAM;
	}
	else {
		ImageInfo->DiskType = VFD_DISKTYPE_FILE;
	}

	ImageInfo->MediaFlags	= DeviceExtension->MediaFlags;
	ImageInfo->FileType		= DeviceExtension->FileType;
	ImageInfo->ImageSize	= DeviceExtension->ImageSize;

	ImageInfo->NameLength	= DeviceExtension->FileName.Length;

	//	output buffer is large enough to hold the file name?

	if (BufferLength < sizeof(VFD_IMAGE_INFO) +
		DeviceExtension->FileName.Length)
	{
		*ReturnLength = sizeof(VFD_IMAGE_INFO);
		return STATUS_BUFFER_OVERFLOW;
	}

	//	copy file name

	if (DeviceExtension->FileName.Length &&
		DeviceExtension->FileName.Buffer) {

		RtlCopyMemory(ImageInfo->FileName,
			DeviceExtension->FileName.Buffer,
			DeviceExtension->FileName.Length);
	}

	//	store the actually returned data length

	*ReturnLength = sizeof(VFD_IMAGE_INFO) +
		DeviceExtension->FileName.Length;

	return STATUS_SUCCESS;
}
