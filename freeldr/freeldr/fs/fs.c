/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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
#include "ext2.h"
#include "fsrec.h"
#include <disk.h>
#include <rtl.h>
#include <ui.h>
#include <arch.h>
#include <debug.h>


/////////////////////////////////////////////////////////////////////////////////////////////
// DATA
/////////////////////////////////////////////////////////////////////////////////////////////

U32			FileSystemType = 0;	// Type of filesystem on boot device, set by FsOpenVolume()

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
 * BOOL FsOpenVolume(U32 DriveNumber, U32 PartitionNumber);
 *
 * This function is called to open a disk volume for file access.
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
BOOL FsOpenVolume(U32 DriveNumber, U32 PartitionNumber)
{
	PARTITION_TABLE_ENTRY	PartitionTableEntry;
	UCHAR					ErrorText[80];
	U8						VolumeType;

	DbgPrint((DPRINT_FILESYSTEM, "FsOpenVolume() DriveNumber: 0x%x PartitionNumber: 0x%x\n", DriveNumber, PartitionNumber));

	// Check and see if it is a floppy drive
	// If so then just assume FAT12 file system type
	if (DiskIsDriveRemovable(DriveNumber))
	{
		DbgPrint((DPRINT_FILESYSTEM, "Drive is a floppy diskette drive. Assuming FAT12 file system.\n"));

		FileSystemType = FS_FAT;
		return FatOpenVolume(DriveNumber, 0);
	}

	// Check for ISO9660 file system type
	if (DriveNumber > 0x80 && FsRecIsIso9660(DriveNumber))
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

	// Try to recognize the file system
	if (!FsRecognizeVolume(DriveNumber, PartitionTableEntry.SectorCountBeforePartition, &VolumeType))
	{
		sprintf(ErrorText, "Unrecognized file system. Type: 0x%x", PartitionTableEntry.SystemIndicator);
		FileSystemError(ErrorText);
	}

	//switch (PartitionTableEntry.SystemIndicator)
	switch (VolumeType)
	{
	case PARTITION_FAT_12:
	case PARTITION_FAT_16:
	case PARTITION_HUGE:
	case PARTITION_XINT13:
	case PARTITION_FAT32:
	case PARTITION_FAT32_XINT13:
		FileSystemType = FS_FAT;
		return FatOpenVolume(DriveNumber, PartitionTableEntry.SectorCountBeforePartition);
	case PARTITION_EXT2:
		FileSystemType = FS_EXT2;
		return Ext2OpenVolume(DriveNumber, PartitionTableEntry.SectorCountBeforePartition);
	default:
		FileSystemType = 0;
		sprintf(ErrorText, "Unsupported file system. Type: 0x%x", PartitionTableEntry.SystemIndicator);
		FileSystemError(ErrorText);
		return FALSE;
	}

	return TRUE;
}

PFILE FsOpenFile(PUCHAR FileName)
{
	PFILE	FileHandle = NULL;

	//
	// Print status message
	//
	DbgPrint((DPRINT_FILESYSTEM, "Opening file '%s'...\n", FileName));

	//
	// Check and see if the first character is '\' or '/' and remove it if so
	//
	while ((*FileName == '\\') || (*FileName == '/'))
	{
		FileName++;
	}
	
	//
	// Check file system type and pass off to appropriate handler
	//
	switch (FileSystemType)
	{
	case FS_FAT:
		FileHandle = FatOpenFile(FileName);
		break;
	case FS_ISO9660:
		FileHandle = IsoOpenFile(FileName);
		break;
	case FS_EXT2:
		FileHandle = Ext2OpenFile(FileName);
		break;
	default:
		FileSystemError("Error: Unknown filesystem.");
		break;
	}

#ifdef DEBUG
	//
	// Check return value
	//
	if (FileHandle != NULL)
	{
		DbgPrint((DPRINT_FILESYSTEM, "FsOpenFile() succeeded. FileHandle: 0x%x\n", FileHandle));
	}
	else
	{
		DbgPrint((DPRINT_FILESYSTEM, "FsOpenFile() failed.\n"));
	}
#endif // defined DEBUG

	return FileHandle;
}

VOID FsCloseFile(PFILE FileHandle)
{
}

/*
 * ReadFile()
 * returns number of bytes read or EOF
 */
BOOL FsReadFile(PFILE FileHandle, U32 BytesToRead, U32* BytesRead, PVOID Buffer)
{
	U64		BytesReadBig;
	BOOL	Success;

	//
	// Set the number of bytes read equal to zero
	//
	if (BytesRead != NULL)
	{
		*BytesRead = 0;
	}

	switch (FileSystemType)
	{
	case FS_FAT:

		return FatReadFile(FileHandle, BytesToRead, BytesRead, Buffer);

	case FS_ISO9660:

		return IsoReadFile(FileHandle, BytesToRead, BytesRead, Buffer);

	case FS_EXT2:

		//return Ext2ReadFile(FileHandle, BytesToRead, BytesRead, Buffer);
		Success = Ext2ReadFile(FileHandle, BytesToRead, &BytesReadBig, Buffer);
		*BytesRead = (U32)BytesReadBig;
		return Success;

	default:

		FileSystemError("Unknown file system.");
		return FALSE;
	}

	return FALSE;
}

U32 FsGetFileSize(PFILE FileHandle)
{
	switch (FileSystemType)
	{
	case FS_FAT:

		return FatGetFileSize(FileHandle);

	case FS_ISO9660:

		return IsoGetFileSize(FileHandle);

	case FS_EXT2:

		return Ext2GetFileSize(FileHandle);

	default:
		FileSystemError("Unknown file system.");
		break;
	}

	return 0;
}

VOID FsSetFilePointer(PFILE FileHandle, U32 NewFilePointer)
{
	switch (FileSystemType)
	{
	case FS_FAT:

		FatSetFilePointer(FileHandle, NewFilePointer);
		break;

	case FS_ISO9660:

		IsoSetFilePointer(FileHandle, NewFilePointer);
		break;

	case FS_EXT2:

		Ext2SetFilePointer(FileHandle, NewFilePointer);
		break;

	default:
		FileSystemError("Unknown file system.");
		break;
	}
}

U32 FsGetFilePointer(PFILE FileHandle)
{
	switch (FileSystemType)
	{
	case FS_FAT:

		return FatGetFilePointer(FileHandle);
		break;

	case FS_ISO9660:

		return IsoGetFilePointer(FileHandle);
		break;

	case FS_EXT2:

		return Ext2GetFilePointer(FileHandle);
		break;

	default:
		FileSystemError("Unknown file system.");
		break;
	}

	return 0;
}

BOOL FsIsEndOfFile(PFILE FileHandle)
{
	if (FsGetFilePointer(FileHandle) >= FsGetFileSize(FileHandle))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

/*
 * FsGetNumPathParts()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and returns the number of parts it has (i.e. 3 - dir1,dir2,file1.ext)
 */
U32 FsGetNumPathParts(PUCHAR Path)
{
	U32		i;
	U32		num;

	for (i=0,num=0; i<(int)strlen(Path); i++)
	{
		if ((Path[i] == '\\') || (Path[i] == '/'))
		{
			num++;
		}
	}
	num++;

	DbgPrint((DPRINT_FILESYSTEM, "FatGetNumPathParts() Path = %s NumPathParts = %d\n", Path, num));

	return num;
}

/*
 * FsGetFirstNameFromPath()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and puts the first name of the path (e.g. "dir1") in buffer
 * compatible with the MSDOS directory structure
 */
VOID FsGetFirstNameFromPath(PUCHAR Buffer, PUCHAR Path)
{
	U32		i;

	// Copy all the characters up to the end of the
	// string or until we hit a '\' character
	// and put them in Buffer
	for (i=0; i<(int)strlen(Path); i++)
	{
		if ((Path[i] == '\\') || (Path[i] == '/'))
		{
			break;
		}
		else
		{
			Buffer[i] = Path[i];
		}
	}

	Buffer[i] = 0;

	DbgPrint((DPRINT_FILESYSTEM, "FatGetFirstNameFromPath() Path = %s FirstName = %s\n", Path, Buffer));
}
