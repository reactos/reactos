/*
 *  ReactOS kernel
 *  Copyright (C) 2002, 2003 ReactOS Team
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
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/partlist.h
 * PURPOSE:         Partition list functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

typedef enum _FORMATSTATE
{
    Unformatted,
    UnformattedOrDamaged,
    UnknownFormat,
    Preformatted,
    Formatted
} FORMATSTATE, *PFORMATSTATE;

typedef enum _FORMATMACHINESTATE
{
    Start,
    FormatSystemPartition,
    FormatInstallPartition,
    FormatOtherPartition,
    FormatDone,
    CheckSystemPartition,
    CheckInstallPartition,
    CheckOtherPartition,
    CheckDone
} FORMATMACHINESTATE, *PFORMATMACHINESTATE;

typedef struct _PARTENTRY
{
    LIST_ENTRY ListEntry;

    struct _DISKENTRY *DiskEntry;

    ULARGE_INTEGER StartSector;
    ULARGE_INTEGER SectorCount;

    BOOLEAN BootIndicator;
    UCHAR PartitionType;
    ULONG HiddenSectors;
    ULONG PartitionNumber;
    ULONG PartitionIndex;

    CHAR DriveLetter;
    CHAR VolumeLabel[17];
    CHAR FileSystemName[9];

    BOOLEAN LogicalPartition;

    /* Partition is partitioned disk space */
    BOOLEAN IsPartitioned;

    /* Partition is new, table does not exist on disk yet */
    BOOLEAN New;

    /* Partition was created automatically */
    BOOLEAN AutoCreate;

    FORMATSTATE FormatState;

    /* Partition must be checked */
    BOOLEAN NeedsCheck;

    struct _FILE_SYSTEM_ITEM *FileSystem;
} PARTENTRY, *PPARTENTRY;


typedef struct _BIOSDISKENTRY
{
    LIST_ENTRY ListEntry;
    ULONG DiskNumber;
    ULONG Signature;
    ULONG Checksum;
    BOOLEAN Recognized;
    CM_DISK_GEOMETRY_DEVICE_DATA DiskGeometry;
    CM_INT13_DRIVE_PARAMETER Int13DiskData;
} BIOSDISKENTRY, *PBIOSDISKENTRY;


typedef struct _DISKENTRY
{
    LIST_ENTRY ListEntry;

    ULONGLONG Cylinders;
    ULONG TracksPerCylinder;
    ULONG SectorsPerTrack;
    ULONG BytesPerSector;

    ULARGE_INTEGER SectorCount;
    ULONG SectorAlignment;
    ULONG CylinderAlignment;

    BOOLEAN BiosFound;
    ULONG BiosDiskNumber;
//    ULONG Signature;
//    ULONG Checksum;

    ULONG DiskNumber;
    USHORT Port;
    USHORT Bus;
    USHORT Id;

    /* Has the partition list been modified? */
    BOOLEAN Dirty;

    BOOLEAN NewDisk;
    BOOLEAN NoMbr; /* MBR is absent */

    UNICODE_STRING DriverName;

    PDRIVE_LAYOUT_INFORMATION LayoutBuffer;

    PPARTENTRY ExtendedPartition;

    LIST_ENTRY PrimaryPartListHead;
    LIST_ENTRY LogicalPartListHead;

} DISKENTRY, *PDISKENTRY;


typedef struct _PARTLIST
{
    SHORT Left;
    SHORT Top;
    SHORT Right;
    SHORT Bottom;

    SHORT Line;
    SHORT Offset;

    PDISKENTRY CurrentDisk;
    PPARTENTRY CurrentPartition;

    /* The system disk and partition where the boot manager resides */
    PDISKENTRY SystemDisk;
    PPARTENTRY SystemPartition;
    /*
     * The original system disk and partition in case we are redefining them
     * because we do not have write support on them.
     * Please not that this is partly a HACK and MUST NEVER happen on
     * architectures where real system partitions are mandatory (because then
     * they are formatted in FAT FS and we support write operation on them).
     */
    PDISKENTRY OriginalSystemDisk;
    PPARTENTRY OriginalSystemPartition;

    PDISKENTRY TempDisk;
    PPARTENTRY TempPartition;
    FORMATMACHINESTATE FormatState;

    LIST_ENTRY DiskListHead;
    LIST_ENTRY BiosDiskListHead;

} PARTLIST, *PPARTLIST;

#define  PARTITION_TBL_SIZE 4

#include <pshpack1.h>

typedef struct _PARTITION
{
    unsigned char   BootFlags;        /* bootable?  0=no, 128=yes  */
    unsigned char   StartingHead;     /* beginning head number */
    unsigned char   StartingSector;   /* beginning sector number */
    unsigned char   StartingCylinder; /* 10 bit nmbr, with high 2 bits put in begsect */
    unsigned char   PartitionType;    /* Operating System type indicator code */
    unsigned char   EndingHead;       /* ending head number */
    unsigned char   EndingSector;     /* ending sector number */
    unsigned char   EndingCylinder;   /* also a 10 bit nmbr, with same high 2 bit trick */
    unsigned int  StartingBlock;      /* first sector relative to start of disk */
    unsigned int  SectorCount;        /* number of sectors in partition */
} PARTITION, *PPARTITION;

typedef struct _PARTITION_SECTOR
{
    UCHAR BootCode[440];                     /* 0x000 */
    ULONG Signature;                         /* 0x1B8 */
    UCHAR Reserved[2];                       /* 0x1BC */
    PARTITION Partition[PARTITION_TBL_SIZE]; /* 0x1BE */
    USHORT Magic;                            /* 0x1FE */
} PARTITION_SECTOR, *PPARTITION_SECTOR;

#include <poppack.h>

typedef struct
{
    LIST_ENTRY ListEntry;
    ULONG DiskNumber;
    ULONG Identifier;
    ULONG Signature;
} BIOS_DISK, *PBIOS_DISK;

VOID
GetPartTypeStringFromPartitionType(
    IN UCHAR partitionType,
    OUT PCHAR strPartType,
    IN ULONG cchPartType);

PPARTLIST
CreatePartitionList(
    SHORT Left,
    SHORT Top,
    SHORT Right,
    SHORT Bottom);

VOID
DestroyPartitionList(
    IN PPARTLIST List);

VOID
DrawPartitionList(
    IN PPARTLIST List);

ULONG
SelectPartition(
    IN PPARTLIST List,
    IN ULONG DiskNumber,
    IN ULONG PartitionNumber);

BOOL
ScrollDownPartitionList(
    IN PPARTLIST List);

BOOL
ScrollUpPartitionList(
    IN PPARTLIST List);

VOID
CreatePrimaryPartition(
    IN PPARTLIST List,
    IN ULONGLONG SectorCount,
    IN BOOLEAN AutoCreate);

VOID
CreateExtendedPartition(
    IN PPARTLIST List,
    IN ULONGLONG SectorCount);

VOID
CreateLogicalPartition(
    IN PPARTLIST List,
    IN ULONGLONG SectorCount,
    IN BOOLEAN AutoCreate);

VOID
DeleteCurrentPartition(
    IN PPARTLIST List);

VOID
CheckActiveSystemPartition(
    IN PPARTLIST List,
    IN PFILE_SYSTEM_LIST FileSystemList);

BOOLEAN
WritePartitionsToDisk(
    IN PPARTLIST List);

BOOLEAN
SetMountedDeviceValues(
    IN PPARTLIST List);

ULONG
PrimaryPartitionCreationChecks(
    IN PPARTLIST List);

ULONG
ExtendedPartitionCreationChecks(
    IN PPARTLIST List);

ULONG
LogicalPartitionCreationChecks(
    IN PPARTLIST List);

BOOLEAN
GetNextUnformattedPartition(
    IN PPARTLIST List,
    OUT PDISKENTRY *pDiskEntry OPTIONAL,
    OUT PPARTENTRY *pPartEntry);

BOOLEAN
GetNextUncheckedPartition(
    IN PPARTLIST List,
    OUT PDISKENTRY *pDiskEntry OPTIONAL,
    OUT PPARTENTRY *pPartEntry);

/* EOF */
