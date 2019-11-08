/*
 * PROJECT:     ReactOS FreeLoader installer for Linux
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Volume functions
 * COPYRIGHT:   Copyright 2001 Brian Palmer (brianp@sginet.com)
 *              Copyright 2019 Arnav Bhatt (arnavbhatt2004@gmail.com)
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
	
	if (hDiskVolume < 0)
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
	
	if (dwFilePosition != (SectorNumber * 512))
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
	
	if (dwFilePosition != (SectorNumber * 512))
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
