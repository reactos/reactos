/*
 *  FreeLoader
 *  Copyright (C) 1998-2003  Brian Palmer  <brianp@sginet.com>
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

#include <freeldr.h>
#include <fs.h>
#include "fsrec.h"
#include "fat.h"
#include "iso.h"
#include "ext2.h"
#include <disk.h>
#include <rtl.h>
#include <arch.h>
#include <debug.h>




/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 * BOOL FsRecognizeVolume(U32 DriveNumber, U32 VolumeStartSector, U8* VolumeType);
 *
 */
BOOL FsRecognizeVolume(U32 DriveNumber, U32 VolumeStartSector, U8* VolumeType)
{

	DbgPrint((DPRINT_FILESYSTEM, "FsRecognizeVolume() DriveNumber: 0x%x VolumeStartSector: %d\n", DriveNumber, VolumeStartSector));

	if (FsRecIsExt2(DriveNumber, VolumeStartSector))
	{
		*VolumeType = PARTITION_EXT2;
		return TRUE;
	}
	else if (FsRecIsFat(DriveNumber, VolumeStartSector))
	{
		*VolumeType = PARTITION_FAT32;
		return TRUE;
	}

	return FALSE;
}

BOOL FsRecIsIso9660(U32 DriveNumber)
{
	PUCHAR Sector = (PUCHAR)DISKREADBUFFER;

	if (!DiskReadLogicalSectors(DriveNumber, 16, 1, Sector))
	{
		FileSystemError("Failed to read the PVD.");
		return FALSE;
	}

	return (Sector[0] == 1 &&
		Sector[1] == 'C' &&
		Sector[2] == 'D' &&
		Sector[3] == '0' &&
		Sector[4] == '0' &&
		Sector[5] == '1');
}

BOOL FsRecIsExt2(U32 DriveNumber, U32 VolumeStartSector)
{
	PEXT2_SUPER_BLOCK	SuperBlock = (PEXT2_SUPER_BLOCK)DISKREADBUFFER;

	if (!DiskReadLogicalSectors(DriveNumber, VolumeStartSector + 2, 2, SuperBlock))
	{
		FileSystemError("Failed to read the super block.");
		return FALSE;
	}

	if (SuperBlock->s_magic == EXT3_SUPER_MAGIC)
	{
		return TRUE;
	}

	return FALSE;
}

BOOL FsRecIsFat(U32 DriveNumber, U32 VolumeStartSector)
{
	PFAT_BOOTSECTOR	BootSector = (PFAT_BOOTSECTOR)DISKREADBUFFER;
	PFAT32_BOOTSECTOR BootSector32 = (PFAT32_BOOTSECTOR)DISKREADBUFFER;
	if (!DiskReadLogicalSectors(DriveNumber, VolumeStartSector, 1, BootSector))
	{
		FileSystemError("Failed to read the boot sector.");
		return FALSE;
	}

	if (strncmp(BootSector->FileSystemType, "FAT12   ", 8) == 0 ||
		strncmp(BootSector->FileSystemType, "FAT16   ", 8) == 0 ||
		strncmp(BootSector32->FileSystemType, "FAT32   ", 8) == 0)
	{
		return TRUE;
	}

	return FALSE;
}
