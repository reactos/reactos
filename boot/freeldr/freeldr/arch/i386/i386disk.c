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

#ifdef __i386__

BOOLEAN DiskResetController(ULONG DriveNumber)
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

BOOLEAN DiskInt13ExtensionsSupported(ULONG DriveNumber)
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

BOOLEAN DiskGetExtendedDriveParameters(ULONG DriveNumber, PVOID Buffer, USHORT BufferSize)
{
	REGS	RegsIn;
	REGS	RegsOut;
	PUSHORT	Ptr = (PUSHORT)(BIOSCALLBUFFER);

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

    DbgPrint((DPRINT_DISK, "size of buffer:                          %x\n", Ptr[0]));
    DbgPrint((DPRINT_DISK, "information flags:                       %x\n", Ptr[1]));
    DbgPrint((DPRINT_DISK, "number of physical cylinders on drive:   %u\n", *(PULONG)&Ptr[2]));
    DbgPrint((DPRINT_DISK, "number of physical heads on drive:       %u\n", *(PULONG)&Ptr[4]));
    DbgPrint((DPRINT_DISK, "number of physical sectors per track:    %u\n", *(PULONG)&Ptr[6]));
    DbgPrint((DPRINT_DISK, "total number of sectors on drive:        %I64u\n", *(unsigned long long*)&Ptr[8]));
    DbgPrint((DPRINT_DISK, "bytes per sector:                        %u\n", Ptr[12]));
    if (Ptr[0] >= 0x1e)
    {
        DbgPrint((DPRINT_DISK, "EED configuration parameters:            %x:%x\n", Ptr[13], Ptr[14]));
        if (Ptr[13] != 0xffff && Ptr[14] != 0xffff)
        {
           PUCHAR SpecPtr = (PUCHAR)((Ptr[13] << 4) + Ptr[14]);
           DbgPrint((DPRINT_DISK, "SpecPtr:                                 %x\n", SpecPtr));
           DbgPrint((DPRINT_DISK, "physical I/O port base address:          %x\n", *(PUSHORT)&SpecPtr[0]));
           DbgPrint((DPRINT_DISK, "disk-drive control port address:         %x\n", *(PUSHORT)&SpecPtr[2]));
           DbgPrint((DPRINT_DISK, "drive flags:                             %x\n", SpecPtr[4]));
           DbgPrint((DPRINT_DISK, "proprietary information:                 %x\n", SpecPtr[5]));
           DbgPrint((DPRINT_DISK, "IRQ for drive:                           %u\n", SpecPtr[6]));
           DbgPrint((DPRINT_DISK, "sector count for multi-sector transfers: %u\n", SpecPtr[7]));
           DbgPrint((DPRINT_DISK, "DMA control:                             %x\n", SpecPtr[8]));
           DbgPrint((DPRINT_DISK, "programmed I/O control:                  %x\n", SpecPtr[9]));
           DbgPrint((DPRINT_DISK, "drive options:                           %x\n", *(PUSHORT)&SpecPtr[10]));
        }
    }
    if (Ptr[0] >= 0x42)
    {
        DbgPrint((DPRINT_DISK, "signature:                             %x\n", Ptr[15]));
    }

	return TRUE;
}

BOOLEAN i386DiskGetBootVolume(PULONG DriveNumber, PULONGLONG StartSector, PULONGLONG SectorCount, int *FsType)
{
	PARTITION_TABLE_ENTRY	PartitionTableEntry;
	UCHAR			VolumeType;
	ULONG			ActivePartition;

	DbgPrint((DPRINT_FILESYSTEM, "FsOpenVolume() DriveNumber: 0x%x PartitionNumber: 0x%x\n", i386BootDrive, i386BootPartition));

	// Check and see if it is a floppy drive
	// If so then just assume FAT12 file system type
	if (DiskIsDriveRemovable(i386BootDrive))
	{
		DbgPrint((DPRINT_FILESYSTEM, "Drive is a floppy diskette drive. Assuming FAT12 file system.\n"));

		*DriveNumber = i386BootDrive;
		*StartSector = 0;
		*SectorCount = 2 * 80 * 18; /* FIXME hardcoded for 1.44 Mb */
		*FsType = FS_FAT;
		return TRUE;
	}

	// Check for ISO9660 file system type
	if (i386BootDrive >= 0x80 && FsRecIsIso9660(i386BootDrive))
	{
		DbgPrint((DPRINT_FILESYSTEM, "Drive is a cdrom drive. Assuming ISO-9660 file system.\n"));

		*DriveNumber = i386BootDrive;
		*StartSector = 0;
		*SectorCount = 0;
		*FsType = FS_ISO9660;
		return TRUE;
	}

	// Get the requested partition entry
	if (i386BootPartition == 0)
	{
		// Partition requested was zero which means the boot partition
		if (! DiskGetActivePartitionEntry(i386BootDrive, &PartitionTableEntry, &ActivePartition))
		{
			/* Try partition-less disk */
			*StartSector = 0;
			*SectorCount = 0;
		}
		/* Check for valid partition */
		else if (PartitionTableEntry.SystemIndicator == PARTITION_ENTRY_UNUSED)
		{
			return FALSE;
		}
		else
		{
			*StartSector = PartitionTableEntry.SectorCountBeforePartition;
			*SectorCount = PartitionTableEntry.PartitionSectorCount;
		}
	}
	else if (0xff == i386BootPartition)
	{
		/* Partition-less disk */
		*StartSector = 0;
		*SectorCount = 0;
	}
	else
	{
		// Get requested partition
		if (! MachDiskGetPartitionEntry(i386BootDrive, i386BootPartition, &PartitionTableEntry))
		{
			return FALSE;
		}
		/* Check for valid partition */
		else if (PartitionTableEntry.SystemIndicator == PARTITION_ENTRY_UNUSED)
		{
			return FALSE;
		}
		else
		{
			*StartSector = PartitionTableEntry.SectorCountBeforePartition;
			*SectorCount = PartitionTableEntry.PartitionSectorCount;
		}
	}

	// Try to recognize the file system
	if (!FsRecognizeVolume(i386BootDrive, *StartSector, &VolumeType))
	{
		return FALSE;
	}

	*DriveNumber = i386BootDrive;

	switch (VolumeType)
	{
	case PARTITION_FAT_12:
	case PARTITION_FAT_16:
	case PARTITION_HUGE:
	case PARTITION_XINT13:
	case PARTITION_FAT32:
	case PARTITION_FAT32_XINT13:
		*FsType = FS_FAT;
		return TRUE;
	case PARTITION_EXT2:
		*FsType = FS_EXT2;
		return TRUE;
	case PARTITION_NTFS:
		*FsType = FS_NTFS;
		return TRUE;
	default:
		*FsType = 0;
		return FALSE;
	}

	return TRUE;
}

VOID
i386DiskGetBootDevice(PULONG BootDevice)
{
  ((char *)BootDevice)[0] = (char)i386BootDrive;
  ((char *)BootDevice)[1] = (char)i386BootPartition;
}

BOOLEAN
i386DiskBootingFromFloppy(VOID)
{
  return i386BootDrive < 0x80;
}

#define IsRecognizedPartition(P)  \
    ((P) == PARTITION_FAT_12       || \
     (P) == PARTITION_FAT_16       || \
     (P) == PARTITION_HUGE         || \
     (P) == PARTITION_IFS          || \
     (P) == PARTITION_EXT2         || \
     (P) == PARTITION_FAT32        || \
     (P) == PARTITION_FAT32_XINT13 || \
     (P) == PARTITION_XINT13)

#define IsContainerPartition(P) \
    ((P) == PARTITION_EXTENDED         || \
     (P) == PARTITION_XINT13_EXTENDED)

BOOLEAN i386DiskGetSystemVolume(char *SystemPath,
                             char *RemainingPath,
                             PULONG Device,
                             PULONG DriveNumber,
                             PULONGLONG StartSector,
                             PULONGLONG SectorCount,
                             int *FsType)
{
	ULONG PartitionNumber;
	PARTITION_TABLE_ENTRY PartitionTableEntry;
	UCHAR VolumeType;
	CHAR BootPath[256];
	unsigned i, RosPartition;

	/*
	 * Verify system path
	 */
	if (!DissectArcPath(SystemPath, BootPath, DriveNumber, &PartitionNumber))
	{
		return FALSE;
	}
	if (NULL != RemainingPath)
	{
		strcpy(RemainingPath, BootPath);
	}

	/* 0xff -> no partition table present, use whole device */
	if (0xff == PartitionNumber)
	{
		PartitionTableEntry.SectorCountBeforePartition = 0;
		i = 0xff;
	}
	else
	{
		/* recalculate the boot partition for freeldr */
		i = 0;
		RosPartition = 0;
		while (1)
		{
			if (!MachDiskGetPartitionEntry(*DriveNumber, ++i, &PartitionTableEntry))
			{
				return FALSE;
			}
                        if (!IsContainerPartition(PartitionTableEntry.SystemIndicator) &&
                            PartitionTableEntry.SystemIndicator != PARTITION_ENTRY_UNUSED)
			{
				if (++RosPartition == PartitionNumber)
				{
					break;
				}
			}
		}
	}

	/* Check for ISO9660 file system type */
	if (*DriveNumber >= 0x80 && FsRecIsIso9660(*DriveNumber))
	{
		DbgPrint((DPRINT_FILESYSTEM, "Drive is a cdrom drive. Assuming ISO-9660 file system.\n"));

		if (NULL != Device)
		{
			((char *)Device)[0] = (char)(*DriveNumber);
			((char *)Device)[1] = (char)i;
		}
		*StartSector = 0;
		*SectorCount = 0;
		*FsType = FS_ISO9660;
		return TRUE;
	}

	if (!FsRecognizeVolume(*DriveNumber, PartitionTableEntry.SectorCountBeforePartition, &VolumeType))
	{
		return FALSE;
	}

	if (NULL != Device)
	{
		((char *)Device)[0] = (char)(*DriveNumber);
		((char *)Device)[1] = (char)i;
	}
	*StartSector = PartitionTableEntry.SectorCountBeforePartition;
	*SectorCount = PartitionTableEntry.PartitionSectorCount;

	switch (VolumeType)
	{
	case PARTITION_FAT_12:
	case PARTITION_FAT_16:
	case PARTITION_HUGE:
	case PARTITION_XINT13:
	case PARTITION_FAT32:
	case PARTITION_FAT32_XINT13:
		*FsType = FS_FAT;
		return TRUE;
	case PARTITION_EXT2:
		*FsType = FS_EXT2;
		return TRUE;
	case PARTITION_NTFS:
		*FsType = FS_NTFS;
		return TRUE;
	default:
		*FsType = 0;
		return FALSE;
	}

	return FALSE;
}

BOOLEAN
i386DiskGetBootPath(char *BootPath, unsigned Size)
{
	static char Path[] = "multi(0)disk(0)";
	char Device[4];

	_itoa(i386BootDrive, Device, 10);
	if (Size <= sizeof(Path) + 6 + strlen(Device))
	{
		return FALSE;
	}
	strcpy(BootPath, Path);
	strcat(BootPath, MachDiskBootingFromFloppy() ? "fdisk" : "cdrom");
	strcat(strcat(strcat(BootPath, "("), Device), ")");

	return TRUE;
}

BOOLEAN
i386DiskNormalizeSystemPath(char *SystemPath, unsigned Size)
{
	CHAR BootPath[256];
	ULONG PartitionNumber;
	ULONG DriveNumber;
	PARTITION_TABLE_ENTRY PartEntry;
	char *p;

	if (!DissectArcPath(SystemPath, BootPath, &DriveNumber, &PartitionNumber))
	{
		return FALSE;
	}

	if (0 != PartitionNumber)
	{
		return TRUE;
	}

	if (! DiskGetActivePartitionEntry(DriveNumber,
	                                  &PartEntry,
	                                  &PartitionNumber) ||
	    PartitionNumber < 1 || 9 < PartitionNumber)
	{
		return FALSE;
	}

	p = SystemPath;
	while ('\0' != *p && 0 != _strnicmp(p, "partition(", 10)) {
		p++;
	}
	p = strchr(p, ')');
	if (NULL == p || '0' != *(p - 1)) {
		return FALSE;
	}
	*(p - 1) = '0' + PartitionNumber;

	return TRUE;
}

#endif /* defined __i386__ */

/* EOF */
