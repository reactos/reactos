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
#include "ini.h"
#include <fs.h>
#include <rtl.h>
#include <mm.h>
#include <debug.h>


BOOL IniFileInitialize(VOID)
{
	PFILE	Freeldr_Ini;	// File handle for freeldr.ini
	PUCHAR	FreeLoaderIniFileData;
	U32		FreeLoaderIniFileSize;
	BOOL	Success;

	// Open freeldr.ini
	// BootDrive & BootPartition are passed
	// in from the boot sector code in the
	// DL & DH registers.
	Freeldr_Ini = IniOpenIniFile(BootDrive, BootPartition);

	// If we couldn't open freeldr.ini on the partition
	// they specified in the boot sector then try
	// opening the active (boot) partition.
	if ((Freeldr_Ini == NULL) && (BootPartition != 0))
	{
		BootPartition = 0;

		Freeldr_Ini = IniOpenIniFile(BootDrive, BootPartition);

		return FALSE;
	}

	if (Freeldr_Ini == NULL)
	{
		printf("Error opening freeldr.ini or file not found.\n");
		printf("You need to re-install FreeLoader.\n");
		return FALSE;
	}

	// Get the file size & allocate enough memory for it
	FreeLoaderIniFileSize = FsGetFileSize(Freeldr_Ini);
	FreeLoaderIniFileData = MmAllocateMemory(FreeLoaderIniFileSize);

	// If we are out of memory then return FALSE
	if (FreeLoaderIniFileData == NULL)
	{
		printf("Out of memory while loading freeldr.ini.\n");
		FsCloseFile(Freeldr_Ini);
		return FALSE;
	}

	// Read freeldr.ini off the disk
	if (!FsReadFile(Freeldr_Ini, FreeLoaderIniFileSize, NULL, FreeLoaderIniFileData))
	{
		FsCloseFile(Freeldr_Ini);
		MmFreeMemory(FreeLoaderIniFileData);
		return FALSE;
	}

	FsCloseFile(Freeldr_Ini);

	// Parse the .ini file data
	Success = IniParseFile(FreeLoaderIniFileData, FreeLoaderIniFileSize);

	MmFreeMemory(FreeLoaderIniFileData);

	return Success;
}

PFILE IniOpenIniFile(U8 BootDriveNumber, U8 BootPartitionNumber)
{
	PFILE	IniFileHandle;	// File handle for freeldr.ini

	if (!FsOpenVolume(BootDriveNumber, BootPartitionNumber))
	{
		if (BootPartitionNumber == 0)
		{
			printf("Error opening active (bootable) partition on boot drive 0x%x for file access.\n", BootDriveNumber);
		}
		else
		{
			printf("Error opening partition %d on boot drive 0x%x for file access.\n", BootPartitionNumber, BootDriveNumber);
		}

		return NULL;
	}

	// Try to open freeldr.ini
	IniFileHandle = FsOpenFile("freeldr.ini");

	return IniFileHandle;
}
