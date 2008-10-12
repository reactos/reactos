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
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystems/fastfat/fsctl.c
 * PURPOSE:          VFAT Filesystem
 */

/* INCLUDES *****************************************************************/

#define NDEBUG
#include "vfat.h"

/* FUNCTIONS ****************************************************************/

#define  CACHEPAGESIZE(pDeviceExt) ((pDeviceExt)->FatInfo.BytesPerCluster > PAGE_SIZE ? \
                                    (pDeviceExt)->FatInfo.BytesPerCluster : PAGE_SIZE)

#define VOLUME_IS_DIRTY 0x00000001

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
   LARGE_INTEGER Offset;
   struct _BootSector* Boot;
   struct _BootSectorFatX* BootFatX;
   BOOLEAN PartitionInfoIsValid = FALSE;

   DPRINT("VfatHasFileSystem\n");

   *RecognizedFS = FALSE;

   Size = sizeof(DISK_GEOMETRY);
   Status = VfatBlockDeviceIoControl(DeviceToMount,
                                     IOCTL_DISK_GET_DRIVE_GEOMETRY,
                                     NULL,
                                     0,
                                     &DiskGeometry,
                                     &Size,
                                     FALSE);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("VfatBlockDeviceIoControl faild (%x)\n", Status);
      return Status;
   }
   FatInfo.FixedMedia = DiskGeometry.MediaType == FixedMedia ? TRUE : FALSE;
   if (DiskGeometry.MediaType == FixedMedia || DiskGeometry.MediaType == RemovableMedia)
   {
      // We have found a hard disk
      Size = sizeof(PARTITION_INFORMATION);
      Status = VfatBlockDeviceIoControl(DeviceToMount,
                                        IOCTL_DISK_GET_PARTITION_INFO,
                                        NULL,
                                        0,
                                        &PartitionInfo,
                                        &Size,
                                        FALSE);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("VfatBlockDeviceIoControl faild (%x)\n", Status);
         return Status;
      }
      PartitionInfoIsValid = TRUE;
      DPRINT("Partition Information:\n");
      DPRINT("StartingOffset      %u\n", PartitionInfo.StartingOffset.QuadPart  / 512);
      DPRINT("PartitionLength     %u\n", PartitionInfo.PartitionLength.QuadPart / 512);
      DPRINT("HiddenSectors       %u\n", PartitionInfo.HiddenSectors);
      DPRINT("PartitionNumber     %u\n", PartitionInfo.PartitionNumber);
      DPRINT("PartitionType       %u\n", PartitionInfo.PartitionType);
      DPRINT("BootIndicator       %u\n", PartitionInfo.BootIndicator);
      DPRINT("RecognizedPartition %u\n", PartitionInfo.RecognizedPartition);
      DPRINT("RewritePartition    %u\n", PartitionInfo.RewritePartition);
      if (PartitionInfo.PartitionType)
      {
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
      else if (DiskGeometry.MediaType == RemovableMedia &&
               PartitionInfo.PartitionNumber > 0 &&
               PartitionInfo.StartingOffset.QuadPart == 0 &&
               PartitionInfo.PartitionLength.QuadPart > 0)
      {
         /* This is possible a removable media formated as super floppy */
         *RecognizedFS = TRUE;
      }
   }
   else if (DiskGeometry.MediaType == Unknown)
   {
      /*
       * Floppy disk driver can return Unknown as media type if it
       * doesn't know yet what floppy in the drive really is. This is
       * perfectly correct to do under Windows.
       */
      *RecognizedFS = TRUE;
      DiskGeometry.BytesPerSector = 512;
   }
   else
   {
      *RecognizedFS = TRUE;
   }
   if (*RecognizedFS)
   {

      Boot = ExAllocatePoolWithTag(NonPagedPool, DiskGeometry.BytesPerSector, TAG_VFAT);
      if (Boot == NULL)
      {
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      Offset.QuadPart = 0;

      /* Try to recognize FAT12/FAT16/FAT32 partitions */
      Status = VfatReadDisk(DeviceToMount, &Offset, DiskGeometry.BytesPerSector, (PUCHAR) Boot, FALSE);
      if (NT_SUCCESS(Status))
      {
         if (Boot->Signatur1 != 0xaa55)
         {
            *RecognizedFS = FALSE;
         }
         if (*RecognizedFS &&
         Boot->BytesPerSector != 512 &&
         Boot->BytesPerSector != 1024 &&
             Boot->BytesPerSector != 2048 &&
         Boot->BytesPerSector != 4096)
         {
            DPRINT1("BytesPerSector %d\n", Boot->BytesPerSector);
            *RecognizedFS = FALSE;
         }

         if (*RecognizedFS &&
             Boot->FATCount != 1 &&
             Boot->FATCount != 2)
         {
            DPRINT1("FATCount %d\n", Boot->FATCount);
            *RecognizedFS = FALSE;
         }

         if (*RecognizedFS &&
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
            DPRINT1("Media             %02x\n", Boot->Media);
            *RecognizedFS = FALSE;
         }

         if (*RecognizedFS &&
             Boot->SectorsPerCluster != 1 &&
             Boot->SectorsPerCluster != 2 &&
             Boot->SectorsPerCluster != 4 &&
             Boot->SectorsPerCluster != 8 &&
             Boot->SectorsPerCluster != 16 &&
             Boot->SectorsPerCluster != 32 &&
             Boot->SectorsPerCluster != 64 &&
             Boot->SectorsPerCluster != 128)
         {
            DPRINT1("SectorsPerCluster %02x\n", Boot->SectorsPerCluster);
            *RecognizedFS = FALSE;
         }

         if (*RecognizedFS &&
             Boot->BytesPerSector * Boot->SectorsPerCluster > 32 * 1024)
         {
            DPRINT1("ClusterSize %dx\n", Boot->BytesPerSector * Boot->SectorsPerCluster);
            *RecognizedFS = FALSE;
         }

         if (*RecognizedFS)
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
               FatInfo.RootCluster = (FatInfo.rootStart - 1) / FatInfo.SectorsPerCluster;
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
               FatInfo.RootCluster = FatInfo.rootStart / FatInfo.SectorsPerCluster;
            }
            if (PartitionInfoIsValid &&
                FatInfo.Sectors > PartitionInfo.PartitionLength.QuadPart / FatInfo.BytesPerSector)
            {
               CHECKPOINT1;
               *RecognizedFS = FALSE;
            }

            if (pFatInfo && *RecognizedFS)
            {
               *pFatInfo = FatInfo;
            }
         }
      }

      ExFreePool(Boot);
   }

   if (!*RecognizedFS && PartitionInfoIsValid)
   {
      BootFatX = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _BootSectorFatX), TAG_VFAT);
      if (BootFatX == NULL)
      {
         *RecognizedFS=FALSE;
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      Offset.QuadPart = 0;

      /* Try to recognize FATX16/FATX32 partitions (Xbox) */
      Status = VfatReadDisk(DeviceToMount, &Offset, sizeof(struct _BootSectorFatX), (PUCHAR) BootFatX, FALSE);
      if (NT_SUCCESS(Status))
      {
         *RecognizedFS = TRUE;
         if (BootFatX->SysType[0] != 'F' ||
             BootFatX->SysType[1] != 'A' ||
             BootFatX->SysType[2] != 'T' ||
             BootFatX->SysType[3] != 'X')
         {
            DPRINT1("SysType %c%c%c%c\n", BootFatX->SysType[0], BootFatX->SysType[1], BootFatX->SysType[2], BootFatX->SysType[3]);
            *RecognizedFS=FALSE;
         }

         if (*RecognizedFS &&
            BootFatX->SectorsPerCluster != 1 &&
            BootFatX->SectorsPerCluster != 2 &&
            BootFatX->SectorsPerCluster != 4 &&
            BootFatX->SectorsPerCluster != 8 &&
            BootFatX->SectorsPerCluster != 16 &&
            BootFatX->SectorsPerCluster != 32 &&
            BootFatX->SectorsPerCluster != 64 &&
            BootFatX->SectorsPerCluster != 128)
         {
            DPRINT1("SectorsPerCluster %lu\n", BootFatX->SectorsPerCluster);
            *RecognizedFS=FALSE;
         }

         if (*RecognizedFS)
         {
            FatInfo.BytesPerSector = DiskGeometry.BytesPerSector;
            FatInfo.SectorsPerCluster = BootFatX->SectorsPerCluster;
            FatInfo.rootDirectorySectors = BootFatX->SectorsPerCluster;
            FatInfo.BytesPerCluster = BootFatX->SectorsPerCluster * DiskGeometry.BytesPerSector;
            FatInfo.Sectors = (ULONG)(PartitionInfo.PartitionLength.QuadPart / DiskGeometry.BytesPerSector);
            if (FatInfo.Sectors / FatInfo.SectorsPerCluster < 65525)
            {
               DPRINT("FATX16\n");
               FatInfo.FatType = FATX16;
            }
            else
            {
               DPRINT("FATX32\n");
               FatInfo.FatType = FATX32;
            }
            FatInfo.VolumeID = BootFatX->VolumeID;
            FatInfo.FATStart = sizeof(struct _BootSectorFatX) / DiskGeometry.BytesPerSector;
            FatInfo.FATCount = BootFatX->FATCount;
            FatInfo.FATSectors =
                  ROUND_UP(FatInfo.Sectors / FatInfo.SectorsPerCluster * (FatInfo.FatType == FATX16 ? 2 : 4), 4096) /
                  FatInfo.BytesPerSector;
            FatInfo.rootStart = FatInfo.FATStart + FatInfo.FATCount * FatInfo.FATSectors;
            FatInfo.RootCluster = (FatInfo.rootStart - 1) / FatInfo.SectorsPerCluster;
            FatInfo.dataStart = FatInfo.rootStart + FatInfo.rootDirectorySectors;
            FatInfo.NumberOfClusters = (FatInfo.Sectors - FatInfo.dataStart) / FatInfo.SectorsPerCluster;

            if (pFatInfo && *RecognizedFS)
            {
               *pFatInfo = FatInfo;
            }
         }
      }
      ExFreePool(BootFatX);
   }

   DPRINT("VfatHasFileSystem done\n");
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
   DPRINT("MountVfatdev %d, PAGE_SIZE = %d\n", DeviceExt->FatInfo.BytesPerCluster, PAGE_SIZE);


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
   PDEVICE_OBJECT DeviceToMount;
   UNICODE_STRING NameU = RTL_CONSTANT_STRING(L"\\$$Fat$$");
   UNICODE_STRING VolumeNameU = RTL_CONSTANT_STRING(L"\\$$Volume$$");
   ULONG HashTableSize;
   ULONG eocMark;
   FATINFO FatInfo;

   DPRINT("VfatMount(IrpContext %p)\n", IrpContext);

   ASSERT(IrpContext);

   if (IrpContext->DeviceObject != VfatGlobalData->DeviceObject)
   {
      Status = STATUS_INVALID_DEVICE_REQUEST;
      goto ByeBye;
   }

   DeviceToMount = IrpContext->Stack->Parameters.MountVolume.DeviceObject;

   Status = VfatHasFileSystem (DeviceToMount, &RecognizedFS, &FatInfo);
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

   /* Use prime numbers for the table size */
   if (FatInfo.FatType == FAT12)
   {
      HashTableSize = 4099; // 4096 = 4 * 1024
   }
   else if (FatInfo.FatType == FAT16 ||
            FatInfo.FatType == FATX16)
   {
      HashTableSize = 16411; // 16384 = 16 * 1024
   }
   else
   {
      HashTableSize = 65537; // 65536 = 64 * 1024;
   }
   HashTableSize = FCB_HASH_TABLE_SIZE;
   DPRINT("VFAT: Recognized volume\n");
   Status = IoCreateDevice(VfatGlobalData->DriverObject,
                           ROUND_UP(sizeof (DEVICE_EXTENSION), sizeof(ULONG)) + sizeof(HASHENTRY*) * HashTableSize,
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
   RtlZeroMemory(DeviceExt, ROUND_UP(sizeof(DEVICE_EXTENSION), sizeof(ULONG)) + sizeof(HASHENTRY*) * HashTableSize);
   DeviceExt->FcbHashTable = (HASHENTRY**)((ULONG_PTR)DeviceExt + ROUND_UP(sizeof(DEVICE_EXTENSION), sizeof(ULONG)));
   DeviceExt->HashTableSize = HashTableSize;

   /* use same vpb as device disk */
   DeviceObject->Vpb = DeviceToMount->Vpb;
   Status = VfatMountDevice(DeviceExt, DeviceToMount);
   if (!NT_SUCCESS(Status))
   {
      /* FIXME: delete device object */
      goto ByeBye;
   }

   DPRINT("BytesPerSector:     %d\n", DeviceExt->FatInfo.BytesPerSector);
   DPRINT("SectorsPerCluster:  %d\n", DeviceExt->FatInfo.SectorsPerCluster);
   DPRINT("FATCount:           %d\n", DeviceExt->FatInfo.FATCount);
   DPRINT("FATSectors:         %d\n", DeviceExt->FatInfo.FATSectors);
   DPRINT("RootStart:          %d\n", DeviceExt->FatInfo.rootStart);
   DPRINT("DataStart:          %d\n", DeviceExt->FatInfo.dataStart);
   if (DeviceExt->FatInfo.FatType == FAT32)
   {
      DPRINT("RootCluster:        %d\n", DeviceExt->FatInfo.RootCluster);
   }

   switch (DeviceExt->FatInfo.FatType)
   {
      case FAT12:
         DeviceExt->GetNextCluster = FAT12GetNextCluster;
         DeviceExt->FindAndMarkAvailableCluster = FAT12FindAndMarkAvailableCluster;
         DeviceExt->WriteCluster = FAT12WriteCluster;
         DeviceExt->CleanShutBitMask = 0;
         break;

      case FAT16:
      case FATX16:
         DeviceExt->GetNextCluster = FAT16GetNextCluster;
         DeviceExt->FindAndMarkAvailableCluster = FAT16FindAndMarkAvailableCluster;
         DeviceExt->WriteCluster = FAT16WriteCluster;
         DeviceExt->CleanShutBitMask = 0x8000;
         break;

      case FAT32:
      case FATX32:
         DeviceExt->GetNextCluster = FAT32GetNextCluster;
         DeviceExt->FindAndMarkAvailableCluster = FAT32FindAndMarkAvailableCluster;
         DeviceExt->WriteCluster = FAT32WriteCluster;
         DeviceExt->CleanShutBitMask = 0x80000000;
         break;
   }

   if (DeviceExt->FatInfo.FatType == FATX16
      || DeviceExt->FatInfo.FatType == FATX32)
   {
      DeviceExt->Flags |= VCB_IS_FATX;
      DeviceExt->GetNextDirEntry = FATXGetNextDirEntry;
      DeviceExt->BaseDateYear = 2000;
   }
   else
   {
      DeviceExt->GetNextDirEntry = FATGetNextDirEntry;
      DeviceExt->BaseDateYear = 1980;
   }

   DeviceExt->StorageDevice = DeviceToMount;
   DeviceExt->StorageDevice->Vpb->DeviceObject = DeviceObject;
   DeviceExt->StorageDevice->Vpb->RealDevice = DeviceExt->StorageDevice;
   DeviceExt->StorageDevice->Vpb->Flags |= VPB_MOUNTED;
   DeviceObject->StackSize = DeviceExt->StorageDevice->StackSize + 1;
   DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   DPRINT("FsDeviceObject %p\n", DeviceObject);

   /* Initialize this resource early ... it's used in VfatCleanup */
   ExInitializeResourceLite(&DeviceExt->DirResource);

   DeviceExt->FATFileObject = IoCreateStreamFileObject(NULL, DeviceExt->StorageDevice);
   Fcb = vfatNewFCB(DeviceExt, &NameU);
   if (Fcb == NULL)
   {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
   }
   Ccb = ExAllocateFromNPagedLookasideList(&VfatGlobalData->CcbLookasideList);
   if (Ccb == NULL)
   {
      Status =  STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
   }

   RtlZeroMemory(Ccb, sizeof (VFATCCB));
   DeviceExt->FATFileObject->FsContext = Fcb;
   DeviceExt->FATFileObject->FsContext2 = Ccb;
   DeviceExt->FATFileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
   DeviceExt->FATFileObject->PrivateCacheMap = NULL;
   DeviceExt->FATFileObject->Vpb = DeviceObject->Vpb;
   Fcb->FileObject = DeviceExt->FATFileObject;

   Fcb->Flags |= FCB_IS_FAT;

   Fcb->RFCB.FileSize.QuadPart = DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector;
   Fcb->RFCB.ValidDataLength = Fcb->RFCB.FileSize;
   Fcb->RFCB.AllocationSize = Fcb->RFCB.FileSize;

   CcInitializeCacheMap(DeviceExt->FATFileObject,
                        (PCC_FILE_SIZES)(&Fcb->RFCB.AllocationSize),
                        FALSE,
                        &VfatGlobalData->CacheMgrCallbacks,
                        Fcb);

   DeviceExt->LastAvailableCluster = 2;
   ExInitializeResourceLite(&DeviceExt->FatResource);

   InitializeListHead(&DeviceExt->FcbListHead);

   VolumeFcb = vfatNewFCB(DeviceExt, &VolumeNameU);
   if (VolumeFcb == NULL)
   {
      Status = STATUS_INSUFFICIENT_RESOURCES;
      goto ByeBye;
   }
   VolumeFcb->Flags = FCB_IS_VOLUME;
   VolumeFcb->RFCB.FileSize.QuadPart = DeviceExt->FatInfo.Sectors * DeviceExt->FatInfo.BytesPerSector;
   VolumeFcb->RFCB.ValidDataLength = VolumeFcb->RFCB.FileSize;
   VolumeFcb->RFCB.AllocationSize = VolumeFcb->RFCB.FileSize;
   DeviceExt->VolumeFcb = VolumeFcb;

   ExAcquireResourceExclusiveLite(&VfatGlobalData->VolumeListLock, TRUE);
   InsertHeadList(&VfatGlobalData->VolumeListHead, &DeviceExt->VolumeListEntry);
   ExReleaseResourceLite(&VfatGlobalData->VolumeListLock);

   /* read serial number */
   DeviceObject->Vpb->SerialNumber = DeviceExt->FatInfo.VolumeID;

   /* read volume label */
   ReadVolumeLabel(DeviceExt,  DeviceObject->Vpb);

   /* read clean shutdown bit status */
   Status = GetNextCluster(DeviceExt, 1, &eocMark);
   if (NT_SUCCESS(Status))
   {
      if (eocMark & DeviceExt->CleanShutBitMask)
      {
         /* unset clean shutdown bit */
         eocMark &= ~DeviceExt->CleanShutBitMask;
         WriteCluster(DeviceExt, 1, eocMark);
         VolumeFcb->Flags |= VCB_CLEAR_DIRTY;
      }
   }
   VolumeFcb->Flags |= VCB_IS_DIRTY;

   Status = STATUS_SUCCESS;

   /* Initialize notify sync */
   _SEH_TRY
   {
      InitializeListHead(&(DeviceExt->NotifyList));
      FsRtlNotifyInitializeSync(&(DeviceExt->NotifySync));
   }
   _SEH_HANDLE
   {
      Status = _SEH_GetExceptionCode();
   }
   _SEH_END;

ByeBye:

  if (!NT_SUCCESS(Status))
  {
     // cleanup
     if (DeviceExt && DeviceExt->FATFileObject)
        ObDereferenceObject(DeviceExt->FATFileObject);
     if (Fcb)
        vfatDestroyFCB(Fcb);
     if (Ccb)
        vfatDestroyCCB(Ccb);
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
 * FUNCTION: Verify the filesystem
 */
{
  PDEVICE_OBJECT DeviceToVerify;
  NTSTATUS Status = STATUS_SUCCESS;
  FATINFO FatInfo;
  BOOLEAN RecognizedFS;
  PDEVICE_EXTENSION DeviceExt = IrpContext->DeviceExt;

  DPRINT("VfatVerify(IrpContext %p)\n", IrpContext);

  DeviceToVerify = IrpContext->Stack->Parameters.VerifyVolume.DeviceObject;
  Status = VfatBlockDeviceIoControl(DeviceToVerify,
                                    IOCTL_DISK_CHECK_VERIFY,
                                    NULL,
                                    0,
                                    NULL,
                                    0,
                                    TRUE);
  DeviceToVerify->Flags &= ~DO_VERIFY_VOLUME;
  if (!NT_SUCCESS(Status) && Status != STATUS_VERIFY_REQUIRED)
    {
      DPRINT("VfatBlockDeviceIoControl() failed (Status %lx)\n", Status);
      Status = STATUS_WRONG_VOLUME;
    }
  else
    {
      Status = VfatHasFileSystem(DeviceToVerify, &RecognizedFS, &FatInfo);
      if (!NT_SUCCESS(Status) || RecognizedFS == FALSE)
        {
          Status = STATUS_WRONG_VOLUME;
        }
      else if (sizeof(FATINFO) == RtlCompareMemory(&FatInfo, &DeviceExt->FatInfo, sizeof(FATINFO)))
        {
          /*
           * FIXME:
           *   Preformated floppy disks have very often a serial number of 0000:0000.
           *   We should calculate a crc sum over the sectors from the root directory as secondary volume number.
           *   Each write to the root directory must update this crc sum.
           */

        }
      else
        {
          Status = STATUS_WRONG_VOLUME;
        }
     }

  return Status;
}


static NTSTATUS
VfatGetVolumeBitmap(PVFAT_IRP_CONTEXT IrpContext)
{
   DPRINT("VfatGetVolumeBitmap (IrpContext %p)\n", IrpContext);

   return STATUS_INVALID_DEVICE_REQUEST;
}


static NTSTATUS
VfatGetRetrievalPointers(PVFAT_IRP_CONTEXT IrpContext)
{
   PIO_STACK_LOCATION Stack;
   LARGE_INTEGER Vcn;
   PRETRIEVAL_POINTERS_BUFFER RetrievalPointers;
   PFILE_OBJECT FileObject;
   ULONG MaxExtentCount;
   PVFATFCB Fcb;
   PDEVICE_EXTENSION DeviceExt;
   ULONG FirstCluster;
   ULONG CurrentCluster;
   ULONG LastCluster;
   NTSTATUS Status;

   DPRINT("VfatGetRetrievalPointers(IrpContext %p)\n", IrpContext);

   DeviceExt = IrpContext->DeviceExt;
   FileObject = IrpContext->FileObject;
   Stack = IrpContext->Stack;
   if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(STARTING_VCN_INPUT_BUFFER) ||
       Stack->Parameters.DeviceIoControl.Type3InputBuffer == NULL)
   {
      return STATUS_INVALID_PARAMETER;
   }
   if (IrpContext->Irp->UserBuffer == NULL ||
       Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(RETRIEVAL_POINTERS_BUFFER))
   {
      return STATUS_BUFFER_TOO_SMALL;
   }

   Fcb = FileObject->FsContext;

   ExAcquireResourceSharedLite(&Fcb->MainResource, TRUE);

   Vcn = ((PSTARTING_VCN_INPUT_BUFFER)Stack->Parameters.DeviceIoControl.Type3InputBuffer)->StartingVcn;
   RetrievalPointers = IrpContext->Irp->UserBuffer;

   MaxExtentCount = ((Stack->Parameters.DeviceIoControl.OutputBufferLength - sizeof(RetrievalPointers->ExtentCount) - sizeof(RetrievalPointers->StartingVcn)) / sizeof(RetrievalPointers->Extents[0]));


   if (Vcn.QuadPart >= Fcb->RFCB.AllocationSize.QuadPart / DeviceExt->FatInfo.BytesPerCluster)
   {
      Status = STATUS_INVALID_PARAMETER;
      goto ByeBye;
   }

   CurrentCluster = FirstCluster = vfatDirEntryGetFirstCluster(DeviceExt, &Fcb->entry);
   Status = OffsetToCluster(DeviceExt, FirstCluster,
                            Vcn.u.LowPart * DeviceExt->FatInfo.BytesPerCluster,
                            &CurrentCluster, FALSE);
   if (!NT_SUCCESS(Status))
   {
      goto ByeBye;
   }

   RetrievalPointers->StartingVcn = Vcn;
   RetrievalPointers->ExtentCount = 0;
   RetrievalPointers->Extents[0].Lcn.u.HighPart = 0;
   RetrievalPointers->Extents[0].Lcn.u.LowPart = CurrentCluster - 2;
   LastCluster = 0;
   while (CurrentCluster != 0xffffffff && RetrievalPointers->ExtentCount < MaxExtentCount)
   {

      LastCluster = CurrentCluster;
      Status = NextCluster(DeviceExt, CurrentCluster, &CurrentCluster, FALSE);
      Vcn.QuadPart++;
      if (!NT_SUCCESS(Status))
      {
         goto ByeBye;
      }

      if (LastCluster + 1 != CurrentCluster)
      {
         RetrievalPointers->Extents[RetrievalPointers->ExtentCount].NextVcn = Vcn;
         RetrievalPointers->ExtentCount++;
         if (RetrievalPointers->ExtentCount < MaxExtentCount)
         {
            RetrievalPointers->Extents[RetrievalPointers->ExtentCount].Lcn.u.HighPart = 0;
            RetrievalPointers->Extents[RetrievalPointers->ExtentCount].Lcn.u.LowPart = CurrentCluster - 2;
         }
      }
   }

   IrpContext->Irp->IoStatus.Information = sizeof(RETRIEVAL_POINTERS_BUFFER) + (sizeof(RetrievalPointers->Extents[0]) * (RetrievalPointers->ExtentCount - 1));
   Status = STATUS_SUCCESS;

ByeBye:
   ExReleaseResourceLite(&Fcb->MainResource);

   return Status;
}

static NTSTATUS
VfatMoveFile(PVFAT_IRP_CONTEXT IrpContext)
{
   DPRINT("VfatMoveFile(IrpContext %p)\n", IrpContext);

   return STATUS_INVALID_DEVICE_REQUEST;
}

#ifdef USE_ROS_CC_AND_FS
static NTSTATUS
VfatRosQueryLcnMapping(PVFAT_IRP_CONTEXT IrpContext)
{
   PDEVICE_EXTENSION DeviceExt;
   PROS_QUERY_LCN_MAPPING LcnQuery;
   PIO_STACK_LOCATION Stack;

   DPRINT("VfatRosQueryLcnMapping(IrpContext %p)\n", IrpContext);

   DeviceExt = IrpContext->DeviceExt;
   Stack = IrpContext->Stack;
   if (IrpContext->Irp->AssociatedIrp.SystemBuffer == NULL ||
       Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ROS_QUERY_LCN_MAPPING))
   {
      return STATUS_BUFFER_TOO_SMALL;
   }
   LcnQuery = (PROS_QUERY_LCN_MAPPING)(IrpContext->Irp->AssociatedIrp.SystemBuffer);
   LcnQuery->LcnDiskOffset.QuadPart = DeviceExt->FatInfo.dataStart * DeviceExt->FatInfo.BytesPerSector;
   IrpContext->Irp->IoStatus.Information = sizeof(ROS_QUERY_LCN_MAPPING);
   return(STATUS_SUCCESS);
}
#endif

static NTSTATUS
VfatIsVolumeDirty(PVFAT_IRP_CONTEXT IrpContext)
{
   PULONG Flags;

   DPRINT("VfatIsVolumeDirty(IrpContext %p)\n", IrpContext);

   if (IrpContext->Stack->Parameters.FileSystemControl.OutputBufferLength != sizeof(ULONG))
      return STATUS_INVALID_BUFFER_SIZE;
   else if (!IrpContext->Irp->AssociatedIrp.SystemBuffer)
      return STATUS_INVALID_USER_BUFFER;

   Flags = (PULONG)IrpContext->Irp->AssociatedIrp.SystemBuffer;
   *Flags = 0;

   if (IrpContext->DeviceExt->VolumeFcb->Flags & VCB_IS_DIRTY
      && !(IrpContext->DeviceExt->VolumeFcb->Flags & VCB_CLEAR_DIRTY))
   {
      *Flags |= VOLUME_IS_DIRTY;
   }

   return STATUS_SUCCESS;
}

static NTSTATUS
VfatMarkVolumeDirty(PVFAT_IRP_CONTEXT IrpContext)
{
   ULONG eocMark;
   PDEVICE_EXTENSION DeviceExt;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("VfatMarkVolumeDirty(IrpContext %p)\n", IrpContext);
   DeviceExt = IrpContext->DeviceExt;

   if (!(DeviceExt->VolumeFcb->Flags & VCB_IS_DIRTY))
   {
      Status = GetNextCluster(DeviceExt, 1, &eocMark);
      if (NT_SUCCESS(Status))
      {
         /* unset clean shutdown bit */
         eocMark &= ~DeviceExt->CleanShutBitMask;
         Status = WriteCluster(DeviceExt, 1, eocMark);
      }
   }

   DeviceExt->VolumeFcb->Flags &= ~VCB_CLEAR_DIRTY;

   return Status;
}

NTSTATUS VfatFileSystemControl(PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: File system control
 */
{

   NTSTATUS Status;

   DPRINT("VfatFileSystemControl(IrpContext %p)\n", IrpContext);

   ASSERT(IrpContext);
   ASSERT(IrpContext->Irp);
   ASSERT(IrpContext->Stack);

   IrpContext->Irp->IoStatus.Information = 0;

   switch (IrpContext->MinorFunction)
   {
      case IRP_MN_USER_FS_REQUEST:
         switch(IrpContext->Stack->Parameters.DeviceIoControl.IoControlCode)
         {
            case FSCTL_GET_VOLUME_BITMAP:
               Status = VfatGetVolumeBitmap(IrpContext);
               break;
            case FSCTL_GET_RETRIEVAL_POINTERS:
               Status = VfatGetRetrievalPointers(IrpContext);
               break;
            case FSCTL_MOVE_FILE:
               Status = VfatMoveFile(IrpContext);
               break;
#ifdef USE_ROS_CC_AND_FS
            case FSCTL_ROS_QUERY_LCN_MAPPING:
               Status = VfatRosQueryLcnMapping(IrpContext);
               break;
#endif
            case FSCTL_IS_VOLUME_DIRTY:
               Status = VfatIsVolumeDirty(IrpContext);
               break;
            case FSCTL_MARK_VOLUME_DIRTY:
               Status = VfatMarkVolumeDirty(IrpContext);
               break;
            default:
               Status = STATUS_INVALID_DEVICE_REQUEST;
         }
         break;

      case IRP_MN_MOUNT_VOLUME:
         Status = VfatMount(IrpContext);
         break;

      case IRP_MN_VERIFY_VOLUME:
        DPRINT("VFATFS: IRP_MN_VERIFY_VOLUME\n");
         Status = VfatVerify(IrpContext);
         break;

      default:
           DPRINT("VFAT FSC: MinorFunction %d\n", IrpContext->MinorFunction);
           Status = STATUS_INVALID_DEVICE_REQUEST;
           break;
   }

   IrpContext->Irp->IoStatus.Status = Status;

   IoCompleteRequest (IrpContext->Irp, IO_NO_INCREMENT);
   VfatFreeIrpContext(IrpContext);
   return (Status);
}
