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
/* $Id: ntfs.c,v 1.1 2002/05/15 09:40:47 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/fs_rec/ntfs.c
 * PURPOSE:          Filesystem recognizer driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

//#define NDEBUG
#include <debug.h>

#include "fs_rec.h"


/* FUNCTIONS ****************************************************************/


NTSTATUS
FsRecNtfsFsControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  UNICODE_STRING RegistryPath;
  NTSTATUS Status;

  Stack = IoGetCurrentIrpStackLocation(Irp);

  switch (Stack->MinorFunction)
    {
      case IRP_MN_MOUNT_VOLUME:
	DPRINT("NTFS: IRP_MN_MOUNT_VOLUME\n");
	Status = STATUS_INVALID_DEVICE_REQUEST;
#if 0
	Status = FsRecIsNtfsVolume(Stack->Parameters.MountVolume.DeviceObject);
	if (NT_SUCCESS(Status))
	  {
	    DPRINT("Identified NTFS volume\n");
	    Status = STATUS_FS_DRIVER_REQUIRED;
	  }
#endif
	break;

      case IRP_MN_LOAD_FILE_SYSTEM:
	DPRINT("NTFS: IRP_MN_LOAD_FILE_SYSTEM\n");
	Status = STATUS_INVALID_DEVICE_REQUEST;
#if 0
#if 0
	RtlInitUnicodeString(&RegistryPath, FSD_REGISTRY_PATH);
#endif
	RtlInitUnicodeString(&RegistryPath,
			     L"\\SystemRoot\\system32\\drivers\\ntfs.sys");
	Status = ZwLoadDriver(&RegistryPath);
	if (!NT_SUCCESS(Status))
	  {
	    DPRINT("ZwLoadDriver failed (Status %x)\n", Status);
	  }
	else
	  {
	    IoUnregisterFileSystem(DeviceObject);
	  }
#endif
	break;

      default:
	DPRINT("NTFS: Unknown minor function %lx\n", Stack->MinorFunction);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;
    }
  return(Status);
}

/* EOF */
