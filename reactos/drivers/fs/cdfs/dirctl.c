/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: dirctl.c,v 1.1 2002/04/15 20:39:49 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/dirctl.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <debug.h>

#include "cdfs.h"


/* FUNCTIONS ****************************************************************/

/* HACK -- NEEDS FIXING */
#if 0
int
FsdExtractDirectoryEntry(PDEVICE_EXTENSION DeviceExt,
			 FsdFcbEntry *parent,
			 FsdFcbEntry *fill_in,
			 int entry_to_get)
{
  switch( entry_to_get )
    {
      case 0:
	wcscpy( fill_in->name, L"." );
	fill_in->extent_start = parent->extent_start;
	fill_in->byte_count = parent->byte_count;
	break;

      case 1:
	wcscpy( fill_in->name, L".." );
	fill_in->extent_start = parent->extent_start;
	fill_in->byte_count = parent->byte_count;
	break;

      case 2:
	wcscpy( fill_in->name, L"readme.txt" );
	fill_in->extent_start = 0x190;
	fill_in->byte_count = 0x800;
	break;

      default:
	return 1;
    }

  return 0;
}
#endif


NTSTATUS STDCALL
CdfsDirectoryControl(PDEVICE_OBJECT DeviceObject,
		     PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  NTSTATUS Status;

  DPRINT("CdfsDirectoryControl() called\n");

  Stack = IoGetCurrentIrpStackLocation(Irp);

  switch (Stack->MinorFunction)
    {

      default:
	DPRINT("CDFS: MinorFunction %d\n", Stack->MinorFunction);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}

/* EOF */
