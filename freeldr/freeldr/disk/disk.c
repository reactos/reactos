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
	UCHAR	ErrorCodeString[200];

	sprintf(ErrorCodeString, "%s\n\nError Code: 0x%x\nError: %s", ErrorString, ErrorCode, DiskGetErrorCodeString(ErrorCode));

	DbgPrint((DPRINT_DISK, "%s\n", ErrorCodeString));

	UiMessageBox(ErrorCodeString);
}

PUCHAR DiskGetErrorCodeString(U32 ErrorCode)
{
	switch (ErrorCode)
	{
	case 0x00:  return "no error";
	case 0x01:  return "bad command passed to driver";
	case 0x02:  return "address mark not found or bad sector";
	case 0x03:  return "diskette write protect error";
	case 0x04:  return "sector not found";
	case 0x05:  return "fixed disk reset failed";
	case 0x06:  return "diskette changed or removed";
	case 0x07:  return "bad fixed disk parameter table";
	case 0x08:  return "DMA overrun";
	case 0x09:  return "DMA access across 64k boundary";
	case 0x0A:  return "bad fixed disk sector flag";
	case 0x0B:  return "bad fixed disk cylinder";
	case 0x0C:  return "unsupported track/invalid media";
	case 0x0D:  return "invalid number of sectors on fixed disk format";
	case 0x0E:  return "fixed disk controlled data address mark detected";
	case 0x0F:  return "fixed disk DMA arbitration level out of range";
	case 0x10:  return "ECC/CRC error on disk read";
	case 0x11:  return "recoverable fixed disk data error, data fixed by ECC";
	case 0x20:  return "controller error (NEC for floppies)";
	case 0x40:  return "seek failure";
	case 0x80:  return "time out, drive not ready";
	case 0xAA:  return "fixed disk drive not ready";
	case 0xBB:  return "fixed disk undefined error";
	case 0xCC:  return "fixed disk write fault on selected drive";
	case 0xE0:  return "fixed disk status error/Error reg = 0";
	case 0xFF:  return "sense operation failed";

	default:  return "unknown error code";
	}
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
