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
/* $Id: format.c,v 1.3 2004/02/23 11:58:27 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/format.c
 * PURPOSE:         Filesystem format support functions
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <fslib/vfatlib.h>

#include "usetup.h"
#include "console.h"
#include "progress.h"

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
FormatPartition (PUNICODE_STRING DriveRoot)
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

  VfatInitialize ();

  Status = VfatFormat (DriveRoot,
		       0,               /* MediaFlag */
		       NULL,            /* Label */
		       TRUE,            /* QuickFormat */
		       0,               /* ClusterSize */
		       (PFMIFSCALLBACK)FormatCallback); /* Callback */

  VfatCleanup ();

  DestroyProgressBar (ProgressBar);
  ProgressBar = NULL;

  DPRINT ("VfatFormat() status 0x%.08x\n", Status);

  return Status;
}

/* EOF */
