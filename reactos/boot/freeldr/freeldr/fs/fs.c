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

#define NDEBUG
#include <debug.h>

/////////////////////////////////////////////////////////////////////////////////////////////
// DATA
/////////////////////////////////////////////////////////////////////////////////////////////

ULONG FsType = 0;	// Type of filesystem on boot device, set by FsOpenVolume()
PVOID FsStaticBufferDisk = 0, FsStaticBufferData = 0;

/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

VOID FileSystemError(PCSTR ErrorString)
{
	DbgPrint((DPRINT_FILESYSTEM, "%s\n", ErrorString));

	UiMessageBox(ErrorString);
}

/*
 *
 * BOOLEAN FsOpenVolume(ULONG DriveNumber, ULONGLONG StartSector, ULONGLONG SectorCount, int Type);
 *
 * This function is called to open a disk volume for file access.
 * It must be called before any of the file functions will work.
 *
 */
static BOOLEAN FsOpenVolume(ULONG DriveNumber, ULONGLONG StartSector, ULONGLONG SectorCount, int Type)
{
	CHAR ErrorText[80];

	FsType = Type;

	if( !FsStaticBufferDisk )
		FsStaticBufferDisk = MmAllocateMemory( 0x20000 );
	if( !FsStaticBufferDisk )
	{
		FileSystemError("could not allocate filesystem static buffer");
		return FALSE;
	}
	FsStaticBufferData = ((PCHAR)FsStaticBufferDisk) + 0x10000;

	switch (FsType)
	{
	case FS_FAT:
		return FatOpenVolume(DriveNumber, StartSector, SectorCount);
	case FS_EXT2:
		return Ext2OpenVolume(DriveNumber, StartSector);
	case FS_NTFS:
		return NtfsOpenVolume(DriveNumber, StartSector);
	case FS_ISO9660:
		return IsoOpenVolume(DriveNumber);
	default:
		FsType = 0;
		sprintf(ErrorText, "Unsupported file system. Type: 0x%x", Type);
		FileSystemError(ErrorText);
	}

	return FALSE;
}
/*
 *
 * BOOLEAN FsOpenBootVolume()
 *
 * This function is called to open the boot disk volume for file access.
 * It must be called before any of the file functions will work.
 */
BOOLEAN FsOpenBootVolume()
{
	ULONG DriveNumber;
	ULONGLONG StartSector;
	ULONGLONG SectorCount;
	int Type;

	if (! MachDiskGetBootVolume(&DriveNumber, &StartSector, &SectorCount, &Type))
	{
		FileSystemError("Unable to locate boot partition\n");
		return FALSE;
	}

	return FsOpenVolume(DriveNumber, StartSector, SectorCount, Type);
}

BOOLEAN FsOpenSystemVolume(char *SystemPath, char *RemainingPath, PULONG Device)
{
	ULONG DriveNumber;
	ULONGLONG StartSector;
	ULONGLONG SectorCount;
	int Type;

	if (! MachDiskGetSystemVolume(SystemPath, RemainingPath, Device,
	                              &DriveNumber, &StartSector, &SectorCount,
	                              &Type))
	{
		FileSystemError("Unable to locate system partition\n");
		return FALSE;
	}

	return FsOpenVolume(DriveNumber, StartSector, SectorCount, Type);
}


PFILE FsOpenFile(PCSTR FileName)
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
	switch (FsType)
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
	case FS_NTFS:
		FileHandle = NtfsOpenFile(FileName);
		break;
	default:
		FileSystemError("Error: Unknown filesystem.");
		break;
	}

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

	return FileHandle;
}

VOID FsCloseFile(PFILE FileHandle)
{
	switch (FsType)
	{
	case FS_FAT:
	case FS_ISO9660:
	case FS_EXT2:
		break;
	case FS_NTFS:
		NtfsCloseFile(FileHandle);
		break;
	default:
		FileSystemError("Error: Unknown filesystem.");
		break;
	}
}

/*
 * ReadFile()
 * returns number of bytes read or EOF
 */
BOOLEAN FsReadFile(PFILE FileHandle, ULONG BytesToRead, ULONG* BytesRead, PVOID Buffer)
{
	ULONGLONG		BytesReadBig;
	BOOLEAN	Success;

	//
	// Set the number of bytes read equal to zero
	//
	if (BytesRead != NULL)
	{
		*BytesRead = 0;
	}

	switch (FsType)
	{
	case FS_FAT:

		return FatReadFile(FileHandle, BytesToRead, BytesRead, Buffer);

	case FS_ISO9660:

		return IsoReadFile(FileHandle, BytesToRead, BytesRead, Buffer);

	case FS_EXT2:

		//return Ext2ReadFile(FileHandle, BytesToRead, BytesRead, Buffer);
		Success = Ext2ReadFile(FileHandle, BytesToRead, &BytesReadBig, Buffer);
		*BytesRead = (ULONG)BytesReadBig;
		return Success;

	case FS_NTFS:

		return NtfsReadFile(FileHandle, BytesToRead, BytesRead, Buffer);

	default:

		FileSystemError("Unknown file system.");
		return FALSE;
	}

	return FALSE;
}

ULONG FsGetFileSize(PFILE FileHandle)
{
	switch (FsType)
	{
	case FS_FAT:

		return FatGetFileSize(FileHandle);

	case FS_ISO9660:

		return IsoGetFileSize(FileHandle);

	case FS_EXT2:

		return Ext2GetFileSize(FileHandle);

	case FS_NTFS:

		return NtfsGetFileSize(FileHandle);

	default:
		FileSystemError("Unknown file system.");
		break;
	}

	return 0;
}

VOID FsSetFilePointer(PFILE FileHandle, ULONG NewFilePointer)
{
	switch (FsType)
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

	case FS_NTFS:

		NtfsSetFilePointer(FileHandle, NewFilePointer);
		break;

	default:
		FileSystemError("Unknown file system.");
		break;
	}
}

ULONG FsGetFilePointer(PFILE FileHandle)
{
	switch (FsType)
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

	case FS_NTFS:

		return NtfsGetFilePointer(FileHandle);
		break;

	default:
		FileSystemError("Unknown file system.");
		break;
	}

	return 0;
}

BOOLEAN FsIsEndOfFile(PFILE FileHandle)
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
ULONG FsGetNumPathParts(PCSTR Path)
{
	size_t		i;
	ULONG		num;

	for (i=0,num=0; i<strlen(Path); i++)
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
VOID FsGetFirstNameFromPath(PCHAR Buffer, PCSTR Path)
{
	size_t		i;

	// Copy all the characters up to the end of the
	// string or until we hit a '\' character
	// and put them in Buffer
	for (i=0; i<strlen(Path); i++)
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
