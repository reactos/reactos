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
/* $Id: create.c,v 1.1 2002/06/25 22:21:41 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/mup/create.c
 * PURPOSE:          Multi UNC Provider
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <debug.h>

#include "mup.h"


/* FUNCTIONS ****************************************************************/

NTSTATUS STDCALL
MupCreate(PDEVICE_OBJECT DeviceObject,
	  PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExt;
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  NTSTATUS Status;

  DPRINT("MupCreate() called\n");

  DeviceExt = DeviceObject->DeviceExtension;
  assert (DeviceExt);
  Stack = IoGetCurrentIrpStackLocation (Irp);
  assert (Stack);

  FileObject = Stack->FileObject;

  DPRINT("FileName: '%wZ'\n", &FileObject->FileName);

  Status = STATUS_ACCESS_DENIED;

  Irp->IoStatus.Information = (NT_SUCCESS(Status)) ? FILE_OPENED : 0;
  Irp->IoStatus.Status = Status;

  IoCompleteRequest(Irp,
		    IO_NO_INCREMENT);

  return(Status);
}

/* EOF */
