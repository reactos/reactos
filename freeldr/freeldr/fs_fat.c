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

int		nFATType = FAT12;

DWORD	nBytesPerSector;		// Bytes per sector
DWORD	nSectorsPerCluster;		// Number of sectors in a cluster
DWORD	nReservedSectors;		// Reserved sectors, usually 1 (the bootsector)
DWORD	nNumberOfFATs;			// Number of FAT tables
DWORD	nRootDirEntries;		// Number of root directory entries (fat12/16)
DWORD	nTotalSectors16;		// Number of total sectors on the drive, 16-bit
DWORD	nSectorsPerFAT16;		// Sectors per FAT table (fat12/16)
DWORD	nSectorsPerTrack;		// Number of sectors in a track
DWORD	nNumberOfHeads;			// Number of heads on the disk
DWORD	nHiddenSectors;			// Hidden sectors (sectors before the partition start like the partition table)
DWORD	nTotalSectors32;		// Number of total sectors on the drive, 32-bit

DWORD	nSectorsPerFAT32;		// Sectors per FAT table (fat32)
DWORD	nExtendedFlags;			// Extended flags (fat32)
DWORD	nFileSystemVersion;		// File system version (fat32)
DWORD	nRootDirStartCluster;	// Starting cluster of the root directory (fat32)

DWORD	nRootDirSectorStart;	// Starting sector of the root directory (fat12/16)
DWORD	nDataSectorStart;		// Starting sector of the data area
DWORD	nSectorsPerFAT;			// Sectors per FAT table
DWORD	nRootDirSectors;		// Number of sectors of the root directory (fat32)
DWORD	nTotalSectors;			// Total sectors on the drive
DWORD	nNumberOfClusters;		// Number of clusters on the drive

BOOL FATReadRootDirectoryEntry(int nDirEntry, void *pDirEntryBuf)
{
	DWORD	nDirEntrySector;
	int		nOffsetWithinSector;

	nDirEntrySector = nRootDirSectorStart + ((nDirEntry * 32) / nBytesPerSector);

	if (!ReadOneSector(nDirEntrySector))
		return FALSE;

	nOffsetWithinSector = (nDirEntry * 32) % nBytesPerSector;

	memcpy(pDirEntryBuf, SectorBuffer + nOffsetWithinSector, 32);

	if (*((char *)pDirEntryBuf) == 0x05)
		*((char *)pDirEntryBuf) = 0xE5;

	return TRUE;
}

BOOL FATReadDirectoryEntry(DWORD nDirStartCluster, int nDirEntry, void *pDirEntryBuf)
{
	DWORD	nRealDirCluster;
	int		nSectorWithinCluster;
	int		nOffsetWithinSector;
	int		nSectorToRead;
	int		i;

	i = (nDirEntry * 32) / (nSectorsPerCluster * nBytesPerSector);
	//if ((nDirEntry * 32) % (nSectorsPerCluster * nBytesPerSector))
	//	i++;

	for (nRealDirCluster = nDirStartCluster; i; i--)
		nRealDirCluster = FATGetFATEntry(nRealDirCluster);

	nSectorWithinCluster = ((nDirEntry * 32) % (nSectorsPerCluster * nBytesPerSector)) / nBytesPerSector;

	nSectorToRead = ((nRealDirCluster - 2) * nSectorsPerCluster) + nDataSectorStart + nSectorWithinCluster;

	if (!ReadOneSector(nSectorToRead))
		return FALSE;

	nOffsetWithinSector = (nDirEntry * 32) % nBytesPerSector;

	memcpy(pDirEntryBuf, SectorBuffer + nOffsetWithinSector, 32);

	if (*((char *)pDirEntryBuf) == 0x05)
		*((char *)pDirEntryBuf) = 0xE5;

	return TRUE;
}

/*
 * FATLookupFile()
 * This function searches the file system for the
 * specified filename and fills in a FAT_STRUCT structure
 * with info describing the file, etc. returns true
 * if the file exists or false otherwise
 */
BOOL FATLookupFile(char *file, PFAT_STRUCT pFatStruct)
{
	int			i, j;
	int			numparts;
	char		filename[12];
	BYTE		direntry[32];
	int			nNumDirEntries;
	FAT_STRUCT	fatstruct;
	BOOL		bFound;
	DWORD		cluster;

	memset(pFatStruct, 0, sizeof(FAT_STRUCT));

	// Check and see if the first character is '\' and remove it if so
	if (*file == '\\')
		file++;

	// Figure out how many sub-directories we are nested in
	numparts = FATGetNumPathParts(file);

	// Loop once for each part
	for (i=0; i<numparts; i++)
	{
		// Make filename compatible with MSDOS dir entry
		if (!FATGetFirstNameFromPath(filename, file))
			return FALSE;
		// Advance to the next part of the path
		for (; (*file != '\\') && (*file != '\0'); file++);
		file++;

		// If we didn't find the correct sub-directory the fail
		if ((i != 0) && !bFound)
			return FALSE;

		bFound = FALSE;

		// Check if we are pulling from the root directory of a fat12/fat16 disk
		if ((i == 0) && ((nFATType == FAT12) || (nFATType == FAT16)))
			nNumDirEntries = nRootDirEntries;
		else if ((i == 0) && (nFATType == FAT32))
		{
			cluster = nRootDirStartCluster;
			fatstruct.dwSize = nSectorsPerCluster * nBytesPerSector;
			while((cluster = FATGetFATEntry(cluster)) < 0x0FFFFFF8)
				fatstruct.dwSize += nSectorsPerCluster * nBytesPerSector;

			fatstruct.dwStartCluster = nRootDirStartCluster;
			nNumDirEntries = fatstruct.dwSize / 32;
		}
		else
			nNumDirEntries = fatstruct.dwSize / 32;

		// Loop through each directory entry
		for (j=0; j<nNumDirEntries; j++)
		{
			// Read the entry
			if ((i == 0) && ((nFATType == FAT12) || (nFATType == FAT16)))
			{
				if (!FATReadRootDirectoryEntry(j, direntry))
					return FALSE;
			}
			else
			{
				if (!FATReadDirectoryEntry(fatstruct.dwStartCluster, j, direntry))
					return FALSE;
			}

			if (memcmp(direntry, filename, 11) == 0)
			{
				fatstruct.dwStartCluster = 0;
				fatstruct.dwCurrentCluster = 0;
				memcpy(&fatstruct.dwStartCluster, direntry + 26, sizeof(WORD));
				memcpy(&fatstruct.dwCurrentCluster, direntry + 20, sizeof(WORD));
				fatstruct.dwStartCluster += (fatstruct.dwCurrentCluster * 0x10000);

				if (direntry[11] & ATTR_DIRECTORY)
				{
					fatstruct.dwSize = nSectorsPerCluster * nBytesPerSector;
					cluster = fatstruct.dwStartCluster;
					switch (nFATType)
					{
					case FAT12:
						while((cluster = FATGetFATEntry(cluster)) < 0xFF8)
							fatstruct.dwSize += nSectorsPerCluster * nBytesPerSector;
						break;
					case FAT16:
						while((cluster = FATGetFATEntry(cluster)) < 0xFFF8)
							fatstruct.dwSize += nSectorsPerCluster * nBytesPerSector;
						break;
					case FAT32:
						while((cluster = FATGetFATEntry(cluster)) < 0x0FFFFFF8)
							fatstruct.dwSize += nSectorsPerCluster * nBytesPerSector;
						break;
					}
				}

				// If we have more parts to go and this isn't a directory then fail
				if ((i < (numparts-1)) && !(direntry[11] & ATTR_DIRECTORY))
					return FALSE;

				// If this is supposed to be a file and it is a directory then fail
				if ((i == (numparts-1)) && (direntry[11] & ATTR_DIRECTORY))
					return FALSE;

				bFound = TRUE;
				break;
			}
		}
	}

	if(!bFound)
		return FALSE;

	memcpy(&pFatStruct->dwStartCluster, direntry + 26, sizeof(WORD));
	memcpy(&pFatStruct->dwCurrentCluster, direntry + 20, sizeof(WORD));
	pFatStruct->dwStartCluster += (pFatStruct->dwCurrentCluster * 0x10000);
	pFatStruct->dwCurrentCluster = pFatStruct->dwStartCluster;

	memcpy(&pFatStruct->dwSize, direntry + 28, sizeof(DWORD));
	pFatStruct->dwCurrentReadOffset = 0;

	return TRUE;
}

/*
 * FATGetNumPathParts()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and returns the number of parts it has (i.e. 3 - dir1,dir2,file1.ext)
 */
int FATGetNumPathParts(char *name)
{
	int	i, num;

	for(i=0,num=0; i<(int)strlen(name); i++)
	{
		if(name[i] == '\\')
			num++;
	}
	num++;

	return num;
}

/*
 * FATGetFirstNameFromPath()
 * This function parses a path in the form of dir1\dir2\file1.ext
 * and puts the first name of the path (e.g. "dir1") in buffer
 * compatible with the MSDOS directory structure
 */
BOOL FATGetFirstNameFromPath(char *buffer, char *name)
{
	int		i;
	char	temp[260];

	// Copy all the characters up to the end of the
	// string or until we hit a '\' character
	// and put them in temp
	for(i=0; i<(int)strlen(name); i++)
	{
		if(name[i] == '\\')
			break;
		else
			temp[i] = name[i];
	}
	temp[i] = 0;

	// If the filename is too long then fail
	if(strlen(temp) > 12)
		return FALSE;

	FATParseFileName(buffer, temp);

	return TRUE;
}

/*
 * FATParseFileName()
 * This function parses "name" which is in the form of file.ext
 * and puts it in "buffer" in the form of "FILE   EXT" which
 * is compatible with the MSDOS directory structure
 */
void FATParseFileName(char *buffer, char *name)
{
	int i, j;

	i = 0;
	j = 0;

	while(i < 8)
		buffer[i++] = (name[j] && (name[j] != '.')) ? toupper(name[j++]) : ' ';

	if(name[j] == '.')
		j++;

	while(i < 11)
		buffer[i++] = name[j] ? toupper(name[j++]) : ' ';

	buffer[i] = 0;
}

/*
 * FATGetFATEntry()
 * returns the FAT entry for a given cluster number
 */
DWORD FATGetFATEntry(DWORD nCluster)
{
	DWORD	fat;
	int		FATOffset;
	int		ThisFATSecNum;
	int		ThisFATEntOffset;
	int		Idx;
	BOOL	bEntryFound;

	switch(nFATType)
	{
	case FAT12:
		FATOffset = nCluster + (nCluster / 2);
		ThisFATSecNum = (FATOffset / nBytesPerSector);
		ThisFATEntOffset = (FATOffset % nBytesPerSector);
		fat = *((WORD *) (pFileSysData + (ThisFATSecNum * nBytesPerSector) + ThisFATEntOffset));
		if (nCluster & 0x0001) 
			fat = fat >> 4;	/* Cluster number is ODD */
		else
			fat = fat & 0x0FFF;	/* Cluster number is EVEN */

		return fat;
		break;
	case FAT16:
		FATOffset = (nCluster * 2);
		//ThisFATSecNum = nReservedSectors + (FATOffset / nBytesPerSector);
		//ThisFATEntOffset = (FATOffset % nBytesPerSector);

		//if (!ReadOneSector(ThisFATSecNum))
		//	return NULL;

		//fat = *((WORD *) &SectorBuffer[ThisFATEntOffset]);
		fat = *((WORD *) (pFileSysData + FATOffset));

		return fat;
		break;
	case FAT32:
		//if (!ReadOneSector(ThisFATSecNum))
		//	return NULL;

		//fat = *((DWORD *) &SectorBuffer[ThisFATEntOffset]) & 0x0FFFFFFF;
		//return fat;

		// This code manages the fat32 fat table entry cache
		// The cache is at address FILESYSADDR which is 128k in size
		// The first two sectors will contain an array of DWORDs that
		// Specify what fat sector is cached. The first two DWORDs
		// should be zero.
		FATOffset = (nCluster * 4);
		ThisFATSecNum = nReservedSectors + (FATOffset / nBytesPerSector);
		ThisFATEntOffset = (FATOffset % nBytesPerSector);

		// Now we go through our cache and see if we already have the sector cached
		bEntryFound = FALSE;
		for (Idx=2; Idx<256; Idx++)
		{
			if (((int*)pFat32FATCacheIndex)[Idx] == ThisFATSecNum)
			{
				bEntryFound = TRUE;
				break;
			}
		}

		if (bEntryFound)
		{
			// Get the fat entry
			fat = (*((DWORD *) (pFat32FATCacheIndex +
				(Idx * nBytesPerSector) + ThisFATEntOffset))) & 0x0FFFFFFF;
		}
		else
		{
			if (!ReadOneSector(ThisFATSecNum))
				return NULL;

			// Move each sector down in the cache to make room for new sector
			for (Idx=255; Idx>2; Idx--)
			{
				memcpy(pFat32FATCacheIndex + (Idx * nBytesPerSector), pFat32FATCacheIndex + ((Idx - 1) * nBytesPerSector), nBytesPerSector);
				((int*)pFat32FATCacheIndex)[Idx] = ((int*)pFat32FATCacheIndex)[Idx - 1];
			}

			// Insert it into the cache
			memcpy(pFat32FATCacheIndex + (2 * nBytesPerSector), SectorBuffer, nBytesPerSector);
			((int*)pFat32FATCacheIndex)[2] = ThisFATSecNum;

			// Get the fat entry
			fat = (*((DWORD *) (pFat32FATCacheIndex +
				(2 * nBytesPerSector) + ThisFATEntOffset))) & 0x0FFFFFFF;
		}

		return fat;
		break;
	}

	return NULL;
}

/*
 * FATOpenFile()
 * Tries to open the file 'name' and returns true or false
 * for success and failure respectively
 */
BOOL FATOpenFile(char *szFileName, PFAT_STRUCT pFatStruct)
{
	if(!FATLookupFile(szFileName, pFatStruct))
		return FALSE;

	/* Fill in file control information */
	pFatStruct->dwCurrentCluster = pFatStruct->dwStartCluster;
	pFatStruct->dwCurrentReadOffset = 0;

	return TRUE;
}

/*
 * FATReadCluster()
 * Reads the specified cluster into memory
 * and returns the number of bytes read
 */
int FATReadCluster(DWORD nCluster, char *cBuffer)
{
	int		nStartSector;

	nStartSector = ((nCluster - 2) * nSectorsPerCluster) + nDataSectorStart;

	ReadMultipleSectors(nStartSector, nSectorsPerCluster, cBuffer);

	return (nSectorsPerCluster * nBytesPerSector);
}

/*
 * FATRead()
 * Reads nNumBytes from open file and
 * returns the number of bytes read
 */
int FATRead(PFAT_STRUCT pFatStruct, int nNumBytes, char *cBuffer)
{
	int		nSectorWithinCluster;
	int		nOffsetWithinSector;
	int		nOffsetWithinCluster;
	int		nNum;
	int		nBytesRead = 0;

	// If all the data is read return zero
	if (pFatStruct->dwCurrentReadOffset >= pFatStruct->dwSize)
		return 0;

	// If they are trying to read more than there is to read
	// then adjust the amount to read
	if ((pFatStruct->dwCurrentReadOffset + nNumBytes) > pFatStruct->dwSize)
		nNumBytes = pFatStruct->dwSize - pFatStruct->dwCurrentReadOffset;

	while (nNumBytes)
	{
		// Check and see if the read offset is aligned to a cluster boundary
		// if so great, if not then read the rest of the current cluster
		if ((pFatStruct->dwCurrentReadOffset % (nSectorsPerCluster * nBytesPerSector)) != 0)
		{
			nSectorWithinCluster = ((pFatStruct->dwCurrentReadOffset / nBytesPerSector) % nSectorsPerCluster);
			nOffsetWithinSector = (pFatStruct->dwCurrentReadOffset % nBytesPerSector);
			nOffsetWithinCluster = (pFatStruct->dwCurrentReadOffset % (nSectorsPerCluster * nBytesPerSector));

			// Read the cluster into the scratch area
			FATReadCluster(pFatStruct->dwCurrentCluster, (char *)FATCLUSTERBUF);

			nNum = (nSectorsPerCluster * nBytesPerSector) - (pFatStruct->dwCurrentReadOffset % (nSectorsPerCluster * nBytesPerSector));
			if (nNumBytes >= nNum)
			{
				memcpy(cBuffer, (char *)(FATCLUSTERBUF + nOffsetWithinCluster), nNum);
				nBytesRead += nNum;
				cBuffer += nNum;
				pFatStruct->dwCurrentReadOffset += nNum;
				pFatStruct->dwCurrentCluster = FATGetFATEntry(pFatStruct->dwCurrentCluster);
				nNumBytes -= nNum;
			}
			else
			{
				memcpy(cBuffer, (char *)(FATCLUSTERBUF + nOffsetWithinCluster), nNumBytes);
				nBytesRead += nNumBytes;
				cBuffer += nNumBytes;
				pFatStruct->dwCurrentReadOffset += nNumBytes;
				nNumBytes -= nNumBytes;
			}
		}
		else
		{
			// Read the cluster into the scratch area
			FATReadCluster(pFatStruct->dwCurrentCluster, (char *)FATCLUSTERBUF);

			nNum = (nSectorsPerCluster * nBytesPerSector);
			if (nNumBytes >= nNum)
			{
				memcpy(cBuffer, (char *)(FATCLUSTERBUF), nNum);
				nBytesRead += nNum;
				cBuffer += nNum;
				pFatStruct->dwCurrentReadOffset += nNum;
				pFatStruct->dwCurrentCluster = FATGetFATEntry(pFatStruct->dwCurrentCluster);
				nNumBytes -= nNum;
			}
			else
			{
				memcpy(cBuffer, (char *)(FATCLUSTERBUF), nNumBytes);
				nBytesRead += nNumBytes;
				cBuffer += nNumBytes;
				pFatStruct->dwCurrentReadOffset += nNumBytes;
				nNumBytes -= nNumBytes;
			}
		}
	}

	return nBytesRead;
}

int FATfseek(PFAT_STRUCT pFatStruct, DWORD offset)
{
	DWORD		cluster;
	int			numclusters;

	numclusters = offset / (nSectorsPerCluster * nBytesPerSector);
	for (cluster=pFatStruct->dwStartCluster; numclusters > 0; numclusters--)
		cluster = FATGetFATEntry(cluster);

	pFatStruct->dwCurrentCluster = cluster;
	pFatStruct->dwCurrentReadOffset = offset;

	return 0;
}
