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
#include <rtl.h>
#include <arch.h>
#include <debug.h>
#include <portio.h>


/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __i386__

BOOL DiskResetController(U32 DriveNumber)
{
	REGS	RegsIn;
	REGS	RegsOut;

	DbgPrint((DPRINT_DISK, "DiskResetController(0x%x) DISK OPERATION FAILED -- RESETTING CONTROLLER\n", DriveNumber));

	// BIOS Int 13h, function 0 - Reset disk system
	// AH = 00h
	// DL = drive (if bit 7 is set both hard disks and floppy disks reset)
	// Return:
	// AH = status
	// CF clear if successful
	// CF set on error
	RegsIn.b.ah = 0x00;
	RegsIn.b.dl = DriveNumber;

	// Reset the disk controller
	Int386(0x13, &RegsIn, &RegsOut);

	return INT386_SUCCESS(RegsOut);
}

BOOL DiskInt13ExtensionsSupported(U32 DriveNumber)
{
	REGS	RegsIn;
	REGS	RegsOut;

	DbgPrint((DPRINT_DISK, "DiskInt13ExtensionsSupported()\n"));

	// IBM/MS INT 13 Extensions - INSTALLATION CHECK
	// AH = 41h
	// BX = 55AAh
	// DL = drive (80h-FFh)
	// Return:
	// CF set on error (extensions not supported)
	// AH = 01h (invalid function)
	// CF clear if successful
	// BX = AA55h if installed
	// AH = major version of extensions
	// 01h = 1.x
	// 20h = 2.0 / EDD-1.0
	// 21h = 2.1 / EDD-1.1
	// 30h = EDD-3.0
	// AL = internal use
	// CX = API subset support bitmap
	// DH = extension version (v2.0+ ??? -- not present in 1.x)
	//
	// Bitfields for IBM/MS INT 13 Extensions API support bitmap
	// Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported
	// Bit 1, removable drive controller functions (AH=45h,46h,48h,49h,INT 15/AH=52h) supported
	// Bit 2, enhanced disk drive (EDD) functions (AH=48h,AH=4Eh) supported
	//        extended drive parameter table is valid
	// Bits 3-15 reserved
	RegsIn.b.ah = 0x41;
	RegsIn.w.bx = 0x55AA;
	RegsIn.b.dl = DriveNumber;

	// Reset the disk controller
	Int386(0x13, &RegsIn, &RegsOut);

	if (!INT386_SUCCESS(RegsOut))
	{
		// CF set on error (extensions not supported)
		return FALSE;
	}

	if (RegsOut.w.bx != 0xAA55)
	{
		// BX = AA55h if installed
		return FALSE;
	}

	// Note:
	// The original check is too strict because some BIOSes report that
	// extended disk access functions are not suported when booting
	// from a CD (e.g. Phoenix BIOS v6.00PG). Argh!
#if 0
	if (!(RegsOut.w.cx & 0x0001))
	{
		// CX = API subset support bitmap
		// Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported
		return FALSE;
	}
#endif

	// Use this relaxed check instead
	if (RegsOut.w.cx == 0x0000)
	{
		// CX = API subset support bitmap
		return FALSE;
	}

	return TRUE;
}

VOID DiskStopFloppyMotor(VOID)
{
	WRITE_PORT_UCHAR((PUCHAR)0x3F2, 0);
}

BOOL DiskGetDriveParameters(U32 DriveNumber, PGEOMETRY Geometry)
{
	REGS	RegsIn;
	REGS	RegsOut;
	U32		Cylinders;

	DbgPrint((DPRINT_DISK, "DiskGetDriveParameters()\n"));

	// BIOS Int 13h, function 08h - Get drive parameters
	// AH = 08h
	// DL = drive (bit 7 set for hard disk)
	// ES:DI = 0000h:0000h to guard against BIOS bugs
	// Return:
	// CF set on error
	// AH = status (07h)
	// CF clear if successful
	// AH = 00h
	// AL = 00h on at least some BIOSes
	// BL = drive type (AT/PS2 floppies only)
	// CH = low eight bits of maximum cylinder number
	// CL = maximum sector number (bits 5-0)
	//      high two bits of maximum cylinder number (bits 7-6)
	// DH = maximum head number
	// DL = number of drives
	// ES:DI -> drive parameter table (floppies only)
	RegsIn.b.ah = 0x08;
	RegsIn.b.dl = DriveNumber;
	RegsIn.w.es = 0x0000;
	RegsIn.w.di = 0x0000;

	// Get drive parameters
	Int386(0x13, &RegsIn, &RegsOut);

	if (!INT386_SUCCESS(RegsOut))
	{
		return FALSE;
	}

	Cylinders = (RegsOut.b.cl & 0xC0) << 2;
	Cylinders += RegsOut.b.ch;
	Cylinders++;
	Geometry->Cylinders = Cylinders;
	Geometry->Heads = RegsOut.b.dh + 1;
	Geometry->Sectors = RegsOut.b.cl & 0x3F;
	Geometry->BytesPerSector = 512;				// Just assume 512 bytes per sector

	return TRUE;
}

BOOL DiskGetExtendedDriveParameters(U32 DriveNumber, PVOID Buffer, U16 BufferSize)
{
	REGS	RegsIn;
	REGS	RegsOut;
	PU16	Ptr = (PU16)(BIOSCALLBUFFER);

	DbgPrint((DPRINT_DISK, "DiskGetExtendedDriveParameters()\n"));

	// Initialize transfer buffer
	*Ptr = BufferSize;

	// BIOS Int 13h, function 48h - Get drive parameters
	// AH = 48h
	// DL = drive (bit 7 set for hard disk)
	// DS:SI = result buffer
	// Return:
	// CF set on error
	// AH = status (07h)
	// CF clear if successful
	// AH = 00h
	// DS:SI -> result buffer
	RegsIn.b.ah = 0x48;
	RegsIn.b.dl = DriveNumber;
	RegsIn.x.ds = BIOSCALLBUFSEGMENT;	// DS:SI -> result buffer
	RegsIn.w.si = BIOSCALLBUFOFFSET;

	// Get drive parameters
	Int386(0x13, &RegsIn, &RegsOut);

	if (!INT386_SUCCESS(RegsOut))
	{
		return FALSE;
	}

	memcpy(Buffer, Ptr, BufferSize);

	return TRUE;
}



U32 DiskGetCacheableBlockCount(U32 DriveNumber)
{
	GEOMETRY	Geometry;

	// Get the disk geometry
	// If this fails then we will just return 1 sector to be safe
	if (!DiskGetDriveParameters(DriveNumber, &Geometry))
	{
		return 1;
	}

	// If LBA is supported then the block size will be 64 sectors (32k)
	// If not then the block size is the size of one track
	if (DiskInt13ExtensionsSupported(DriveNumber))
	{
		return 64;
	}
	else
	{
		return Geometry.Sectors;
	}
}

#endif // defined __i386__
