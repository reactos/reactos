/*
	vfdlink.c

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver: persistent drive letter functions

	Copyright (C) 2003-2005 Ken Kato
*/

#include "imports.h"
#include "vfddrv.h"
#include "vfddbg.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VfdSetLink)
#pragma alloc_text(PAGE, VfdLoadLink)
#pragma alloc_text(PAGE, VfdStoreLink)
#endif	// ALLOC_PRAGMA

//
//	create or remove the persistent drive letter (Windows NT)
//
NTSTATUS
VfdSetLink(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	CHAR					DriveLetter)
{
	UNICODE_STRING				unicode_name;
	WCHAR						name_buf[15];
	NTSTATUS					status = STATUS_SUCCESS;

	VFDTRACE(VFDINFO, ("[VFD] VfdSetLink - IN\n"));

	//	convert lower case into upper case

	if (DriveLetter >= 'a' && DriveLetter <= 'z') {
		DriveLetter -= ('a' - 'A');
	}

	//	check the drive letter range

	if (DriveLetter != 0 &&
		(DriveLetter < 'A' || DriveLetter > 'Z')) {
		return STATUS_INVALID_PARAMETER;
	}

	if (DeviceExtension->DriveLetter &&
		DeviceExtension->DriveLetter != DriveLetter) {
		//
		//	Delete the old drive letter
		//
#ifndef __REACTOS__
		name_buf[sizeof(name_buf) - 1] = UNICODE_NULL;
#else
		name_buf[ARRAYSIZE(name_buf) - 1] = UNICODE_NULL;
#endif

		_snwprintf(name_buf, sizeof(name_buf) - 1,
			L"\\??\\%wc:", DeviceExtension->DriveLetter);

		RtlInitUnicodeString(&unicode_name, name_buf);

		status = IoDeleteSymbolicLink(&unicode_name);

		if (NT_SUCCESS(status)) {
			VFDTRACE(VFDINFO,
				("[VFD] Link %ws deleted\n", name_buf));

			DeviceExtension->DriveLetter = 0;
		}
		else if (status != STATUS_OBJECT_NAME_NOT_FOUND) {
			//	the driver letter did not exist in the first place

			VFDTRACE(VFDINFO,
				("[VFD] Link %ws not found\n", name_buf));

			DeviceExtension->DriveLetter = 0;
			status = STATUS_SUCCESS;
		}
		else {
			VFDTRACE(VFDWARN,
				("[VFD] IoDeleteSymbolicLink %ws - %s\n",
				name_buf, GetStatusName(status)));
		}
	}

	if (NT_SUCCESS(status) && DriveLetter) {
		//
		//	Create a new drive letter
		//

#ifndef __REACTOS__
		name_buf[sizeof(name_buf) - 1] = UNICODE_NULL;

		_snwprintf(name_buf, sizeof(name_buf) - 1,
			(OsMajorVersion >= 5) ?
			L"\\??\\Global\\%wc:" : L"\\??\\%wc:",
			DriveLetter);
#else
		name_buf[ARRAYSIZE(name_buf) - 1] = UNICODE_NULL;

		_snwprintf(name_buf, ARRAYSIZE(name_buf) - 1,
			(OsMajorVersion >= 5) ?
			L"\\??\\Global\\%wc:" : L"\\??\\%wc:",
			DriveLetter);
#endif

		RtlInitUnicodeString(&unicode_name, name_buf);

		status = IoCreateSymbolicLink(
			&unicode_name, &(DeviceExtension->DeviceName));

		if (NT_SUCCESS(status)) {
			VFDTRACE(VFDINFO, ("[VFD] Link %ws created\n", name_buf));

			DeviceExtension->DriveLetter = DriveLetter;
		}
		else {
			VFDTRACE(VFDWARN,
				("[VFD] IoCreateSymbolicLink %ws - %s\n",
				name_buf, GetStatusName(status)));
		}
	}

	VFDTRACE(VFDINFO,
		("[VFD] VfdSetLink - %s\n", GetStatusName(status)));

	return status;
}

//
//	load the persistent drive letter from the registry
//
NTSTATUS
VfdLoadLink(
	IN	PDEVICE_EXTENSION		DeviceExtension,
	IN	PWSTR					RegistryPath)
{
	RTL_QUERY_REGISTRY_TABLE	params[2];
	WCHAR						name_buf[20];
	ULONG						letter;
	ULONG						zero	= 0;
	NTSTATUS					status;

	VFDTRACE(VFDINFO, ("[VFD] VfdLoadLink - IN\n"));

	RtlZeroMemory(params, sizeof(params));

#ifndef __REACTOS__
	name_buf[sizeof(name_buf) - 1] = UNICODE_NULL;

	_snwprintf(name_buf, sizeof(name_buf) - 1,
		VFD_REG_DRIVE_LETTER L"%lu",
		DeviceExtension->DeviceNumber);
#else
	name_buf[ARRAYSIZE(name_buf) - 1] = UNICODE_NULL;

	_snwprintf(name_buf, ARRAYSIZE(name_buf) - 1,
		VFD_REG_DRIVE_LETTER L"%lu",
		DeviceExtension->DeviceNumber);
#endif

	params[0].Flags			= RTL_QUERY_REGISTRY_DIRECT;
	params[0].Name			= name_buf;
	params[0].EntryContext	= &letter;
	params[0].DefaultType	= REG_DWORD;
	params[0].DefaultData	= &zero;
	params[0].DefaultLength	= sizeof(ULONG);

	status = RtlQueryRegistryValues(
		RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
		RegistryPath, &params[0], NULL, NULL);

	VFDTRACE(VFDINFO,
		("[VFD] Drive letter '%wc' loaded from the registry\n",
		letter ? letter : ' '));

	DeviceExtension->DriveLetter = (CHAR)letter;

	VFDTRACE(VFDINFO,
		("[VFD] VfdLoadLink - %s\n", GetStatusName(status)));

	return status;
}

//
//	store the persistent drive letter into the registry
//
NTSTATUS
VfdStoreLink(
	IN	PDEVICE_EXTENSION		DeviceExtension)
{
	PVFD_DRIVER_EXTENSION		driver_extension;
	WCHAR						name_buf[20];
	ULONG						letter;
	NTSTATUS					status;

	VFDTRACE(VFDINFO, ("[VFD] VfdStoreLink - IN\n"));

#ifdef VFD_PNP
	driver_extension = IoGetDriverObjectExtension(
		DeviceExtension->device_object->DriverObject,
		VFD_DRIVER_EXTENSION_ID);
#else	// VFD_PNP
	driver_extension = DeviceExtension->DriverExtension;
#endif	// VFD_PNP

	if (!driver_extension ||
		!driver_extension->RegistryPath.Buffer) {

		VFDTRACE(VFDWARN, ("[VFD] Registry Path not present.\n"));
		VFDTRACE(VFDINFO, ("[VFD] VfdStoreLinks - OUT\n"));
		return STATUS_DRIVER_INTERNAL_ERROR;
	}

#ifndef __REACTOS__
	name_buf[sizeof(name_buf) - 1] = UNICODE_NULL;

	_snwprintf(name_buf, sizeof(name_buf) - 1,
		VFD_REG_DRIVE_LETTER L"%lu",
		DeviceExtension->DeviceNumber);
#else
	name_buf[ARRAYSIZE(name_buf) - 1] = UNICODE_NULL;

	_snwprintf(name_buf, ARRAYSIZE(name_buf) - 1,
		VFD_REG_DRIVE_LETTER L"%lu",
		DeviceExtension->DeviceNumber);
#endif

	letter = DeviceExtension->DriveLetter;

	status = RtlWriteRegistryValue(
		RTL_REGISTRY_ABSOLUTE,
		driver_extension->RegistryPath.Buffer,
		name_buf,
		REG_DWORD,
		&letter,
		sizeof(ULONG));

	if (!NT_SUCCESS(status)) {
		VFDTRACE(VFDWARN,
			("[VFD] RtlWriteRegistryValue - %s\n",
			GetStatusName(status)));
	}
	else {
		VFDTRACE(VFDINFO,
			("[VFD] Drive letter '%wc' stored into the registry\n",
			letter ? letter : L' '));
	}

	VFDTRACE(VFDINFO,
		("[VFD] VfdStoreLink - %s\n", GetStatusName(status)));

	return status;
}
