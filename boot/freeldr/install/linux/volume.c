/*
 *  FreeLoader - volume.c
 *
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
 *
 *  Ported to Linux by Arnav Bhatt <arnavbhatt2004@gmail.com>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h> 
#include "volume.h"
#include <unistd.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h> 

static int hDiskVolume = 0;

bool OpenVolume(char* lpszVolumeName)
{
	char RealVolumeName[512];
	
	strcpy(RealVolumeName, lpszVolumeName);
	
	printf("Opening volume %s\n", lpszVolumeName);
	
	hDiskVolume = open(lpszVolumeName, O_RDWR | O_SYNC);
	
	if(hDiskVolume < 0)
	{
		perror("OpenVolume() failed!");
		return false;
	}
	
	return true;
}

void CloseVolume(void)
{
    close(hDiskVolume);
}

bool ReadVolumeSector(long SectorNumber, void* SectorBuffer)
{
	int dwNumberOfBytesRead;
	int dwFilePosition;
	
	dwFilePosition = lseek(hDiskVolume, (SectorNumber* 512), SEEK_SET);
	
	if(dwFilePosition != (SectorNumber * 512))
	{
		perror("ReadVolumeSector() failed!");
		return false;
	}
	
	dwNumberOfBytesRead = read(hDiskVolume, SectorBuffer, 512);
	
	if (dwNumberOfBytesRead != 512)
	{
		perror("ReadVolumeSector() failed!");
		return false;
	}
	
	return true;
}

bool WriteVolumeSector(long SectorNumber, void* SectorBuffer)
{
	int dwNumberOfBytesWritten;
	int dwFilePosition;
	
	dwFilePosition = lseek(hDiskVolume, (SectorNumber * 512), SEEK_SET);
	
	if(dwFilePosition != (SectorNumber * 512))
	{
		perror("WriteVolumeSector() failed!");
		return false;
	}
	
	dwNumberOfBytesWritten = write(hDiskVolume, SectorBuffer, 512);
	
	if (dwNumberOfBytesWritten != 512)
	{
		perror("WriteVolumeSector() failed!");
		return false;
	}
	
	return true;
}
