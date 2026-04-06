/*
 * PROJECT:     FreeLoader
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Header file for ramdisk support.
 * COPYRIGHT:   Copyright 2008 ReactOS Portable Systems Group
 *              Copyright 2009 Hervé Poussineau
 *              Copyright 2019 Hermes Belusca-Maito
 */

#pragma once

typedef struct _RAMDISK_FAT32_LAYOUT
{
    ULONG BytesPerSector;
    ULONG SectorsPerCluster;
    ULONG ReservedSectors;
    ULONG NumberOfFats;
    ULONG FatSizeSectors;
    ULONG FirstDataSector;
    ULONG RootDirFirstCluster;
    ULONG TotalSectors;
    ULONG HiddenSectors;
    ULONGLONG VolumeSizeBytes;
} RAMDISK_FAT32_LAYOUT, *PRAMDISK_FAT32_LAYOUT;

ARC_STATUS
RamDiskInitialize(
    IN BOOLEAN InitRamDisk,
    IN PCSTR LoadOptions OPTIONAL,
    IN PCSTR DefaultPath OPTIONAL);

ULONGLONG
RamDiskGetRequestedSize(VOID);

BOOLEAN
RamDiskGetReservedBuffer(IN ULONGLONG MinimumSize,
                         OUT PVOID *BaseAddress,
                         OUT PULONGLONG ActualSize);

ULONGLONG
RamDiskGetImageLength(VOID);

ULONG
RamDiskGetImageOffset(VOID);

ULONGLONG
RamDiskGetVolumeOffset(VOID);

BOOLEAN
RamDiskFormatFat32(IN PVOID BaseAddress,
                   IN ULONGLONG DiskSize,
                   OUT PRAMDISK_FAT32_LAYOUT Layout OPTIONAL);

extern PVOID gInitRamDiskBase;
extern ULONG gInitRamDiskSize;

/*
 * RamDiskGetBackingStore - Get ramdisk backing store information for kernel.
 *
 * Returns the base page and page count of the ramdisk backing store.
 * This information is used by the kernel's memory manager to mark these
 * pages as non-reclaimable (ROM) in the PFN database, ensuring the ramdisk
 * backing store remains stable throughout the system lifetime.
 *
 * @param BasePage - Receives the base page frame number of the ramdisk.
 * @param PageCount - Receives the number of pages in the ramdisk.
 *
 * @return TRUE if ramdisk is active and parameters are set, FALSE otherwise.
 */
BOOLEAN
RamDiskGetBackingStore(
    OUT PFN_NUMBER *BasePage,
    OUT PFN_NUMBER *PageCount);
