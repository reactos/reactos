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
#include <rtl.h>
#include <mm.h>


typedef struct
{
	LIST_ITEM	ListEntry;

	ULONG		DriveNumber;
	GEOMETRY	DriveGeometry;

} DRIVE_GEOMETRY, *PDRIVE_GEOMETRY;


PDRIVE_GEOMETRY	DriveGeometryListHead = NULL;


BOOL DiskGetDriveGeometry(ULONG DriveNumber, PGEOMETRY DriveGeometry)
{
	PDRIVE_GEOMETRY	DriveGeometryListEntry;

	//
	// Search the drive geometry list for the requested drive
	//
	DriveGeometryListEntry = DriveGeometryListHead;
	while (DriveGeometryListEntry != NULL)
	{
		//
		// Check to see if this is the drive we want
		//
		if (DriveGeometryListEntry->DriveNumber == DriveNumber)
		{
			//
			// Yep - return the information
			//
			RtlCopyMemory(DriveGeometry, &DriveGeometryListEntry->DriveGeometry, sizeof(GEOMETRY));
			return TRUE;
		}

		//
		// Nope, get next item
		//
		DriveGeometryListEntry = (PDRIVE_GEOMETRY)RtlListGetNext((PLIST_ITEM)DriveGeometryListEntry);
	}

	DiskError("Drive geometry unknown.");
	return FALSE;
}

BOOL DiskSetDriveGeometry(ULONG DriveNumber, ULONG Cylinders, ULONG Heads, ULONG Sectors, ULONG BytesPerSector)
{
	PDRIVE_GEOMETRY	DriveGeometryListEntry;

	//
	// Search the drive geometry list for the requested drive
	//
	DriveGeometryListEntry = DriveGeometryListHead;
	while (DriveGeometryListEntry != NULL)
	{
		//
		// Check to see if this is the drive
		//
		if (DriveGeometryListEntry->DriveNumber == DriveNumber)
		{
			//
			// Yes, we already have this drive's geometry information
			// so just return
			//
			return TRUE;
		}

		//
		// Nope, get next item
		//
		DriveGeometryListEntry = (PDRIVE_GEOMETRY)RtlListGetNext((PLIST_ITEM)DriveGeometryListEntry);
	}

	//
	// If we get here then this is a new drive and we have
	// to add it's information to our list
	//
	DriveGeometryListEntry = (PDRIVE_GEOMETRY)AllocateMemory(sizeof(DRIVE_GEOMETRY));
	if (DriveGeometryListEntry == NULL)
	{
		return FALSE;
	}

	RtlZeroMemory(DriveGeometryListEntry, sizeof(DRIVE_GEOMETRY));
	DriveGeometryListEntry->DriveNumber = DriveNumber;
	DriveGeometryListEntry->DriveGeometry.Cylinders = Cylinders;
	DriveGeometryListEntry->DriveGeometry.Heads = Heads;
	DriveGeometryListEntry->DriveGeometry.Sectors = Sectors;
	DriveGeometryListEntry->DriveGeometry.BytesPerSector = BytesPerSector;

	if (DriveGeometryListHead == NULL)
	{
		DriveGeometryListHead = DriveGeometryListEntry;
	}
	else
	{
		RtlListInsertTail((PLIST_ITEM)DriveGeometryListHead, (PLIST_ITEM)DriveGeometryListEntry);
	}

	return TRUE;
}
