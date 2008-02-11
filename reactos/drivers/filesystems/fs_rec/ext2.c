/*
 *  ReactOS kernel
 *  Copyright (C) 2002,2003 ReactOS Team
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
/* $Id: fat.c 9284 2004-05-02 20:12:38Z hbirr $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/fs_rec/ext2.c (based on vfat.c)
 * PURPOSE:          Filesystem recognizer driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

#include "fs_rec.h"


/* FUNCTIONS ****************************************************************/

static NTSTATUS
FsRecIsExt2Volume(IN PDEVICE_OBJECT DeviceObject)
{
   NTSTATUS Status;
   PARTITION_INFORMATION PartitionInfo;
   DISK_GEOMETRY DiskGeometry;
   ULONG Size;
   BOOL RecognizedFS = FALSE;
   Size = sizeof(DISK_GEOMETRY);

   Status = FsRecDeviceIoControl(DeviceObject,
				 IOCTL_DISK_GET_DRIVE_GEOMETRY,
				 NULL,
				 0,
				 &DiskGeometry,
				 &Size);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("FsRecDeviceIoControl faild (%x)\n", Status);
      return Status;
   }
   if (DiskGeometry.MediaType == FixedMedia || DiskGeometry.MediaType == RemovableMedia)
   {
      // We have found a hard disk
      Size = sizeof(PARTITION_INFORMATION);
      Status = FsRecDeviceIoControl(DeviceObject,
				    IOCTL_DISK_GET_PARTITION_INFO,
				    NULL,
				    0,
				    &PartitionInfo,
				    &Size);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("FsRecDeviceIoControl faild (%x)\n", Status);
         return Status;
      }
      
      if (PartitionInfo.PartitionType)
      {
          if (PartitionInfo.PartitionType == PARTITION_EXT2)
         {
            RecognizedFS = TRUE;
         }
      }
   }

   return RecognizedFS ? STATUS_SUCCESS : STATUS_UNRECOGNIZED_VOLUME;
}


NTSTATUS
FsRecExt2FsControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  UNICODE_STRING RegistryPath;
  NTSTATUS Status;

  Stack = IoGetCurrentIrpStackLocation(Irp);

  switch (Stack->MinorFunction)
    {
      case IRP_MN_MOUNT_VOLUME:
	DPRINT("FAT: IRP_MN_MOUNT_VOLUME\n");
	Status = FsRecIsExt2Volume(Stack->Parameters.MountVolume.DeviceObject);
	if (NT_SUCCESS(Status))
	  {
	    DPRINT("Identified FAT volume\n");
	    Status = STATUS_FS_DRIVER_REQUIRED;
	  }
	break;

      case IRP_MN_LOAD_FILE_SYSTEM:
	DPRINT("FAT: IRP_MN_LOAD_FILE_SYSTEM\n");
	RtlRosInitUnicodeStringFromLiteral(&RegistryPath,
			     L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Ext2");
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
	DPRINT("FAT: Unknown minor function %lx\n", Stack->MinorFunction);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;
    }
  return(Status);
}

/* EOF */
