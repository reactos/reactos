/*
 *  FreeLoader NTFS support
 *  Copyright (C) 2004       Filip Navara  <xnavara@volny.cz>
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

#ifndef __NTFS_H
#define __NTFS_H

#define NTFS_FILE_MFT				0
#define NTFS_FILE_MFTMIRR			1
#define NTFS_FILE_LOGFILE			2
#define NTFS_FILE_VOLUME			3
#define NTFS_FILE_ATTRDEF			4
#define NTFS_FILE_ROOT				5
#define NTFS_FILE_BITMAP			6
#define NTFS_FILE_BOOT				7
#define NTFS_FILE_BADCLUS			8
#define NTFS_FILE_QUOTA				9
#define NTFS_FILE_UPCASE			10

#define NTFS_ATTR_TYPE_STANDARD_INFORMATION	0x10
#define NTFS_ATTR_TYPE_ATTRIBUTE_LIST		0x20
#define NTFS_ATTR_TYPE_FILENAME			0x30
#define NTFS_ATTR_TYPE_SECURITY_DESCRIPTOR	0x50
#define NTFS_ATTR_TYPE_DATA			0x80
#define NTFS_ATTR_TYPE_INDEX_ROOT		0x90
#define NTFS_ATTR_TYPE_INDEX_ALLOCATION		0xa0
#define NTFS_ATTR_TYPE_BITMAP			0xb0
#define NTFS_ATTR_TYPE_SYMLINK			0xc0
#define NTFS_ATTR_TYPE_END			0xffffffff

#define NTFS_ATTR_NORMAL			0
#define NTFS_ATTR_COMPRESSED			1
#define NTFS_ATTR_RESIDENT			2
#define NTFS_ATTR_ENCRYPTED			0x4000

#define NTFS_SMALL_INDEX			0
#define NTFS_LARGE_INDEX			1

#define NTFS_INDEX_ENTRY_NODE			1
#define NTFS_INDEX_ENTRY_END			2

#define NTFS_FILE_NAME_POSIX			0
#define NTFS_FILE_NAME_WIN32			1
#define NTFS_FILE_NAME_DOS			2
#define NTFS_FILE_NAME_WIN32_AND_DOS		3 

typedef struct
{
	U8		JumpBoot[3];			// Jump to the boot loader routine
	U8		SystemId[8];			// System Id ("NTFS    ")
	U16		BytesPerSector;			// Bytes per sector
	U8		SectorsPerCluster;		// Number of sectors in a cluster
	U8		Unused1[7];
	U8		MediaDescriptor;		// Media descriptor byte
	U8		Unused2[2];
	U16		SectorsPerTrack;		// Number of sectors in a track
	U16		NumberOfHeads;			// Number of heads on the disk
	U8		Unused3[8];
	U8		DriveNumber;			// Int 0x13 drive number (e.g. 0x80)
	U8		CurrentHead;
	U8		BootSignature;			// Extended boot signature (0x80)
	U8		Unused4;
	U64		VolumeSectorCount;		// Number of sectors in the volume
	U64		MftLocation;
	U64		MftMirrorLocation;
	S8		ClustersPerMftRecord;		// Clusters per MFT Record
	U8		Unused5[3];
	S8		ClustersPerIndexRecord;		// Clusters per Index Record
	U8		Unused6[3];
	U64		VolumeSerialNumber;		// Volume serial number
	U8		BootCodeAndData[430];		// The remainder of the boot sector
	U16		BootSectorMagic;		// 0xAA55
} PACKED NTFS_BOOTSECTOR, *PNTFS_BOOTSECTOR;

typedef struct
{
	U32		Magic;
	U16		USAOffset;					// Offset to the Update Sequence Array from the start of the ntfs record
	U16		USACount;
} PACKED NTFS_RECORD, *PNTFS_RECORD;

typedef struct
{
	U32		Magic;
	U16		USAOffset;					// Offset to the Update Sequence Array from the start of the ntfs record
	U16		USACount;
	U64		LogSequenceNumber;
	U16		SequenceNumber;
	U16		LinkCount;
	U16		AttributesOffset;
	U16		Flags;
	U32		BytesInUse;					// Number of bytes used in this mft record.
	U32		BytesAllocated;
	U64		BaseMFTRecord;
	U16		NextAttributeInstance;
} PACKED NTFS_MFT_RECORD, *PNTFS_MFT_RECORD;

typedef struct
{
	U32		Type;
	U32		Length;
	U8		IsNonResident;
	U8		NameLength;
	U16		NameOffset;
	U16		Flags;
	U16		Instance;
	union
	{
		// Resident attributes
		struct
		{
			U32		ValueLength;
			U16		ValueOffset;
			U16		Flags;
		} PACKED Resident;
		// Non-resident attributes
		struct
		{
			U64		LowestVCN;
			U64		HighestVCN;
			U16		MappingPairsOffset;
			U8		CompressionUnit;
			U8		Reserved[5];
			S64		AllocatedSize;
			S64		DataSize;
			S64		InitializedSize;
			S64		CompressedSize;
		} PACKED NonResident;
	} PACKED;
} PACKED NTFS_ATTR_RECORD, *PNTFS_ATTR_RECORD;

typedef struct
{
	U32		EntriesOffset;
	U32		IndexLength;
	U32		AllocatedSize;
	U8		Flags;
	U8		Reserved[3];
} PACKED NTFS_INDEX_HEADER, *PNTFS_INDEX_HEADER;

typedef struct
{
	U32		Type;
	U32		CollationRule;
	U32		IndexBlockSize;
	U8		ClustersPerIndexBlock;
	U8		Reserved[3];
	NTFS_INDEX_HEADER	IndexHeader;
} PACKED NTFS_INDEX_ROOT, *PNTFS_INDEX_ROOT;

typedef struct
{
	U64		ParentDirectory;
	S64		CreationTime;
	S64		LastDataChangeTime;
	S64		LastMftChangeTime;
	S64		LastAccessTime;
	S64		AllocatedSize;
	S64		DataSize;
	U32		FileAttributes;
	U16		PackedExtendedAttributeSize;
	U16		Reserved;
	U8		FileNameLength;
	U8		FileNameType;
	WCHAR		FileName[0];
} PACKED NTFS_FILE_NAME_ATTR, *PNTFS_FILE_NAME_ATTR;

typedef struct {
	union
	{
		struct
		{
			U64	IndexedFile;
		} PACKED Directory;
		struct
		{
			U16	DataOffset;
			U16	DataLength;
			U32	Reserved;
		} PACKED ViewIndex;
	} PACKED Data;
	U16			Length;
	U16			KeyLength;
	U16			Flags;
	U16			Reserved;
	NTFS_FILE_NAME_ATTR	FileName;
} PACKED NTFS_INDEX_ENTRY, *PNTFS_INDEX_ENTRY;

typedef struct
{
	PNTFS_ATTR_RECORD	Record;
	PUCHAR			CacheRun;
	U64			CacheRunOffset;
	S64			CacheRunStartLCN;
	U64			CacheRunLength;
	S64			CacheRunLastLCN;
	U64			CacheRunCurrentOffset;
} NTFS_ATTR_CONTEXT, *PNTFS_ATTR_CONTEXT;

typedef struct
{
	NTFS_ATTR_CONTEXT	DataContext;
	U64			Offset;
} PACKED NTFS_FILE_HANDLE, *PNTFS_FILE_HANDLE;

BOOL	NtfsOpenVolume(U32 DriveNumber, U32 VolumeStartSector);
FILE*	NtfsOpenFile(PUCHAR FileName);
BOOL	NtfsReadFile(FILE *FileHandle, U32 BytesToRead, U32* BytesRead, PVOID Buffer);
U32	NtfsGetFileSize(FILE *FileHandle);
VOID	NtfsSetFilePointer(FILE *FileHandle, U32 NewFilePointer);
U32	NtfsGetFilePointer(FILE *FileHandle);

#endif // #defined __NTFS_H
