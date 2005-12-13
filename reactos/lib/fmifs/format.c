/* $Id$
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
#include "precomp.h"

#define NDEBUG
#include <debug.h>


/* FMIFS.6 */
VOID STDCALL
Format (VOID)
{
}


/* FMIFS.7 */
VOID STDCALL
FormatEx (PWCHAR DriveRoot,
	  ULONG MediaFlag,
	  PWCHAR Format,
	  PWCHAR Label,
	  BOOLEAN QuickFormat,
	  ULONG ClusterSize,
	  PFMIFSCALLBACK Callback)
{
  UNICODE_STRING usDriveRoot;
  UNICODE_STRING usLabel;
  BOOLEAN Argument = FALSE;
  WCHAR VolumeName[MAX_PATH];
  CURDIR CurDir;

  if (_wcsnicmp(Format, L"FAT", 3) != 0)
    {
      /* Unknown file system */
      Callback (DONE,        /* Command */
		0,           /* DWORD Modifier */
		&Argument);  /* Argument */
    }

  if (!GetVolumeNameForVolumeMountPointW(DriveRoot, VolumeName, MAX_PATH) ||
      !RtlDosPathNameToNtPathName_U(VolumeName, &usDriveRoot, NULL, &CurDir))
    {
      /* Report an error. */
      Callback (DONE,        /* Command */
		0,           /* DWORD Modifier */
		&Argument);  /* Argument */

      return;
    }

  RtlInitUnicodeString(&usLabel, Label);

  if (_wcsnicmp(Format, L"FAT", 3) == 0)
    {
      DPRINT1("FormatEx - FAT\n");

      VfatInitialize ();
      VfatFormat (&usDriveRoot,
		  MediaFlag,
		  &usLabel,
		  QuickFormat,
		  ClusterSize,
		  Callback);
      VfatCleanup ();
      RtlFreeUnicodeString(&usDriveRoot);
    }
}

/* EOF */
