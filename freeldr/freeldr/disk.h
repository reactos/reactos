/*
 *  FreeLoader
 *  Copyright (C) 1999, 2000, 2001  Brian Palmer  <brianp@sginet.com>
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

#ifndef __DISK_H
#define __DISK_H


typedef struct _GEOMETRY
{
	ULONG	Cylinders;
	ULONG	Heads;
	ULONG	Sectors;
	ULONG	BytesPerSector;

} GEOMETRY, *PGEOMETRY;

///////////////////////////////////////////////////////////////////////////////////////
//
// BIOS Disk Functions
//
///////////////////////////////////////////////////////////////////////////////////////
int		biosdisk(int cmd, int drive, int head, int track, int sector, int nsects, void *buffer); // Implemented in asmcode.S

BOOL	BiosInt13Read(ULONG Drive, ULONG Head, ULONG Track, ULONG Sector, ULONG SectorCount, PVOID Buffer); // Implemented in asmcode.S
BOOL	BiosInt13ReadExtended(ULONG Drive, ULONG Sector, ULONG SectorCount, PVOID Buffer); // Implemented in asmcode.S
BOOL	BiosInt13ExtensionsSupported(ULONG Drive);

void	stop_floppy(void);			// Implemented in asmcode.S
int		get_heads(int drive);		// Implemented in asmcode.S
int		get_cylinders(int drive);	// Implemented in asmcode.S
int		get_sectors(int drive);		// Implemented in asmcode.S

///////////////////////////////////////////////////////////////////////////////////////
//
// FreeLoader Disk Functions
//
///////////////////////////////////////////////////////////////////////////////////////
VOID	DiskError(PUCHAR ErrorString);
BOOL	DiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry);
BOOL	DiskSetDriveGeometry(ULONG DriveNumber, ULONG Cylinders, ULONG Heads, ULONG Sectors, ULONG BytesPerSector);
BOOL	DiskReadMultipleLogicalSectors(ULONG DriveNumber, ULONG SectorNumber, ULONG SectorCount, PVOID Buffer);
BOOL	DiskReadLogicalSector(ULONG DriveNumber, ULONG SectorNumber, PVOID Buffer);

#endif  // defined __DISK_H
