/*
	vfdfmt.c

	Virtual Floppy Drive for Windows NT platform
	Kernel mode driver: disk format functions

	Copyright (C) 2003-2005 Ken Kato
*/

#include "imports.h"
#include "vfddrv.h"
#include "vfddbg.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VfdFormatCheck)
#pragma alloc_text(PAGE, VfdFormatTrack)
#endif	// ALLOC_PRAGMA

//
//	Media geometry constant table
//
//	MediaTypes values added since Win NT DDK
#ifndef F3_640_512
#define F3_640_512				(MEDIA_TYPE)14
#endif
#ifndef F3_1Pt2_512
#define F3_1Pt2_512				(MEDIA_TYPE)17
#endif

#ifndef __REACTOS__
extern DISK_GEOMETRY const geom_tbl[VFD_MEDIA_MAX] = {
	{{ 80 },	F3_1Pt44_512,	2,	18,	VFD_BYTES_PER_SECTOR },	// default
	{{ 40 },	F5_160_512,		1,	8,	VFD_BYTES_PER_SECTOR },	// 160K
	{{ 40 },	F5_180_512,		1,	9,	VFD_BYTES_PER_SECTOR },	// 180K
	{{ 40 },	F5_320_512,		2,	8,	VFD_BYTES_PER_SECTOR },	// 320K
	{{ 40 },	F5_360_512,		2,	9,	VFD_BYTES_PER_SECTOR },	// 360K
	{{ 80 },	F3_640_512,		2,	8,	VFD_BYTES_PER_SECTOR },	// 640k
	{{ 80 },	F5_640_512,		2,	8,	VFD_BYTES_PER_SECTOR },	// 640k
	{{ 80 },	F3_720_512,		2,	9,	VFD_BYTES_PER_SECTOR },	// 720K
	{{ 80 },	F5_720_512,		2,	9,	VFD_BYTES_PER_SECTOR },	// 720K
	{{ 82 },	RemovableMedia,	2,	10,	VFD_BYTES_PER_SECTOR },	// 820K
	{{ 80 },	F3_1Pt2_512,	2,	15,	VFD_BYTES_PER_SECTOR },	// 1200K
	{{ 80 },	F5_1Pt2_512,	2,	15,	VFD_BYTES_PER_SECTOR },	// 1200K
	{{ 80 },	F3_1Pt44_512,	2,	18,	VFD_BYTES_PER_SECTOR },	// 1440K
	{{ 80 },	RemovableMedia,	2,	21,	VFD_BYTES_PER_SECTOR },	// 1680K DMF
	{{ 82 },	RemovableMedia,	2,	21,	VFD_BYTES_PER_SECTOR },	// 1720K DMF
	{{ 80 },	F3_2Pt88_512,	2,	36,	VFD_BYTES_PER_SECTOR },	// 2880K
#else
DISK_GEOMETRY const geom_tbl[VFD_MEDIA_MAX] = {
	{{ {80} },	F3_1Pt44_512,	2,	18,	VFD_BYTES_PER_SECTOR },	// default
	{{ {40} },	F5_160_512,		1,	8,	VFD_BYTES_PER_SECTOR },	// 160K
	{{ {40} },	F5_180_512,		1,	9,	VFD_BYTES_PER_SECTOR },	// 180K
	{{ {40} },	F5_320_512,		2,	8,	VFD_BYTES_PER_SECTOR },	// 320K
	{{ {40} },	F5_360_512,		2,	9,	VFD_BYTES_PER_SECTOR },	// 360K
	{{ {80} },	F3_640_512,		2,	8,	VFD_BYTES_PER_SECTOR },	// 640k
	{{ {80} },	F5_640_512,		2,	8,	VFD_BYTES_PER_SECTOR },	// 640k
	{{ {80} },	F3_720_512,		2,	9,	VFD_BYTES_PER_SECTOR },	// 720K
	{{ {80} },	F5_720_512,		2,	9,	VFD_BYTES_PER_SECTOR },	// 720K
	{{ {82} },	RemovableMedia,	2,	10,	VFD_BYTES_PER_SECTOR },	// 820K
	{{ {80} },	F3_1Pt2_512,	2,	15,	VFD_BYTES_PER_SECTOR },	// 1200K
	{{ {80} },	F5_1Pt2_512,	2,	15,	VFD_BYTES_PER_SECTOR },	// 1200K
	{{ {80} },	F3_1Pt44_512,	2,	18,	VFD_BYTES_PER_SECTOR },	// 1440K
	{{ {80} },	RemovableMedia,	2,	21,	VFD_BYTES_PER_SECTOR },	// 1680K DMF
	{{ {82} },	RemovableMedia,	2,	21,	VFD_BYTES_PER_SECTOR },	// 1720K DMF
	{{ {80} },	F3_2Pt88_512,	2,	36,	VFD_BYTES_PER_SECTOR },	// 2880K
#endif
};

//
//	Parameter check for IOCTL_DISK_FORMAT_TRACK and IOCTL_DISK_FORMAT_TRACK_EX
//
NTSTATUS
VfdFormatCheck(
	PDEVICE_EXTENSION			DeviceExtension,
	PFORMAT_PARAMETERS			FormatParams,
	ULONG						InputLength,
	ULONG						ControlCode)
{
	const DISK_GEOMETRY			*geometry;

	//	Check media status

	if (!DeviceExtension->FileHandle &&
		!DeviceExtension->FileBuffer) {
		return STATUS_NO_MEDIA_IN_DEVICE;
	}

	//	Media is writable?

	if (DeviceExtension->MediaFlags & VFD_FLAG_WRITE_PROTECTED) {
		return STATUS_MEDIA_WRITE_PROTECTED;
	}

	//	Check input parameter size

	if (InputLength < sizeof(FORMAT_PARAMETERS)) {
		return STATUS_INVALID_PARAMETER;
	}

	//	Choose appropriate DISK_GEOMETRY for current image size

	geometry = DeviceExtension->Geometry;

	if (!geometry) {
		return STATUS_DRIVER_INTERNAL_ERROR;
	}

	//	Input parameter sanity check

	if ((FormatParams->StartHeadNumber		> geometry->TracksPerCylinder - 1)	||
		(FormatParams->EndHeadNumber		> geometry->TracksPerCylinder - 1)	||
		(FormatParams->StartCylinderNumber	> geometry->Cylinders.LowPart)		||
		(FormatParams->EndCylinderNumber	> geometry->Cylinders.LowPart)		||
		(FormatParams->EndCylinderNumber	< FormatParams->StartCylinderNumber))
	{
		return STATUS_INVALID_PARAMETER;
	}

	//	If this is an EX request then make a couple of extra checks

	if (ControlCode == IOCTL_DISK_FORMAT_TRACKS_EX) {

		PFORMAT_EX_PARAMETERS exparams;

		if (InputLength < sizeof(FORMAT_EX_PARAMETERS)) {
			return STATUS_INVALID_PARAMETER;
		}

		exparams = (PFORMAT_EX_PARAMETERS)FormatParams;

		if (InputLength <
			FIELD_OFFSET(FORMAT_EX_PARAMETERS, SectorNumber) +
			exparams->SectorsPerTrack * sizeof(USHORT))
		{
			return STATUS_INVALID_PARAMETER;
		}

		if (exparams->FormatGapLength > geometry->SectorsPerTrack ||
			exparams->SectorsPerTrack != geometry->SectorsPerTrack)
		{
			return STATUS_INVALID_PARAMETER;
		}
	}

	return STATUS_SUCCESS;
}

//
//	Format tracks
//	Actually, just fills specified range of tracks with fill characters
//
NTSTATUS
VfdFormatTrack(
	IN PDEVICE_EXTENSION		DeviceExtension,
	IN PFORMAT_PARAMETERS		FormatParams)
{
	const DISK_GEOMETRY			*geometry;
	ULONG						track_length;
	PUCHAR						format_buffer;
	LARGE_INTEGER				start_offset;
	LARGE_INTEGER				end_offset;
	NTSTATUS					status;

	VFDTRACE(0, ("[VFD] VfdFormatTrack - IN\n"));

	ASSERT(DeviceExtension != NULL);

	geometry = DeviceExtension->Geometry;

	if (!geometry) {
		return STATUS_DRIVER_INTERNAL_ERROR;
	}

	track_length = geometry->BytesPerSector * geometry->SectorsPerTrack;

	format_buffer = (PUCHAR)ExAllocatePoolWithTag(
		PagedPool, track_length, VFD_POOL_TAG);

	if (format_buffer == NULL) {
		VFDTRACE(0, ("[VFD] cannot allocate a format buffer\n"));
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlFillMemory(format_buffer, track_length, VFD_FORMAT_FILL_DATA);

	start_offset.QuadPart =
		FormatParams->StartCylinderNumber * geometry->TracksPerCylinder * track_length +
		FormatParams->StartHeadNumber * track_length;

	end_offset.QuadPart =
		FormatParams->EndCylinderNumber * geometry->TracksPerCylinder * track_length +
		FormatParams->EndHeadNumber * track_length;

	do {
		if (DeviceExtension->FileHandle) {
			IO_STATUS_BLOCK io_status;

			status = ZwWriteFile(
				DeviceExtension->FileHandle,
				NULL,
				NULL,
				NULL,
				&io_status,
				format_buffer,
				track_length,
				&start_offset,
				NULL);

			if (!NT_SUCCESS(status)) {
				VFDTRACE(0, ("[VFD] ZwWriteFile - %s\n",
					GetStatusName(status)));
				break;
			}
		}
		else {
			RtlMoveMemory(
				DeviceExtension->FileBuffer + start_offset.QuadPart,
				format_buffer,
				track_length);

			status = STATUS_SUCCESS;
		}

		start_offset.QuadPart += track_length;
	}
	while (start_offset.QuadPart <= end_offset.QuadPart);

	ExFreePool(format_buffer);

	VFDTRACE(0, ("[VFD] VfdFormatTrack - OUT\n"));

	return status;
}
