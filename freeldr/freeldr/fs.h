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

#ifndef __FS_FAT_H
#define __FS_FAT_H

// Bootsector BPB defines
#define BPB_JMPBOOT					0
#define BPB_OEMNAME					3
#define BPB_BYTESPERSECTOR			11
#define BPB_SECTORSPERCLUSTER		13
#define BPB_RESERVEDSECTORS			14
#define BPB_NUMBEROFFATS			16
#define BPB_ROOTDIRENTRIES			17
#define BPB_TOTALSECTORS16			19
#define BPB_MEDIADESCRIPTOR			21
#define BPB_SECTORSPERFAT16			22
#define BPB_SECTORSPERTRACK			24
#define BPB_NUMBEROFHEADS			26
#define BPB_HIDDENSECTORS			28
#define BPB_TOTALSECTORS32			32

// Fat12/16 extended BPB defines
#define BPB_DRIVENUMBER16			36
#define BPB_RESERVED16_1			37
#define BPB_BOOTSIGNATURE16			38
#define BPB_VOLUMESERIAL16			39
#define BPB_VOLUMELABEL16			43
#define BPB_FILESYSTEMTYPE16		54

// Fat32 extended BPB defines
#define BPB_SECTORSPERFAT32			36
#define BPB_EXTENDEDFLAGS32			40
#define BPB_FILESYSTEMVERSION32		42
#define BPB_ROOTDIRSTARTCLUSTER32	44
#define BPB_FILESYSTEMINFOSECTOR32	48
#define BPB_BACKUPBOOTSECTOR32		50
#define BPB_RESERVED32_1			52
#define BPB_DRIVENUMBER32			64
#define BPB_RESERVED32_2			65
#define BPB_BOOTSIGNATURE32			66
#define BPB_VOLUMESERIAL32			67
#define BPB_VOLUMELABEL32			71
#define BPB_FILESYSTEMTYPE32		82

/*
 * Structure of MSDOS directory entry
 */
typedef struct //_DIRENTRY
{
	BYTE	cFileName[11];	/* Filename + extension */
	BYTE	cAttr;			/* File attributes */
	BYTE	cReserved[10];	/* Reserved area */
	WORD	wTime;			/* Time last modified */
	WORD	wData;			/* Date last modified */
	WORD	wCluster;		/* First cluster number */
	DWORD	dwSize;			/* File size */
} DIRENTRY, * PDIRENTRY;

/*
 * Structure of internal file control block
 */
typedef struct //_FCB
{
	BYTE	 	cAttr;			/* Open attributes */
	BYTE	 	cSector;		/* Sector within cluster */
	PDIRENTRY	pDirptr;		/* Pointer to directory entry */
	WORD		wDirSector;		/* Directory sector */
	WORD		wFirstCluster;	/* First cluster in file */
	WORD		wLastCluster;	/* Last cluster read/written */
	WORD		wNextCluster;	/* Next cluster to read/write */
	WORD		wOffset;		/* Read/Write offset within sector */
	DWORD		dwSize;			/* File size */
	BYTE		cBuffer[512];	/* Data transfer buffer */
} FCB, * PFCB;

typedef struct //_FAT_STRUCT
{
	DWORD	dwStartCluster;		// File's starting cluster
	DWORD	dwCurrentCluster;	// Current read cluster number
	DWORD	dwSize;				// File size
	DWORD	dwCurrentReadOffset;// Amount of data already read
} FAT_STRUCT, * PFAT_STRUCT;

typedef struct //_FILE
{
	//DIRENTRY		de;
	//FCB				fcb;
	FAT_STRUCT		fat;
	unsigned long	filesize;
} FILE;

extern	int		nSectorBuffered;	// Tells us which sector was read into SectorBuffer[]
extern	BYTE	SectorBuffer[512];	// 512 byte buffer space for read operations, ReadOneSector reads to here

extern	int		nFATType;

extern	DWORD	nBytesPerSector;		// Bytes per sector
extern	DWORD	nSectorsPerCluster;		// Number of sectors in a cluster
extern	DWORD	nReservedSectors;		// Reserved sectors, usually 1 (the bootsector)
extern	DWORD	nNumberOfFATs;			// Number of FAT tables
extern	DWORD	nRootDirEntries;		// Number of root directory entries (fat12/16)
extern	DWORD	nTotalSectors16;		// Number of total sectors on the drive, 16-bit
extern	DWORD	nSectorsPerFAT16;		// Sectors per FAT table (fat12/16)
extern	DWORD	nSectorsPerTrack;		// Number of sectors in a track
extern	DWORD	nNumberOfHeads;			// Number of heads on the disk
extern	DWORD	nHiddenSectors;			// Hidden sectors (sectors before the partition start like the partition table)
extern	DWORD	nTotalSectors32;		// Number of total sectors on the drive, 32-bit

extern	DWORD	nSectorsPerFAT32;		// Sectors per FAT table (fat32)
extern	DWORD	nExtendedFlags;			// Extended flags (fat32)
extern	DWORD	nFileSystemVersion;		// File system version (fat32)
extern	DWORD	nRootDirStartCluster;	// Starting cluster of the root directory (fat32)

extern	DWORD	nRootDirSectorStart;	// Starting sector of the root directory (fat12/16)
extern	DWORD	nDataSectorStart;		// Starting sector of the data area
extern	DWORD	nSectorsPerFAT;			// Sectors per FAT table
extern	DWORD	nRootDirSectors;		// Number of sectors of the root directory (fat32)
extern	DWORD	nTotalSectors;			// Total sectors on the drive
extern	DWORD	nNumberOfClusters;		// Number of clusters on the drive

extern	int	FSType;						// Type of filesystem on boot device, set by OpenDiskDrive()

extern	char *pFileSysData;				// Load address for filesystem data
extern	char *pFat32FATCacheIndex;		// Load address for filesystem data

BOOL	OpenDiskDrive(int nDrive, int nPartition);	// Opens the disk drive device for reading
BOOL	ReadMultipleSectors(int nSect, int nNumberOfSectors, void *pBuffer);// Reads a sector from the open device
BOOL	ReadOneSector(int nSect);				// Reads one sector from the open device
BOOL	OpenFile(char *filename, FILE *pFile);	// Opens a file
int		ReadFile(FILE *pFile, int count, void *buffer); // Reads count bytes from pFile into buffer
DWORD	GetFileSize(FILE *pFile);
DWORD	Rewind(FILE *pFile);	// Rewinds a file and returns it's size
int		feof(FILE *pFile);
int		fseek(FILE *pFILE, DWORD offset);

BOOL	FATLookupFile(char *file, PFAT_STRUCT pFatStruct);
int		FATGetNumPathParts(char *name);
BOOL	FATGetFirstNameFromPath(char *buffer, char *name);
void	FATParseFileName(char *buffer, char *name);
DWORD	FATGetFATEntry(DWORD nCluster);
BOOL	FATOpenFile(char *szFileName, PFAT_STRUCT pFatStruct);
int		FATReadCluster(DWORD nCluster, char *cBuffer);
int		FATRead(PFAT_STRUCT pFatStruct, int nNumBytes, char *cBuffer);
int		FATfseek(PFAT_STRUCT pFatStruct, DWORD offset);


#define	EOF	-1

#define	ATTR_NORMAL		0x00
#define	ATTR_READONLY	0x01
#define	ATTR_HIDDEN		0x02
#define	ATTR_SYSTEM		0x04
#define	ATTR_VOLUMENAME	0x08
#define	ATTR_DIRECTORY	0x10
#define	ATTR_ARCHIVE	0x20

#define	FS_FAT			1
#define	FS_NTFS			2
#define	FS_EXT2			3

#define	FAT12			1
#define	FAT16			2
#define	FAT32			3

#endif // #defined __FS_FAT_H