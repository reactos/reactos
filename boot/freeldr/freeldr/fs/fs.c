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

const FS_VTBL* pFSVtbl = NULL; // Type of filesystem on boot device, set by FsOpenVolume()
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

	if( !FsStaticBufferDisk )
		FsStaticBufferDisk = MmAllocateMemory( 0x20000 );
	if( !FsStaticBufferDisk )
	{
		FileSystemError("could not allocate filesystem static buffer");
		return FALSE;
	}
	FsStaticBufferData = ((PCHAR)FsStaticBufferDisk) + 0x10000;

	switch (Type)
	{
	case FS_FAT:
		pFSVtbl = &FatVtbl;
		break;
	case FS_NTFS:
		pFSVtbl = &NtfsVtbl;
		break;
	case FS_EXT2:
		pFSVtbl = &Ext2Vtbl;
		break;
	case FS_ISO9660:
		pFSVtbl = &Iso9660Vtbl;
		break;
	default:
		pFSVtbl = NULL;
		break;
	}

	if (pFSVtbl && pFSVtbl->OpenVolume)
	{
		return (*pFSVtbl->OpenVolume)(DriveNumber, StartSector, SectorCount);
	}
	else
	{
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
	if (pFSVtbl && pFSVtbl->OpenFile)
	{
		FileHandle = pFSVtbl->OpenFile(FileName);
	}
	else
	{
		FileSystemError("Error: Unknown filesystem.");
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
	if (pFSVtbl)
	{
		if (pFSVtbl->CloseFile)
			(*pFSVtbl->CloseFile)(FileHandle);
	}
	else
	{
		FileSystemError("Error: Unknown filesystem.");
	}
}

/*
 * ReadFile()
 * returns number of bytes read or EOF
 */
BOOLEAN FsReadFile(PFILE FileHandle, ULONG BytesToRead, ULONG* BytesRead, PVOID Buffer)
{
	//
	// Set the number of bytes read equal to zero
	//
	if (BytesRead != NULL)
	{
		*BytesRead = 0;
	}

	if (pFSVtbl && pFSVtbl->ReadFile)
	{
		return (*pFSVtbl->ReadFile)(FileHandle, BytesToRead, BytesRead, Buffer);
	}
	else
	{
		FileSystemError("Unknown file system.");
		return FALSE;
	}
}

ULONG FsGetFileSize(PFILE FileHandle)
{
	if (pFSVtbl && pFSVtbl->GetFileSize)
	{
		return (*pFSVtbl->GetFileSize)(FileHandle);
	}
	else
	{
		FileSystemError("Unknown file system.");
		return 0;
	}
}

VOID FsSetFilePointer(PFILE FileHandle, ULONG NewFilePointer)
{
	if (pFSVtbl && pFSVtbl->SetFilePointer)
	{
		(*pFSVtbl->SetFilePointer)(FileHandle, NewFilePointer);
	}
	else
	{
		FileSystemError("Unknown file system.");
	}
}

ULONG FsGetFilePointer(PFILE FileHandle)
{
	if (pFSVtbl && pFSVtbl->SetFilePointer)
	{
		return (*pFSVtbl->GetFilePointer)(FileHandle);
	}
	else
	{
		FileSystemError("Unknown file system.");
		return 0;
	}
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
