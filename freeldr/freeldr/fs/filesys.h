/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
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

#include <fs.h>

#ifndef __FILESYS_H
#define __FILESYS_H

BOOL	FsInternalIsDiskPartitioned(ULONG DriveNumber);		// Returns TRUE if the disk contains partitions, FALSE if floppy disk
BOOL	FsInternalGetActivePartitionEntry(ULONG DriveNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);	// Returns the active partition table entry
BOOL	FsInternalGetPartitionEntry(ULONG DriveNumber, ULONG PartitionNumber, PPARTITION_TABLE_ENTRY PartitionTableEntry);	// Returns the active partition table entry
ULONG	FsInternalGetPartitionCount(ULONG DriveNumber);		// Returns the number of partitions on the disk

#endif // #defined __FILESYS_H
