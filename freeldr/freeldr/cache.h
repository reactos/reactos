/*
 *  FreeLoader
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
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


#ifndef __CACHE_H
#define __CACHE_H

BOOL	CacheInitializeDrive(ULONG DriveNumber);
BOOL	CacheReadDiskSectors(ULONG DiskNumber, ULONG StartSector, ULONG SectorCount, PVOID Buffer);
BOOL	CacheForceDiskSectorsIntoCache(ULONG DiskNumber, ULONG StartSector, ULONG SectorCount);
BOOL	CacheReleaseMemory(ULONG MinimumAmountToRelease);

#endif // defined __CACHE_H
