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
	U8		JumpBoot[3];				// Jump instruction to boot code
	U8		OemName[8];					// "MSWIN4.1" for MS formatted volumes
	U16		BytesPerSector;				// Bytes per sector
	U8		SectorsPerCluster;			// Number of sectors in a cluster
	U16		ReservedSectors;			// Reserved sectors, usually 1 (the bootsector)
	U8		NumberOfFats;				// Number of FAT tables
	U16		RootDirEntries;				// Number of root directory entries (fat12/16)
	U16		TotalSectors;				// Number of total sectors on the drive, 16-bit
	U8		MediaDescriptor;			// Media descriptor byte
	U16		SectorsPerFat;				// Sectors per FAT table (fat12/16)
	U16		SectorsPerTrack;			// Number of sectors in a track
	U16		NumberOfHeads;				// Number of heads on the disk
	U32		HiddenSectors;				// Hidden sectors (sectors before the partition start like the partition table)
	U32		TotalSectorsBig;			// This field is the new 32-bit total count of sectors on the volume
	U8		DriveNumber;				// Int 0x13 drive number (e.g. 0x80)
	U8		Reserved1;					// Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
	U8		BootSignature;				// Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
	U32		VolumeSerialNumber;			// Volume serial number
	U8		VolumeLabel[11];			// Volume label. This field matches the 11-byte volume label recorded in the root directory
	U8		FileSystemType[8];			// One of the strings "FAT12   ", "FAT16   ", or "FAT     "

	U8		BootCodeAndData[448];		// The remainder of the boot sector

	U16		BootSectorMagic;			// 0xAA55
	
} PACKED FAT_BOOTSECTOR, *PFAT_BOOTSECTOR;

typedef struct _FAT32_BOOTSECTOR
{
	U8		JumpBoot[3];				// Jump instruction to boot code
	U8		OemName[8];					// "MSWIN4.1" for MS formatted volumes
	U16		BytesPerSector;				// Bytes per sector
	U8		SectorsPerCluster;			// Number of sectors in a cluster
	U16		ReservedSectors;			// Reserved sectors, usually 1 (the bootsector)
	U8		NumberOfFats;				// Number of FAT tables
	U16		RootDirEntries;				// Number of root directory entries (fat12/16)
	U16		TotalSectors;				// Number of total sectors on the drive, 16-bit
	U8		MediaDescriptor;			// Media descriptor byte
	U16		SectorsPerFat;				// Sectors per FAT table (fat12/16)
	U16		SectorsPerTrack;			// Number of sectors in a track
	U16		NumberOfHeads;				// Number of heads on the disk
	U32		HiddenSectors;				// Hidden sectors (sectors before the partition start like the partition table)
	U32		TotalSectorsBig;			// This field is the new 32-bit total count of sectors on the volume
	U32		SectorsPerFatBig;			// This field is the FAT32 32-bit count of sectors occupied by ONE FAT. BPB_FATSz16 must be 0
	U16		ExtendedFlags;				// Extended flags (fat32)
	U16		FileSystemVersion;			// File system version (fat32)
	U32		RootDirStartCluster;		// Starting cluster of the root directory (fat32)
	U16		FsInfo;						// Sector number of FSINFO structure in the reserved area of the FAT32 volume. Usually 1.
	U16		BackupBootSector;			// If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record. Usually 6.
	U8		Reserved[12];				// Reserved for future expansion
	U8		DriveNumber;				// Int 0x13 drive number (e.g. 0x80)
	U8		Reserved1;					// Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
	U8		BootSignature;				// Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
	U32		VolumeSerialNumber;			// Volume serial number
	U8		VolumeLabel[11];			// Volume label. This field matches the 11-byte volume label recorded in the root directory
	U8		FileSystemType[8];			// Always set to the string "FAT32   "

	U8		BootCodeAndData[420];		// The remainder of the boot sector

	U16		BootSectorMagic;			// 0xAA55
	
} PACKED FAT32_BOOTSECTOR, *PFAT32_BOOTSECTOR;

/*
 * Structure of MSDOS directory entry
 */
typedef struct //_DIRENTRY
{
	UCHAR	FileName[11];	/* Filename + extension */
	U8		Attr;			/* File attributes */
	U8		ReservedNT;		/* Reserved for use by Windows NT */
	U8		TimeInTenths;	/* Millisecond stamp at file creation */
	U16		CreateTime;		/* Time file was created */
	U16		CreateDate;		/* Date file was created */
	U16		LastAccessDate;	/* Date file was last accessed */
	U16		ClusterHigh;	/* High word of this entry's start cluster */
	U16		Time;			/* Time last modified */
	U16		Date;			/* Date last modified */
	U16		ClusterLow;		/* First cluster number low word */
	U32		Size;			/* File size */
} PACKED DIRENTRY, * PDIRENTRY;

typedef struct
{
	U8		SequenceNumber;		/* Sequence number for slot */
	WCHAR	Name0_4[5];			/* First 5 characters in name */
	U8		EntryAttributes;	/* Attribute byte */
	U8		Reserved;			/* Always 0 */
	U8		AliasChecksum;		/* Checksum for 8.3 alias */
	WCHAR	Name5_10[6];		/* 6 more characters in name */
	U16		StartCluster;		/* Starting cluster number */
	WCHAR	Name11_12[2];		/* Last 2 characters in name */
} PACKED LFN_DIRENTRY, * PLFN_DIRENTRY;

typedef struct
{
	U32		FileSize;			// File size
	U32		FilePointer;		// File pointer
	U32*	FileFatChain;		// File fat chain array
	U32		DriveNumber;
} FAT_FILE_INFO, * PFAT_FILE_INFO;



BOOL	FatOpenVolume(U32 DriveNumber, U32 VolumeStartSector);
U32		FatDetermineFatType(PFAT_BOOTSECTOR FatBootSector);
PVOID	FatBufferDirectory(U32 DirectoryStartCluster, U32* EntryCountPointer, BOOL RootDirectory);
BOOL	FatSearchDirectoryBufferForFile(PVOID DirectoryBuffer, U32 EntryCount, PUCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer);
BOOL	FatLookupFile(PUCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer);
void	FatParseShortFileName(PUCHAR Buffer, PDIRENTRY DirEntry);
BOOL	FatGetFatEntry(U32 Cluster, U32* ClusterPointer);
FILE*	FatOpenFile(PUCHAR FileName);
U32		FatCountClustersInChain(U32 StartCluster);
U32*	FatGetClusterChainArray(U32 StartCluster);
BOOL	FatReadCluster(U32 ClusterNumber, PVOID Buffer);
BOOL	FatReadClusterChain(U32 StartClusterNumber, U32 NumberOfClusters, PVOID Buffer);
BOOL	FatReadPartialCluster(U32 ClusterNumber, U32 StartingOffset, U32 Length, PVOID Buffer);
BOOL	FatReadFile(FILE *FileHandle, U32 BytesToRead, U32* BytesRead, PVOID Buffer);
U32		FatGetFileSize(FILE *FileHandle);
VOID	FatSetFilePointer(FILE *FileHandle, U32 NewFilePointer);
U32		FatGetFilePointer(FILE *FileHandle);
BOOL	FatReadVolumeSectors(U32 DriveNumber, U32 SectorNumber, U32 SectorCount, PVOID Buffer);


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

#endif // #defined __FAT_H
