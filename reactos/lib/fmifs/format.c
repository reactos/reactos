/* $Id: format.c,v 1.4 2004/02/23 11:55:12 ekohl Exp $
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
#include <string.h>

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

  RtlInitUnicodeString(&usDriveRoot, DriveRoot);
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
    }
  else
    {
      /* Unknown file system */
      Callback (DONE,        /* Command */
		0,           /* DWORD Modifier */
		&Argument);  /* Argument */
    }
}

/* EOF */
