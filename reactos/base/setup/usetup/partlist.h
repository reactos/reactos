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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/partlist.h
 * PURPOSE:         Partition list functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __PARTLIST_H__
#define __PARTLIST_H__

typedef enum _FORMATSTATE
{
  Unformatted,
  UnformattedOrDamaged,
  UnknownFormat,
  Preformatted,
  Formatted
} FORMATSTATE, *PFORMATSTATE;


typedef struct _PARTENTRY
{
  LIST_ENTRY ListEntry;

  CHAR DriveLetter;
  CHAR VolumeLabel[17];
  CHAR FileSystemName[9];

  /* Partition is unused disk space */
  BOOLEAN Unpartitioned;

  /* Partition is new. Table does not exist on disk yet */
  BOOLEAN New;

  /* Partition was created automatically. */
  BOOLEAN AutoCreate;

  FORMATSTATE FormatState;

  /*
   * Raw offset and length of the unpartitioned disk space.
   * Includes the leading, not yet existing, partition table.
   */
  ULONGLONG UnpartitionedOffset;
  ULONGLONG UnpartitionedLength;

  PARTITION_INFORMATION PartInfo[4];

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
  ULONGLONG TracksPerCylinder;
  ULONGLONG SectorsPerTrack;
  ULONGLONG BytesPerSector;

  ULONGLONG DiskSize;
  ULONGLONG CylinderSize;
  ULONGLONG TrackSize;

  BOOLEAN BiosFound;
  ULONG BiosDiskNumber;
  ULONG Signature;
  ULONG Checksum;

  ULONG DiskNumber;
  USHORT Port;
  USHORT Bus;
  USHORT Id;

  /* Has the partition list been modified? */
  BOOLEAN Modified;

  BOOLEAN NewDisk;

  UNICODE_STRING DriverName;

  LIST_ENTRY PartListHead;

} DISKENTRY, *PDISKENTRY;


typedef struct _PARTLIST
{
  SHORT Left;
  SHORT Top;
  SHORT Right;
  SHORT Bottom;

  SHORT Line;
  SHORT Offset;

  ULONG TopDisk;
  ULONG TopPartition;

  PDISKENTRY CurrentDisk;
  PPARTENTRY CurrentPartition;

  PDISKENTRY ActiveBootDisk;
  PPARTENTRY ActiveBootPartition;

  LIST_ENTRY DiskListHead;
  LIST_ENTRY BiosDiskListHead;

} PARTLIST, *PPARTLIST;

#define  PARTITION_TBL_SIZE 4

#include <pshpack1.h>

typedef struct _PARTITION
{
  unsigned char   BootFlags;					/* bootable?  0=no, 128=yes  */
  unsigned char   StartingHead;					/* beginning head number */
  unsigned char   StartingSector;				/* beginning sector number */
  unsigned char   StartingCylinder;				/* 10 bit nmbr, with high 2 bits put in begsect */
  unsigned char   PartitionType;				/* Operating System type indicator code */
  unsigned char   EndingHead;					/* ending head number */
  unsigned char   EndingSector;					/* ending sector number */
  unsigned char   EndingCylinder;				/* also a 10 bit nmbr, with same high 2 bit trick */
  unsigned int  StartingBlock;					/* first sector relative to start of disk */
  unsigned int  SectorCount;					/* number of sectors in partition */
} PARTITION, *PPARTITION;

typedef struct _PARTITION_SECTOR
{
  UCHAR BootCode[440];				/* 0x000 */
  ULONG Signature;				/* 0x1B8 */
  UCHAR Reserved[2];				/* 0x1BC */
  PARTITION Partition[PARTITION_TBL_SIZE];	/* 0x1BE */
  USHORT Magic;					/* 0x1FE */
} PARTITION_SECTOR, *PPARTITION_SECTOR;

#include <poppack.h>

typedef struct
{
   LIST_ENTRY ListEntry;
   ULONG DiskNumber;
   ULONG Idendifier;
   ULONG Signature;
} BIOS_DISK, *PBIOS_DISK;

PPARTLIST
CreatePartitionList (SHORT Left,
		     SHORT Top,
		     SHORT Right,
		     SHORT Bottom);

VOID
DestroyPartitionList (PPARTLIST List);

VOID
DrawPartitionList (PPARTLIST List);

DWORD
SelectPartition(PPARTLIST List, ULONG DiskNumber, ULONG PartitionNumber);

BOOL
SetMountedDeviceValues(PPARTLIST List);

VOID
ScrollDownPartitionList (PPARTLIST List);

VOID
ScrollUpPartitionList (PPARTLIST List);

VOID
CreateNewPartition (PPARTLIST List,
		    ULONGLONG PartitionSize,
		    BOOLEAN AutoCreate);

VOID
DeleteCurrentPartition (PPARTLIST List);

VOID
CheckActiveBootPartition (PPARTLIST List);

BOOLEAN
CheckForLinuxFdiskPartitions (PPARTLIST List);

BOOLEAN
WritePartitionsToDisk (PPARTLIST List);

#endif /* __PARTLIST_H__ */

/* EOF */
