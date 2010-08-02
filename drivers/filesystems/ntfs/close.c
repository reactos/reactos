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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/ntfs/close.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Art Yerkes
 * UPDATE HISTORY:
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/


/* FUNCTIONS ****************************************************************/

static NTSTATUS
NtfsCloseFile(PDEVICE_EXTENSION DeviceExt,
	      PFILE_OBJECT FileObject)
/*
 * FUNCTION: Closes a file
 */
{
  PNTFS_CCB Ccb;

  DPRINT("NtfsCloseFile(DeviceExt %p, FileObject %p)\n",
	 DeviceExt,
	 FileObject);

  Ccb = (PNTFS_CCB)(FileObject->FsContext2);

  DPRINT("Ccb %p\n", Ccb);
  if (Ccb == NULL)
    {
      return(STATUS_SUCCESS);
    }

  FileObject->FsContext2 = NULL;

  if (FileObject->FileName.Buffer)
    {
      // This a FO, that was created outside from FSD.
      // Some FO's are created with IoCreateStreamFileObject() insid from FSD.
      // This FO's don't have a FileName.
      NtfsReleaseFCB(DeviceExt, FileObject->FsContext);
    }

  if (Ccb->DirectorySearchPattern)
    {
      ExFreePool(Ccb->DirectorySearchPattern);
    }
  ExFreePool(Ccb);

  return(STATUS_SUCCESS);
}


NTSTATUS NTAPI
NtfsFsdClose(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  NTSTATUS Status;

  DPRINT("NtfsClose() called\n");

  if (DeviceObject == NtfsGlobalData->DeviceObject)
    {
      DPRINT("Closing file system\n");
      Status = STATUS_SUCCESS;
      goto ByeBye;
    }

  Stack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = Stack->FileObject;
  DeviceExtension = DeviceObject->DeviceExtension;

  Status = NtfsCloseFile(DeviceExtension,FileObject);

ByeBye:
  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return(Status);
}
