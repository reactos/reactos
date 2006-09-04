/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/format.c
 * PURPOSE:         Volume format
 *
 * PROGRAMMERS:     Emanuele Aliberti
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/* FMIFS.6 */
VOID NTAPI
Format(void)
{
}

/* FMIFS.7 */
VOID NTAPI
FormatEx(
	IN PWCHAR DriveRoot,
	IN FMIFS_MEDIA_FLAG MediaFlag,
	IN PWCHAR Format,
	IN PWCHAR Label,
	IN BOOLEAN QuickFormat,
	IN ULONG ClusterSize,
	IN PFMIFSCALLBACK Callback)
{
	UNICODE_STRING usDriveRoot;
	UNICODE_STRING usLabel;
	BOOLEAN Argument = FALSE;
	WCHAR VolumeName[MAX_PATH];
	CURDIR CurDir;

	if (_wcsnicmp(Format, L"FAT", 3) != 0)
	{
		/* Unknown file system */
		Callback(
			DONE, /* Command */
			0, /* DWORD Modifier */
			&Argument); /* Argument */
		return;
	}

	if (!GetVolumeNameForVolumeMountPointW(DriveRoot, VolumeName, MAX_PATH)
	 || !RtlDosPathNameToNtPathName_U(VolumeName, &usDriveRoot, NULL, &CurDir))
	{
		/* Report an error. */
		Callback(
			DONE, /* Command */
			0, /* DWORD Modifier */
			&Argument); /* Argument */
		return;
	}

	RtlInitUnicodeString(&usLabel, Label);

	DPRINT1("FormatEx - FAT\n");
	VfatInitialize();
	VfatFormat(
		&usDriveRoot,
		MediaFlag,
		&usLabel,
		QuickFormat,
		ClusterSize,
		Callback);
	VfatCleanup();
	RtlFreeUnicodeString(&usDriveRoot);
}

/* EOF */
