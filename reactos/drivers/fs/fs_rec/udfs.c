/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
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
/* $Id: udfs.c,v 1.1 2003/01/16 11:58:15 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/fs_rec/udfs.c
 * PURPOSE:          Filesystem recognizer driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "fs_rec.h"


#define UDFS_VRS_START_SECTOR   16
#define UDFS_AVDP_SECTOR       256

/* TYPES ********************************************************************/

typedef struct _TAG
{
  USHORT Identifier;
  USHORT Version;
  UCHAR  Checksum;
  UCHAR  Reserved;
  USHORT SerialNumber;
  USHORT Crc;
  USHORT CrcLength;
  ULONG  Location;
} PACKED TAG, *PTAG;

typedef struct _EXTENT
{
  ULONG Length;
  ULONG Location;
} PACKED EXTENT, *PEXTENT;

typedef struct _AVDP
{
  TAG    DescriptorTag;
  EXTENT MainVolumeDescriptorExtent;
  EXTENT ReserveVolumeDescriptorExtent;
} PACKED AVDP, *PAVDP;

/* FUNCTIONS ****************************************************************/

static NTSTATUS
FsRecCheckVolumeRecognitionSequence(IN PDEVICE_OBJECT DeviceObject,
				    IN ULONG SectorSize)
{
  PUCHAR Buffer;
  ULONG Sector;
  NTSTATUS Status;
  ULONG State;

  Buffer = ExAllocatePool(NonPagedPool,
			  SectorSize);
  if (Buffer == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  State = 0;
  Sector = UDFS_VRS_START_SECTOR;
  while (TRUE)
    {
      Status = FsRecReadSectors(DeviceObject,
				Sector,
				1,
				SectorSize,
				Buffer);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("FsRecReadSectors() failed (Status %lx)\n", Status);
	  break;
	}

      DPRINT1("Descriptor identifier: [%.5s]\n", Buffer + 1);

      switch (State)
	{
	  case 0:
	    if ((Sector == UDFS_VRS_START_SECTOR) &&
		(Buffer[1] == 'B') &&
		(Buffer[2] == 'E') &&
		(Buffer[3] == 'A') &&
		(Buffer[4] == '0') &&
		(Buffer[5] == '1'))
	      {
		State = 1;
	      }
	    else
	      {
		DPRINT1("VRS start sector is not 'BEA01'\n");
		ExFreePool(Buffer);
		return(STATUS_UNRECOGNIZED_VOLUME);
	      }
	    break;

	  case 1:
	    if ((Buffer[1] == 'N') &&
		(Buffer[2] == 'S') &&
		(Buffer[3] == 'R') &&
		(Buffer[4] == '0') &&
		((Buffer[5] == '2') || (Buffer[5] == '3')))
	      {
		State = 2;
	      }
	    break;

	  case 2:
	    if ((Buffer[1] == 'T') &&
		(Buffer[2] == 'E') &&
		(Buffer[3] == 'A') &&
		(Buffer[4] == '0') &&
		(Buffer[5] == '1'))
	      {
		DPRINT1("Found 'TEA01'\n");
		ExFreePool(Buffer);
		return(STATUS_SUCCESS);
	      }
	    break;
	}

      Sector++;
      if (Sector == UDFS_AVDP_SECTOR)
	{
	  DPRINT1("No 'TEA01' found\n");
	  ExFreePool(Buffer);
	  return(STATUS_UNRECOGNIZED_VOLUME);
	}
    }

  ExFreePool(Buffer);

  return(STATUS_UNRECOGNIZED_VOLUME);
}


static NTSTATUS
FsRecCheckAnchorVolumeDescriptorPointer(IN PDEVICE_OBJECT DeviceObject,
					IN ULONG SectorSize)
{
  PUCHAR Buffer;
  ULONG Sector;
  NTSTATUS Status;
  PAVDP Avdp;

  Buffer = ExAllocatePool(NonPagedPool,
			  SectorSize);
  if (Buffer == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Sector = UDFS_AVDP_SECTOR;
  Status = FsRecReadSectors(DeviceObject,
			    Sector,
			    1,
			    SectorSize,
			    Buffer);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("FsRecReadSectors() failed (Status %lx)\n", Status);
      ExFreePool(Buffer);
      return(Status);
    }

  Avdp = (PAVDP)Buffer;
  DPRINT1("Descriptor identifier: %hu\n", Avdp->DescriptorTag.Identifier);

  DPRINT1("Main volume descriptor sequence location: %lu\n",
	  Avdp->MainVolumeDescriptorExtent.Location);

  DPRINT1("Main volume descriptor sequence length: %lu\n",
	  Avdp->MainVolumeDescriptorExtent.Length);

  DPRINT1("Reserve volume descriptor sequence location: %lu\n",
	  Avdp->ReserveVolumeDescriptorExtent.Location);

  DPRINT1("Reserve volume descriptor sequence length: %lu\n",
	  Avdp->ReserveVolumeDescriptorExtent.Length);

  ExFreePool(Buffer);

//  return(Status);
  return(STATUS_SUCCESS);
}


static NTSTATUS
FsRecIsUdfsVolume(IN PDEVICE_OBJECT DeviceObject)
{
  DISK_GEOMETRY DiskGeometry;
  ULONG Size;
  NTSTATUS Status;

  Size = sizeof(DISK_GEOMETRY);
  Status = FsRecDeviceIoControl(DeviceObject,
				IOCTL_CDROM_GET_DRIVE_GEOMETRY,
				NULL,
				0,
				&DiskGeometry,
				&Size);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("FsRecDeviceIoControl() failed (Status %lx)\n", Status);
      return(Status);
    }
  DPRINT1("BytesPerSector: %lu\n", DiskGeometry.BytesPerSector);

  /* Check the volume recognition sequence */
  Status = FsRecCheckVolumeRecognitionSequence(DeviceObject,
					       DiskGeometry.BytesPerSector);
  if (!NT_SUCCESS(Status))
    return(Status);

  Status = FsRecCheckAnchorVolumeDescriptorPointer(DeviceObject,
						   DiskGeometry.BytesPerSector);
  if (!NT_SUCCESS(Status))
    return(Status);


  return(STATUS_SUCCESS);
}


NTSTATUS
FsRecUdfsFsControl(IN PDEVICE_OBJECT DeviceObject,
		   IN PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  UNICODE_STRING RegistryPath;
  NTSTATUS Status;

  Stack = IoGetCurrentIrpStackLocation(Irp);

  switch (Stack->MinorFunction)
    {
      case IRP_MN_MOUNT_VOLUME:
	DPRINT("Udfs: IRP_MN_MOUNT_VOLUME\n");
	Status = FsRecIsUdfsVolume(Stack->Parameters.MountVolume.DeviceObject);
	if (NT_SUCCESS(Status))
	  {
	    DPRINT("Identified UDFS volume\n");
	    Status = STATUS_FS_DRIVER_REQUIRED;
	  }
	break;

      case IRP_MN_LOAD_FILE_SYSTEM:
	DPRINT("Udfs: IRP_MN_LOAD_FILE_SYSTEM\n");
	RtlInitUnicodeStringFromLiteral(&RegistryPath,
			     L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Udfs");
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
	DPRINT("Udfs: Unknown minor function %lx\n", Stack->MinorFunction);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;
    }
  return(Status);
}

/* EOF */
