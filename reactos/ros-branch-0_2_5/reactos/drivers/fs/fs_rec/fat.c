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
/* $Id: fat.c,v 1.9 2004/05/02 20:12:38 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/fs_rec/vfat.c
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
FsRecIsFatVolume(IN PDEVICE_OBJECT DeviceObject)
{
   NTSTATUS Status;
   PARTITION_INFORMATION PartitionInfo;
   DISK_GEOMETRY DiskGeometry;
   ULONG Size;
   struct _BootSector* Boot;
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
         if (PartitionInfo.PartitionType == PARTITION_FAT_12       ||
             PartitionInfo.PartitionType == PARTITION_FAT_16       ||
             PartitionInfo.PartitionType == PARTITION_HUGE         ||
             PartitionInfo.PartitionType == PARTITION_FAT32        ||
             PartitionInfo.PartitionType == PARTITION_FAT32_XINT13 ||
             PartitionInfo.PartitionType == PARTITION_XINT13)
         {
            RecognizedFS = TRUE;
         }
      }
      else if (DiskGeometry.MediaType == RemovableMedia &&
               PartitionInfo.PartitionNumber > 0 &&
               PartitionInfo.StartingOffset.QuadPart == 0LL &&
               PartitionInfo.PartitionLength.QuadPart > 0LL)
      {
         /* This is possible a removable media formated as super floppy */
         RecognizedFS = TRUE;
      }
   }
   if (DiskGeometry.MediaType > Unknown && DiskGeometry.MediaType < RemovableMedia )
   {
      RecognizedFS = TRUE;
   }
   if (RecognizedFS == FALSE)
   {
      return STATUS_UNRECOGNIZED_VOLUME;
   }

   Boot = ExAllocatePool(NonPagedPool, DiskGeometry.BytesPerSector);
   if (Boot == NULL)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   Status = FsRecReadSectors(DeviceObject, 
                             0, 
                             1,
                             DiskGeometry.BytesPerSector, 
                             (PUCHAR) Boot);
   if (!NT_SUCCESS(Status))
   {
      return Status;
   }
   
   if (Boot->Signatur1 != 0xaa55)
   {
      RecognizedFS=FALSE;
   }
   if (RecognizedFS &&
       Boot->BytesPerSector != 512 &&
       Boot->BytesPerSector != 1024 &&
       Boot->BytesPerSector != 2048 && 
       Boot->BytesPerSector == 4096)
   {
      RecognizedFS=FALSE;
   }

   if (RecognizedFS &&
       Boot->FATCount != 1 && 
       Boot->FATCount != 2)
   {
      RecognizedFS=FALSE;
   }

   if (RecognizedFS &&
       Boot->Media != 0xf0 && 
       Boot->Media != 0xf8 &&
       Boot->Media != 0xf9 &&
       Boot->Media != 0xfa && 
       Boot->Media != 0xfb &&
       Boot->Media != 0xfc &&
       Boot->Media != 0xfd &&
       Boot->Media != 0xfe && 
       Boot->Media != 0xff)
   {
      RecognizedFS=FALSE;
   }

   if (RecognizedFS &&
       Boot->SectorsPerCluster != 1 &&
       Boot->SectorsPerCluster != 2 &&
       Boot->SectorsPerCluster != 4 && 
       Boot->SectorsPerCluster != 8 &&
       Boot->SectorsPerCluster != 16 &&
       Boot->SectorsPerCluster != 32 && 
       Boot->SectorsPerCluster != 64 &&
       Boot->SectorsPerCluster != 128)
   {
      RecognizedFS=FALSE;
   }

   if (RecognizedFS &&
       Boot->BytesPerSector * Boot->SectorsPerCluster > 32 * 1024)
   {
      RecognizedFS=FALSE;
   }


   ExFreePool(Boot);
   return RecognizedFS ? STATUS_SUCCESS : STATUS_UNRECOGNIZED_VOLUME;
}


NTSTATUS
FsRecVfatFsControl(IN PDEVICE_OBJECT DeviceObject,
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
	Status = FsRecIsFatVolume(Stack->Parameters.MountVolume.DeviceObject);
	if (NT_SUCCESS(Status))
	  {
	    DPRINT("Identified FAT volume\n");
	    Status = STATUS_FS_DRIVER_REQUIRED;
	  }
	break;

      case IRP_MN_LOAD_FILE_SYSTEM:
	DPRINT("FAT: IRP_MN_LOAD_FILE_SYSTEM\n");
	RtlRosInitUnicodeStringFromLiteral(&RegistryPath,
			     L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Vfatfs");
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
