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
#include <disk.h>
#include <rtl.h>
#include <ui.h>
#include <arch.h>
#include <mm.h>
#include <debug.h>
#include <cache.h>


PFAT_BOOTSECTOR		FatVolumeBootSector = NULL;
PFAT32_BOOTSECTOR	Fat32VolumeBootSector = NULL;

ULONG				RootDirSectorStart;		// Starting sector of the root directory (fat12/16)
ULONG				DataSectorStart;		// Starting sector of the data area
ULONG				SectorsPerFat;			// Sectors per FAT table
ULONG				RootDirSectors;			// Number of sectors of the root directory (fat32)

ULONG				FatType = 0;			// FAT12, FAT16, or FAT32
ULONG				FatDriveNumber = 0;

BOOL FatOpenVolume(ULONG DriveNumber, ULONG VolumeStartSector)
{

	DbgPrint((DPRINT_FILESYSTEM, "FatOpenVolume() DriveNumber = 0x%x VolumeStartSector = %d\n", DriveNumber, VolumeStartSector));

	// Store the drive number
	FatDriveNumber = DriveNumber;

	//
	// Free any memory previously allocated
	//
	if (FatVolumeBootSector != NULL)
	{
		FreeMemory(FatVolumeBootSector);

		FatVolumeBootSector = NULL;
		Fat32VolumeBootSector = NULL;
	}

	//
	// Now allocate the memory to hold the boot sector
	//
	FatVolumeBootSector = (PFAT_BOOTSECTOR) AllocateMemory(512);
	Fat32VolumeBootSector = (PFAT32_BOOTSECTOR) FatVolumeBootSector;

	//
	// Make sure we got the memory
	//
	if (FatVolumeBootSector == NULL)
	{
		FileSystemError("Out of memory.");
		return FALSE;
	}

	// Now try to read the boot sector
	// If this fails then abort
	if (!DiskReadLogicalSectors(DriveNumber, VolumeStartSector, 1, FatVolumeBootSector))
	{
		return FALSE;
	}

	// Get the FAT type
	FatType = FatDetermineFatType(FatVolumeBootSector);

#ifdef DEBUG

	DbgPrint((DPRINT_FILESYSTEM, "Dumping boot sector:\n"));

	if (FatType == FAT32)
	{
		DbgPrint((DPRINT_FILESYSTEM, "sizeof(FAT32_BOOTSECTOR) = 0x%x.\n", sizeof(FAT32_BOOTSECTOR)));

		DbgPrint((DPRINT_FILESYSTEM, "JumpBoot: 0x%x 0x%x 0x%x\n", Fat32VolumeBootSector->JumpBoot[0], Fat32VolumeBootSector->JumpBoot[1], Fat32VolumeBootSector->JumpBoot[2]));
		DbgPrint((DPRINT_FILESYSTEM, "OemName: %c%c%c%c%c%c%c%c\n", Fat32VolumeBootSector->OemName[0], Fat32VolumeBootSector->OemName[1], Fat32VolumeBootSector->OemName[2], Fat32VolumeBootSector->OemName[3], Fat32VolumeBootSector->OemName[4], Fat32VolumeBootSector->OemName[5], Fat32VolumeBootSector->OemName[6], Fat32VolumeBootSector->OemName[7]));
		DbgPrint((DPRINT_FILESYSTEM, "BytesPerSector: %d\n", Fat32VolumeBootSector->BytesPerSector));
		DbgPrint((DPRINT_FILESYSTEM, "SectorsPerCluster: %d\n", Fat32VolumeBootSector->SectorsPerCluster));
		DbgPrint((DPRINT_FILESYSTEM, "ReservedSectors: %d\n", Fat32VolumeBootSector->ReservedSectors));
		DbgPrint((DPRINT_FILESYSTEM, "NumberOfFats: %d\n", Fat32VolumeBootSector->NumberOfFats));
		DbgPrint((DPRINT_FILESYSTEM, "RootDirEntries: %d\n", Fat32VolumeBootSector->RootDirEntries));
		DbgPrint((DPRINT_FILESYSTEM, "TotalSectors: %d\n", Fat32VolumeBootSector->TotalSectors));
		DbgPrint((DPRINT_FILESYSTEM, "MediaDescriptor: 0x%x\n", Fat32VolumeBootSector->MediaDescriptor));
		DbgPrint((DPRINT_FILESYSTEM, "SectorsPerFat: %d\n", Fat32VolumeBootSector->SectorsPerFat));
		DbgPrint((DPRINT_FILESYSTEM, "SectorsPerTrack: %d\n", Fat32VolumeBootSector->SectorsPerTrack));
		DbgPrint((DPRINT_FILESYSTEM, "NumberOfHeads: %d\n", Fat32VolumeBootSector->NumberOfHeads));
		DbgPrint((DPRINT_FILESYSTEM, "HiddenSectors: %d\n", Fat32VolumeBootSector->HiddenSectors));
		DbgPrint((DPRINT_FILESYSTEM, "TotalSectorsBig: %d\n", Fat32VolumeBootSector->TotalSectorsBig));
		DbgPrint((DPRINT_FILESYSTEM, "SectorsPerFatBig: %d\n", Fat32VolumeBootSector->SectorsPerFatBig));
		DbgPrint((DPRINT_FILESYSTEM, "ExtendedFlags: 0x%x\n", Fat32VolumeBootSector->ExtendedFlags));
		DbgPrint((DPRINT_FILESYSTEM, "FileSystemVersion: 0x%x\n", Fat32VolumeBootSector->FileSystemVersion));
		DbgPrint((DPRINT_FILESYSTEM, "RootDirStartCluster: %d\n", Fat32VolumeBootSector->RootDirStartCluster));
		DbgPrint((DPRINT_FILESYSTEM, "FsInfo: %d\n", Fat32VolumeBootSector->FsInfo));
		DbgPrint((DPRINT_FILESYSTEM, "BackupBootSector: %d\n", Fat32VolumeBootSector->BackupBootSector));
		DbgPrint((DPRINT_FILESYSTEM, "Reserved: 0x%x\n", Fat32VolumeBootSector->Reserved));
		DbgPrint((DPRINT_FILESYSTEM, "DriveNumber: 0x%x\n", Fat32VolumeBootSector->DriveNumber));
		DbgPrint((DPRINT_FILESYSTEM, "Reserved1: 0x%x\n", Fat32VolumeBootSector->Reserved1));
		DbgPrint((DPRINT_FILESYSTEM, "BootSignature: 0x%x\n", Fat32VolumeBootSector->BootSignature));
		DbgPrint((DPRINT_FILESYSTEM, "VolumeSerialNumber: 0x%x\n", Fat32VolumeBootSector->VolumeSerialNumber));
		DbgPrint((DPRINT_FILESYSTEM, "VolumeLabel: %c%c%c%c%c%c%c%c%c%c%c\n", Fat32VolumeBootSector->VolumeLabel[0], Fat32VolumeBootSector->VolumeLabel[1], Fat32VolumeBootSector->VolumeLabel[2], Fat32VolumeBootSector->VolumeLabel[3], Fat32VolumeBootSector->VolumeLabel[4], Fat32VolumeBootSector->VolumeLabel[5], Fat32VolumeBootSector->VolumeLabel[6], Fat32VolumeBootSector->VolumeLabel[7], Fat32VolumeBootSector->VolumeLabel[8], Fat32VolumeBootSector->VolumeLabel[9], Fat32VolumeBootSector->VolumeLabel[10]));
		DbgPrint((DPRINT_FILESYSTEM, "FileSystemType: %c%c%c%c%c%c%c%c\n", Fat32VolumeBootSector->FileSystemType[0], Fat32VolumeBootSector->FileSystemType[1], Fat32VolumeBootSector->FileSystemType[2], Fat32VolumeBootSector->FileSystemType[3], Fat32VolumeBootSector->FileSystemType[4], Fat32VolumeBootSector->FileSystemType[5], Fat32VolumeBootSector->FileSystemType[6], Fat32VolumeBootSector->FileSystemType[7]));
		DbgPrint((DPRINT_FILESYSTEM, "BootSectorMagic: 0x%x\n", Fat32VolumeBootSector->BootSectorMagic));
	}
	else
	{
		DbgPrint((DPRINT_FILESYSTEM, "sizeof(FAT_BOOTSECTOR) = 0x%x.\n", sizeof(FAT_BOOTSECTOR)));

		DbgPrint((DPRINT_FILESYSTEM, "JumpBoot: 0x%x 0x%x 0x%x\n", FatVolumeBootSector->JumpBoot[0], FatVolumeBootSector->JumpBoot[1], FatVolumeBootSector->JumpBoot[2]));
		DbgPrint((DPRINT_FILESYSTEM, "OemName: %c%c%c%c%c%c%c%c\n", FatVolumeBootSector->OemName[0], FatVolumeBootSector->OemName[1], FatVolumeBootSector->OemName[2], FatVolumeBootSector->OemName[3], FatVolumeBootSector->OemName[4], FatVolumeBootSector->OemName[5], FatVolumeBootSector->OemName[6], FatVolumeBootSector->OemName[7]));
		DbgPrint((DPRINT_FILESYSTEM, "BytesPerSector: %d\n", FatVolumeBootSector->BytesPerSector));
		DbgPrint((DPRINT_FILESYSTEM, "SectorsPerCluster: %d\n", FatVolumeBootSector->SectorsPerCluster));
		DbgPrint((DPRINT_FILESYSTEM, "ReservedSectors: %d\n", FatVolumeBootSector->ReservedSectors));
		DbgPrint((DPRINT_FILESYSTEM, "NumberOfFats: %d\n", FatVolumeBootSector->NumberOfFats));
		DbgPrint((DPRINT_FILESYSTEM, "RootDirEntries: %d\n", FatVolumeBootSector->RootDirEntries));
		DbgPrint((DPRINT_FILESYSTEM, "TotalSectors: %d\n", FatVolumeBootSector->TotalSectors));
		DbgPrint((DPRINT_FILESYSTEM, "MediaDescriptor: 0x%x\n", FatVolumeBootSector->MediaDescriptor));
		DbgPrint((DPRINT_FILESYSTEM, "SectorsPerFat: %d\n", FatVolumeBootSector->SectorsPerFat));
		DbgPrint((DPRINT_FILESYSTEM, "SectorsPerTrack: %d\n", FatVolumeBootSector->SectorsPerTrack));
		DbgPrint((DPRINT_FILESYSTEM, "NumberOfHeads: %d\n", FatVolumeBootSector->NumberOfHeads));
		DbgPrint((DPRINT_FILESYSTEM, "HiddenSectors: %d\n", FatVolumeBootSector->HiddenSectors));
		DbgPrint((DPRINT_FILESYSTEM, "TotalSectorsBig: %d\n", FatVolumeBootSector->TotalSectorsBig));
		DbgPrint((DPRINT_FILESYSTEM, "DriveNumber: 0x%x\n", FatVolumeBootSector->DriveNumber));
		DbgPrint((DPRINT_FILESYSTEM, "Reserved1: 0x%x\n", FatVolumeBootSector->Reserved1));
		DbgPrint((DPRINT_FILESYSTEM, "BootSignature: 0x%x\n", FatVolumeBootSector->BootSignature));
		DbgPrint((DPRINT_FILESYSTEM, "VolumeSerialNumber: 0x%x\n", FatVolumeBootSector->VolumeSerialNumber));
		DbgPrint((DPRINT_FILESYSTEM, "VolumeLabel: %c%c%c%c%c%c%c%c%c%c%c\n", FatVolumeBootSector->VolumeLabel[0], FatVolumeBootSector->VolumeLabel[1], FatVolumeBootSector->VolumeLabel[2], FatVolumeBootSector->VolumeLabel[3], FatVolumeBootSector->VolumeLabel[4], FatVolumeBootSector->VolumeLabel[5], FatVolumeBootSector->VolumeLabel[6], FatVolumeBootSector->VolumeLabel[7], FatVolumeBootSector->VolumeLabel[8], FatVolumeBootSector->VolumeLabel[9], FatVolumeBootSector->VolumeLabel[10]));
		DbgPrint((DPRINT_FILESYSTEM, "FileSystemType: %c%c%c%c%c%c%c%c\n", FatVolumeBootSector->FileSystemType[0], FatVolumeBootSector->FileSystemType[1], FatVolumeBootSector->FileSystemType[2], FatVolumeBootSector->FileSystemType[3], FatVolumeBootSector->FileSystemType[4], FatVolumeBootSector->FileSystemType[5], FatVolumeBootSector->FileSystemType[6], FatVolumeBootSector->FileSystemType[7]));
		DbgPrint((DPRINT_FILESYSTEM, "BootSectorMagic: 0x%x\n", FatVolumeBootSector->BootSectorMagic));
	}

#endif // defined DEBUG

	//
	// Check the boot sector magic
	//
	if (FatVolumeBootSector->BootSectorMagic != 0xaa55)
	{
		FileSystemError("Invalid boot sector magic (0xaa55)");
		return FALSE;
	}

	//
	// Check the FAT cluster size
	// We do not support clusters bigger than 64k
	//
	if ((FatVolumeBootSector->SectorsPerCluster * FatVolumeBootSector->BytesPerSector) > (64 * 1024))
	{
		FileSystemError("This file system has cluster sizes bigger than 64k.\nFreeLoader does not support this.");
		return FALSE;
	}

	//
	// Clear our variables
	//
	RootDirSectorStart = 0;
	DataSectorStart = 0;
	SectorsPerFat = 0;
	RootDirSectors = 0;

	//
	// Get the sectors per FAT,
	// root directory starting sector,
	// and data sector start
	//
	if (FatType != FAT32)
	{
		SectorsPerFat = FatVolumeBootSector->SectorsPerFat;

		RootDirSectorStart = (FatVolumeBootSector->NumberOfFats * SectorsPerFat) + FatVolumeBootSector->ReservedSectors;
		RootDirSectors = ((FatVolumeBootSector->RootDirEntries * 32) + (FatVolumeBootSector->BytesPerSector - 1)) / FatVolumeBootSector->BytesPerSector;

		DataSectorStart = FatVolumeBootSector->ReservedSectors + (FatVolumeBootSector->NumberOfFats * FatVolumeBootSector->SectorsPerFat) + RootDirSectors;
	}
	else
	{
		SectorsPerFat = Fat32VolumeBootSector->SectorsPerFatBig;

		DataSectorStart = FatVolumeBootSector->ReservedSectors + (FatVolumeBootSector->NumberOfFats * Fat32VolumeBootSector->SectorsPerFatBig) + RootDirSectors;
		

		//
		// Check version
		// we only work with version 0
		//
		if (Fat32VolumeBootSector->FileSystemVersion != 0)
		{
			FileSystemError("FreeLoader is too old to work with this FAT32 filesystem.\nPlease update FreeLoader.");
			return FALSE;
		}
	}

	//
	// Initialize the disk cache for this drive
	//
	if (!CacheInitializeDrive(DriveNumber))
	{
		return FALSE;
	}

	//
	// Force the FAT sectors into the cache
	// as long as it is FAT12 or FAT16. FAT32 can
	// have a multi-megabyte FAT so we don't want that.
	//
	if (FatType != FAT32)
	{
		if (!CacheForceDiskSectorsIntoCache(DriveNumber, FatVolumeBootSector->HiddenSectors + FatVolumeBootSector->ReservedSectors, FatVolumeBootSector->SectorsPerFat))
		{
			return FALSE;
		}
	}

	return TRUE;
}

ULONG FatDetermineFatType(PFAT_BOOTSECTOR FatBootSector)
{
	ULONG				RootDirSectors;
	ULONG				DataSectorCount;
	ULONG				SectorsPerFat;
	ULONG				TotalSectors;
	ULONG				CountOfClusters;
	PFAT32_BOOTSECTOR	Fat32BootSector = (PFAT32_BOOTSECTOR)FatBootSector;

	RootDirSectors = ((FatBootSector->RootDirEntries * 32) + (FatBootSector->BytesPerSector - 1)) / FatBootSector->BytesPerSector;
	SectorsPerFat = FatBootSector->SectorsPerFat ? FatBootSector->SectorsPerFat : Fat32BootSector->SectorsPerFatBig;
	TotalSectors = FatBootSector->TotalSectors ? FatBootSector->TotalSectors : FatBootSector->TotalSectorsBig;
	DataSectorCount = TotalSectors - (FatBootSector->ReservedSectors + (FatBootSector->NumberOfFats * SectorsPerFat) + RootDirSectors);
	
//mjl 
	if (FatBootSector->SectorsPerCluster == 0)
		CountOfClusters = 0;
	else
		CountOfClusters = DataSectorCount / FatBootSector->SectorsPerCluster;

	if (CountOfClusters < 4085)
	{
		/* Volume is FAT12 */
		return FAT12;
	}
	else if (CountOfClusters < 65525)
	{
		/* Volume is FAT16 */
		return FAT16;
	}
	else
	{
		/* Volume is FAT32 */
		return FAT32;
	}
}

PVOID FatBufferDirectory(UINT32 DirectoryStartCluster, PUINT32 EntryCountPointer, BOOL RootDirectory)
{
	UINT32	RootDirectoryStartSector;
	UINT32	RootDirectorySectorCount;
	PVOID	DirectoryBuffer;
	UINT32	DirectorySize;

	DbgPrint((DPRINT_FILESYSTEM, "FatBufferDirectory() DirectoryStartCluster = %d RootDirectory = %s\n", DirectoryStartCluster, (RootDirectory ? "TRUE" : "FALSE")));

	//
	// Calculate the size of the directory
	//
	if ((RootDirectory) && (FatType != FAT32))
	{
		DbgPrint((DPRINT_FILESYSTEM, "We are here.\n"));
		/*DbgPrint((DPRINT_FILESYSTEM, "sizeof(FAT_BOOTSECTOR) = 0x%x.\n", sizeof(FAT_BOOTSECTOR)));

		DbgPrint((DPRINT_FILESYSTEM, "JumpBoot: 0x%x 0x%x 0x%x\n", FatVolumeBootSector->JumpBoot[0], FatVolumeBootSector->JumpBoot[1], FatVolumeBootSector->JumpBoot[2]));
		DbgPrint((DPRINT_FILESYSTEM, "OemName: %c%c%c%c%c%c%c%c\n", FatVolumeBootSector->OemName[0], FatVolumeBootSector->OemName[1], FatVolumeBootSector->OemName[2], FatVolumeBootSector->OemName[3], FatVolumeBootSector->OemName[4], FatVolumeBootSector->OemName[5], FatVolumeBootSector->OemName[6], FatVolumeBootSector->OemName[7]));*/
		DbgPrint((DPRINT_FILESYSTEM, "BytesPerSector: %d\n", FatVolumeBootSector->BytesPerSector));
		DbgPrint((DPRINT_FILESYSTEM, "SectorsPerCluster: %d\n", FatVolumeBootSector->SectorsPerCluster));
		DbgPrint((DPRINT_FILESYSTEM, "ReservedSectors: %d\n", FatVolumeBootSector->ReservedSectors));
		DbgPrint((DPRINT_FILESYSTEM, "NumberOfFats: %d\n", FatVolumeBootSector->NumberOfFats));
		DbgPrint((DPRINT_FILESYSTEM, "RootDirEntries: %d\n", FatVolumeBootSector->RootDirEntries));
		DbgPrint((DPRINT_FILESYSTEM, "TotalSectors: %d\n", FatVolumeBootSector->TotalSectors));
		DbgPrint((DPRINT_FILESYSTEM, "MediaDescriptor: 0x%x\n", FatVolumeBootSector->MediaDescriptor));
		DbgPrint((DPRINT_FILESYSTEM, "SectorsPerFat: %d\n", FatVolumeBootSector->SectorsPerFat));
		DbgPrint((DPRINT_FILESYSTEM, "SectorsPerTrack: %d\n", FatVolumeBootSector->SectorsPerTrack));
		DbgPrint((DPRINT_FILESYSTEM, "NumberOfHeads: %d\n", FatVolumeBootSector->NumberOfHeads));
		DbgPrint((DPRINT_FILESYSTEM, "HiddenSectors: %d\n", FatVolumeBootSector->HiddenSectors));
		DbgPrint((DPRINT_FILESYSTEM, "TotalSectorsBig: %d\n", FatVolumeBootSector->TotalSectorsBig));
		DbgPrint((DPRINT_FILESYSTEM, "DriveNumber: 0x%x\n", FatVolumeBootSector->DriveNumber));
		DbgPrint((DPRINT_FILESYSTEM, "Reserved1: 0x%x\n", FatVolumeBootSector->Reserved1));
		DbgPrint((DPRINT_FILESYSTEM, "BootSignature: 0x%x\n", FatVolumeBootSector->BootSignature));
		DbgPrint((DPRINT_FILESYSTEM, "VolumeSerialNumber: 0x%x\n", FatVolumeBootSector->VolumeSerialNumber));
		/*DbgPrint((DPRINT_FILESYSTEM, "VolumeLabel: %c%c%c%c%c%c%c%c%c%c%c\n", FatVolumeBootSector->VolumeLabel[0], FatVolumeBootSector->VolumeLabel[1], FatVolumeBootSector->VolumeLabel[2], FatVolumeBootSector->VolumeLabel[3], FatVolumeBootSector->VolumeLabel[4], FatVolumeBootSector->VolumeLabel[5], FatVolumeBootSector->VolumeLabel[6], FatVolumeBootSector->VolumeLabel[7], FatVolumeBootSector->VolumeLabel[8], FatVolumeBootSector->VolumeLabel[9], FatVolumeBootSector->VolumeLabel[10]));
		DbgPrint((DPRINT_FILESYSTEM, "FileSystemType: %c%c%c%c%c%c%c%c\n", FatVolumeBootSector->FileSystemType[0], FatVolumeBootSector->FileSystemType[1], FatVolumeBootSector->FileSystemType[2], FatVolumeBootSector->FileSystemType[3], FatVolumeBootSector->FileSystemType[4], FatVolumeBootSector->FileSystemType[5], FatVolumeBootSector->FileSystemType[6], FatVolumeBootSector->FileSystemType[7]));*/
		DbgPrint((DPRINT_FILESYSTEM, "BootSectorMagic: 0x%x\n", FatVolumeBootSector->BootSectorMagic));
		DirectorySize = ROUND_UP((FatVolumeBootSector->RootDirEntries * 32), FatVolumeBootSector->BytesPerSector);
	}
	else
	{
		DbgPrint((DPRINT_FILESYSTEM, "No we are here.\n"));
		if (RootDirectory)
		{
			DirectorySize = (FatCountClustersInChain(Fat32VolumeBootSector->RootDirStartCluster) * Fat32VolumeBootSector->SectorsPerCluster) * Fat32VolumeBootSector->BytesPerSector;
		}
		else
		{
			DirectorySize = (FatCountClustersInChain(DirectoryStartCluster) * FatVolumeBootSector->SectorsPerCluster) * FatVolumeBootSector->BytesPerSector;
		}
	}

	//
	// Attempt to allocate memory for directory buffer
	//
	DbgPrint((DPRINT_FILESYSTEM, "Trying to allocate (DirectorySize) %d bytes.\n", DirectorySize));
	DirectoryBuffer = AllocateMemory(DirectorySize);

	if (DirectoryBuffer == NULL)
	{
		return NULL;
	}

	//
	// Now read directory contents into DirectoryBuffer
	//
	if (RootDirectory)
	{
		if (FatType == FAT32)
		{
			if (!FatReadClusterChain(Fat32VolumeBootSector->RootDirStartCluster, 0xFFFFFFFF, DirectoryBuffer))
			{
				FreeMemory(DirectoryBuffer);
				return NULL;
			}
		}
		else
		{
			//
			// FAT type is not FAT32 so the root directory comes right after the fat table
			//
			RootDirectoryStartSector = FatVolumeBootSector->ReservedSectors + (FatVolumeBootSector->NumberOfFats * FatVolumeBootSector->SectorsPerFat);
			RootDirectorySectorCount = (DirectorySize / FatVolumeBootSector->BytesPerSector);

			if (!FatReadVolumeSectors(FatDriveNumber, RootDirectoryStartSector, RootDirectorySectorCount, DirectoryBuffer))
			{
				FreeMemory(DirectoryBuffer);
				return NULL;
			}
		}
	}
	else
	{
		if (!FatReadClusterChain(DirectoryStartCluster, 0xFFFFFFFF, DirectoryBuffer))
		{
			FreeMemory(DirectoryBuffer);
			return NULL;
		}
	}

	*EntryCountPointer = (DirectorySize / 32);

	return DirectoryBuffer;
}

BOOL FatSearchDirectoryBufferForFile(PVOID DirectoryBuffer, UINT32 EntryCount, PUCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer)
{
	ULONG			CurrentEntry;
	PDIRENTRY		DirEntry;
	PLFN_DIRENTRY	LfnDirEntry;
	UCHAR			LfnNameBuffer[261];
	UCHAR			ShortNameBuffer[13];
	UINT32			StartCluster;

	DbgPrint((DPRINT_FILESYSTEM, "FatSearchDirectoryBufferForFile() DirectoryBuffer = 0x%x EntryCount = %d FileName = %s\n", DirectoryBuffer, EntryCount, FileName));

	memset(ShortNameBuffer, 0, 13 * sizeof(UCHAR));
	memset(LfnNameBuffer, 0, 261 * sizeof(UCHAR));

	for (CurrentEntry=0; CurrentEntry<EntryCount; CurrentEntry++)
	{
		DirEntry = (PDIRENTRY)(DirectoryBuffer + (CurrentEntry * 32) );
		LfnDirEntry = (PLFN_DIRENTRY)DirEntry;

		//
		// Check if this is the last file in the directory
		// If DirEntry[0] == 0x00 then that means all the
		// entries after this one are unused. If this is the
		// last entry then we didn't find the file in this directory.
		//
		if (DirEntry->FileName[0] == 0x00)
		{
			return FALSE;
		}

		//
		// Check if this is a deleted entry or not
		//
		if (DirEntry->FileName[0] == 0xE5)
		{
			memset(ShortNameBuffer, 0, 13 * sizeof(UCHAR));
			memset(LfnNameBuffer, 0, 261 * sizeof(UCHAR));
			continue;
		}

		//
		// Check if this is a LFN entry
		// If so it needs special handling
		//
		if (DirEntry->Attr == ATTR_LONG_NAME)
		{
			//
			// Check to see if this is a deleted LFN entry, if so continue
			//
			if (LfnDirEntry->SequenceNumber & 0x80)
			{
				continue;
			}

			//
			// Mask off high two bits of sequence number
			// and make the sequence number zero-based
			//
			LfnDirEntry->SequenceNumber &= 0x3F;
			LfnDirEntry->SequenceNumber--;

			//
			// Get all 13 LFN entry characters
			//
			if (LfnDirEntry->Name0_4[0] != 0xFFFF)
			{
				LfnNameBuffer[0 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[0];
			}
			if (LfnDirEntry->Name0_4[1] != 0xFFFF)
			{
				LfnNameBuffer[1 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[1];
			}
			if (LfnDirEntry->Name0_4[2] != 0xFFFF)
			{
				LfnNameBuffer[2 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[2];
			}
			if (LfnDirEntry->Name0_4[3] != 0xFFFF)
			{
				LfnNameBuffer[3 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[3];
			}
			if (LfnDirEntry->Name0_4[4] != 0xFFFF)
			{
				LfnNameBuffer[4 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name0_4[4];
			}
			if (LfnDirEntry->Name5_10[0] != 0xFFFF)
			{
				LfnNameBuffer[5 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[0];
			}
			if (LfnDirEntry->Name5_10[1] != 0xFFFF)
			{
				LfnNameBuffer[6 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[1];
			}
			if (LfnDirEntry->Name5_10[2] != 0xFFFF)
			{
				LfnNameBuffer[7 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[2];
			}
			if (LfnDirEntry->Name5_10[3] != 0xFFFF)
			{
				LfnNameBuffer[8 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[3];
			}
			if (LfnDirEntry->Name5_10[4] != 0xFFFF)
			{
				LfnNameBuffer[9 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[4];
			}
			if (LfnDirEntry->Name5_10[5] != 0xFFFF)
			{
				LfnNameBuffer[10 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name5_10[5];
			}
			if (LfnDirEntry->Name11_12[0] != 0xFFFF)
			{
				LfnNameBuffer[11 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name11_12[0];
			}
			if (LfnDirEntry->Name11_12[1] != 0xFFFF)
			{
				LfnNameBuffer[12 + (LfnDirEntry->SequenceNumber * 13)] = (UCHAR)LfnDirEntry->Name11_12[1];
			}

			continue;
		}

		//
		// Check for the volume label attribute
		// and skip over this entry if found
		//
		if (DirEntry->Attr & ATTR_VOLUMENAME)
		{
			memset(ShortNameBuffer, 0, 13 * sizeof(UCHAR));
			memset(LfnNameBuffer, 0, 261 * sizeof(UCHAR));
			continue;
		}

		//
		// If we get here then we've found a short file name
		// entry and LfnNameBuffer contains the long file
		// name or zeroes. All we have to do now is see if the
		// file name matches either the short or long file name
		// and fill in the FAT_FILE_INFO structure if it does
		// or zero our buffers and continue looking.
		//

		//
		// Get short file name
		//
		FatParseShortFileName(ShortNameBuffer, DirEntry);

		DbgPrint((DPRINT_FILESYSTEM, "Entry: %d LFN = %s\n", CurrentEntry, LfnNameBuffer));
		DbgPrint((DPRINT_FILESYSTEM, "Entry: %d DOS name = %s\n", CurrentEntry, ShortNameBuffer));

		//
		// See if the file name matches either the short or long name
		//
		if (((strlen(FileName) == strlen(LfnNameBuffer)) && (stricmp(FileName, LfnNameBuffer) == 0)) ||
			((strlen(FileName) == strlen(ShortNameBuffer)) && (stricmp(FileName, ShortNameBuffer) == 0)))
		{
			//
			// We found the entry, now fill in the FAT_FILE_INFO struct
			//
			FatFileInfoPointer->FileSize = DirEntry->Size;
			FatFileInfoPointer->FilePointer = 0;

			DbgPrint((DPRINT_FILESYSTEM, "MSDOS Directory Entry:\n"));
			DbgPrint((DPRINT_FILESYSTEM, "FileName[11] = %c%c%c%c%c%c%c%c%c%c%c\n", DirEntry->FileName[0], DirEntry->FileName[1], DirEntry->FileName[2], DirEntry->FileName[3], DirEntry->FileName[4], DirEntry->FileName[5], DirEntry->FileName[6], DirEntry->FileName[7], DirEntry->FileName[8], DirEntry->FileName[9], DirEntry->FileName[10]));
			DbgPrint((DPRINT_FILESYSTEM, "Attr = 0x%x\n", DirEntry->Attr));
			DbgPrint((DPRINT_FILESYSTEM, "ReservedNT = 0x%x\n", DirEntry->ReservedNT));
			DbgPrint((DPRINT_FILESYSTEM, "TimeInTenths = %d\n", DirEntry->TimeInTenths));
			DbgPrint((DPRINT_FILESYSTEM, "CreateTime = %d\n", DirEntry->CreateTime));
			DbgPrint((DPRINT_FILESYSTEM, "CreateDate = %d\n", DirEntry->CreateDate));
			DbgPrint((DPRINT_FILESYSTEM, "LastAccessDate = %d\n", DirEntry->LastAccessDate));
			DbgPrint((DPRINT_FILESYSTEM, "ClusterHigh = 0x%x\n", DirEntry->ClusterHigh));
			DbgPrint((DPRINT_FILESYSTEM, "Time = %d\n", DirEntry->Time));
			DbgPrint((DPRINT_FILESYSTEM, "Date = %d\n", DirEntry->Date));
			DbgPrint((DPRINT_FILESYSTEM, "ClusterLow = 0x%x\n", DirEntry->ClusterLow));
			DbgPrint((DPRINT_FILESYSTEM, "Size = %d\n", DirEntry->Size));

			//
			// Get the cluster chain
			//
			StartCluster = ((UINT32)DirEntry->ClusterHigh << 16) + DirEntry->ClusterLow;
			DbgPrint((DPRINT_FILESYSTEM, "StartCluster = 0x%x\n", StartCluster));
			FatFileInfoPointer->FileFatChain = FatGetClusterChainArray(StartCluster);

			//
			// See if memory allocation failed
			//
			if (FatFileInfoPointer->FileFatChain == NULL)
			{
				return FALSE;
			}

			return TRUE;
		}

		//
		// Nope, no match - zero buffers and continue looking
		//
		memset(ShortNameBuffer, 0, 13 * sizeof(UCHAR));
		memset(LfnNameBuffer, 0, 261 * sizeof(UCHAR));
		continue;
	}

	return FALSE;
}

/*
 * FatLookupFile()
 * This function searches the file system for the
 * specified filename and fills in a FAT_STRUCT structure
 * with info describing the file, etc. returns true
 * if the file exists or false otherwise
 */
BOOL FatLookupFile(PUCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer)
{
	int				i;
	ULONG			NumberOfPathParts;
	UCHAR			PathPart[261];
	PVOID			DirectoryBuffer;
	UINT32			DirectoryStartCluster = 0;
	ULONG			DirectoryEntryCount;
	FAT_FILE_INFO	FatFileInfo;

	DbgPrint((DPRINT_FILESYSTEM, "FatLookupFile() FileName = %s\n", FileName));

	memset(FatFileInfoPointer, 0, sizeof(FAT_FILE_INFO));

	//
	// Check and see if the first character is '\' and remove it if so
	//
	while (*FileName == '\\')
	{
		FileName++;
	}

	//
	// Figure out how many sub-directories we are nested in
	//
	NumberOfPathParts = FatGetNumPathParts(FileName);

	//
	// Loop once for each part
	//
	for (i=0; i<NumberOfPathParts; i++)
	{
		//
		// Get first path part
		//
		FatGetFirstNameFromPath(PathPart, FileName);

		//
		// Advance to the next part of the path
		//
		for (; (*FileName != '\\') && (*FileName != '\0'); FileName++)
		{
		}
		FileName++;

		//
		// Buffer the directory contents
		//
		DirectoryBuffer = FatBufferDirectory(DirectoryStartCluster, &DirectoryEntryCount, (i == 0) );
		if (DirectoryBuffer == NULL)
		{
			return FALSE;
		}

		//
		// Search for file name in directory
		//
		if (!FatSearchDirectoryBufferForFile(DirectoryBuffer, DirectoryEntryCount, PathPart, &FatFileInfo))
		{
			FreeMemory(DirectoryBuffer);
			return FALSE;
		}

		FreeMemory(DirectoryBuffer);

		//
		// If we have another sub-directory to go then
		// grab the start cluster and free the fat chain array
		//
		if ((i+1) < NumberOfPathParts)
		{
			DirectoryStartCluster = FatFileInfo.FileFatChain[0];
			FreeMemory(FatFileInfo.FileFatChain);
		}
	}

	memcpy(FatFileInfoPointer, &FatFileInfo, sizeof(FAT_FILE_INFO));

	return TRUE;
}

/*
 * FatGetNumPathParts()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and returns the number of parts it has (i.e. 3 - dir1,dir2,file1.ext)
 */
ULONG FatGetNumPathParts(PUCHAR Path)
{
	ULONG	i;
	ULONG	num;

	for (i=0,num=0; i<(int)strlen(Path); i++)
	{
		if (Path[i] == '\\')
		{
			num++;
		}
	}
	num++;

	DbgPrint((DPRINT_FILESYSTEM, "FatGetNumPathParts() Path = %s NumPathParts = %d\n", Path, num));

	return num;
}

/*
 * FatGetFirstNameFromPath()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and puts the first name of the path (e.g. "dir1") in buffer
 * compatible with the MSDOS directory structure
 */
VOID FatGetFirstNameFromPath(PUCHAR Buffer, PUCHAR Path)
{
	ULONG	i;

	// Copy all the characters up to the end of the
	// string or until we hit a '\' character
	// and put them in Buffer
	for (i=0; i<(int)strlen(Path); i++)
	{
		if (Path[i] == '\\')
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

/*
 * FatParseFileName()
 * This function parses a directory entry name which
 * is in the form of "FILE   EXT" and puts it in Buffer
 * in the form of "file.ext"
 */
void FatParseShortFileName(PUCHAR Buffer, PDIRENTRY DirEntry)
{
	ULONG	Idx;

	Idx = 0;

	//
	// Fixup first character
	//
	if (DirEntry->FileName[0] == 0x05)
	{
		DirEntry->FileName[0] = 0xE5;
	}

	//
	// Get the file name
	//
	while (Idx < 8)
	{
		if (DirEntry->FileName[Idx] == ' ')
		{
			break;
		}

		Buffer[Idx] = DirEntry->FileName[Idx];
		Idx++;
	}

	//
	// Get extension
	//
	if ((DirEntry->FileName[8] != ' '))
	{
		Buffer[Idx++] = '.';
		Buffer[Idx++] = (DirEntry->FileName[8] == ' ') ? '\0' : DirEntry->FileName[8];
		Buffer[Idx++] = (DirEntry->FileName[9] == ' ') ? '\0' : DirEntry->FileName[9];
		Buffer[Idx++] = (DirEntry->FileName[10] == ' ') ? '\0' : DirEntry->FileName[10];
	}

	//
	// Null-Terminate string
	//
	Buffer[Idx + 4] = '\0';

	DbgPrint((DPRINT_FILESYSTEM, "FatParseShortFileName() ShortName = %s\n", Buffer));
}

/*
 * FatGetFatEntry()
 * returns the Fat entry for a given cluster number
 */
BOOL FatGetFatEntry(UINT32 Cluster, PUINT32 ClusterPointer)
{
	DWORD	fat = 0;
	int		FatOffset;
	int		ThisFatSecNum;
	int		ThisFatEntOffset;

	DbgPrint((DPRINT_FILESYSTEM, "FatGetFatEntry() Retrieving FAT entry for cluster %d.\n", Cluster));

	switch(FatType)
	{
	case FAT12:

		FatOffset = Cluster + (Cluster / 2);
		ThisFatSecNum = FatVolumeBootSector->ReservedSectors + (FatOffset / FatVolumeBootSector->BytesPerSector);
		ThisFatEntOffset = (FatOffset % FatVolumeBootSector->BytesPerSector);

		DbgPrint((DPRINT_FILESYSTEM, "FatOffset: %d\n", FatOffset));
		DbgPrint((DPRINT_FILESYSTEM, "ThisFatSecNum: %d\n", ThisFatSecNum));
		DbgPrint((DPRINT_FILESYSTEM, "ThisFatEntOffset: %d\n", ThisFatEntOffset));

		if (ThisFatEntOffset == (FatVolumeBootSector->BytesPerSector - 1))
		{
			if (!FatReadVolumeSectors(FatDriveNumber, ThisFatSecNum, 2, (PVOID)FILESYSBUFFER))
			{
				return FALSE;
			}
		}
		else
		{
			if (!FatReadVolumeSectors(FatDriveNumber, ThisFatSecNum, 1, (PVOID)FILESYSBUFFER))
			{
				return FALSE;
			}
		}

		fat = *((WORD *) ((PVOID)FILESYSBUFFER + ThisFatEntOffset));
		if (Cluster & 0x0001) 
			fat = fat >> 4;	/* Cluster number is ODD */
		else
			fat = fat & 0x0FFF;	/* Cluster number is EVEN */

		break;

	case FAT16:
		
		FatOffset = (Cluster * 2);
		ThisFatSecNum = FatVolumeBootSector->ReservedSectors + (FatOffset / FatVolumeBootSector->BytesPerSector);
		ThisFatEntOffset = (FatOffset % FatVolumeBootSector->BytesPerSector);

		if (!FatReadVolumeSectors(FatDriveNumber, ThisFatSecNum, 1, (PVOID)FILESYSBUFFER))
		{
			return FALSE;
		}

		fat = *((WORD *) ((PVOID)FILESYSBUFFER + ThisFatEntOffset));

		break;

	case FAT32:

		FatOffset = (Cluster * 4);
		ThisFatSecNum = (Fat32VolumeBootSector->ExtendedFlags & 0x80) ? ((Fat32VolumeBootSector->ExtendedFlags & 0x0f) * Fat32VolumeBootSector->SectorsPerFatBig) : 0; // Get the active fat sector offset
		ThisFatSecNum += FatVolumeBootSector->ReservedSectors + (FatOffset / FatVolumeBootSector->BytesPerSector);
		ThisFatEntOffset = (FatOffset % FatVolumeBootSector->BytesPerSector);

		if (!FatReadVolumeSectors(FatDriveNumber, ThisFatSecNum, 1, (PVOID)FILESYSBUFFER))
		{
			return FALSE;
		}

		// Get the fat entry
		fat = (*((DWORD *) ((PVOID)FILESYSBUFFER + ThisFatEntOffset))) & 0x0FFFFFFF;

		break;

	}

	DbgPrint((DPRINT_FILESYSTEM, "FAT entry is 0x%x.\n", fat));

	*ClusterPointer = fat;

	return TRUE;
}

/*
 * FatOpenFile()
 * Tries to open the file 'name' and returns true or false
 * for success and failure respectively
 */
FILE* FatOpenFile(PUCHAR FileName)
{
	FAT_FILE_INFO		TempFatFileInfo;
	PFAT_FILE_INFO		FileHandle;

	DbgPrint((DPRINT_FILESYSTEM, "FatOpenFile() FileName = %s\n", FileName));

	if (!FatLookupFile(FileName, &TempFatFileInfo))
	{
		return NULL;
	}

	FileHandle = AllocateMemory(sizeof(FAT_FILE_INFO));

	if (FileHandle == NULL)
	{
		return NULL;
	}

	memcpy(FileHandle, &TempFatFileInfo, sizeof(FAT_FILE_INFO));

	return (FILE*)FileHandle;
}

UINT32 FatCountClustersInChain(UINT32 StartCluster)
{
	UINT32	ClusterCount = 0;

	DbgPrint((DPRINT_FILESYSTEM, "FatCountClustersInChain() StartCluster = %d\n", StartCluster));

	while (1)
	{
		//
		// If end of chain then break out of our cluster counting loop
		//
		if (((FatType == FAT12) && (StartCluster >= 0xff8)) ||
			((FatType == FAT16) && (StartCluster >= 0xfff8)) ||
			((FatType == FAT32) && (StartCluster >= 0x0ffffff8)))
		{
			break;
		}

		//
		// Increment count
		//
		ClusterCount++;

		//
		// Get next cluster
		//
		if (!FatGetFatEntry(StartCluster, &StartCluster))
		{
			return 0;
		}
	}

	DbgPrint((DPRINT_FILESYSTEM, "FatCountClustersInChain() ClusterCount = %d\n", ClusterCount));

	return ClusterCount;
}

PUINT32 FatGetClusterChainArray(UINT32 StartCluster)
{
	UINT32	ClusterCount;
	ULONG	ArraySize;
	PUINT32	ArrayPointer;
	ULONG	Idx;

	DbgPrint((DPRINT_FILESYSTEM, "FatGetClusterChainArray() StartCluster = %d\n", StartCluster));

	ClusterCount = FatCountClustersInChain(StartCluster) + 1; // Lets get the 0x0ffffff8 on the end of the array
	ArraySize = ClusterCount * sizeof(UINT32);

	//
	// Allocate array memory
	//
	ArrayPointer = AllocateMemory(ArraySize);

	if (ArrayPointer == NULL)
	{
		return NULL;
	}

	//
	// Loop through and set array values
	//
	for (Idx=0; Idx<ClusterCount; Idx++)
	{
		//
		// Set current cluster
		//
		ArrayPointer[Idx] = StartCluster;

		//
		// Don't try to get next cluster for last cluster
		//
		if (((FatType == FAT12) && (StartCluster >= 0xff8)) ||
			((FatType == FAT16) && (StartCluster >= 0xfff8)) ||
			((FatType == FAT32) && (StartCluster >= 0x0ffffff8)))
		{
			Idx++;
			break;
		}

		//
		// Get next cluster
		//
		if (!FatGetFatEntry(StartCluster, &StartCluster))
		{
			FreeMemory(ArrayPointer);
			return NULL;
		}
	}

	return ArrayPointer;
}

/*
 * FatReadCluster()
 * Reads the specified cluster into memory
 * and returns the number of bytes read
 */
BOOL FatReadCluster(ULONG ClusterNumber, PVOID Buffer)
{
	ULONG	ClusterStartSector;

	ClusterStartSector = ((ClusterNumber - 2) * FatVolumeBootSector->SectorsPerCluster) + DataSectorStart;

	DbgPrint((DPRINT_FILESYSTEM, "FatReadCluster() ClusterNumber = %d Buffer = 0x%x ClusterStartSector = %d\n", ClusterNumber, Buffer, ClusterStartSector));

	if (!FatReadVolumeSectors(FatDriveNumber, ClusterStartSector, FatVolumeBootSector->SectorsPerCluster, (PVOID)FILESYSBUFFER))
	{
		return FALSE;
	}

	memcpy(Buffer, (PVOID)FILESYSBUFFER, FatVolumeBootSector->SectorsPerCluster * FatVolumeBootSector->BytesPerSector);

	return TRUE;
}

/*
 * FatReadClusterChain()
 * Reads the specified clusters into memory
 */
BOOL FatReadClusterChain(ULONG StartClusterNumber, ULONG NumberOfClusters, PVOID Buffer)
{
	ULONG	ClusterStartSector;

	DbgPrint((DPRINT_FILESYSTEM, "FatReadClusterChain() StartClusterNumber = %d NumberOfClusters = %d Buffer = 0x%x\n", StartClusterNumber, NumberOfClusters, Buffer));

	while (NumberOfClusters > 0)
	{

		DbgPrint((DPRINT_FILESYSTEM, "FatReadClusterChain() StartClusterNumber = %d NumberOfClusters = %d Buffer = 0x%x\n", StartClusterNumber, NumberOfClusters, Buffer));
		//
		// Calculate starting sector for cluster
		//
		ClusterStartSector = ((StartClusterNumber - 2) * FatVolumeBootSector->SectorsPerCluster) + DataSectorStart;

		//
		// Read cluster into memory
		//
		if (!FatReadVolumeSectors(FatDriveNumber, ClusterStartSector, FatVolumeBootSector->SectorsPerCluster, (PVOID)FILESYSBUFFER))
		{
			return FALSE;
		}

		memcpy(Buffer, (PVOID)FILESYSBUFFER, FatVolumeBootSector->SectorsPerCluster * FatVolumeBootSector->BytesPerSector);

		//
		// Decrement count of clusters left to read
		//
		NumberOfClusters--;

		//
		// Increment buffer address by cluster size
		//
		Buffer += (FatVolumeBootSector->SectorsPerCluster * FatVolumeBootSector->BytesPerSector);

		//
		// Get next cluster
		//
		if (!FatGetFatEntry(StartClusterNumber, &StartClusterNumber))
		{
			return FALSE;
		}

		//
		// If end of chain then break out of our cluster reading loop
		//
		if (((FatType == FAT12) && (StartClusterNumber >= 0xff8)) ||
			((FatType == FAT16) && (StartClusterNumber >= 0xfff8)) ||
			((FatType == FAT32) && (StartClusterNumber >= 0x0ffffff8)))
		{
			break;
		}
	}
	
	return TRUE;
}

/*
 * FatReadPartialCluster()
 * Reads part of a cluster into memory
 */
BOOL FatReadPartialCluster(ULONG ClusterNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer)
{
	ULONG	ClusterStartSector;

	DbgPrint((DPRINT_FILESYSTEM, "FatReadPartialCluster() ClusterNumber = %d StartingOffset = %d Length = %d Buffer = 0x%x\n", ClusterNumber, StartingOffset, Length, Buffer));

	ClusterStartSector = ((ClusterNumber - 2) * FatVolumeBootSector->SectorsPerCluster) + DataSectorStart;

	if (!FatReadVolumeSectors(FatDriveNumber, ClusterStartSector, FatVolumeBootSector->SectorsPerCluster, (PVOID)FILESYSBUFFER))
	{
		return FALSE;
	}

	memcpy(Buffer, ((PVOID)FILESYSBUFFER + StartingOffset), Length);

	return TRUE;
}

/*
 * FatReadFile()
 * Reads BytesToRead from open file and
 * returns the number of bytes read in BytesRead
 */
BOOL FatReadFile(FILE *FileHandle, ULONG BytesToRead, PULONG BytesRead, PVOID Buffer)
{
	PFAT_FILE_INFO	FatFileInfo = (PFAT_FILE_INFO)FileHandle;
	UINT32			ClusterNumber;
	UINT32			OffsetInCluster;
	UINT32			LengthInCluster;
	UINT32			NumberOfClusters;
	UINT32			BytesPerCluster;

	DbgPrint((DPRINT_FILESYSTEM, "FatReadFile() BytesToRead = %d Buffer = 0x%x\n", BytesToRead, Buffer));

	if (BytesRead != NULL)
	{
		*BytesRead = 0;
	}

	//
	// If they are trying to read past the
	// end of the file then return success
	// with BytesRead == 0
	//
	if (FatFileInfo->FilePointer >= FatFileInfo->FileSize)
	{
		return TRUE;
	}

	//
	// If they are trying to read more than there is to read
	// then adjust the amount to read
	//
	if ((FatFileInfo->FilePointer + BytesToRead) > FatFileInfo->FileSize)
	{
		BytesToRead = (FatFileInfo->FileSize - FatFileInfo->FilePointer);
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
	// | Cluster 1 | Cluster 2 | Cluster 3 | Cluster 4 |
	// +-----------+-----------+-----------+-----------+
	//    |                                    |
	//    +---------------+--------------------+
	//                    |
	// BytesToRead -------+
	//
	// 1 - The first calculation (and read) will align
	//     the file pointer with the next cluster.
	//     boundary (if we are supposed to read that much)
	// 2 - The next calculation (and read) will read
	//     in all the full clusters that the requested
	//     amount of data would cover (in this case
	//     clusters 2 & 3).
	// 3 - The last calculation (and read) would read
	//     in the remainder of the data requested out of
	//     the last cluster.
	//

	BytesPerCluster = (FatVolumeBootSector->SectorsPerCluster * FatVolumeBootSector->BytesPerSector);

	//
	// Only do the first read if we
	// aren't aligned on a cluster boundary
	//
	if (FatFileInfo->FilePointer % BytesPerCluster)
	{
		//
		// Do the math for our first read
		//
		ClusterNumber = (FatFileInfo->FilePointer / BytesPerCluster);
		ClusterNumber = FatFileInfo->FileFatChain[ClusterNumber];
		OffsetInCluster = (FatFileInfo->FilePointer % BytesPerCluster);
		LengthInCluster = (BytesToRead > (BytesPerCluster - OffsetInCluster)) ? (BytesPerCluster - OffsetInCluster) : BytesToRead;

		//
		// Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
		//
		if (!FatReadPartialCluster(ClusterNumber, OffsetInCluster, LengthInCluster, Buffer))
		{
			return FALSE;
		}
		if (BytesRead != NULL)
		{
			*BytesRead += LengthInCluster;
		}
		BytesToRead -= LengthInCluster;
		FatFileInfo->FilePointer += LengthInCluster;
		Buffer += LengthInCluster;
	}

	//
	// Do the math for our second read (if any data left)
	//
	if (BytesToRead > 0)
	{
		//
		// Determine how many full clusters we need to read
		//
		NumberOfClusters = (BytesToRead / BytesPerCluster);

		if (NumberOfClusters > 0)
		{
			ClusterNumber = (FatFileInfo->FilePointer / BytesPerCluster);
			ClusterNumber = FatFileInfo->FileFatChain[ClusterNumber];

			//
			// Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
			//
			if (!FatReadClusterChain(ClusterNumber, NumberOfClusters, Buffer))
			{
				return FALSE;
			}
			if (BytesRead != NULL)
			{
				*BytesRead += (NumberOfClusters * BytesPerCluster);
			}
			BytesToRead -= (NumberOfClusters * BytesPerCluster);
			FatFileInfo->FilePointer += (NumberOfClusters * BytesPerCluster);
			Buffer += (NumberOfClusters * BytesPerCluster);
		}
	}

	//
	// Do the math for our third read (if any data left)
	//
	if (BytesToRead > 0)
	{
		ClusterNumber = (FatFileInfo->FilePointer / BytesPerCluster);
		ClusterNumber = FatFileInfo->FileFatChain[ClusterNumber];

		//
		// Now do the read and update BytesRead, BytesToRead, FilePointer, & Buffer
		//
		if (!FatReadPartialCluster(ClusterNumber, 0, BytesToRead, Buffer))
		{
			return FALSE;
		}
		if (BytesRead != NULL)
		{
			*BytesRead += BytesToRead;
		}
		BytesToRead -= BytesToRead;
		FatFileInfo->FilePointer += BytesToRead;
		Buffer += BytesToRead;
	}

	return TRUE;
}

ULONG FatGetFileSize(FILE *FileHandle)
{
	PFAT_FILE_INFO	FatFileHandle = (PFAT_FILE_INFO)FileHandle;

	DbgPrint((DPRINT_FILESYSTEM, "FatGetFileSize() FileSize = %d\n", FatFileHandle->FileSize));

	return FatFileHandle->FileSize;
}

VOID FatSetFilePointer(FILE *FileHandle, ULONG NewFilePointer)
{
	PFAT_FILE_INFO	FatFileHandle = (PFAT_FILE_INFO)FileHandle;

	DbgPrint((DPRINT_FILESYSTEM, "FatSetFilePointer() NewFilePointer = %d\n", NewFilePointer));

	FatFileHandle->FilePointer = NewFilePointer;
}

ULONG FatGetFilePointer(FILE *FileHandle)
{
	PFAT_FILE_INFO	FatFileHandle = (PFAT_FILE_INFO)FileHandle;

	DbgPrint((DPRINT_FILESYSTEM, "FatGetFilePointer() FilePointer = %d\n", FatFileHandle->FilePointer));

	return FatFileHandle->FilePointer;
}

BOOL FatReadVolumeSectors(ULONG DriveNumber, ULONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
	//return DiskReadMultipleLogicalSectors(DriveNumber, SectorNumber + FatVolumeBootSector->HiddenSectors, SectorCount, Buffer);
	return CacheReadDiskSectors(DriveNumber, SectorNumber + FatVolumeBootSector->HiddenSectors, SectorCount, Buffer);
}
