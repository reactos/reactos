/*
 *  FreeLoader NTFS support
 *  Copyright (C) 2004  Filip Navara  <xnavara@volny.cz>
 *  Copyright (C) 2011  Pierre Schweitzer <pierre.schweitzer@reactos.org>
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

#pragma once

#define NTFS_FILE_MFT                0
#define NTFS_FILE_MFTMIRR            1
#define NTFS_FILE_LOGFILE            2
#define NTFS_FILE_VOLUME            3
#define NTFS_FILE_ATTRDEF            4
#define NTFS_FILE_ROOT                5
#define NTFS_FILE_BITMAP            6
#define NTFS_FILE_BOOT                7
#define NTFS_FILE_BADCLUS            8
#define NTFS_FILE_QUOTA                9
#define NTFS_FILE_UPCASE            10
#define NTFS_FILE_EXTEND            11

#define NTFS_ATTR_TYPE_STANDARD_INFORMATION    0x10
#define NTFS_ATTR_TYPE_ATTRIBUTE_LIST        0x20
#define NTFS_ATTR_TYPE_FILENAME            0x30
#define NTFS_ATTR_TYPE_OBJECT_ID        0x40
#define NTFS_ATTR_TYPE_SECURITY_DESCRIPTOR    0x50
#define NTFS_ATTR_TYPE_VOLUME_NAME        0x60
#define NTFS_ATTR_TYPE_VOLUME_INFORMATION    0x70
#define NTFS_ATTR_TYPE_DATA            0x80
#define NTFS_ATTR_TYPE_INDEX_ROOT        0x90
#define NTFS_ATTR_TYPE_INDEX_ALLOCATION        0xa0
#define NTFS_ATTR_TYPE_BITMAP            0xb0
#define NTFS_ATTR_TYPE_REPARSE_POINT    0xc0
#define NTFS_ATTR_TYPE_EA_INFORMATION    0xd0
#define NTFS_ATTR_TYPE_EA            0xe0
#define NTFS_ATTR_TYPE_END            0xffffffff

#define NTFS_ATTR_NORMAL            0
#define NTFS_ATTR_COMPRESSED            1
#define NTFS_ATTR_RESIDENT            2
#define NTFS_ATTR_ENCRYPTED            0x4000

#define NTFS_SMALL_INDEX            0
#define NTFS_LARGE_INDEX            1

#define NTFS_INDEX_ENTRY_NODE            1
#define NTFS_INDEX_ENTRY_END            2

#define NTFS_FILE_NAME_POSIX            0
#define NTFS_FILE_NAME_WIN32            1
#define NTFS_FILE_NAME_DOS            2
#define NTFS_FILE_NAME_WIN32_AND_DOS        3

#include <pshpack1.h>
typedef struct
{
    UCHAR        JumpBoot[3];            // Jump to the boot loader routine
    CHAR        SystemId[8];            // System Id ("NTFS    ")
    USHORT        BytesPerSector;            // Bytes per sector
    UCHAR        SectorsPerCluster;        // Number of sectors in a cluster
    UCHAR        Unused1[7];
    UCHAR        MediaDescriptor;        // Media descriptor byte
    UCHAR        Unused2[2];
    USHORT        SectorsPerTrack;        // Number of sectors in a track
    USHORT        NumberOfHeads;            // Number of heads on the disk
    UCHAR        Unused3[8];
    UCHAR        DriveNumber;            // Int 0x13 drive number (e.g. 0x80)
    UCHAR        CurrentHead;
    UCHAR        BootSignature;            // Extended boot signature (0x80)
    UCHAR        Unused4;
    ULONGLONG        VolumeSectorCount;        // Number of sectors in the volume
    ULONGLONG        MftLocation;
    ULONGLONG        MftMirrorLocation;
    CHAR        ClustersPerMftRecord;        // Clusters per MFT Record
    UCHAR        Unused5[3];
    CHAR        ClustersPerIndexRecord;        // Clusters per Index Record
    UCHAR        Unused6[3];
    ULONGLONG        VolumeSerialNumber;        // Volume serial number
    UCHAR        BootCodeAndData[430];        // The remainder of the boot sector
    USHORT        BootSectorMagic;        // 0xAA55
} NTFS_BOOTSECTOR, *PNTFS_BOOTSECTOR;

typedef struct
{
    ULONG        Magic;
    USHORT        USAOffset;                    // Offset to the Update Sequence Array from the start of the ntfs record
    USHORT        USACount;
} NTFS_RECORD, *PNTFS_RECORD;

typedef struct
{
    ULONG        Magic;
    USHORT        USAOffset;                    // Offset to the Update Sequence Array from the start of the ntfs record
    USHORT        USACount;
    ULONGLONG        LogSequenceNumber;
    USHORT        SequenceNumber;
    USHORT        LinkCount;
    USHORT        AttributesOffset;
    USHORT        Flags;
    ULONG        BytesInUse;                    // Number of bytes used in this mft record.
    ULONG        BytesAllocated;
    ULONGLONG        BaseMFTRecord;
    USHORT        NextAttributeInstance;
} NTFS_MFT_RECORD, *PNTFS_MFT_RECORD;

typedef struct
{
    ULONG        Type;
    ULONG        Length;
    UCHAR        IsNonResident;
    UCHAR        NameLength;
    USHORT        NameOffset;
    USHORT        Flags;
    USHORT        Instance;
    union
    {
        // Resident attributes
        struct
        {
            ULONG        ValueLength;
            USHORT        ValueOffset;
            UCHAR        Flags;
            UCHAR        Reserved;
        } Resident;
        // Non-resident attributes
        struct
        {
            ULONGLONG        LowestVCN;
            ULONGLONG        HighestVCN;
            USHORT        MappingPairsOffset;
            USHORT        CompressionUnit;
            UCHAR        Reserved[4];
            LONGLONG        AllocatedSize;
            LONGLONG        DataSize;
            LONGLONG        InitializedSize;
            LONGLONG        CompressedSize;
        } NonResident;
    };
} NTFS_ATTR_RECORD, *PNTFS_ATTR_RECORD;

typedef struct
{
    ULONG        EntriesOffset;
    ULONG        IndexLength;
    ULONG        AllocatedSize;
    UCHAR        Flags;
    UCHAR        Reserved[3];
} NTFS_INDEX_HEADER, *PNTFS_INDEX_HEADER;

typedef struct
{
    ULONG        Type;
    ULONG        CollationRule;
    ULONG        IndexBlockSize;
    UCHAR        ClustersPerIndexBlock;
    UCHAR        Reserved[3];
    NTFS_INDEX_HEADER    IndexHeader;
} NTFS_INDEX_ROOT, *PNTFS_INDEX_ROOT;

typedef struct
{
    ULONGLONG        ParentDirectory;
    LONGLONG        CreationTime;
    LONGLONG        LastDataChangeTime;
    LONGLONG        LastMftChangeTime;
    LONGLONG        LastAccessTime;
    LONGLONG        AllocatedSize;
    LONGLONG        DataSize;
    ULONG        FileAttributes;
    USHORT        PackedExtendedAttributeSize;
    USHORT        Reserved;
    UCHAR        FileNameLength;
    UCHAR        FileNameType;
    WCHAR        FileName[0];
} NTFS_FILE_NAME_ATTR, *PNTFS_FILE_NAME_ATTR;

typedef struct
{
    ULONG        Type;
    USHORT        RecLength;
    UCHAR        NameLength;
    UCHAR        NameOffset;
    ULONGLONG    StartingVCN;
    ULONGLONG    BaseFileRef;
    USHORT        AttrId;
    PWCHAR        Name;
} NTFS_ATTR_LIST_ATTR, *PNTFS_ATTR_LIST_ATTR;

typedef struct
{
    union
    {
        struct
        {
            ULONGLONG    IndexedFile;
        } Directory;
        struct
        {
            USHORT    DataOffset;
            USHORT    DataLength;
            ULONG    Reserved;
        } ViewIndex;
    } Data;
    USHORT            Length;
    USHORT            KeyLength;
    USHORT            Flags;
    USHORT            Reserved;
    NTFS_FILE_NAME_ATTR    FileName;
} NTFS_INDEX_ENTRY, *PNTFS_INDEX_ENTRY;
#include <poppack.h>

typedef struct
{
    PUCHAR            CacheRun;
    ULONGLONG            CacheRunOffset;
    LONGLONG            CacheRunStartLCN;
    ULONGLONG            CacheRunLength;
    LONGLONG            CacheRunLastLCN;
    ULONGLONG            CacheRunCurrentOffset;
    NTFS_ATTR_RECORD    Record;
} NTFS_ATTR_CONTEXT, *PNTFS_ATTR_CONTEXT;

typedef struct _NTFS_VOLUME_INFO *PNTFS_VOLUME_INFO;

#include <pshpack1.h>
typedef struct
{
    PNTFS_ATTR_CONTEXT    DataContext;
    ULONGLONG            Offset;
    PNTFS_VOLUME_INFO    Volume;
} NTFS_FILE_HANDLE, *PNTFS_FILE_HANDLE;
#include <poppack.h>

const DEVVTBL* NtfsMount(ULONG DeviceId);
