/*
 *  FreeLoader - install.c
 *
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
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

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "install.h"
#include "volume.h"
#include "../bootsect/fat.h"
#include "../bootsect/fat32.h"

BOOL	BackupBootSector(LPCTSTR lpszVolumeName);
BOOL	InstallBootSector(LPCTSTR lpszVolumeType);

int main(int argc, char *argv[])
{

	if (argc < 3)
	{
		_tprintf(_T("syntax: install x: [fs_type]\nwhere fs_type is fat or fat32\n"));
		return -1;
	}

	if (!OpenVolume(argv[1]))
	{
		return -1;
	}

	BackupBootSector(argv[1]);

	InstallBootSector(argv[2]);

	_tprintf(_T("You must now copy freeldr.sys & freeldr.ini to %s.\n"), argv[1]);

	CloseVolume();

	return 0;
}

BOOL BackupBootSector(LPCTSTR lpszVolumeName)
{
	HANDLE	hBackupFile;
	TCHAR	szFileName[MAX_PATH];
	ULONG	Count;
	BYTE	BootSectorBuffer[512];
	DWORD	dwNumberOfBytesWritten;
	BOOL	bRetVal;

	//
	// Find the next unused filename and open it
	//
	for (Count=0; ; Count++)
	{
		//
		// Generate the next filename
		//
		_stprintf(szFileName, _T("%s\\bootsect.%03ld"), lpszVolumeName, Count);

		//
		// Try to create a new file, fail if exists
		//
		hBackupFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_NEW, /*FILE_ATTRIBUTE_SYSTEM*/0, NULL);

		//
		// Check to see if it worked
		//
		if (hBackupFile != INVALID_HANDLE_VALUE)
		{
			break;
		}

		//
		// Nope, didn't work
		// Check to see if it already existed
		//
		if (!(GetLastError() != ERROR_ALREADY_EXISTS))
		{
			_tprintf(_T("%s:%d: "), __FILE__, __LINE__);
			_tprintf(_T("Boot sector backup failed. Error code %ld.\n"), GetLastError());
			return FALSE;
		}
	}

	//
	// Try to read the boot sector
	//
	if (!ReadVolumeSector(0, BootSectorBuffer))
	{
		CloseHandle(hBackupFile);
		return FALSE;
	}

	//
	// Try to write the boot sector data to the file
	//
	bRetVal = WriteFile(hBackupFile, BootSectorBuffer, 512, &dwNumberOfBytesWritten, NULL);
	if (!bRetVal || (dwNumberOfBytesWritten != 512))
	{
		CloseHandle(hBackupFile);
		_tprintf(_T("%s:%d: "), __FILE__, __LINE__);
		_tprintf(_T("WriteFile() failed. Error code %ld.\n"), GetLastError());
		return FALSE;
	}

	_tprintf(_T("Boot sector backed up to file: %s\n"), szFileName);

	CloseHandle(hBackupFile);

	return TRUE;
}

BOOL InstallBootSector(LPCTSTR lpszVolumeType)
{
	BYTE	BootSectorBuffer[512];

	//
	// Read in the old boot sector
	//
	if (!ReadVolumeSector(0, BootSectorBuffer))
	{
		return FALSE;
	}

	if (_tcsicmp(lpszVolumeType, _T("fat")) == 0)
	{
		//
		// Update the BPB in the new boot sector
		//
		memcpy((fat_data+3), (BootSectorBuffer+3), 59 /*fat BPB length*/);

		//
		// Write out new boot sector
		//
		if (!WriteVolumeSector(0, fat_data))
		{
			return FALSE;
		}
	}
	else if (_tcsicmp(lpszVolumeType, _T("fat32")) == 0)
	{
		//
		// Update the BPB in the new boot sector
		//
		memcpy((fat32_data+3), (BootSectorBuffer+3), 87 /*fat32 BPB length*/);

		//
		// Write out new boot sector
		//
		if (!WriteVolumeSector(0, fat32_data))
		{
			return FALSE;
		}

		//
		// Write out new extra sector
		//
		if (!WriteVolumeSector(14, (fat32_data+512)))
		{
			return FALSE;
		}
	}
	else
	{
		_tprintf(_T("%s:%d: "), __FILE__, __LINE__);
		_tprintf(_T("File system type %s unknown.\n"), lpszVolumeType);
		return FALSE;
	}

	_tprintf(_T("%s boot sector installed.\n"), lpszVolumeType);

	return TRUE;
}
