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
/* $Id: ntfs.c,v 1.5 2002/09/07 15:12:01 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/fs_rec/ntfs.c
 * PURPOSE:          Filesystem recognizer driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <ddk/ntdddisk.h>

#define NDEBUG
#include <debug.h>

#include "fs_rec.h"


/* FUNCTIONS ****************************************************************/

static NTSTATUS
FsRecIsNtfsVolume(IN PDEVICE_OBJECT DeviceObject)
{
  DISK_GEOMETRY DiskGeometry;
  PUCHAR Buffer;
  ULONG Size;
  NTSTATUS Status;

  Size = sizeof(DISK_GEOMETRY);
  Status = FsRecDeviceIoControl(DeviceObject,
				IOCTL_DISK_GET_DRIVE_GEOMETRY,
				NULL,
				0,
				&DiskGeometry,
				&Size);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("FsRecDeviceIoControl() failed (Status %lx)\n", Status);
      return(Status);
    }

  DPRINT("BytesPerSector: %lu\n", DiskGeometry.BytesPerSector);
  Buffer = ExAllocatePool(NonPagedPool,
			  DiskGeometry.BytesPerSector);
  if (Buffer == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Status = FsRecReadSectors(DeviceObject,
			    0, /* Partition boot sector */
			    1,
			    DiskGeometry.BytesPerSector,
			    Buffer);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("FsRecReadSectors() failed (Status %lx)\n", Status);
      return(Status);
    }

  DPRINT("NTFS-identifier: [%.8s]\n", &Buffer[3]);
  if (strncmp(&Buffer[3], "NTFS    ", 8) == 0)
    {
      Status = STATUS_SUCCESS;
    }
  else
    {
      Status = STATUS_UNRECOGNIZED_VOLUME;
    }

  ExFreePool(Buffer);

  return(Status);
}


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

	Status = FsRecIsNtfsVolume(Stack->Parameters.MountVolume.DeviceObject);
	if (NT_SUCCESS(Status))
	  {
	    DPRINT("Identified NTFS volume\n");
	    Status = STATUS_FS_DRIVER_REQUIRED;
	  }
	break;

      case IRP_MN_LOAD_FILE_SYSTEM:
	DPRINT("NTFS: IRP_MN_LOAD_FILE_SYSTEM\n");
	RtlInitUnicodeString(&RegistryPath,
			     L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Ntfs");
	Status = ZwLoadDriver(&RegistryPath);
	if (!NT_SUCCESS(Status))
	  {
	    DPRINT("ZwLoadDriver failed (Status %x)\n", Status);
	  }
	else
	  {
	    IoUnregisterFileSystem(DeviceObject);
	  }
	break;

      default:
	DPRINT("NTFS: Unknown minor function %lx\n", Stack->MinorFunction);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;
    }
  return(Status);
}

/* EOF */
