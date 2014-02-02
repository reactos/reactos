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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/mup/create.c
 * PURPOSE:          Multi UNC Provider
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "mup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

NTSTATUS NTAPI
MupCreate(PDEVICE_OBJECT DeviceObject,
          PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExt;
    PIO_STACK_LOCATION Stack;
    PFILE_OBJECT FileObject;
    NTSTATUS Status;

    DPRINT("MupCreate() called\n");

    DeviceExt = DeviceObject->DeviceExtension;
    ASSERT(DeviceExt);
    Stack = IoGetCurrentIrpStackLocation (Irp);
    ASSERT(Stack);

    FileObject = Stack->FileObject;

    DPRINT1("MUP - Unimplemented (FileName: '%wZ')\n", &FileObject->FileName);
    Status = STATUS_OBJECT_NAME_INVALID; // STATUS_BAD_NETWORK_PATH;

    Irp->IoStatus.Information = (NT_SUCCESS(Status)) ? FILE_OPENED : 0;
    Irp->IoStatus.Status = Status;

    IoCompleteRequest(Irp,
                      IO_NO_INCREMENT);

    return Status;
}

/* EOF */
