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


typedef struct
{
	U8		PacketSize;				// 00h - Size of packet (10h or 18h)
	U8		Reserved;				// 01h - Reserved (0)
	U16		LBABlockCount;			// 02h - Number of blocks to transfer (max 007Fh for Phoenix EDD)
	U16		TransferBufferOffset;	// 04h - Transfer buffer offset (seg:off)
	U16		TransferBufferSegment;	//       Transfer buffer segment (seg:off)
	U64		LBAStartBlock;			// 08h - Starting absolute block number
	U64		TransferBuffer64;		// 10h - (EDD-3.0, optional) 64-bit flat address of transfer buffer
									//       used if DWORD at 04h is FFFFh:FFFFh
} PACKED I386_DISK_ADDRESS_PACKET, *PI386_DISK_ADDRESS_PACKET;

/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __i386__

BOOL DiskReadLogicalSectors(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer)
{

	DbgPrint((DPRINT_DISK, "DiskReadLogicalSectors() DriveNumber: 0x%x SectorNumber: %d%d SectorCount: %d Buffer: 0x%x\n", DriveNumber, (U32)SectorNumber, SectorCount, Buffer));

	//
	// Check to see if it is a fixed disk drive
	// If so then check to see if Int13 extensions work
	// If they do then use them, otherwise default back to BIOS calls
	//
	if ((DriveNumber >= 0x80) && DiskInt13ExtensionsSupported(DriveNumber))
	{
		DbgPrint((DPRINT_DISK, "Using Int 13 Extensions for read. DiskInt13ExtensionsSupported(%d) = %s\n", DriveNumber, DiskInt13ExtensionsSupported(DriveNumber) ? "TRUE" : "FALSE"));

		//
		// LBA is easy, nothing to calculate
		// Just do the read
		//
		return DiskReadLogicalSectorsLBA(DriveNumber, SectorNumber, SectorCount, Buffer);
	}
	else
	{
		// LBA is not supported default to the CHS calls
		return DiskReadLogicalSectorsCHS(DriveNumber, SectorNumber, SectorCount, Buffer);
	}

	return TRUE;
}

BOOL DiskReadLogicalSectorsLBA(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer)
{
	REGS						RegsIn;
	REGS						RegsOut;
	U32							RetryCount;
	PI386_DISK_ADDRESS_PACKET	Packet = (PI386_DISK_ADDRESS_PACKET)(BIOSCALLBUFFER);

	DbgPrint((DPRINT_DISK, "DiskReadLogicalSectorsLBA()\n"));

	// BIOS int 0x13, function 42h - IBM/MS INT 13 Extensions - EXTENDED READ
	RegsIn.b.ah = 0x42;					// Subfunction 42h
	RegsIn.b.dl = DriveNumber;			// Drive number in DL (0 - floppy, 0x80 - harddisk)
	RegsIn.x.ds = BIOSCALLBUFSEGMENT;	// DS:SI -> disk address packet
	RegsIn.w.si = BIOSCALLBUFOFFSET;

	// Setup disk address packet
	RtlZeroMemory(Packet, sizeof(I386_DISK_ADDRESS_PACKET));
	Packet->PacketSize = sizeof(I386_DISK_ADDRESS_PACKET);
	Packet->Reserved = 0;
	Packet->LBABlockCount = SectorCount;
	Packet->TransferBufferOffset = ((U32)Buffer) & 0x0F;
	Packet->TransferBufferSegment = ((U32)Buffer) >> 4;
	Packet->LBAStartBlock = SectorNumber;
	Packet->TransferBuffer64 = 0;

	// BIOS int 0x13, function 42h - IBM/MS INT 13 Extensions - EXTENDED READ
	// Return:
	// CF clear if successful
	// AH = 00h
	// CF set on error
	// AH = error code
	// disk address packet's block count field set to the
	// number of blocks successfully transferred

	// Retry 3 times
	for (RetryCount=0; RetryCount<3; RetryCount++)
	{
		Int386(0x13, &RegsIn, &RegsOut);

		// If it worked return TRUE
		if (INT386_SUCCESS(RegsOut))
		{
			return TRUE;
		}
		// If it was a corrected ECC error then the data is still good
		else if (RegsOut.b.ah == 0x11)
		{
			return TRUE;
		}
		// If it failed the do the next retry
		else
		{
			DiskResetController(DriveNumber);

			continue;
		}
	}

	// If we get here then the read failed
	DiskError("Disk Read Failed", RegsOut.b.ah);

	return FALSE;
}

BOOL DiskReadLogicalSectorsCHS(U32 DriveNumber, U64 SectorNumber, U32 SectorCount, PVOID Buffer)
{
	U32			PhysicalSector;
	U32			PhysicalHead;
	U32			PhysicalTrack;
	GEOMETRY	DriveGeometry;
	U32			NumberOfSectorsToRead;
	REGS		RegsIn;
	REGS		RegsOut;
	U32			RetryCount;

	DbgPrint((DPRINT_DISK, "DiskReadLogicalSectorsCHS()\n"));

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
		// Calculate how many sectors we need to read this round
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

		//
		// Make sure the read is within the geometry boundaries
		//
		if ((PhysicalHead >= DriveGeometry.Heads) ||
			(PhysicalTrack >= DriveGeometry.Cylinders) ||
			((NumberOfSectorsToRead + PhysicalSector) > (DriveGeometry.Sectors + 1)) ||
			(PhysicalSector > DriveGeometry.Sectors))
		{
			DiskError("Disk read exceeds drive geometry limits.", 0);
			return FALSE;
		}

		// BIOS Int 13h, function 2 - Read Disk Sectors
		// AH = 02h
		// AL = number of sectors to read (must be nonzero)
		// CH = low eight bits of cylinder number
		// CL = sector number 1-63 (bits 0-5)
		//      high two bits of cylinder (bits 6-7, hard disk only)
		// DH = head number
		// DL = drive number (bit 7 set for hard disk)
		// ES:BX -> data buffer
		// Return:
		// CF set on error
		// if AH = 11h (corrected ECC error), AL = burst length
		// CF clear if successful
		// AH = status
		// AL = number of sectors transferred
		//  (only valid if CF set for some BIOSes)
		RegsIn.b.ah = 0x02;
		RegsIn.b.al = NumberOfSectorsToRead;
		RegsIn.b.ch = (PhysicalTrack & 0xFF);
		RegsIn.b.cl = (PhysicalSector + ((PhysicalTrack & 0x300) >> 2));
		RegsIn.b.dh = PhysicalHead;
		RegsIn.b.dl = DriveNumber;
		RegsIn.w.es = ((U32)Buffer) >> 4;
		RegsIn.w.bx = ((U32)Buffer) & 0x0F;

		//
		// Perform the read
		// Retry 3 times
		//
		for (RetryCount=0; RetryCount<3; RetryCount++)
		{
			Int386(0x13, &RegsIn, &RegsOut);

			// If it worked break out
			if (INT386_SUCCESS(RegsOut))
			{
				break;
			}
			// If it was a corrected ECC error then the data is still good
			else if (RegsOut.b.ah == 0x11)
			{
				break;
			}
			// If it failed the do the next retry
			else
			{
				DiskResetController(DriveNumber);

				continue;
			}
		}

		// If we retried 3 times then fail
		if (RetryCount >= 3)
		{
			DiskError("Disk Read Failed", RegsOut.b.ah);
			return FALSE;
		}

		// I have learned that not all bioses return
		// the sector read count in the AL register (at least mine doesn't)
		// even if the sectors were read correctly. So instead
		// of checking the sector read count we will rely solely
		// on the carry flag being set on error

		Buffer += (NumberOfSectorsToRead * DriveGeometry.BytesPerSector);
		SectorCount -= NumberOfSectorsToRead;
		SectorNumber += NumberOfSectorsToRead;
	}

	return TRUE;
}

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
	RegsIn.b.ah = 0x02;
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

	if (!(RegsOut.w.cx & 0x01))
	{
		// CX = API subset support bitmap
		// Bit 0, extended disk access functions (AH=42h-44h,47h,48h) supported
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
	if (DiskInt13ExtensionsSupported)
	{
		return 64;
	}
	else
	{
		return Geometry.Sectors;
	}
}

#endif // defined __i386__
