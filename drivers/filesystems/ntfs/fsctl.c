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
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/ntfs/fsctl.c
 * PURPOSE:          NTFS filesystem driver
 * PROGRAMMER:       Eric Kohl
 *                   Valentin Verkhovsky
 *                   Pierre Schweitzer 
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
  ULONG ClusterSize, Size, k;
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
  BootSector = ExAllocatePoolWithTag(NonPagedPool,
                                     DiskGeometry.BytesPerSector, TAG_NTFS);
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
  if (!NT_SUCCESS(Status))
  {
    goto ByeBye;
  }
  
  /* Check values of different fields. If those fields have not expected
   * values, we fail, to avoid mounting partitions that Windows won't mount.
   */
  /* OEMID: this field must be NTFS */
  if (RtlCompareMemory(BootSector->OEMID, "NTFS    ", 8) != 8)
  {
    DPRINT1("Failed with NTFS-identifier: [%.8s]\n", BootSector->OEMID);
    Status = STATUS_UNRECOGNIZED_VOLUME;
    goto ByeBye;
  }
  /* Unused0: this field must be COMPLETELY null */
  for (k=0; k<7; k++)
  {
    if (BootSector->BPB.Unused0[k] != 0)
    {
      DPRINT1("Failed in field Unused0: [%.7s]\n", BootSector->BPB.Unused0);
      Status = STATUS_UNRECOGNIZED_VOLUME;
      goto ByeBye;
    }
  }
  /* Unused3: this field must be COMPLETELY null */
  for (k=0; k<4; k++)
  {
    if (BootSector->BPB.Unused3[k] != 0)
    {
      DPRINT1("Failed in field Unused3: [%.4s]\n", BootSector->BPB.Unused3);
      Status = STATUS_UNRECOGNIZED_VOLUME;
      goto ByeBye;
    }
  }
  /* Check cluster size */
  ClusterSize = BootSector->BPB.BytesPerSector * BootSector->BPB.SectorsPerCluster;
  if (ClusterSize != 512 && ClusterSize != 1024 && 
      ClusterSize != 2048 && ClusterSize != 4096 &&
      ClusterSize != 8192 && ClusterSize != 16384 &&
      ClusterSize != 32768 && ClusterSize != 65536)
  {
    DPRINT1("Cluster size failed: %hu, %hu, %hu\n", BootSector->BPB.BytesPerSector,
                                                    BootSector->BPB.SectorsPerCluster,
                                                    ClusterSize);
    Status = STATUS_UNRECOGNIZED_VOLUME;
    goto ByeBye;
  }

ByeBye:
  ExFreePool(BootSector);
  return Status;
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
  BootSector = ExAllocatePoolWithTag(NonPagedPool,
                                     DiskGeometry.BytesPerSector, TAG_NTFS);
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

  DPRINT("Boot sector information:\n");
  DPRINT("  BytesPerSector:         %hu\n", BootSector->BPB.BytesPerSector);
  DPRINT("  SectorsPerCluster:      %hu\n", BootSector->BPB.SectorsPerCluster);
  DPRINT("  SectorCount:            %I64u\n", BootSector->EBPB.SectorCount);
  DPRINT("  MftStart:               %I64u\n", BootSector->EBPB.MftLocation);
  DPRINT("  MftMirrStart:           %I64u\n", BootSector->EBPB.MftMirrLocation);
  DPRINT("  ClustersPerMftRecord:   %lx\n", BootSector->EBPB.ClustersPerMftRecord);
  DPRINT("  ClustersPerIndexRecord: %lx\n", BootSector->EBPB.ClustersPerIndexRecord);
  DPRINT("  SerialNumber:           %I64x\n", BootSector->EBPB.SerialNumber);

  ExFreePool(BootSector);

  MftRecord = ExAllocatePoolWithTag(NonPagedPool,
                                    NtfsInfo->BytesPerFileRecord, TAG_NTFS);
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
    ExFreePool(MftRecord);
    return Status;
  }

  VolumeRecord = ExAllocatePoolWithTag(NonPagedPool, NtfsInfo->BytesPerFileRecord, TAG_NTFS);
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
    ExFreePool(MftRecord);
    return Status;
  }

  /* Enumerate attributes */
  NtfsDumpFileAttributes (MftRecord);

  /* Enumerate attributes */
  NtfsDumpFileAttributes (VolumeRecord);

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

  ExFreePool(MftRecord);
  ExFreePool(VolumeRecord);

  return Status;
}


static NTSTATUS
NtfsMountVolume(PDEVICE_OBJECT DeviceObject,
                PIRP Irp)
{
  PDEVICE_OBJECT NewDeviceObject = NULL;
  PDEVICE_OBJECT DeviceToMount;
  PIO_STACK_LOCATION Stack;
  PNTFS_FCB Fcb = NULL;
  PNTFS_CCB Ccb = NULL;
  PNTFS_VCB Vcb = NULL;
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
                          FILE_DEVICE_DISK_FILE_SYSTEM,
                          0,
                          FALSE,
                          &NewDeviceObject);
  if (!NT_SUCCESS(Status))
    goto ByeBye;

  NewDeviceObject->Flags |= DO_DIRECT_IO;
  Vcb = (PVOID)NewDeviceObject->DeviceExtension;
  RtlZeroMemory(Vcb, sizeof(NTFS_VCB));

  Vcb->Identifier.Type = NTFS_TYPE_VCB;
  Vcb->Identifier.Size = sizeof(NTFS_TYPE_VCB);

  Status = NtfsGetVolumeData(DeviceToMount,
                             Vcb);
  if (!NT_SUCCESS(Status))
    goto ByeBye;

  NewDeviceObject->Vpb = DeviceToMount->Vpb;

  Vcb->StorageDevice = DeviceToMount;
  Vcb->StorageDevice->Vpb->DeviceObject = NewDeviceObject;
  Vcb->StorageDevice->Vpb->RealDevice = Vcb->StorageDevice;
  Vcb->StorageDevice->Vpb->Flags |= VPB_MOUNTED;
  NewDeviceObject->StackSize = Vcb->StorageDevice->StackSize + 1;
  NewDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  Vcb->StreamFileObject = IoCreateStreamFileObject(NULL,
                                                   Vcb->StorageDevice);

  InitializeListHead(&Vcb->FcbListHead);

  Fcb = NtfsCreateFCB(NULL, Vcb);
  if (Fcb == NULL)
  {
    Status = STATUS_INSUFFICIENT_RESOURCES;
    goto ByeBye;
  }

  Ccb = ExAllocatePoolWithTag(NonPagedPool,
                              sizeof(NTFS_CCB),
                              TAG_CCB);
  if (Ccb == NULL)
  {
    Status =  STATUS_INSUFFICIENT_RESOURCES;
    goto ByeBye;
  }
  RtlZeroMemory(Ccb, sizeof(NTFS_CCB));

  Ccb->Identifier.Type = NTFS_TYPE_CCB;
  Ccb->Identifier.Size = sizeof(NTFS_TYPE_CCB);

  Vcb->StreamFileObject->FsContext = Fcb;
  Vcb->StreamFileObject->FsContext2 = Ccb;
  Vcb->StreamFileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
  Vcb->StreamFileObject->PrivateCacheMap = NULL;
  Vcb->StreamFileObject->Vpb = Vcb->Vpb;
  Ccb->PtrFileObject = Vcb->StreamFileObject;
  Fcb->FileObject = Vcb->StreamFileObject;
  Fcb->Vcb = (PDEVICE_EXTENSION)Vcb->StorageDevice;

  Fcb->Flags = FCB_IS_VOLUME_STREAM;

  Fcb->RFCB.FileSize.QuadPart = Vcb->NtfsInfo.SectorCount * Vcb->NtfsInfo.BytesPerSector;
  Fcb->RFCB.ValidDataLength.QuadPart = Vcb->NtfsInfo.SectorCount * Vcb->NtfsInfo.BytesPerSector;
  Fcb->RFCB.AllocationSize.QuadPart = Vcb->NtfsInfo.SectorCount * Vcb->NtfsInfo.BytesPerSector; /* Correct? */

//  Fcb->Entry.ExtentLocationL = 0;
//  Fcb->Entry.DataLengthL = DeviceExt->CdInfo.VolumeSpaceSize * BLOCKSIZE;

  CcInitializeCacheMap(Vcb->StreamFileObject,
                       (PCC_FILE_SIZES)(&Fcb->RFCB.AllocationSize),
                       FALSE,
                       &(NtfsGlobalData->CacheMgrCallbacks),
                       Fcb);

  ExInitializeResourceLite(&Vcb->DirResource);

  KeInitializeSpinLock(&Vcb->FcbListLock);

  /* Get serial number */
  NewDeviceObject->Vpb->SerialNumber = Vcb->NtfsInfo.SerialNumber;

  /* Get volume label */
  NewDeviceObject->Vpb->VolumeLabelLength = Vcb->NtfsInfo.VolumeLabelLength;
  RtlCopyMemory(NewDeviceObject->Vpb->VolumeLabel,
                Vcb->NtfsInfo.VolumeLabel,
                Vcb->NtfsInfo.VolumeLabelLength);

  Status = STATUS_SUCCESS;

ByeBye:
  if (!NT_SUCCESS(Status))
  {
    /* Cleanup */
    if (Vcb && Vcb->StreamFileObject)
      ObDereferenceObject(Vcb->StreamFileObject);
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


NTSTATUS NTAPI
NtfsFsdFileSystemControl(PDEVICE_OBJECT DeviceObject,
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
