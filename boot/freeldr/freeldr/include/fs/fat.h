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

#ifndef __FAT_H
#define __FAT_H

typedef struct _FAT_BOOTSECTOR
{
	UCHAR		JumpBoot[3];				// Jump instruction to boot code
	CHAR		OemName[8];					// "MSWIN4.1" for MS formatted volumes
	USHORT		BytesPerSector;				// Bytes per sector
	UCHAR		SectorsPerCluster;			// Number of sectors in a cluster
	USHORT		ReservedSectors;			// Reserved sectors, usually 1 (the bootsector)
	UCHAR		NumberOfFats;				// Number of FAT tables
	USHORT		RootDirEntries;				// Number of root directory entries (fat12/16)
	USHORT		TotalSectors;				// Number of total sectors on the drive, 16-bit
	UCHAR		MediaDescriptor;			// Media descriptor byte
	USHORT		SectorsPerFat;				// Sectors per FAT table (fat12/16)
	USHORT		SectorsPerTrack;			// Number of sectors in a track
	USHORT		NumberOfHeads;				// Number of heads on the disk
	ULONG		HiddenSectors;				// Hidden sectors (sectors before the partition start like the partition table)
	ULONG		TotalSectorsBig;			// This field is the new 32-bit total count of sectors on the volume
	UCHAR		DriveNumber;				// Int 0x13 drive number (e.g. 0x80)
	UCHAR		Reserved1;					// Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
	UCHAR		BootSignature;				// Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
	ULONG		VolumeSerialNumber;			// Volume serial number
	CHAR		VolumeLabel[11];			// Volume label. This field matches the 11-byte volume label recorded in the root directory
	CHAR		FileSystemType[8];			// One of the strings "FAT12   ", "FAT16   ", or "FAT     "

	UCHAR		BootCodeAndData[448];		// The remainder of the boot sector

	USHORT		BootSectorMagic;			// 0xAA55

} PACKED FAT_BOOTSECTOR, *PFAT_BOOTSECTOR;

typedef struct _FAT32_BOOTSECTOR
{
	UCHAR		JumpBoot[3];				// Jump instruction to boot code
	CHAR		OemName[8];					// "MSWIN4.1" for MS formatted volumes
	USHORT		BytesPerSector;				// Bytes per sector
	UCHAR		SectorsPerCluster;			// Number of sectors in a cluster
	USHORT		ReservedSectors;			// Reserved sectors, usually 1 (the bootsector)
	UCHAR		NumberOfFats;				// Number of FAT tables
	USHORT		RootDirEntries;				// Number of root directory entries (fat12/16)
	USHORT		TotalSectors;				// Number of total sectors on the drive, 16-bit
	UCHAR		MediaDescriptor;			// Media descriptor byte
	USHORT		SectorsPerFat;				// Sectors per FAT table (fat12/16)
	USHORT		SectorsPerTrack;			// Number of sectors in a track
	USHORT		NumberOfHeads;				// Number of heads on the disk
	ULONG		HiddenSectors;				// Hidden sectors (sectors before the partition start like the partition table)
	ULONG		TotalSectorsBig;			// This field is the new 32-bit total count of sectors on the volume
	ULONG		SectorsPerFatBig;			// This field is the FAT32 32-bit count of sectors occupied by ONE FAT. BPB_FATSz16 must be 0
	USHORT		ExtendedFlags;				// Extended flags (fat32)
	USHORT		FileSystemVersion;			// File system version (fat32)
	ULONG		RootDirStartCluster;		// Starting cluster of the root directory (fat32)
	USHORT		FsInfo;						// Sector number of FSINFO structure in the reserved area of the FAT32 volume. Usually 1.
	USHORT		BackupBootSector;			// If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record. Usually 6.
	UCHAR		Reserved[12];				// Reserved for future expansion
	UCHAR		DriveNumber;				// Int 0x13 drive number (e.g. 0x80)
	UCHAR		Reserved1;					// Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
	UCHAR		BootSignature;				// Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
	ULONG		VolumeSerialNumber;			// Volume serial number
	CHAR		VolumeLabel[11];			// Volume label. This field matches the 11-byte volume label recorded in the root directory
	CHAR		FileSystemType[8];			// Always set to the string "FAT32   "

	UCHAR		BootCodeAndData[420];		// The remainder of the boot sector

	USHORT		BootSectorMagic;			// 0xAA55

} PACKED FAT32_BOOTSECTOR, *PFAT32_BOOTSECTOR;

typedef struct _FATX_BOOTSECTOR
{
	CHAR		FileSystemType[4];			/* String "FATX" */
	ULONG		VolumeSerialNumber;			/* Volume serial number */
	ULONG		SectorsPerCluster;			/* Number of sectors in a cluster */
	USHORT		NumberOfFats;				/* Number of FAT tables */
	ULONG		Unknown;				/* Always 0? */
	UCHAR		Unused[494];				/* Actually size should be 4078 (boot block is 4096 bytes) */

} PACKED FATX_BOOTSECTOR, *PFATX_BOOTSECTOR;

/*
 * Structure of MSDOS directory entry
 */
typedef struct //_DIRENTRY
{
	CHAR	FileName[11];	/* Filename + extension */
	UCHAR	Attr;		/* File attributes */
	UCHAR	ReservedNT;	/* Reserved for use by Windows NT */
	UCHAR	TimeInTenths;	/* Millisecond stamp at file creation */
	USHORT	CreateTime;	/* Time file was created */
	USHORT	CreateDate;	/* Date file was created */
	USHORT	LastAccessDate;	/* Date file was last accessed */
	USHORT	ClusterHigh;	/* High word of this entry's start cluster */
	USHORT	Time;		/* Time last modified */
	USHORT	Date;		/* Date last modified */
	USHORT	ClusterLow;	/* First cluster number low word */
	ULONG	Size;		/* File size */
} PACKED DIRENTRY, * PDIRENTRY;

typedef struct
{
	UCHAR	SequenceNumber;		/* Sequence number for slot */
	WCHAR	Name0_4[5];		/* First 5 characters in name */
	UCHAR	EntryAttributes;	/* Attribute byte */
	UCHAR	Reserved;		/* Always 0 */
	UCHAR	AliasChecksum;		/* Checksum for 8.3 alias */
	WCHAR	Name5_10[6];		/* 6 more characters in name */
	USHORT	StartCluster;		/* Starting cluster number */
	WCHAR	Name11_12[2];		/* Last 2 characters in name */
} PACKED LFN_DIRENTRY, * PLFN_DIRENTRY;

typedef struct
{
	UCHAR	FileNameSize;	/* Size of filename (max 42) */
	UCHAR	Attr;		/* File attributes */
	CHAR	FileName[42];	/* Filename in ASCII, padded with 0xff (not zero-terminated) */
	ULONG	StartCluster;	/* Starting cluster number */
	ULONG	Size;		/* File size */
	USHORT	Time;		/* Time last modified */
	USHORT	Date;		/* Date last modified */
	USHORT	CreateTime;	/* Time file was created */
	USHORT	CreateDate;	/* Date file was created */
	USHORT	LastAccessTime;	/* Time file was last accessed */
	USHORT	LastAccessDate;	/* Date file was last accessed */
} PACKED FATX_DIRENTRY, * PFATX_DIRENTRY;

typedef struct
{
	ULONG	FileSize;		/* File size */
	ULONG	FilePointer;		/* File pointer */
	ULONG*	FileFatChain;		/* File fat chain array */
	ULONG	DriveNumber;
} FAT_FILE_INFO, * PFAT_FILE_INFO;



BOOLEAN	FatOpenVolume(ULONG DriveNumber, ULONG VolumeStartSector, ULONG PartitionSectorCount);
ULONG	FatDetermineFatType(PFAT_BOOTSECTOR FatBootSector, ULONG PartitionSectorCount);
PVOID	FatBufferDirectory(ULONG DirectoryStartCluster, ULONG* EntryCountPointer, BOOLEAN RootDirectory);
BOOLEAN	FatSearchDirectoryBufferForFile(PVOID DirectoryBuffer, ULONG EntryCount, PCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer);
BOOLEAN	FatLookupFile(PCSTR FileName, PFAT_FILE_INFO FatFileInfoPointer);
void	FatParseShortFileName(PCHAR Buffer, PDIRENTRY DirEntry);
BOOLEAN	FatGetFatEntry(ULONG Cluster, ULONG* ClusterPointer);
FILE*	FatOpenFile(PCSTR FileName);
ULONG	FatCountClustersInChain(ULONG StartCluster);
ULONG*	FatGetClusterChainArray(ULONG StartCluster);
BOOLEAN	FatReadCluster(ULONG ClusterNumber, PVOID Buffer);
BOOLEAN	FatReadClusterChain(ULONG StartClusterNumber, ULONG NumberOfClusters, PVOID Buffer);
BOOLEAN	FatReadPartialCluster(ULONG ClusterNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer);
BOOLEAN	FatReadFile(FILE *FileHandle, ULONG BytesToRead, ULONG* BytesRead, PVOID Buffer);
ULONG		FatGetFileSize(FILE *FileHandle);
VOID	FatSetFilePointer(FILE *FileHandle, ULONG NewFilePointer);
ULONG		FatGetFilePointer(FILE *FileHandle);
BOOLEAN	FatReadVolumeSectors(ULONG DriveNumber, ULONG SectorNumber, ULONG SectorCount, PVOID Buffer);


#define	ATTR_NORMAL		0x00
#define	ATTR_READONLY	0x01
#define	ATTR_HIDDEN		0x02
#define	ATTR_SYSTEM		0x04
#define	ATTR_VOLUMENAME	0x08
#define	ATTR_DIRECTORY	0x10
#define	ATTR_ARCHIVE	0x20
#define ATTR_LONG_NAME	(ATTR_READONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUMENAME)

#define	FAT12			1
#define	FAT16			2
#define	FAT32			3
#define FATX16			4
#define FATX32			5

#define ISFATX(FT) ((FT) == FATX16 || (FT) == FATX32)

#endif // #defined __FAT_H
