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
/* $Id: partlist.h,v 1.14 2003/08/05 20:39:24 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/partlist.h
 * PURPOSE:         Partition list functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __PARTLIST_H__
#define __PARTLIST_H__


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

  /*
   * Raw offset and length of the unpartitioned disk space.
   * Includes the leading, not yet existing, partition table.
   */
  ULONGLONG UnpartitionedOffset;
  ULONGLONG UnpartitionedLength;

  PARTITION_INFORMATION PartInfo[4];

} PARTENTRY, *PPARTENTRY;


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

  ULONG DiskNumber;
  USHORT Port;
  USHORT Bus;
  USHORT Id;

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

  ULONG TopDisk;
  ULONG TopPartition;

  PDISKENTRY CurrentDisk;
  PPARTENTRY CurrentPartition;

#if 0
  /* Not used yet! */
  PDISKENTRY ActiveBootDisk;
  PPARTENTRY ActiveBootPartition;
#endif

  LIST_ENTRY DiskListHead;

} PARTLIST, *PPARTLIST;



PPARTLIST
CreatePartitionList(SHORT Left,
		    SHORT Top,
		    SHORT Right,
		    SHORT Bottom);

VOID
DestroyPartitionList(PPARTLIST List);

VOID
DrawPartitionList(PPARTLIST List);

VOID
ScrollDownPartitionList(PPARTLIST List);

VOID
ScrollUpPartitionList(PPARTLIST List);

VOID
GetActiveBootPartition(PPARTLIST List,
		       PDISKENTRY *DiskEntry,
		       PPARTENTRY *PartEntry);

BOOLEAN
CreateSelectedPartition(PPARTLIST List,
			ULONG PartType,
			ULONGLONG NewPartSize);

BOOLEAN
DeleteSelectedPartition(PPARTLIST List);

#if 0
BOOLEAN
MarkPartitionActive(ULONG DiskNumber,
		    ULONG PartitionNumber,
		    PPARTDATA ActivePartition);
#endif

#endif /* __PARTLIST_H__ */

/* EOF */
