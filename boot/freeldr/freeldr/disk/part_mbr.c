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
DiskGetActivePartitionEntry(
    _In_ UCHAR DriveNumber,
    _In_ ULONG SectorSize,
    _Out_opt_ PPARTITION_INFORMATION PartitionEntry,
    _Out_ PULONG ActivePartition)
{
    MASTER_BOOT_RECORD MasterBootRecord;
    ULONG BootablePartitionCount = 0;
    ULONG CurrentPartitionNumber;
    ULONG Index;

    ASSERT(SectorSize >= 512);

    *ActivePartition = 0;

    /* Read master boot record */
    if (!DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord))
        return FALSE;

    CurrentPartitionNumber = 0;
    for (Index = 0; Index < 4; Index++)
    {
        PPARTITION_TABLE_ENTRY PartitionTableEntry = &MasterBootRecord.PartitionTable[Index];

        if (PartitionTableEntry->SystemIndicator != PARTITION_ENTRY_UNUSED &&
            PartitionTableEntry->SystemIndicator != PARTITION_EXTENDED &&
            PartitionTableEntry->SystemIndicator != PARTITION_XINT13_EXTENDED)
        {
            CurrentPartitionNumber++;

            /* Test if this is the bootable partition */
            if (PartitionTableEntry->BootIndicator == 0x80)
            {
                BootablePartitionCount++;
                *ActivePartition = CurrentPartitionNumber;

                /* Copy the partition table entry */
                if (PartitionEntry)
                {
                    DiskMbrPartitionTableEntryToInformation(PartitionEntry, PartitionTableEntry,
                                                            *ActivePartition, SectorSize);
                }
            }
        }
    }

    /* Make sure there was only one bootable partition */
    if (BootablePartitionCount == 0)
    {
        ERR("No bootable (active) partitions found.\n");
        return FALSE;
    }
    else if (BootablePartitionCount != 1)
    {
        ERR("Too many bootable (active) partitions found.\n");
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
DiskGetMbrPartitionEntry(
    _In_ UCHAR DriveNumber,
    _In_ ULONG SectorSize,
    _In_ ULONG PartitionNumber,
    _Out_ PPARTITION_INFORMATION PartitionEntry)
{
    MASTER_BOOT_RECORD MasterBootRecord;
    PPARTITION_TABLE_ENTRY PartitionTableEntry;
    ULONG ExtendedPartitionNumber;
    ULONG ExtendedPartitionOffset;
    ULONG Index;
    ULONG CurrentPartitionNumber;

    ASSERT(SectorSize >= 512);

    /* Validate partition number */
    if (PartitionNumber < 1) //if (PartitionNumber == 0)
        return FALSE;

    /* Read master boot record */
    if (!DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord))
        return FALSE;

    CurrentPartitionNumber = 0;
    for (Index = 0; Index < 4; Index++)
    {
        PartitionTableEntry = &MasterBootRecord.PartitionTable[Index];

        if (PartitionTableEntry->SystemIndicator != PARTITION_ENTRY_UNUSED &&
            PartitionTableEntry->SystemIndicator != PARTITION_EXTENDED &&
            PartitionTableEntry->SystemIndicator != PARTITION_XINT13_EXTENDED)
        {
            CurrentPartitionNumber++;
        }

        if (PartitionNumber == CurrentPartitionNumber)
        {
            DiskMbrPartitionTableEntryToInformation(PartitionEntry, PartitionTableEntry,
                                                    PartitionNumber, SectorSize);
            return TRUE;
        }
    }

    /*
     * They want an extended partition entry so we will need
     * to loop through all the extended partitions on the disk
     * and return the one they want.
     */
    ExtendedPartitionNumber = PartitionNumber - CurrentPartitionNumber - 1;

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

    /* When we get here we should have the correct entry already
     * stored in PartitionTableEntry, so just return TRUE. */
    DiskMbrPartitionTableEntryToInformation(PartitionEntry, PartitionTableEntry,
                                            PartitionNumber, SectorSize);
    return TRUE;
}

#endif
