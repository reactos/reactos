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

#include <freeldr.h>
#include <disk.h>
#include <fs.h>
#include <rtl.h>
#include <ui.h>
#include <asmcode.h>
#include <debug.h>


/////////////////////////////////////////////////////////////////////////////////////////////
// DATA
/////////////////////////////////////////////////////////////////////////////////////////////

GEOMETRY	DriveGeometry;
ULONG		VolumeHiddenSectors;
ULONG		CurrentlyOpenDriveNumber;

/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

VOID DiskError(PUCHAR ErrorString)
{
	DbgPrint((DPRINT_DISK, "%s\n", ErrorString));

	if (UserInterfaceUp)
	{
		MessageBox(ErrorString);
	}
	else
	{
		printf("%s", ErrorString);
		printf("\nPress any key\n");
		getch();
	}
}

VOID DiskSetDriveGeometry(ULONG Cylinders, ULONG Heads, ULONG Sectors, ULONG BytesPerSector)
{
	DriveGeometry.Cylinders = Cylinders;
	DriveGeometry.Heads = Heads;
	DriveGeometry.Sectors = Sectors;
	DriveGeometry.BytesPerSector = BytesPerSector;

	DbgPrint((DPRINT_DISK, "DriveGeometry.Cylinders: %d\n", DriveGeometry.Cylinders));
	DbgPrint((DPRINT_DISK, "DriveGeometry.Heads: %d\n", DriveGeometry.Heads));
	DbgPrint((DPRINT_DISK, "DriveGeometry.Sectors: %d\n", DriveGeometry.Sectors));
	DbgPrint((DPRINT_DISK, "DriveGeometry.BytesPerSector: %d\n", DriveGeometry.BytesPerSector));
}

VOID DiskSetVolumeProperties(ULONG HiddenSectors)
{
	VolumeHiddenSectors = HiddenSectors;
}

BOOL DiskReadMultipleLogicalSectors(ULONG SectorNumber, ULONG SectorCount, PVOID Buffer)
{
	/*BOOL	bRetVal;
	int		PhysicalSector;
	int		PhysicalHead;
	int		PhysicalTrack;
	int		nNum;

	nSect += nHiddenSectors;

	while (nNumberOfSectors)
	{
		PhysicalSector = 1 + (nSect % nSectorsPerTrack);
		PhysicalHead = (nSect / nSectorsPerTrack) % nNumberOfHeads;
		PhysicalTrack = nSect / (nSectorsPerTrack * nNumberOfHeads);

		if (PhysicalSector > 1)
		{
			if (nNumberOfSectors >= (nSectorsPerTrack - (PhysicalSector - 1)))
				nNum = (nSectorsPerTrack - (PhysicalSector - 1));
			else
				nNum = nNumberOfSectors;
		}
		else
		{
			if (nNumberOfSectors >= nSectorsPerTrack)
				nNum = nSectorsPerTrack;
			else
				nNum = nNumberOfSectors;
		}

		bRetVal = biosdisk(CurrentlyOpenDriveNumber, PhysicalHead, PhysicalTrack, PhysicalSector, nNum, pBuffer);

		if (!bRetVal)
		{
			FS_DO_ERROR("Disk Error");
			return FALSE;
		}

		pBuffer += (nNum * 512);
		nNumberOfSectors -= nNum;
		nSect += nNum;
	}*/

	ULONG	CurrentSector;
	PVOID	RealBuffer = Buffer;

	for (CurrentSector=SectorNumber; CurrentSector<(SectorNumber + SectorCount); CurrentSector++)
	{
		if (!DiskReadLogicalSector(CurrentSector, RealBuffer) )
		{
			return FALSE;
		}

		RealBuffer += DriveGeometry.BytesPerSector;
	}

	return TRUE;
}

BOOL DiskReadLogicalSector(ULONG SectorNumber, PVOID Buffer)
{
	ULONG	PhysicalSector;
	ULONG	PhysicalHead;
	ULONG	PhysicalTrack;

	DbgPrint((DPRINT_DISK, "ReadLogicalSector() SectorNumber: %d Buffer: 0x%x\n", SectorNumber, Buffer));

	SectorNumber += VolumeHiddenSectors;
	PhysicalSector = 1 + (SectorNumber % DriveGeometry.Sectors);
	PhysicalHead = (SectorNumber / DriveGeometry.Sectors) % DriveGeometry.Heads;
	PhysicalTrack = (SectorNumber / DriveGeometry.Sectors) / DriveGeometry.Heads;

	//DbgPrint((DPRINT_FILESYSTEM, "Calling BiosInt13Read() with PhysicalHead: %d\n", PhysicalHead));
	//DbgPrint((DPRINT_FILESYSTEM, "Calling BiosInt13Read() with PhysicalTrack: %d\n", PhysicalTrack));
	//DbgPrint((DPRINT_FILESYSTEM, "Calling BiosInt13Read() with PhysicalSector: %d\n", PhysicalSector));
	if (PhysicalHead >= DriveGeometry.Heads)
	{
		BugCheck((DPRINT_DISK, "PhysicalHead >= DriveGeometry.Heads\nPhysicalHead = %d\nDriveGeometry.Heads = %d\n", PhysicalHead, DriveGeometry.Heads));
	}
	if (PhysicalTrack >= DriveGeometry.Cylinders)
	{
		BugCheck((DPRINT_DISK, "PhysicalTrack >= DriveGeometry.Cylinders\nPhysicalTrack = %d\nDriveGeometry.Cylinders = %d\n", PhysicalTrack, DriveGeometry.Cylinders));
	}
	if (PhysicalSector > DriveGeometry.Sectors)
	{
		BugCheck((DPRINT_DISK, "PhysicalSector > DriveGeometry.Sectors\nPhysicalSector = %d\nDriveGeometry.Sectors = %d\n", PhysicalSector, DriveGeometry.Sectors));
	}

	//
	// Check to see if it is a fixed disk drive
	// If so then check to see if Int13 extensions work
	// If they do then use them, otherwise default back to BIOS calls
	//
	if ((CurrentlyOpenDriveNumber >= 0x80) && (BiosInt13ExtensionsSupported(CurrentlyOpenDriveNumber)) && (SectorNumber > (DriveGeometry.Cylinders * DriveGeometry.Heads * DriveGeometry.Sectors)))
	{
		DbgPrint((DPRINT_DISK, "Using Int 13 Extensions for read. BiosInt13ExtensionsSupported(%d) = %s\n", CurrentlyOpenDriveNumber, BiosInt13ExtensionsSupported(CurrentlyOpenDriveNumber) ? "TRUE" : "FALSE"));
		if ( !BiosInt13ReadExtended(CurrentlyOpenDriveNumber, SectorNumber, 1, Buffer) )
		{
			DiskError("Disk read error.");
			return FALSE;
		}
	}
	else
	{
		if ( !BiosInt13Read(CurrentlyOpenDriveNumber, PhysicalHead, PhysicalTrack, PhysicalSector, 1, Buffer) )
		{
			DiskError("Disk read error.");
			return FALSE;
		}
	}

	return TRUE;
}
