/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: partlist.h,v 1.8 2003/04/05 15:36:34 chorns Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/partlist.h
 * PURPOSE:         Partition list functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __PARTLIST_H__
#define __PARTLIST_H__

typedef struct _PARTDATA
{
  ULONGLONG DiskSize;
  ULONG DiskNumber;
  USHORT Port;
  USHORT Bus;
  USHORT Id;

  ULONGLONG PartSize;
  ULONG PartNumber;
  ULONG PartType;

  CHAR DriveLetter;

  UNICODE_STRING DriverName;
} PARTDATA, *PPARTDATA;


typedef struct _PARTENTRY
{
  ULONGLONG PartSize;
  ULONG PartNumber;
  ULONG PartType;
  BOOLEAN Active;

  CHAR DriveLetter;
  CHAR VolumeLabel[17];
  CHAR FileSystemName[9];

  BOOL Unpartitioned;

  BOOL Used;
} PARTENTRY, *PPARTENTRY;

typedef struct _DISKENTRY
{
  ULONGLONG DiskSize;
  ULONG DiskNumber;
  USHORT Port;
  USHORT Bus;
  USHORT Id;
  BOOL FixedDisk;

  UNICODE_STRING DriverName;

  ULONG PartCount;
  PPARTENTRY PartArray;

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

  ULONG CurrentDisk;
  ULONG CurrentPartition;

  ULONG DiskCount;
  PDISKENTRY DiskArray;

} PARTLIST, *PPARTLIST;




PPARTLIST
CreatePartitionList(SHORT Left,
		    SHORT Top,
		    SHORT Right,
		    SHORT Bottom);

BOOLEAN
MarkPartitionActive(ULONG DiskNumber,
			ULONG PartitionNumber,
			PPARTDATA ActivePartition);

VOID
DestroyPartitionList(PPARTLIST List);

VOID
DrawPartitionList(PPARTLIST List);

VOID
ScrollDownPartitionList(PPARTLIST List);

VOID
ScrollUpPartitionList(PPARTLIST List);

BOOL
GetSelectedPartition(PPARTLIST List,
		     PPARTDATA Data);

BOOL
GetActiveBootPartition(PPARTLIST List,
		       PPARTDATA Data);

#endif /* __PARTLIST_H__ */

/* EOF */
