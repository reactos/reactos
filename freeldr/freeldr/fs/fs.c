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
#include <fs.h>
#include "filesys.h"
#include "fat.h"
#include <disk.h>
#include <rtl.h>
#include <ui.h>
#include <asmcode.h>
#include <debug.h>


/////////////////////////////////////////////////////////////////////////////////////////////
// DATA
/////////////////////////////////////////////////////////////////////////////////////////////

ULONG		FileSystemType = 0;	// Type of filesystem on boot device, set by OpenDiskDrive()

/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

VOID FileSystemError(PUCHAR ErrorString)
{
	DbgPrint((DPRINT_FILESYSTEM, "%s\n", ErrorString));

	if (UserInterfaceUp)
	{
		MessageBox(ErrorString);
	}
	else
	{
		printf("%s", ErrorString);
		printf("\nPress any key\n");
		getch();
	}
}

/*
 *
 * BOOL OpenDiskDrive(ULONG DriveNumber, ULONG PartitionNumber);
 *
 * This function is called to open a disk drive for file access.
 * It must be called before any of the file functions will work.
 * It takes two parameters:
 *
 * Drive: The BIOS drive number of the disk to open
 * Partition: This is zero for floppy drives.
 *            If the disk is a hard disk then this specifies
 *            The partition number to open (1 - 4)
 *            If it is zero then it opens the active (bootable) partition
 *
 */
BOOL OpenDiskDrive(ULONG DriveNumber, ULONG PartitionNumber)
{
	MASTER_BOOT_RECORD		DriveMasterBootRecord;
	PARTITION_TABLE_ENTRY	PartitionTableEntry;

	DbgPrint((DPRINT_FILESYSTEM, "OpenDiskDrive() DriveNumber: 0x%x PartitionNumber: 0x%x\n", DriveNumber, PartitionNumber));

	// Check and see if it is a floppy drive
	// If so then just assume FAT12 file system type
	if (FsInternalIsDiskPartitioned(DriveNumber) == FALSE)
	{
		DbgPrint((DPRINT_FILESYSTEM, "Drive is a floppy diskette drive. Assuming FAT12 file system.\n"));

		FileSystemType = FS_FAT;
		return FatOpenVolume(DriveNumber, 0);
	}

	//
	// Read master boot record
	//
	if (!BiosInt13Read(DriveNumber, 0, 0, 1, 1, &DriveMasterBootRecord))
	{
		FileSystemError("Disk read error.");
		return FALSE;
	}


#ifdef DEBUG

	DbgPrint((DPRINT_FILESYSTEM, "Drive is a hard disk, dumping partition table:\n"));
	DbgPrint((DPRINT_FILESYSTEM, "sizeof(MASTER_BOOT_RECORD) = 0x%x.\n", sizeof(MASTER_BOOT_RECORD)));

	for (BootPartition=0; BootPartition<4; BootPartition++)
	{
		DbgPrint((DPRINT_FILESYSTEM, "-------------------------------------------\n"));
		DbgPrint((DPRINT_FILESYSTEM, "Partition %d\n", (BootPartition + 1)));
		DbgPrint((DPRINT_FILESYSTEM, "BootIndicator: 0x%x\n", DriveMasterBootRecord.PartitionTable[BootPartition].BootIndicator));
		DbgPrint((DPRINT_FILESYSTEM, "StartHead: 0x%x\n", DriveMasterBootRecord.PartitionTable[BootPartition].StartHead));
		DbgPrint((DPRINT_FILESYSTEM, "StartSector (Plus 2 cylinder bits): 0x%x\n", DriveMasterBootRecord.PartitionTable[BootPartition].StartSector));
		DbgPrint((DPRINT_FILESYSTEM, "StartCylinder: 0x%x\n", DriveMasterBootRecord.PartitionTable[BootPartition].StartCylinder));
		DbgPrint((DPRINT_FILESYSTEM, "SystemIndicator: 0x%x\n", DriveMasterBootRecord.PartitionTable[BootPartition].SystemIndicator));
		DbgPrint((DPRINT_FILESYSTEM, "EndHead: 0x%x\n", DriveMasterBootRecord.PartitionTable[BootPartition].EndHead));
		DbgPrint((DPRINT_FILESYSTEM, "EndSector (Plus 2 cylinder bits): 0x%x\n", DriveMasterBootRecord.PartitionTable[BootPartition].EndSector));
		DbgPrint((DPRINT_FILESYSTEM, "EndCylinder: 0x%x\n", DriveMasterBootRecord.PartitionTable[BootPartition].EndCylinder));
		DbgPrint((DPRINT_FILESYSTEM, "SectorCountBeforePartition: 0x%x\n", DriveMasterBootRecord.PartitionTable[BootPartition].SectorCountBeforePartition));
		DbgPrint((DPRINT_FILESYSTEM, "PartitionSectorCount: 0x%x\n", DriveMasterBootRecord.PartitionTable[BootPartition].PartitionSectorCount));
	}

#endif // defined DEBUG


	// Check the partition table magic value
	if (DriveMasterBootRecord.MasterBootRecordMagic != 0xaa55)
	{
		FileSystemError("Invalid partition table magic (0xaa55)");
		return FALSE;
	}

	// Get the requested partition entry
	if (PartitionNumber == 0)
	{
		// Partition requested was zero which means the boot partition
		if (FsInternalGetActivePartitionEntry(DriveNumber, &PartitionTableEntry) == FALSE)
		{
			return FALSE;
		}
	}
	else
	{
		// Get requested partition
		if (FsInternalGetPartitionEntry(DriveNumber, PartitionNumber, &PartitionTableEntry) == FALSE)
		{
			return FALSE;
		}
	}

	// Check for valid partition
	if (PartitionTableEntry.SystemIndicator == PARTITION_ENTRY_UNUSED)
	{
		FileSystemError("Invalid partition.");
		return FALSE;
	}

	switch (PartitionTableEntry.SystemIndicator)
	{
	case PARTITION_FAT_12:
	case PARTITION_FAT_16:
	case PARTITION_HUGE:
	case PARTITION_XINT13:
	case PARTITION_FAT32:
	case PARTITION_FAT32_XINT13:
		FileSystemType = FS_FAT;
		return FatOpenVolume(DriveNumber, PartitionTableEntry.StartSector);
	default:
		FileSystemType = 0;
		FileSystemError("Unsupported file system.");
		return FALSE;
	}

	return TRUE;
}

BOOL FsInternalIsDiskPartitioned(ULONG DriveNumber)
{
	// Hard disks use drive numbers >= 0x80
	// So if the drive number indicates a hard disk
	// then return TRUE
	if (DriveNumber >= 0x80)
	{
		return TRUE;
	}

	// Drive is a floppy diskette so return FALSE
	return FALSE;
}

BOOL FsInternalGetActivePartitionEntry(ULONG DriveNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
	ULONG				BootablePartitionCount = 0;
	MASTER_BOOT_RECORD	MasterBootRecord;

	// Read master boot record
	if (!BiosInt13Read(DriveNumber, 0, 0, 1, 1, &MasterBootRecord))
	{
		FileSystemError("Disk read error.");
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
		FileSystemError("Too many bootable partitions or none found.");
		return FALSE;
	}

	// Copy the partition table entry
	RtlCopyMemory(PartitionTableEntry, &MasterBootRecord.PartitionTable[BootPartition], sizeof(PARTITION_TABLE_ENTRY));

	return TRUE;
}

BOOL FsInternalGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry)
{
	MASTER_BOOT_RECORD	MasterBootRecord;

	// Read master boot record
	if (!BiosInt13Read(DriveNumber, 0, 0, 1, 1, &MasterBootRecord))
	{
		FileSystemError("Disk read error.");
		return FALSE;
	}

	// PartitionNumber is one-based and we need it zero-based
	PartitionNumber--;

	// Copy the partition table entry
	RtlCopyMemory(PartitionTableEntry, &MasterBootRecord.PartitionTable[PartitionNumber], sizeof(PARTITION_TABLE_ENTRY));

	return TRUE;
}

PFILE OpenFile(PUCHAR FileName)
{
	PFILE	FileHandle = NULL;

	//
	// Print status message
	//
	DbgPrint((DPRINT_FILESYSTEM, "Opening file '%s'...\n", FileName));
	
	//
	// Check file system type and pass off to appropriate handler
	//
	if (FileSystemType == FS_FAT)
	{
		FileHandle = FatOpenFile(FileName);
	}
	else
	{
		FileSystemError("Error: Unknown filesystem.");
	}

#ifdef DEBUG
	//
	// Check return value
	//
	if (FileHandle != NULL)
	{
		DbgPrint((DPRINT_FILESYSTEM, "OpenFile() succeeded. FileHandle: 0x%x\n", FileHandle));
	}
	else
	{
		DbgPrint((DPRINT_FILESYSTEM, "OpenFile() failed.\n"));
	}
#endif // defined DEBUG

	return FileHandle;
}

VOID CloseFile(PFILE FileHandle)
{
}

/*
 * ReadFile()
 * returns number of bytes read or EOF
 */
BOOL ReadFile(PFILE FileHandle, ULONG BytesToRead, PULONG BytesRead, PVOID Buffer)
{
	//
	// Set the number of bytes read equal to zero
	//
	if (BytesRead !=NULL)
	{
		*BytesRead = 0;
	}

	switch (FileSystemType)
	{
	case FS_FAT:

		return FatReadFile(FileHandle, BytesToRead, BytesRead, Buffer);

	default:

		FileSystemError("Unknown file system.");
		return FALSE;
	}

	return FALSE;
}

ULONG GetFileSize(PFILE FileHandle)
{
	switch (FileSystemType)
	{
	case FS_FAT:

		return FatGetFileSize(FileHandle);

	default:
		FileSystemError("Unknown file system.");
		break;
	}

	return 0;
}

VOID SetFilePointer(PFILE FileHandle, ULONG NewFilePointer)
{
	switch (FileSystemType)
	{
	case FS_FAT:

		FatSetFilePointer(FileHandle, NewFilePointer);
		break;

	default:
		FileSystemError("Unknown file system.");
		break;
	}
}

ULONG GetFilePointer(PFILE FileHandle)
{
	switch (FileSystemType)
	{
	case FS_FAT:

		return FatGetFilePointer(FileHandle);
		break;

	default:
		FileSystemError("Unknown file system.");
		break;
	}

	return 0;
}

BOOL IsEndOfFile(PFILE FileHandle)
{
	if (GetFilePointer(FileHandle) >= GetFileSize(FileHandle))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
