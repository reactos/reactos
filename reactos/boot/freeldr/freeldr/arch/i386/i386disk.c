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

#define NDEBUG
#include <debug.h>

/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

BOOLEAN DiskResetController(ULONG DriveNumber)
{
	REGS	RegsIn;
	REGS	RegsOut;

	DPRINTM(DPRINT_DISK, "DiskResetController(0x%x) DISK OPERATION FAILED -- RESETTING CONTROLLER\n", DriveNumber);

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

BOOLEAN DiskInt13ExtensionsSupported(ULONG DriveNumber)
{
	REGS	RegsIn;
	REGS	RegsOut;

	DPRINTM(DPRINT_DISK, "DiskInt13ExtensionsSupported()\n");

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

BOOLEAN DiskGetExtendedDriveParameters(ULONG DriveNumber, PVOID Buffer, USHORT BufferSize)
{
	REGS	RegsIn;
	REGS	RegsOut;
	PUSHORT	Ptr = (PUSHORT)(BIOSCALLBUFFER);

	DPRINTM(DPRINT_DISK, "DiskGetExtendedDriveParameters()\n");

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

    DPRINTM(DPRINT_DISK, "size of buffer:                          %x\n", Ptr[0]);
    DPRINTM(DPRINT_DISK, "information flags:                       %x\n", Ptr[1]);
    DPRINTM(DPRINT_DISK, "number of physical cylinders on drive:   %u\n", *(PULONG)&Ptr[2]);
    DPRINTM(DPRINT_DISK, "number of physical heads on drive:       %u\n", *(PULONG)&Ptr[4]);
    DPRINTM(DPRINT_DISK, "number of physical sectors per track:    %u\n", *(PULONG)&Ptr[6]);
    DPRINTM(DPRINT_DISK, "total number of sectors on drive:        %I64u\n", *(unsigned long long*)&Ptr[8]);
    DPRINTM(DPRINT_DISK, "bytes per sector:                        %u\n", Ptr[12]);
    if (Ptr[0] >= 0x1e)
    {
        DPRINTM(DPRINT_DISK, "EED configuration parameters:            %x:%x\n", Ptr[13], Ptr[14]);
        if (Ptr[13] != 0xffff && Ptr[14] != 0xffff)
        {
           PUCHAR SpecPtr = (PUCHAR)(ULONG_PTR)((Ptr[13] << 4) + Ptr[14]);
           DPRINTM(DPRINT_DISK, "SpecPtr:                                 %x\n", SpecPtr);
           DPRINTM(DPRINT_DISK, "physical I/O port base address:          %x\n", *(PUSHORT)&SpecPtr[0]);
           DPRINTM(DPRINT_DISK, "disk-drive control port address:         %x\n", *(PUSHORT)&SpecPtr[2]);
           DPRINTM(DPRINT_DISK, "drive flags:                             %x\n", SpecPtr[4]);
           DPRINTM(DPRINT_DISK, "proprietary information:                 %x\n", SpecPtr[5]);
           DPRINTM(DPRINT_DISK, "IRQ for drive:                           %u\n", SpecPtr[6]);
           DPRINTM(DPRINT_DISK, "sector count for multi-sector transfers: %u\n", SpecPtr[7]);
           DPRINTM(DPRINT_DISK, "DMA control:                             %x\n", SpecPtr[8]);
           DPRINTM(DPRINT_DISK, "programmed I/O control:                  %x\n", SpecPtr[9]);
           DPRINTM(DPRINT_DISK, "drive options:                           %x\n", *(PUSHORT)&SpecPtr[10]);
        }
    }
    if (Ptr[0] >= 0x42)
    {
        DPRINTM(DPRINT_DISK, "signature:                             %x\n", Ptr[15]);
    }

	return TRUE;
}

/* EOF */
