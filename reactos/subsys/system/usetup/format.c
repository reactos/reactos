/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/format.c
 * PURPOSE:         Filesystem format support functions
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include <ntdll/rtl.h>
#include <fslib/vfatlib.h>
#include <fslib/ext2lib.h>

#include "usetup.h"
#include "console.h"
#include "progress.h"
#include "fslist.h"

#define NDEBUG
#include <debug.h>


PPROGRESSBAR ProgressBar = NULL;

/* FUNCTIONS ****************************************************************/


BOOLEAN STDCALL
FormatCallback (CALLBACKCOMMAND Command,
		ULONG Modifier,
		PVOID Argument)
{
//  DPRINT1 ("FormatCallback() called\n");

  switch (Command)
    {
      case PROGRESS:
	{
	  PULONG Percent;

	  Percent = (PULONG)Argument;
	  DPRINT ("%lu percent completed\n", *Percent);

	  ProgressSetStep (ProgressBar, *Percent);
	}
	break;

//      case OUTPUT:
//	{
//	  PTEXTOUTPUT Output;
//		output = (PTEXTOUTPUT) Argument;
//		fprintf(stdout, "%s", output->Output);
//	}
//	break;

      case DONE:
	{
	  DPRINT ("Done\n");
//	  PBOOLEAN Success;
//		status = (PBOOLEAN) Argument;
//		if ( *status == FALSE )
//		{
//			wprintf(L"FormatEx was unable to complete successfully.\n\n");
//			Error = TRUE;
//		}
	}
	break;

      default:
	DPRINT ("Unknown callback %lu\n", (ULONG)Command);
	break;
    }

//  DPRINT1 ("FormatCallback() done\n");

  return TRUE;
}


NTSTATUS
FormatPartition (PUNICODE_STRING DriveRoot, FILE_SYSTEM FsType)
{
  NTSTATUS Status;
  SHORT xScreen;
  SHORT yScreen;

  GetScreenSize(&xScreen, &yScreen);

  ProgressBar = CreateProgressBar (6,
				   yScreen - 14,
				   xScreen - 7,
				   yScreen - 10);

  ProgressSetStepCount (ProgressBar, 100);

  if (FsType == FsFat)
    {
      VfatInitialize ();
      Status = VfatFormat (DriveRoot,
                           0,               /* MediaFlag */
                           NULL,            /* Label */
                           TRUE,            /* QuickFormat */
                           0,               /* ClusterSize */
                           FormatCallback); /* Callback */
      VfatCleanup ();
    }
  else if (FsType == FsExt2)
    {
      Status = Ext2Format (DriveRoot,
                           0,               /* MediaFlag */
                           NULL,            /* Label */
                           TRUE,            /* QuickFormat */
                           0,               /* ClusterSize */
                           FormatCallback); /* Callback */
    }

  DestroyProgressBar (ProgressBar);
  ProgressBar = NULL;

  DPRINT ("VfatFormat() status 0x%.08x\n", Status);

  return Status;
}


#if 0
NTSTATUS STDCALL
InstallFileSystemDriver (PUNICODE_STRING Name)
{ 
  ULONG StartValue = 0; /* Boot start driver. */

  Status = RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,
				 Name,
				 L"Start",
				 REG_DWORD,
				 &StartValue,
				 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RtlWriteRegistryValue() failed (Status %lx)\n", Status);
      return FALSE;
    }
}
#endif

/* EOF */
