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
#include "iso.h"
#include <disk.h>
#include <rtl.h>
#include <ui.h>
#include <arch.h>
#include <mm.h>
#include <debug.h>
#include <cache.h>


#define SECTORSIZE 2048

static U32		IsoRootSector;		// Starting sector of the root directory
static U32		IsoRootLength;		// Length of the root directory

U32			IsoDriveNumber = 0;


BOOL IsoOpenVolume(U32 DriveNumber)
{
	PPVD Pvd;

	DbgPrint((DPRINT_FILESYSTEM, "IsoOpenVolume() DriveNumber = 0x%x VolumeStartSector = 16\n", DriveNumber));

	// Store the drive number
	IsoDriveNumber = DriveNumber;

	IsoRootSector = 0;
	IsoRootLength = 0;

	Pvd = MmAllocateMemory(SECTORSIZE);

	if (!DiskReadLogicalSectors(DriveNumber, 16, 1, Pvd))
	{
		FileSystemError("Failed to read the PVD.");
		MmFreeMemory(Pvd);
		return FALSE;
	}

	IsoRootSector = Pvd->RootDirRecord.ExtentLocationL;
	IsoRootLength = Pvd->RootDirRecord.DataLengthL;

	MmFreeMemory(Pvd);

	DbgPrint((DPRINT_FILESYSTEM, "IsoRootSector = %u  IsoRootLegth = %u\n", IsoRootSector, IsoRootLength));

	return TRUE;
}


static BOOL IsoSearchDirectoryBufferForFile(PVOID DirectoryBuffer, U32 DirectoryLength, PUCHAR FileName, PISO_FILE_INFO IsoFileInfoPointer)
{
	PDIR_RECORD	Record;
	U32			Offset;
	U32 i;
	UCHAR Name[32];

	DbgPrint((DPRINT_FILESYSTEM, "IsoSearchDirectoryBufferForFile() DirectoryBuffer = 0x%x DirectoryLength = %d FileName = %s\n", DirectoryBuffer, DirectoryLength, FileName));

	memset(Name, 0, 32 * sizeof(UCHAR));

	Offset = 0;
	Record = (PDIR_RECORD)DirectoryBuffer;
	while (TRUE)
	{
		Offset = Offset + Record->RecordLength;
		Record = (PDIR_RECORD)(DirectoryBuffer + Offset);

		if (Record->RecordLength == 0)
		{
			Offset = ROUND_UP(Offset, SECTORSIZE);
			Record = (PDIR_RECORD)(DirectoryBuffer + Offset);
		}

		if (Record->FileIdLength == 1 && Record->FileId[0] == 0)
		{
			DbgPrint((DPRINT_FILESYSTEM, "Name '.'\n"));
		}
		else if (Record->FileIdLength == 1 && Record->FileId[0] == 1)
		{
			DbgPrint((DPRINT_FILESYSTEM, "Name '..'\n"));
		}
		else
		{
			for (i = 0; i < Record->FileIdLength && Record->FileId[i] != ';'; i++)
				Name[i] = Record->FileId[i];
			Name[i] = 0;
			DbgPrint((DPRINT_FILESYSTEM, "Name '%s'\n", Name));

			if (strlen(FileName) == strlen(Name) && stricmp(FileName, Name) == 0)
			{
				IsoFileInfoPointer->FileStart = Record->ExtentLocationL;
				IsoFileInfoPointer->FileSize = Record->DataLengthL;
				IsoFileInfoPointer->FilePointer = 0;
				IsoFileInfoPointer->Directory = (Record->FileFlags & 0x02)?TRUE:FALSE;

				return TRUE;
			}

		}

		if (Offset >= DirectoryLength)
			return FALSE;

		memset(Name, 0, 32 * sizeof(UCHAR));
	}

	return FALSE;
}



static PVOID IsoBufferDirectory(U32 DirectoryStartSector, U32 DirectoryLength)
{
	PVOID	DirectoryBuffer;
	U32	SectorCount;

	DbgPrint((DPRINT_FILESYSTEM, "IsoBufferDirectory() DirectoryStartSector = %d DirectoryLength = %d\n", DirectoryStartSector, DirectoryLength));

	//
	// Attempt to allocate memory for directory buffer
	//
	DbgPrint((DPRINT_FILESYSTEM, "Trying to allocate (DirectoryLength) %d bytes.\n", DirectoryLength));
	DirectoryBuffer = MmAllocateMemory(DirectoryLength);

	if (DirectoryBuffer == NULL)
	{
		return NULL;
	}

	SectorCount = ROUND_UP(DirectoryLength, SECTORSIZE) / SECTORSIZE;
	DbgPrint((DPRINT_FILESYSTEM, "Trying to read (DirectoryCount) %d sectors.\n", SectorCount));

	//
	// Now read directory contents into DirectoryBuffer
	//
	if (!DiskReadLogicalSectors(IsoDriveNumber, DirectoryStartSector, SectorCount, DirectoryBuffer))
	{
		MmFreeMemory(DirectoryBuffer);
		return NULL;
	}

	return DirectoryBuffer;
}


/*
 * IsoLookupFile()
 * This function searches the file system for the
 * specified filename and fills in an ISO_FILE_INFO structure
 * with info describing the file, etc. returns true
 * if the file exists or false otherwise
 */
static BOOL IsoLookupFile(PUCHAR FileName, PISO_FILE_INFO IsoFileInfoPointer)
{
	int		i;
	U32			NumberOfPathParts;
	UCHAR		PathPart[261];
	PVOID		DirectoryBuffer;
	U32		DirectorySector;
	U32		DirectoryLength;
	ISO_FILE_INFO	IsoFileInfo;

	DbgPrint((DPRINT_FILESYSTEM, "IsoLookupFile() FileName = %s\n", FileName));

	memset(IsoFileInfoPointer, 0, sizeof(ISO_FILE_INFO));

	//
	// Figure out how many sub-directories we are nested in
	//
	NumberOfPathParts = FsGetNumPathParts(FileName);

	DirectorySector = IsoRootSector;
	DirectoryLength = IsoRootLength;

	//
	// Loop once for each part
	//
	for (i=0; i<NumberOfPathParts; i++)
	{
		//
		// Get first path part
		//
		FsGetFirstNameFromPath(PathPart, FileName);

		//
		// Advance to the next part of the path
		//
		for (; (*FileName != '\\') && (*FileName != '/') && (*FileName != '\0'); FileName++)
		{
		}
		FileName++;

		//
		// Buffer the directory contents
		//
		DirectoryBuffer = IsoBufferDirectory(DirectorySector, DirectoryLength);
		if (DirectoryBuffer == NULL)
		{
			return FALSE;
		}

		//
		// Search for file name in directory
		//
		if (!IsoSearchDirectoryBufferForFile(DirectoryBuffer, DirectoryLength, PathPart, &IsoFileInfo))
		{
			MmFreeMemory(DirectoryBuffer);
			return FALSE;
		}

		MmFreeMemory(DirectoryBuffer);

		//
		// If we have another sub-directory to go then
		// grab the start sector and file size
		//
		if ((i+1) < NumberOfPathParts)
		{
			DirectorySector = IsoFileInfo.FileStart;
			DirectoryLength = IsoFileInfo.FileSize;
		}

	}

	memcpy(IsoFileInfoPointer, &IsoFileInfo, sizeof(ISO_FILE_INFO));

	return TRUE;
}


/*
 * IsoOpenFile()
 * Tries to open the file 'name' and returns true or false
 * for success and failure respectively
 */
FILE* IsoOpenFile(PUCHAR FileName)
{
	ISO_FILE_INFO		TempFileInfo;
	PISO_FILE_INFO		FileHandle;

	DbgPrint((DPRINT_FILESYSTEM, "IsoOpenFile() FileName = %s\n", FileName));

	if (!IsoLookupFile(FileName, &TempFileInfo))
	{
		return NULL;
	}

	FileHandle = MmAllocateMemory(sizeof(ISO_FILE_INFO));

	if (FileHandle == NULL)
	{
		return NULL;
	}

	memcpy(FileHandle, &TempFileInfo, sizeof(ISO_FILE_INFO));

	return (FILE*)FileHandle;
}


/*
 * IsoReadPartialSector()
 * Reads part of a cluster into memory
 */
static BOOL IsoReadPartialSector(U32 SectorNumber, U32 StartingOffset, U32 Length, PVOID Buffer)
{
	PUCHAR	SectorBuffer;

	DbgPrint((DPRINT_FILESYSTEM, "IsoReadPartialSector() SectorNumber = %d StartingOffset = %d Length = %d Buffer = 0x%x\n", SectorNumber, StartingOffset, Length, Buffer));

	SectorBuffer = MmAllocateMemory(SECTORSIZE);
	if (SectorBuffer == NULL)
	{
		return FALSE;
	}

	if (!DiskReadLogicalSectors(IsoDriveNumber, SectorNumber, 1, SectorBuffer))
	{
		MmFreeMemory(SectorBuffer);
		return FALSE;
	}

	memcpy(Buffer, ((PVOID)SectorBuffer + StartingOffset), Length);

	MmFreeMemory(SectorBuffer);

	return TRUE;
}


/*
 * IsoReadFile()
 * Reads BytesToRead from open file and
 * returns the number of bytes read in BytesRead
 */
BOOL IsoReadFile(FILE *FileHandle, U32 BytesToRead, U32* BytesRead, PVOID Buffer)
{
	PISO_FILE_INFO	IsoFileInfo = (PISO_FILE_INFO)FileHandle;
	U32		SectorNumber;
	U32		OffsetInSector;
	U32		LengthInSector;
	U32		NumberOfSectors;

	DbgPrint((DPRINT_FILESYSTEM, "IsoReadFile() BytesToRead = %d Buffer = 0x%x\n", BytesToRead, Buffer));

	if (BytesRead != NULL)
	{
		*BytesRead = 0;
	}

	//
	// If they are trying to read past the
	// end of the file then return success
	// with BytesRead == 0
	//
	if (IsoFileInfo->FilePointer >= IsoFileInfo->FileSize)
	{
		return TRUE;
	}

	//
	// If they are trying to read more than there is to read
	// then adjust the amount to read
	//
	if ((IsoFileInfo->FilePointer + BytesToRead) > IsoFileInfo->FileSize)
	{
		BytesToRead = (IsoFileInfo->FileSize - IsoFileInfo->FilePointer);
	}

	//
	// Ok, now we have to perform at most 3 calculations
	// I'll draw you a picture (using nifty ASCII art):
	//
	// CurrentFilePointer -+
	//                     |
	//    +----------------+
	//    |
	// +-----------+-----------+-----------+-----------+
	// | Sector  1 | Sector  2 | Sector  3 | Sector  4 |
	// +-----------+-----------+-----------+-----------+
	//    |                                    |
	//    +---------------+--------------------+
	//                    |
	// BytesToRead -------+
	//
	// 1 - The first calculation (and read) will align
	//     the file pointer with the next sector
	//     boundary (if we are supposed to read that much)
	// 2 - The next calculation (and read) will read
	//     in all the full sectors that the requested
	//     amount of data would cover (in this case
	//     sectors 2 & 3).
	// 3 - The last calculation (and read) would read
	//     in the remainder of the data requested out of
	//     the last sector.
	//


	//
	// Only do the first read if we
	// aren't aligned on a cluster boundary
	//
	if (IsoFileInfo->FilePointer % SECTORSIZE)
	{
		//
		// Do the math for our first read
		//
		SectorNumber = IsoFileInfo->FileStart + (IsoFileInfo->FilePointer / SECTORSIZE);
		OffsetInSector = IsoFileInfo->FilePointer % SECTORSIZE;
		LengthInSector = (BytesToRead > (SECTORSIZE - OffsetInSector)) ? (SECTORSIZE - OffsetInSector) : BytesToRead;

		//
		// Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
		//
		if (!IsoReadPartialSector(SectorNumber, OffsetInSector, LengthInSector, Buffer))
		{
			return FALSE;
		}
		if (BytesRead != NULL)
		{
			*BytesRead += LengthInSector;
		}
		BytesToRead -= LengthInSector;
		IsoFileInfo->FilePointer += LengthInSector;
		Buffer += LengthInSector;
	}

	//
	// Do the math for our second read (if any data left)
	//
	if (BytesToRead > 0)
	{
		//
		// Determine how many full clusters we need to read
		//
		NumberOfSectors = (BytesToRead / SECTORSIZE);

		if (NumberOfSectors > 0)
		{
			SectorNumber = IsoFileInfo->FileStart + (IsoFileInfo->FilePointer / SECTORSIZE);

			//
			// Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
			//
			if (!DiskReadLogicalSectors(IsoDriveNumber, SectorNumber, NumberOfSectors, Buffer))
			{
				return FALSE;
			}
			if (BytesRead != NULL)
			{
				*BytesRead += (NumberOfSectors * SECTORSIZE);
			}
			BytesToRead -= (NumberOfSectors * SECTORSIZE);
			IsoFileInfo->FilePointer += (NumberOfSectors * SECTORSIZE);
			Buffer += (NumberOfSectors * SECTORSIZE);
		}
	}

	//
	// Do the math for our third read (if any data left)
	//
	if (BytesToRead > 0)
	{
		SectorNumber = IsoFileInfo->FileStart + (IsoFileInfo->FilePointer / SECTORSIZE);

		//
		// Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
		//
		if (!IsoReadPartialSector(SectorNumber, 0, BytesToRead, Buffer))
		{
			return FALSE;
		}
		if (BytesRead != NULL)
		{
			*BytesRead += BytesToRead;
		}
		IsoFileInfo->FilePointer += BytesToRead;
		BytesToRead -= BytesToRead;
		Buffer += BytesToRead;
	}

	printf("IsoReadFile() done\n");

	return TRUE;
}


U32 IsoGetFileSize(FILE *FileHandle)
{
	PISO_FILE_INFO	IsoFileHandle = (PISO_FILE_INFO)FileHandle;

	DbgPrint((DPRINT_FILESYSTEM, "IsoGetFileSize() FileSize = %d\n", IsoFileHandle->FileSize));

	return IsoFileHandle->FileSize;
}

VOID IsoSetFilePointer(FILE *FileHandle, U32 NewFilePointer)
{
	PISO_FILE_INFO	IsoFileHandle = (PISO_FILE_INFO)FileHandle;

	DbgPrint((DPRINT_FILESYSTEM, "IsoSetFilePointer() NewFilePointer = %d\n", NewFilePointer));

	IsoFileHandle->FilePointer = NewFilePointer;
}

U32 IsoGetFilePointer(FILE *FileHandle)
{
	PISO_FILE_INFO	IsoFileHandle = (PISO_FILE_INFO)FileHandle;

	DbgPrint((DPRINT_FILESYSTEM, "IsoGetFilePointer() FilePointer = %d\n", IsoFileHandle->FilePointer));

	return IsoFileHandle->FilePointer;
}
