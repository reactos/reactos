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
/* $Id: close.c,v 1.2 2002/05/01 13:15:42 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/close.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "cdfs.h"


/* FUNCTIONS ****************************************************************/

static NTSTATUS
CdfsCloseFile(PDEVICE_EXTENSION DeviceExt,
	      PFILE_OBJECT FileObject)
/*
 * FUNCTION: Closes a file
 */
{
  PCCB Ccb;

  DPRINT("CdfsCloseFile(DeviceExt %x, FileObject %x)\n",
	 DeviceExt,
	 FileObject);

  Ccb = (PCCB)(FileObject->FsContext2);

  DPRINT("Ccb %x\n", Ccb);
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
      CdfsReleaseFCB(DeviceExt,
		     Ccb->Fcb);
    }

  ExFreePool(Ccb);

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
CdfsClose(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  NTSTATUS Status;

  DPRINT("CdfsClose() called\n");

  Stack = IoGetCurrentIrpStackLocation(Irp);
  FileObject = Stack->FileObject;
  DeviceExtension = DeviceObject->DeviceExtension;

  Status = CdfsCloseFile(DeviceExtension,FileObject);

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);
  return(Status);
}

/* EOF */