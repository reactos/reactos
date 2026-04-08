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

static BOOLEAN
DiskGetFirstPartitionEntry(
    _In_ PMASTER_BOOT_RECORD MasterBootRecord,
    _Out_ PPARTITION_TABLE_ENTRY* pPartitionTableEntry)
{
    ULONG Index;

    for (Index = 0; Index < 4; Index++)
    {
        /* Check the system indicator. If it's not an extended or unused partition then we're done. */
        if ((MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_ENTRY_UNUSED) &&
            (MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_EXTENDED) &&
            (MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_XINT13_EXTENDED))
        {
            *pPartitionTableEntry = &MasterBootRecord->PartitionTable[Index];
            return TRUE;
        }
    }

    return FALSE;
}

static BOOLEAN
DiskGetFirstExtendedPartitionEntry(
    _In_ PMASTER_BOOT_RECORD MasterBootRecord,
    _Out_ PPARTITION_TABLE_ENTRY* pPartitionTableEntry)
{
    ULONG Index;

    for (Index = 0; Index < 4; Index++)
    {
        /* Check the system indicator. If it an extended partition then we're done. */
        if ((MasterBootRecord->PartitionTable[Index].SystemIndicator == PARTITION_EXTENDED) ||
            (MasterBootRecord->PartitionTable[Index].SystemIndicator == PARTITION_XINT13_EXTENDED))
        {
            *pPartitionTableEntry = &MasterBootRecord->PartitionTable[Index];
            return TRUE;
        }
    }

    return FALSE;
}

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
    PPARTITION_TABLE_ENTRY PartitionTableEntry;
    ULONG ExtendedPartitionNumber;
    ULONG ExtendedPartitionOffset;
    ULONG Index;
    PPARTITION_TABLE_ENTRY ThisPartitionTableEntry;
    PARTITION_TABLE_ENTRY PartitionTableEntry = {0};

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
            Result = DiskGetExtendedMbrPartitionEntry(DriveNumber, PartitionNumber, ThisPartitionTableEntry, PartitionEntry);
            break;
        }

        if (PartitionNumber == Index + 1)
        {
            DiskMbrPartitionTableEntryToInformation(PartitionEntry, ThisPartitionTableEntry, PartitionNumber, SectorSize);
            break;
        }
    }

    /*
     * They want an extended partition entry so we will need
     * to loop through all the extended partitions on the disk
     * and return the one they want.
     */
    ExtendedPartitionNumber = PartitionNumber - 4;

    /*
     * Set the initial relative starting sector to 0.
     * This is because extended partition starting
     * sectors a numbered relative to their parent.
     */
    ExtendedPartitionOffset = 0;

    for (Index = 0; Index <= ExtendedPartitionNumber; Index++)
    {
        PPARTITION_TABLE_ENTRY ExtendedPartitionTableEntry;
        ULONG SectorCountBeforePartition;

        /* Get the extended partition table entry */
        if (!DiskGetFirstExtendedPartitionEntry(&MasterBootRecord, &ExtendedPartitionTableEntry))
            return FALSE;

        /* Adjust the relative starting sector of the partition */
        ExtendedPartitionTableEntry->SectorCountBeforePartition += ExtendedPartitionOffset;
        if (ExtendedPartitionOffset == 0)
        {
            /* Set the start of the parent extended partition */
            ExtendedPartitionOffset = ExtendedPartitionTableEntry->SectorCountBeforePartition;
        }
        SectorCountBeforePartition = ExtendedPartitionTableEntry->SectorCountBeforePartition;

        /* Read the partition boot record */
        if (!DiskReadBootRecord(DriveNumber, SectorCountBeforePartition, &MasterBootRecord))
            return FALSE;

        /* Get the first real partition table entry */
        if (!DiskGetFirstPartitionEntry(&MasterBootRecord, &PartitionTableEntry))
            return FALSE;

        /* Now correct the start sector of the partition */
        PartitionTableEntry->SectorCountBeforePartition += SectorCountBeforePartition;
    }

    /* Check if partition is usable when the flag is not ignored */
    if (!IgnoreUnusedFlag)
        Result &= PartitionEntry->PartitionType != PARTITION_ENTRY_UNUSED &&
                  PartitionEntry->PartitionType != PARTITION_EXTENDED &&
                  PartitionEntry->PartitionType != PARTITION_XINT13_EXTENDED;

    return Result;
}

#endif
