/* $Id: format.c,v 1.2 2003/04/05 23:17:21 chorns Exp $
 *
 * COPYING:	See the top level directory
 * PROJECT:	ReactOS 
 * FILE:	reactos/lib/fmifs/format.c
 * DESCRIPTION:	File management IFS utility functions
 * PROGRAMMER:	Emanuele Aliberti
 * UPDATED
 * 	1999-02-16 (Emanuele Aliberti)
 * 		Entry points added.
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <fmifs.h>
#include <fslib/vfatlib.h>

#define NDEBUG
#include <debug.h>


/* FMIFS.6 */
VOID
__stdcall
Format(VOID)
{
}


/* FMIFS.7 */
VOID
__stdcall
FormatEx(
	PWCHAR		DriveRoot,
	DWORD		MediaFlag,
	PWCHAR		Format,
	PWCHAR		Label,
	BOOL		QuickFormat,
	DWORD		ClusterSize,
	PFMIFSCALLBACK	Callback
	)
{
	UNICODE_STRING usDriveRoot;
	UNICODE_STRING usLabel;
	BOOL Argument = FALSE;

	RtlInitUnicodeString(&usDriveRoot, DriveRoot);
	RtlInitUnicodeString(&usLabel, Label);

	if (_wcsnicmp(Format, L"FAT", 3) == 0)
	{
		DPRINT1("FormatEx - FAT\n");
		VfatInitialize();

		VfatFormat(&usDriveRoot, MediaFlag, &usLabel, QuickFormat, ClusterSize, Callback);

		VfatCleanup();
	}
	else
	{
		/* Unknown file system */
		Callback(DONE,		/* Command */
			0,				/* DWORD Modifier */
			&Argument		/* Argument */
		);
	}
}


/* EOF */
