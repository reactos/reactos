/*
 *  FreeLoader - volume.c
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
#include <stdio.h>
#include <tchar.h>
#include "volume.h"

static HANDLE	hDiskVolume = NULL;

BOOL OpenVolume(LPCTSTR lpszVolumeName)
{
	TCHAR	RealVolumeName[MAX_PATH];

	//
	// If they passed in a drive letter (e.g. "A:")
	// then try to open the physical device volume,
	// otherwise we will assume it is a disk image
	// file they are writing to. (not fully supported yet)
	//
	if ((_tcslen(lpszVolumeName) == 2) && (lpszVolumeName[1] == _T(':')))
	{
		_tcscpy(RealVolumeName, _T("\\\\.\\"));
		_tcscat(RealVolumeName, lpszVolumeName);
	}
	else
	{
		_tcscpy(RealVolumeName, lpszVolumeName);
	}

	_tprintf(_T("Opening volume \'%s\'\n"), lpszVolumeName);

	hDiskVolume = CreateFile(RealVolumeName,
							GENERIC_READ | GENERIC_WRITE,
							FILE_SHARE_READ | FILE_SHARE_WRITE,
							NULL,
							OPEN_EXISTING,
							0,
							NULL);

	if (hDiskVolume == INVALID_HANDLE_VALUE)
	{
		_tprintf(_T("%s:%d: "), __FILE__, __LINE__);
		_tprintf(_T("Failed. Error code %d.\n"), GetLastError());
		return FALSE;
	}

	return TRUE;
}

void CloseVolume(void)
{
	CloseHandle(hDiskVolume);
}

BOOL ReadVolumeSector(ULONG SectorNumber, PVOID SectorBuffer)
{
	DWORD	dwNumberOfBytesRead;
	DWORD	dwFilePosition;
	BOOL	bRetVal;

	//
	// FIXME: this doesn't seem to handle the situation
	// properly when SectorNumber is bigger than the
	// amount of sectors on the disk. Seems to me that
	// the call to SetFilePointer() should just give an
	// out of bounds error or something but it doesn't.
	//
	dwFilePosition = SetFilePointer(hDiskVolume, (SectorNumber * 512), NULL, FILE_BEGIN);
	if (dwFilePosition != (SectorNumber * 512))
	{
		_tprintf(_T("%s:%d: "), __FILE__, __LINE__);
		_tprintf(_T("SetFilePointer() failed. Error code %d.\n"), GetLastError());
		return FALSE;
	}
	
	bRetVal = ReadFile(hDiskVolume, SectorBuffer, 512, &dwNumberOfBytesRead, NULL);
	if (!bRetVal || (dwNumberOfBytesRead != 512))
	{
		_tprintf(_T("%s:%d: "), __FILE__, __LINE__);
		_tprintf(_T("ReadFile() failed. Error code %d.\n"), GetLastError());
		return FALSE;
	}

	return TRUE;
}

BOOL WriteVolumeSector(ULONG SectorNumber, PVOID SectorBuffer)
{
	DWORD	dwNumberOfBytesWritten;
	DWORD	dwFilePosition;
	BOOL	bRetVal;

	//
	// FIXME: this doesn't seem to handle the situation
	// properly when SectorNumber is bigger than the
	// amount of sectors on the disk. Seems to me that
	// the call to SetFilePointer() should just give an
	// out of bounds error or something but it doesn't.
	//
	dwFilePosition = SetFilePointer(hDiskVolume, (SectorNumber * 512), NULL, FILE_BEGIN);
	if (dwFilePosition != (SectorNumber * 512))
	{
		_tprintf(_T("%s:%d: "), __FILE__, __LINE__);
		_tprintf(_T("SetFilePointer() failed. Error code %d.\n"), GetLastError());
		return FALSE;
	}
	
	bRetVal = WriteFile(hDiskVolume, SectorBuffer, 512, &dwNumberOfBytesWritten, NULL);
	if (!bRetVal || (dwNumberOfBytesWritten != 512))
	{
		_tprintf(_T("%s:%d: "), __FILE__, __LINE__);
		_tprintf(_T("WriteFile() failed. Error code %d.\n"), GetLastError());
		return FALSE;
	}

	return TRUE;
}
