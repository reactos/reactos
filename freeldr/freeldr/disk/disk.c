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
#include <fs.h>
#include <rtl.h>
#include <ui.h>
#include <arch.h>
#include <debug.h>


/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

VOID DiskError(PUCHAR ErrorString)
{
	UCHAR	ErrorCodeString[80];

	sprintf(ErrorCodeString, "%s\nError Code: 0x%lx", ErrorString, BiosInt13GetLastErrorCode());

	DbgPrint((DPRINT_DISK, "%s\n", ErrorCodeString));

	UiMessageBox(ErrorCodeString);
}

BOOL DiskReadLogicalSectors(U32 DriveNumber, U32 SectorNumber, U32 SectorCount, PVOID Buffer)
{
	U32			PhysicalSector;
	U32			PhysicalHead;
	U32			PhysicalTrack;
	GEOMETRY	DriveGeometry;
	U32			NumberOfSectorsToRead;

	DbgPrint((DPRINT_DISK, "ReadLogicalSectors() DriveNumber: 0x%x SectorNumber: %d SectorCount: %d Buffer: 0x%x\n", DriveNumber, SectorNumber, SectorCount, Buffer));

	//
	// Check to see if it is a fixed disk drive
	// If so then check to see if Int13 extensions work
	// If they do then use them, otherwise default back to BIOS calls
	//
	if ((DriveNumber >= 0x80) && (BiosInt13ExtensionsSupported(DriveNumber)))
	{
		DbgPrint((DPRINT_DISK, "Using Int 13 Extensions for read. BiosInt13ExtensionsSupported(%d) = %s\n", DriveNumber, BiosInt13ExtensionsSupported(DriveNumber) ? "TRUE" : "FALSE"));

		//
		// LBA is easy, nothing to calculate
		// Just do the read
		//
		if (!BiosInt13ReadExtended(DriveNumber, SectorNumber, SectorCount, Buffer))
		{
			DiskError("Disk read error.");
			return FALSE;
		}
	}
	else
	{
		//
		// Get the drive geometry
		//
		if (!DiskGetDriveGeometry(DriveNumber, &DriveGeometry))
		{
			return FALSE;
		}

		while (SectorCount)
		{

			//
			// Calculate the physical disk offsets
			//
			PhysicalSector = 1 + (SectorNumber % DriveGeometry.Sectors);
			PhysicalHead = (SectorNumber / DriveGeometry.Sectors) % DriveGeometry.Heads;
			PhysicalTrack = (SectorNumber / DriveGeometry.Sectors) / DriveGeometry.Heads;

			//
			// Calculate how many sectors we are supposed to read
			//
			if (PhysicalSector > 1)
			{
				if (SectorCount >= (DriveGeometry.Sectors - (PhysicalSector - 1)))
					NumberOfSectorsToRead = (DriveGeometry.Sectors - (PhysicalSector - 1));
				else
					NumberOfSectorsToRead = SectorCount;
			}
			else
			{
				if (SectorCount >= DriveGeometry.Sectors)
					NumberOfSectorsToRead = DriveGeometry.Sectors;
				else
					NumberOfSectorsToRead = SectorCount;
			}

			DbgPrint((DPRINT_DISK, "Calling BiosInt13Read() with PhysicalHead: %d\n", PhysicalHead));
			DbgPrint((DPRINT_DISK, "Calling BiosInt13Read() with PhysicalTrack: %d\n", PhysicalTrack));
			DbgPrint((DPRINT_DISK, "Calling BiosInt13Read() with PhysicalSector: %d\n", PhysicalSector));
			DbgPrint((DPRINT_DISK, "Calling BiosInt13Read() with NumberOfSectorsToRead: %d\n", NumberOfSectorsToRead));

			//
			// Make sure the read is within the geometry boundaries
			//
			if ((PhysicalHead >= DriveGeometry.Heads) ||
				(PhysicalTrack >= DriveGeometry.Cylinders) ||
				((NumberOfSectorsToRead + PhysicalSector) > (DriveGeometry.Sectors + 1)) ||
				(PhysicalSector > DriveGeometry.Sectors))
			{
				DiskError("Disk read exceeds drive geometry limits.");
				return FALSE;
			}

			//
			// Perform the read
			//
			if (!BiosInt13Read(DriveNumber, PhysicalHead, PhysicalTrack, PhysicalSector, NumberOfSectorsToRead, Buffer))
			{
				DiskError("Disk read error.");
				return FALSE;
			}

			Buffer += (NumberOfSectorsToRead * DriveGeometry.BytesPerSector);
			SectorCount -= NumberOfSectorsToRead;
			SectorNumber += NumberOfSectorsToRead;
		}
	}

	return TRUE;
}
