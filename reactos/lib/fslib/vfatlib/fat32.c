/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS VFAT filesystem library
 * FILE:        fat32.c
 * PURPOSE:     Fat32 support
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Eric Kohl
 */

/* INCLUDES *******************************************************************/

#include "vfatlib.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ******************************************************************/

static NTSTATUS
Fat32WriteBootSector(IN HANDLE FileHandle,
                     IN PFAT32_BOOT_SECTOR BootSector,
                     IN OUT PFORMAT_CONTEXT Context)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    PFAT32_BOOT_SECTOR NewBootSector;
    LARGE_INTEGER FileOffset;

    /* Allocate buffer for new bootsector */
    NewBootSector = (PFAT32_BOOT_SECTOR)RtlAllocateHeap(RtlGetProcessHeap(),
                                            0,
                                            BootSector->BytesPerSector);
    if (NewBootSector == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Zero the new bootsector */
    RtlZeroMemory(NewBootSector, BootSector->BytesPerSector);

    /* Copy FAT32 BPB to new bootsector */
    memcpy(&NewBootSector->OEMName[0],
           &BootSector->OEMName[0],
           FIELD_OFFSET(FAT32_BOOT_SECTOR, Res2) - FIELD_OFFSET(FAT32_BOOT_SECTOR, OEMName));
           /* FAT32 BPB length (up to (not including) Res2) */

    /* Write the boot sector signature */
    NewBootSector->Signature1 = 0xAA550000;

    /* Write sector 0 */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         NewBootSector,
                         BootSector->BytesPerSector,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
        goto done;
    }

    UpdateProgress(Context, 1);

    /* Write backup boot sector */
    if (BootSector->BootBackup != 0x0000)
    {
        FileOffset.QuadPart = (ULONGLONG)((ULONG)BootSector->BootBackup * BootSector->BytesPerSector);
        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             NewBootSector,
                             BootSector->BytesPerSector,
                             &FileOffset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
            goto done;
        }

        UpdateProgress(Context, 1);
    }

done:
    /* Free the buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, NewBootSector);
    return Status;
}


static NTSTATUS
Fat32WriteFsInfo(IN HANDLE FileHandle,
                 IN PFAT32_BOOT_SECTOR BootSector,
                 IN OUT PFORMAT_CONTEXT Context)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    PFAT32_FSINFO FsInfo;
    LARGE_INTEGER FileOffset;
    ULONGLONG FirstDataSector;

    /* Allocate buffer for new sector */
    FsInfo = (PFAT32_FSINFO)RtlAllocateHeap(RtlGetProcessHeap(),
                                            0,
                                            BootSector->BytesPerSector);
    if (FsInfo == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Zero the first FsInfo sector */
    RtlZeroMemory(FsInfo, BootSector->BytesPerSector);

    FirstDataSector = BootSector->ReservedSectors +
        (BootSector->FATCount * BootSector->FATSectors32) + 0 /* RootDirSectors */;

    FsInfo->LeadSig   = FSINFO_SECTOR_BEGIN_SIGNATURE;
    FsInfo->StrucSig  = FSINFO_SIGNATURE;
    FsInfo->FreeCount = (BootSector->SectorsHuge - FirstDataSector) / BootSector->SectorsPerCluster - 1;
    FsInfo->NextFree  = 0xffffffff;
    FsInfo->TrailSig  = FSINFO_SECTOR_END_SIGNATURE;

    /* Write the first FsInfo sector */
    FileOffset.QuadPart = BootSector->FSInfoSector * BootSector->BytesPerSector;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         FsInfo,
                         BootSector->BytesPerSector,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
        goto done;
    }

    UpdateProgress(Context, 1);

    /* Write backup of the first FsInfo sector */
    if (BootSector->BootBackup != 0x0000)
    {
        /* Reset the free cluster count for the backup */
        FsInfo->FreeCount = 0xffffffff;

        FileOffset.QuadPart = (ULONGLONG)(((ULONG)BootSector->BootBackup + (ULONG)BootSector->FSInfoSector) * BootSector->BytesPerSector);
        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FsInfo,
                             BootSector->BytesPerSector,
                             &FileOffset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
            goto done;
        }

        UpdateProgress(Context, 1);
    }

    /* Zero the second FsInfo sector */
    RtlZeroMemory(FsInfo, BootSector->BytesPerSector);
    FsInfo->TrailSig = FSINFO_SECTOR_END_SIGNATURE;

    /* Write the second FsInfo sector */
    FileOffset.QuadPart = (BootSector->FSInfoSector + 1) * BootSector->BytesPerSector;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         FsInfo,
                         BootSector->BytesPerSector,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
        goto done;
    }

    UpdateProgress(Context, 1);

    /* Write backup of the second FsInfo sector */
    if (BootSector->BootBackup != 0x0000)
    {
        FileOffset.QuadPart = (ULONGLONG)(((ULONG)BootSector->BootBackup + (ULONG)BootSector->FSInfoSector + 1) * BootSector->BytesPerSector);
        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FsInfo,
                             BootSector->BytesPerSector,
                             &FileOffset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
            goto done;
        }

        UpdateProgress(Context, 1);
    }

done:
    /* Free the buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, FsInfo);
    return Status;
}


static NTSTATUS
Fat32WriteFAT(IN HANDLE FileHandle,
              IN ULONG SectorOffset,
              IN PFAT32_BOOT_SECTOR BootSector,
              IN OUT PFORMAT_CONTEXT Context)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    PUCHAR Buffer;
    LARGE_INTEGER FileOffset;
    ULONG i;
    ULONG Sectors;

    /* Allocate buffer */
    Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
                                     0,
                                     64 * 1024);
    if (Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Zero the buffer */
    RtlZeroMemory(Buffer, 64 * 1024);

    /* FAT cluster 0 */
    Buffer[0] = 0xf8; /* Media type */
    Buffer[1] = 0xff;
    Buffer[2] = 0xff;
    Buffer[3] = 0x0f;

    /* FAT cluster 1 */
    Buffer[4] = 0xff; /* Clean shutdown, no disk read/write errors, end-of-cluster (EOC) mark */
    Buffer[5] = 0xff;
    Buffer[6] = 0xff;
    Buffer[7] = 0x0f;

    /* FAT cluster 2 */
    Buffer[8] = 0xff; /* End of root directory */
    Buffer[9] = 0xff;
    Buffer[10] = 0xff;
    Buffer[11] = 0x0f;

    /* Write first sector of the FAT */
    FileOffset.QuadPart = (SectorOffset + BootSector->ReservedSectors) * BootSector->BytesPerSector;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Buffer,
                         BootSector->BytesPerSector,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
        goto done;
    }

    UpdateProgress(Context, 1);

    /* Zero the begin of the buffer */
    RtlZeroMemory(Buffer, 12);

    /* Zero the rest of the FAT */
    Sectors = 64 * 1024 / BootSector->BytesPerSector;
    for (i = 1; i < BootSector->FATSectors32; i += Sectors)
    {
        /* Zero some sectors of the FAT */
        FileOffset.QuadPart = (SectorOffset + BootSector->ReservedSectors + i) * BootSector->BytesPerSector;

        if ((BootSector->FATSectors32 - i) <= Sectors)
        {
            Sectors = BootSector->FATSectors32 - i;
        }

        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             Buffer,
                             Sectors * BootSector->BytesPerSector,
                             &FileOffset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
            goto done;
        }

        UpdateProgress(Context, Sectors);
    }

done:
    /* Free the buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    return Status;
}


static NTSTATUS
Fat32WriteRootDirectory(IN HANDLE FileHandle,
                        IN PFAT32_BOOT_SECTOR BootSector,
                        IN OUT PFORMAT_CONTEXT Context)
{
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;
    PUCHAR Buffer;
    LARGE_INTEGER FileOffset;
    ULONGLONG FirstDataSector;
    ULONGLONG FirstRootDirSector;

    /* Allocate buffer for the cluster */
    Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
                                     0,
                                     BootSector->SectorsPerCluster * BootSector->BytesPerSector);
    if (Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Zero the buffer */
    RtlZeroMemory(Buffer, BootSector->SectorsPerCluster * BootSector->BytesPerSector);

    DPRINT("BootSector->ReservedSectors = %lu\n", BootSector->ReservedSectors);
    DPRINT("BootSector->FATSectors32 = %lu\n", BootSector->FATSectors32);
    DPRINT("BootSector->RootCluster = %lu\n", BootSector->RootCluster);
    DPRINT("BootSector->SectorsPerCluster = %lu\n", BootSector->SectorsPerCluster);

    /* Write cluster */
    FirstDataSector = BootSector->ReservedSectors +
        (BootSector->FATCount * BootSector->FATSectors32) + 0 /* RootDirSectors */;

    DPRINT("FirstDataSector = %lu\n", FirstDataSector);

    FirstRootDirSector = ((BootSector->RootCluster - 2) * BootSector->SectorsPerCluster) + FirstDataSector;
    FileOffset.QuadPart = FirstRootDirSector * BootSector->BytesPerSector;

    DPRINT("FirstRootDirSector = %lu\n", FirstRootDirSector);
    DPRINT("FileOffset = %lu\n", FileOffset.QuadPart);

    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Buffer,
                         BootSector->SectorsPerCluster * BootSector->BytesPerSector,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
        goto done;
    }

    UpdateProgress(Context, (ULONG)BootSector->SectorsPerCluster);

done:
    /* Free the buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    return Status;
}


static
NTSTATUS
Fat32WipeSectors(
    IN HANDLE FileHandle,
    IN PFAT32_BOOT_SECTOR BootSector,
    IN OUT PFORMAT_CONTEXT Context)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PUCHAR Buffer;
    LARGE_INTEGER FileOffset;
    ULONGLONG Sector;
    ULONG Length;
    NTSTATUS Status;

    /* Allocate buffer for the cluster */
    Buffer = (PUCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     BootSector->SectorsPerCluster * BootSector->BytesPerSector);
    if (Buffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Sector = 0;
    Length = BootSector->SectorsPerCluster * BootSector->BytesPerSector;

    while (Sector + BootSector->SectorsPerCluster < BootSector->SectorsHuge)
    {
        FileOffset.QuadPart = Sector * BootSector->BytesPerSector;

        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             Buffer,
                             Length,
                             &FileOffset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
            goto done;
        }

        UpdateProgress(Context, (ULONG)BootSector->SectorsPerCluster);

        Sector += BootSector->SectorsPerCluster;
    }

    if (Sector + BootSector->SectorsPerCluster > BootSector->SectorsHuge)
    {
        DPRINT("Remaining sectors %lu\n", BootSector->SectorsHuge - Sector);

        FileOffset.QuadPart = Sector * BootSector->BytesPerSector;
        Length = (BootSector->SectorsHuge - Sector) * BootSector->BytesPerSector;

        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             Buffer,
                             Length,
                             &FileOffset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
            goto done;
        }

        UpdateProgress(Context, BootSector->SectorsHuge - Sector);
    }

done:
    /* Free the buffer */
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    return Status;
}


NTSTATUS
Fat32Format(IN HANDLE FileHandle,
            IN PPARTITION_INFORMATION PartitionInfo,
            IN PDISK_GEOMETRY DiskGeometry,
            IN PUNICODE_STRING Label,
            IN BOOLEAN QuickFormat,
            IN ULONG ClusterSize,
            IN OUT PFORMAT_CONTEXT Context)
{
    FAT32_BOOT_SECTOR BootSector;
    OEM_STRING VolumeLabel;
    ULONG TmpVal1;
    ULONG TmpVal2;
    NTSTATUS Status;

    /* Calculate cluster size */
    if (ClusterSize == 0)
    {
        if (PartitionInfo->PartitionLength.QuadPart < 8LL * 1024LL * 1024LL * 1024LL)
        {
            /* Partition < 8GB ==> 4KB Cluster */
            ClusterSize = 4096;
        }
        else if (PartitionInfo->PartitionLength.QuadPart < 16LL * 1024LL * 1024LL * 1024LL)
        {
            /* Partition 8GB - 16GB ==> 8KB Cluster */
            ClusterSize = 8192;
        }
        else if (PartitionInfo->PartitionLength.QuadPart < 32LL * 1024LL * 1024LL * 1024LL)
        {
            /* Partition 16GB - 32GB ==> 16KB Cluster */
            ClusterSize = 16384;
        }
        else
        {
            /* Partition >= 32GB ==> 32KB Cluster */
            ClusterSize = 32768;
        }
    }

    RtlZeroMemory(&BootSector, sizeof(FAT32_BOOT_SECTOR));
    memcpy(&BootSector.OEMName[0], "MSWIN4.1", 8);
    BootSector.BytesPerSector = DiskGeometry->BytesPerSector;
    BootSector.SectorsPerCluster = ClusterSize / BootSector.BytesPerSector;
    BootSector.ReservedSectors = 32;
    BootSector.FATCount = 2;
    BootSector.RootEntries = 0;
    BootSector.Sectors = 0;
    BootSector.Media = 0xf8;
    BootSector.FATSectors = 0;
    BootSector.SectorsPerTrack = DiskGeometry->SectorsPerTrack;
    BootSector.Heads = DiskGeometry->TracksPerCylinder;
    BootSector.HiddenSectors = PartitionInfo->HiddenSectors;
    BootSector.SectorsHuge = PartitionInfo->PartitionLength.QuadPart >>
        GetShiftCount(BootSector.BytesPerSector); /* Use shifting to avoid 64-bit division */
    BootSector.FATSectors32 = 0; /* Set later */
    BootSector.ExtFlag = 0; /* Mirror all FATs */
    BootSector.FSVersion = 0x0000; /* 0:0 */
    BootSector.RootCluster = 2;
    BootSector.FSInfoSector = 1;
    BootSector.BootBackup = 6;
    BootSector.Drive = (DiskGeometry->MediaType == FixedMedia) ? 0x80 : 0x00;
    BootSector.ExtBootSignature = 0x29;
    BootSector.VolumeID = CalcVolumeSerialNumber();
    if ((Label == NULL) || (Label->Buffer == NULL))
    {
        memcpy(&BootSector.VolumeLabel[0], "NO NAME    ", 11);
    }
    else
    {
        RtlUnicodeStringToOemString(&VolumeLabel, Label, TRUE);
        RtlFillMemory(&BootSector.VolumeLabel[0], 11, ' ');
        memcpy(&BootSector.VolumeLabel[0], VolumeLabel.Buffer,
               VolumeLabel.Length < 11 ? VolumeLabel.Length : 11);
        RtlFreeOemString(&VolumeLabel);
    }

    memcpy(&BootSector.SysType[0], "FAT32   ", 8);

    /* Calculate number of FAT sectors */
    /* (BytesPerSector / 4) FAT entries (32bit) fit into one sector */
    TmpVal1 = BootSector.SectorsHuge - BootSector.ReservedSectors;
    TmpVal2 = ((BootSector.BytesPerSector / 4) * BootSector.SectorsPerCluster) + BootSector.FATCount;
    BootSector.FATSectors32 = (TmpVal1 + (TmpVal2 - 1)) / TmpVal2;
    DPRINT("FATSectors32 = %lu\n", BootSector.FATSectors32);

    /* Init context data */
    Context->TotalSectorCount =
        2 + (BootSector.FATSectors32 * BootSector.FATCount) + BootSector.SectorsPerCluster;

    if (!QuickFormat)
    {
        Context->TotalSectorCount += BootSector.SectorsHuge;

        Status = Fat32WipeSectors(FileHandle,
                                  &BootSector,
                                  Context);
        if (!NT_SUCCESS(Status))
        {
            DPRINT("Fat32WipeSectors() failed with status 0x%.08x\n", Status);
            return Status;
        }
    }

    Status = Fat32WriteBootSector(FileHandle,
                                  &BootSector,
                                  Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Fat32WriteBootSector() failed with status 0x%.08x\n", Status);
        return Status;
    }

    Status = Fat32WriteFsInfo(FileHandle,
                              &BootSector,
                              Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Fat32WriteFsInfo() failed with status 0x%.08x\n", Status);
        return Status;
    }

    /* Write first FAT copy */
    Status = Fat32WriteFAT(FileHandle,
                           0,
                           &BootSector,
                           Context);
    if (!NT_SUCCESS(Status))
    {
      DPRINT("Fat32WriteFAT() failed with status 0x%.08x\n", Status);
      return Status;
    }

    /* Write second FAT copy */
    Status = Fat32WriteFAT(FileHandle,
                           BootSector.FATSectors32,
                           &BootSector,
                           Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Fat32WriteFAT() failed with status 0x%.08x.\n", Status);
        return Status;
    }

    Status = Fat32WriteRootDirectory(FileHandle,
                                     &BootSector,
                                     Context);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Fat32WriteRootDirectory() failed with status 0x%.08x\n", Status);
    }

    return Status;
}

/* EOF */
