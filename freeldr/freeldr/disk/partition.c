/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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

#include <freeldr.h>
#include <disk.h>
#include <rtl.h>
#include <mm.h>
#include <debug.h>



BOOL DiskIsDriveRemovable(ULONG DriveNumber)
{
	// Hard disks use drive numbers >= 0x80
	// So if the drive number indicates a hard disk
	// then return FALSE
	if (DriveNumber >= 0x80)
	{
		return FALSE;
	}

	// Drive is a floppy diskette so return TRUE
	return TRUE;
}

BOOL DiskGetActivePartitionEntry(ULONG DriveNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
	ULONG				BootablePartitionCount = 0;
	MASTER_BOOT_RECORD	MasterBootRecord;

	// Read master boot record
	if (!DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord))
	{
		return FALSE;
	}

	// Count the bootable partitions
	if (MasterBootRecord.PartitionTable[0].BootIndicator == 0x80)
	{
		BootablePartitionCount++;
		BootPartition = 0;
	}
	if (MasterBootRecord.PartitionTable[1].BootIndicator == 0x80)
	{
		BootablePartitionCount++;
		BootPartition = 1;
	}
	if (MasterBootRecord.PartitionTable[2].BootIndicator == 0x80)
	{
		BootablePartitionCount++;
		BootPartition = 2;
	}
	if (MasterBootRecord.PartitionTable[3].BootIndicator == 0x80)
	{
		BootablePartitionCount++;
		BootPartition = 3;
	}

	// Make sure there was only one bootable partition
	if (BootablePartitionCount != 1)
	{
		DiskError("Too many bootable partitions or none found.");
		return FALSE;
	}

	// Copy the partition table entry
	RtlCopyMemory(PartitionTableEntry, &MasterBootRecord.PartitionTable[BootPartition], sizeof(PARTITION_TABLE_ENTRY));

	return TRUE;
}

BOOL DiskGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
	MASTER_BOOT_RECORD		MasterBootRecord;
	PARTITION_TABLE_ENTRY	ExtendedPartitionTableEntry;
	ULONG					ExtendedPartitionNumber;
	ULONG					Index;

	// Read master boot record
	if (!DiskReadBootRecord(DriveNumber, 0, &MasterBootRecord))
	{
		return FALSE;
	}

	// If they are asking for a primary
	// partition then things are easy
	if (PartitionNumber < 5)
	{
		// PartitionNumber is one-based and we need it zero-based
		PartitionNumber--;

		// Copy the partition table entry
		RtlCopyMemory(PartitionTableEntry, &MasterBootRecord.PartitionTable[PartitionNumber], sizeof(PARTITION_TABLE_ENTRY));

		return TRUE;
	}
	else
	{
		// They want an extended partition entry so we will need
		// to loop through all the extended partitions on the disk
		// and return the one they want.
		
		ExtendedPartitionNumber = PartitionNumber - 5;

		for (Index=0; Index<=ExtendedPartitionNumber; Index++)
		{
			// Get the extended partition table entry
			if (!DiskGetFirstExtendedPartitionEntry(&MasterBootRecord, &ExtendedPartitionTableEntry))
			{
				return FALSE;
			}

			// Read the partition boot record
			if (!DiskReadBootRecord(DriveNumber, ExtendedPartitionTableEntry.SectorCountBeforePartition, &MasterBootRecord))
			{
				return FALSE;
			}

			// Get the first real partition table entry
			if (!DiskGetFirstPartitionEntry(&MasterBootRecord, PartitionTableEntry))
			{
				return FALSE;
			}
		}

		// When we get here we should have the correct entry
		// already stored in PartitionTableEntry
		// so just return TRUE
		return TRUE;
	}

}

BOOL DiskGetFirstPartitionEntry(PMASTER_BOOT_RECORD MasterBootRecord, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
	ULONG	Index;

	for (Index=0; Index<4; Index++)
	{
		// Check the system indicator
		// If it's not an extended or unused partition
		// then we're done
		if ((MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_ENTRY_UNUSED) &&
			(MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_EXTENDED) &&
			(MasterBootRecord->PartitionTable[Index].SystemIndicator != PARTITION_XINT13_EXTENDED))
		{
			RtlCopyMemory(PartitionTableEntry, &MasterBootRecord->PartitionTable[Index], sizeof(PARTITION_TABLE_ENTRY));
			return TRUE;
		}
	}

	return FALSE;
}

BOOL DiskGetFirstExtendedPartitionEntry(PMASTER_BOOT_RECORD MasterBootRecord, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
	ULONG	Index;

	for (Index=0; Index<4; Index++)
	{
		// Check the system indicator
		// If it an extended partition then we're done
		if ((MasterBootRecord->PartitionTable[Index].SystemIndicator == PARTITION_EXTENDED) ||
			(MasterBootRecord->PartitionTable[Index].SystemIndicator == PARTITION_XINT13_EXTENDED))
		{
			RtlCopyMemory(PartitionTableEntry, &MasterBootRecord->PartitionTable[Index], sizeof(PARTITION_TABLE_ENTRY));
			return TRUE;
		}
	}

	return FALSE;
}

BOOL DiskReadBootRecord(ULONG DriveNumber, ULONG LogicalSectorNumber, PMASTER_BOOT_RECORD BootRecord)
{
	ULONG	Index;

	// Read master boot record
	if (!DiskReadLogicalSectors(DriveNumber, LogicalSectorNumber, 1, BootRecord))
	{
		return FALSE;
	}


#ifdef DEBUG

	DbgPrint((DPRINT_DISK, "Dumping partition table for drive 0x%x:\n", DriveNumber));
	DbgPrint((DPRINT_DISK, "Boot record logical start sector = %d\n", LogicalSectorNumber));
	DbgPrint((DPRINT_DISK, "sizeof(MASTER_BOOT_RECORD) = 0x%x.\n", sizeof(MASTER_BOOT_RECORD)));

	for (Index=0; Index<4; Index++)
	{
		DbgPrint((DPRINT_DISK, "-------------------------------------------\n"));
		DbgPrint((DPRINT_DISK, "Partition %d\n", (Index + 1)));
		DbgPrint((DPRINT_DISK, "BootIndicator: 0x%x\n", BootRecord->PartitionTable[Index].BootIndicator));
		DbgPrint((DPRINT_DISK, "StartHead: 0x%x\n", BootRecord->PartitionTable[Index].StartHead));
		DbgPrint((DPRINT_DISK, "StartSector (Plus 2 cylinder bits): 0x%x\n", BootRecord->PartitionTable[Index].StartSector));
		DbgPrint((DPRINT_DISK, "StartCylinder: 0x%x\n", BootRecord->PartitionTable[Index].StartCylinder));
		DbgPrint((DPRINT_DISK, "SystemIndicator: 0x%x\n", BootRecord->PartitionTable[Index].SystemIndicator));
		DbgPrint((DPRINT_DISK, "EndHead: 0x%x\n", BootRecord->PartitionTable[Index].EndHead));
		DbgPrint((DPRINT_DISK, "EndSector (Plus 2 cylinder bits): 0x%x\n", BootRecord->PartitionTable[Index].EndSector));
		DbgPrint((DPRINT_DISK, "EndCylinder: 0x%x\n", BootRecord->PartitionTable[Index].EndCylinder));
		DbgPrint((DPRINT_DISK, "SectorCountBeforePartition: 0x%x\n", BootRecord->PartitionTable[Index].SectorCountBeforePartition));
		DbgPrint((DPRINT_DISK, "PartitionSectorCount: 0x%x\n", BootRecord->PartitionTable[Index].PartitionSectorCount));
	}

#endif // defined DEBUG

	// Check the partition table magic value
	if (BootRecord->MasterBootRecordMagic != 0xaa55)
	{
		DiskError("Invalid partition table magic (0xaa55)");
		return FALSE;
	}

	return TRUE;
}
