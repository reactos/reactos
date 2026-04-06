/*
 * PROJECT:     FreeLoader
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Helpers to create a writable FAT32 ramdisk image in memory
 * COPYRIGHT:   Copyright 2025 Ahmed Arif <arif.ing@outlook.com>
 */

#include <freeldr.h>
#include <debug.h>
#include <fs/fat.h>
#include <disk.h>
#include "ramdisk.h"
#include "ramdisk_signature.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif
#define DEFAULT_BYTES_PER_SECTOR    512U
#define DEFAULT_RESERVED_SECTORS    32U
#define DEFAULT_NUM_FATS            2U
#define DEFAULT_MEDIA_DESCRIPTOR    0xF8U
#define DEFAULT_SECTORS_PER_TRACK   63U
#define DEFAULT_NUMBER_OF_HEADS     255U
#define DEFAULT_ROOT_DIR_CLUSTER    2U
#define DEFAULT_BACKUP_BOOT_SECTOR  6U

#define FAT32_MIN_CLUSTERS          65525U
#define FAT32_MAX_CLUSTERS          0x0FFFFFF5U

typedef struct _FAT32_FSINFO_RECORD
{
    ULONG LeadSignature;
    UCHAR Reserved1[480];
    ULONG StructSignature;
    ULONG FreeCount;
    ULONG NextFree;
    UCHAR Reserved2[12];
    ULONG TrailSignature;
} FAT32_FSINFO_RECORD, *PFAT32_FSINFO_RECORD;

static
BOOLEAN
ComputeFatLayout(IN ULONG TotalSectors,
                 IN ULONG ReservedSectors,
                 IN ULONG BytesPerSector,
                 IN ULONG NumberOfFats,
                 IN ULONG SectorsPerCluster,
                 OUT ULONG *FatSizeSectors,
                 OUT ULONG *DataSectors,
                 OUT ULONG *ClusterCount)
{
    ULONGLONG FatSize = 0;
    ULONG Iteration;

    for (Iteration = 0; Iteration < 128; ++Iteration)
    {
        ULONGLONG CalcDataSectors;
        ULONGLONG TotalClusters;
        ULONGLONG RequiredEntries;
        ULONGLONG FatBytes;
        ULONGLONG NewFatSize;

        if (TotalSectors < ReservedSectors)
            return FALSE;

        CalcDataSectors = (ULONGLONG)TotalSectors - ReservedSectors - NumberOfFats * FatSize;
        if ((LONGLONG)CalcDataSectors <= 0)
            return FALSE;

        TotalClusters = CalcDataSectors / SectorsPerCluster;
        if (TotalClusters > FAT32_MAX_CLUSTERS)
            return FALSE;

        RequiredEntries = TotalClusters + 2ULL;
        FatBytes = RequiredEntries * sizeof(ULONG);
        NewFatSize = (FatBytes + BytesPerSector - 1) / BytesPerSector;
        if (NewFatSize == FatSize)
        {
            *FatSizeSectors = (ULONG)FatSize;
            *DataSectors = (ULONG)CalcDataSectors;
            *ClusterCount = (ULONG)TotalClusters;
            return TRUE;
        }

        if (NewFatSize == 0 || NewFatSize > 0xFFFFFFFFULL)
            return FALSE;

        FatSize = NewFatSize;
    }

    return FALSE;
}

static
BOOLEAN
FormatFat32Volume(IN PUCHAR VolumeBase,
                  IN ULONGLONG VolumeSize,
                  IN ULONG HiddenSectors,
                  OUT PRAMDISK_FAT32_LAYOUT Layout)
{
    static const UCHAR ClusterCandidates[] = { 128, 64, 32, 16, 8, 4, 2, 1 };
    PFAT32_BOOTSECTOR BootSector;
    PFAT32_FSINFO_RECORD FsInfo;
    ULONG BytesPerSector = DEFAULT_BYTES_PER_SECTOR;
    ULONG ReservedSectors = DEFAULT_RESERVED_SECTORS;
    ULONG NumberOfFats = DEFAULT_NUM_FATS;
    ULONG TotalSectors;
    ULONG FatSize = 0;
    ULONG DataSectors = 0;
    ULONG ClusterCount = 0;
    ULONG SectorsPerCluster = 0;
    ULONG FirstDataSector;
    ULONG CandidateIndex;
    SIZE_T ByteSize;
    PUCHAR Buffer;
    PULONG FatTable;

    if (!VolumeBase)
        return FALSE;

    if (VolumeSize == 0 || VolumeSize > MAXULONG)
        return FALSE;

    if ((VolumeSize % BytesPerSector) != 0)
        return FALSE;

    TotalSectors = (ULONG)(VolumeSize / BytesPerSector);
    if (TotalSectors <= (ReservedSectors + NumberOfFats))
        return FALSE;

    for (CandidateIndex = 0; CandidateIndex < ARRAYSIZE(ClusterCandidates); ++CandidateIndex)
    {
        ULONG Candidate = ClusterCandidates[CandidateIndex];

        if (!ComputeFatLayout(TotalSectors,
                              ReservedSectors,
                              BytesPerSector,
                              NumberOfFats,
                              Candidate,
                              &FatSize,
                              &DataSectors,
                              &ClusterCount))
        {
            continue;
        }

        if (ClusterCount >= FAT32_MIN_CLUSTERS && ClusterCount < FAT32_MAX_CLUSTERS)
        {
            SectorsPerCluster = Candidate;
            break;
        }
    }

    if (SectorsPerCluster == 0)
        return FALSE;

    ByteSize = (SIZE_T)VolumeSize;
    /* Zero the entire volume to prevent information leaks from prior boot
     * memory contents and to ensure all FAT/directory regions start clean. */
    RtlZeroMemory(VolumeBase, ByteSize);

    BootSector = (PFAT32_BOOTSECTOR)VolumeBase;
    BootSector->JumpBoot[0] = 0xEB;
    BootSector->JumpBoot[1] = 0x58;
    BootSector->JumpBoot[2] = 0x90;
    RtlCopyMemory(BootSector->OemName, "MSWIN4.1", sizeof(BootSector->OemName));
    BootSector->BytesPerSector = BytesPerSector;
    BootSector->SectorsPerCluster = (UCHAR)SectorsPerCluster;
    BootSector->ReservedSectors = ReservedSectors;
    BootSector->NumberOfFats = (UCHAR)NumberOfFats;
    BootSector->RootDirEntries = 0;
    BootSector->TotalSectors = 0;
    BootSector->MediaDescriptor = DEFAULT_MEDIA_DESCRIPTOR;
    BootSector->SectorsPerFat = 0;
    BootSector->SectorsPerTrack = DEFAULT_SECTORS_PER_TRACK;
    BootSector->NumberOfHeads = DEFAULT_NUMBER_OF_HEADS;
    BootSector->HiddenSectors = HiddenSectors;
    BootSector->TotalSectorsBig = TotalSectors;
    BootSector->SectorsPerFatBig = FatSize;
    BootSector->ExtendedFlags = 0;
    BootSector->FileSystemVersion = 0;
    BootSector->RootDirStartCluster = DEFAULT_ROOT_DIR_CLUSTER;
    BootSector->FsInfo = 1;
    BootSector->BackupBootSector = DEFAULT_BACKUP_BOOT_SECTOR;
    RtlZeroMemory(BootSector->Reserved, sizeof(BootSector->Reserved));
    BootSector->DriveNumber = 0x80;
    BootSector->Reserved1 = 0;
    BootSector->BootSignature = 0x29;
    BootSector->VolumeSerialNumber = 0x20250101;
    RtlCopyMemory(BootSector->VolumeLabel, "REACTOS    ", sizeof(BootSector->VolumeLabel));
    RtlCopyMemory(BootSector->FileSystemType, "FAT32   ", sizeof(BootSector->FileSystemType));
    BootSector->BootSectorMagic = 0xAA55;

    FsInfo = (PFAT32_FSINFO_RECORD)(VolumeBase + BytesPerSector * BootSector->FsInfo);
    FsInfo->LeadSignature = 0x41615252;
    RtlZeroMemory(FsInfo->Reserved1, sizeof(FsInfo->Reserved1));
    FsInfo->StructSignature = 0x61417272;
    FsInfo->FreeCount = 0xFFFFFFFF;
    FsInfo->NextFree = 0xFFFFFFFF;
    RtlZeroMemory(FsInfo->Reserved2, sizeof(FsInfo->Reserved2));
    FsInfo->TrailSignature = 0xAA550000;

    /* Create backup copies of the boot sector and FSINFO if they fit in the reserved region. */
    if (BootSector->BackupBootSector < ReservedSectors)
    {
        PUCHAR BackupBoot = VolumeBase + BytesPerSector * BootSector->BackupBootSector;
        RtlCopyMemory(BackupBoot, BootSector, BytesPerSector);

        if (BootSector->BackupBootSector + 1U < ReservedSectors)
        {
            PUCHAR BackupFsInfo = VolumeBase + BytesPerSector * (BootSector->BackupBootSector + 1U);
            RtlCopyMemory(BackupFsInfo, FsInfo, BytesPerSector);
        }
    }

    /* Initialize both FATs. */
    Buffer = VolumeBase + (ReservedSectors * BytesPerSector);
    ULONG FatIndex;
    for (FatIndex = 0; FatIndex < NumberOfFats; ++FatIndex)
    {
        FatTable = (PULONG)(Buffer + ((ULONGLONG)FatIndex * FatSize * BytesPerSector));
        RtlZeroMemory(FatTable, (ULONGLONG)FatSize * BytesPerSector);
        FatTable[0] = 0x0FFFFFF8 | DEFAULT_MEDIA_DESCRIPTOR;
        FatTable[1] = 0xFFFFFFFF;
        FatTable[2] = 0x0FFFFFFF; /* Root directory cluster */
    }

    FirstDataSector = ReservedSectors + (NumberOfFats * FatSize);

    if (Layout)
    {
        Layout->BytesPerSector = BytesPerSector;
        Layout->SectorsPerCluster = SectorsPerCluster;
        Layout->ReservedSectors = ReservedSectors;
        Layout->NumberOfFats = NumberOfFats;
        Layout->FatSizeSectors = FatSize;
        Layout->FirstDataSector = FirstDataSector;
        Layout->RootDirFirstCluster = DEFAULT_ROOT_DIR_CLUSTER;
        Layout->TotalSectors = TotalSectors;
        Layout->HiddenSectors = HiddenSectors;
        Layout->VolumeSizeBytes = VolumeSize;
    }

    return TRUE;
}

BOOLEAN
RamDiskFormatFat32(IN PVOID BaseAddress,
                   IN ULONGLONG DiskSize,
                   OUT PRAMDISK_FAT32_LAYOUT Layout)
{
    const ULONG BytesPerSector = DEFAULT_BYTES_PER_SECTOR;
    ULONGLONG MinimumSize;
    PUCHAR VolumeBase;
    ULONGLONG VolumeSize;
    RAMDISK_FAT32_LAYOUT LocalLayout;
    PRAMDISK_FAT32_LAYOUT LayoutOut;
    PMASTER_BOOT_RECORD MasterBootRecord;
    ULONG PartitionSectors;

    if (!BaseAddress)
        return FALSE;

    MinimumSize = (ULONGLONG)BytesPerSector * 2ULL;
    if (DiskSize < MinimumSize || (DiskSize % BytesPerSector) != 0)
        return FALSE;

    VolumeBase = (PUCHAR)BaseAddress + BytesPerSector;
    VolumeSize = DiskSize - BytesPerSector;
    LayoutOut = Layout ? Layout : &LocalLayout;

    if (!FormatFat32Volume(VolumeBase, VolumeSize, 1, LayoutOut))
        return FALSE;

    MasterBootRecord = (PMASTER_BOOT_RECORD)BaseAddress;
    RtlZeroMemory(MasterBootRecord, BytesPerSector);

    MasterBootRecord->Signature = RamDiskDeriveDiskSignature(BaseAddress, DiskSize);
    MasterBootRecord->PartitionTable[0].BootIndicator = 0x80;
    MasterBootRecord->PartitionTable[0].StartHead = 0x00;
    MasterBootRecord->PartitionTable[0].StartSector = 0x02; /* LBA 1 */
    MasterBootRecord->PartitionTable[0].StartCylinder = 0x00;
    MasterBootRecord->PartitionTable[0].SystemIndicator = PARTITION_FAT32_XINT13;
    MasterBootRecord->PartitionTable[0].EndHead = 0xFE;
    MasterBootRecord->PartitionTable[0].EndSector = 0xFF;
    MasterBootRecord->PartitionTable[0].EndCylinder = 0xFF;
    MasterBootRecord->PartitionTable[0].SectorCountBeforePartition = 1;

    PartitionSectors = LayoutOut->TotalSectors;
    if (PartitionSectors == 0)
        return FALSE;

    MasterBootRecord->PartitionTable[0].PartitionSectorCount = PartitionSectors;
    MasterBootRecord->MasterBootRecordMagic = 0xAA55;

    return TRUE;
}
