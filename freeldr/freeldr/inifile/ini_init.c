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

	// Open the boot drive for file access
	if (!OpenDiskDrive(BootDrive, 0))
	{
		printf("Error opening boot drive for file access.\n");
		return FALSE;
	}

	// Try to open freeldr.ini or fail
	Freeldr_Ini = OpenFile("freeldr.ini");
	if (Freeldr_Ini == NULL)
	{
		printf("FREELDR.INI not found.\nYou need to re-install FreeLoader.\n");
		return FALSE;
	}

	// Get the file size & allocate enough memory for it
	FreeLoaderIniFileSize = GetFileSize(Freeldr_Ini);
	FreeLoaderIniFileData = MmAllocateMemory(FreeLoaderIniFileSize);

	// If we are out of memory then return FALSE
	if (FreeLoaderIniFileData == NULL)
	{
		printf("Out of memory while loading FREELDR.INI.\n");
		CloseFile(Freeldr_Ini);
		return FALSE;
	}

	// Read freeldr.ini off the disk
	if (!ReadFile(Freeldr_Ini, FreeLoaderIniFileSize, NULL, FreeLoaderIniFileData))
	{
		CloseFile(Freeldr_Ini);
		MmFreeMemory(FreeLoaderIniFileData);
		return FALSE;
	}

	CloseFile(Freeldr_Ini);

	// Parse the .ini file data
	Success = IniParseFile(FreeLoaderIniFileData, FreeLoaderIniFileSize);

	MmFreeMemory(FreeLoaderIniFileData);

	return Success;
}
