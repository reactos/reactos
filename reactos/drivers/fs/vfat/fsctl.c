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
/* $Id: fsctl.c,v 1.7 2002/06/07 16:53:18 hbirr Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/fsctl.c
 * PURPOSE:          VFAT Filesystem
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

#define  CACHEPAGESIZE(pDeviceExt) ((pDeviceExt)->FatInfo.BytesPerCluster > PAGESIZE ? \
		   (pDeviceExt)->FatInfo.BytesPerCluster : PAGESIZE)


static NTSTATUS
VfatHasFileSystem(PDEVICE_OBJECT DeviceToMount,
		  PBOOLEAN RecognizedFS,
		  PFATINFO pFatInfo)
{
   NTSTATUS Status;
   PARTITION_INFORMATION PartitionInfo;
   DISK_GEOMETRY DiskGeometry;
   FATINFO FatInfo;
   ULONG Size;
   ULONG Sectors;
   struct _BootSector* Boot;

   *RecognizedFS = FALSE;

   Size = sizeof(DISK_GEOMETRY);
   Status = VfatBlockDeviceIoControl(DeviceToMount,
				     IOCTL_DISK_GET_DRIVE_GEOMETRY,
				     NULL,
				     0,
				     &DiskGeometry,
				     &Size);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("VfatBlockDeviceIoControl faild (%x)\n", Status);
      return Status;
   }
   if (DiskGeometry.MediaType == FixedMedia)
   {
      // We have found a hard disk
      Size = sizeof(PARTITION_INFORMATION);
      Status = VfatBlockDeviceIoControl(DeviceToMount,
					IOCTL_DISK_GET_PARTITION_INFO,
					NULL,
					0,
					&PartitionInfo,
					&Size);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("VfatBlockDeviceIoControl faild (%x)\n", Status);
         return Status;
      }
#ifndef NDEBUG
      DbgPrint("Partition Information:\n");
      DbgPrint("StartingOffset      %u\n", PartitionInfo.StartingOffset.QuadPart  / 512);
      DbgPrint("PartitionLength     %u\n", PartitionInfo.PartitionLength.QuadPart / 512);
      DbgPrint("HiddenSectors       %u\n", PartitionInfo.HiddenSectors);
      DbgPrint("PartitionNumber     %u\n", PartitionInfo.PartitionNumber);
      DbgPrint("PartitionType       %u\n", PartitionInfo.PartitionType);
      DbgPrint("BootIndicator       %u\n", PartitionInfo.BootIndicator);
      DbgPrint("RecognizedPartition %u\n", PartitionInfo.RecognizedPartition);
      DbgPrint("RewritePartition    %u\n", PartitionInfo.RewritePartition);
#endif
      if (PartitionInfo.PartitionType == PARTITION_FAT_12       ||
          PartitionInfo.PartitionType == PARTITION_FAT_16       ||
          PartitionInfo.PartitionType == PARTITION_HUGE         ||
          PartitionInfo.PartitionType == PARTITION_FAT32        ||
          PartitionInfo.PartitionType == PARTITION_FAT32_XINT13 ||
          PartitionInfo.PartitionType == PARTITION_XINT13)
      {
         *RecognizedFS = TRUE;
      }
   }
   else if (DiskGeometry.MediaType > Unknown && DiskGeometry.MediaType <= RemovableMedia)
   {
      *RecognizedFS = TRUE;
   }
   if (*RecognizedFS == FALSE)
   {
      return STATUS_SUCCESS;
   }
ReadSector:
   Boot = ExAllocatePool(NonPagedPool, BLOCKSIZE);
   if (Boot == NULL)
   {
      *RecognizedFS=FALSE;
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   Status = VfatReadSectors(DeviceToMount, 0, 1, (PUCHAR) Boot);
   if (NT_SUCCESS(Status))
   {
      FatInfo.VolumeID = Boot->VolumeID;
      FatInfo.FATStart = Boot->ReservedSectors;
      FatInfo.FATCount = Boot->FATCount;
      FatInfo.FATSectors = Boot->FATSectors ? Boot->FATSectors : ((struct _BootSector32*) Boot)->FATSectors32;
      FatInfo.BytesPerSector = Boot->BytesPerSector;
      FatInfo.SectorsPerCluster = Boot->SectorsPerCluster;
      FatInfo.BytesPerCluster = FatInfo.BytesPerSector * FatInfo.SectorsPerCluster;
      FatInfo.rootDirectorySectors = ((Boot->RootEntries * 32) + Boot->BytesPerSector - 1) / Boot->BytesPerSector;
      FatInfo.rootStart = FatInfo.FATStart + FatInfo.FATCount * FatInfo.FATSectors;
      FatInfo.dataStart = FatInfo.rootStart + FatInfo.rootDirectorySectors;
      FatInfo.Sectors = Sectors = Boot->Sectors ? Boot->Sectors : Boot->SectorsHuge;
      Sectors -= Boot->ReservedSectors + FatInfo.FATCount * FatInfo.FATSectors + FatInfo.rootDirectorySectors;
      FatInfo.NumberOfClusters = Sectors / Boot->SectorsPerCluster;
      if (FatInfo.NumberOfClusters < 4085)
      {
         DPRINT("FAT12\n");
         FatInfo.FatType = FAT12;
      }
      else if (FatInfo.NumberOfClusters >= 65525)
      {
         DPRINT("FAT32\n");
         FatInfo.FatType = FAT32;
         FatInfo.RootCluster = ((struct _BootSector32*) Boot)->RootCluster;
         FatInfo.rootStart = FatInfo.dataStart + ((FatInfo.RootCluster - 2) * FatInfo.SectorsPerCluster);
         FatInfo.VolumeID = ((struct _BootSector32*) Boot)->VolumeID;
      }
      else
      {
         DPRINT("FAT16\n");
         FatInfo.FatType = FAT16;
      }
      if (pFatInfo)
      {
         *pFatInfo = FatInfo;
      }
   }
   ExFreePool(Boot);
   return Status;
}

static NTSTATUS
VfatMountDevice(PDEVICE_EXTENSION DeviceExt,
		PDEVICE_OBJECT DeviceToMount)
/*
 * FUNCTION: Mounts the device
 */
{
   NTSTATUS Status;
   BOOLEAN RecognizedFS;

   DPRINT("Mounting VFAT device...\n");

   Status = VfatHasFileSystem(DeviceToMount, &RecognizedFS, &DeviceExt->FatInfo);
   if (!NT_SUCCESS(Status))
   {
      return(Status);
   }

   if (DeviceExt->FatInfo.BytesPerCluster >= PAGESIZE &&
      (DeviceExt->FatInfo.BytesPerCluster % PAGESIZE) != 0)
   {
      DbgPrint("(%s:%d) Invalid cluster size\n", __FILE__, __LINE__);
      KeBugCheck(0);
   }
   else if (DeviceExt->FatInfo.BytesPerCluster < PAGESIZE &&
      (PAGESIZE % DeviceExt->FatInfo.BytesPerCluster) != 0)
   {
      DbgPrint("(%s:%d) Invalid cluster size2\n", __FILE__, __LINE__);
      KeBugCheck(0);
   }

   return(STATUS_SUCCESS);
}


static NTSTATUS
VfatMount (PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Mount the filesystem
 */
{
   PDEVICE_OBJECT DeviceObject = NULL;
   PDEVICE_EXTENSION DeviceExt = NULL;
   BOOLEAN RecognizedFS;
   NTSTATUS Status;
   PVFATFCB Fcb = NULL;
   PVFATFCB VolumeFcb = NULL;
   PVFATCCB Ccb = NULL;
   LARGE_INTEGER timeout;

   DPRINT("VfatMount(IrpContext %x)\n", IrpContext);

   assert (IrpContext);

   if (IrpContext->DeviceObject != VfatGlobalData->DeviceObject)
   {
      Status = STATUS_INVALID_DEVICE_REQUEST;
      goto ByeBye;
   }

   Status = VfatHasFileSystem (IrpContext->Stack->Parameters.MountVolume.DeviceObject, &RecognizedFS, NULL);
   if (!NT_SUCCESS(Status))
   {
      goto ByeBye;
   }

   if (RecognizedFS == FALSE)
   {
      DPRINT("VFAT: Unrecognized Volume\n");
      Status = STATUS_UNRECOGNIZED_VOLUME;
      goto ByeBye;
   }

   DPRINT("VFAT: Recognized volume\n");
   Status = IoCreateDevice(VfatGlobalData->DriverObject,
			   sizeof (DEVICE_EXTENSION),
			   NULL,
			   FILE_DEVICE_FILE_SYSTEM,
			   0,
			   FALSE,
			   &DeviceObject);
   if (!NT_SUCCESS(Status))
   {
      goto ByeBye;
   }

   DeviceObject->Flags = DeviceObject->Flags | DO_DIRECT_IO;
   DeviceExt = (PVOID) DeviceObject->DeviceExtension;
   RtlZeroMemory(DeviceExt, sizeof(DEVICE_EXTENSION));

   /* use same vpb as device disk */
   DeviceObject->Vpb = IrpContext->Stack->Parameters.MountVolume.DeviceObject->Vpb;
   Status = VfatMountDevice(DeviceExt, IrpContext->Stack->Parameters.MountVolume.DeviceObject);
   if (!NT_SUCCESS(Status))
   {
      /* FIXME: delete device object */
      goto ByeBye;
   }

#ifndef NDEBUG
   DbgPrint("BytesPerSector:     %d\n", DeviceExt->FatInfo.BytesPerSector);
   DbgPrint("SectorsPerCluster:  %d\n", DeviceExt->FatInfo.SectorsPerCluster);
   DbgPrint("FATCount:           %d\n", DeviceExt->FatInfo.FATCount);
   DbgPrint("FATSectors:         %d\n", DeviceExt->FatInfo.FATSectors);
   DbgPrint("RootStart:          %d\n", DeviceExt->FatInfo.rootStart);
   DbgPrint("DataStart:          %d\n", DeviceExt->FatInfo.dataStart);
   if (DeviceExt->FatInfo.FatType == FAT32)
   {
      DbgPrint("RootCluster:        %d\n", DeviceExt->FatInfo.RootCluster);
   }
#endif

  DeviceExt->StorageDevice = IrpContext->Stack->Parameters.MountVolume.DeviceObject;
  DeviceExt->StorageDevice->Vpb->DeviceObject = DeviceObject;
  DeviceExt->StorageDevice->Vpb->RealDevice = DeviceExt->StorageDevice;
  DeviceExt->StorageDevice->Vpb->Flags |= VPB_MOUNTED;
  DeviceObject->StackSize = DeviceExt->StorageDevice->StackSize + 1;
  DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  DPRINT("FsDeviceObject %lx\n", DeviceObject);

   DeviceExt->FATFileObject = IoCreateStreamFileObject(NULL, DeviceExt->StorageDevice);
   Fcb = vfatNewFCB(NULL);
   if (Fcb == NULL)
   {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
   }
   Ccb = ExAllocatePoolWithTag (NonPagedPool, sizeof (VFATCCB), TAG_CCB);
   if (Ccb == NULL)
   {
      Status =  STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
   }
   memset(Ccb, 0, sizeof (VFATCCB));
   wcscpy(Fcb->PathName, L"$$Fat$$");
   Fcb->ObjectName = Fcb->PathName;
   DeviceExt->FATFileObject->Flags = DeviceExt->FATFileObject->Flags | FO_FCB_IS_VALID | FO_DIRECT_CACHE_PAGING_READ;
   DeviceExt->FATFileObject->FsContext = (PVOID) &Fcb->RFCB;
   DeviceExt->FATFileObject->FsContext2 = Ccb;
   DeviceExt->FATFileObject->SectionObjectPointers = &Fcb->SectionObjectPointers;
   DeviceExt->FATFileObject->PrivateCacheMap = NULL;
   DeviceExt->FATFileObject->Vpb = DeviceObject->Vpb;
   Ccb->pFcb = Fcb;
   Ccb->PtrFileObject = DeviceExt->FATFileObject;
   Fcb->FileObject = DeviceExt->FATFileObject;
   Fcb->pDevExt = (PDEVICE_EXTENSION)DeviceExt->StorageDevice;

   Fcb->Flags = FCB_IS_FAT;

   Fcb->RFCB.FileSize.QuadPart = DeviceExt->FatInfo.FATSectors * BLOCKSIZE;
   Fcb->RFCB.ValidDataLength = Fcb->RFCB.FileSize;
   Fcb->RFCB.AllocationSize = Fcb->RFCB.FileSize;

   if (DeviceExt->FatInfo.FatType != FAT12)
   {
      Status = CcRosInitializeFileCache(DeviceExt->FATFileObject, &Fcb->RFCB.Bcb, CACHEPAGESIZE(DeviceExt));
   }
   else
   {
      Status = CcRosInitializeFileCache(DeviceExt->FATFileObject, &Fcb->RFCB.Bcb, 2 * PAGESIZE);
   }
   if (!NT_SUCCESS (Status))
   {
      DbgPrint ("CcRosInitializeFileCache failed\n");
      goto ByeBye;
   }
   DeviceExt->LastAvailableCluster = 0;
   ExInitializeResourceLite(&DeviceExt->DirResource);
   ExInitializeResourceLite(&DeviceExt->FatResource);

   KeInitializeSpinLock(&DeviceExt->FcbListLock);
   InitializeListHead(&DeviceExt->FcbListHead);

   VolumeFcb = vfatNewFCB(NULL);
   if (VolumeFcb == NULL)
   {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
   }
   wcscpy(VolumeFcb->PathName, L"$$Volume$$");
   VolumeFcb->ObjectName = VolumeFcb->PathName;
   VolumeFcb->Flags = FCB_IS_VOLUME;
   VolumeFcb->RFCB.FileSize.QuadPart = DeviceExt->FatInfo.Sectors * BLOCKSIZE;
   VolumeFcb->RFCB.ValidDataLength = VolumeFcb->RFCB.FileSize;
   VolumeFcb->RFCB.AllocationSize = VolumeFcb->RFCB.FileSize;
   VolumeFcb->pDevExt = (PDEVICE_EXTENSION)DeviceExt->StorageDevice;
   DeviceExt->VolumeFcb = VolumeFcb;

   /* read serial number */
   DeviceObject->Vpb->SerialNumber = DeviceExt->FatInfo.VolumeID;

   /* read volume label */
   ReadVolumeLabel(DeviceExt,  DeviceObject->Vpb);

   Status = STATUS_SUCCESS;
ByeBye:

  if (!NT_SUCCESS(Status))
  {
     // cleanup
     if (DeviceExt && DeviceExt->FATFileObject)
        ObDereferenceObject (DeviceExt->FATFileObject);
     if (Fcb)
        ExFreePool(Fcb);
     if (Ccb)
        ExFreePool(Ccb);
     if (DeviceObject)
       IoDeleteDevice(DeviceObject);
     if (VolumeFcb)
        vfatDestroyFCB(VolumeFcb);
  }
  return Status;
}


static NTSTATUS
VfatVerify (PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Mount the filesystem
 */
{
  DPRINT("VfatVerify(IrpContext %x)\n", IrpContext);

  assert(IrpContext);

  return(STATUS_INVALID_DEVICE_REQUEST);
}


NTSTATUS VfatFileSystemControl(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: File system control
 */
{

   NTSTATUS Status;

   DPRINT("VfatFileSystemControl(IrpContext %x)\n", IrpContext);

   assert (IrpContext);

   switch (IrpContext->MinorFunction)
   {
      case IRP_MN_USER_FS_REQUEST:
         DPRINT("VFAT FSC: IRP_MN_USER_FS_REQUEST\n");
	 Status = STATUS_INVALID_DEVICE_REQUEST;
	 break;

      case IRP_MN_MOUNT_VOLUME:
         Status = VfatMount(IrpContext);
	 break;

      case IRP_MN_VERIFY_VOLUME:
	 Status = VfatVerify(IrpContext);
	 break;

      default:
	   DPRINT("VFAT FSC: MinorFunction %d\n", IrpContext->MinorFunction);
	   Status = STATUS_INVALID_DEVICE_REQUEST;
	   break;
   }

   IrpContext->Irp->IoStatus.Status = Status;
   IrpContext->Irp->IoStatus.Information = 0;

   IoCompleteRequest (IrpContext->Irp, IO_NO_INCREMENT);
   VfatFreeIrpContext(IrpContext);
   return (Status);
}

