/*
 *  FreeLoader NTFS support
 *  Copyright (C) 2004  Filip Navara  <xnavara@volny.cz>
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
	UCHAR		JumpBoot[3];			// Jump to the boot loader routine
	CHAR		SystemId[8];			// System Id ("NTFS    ")
	USHORT		BytesPerSector;			// Bytes per sector
	UCHAR		SectorsPerCluster;		// Number of sectors in a cluster
	UCHAR		Unused1[7];
	UCHAR		MediaDescriptor;		// Media descriptor byte
	UCHAR		Unused2[2];
	USHORT		SectorsPerTrack;		// Number of sectors in a track
	USHORT		NumberOfHeads;			// Number of heads on the disk
	UCHAR		Unused3[8];
	UCHAR		DriveNumber;			// Int 0x13 drive number (e.g. 0x80)
	UCHAR		CurrentHead;
	UCHAR		BootSignature;			// Extended boot signature (0x80)
	UCHAR		Unused4;
	ULONGLONG		VolumeSectorCount;		// Number of sectors in the volume
	ULONGLONG		MftLocation;
	ULONGLONG		MftMirrorLocation;
	CHAR		ClustersPerMftRecord;		// Clusters per MFT Record
	UCHAR		Unused5[3];
	CHAR		ClustersPerIndexRecord;		// Clusters per Index Record
	UCHAR		Unused6[3];
	ULONGLONG		VolumeSerialNumber;		// Volume serial number
	UCHAR		BootCodeAndData[430];		// The remainder of the boot sector
	USHORT		BootSectorMagic;		// 0xAA55
} PACKED NTFS_BOOTSECTOR, *PNTFS_BOOTSECTOR;

typedef struct
{
	ULONG		Magic;
	USHORT		USAOffset;					// Offset to the Update Sequence Array from the start of the ntfs record
	USHORT		USACount;
} PACKED NTFS_RECORD, *PNTFS_RECORD;

typedef struct
{
	ULONG		Magic;
	USHORT		USAOffset;					// Offset to the Update Sequence Array from the start of the ntfs record
	USHORT		USACount;
	ULONGLONG		LogSequenceNumber;
	USHORT		SequenceNumber;
	USHORT		LinkCount;
	USHORT		AttributesOffset;
	USHORT		Flags;
	ULONG		BytesInUse;					// Number of bytes used in this mft record.
	ULONG		BytesAllocated;
	ULONGLONG		BaseMFTRecord;
	USHORT		NextAttributeInstance;
} PACKED NTFS_MFT_RECORD, *PNTFS_MFT_RECORD;

typedef struct
{
	ULONG		Type;
	ULONG		Length;
	UCHAR		IsNonResident;
	UCHAR		NameLength;
	USHORT		NameOffset;
	USHORT		Flags;
	USHORT		Instance;
	union
	{
		// Resident attributes
		struct
		{
			ULONG		ValueLength;
			USHORT		ValueOffset;
			USHORT		Flags;
		} PACKED Resident;
		// Non-resident attributes
		struct
		{
			ULONGLONG		LowestVCN;
			ULONGLONG		HighestVCN;
			USHORT		MappingPairsOffset;
			UCHAR		CompressionUnit;
			UCHAR		Reserved[5];
			LONGLONG		AllocatedSize;
			LONGLONG		DataSize;
			LONGLONG		InitializedSize;
			LONGLONG		CompressedSize;
		} PACKED NonResident;
	} PACKED;
} PACKED NTFS_ATTR_RECORD, *PNTFS_ATTR_RECORD;

typedef struct
{
	ULONG		EntriesOffset;
	ULONG		IndexLength;
	ULONG		AllocatedSize;
	UCHAR		Flags;
	UCHAR		Reserved[3];
} PACKED NTFS_INDEX_HEADER, *PNTFS_INDEX_HEADER;

typedef struct
{
	ULONG		Type;
	ULONG		CollationRule;
	ULONG		IndexBlockSize;
	UCHAR		ClustersPerIndexBlock;
	UCHAR		Reserved[3];
	NTFS_INDEX_HEADER	IndexHeader;
} PACKED NTFS_INDEX_ROOT, *PNTFS_INDEX_ROOT;

typedef struct
{
	ULONGLONG		ParentDirectory;
	LONGLONG		CreationTime;
	LONGLONG		LastDataChangeTime;
	LONGLONG		LastMftChangeTime;
	LONGLONG		LastAccessTime;
	LONGLONG		AllocatedSize;
	LONGLONG		DataSize;
	ULONG		FileAttributes;
	USHORT		PackedExtendedAttributeSize;
	USHORT		Reserved;
	UCHAR		FileNameLength;
	UCHAR		FileNameType;
	WCHAR		FileName[0];
} PACKED NTFS_FILE_NAME_ATTR, *PNTFS_FILE_NAME_ATTR;

typedef struct {
	union
	{
		struct
		{
			ULONGLONG	IndexedFile;
		} PACKED Directory;
		struct
		{
			USHORT	DataOffset;
			USHORT	DataLength;
			ULONG	Reserved;
		} PACKED ViewIndex;
	} PACKED Data;
	USHORT			Length;
	USHORT			KeyLength;
	USHORT			Flags;
	USHORT			Reserved;
	NTFS_FILE_NAME_ATTR	FileName;
} PACKED NTFS_INDEX_ENTRY, *PNTFS_INDEX_ENTRY;

typedef struct
{
	PUCHAR			CacheRun;
	ULONGLONG			CacheRunOffset;
	LONGLONG			CacheRunStartLCN;
	ULONGLONG			CacheRunLength;
	LONGLONG			CacheRunLastLCN;
	ULONGLONG			CacheRunCurrentOffset;
	NTFS_ATTR_RECORD	Record;
} NTFS_ATTR_CONTEXT, *PNTFS_ATTR_CONTEXT;

typedef struct
{
	PNTFS_ATTR_CONTEXT	DataContext;
	ULONGLONG			Offset;
} PACKED NTFS_FILE_HANDLE, *PNTFS_FILE_HANDLE;

BOOLEAN	NtfsOpenVolume(ULONG DriveNumber, ULONG VolumeStartSector);
FILE*	NtfsOpenFile(PCSTR FileName);
VOID	NtfsCloseFile(FILE *FileHandle);
BOOLEAN	NtfsReadFile(FILE *FileHandle, ULONG BytesToRead, ULONG* BytesRead, PVOID Buffer);
ULONG	NtfsGetFileSize(FILE *FileHandle);
VOID	NtfsSetFilePointer(FILE *FileHandle, ULONG NewFilePointer);
ULONG	NtfsGetFilePointer(FILE *FileHandle);

#endif // #defined __NTFS_H
