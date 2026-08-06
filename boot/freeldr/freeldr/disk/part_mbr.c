/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     MBR partitioning scheme support
 * COPYRIGHT:   Copyright 2002-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2016 Wim Hueskes
 */

#ifndef _M_ARM
// #include <freeldr.h>
#include "part_mbr.h"

// #include <debug.h>
// DBG_DEFAULT_CHANNEL(DISK);

static VOID
DiskMbrPartitionTableEntryToInformation(
    _Out_ PPARTITION_INFORMATION PartitionEntry,
    _In_ PPARTITION_TABLE_ENTRY PartitionTableEntry,
    _In_ ULONG PartitionNumber,
    _In_ ULONG SectorSize)
{
    PartitionEntry->StartingOffset.QuadPart  = (ULONGLONG)PartitionTableEntry->SectorCountBeforePartition * SectorSize;
    PartitionEntry->PartitionLength.QuadPart = (ULONGLONG)PartitionTableEntry->PartitionSectorCount * SectorSize;
    PartitionEntry->HiddenSectors = 0;
    PartitionEntry->PartitionNumber = PartitionNumber;
    PartitionEntry->PartitionType = PartitionTableEntry->SystemIndicator;
    PartitionEntry->BootIndicator = (PartitionTableEntry->BootIndicator == 0x80); // or "& 0x80"
    PartitionEntry->RecognizedPartition = TRUE;
    PartitionEntry->RewritePartition = FALSE;
}

static BOOLEAN
DiskGetExtendedMbrPartitionEntry(
    _In_ UCHAR DriveNumber,
    _In_ ULONG SectorSize,
    _In_ ULONG PartitionNumber,
    _In_ PPARTITION_TABLE_ENTRY ExtendedPartitionTableEntry,
    _Out_ PPARTITION_INFORMATION PartitionEntry)
{
    ULONG EbrIndex;
    MASTER_BOOT_RECORD ExtendedBootRecord;

    ULONG ExtendedBootRecordBaseSector = ExtendedPartitionTableEntry->SectorCountBeforePartition;
    ULONG ExtendedBootRecordCurrentSector = ExtendedBootRecordBaseSector;
    ULONG ExtendedPartitionNumber = PartitionNumber - 5;

    for (EbrIndex = 0; EbrIndex <= ExtendedPartitionNumber; EbrIndex++)
    {
        /* Read the partition boot record */
        if (!DiskReadBootRecord(DriveNumber, ExtendedBootRecordCurrentSector, &ExtendedBootRecord))
        {
            return FALSE;
        }

        TRACE("EbrIndex = %u\n", EbrIndex);

        PPARTITION_TABLE_ENTRY Logical = &ExtendedBootRecord.PartitionTable[0];
        PPARTITION_TABLE_ENTRY Next = &ExtendedBootRecord.PartitionTable[1];

        if (EbrIndex == ExtendedPartitionNumber)
        {
            /* Now correct the start sector of the partition */
            Logical->SectorCountBeforePartition += ExtendedBootRecordCurrentSector;

            DiskMbrPartitionTableEntryToInformation(PartitionEntry, Logical, PartitionNumber, SectorSize);
            return TRUE;
        }

        if (!Next->SectorCountBeforePartition)
        {
            return FALSE;
        }

        /* Move to next EBR (relative to base EBR) */
        ExtendedBootRecordCurrentSector = ExtendedBootRecordBaseSector + Next->SectorCountBeforePartition;
    }

    return FALSE;
}

BOOLEAN
DiskGetMbrPartitionEntry(
    _In_ UCHAR DriveNumber,
    _In_ ULONG SectorSize,
    _In_ ULONG PartitionNumber,
    _Out_ PPARTITION_INFORMATION PartitionEntry,
    _In_ BOOLEAN IgnoreUnusedFlag)
{
    BOOLEAN Result = TRUE;
    MASTER_BOOT_RECORD MasterBootRecord;
    ULONG Index;
    PPARTITION_TABLE_ENTRY ThisPartitionTableEntry;

    /* Read master boot record */
    if (!DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord))
        return FALSE;

    for (Index = 0; Index < 4; Index++)
    {
        ThisPartitionTableEntry = &MasterBootRecord.PartitionTable[Index];

        if (PartitionNumber > 4 &&
            (ThisPartitionTableEntry->SystemIndicator == PARTITION_EXTENDED ||
             ThisPartitionTableEntry->SystemIndicator == PARTITION_XINT13_EXTENDED))
        {
            Result = DiskGetExtendedMbrPartitionEntry(DriveNumber, SectorSize, PartitionNumber, ThisPartitionTableEntry, PartitionEntry);
            break;
        }

        if (PartitionNumber == Index + 1)
        {
            DiskMbrPartitionTableEntryToInformation(PartitionEntry, ThisPartitionTableEntry, PartitionNumber, SectorSize);
            break;
        }
    }

    /* Check if partition is usable when the flag is not ignored */
    if (!IgnoreUnusedFlag)
        Result &= PartitionEntry->PartitionType != PARTITION_ENTRY_UNUSED &&
                  PartitionEntry->PartitionType != PARTITION_EXTENDED &&
                  PartitionEntry->PartitionType != PARTITION_XINT13_EXTENDED;

    return Result;
}

#endif
