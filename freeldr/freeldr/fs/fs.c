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
#include "fat.h"
#include "iso.h"
#include <disk.h>
#include <rtl.h>
#include <ui.h>
#include <arch.h>
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

	UiMessageBox(ErrorString);
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
	PARTITION_TABLE_ENTRY	PartitionTableEntry;

	DbgPrint((DPRINT_FILESYSTEM, "OpenDiskDrive() DriveNumber: 0x%x PartitionNumber: 0x%x\n", DriveNumber, PartitionNumber));

	// Check and see if it is a floppy drive
	// If so then just assume FAT12 file system type
	if (DiskIsDriveRemovable(DriveNumber))
	{
		DbgPrint((DPRINT_FILESYSTEM, "Drive is a floppy diskette drive. Assuming FAT12 file system.\n"));

		FileSystemType = FS_FAT;
		return FatOpenVolume(DriveNumber, 0);
	}

	// Check and see if it is a cdrom drive
	// If so then just assume ISO9660 file system type
	if (DiskIsDriveCdRom(DriveNumber))
	{
		DbgPrint((DPRINT_FILESYSTEM, "Drive is a cdrom drive. Assuming ISO-9660 file system.\n"));

		FileSystemType = FS_ISO9660;
		return IsoOpenVolume(DriveNumber);
	}

	// Set the boot partition
	BootPartition = PartitionNumber;

	// Get the requested partition entry
	if (PartitionNumber == 0)
	{
		// Partition requested was zero which means the boot partition
		if (DiskGetActivePartitionEntry(DriveNumber, &PartitionTableEntry) == FALSE)
		{
			FileSystemError("No active partition.");
			return FALSE;
		}
	}
	else
	{
		// Get requested partition
		if (DiskGetPartitionEntry(DriveNumber, PartitionNumber, &PartitionTableEntry) == FALSE)
		{
			FileSystemError("Partition not found.");
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
		return FatOpenVolume(DriveNumber, PartitionTableEntry.SectorCountBeforePartition);
	default:
		FileSystemType = 0;
		FileSystemError("Unsupported file system.");
		return FALSE;
	}

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
	else if (FileSystemType == FS_ISO9660)
	{
		FileHandle = IsoOpenFile(FileName);
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

	case FS_ISO9660:

		return IsoReadFile(FileHandle, BytesToRead, BytesRead, Buffer);

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

	case FS_ISO9660:

		return IsoGetFileSize(FileHandle);

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

	case FS_ISO9660:

		IsoSetFilePointer(FileHandle, NewFilePointer);
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

	case FS_ISO9660:

		return IsoGetFilePointer(FileHandle);
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
