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
/* $Id: cleanup.c,v 1.3 2003/02/13 22:24:15 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/cleanup.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Hartmut Birr
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "cdfs.h"


/* FUNCTIONS ****************************************************************/

static NTSTATUS
CdfsCleanupFile(PDEVICE_EXTENSION DeviceExt,
	      PFILE_OBJECT FileObject)
/*
 * FUNCTION: Cleans up after a file has been closed.
 */
{

  DPRINT("CdfsCleanupFile(DeviceExt %x, FileObject %x)\n",
	 DeviceExt,
	 FileObject);


  /* Uninitialize file cache if initialized for this file object. */
  if (FileObject->SectionObjectPointers && FileObject->SectionObjectPointers->SharedCacheMap)
    {
      CcRosReleaseFileCache (FileObject);
    }
 
  return STATUS_SUCCESS;
}

NTSTATUS STDCALL
CdfsCleanup(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  NTSTATUS Status;

  DPRINT("CdfsCleanup() called\n");

  if (DeviceObject == CdfsGlobalData->DeviceObject)
    {
      DPRINT("Closing file system\n");
      Status = STATUS_SUCCESS;
      goto ByeBye;
    }

  Stack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = Stack->FileObject;
  DeviceExtension = DeviceObject->DeviceExtension;

  ExAcquireResourceExclusiveLite(&DeviceExtension->DirResource, TRUE);

  Status = CdfsCleanupFile(DeviceExtension, FileObject);

  ExReleaseResourceLite(&DeviceExtension->DirResource);


ByeBye:
  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return(Status);
}

/* EOF */