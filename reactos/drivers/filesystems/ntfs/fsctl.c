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
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/ntfs/fsctl.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 * Updated by Valentin Verkhovsky  2003/09/12
 */

/* INCLUDES *****************************************************************/

#include "ntfs.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS *****************************************************************/

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

  DPRINT1("NtfsHasFileSystem() called\n");

  Size = sizeof(DISK_GEOMETRY);
  Status = NtfsDeviceIoControl(DeviceToMount,
			       IOCTL_DISK_GET_DRIVE_GEOMETRY,
			       NULL,
			       0,
			       &DiskGeometry,
			       &Size,
			       TRUE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtfsDeviceIoControl() failed (Status %lx)\n", Status);
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
				   &Size,
				   TRUE);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT1("NtfsDeviceIoControl() failed (Status %lx)\n", Status);
	  return(Status);
	}

      if (PartitionInfo.PartitionType != PARTITION_IFS)
	{
	  DPRINT1("Invalid partition type\n");
	  return(STATUS_UNRECOGNIZED_VOLUME);
	}
    }

  DPRINT1("BytesPerSector: %lu\n", DiskGeometry.BytesPerSector);
  BootSector = ExAllocatePool(NonPagedPool,
			      DiskGeometry.BytesPerSector);
  if (BootSector == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  Status = NtfsReadSectors (DeviceToMount,
			    0,
			    1,
			    DiskGeometry.BytesPerSector,
			    (PVOID)BootSector,
			    TRUE);
  if (NT_SUCCESS(Status))
    {
      DPRINT1("NTFS-identifier: [%.8s]\n", BootSector->OEMID);
      if (RtlCompareMemory(BootSector->OEMID, "NTFS    ", 8) != 8)
	{
	  Status = STATUS_UNRECOGNIZED_VOLUME;
	}
    }

  ExFreePool(BootSector);

  return(Status);
}


static NTSTATUS
NtfsGetVolumeData(PDEVICE_OBJECT DeviceObject,
                  PDEVICE_EXTENSION DeviceExt)
{
  DISK_GEOMETRY DiskGeometry;
  PFILE_RECORD_HEADER MftRecord;
  PFILE_RECORD_HEADER VolumeRecord;
  PVOLINFO_ATTRIBUTE VolumeInfo;
  PBOOT_SECTOR BootSector;
  PATTRIBUTE Attribute;
  ULONG Size;
  NTSTATUS Status;
  PNTFS_INFO NtfsInfo = &DeviceExt->NtfsInfo;

  DPRINT("NtfsGetVolumeData() called\n");

  Size = sizeof(DISK_GEOMETRY);
  Status = NtfsDeviceIoControl(DeviceObject,
			       IOCTL_DISK_GET_DRIVE_GEOMETRY,
			       NULL,
			       0,
			       &DiskGeometry,
			       &Size,
			       TRUE);
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

  Status = NtfsReadSectors(DeviceObject,
			   0, /* Partition boot sector */
			   1,
			   DiskGeometry.BytesPerSector,
			   (PVOID)BootSector,
			   TRUE);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool(BootSector);
      return Status;
    }

  /* Read data from the bootsector */
  NtfsInfo->BytesPerSector = BootSector->BPB.BytesPerSector;
  NtfsInfo->SectorsPerCluster = BootSector->BPB.SectorsPerCluster;
  NtfsInfo->BytesPerCluster = BootSector->BPB.BytesPerSector * BootSector->BPB.SectorsPerCluster;
  NtfsInfo->SectorCount = BootSector->EBPB.SectorCount;

  NtfsInfo->MftStart.QuadPart = BootSector->EBPB.MftLocation;
  NtfsInfo->MftMirrStart.QuadPart = BootSector->EBPB.MftMirrLocation;
  NtfsInfo->SerialNumber = BootSector->EBPB.SerialNumber;
  if (BootSector->EBPB.ClustersPerMftRecord > 0)
    NtfsInfo->BytesPerFileRecord = BootSector->EBPB.ClustersPerMftRecord * NtfsInfo->BytesPerCluster;
  else
    NtfsInfo->BytesPerFileRecord = 1 << (-BootSector->EBPB.ClustersPerMftRecord);

//#ifndef NDEBUG
  DbgPrint("Boot sector information:\n");
  DbgPrint("  BytesPerSector:         %hu\n", BootSector->BPB.BytesPerSector);
  DbgPrint("  SectorsPerCluster:      %hu\n", BootSector->BPB.SectorsPerCluster);

  DbgPrint("  SectorCount:            %I64u\n", BootSector->EBPB.SectorCount);

  DbgPrint("  MftStart:               %I64u\n", BootSector->EBPB.MftLocation);
  DbgPrint("  MftMirrStart:           %I64u\n", BootSector->EBPB.MftMirrLocation);

  DbgPrint("  ClustersPerMftRecord:   %lx\n", BootSector->EBPB.ClustersPerMftRecord);
  DbgPrint("  ClustersPerIndexRecord: %lx\n", BootSector->EBPB.ClustersPerIndexRecord);

  DbgPrint("  SerialNumber:           %I64x\n", BootSector->EBPB.SerialNumber);
//#endif

  ExFreePool(BootSector);

  MftRecord = ExAllocatePool(NonPagedPool,
			     NtfsInfo->BytesPerFileRecord);
  if (MftRecord == NULL)
    {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  Status = NtfsReadSectors(DeviceObject,
			   NtfsInfo->MftStart.u.LowPart * NtfsInfo->SectorsPerCluster,
			   NtfsInfo->BytesPerFileRecord / NtfsInfo->BytesPerSector,
			   NtfsInfo->BytesPerSector,
			   (PVOID)MftRecord,
			   TRUE);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool (MftRecord);
      return Status;
    }

  VolumeRecord = ExAllocatePool(NonPagedPool, NtfsInfo->BytesPerFileRecord);
  if (VolumeRecord == NULL)
    {
      ExFreePool (MftRecord);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  /* Read Volume File (MFT index 3) */
  DeviceExt->StorageDevice = DeviceObject;
  Status = ReadFileRecord(DeviceExt, 3, VolumeRecord, MftRecord);
  if (!NT_SUCCESS(Status))
    {
      ExFreePool (MftRecord);
      return Status;
    }

#ifndef NDEBUG
  DbgPrint("\n\n");
  /* Enumerate attributes */
  NtfsDumpFileAttributes (MftRecord);
  DbgPrint("\n\n");

  DbgPrint("\n\n");
  /* Enumerate attributes */
  NtfsDumpFileAttributes (VolumeRecord);
  DbgPrint("\n\n");
#endif

  /* Get volume name */
  Attribute = FindAttribute (VolumeRecord, AttributeVolumeName, NULL);
  DPRINT("Attribute %p\n", Attribute);

  if (Attribute != NULL && ((PRESIDENT_ATTRIBUTE)Attribute)->ValueLength != 0)
    {
      DPRINT("Data length %lu\n", AttributeDataLength (Attribute));
      NtfsInfo->VolumeLabelLength =
	min (((PRESIDENT_ATTRIBUTE)Attribute)->ValueLength, MAXIMUM_VOLUME_LABEL_LENGTH);
      RtlCopyMemory (NtfsInfo->VolumeLabel,
		     (PVOID)((ULONG_PTR)Attribute + ((PRESIDENT_ATTRIBUTE)Attribute)->ValueOffset),
		     NtfsInfo->VolumeLabelLength);
    }
  else
    {
      NtfsInfo->VolumeLabelLength = 0;
    }

  /* Get volume information */
  Attribute = FindAttribute (VolumeRecord, AttributeVolumeInformation, NULL);
  DPRINT("Attribute %p\n", Attribute);

  if (Attribute != NULL && ((PRESIDENT_ATTRIBUTE)Attribute)->ValueLength != 0)
    {
      DPRINT("Data length %lu\n", AttributeDataLength (Attribute));
      VolumeInfo = (PVOID)((ULONG_PTR)Attribute + ((PRESIDENT_ATTRIBUTE)Attribute)->ValueOffset);

      NtfsInfo->MajorVersion = VolumeInfo->MajorVersion;
      NtfsInfo->MinorVersion = VolumeInfo->MinorVersion;
      NtfsInfo->Flags = VolumeInfo->Flags;
    }

  ExFreePool (MftRecord);

  return Status;
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

  DPRINT1("NtfsMountVolume() called\n");

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

  DeviceExt->StreamFileObject->FsContext = Fcb;
  DeviceExt->StreamFileObject->FsContext2 = Ccb;
  DeviceExt->StreamFileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
  DeviceExt->StreamFileObject->PrivateCacheMap = NULL;
  DeviceExt->StreamFileObject->Vpb = DeviceExt->Vpb;
  Ccb->PtrFileObject = DeviceExt->StreamFileObject;
  Fcb->FileObject = DeviceExt->StreamFileObject;
  Fcb->DevExt = (PDEVICE_EXTENSION)DeviceExt->StorageDevice;

  Fcb->Flags = FCB_IS_VOLUME_STREAM;

  Fcb->RFCB.FileSize.QuadPart = DeviceExt->NtfsInfo.SectorCount * DeviceExt->NtfsInfo.BytesPerSector;
  Fcb->RFCB.ValidDataLength.QuadPart = DeviceExt->NtfsInfo.SectorCount * DeviceExt->NtfsInfo.BytesPerSector;
  Fcb->RFCB.AllocationSize.QuadPart = DeviceExt->NtfsInfo.SectorCount * DeviceExt->NtfsInfo.BytesPerSector; /* Correct? */

//  Fcb->Entry.ExtentLocationL = 0;
//  Fcb->Entry.DataLengthL = DeviceExt->CdInfo.VolumeSpaceSize * BLOCKSIZE;
#ifdef ROS_USE_CC_AND_FS
  Status = CcRosInitializeFileCache(DeviceExt->StreamFileObject,
				    CACHEPAGESIZE(DeviceExt));
  if (!NT_SUCCESS (Status))
    {
      DbgPrint("CcRosInitializeFileCache() failed (Status %lx)\n", Status);
      goto ByeBye;
    }
#else
  CcInitializeCacheMap(DeviceExt->StreamFileObject,
                       (PCC_FILE_SIZES)(&Fcb->RFCB.AllocationSize),
                       FALSE,
                       NULL,
                       NULL);
#endif
  ExInitializeResourceLite(&DeviceExt->DirResource);
//  ExInitializeResourceLite(&DeviceExt->FatResource);

  KeInitializeSpinLock(&DeviceExt->FcbListLock);
  InitializeListHead(&DeviceExt->FcbListHead);

  /* Get serial number */
  NewDeviceObject->Vpb->SerialNumber = DeviceExt->NtfsInfo.SerialNumber;

  /* Get volume label */
  NewDeviceObject->Vpb->VolumeLabelLength = DeviceExt->NtfsInfo.VolumeLabelLength;
  RtlCopyMemory (NewDeviceObject->Vpb->VolumeLabel,
		 DeviceExt->NtfsInfo.VolumeLabel,
		 DeviceExt->NtfsInfo.VolumeLabelLength);

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

  return Status;
}


static NTSTATUS
NtfsVerifyVolume(PDEVICE_OBJECT DeviceObject,
		 PIRP Irp)
{
  DPRINT1("NtfsVerifyVolume() called\n");

  return STATUS_WRONG_VOLUME;
}


NTSTATUS STDCALL
NtfsFileSystemControl(PDEVICE_OBJECT DeviceObject,
		      PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  NTSTATUS Status;

  DPRINT1("NtfsFileSystemControl() called\n");

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
