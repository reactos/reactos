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

} PARTLIST, *PPARTLIST;



PPARTLIST
CreatePartitionList (SHORT Left,
		     SHORT Top,
		     SHORT Right,
		     SHORT Bottom);

VOID
DestroyPartitionList (PPARTLIST List);

VOID
DrawPartitionList (PPARTLIST List);

VOID
SelectPartition(PPARTLIST List, ULONG DiskNumber, ULONG PartitionNumber);

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
