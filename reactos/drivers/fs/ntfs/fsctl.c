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
/* $Id: fsctl.c,v 1.2 2002/07/15 15:37:33 ekohl Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/ntfs/fsctl.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "ntfs.h"

/* FUNCTIONS ****************************************************************/

static NTSTATUS
NtfsHasFileSystem(PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Tests if the device contains a filesystem that can be mounted 
 * by this fsd
 */
{
  PARTITION_INFORMATION PartitionInfo;
  DISK_GEOMETRY DiskGeometry;
  ULONG Size;
  PBOOT_SECTOR BootSector;
  NTSTATUS Status;

  DPRINT("NtfsHasFileSystem() called\n");

  Size = sizeof(DISK_GEOMETRY);
  Status = NtfsDeviceIoControl(DeviceToMount,
			       IOCTL_DISK_GET_DRIVE_GEOMETRY,
			       NULL,
			       0,
			       &DiskGeometry,
			       &Size);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtfsDeviceIoControl() failed (Status %lx)\n", Status);
      return(Status);
    }

  if (DiskGeometry.MediaType == FixedMedia)
    {
      /* We have found a hard disk */
      Size = sizeof(PARTITION_INFORMATION);
      Status = NtfsDeviceIoControl(DeviceToMount,
				   IOCTL_DISK_GET_PARTITION_INFO,
				   NULL,
				   0,
				   &PartitionInfo,
				   &Size);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("NtfsDeviceIoControl() failed (Status %lx)\n", Status);
	  return(Status);
	}

      if (PartitionInfo.PartitionType != PARTITION_IFS)
	{
	  return(STATUS_UNRECOGNIZED_VOLUME);
	}
    }

  DPRINT("BytesPerSector: %lu\n", DiskGeometry.BytesPerSector);
  BootSector = ExAllocatePool(NonPagedPool,
			      DiskGeometry.BytesPerSector);
  if (BootSector == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Status = NtfsReadRawSectors(DeviceToMount,
			      0,
			      1,
			      DiskGeometry.BytesPerSector,
			      (PVOID)BootSector);
  if (NT_SUCCESS(Status))
    {
      DPRINT("NTFS-identifier: [%.8s]\n", BootSector->OemName);
      if (strncmp(BootSector->OemName, "NTFS    ", 8) != 0)
	{
	  Status = STATUS_UNRECOGNIZED_VOLUME;
	}
    }

  ExFreePool(BootSector);

  return(Status);
}


static NTSTATUS
NtfsGetVolumeData(PDEVICE_OBJECT DeviceObject,
		  PDEVICE_EXTENSION Vcb)
{
  DISK_GEOMETRY DiskGeometry;
//  PUCHAR Buffer;
  ULONG Size;
  NTSTATUS Status;
  PBOOT_SECTOR BootSector;

  DPRINT("NtfsGetVolumeData() called\n");

  Size = sizeof(DISK_GEOMETRY);
  Status = NtfsDeviceIoControl(DeviceObject,
			       IOCTL_DISK_GET_DRIVE_GEOMETRY,
			       NULL,
			       0,
			       &DiskGeometry,
			       &Size);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtfsDeviceIoControl() failed (Status %lx)\n", Status);
      return(Status);
    }

  DPRINT("BytesPerSector: %lu\n", DiskGeometry.BytesPerSector);
  BootSector = ExAllocatePool(NonPagedPool,
			      DiskGeometry.BytesPerSector);
  if (BootSector == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Status = NtfsReadRawSectors(DeviceObject,
			      0, /* Partition boot sector */
			      1,
			      DiskGeometry.BytesPerSector,
			      (PVOID)BootSector);
  if (NT_SUCCESS(Status))
    {
      /* Read data from the bootsector */
      Vcb->NtfsInfo.BytesPerSector = BootSector->BytesPerSector;
      Vcb->NtfsInfo.SectorsPerCluster = BootSector->SectorsPerCluster;
      Vcb->NtfsInfo.BytesPerCluster = BootSector->BytesPerSector * BootSector->SectorsPerCluster;
      Vcb->NtfsInfo.SectorCount = BootSector->SectorCount;

      Vcb->NtfsInfo.MftStart.QuadPart = BootSector->MftLocation;
      Vcb->NtfsInfo.MftMirrStart.QuadPart = BootSector->MftMirrLocation;
      Vcb->NtfsInfo.SerialNumber = BootSector->SerialNumber;

//#indef NDEBUG
      DbgPrint("Boot sector information:\n");
      DbgPrint("  BytesPerSector:         %hu\n", BootSector->BytesPerSector);
      DbgPrint("  SectorsPerCluster:      %hu\n", BootSector->SectorsPerCluster);

      DbgPrint("  SectorCount:            %I64u\n", BootSector->SectorCount);

      DbgPrint("  MftStart:               %I64u\n", BootSector->MftLocation);
      DbgPrint("  MftMirrStart:           %I64u\n", BootSector->MftMirrLocation);

      DbgPrint("  ClustersPerMftRecord:   %lx\n", BootSector->ClustersPerMftRecord);
      DbgPrint("  ClustersPerIndexRecord: %lx\n", BootSector->ClustersPerIndexRecord);

      DbgPrint("  SerialNumber:           %I64x\n", BootSector->SerialNumber);
//#endif

      NtfsOpenMft(DeviceObject, Vcb);

    }

  ExFreePool(BootSector);

  return(Status);
}



static NTSTATUS
NtfsMountVolume(PDEVICE_OBJECT DeviceObject,
		PIRP Irp)
{
  PDEVICE_EXTENSION DeviceExt = NULL;
  PDEVICE_OBJECT NewDeviceObject = NULL;
  PDEVICE_OBJECT DeviceToMount;
  PIO_STACK_LOCATION Stack;
  PFCB Fcb = NULL;
  PCCB Ccb = NULL;
  PVPB Vpb;
  NTSTATUS Status;

  DPRINT("NtfsMountVolume() called\n");

  if (DeviceObject != NtfsGlobalData->DeviceObject)
    {
      Status = STATUS_INVALID_DEVICE_REQUEST;
      goto ByeBye;
    }

  Stack = IoGetCurrentIrpStackLocation(Irp);
  DeviceToMount = Stack->Parameters.MountVolume.DeviceObject;
  Vpb = Stack->Parameters.MountVolume.Vpb;

  Status = NtfsHasFileSystem(DeviceToMount);
  if (!NT_SUCCESS(Status))
    {
      goto ByeBye;
    }

  Status = IoCreateDevice(NtfsGlobalData->DriverObject,
			  sizeof(DEVICE_EXTENSION),
			  NULL,
			  FILE_DEVICE_FILE_SYSTEM,
//			  FILE_DEVICE_DISK_FILE_SYSTEM,
			  0,
			  FALSE,
			  &NewDeviceObject);
  if (!NT_SUCCESS(Status))
    goto ByeBye;

  NewDeviceObject->Flags = NewDeviceObject->Flags | DO_DIRECT_IO;
  DeviceExt = (PVOID)NewDeviceObject->DeviceExtension;
  RtlZeroMemory(DeviceExt,
		sizeof(DEVICE_EXTENSION));

  Status = NtfsGetVolumeData(DeviceToMount,
			     DeviceExt);
  if (!NT_SUCCESS(Status))
    goto ByeBye;

  NewDeviceObject->Vpb = DeviceToMount->Vpb;

  DeviceExt->StorageDevice = DeviceToMount;
  DeviceExt->StorageDevice->Vpb->DeviceObject = NewDeviceObject;
  DeviceExt->StorageDevice->Vpb->RealDevice = DeviceExt->StorageDevice;
  DeviceExt->StorageDevice->Vpb->Flags |= VPB_MOUNTED;
  NewDeviceObject->StackSize = DeviceExt->StorageDevice->StackSize + 1;
  NewDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  DeviceExt->StreamFileObject = IoCreateStreamFileObject(NULL,
							 DeviceExt->StorageDevice);


  Fcb = NtfsCreateFCB(NULL);
  if (Fcb == NULL)
    {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
    }

  Ccb = ExAllocatePoolWithTag(NonPagedPool,
			      sizeof(CCB),
			      TAG_CCB);
  if (Ccb == NULL)
    {
      Status =  STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
    }
  RtlZeroMemory(Ccb,
		sizeof(CCB));

  DeviceExt->StreamFileObject->Flags = DeviceExt->StreamFileObject->Flags | FO_FCB_IS_VALID | FO_DIRECT_CACHE_PAGING_READ;
  DeviceExt->StreamFileObject->FsContext = (PVOID)&Fcb->RFCB;
  DeviceExt->StreamFileObject->FsContext2 = Ccb;
  DeviceExt->StreamFileObject->SectionObjectPointers = &Fcb->SectionObjectPointers;
  DeviceExt->StreamFileObject->PrivateCacheMap = NULL;
  DeviceExt->StreamFileObject->Vpb = DeviceExt->Vpb;
  Ccb->Fcb = Fcb;
  Ccb->PtrFileObject = DeviceExt->StreamFileObject;
  Fcb->FileObject = DeviceExt->StreamFileObject;
  Fcb->DevExt = (PDEVICE_EXTENSION)DeviceExt->StorageDevice;

  Fcb->Flags = FCB_IS_VOLUME_STREAM;

  Fcb->RFCB.FileSize.QuadPart = DeviceExt->NtfsInfo.SectorCount * DeviceExt->NtfsInfo.BytesPerSector;
  Fcb->RFCB.ValidDataLength.QuadPart = DeviceExt->NtfsInfo.SectorCount * DeviceExt->NtfsInfo.BytesPerSector;
  Fcb->RFCB.AllocationSize.QuadPart = DeviceExt->NtfsInfo.SectorCount * DeviceExt->NtfsInfo.BytesPerSector; /* Correct? */

//  Fcb->Entry.ExtentLocationL = 0;
//  Fcb->Entry.DataLengthL = DeviceExt->CdInfo.VolumeSpaceSize * BLOCKSIZE;

  Status = CcRosInitializeFileCache(DeviceExt->StreamFileObject,
				    &Fcb->RFCB.Bcb,
				    CACHEPAGESIZE(DeviceExt));
  if (!NT_SUCCESS (Status))
    {
      DbgPrint("CcRosInitializeFileCache() failed (Status %lx)\n", Status);
      goto ByeBye;
    }

  ExInitializeResourceLite(&DeviceExt->DirResource);
//  ExInitializeResourceLite(&DeviceExt->FatResource);

  KeInitializeSpinLock(&DeviceExt->FcbListLock);
  InitializeListHead(&DeviceExt->FcbListHead);

  /* Read serial number */
  NewDeviceObject->Vpb->SerialNumber = DeviceExt->NtfsInfo.SerialNumber;

  /* Read volume label */
//  NtfsReadVolumeLabel(DeviceExt,
//		      NewDeviceObject->Vpb);

  Status = STATUS_SUCCESS;

ByeBye:
  if (!NT_SUCCESS(Status))
    {
      /* Cleanup */
      if (DeviceExt && DeviceExt->StreamFileObject)
	ObDereferenceObject(DeviceExt->StreamFileObject);
      if (Fcb)
	ExFreePool(Fcb);
      if (Ccb)
	ExFreePool(Ccb);
      if (NewDeviceObject)
	IoDeleteDevice(NewDeviceObject);
    }

  DPRINT("NtfsMountVolume() done (Status: %lx)\n", Status);

  return(Status);
}


static NTSTATUS
NtfsVerifyVolume(PDEVICE_OBJECT DeviceObject,
		 PIRP Irp)
{
#if 0
  PDEVICE_OBJECT DeviceToVerify;
  PIO_STACK_LOCATION Stack;
  PUCHAR Buffer;
  ULONG Sector;
  ULONG i;
  NTSTATUS Status;

  union
    {
      ULONG Value;
      UCHAR Part[4];
    } Serial;
#endif

  DPRINT1("NtfsVerifyVolume() called\n");

#if 0
  Stack = IoGetCurrentIrpStackLocation(Irp);
  DeviceToVerify = Stack->Parameters.VerifyVolume.DeviceObject;

  DPRINT("Device object %p  Device to verify %p\n", DeviceObject, DeviceToVerify);

  Sector = CDFS_PRIMARY_DESCRIPTOR_LOCATION;

  Buffer = ExAllocatePool(NonPagedPool,
			  CDFS_BASIC_SECTOR);
  if (Buffer == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  do
    {
      /* Read the Primary Volume Descriptor (PVD) */
      Status = CdfsReadRawSectors(DeviceToVerify,
				  Sector,
				  1,
				  Buffer);
      DPRINT("CdfsReadRawSectors() status %lx\n", Status);
      if (!NT_SUCCESS(Status))
	{
	  goto ByeBye;
	}

      if (Buffer[0] == 1 &&
	  Buffer[1] == 'C' &&
	  Buffer[2] == 'D' &&
	  Buffer[3] == '0' &&
	  Buffer[4] == '0' &&
	  Buffer[5] == '1')
	{
	  break;
	}

      Sector++;
    }
  while (Buffer[0] != 255);

  if (Buffer[0] == 255)
    goto ByeBye;

  Status = STATUS_WRONG_VOLUME;

  /* Calculate the volume serial number */
  Serial.Value = 0;
  for (i = 0; i < 2048; i += 4)
    {
      /* DON'T optimize this to ULONG!!! (breaks overflow) */
      Serial.Part[0] += Buffer[i+3];
      Serial.Part[1] += Buffer[i+2];
      Serial.Part[2] += Buffer[i+1];
      Serial.Part[3] += Buffer[i+0];
    }

  DPRINT("Current serial number %08lx  Vpb serial number %08lx\n",
	  Serial.Value, DeviceToVerify->Vpb->SerialNumber);

  if (Serial.Value == DeviceToVerify->Vpb->SerialNumber)
    Status = STATUS_SUCCESS;

ByeBye:
  ExFreePool(Buffer);

  DPRINT("CdfsVerifyVolume() done (Status: %lx)\n", Status);

  return(Status);
#endif
  return(STATUS_UNSUCCESSFUL);
}


NTSTATUS STDCALL
NtfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  NTSTATUS Status;

  DPRINT("NtfsFileSystemControl() called\n");

  Stack = IoGetCurrentIrpStackLocation(Irp);

  switch (Stack->MinorFunction)
    {
      case IRP_MN_USER_FS_REQUEST:
	DPRINT("NTFS: IRP_MN_USER_FS_REQUEST\n");
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;

      case IRP_MN_MOUNT_VOLUME:
	DPRINT("NTFS: IRP_MN_MOUNT_VOLUME\n");
	Status = NtfsMountVolume(DeviceObject, Irp);
	break;

      case IRP_MN_VERIFY_VOLUME:
	DPRINT1("NTFS: IRP_MN_VERIFY_VOLUME\n");
	Status = NtfsVerifyVolume(DeviceObject, Irp);
	break;

      default:
	DPRINT("NTFS FSC: MinorFunction %d\n", Stack->MinorFunction);
	Status = STATUS_INVALID_DEVICE_REQUEST;
	break;
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}

/* EOF */
