/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000  Brian Palmer  <brianp@sginet.com>
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

#include "freeldr.h"
#include "fs.h"
#include "stdlib.h"
#include "tui.h"
#include "asmcode.h"

#define	FS_DO_ERROR(s)	\
	{ \
		if (UserInterfaceUp) \
			MessageBox(s); \
		else \
		{ \
			printf(s); \
			printf("\nPress any key\n"); \
			getch(); \
		} \
	}


int		nSectorBuffered  = -1;	// Tells us which sector was read into SectorBuffer[]

BYTE	SectorBuffer[512];		// 512 byte buffer space for read operations, ReadOneSector reads to here

int		FSType = NULL;			// Type of filesystem on boot device, set by OpenDiskDrive()

char	*pFileSysData = (char *)(FILESYSADDR); // Load address for filesystem data
char	*pFat32FATCacheIndex = (char *)(FILESYSADDR); // Load address for filesystem data

BOOL OpenDiskDrive(int nDrive, int nPartition)
{
	int		num_bootable_partitions = 0;
	int		boot_partition = 0;
	int		partition_type = 0;
	int		head, sector, cylinder;
	int		offset;

	// Check and see if it is a floppy drive
	if (nDrive < 0x80)
	{
		// Read boot sector
		if (!biosdisk(_DISK_READ, nDrive, 0, 0, 1, 1, SectorBuffer))
		{
			FS_DO_ERROR("Disk Read Error");
			return FALSE;
		}

		// Check for validity
		if (*((WORD*)(SectorBuffer + 0x1fe)) != 0xaa55)//(SectorBuffer[0x1FE] != 0x55) || (SectorBuffer[0x1FF] != 0xAA))
		{
			FS_DO_ERROR("Invalid boot sector magic (0xaa55)");
			return FALSE;
		}
	}
	else
	{
		// Read master boot record
		if (!biosdisk(_DISK_READ, nDrive, 0, 0, 1, 1, SectorBuffer))
		{
			FS_DO_ERROR("Disk Read Error");
			return FALSE;
		}

		// Check for validity
		if (*((WORD*)(SectorBuffer + 0x1fe)) != 0xaa55)//(SectorBuffer[0x1FE] != 0x55) || (SectorBuffer[0x1FF] != 0xAA))
		{
			FS_DO_ERROR("Invalid partition table magic (0xaa55)");
			return FALSE;
		}

		if (nPartition == 0)
		{
			// Check for bootable partitions
			if (SectorBuffer[0x1BE] == 0x80)
				num_bootable_partitions++;
			if (SectorBuffer[0x1CE] == 0x80)
			{
				num_bootable_partitions++;
				boot_partition = 1;
			}
			if (SectorBuffer[0x1DE] == 0x80)
			{
				num_bootable_partitions++;
				boot_partition = 2;
			}
			if (SectorBuffer[0x1EE] == 0x80)
			{
				num_bootable_partitions++;
				boot_partition = 3;
			}

			// Make sure there was only one bootable partition
			if (num_bootable_partitions > 1)
			{
				FS_DO_ERROR("Too many boot partitions");
				return FALSE;
			}

			offset = 0x1BE + (boot_partition * 0x10);
		}
		else
			offset = 0x1BE + ((nPartition-1) * 0x10);

		partition_type = SectorBuffer[offset + 4];

		// Check for valid partition
		if (partition_type == 0)
		{
			FS_DO_ERROR("Invalid boot partition");
			return FALSE;
		}

		head = SectorBuffer[offset + 1];
		sector = (SectorBuffer[offset + 2] & 0x3F);
		cylinder = SectorBuffer[offset + 3];
		if (SectorBuffer[offset + 2] & 0x80)
			cylinder += 0x200;
		if (SectorBuffer[offset + 2] & 0x40)
			cylinder += 0x100;

		// Read partition boot sector
		if (!biosdisk(_DISK_READ, nDrive, head, cylinder, sector, 1, SectorBuffer))
		{
			FS_DO_ERROR("Disk Read Error");
			return FALSE;
		}

		// Check for validity
		if (*((WORD*)(SectorBuffer + 0x1fe)) != 0xaa55)//(SectorBuffer[0x1FE] != 0x55) || (SectorBuffer[0x1FF] != 0xAA))
		{
			FS_DO_ERROR("Invalid boot sector magic (0xaa55)");
			return FALSE;
		}
	}


	// Reset data
	nBytesPerSector = 0;
	nSectorsPerCluster = 0;
	nReservedSectors = 0;
	nNumberOfFATs = 0;
	nRootDirEntries = 0;
	nTotalSectors16 = 0;
	nSectorsPerFAT16 = 0;
	nSectorsPerTrack = 0;
	nNumberOfHeads = 0;
	nHiddenSectors = 0;
	nTotalSectors32 = 0;

	nSectorsPerFAT32 = 0;
	nExtendedFlags = 0;
	nFileSystemVersion = 0;
	nRootDirStartCluster = 0;

	nRootDirSectorStart = 0;
	nDataSectorStart = 0;
	nSectorsPerFAT = 0;
	nRootDirSectors = 0;
	nTotalSectors = 0;
	nNumberOfClusters = 0;

	// Get data
	memcpy(&nBytesPerSector, SectorBuffer + BPB_BYTESPERSECTOR, 2);
	memcpy(&nSectorsPerCluster, SectorBuffer + BPB_SECTORSPERCLUSTER, 1);
	memcpy(&nReservedSectors, SectorBuffer + BPB_RESERVEDSECTORS, 2);
	memcpy(&nNumberOfFATs, SectorBuffer + BPB_NUMBEROFFATS, 1);
	memcpy(&nRootDirEntries, SectorBuffer + BPB_ROOTDIRENTRIES, 2);
	memcpy(&nTotalSectors16, SectorBuffer + BPB_TOTALSECTORS16, 2);
	memcpy(&nSectorsPerFAT16, SectorBuffer + BPB_SECTORSPERFAT16, 2);
	memcpy(&nSectorsPerTrack, SectorBuffer + BPB_SECTORSPERTRACK, 2);
	memcpy(&nNumberOfHeads, SectorBuffer + BPB_NUMBEROFHEADS, 2);
	memcpy(&nHiddenSectors, SectorBuffer + BPB_HIDDENSECTORS, 4);
	memcpy(&nTotalSectors32, SectorBuffer + BPB_TOTALSECTORS32, 4);

	memcpy(&nSectorsPerFAT32, SectorBuffer + BPB_SECTORSPERFAT32, 4);
	memcpy(&nExtendedFlags, SectorBuffer + BPB_EXTENDEDFLAGS32, 2);
	memcpy(&nFileSystemVersion, SectorBuffer + BPB_FILESYSTEMVERSION32, 2);
	memcpy(&nRootDirStartCluster, SectorBuffer + BPB_ROOTDIRSTARTCLUSTER32, 4);

	// Calc some stuff
	if (nTotalSectors16 != 0)
		nTotalSectors = nTotalSectors16;
	else
		nTotalSectors = nTotalSectors32; 

	if (nSectorsPerFAT16 != 0)
		nSectorsPerFAT = nSectorsPerFAT16;
	else
		nSectorsPerFAT = nSectorsPerFAT32; 

	nRootDirSectorStart = (nNumberOfFATs * nSectorsPerFAT) + nReservedSectors;
	nRootDirSectors = ((nRootDirEntries * 32) + (nBytesPerSector - 1)) / nBytesPerSector;
	nDataSectorStart = nReservedSectors + (nNumberOfFATs * nSectorsPerFAT) + nRootDirSectors;
	nNumberOfClusters = (nTotalSectors - nDataSectorStart) / nSectorsPerCluster;

	// Determine FAT type
	if (nNumberOfClusters < 4085)
	{
		/* Volume is FAT12 */
		nFATType = FAT12;
	}
	else if (nNumberOfClusters < 65525)
	{
		/* Volume is FAT16 */
		nFATType = FAT16;
	}
	else
	{
		/* Volume is FAT32 */
		nFATType = FAT32;

		// Check version
		// we only work with version 0
		if (*((WORD*)(SectorBuffer + BPB_FILESYSTEMVERSION32)) != 0)
		{
			FS_DO_ERROR("Error: FreeLoader is too old to work with this FAT32 filesystem.\nPlease update FreeLoader.");
			return FALSE;
		}
	}

	FSType = FS_FAT;

	// Buffer the FAT table if it is small enough
	if ((FSType == FS_FAT) && (nFATType == FAT12))
	{
		if (!ReadMultipleSectors(nReservedSectors, nSectorsPerFAT, pFileSysData))
			return FALSE;
	}
	else if ((FSType == FS_FAT) && (nFATType == FAT16))
	{
		if (!ReadMultipleSectors(nReservedSectors, nSectorsPerFAT, pFileSysData))
			return FALSE;
	}
	else if ((FSType == FS_FAT) && (nFATType == FAT32))
	{
		// The FAT table is too big to be buffered so
		// we will initialize our cache and cache it
		// on demand
		for (offset=0; offset<256; offset++)
			((int*)pFat32FATCacheIndex)[offset] = -1;
	}

	return TRUE;
}

BOOL ReadMultipleSectors(int nSect, int nNumberOfSectors, void *pBuffer)
{
	BOOL	bRetVal;
	int		PhysicalSector;
	int		PhysicalHead;
	int		PhysicalTrack;
	int		nNum;

	nSect += nHiddenSectors;

	while (nNumberOfSectors)
	{
		PhysicalSector = 1 + (nSect % nSectorsPerTrack);
		PhysicalHead = (nSect / nSectorsPerTrack) % nNumberOfHeads;
		PhysicalTrack = nSect / (nSectorsPerTrack * nNumberOfHeads);

		if (PhysicalSector > 1)
		{
			if (nNumberOfSectors >= (nSectorsPerTrack - (PhysicalSector - 1)))
				nNum = (nSectorsPerTrack - (PhysicalSector - 1));
			else
				nNum = nNumberOfSectors;
		}
		else
		{
			if (nNumberOfSectors >= nSectorsPerTrack)
				nNum = nSectorsPerTrack;
			else
				nNum = nNumberOfSectors;
		}

		bRetVal = biosdisk(_DISK_READ, BootDrive, PhysicalHead, PhysicalTrack, PhysicalSector, nNum, pBuffer);

		if (!bRetVal)
		{
			FS_DO_ERROR("Disk Error");
			return FALSE;
		}

		pBuffer += (nNum * 512);
		nNumberOfSectors -= nNum;
		nSect += nNum;
	}

	return TRUE;
}

BOOL ReadOneSector(int nSect)
{
	BOOL	bRetVal;
	int		PhysicalSector;
	int		PhysicalHead;
	int		PhysicalTrack;

	nSectorBuffered = nSect;

	nSect += nHiddenSectors;
	PhysicalSector = 1 + (nSect % nSectorsPerTrack);
	PhysicalHead = (nSect / nSectorsPerTrack) % nNumberOfHeads;
	PhysicalTrack = nSect / (nSectorsPerTrack * nNumberOfHeads);


	bRetVal = biosdisk(_DISK_READ, BootDrive, PhysicalHead, PhysicalTrack, PhysicalSector, 1, SectorBuffer);

	if (!bRetVal)
	{
		FS_DO_ERROR("Disk Error");
		return FALSE;
	}

	return TRUE;
}

BOOL OpenFile(char *filename, FILE *pFile)
{
	switch(FSType)
	{
	case FS_FAT:
		if(!FATOpenFile(filename, &(pFile->fat)))
			return FALSE;
		pFile->filesize = pFile->fat.dwSize;
		break;
	default:
		FS_DO_ERROR("Error: Unknown filesystem.");
		return FALSE;
		break;
	}

	return TRUE;
}

/*
 * ReadFile()
 * returns number of bytes read or EOF
 */
int ReadFile(FILE *pFile, int count, void *buffer)
{
	switch(FSType)
	{
	case FS_FAT:
		return FATRead(&(pFile->fat), count, buffer);
	default:
		FS_DO_ERROR("Error: Unknown filesystem.");
		return EOF;
	}

	return 0;
}

DWORD GetFileSize(FILE *pFile)
{
	return pFile->filesize;
}

DWORD Rewind(FILE *pFile)
{
	switch (FSType)
	{
	case FS_FAT:
		pFile->fat.dwCurrentCluster = pFile->fat.dwStartCluster;
		pFile->fat.dwCurrentReadOffset = 0;
		break;
	default:
		FS_DO_ERROR("Error: Unknown filesystem.");
		break;
	}

	return pFile->filesize;
}

int feof(FILE *pFile)
{
	switch (FSType)
	{
	case FS_FAT:
		if (pFile->fat.dwCurrentReadOffset >= pFile->fat.dwSize)
			return TRUE;
		else
			return FALSE;
		break;
	default:
		FS_DO_ERROR("Error: Unknown filesystem.");
		return TRUE;
		break;
	}

	return TRUE;
}

int fseek(FILE *pFile, DWORD offset)
{
	switch (FSType)
	{
	case FS_FAT:
		return FATfseek(&(pFile->fat), offset);
		break;
	default:
		FS_DO_ERROR("Error: Unknown filesystem.");
		break;
	}

	return -1;
}
