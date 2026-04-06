/*
 * PROJECT:     FreeLoader
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Minimal write-only FAT32 populator for in-memory ramdisks
 * COPYRIGHT:   Copyright 2025 Ahmed Arif <arif.ing@outlook.com>
 */

#ifndef _RAMDISK_FATWRITE_H_
#define _RAMDISK_FATWRITE_H_

#include <ramdisk.h>

typedef struct _FAT32_WRITER
{
    PUCHAR VolumeBase;          /* Base pointer of the FAT32 volume (after MBR) */
    ULONGLONG VolumeSize;       /* Size in bytes */

    /* Geometry (from RAMDISK_FAT32_LAYOUT) */
    ULONG BytesPerSector;       /* Always 512 */
    ULONG SectorsPerCluster;
    ULONG BytesPerCluster;      /* Cached: BytesPerSector * SectorsPerCluster */
    ULONG ReservedSectors;
    ULONG NumberOfFats;
    ULONG FatSizeSectors;
    ULONG FirstDataSector;      /* ReservedSectors + NumberOfFats * FatSizeSectors */
    ULONG RootDirCluster;       /* Always 2 */
    ULONG TotalClusters;

    /* Direct FAT pointers */
    PULONG Fat0;                /* First FAT table in memory */
    PULONG Fat1;                /* Second FAT table (NULL if NumberOfFats < 2) */

    /* Sequential allocator state */
    ULONG NextFreeCluster;      /* Next cluster to allocate (starts at 3) */

    /* Fixed timestamp for all entries */
    USHORT FatDate;
    USHORT FatTime;
} FAT32_WRITER, *PFAT32_WRITER;

BOOLEAN
Fat32WriterInit(
    _Out_ PFAT32_WRITER Writer,
    _In_ PVOID VolumeBase,
    _In_ ULONGLONG VolumeSize,
    _In_ PRAMDISK_FAT32_LAYOUT Layout);

BOOLEAN
Fat32CreateDirectory(
    _Inout_ PFAT32_WRITER Writer,
    _In_ PCSTR Path);

BOOLEAN
Fat32CreateFileEx(
    _Inout_ PFAT32_WRITER Writer,
    _In_ PCSTR Path,
    _In_ ULONG FileSize,
    _Out_ PUCHAR *DataPointer);

#endif /* _RAMDISK_FATWRITE_H_ */
