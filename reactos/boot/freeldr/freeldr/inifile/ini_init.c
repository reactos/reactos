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

BOOLEAN IniFileInitialize(VOID)
{
	PFILE	Freeldr_Ini;	// File handle for freeldr.ini
	PCHAR	FreeLoaderIniFileData;
	ULONG		FreeLoaderIniFileSize;
	BOOLEAN	Success;

	// Open freeldr.ini
	Freeldr_Ini = IniOpenIniFile();

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

PFILE IniOpenIniFile()
{
	PFILE	IniFileHandle;	// File handle for freeldr.ini

	// Try to open freeldr.ini
	IniFileHandle = FsOpenFile("freeldr.ini");

	return IniFileHandle;
}
