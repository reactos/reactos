/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/chkdsk.c
 * PURPOSE:         Chkdsk
 *
 * PROGRAMMERS:     (none)
 */

#include "precomp.h"

/* FMIFS.1 */
VOID NTAPI
Chkdsk(
	IN PWCHAR DriveRoot,
	IN PWCHAR Format,
	IN BOOLEAN CorrectErrors,
	IN BOOLEAN Verbose,
	IN BOOLEAN CheckOnlyIfDirty,
	IN BOOLEAN ScanDrive,
	IN PVOID Unused2,
	IN PVOID Unused3,
	IN PFMIFSCALLBACK Callback)
{
	BOOLEAN Argument = FALSE;

	/* FAIL immediately */
	Callback(
		DONE, /* Command */
		0, /* DWORD Modifier */
		&Argument);/* Argument */
}

/* EOF */
