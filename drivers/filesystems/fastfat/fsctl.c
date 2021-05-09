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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/fsctl.c
 * PURPOSE:          VFAT Filesystem
 */

/* INCLUDES *****************************************************************/

#include "vfat.h"

#define NDEBUG
#include <debug.h>

extern VFAT_DISPATCH FatXDispatch;
extern VFAT_DISPATCH FatDispatch;

/* FUNCTIONS ****************************************************************/

#define  CACHEPAGESIZE(pDeviceExt) ((pDeviceExt)->FatInfo.BytesPerCluster > PAGE_SIZE ? \
                                    (pDeviceExt)->FatInfo.BytesPerCluster : PAGE_SIZE)

static
NTSTATUS
VfatHasFileSystem(
    PDEVICE_OBJECT DeviceToMount,
    PBOOLEAN RecognizedFS,
    PFATINFO pFatInfo,
    BOOLEAN Override)
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
                                      Override);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("VfatBlockDeviceIoControl failed (%x)\n", Status);
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
                                          Override);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("VfatBlockDeviceIoControl failed (%x)\n", Status);
            return Status;
        }

        DPRINT("Partition Information:\n");
        DPRINT("StartingOffset      %I64x\n", PartitionInfo.StartingOffset.QuadPart  / 512);
        DPRINT("PartitionLength     %I64x\n", PartitionInfo.PartitionLength.QuadPart / 512);
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
                 PartitionInfoIsValid = TRUE;
                *RecognizedFS = TRUE;
            }
        }
        else if (DiskGeometry.MediaType == RemovableMedia &&
                 PartitionInfo.PartitionNumber > 0 &&
                 PartitionInfo.StartingOffset.QuadPart == 0 &&
                 PartitionInfo.PartitionLength.QuadPart > 0)
        {
            /* This is possible a removable media formated as super floppy */
            PartitionInfoIsValid = TRUE;
            *RecognizedFS = TRUE;
        }
    }
    else
    {
        *RecognizedFS = TRUE;
    }

    if (*RecognizedFS)
    {
        Boot = ExAllocatePoolWithTag(NonPagedPool, DiskGeometry.BytesPerSector, TAG_BUFFER);
        if (Boot == NULL)
        {
           return STATUS_INSUFFICIENT_RESOURCES;
        }

        Offset.QuadPart = 0;

        /* Try to recognize FAT12/FAT16/FAT32 partitions */
        Status = VfatReadDisk(DeviceToMount, &Offset, DiskGeometry.BytesPerSector, (PUCHAR) Boot, Override);
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
                DPRINT1("BytesPerSector %u\n", Boot->BytesPerSector);
                *RecognizedFS = FALSE;
            }

            if (*RecognizedFS &&
                Boot->FATCount != 1 &&
                Boot->FATCount != 2)
            {
                DPRINT1("FATCount %u\n", Boot->FATCount);
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
                Boot->BytesPerSector * Boot->SectorsPerCluster > 64 * 1024)
            {
                DPRINT1("ClusterSize %d\n", Boot->BytesPerSector * Boot->SectorsPerCluster);
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
                    RtlCopyMemory(&FatInfo.VolumeLabel, &Boot->VolumeLabel, sizeof(FatInfo.VolumeLabel));
                }
                else if (FatInfo.NumberOfClusters >= 65525)
                {
                    DPRINT("FAT32\n");
                    FatInfo.FatType = FAT32;
                    FatInfo.RootCluster = ((struct _BootSector32*) Boot)->RootCluster;
                    FatInfo.rootStart = FatInfo.dataStart + ((FatInfo.RootCluster - 2) * FatInfo.SectorsPerCluster);
                    FatInfo.VolumeID = ((struct _BootSector32*) Boot)->VolumeID;
                    FatInfo.FSInfoSector = ((struct _BootSector32*) Boot)->FSInfoSector;
                    RtlCopyMemory(&FatInfo.VolumeLabel, &((struct _BootSector32*)Boot)->VolumeLabel, sizeof(FatInfo.VolumeLabel));
                }
                else
                {
                    DPRINT("FAT16\n");
                    FatInfo.FatType = FAT16;
                    FatInfo.RootCluster = FatInfo.rootStart / FatInfo.SectorsPerCluster;
                    RtlCopyMemory(&FatInfo.VolumeLabel, &Boot->VolumeLabel, sizeof(FatInfo.VolumeLabel));
                }

                if (PartitionInfoIsValid &&
                    FatInfo.Sectors > PartitionInfo.PartitionLength.QuadPart / FatInfo.BytesPerSector)
                {
                    *RecognizedFS = FALSE;
                }

                if (pFatInfo && *RecognizedFS)
                {
                    *pFatInfo = FatInfo;
                }
            }
        }

        ExFreePoolWithTag(Boot, TAG_BUFFER);
    }

    if (!*RecognizedFS && PartitionInfoIsValid)
    {
        BootFatX = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _BootSectorFatX), TAG_BUFFER);
        if (BootFatX == NULL)
        {
            *RecognizedFS=FALSE;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Offset.QuadPart = 0;

        /* Try to recognize FATX16/FATX32 partitions (Xbox) */
        Status = VfatReadDisk(DeviceToMount, &Offset, sizeof(struct _BootSectorFatX), (PUCHAR) BootFatX, Override);
        if (NT_SUCCESS(Status))
        {
            *RecognizedFS = TRUE;
            if (BootFatX->SysType[0] != 'F' ||
                BootFatX->SysType[1] != 'A' ||
                BootFatX->SysType[2] != 'T' ||
                BootFatX->SysType[3] != 'X')
            {
                DPRINT1("SysType %02X%02X%02X%02X (%c%c%c%c)\n",
                        BootFatX->SysType[0], BootFatX->SysType[1], BootFatX->SysType[2], BootFatX->SysType[3],
                        isprint(BootFatX->SysType[0]) ? BootFatX->SysType[0] : '.',
                        isprint(BootFatX->SysType[1]) ? BootFatX->SysType[1] : '.',
                        isprint(BootFatX->SysType[2]) ? BootFatX->SysType[2] : '.',
                        isprint(BootFatX->SysType[3]) ? BootFatX->SysType[3] : '.');

                *RecognizedFS = FALSE;
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
        ExFreePoolWithTag(BootFatX, TAG_BUFFER);
    }

    DPRINT("VfatHasFileSystem done\n");
    return Status;
}

/*
 * FUNCTION: Read the volume label
 * WARNING: Read this comment carefully before using it (and using it wrong)
 * Device parameter is expected to be the lower DO is start isn't 0
 *                  otherwise, it is expected to be the VCB is start is 0
 * Start parameter is expected to be, in bytes, the beginning of the root start.
 *                 Set it to 0 if you wish to use the associated FCB with caching.
 *                 In that specific case, Device parameter is expected to be the VCB!
 * VolumeLabel parameter is expected to be a preallocated UNICODE_STRING (ie, with buffer)
 *                       Its buffer has to be able to contain MAXIMUM_VOLUME_LABEL_LENGTH bytes 
 */
static
NTSTATUS
ReadVolumeLabel(
    PVOID Device,
    ULONG Start,
    BOOLEAN IsFatX,
    PUNICODE_STRING VolumeLabel)
{
    PDEVICE_EXTENSION DeviceExt;
    PDEVICE_OBJECT DeviceObject;
    PVOID Context = NULL;
    ULONG DirIndex = 0;
    PDIR_ENTRY Entry;
    PVFATFCB pFcb;
    LARGE_INTEGER FileOffset;
    ULONG SizeDirEntry;
    ULONG EntriesPerPage;
    OEM_STRING StringO;
    BOOLEAN NoCache = (Start != 0);
    PVOID Buffer;
    NTSTATUS Status = STATUS_SUCCESS;

    if (IsFatX)
    {
        SizeDirEntry = sizeof(FATX_DIR_ENTRY);
        EntriesPerPage = FATX_ENTRIES_PER_PAGE;
    }
    else
    {
        SizeDirEntry = sizeof(FAT_DIR_ENTRY);
        EntriesPerPage = FAT_ENTRIES_PER_PAGE;
    }

    FileOffset.QuadPart = Start;
    if (!NoCache)
    {
        DeviceExt = Device;

        /* FIXME: Check we really have a VCB
        ASSERT();
        */

        ExAcquireResourceExclusiveLite(&DeviceExt->DirResource, TRUE);
        pFcb = vfatOpenRootFCB(DeviceExt);
        ExReleaseResourceLite(&DeviceExt->DirResource);

        _SEH2_TRY
        {
            CcMapData(pFcb->FileObject, &FileOffset, SizeDirEntry, MAP_WAIT, &Context, (PVOID*)&Entry);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        DeviceObject = Device;

        ASSERT(DeviceObject->Type == 3);

        Buffer = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, TAG_DIRENT);
        if (Buffer != NULL)
        {
            Status = VfatReadDisk(DeviceObject, &FileOffset, PAGE_SIZE, (PUCHAR)Buffer, TRUE);
            if (!NT_SUCCESS(Status))
            {
                ExFreePoolWithTag(Buffer, TAG_DIRENT);
            }
            else
            {
                Entry = Buffer;
            }
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (NT_SUCCESS(Status))
    {
        while (TRUE)
        {
            if (ENTRY_VOLUME(IsFatX, Entry))
            {
                /* copy volume label */
                if (IsFatX)
                {
                    StringO.Buffer = (PCHAR)Entry->FatX.Filename;
                    StringO.MaximumLength = StringO.Length = Entry->FatX.FilenameLength;
                    RtlOemStringToUnicodeString(VolumeLabel, &StringO, FALSE);
                }
                else
                {
                    vfat8Dot3ToString(&Entry->Fat, VolumeLabel);
                }
                break;
            }
            if (ENTRY_END(IsFatX, Entry))
            {
                break;
            }
            DirIndex++;
            Entry = (PDIR_ENTRY)((ULONG_PTR)Entry + SizeDirEntry);
            if ((DirIndex % EntriesPerPage) == 0)
            {
                FileOffset.u.LowPart += PAGE_SIZE;

                if (!NoCache)
                {
                    CcUnpinData(Context);

                    _SEH2_TRY
                    {
                        CcMapData(pFcb->FileObject, &FileOffset, SizeDirEntry, MAP_WAIT, &Context, (PVOID*)&Entry);
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                    }
                    _SEH2_END;
                    if (!NT_SUCCESS(Status))
                    {
                        Context = NULL;
                        break;
                    }
                }
                else
                {
                    Status = VfatReadDisk(DeviceObject, &FileOffset, PAGE_SIZE, (PUCHAR)Buffer, TRUE);
                    if (!NT_SUCCESS(Status))
                    {
                        break;
                    }
                    Entry = Buffer;
                }
            }
        }
        if (Context)
        {
            CcUnpinData(Context);
        }
        else if (NoCache)
        {
            ExFreePoolWithTag(Buffer, TAG_DIRENT);
        }
    }

    if (!NoCache)
    {
        ExAcquireResourceExclusiveLite(&DeviceExt->DirResource, TRUE);
        vfatReleaseFCB(DeviceExt, pFcb);
        ExReleaseResourceLite(&DeviceExt->DirResource);
    }

    return STATUS_SUCCESS;
}


/*
 * FUNCTION: Mount the filesystem
 */
static
NTSTATUS
VfatMount(
    PVFAT_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT DeviceObject = NULL;
    PDEVICE_EXTENSION DeviceExt = NULL;
    BOOLEAN RecognizedFS;
    NTSTATUS Status;
    PVFATFCB Fcb = NULL;
    PVFATFCB VolumeFcb = NULL;
    PDEVICE_OBJECT DeviceToMount;
    PVPB Vpb;
    UNICODE_STRING NameU = RTL_CONSTANT_STRING(L"\\$$Fat$$");
    UNICODE_STRING VolumeNameU = RTL_CONSTANT_STRING(L"\\$$Volume$$");
    UNICODE_STRING VolumeLabelU;
    ULONG HashTableSize;
    ULONG i;
    FATINFO FatInfo;
    BOOLEAN Dirty;

    DPRINT("VfatMount(IrpContext %p)\n", IrpContext);

    ASSERT(IrpContext);

    if (IrpContext->DeviceObject != VfatGlobalData->DeviceObject)
    {
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto ByeBye;
    }

    DeviceToMount = IrpContext->Stack->Parameters.MountVolume.DeviceObject;
    Vpb = IrpContext->Stack->Parameters.MountVolume.Vpb;

    Status = VfatHasFileSystem(DeviceToMount, &RecognizedFS, &FatInfo, FALSE);
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
    DPRINT("VFAT: Recognized volume\n");
    Status = IoCreateDevice(VfatGlobalData->DriverObject,
                            ROUND_UP(sizeof (DEVICE_EXTENSION), sizeof(ULONG)) + sizeof(HASHENTRY*) * HashTableSize,
                            NULL,
                            FILE_DEVICE_DISK_FILE_SYSTEM,
                            DeviceToMount->Characteristics,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        goto ByeBye;
    }

    DeviceExt = DeviceObject->DeviceExtension;
    RtlZeroMemory(DeviceExt, ROUND_UP(sizeof(DEVICE_EXTENSION), sizeof(ULONG)) + sizeof(HASHENTRY*) * HashTableSize);
    DeviceExt->FcbHashTable = (HASHENTRY**)((ULONG_PTR)DeviceExt + ROUND_UP(sizeof(DEVICE_EXTENSION), sizeof(ULONG)));
    DeviceExt->HashTableSize = HashTableSize;
    DeviceExt->VolumeDevice = DeviceObject;

    KeInitializeSpinLock(&DeviceExt->OverflowQueueSpinLock);
    InitializeListHead(&DeviceExt->OverflowQueue);
    DeviceExt->OverflowQueueCount = 0;
    DeviceExt->PostedRequestCount = 0;

    /* use same vpb as device disk */
    DeviceObject->Vpb = Vpb;
    DeviceToMount->Vpb = Vpb;

    RtlCopyMemory(&DeviceExt->FatInfo, &FatInfo, sizeof(FATINFO));

    DPRINT("BytesPerSector:     %u\n", DeviceExt->FatInfo.BytesPerSector);
    DPRINT("SectorsPerCluster:  %u\n", DeviceExt->FatInfo.SectorsPerCluster);
    DPRINT("FATCount:           %u\n", DeviceExt->FatInfo.FATCount);
    DPRINT("FATSectors:         %u\n", DeviceExt->FatInfo.FATSectors);
    DPRINT("RootStart:          %u\n", DeviceExt->FatInfo.rootStart);
    DPRINT("DataStart:          %u\n", DeviceExt->FatInfo.dataStart);
    if (DeviceExt->FatInfo.FatType == FAT32)
    {
        DPRINT("RootCluster:        %u\n", DeviceExt->FatInfo.RootCluster);
    }

    switch (DeviceExt->FatInfo.FatType)
    {
        case FAT12:
            DeviceExt->GetNextCluster = FAT12GetNextCluster;
            DeviceExt->FindAndMarkAvailableCluster = FAT12FindAndMarkAvailableCluster;
            DeviceExt->WriteCluster = FAT12WriteCluster;
            /* We don't define dirty bit functions here
             * FAT12 doesn't have such bit and they won't get called
             */
            break;

        case FAT16:
        case FATX16:
            DeviceExt->GetNextCluster = FAT16GetNextCluster;
            DeviceExt->FindAndMarkAvailableCluster = FAT16FindAndMarkAvailableCluster;
            DeviceExt->WriteCluster = FAT16WriteCluster;
            DeviceExt->GetDirtyStatus = FAT16GetDirtyStatus;
            DeviceExt->SetDirtyStatus = FAT16SetDirtyStatus;
            break;

        case FAT32:
        case FATX32:
            DeviceExt->GetNextCluster = FAT32GetNextCluster;
            DeviceExt->FindAndMarkAvailableCluster = FAT32FindAndMarkAvailableCluster;
            DeviceExt->WriteCluster = FAT32WriteCluster;
            DeviceExt->GetDirtyStatus = FAT32GetDirtyStatus;
            DeviceExt->SetDirtyStatus = FAT32SetDirtyStatus;
            break;
    }

    if (DeviceExt->FatInfo.FatType == FATX16 ||
        DeviceExt->FatInfo.FatType == FATX32)
    {
        DeviceExt->Flags |= VCB_IS_FATX;
        DeviceExt->BaseDateYear = 2000;
        RtlCopyMemory(&DeviceExt->Dispatch, &FatXDispatch, sizeof(VFAT_DISPATCH));
    }
    else
    {
        DeviceExt->BaseDateYear = 1980;
        RtlCopyMemory(&DeviceExt->Dispatch, &FatDispatch, sizeof(VFAT_DISPATCH));
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

    DeviceExt->IoVPB = DeviceObject->Vpb;
    DeviceExt->SpareVPB = ExAllocatePoolWithTag(NonPagedPool, sizeof(VPB), TAG_VPB);
    if (DeviceExt->SpareVPB == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ByeBye;
    }

    DeviceExt->Statistics = ExAllocatePoolWithTag(NonPagedPool,
                                                  sizeof(STATISTICS) * VfatGlobalData->NumberProcessors,
                                                  TAG_STATS);
    if (DeviceExt->Statistics == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ByeBye;
    }

    RtlZeroMemory(DeviceExt->Statistics, sizeof(STATISTICS) * VfatGlobalData->NumberProcessors);
    for (i = 0; i < VfatGlobalData->NumberProcessors; ++i)
    {
        DeviceExt->Statistics[i].Base.FileSystemType = FILESYSTEM_STATISTICS_TYPE_FAT;
        DeviceExt->Statistics[i].Base.Version = 1;
        DeviceExt->Statistics[i].Base.SizeOfCompleteStructure = sizeof(STATISTICS);
    }

    DeviceExt->FATFileObject = IoCreateStreamFileObject(NULL, DeviceExt->StorageDevice);
    Fcb = vfatNewFCB(DeviceExt, &NameU);
    if (Fcb == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ByeBye;
    }

    Status = vfatAttachFCBToFileObject(DeviceExt, Fcb, DeviceExt->FATFileObject);
    if (!NT_SUCCESS(Status))
        goto ByeBye;

    DeviceExt->FATFileObject->PrivateCacheMap = NULL;
    Fcb->FileObject = DeviceExt->FATFileObject;

    Fcb->Flags = FCB_IS_FAT;
    Fcb->RFCB.FileSize.QuadPart = DeviceExt->FatInfo.FATSectors * DeviceExt->FatInfo.BytesPerSector;
    Fcb->RFCB.ValidDataLength = Fcb->RFCB.FileSize;
    Fcb->RFCB.AllocationSize = Fcb->RFCB.FileSize;

    _SEH2_TRY
    {
        CcInitializeCacheMap(DeviceExt->FATFileObject,
                             (PCC_FILE_SIZES)(&Fcb->RFCB.AllocationSize),
                             TRUE,
                             &VfatGlobalData->CacheMgrCallbacks,
                             Fcb);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        goto ByeBye;
    }
    _SEH2_END;

    DeviceExt->LastAvailableCluster = 2;
    CountAvailableClusters(DeviceExt, NULL);
    ExInitializeResourceLite(&DeviceExt->FatResource);

    InitializeListHead(&DeviceExt->FcbListHead);

    VolumeFcb = vfatNewFCB(DeviceExt, &VolumeNameU);
    if (VolumeFcb == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto ByeBye;
    }

    VolumeFcb->Flags = FCB_IS_VOLUME;
    VolumeFcb->RFCB.FileSize.QuadPart = (LONGLONG) DeviceExt->FatInfo.Sectors * DeviceExt->FatInfo.BytesPerSector;
    VolumeFcb->RFCB.ValidDataLength = VolumeFcb->RFCB.FileSize;
    VolumeFcb->RFCB.AllocationSize = VolumeFcb->RFCB.FileSize;
    DeviceExt->VolumeFcb = VolumeFcb;

    ExAcquireResourceExclusiveLite(&VfatGlobalData->VolumeListLock, TRUE);
    InsertHeadList(&VfatGlobalData->VolumeListHead, &DeviceExt->VolumeListEntry);
    ExReleaseResourceLite(&VfatGlobalData->VolumeListLock);

    /* read serial number */
    DeviceObject->Vpb->SerialNumber = DeviceExt->FatInfo.VolumeID;

    /* read volume label */
    VolumeLabelU.Buffer = DeviceObject->Vpb->VolumeLabel;
    VolumeLabelU.Length = 0;
    VolumeLabelU.MaximumLength = sizeof(DeviceObject->Vpb->VolumeLabel);
    ReadVolumeLabel(DeviceExt, 0, vfatVolumeIsFatX(DeviceExt), &VolumeLabelU);
    Vpb->VolumeLabelLength = VolumeLabelU.Length;

    /* read dirty bit status */
    Status = GetDirtyStatus(DeviceExt, &Dirty);
    if (NT_SUCCESS(Status))
    {
        /* The volume wasn't dirty, it was properly dismounted */
        if (!Dirty)
        {
            /* Mark it dirty now! */
            SetDirtyStatus(DeviceExt, TRUE);
            VolumeFcb->Flags |= VCB_CLEAR_DIRTY;
        }
        else
        {
            DPRINT1("Mounting a dirty volume\n");
        }
    }

    VolumeFcb->Flags |= VCB_IS_DIRTY;
    if (BooleanFlagOn(Vpb->RealDevice->Flags, DO_SYSTEM_BOOT_PARTITION))
    {
        SetFlag(DeviceExt->Flags, VCB_IS_SYS_OR_HAS_PAGE);
    }

    /* Initialize the notify list and synchronization object */
    InitializeListHead(&DeviceExt->NotifyList);
    FsRtlNotifyInitializeSync(&DeviceExt->NotifySync);

    /* The VCB is OK for usage */
    SetFlag(DeviceExt->Flags, VCB_GOOD);

    /* Send the mount notification */
    FsRtlNotifyVolumeEvent(DeviceExt->FATFileObject, FSRTL_VOLUME_MOUNT);

    DPRINT("Mount success\n");

    Status = STATUS_SUCCESS;

ByeBye:
    if (!NT_SUCCESS(Status))
    {
        /* Cleanup */
        if (DeviceExt && DeviceExt->FATFileObject)
        {
            LARGE_INTEGER Zero = {{0,0}};
            PVFATCCB Ccb = (PVFATCCB)DeviceExt->FATFileObject->FsContext2;

            CcUninitializeCacheMap(DeviceExt->FATFileObject,
                                   &Zero,
                                   NULL);
            ObDereferenceObject(DeviceExt->FATFileObject);
            if (Ccb)
                vfatDestroyCCB(Ccb);
            DeviceExt->FATFileObject = NULL;
        }
        if (Fcb)
            vfatDestroyFCB(Fcb);
        if (DeviceExt && DeviceExt->SpareVPB)
            ExFreePoolWithTag(DeviceExt->SpareVPB, TAG_VPB);
        if (DeviceExt && DeviceExt->Statistics)
            ExFreePoolWithTag(DeviceExt->Statistics, TAG_STATS);
        if (DeviceObject)
            IoDeleteDevice(DeviceObject);
    }

    return Status;
}


/*
 * FUNCTION: Verify the filesystem
 */
static
NTSTATUS
VfatVerify(
    PVFAT_IRP_CONTEXT IrpContext)
{
    PDEVICE_OBJECT DeviceToVerify;
    NTSTATUS Status;
    FATINFO FatInfo;
    BOOLEAN RecognizedFS;
    PDEVICE_EXTENSION DeviceExt;
    BOOLEAN AllowRaw;
    PVPB Vpb;
    ULONG ChangeCount, BufSize = sizeof(ChangeCount);

    DPRINT("VfatVerify(IrpContext %p)\n", IrpContext);

    DeviceToVerify = IrpContext->Stack->Parameters.VerifyVolume.DeviceObject;
    DeviceExt = DeviceToVerify->DeviceExtension;
    Vpb = IrpContext->Stack->Parameters.VerifyVolume.Vpb;
    AllowRaw = BooleanFlagOn(IrpContext->Stack->Flags, SL_ALLOW_RAW_MOUNT);

    if (!BooleanFlagOn(Vpb->RealDevice->Flags, DO_VERIFY_VOLUME))
    {
        DPRINT("Already verified\n");
        return STATUS_SUCCESS;
    }

    Status = VfatBlockDeviceIoControl(DeviceExt->StorageDevice,
                                      IOCTL_DISK_CHECK_VERIFY,
                                      NULL,
                                      0,
                                      &ChangeCount,
                                      &BufSize,
                                      TRUE);
    if (!NT_SUCCESS(Status) && Status != STATUS_VERIFY_REQUIRED)
    {
        DPRINT("VfatBlockDeviceIoControl() failed (Status %lx)\n", Status);
        Status = (AllowRaw ? STATUS_WRONG_VOLUME : Status);
    }
    else
    {
        Status = VfatHasFileSystem(DeviceExt->StorageDevice, &RecognizedFS, &FatInfo, TRUE);
        if (!NT_SUCCESS(Status) || RecognizedFS == FALSE)
        {
            if (NT_SUCCESS(Status) || AllowRaw)
            {
                Status = STATUS_WRONG_VOLUME;
            }
        }
        else if (sizeof(FATINFO) == RtlCompareMemory(&FatInfo, &DeviceExt->FatInfo, sizeof(FATINFO)))
        {
            WCHAR BufferU[MAXIMUM_VOLUME_LABEL_LENGTH / sizeof(WCHAR)];
            UNICODE_STRING VolumeLabelU;
            UNICODE_STRING VpbLabelU;

            VolumeLabelU.Buffer = BufferU;
            VolumeLabelU.Length = 0;
            VolumeLabelU.MaximumLength = sizeof(BufferU);
            Status = ReadVolumeLabel(DeviceExt->StorageDevice, FatInfo.rootStart * FatInfo.BytesPerSector, (FatInfo.FatType >= FATX16), &VolumeLabelU);
            if (!NT_SUCCESS(Status))
            {
                if (AllowRaw)
                {
                    Status = STATUS_WRONG_VOLUME;
                }
            }
            else
            {
                VpbLabelU.Buffer = Vpb->VolumeLabel;
                VpbLabelU.Length = Vpb->VolumeLabelLength;
                VpbLabelU.MaximumLength = sizeof(Vpb->VolumeLabel);

                if (RtlCompareUnicodeString(&VpbLabelU, &VolumeLabelU, FALSE) != 0)
                {
                    Status = STATUS_WRONG_VOLUME;
                }
                else
                {
                    DPRINT1("Same volume\n");
                }
            }
        }
        else
        {
            Status = STATUS_WRONG_VOLUME;
        }
    }

    Vpb->RealDevice->Flags &= ~DO_VERIFY_VOLUME;

    return Status;
}


static
NTSTATUS
VfatGetVolumeBitmap(
    PVFAT_IRP_CONTEXT IrpContext)
{
    DPRINT("VfatGetVolumeBitmap (IrpContext %p)\n", IrpContext);
    return STATUS_INVALID_DEVICE_REQUEST;
}


static
NTSTATUS
VfatGetRetrievalPointers(
    PVFAT_IRP_CONTEXT IrpContext)
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

static
NTSTATUS
VfatMoveFile(
    PVFAT_IRP_CONTEXT IrpContext)
{
    DPRINT("VfatMoveFile(IrpContext %p)\n", IrpContext);
    return STATUS_INVALID_DEVICE_REQUEST;
}

static
NTSTATUS
VfatIsVolumeDirty(
    PVFAT_IRP_CONTEXT IrpContext)
{
    PULONG Flags;

    DPRINT("VfatIsVolumeDirty(IrpContext %p)\n", IrpContext);

    if (IrpContext->Stack->Parameters.FileSystemControl.OutputBufferLength != sizeof(ULONG))
        return STATUS_INVALID_BUFFER_SIZE;
    else if (!IrpContext->Irp->AssociatedIrp.SystemBuffer)
        return STATUS_INVALID_USER_BUFFER;

    Flags = (PULONG)IrpContext->Irp->AssociatedIrp.SystemBuffer;
    *Flags = 0;

    if (BooleanFlagOn(IrpContext->DeviceExt->VolumeFcb->Flags, VCB_IS_DIRTY) &&
        !BooleanFlagOn(IrpContext->DeviceExt->VolumeFcb->Flags, VCB_CLEAR_DIRTY))
    {
        *Flags |= VOLUME_IS_DIRTY;
    }

    IrpContext->Irp->IoStatus.Information = sizeof(ULONG);

    return STATUS_SUCCESS;
}

static
NTSTATUS
VfatMarkVolumeDirty(
    PVFAT_IRP_CONTEXT IrpContext)
{
    PDEVICE_EXTENSION DeviceExt;
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("VfatMarkVolumeDirty(IrpContext %p)\n", IrpContext);
    DeviceExt = IrpContext->DeviceExt;

    if (!BooleanFlagOn(DeviceExt->VolumeFcb->Flags, VCB_IS_DIRTY))
    {
        Status = SetDirtyStatus(DeviceExt, TRUE);
    }

    DeviceExt->VolumeFcb->Flags &= ~VCB_CLEAR_DIRTY;

    return Status;
}

static
NTSTATUS
VfatLockOrUnlockVolume(
    PVFAT_IRP_CONTEXT IrpContext,
    BOOLEAN Lock)
{
    PFILE_OBJECT FileObject;
    PDEVICE_EXTENSION DeviceExt;
    PVFATFCB Fcb;
    PVPB Vpb;

    DPRINT("VfatLockOrUnlockVolume(%p, %d)\n", IrpContext, Lock);

    DeviceExt = IrpContext->DeviceExt;
    FileObject = IrpContext->FileObject;
    Fcb = FileObject->FsContext;
    Vpb = DeviceExt->FATFileObject->Vpb;

    /* Only allow locking with the volume open */
    if (!BooleanFlagOn(Fcb->Flags, FCB_IS_VOLUME))
    {
        return STATUS_ACCESS_DENIED;
    }

    /* Bail out if it's already in the demanded state */
    if ((BooleanFlagOn(DeviceExt->Flags, VCB_VOLUME_LOCKED) && Lock) ||
        (!BooleanFlagOn(DeviceExt->Flags, VCB_VOLUME_LOCKED) && !Lock))
    {
        return STATUS_ACCESS_DENIED;
    }

    /* Bail out if it's already in the demanded state */
    if ((BooleanFlagOn(Vpb->Flags, VPB_LOCKED) && Lock) ||
        (!BooleanFlagOn(Vpb->Flags, VPB_LOCKED) && !Lock))
    {
        return STATUS_ACCESS_DENIED;
    }

    if (Lock)
    {
        FsRtlNotifyVolumeEvent(IrpContext->Stack->FileObject, FSRTL_VOLUME_LOCK);
    }

    /* Deny locking if we're not alone */
    if (Lock && DeviceExt->OpenHandleCount != 1)
    {
        PLIST_ENTRY ListEntry;

#if 1
        /* FIXME: Hack that allows locking the system volume on
         * boot so that autochk can run properly
         * That hack is, on purpose, really restrictive
         * it will only allow locking with two directories
         * open: current directory of smss and autochk.
         */
        BOOLEAN ForceLock = TRUE;
        ULONG HandleCount = 0;

        /* Only allow boot volume */
        if (BooleanFlagOn(DeviceExt->Flags, VCB_IS_SYS_OR_HAS_PAGE))
        {
            /* We'll browse all the FCB */
            ListEntry = DeviceExt->FcbListHead.Flink;
            while (ListEntry != &DeviceExt->FcbListHead)
            {
                Fcb = CONTAINING_RECORD(ListEntry, VFATFCB, FcbListEntry);
                ListEntry = ListEntry->Flink;

                /* If no handle: that FCB is no problem for locking
                 * so ignore it
                 */
                if (Fcb->OpenHandleCount == 0)
                {
                    continue;
                }

                /* Not a dir? We're no longer at boot */
                if (!vfatFCBIsDirectory(Fcb))
                {
                    ForceLock = FALSE;
                    break;
                }

                /* If we have cached initialized and several handles, we're
                   not in the boot case
                 */
                if (Fcb->FileObject != NULL && Fcb->OpenHandleCount > 1)
                {
                    ForceLock = FALSE;
                    break;
                }

                /* Count the handles */
                HandleCount += Fcb->OpenHandleCount;
                /* More than two handles? Then, we're not booting anymore */
                if (HandleCount > 2)
                {
                    ForceLock = FALSE;
                    break;
                }
            }
        }
        else
        {
            ForceLock = FALSE;
        }

        /* Here comes the hack, ignore the failure! */
        if (!ForceLock)
        {
#endif

        DPRINT1("Can't lock: %u opened\n", DeviceExt->OpenHandleCount);

        ListEntry = DeviceExt->FcbListHead.Flink;
        while (ListEntry != &DeviceExt->FcbListHead)
        {
            Fcb = CONTAINING_RECORD(ListEntry, VFATFCB, FcbListEntry);
            ListEntry = ListEntry->Flink;

            if (Fcb->OpenHandleCount  > 0)
            {
                DPRINT1("Opened (%u - %u): %wZ\n", Fcb->OpenHandleCount, Fcb->RefCount, &Fcb->PathNameU);
            }
        }

        FsRtlNotifyVolumeEvent(IrpContext->Stack->FileObject, FSRTL_VOLUME_LOCK_FAILED);

        return STATUS_ACCESS_DENIED;

#if 1
        /* End of the hack: be verbose about its usage,
         * just in case we would mess up everything!
         */
        }
        else
        {
            DPRINT1("HACK: Using lock-hack!\n");
        }
#endif
    }

    /* Finally, proceed */
    if (Lock)
    {
        /* Flush volume & files */
        VfatFlushVolume(DeviceExt, DeviceExt->VolumeFcb);

        /* The volume is now clean */
        if (BooleanFlagOn(DeviceExt->VolumeFcb->Flags, VCB_CLEAR_DIRTY) &&
            BooleanFlagOn(DeviceExt->VolumeFcb->Flags, VCB_IS_DIRTY))
        {
            /* Drop the dirty bit */
            if (NT_SUCCESS(SetDirtyStatus(DeviceExt, FALSE)))
                ClearFlag(DeviceExt->VolumeFcb->Flags, VCB_IS_DIRTY);
        }

        DeviceExt->Flags |= VCB_VOLUME_LOCKED;
        Vpb->Flags |= VPB_LOCKED;
    }
    else
    {
        DeviceExt->Flags &= ~VCB_VOLUME_LOCKED;
        Vpb->Flags &= ~VPB_LOCKED;

        FsRtlNotifyVolumeEvent(IrpContext->Stack->FileObject, FSRTL_VOLUME_UNLOCK);
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
VfatDismountVolume(
    PVFAT_IRP_CONTEXT IrpContext)
{
    PDEVICE_EXTENSION DeviceExt;
    PLIST_ENTRY NextEntry;
    PVFATFCB Fcb;
    PFILE_OBJECT FileObject;

    DPRINT("VfatDismountVolume(%p)\n", IrpContext);

    DeviceExt = IrpContext->DeviceExt;
    FileObject = IrpContext->FileObject;

    /* We HAVE to be locked. Windows also allows dismount with no lock
     * but we're here mainly for 1st stage, so KISS
     */
    if (!BooleanFlagOn(DeviceExt->Flags, VCB_VOLUME_LOCKED))
    {
        return STATUS_ACCESS_DENIED;
    }

    /* Deny dismount of boot volume */
    if (BooleanFlagOn(DeviceExt->Flags, VCB_IS_SYS_OR_HAS_PAGE))
    {
        return STATUS_ACCESS_DENIED;
    }

    /* Race condition? */
    if (BooleanFlagOn(DeviceExt->Flags, VCB_DISMOUNT_PENDING))
    {
        return STATUS_VOLUME_DISMOUNTED;
    }

    /* Notify we'll dismount. Pass that point there's no reason we fail */
    FsRtlNotifyVolumeEvent(IrpContext->Stack->FileObject, FSRTL_VOLUME_DISMOUNT);

    ExAcquireResourceExclusiveLite(&DeviceExt->FatResource, TRUE);

    /* Flush volume & files */
    VfatFlushVolume(DeviceExt, (PVFATFCB)FileObject->FsContext);

    /* The volume is now clean */
    if (BooleanFlagOn(DeviceExt->VolumeFcb->Flags, VCB_CLEAR_DIRTY) &&
        BooleanFlagOn(DeviceExt->VolumeFcb->Flags, VCB_IS_DIRTY))
    {
        /* Drop the dirty bit */
        if (NT_SUCCESS(SetDirtyStatus(DeviceExt, FALSE)))
            DeviceExt->VolumeFcb->Flags &= ~VCB_IS_DIRTY;
    }

    /* Rebrowse the FCB in order to free them now */
    while (!IsListEmpty(&DeviceExt->FcbListHead))
    {
        NextEntry = RemoveTailList(&DeviceExt->FcbListHead);
        Fcb = CONTAINING_RECORD(NextEntry, VFATFCB, FcbListEntry);

        if (Fcb == DeviceExt->RootFcb)
            DeviceExt->RootFcb = NULL;
        else if (Fcb == DeviceExt->VolumeFcb)
            DeviceExt->VolumeFcb = NULL;

        vfatDestroyFCB(Fcb);
    }

    /* We are uninitializing, the VCB cannot be used anymore */
    ClearFlag(DeviceExt->Flags, VCB_GOOD);

    /* Mark we're being dismounted */
    DeviceExt->Flags |= VCB_DISMOUNT_PENDING;
#ifndef ENABLE_SWAPOUT
    IrpContext->DeviceObject->Vpb->Flags &= ~VPB_MOUNTED;
#endif

    ExReleaseResourceLite(&DeviceExt->FatResource);

    return STATUS_SUCCESS;
}

static
NTSTATUS
VfatGetStatistics(
    PVFAT_IRP_CONTEXT IrpContext)
{
    PVOID Buffer;
    ULONG Length;
    NTSTATUS Status;
    PDEVICE_EXTENSION DeviceExt;

    DeviceExt = IrpContext->DeviceExt;
    Length = IrpContext->Stack->Parameters.FileSystemControl.OutputBufferLength;
    Buffer = IrpContext->Irp->AssociatedIrp.SystemBuffer;

    if (Length < sizeof(FILESYSTEM_STATISTICS))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (Buffer == NULL)
    {
        return STATUS_INVALID_USER_BUFFER;
    }

    if (Length >= sizeof(STATISTICS) * VfatGlobalData->NumberProcessors)
    {
        Length = sizeof(STATISTICS) * VfatGlobalData->NumberProcessors;
        Status = STATUS_SUCCESS;
    }
    else
    {
        Status = STATUS_BUFFER_OVERFLOW;
    }

    RtlCopyMemory(Buffer, DeviceExt->Statistics, Length);
    IrpContext->Irp->IoStatus.Information = Length;

    return Status;
}

/*
 * FUNCTION: File system control
 */
NTSTATUS
VfatFileSystemControl(
    PVFAT_IRP_CONTEXT IrpContext)
{
    NTSTATUS Status;

    DPRINT("VfatFileSystemControl(IrpContext %p)\n", IrpContext);

    ASSERT(IrpContext);
    ASSERT(IrpContext->Irp);
    ASSERT(IrpContext->Stack);

    IrpContext->Irp->IoStatus.Information = 0;

    switch (IrpContext->MinorFunction)
    {
        case IRP_MN_KERNEL_CALL:
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

                case FSCTL_IS_VOLUME_DIRTY:
                    Status = VfatIsVolumeDirty(IrpContext);
                    break;

                case FSCTL_MARK_VOLUME_DIRTY:
                    Status = VfatMarkVolumeDirty(IrpContext);
                    break;

                case FSCTL_LOCK_VOLUME:
                    Status = VfatLockOrUnlockVolume(IrpContext, TRUE);
                    break;

                case FSCTL_UNLOCK_VOLUME:
                    Status = VfatLockOrUnlockVolume(IrpContext, FALSE);
                    break;

                case FSCTL_DISMOUNT_VOLUME:
                    Status = VfatDismountVolume(IrpContext);
                    break;

                case FSCTL_FILESYSTEM_GET_STATISTICS:
                    Status = VfatGetStatistics(IrpContext);
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
            DPRINT("VFAT FSC: MinorFunction %u\n", IrpContext->MinorFunction);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    return Status;
}
