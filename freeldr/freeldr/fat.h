/*
 *  FreeLoader
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

#ifndef __FAT_H
#define __FAT_H

typedef struct _FAT_BOOTSECTOR
{
	BYTE	JumpBoot[3];				// Jump instruction to boot code
	UCHAR	OemName[8];					// "MSWIN4.1" for MS formatted volumes
	WORD	BytesPerSector;				// Bytes per sector
	BYTE	SectorsPerCluster;			// Number of sectors in a cluster
	WORD	ReservedSectors;			// Reserved sectors, usually 1 (the bootsector)
	BYTE	NumberOfFats;				// Number of FAT tables
	WORD	RootDirEntries;				// Number of root directory entries (fat12/16)
	WORD	TotalSectors;				// Number of total sectors on the drive, 16-bit
	BYTE	MediaDescriptor;			// Media descriptor byte
	WORD	SectorsPerFat;				// Sectors per FAT table (fat12/16)
	WORD	SectorsPerTrack;			// Number of sectors in a track
	WORD	NumberOfHeads;				// Number of heads on the disk
	DWORD	HiddenSectors;				// Hidden sectors (sectors before the partition start like the partition table)
	DWORD	TotalSectorsBig;			// This field is the new 32-bit total count of sectors on the volume
	BYTE	DriveNumber;				// Int 0x13 drive number (e.g. 0x80)
	BYTE	Reserved1;					// Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
	BYTE	BootSignature;				// Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
	DWORD	VolumeSerialNumber;			// Volume serial number
	UCHAR	VolumeLabel[11];			// Volume label. This field matches the 11-byte volume label recorded in the root directory
	UCHAR	FileSystemType[8];			// One of the strings "FAT12   ", "FAT16   ", or "FAT     "

	BYTE	BootCodeAndData[448];		// The remainder of the boot sector

	WORD	BootSectorMagic;			// 0xAA55
	
} PACKED FAT_BOOTSECTOR, *PFAT_BOOTSECTOR;

typedef struct _FAT32_BOOTSECTOR
{
	BYTE	JumpBoot[3];				// Jump instruction to boot code
	UCHAR	OemName[8];					// "MSWIN4.1" for MS formatted volumes
	WORD	BytesPerSector;				// Bytes per sector
	BYTE	SectorsPerCluster;			// Number of sectors in a cluster
	WORD	ReservedSectors;			// Reserved sectors, usually 1 (the bootsector)
	BYTE	NumberOfFats;				// Number of FAT tables
	WORD	RootDirEntries;				// Number of root directory entries (fat12/16)
	WORD	TotalSectors;				// Number of total sectors on the drive, 16-bit
	BYTE	MediaDescriptor;			// Media descriptor byte
	WORD	SectorsPerFat;				// Sectors per FAT table (fat12/16)
	WORD	SectorsPerTrack;			// Number of sectors in a track
	WORD	NumberOfHeads;				// Number of heads on the disk
	DWORD	HiddenSectors;				// Hidden sectors (sectors before the partition start like the partition table)
	DWORD	TotalSectorsBig;			// This field is the new 32-bit total count of sectors on the volume
	DWORD	SectorsPerFatBig;			// This field is the FAT32 32-bit count of sectors occupied by ONE FAT. BPB_FATSz16 must be 0
	WORD	ExtendedFlags;				// Extended flags (fat32)
	WORD	FileSystemVersion;			// File system version (fat32)
	DWORD	RootDirStartCluster;		// Starting cluster of the root directory (fat32)
	WORD	FsInfo;						// Sector number of FSINFO structure in the reserved area of the FAT32 volume. Usually 1.
	WORD	BackupBootSector;			// If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record. Usually 6.
	BYTE	Reserved[12];				// Reserved for future expansion
	BYTE	DriveNumber;				// Int 0x13 drive number (e.g. 0x80)
	BYTE	Reserved1;					// Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
	BYTE	BootSignature;				// Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
	DWORD	VolumeSerialNumber;			// Volume serial number
	UCHAR	VolumeLabel[11];			// Volume label. This field matches the 11-byte volume label recorded in the root directory
	UCHAR	FileSystemType[8];			// Always set to the string "FAT32   "

	BYTE	BootCodeAndData[420];		// The remainder of the boot sector

	WORD	BootSectorMagic;			// 0xAA55
	
} PACKED FAT32_BOOTSECTOR, *PFAT32_BOOTSECTOR;

/*
 * Structure of MSDOS directory entry
 */
typedef struct //_DIRENTRY
{
	UCHAR	FileName[11];	/* Filename + extension */
	UINT8	Attr;			/* File attributes */
	UINT8	ReservedNT;		/* Reserved for use by Windows NT */
	UINT8	TimeInTenths;	/* Millisecond stamp at file creation */
	UINT16	CreateTime;		/* Time file was created */
	UINT16	CreateDate;		/* Date file was created */
	UINT16	LastAccessDate;	/* Date file was last accessed */
	UINT16	ClusterHigh;	/* High word of this entry's start cluster */
	UINT16	Time;			/* Time last modified */
	UINT16	Date;			/* Date last modified */
	UINT16	ClusterLow;		/* First cluster number low word */
	UINT32	Size;			/* File size */
} PACKED DIRENTRY, * PDIRENTRY;

typedef struct
{
	UINT8	SequenceNumber;		/* Sequence number for slot */
	WCHAR	Name0_4[5];			/* First 5 characters in name */
	UINT8	EntryAttributes;	/* Attribute byte */
	UINT8	Reserved;			/* Always 0 */
	UINT8	AliasChecksum;		/* Checksum for 8.3 alias */
	WCHAR	Name5_10[6];		/* 6 more characters in name */
	UINT16	StartCluster;		/* Starting cluster number */
	WCHAR	Name11_12[2];		/* Last 2 characters in name */
} PACKED LFN_DIRENTRY, * PLFN_DIRENTRY;

typedef struct
{
	ULONG	FileSize;			// File size
	ULONG	FilePointer;		// File pointer
	PUINT32	FileFatChain;		// File fat chain array
} FAT_FILE_INFO, * PFAT_FILE_INFO;



BOOL	FatOpenVolume(ULONG DriveNumber, ULONG VolumeStartHead, ULONG VolumeStartTrack, ULONG VolumeStartSector, ULONG FatFileSystemType);
PVOID	FatBufferDirectory(UINT32 DirectoryStartCluster, PUINT32 EntryCountPointer, BOOL RootDirectory);
BOOL	FatSearchDirectoryBufferForFile(PVOID DirectoryBuffer, UINT32 EntryCount, PUCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer);
BOOL	FatLookupFile(PUCHAR FileName, PFAT_FILE_INFO FatFileInfoPointer);
ULONG	FatGetNumPathParts(PUCHAR Path);
VOID	FatGetFirstNameFromPath(PUCHAR Buffer, PUCHAR Path);
void	FatParseShortFileName(PUCHAR Buffer, PDIRENTRY DirEntry);
DWORD	FatGetFatEntry(DWORD nCluster);
FILE*	FatOpenFile(PUCHAR FileName);
UINT32	FatCountClustersInChain(UINT32 StartCluster);
PUINT32	FatGetClusterChainArray(UINT32 StartCluster);
BOOL	FatReadCluster(ULONG ClusterNumber, PVOID Buffer);
BOOL	FatReadClusterChain(ULONG StartClusterNumber, ULONG NumberOfClusters, PVOID Buffer);
BOOL	FatReadPartialCluster(ULONG ClusterNumber, ULONG StartingOffset, ULONG Length, PVOID Buffer);
BOOL	FatReadFile(FILE *FileHandle, ULONG BytesToRead, PULONG BytesRead, PVOID Buffer);
ULONG	FatGetFileSize(FILE *FileHandle);
VOID	FatSetFilePointer(FILE *FileHandle, ULONG NewFilePointer);
ULONG	FatGetFilePointer(FILE *FileHandle);


/*BOOL	FatLookupFile(char *file, PFAT_STRUCT pFatStruct);
int		FatGetNumPathParts(char *name);
BOOL	FatGetFirstNameFromPath(char *buffer, char *name);
void	FatParseFileName(char *buffer, char *name);
DWORD	FatGetFatEntry(DWORD nCluster);
int		FatReadCluster(DWORD nCluster, char *cBuffer);
int		FatRead(PFAT_STRUCT pFatStruct, int nNumBytes, char *cBuffer);
int		Fatfseek(PFAT_STRUCT pFatStruct, DWORD offset);

FILE*	FatOpenFile(PUCHAR FileName);
BOOL	FatReadFile(FILE *FileHandle, ULONG BytesToRead, PULONG BytesRead, PVOID Buffer);
ULONG	FatGetFileSize(FILE *FileHandle);
VOID	FatSetFilePointer(FILE *FileHandle, ULONG NewFilePointer);
ULONG	FatGetFilePointer(FILE *FileHandle);*/


#define	EOF	-1

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