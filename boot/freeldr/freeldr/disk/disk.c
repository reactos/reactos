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
#include <debug.h>

#undef  UNIMPLEMENTED
#define UNIMPLEMENTED   BugCheck((DPRINT_WARNING, "Unimplemented\n"));

static BOOLEAN bReportError = TRUE;

/////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
/////////////////////////////////////////////////////////////////////////////////////////////

VOID DiskReportError (BOOLEAN bError)
{
	bReportError = bError;
}

VOID DiskError(PCSTR ErrorString, ULONG ErrorCode)
{
	CHAR	ErrorCodeString[200];

	if (bReportError == FALSE)
		return;

	sprintf(ErrorCodeString, "%s\n\nError Code: 0x%lx\nError: %s", ErrorString, ErrorCode, DiskGetErrorCodeString(ErrorCode));

	DPRINTM(DPRINT_DISK, "%s\n", ErrorCodeString);

	UiMessageBox(ErrorCodeString);
}

PCSTR DiskGetErrorCodeString(ULONG ErrorCode)
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
//BOOLEAN DiskReadLogicalSectors(ULONG DriveNumber, U64 SectorNumber, ULONG SectorCount, PVOID Buffer)

BOOLEAN DiskIsDriveRemovable(ULONG DriveNumber)
{
	// Hard disks use drive numbers >= 0x80
	// So if the drive number indicates a hard disk
	// then return FALSE
    // 0x49 is our magic ramdisk drive, so return FALSE for that too
	if ((DriveNumber >= 0x80) || (DriveNumber == 0x49))
	{
		return FALSE;
	}

	// Drive is a floppy diskette so return TRUE
	return TRUE;
}

BOOLEAN DiskGetBootVolume(PULONG DriveNumber, PULONGLONG StartSector, PULONGLONG SectorCount, int *FsType)
{
	PARTITION_TABLE_ENTRY	PartitionTableEntry;
	UCHAR			VolumeType;
	ULONG			ActivePartition;
    
	DPRINTM(DPRINT_FILESYSTEM, "FsOpenVolume() DriveNumber: 0x%x PartitionNumber: 0x%x\n", BootDrive, BootPartition);
    
	// Check and see if it is a floppy drive
	// If so then just assume FAT12 file system type
	if (DiskIsDriveRemovable(BootDrive))
	{
		DPRINTM(DPRINT_FILESYSTEM, "Drive is a floppy diskette drive. Assuming FAT12 file system.\n");
        
		*DriveNumber = BootDrive;
		*StartSector = 0;
		*SectorCount = 2 * 80 * 18; /* FIXME hardcoded for 1.44 Mb */
		*FsType = FS_FAT;
		return TRUE;
	}
    
	// Check for ISO9660 file system type
	if (BootDrive >= 0x80 && FsRecIsIso9660(BootDrive))
	{
		DPRINTM(DPRINT_FILESYSTEM, "Drive is a cdrom drive. Assuming ISO-9660 file system.\n");
        
		*DriveNumber = BootDrive;
		*StartSector = 0;
		*SectorCount = 0;
		*FsType = FS_ISO9660;
		return TRUE;
	}
    
	// Get the requested partition entry
	if (BootPartition == 0)
	{
		// Partition requested was zero which means the boot partition
		if (! DiskGetActivePartitionEntry(BootDrive, &PartitionTableEntry, &ActivePartition))
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
	else if (0xff == BootPartition)
	{
		/* Partition-less disk */
		*StartSector = 0;
		*SectorCount = 0;
	}
	else
	{
		// Get requested partition
		if (! MachDiskGetPartitionEntry(BootDrive, BootPartition, &PartitionTableEntry))
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
	if (!FsRecognizeVolume(BootDrive, *StartSector, &VolumeType))
	{
		return FALSE;
	}
    
	*DriveNumber = BootDrive;
    
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
DiskGetBootDevice(PULONG BootDevice)
{
    ((char *)BootDevice)[0] = (char)BootDrive;
    ((char *)BootDevice)[1] = (char)BootPartition;
}

BOOLEAN
DiskBootingFromFloppy(VOID)
{
    return BootDrive < 0x80;
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

BOOLEAN DiskGetSystemVolume(char *SystemPath,
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
		DPRINTM(DPRINT_FILESYSTEM, "Drive is a cdrom drive. Assuming ISO-9660 file system.\n");
        
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
DiskGetBootPath(char *BootPath, unsigned Size)
{
	static char Path[] = "multi(0)disk(0)";
	char Device[4];
    
	_itoa(BootDrive, Device, 10);
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
DiskNormalizeSystemPath(char *SystemPath, unsigned Size)
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


// This function is in arch/i386/i386disk.c
//VOID DiskStopFloppyMotor(VOID)

// This function is in arch/i386/i386disk.c
//ULONG DiskGetCacheableBlockCount(ULONG DriveNumber)
