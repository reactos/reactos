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

#include <freeldr.h>
#include <disk.h>
#include <arch.h>
#include <rtl.h>
#include <ui.h>
#include <debug.h>


#undef  UNIMPLEMENTED
#define UNIMPLEMENTED   BugCheck((DPRINT_WARNING, "Unimplemented\n"));

/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

VOID DiskError(PUCHAR ErrorString, U32 ErrorCode)
{
	UCHAR	ErrorCodeString[80];

	sprintf(ErrorCodeString, "%s\n\nError Code: 0x%x", ErrorString, ErrorCode);

	DbgPrint((DPRINT_DISK, "%s\n", ErrorCodeString));

	UiMessageBox(ErrorCodeString);
}

// This function is in arch/i386/i386disk.c
//BOOL DiskReadLogicalSectors(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer)

BOOL DiskIsDriveRemovable(U32 DriveNumber)
{
	// Hard disks use drive numbers >= 0x80
	// So if the drive number indicates a hard disk
	// then return FALSE
	if (DriveNumber >= 0x80)
	{
		return FALSE;
	}

	// Drive is a floppy diskette so return TRUE
	return TRUE;
}


BOOL DiskIsDriveCdRom(U32 DriveNumber)
{
	PUCHAR Sector = (PUCHAR)DISKREADBUFFER;

	// FIXME:
	// I need to move this code to somewhere else
	// probably in the file system code.
	// I don't like the fact that the disk code
	// reads the on-disk structures to determine
	// if it's a CD-ROM drive. Only the file system
	// should interpret on-disk data.
	// The disk code should use some other method
	// to determine if it's a CD-ROM drive.

	// Hard disks use drive numbers >= 0x80
	// So if the drive number indicates a hard disk
	// then return FALSE
	//
	// We first check if we are running as a SetupLoader
	// If so then we are probably booting from a CD-ROM
	// So we shouldn't call i386DiskInt13ExtensionsSupported()
	// because apparently it screws up some BIOSes
	if ((DriveNumber >= 0x80) && (IsSetupLdr || DiskInt13ExtensionsSupported(DriveNumber)))
	{

		// We are a CD-ROM drive so we should only
		// use the extended Int13 disk functions
		if (!DiskReadLogicalSectorsLBA(DriveNumber, 16, 1, Sector))
		{
			DiskError("Disk read error.", 0);
			return FALSE;
		}

		return (Sector[0] == 1 &&
			Sector[1] == 'C' &&
			Sector[2] == 'D' &&
			Sector[3] == '0' &&
			Sector[4] == '0' &&
			Sector[5] == '1');
	}

	// Drive is not CdRom so return FALSE
	return FALSE;
}

// This function is in arch/i386/i386disk.c
//VOID DiskStopFloppyMotor(VOID)

// This function is in arch/i386/i386disk.c
//U32 DiskGetCacheableBlockCount(U32 DriveNumber)
